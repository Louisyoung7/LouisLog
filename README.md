# LouisLog

一个轻量级、高性能的C++日志库，提供了灵活的日志配置和输出选项。

## 特性

- **单例模式**：全局唯一的日志实例，方便在整个应用中使用
- **异步日志**：前端线程仅格式化并入队，后台线程批量落盘，日志 I/O 不阻塞业务线程
- **多日志级别**：支持 TRACE、DEBUG、INFO、WARN、ERROR、FATAL 六个级别的日志
- **多输出目标**：支持输出到控制台、文件或同时输出到两者
- **日志文件翻滚**：基于文件大小的自动翻滚，同毫秒多次翻滚自动追加序号，避免文件被覆盖
- **多线程安全**：支持多线程并发写入，级别/目标/文件大小等配置可随时原子热更新
- **可靠的退出保障**：FATAL 级别自动阻塞等待落盘，`flush()`/`stop()` 确保退出前不丢日志
- **类型安全的格式化**：基于 C++20 `std::format` 的函数式 API，格式串编译期检查，
  无宏依赖，调用点文件/行号经 `std::source_location` 自动捕获
- **详细的日志信息**：每条日志包含时间戳、线程ID、日志级别、文件名、行号等信息

## 安装

### 依赖

- C++20 或更高版本（实现使用了 `std::format`、`std::source_location`，需 GCC 13+ / Clang 16+）
- CMake 3.28 或更高版本
- POSIX 线程库（CMake 自动检测并链接）
- Google Test (仅用于测试)

### 构建

```bash
# 克隆仓库
git clone https://github.com/Louisyoung7/LouisLog.git
cd LouisLog

# 创建构建目录
mkdir build && cd build

# 配置并构建（建议 Release，见下方基准测试说明）
cmake -DCMAKE_BUILD_TYPE=Release .. && make
```

## 使用示例

### 基本使用

```cpp
#include "LouisLog.h"

using namespace louis::log;

int main() {
    // 初始化日志（默认配置：INFO级别，输出到控制台）
    LouisLog::getInstance().init();

    // 直接调用函数输出不同级别的日志，无需宏
    trace("这是一条TRACE级别的日志");
    debug("这是一条DEBUG级别的日志");
    info("这是一条INFO级别的日志");
    warn("这是一条WARN级别的日志");
    error("这是一条ERROR级别的日志");
    fatal("这是一条FATAL级别的日志");

    // 使用 std::format 风格的格式化参数（格式串编译期检查）
    info("Hello, {}! The answer is {}.", "world", 42);

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
    info("配置已更新");
    
    // 程序退出前确保所有日志落盘（析构时会自动调用 stop()，也可显式调用）
    LouisLog::getInstance().flush();
    
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
- **void flush()**：阻塞直到队列中所有待写日志输出完成
- **void stop()**：停止后台日志线程并排空队列（析构时自动调用）

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

### 日志函数

`using namespace louis::log;` 后直接调用，格式串为 `std::format` 风格（`{}` 占位），
编译期检查；文件/行号经 `std::source_location` 自动捕获，无需透传：

- **trace(format, args...)**：输出TRACE级别的日志
- **debug(format, args...)**：输出DEBUG级别的日志
- **info(format, args...)**：输出INFO级别的日志
- **warn(format, args...)**：输出WARN级别的日志
- **error(format, args...)**：输出ERROR级别的日志
- **fatal(format, args...)**：输出FATAL级别的日志（自动阻塞至落盘）

> 超大预构建消息（如 4KB 以上）想避免中间字符串的额外拷贝时，
> 可直接调用底层 `log(level, file, line, msg)`（详见 ADR-0002）。

## 日志格式

每条日志的格式如下：

```
[2026-09-19 18:30:00.123][INFO][main.cc][42][140234512316160] This is a log message
```

- **时间戳**：年-月-日 时:分:秒.毫秒
- **日志级别**：日志的级别
- **文件位置**：日志产生的源文件与行号
- **线程ID**：当前线程的ID
- **日志内容**：日志的具体内容

## 性能特性

- **异步写入**：`log()` 仅做格式化与入队，控制台/文件 I/O 由后台线程批量完成，且 I/O 全程在锁外执行，前端可无阻塞继续入队
- **低开销级别过滤**：级别以原子变量存储，过滤判断先于任何格式化执行，
  被过滤日志仅付一次原子读的代价（实测约 8 亿条/秒）
- **原子热配置**：级别/目标/文件大小可在运行中无锁修改
- **同步保障**：FATAL 日志自动阻塞至落盘，`stop()` 退出前排空队列

### 基准测试结果

在 Linux x86-64 上的吞吐量参考（**Release 构建**，详见 `benchmark/` 目录与 `bench_results.csv`；
未配置构建类型时 std::format 在 -O0 下性能失真，严禁以 Debug 数据对比）：

| 场景 | 吞吐量 |
|---|---|
| 单线程 64B 消息 | 约 134 万条/秒（0.748µs/条） |
| 单线程 4KB 消息 | 约 46 万条/秒 |
| 8 线程并发 1KB 消息 | 约 203 万条/秒 |
| 单线程级别过滤（无 I/O） | 约 8 亿条/秒 |

## 测试

项目包含 8 个测试用例（GTest），全部带输出断言：

- **LogLevels**：级别过滤边界（INFO 阈值下 trace/debug 被过滤，info 及以上各恰好一条）
- **LevelFilterRuntime**：运行时 setLevel 升/降阈值动态生效
- **LogFormat**：行结构 `[时间戳][级别][文件][行号][线程ID] 消息`、毫秒精度、单行
- **FatalSyncFlush**：FATAL 不显式 flush 也立即落盘
- **LogRolling**：文件翻滚产生多个文件且总条数不丢
- **MultiThreading**：5 线程 × 100 条并发写入，全部落盘且无交错损坏
- **LongMessageNoTruncation**：8KB 超长消息不截断
- **ReinitAfterStop**：stop 后重新 init，worker 线程正常重启

## 许可证

本项目采用 MIT 许可证。详见 LICENSE 文件。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进这个日志库。

## 联系方式

如有问题或建议，请联系项目维护者。