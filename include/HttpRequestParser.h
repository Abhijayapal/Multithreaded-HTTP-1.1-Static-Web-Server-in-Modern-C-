#ifndef HTTPREQUESTPARSER_H
#define HTTPREQUESTPARSER_H

#include "HttpRequest.h"
#include <string>

class HttpRequestParser {
public:
    // Parses a raw HTTP request string into an HttpRequest object.
    // Throws std::invalid_argument if the request is malformed or unsupported.
    static HttpRequest parse(const std::string& rawRequest);
};

#endif // HTTPREQUESTPARSER_H
