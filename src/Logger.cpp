#include "Logger.h"
#include <iomanip>
#include <sstream>

std::string Logger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    // Acquire lock to ensure thread-safety when writing to std::cout
    //as cout is shared 
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::cout << "[" << getTimestamp() << "] "<< "[" << levelToString(level) << "] "<< message << "\n";
}
