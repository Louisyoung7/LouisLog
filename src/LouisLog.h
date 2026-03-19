#pragma once

#include <cstdio>
#include <fstream>
#include <mutex>
#include <string>

namespace louis {
namespace log {
// 日志级别枚举
enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

// 日志目标枚举
enum class LogTarget { CONSOLE, FILE, BOTH };

class LouisLog {
    LogLevel level_;            // 日志级别
    LogTarget target_;          // 日志目标
    std::string logFile_;       // 日志文件名
    size_t maxFileSize_;        // 日志文件最大大小，用于实现文件翻滚
    std::ofstream fileStream_;  // 用于将日志输出到文件的文件流对象
    std::mutex mutex_;

   public:
    // 获取日志单例
    static LouisLog& getInstance();

    // 初始化日志对象
    void init(LogLevel level = LogLevel::INFO, LogTarget target = LogTarget::CONSOLE, std::string logFile = "app.log",
              size_t maxFileSize = 1024 * 1024);

    // 日志写入
    void log(LogLevel level, const std::string& file, int line, const std::string& msg);

    // 设置日志级别
    void setLevel(LogLevel level);

    // 设置日志输出目标
    void setTarget(LogTarget target);

    // 设置日志输出文件
    void setLogFile(const std::string& logFile);

    // 设置日志文件最大大小
    void setMaxSize(size_t maxSize);

   private:
    // 私有构造
    LouisLog() = default;

    // 禁用拷贝与赋值
    LouisLog(const LouisLog&) = delete;
    LouisLog& operator=(const LouisLog&) = delete;

    // 获取时间戳
    std::string getTimestamp();

    // 获取日志级别对应的字符串
    std::string getLevelString(LogLevel level);

    // 获取线程ID
    std::string getThreadId();

    // 检查文件大小，判断是否翻滚文件
    void checkAndRollLog();

    // 打开日志文件
    void openLogFile();
};

// 日志宏定义
#define TRACE(message) LouisLog::getInstance().log(LogLevel::TRACE, __FILE__, __LINE__, message)
#define DEBUG(message) LouisLog::getInstance().log(LogLevel::DEBUG, __FILE__, __LINE__, message)
#define INFO(message) LouisLog::getInstance().log(LogLevel::INFO, __FILE__, __LINE__, message)
#define WARN(message) LouisLog::getInstance().log(LogLevel::WARN, __FILE__, __LINE__, message)
#define ERROR(message) LouisLog::getInstance().log(LogLevel::ERROR, __FILE__, __LINE__, message)
#define FATAL(message) LouisLog::getInstance().log(LogLevel::FATAL, __FILE__, __LINE__, message)

// 支持可变参数的日志宏定义
#define TRACE_F(format, ...)                                                      \
    do {                                                                          \
        char buffer[1024];                                                        \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);                  \
        LouisLog::getInstance().log(LogLevel::TRACE, __FILE__, __LINE__, buffer); \
    } while (0)

#define DEBUG_F(format, ...)                                                      \
    do {                                                                          \
        char buffer[1024];                                                        \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);                  \
        LouisLog::getInstance().log(LogLevel::DEBUG, __FILE__, __LINE__, buffer); \
    } while (0)

#define INFO_F(format, ...)                                                      \
    do {                                                                         \
        char buffer[1024];                                                       \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);                 \
        LouisLog::getInstance().log(LogLevel::INFO, __FILE__, __LINE__, buffer); \
    } while (0)

#define WARN_F(format, ...)                                                      \
    do {                                                                         \
        char buffer[1024];                                                       \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);                 \
        LouisLog::getInstance().log(LogLevel::WARN, __FILE__, __LINE__, buffer); \
    } while (0)

#define ERROR_F(format, ...)                                                      \
    do {                                                                          \
        char buffer[1024];                                                        \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);                  \
        LouisLog::getInstance().log(LogLevel::ERROR, __FILE__, __LINE__, buffer); \
    } while (0)

#define FATAL_F(format, ...)                                                      \
    do {                                                                          \
        char buffer[1024];                                                        \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);                  \
        LouisLog::getInstance().log(LogLevel::FATAL, __FILE__, __LINE__, buffer); \
    } while (0)

}  // namespace log
}  // namespace louis
