#include "HttpRequestParser.h"
#include <sstream>
#include <stdexcept>

HttpRequest HttpRequestParser::parse(const std::string& rawRequest) {
    if (rawRequest.empty()) {
        throw std::invalid_argument("Empty request");
    }

    std::istringstream iss(rawRequest);
    HttpRequest request;

    // The first line of an HTTP request is the Request-Line:
    // Method SP Request-URI SP HTTP-Version CRLF
    iss >> request.method >> request.path >> request.version;

    // Check if we successfully parsed all three components
    if (request.method.empty() || request.path.empty() || request.version.empty()) {
        throw std::invalid_argument("Malformed request line");
    }

    // Phase 3 Requirement: Only support GET
    if (request.method != "GET") {
        throw std::invalid_argument("Unsupported HTTP method (only GET is supported)");
    }

    // Phase 3 Requirement: Basic validation of HTTP version
    if (request.version.substr(0, 5) != "HTTP/") {
        throw std::invalid_argument("Invalid HTTP version");
    }

    return request;
}
