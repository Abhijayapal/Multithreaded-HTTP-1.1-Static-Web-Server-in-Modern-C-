# Deep-Dive: Complete Technical Workflow

This document explains exactly how a request flows through the entire codebase, from the moment you execute `./server` to the moment a browser receives an HTML page. 

Use this to thoroughly understand the architecture so you can confidently explain every single system call and design decision in your Qualcomm interview!

---

## 1. Initialization (`main.cpp`)

When you run `./server 8080 8 50`, the OS hands execution to `main.cpp`.
1. **Argument Parsing**: The program uses `std::stoi` and `std::stoull` to safely parse the Port (`8080`), Thread Count (`8`), and Cache Capacity (`50`).
2. **Signal Handling**: We configure a POSIX `sigaction` to intercept `SIGINT` (Ctrl+C). This tells the OS, *"If the user presses Ctrl+C, do not kill this process. Instead, run my `handle_sigint` function."*
3. **Instantiation**: The `SocketServer` object is created. The constructor internally initializes the `ThreadPool` and `LRUCache`.

## 2. Server Startup (`SocketServer::start`)

To accept network traffic, the server must prepare the OS networking stack.
1. `socket()`: Asks the OS to create a raw IPv4 TCP socket (a file descriptor).
2. `setsockopt()`: Sets `SO_REUSEADDR`. This prevents the OS from locking the port if the server crashes, allowing you to restart it immediately.
3. `bind()`: Links the socket to port `8080` and your machine's IP address (`INADDR_ANY`).
4. `listen()`: Tells the OS, *"I am a server. Start queuing up incoming connections (up to `SOMAXCONN`)."*

## 3. The Infinite Accept Loop (`SocketServer::run`)

The server enters a `while (m_running)` loop on the main thread.
1. `accept()`: This is a **blocking system call**. The main thread goes to sleep and yields its CPU time back to the OS. 
2. When a browser connects, the OS wakes the thread up, and `accept()` returns a new `clientSocket` integer. (This represents the dedicated connection to that specific browser).
3. The server takes this `clientSocket` and pushes it into the `ThreadPool` using a C++ lambda function.
4. The main thread immediately loops back and calls `accept()` again to wait for the next user.

## 4. Concurrency Execution (`ThreadPool.cpp`)

Instead of wasting CPU cycles constantly creating and destroying OS threads, we pre-allocated 8 threads when the program started.
1. **The Worker Loop**: All 8 threads sit inside a `workerLoop()`, sleeping on a `std::condition_variable`.
2. **Waking Up**: When `SocketServer::run` pushes a new client into the task queue, it calls `m_condition.notify_one()`. 
3. **Execution**: One sleeping thread wakes up, acquires the `m_queueMutex`, pops the client task off the queue, *releases the mutex* (so other threads aren't blocked), and executes the lambda.

## 5. Request Processing (`SocketServer::handleSingleClient`)

The worker thread is now executing the lambda for our client.
1. `recv()`: The thread reads the raw bytes sent by the browser (e.g., `"GET /index.html HTTP/1.1\r\nHost: localhost..."`).
2. **Parsing**: The raw string is passed to `HttpRequestParser::parse()`. 
   - We use `std::istringstream` to safely read the string word-by-word.
   - If the method isn't `"GET"`, we throw an exception and immediately return a `400 Bad Request` to protect the server.

## 6. The Memory Cache (`LRUCache.cpp`)

Before hitting the slow hard drive, the worker thread checks the `LRUCache`.
1. The thread locks the cache's internal mutex.
2. `m_cacheMap.find(path)`: It checks the `std::unordered_map` for O(1) lookup.
3. **Cache Hit**: 
   - If found, it uses `std::list::splice` to instantly detach the item and move it to the front of the list (marking it as the "Most Recently Used" item). It returns the data from RAM.
4. **Cache Miss**:
   - If not found, it calls `FileService::readFile()`, which uses `std::ifstream` in binary mode to read the hard drive.
   - The thread then inserts this file into the cache. If the cache is full, it pops the "Least Recently Used" item from the back of the `std::list` in O(1) time.

## 7. Sending the Response 

Now that we have the HTML file in memory, we must send it back.
1. We construct the HTTP headers (`200 OK`, `Content-Length`, `X-Cache`).
2. `sendAll()`: We do not just call `send()` once. If the HTML file is large, the OS network buffers might fill up halfway through. `sendAll()` contains a `while` loop that keeps calling `send()` on the remaining bytes until exactly 100% of the file has been transmitted.
3. `close()`: The worker thread closes the `clientSocket`. The browser renders the webpage, and the worker thread goes back to sleep, waiting for the next client!

---

## 8. Graceful Shutdown (The Finale)

When you press `Ctrl+C`, the `handle_sigint` function intercepts it.
1. It calls `g_server->stop()`.
2. `stop()` forcefully closes the main listening socket.
3. Because the listening socket was closed, the main thread (which is asleep inside the `accept()` call) is instantly interrupted by the OS. `accept()` returns an error.
4. The main thread realizes `m_running == false`, exits the `while` loop, and the `SocketServer` is destroyed.
5. **RAII Magic**: The `SocketServer` destructor automatically triggers the `ThreadPool` destructor. The thread pool flips `m_stop = true`, wakes up all sleeping workers, and calls `join()`. 
6. `join()` guarantees that if a worker thread is currently halfway through sending an image to a client, the process will politely wait for them to finish before finally turning off.
