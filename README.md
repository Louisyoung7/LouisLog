# logging

一个基于C++11的轻量级日志库，采用单例模式设计，支持多种日志级别和日志文件滚动。

## 核心功能

- ✅ **日志级别**：支持DEBUG、INFO、WARN、ERROR、FATAL五个级别
- ✅ **单例设计**：全局唯一日志实例，方便使用
- ✅ **文件滚动**：支持按文件大小自动滚动日志文件
- ✅ **格式化输出**：包含时间、日志级别、文件名、行号等信息
- ✅ **宏定义简化**：提供debug/info/warn/error/fatal宏，使用更方便
- ✅ **异常处理**：文件操作异常处理

## 目录结构

```
logging/
├── include/                 # 头文件目录
│   ├── logging/            # 日志库核心头文件
│   │   ├── logger.h        # 日志类定义
│   │   └── logging.h       # 日志宏定义
│   └── patterns/           # 设计模式实现
│       └── singleton.h     # 单例模式模板
├── src/                    # 源文件目录
│   └── logger.cpp          # 日志类实现
├── test/                   # 测试目录
├── build/                  # 构建输出目录
└── CMakeLists.txt          # CMake构建文件
```

## 快速开始

### 构建步骤

1. **创建构建目录**
   ```bash
   mkdir -p build
   cd build
   ```

2. **运行CMake**
   ```bash
   cmake ..
   ```

3. **编译项目**
   ```bash
   make
   ```

### 基本使用

```cpp
#include "logging/logging.h"

int main() {
    // 打开日志文件
    louis::logging::Logger::getInstance().open("test.log");
    
    // 设置日志级别为INFO（仅输出INFO及以上级别日志）
    louis::logging::Logger::getInstance().setLevel(louis::logging::Level::INFO);
    
    // 设置日志文件最大大小为1MB
    louis::logging::Logger::getInstance().setMaxLen(1024 * 1024);
    
    // 使用日志宏记录日志
    debug("This is a debug message");  // 不会输出，因为日志级别设置为INFO
    info("This is an info message");    // 会输出
    warn("This is a warning message");  // 会输出
    error("This is an error message");  // 会输出
    fatal("This is a fatal message");  // 会输出
    
    // 关闭日志文件
    louis::logging::Logger::getInstance().close();
    
    return 0;
}
```

## API文档

### 日志级别枚举

```cpp
enum class Level {
    DEBUG = 0,  // 调试信息
    INFO,       // 普通信息
    WARN,       // 警告信息
    ERROR,      // 错误信息
    FATAL,      // 致命错误
    LEVEL_COUNT // 级别数量（用于内部计算）
};
```

### 核心方法

#### open(const std::string &fileName)
- **功能**：打开日志文件
- **参数**：
  - `fileName`：日志文件路径
- **异常**：文件打开失败时抛出`std::logic_error`

#### close()
- **功能**：关闭日志文件

#### setLevel(Level level)
- **功能**：设置日志级别，低于该级别的日志将被忽略
- **参数**：
  - `level`：日志级别枚举值

#### setMaxLen(int len)
- **功能**：设置日志文件最大长度，超过该长度将自动滚动
- **参数**：
  - `len`：日志文件最大长度（字节）

### 日志宏

| 宏名称  | 对应级别 | 功能描述               |
|---------|----------|------------------------|
| `debug` | DEBUG    | 输出调试信息           |
| `info`  | INFO     | 输出普通信息           |
| `warn`  | WARN     | 输出警告信息           |
| `error` | ERROR    | 输出错误信息           |
| `fatal` | FATAL    | 输出致命错误信息       |

## 注意事项

1. **线程安全性**：当前实现非线程安全，多线程环境下使用需谨慎
2. **异常处理**：文件操作可能抛出异常，建议捕获处理
3. **性能考虑**：频繁的日志输出可能影响性能，建议合理设置日志级别
4. **文件权限**：确保程序有日志文件所在目录的读写权限
5. **C++11支持**：项目依赖C++11特性，编译时需指定`-std=c++11`或更高标准

## 许可证

MIT License

## 作者

louis
