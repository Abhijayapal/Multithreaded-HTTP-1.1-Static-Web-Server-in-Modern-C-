#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include <string>
#include <netinet/in.h> // sockaddr_in
#include <memory>
#include "ThreadPool.h"
#include "LRUCache.h"

class SocketServer {
public:
    // Constructor initializes the server with a specific port, thread count, and cache capacity
    SocketServer(int port, size_t numThreads, size_t cacheCapacity);
    
    // Destructor ensures the listening socket is closed via RAII
    ~SocketServer();

    // Prevent copying because socket descriptors are unique resources
    SocketServer(const SocketServer&) = delete;
    SocketServer& operator=(const SocketServer&) = delete;

    // Starts the server: creates socket, binds, and listens
    void start();

    // The main server loop that accepts clients and dispatches them to the ThreadPool
    void run();

    // Initiates a graceful shutdown
    void stop();

private:
    // Accepts a single client connection (blocking call)
    // Returns the client socket descriptor
    int acceptClient();

    // Method to handle one client (will be executed by a worker thread)
    void handleSingleClient(int clientSocket);

    // Helper function to guarantee all data is sent over the TCP socket
    bool sendAll(int clientSocket, const char* buffer, size_t length);

    int m_port;
    int m_serverSocket;
    struct sockaddr_in m_serverAddress;
    
    std::unique_ptr<ThreadPool> m_threadPool;
    std::unique_ptr<LRUCache> m_cache;
    
    std::atomic<bool> m_running;
};

#endif // SOCKETSERVER_H
