### 作者 
DaXiaoZi

### 克隆项目

```bash
git clone https://gitee.com/stone-master/zlogging.git
git clone https://github.com/daxiaozi999/zlogging.git

# ZLogging 使用文档

一个异步C++日志系统，支持多种输出模式、日志轮转、性能监控等功能

## 快速开始

### 基础使用三步骤

```cpp
#include "zlogging.h"

int main() {
    ZLOG_INIT();                        // 1. 初始化
    ZLOG_SET_PROGRAM_NAME("MyApp");     // 2. 设置程序名（可选）
    ZLOG_SET_OUTPUT_DIR("./logs");      // 3. 设置输出目录（可选）
    
    ZINFO() << "Hello ZLogging!";       // 开始使用
    
    ZLOG_SHUTDOWN();                    // 程序结束前关闭（推荐）
    return 0;
}
```

### 两种输出方式

```cpp
// 方式1：流式输出（推荐）
ZINFO() << "用户ID: " << userId << ", 状态: " << status;

// 方式2：格式化输出
ZINFOF("用户ID: %d, 状态: %s", userId, status.c_str());
```

## 日志级别

| 级别    | 宏           | 格式化宏      | 说明             |
|---------|--------------|---------------|------------------|
| TRACE   | `ZTRACE()`   | `ZTRACEF()`   | 最详细的调试信息 |
| DEBUG   | `ZDEBUG()`   | `ZDEBUGF()`   | 调试信息         |
| INFO    | `ZINFO()`    | `ZINFOF()`    | 一般信息         |
| WARNING | `ZWARNING()` | `ZWARNINGF()` | 警告信息         |
| ERROR   | `ZERROR()`   | `ZERRORF()`   | 错误信息         |
| FATAL   | `ZFATAL()`   | `ZFATALF()`   | 致命错误         |

## 基础配置

### 常用配置

```cpp
ZLOG_INIT();                                    // 初始化（必需）
ZLOG_SET_PROGRAM_NAME("MyApp");                 // 程序名
ZLOG_SET_OUTPUT_DIR("./logs");                  // 输出目录
ZLOG_SET_MIN_LEVEL(INFO);                       // 最小日志级别
ZLOG_SET_MAX_LOG_SIZE(50 * 1024 * 1024);        // 单文件最大50MB
ZLOG_SET_MAX_CACHE_SIZE(1000);                  // 最大缓存1000条
```

### 输出模式配置

```cpp
// 预定义模式
ZLOG_SET_OUTPUT_MODE(ZLOG_CONSOLE_ONLY, false, "");     // 仅控制台
ZLOG_SET_OUTPUT_MODE(ZLOG_FILE_ONLY, false, "");        // 仅文件
ZLOG_SET_OUTPUT_MODE(ZLOG_COLORED_CONSOLE, false, "");  // 彩色控制台
ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");     // 控制台+文件+彩色

// 文件输出模式
ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");             // 多文件：每级别独立
ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, true, "app.log");       // 单文件：自定义名称
ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, true, zlog::ZLOG_INFO); // 单文件：使用INFO级别文件
```

### 文件操作模式

```cpp
ZLOG_SET_FILE_MODE(ALWAYS_OPEN);    // 文件始终打开（推荐，性能好）
ZLOG_SET_FILE_MODE(OPEN_ON_WRITE);  // 写入时打开（更安全但性能较低）
```

## 高级功能

### 频率控制

```cpp
// 每N次调用输出一次
for (int i = 0; i < 100; ++i) {
    ZLOG_EVERY_N(INFO, 10) << "每10次输出: " << i;
}

// 只输出前N次
for (int i = 0; i < 20; ++i) {
    ZLOG_FIRST_N(WARNING, 3) << "只输出前3次: " << i;
}

// 只输出一次
for (int i = 0; i < 5; ++i) {
    ZLOG_ONCE(ERROR) << "这个错误只记录一次";
}

// 时间间隔控制
while (running) {
    ZLOG_EVERY_T(INFO, 5) << "每5秒输出一次状态报告";
    ZLOG_EVERY_T_END();
    // 业务逻辑...
}
```

### 作用域追踪

```cpp
void myFunction() {
    ZLOG_FUNCTION(); // 自动记录函数进入和退出
    
    {
        ZLOG_SCOPE("数据处理"); // 自定义作用域追踪
        // 处理逻辑...
    } // 自动记录退出
}
```

### 性能计时

```cpp
// 自动计时器
{
    ZLOG_TIMER("算法耗时");
    // 要计时的代码...
} // 作用域结束时自动输出耗时

// 手动计时器
ZLOG_TIMER_BEGIN(数据库操作);
// 数据库操作...
ZLOG_TIMER_END(数据库操作);
```

### 条件输出

```cpp
bool debugMode = true;
int errorCode = 0;

ZINFO_IF(debugMode) << "调试模式信息";
ZERROR_IF(errorCode != 0) << "错误代码: " << errorCode;
```

### 断言和检查

```cpp
int value = 42;

// 断言（失败时记录日志，但不终止程序）
ZASSERT(value > 0, "值应该大于0");
ZASSERT_EQ(value, 42, "值应该等于42");
ZASSERT_NE(value, 0, "值不应该等于0");

// 检查（失败时终止程序）
ZCHECK(value > 0);
ZCHECK_EQ(value, 42);
ZCHECK_NE(value, 0);
```

## 日志轮转

### 配置轮转策略

```cpp
// 按文件大小轮转
ZLOG_SET_MAX_LOG_SIZE(10 * 1024 * 1024);  // 10MB
ZLOG_SET_ROTATE_POLICY(SIZE_ROTATE);

// 每日轮转
ZLOG_SET_ROTATE_POLICY(DAILY_ROTATE);

// 手动轮转
ZLOG_ROTATE();

// 不轮转（默认）
ZLOG_SET_ROTATE_POLICY(NO_ROTATE);
```

### 轮转文件命名

- 原文件：`info_log.txt`
- 轮转后：`info_log_20240101_143022.txt`（自动添加时间戳）

## 系统监控

### 获取系统状态

```cpp
bool initialized = ZLOG_IS_INITIALIZED();      // 是否已初始化
bool enabled = ZLOG_IS_ENABLED(INFO);          // 指定级别是否启用
std::string dir = ZLOG_GET_OUTPUT_DIR();       // 输出目录
int mode = ZLOG_GET_OUTPUT_MODE();             // 输出模式
```

### 获取统计信息

```cpp
size_t total = ZLOG_GET_TOTAL_COUNT();              // 总日志数
size_t infoCount = ZLOG_GET_LEVEL_COUNT(INFO);      // INFO级别数量
size_t queueSize = ZLOG_GET_QUEUE_SIZE();           // 当前队列大小
size_t dropped = ZLOG_GET_DROPPED_COUNT();          // 丢弃的消息数
```

## 自定义配置

### 自定义输出文件

```cpp
// 为特定级别设置输出文件
ZLOG_SET_LEVEL_FILE(ERROR, "errors.log");
ZLOG_SET_LEVEL_FILE(WARNING, "warnings.log");
```

### 动态调整配置

```cpp
// 动态调整最小级别
ZLOG_SET_MIN_LEVEL(WARNING);  // 只输出WARNING及以上级别
ZLOG_SET_MIN_LEVEL(DEBUG);    // 恢复DEBUG级别

// 动态切换输出模式
ZLOG_SET_OUTPUT_MODE(ZLOG_FILE_ONLY, false, "");      // 切换到仅文件
ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");   // 恢复默认模式
```

## 多线程使用

ZLogging完全线程安全，可以直接在多线程环境中使用：

```cpp
void workerThread(int threadId) {
    ZLOG_FUNCTION(); // 函数追踪包含线程ID
    
    ZINFOF("工作线程 %d 开始", threadId);
    
    for (int i = 0; i < 100; ++i) {
        ZDEBUGF("线程 %d 处理任务 %d", threadId, i);
        // 每条日志都会自动包含线程ID
    }
}
```

## 系统关闭

```cpp
ZLOG_FLUSH();       // 刷新待处理日志（可选）
ZLOG_SHUTDOWN(3000); // 安全关闭，最多等待3秒
// 关闭后不应再使用日志功能
```

## 编译要求

- **C++标准**: C++11 或更高
- **编译器**: GCC 4.8+, Clang 3.3+, MSVC 2015+
- **依赖**: 无外部依赖

### 编译示例

```bash
# Linux/Mac
g++ -std=c++11 -pthread main.cpp zlogging.cpp -o myapp

# Windows (MSVC)
cl /EHsc /std:c++11 main.cpp zlogging.cpp
```

## 配置建议

### 开发环境

```cpp
ZLOG_SET_MIN_LEVEL(DEBUG);                          // 显示调试信息
ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, ""); // 控制台+文件+彩色
ZLOG_SET_FILE_MODE(ALWAYS_OPEN);                    // 性能优先
```

### 生产环境

```cpp
ZLOG_SET_MIN_LEVEL(INFO);                          // 过滤调试日志
ZLOG_SET_OUTPUT_MODE(ZLOG_FILE_ONLY, false, "");   // 仅文件输出
ZLOG_SET_ROTATE_POLICY(SIZE_ROTATE);               // 启用轮转
ZLOG_SET_MAX_LOG_SIZE(100 * 1024 * 1024);         // 100MB轮转
```

### 调试环境

```cpp
ZLOG_SET_MIN_LEVEL(TRACE);                              // 最详细日志
ZLOG_SET_OUTPUT_MODE(ZLOG_COLORED_CONSOLE, false, ""); // 彩色控制台
```

## 常见问题

**Q: 如何减少性能影响？**
A: 使用合适的日志级别过滤，启用`ALWAYS_OPEN`模式，避免在循环中频繁输出。

**Q: 多进程环境如何使用？**
A: 每个进程使用不同的输出目录或程序名，避免文件冲突。

**Q: 日志文件过大怎么办？**
A: 配置日志轮转，设置合适的文件大小限制。

**Q: 如何在库中使用？**
A: 在库头文件中声明，在使用库的应用程序中初始化。

## 完整示例

```cpp
#include "zlogging.h"
#include <thread>

int main() {
    // 初始化和配置
    ZLOG_INIT();
    ZLOG_SET_PROGRAM_NAME("MyApp");
    ZLOG_SET_OUTPUT_DIR("./logs");
    ZLOG_SET_MIN_LEVEL(DEBUG);
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
    ZLOG_SET_ROTATE_POLICY(SIZE_ROTATE);
    
    // 基础使用
    ZINFO() << "应用程序启动";
    ZDEBUGF("调试信息: %d", 42);
    
    // 高级功能
    {
        ZLOG_TIMER("初始化耗时");
        ZLOG_SCOPE("初始化阶段");
        
        // 初始化逻辑...
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 条件和频率控制
    for (int i = 0; i < 20; ++i) {
        ZLOG_EVERY_N(INFO, 5) << "处理进度: " << i;
        ZINFO_IF(i % 10 == 0) << "里程碑: " << i;
    }
    
    // 错误处理
    int result = some_operation();
    ZASSERT(result >= 0, "操作应该成功");
    ZERROR_IF(result < 0) << "操作失败，错误码: " << result;
    
    // 系统关闭
    ZINFO() << "应用程序即将关闭";
    ZLOG_SHUTDOWN(3000);
    
    return 0;
}
```