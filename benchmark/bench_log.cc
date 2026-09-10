// ============================================================
// LouisLog 压测程序：测同步/异步日志的吞吐与延迟，供迭代前后对比
//
// 用法：./bench_log [--csv] [--tag <名称>]
//   --csv        以 CSV 格式输出到终端
//   --tag 名称   标记本次运行（如 sync / async），写入结果文件便于对比
//
// 结果同时追加到 bench_results.csv（含时间戳与 tag），日志写入 bench.log
// 测试项：单线程（64B/1KB/4KB 消息、级别过滤）、4/8 线程并发写入
// ============================================================
#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "LouisLog.h"

using namespace louis::log;

// ============================================================
// 配置
// ============================================================
constexpr int kDurationSec = 3;                // 多线程测试持续时间（秒）
constexpr int kSmallMsgSize = 64;              // 小消息大小（字节）
constexpr int kLargeMsgSize = 1024;            // 大消息大小（字节）
constexpr int kHugeMsgSize = 4096;             // 超大消息大小（字节）
constexpr const char* kLogFile = "bench.log";  // 压测日志文件（每次运行前删除）
constexpr const char* kResultsFile = "bench_results.csv";  // 结果文件（追加模式，跨版本对比用）

// ============================================================
// 工具
// ============================================================
std::string makeMessage(int size) {
    std::string msg;
    msg.reserve(size + 1);
    const char* pattern = "The quick brown fox jumps over the lazy dog. ";
    while (msg.size() < static_cast<size_t>(size)) {
        msg += pattern;
    }
    msg.resize(size);
    return msg;
}

// ============================================================
// 测试结果
// ============================================================
struct BenchResult {
    std::string label;
    int64_t count;
    double elapsedSec;
    double msgPerSec;
    double mbPerSec;
    double avgLatencyUs;

    void print() const {
        std::cout << "| " << std::left << std::setw(55) << label.substr(0, 55) << " |";
        std::cout << " " << std::right << std::setw(12) << std::fixed << std::setprecision(1)
                  << msgPerSec << "  " << std::setw(10) << std::setprecision(2) << mbPerSec
                  << " MB/s  ";
        if (avgLatencyUs > 0) {
            std::cout << std::setw(7) << std::setprecision(2) << avgLatencyUs << " us |";
        } else {
            std::cout << "   N/A  |";
        }
        std::cout << "\n";
    }

    void printCSV() const {
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "\"" << label << "\"," << elapsedSec << "," << count << "," << msgPerSec << ","
                  << mbPerSec << "," << avgLatencyUs << "\n";
    }
};

// ============================================================
// 单线程：固定消息数，测调用方耗时
// ============================================================
BenchResult benchSingle(
    const std::string& label, int count, const std::string& msg, LogLevel level = LogLevel::INFO
) {
    LouisLog& logger = LouisLog::getInstance();

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < count; ++i) {
        logger.log(level, __FILE__, __LINE__, msg);
    }
    auto end = std::chrono::steady_clock::now();

    // 注意：flush 在计时窗口之外——测量的是调用方（前端）的代价。
    // 若放进计时窗口，异步版本会把后台队列清空的等待时间计入，同步/异步对比失去意义。
    logger.flush();

    double sec = std::chrono::duration<double>(end - start).count();
    double mb = static_cast<double>(count) * msg.size() / (1024.0 * 1024.0);

    return BenchResult{
        label, count, sec, count / sec, mb / sec, sec * 1'000'000.0 / count,
    };
}

// ============================================================
// 多线程：固定时间，测总吞吐
// ============================================================
BenchResult benchMulti(
    const std::string& label, int threadCount, int durationSec, const std::string& msg,
    LogLevel level = LogLevel::INFO
) {
    LouisLog& logger = LouisLog::getInstance();
    std::atomic<bool> stopFlag{false};
    std::atomic<int64_t> totalCount{0};

    auto start = std::chrono::steady_clock::now();  // 实测：从起线程前开始计时

    std::vector<std::thread> workers;
    for (int t = 0; t < threadCount; ++t) {
        workers.emplace_back([&]() {
            while (!stopFlag.load(std::memory_order_relaxed)) {
                logger.log(level, __FILE__, __LINE__, msg);
                totalCount.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(durationSec));
    stopFlag.store(true, std::memory_order_relaxed);

    for (auto& w : workers) {
        w.join();
    }
    auto end = std::chrono::steady_clock::now();  // 实测：到所有线程退出为止

    logger.flush();  // 计时窗口之外

    int64_t count = totalCount.load(std::memory_order_relaxed);
    double sec = std::chrono::duration<double>(end - start).count();
    double mb = static_cast<double>(count) * msg.size() / (1024.0 * 1024.0);

    return BenchResult{
        label, count, sec, count / sec, mb / sec, 0.0,
    };
}

// ============================================================
// 单线程：级别过滤路径
// 级别设为 WARN 后发 INFO 日志，全部被级别检查拦下，
// 不做格式化/加锁/I/O，测最快路径的纯内存开销
// ============================================================
BenchResult benchFiltered(const std::string& label, int count, const std::string& msg) {
    LouisLog& logger = LouisLog::getInstance();
    logger.setLevel(LogLevel::WARN);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < count; ++i) {
        logger.log(LogLevel::INFO, __FILE__, __LINE__, msg);
    }
    auto end = std::chrono::steady_clock::now();

    // 恢复级别
    logger.setLevel(LogLevel::TRACE);

    double sec = std::chrono::duration<double>(end - start).count();
    double mb = static_cast<double>(count) * msg.size() / (1024.0 * 1024.0);

    return BenchResult{
        label, count, sec, count / sec, mb / sec, sec * 1'000'000.0 / count,
    };
}

// ============================================================
// 结果追加写入 CSV 文件（文件不存在或为空时自动写表头）
// ============================================================
void writeResultsToFile(const std::vector<BenchResult>& results, const std::string& tag) {
    std::ofstream out(kResultsFile, std::ios_base::app);
    if (!out.is_open()) {
        std::cerr << "Failed to open results file: " << kResultsFile << "\n";
        return;
    }

    // 文件为空（含刚创建）时写表头
    {
        std::ifstream check(kResultsFile);
        if (check.peek() == std::ifstream::traits_type::eof()) {
            out << "timestamp,tag,label,elapsed_sec,count,msg_per_sec,MB_per_sec,avg_latency_us\n";
        }
    }

    // 本次运行的时间戳
    char timeBuf[32];
    std::time_t now = std::time(nullptr);
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    out << std::fixed << std::setprecision(3);
    for (const auto& r : results) {
        out << timeBuf << "," << tag << ",\"" << r.label << "\"," << r.elapsedSec << "," << r.count
            << "," << r.msgPerSec << "," << r.mbPerSec << "," << r.avgLatencyUs << "\n";
    }

    // 每次追加结束后补一个空行，分隔不同批次的运行结果
    out << "\n";
}

// ============================================================
// 主程序
// ============================================================
int main(int argc, char* argv[]) {
    bool csvMode = false;
    std::string tag = "run";

    // 参数：--csv 标准输出 CSV；--tag <名称> 标记本次运行（如 sync / async），用于结果文件对比
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--csv") {
            csvMode = true;
        } else if (arg == "--tag" && i + 1 < argc) {
            tag = argv[++i];
        }
    }

    // 删除旧日志：日志文件为追加模式，不删除则跨次运行累积，
    // 旧数据早已超过 maxSize，开局即触发翻滚
    std::remove(kLogFile);

    // 初始化日志：纯文件模式，避免终端 I/O 抖动
    LouisLog& logger = LouisLog::getInstance();
    logger.init(LogLevel::TRACE, LogTarget::FILE, kLogFile, 200 * 1024 * 1024);

    std::string smallMsg = makeMessage(kSmallMsgSize);
    std::string largeMsg = makeMessage(kLargeMsgSize);
    // 超大消息直接调 log() 而不用宏：_F 系列宏的 snprintf 缓冲区只有 1024，会截断 4KB 消息
    std::string hugeMsg = makeMessage(kHugeMsgSize);

    // --- 预热：清掉初始化的抖动 ---
    std::string warmupMsg = makeMessage(32);
    for (int i = 0; i < 200'000; ++i) {
        logger.log(LogLevel::INFO, __FILE__, __LINE__, warmupMsg);
    }
    logger.flush();

    // 固定消息数：保证同步版本运行足够久（≥3秒）；
    // 异步版本会更快跑完，但样本量依然足够
    const int smallCount = 1'000'000;
    const int largeCount = 100'000;
    const int hugeCount = 30'000;
    const int filterCount = 10'000'000;

    // ============================================================
    // 执行测试
    // ============================================================
    std::vector<BenchResult> results;

    results.push_back(benchSingle("1-thread small msg (64B)", smallCount, smallMsg));
    results.push_back(benchSingle("1-thread large msg (1KB)", largeCount, largeMsg));
    results.push_back(benchSingle("1-thread huge msg (4KB)", hugeCount, hugeMsg));
    results.push_back(benchFiltered("1-thread filtered (level gate, no I/O)", filterCount, smallMsg)
    );
    results.push_back(benchMulti("4-thread concurrent small msg (64B)", 4, kDurationSec, smallMsg));
    results.push_back(benchMulti("8-thread concurrent small msg (64B)", 8, kDurationSec, smallMsg));
    results.push_back(benchMulti("4-thread concurrent large msg (1KB)", 4, kDurationSec, largeMsg));
    results.push_back(benchMulti("8-thread concurrent large msg (1KB)", 8, kDurationSec, largeMsg));

    // 结果持久化到文件（追加），供迭代前后对比
    writeResultsToFile(results, tag);

    // ============================================================
    // 输出
    // ============================================================
    if (csvMode) {
        std::cout << "\"label\",\"elapsed_sec\",\"count\",\"msg_per_sec\",\"MB_per_sec\",\"avg_"
                     "latency_us\"\n";
        for (const auto& r : results) {
            r.printCSV();
        }
    } else {
        std::cout << "\n";
        std::cout << "+---------------------------------------------------------+"
                  << "-------------------------------------+\n";
        std::cout << "| Test Case                                               |"
                  << "   msgs/s          throughput    avg   |\n";
        std::cout << "+---------------------------------------------------------+"
                  << "-------------------------------------+\n";
        for (const auto& r : results) {
            r.print();
        }
        std::cout << "+---------------------------------------------------------+"
                  << "-------------------------------------+\n";
        std::cout << "\nResults appended to " << std::filesystem::absolute(kResultsFile).string()
                  << " (tag: " << tag << ")\n";
        std::cout << "CSV output: " << argv[0] << " --csv --tag <name>\n";
    }

    return 0;
}