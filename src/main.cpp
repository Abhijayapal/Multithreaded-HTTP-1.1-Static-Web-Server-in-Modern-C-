#include "Logger.h"
#include "SocketServer.h"
#include <iostream>
#include <string>
#include <signal.h>

// Global pointer needed for the C-style signal handler
SocketServer* g_server = nullptr;

// Signal handler for SIGINT (Ctrl+C)
void handle_sigint(int sig) {
    LOG_INFO("\nCaught signal " + std::to_string(sig) + " (SIGINT). Initiating graceful shutdown...");
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    int port = 8080; // Default port
    size_t numThreads = 4; // Default thread pool size
    size_t cacheCapacity = 10; // Default LRU cache size

    if (argc >= 2) {
        try { port = std::stoi(argv[1]); } 
        catch (const std::exception& e) { LOG_ERROR("Invalid port. Using 8080."); port = 8080; }
    }
    
    if (argc >= 3) {
        try { numThreads = std::stoull(argv[2]); } 
        catch (const std::exception& e) { LOG_ERROR("Invalid thread count. Using 4."); numThreads = 4; }
    }

    if (argc >= 4) {
        try { cacheCapacity = std::stoull(argv[3]); } 
        catch (const std::exception& e) { LOG_ERROR("Invalid cache capacity. Using 10."); cacheCapacity = 10; }
    }

    // Set up the signal handler using sigaction (POSIX compliant)
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        LOG_ERROR("Failed to set up signal handler.");
        return 1;
    }

    LOG_INFO("Initializing server...");

    try {
        SocketServer server(port, numThreads, cacheCapacity);
        g_server = &server; // Assign to global pointer so the signal handler can access it
        
        server.start();

        // Phase 5 & 7 Requirement: Start the run loop. 
        // This will block until stop() is called by the signal handler.
        server.run();
        
        // When run() returns, the server object goes out of scope and its destructor is called.
        // The destructor cleans up the ThreadPool (joining all threads) and the LRUCache.
        g_server = nullptr;
        
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Server crashed: ") + e.what());
        return 1;
    }
    
    return 0;
}
