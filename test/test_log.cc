#include <gtest/gtest.h>

#include <vector>
#include <thread>

#include "LouisLog.h"

using namespace louis::log;

class LogTest : public testing::Test {
   protected:
    void SetUp() override {
        LouisLog& logger = LouisLog::getInstance();
        logger.init();
    }
};

// 输出不同级别的日志
TEST_F(LogTest, LogLevels) {
    TRACE("This is a TRACE level message");
    DEBUG("This is a DEBUG level message");
    INFO("This is a INFO level message");
    WARN("This is a WARN level message");
    ERROR("This is a ERROR level message");
    FATAL("This is a FATAL level message");
}

// 测试日志翻滚
TEST_F(LogTest, LogRolling) {
    // 获取日志实例
    LouisLog& logger = LouisLog::getInstance();

    // 重新初始化
    logger.init(LogLevel::INFO, LogTarget::FILE, "test_roll.log", 1024);

    // 输出大量日志，触发翻滚
    for (int i = 0; i < 100; ++i) {
        INFO_F("Test log message %d for rolling test", i);
    }
}

// 测试多线程并发
TEST_F(LogTest, MultiThreading) {
    // 重新初始化
    LouisLog& logger = LouisLog::getInstance();
    logger.init(LogLevel::INFO, LogTarget::BOTH, "test_thread.log", 1024 * 1024);

    // 创建多个线程同时写日志
    std::vector<std::thread> threads;

    // 填充日志向量
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 10; ++j) {
                INFO_F("Thread %d: Log message %d", i, j);

                // 稍微延迟，增加并发冲突的可能性
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
}


