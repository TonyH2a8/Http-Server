#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <thread>
#include <utility>

#include "httpmessage.h"

namespace SimpleHttpServer
{

    constexpr size_t kMaxBufferSize = 4096;

    struct EventData{
        EventData() : fd(0), length(0),  cursor(0), buffer() {} 
        int fd; 
        size_t length;
        size_t cursor;
        char buffer[kMaxBufferSize];
    };

    using HttpRequestHandler = std::function<HttpResponse(const HttpRequest&)>;

    class HttpServer
    {
    public:
        explicit HttpServer(const std::string& host, std::uint16_t port);
        ~HttpServer() = default;

        HttpServer() = default;
        HttpServer(HttpServer&&) = default;
        HttpServer& operator=(HttpServer&&) = default;

        void Run();
        void Stop();

        void RegisterHttpRequestHandler(const std::string& uri, HttpMethod method,
                                        const HttpRequestHandler callback) {
            request_handlers[uri].insert(std::make_pair(method, std::move(callback)));
        }

        std::string host() const {return _host;}
        std::uint16_t port() const {return _port;}
        bool running() const {return _running;}
    private:
        static constexpr int kThreadPoolSize = 5;
        static constexpr int kBacklogSize = 1000;
        static constexpr int kMaxConnections = 10000;
        static constexpr int kMaxEvents = 10000;

        std::string _host;
        std::uint16_t _port;
        int _sockfd;
        bool _running;
        std::thread listener_thread;
        std::thread worker_threads[kThreadPoolSize];
        int worker_epoll_fd[kThreadPoolSize];
        epoll_event worker_events[kThreadPoolSize][kMaxEvents];
        std::map<std::string, std::map<HttpMethod, HttpRequestHandler>> request_handlers;

        void InitSocket();
        void Listen();
        void ProcessEvent(int workerId);
        void HandleEvent(int epollfd, const epoll_event *event);
        void HandleHttpRequest(const EventData& request, EventData* response);
        void control_epoll_event(int epoll_fd, int op, int fd,
                                     std::uint32_t events = 0, void *data = nullptr);
    };
    
} // namespace HttpServer

#endif //HTTP_SERVER_H