/**
 * ZLogging ���ܲ��Գ���
 *
 * ���������־���Դ������ܲ��Թ��ܣ�
 * - ZLOG_TIMER: �Զ���ʱ��
 * - ZLOG_TIMER_BEGIN/END: �ֶ���ʱ��
 * - ����ͳ�ƹ���
 */

#include "zlogging.h"
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>

 // ���Բ���
const int BASIC_TEST_COUNT = 50000;        // ������������
const int THREAD_COUNT = 2;                // ���ٵ�2���߳�
const int PER_THREAD_COUNT = 25000;        // ÿ�߳���־��

//==============================================================================
// 1. �������ܲ���
//==============================================================================

void basicPerformanceTest() {
    std::cout << "\n=== �������ܲ��� ===" << std::endl;

    // ����1����ʽ�������
    {
        ZLOG_TIMER("��ʽ������ܲ���");
        for (int i = 0; i < BASIC_TEST_COUNT; ++i) {
            ZINFO() << "��ʽ���������Ϣ " << i << " ����: " << (i * 1.5);
        }
        ZLOG_FLUSH();
    } // �Զ������ʱ���

    // ����2����ʽ���������
    {
        ZLOG_TIMER("��ʽ��������ܲ���");
        for (int i = 0; i < BASIC_TEST_COUNT; ++i) {
            ZINFOF("��ʽ������ ID:%d, ����:%d, ֵ:%.2f, ״̬:%s",
                12345, i, i * 2.5, (i % 2) ? "active" : "inactive");
        }
        ZLOG_FLUSH();
    }

    // ����3����ϼ������
    {
        ZLOG_TIMER("��ϼ����������");
        for (int i = 0; i < BASIC_TEST_COUNT; ++i) {
            switch (i % 4) {
            case 0: ZDEBUG() << "������Ϣ " << i; break;
            case 1: ZINFO() << "��ͨ��Ϣ " << i; break;
            case 2: ZWARNING() << "������Ϣ " << i; break;
            case 3: ZERROR() << "������Ϣ " << i; break;
            }
        }
        ZLOG_FLUSH();
    }

    std::cout << "�������ܲ������" << std::endl;
}

//==============================================================================
// 2. ��ͬ���ģʽ���ܶԱ�
//==============================================================================

void outputModePerformanceTest() {
    std::cout << "\n=== ���ģʽ���ܶԱ� ===" << std::endl;

    const int modeTestCount = 20000;

    // ���ļ��������
    ZLOG_SET_OUTPUT_MODE(ZLOG_FILE_ONLY, false, "");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    {
        ZLOG_TIMER("���ļ����ģʽ");
        for (int i = 0; i < modeTestCount; ++i) {
            ZINFO() << "���ļ�������� " << i << " ʱ���: " << std::chrono::system_clock::now().time_since_epoch().count();
        }
        ZLOG_FLUSH();
    }

    // ������̨�������
    ZLOG_SET_OUTPUT_MODE(ZLOG_CONSOLE_ONLY, false, "");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    {
        ZLOG_TIMER("������̨���ģʽ");
        for (int i = 0; i < modeTestCount; ++i) {
            ZINFO() << "������̨������� " << i;
        }
        ZLOG_FLUSH();
    }

    // ˫���������
    ZLOG_SET_OUTPUT_MODE(ZLOG_BOTH, false, "");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    {
        ZLOG_TIMER("����̨+�ļ����ģʽ");
        for (int i = 0; i < modeTestCount; ++i) {
            ZINFO() << "˫��������� " << i;
        }
        ZLOG_FLUSH();
    }

    // �ָ�Ĭ��ģʽ
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
    std::cout << "���ģʽ���ܶԱ����" << std::endl;
}

//==============================================================================
// 3. ���߳����ܲ��ԣ������߳�����
//==============================================================================

void multiThreadPerformanceTest() {
    std::cout << "\n=== ���߳����ܲ��� (" << THREAD_COUNT << "���߳�) ===" << std::endl;

    ZLOG_TIMER_BEGIN(���߳��������);

    std::vector<std::thread> threads;

    // ���������߳�
    for (int threadId = 0; threadId < THREAD_COUNT; ++threadId) {
        threads.emplace_back([threadId]() {
            ZLOG_FUNCTION(); // ����������׷��

            // ÿ���̵߳ļ�ʱ
            ZLOG_TIMER("�߳�" + std::to_string(threadId) + "ִ��ʱ��");

            ZINFOF("�߳� %d ��ʼִ�У�Ŀ����� %d ����־", threadId, PER_THREAD_COUNT);

            for (int i = 0; i < PER_THREAD_COUNT; ++i) {
                // ��ϲ�ͬ���͵���־���
                if (i % 100 == 0) {
                    ZINFOF("�߳�%d ���ȱ���: %d/%d (%.1f%%)",
                        threadId, i, PER_THREAD_COUNT, (i * 100.0 / PER_THREAD_COUNT));
                }

                if (i % 3 == 0) {
                    ZDEBUGF("�߳�%d ������Ϣ %d", threadId, i);
                }
                else if (i % 7 == 0) {
                    ZWARNINGF("�߳�%d ������Ϣ %d", threadId, i);
                }
                else {
                    ZINFO() << "�߳�" << threadId << " ��ͨ��Ϣ " << i
                        << " ����: " << (i * threadId);
                }

                // ż��������������
                if (i % 5000 == 0 && i > 0) {
                    ZLOG_SCOPE("�߳�" + std::to_string(threadId) + "������");
                    ZDEBUG() << "ִ�������� " << i;
                }
            }

            ZINFOF("�߳� %d ִ�����", threadId);
            });
    }

    // �ȴ������߳����
    for (auto& thread : threads) {
        thread.join();
    }

    ZLOG_TIMER_END(���߳��������);

    // �ȴ��첽�������
    ZLOG_FLUSH();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::cout << "���߳����ܲ������" << std::endl;
}

//==============================================================================
// 4. ���ӳ������ܲ���
//==============================================================================

void complexScenarioTest() {
    std::cout << "\n=== ���ӳ������ܲ��� ===" << std::endl;

    ZLOG_TIMER("���ӳ����������");

    // ģ��ҵ��������
    {
        ZLOG_SCOPE("ҵ���ʼ���׶�");
        ZINFO() << "��ʼҵ���ʼ��";

        {
            ZLOG_TIMER("���ü���");
            for (int i = 0; i < 1000; ++i) {
                ZDEBUGF("���������� %d: key=config_%d, value=%d", i, i, i * 2);
            }
        }

        {
            ZLOG_TIMER("���ݿ����ӳ�ʼ��");
            for (int i = 0; i < 500; ++i) {
                ZINFOF("��ʼ�����ӳ� %d/500", i + 1);
            }
        }

        ZINFO() << "ҵ���ʼ�����";
    }

    // ģ��������
    {
        ZLOG_SCOPE("������׶�");
        ZINFO() << "��ʼ�����û�����";

        for (int requestId = 1; requestId <= 1000; ++requestId) {
            ZLOG_TIMER_BEGIN(������);

            // ģ���������߼�
            ZINFOF("�������� %d", requestId);
            ZDEBUGF("�������: userId=%d, action=%s", requestId * 100, "query");

            // ģ��ҵ���߼�
            if (requestId % 10 == 0) {
                ZWARNINGF("���� %d �������", requestId);
            }

            if (requestId % 50 == 0) {
                ZLOG_SCOPE("���ݿ����");
                ZDEBUGF("ִ�����ݿ��ѯ������ID: %d", requestId);
            }

            ZLOG_TIMER_END(������);

            // ����ģ��
            if (requestId % 100 == 0) {
                ZERRORF("���� %d ����ʧ�ܣ�������: %d", requestId, -1001);
            }
        }

        ZINFO() << "������׶����";
    }

    // ģ��ϵͳ���
    {
        ZLOG_SCOPE("ϵͳ���");
        ZLOG_TIMER("��������ռ�");

        for (int i = 0; i < 100; ++i) {
            ZINFOF("ϵͳ��� - CPU: %.1f%%, �ڴ�: %.1f%%, ����: %d KB/s",
                (i % 100) * 0.8, (i % 80) * 1.2, i * 10);
        }
    }

    std::cout << "���ӳ����������" << std::endl;
}

//==============================================================================
// 5. ѹ�����ԣ��ʶȣ�
//==============================================================================

void stressTest() {
    std::cout << "\n=== �ʶ�ѹ������ ===" << std::endl;

    // ��ʱ���������С
    size_t originalCache = ZLOG_GET_MAX_CACHE_SIZE();
    ZLOG_SET_MAX_CACHE_SIZE(2000);  // ���ý�С����۲���Ϊ

    ZLOG_TIMER("ѹ������");

    const int stressCount = 80000;  // �ʶȵ�ѹ������

    ZINFOF("��ʼѹ�����ԣ�Ŀ��: %d ����־", stressCount);

    for (int i = 0; i < stressCount; ++i) {
        // ����������
        if (i % 5 == 0) {
            ZINFOF("ѹ������ %d/%d - ״̬: %s, ����: %d",
                i, stressCount, (i % 2) ? "processing" : "waiting", i * 3);
        }
        else {
            ZDEBUG() << "ѹ�����Ե�����Ϣ " << i << " ������: " << (i * i % 1000);
        }

        // ���ڼ��ϵͳ״̬
        if (i % 10000 == 0 && i > 0) {
            size_t queueSize = ZLOG_GET_QUEUE_SIZE();
            size_t droppedCount = ZLOG_GET_DROPPED_COUNT();

            ZINFOF("ѹ�����Խ���: %d/%d, ����: %zu, ����: %zu",
                i, stressCount, queueSize, droppedCount);

            if (droppedCount > 0) {
                ZWARNINGF("��⵽��Ϣ����: %zu ��", droppedCount);
            }
        }
    }

    ZLOG_FLUSH();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // ���ѹ�����Խ��
    ZINFOF("ѹ��������� - �ܶ���: %zu, ���ն���: %zu",
        ZLOG_GET_DROPPED_COUNT(), ZLOG_GET_QUEUE_SIZE());

    // �ָ�ԭʼ�����С
    ZLOG_SET_MAX_CACHE_SIZE(originalCache);

    std::cout << "ѹ���������" << std::endl;
}

//==============================================================================
// ������
//==============================================================================

int main() {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "       ZLogging ���ܲ��Գ���" << std::endl;
        std::cout << "========================================" << std::endl;

        // ��ʼ������
        ZLOG_INIT();
        ZLOG_SET_PROGRAM_NAME("demo");
        ZLOG_SET_OUTPUT_DIR("./zlog");
        ZLOG_SET_MIN_LEVEL(DEBUG);
        ZLOG_SET_MAX_CACHE_SIZE(3000);  // ���еĻ����С
        ZLOG_SET_MAX_BUFFER_SIZE(32 * 1024);  // 32KB������
        ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
        ZLOG_SET_FILE_MODE(ALWAYS_OPEN);  // �������

        ZINFO() << "���ܲ��Կ�ʼ";
        ZINFOF("�������� - ��������: %d��, �߳���: %d, ÿ�߳�: %d��",
            BASIC_TEST_COUNT, THREAD_COUNT, PER_THREAD_COUNT);

        // ������Լ�ʱ
        ZLOG_TIMER("���ܲ�������ʱ��");

        // ִ�и������
        basicPerformanceTest();
        outputModePerformanceTest();
        multiThreadPerformanceTest();
        complexScenarioTest();
        stressTest();

        // �������ͳ��
        std::cout << "\n========================================" << std::endl;
        std::cout << "              ����ͳ�ƽ��" << std::endl;
        std::cout << "========================================" << std::endl;

        ZINFOF("����־����: %zu", ZLOG_GET_TOTAL_COUNT());
        ZINFOF("������ͳ��:");
        ZINFOF("  DEBUG: %zu ��", ZLOG_GET_LEVEL_COUNT(DEBUG));
        ZINFOF("  INFO: %zu ��", ZLOG_GET_LEVEL_COUNT(INFO));
        ZINFOF("  WARNING: %zu ��", ZLOG_GET_LEVEL_COUNT(WARNING));
        ZINFOF("  ERROR: %zu ��", ZLOG_GET_LEVEL_COUNT(ERROR));
        ZINFOF("��ǰ���д�С: %zu", ZLOG_GET_QUEUE_SIZE());
        ZINFOF("�ܶ�����Ϣ��: %zu", ZLOG_GET_DROPPED_COUNT());

        ZINFO() << "�������ܲ�����ɣ�";

        std::cout << "\n��ϸ����������鿴����̨����еļ�ʱ���" << std::endl;
        std::cout << "��־�ļ�������: ./perf_logs Ŀ¼" << std::endl;

        // ��ȫ�ر�
        ZLOG_SHUTDOWN(5000);

    }
    catch (const std::exception& e) {
        std::cerr << "���Գ����쳣: " << e.what() << std::endl;
        ZFATAL() << "�����쳣: " << e.what();
        ZLOG_SHUTDOWN(1000);
        return 1;
    }

    return 0;
}