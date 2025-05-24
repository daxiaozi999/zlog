/**
 * ZLogging 日志系统使用示例
 *
 * 高性能异步日志系统，支持多种输出模式、日志轮转、性能监控等功能
 *
 * 主要特性：
 * - 异步多线程安全处理
 * - 流式输出和格式化输出
 * - 可配置的日志级别过滤
 * - 内置性能计时和作用域追踪
 * - 丰富的统计和监控功能
 */

#include "zlogging.h"
#include <thread>
#include <chrono>
#include <vector>

 //==============================================================================
 // 1. 快速开始 - 最简单的使用方式
 //==============================================================================

void quickStart() {
    std::cout << "================================== 开始演示 ==================================" << std::endl;

    // 三步开始使用
    ZLOG_INIT();                                    // 1. 初始化
    ZLOG_SET_PROGRAM_NAME("demo");                  // 2. 设置程序名
    ZLOG_SET_OUTPUT_DIR("./zlog");                  // 3. 设置输出目录

    // 开始记录日志
    ZINFO() << "Hello ZLogging!";
    ZINFO() << "日志系统已启动，输出目录: ./logs";
}

//==============================================================================
// 2. 基础配置演示
//==============================================================================

void basicConfiguration() {
    std::cout << "================================== 基础配置演示 ==================================" << std::endl;

    // 基础参数配置
    ZLOG_SET_MAX_LOG_SIZE(10 * 1024 * 1024);       // 单文件最大10MB
    ZLOG_SET_MAX_CACHE_SIZE(2000);                 // 最大缓存2000条
    ZLOG_SET_MIN_LEVEL(DEBUG);                     // 最小级别DEBUG

    // 输出模式配置（必须3个参数）
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");  // 默认多文件模式

    // 文件操作和轮转配置
    ZLOG_SET_FILE_MODE(ALWAYS_OPEN);               // 文件始终打开
    ZLOG_SET_ROTATE_POLICY(SIZE_ROTATE);           // 按大小轮转

    ZINFO() << "基础配置完成";
}

//==============================================================================
// 3. 日志输出方式演示
//==============================================================================

void loggingMethods() {
    std::cout << "================================== 日志输出方式演示 ==================================" << std::endl;

    ZINFO() << "--- 方式1：流式输出 ---";
    ZTRACE() << "TRACE: 详细调试信息";
    ZDEBUG() << "DEBUG: 调试信息";
    ZINFO() << "INFO: 一般信息";
    ZWARNING() << "WARNING: 警告信息";
    ZERROR() << "ERROR: 错误信息";
    ZFATAL() << "FATAL: 致命错误";

    ZINFO() << "--- 方式2：格式化输出 ---";
    int userId = 12345;
    const char* userName = "张三";
    double balance = 1234.56;

    ZINFOF("用户信息 - ID:%d, 姓名:%s, 余额:%.2f", userId, userName, balance);
    ZWARNINGF("余额警告: %.2f < 100.00", balance);
    ZERRORF("操作失败: 用户%s操作异常", userName);

    ZINFO() << "--- 方式3：条件输出 ---";
    bool isVip = true;
    bool hasError = false;

    ZINFO_IF(isVip) << "VIP用户特权信息";
    ZERROR_IF(hasError) << "这条不会输出（条件为false）";
    ZWARNING_IF(!hasError) << "这条会输出（条件为true）";

    ZINFO() << "--- 方式4：复杂数据输出 ---";
    std::vector<int> data = { 1, 2, 3, 4, 5 };
    ZINFO() << "数组大小: " << data.size();
    for (size_t i = 0; i < data.size(); ++i) {
        ZDEBUG() << "data[" << i << "] = " << data[i];
    }
}

//==============================================================================
// 4. 输出模式配置演示
//==============================================================================

void outputModeDemo() {
    std::cout << "================================== 输出模式演示 ==================================" << std::endl;

    ZINFO() << "--- 预定义模式 ---";

    // 仅控制台
    ZLOG_SET_OUTPUT_MODE(ZLOG_CONSOLE_ONLY, false, "");
    ZINFO() << "仅控制台输出";

    // 仅文件
    ZLOG_SET_OUTPUT_MODE(ZLOG_FILE_ONLY, false, "");
    ZINFO() << "仅文件输出";

    // 彩色控制台
    ZLOG_SET_OUTPUT_MODE(ZLOG_COLORED_CONSOLE, false, "");
    ZINFO() << "彩色控制台输出";

    ZINFO() << "--- 文件输出模式 ---";

    // 多文件模式：每个级别独立文件
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
    ZDEBUG() << "DEBUG -> debug_log.txt";
    ZINFO() << "INFO -> info_log.txt";
    ZERROR() << "ERROR -> error_log.txt";

    // 单文件模式：所有日志一个文件
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, true, "all.log");
    ZINFO() << "所有日志 -> all.log";
    ZDEBUG() << "不同级别都在同一文件";

    // 单文件模式：使用指定级别的文件
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, true, zlog::ZLOG_INFO);
    ZINFO() << "使用INFO级别的文件";

    // 恢复默认
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
    ZINFO() << "恢复默认多文件模式";
}

//==============================================================================
// 5. 高级功能演示
//==============================================================================

void advancedFeatures() {
    std::cout << "================================== 高级功能演示 ==================================" << std::endl;

    ZLOG_FUNCTION(); // 函数追踪

    ZINFO() << "--- 频率控制 ---";

    // 每N次输出
    for (int i = 1; i <= 12; ++i) {
        ZLOG_EVERY_N(INFO, 3) << "每3次输出: " << i;
    }

    // 只输出前N次
    for (int i = 1; i <= 8; ++i) {
        ZLOG_FIRST_N(DEBUG, 3) << "前3次: " << i;
    }

    // 只输出一次
    for (int i = 0; i < 5; ++i) {
        ZLOG_ONCE(WARNING) << "只输出一次，循环" << i << "次";
    }

    ZINFO() << "--- 作用域追踪 ---";

    {
        ZLOG_SCOPE("数据处理");
        ZINFO() << "开始处理数据";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        {
            ZLOG_SCOPE("数据验证");
            ZINFO() << "验证数据格式";
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        } // 自动记录退出

        ZINFO() << "数据处理完成";
    } // 自动记录退出

    ZINFO() << "--- 性能计时 ---";

    // 自动计时
    {
        ZLOG_TIMER("计算性能");
        volatile long sum = 0;
        for (int i = 0; i < 1000000; ++i) {
            sum += i;
        }
        ZINFO() << "计算结果: " << sum;
    } // 自动输出耗时

    // 手动计时
    ZLOG_TIMER_BEGIN(手动测试);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ZLOG_TIMER_END(手动测试);
}

//==============================================================================
// 6. 断言和检查演示
//==============================================================================

void assertionsDemo() {
    std::cout << "================================== 断言和检查演示 ==================================" << std::endl;

    ZINFO() << "--- 断言测试（失败时记录日志但不终止） ---";

    int value = 42;
    const char* ptr = "test";

    // 成功的断言
    ZASSERT(value > 0, "value应该大于0");
    ZASSERT_EQ(value, 42, "value应该等于42");
    ZASSERT_NE(ptr, nullptr, "指针不应为空");

    // 失败的断言（会记录错误日志）
    ZASSERT(value < 0, "这个断言会失败并记录日志");
    ZASSERT_EQ(value, 100, "这个断言也会失败");

    ZINFO() << "--- 检查测试（失败时终止程序） ---";

    // 成功的检查
    ZCHECK(value > 0);
    ZCHECK_EQ(value, 42);
    ZCHECK_NE(value, 0);
    ZCHECK_GT(value, 10);

    // 注意：下面的检查如果启用会终止程序
    // ZCHECK(value < 0); // 这会导致程序终止

    ZINFO() << "断言和检查测试完成";
}

//==============================================================================
// 7. 系统监控演示
//==============================================================================

void monitoringDemo() {
    std::cout << "================================== 系统监控演示 ==================================" << std::endl;

    ZINFO() << "--- 系统状态 ---";
    ZINFOF("系统已初始化: %s", ZLOG_IS_INITIALIZED() ? "是" : "否");
    ZINFOF("INFO级别启用: %s", ZLOG_IS_ENABLED(INFO) ? "是" : "否");
    ZINFOF("DEBUG级别启用: %s", ZLOG_IS_ENABLED(DEBUG) ? "是" : "否");

    ZINFO() << "--- 配置信息 ---";
    ZINFOF("输出目录: %s", ZLOG_GET_OUTPUT_DIR().c_str());
    ZINFOF("最大缓存: %zu", ZLOG_GET_MAX_CACHE_SIZE());
    ZINFOF("输出模式: %d", ZLOG_GET_OUTPUT_MODE());
    ZINFOF("文件模式: %d", ZLOG_GET_FILE_MODE());
    ZINFOF("最小级别: %d", ZLOG_GET_MIN_LEVEL());

    // 生成一些日志用于统计
    for (int i = 0; i < 5; ++i) {
        ZTRACE() << "统计测试TRACE " << i;
        ZDEBUG() << "统计测试DEBUG " << i;
        ZINFO() << "统计测试INFO " << i;
        if (i % 2 == 0) ZWARNING() << "统计测试WARNING " << i;
        if (i % 3 == 0) ZERROR() << "统计测试ERROR " << i;
    }

    // 等待异步处理
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ZINFO() << "--- 统计信息 ---";
    ZINFOF("总日志数量: %zu", ZLOG_GET_TOTAL_COUNT());
    ZINFOF("TRACE数量: %zu", ZLOG_GET_LEVEL_COUNT(TRACE));
    ZINFOF("DEBUG数量: %zu", ZLOG_GET_LEVEL_COUNT(DEBUG));
    ZINFOF("INFO数量: %zu", ZLOG_GET_LEVEL_COUNT(INFO));
    ZINFOF("WARNING数量: %zu", ZLOG_GET_LEVEL_COUNT(WARNING));
    ZINFOF("ERROR数量: %zu", ZLOG_GET_LEVEL_COUNT(ERROR));
    ZINFOF("当前队列大小: %zu", ZLOG_GET_QUEUE_SIZE());
    ZINFOF("丢弃消息数量: %zu", ZLOG_GET_DROPPED_COUNT());
}

//==============================================================================
// 8. 多线程演示
//==============================================================================

void multiThreadDemo() {
    std::cout << "================================== 多线程演示 ==================================" << std::endl;

    ZINFO() << "启动多线程日志测试";

    std::vector<std::thread> threads;

    // 创建3个工作线程
    for (int threadId = 0; threadId < 3; ++threadId) {
        threads.emplace_back([threadId]() {
            ZLOG_FUNCTION(); // 每个线程的函数追踪

            ZINFOF("线程 %d 开始执行", threadId);

            for (int i = 0; i < 5; ++i) {
                ZINFOF("线程 %d - 任务 %d", threadId, i);
                ZDEBUGF("线程 %d - 调试信息 %d", threadId, i);

                if (i % 2 == 0) {
                    ZWARNINGF("线程 %d - 警告 %d", threadId, i);
                }

                // 模拟不同处理时间
                std::this_thread::sleep_for(std::chrono::milliseconds(10 + threadId * 5));
            }

            ZINFOF("线程 %d 执行完成", threadId);
            });
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    ZINFO() << "多线程测试完成，所有日志都包含线程ID";
}

//==============================================================================
// 9. 配置管理演示
//==============================================================================

void configurationDemo() {
    std::cout << "================================== 配置管理演示 ==================================" << std::endl;

    ZINFO() << "--- 自定义文件配置 ---";

    // 自定义各级别输出文件
    ZLOG_SET_LEVEL_FILE(ERROR, "custom_errors.log");
    ZLOG_SET_LEVEL_FILE(WARNING, "custom_warnings.log");

    ZERROR() << "这条错误输出到 custom_errors.log";
    ZWARNING() << "这条警告输出到 custom_warnings.log";
    ZINFO() << "这条信息输出到默认的 info_log.txt";

    ZINFO() << "--- 动态级别调整 ---";

    ZINFOF("当前最小级别: %d", ZLOG_GET_MIN_LEVEL());

    // 提高最小级别到WARNING
    ZLOG_SET_MIN_LEVEL(WARNING);
    ZINFO() << "这条INFO不会输出（级别已调整）";
    ZDEBUG() << "这条DEBUG也不会输出";
    ZWARNING() << "这条WARNING会输出";
    ZERROR() << "这条ERROR会输出";

    // 恢复到DEBUG级别
    ZLOG_SET_MIN_LEVEL(DEBUG);
    ZINFO() << "恢复到DEBUG级别，这条INFO会输出";

    ZINFO() << "--- 日志轮转 ---";

    ZINFO() << "当前INFO文件: " << ZLOG_GET_LEVEL_FILE(INFO);
    ZINFO() << "执行手动轮转";
    ZLOG_ROTATE();
    ZINFO() << "轮转完成，旧文件已备份";

    // 演示不同场景配置
    ZINFO() << "--- 场景配置示例 ---";

    // 开发环境
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
    ZLOG_SET_MIN_LEVEL(DEBUG);
    ZINFO() << "开发环境：全输出+DEBUG级别";

    // 生产环境
    ZLOG_SET_OUTPUT_MODE(ZLOG_FILE_ONLY, false, "");
    ZLOG_SET_MIN_LEVEL(INFO);
    ZINFO() << "生产环境：仅文件+INFO级别";

    // 恢复默认
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
    ZLOG_SET_MIN_LEVEL(DEBUG);
    ZINFO() << "恢复默认配置";
}

//==============================================================================
// 主函数
//==============================================================================

int use_main() {
    try {
        std::cout << "==================================" << std::endl;
        std::cout << "    ZLogging 日志系统完整演示" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "查看控制台输出和 ./logs 目录中的文件" << std::endl;
        std::cout << std::endl;

        // 1. 快速开始
        quickStart();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 2. 基础配置
        basicConfiguration();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 3. 日志输出方式
        loggingMethods();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 4. 输出模式配置
        outputModeDemo();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 5. 高级功能
        advancedFeatures();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 6. 断言和检查
        assertionsDemo();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 7. 系统监控
        monitoringDemo();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 8. 多线程演示
        multiThreadDemo();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // 9. 配置管理
        configurationDemo();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 10. 系统关闭
        std::cout << "=== 系统关闭演示 ===" << std::endl;
        ZINFO() << "准备关闭日志系统";
        ZINFOF("最终统计 - 总数:%zu, 队列:%zu, 丢弃:%zu",
            ZLOG_GET_TOTAL_COUNT(), ZLOG_GET_QUEUE_SIZE(), ZLOG_GET_DROPPED_COUNT());

        ZLOG_FLUSH();           // 刷新待处理日志
        ZLOG_SHUTDOWN(3000);    // 3秒超时关闭

        std::cout << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "           演示程序完成" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "请检查以下内容：" << std::endl;
        std::cout << "1. 控制台的彩色输出效果" << std::endl;
        std::cout << "2. ./zlog 目录中的日志文件" << std::endl;
        std::cout << "3. 不同级别日志的文件分离" << std::endl;
        std::cout << "4. 多线程日志中的线程ID" << std::endl;
        std::cout << "5. 性能计时和作用域追踪效果" << std::endl;

    }
    catch (const std::exception& e) {
        ZFATAL() << "程序发生异常: " << e.what();
        ZLOG_SHUTDOWN(1000);
        std::cerr << "程序异常: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}