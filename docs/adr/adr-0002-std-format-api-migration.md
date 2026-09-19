---
title: "格式化迁移至 std::format，对外 API 从宏重设计为函数模板"
date: 2026-09-19
status: Accepted
superseded_by: ""
---

# ADR-0002: 格式化迁移至 std::format，对外 API 从宏重设计为函数模板

## 状态
Accepted（已实现，8 个单测全绿，Release 压测 A/B 对比验证）
参与者：louis + AI 助手

## 背景
ADR-0001 完成异步化后，格式化与 API 层仍遗留一批问题：

- CON-001: 旧 `_F` 系列宏内部用 snprintf + 固定 1024 字节缓冲，超长消息静默截断
  （4KB 消息实测被截断，压测被迫绕行底层 log()）
- CON-002: printf 风格格式串（%s/%d）无编译期检查，类型不匹配只能运行时发现
- CON-003: 宏依赖 __FILE__/__LINE__ 手工透传，宏与 logger.log() 双入口并存，认知负担大
- CON-004: 级别过滤发生在格式化之后，被过滤的日志仍要付出格式化成本
- CON-005: 每条日志构造两次 stringstream（put_time 时间戳 + 线程 ID），热路径开销大

## 决策
启用 C++20，格式化与 API 层全面重设计：

- DEC-001: 格式化引擎全面切换 std::format（日志行组装、时间戳、用户消息），
  删除全部旧宏；CMAKE_BUILD_TYPE 应配置 Release（见压测教训）
- DEC-002: 对外 API 为 louis::log 命名空间下 6 个函数模板
  trace/debug/info/warn/error/fatal(fmt, args...)，std::format_string 提供编译期格式串检查，
  std::format 天然支持任意可格式化类型，消灭 1024 截断
- DEC-003: 调用点捕获用 with_source_location 包装器（cppreference 模式）：
  变参包后的尾置默认实参无法参与推导（变参包吞掉全部剩余实参），
  故 loc 的求值移入 consteval 构造函数默认实参（仍在用户调用点求值）；
  构造参数必须 U&&（避免 字面量→format_string→包装器 两次自定义转换），
  第一参用 type_identity_t 阻断推导（否则 GCC 拿字面量匹配类模板特化直接失败）
- DEC-004: 级别过滤前移至 detail::emit 核心：shouldLog 先于任何格式化，
  6 个级别函数收敛为 2 行转发，消除重复；6 函数共享一个 emit 实现
- DEC-005: 热路径优化——时间戳 localtime_r + 栈上 tm + std::format 整数字段
  （否决 chrono current_zone()/zoned_time 逐条调用，其 tzdb 解析实测拖慢 5 倍）；
  线程 ID thread_local 缓存（每线程仅构造一次）
- DEC-006: 兼容策略为直接删除旧宏（个人项目无下游用户），测试同步迁移并补齐断言；
  shouldLog 语义修正为 level >= level_（内联重构时曾写反，导致 INFO 阈值下
  WARN/ERROR/FATAL 被静默丢弃——无断言的冒烟测试完全无法暴露，教训：单测必须验证输出）
- DEC-007: 大消息 escape hatch 保留：emit 的类型安全代价是用户消息先物化一次中间串，
  4KB 消息单线程约 +0.68µs/条（双拷贝）；超大预构建消息可直接调底层 log() 规避

## 压测结论（同机同 Release A/B，bench_results.csv tag: format+localtime_r+thread_local+release）

| 场景 | 旧 API | 新 API | 结论 |
|---|---|---|---|
| 1T 64B | 0.878µs/条 | 0.748µs/条 | 快 17% |
| 1T 1KB | 0.886µs/条 | 0.894µs/条 | 持平 |
| 1T 4KB | 1.49µs/条 | 2.17µs/条 | 慢 31%（DEC-007 双拷贝代价） |
| 1T 级别过滤 | 1.07 亿/s | 7.98 亿/s | 快 7.5 倍（DEC-004 生效） |
| 4/8 线程 | 214~242 万/s | 179~228 万/s | ±10% 内，队列锁主导 |

教训：严禁用未配置构建类型（-O0）的产物做对比——std::format 在 -O0 下比
stringstream+put_time 慢约 4 倍，产生假"回退"；CSV 中历史 tag（sync/async/localtime_r）
均为 -O0 数据，与本表 Release 数据不可比。

## 备选方案

### ALT-001: 保留旧宏作为语法糖（INFO(...) 转发新 API）
- 否决原因：宏无法参与编译期格式检查，且 source_location 需要宏/函数双入口并存；
  个人项目无兼容负担，双入口徒增认知成本

### ALT-002: chrono zoned_time + current_zone() 格式化时间戳
- 否决原因：current_zone() 每次调用重新解析 tzdb，实测拖慢 5 倍吞吐（本 ADR 排查过程发现）；
  缓存 zone 指针可缓解但仍引入 tzdb 依赖，不如 localtime_r + 栈上 tm 直观可控

### ALT-003: printf 风格 + vformat 运行时检查
- 否决原因：保留旧格式串写法但丢失编译期检查（CON-002 无解），
  且运行时解析格式串的开销高于编译期检查版本
