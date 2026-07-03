#include "SocketServer.h"
#include "Logger.h"
#include "HttpRequestParser.h"
#include "FileService.h"
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <iostream>

SocketServer::SocketServer(int port, size_t numThreads, size_t cacheCapacity) 
    : m_port(port), m_serverSocket(-1), m_running(false) {
    // Initialize server address structure to zero
    std::memset(&m_serverAddress, 0, sizeof(m_serverAddress));
    
    // Initialize the ThreadPool
    m_threadPool = std::make_unique<ThreadPool>(numThreads);
    
    // Initialize the LRU Cache
    m_cache = std::make_unique<LRUCache>(cacheCapacity);
}

SocketServer::~SocketServer() {
    // Stop the server if it hasn't been stopped already
    stop();
    LOG_INFO("Server socket closed.");
}

void SocketServer::start() {
    // 1. Create socket (IPv4, TCP stream)
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket == -1) {
        LOG_ERROR("Failed to create socket.");
        throw std::runtime_error("Socket creation failed.");
    }

    // 2. Configure socket options
    // SO_REUSEADDR is crucial. When a server restarts, the previous socket might still be in the TIME_WAIT state.
    // Without SO_REUSEADDR, binding to the same port would fail with "Address already in use".
    // This allows us to quickly restart the server during development or after a crash.
    int opt = 1;
    if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        LOG_ERROR("Failed to set SO_REUSEADDR.");
        throw std::runtime_error("setsockopt failed.");
    }

    // 3. Bind the socket to the port
    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = INADDR_ANY; // Bind to all available network interfaces
    m_serverAddress.sin_port = htons(m_port);     // Convert port to network byte order

    if (bind(m_serverSocket, (struct sockaddr*)&m_serverAddress, sizeof(m_serverAddress)) == -1) {
        LOG_ERROR("Failed to bind socket to port " + std::to_string(m_port));
        throw std::runtime_error("Bind failed.");
    }

    // 4. Listen for incoming connections
    // SOMAXCONN defines the maximum number of backlogged connections
    if (listen(m_serverSocket, SOMAXCONN) == -1) {
        LOG_ERROR("Failed to listen on socket.");
        throw std::runtime_error("Listen failed.");
    }

    LOG_INFO("Server started successfully. Listening on port " + std::to_string(m_port));
}

int SocketServer::acceptClient() {
    struct sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);

    // Block until a client connects
    int clientSocket = accept(m_serverSocket, (struct sockaddr*)&clientAddress, &clientLen);
    if (clientSocket == -1) {
        if (m_running) {
            LOG_ERROR("Failed to accept client connection.");
        }
        return -1; // Return -1 instead of throwing to allow the server to keep running
    }

    return clientSocket;
}

void SocketServer::run() {
    m_running = true;
    LOG_INFO("Server is running. Waiting for connections...");
    
    // The server loops until m_running is set to false.
    while (m_running) {
        int clientSocket = acceptClient();
        if (clientSocket != -1) {
            // Enqueue the task. We capture 'this' and 'clientSocket' by value.
            m_threadPool->enqueue([this, clientSocket]() {
                this->handleSingleClient(clientSocket);
            });
        }
    }
    LOG_INFO("Server stopped accepting new connections.");
}

void SocketServer::stop() {
    if (m_running) {
        m_running = false;
        // Closing the listening socket causes the blocking accept() in run() to immediately unblock and return -1.
        if (m_serverSocket != -1) {
            close(m_serverSocket);
            m_serverSocket = -1;
        }
    }
}

// Helper function to ensure all bytes are sent
// A single send() call on a TCP socket is not guaranteed to send the entire buffer
// if the OS networking buffers are full. We must loop until all bytes are sent.
bool SocketServer::sendAll(int clientSocket, const char* buffer, size_t length) {
    size_t totalSent = 0;
    while (totalSent < length) {
        ssize_t bytesSent = send(clientSocket, buffer + totalSent, length - totalSent, 0);
        if (bytesSent == -1) {
            LOG_ERROR("Failed to send data to client.");
            return false; // Connection closed or error occurred
        }
        totalSent += bytesSent;
    }
    return true;
}

void SocketServer::handleSingleClient(int clientSocket) {
    if (clientSocket == -1) return;

    char buffer[4096] = {0};
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead > 0) {
        std::string rawRequest(buffer);
        
        try {
            // Parse the incoming request
            HttpRequest request = HttpRequestParser::parse(rawRequest);
            LOG_INFO("Parsed Request -> Method: " + request.method + ", Path: " + request.path + ", Version: " + request.version);
            
            // Phase 4 & 6: Static File Serving with LRU Cache
            try {
                std::string filePath = FileService::resolvePath(request.path);
                std::string mimeType = FileService::getMimeType(filePath);
                std::string fileContent;
                std::string cacheStatus;
                
                // Attempt to fetch from cache first
                auto cachedContent = m_cache->get(filePath);
                if (cachedContent.has_value()) {
                    fileContent = cachedContent.value();
                    cacheStatus = "HIT";
                } else {
                    // Cache Miss: read from disk
                    fileContent = FileService::readFile(filePath);
                    // Store in cache for next time
                    m_cache->put(filePath, fileContent);
                    cacheStatus = "MISS";
                }
                
                std::string response = 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: " + mimeType + "\r\n"
                    "Content-Length: " + std::to_string(fileContent.length()) + "\r\n"
                    "X-Cache: " + cacheStatus + "\r\n"
                    "Connection: close\r\n\r\n" + 
                    fileContent;
                    
                sendAll(clientSocket, response.c_str(), response.length());
                LOG_INFO("Successfully served: " + filePath);
                
            } catch (const std::runtime_error& e) {
                // File not found
                LOG_WARNING("File not found: " + request.path);
                std::string body = "<html><body><h1>404 Not Found</h1></body></html>";
                std::string response = 
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: " + std::to_string(body.length()) + "\r\n"
                    "Connection: close\r\n\r\n" + body;
                sendAll(clientSocket, response.c_str(), response.length());
            }
            
        } catch (const std::invalid_argument& e) {
            LOG_WARNING(std::string("Malformed request received: ") + e.what());
            std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            sendAll(clientSocket, response.c_str(), response.length());
        }
        
    } else if (bytesRead == 0) {
        LOG_INFO("Client disconnected cleanly.");
    } else {
        LOG_ERROR("Failed to read from client.");
    }

    close(clientSocket);
    LOG_INFO("Client connection closed.");
}
