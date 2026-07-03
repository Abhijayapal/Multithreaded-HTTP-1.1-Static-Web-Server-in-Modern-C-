# Multithreaded HTTP/1.1 Static Web Server in Modern C++

## 1. Project Overview
A systems-oriented, multithreaded HTTP/1.1 static file server written entirely from scratch in Modern C++17. This project demonstrates strong systems programming fundamentals by utilizing low-level POSIX sockets. The implementation incorporates performance-oriented techniques such as a custom thread pool and an in-memory LRU cache.

The architecture strictly adheres to RAII and Clean Code principles, ensuring robust memory management and thread safety without relying on external networking frameworks (like Boost.Asio or cpp-httplib).

## 2. Project Motivation
This project was built to strengthen my understanding of systems programming concepts including socket programming, concurrency, synchronization, resource management, and HTTP protocol fundamentals by implementing a web server without relying on high-level networking frameworks.

## 3. Project Statistics
- **Language**: C++17
- **Architecture**: Modular
- **Core Components**: 7
- **Build System**: CMake
- **Platform**: Linux / WSL
- **Concurrency Model**: Thread Pool
- **Networking API**: POSIX Socket API

## 4. Repository Structure
```text
.
├── include/
│   ├── SocketServer.h
│   ├── ThreadPool.h
│   ├── HttpRequest.h
│   ├── HttpRequestParser.h
│   ├── FileService.h
│   ├── LRUCache.h
│   └── Logger.h
│
├── src/
│   ├── main.cpp
│   ├── SocketServer.cpp
│   ├── ThreadPool.cpp
│   ├── HttpRequestParser.cpp
│   ├── FileService.cpp
│   ├── LRUCache.cpp
│   └── Logger.cpp
│
├── public/
├── README.md
└── CMakeLists.txt
```

## 6. Features
- **HTTP/1.1 GET Support**: Parses HTTP/1.1 GET requests to extract the requested resource and protocol version.
- **Custom Thread Pool**: A pre-allocated, fixed-size thread pool utilizes `std::condition_variable` to manage concurrent loads without OS thread-creation overhead.
- **POSIX Sockets**: Uses the POSIX Socket API (`socket`, `bind`, `listen`, `accept`, `recv`, `send`) without any external networking framework.
- **Custom LRU Cache**: O(1) in-memory Least Recently Used cache (`std::list` + `std::unordered_map`) minimizes disk I/O.
- **Thread-safe Logger**: Thread-safe logger that serializes log writes using a mutex to prevent interleaved output from multiple worker threads.
- **Graceful Shutdown**: Traps `SIGINT` (Ctrl+C) to safely drain the thread pool, ensuring all active file transfers complete before exiting.
- **RAII Compliance**: Uses RAII to automatically manage socket descriptors, file handles, and thread lifetimes, reducing the risk of resource leaks and simplifying cleanup.
- **MIME Types**: Detects and serves correct headers for HTML, CSS, JS, PNG, JPG, and TXT files using `std::filesystem`.

## 7. Build Instructions

This project targets Linux/WSL environments. It requires `CMake` and a modern C++ compiler (`g++` or `clang`).

```bash
# Clone the repository
git clone https://github.com/yourusername/qualcomm-project-cpp.git
cd qualcomm-project-cpp

# Create build directory and compile
mkdir build
cd build
cmake ..
make

# Navigate back to the root directory so the server can find the 'public/' folder
cd ..

# Run the server (Port 8080, 8 Threads, Cache capacity 50)
./build/server 8080 8 50
```

## 8. Design Decisions

### Why Modern C++?
The project uses C++17 features such as smart pointers, RAII, `std::thread`, `std::mutex`, `std::condition_variable`, `std::filesystem`, and STL containers to improve safety, readability, and resource management compared to traditional C-style implementations.

### Why a Thread Pool?
Creating a new thread for every client request introduces significant overhead due to thread creation and destruction. A fixed-size thread pool allows worker threads to be reused, improving scalability under concurrent workloads.

### Why an LRU Cache?
Frequently requested static assets are served directly from memory instead of repeatedly reading from disk, reducing file I/O latency.

### Why RAII?
Socket descriptors and other resources are automatically released when objects go out of scope, preventing leaks and ensuring exception-safe cleanup.

### Why POSIX Sockets?
Using the POSIX Socket API provides direct exposure to low-level networking concepts without abstracting away important system calls.

### Why Blocking I/O?
Blocking sockets were intentionally chosen to keep the networking model simple and focus on understanding socket programming, synchronization, and thread management. Future improvements could replace this with an event-driven model using epoll.

## 9. Screenshots & Logs

*(Note: Replace the image paths below with actual screenshots of your terminal and browser)*

### Server Started & Listening
```text
[INFO] ThreadPool initialized with 8 threads.
[INFO] Server socket created successfully.
[INFO] Server bound to port 8080.
[INFO] Server is listening on port 8080.
[INFO] Server is running. Waiting for connections...
```
![Server Started](./screenshots/server_started.png)

### Client Connected (Cache Miss & Cache Hit)
```text
[INFO] Parsed Request -> Method: GET, Path: /index.html, Version: HTTP/1.1
[INFO] Cache Miss: public/index.html
[INFO] Successfully served: public/index.html
[INFO] Parsed Request -> Method: GET, Path: /index.html, Version: HTTP/1.1
[INFO] Cache Hit: public/index.html
[INFO] Successfully served: public/index.html
```
![Cache Hit/Miss](./screenshots/cache_logs.png)

### Browser Displaying Webpage (`localhost:8080/index.html`)
![Browser Preview](./screenshots/browser_preview.png)

## 10. Performance Benchmarks

The server was benchmarked using ApacheBench to evaluate throughput and latency under concurrent client loads.

**Command:**
```bash
ab -n 10000 -c 100 http://localhost:8080/index.html
```

**Results:**
- **Zero Failed TCP Requests**: Completed 10,000 requests with zero TCP-level failures under a concurrency level of 100.
- **Throughput**: Maintained ~3,692 Requests per second (mean).
- **Latency**: Achieved a 0.271 ms mean latency per request across all concurrent connections.

## 11. Current Limitations
- Supports HTTP/1.1 GET requests only.
- Serves static files only.
- Uses blocking sockets with a thread pool.
- Does not support HTTPS.
- Does not support persistent connections (Keep-Alive).

## 12. Future Improvements
- **Request Pipelining**: Processing multiple HTTP requests on a single connection sequentially.
- **Connection Timeout Handling**: Closing stale or inactive connections to free up worker threads.
- **HTTPS Support**: Integrating OpenSSL or BoringSSL for secure TLS connections.
- **Keep-Alive**: Implementing persistent connections to reduce TCP handshake overhead on subsequent requests.
- **POST/PUT Requests**: Adding support for varying HTTP verbs and request bodies.
- **Asynchronous I/O (`epoll`)**: Transitioning from a blocking Thread Pool model to an event-driven `epoll`/`kqueue` reactor pattern for handling highly concurrent connections on a single thread.
