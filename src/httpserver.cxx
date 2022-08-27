#include "httpserver.h"

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>

#include "httpmessage.h"

namespace SimpleHttpServer{
    HttpServer::HttpServer(const std::string &host, std::uint16_t port)
        : _host(host),
          _port(port),
          _sockfd(0),
          _running(false),
          worker_epoll_fd(){
            InitSocket();
          }


    void HttpServer::InitSocket(){
        if((_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0){
            throw std::runtime_error("Failed to create TCP socket");
        }
        int opt = 1;
        if(setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0){
            throw std::runtime_error("Failed to set socket options");
        }
        struct sockaddr_in server_addr; 
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(_port);

        if (bind(_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            throw std::runtime_error("Failed to bind to socket");
        }
        if (listen(_sockfd, kBacklogSize) < 0) {
            throw std::runtime_error("Failed to listen on port " + std::to_string(_port));
        }
    }

    void HttpServer::Run(){
        for(int i = 0; i < kThreadPoolSize; ++i){
            if((worker_epoll_fd[i] = epoll_create1(0)) < 0){
                throw std::runtime_error("Failed to create epoll file descriptor");
            }
        }
        _running = true;
        // start thread to listen client request
        listener_thread= std::thread(&HttpServer::Listen, this);
        // start thread pool to handle distribute task
        for(int i = 0; i < kThreadPoolSize; ++i){
            worker_threads[i] = std::thread(&HttpServer::ProcessEvent, this, i);
        }
    }

    void HttpServer::Stop() {
        _running = false;
        listener_thread.join();
        for (int i = 0; i < kThreadPoolSize; i++) {
            worker_threads[i].join();
        }
        for (int i = 0; i < kThreadPoolSize; i++) {
            close(worker_epoll_fd[i]);
        }
        close(_sockfd);
    }

    void HttpServer::Listen(){
        sockaddr_in client_addr;
        socklen_t client_size = sizeof(client_addr);
        int clientfd;
        int current_worker = 0;
        while (_running)
        {
            clientfd = accept4(_sockfd, (struct sockaddr*)&client_addr, &client_size, SOCK_NONBLOCK);
            if(clientfd < 0)
                continue;

            // add entry to the interest list
            EventData *client_data = new EventData();
            client_data->fd = clientfd;
            control_epoll_event(worker_epoll_fd[current_worker], EPOLL_CTL_ADD,
                        clientfd, EPOLLIN, client_data);
            current_worker++;
            if (current_worker == HttpServer::kThreadPoolSize) current_worker = 0;
        }
    }

    void HttpServer::ProcessEvent(int workerId){
        int epollfd =  worker_epoll_fd[workerId];
        EventData *data;
        int timeout_ms = 200;
        while (_running)
        {
            int readyFd = epoll_wait(worker_epoll_fd[workerId], worker_events[workerId], HttpServer::kMaxEvents, timeout_ms);
            if(readyFd < 0)
                continue;
            
            for(int i = 0; i < readyFd; ++i){
                const epoll_event current_event = worker_events[workerId][i];
                data = reinterpret_cast<EventData *>(current_event.data.ptr);
                if(current_event.events == EPOLLHUP || current_event.events == EPOLLERR){
                    control_epoll_event(epollfd, EPOLL_CTL_DEL, data->fd);
                    close(data->fd);
                    delete data;
                }else if (current_event.events == EPOLLIN || current_event.events == EPOLLOUT)
                {
                    HandleEvent(epollfd, &current_event);
                }else{
                    control_epoll_event(epollfd, EPOLL_CTL_DEL, data->fd);
                    close(data->fd);
                    delete data;
                }
                
            }
        }
    }

    void HttpServer::HandleEvent(int epollfd, const epoll_event *event){
        EventData *data = reinterpret_cast<EventData*>(event->data.ptr);
        int fd = data->fd;
        EventData *request, *response;

        if(event->events == EPOLLIN){
            request = data;
            ssize_t byte_count = recv(fd, request->buffer, kMaxBufferSize, 0);
            if(byte_count > 0){
                response = new EventData();
                response->fd = fd;
                // generate http response data
                HandleHttpRequest(*request, response);
                control_epoll_event(epollfd, EPOLL_CTL_MOD, fd, EPOLLOUT, response);
                delete request;
            }else if(byte_count == 0){ // client has closed connection
                control_epoll_event(epollfd, EPOLL_CTL_DEL, fd);
                close(fd);
                delete request;
            }else{
                if (errno == EAGAIN || errno == EWOULDBLOCK) {  // retry
                    request->fd = fd;
                    control_epoll_event(epollfd, EPOLL_CTL_MOD, fd, EPOLLIN, request);
                } else {  // other error
                    control_epoll_event(epollfd, EPOLL_CTL_DEL, fd);
                    close(fd);
                    delete request;
                }
            }
        }else {
            response = data;
            ssize_t byte_count = send(fd, response->buffer + response->cursor, response->length, 0);
            if (byte_count >= 0) {
                if (byte_count < response->length) {  // there are still bytes to write
                    response->cursor += byte_count;
                    response->length -= byte_count;
                    control_epoll_event(epollfd, EPOLL_CTL_MOD, fd, EPOLLOUT, response);
                } else {  // we have written the complete message
                    request = new EventData();
                    request->fd = fd;
                    control_epoll_event(epollfd, EPOLL_CTL_MOD, fd, EPOLLIN, request);
                    delete response;
                }
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {  // retry
                    control_epoll_event(epollfd, EPOLL_CTL_ADD, fd, EPOLLOUT, response);
                } else {  // other error
                    control_epoll_event(epollfd, EPOLL_CTL_DEL, fd);
                    close(fd);
                    delete response;
                }
            }
        }
    }

    void HttpServer::HandleHttpRequest(const EventData& request, EventData* response){
        std::string request_str(request.buffer);
        std::string response_str;
        HttpRequest httpRequest;
        HttpResponse httpResponse;

        try{
            httpRequest = string_to_request(request_str);
            auto it = request_handlers.find(httpRequest.uri());
            if(it == request_handlers.end()){
                httpResponse = HttpResponse(HttpStatusCode::NotFound);
            }else{
                auto callback = it->second.find(httpRequest.method());
                if(callback == it->second.end()){
                    httpResponse = HttpResponse(HttpStatusCode::MethodNotAllowed);
                }else{
                    httpResponse = callback->second(httpRequest);
                }
            }
        }catch (const std::invalid_argument &e){
            httpResponse = HttpResponse(HttpStatusCode::BadRequest);
            httpResponse.setContent(e.what());
        }catch (const std::logic_error &e){
            httpResponse = HttpResponse(HttpStatusCode::HttpVersionNotSupported);
            httpResponse.setContent(e.what());
        }catch (const std::exception &e){
            httpResponse = HttpResponse(HttpStatusCode::InternalServerError);
            httpResponse.setContent(e.what());
        }

        response_str = to_string(httpResponse, httpRequest.method() != HttpMethod::HEAD);
        memcpy(response->buffer, response_str.c_str(), kMaxBufferSize);
        response->length = response_str.length();
    }

    void HttpServer::control_epoll_event(int epoll_fd, int op, int fd,
                                     std::uint32_t events, void *data) {
        if (op == EPOLL_CTL_DEL) {
            if (epoll_ctl(epoll_fd, op, fd, nullptr) < 0) {
                throw std::runtime_error("Failed to remove file descriptor");
            }
        } else {
            epoll_event ev;
            ev.events = events;
            ev.data.ptr = data;
            if (epoll_ctl(epoll_fd, op, fd, &ev) < 0) {
                throw std::runtime_error("Failed to add file descriptor");
            }
        }
    }
}   