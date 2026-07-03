#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>

// A simple structure to hold the parsed components of an HTTP request
struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
};

#endif // HTTPREQUEST_H
