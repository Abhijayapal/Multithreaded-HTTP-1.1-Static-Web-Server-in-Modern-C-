#ifndef FILESERVICE_H
#define FILESERVICE_H

#include <string>

class FileService {
public:
    // Resolves the requested URL path to a local file path inside the 'public' directory.
    // e.g., "/" -> "public/index.html", "/style.css" -> "public/style.css"
    static std::string resolvePath(const std::string& requestPath);

    // Determines the MIME type based on the file extension.
    static std::string getMimeType(const std::string& filePath);

    // Reads the entire contents of a file into a string (binary safe).
    // Throws std::runtime_error if the file cannot be opened.
    static std::string readFile(const std::string& filePath);
};

#endif // FILESERVICE_H
