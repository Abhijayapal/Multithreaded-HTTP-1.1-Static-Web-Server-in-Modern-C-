#ifndef LOGGER_H //if not defined, define logger 
#define LOGGER_H

#include <string>
#include <mutex>
#include <iostream>
#include <fstream>
#include <chrono>

// Define log levels & prevent implicit conversion to int & avoid naming collisions. (Scoping)
enum class LogLevel {
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    // Delete copy constructor and assignment operator for Singleton (if desired, or just static methods)
    // We'll use a static approach for simplicity and global access without passing instances around.
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    // Prevent copy constructor and assignment operator for Singleton
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Log a message with a specific level
    void log(LogLevel level, const std::string& message);

    // Convenience methods
    void info(const std::string& message) { log(LogLevel::INFO, message); }
    void warning(const std::string& message) { log(LogLevel::WARNING, message); }
    void error(const std::string& message) { log(LogLevel::ERROR, message); }

private:
    Logger() = default;
    ~Logger() = default;

    std::string getTimestamp() const;
    std::string levelToString(LogLevel level) const;

    std::mutex m_mutex; // Protects access to the output stream
};

// Global shorthand macro 
#define LOG_INFO(msg) Logger::getInstance().info(msg)
#define LOG_WARNING(msg) Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) Logger::getInstance().error(msg)

#endif // LOGGER_H
