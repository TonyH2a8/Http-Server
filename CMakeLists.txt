cmake_minimum_required(VERSION 3.10)

project(HttpServer
    DESCRIPTION "A simple web server that supports HTTP/1.1"
    LANGUAGES CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(src)

set(SRC_DIR src)
set(TEST_DIR test)

add_executable(HttpServer
    ${SRC_DIR}/main.cc
    ${SRC_DIR}/httpserver.cxx
    ${SRC_DIR}/httpmessage.cxx
)

add_executable(testHttpServer
    ${TEST_DIR}/main.cc
    ${SRC_DIR}/httpserver.cxx
    ${SRC_DIR}/httpmessage.cxx
)

target_link_libraries(HttpServer PRIVATE Threads::Threads)
target_link_libraries(testHttpServer PRIVATE Threads::Threads)