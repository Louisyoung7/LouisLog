/**
 * @file logging_test.cpp
 * @brief 测试日志功能的头文件
 * @details 包含日志系统测试函数的声明，用于验证日志记录功能是否正常工作
 * @author Louis
 * @date 2025-10-4
 * @version 1.0.0
 */

#include "logging/logger.h"
#include "logging/logging.h"
using namespace louis::logging;

int main() {
    // 新建并打开文件
    Logger::getInstance()->open("./logging_test.log");
    // 设置文件级别
    Logger::getInstance()->setLevel(Level::INFO);
    // 设置日志最大长度
    Logger::getInstance()->setMaxLen(1024);

    // // 基本功能测试
    // // 大于等于当前日志级别
    // Logger::getInstance()->log(Level::INFO, __FILE__, __LINE__, "This is a test log.");
    // Logger::getInstance()->log(Level::WARN, __FILE__, __LINE__, "This is a test log.");
    // // 小于当前日志级别，不输出
    // Logger::getInstance()->log(Level::DEBUG, __FILE__, __LINE__, "This is a test log.");

    // // 宏测试（基本）
    // // 小于当前日志级别，不输出
    // debug("This is a test log.");
    // // 大于等于当前日志级别
    // info("This is a test log.");
    // warn("This is a test log.");
    // error("This is a test log.");
    // fatal("This is a test log.");

    // 宏测试（带参数）
    // 小于当前日志级别，不输出
    debug("This is a test log. %d %s", 123, "abc");
    // 大于等于当前日志级别
    info("This is a test log. %d %s", 123, "abc");
    warn("This is a test log. %d %s", 123, "abc");
    error("This is a test log. %d %s", 123, "abc");
    fatal("This is a test log. %d %s", 123, "abc");
}