#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <chrono>
#include <ctime>

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
                      << "[" << getCurrentTimestamp() << "]"
                      << "[+" << getRuntime() << "] "
                      << message << std::endl;
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
    Logger() : currentLogLevel(LogLevel::TRACE), startTime(std::chrono::steady_clock::now()) {}

    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
        return ss.str();
    }

    std::string getRuntime() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = now - startTime;
        
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
        duration -= hours;
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
        duration -= minutes;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        duration -= seconds;
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

        std::stringstream ss;
        ss << std::setfill('0');
        
        if (hours.count() > 0) {
            ss << hours.count() << ":" 
               << std::setw(2) << minutes.count() << ":"
               << std::setw(2) << seconds.count();
        } else if (minutes.count() > 0) {
            ss << minutes.count() << ":"
               << std::setw(2) << seconds.count();
        } else {
            ss << seconds.count();
        }
        ss << "." << std::setw(3) << milliseconds.count();
        return ss.str();
    }

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
    const std::chrono::steady_clock::time_point startTime; // 程式啟動時間
};

#define LOG_TRACE(message) Logger::getInstance().trace(message)
#define LOG_INFO(message) Logger::getInstance().info(message)
#define LOG_WARNING(message) Logger::getInstance().warning(message)
#define LOG_ERROR(message) Logger::getInstance().error(message)
#define LOG_CRITICAL(message) Logger::getInstance().critical(message)