#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <mutex>

class Logger {
public:
    enum class LogLevel {
        TRACE,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void log(LogLevel level, const std::string& message) {
#ifdef DEBUG
        std::lock_guard<std::mutex> lock(logMutex); // 確保多執行緒安全
        if (level >= currentLogLevel) {
            std::cout << getLogLevelColor(level)
                      << "[" << std::setw(levelWidth) << std::left << logLevelToString(level) << "]"
                      << resetColor()
                      << " " << message << std::endl;
        }
#endif
    }

    void setLogLevel(LogLevel level) {
#ifdef DEBUG
        currentLogLevel = level;
#endif
    }

    void trace(const std::string& message) {
        log(LogLevel::TRACE, message);
    }

    void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }

    void warning(const std::string& message) {
        log(LogLevel::WARNING, message);
    }

    void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }

    void critical(const std::string& message) {
        log(LogLevel::CRITICAL, message);
    }

private:
    Logger() : currentLogLevel(LogLevel::TRACE) {}

    std::string logLevelToString(LogLevel level) const {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }

    std::string getLogLevelColor(LogLevel level) const {
        switch (level) {
            case LogLevel::TRACE: return "\033[37m";    // White
            case LogLevel::INFO: return "\033[32m";     // Green
            case LogLevel::WARNING: return "\033[33m";  // Yellow
            case LogLevel::ERROR: return "\033[31m";    // Red
            case LogLevel::CRITICAL: return "\033[41m"; // Red background
            default: return "\033[0m";                  // Reset color
        }
    }

    std::string resetColor() const {
        return "\033[0m";
    }

    LogLevel currentLogLevel;
    std::mutex logMutex;
    static constexpr int levelWidth = 9;
};

#define LOG_TRACE(message) Logger::getInstance().trace(message)
#define LOG_INFO(message) Logger::getInstance().info(message)
#define LOG_WARNING(message) Logger::getInstance().warning(message)
#define LOG_ERROR(message) Logger::getInstance().error(message)
#define LOG_CRITICAL(message) Logger::getInstance().critical(message)