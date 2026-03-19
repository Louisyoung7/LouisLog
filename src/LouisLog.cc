#include "LouisLog.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace louis {
namespace log {
// 获取日志单例
LouisLog& LouisLog::getInstance() {
    // Meyers 单例：静态局部变量的首次初始化是线程安全的
    static LouisLog instance;
    return instance;
}

// 初始化日志对象
void LouisLog::init(LogLevel level, LogTarget target, std::string logFile, size_t maxFileSize) {
    std::lock_guard<std::mutex> lock(mutex_);

    level_ = level;
    target_ = target;
    logFile_ = logFile;
    maxFileSize_ = maxFileSize;

    // 如果输出目标包含文件，则打开文件
    if (target_ == LogTarget::FILE || target_ == LogTarget::BOTH) {
        openLogFile();
    }
}

// 日志写入
void LouisLog::log(LogLevel level, const std::string& file, int line, const std::string& msg) {
    // 判断日志级别
    if (level < level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // 构建日志信息
    std::string timestamp = getTimestamp();
    std::string levelStr = getLevelString(level);
    std::string threadId = getThreadId();

    std::string logMessage =
        "[" + timestamp + "] [" + levelStr + "] [" + file + "] [" + std::to_string(line) + "] [" + threadId + "]" + msg;

    // 输出到终端
    if (target_ == LogTarget::CONSOLE || target_ == LogTarget::BOTH) {
        // 根据级别选择输出流
        if (level_ >= LogLevel::ERROR) {
            std::cerr << logMessage << std::endl;
        } else {
            std::cout << logMessage << std::endl;
        }
    }

    // 输出到文件
    if (target_ == LogTarget::FILE || target_ == LogTarget::BOTH) {
        // 检查文件大小，需要时翻滚
        checkAndRollLog();

        if (fileStream_.is_open()) {
            fileStream_ << logMessage << std::endl;
            // 刷新缓冲区
            fileStream_.flush();
        }
    }
}

// 设置日志级别
void LouisLog::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);

    level_ = level;
}

// 设置日志输出目标
void LouisLog::setTarget(LogTarget target) {
    std::lock_guard<std::mutex> lock(mutex_);

    target_ = target;

    // 如果日志输出目标包含文件，则打开文件
    if (target_ == LogTarget::FILE || target_ == LogTarget::BOTH) {
        openLogFile();
    }
    // 如果不包含文件，则关闭文件
    else if (fileStream_.is_open()) {
    }
}

// 设置日志输出文件
void LouisLog::setLogFile(const std::string& logFile) {
    std::lock_guard<std::mutex> lock(mutex_);

    logFile_ = logFile;

    // 如果输出目标包含文件，关闭并打开新文件
    if ((target_ == LogTarget::FILE || target_ == LogTarget::BOTH) && fileStream_.is_open()) {
        fileStream_.close();
        openLogFile();
    }
}

// 设置日志文件最大大小
void LouisLog::setMaxSize(size_t maxSize) {
    std::lock_guard<std::mutex> lock(mutex_);

    maxFileSize_ = maxSize;
}

// 获取时间戳
std::string LouisLog::getTimestamp() {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();

    // 将时间点转换为C风格的time_t类型
    auto now_c = std::chrono::system_clock::to_time_t(now);

    // 获取毫秒
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // 将时间戳输出到字符串流
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S") << "." << std::setw(3) << std::setfill('0')
       << ms.count();

    return ss.str();
}

// 获取日志级别对应的字符串
std::string LouisLog::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:
            return "TRACE";
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}

// 获取线程ID
std::string LouisLog::getThreadId() {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

// 检查文件大小，判断是否翻滚文件
void LouisLog::checkAndRollLog() {
    if (!fileStream_.is_open()) {
        return;
    }

    // 获取文件大小
    fileStream_.seekp(0, std::ios_base::end);
    size_t currentSize = fileStream_.tellp();

    // 检查是否需要翻滚
    if (currentSize >= maxFileSize_) {
        // 获取时间戳
        std::string timestamp = getTimestamp();
        // 替换时间戳的分隔符，使其符合文件名规范
        for (auto& c : timestamp) {
            if (c == ' ' || c == ':') {
                c = '-';
            }
        }

        // 创建文件名
        std::string rolledFileName = logFile_ + "." + timestamp;

        // 重命名当前文件
        if (fileStream_.good()) {
            fileStream_.close();
            std::rename(logFile_.c_str(), rolledFileName.c_str());
        }

        // 打开新文件
        openLogFile();
    }
}

// 打开日志文件
void LouisLog::openLogFile() {
    // 关闭已打开的文件
    if (fileStream_.is_open()) {
        fileStream_.close();
    }

    // 重新以追加模式打开文件
    fileStream_.open(logFile_, std::ios_base::out | std::ios_base::app);
    if (!fileStream_.is_open()) {
        std::cerr << "Failed to open log file: " << logFile_ << std::endl;
    }
}

}  // namespace log
}  // namespace louis
