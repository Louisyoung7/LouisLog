#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace louis {
namespace log {
// 日志级别枚举
enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

// 日志目标枚举
enum class LogTarget { CONSOLE, FILE, BOTH };

class LouisLog {
   public:
    static LouisLog& getInstance();  // 获取日志单例

    void init(
        LogLevel level = LogLevel::INFO, LogTarget target = LogTarget::CONSOLE,
        std::string logFile = "app.log", size_t maxFileSize = 1024 * 1024
    );  // 初始化日志对象

    void log(
        LogLevel level, const std::string& file, int line, const std::string& msg
    );  // 日志写入

    void setLevel(LogLevel level);
    void setTarget(LogTarget target);
    void setLogFile(const std::string& logFile);
    void setMaxSize(size_t maxSize);  // 设置日志文件最大大小

    void flush();  // 阻塞直到队列清空（测试/退出前用）
    void stop();   // 停止异步日志处理线程

   private:
    LouisLog() = default;
    ~LouisLog();
    LouisLog(const LouisLog&) = delete;
    LouisLog& operator=(const LouisLog&) = delete;

    std::string getTimestamp() const;
    std::string getLevelString(LogLevel level) const;
    std::string getThreadId() const;

    void openLogFile(const std::string& logFile);  // 打开日志文件

    void checkAndRollLog(const std::string& logFile);  // 检查文件大小，判断是否翻滚文件

    void writeLoop();  // 异步日志写入循环

    void outputBatch(
        std::vector<std::string> msgs, LogTarget target, std::string logFile
    );  // 真正写 console/file，仅后台线程调用

    std::atomic<log::LogLevel> level_;    // 日志级别
    std::atomic<log::LogTarget> target_;  // 日志目标
    bool initialized_{false};             // 是否已初始化

    std::string logFile_;             // 日志文件名
    std::atomic_size_t maxFileSize_;  // 日志文件最大大小，用于实现文件翻滚
    std::ofstream fileStream_;        // 用于将日志输出到文件的文件流对象

    std::deque<std::string> pendingLogs_;  // 异步日志队列，用于存储待处理的日志消息
    std::thread workerThread_;             // 异步日志处理线程
    std::condition_variable cv_worker_;  // 条件变量，用于通知worker线程有新日志，或已停止
    std::atomic_bool stopped_{false};  // 异步日志处理线程是否已停止
    bool writing_{false};  // 异步日志处理线程是否正在写入，配合 cv_written_
    std::condition_variable cv_written_;  // 条件变量，用于通知 flush/stop 写入完成

    std::mutex mutex_;
};

}  // namespace log
}  // namespace louis

// 日志宏定义
#define TRACE(message)                                           \
    louis::log::LouisLog::getInstance().log(                     \
        louis::log::LogLevel::TRACE, __FILE__, __LINE__, message \
    )
#define DEBUG(message)                                           \
    louis::log::LouisLog::getInstance().log(                     \
        louis::log::LogLevel::DEBUG, __FILE__, __LINE__, message \
    )
#define INFO(message) \
    louis::log::LouisLog::getInstance().log(louis::log::LogLevel::INFO, __FILE__, __LINE__, message)
#define WARN(message) \
    louis::log::LouisLog::getInstance().log(louis::log::LogLevel::WARN, __FILE__, __LINE__, message)
#define ERROR(message)                                           \
    louis::log::LouisLog::getInstance().log(                     \
        louis::log::LogLevel::ERROR, __FILE__, __LINE__, message \
    )
#define FATAL(message)                                           \
    louis::log::LouisLog::getInstance().log(                     \
        louis::log::LogLevel::FATAL, __FILE__, __LINE__, message \
    )

// 支持可变参数的日志宏定义
#define TRACE_F(format, ...)                                        \
    do {                                                            \
        char buffer[1024];                                          \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);    \
        louis::log::LouisLog::getInstance().log(                    \
            louis::log::LogLevel::TRACE, __FILE__, __LINE__, buffer \
        );                                                          \
    } while (0)

#define DEBUG_F(format, ...)                                        \
    do {                                                            \
        char buffer[1024];                                          \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);    \
        louis::log::LouisLog::getInstance().log(                    \
            louis::log::LogLevel::DEBUG, __FILE__, __LINE__, buffer \
        );                                                          \
    } while (0)

#define INFO_F(format, ...)                                        \
    do {                                                           \
        char buffer[1024];                                         \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);   \
        louis::log::LouisLog::getInstance().log(                   \
            louis::log::LogLevel::INFO, __FILE__, __LINE__, buffer \
        );                                                         \
    } while (0)

#define WARN_F(format, ...)                                        \
    do {                                                           \
        char buffer[1024];                                         \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);   \
        louis::log::LouisLog::getInstance().log(                   \
            louis::log::LogLevel::WARN, __FILE__, __LINE__, buffer \
        );                                                         \
    } while (0)

#define ERROR_F(format, ...)                                        \
    do {                                                            \
        char buffer[1024];                                          \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);    \
        louis::log::LouisLog::getInstance().log(                    \
            louis::log::LogLevel::ERROR, __FILE__, __LINE__, buffer \
        );                                                          \
    } while (0)

#define FATAL_F(format, ...)                                        \
    do {                                                            \
        char buffer[1024];                                          \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);    \
        louis::log::LouisLog::getInstance().log(                    \
            louis::log::LogLevel::FATAL, __FILE__, __LINE__, buffer \
        );                                                          \
    } while (0)
