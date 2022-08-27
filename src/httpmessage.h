#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H

#include <string>
#include <map>

namespace SimpleHttpServer
{
    enum class HttpMethod{
        GET,
        HEAD,
        POST,
        PUT,
        DELETE,
        CONNECT,
        OPTIONS,
        TRACE,
        PATCH
    };
    enum class HttpVersion{
        HTTP_0_9,
        HTTP_1_0,
        HTTP_1_1,
        HTTP_2_0
    };
    enum class HttpStatusCode {
        Continue = 100,
        SwitchingProtocols = 101,
        EarlyHints = 103,
        Ok = 200,
        Created = 201,
        Accepted = 202,
        NonAuthoritativeInformation = 203,
        NoContent = 204,
        ResetContent = 205,
        PartialContent = 206,
        MultipleChoices = 300,
        MovedPermanently = 301,
        Found = 302,
        NotModified = 304,
        BadRequest = 400,
        Unauthorized = 401,
        Forbidden = 403,
        NotFound = 404,
        MethodNotAllowed = 405,
        RequestTimeout = 408,
        ImATeapot = 418,
        InternalServerError = 500,
        NotImplemented = 501,
        BadGateway = 502,
        ServiceUnvailable = 503,
        GatewayTimeout = 504,
        HttpVersionNotSupported = 505
    };

    class HttpMessageInterface{
        public:
            HttpMessageInterface() : _version(HttpVersion::HTTP_1_1) {}
            virtual ~HttpMessageInterface() = default;

            HttpVersion version() const { return _version;}

            std::string content() const { return _content;}
            std::size_t content_length() const { return _content.length();}
            void setContent(const std::string& content){
                _content = std::move(content);
                setContentLength();
            }

            std::string header(const std::string& key) const {
                if(_headers.count(key) > 0)
                    return _headers.at(key);
                return std::string();
            }
            std::map<std::string, std::string> headers() const { return _headers; }

            void setHeader(const std::string& header, const std::string& detail){
                _headers[header] = std::move(detail);
            }
            
        protected:
            HttpVersion _version;
            std::string _content;
            std::map<std::string, std::string> _headers;

            void setContentLength(){
                setHeader("Content-Length", std::to_string(_content.length()));
            }
    };

    class HttpRequest : public HttpMessageInterface{
        public:
            HttpRequest() : _method(HttpMethod::GET){}
            ~HttpRequest() = default;

            HttpMethod method() const { return _method; }
            std::string uri() const { return _Uri;} 

            void setUri(const std::string& uri) { _Uri = std::move(uri);}
            void setMethod(HttpMethod method) { _method = method;}
      
        private:
            HttpMethod _method;
            std::string _Uri;
    };

    class HttpResponse : public HttpMessageInterface{
        public:
            HttpResponse() : _status(HttpStatusCode::Ok){}
            HttpResponse(HttpStatusCode status) : _status(status){}
            ~HttpResponse() = default;

            HttpStatusCode status() const { return _status;}
            void SetStatusCode(HttpStatusCode status) { _status = status; }

        private:
            HttpStatusCode _status;
    };

    // Utility functions 
    std::string to_string(const HttpRequest& request);
    std::string to_string(const HttpResponse& response, bool send_content = true);
    HttpRequest string_to_request(const std::string& request_string);
    HttpResponse string_to_response(const std::string& response_string);

} // namespace SimpleHttpServer

#endif 