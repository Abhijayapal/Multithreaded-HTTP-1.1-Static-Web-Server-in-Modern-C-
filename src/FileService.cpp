#include "FileService.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>

std::string FileService::resolvePath(const std::string& requestPath) {
    std::string path = requestPath;
    
    // Default to index.html if the root is requested
    if (path == "/") {
        path = "/index.html";
    }

    // Prepend the public directory
    return "public" + path;
}

std::string FileService::getMimeType(const std::string& filePath) {
    // Extract the extension using std::filesystem (C++17 feature)
    std::string ext = std::filesystem::path(filePath).extension().string();

    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".txt") return "text/plain";
    
    // Image types
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".ico") return "image/x-icon";

    // Default for unknown binary files
    return "application/octet-stream";
}

std::string FileService::readFile(const std::string& filePath) {
    // Open file in binary mode to prevent OS-specific line ending translations
    // and to safely read binary files like images.
    std::ifstream file(filePath, std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("File not found");
    }

    // Read the entire file into an std::stringstream
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return buffer.str();
}
