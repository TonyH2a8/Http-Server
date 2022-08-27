# Basic HTTP server support HTTP/1.1

Implementation of a simple HTTP server in C++ 

## Features

- Can handle multiple concurrent connections, tested up to 10k.
- Serve at least 100000 requests per second, tested up to 250k.
- Support basic HTTP request and response.

## Quick start

```bash
mkdir build && cd build
cmake ..
make -j4
./test_HttpServer # Run unit tests
./HttpServer      # Start the HTTP server on port 8080
```

- There are two endpoints available at `/` and `/hello.html` which are created for demo purpose.

## Design

The server program consists of:

- 1 main thread for user interaction.
- 1 listener thread to accept incoming clients.
- 5 worker threads to process HTTP requests and sends response back to client.
- Utility functions to parse and manipulate HTTP requests and repsonses conveniently.

## Benchmark

I used a tool called [wrk](https://github.com/wg/wrk) to benchmark this HTTP server. The tests were performed on my PC with the following specs:

```bash
OS: Ubuntu 18.04 TLS x84_64
Kernel: 4.15.0-191-generic
CPU: Intel i5-9400 @ 2.90 GHz
Memory: 15500 MiB
```

Here are the results for two test runs. Each test ran for 1 minute, with 10 client threads. The first test had only 500 concurrent connections, while the second test had 10000.

```bash
$ ./wrk -t10 -c500 -d60s http://127.0.0.1:8080/
Running 1m test @ http://127.0.0.1:8080/
  10 threads and 500 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     2.43ms    2.55ms  37.71ms   89.71%
    Req/Sec     25.99k     7.88k   95.88k    70.70%
  15511958 requests in 1.00m, 1.13GB read
Requests/sec:  258163.38
Transfer/sec:      19.20MB
```

```bash
$ ./wrk -t10 -c10000 -d60s http://127.0.0.1:8080/
Running 1m test @ http://127.0.0.1:8080/
  10 threads and 10000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     3.87ms    3.11ms  215.90ms   80.92%
    Req/Sec     27.10k    15.20k   75.78k    55.19%
  14576586 requests in 1.00m, 1.06GB read
Requests/sec:  242616.58
Transfer/sec:      18.05MB

```

