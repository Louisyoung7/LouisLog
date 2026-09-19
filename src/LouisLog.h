#pragma once

#include <atomic>
#include <concepts>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <format>
#include <fstream>
#include <mutex>
#include <source_location>
#include <string>
#include <thread>
#include <type_traits>
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

    bool shouldLog(LogLevel level) const;

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

// 包装器：在调用点捕获 source_location。
// 不能直接给变参函数加尾置默认实参（fmt, args..., loc = current()）：
// 变参包推导时会吞掉全部剩余实参，带默认值的尾参无法参与推导，编译报
// no matching function。cppreference 推荐用此模式绕过：
// loc 的求值发生在包装器构造函数的默认实参中，仍处于用户调用点。
template <typename... Args>
struct with_source_location {
    std::format_string<Args...> fmt;
    std::source_location loc;

    // 构造参数必须用 U&& 而非 std::format_string<Args...>：
    // 后者会造成 字面量->format_string->包装器 连续两次用户自定义转换，
    // 隐式转换序列非法；U&& 让字面量直接绑定参数，format_string 的构造在函数体内完成
    template <typename U>
        requires std::constructible_from<std::format_string<Args...>, U&&>
    consteval with_source_location(U&& u, std::source_location l = std::source_location::current())
        : fmt(std::forward<U>(u)), loc(l) {}
};

namespace detail {
// 统一核心：级别过滤在格式化前，I/O 仍由 LouisLog 后台线程异步完成
template <typename... Args>
void emit(LogLevel level, with_source_location<std::type_identity_t<Args>...> f, Args&&... args) {
    auto& logger = LouisLog::getInstance();
    if (!logger.shouldLog(level)) return;  // 过滤在格式化前
    logger.log(
        level, f.loc.file_name(), static_cast<int>(f.loc.line()),
        std::format(f.fmt, std::forward<Args>(args)...)
    );  // 编译期检查格式串
}
}  // namespace detail

// 用户面 API：无宏，编译期格式串检查，自动记录调用点的文件/行号。
// Args 用 type_identity_t 禁止从第一参（字符串字面量）推导，只从变参包推导，
// 否则 GCC 会拿字面量去匹配类模板特化，推导直接失败
template <typename... Args>
void trace(with_source_location<std::type_identity_t<Args>...> f, Args&&... args) {
    detail::emit(LogLevel::TRACE, std::move(f), std::forward<Args>(args)...);
}

template <typename... Args>
void debug(with_source_location<std::type_identity_t<Args>...> f, Args&&... args) {
    detail::emit(LogLevel::DEBUG, std::move(f), std::forward<Args>(args)...);
}

template <typename... Args>
void info(with_source_location<std::type_identity_t<Args>...> f, Args&&... args) {
    detail::emit(LogLevel::INFO, std::move(f), std::forward<Args>(args)...);
}

template <typename... Args>
void warn(with_source_location<std::type_identity_t<Args>...> f, Args&&... args) {
    detail::emit(LogLevel::WARN, std::move(f), std::forward<Args>(args)...);
}

template <typename... Args>
void error(with_source_location<std::type_identity_t<Args>...> f, Args&&... args) {
    detail::emit(LogLevel::ERROR, std::move(f), std::forward<Args>(args)...);
}

template <typename... Args>
void fatal(with_source_location<std::type_identity_t<Args>...> f, Args&&... args) {
    detail::emit(LogLevel::FATAL, std::move(f), std::forward<Args>(args)...);
}

}  // namespace log
}  // namespace louis
