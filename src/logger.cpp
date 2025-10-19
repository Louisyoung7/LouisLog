/**
 * @file logger.cpp
 * @brief 日志系统核心实现文件
 * @details 实现Logger类的核心功能，包括日志级别控制、输出格式化和多目标输出支持
 * @author Louis
 * @date 2025-10-3
 * @version 1.0.0
 */

#include <iostream>
#include <cstdarg>
#include <cstring>

#include "logging/logger.h"
using namespace louis::logging;

// 将枚举转换为整型，方便使用LEVEL_COUNT
constexpr int Logger::toInt(Level level) {
    return static_cast<int>(level);
}

// 初始化与枚举对应的字符串数组
const char* Logger::m_levelStr[toInt(Level::LEVEL_COUNT)] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

// 打开日志文件
void Logger::open(const std::string &fileName) {
    // 记录文件名
    m_fileName = fileName;
    // 追加模式打开文件
    m_file.open(fileName,std::ios::app);
    // 异常处理
    if (m_file.fail()) {
        throw std::logic_error(std::string("open file failed") + fileName);
    }
    // 将文件指针定位到文件末尾，准备追加写入
    m_file.seekp(0,std::ios::end);
    // 获取当前文件指针位置（在文件末尾），即文件长度
    m_len = m_file.tellp();
}

// 关闭日志文件
void Logger::close() {
    m_file.close();
}

// 记录日志（核心业务方法）
void Logger::log(Level level, const char* fileName, int line, const char* format, ...) {
    // 判断日志级别，避免大量低级别日志输出
    if (level < m_level) {
        return;
    }

    if (m_file.fail()) {
        throw std::logic_error(std::string("open file failed") + m_fileName);
    }

    // 获取当前时间
    time_t now = time(nullptr);
    tm *localTime = localtime(&now);
    // 创建时间字符串
    char timeStr[32] = {};  // 零初始化
    // 格式化时间
    strftime(timeStr,sizeof(timeStr),"%Y-%m-%d %H:%M:%S",localTime);

    // 格式化日志内容（时间 级别 文件：行号）
    const char* fmt = "%s\t\t%s\t\t%s: %d ";
    // 利用snprintf()函数的特性，获取格式化后的日志所需要的缓冲区大小
    int size = snprintf(nullptr,0,fmt,timeStr,m_levelStr[toInt(level)],fileName,line);
    if (size > 0) {
        char* buf = new char[size + 1];
        snprintf(buf,size + 1,fmt,timeStr,m_levelStr[toInt(level)],fileName,line);
        buf[size] = '\0';
        m_file << buf << std::endl;
        delete[] buf;
        m_len += size;  //C++文件流不会将'\0'输入到文件
    }

    // 格式化可变参数
    va_list args;
    va_start(args,format);
    // 利用vsnprintf()函数的特性，获取格式化后的可变参数所需要的缓冲区大小
    size = vsnprintf(nullptr,0,format,args);
    va_end(args);
    if (size > 0) {
        char* buf = new char[size + 1];
        va_start(args,format);
        vsnprintf(buf,size + 1,format,args);
        va_end(args);
        m_file << buf << std::endl;
        m_len += size;
        delete[] buf;
    }
    m_file << std::endl;    // 刷新缓冲区

    // 判断当前文件长度
    if (m_maxLen > 0 && m_len >= m_maxLen) {
        rotation();
    }
}

// 设置日志级别
void Logger::setLevel(Level level) {
    m_level = level;
}

// 设置日志最大长度
void Logger::setMaxLen(int len) {
    m_maxLen = len;
}

// 日志翻滚
void Logger::rotation() {
    // 关闭当前日志文件
    close();
    // 获取当前时间
    time_t now = time(nullptr);
    tm *localTime = localtime(&now);
    char timeStr[32] = {};
    strftime(timeStr,sizeof(timeStr),"%Y-%m-%d_%H-%M-%S",localTime);
    std::string newFileName = m_fileName + "(" + timeStr + ")";
    // 将写满的日志文件重命名为带时间戳的文件
    if(rename(m_fileName.c_str(),newFileName.c_str()) != 0) {
        throw std::logic_error("rename log file failed: " + std::string(strerror(errno)));
    }
    // 打开新文件，文件名仍然用旧文件名
    open(m_fileName);
}
