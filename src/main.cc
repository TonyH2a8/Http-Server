#include <sys/time.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <stdexcept>

#include "httpmessage.h"
#include "httpserver.h"

using namespace SimpleHttpServer;

int main(void)
{
    // Setup server host and port
    std::string host = "127.0.0.1";
    int port = 8080;
    HttpServer server(host, port);

    // Register a few endpoints for demo and benchmarking
    auto say_hello = [](const HttpRequest& request) -> HttpResponse {
        HttpResponse response(HttpStatusCode::Ok);
        response.setHeader("Content-Type", "text/plain");
        response.setContent("Hello, world\n");
        return response;
    };
    auto send_html = [](const HttpRequest& request) -> HttpResponse {
        HttpResponse response(HttpStatusCode::Ok);
        std::string content;
        content += "<!doctype html>\n";
        content += "<html>\n<body>\n\n";
        content += "<h1>Hello, world in an Html page</h1>\n";
        content += "<p>A Paragraph</p>\n\n";
        content += "</body>\n</html>\n";

        response.setHeader("Content-Type", "text/html");
        response.setContent(content);
        return response;
    };

    server.RegisterHttpRequestHandler("/", HttpMethod::HEAD, say_hello);
    server.RegisterHttpRequestHandler("/", HttpMethod::GET, say_hello);
    server.RegisterHttpRequestHandler("/hello.html", HttpMethod::HEAD, send_html);
    server.RegisterHttpRequestHandler("/hello.html", HttpMethod::GET, send_html);

    try{
        std::cout << "Starting the web server.." << std::endl;
        server.Run();
        std::cout << "Server listening on " << host << ":" << port << std::endl;

        std::cout << "Enter [quit] to stop the server" << std::endl;
        std::string command;
        while (std::cin >> command, command != "quit")
        ;
        std::cout << "'quit' command entered. Stopping the web server.."
                << std::endl;
        server.Stop();
        std::cout << "Server stopped" << std::endl;
    } catch (std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}