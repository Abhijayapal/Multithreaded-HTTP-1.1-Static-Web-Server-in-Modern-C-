cd "/mnt/c/Users/ABHIJAYS/System-design/qualcomm-project cpp/build"
cmake ..
make
./server 8080 8 50



# High-Performance Multithreaded HTTP/1.1 Static Web Server

This document outlines the implementation plan for building a production-inspired HTTP/1.1 static file server from scratch using Modern C++17/20 and POSIX sockets.

As you have requested, this project will be implemented incrementally, phase by phase. I will explain the design decisions, C++, OS, and networking concepts, as well as possible interview questions after completing each phase.

## User Review Required

Since you do not currently have an active workspace, I plan to create this project in the following directory:
`C:\Users\ABHIJAYS\.gemini\antigravity-ide\scratch\http-server`

> [!IMPORTANT]
> Please confirm if this directory is acceptable or if you would like me to use a different location for the project. Also, I highly recommend opening this folder in your IDE as your active workspace once it is created.

## Open Questions

> [!TIP]
> 1. Do you have a preference between C++17 and C++20 for this project? I will default to C++17 as it provides all the necessary features (`std::string_view`, `std::filesystem`, etc.) for this scope, but I can use C++20 (e.g., concepts, ranges) if you prefer.
> 2. For testing on Windows, standard POSIX sockets (like `<sys/socket.h>`) are not natively available; Winsock (`<winsock2.h>`) is used instead. If you plan to compile and run this *natively* on Windows, I will need to add a thin abstraction layer or use Winsock. Alternatively, if you plan to run this using WSL (Windows Subsystem for Linux), we can strictly stick to POSIX sockets. **Please let me know if we are targeting native Windows or WSL/Linux.**

## Proposed Implementation Phases

### Phase 1: Project Setup
- Establish folder structure (`src/`, `include/`, `public/`).
- Create `CMakeLists.txt` for standard compilation.
- Implement the basic `Logger` class (thread-safe, supporting INFO, WARNING, ERROR).

### Phase 2: Socket Server Foundation
- Implement `SocketServer` to handle `socket()`, `bind()`, `listen()`, and `accept()`.
- Enable `SO_REUSEADDR`.
- Test single-client connection handling.

### Phase 3: HTTP Parsing
- Implement `HttpRequestParser`.
- Parse the HTTP method (GET only), path, and version.
- Graceful handling of malformed requests.

### Phase 4: Static File Serving
- Implement `FileService`.
- Read files from the `/public` directory.
- Determine MIME types and handle 404 Not Found scenarios.

### Phase 5: Thread Pool
- Implement the `ThreadPool` class.
- Manage worker threads, a task queue, `std::mutex`, and `std::condition_variable`.
- Integrate the thread pool into the server to handle multiple concurrent clients.

### Phase 6: LRU Cache
- Implement an in-memory `LRUCache`.
- Use `std::unordered_map` and `std::list` for O(1) operations.
- Thread-safe caching of file contents.

### Phase 7: Graceful Shutdown
- Implement signal handling (`SIGINT`).
- Cleanly stop accepting connections, notify and join all worker threads, and close sockets.

### Phase 8: Testing & Documentation
- Write the final `README.md` with architecture details and build instructions.
- Test with `curl`, browser, and benchmarking tools like ApacheBench (`ab`).

## Next Steps
Once you approve this plan (and answer the open questions regarding C++ version and OS target), I will immediately begin executing **Phase 1: Project Setup**.




ques 
1. **We use \n instead of std::endl because std::endl aggressively flushes the output buffer, which can slightly reduce performance in high-throughput applications.**

std::cout has an internal memory buffer. When you print characters, they go into this buffer instead of directly to the screen (which is slow). \n just adds a newline character to the buffer. std::endl adds a newline character and forces the buffer to immediately dump its contents to the screen (a "flush"). In a high-performance server writing thousands of logs per second, forcing a flush every single time can become a massive bottleneck.

argc (argument count) and argv (argument vector/values) 
argc is always at least 1 (because the name of the program itself, e.g., ./server, counts as the first argument).


> 1. Why did you use std::lock_guard instead of manually calling mutex.lock() and mutex.unlock()?
Answer: It guarantees that the mutex is unlocked even if an exception is thrown before unlock() can be called. This is the core principle of RAII (Resource Acquisition Is Initialization).

>2. Is the Meyers Singleton thread-safe in C++?
Answer: Yes, since C++11, the initialization of static local variables is guaranteed to be thread-safe by the standard.

====================================================================================
>3.Why do we need to call htons() on the port?
Answer: Different computer architectures store multi-byte integers in different orders (little-endian vs. big-endian). The network protocol requires a standard (big-endian). htons() ensures the port is converted to big-endian regardless of the host machine's architecture.

>What does SOMAXCONN do in the listen() call?
Answer: It defines the maximum length of the queue of pending connections. If the server is busy processing a request, new incoming connections wait in this queue. If the queue is full, the client receives a "Connection refused" error.

>Why did we delete the copy constructor of SocketServer?
Answer: To prevent two objects from holding the same raw file descriptor. If a copy was made, both objects' destructors would attempt to close() the same socket, causing undefined behavior.

>Why use std::istringstream instead of string::find and string::substr to parse the method and path?
Answer: istringstream is safer and more readable. It automatically handles multiple consecutive spaces and gracefully stops at the end of the string, preventing "out of bounds" errors that are common when manually doing math with string indices.

>Why throw an exception in the parser instead of returning a special "error" HttpRequest object?
Answer: Returning an "error" object forces the caller to manually check if (request.isValid()) every single time. Throwing an exception guarantees that the caller cannot accidentally use a malformed request, leading to safer code.

>Why did you implement a sendAll loop instead of just calling send() once?
Answer: TCP is a stream protocol. If the response is large (like an image) or the network is congested, the OS TCP send buffer might fill up. When this happens, send() returns the number of bytes it successfully queued, which might be less than the total size of the file. sendAll loops and repeatedly calls send() with an offset until all bytes are transmitted.

>Why is std::ios::binary necessary when reading files for a web server?
Answer: A web server must serve all types of files, including compiled binaries and images. Without the binary flag, the C++ standard library might intercept bytes that look like line endings and alter them to match the host OS format, which permanently corrupts non-text files.

> Why do we pass a lambda with a while loop to m_condition.wait(lock, [...] { return m_stop || !m_tasks.empty(); })?
Answer: To protect against "Spurious Wakeups". Occasionally, the OS might wake up a thread even if notify_one() wasn't called. By passing the lambda predicate, wait will immediately check the condition upon waking. If the queue is actually empty, it goes right back to sleep automatically.

>Why do we extract the task from the queue inside the lock, but execute task() outside the lock?
Answer: If we executed task() while holding the m_queueMutex, no other worker thread could pop a task from the queue while that first task was running! By releasing the lock before calling task(), we ensure maximum parallelism.

>Why not just use std::vector instead of std::list to keep track of the MRU order?
Answer: If we used a vector, moving an item to the front of the cache would require shifting every other element down by one index, which is an O(N) operation. A doubly-linked list (std::list) allows us to move nodes in O(1) time by just changing a few pointers.

>Why is the mutex inside LRUCache rather than locking inside SocketServer before calling m_cache->get()?
Answer: Encapsulation. The LRUCache should be responsible for its own thread-safety. If we made SocketServer do it, another developer might write a new class that uses LRUCache, forget to lock the mutex, and cause a race condition.

>Why did you close the listening socket inside the signal handler's stop() method instead of just waiting for the loop to naturally end?

Answer: Because accept() is a blocking system call. If no clients connect, the server will sit frozen on accept() forever, completely ignoring the m_running = false flag. Closing the file descriptor forces accept() to return an error, allowing the loop to evaluate the flag and exit cleanly.
>How do you ensure that worker threads currently sending large files aren't killed instantly?
Answer: The ThreadPool destructor explicitly calls worker.join() on every thread. join() pauses the main thread until the worker finishes its current task, guaranteeing that active clients receive their full files before the process exits.


============================================================================================
