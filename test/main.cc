// unit tests

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <string>

#include "httpmessage.h"

using namespace SimpleHttpServer;

#define EXPECT_TRUE(x)                                                   \
  {                                                                      \
    if (!(x))                                                            \
      err++, std::cerr << __FUNCTION__ << " failed on line " << __LINE__ \
                       << std::endl;                                     \
  }

int err = 0;

void test_request_to_string() {
  HttpRequest request;
  request.setMethod(HttpMethod::GET);
  request.setUri("/");
  request.setHeader("Connection", "Keep-Alive");
  request.setContent("hello, world\n");

  std::string expected_str;
  expected_str += "GET / HTTP/1.1\r\n";
  expected_str += "Connection: Keep-Alive\r\n";
  expected_str += "Content-Length: 13\r\n\r\n";
  expected_str += "hello, world\n";

  EXPECT_TRUE(to_string(request) == expected_str);
}

void test_response_to_string() {
  HttpResponse response;
  response.SetStatusCode(HttpStatusCode::InternalServerError);

  std::string expected_str;
  expected_str += "HTTP/1.1 500 Internal Server Error\r\n\r\n";

  EXPECT_TRUE(to_string(response) == expected_str);
}

int main(void){
    std::cout << "Running tests..." << std::endl;

    test_request_to_string();
    test_response_to_string();

    std::cout << "All tests have finished. There were " << err
                << " errors in total" << std::endl;
    return 0;
}