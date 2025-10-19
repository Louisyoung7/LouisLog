/**
 * @file logger.h
 * @brief 日志系统核心类定义
 * @details 包含Logger类的声明，提供日志记录的主要接口
 * @author Louis
 * @date 2025-10-3
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <fstream>

#include "patterns/singleton.h"

namespace louis {
    namespace logging {
        enum class Level {  // 枚举类，C++11支持
            DEBUG = 0,
            INFO,
            WARN,
            ERROR,
            FATAL,
            LEVEL_COUNT
        };
        
        // 日志类继承单例模板，单例类模板参数为日志类自身
        class Logger : public patterns::Singleton<Logger> {
            // 允许Singleton模板访问私有成员
            friend class Singleton<Logger>;
        public:
            // 打开日志文件
            void open(const std::string &fileName);

            // 关闭日志文件
            void close();

            // 记录日志（核心业务方法）
            void log(Level level, const char* fileName, int line, const char* format, ...);

            // 设置日志级别
            void setLevel(Level level);

            // 设置日志最大长度
            void setMaxLen(int len);

            // 禁用拷贝（显式声明，增强可读性）C++11支持
            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;
        private:
            // 私有构造：仅单例模板能通过getInstance()调用
            Logger() = default; // C++11支持

            // 私有析构：释放资源（仅在程序结束时由系统调用）
            ~Logger() {
                close();
            }

            // 将枚举转换为整型，方便使用LEVEL_COUNT
            static constexpr int toInt(Level level);    // C++11支持

            // 日志翻滚
            void rotation();

            std::string m_fileName; // 日志文件名
            std::ofstream m_file; // 日志文件流
            Level m_level = Level::DEBUG; // 日志级别
            int m_len = 0;  // 日志长度
            int m_maxLen = 1024;   // 日志最大长度
            static const char* m_levelStr[];
        };
    } // namespace logging
} // namespace louis

