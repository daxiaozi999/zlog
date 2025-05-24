/**
 * ZLogging ��־ϵͳʹ��ʾ��
 *
 * �������첽��־ϵͳ��֧�ֶ������ģʽ����־��ת�����ܼ�صȹ���
 *
 * ��Ҫ���ԣ�
 * - �첽���̰߳�ȫ����
 * - ��ʽ����͸�ʽ�����
 * - �����õ���־�������
 * - �������ܼ�ʱ��������׷��
 * - �ḻ��ͳ�ƺͼ�ع���
 */

#include "zlogging.h"
#include <thread>
#include <chrono>
#include <vector>

 //==============================================================================
 // 1. ���ٿ�ʼ - ��򵥵�ʹ�÷�ʽ
 //==============================================================================

void quickStart() {
    std::cout << "================================== ��ʼ��ʾ ==================================" << std::endl;

    // ������ʼʹ��
    ZLOG_INIT();                                    // 1. ��ʼ��
    ZLOG_SET_PROGRAM_NAME("demo");                  // 2. ���ó�����
    ZLOG_SET_OUTPUT_DIR("./zlog");                  // 3. �������Ŀ¼

    // ��ʼ��¼��־
    ZINFO() << "Hello ZLogging!";
    ZINFO() << "��־ϵͳ�����������Ŀ¼: ./logs";
}

//==============================================================================
// 2. ����������ʾ
//==============================================================================

void basicConfiguration() {
    std::cout << "================================== ����������ʾ ==================================" << std::endl;

    // ������������
    ZLOG_SET_MAX_LOG_SIZE(10 * 1024 * 1024);       // ���ļ����10MB
    ZLOG_SET_MAX_CACHE_SIZE(2000);                 // ��󻺴�2000��
    ZLOG_SET_MIN_LEVEL(DEBUG);                     // ��С����DEBUG

    // ���ģʽ���ã�����3��������
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");  // Ĭ�϶��ļ�ģʽ

    // �ļ���������ת����
    ZLOG_SET_FILE_MODE(ALWAYS_OPEN);               // �ļ�ʼ�մ�
    ZLOG_SET_ROTATE_POLICY(SIZE_ROTATE);           // ����С��ת

    ZINFO() << "�����������";
}

//==============================================================================
// 3. ��־�����ʽ��ʾ
//==============================================================================

void loggingMethods() {
    std::cout << "================================== ��־�����ʽ��ʾ ==================================" << std::endl;

    ZINFO() << "--- ��ʽ1����ʽ��� ---";
    ZTRACE() << "TRACE: ��ϸ������Ϣ";
    ZDEBUG() << "DEBUG: ������Ϣ";
    ZINFO() << "INFO: һ����Ϣ";
    ZWARNING() << "WARNING: ������Ϣ";
    ZERROR() << "ERROR: ������Ϣ";
    ZFATAL() << "FATAL: ��������";

    ZINFO() << "--- ��ʽ2����ʽ����� ---";
    int userId = 12345;
    const char* userName = "����";
    double balance = 1234.56;

    ZINFOF("�û���Ϣ - ID:%d, ����:%s, ���:%.2f", userId, userName, balance);
    ZWARNINGF("����: %.2f < 100.00", balance);
    ZERRORF("����ʧ��: �û�%s�����쳣", userName);

    ZINFO() << "--- ��ʽ3��������� ---";
    bool isVip = true;
    bool hasError = false;

    ZINFO_IF(isVip) << "VIP�û���Ȩ��Ϣ";
    ZERROR_IF(hasError) << "�����������������Ϊfalse��";
    ZWARNING_IF(!hasError) << "���������������Ϊtrue��";

    ZINFO() << "--- ��ʽ4������������� ---";
    std::vector<int> data = { 1, 2, 3, 4, 5 };
    ZINFO() << "�����С: " << data.size();
    for (size_t i = 0; i < data.size(); ++i) {
        ZDEBUG() << "data[" << i << "] = " << data[i];
    }
}

//==============================================================================
// 4. ���ģʽ������ʾ
//==============================================================================

void outputModeDemo() {
    std::cout << "================================== ���ģʽ��ʾ ==================================" << std::endl;

    ZINFO() << "--- Ԥ����ģʽ ---";

    // ������̨
    ZLOG_SET_OUTPUT_MODE(ZLOG_CONSOLE_ONLY, false, "");
    ZINFO() << "������̨���";

    // ���ļ�
    ZLOG_SET_OUTPUT_MODE(ZLOG_FILE_ONLY, false, "");
    ZINFO() << "���ļ����";

    // ��ɫ����̨
    ZLOG_SET_OUTPUT_MODE(ZLOG_COLORED_CONSOLE, false, "");
    ZINFO() << "��ɫ����̨���";

    ZINFO() << "--- �ļ����ģʽ ---";

    // ���ļ�ģʽ��ÿ����������ļ�
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
    ZDEBUG() << "DEBUG -> debug_log.txt";
    ZINFO() << "INFO -> info_log.txt";
    ZERROR() << "ERROR -> error_log.txt";

    // ���ļ�ģʽ��������־һ���ļ�
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, true, "all.log");
    ZINFO() << "������־ -> all.log";
    ZDEBUG() << "��ͬ������ͬһ�ļ�";

    // ���ļ�ģʽ��ʹ��ָ��������ļ�
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, true, zlog::ZLOG_INFO);
    ZINFO() << "ʹ��INFO������ļ�";

    // �ָ�Ĭ��
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
    ZINFO() << "�ָ�Ĭ�϶��ļ�ģʽ";
}

//==============================================================================
// 5. �߼�������ʾ
//==============================================================================

void advancedFeatures() {
    std::cout << "================================== �߼�������ʾ ==================================" << std::endl;

    ZLOG_FUNCTION(); // ����׷��

    ZINFO() << "--- Ƶ�ʿ��� ---";

    // ÿN�����
    for (int i = 1; i <= 12; ++i) {
        ZLOG_EVERY_N(INFO, 3) << "ÿ3�����: " << i;
    }

    // ֻ���ǰN��
    for (int i = 1; i <= 8; ++i) {
        ZLOG_FIRST_N(DEBUG, 3) << "ǰ3��: " << i;
    }

    // ֻ���һ��
    for (int i = 0; i < 5; ++i) {
        ZLOG_ONCE(WARNING) << "ֻ���һ�Σ�ѭ��" << i << "��";
    }

    ZINFO() << "--- ������׷�� ---";

    {
        ZLOG_SCOPE("���ݴ���");
        ZINFO() << "��ʼ��������";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        {
            ZLOG_SCOPE("������֤");
            ZINFO() << "��֤���ݸ�ʽ";
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        } // �Զ���¼�˳�

        ZINFO() << "���ݴ������";
    } // �Զ���¼�˳�

    ZINFO() << "--- ���ܼ�ʱ ---";

    // �Զ���ʱ
    {
        ZLOG_TIMER("��������");
        volatile long sum = 0;
        for (int i = 0; i < 1000000; ++i) {
            sum += i;
        }
        ZINFO() << "������: " << sum;
    } // �Զ������ʱ

    // �ֶ���ʱ
    ZLOG_TIMER_BEGIN(�ֶ�����);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ZLOG_TIMER_END(�ֶ�����);
}

//==============================================================================
// 6. ���Ժͼ����ʾ
//==============================================================================

void assertionsDemo() {
    std::cout << "================================== ���Ժͼ����ʾ ==================================" << std::endl;

    ZINFO() << "--- ���Բ��ԣ�ʧ��ʱ��¼��־������ֹ�� ---";

    int value = 42;
    const char* ptr = "test";

    // �ɹ��Ķ���
    ZASSERT(value > 0, "valueӦ�ô���0");
    ZASSERT_EQ(value, 42, "valueӦ�õ���42");
    ZASSERT_NE(ptr, nullptr, "ָ�벻ӦΪ��");

    // ʧ�ܵĶ��ԣ����¼������־��
    ZASSERT(value < 0, "������Ի�ʧ�ܲ���¼��־");
    ZASSERT_EQ(value, 100, "�������Ҳ��ʧ��");

    ZINFO() << "--- �����ԣ�ʧ��ʱ��ֹ���� ---";

    // �ɹ��ļ��
    ZCHECK(value > 0);
    ZCHECK_EQ(value, 42);
    ZCHECK_NE(value, 0);
    ZCHECK_GT(value, 10);

    // ע�⣺����ļ��������û���ֹ����
    // ZCHECK(value < 0); // ��ᵼ�³�����ֹ

    ZINFO() << "���Ժͼ��������";
}

//==============================================================================
// 7. ϵͳ�����ʾ
//==============================================================================

void monitoringDemo() {
    std::cout << "================================== ϵͳ�����ʾ ==================================" << std::endl;

    ZINFO() << "--- ϵͳ״̬ ---";
    ZINFOF("ϵͳ�ѳ�ʼ��: %s", ZLOG_IS_INITIALIZED() ? "��" : "��");
    ZINFOF("INFO��������: %s", ZLOG_IS_ENABLED(INFO) ? "��" : "��");
    ZINFOF("DEBUG��������: %s", ZLOG_IS_ENABLED(DEBUG) ? "��" : "��");

    ZINFO() << "--- ������Ϣ ---";
    ZINFOF("���Ŀ¼: %s", ZLOG_GET_OUTPUT_DIR().c_str());
    ZINFOF("��󻺴�: %zu", ZLOG_GET_MAX_CACHE_SIZE());
    ZINFOF("���ģʽ: %d", ZLOG_GET_OUTPUT_MODE());
    ZINFOF("�ļ�ģʽ: %d", ZLOG_GET_FILE_MODE());
    ZINFOF("��С����: %d", ZLOG_GET_MIN_LEVEL());

    // ����һЩ��־����ͳ��
    for (int i = 0; i < 5; ++i) {
        ZTRACE() << "ͳ�Ʋ���TRACE " << i;
        ZDEBUG() << "ͳ�Ʋ���DEBUG " << i;
        ZINFO() << "ͳ�Ʋ���INFO " << i;
        if (i % 2 == 0) ZWARNING() << "ͳ�Ʋ���WARNING " << i;
        if (i % 3 == 0) ZERROR() << "ͳ�Ʋ���ERROR " << i;
    }

    // �ȴ��첽����
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ZINFO() << "--- ͳ����Ϣ ---";
    ZINFOF("����־����: %zu", ZLOG_GET_TOTAL_COUNT());
    ZINFOF("TRACE����: %zu", ZLOG_GET_LEVEL_COUNT(TRACE));
    ZINFOF("DEBUG����: %zu", ZLOG_GET_LEVEL_COUNT(DEBUG));
    ZINFOF("INFO����: %zu", ZLOG_GET_LEVEL_COUNT(INFO));
    ZINFOF("WARNING����: %zu", ZLOG_GET_LEVEL_COUNT(WARNING));
    ZINFOF("ERROR����: %zu", ZLOG_GET_LEVEL_COUNT(ERROR));
    ZINFOF("��ǰ���д�С: %zu", ZLOG_GET_QUEUE_SIZE());
    ZINFOF("������Ϣ����: %zu", ZLOG_GET_DROPPED_COUNT());
}

//==============================================================================
// 8. ���߳���ʾ
//==============================================================================

void multiThreadDemo() {
    std::cout << "================================== ���߳���ʾ ==================================" << std::endl;

    ZINFO() << "�������߳���־����";

    std::vector<std::thread> threads;

    // ����3�������߳�
    for (int threadId = 0; threadId < 3; ++threadId) {
        threads.emplace_back([threadId]() {
            ZLOG_FUNCTION(); // ÿ���̵߳ĺ���׷��

            ZINFOF("�߳� %d ��ʼִ��", threadId);

            for (int i = 0; i < 5; ++i) {
                ZINFOF("�߳� %d - ���� %d", threadId, i);
                ZDEBUGF("�߳� %d - ������Ϣ %d", threadId, i);

                if (i % 2 == 0) {
                    ZWARNINGF("�߳� %d - ���� %d", threadId, i);
                }

                // ģ�ⲻͬ����ʱ��
                std::this_thread::sleep_for(std::chrono::milliseconds(10 + threadId * 5));
            }

            ZINFOF("�߳� %d ִ�����", threadId);
            });
    }

    // �ȴ������߳����
    for (auto& thread : threads) {
        thread.join();
    }

    ZINFO() << "���̲߳�����ɣ�������־�������߳�ID";
}

//==============================================================================
// 9. ���ù�����ʾ
//==============================================================================

void configurationDemo() {
    std::cout << "================================== ���ù�����ʾ ==================================" << std::endl;

    ZINFO() << "--- �Զ����ļ����� ---";

    // �Զ������������ļ�
    ZLOG_SET_LEVEL_FILE(ERROR, "custom_errors.log");
    ZLOG_SET_LEVEL_FILE(WARNING, "custom_warnings.log");

    ZERROR() << "������������� custom_errors.log";
    ZWARNING() << "������������� custom_warnings.log";
    ZINFO() << "������Ϣ�����Ĭ�ϵ� info_log.txt";

    ZINFO() << "--- ��̬������� ---";

    ZINFOF("��ǰ��С����: %d", ZLOG_GET_MIN_LEVEL());

    // �����С����WARNING
    ZLOG_SET_MIN_LEVEL(WARNING);
    ZINFO() << "����INFO��������������ѵ�����";
    ZDEBUG() << "����DEBUGҲ�������";
    ZWARNING() << "����WARNING�����";
    ZERROR() << "����ERROR�����";

    // �ָ���DEBUG����
    ZLOG_SET_MIN_LEVEL(DEBUG);
    ZINFO() << "�ָ���DEBUG��������INFO�����";

    ZINFO() << "--- ��־��ת ---";

    ZINFO() << "��ǰINFO�ļ�: " << ZLOG_GET_LEVEL_FILE(INFO);
    ZINFO() << "ִ���ֶ���ת";
    ZLOG_ROTATE();
    ZINFO() << "��ת��ɣ����ļ��ѱ���";

    // ��ʾ��ͬ��������
    ZINFO() << "--- ��������ʾ�� ---";

    // ��������
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
    ZLOG_SET_MIN_LEVEL(DEBUG);
    ZINFO() << "����������ȫ���+DEBUG����";

    // ��������
    ZLOG_SET_OUTPUT_MODE(ZLOG_FILE_ONLY, false, "");
    ZLOG_SET_MIN_LEVEL(INFO);
    ZINFO() << "�������������ļ�+INFO����";

    // �ָ�Ĭ��
    ZLOG_SET_OUTPUT_MODE(ZLOG_DEFAULT_MODE, false, "");
    ZLOG_SET_MIN_LEVEL(DEBUG);
    ZINFO() << "�ָ�Ĭ������";
}

//==============================================================================
// ������
//==============================================================================

int use_main() {
    try {
        std::cout << "==================================" << std::endl;
        std::cout << "    ZLogging ��־ϵͳ������ʾ" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "�鿴����̨����� ./logs Ŀ¼�е��ļ�" << std::endl;
        std::cout << std::endl;

        // 1. ���ٿ�ʼ
        quickStart();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 2. ��������
        basicConfiguration();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 3. ��־�����ʽ
        loggingMethods();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 4. ���ģʽ����
        outputModeDemo();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 5. �߼�����
        advancedFeatures();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 6. ���Ժͼ��
        assertionsDemo();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 7. ϵͳ���
        monitoringDemo();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 8. ���߳���ʾ
        multiThreadDemo();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // 9. ���ù���
        configurationDemo();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 10. ϵͳ�ر�
        std::cout << "=== ϵͳ�ر���ʾ ===" << std::endl;
        ZINFO() << "׼���ر���־ϵͳ";
        ZINFOF("����ͳ�� - ����:%zu, ����:%zu, ����:%zu",
            ZLOG_GET_TOTAL_COUNT(), ZLOG_GET_QUEUE_SIZE(), ZLOG_GET_DROPPED_COUNT());

        ZLOG_FLUSH();           // ˢ�´�������־
        ZLOG_SHUTDOWN(3000);    // 3�볬ʱ�ر�

        std::cout << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "           ��ʾ�������" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "�����������ݣ�" << std::endl;
        std::cout << "1. ����̨�Ĳ�ɫ���Ч��" << std::endl;
        std::cout << "2. ./zlog Ŀ¼�е���־�ļ�" << std::endl;
        std::cout << "3. ��ͬ������־���ļ�����" << std::endl;
        std::cout << "4. ���߳���־�е��߳�ID" << std::endl;
        std::cout << "5. ���ܼ�ʱ��������׷��Ч��" << std::endl;

    }
    catch (const std::exception& e) {
        ZFATAL() << "�������쳣: " << e.what();
        ZLOG_SHUTDOWN(1000);
        std::cerr << "�����쳣: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}