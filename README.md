# LouisLog

一个轻量级、高性能的C++日志库，提供了灵活的日志配置和输出选项。

## 特性

- **单例模式**：全局唯一的日志实例，方便在整个应用中使用
- **多日志级别**：支持 TRACE、DEBUG、INFO、WARN、ERROR、FATAL 六个级别的日志
- **多输出目标**：支持输出到控制台、文件或同时输出到两者
- **日志文件翻滚**：基于文件大小的自动翻滚功能
- **多线程安全**：支持多线程并发写入日志
- **便捷的宏定义**：提供了简单易用的日志宏，支持可变参数格式化
- **详细的日志信息**：每条日志包含时间戳、线程ID、日志级别、文件名、行号等信息

## 安装

### 依赖

- C++14 或更高版本
- CMake 3.10 或更高版本
- Google Test (仅用于测试)

### 构建

```bash
# 克隆仓库
git clone https://github.com/Louisyoung7/LouisLog.git
cd LouisLog

# 创建构建目录
mkdir build && cd build

# 配置并构建
cmake .. && make
```

## 使用示例

### 基本使用

```cpp
#include "LouisLog.h"

using namespace louis::log;

int main() {
    // 初始化日志（默认配置：INFO级别，输出到控制台）
    LouisLog::getInstance().init();
    
    // 使用日志宏输出不同级别的日志
    TRACE("这是一条TRACE级别的日志");
    DEBUG("这是一条DEBUG级别的日志");
    INFO("这是一条INFO级别的日志");
    WARN("这是一条WARN级别的日志");
    ERROR("这是一条ERROR级别的日志");
    FATAL("这是一条FATAL级别的日志");
    
    // 使用带格式化参数的日志宏
    INFO_F("Hello, %s! The answer is %d.", "world", 42);
    
    return 0;
}
```

### 高级配置

```cpp
#include "LouisLog.h"

using namespace louis::log;

int main() {
    // 自定义配置：DEBUG级别，同时输出到控制台和文件，文件大小限制为1MB
    LouisLog::getInstance().init(
        LogLevel::DEBUG,      // 日志级别
        LogTarget::BOTH,      // 输出目标
        "app.log",            // 日志文件名
        1024 * 1024           // 日志文件最大大小（字节）
    );
    
    // 动态修改配置
    LouisLog::getInstance().setLevel(LogLevel::INFO);
    LouisLog::getInstance().setTarget(LogTarget::FILE);
    LouisLog::getInstance().setLogFile("new_app.log");
    LouisLog::getInstance().setMaxSize(2 * 1024 * 1024);
    
    // 输出日志
    INFO("配置已更新");
    
    return 0;
}
```

## API 参考

### 核心类

#### LouisLog

- **static LouisLog& getInstance()**：获取日志单例实例
- **void init(LogLevel level = LogLevel::INFO, LogTarget target = LogTarget::CONSOLE, std::string logFile = "app.log", size_t maxFileSize = 1024 * 1024)**：初始化日志
- **void log(LogLevel level, const std::string& file, int line, const std::string& msg)**：写入日志
- **void setLevel(LogLevel level)**：设置日志级别
- **void setTarget(LogTarget target)**：设置输出目标
- **void setLogFile(const std::string& logFile)**：设置日志文件
- **void setMaxSize(size_t maxSize)**：设置日志文件最大大小

### 日志级别

- **LogLevel::TRACE**：最详细的日志级别，通常用于调试
- **LogLevel::DEBUG**：调试信息，用于开发阶段
- **LogLevel::INFO**：普通信息，记录程序运行状态
- **LogLevel::WARN**：警告信息，可能的问题但不影响程序运行
- **LogLevel::ERROR**：错误信息，程序出现错误但可以继续运行
- **LogLevel::FATAL**：致命错误，程序无法继续运行

### 输出目标

- **LogTarget::CONSOLE**：仅输出到控制台
- **LogTarget::FILE**：仅输出到文件
- **LogTarget::BOTH**：同时输出到控制台和文件

### 日志宏

#### 基本宏

- **TRACE(message)**：输出TRACE级别的日志
- **DEBUG(message)**：输出DEBUG级别的日志
- **INFO(message)**：输出INFO级别的日志
- **WARN(message)**：输出WARN级别的日志
- **ERROR(message)**：输出ERROR级别的日志
- **FATAL(message)**：输出FATAL级别的日志

#### 带格式化参数的宏

- **TRACE_F(format, ...)**：带格式化参数的TRACE级别日志
- **DEBUG_F(format, ...)**：带格式化参数的DEBUG级别日志
- **INFO_F(format, ...)**：带格式化参数的INFO级别日志
- **WARN_F(format, ...)**：带格式化参数的WARN级别日志
- **ERROR_F(format, ...)**：带格式化参数的ERROR级别日志
- **FATAL_F(format, ...)**：带格式化参数的FATAL级别日志

## 日志格式

每条日志的格式如下：

```
[2024-01-01 12:00:00.000] [Thread 12345] [INFO] [file.cpp:42] This is a log message
```

- **时间戳**：年-月-日 时:分:秒.毫秒
- **线程ID**：当前线程的ID
- **日志级别**：日志的级别
- **文件位置**：日志产生的文件和行号
- **日志内容**：日志的具体内容

## 性能特性

- **线程安全**：使用互斥锁确保多线程环境下的安全
- **高效IO**：文件写入使用缓冲，减少IO操作
- **内存优化**：日志消息格式化使用固定大小的缓冲区，避免频繁内存分配

## 测试

项目包含了以下测试用例：

- **LogLevels**：测试不同级别的日志输出
- **LogRolling**：测试日志文件翻滚功能
- **MultiThreading**：测试多线程并发写入日志

## 许可证

本项目采用 MIT 许可证。详见 LICENSE 文件。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进这个日志库。

## 联系方式

如有问题或建议，请联系项目维护者。