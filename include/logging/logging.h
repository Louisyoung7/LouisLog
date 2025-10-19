/**
 * @file logging.h
 * @brief 日志系统核心API
 * @details 利用含参数的宏提供简明的日志系统API，支持不同级别的日志输出
 * @author Louis
 * @date 2025-10-4
 * @version 1.0.0
 */
#pragma once

#include "logger.h"

namespace louis {
    namespace logging {
#define debug(format, ...)\
Logger::getInstance()->log(Level::DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__);
#define info(format, ...)\
Logger::getInstance()->log(Level::INFO, __FILE__, __LINE__, format, ##__VA_ARGS__);
#define warn(format, ...)\
Logger::getInstance()->log(Level::WARN, __FILE__, __LINE__, format, ##__VA_ARGS__);
#define error(format, ...)\
Logger::getInstance()->log(Level::ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__);
#define fatal(format, ...)\
Logger::getInstance()->log(Level::FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__);
    } // namespace logging
} // namespace louis
