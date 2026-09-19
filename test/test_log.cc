#include <gtest/gtest.h>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "LouisLog.h"

using namespace louis::log;

namespace {
// 读取整个文件
std::string readAll(const std::string& file) {
    std::ifstream in(file);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// 统计子串出现次数
int countNeedle(const std::string& content, const std::string& needle) {
    int count = 0;
    for (size_t pos = content.find(needle); pos != std::string::npos;
         pos = content.find(needle, pos + needle.size())) {
        ++count;
    }
    return count;
}
}  // namespace

class LogTest : public testing::Test {
   protected:
    void TearDown() override {
        LouisLog::getInstance().stop();

        // 清理测试产物（含翻滚产生的 name.*），避免污染仓库
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(".", ec)) {
            const std::string name = entry.path().filename().string();
            for (const auto& prefix : files_) {
                if (name == prefix || name.rfind(prefix + ".", 0) == 0) {
                    std::filesystem::remove(entry.path(), ec);
                    break;
                }
            }
        }
    }

    // 登记测试产物，并清除历史残留，保证条数类断言确定性
    std::string resetFile(const std::string& name) {
        std::error_code ec;
        std::filesystem::remove(name, ec);
        for (const auto& entry : std::filesystem::directory_iterator(".", ec)) {
            if (entry.path().filename().string().rfind(name + ".", 0) == 0) {
                std::filesystem::remove(entry.path(), ec);
            }
        }
        files_.push_back(name);
        return name;
    }

    std::vector<std::string> files_;
};

// 级别过滤边界：INFO 阈值下 trace/debug 被过滤，info 及以上输出
TEST_F(LogTest, LogLevels) {
    LouisLog& logger = LouisLog::getInstance();
    std::string file = resetFile("test_levels.log");
    logger.init(LogLevel::INFO, LogTarget::FILE, file, 1024 * 1024);

    trace("This is a TRACE level message");
    debug("This is a DEBUG level message");
    info("This is a INFO level message");
    warn("This is a WARN level message");
    error("This is a ERROR level message");
    fatal("This is a FATAL level message");
    logger.flush();

    std::string content = readAll(file);
    EXPECT_EQ(countNeedle(content, "TRACE level"), 0);
    EXPECT_EQ(countNeedle(content, "DEBUG level"), 0);
    EXPECT_EQ(countNeedle(content, "INFO level"), 1);
    EXPECT_EQ(countNeedle(content, "WARN level"), 1);
    EXPECT_EQ(countNeedle(content, "ERROR level"), 1);
    EXPECT_EQ(countNeedle(content, "FATAL level"), 1);
}

// 运行时 setLevel 动态生效
TEST_F(LogTest, LevelFilterRuntime) {
    LouisLog& logger = LouisLog::getInstance();
    std::string file = resetFile("test_filter.log");
    logger.init(LogLevel::INFO, LogTarget::FILE, file, 1024 * 1024);

    info("runtime info before change");
    logger.setLevel(LogLevel::ERROR);
    info("runtime info after raise");
    error("runtime error after raise");
    logger.flush();

    std::string content = readAll(file);
    EXPECT_EQ(countNeedle(content, "info before change"), 1);
    EXPECT_EQ(countNeedle(content, "info after raise"), 0);
    EXPECT_EQ(countNeedle(content, "error after raise"), 1);

    logger.setLevel(LogLevel::TRACE);
    trace("runtime trace after lower");
    logger.flush();
    EXPECT_EQ(countNeedle(readAll(file), "trace after lower"), 1);
}

// 行格式：[时间戳][级别][文件][行号][线程ID] 消息，单条日志占一行
TEST_F(LogTest, LogFormat) {
    LouisLog& logger = LouisLog::getInstance();
    std::string file = resetFile("test_format.log");
    logger.init(LogLevel::INFO, LogTarget::FILE, file, 1024 * 1024);

    info("format check {}", 42);
    logger.flush();

    std::string line = readAll(file);
    ASSERT_FALSE(line.empty());
    ASSERT_EQ(line.back(), '\n');
    line.pop_back();
    EXPECT_EQ(countNeedle(line, "\n"), 0);  // 仅一行

    EXPECT_EQ(line.front(), '[');
    EXPECT_NE(line.find("][INFO]["), std::string::npos);
    EXPECT_NE(line.find(".cc]["), std::string::npos);  // source_location 文件
    EXPECT_TRUE(line.ends_with("format check 42"));

    // 时间戳形如 2026-09-19 12:34:56.789（line[0] 是 '['，时间戳从 1 开始）
    ASSERT_GE(line.size(), 24u);
    EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(line[1])));
    EXPECT_EQ(line[5], '-');
    EXPECT_EQ(line[11], ' ');
    EXPECT_EQ(line[20], '.');  // 毫秒部分
}

// FATAL 不显式 flush 也必须立即落盘（同步刷新约定）
TEST_F(LogTest, FatalSyncFlush) {
    LouisLog& logger = LouisLog::getInstance();
    std::string file = resetFile("test_fatal.log");
    logger.init(LogLevel::WARN, LogTarget::FILE, file, 1024 * 1024);

    fatal("fatal sync flush check");

    EXPECT_EQ(countNeedle(readAll(file), "fatal sync flush check"), 1);
}

// 文件翻滚：产生多个文件且总条数不丢
TEST_F(LogTest, LogRolling) {
    LouisLog& logger = LouisLog::getInstance();
    std::string file = resetFile("test_roll.log");
    logger.init(LogLevel::INFO, LogTarget::FILE, file, 1024);

    for (int i = 0; i < 100; ++i) {
        info("Test log message {} for rolling test", i);
    }
    logger.flush();

    std::error_code ec;
    int fileCount = 0, total = 0;
    for (const auto& entry : std::filesystem::directory_iterator(".", ec)) {
        const std::string name = entry.path().filename().string();
        if (name.rfind("test_roll.log", 0) != 0) continue;
        ++fileCount;
        total += countNeedle(readAll(name), "rolling test");
    }
    EXPECT_GT(fileCount, 1);
    EXPECT_EQ(total, 100);
}

// 多线程并发：500 条全部落盘且无交错损坏
TEST_F(LogTest, MultiThreading) {
    LouisLog& logger = LouisLog::getInstance();
    std::string file = resetFile("test_thread.log");
    logger.init(LogLevel::INFO, LogTarget::FILE, file, 1024 * 1024);

    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 100; ++j) {
                info("Thread {}: Log message {}", i, j);
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    logger.flush();

    std::string content = readAll(file);
    EXPECT_EQ(countNeedle(content, "Log message"), 500);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(countNeedle(content, "Thread " + std::to_string(i) + ": Log message"), 100);
    }
}

// 超长消息不截断（旧 snprintf 1024 字节上限的回归防护）
TEST_F(LogTest, LongMessageNoTruncation) {
    LouisLog& logger = LouisLog::getInstance();
    std::string file = resetFile("test_long.log");
    logger.init(LogLevel::INFO, LogTarget::FILE, file, 1024 * 1024);

    std::string payload(8192, 'x');
    info("long message: LONG-HEAD-{}-LONG-TAIL", payload);
    logger.flush();

    std::string content = readAll(file);
    EXPECT_EQ(countNeedle(content, "LONG-HEAD-"), 1);
    EXPECT_EQ(countNeedle(content, "-LONG-TAIL"), 1);
    EXPECT_EQ(countNeedle(content, "long message: LONG-HEAD-" + payload + "-LONG-TAIL"), 1);
}

// stop 后重新 init：worker 线程重启，日志写入新文件，旧文件不再增长
TEST_F(LogTest, ReinitAfterStop) {
    LouisLog& logger = LouisLog::getInstance();
    std::string fileA = resetFile("test_reinit_a.log");
    std::string fileB = resetFile("test_reinit_b.log");

    logger.init(LogLevel::INFO, LogTarget::FILE, fileA, 1024 * 1024);
    info("before stop");
    logger.flush();
    logger.stop();
    EXPECT_EQ(countNeedle(readAll(fileA), "before stop"), 1);

    logger.init(LogLevel::INFO, LogTarget::FILE, fileB, 1024 * 1024);
    info("after reinit");
    logger.flush();
    EXPECT_EQ(countNeedle(readAll(fileB), "after reinit"), 1);
    EXPECT_EQ(countNeedle(readAll(fileA), "after reinit"), 0);
}
