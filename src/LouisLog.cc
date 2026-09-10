#include "LouisLog.h"

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace louis {
namespace log {
constexpr static size_t flushIntervalMs = 3000;  // 3秒刷新一次

LouisLog& LouisLog::getInstance() {
    // Meyers 单例：静态局部变量的首次初始化是线程安全的
    static LouisLog instance;
    return instance;
}

LouisLog::~LouisLog() {
    stop();

    // 关闭文件流
    if (fileStream_.is_open()) fileStream_.close();
}

void LouisLog::init(LogLevel level, LogTarget target, std::string logFile, size_t maxFileSize) {
    // 重新初始化会重启日志线程
    if (initialized_) {
        stopped_.store(true);
        cv_worker_.notify_all();
        if (workerThread_.joinable()) workerThread_.join();
        stopped_.store(false);

        // 此时 worker 已死，关掉旧文件流是安全的；
        // 否则新 worker 看到 is_open() 为真，会继续往旧文件写
        if (fileStream_.is_open()) fileStream_.close();
    }
    std::lock_guard<std::mutex> lock(mutex_);

    level_ = level;
    target_ = target;
    logFile_ = logFile;
    maxFileSize_ = maxFileSize;

    // 启动后台线程
    workerThread_ = std::thread(&LouisLog::writeLoop, this);

    initialized_ = true;
}

// 日志写入
void LouisLog::log(LogLevel level, const std::string& file, int line, const std::string& msg) {
    if (level < level_.load(std::memory_order_relaxed)) return;

    // 获取日志信息
    std::string timestamp = getTimestamp();
    std::string levelString = getLevelString(level);
    std::string threadId = getThreadId();

    // 前端线程、锁外格式化日志消息
    std::string logMessage = "[" + timestamp + "] [" + levelString + "] [" + file + "] [" +
                             std::to_string(line) + "] [" + threadId + "]" + msg;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingLogs_.emplace_back(std::move(logMessage));
    }
    cv_worker_.notify_one();

    if (level == LogLevel::FATAL) flush();
}

void LouisLog::setLevel(LogLevel level) { level_ = level; }

void LouisLog::setTarget(LogTarget target) { target_ = target; }

void LouisLog::setLogFile(const std::string& logFile) {
    std::lock_guard<std::mutex> lock(mutex_);

    logFile_ = logFile;
}

void LouisLog::setMaxSize(size_t maxSize) { maxFileSize_.store(maxSize); }

void LouisLog::flush() {
    std::unique_lock<std::mutex> lock(mutex_);
    // 等待所有日志写入完成
    cv_written_.wait(lock, [this]() { return pendingLogs_.empty() && !writing_; });

    LogTarget target = target_;
    if (target == LogTarget::CONSOLE || target == LogTarget::BOTH) {
        std::cout.flush();
    }

    if (fileStream_.is_open()) {
        fileStream_.flush();
    }
}

void LouisLog::stop() {
    // 刷新日志队列
    flush();

    // 停止后台线程
    stopped_.store(true, std::memory_order_release);

    // 通知后台线程停止
    cv_worker_.notify_all();
    if (workerThread_.joinable()) workerThread_.join();
}

// 获取时间戳
std::string LouisLog::getTimestamp() const {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();

    // 将时间点转换为C风格的time_t类型
    auto now_c = std::chrono::system_clock::to_time_t(now);

    // 获取毫秒
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // 将时间戳输出到字符串流
    std::stringstream ss;
    std::tm tm{};
    ss << std::put_time(localtime_r(&now_c, &tm), "%Y-%m-%d %H:%M:%S") << "." << std::setw(3)
       << std::setfill('0') << ms.count();

    return ss.str();
}

// 获取日志级别对应的字符串
std::string LouisLog::getLevelString(LogLevel level) const {
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
std::string LouisLog::getThreadId() const {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

void LouisLog::openLogFile(const std::string& logFile) {
    // 关闭已打开的文件
    if (fileStream_.is_open()) fileStream_.close();

    // 重新以追加模式打开文件
    fileStream_.open(logFile, std::ios_base::out | std::ios_base::app);
    if (!fileStream_.is_open()) {
        std::cerr << "Failed to open log file: " << logFile << std::endl;
        // 如果日志目标只是文件，终止程序
        // 否则，继续执行
        if (target_ == LogTarget::FILE) std::terminate();
    }
}

// 检查文件大小，判断是否翻滚文件
void LouisLog::checkAndRollLog(const std::string& logFile) {
    if (!fileStream_.is_open()) return;

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

        // 创建文件名；同一毫秒可能发生多次翻滚，若目标文件已存在则追加序号，
        // 否则 std::rename 会静默覆盖同名文件，导致日志丢失
        std::string rolledFileName = logFile + "." + timestamp;
        for (int i = 1; std::filesystem::exists(rolledFileName); ++i) {
            rolledFileName = logFile + "." + timestamp + "." + std::to_string(i);
        }

        // 重命名当前文件
        if (fileStream_.good()) {
            fileStream_.close();
            std::rename(logFile.c_str(), rolledFileName.c_str());
        }

        // 打开新文件
        openLogFile(logFile);
    }
}

void LouisLog::writeLoop() {
    std::vector<std::string> batch;
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        // 等待有日志可写
        cv_worker_.wait_for(lock, std::chrono::milliseconds(flushIntervalMs), [this]() {
            return !pendingLogs_.empty() || stopped_;
        });

        // 从队列中获取日志
        batch.assign(
            std::make_move_iterator(pendingLogs_.begin()),
            std::make_move_iterator(pendingLogs_.end())
        );
        pendingLogs_.clear();

        // 获取当前日志目标与日志文件名
        LogTarget target = target_.load();
        std::string logFile = logFile_;

        writing_ = true;
        lock.unlock();

        outputBatch(batch, target, logFile);  // I/O 全在锁外，前端可以继续入队
        batch.clear();

        lock.lock();
        writing_ = false;
        // 通知I/O写入完成
        cv_written_.notify_all();

        if (stopped_ && pendingLogs_.empty()) break;
    }
}

void LouisLog::outputBatch(std::vector<std::string> msgs, LogTarget target, std::string logFile) {
    // 输出到终端
    if (target == LogTarget::CONSOLE || target == LogTarget::BOTH) {
        for (const auto& msg : msgs) {
            std::cout << msg << '\n';
        }
    }

    // 输出到文件
    if (target == LogTarget::FILE || target == LogTarget::BOTH) {
        // 打开文件
        if (!fileStream_.is_open()) openLogFile(logFile);

        // 检查文件大小，需要时翻滚
        checkAndRollLog(logFile);

        if (fileStream_.is_open()) {
            for (const auto& msg : msgs) {
                fileStream_ << msg << '\n';
            }
        }
    }

    std::cout.flush();
    if (fileStream_.is_open()) fileStream_.flush();

    // 如果日志目标变化了，关闭文件
    if (target_ == LogTarget::CONSOLE) fileStream_.close();
}
}  // namespace log
}  // namespace louis
