/**
 * ZLogging 性能测试程序
 *
 * 充分利用日志库自带的性能测试功能：
 * - ZLOG_TIMER: 自动计时器
 * - ZLOG_TIMER_BEGIN/END: 手动计时器
 * - 内置统计功能
 */

#include "zlogging.h"
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>

 // 测试参数
const int BASIC_TEST_COUNT = 50000;        // 基础测试数量
const int THREAD_COUNT = 2;                // 减少到2个线程
const int PER_THREAD_COUNT = 25000;        // 每线程日志数

//==============================================================================
// 1. 基础性能测试
//==============================================================================

void basicPerformanceTest() {
    std::cout << "\n=== 基础性能测试 ===" << std::endl;

    // 测试1：流式输出性能
    {
        ZLOG_TIMER("流式输出性能测试");
        for (int i = 0; i < BASIC_TEST_COUNT; ++i) {
            ZINFO() << "流式输出测试消息 " << i << " 数据: " << (i * 1.5);
        }
        ZLOG_FLUSH();
    } // 自动输出计时结果

    // 测试2：格式化输出性能
    {
        ZLOG_TIMER("格式化输出性能测试");
        for (int i = 0; i < BASIC_TEST_COUNT; ++i) {
            ZINFOF("格式化测试 ID:%d, 索引:%d, 值:%.2f, 状态:%s",
                12345, i, i * 2.5, (i % 2) ? "active" : "inactive");
        }
        ZLOG_FLUSH();
    }

    // 测试3：混合级别输出
    {
        ZLOG_TIMER("混合级别输出测试");
        for (int i = 0; i < BASIC_TEST_COUNT; ++i) {
            switch (i % 4) {
            case 0: ZDEBUG() << "调试信息 " << i; break;
            case 1: ZINFO() << "普通信息 " << i; break;
            case 2: ZWARNING() << "警告信息 " << i; break;
            case 3: ZERROR() << "错误信息 " << i; break;
            }
        }
        ZLOG_FLUSH();
    }

    std::cout << "基础性能测试完成" << std::endl;
}

//==============================================================================
// 2. 不同输出模式性能对比
//==============================================================================

void outputModePerformanceTest() {
    std::cout << "\n=== 输出模式性能对比 ===" << std::endl;

    const int modeTestCount = 20000;

    // 仅文件输出测试
    ZLOG_SET_OUTPUT_MODE(ZLOG_FILE_ONLY, false, "");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    {
        ZLOG_TIMER("仅文件输出模式");
        for (int i = 0; i < modeTestCount; ++i) {
            ZINFO() << "仅文件输出测试 " << i << " 时间戳: " << std::chrono::system_clock::now().time_since_epoch().count();
        }
        ZLOG_FLUSH();
    }

    // 仅控制台输出测试
    ZLOG_SET_OUTPUT_MODE(ZLOG_CONSOLE_ONLY, false, "");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    {
        ZLOG_TIMER("仅控制台输出模式");
        for (int i = 0; i < modeTestCount; ++i) {
            ZINFO() << "仅控制台输出测试 " << i;
        }
        ZLOG_FLUSH();
    }

    // 双重输出测试
    ZLOG_SET_OUTPUT_MODE(ZLOG_BOTH, false, "");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    {
        ZLOG_TIMER("控制台+文件输出模式");
        for (int i = 0; i < modeTestCount; ++i) {
            ZINFO() << "双重输出测试 " << i;
        }
        ZLOG_FLUSH();
    }

    // 恢复默认模式
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
    std::cout << "输出模式性能对比完成" << std::endl;
}

//==============================================================================
// 3. 多线程性能测试（减少线程数）
//==============================================================================

void multiThreadPerformanceTest() {
    std::cout << "\n=== 多线程性能测试 (" << THREAD_COUNT << "个线程) ===" << std::endl;

    ZLOG_TIMER_BEGIN(多线程总体测试);

    std::vector<std::thread> threads;

    // 创建测试线程
    for (int threadId = 0; threadId < THREAD_COUNT; ++threadId) {
        threads.emplace_back([threadId]() {
            ZLOG_FUNCTION(); // 函数作用域追踪

            // 每个线程的计时
            ZLOG_TIMER("线程" + std::to_string(threadId) + "执行时间");

            ZINFOF("线程 %d 开始执行，目标输出 %d 条日志", threadId, PER_THREAD_COUNT);

            for (int i = 0; i < PER_THREAD_COUNT; ++i) {
                // 混合不同类型的日志输出
                if (i % 100 == 0) {
                    ZINFOF("线程%d 进度报告: %d/%d (%.1f%%)",
                        threadId, i, PER_THREAD_COUNT, (i * 100.0 / PER_THREAD_COUNT));
                }

                if (i % 3 == 0) {
                    ZDEBUGF("线程%d 调试信息 %d", threadId, i);
                }
                else if (i % 7 == 0) {
                    ZWARNINGF("线程%d 警告信息 %d", threadId, i);
                }
                else {
                    ZINFO() << "线程" << threadId << " 普通消息 " << i
                        << " 数据: " << (i * threadId);
                }

                // 偶尔添加作用域测试
                if (i % 5000 == 0 && i > 0) {
                    ZLOG_SCOPE("线程" + std::to_string(threadId) + "子任务");
                    ZDEBUG() << "执行子任务 " << i;
                }
            }

            ZINFOF("线程 %d 执行完成", threadId);
            });
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    ZLOG_TIMER_END(多线程总体测试);

    // 等待异步处理完成
    ZLOG_FLUSH();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::cout << "多线程性能测试完成" << std::endl;
}

//==============================================================================
// 4. 复杂场景性能测试
//==============================================================================

void complexScenarioTest() {
    std::cout << "\n=== 复杂场景性能测试 ===" << std::endl;

    ZLOG_TIMER("复杂场景总体测试");

    // 模拟业务处理流程
    {
        ZLOG_SCOPE("业务初始化阶段");
        ZINFO() << "开始业务初始化";

        {
            ZLOG_TIMER("配置加载");
            for (int i = 0; i < 1000; ++i) {
                ZDEBUGF("加载配置项 %d: key=config_%d, value=%d", i, i, i * 2);
            }
        }

        {
            ZLOG_TIMER("数据库连接初始化");
            for (int i = 0; i < 500; ++i) {
                ZINFOF("初始化连接池 %d/500", i + 1);
            }
        }

        ZINFO() << "业务初始化完成";
    }

    // 模拟请求处理
    {
        ZLOG_SCOPE("请求处理阶段");
        ZINFO() << "开始处理用户请求";

        for (int requestId = 1; requestId <= 1000; ++requestId) {
            ZLOG_TIMER_BEGIN(请求处理);

            // 模拟请求处理逻辑
            ZINFOF("处理请求 %d", requestId);
            ZDEBUGF("请求参数: userId=%d, action=%s", requestId * 100, "query");

            // 模拟业务逻辑
            if (requestId % 10 == 0) {
                ZWARNINGF("请求 %d 处理较慢", requestId);
            }

            if (requestId % 50 == 0) {
                ZLOG_SCOPE("数据库操作");
                ZDEBUGF("执行数据库查询，请求ID: %d", requestId);
            }

            ZLOG_TIMER_END(请求处理);

            // 错误模拟
            if (requestId % 100 == 0) {
                ZERRORF("请求 %d 处理失败，错误码: %d", requestId, -1001);
            }
        }

        ZINFO() << "请求处理阶段完成";
    }

    // 模拟系统监控
    {
        ZLOG_SCOPE("系统监控");
        ZLOG_TIMER("监控数据收集");

        for (int i = 0; i < 100; ++i) {
            ZINFOF("系统监控 - CPU: %.1f%%, 内存: %.1f%%, 网络: %d KB/s",
                (i % 100) * 0.8, (i % 80) * 1.2, i * 10);
        }
    }

    std::cout << "复杂场景测试完成" << std::endl;
}

//==============================================================================
// 5. 压力测试（适度）
//==============================================================================

void stressTest() {
    std::cout << "\n=== 适度压力测试 ===" << std::endl;

    // 临时调整缓存大小
    size_t originalCache = ZLOG_GET_MAX_CACHE_SIZE();
    ZLOG_SET_MAX_CACHE_SIZE(2000);  // 设置较小缓存观察行为

    ZLOG_TIMER("压力测试");

    const int stressCount = 80000;  // 适度的压力测试

    ZINFOF("开始压力测试，目标: %d 条日志", stressCount);

    for (int i = 0; i < stressCount; ++i) {
        // 混合输出类型
        if (i % 5 == 0) {
            ZINFOF("压力测试 %d/%d - 状态: %s, 数据: %d",
                i, stressCount, (i % 2) ? "processing" : "waiting", i * 3);
        }
        else {
            ZDEBUG() << "压力测试调试信息 " << i << " 计算结果: " << (i * i % 1000);
        }

        // 定期检查系统状态
        if (i % 10000 == 0 && i > 0) {
            size_t queueSize = ZLOG_GET_QUEUE_SIZE();
            size_t droppedCount = ZLOG_GET_DROPPED_COUNT();

            ZINFOF("压力测试进度: %d/%d, 队列: %zu, 丢弃: %zu",
                i, stressCount, queueSize, droppedCount);

            if (droppedCount > 0) {
                ZWARNINGF("检测到消息丢弃: %zu 条", droppedCount);
            }
        }
    }

    ZLOG_FLUSH();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // 输出压力测试结果
    ZINFOF("压力测试完成 - 总丢弃: %zu, 最终队列: %zu",
        ZLOG_GET_DROPPED_COUNT(), ZLOG_GET_QUEUE_SIZE());

    // 恢复原始缓存大小
    ZLOG_SET_MAX_CACHE_SIZE(originalCache);

    std::cout << "压力测试完成" << std::endl;
}

//==============================================================================
// 主函数
//==============================================================================

int main() {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "       ZLogging 性能测试程序" << std::endl;
        std::cout << "========================================" << std::endl;

        // 初始化配置
        ZLOG_INIT();
        ZLOG_SET_PROGRAM_NAME("demo");
        ZLOG_SET_OUTPUT_DIR("./zlog");
        ZLOG_SET_MIN_LEVEL(DEBUG);
        ZLOG_SET_MAX_CACHE_SIZE(3000);  // 适中的缓存大小
        ZLOG_SET_MAX_BUFFER_SIZE(32 * 1024);  // 32KB缓冲区
        ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
        ZLOG_SET_FILE_MODE(ALWAYS_OPEN);  // 最佳性能

        ZINFO() << "性能测试开始";
        ZINFOF("测试配置 - 基础测试: %d条, 线程数: %d, 每线程: %d条",
            BASIC_TEST_COUNT, THREAD_COUNT, PER_THREAD_COUNT);

        // 整体测试计时
        ZLOG_TIMER("性能测试总体时间");

        // 执行各项测试
        basicPerformanceTest();
        outputModePerformanceTest();
        multiThreadPerformanceTest();
        complexScenarioTest();
        stressTest();

        // 输出最终统计
        std::cout << "\n========================================" << std::endl;
        std::cout << "              测试统计结果" << std::endl;
        std::cout << "========================================" << std::endl;

        ZINFOF("总日志条数: %zu", ZLOG_GET_TOTAL_COUNT());
        ZINFOF("各级别统计:");
        ZINFOF("  DEBUG: %zu 条", ZLOG_GET_LEVEL_COUNT(DEBUG));
        ZINFOF("  INFO: %zu 条", ZLOG_GET_LEVEL_COUNT(INFO));
        ZINFOF("  WARNING: %zu 条", ZLOG_GET_LEVEL_COUNT(WARNING));
        ZINFOF("  ERROR: %zu 条", ZLOG_GET_LEVEL_COUNT(ERROR));
        ZINFOF("当前队列大小: %zu", ZLOG_GET_QUEUE_SIZE());
        ZINFOF("总丢弃消息数: %zu", ZLOG_GET_DROPPED_COUNT());

        ZINFO() << "所有性能测试完成！";

        std::cout << "\n详细性能数据请查看控制台输出中的计时结果" << std::endl;
        std::cout << "日志文件保存在: ./perf_logs 目录" << std::endl;

        // 安全关闭
        ZLOG_SHUTDOWN(5000);

    }
    catch (const std::exception& e) {
        std::cerr << "测试程序异常: " << e.what() << std::endl;
        ZFATAL() << "程序异常: " << e.what();
        ZLOG_SHUTDOWN(1000);
        return 1;
    }

    return 0;
}