#ifndef __ZLOG_LOGGING__
#define __ZLOG_LOGGING__

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <mutex>
#include <thread>
#include <chrono>
#include <ctime>
#include <deque>
#include <map>
#include <memory>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <future>

namespace zlog {

	static const std::string DEFAULT_PROGRAM_NAME = "main";

	static const std::string DEFAULT_OUTPUT_DIR  = "./zlog";
	static const std::string DEFAULT_OUTPUT_FILE = "log.txt";

	static const std::string DEFAULT_TRACE_FILE   = "trace_log.txt";
	static const std::string DEFAULT_DEBUG_FILE   = "debug_log.txt";
	static const std::string DEFAULT_INFO_FILE    = "info_log.txt";
	static const std::string DEFAULT_WARNING_FILE = "warning_log.txt";
	static const std::string DEFAULT_ERROR_FILE   = "error_log.txt";
	static const std::string DEFAULT_FATAL_FILE   = "fatal_log.txt";

	static const size_t DEFAULT_MAX_LOG_SIZE     = 100 * 1024 * 1024;
	static const size_t DEFAULT_MAX_CACHE_SIZE   = 1000;
	static const size_t DEFAULT_MAX_BUFFER_SIZE  = 10 * 1024;
	static const size_t DEFAULT_MAX_MESSAGE_SIZE = 4 * 1024;

	enum ZLogLevel {
		ZLOG_TRACE,
		ZLOG_DEBUG,
		ZLOG_INFO,
		ZLOG_WARNING,
		ZLOG_ERROR,
		ZLOG_FATAL,

#ifndef ZLOG_NO_ABBREVIATED_SEVERITIES
# ifdef ERROR
#	error ERROR macro is defined. Define ZLOG_NO_ABBREVIATED_SEVERITIES before including logging.h.
# endif
		TRACE   = ZLOG_TRACE,
		DEBUG   = ZLOG_DEBUG,
		INFO    = ZLOG_INFO,
		WARNING = ZLOG_WARNING,
		ERROR   = ZLOG_ERROR,
		FATAL   = ZLOG_FATAL
#endif
	};

	enum ZLogOutputMode {
		CONSOLE_OUT = 1 << 0,
		FILE_OUT    = 1 << 2,
		COLOR_OUT   = 1 << 3
	};

	enum ZLogFileMode {
		ALWAYS_OPEN,
		OPEN_ON_WRITE
	};

	enum ZLogRotatePolicy {
		NO_ROTATE,
		SIZE_ROTATE,
		TIME_ROTATE,
		DAILY_ROTATE
	};

	inline const char* getLevelName(ZLogLevel level) {
		switch (level) {
		case ZLOG_TRACE:   return "TRACE";
		case ZLOG_DEBUG:   return "DEBUG";
		case ZLOG_INFO:    return "INFO";
		case ZLOG_WARNING: return "WARNING";
		case ZLOG_ERROR:   return "ERROR";
		case ZLOG_FATAL:   return "FATAL";
		default:           return "UNKNOWN";
		}
	}

	inline const char* getLevelColorCode(ZLogLevel level) {
		switch (level) {
		case ZLOG_TRACE:    return "\033[90m";
		case ZLOG_DEBUG:    return "\033[36m";
		case ZLOG_INFO:     return "\033[32m";
		case ZLOG_WARNING:  return "\033[33m";
		case ZLOG_ERROR:    return "\033[31m";
		case ZLOG_FATAL:    return "\033[35m";
		default:            return "\033[0m";
		}
	}

	inline const char* getColorReset() {
		return "\033[0m";
	}

	struct ZLogEntry {
		ZLogLevel level;
		std::string message;
		std::string filePath;
		std::string functionName;
		std::chrono::system_clock::time_point timestamp;
		std::thread::id threadId;
		int lineNumber;
		size_t sequence;

		ZLogEntry() : level(ZLOG_INFO), lineNumber(0), sequence(0) {}

		ZLogEntry(ZLogLevel l, const std::string& msg, const std::string& file, const std::string& func, int line = 0, size_t seq = 0)
			: level(l), message(msg), filePath(file), functionName(func), lineNumber(line), sequence(seq)
			, timestamp(std::chrono::system_clock::now())
			, threadId(std::this_thread::get_id()) {
		}

		ZLogEntry(ZLogLevel l, std::string&& msg, const std::string& file, const std::string& func, int line, size_t seq = 0)
			: level(l), message(std::move(msg)), filePath(file), functionName(func), lineNumber(line)
			, timestamp(std::chrono::system_clock::now())
			, threadId(std::this_thread::get_id()), sequence(seq) {
		}

		ZLogEntry(const ZLogEntry& other)
			: level(other.level), message(other.message), filePath(other.filePath), functionName(other.functionName)
			, timestamp(other.timestamp), threadId(other.threadId), lineNumber(other.lineNumber), sequence(other.sequence) {
		}

		ZLogEntry& operator=(const ZLogEntry& other) {
			if (this != &other) {
				level = other.level;
				message = other.message;
				filePath = other.filePath;
				functionName = other.functionName;
				timestamp = other.timestamp;
				threadId = other.threadId;
				lineNumber = other.lineNumber;
				sequence = other.sequence;
			}
			return *this;
		}

		ZLogEntry(ZLogEntry&& other) noexcept
			: level(other.level), message(std::move(other.message)), filePath(std::move(other.filePath))
			, functionName(std::move(other.functionName)), timestamp(other.timestamp)
			, threadId(other.threadId), lineNumber(other.lineNumber), sequence(other.sequence) {
		}

		ZLogEntry& operator=(ZLogEntry&& other) noexcept {
			if (this != &other) {
				level = other.level;
				message = std::move(other.message);
				filePath = std::move(other.filePath);
				functionName = std::move(other.functionName);
				timestamp = other.timestamp;
				threadId = other.threadId;
				lineNumber = other.lineNumber;
				sequence = other.sequence;
			}
			return *this;
		}

		~ZLogEntry() = default;
	};

	class ZLogStream;
	class ZLogScope;
	class ZLogTimer;
	class ZLogging;

	class ZLogging {
	public:
		~ZLogging();

		ZLogging(const ZLogging&) = delete;
		ZLogging(ZLogging&&) = delete;
		ZLogging& operator=(const ZLogging&) = delete;
		ZLogging& operator=(ZLogging&&) = delete;

		static ZLogging& getInstance();

		int initialize();

		int setProgramName(const std::string& name);
		int setOutputDirectory(const std::string& dir);
		int setMaxLogSize(size_t size);
		int setMaxCacheSize(size_t size);
		int setMaxBufferSize(size_t size);
		int setMinLevel(ZLogLevel level);
		int setLevelFile(ZLogLevel level, const std::string& fileName);
		int setOutputMode(int mode, bool singleFile = false, ZLogLevel level = ZLOG_INFO);
		int setOutputMode(int mode, bool singleFile = false, const std::string& filePath = "");
		int setFileMode(ZLogFileMode mode);
		int setRotatePolicy(ZLogRotatePolicy policy);

		void writeLog(const ZLogEntry& entry);
		void writeLog(ZLogEntry&& entry);

		void logDirect(ZLogLevel level, const std::string& msg, const std::string& filePath, const std::string& function, int line = 0);
		void logDirect(ZLogLevel level, std::string&& msg, const std::string& filePath, const std::string& function, int line = 0);

		ZLogStream createStream(ZLogLevel level, const std::string& filePath, const std::string& functionName, int line = 0);

		void flush();
		void shutdown(int timeoutMs = 3000);
		void rotateLogFiles();

		bool isInitialized() const;
		bool shouldOutput(ZLogLevel level) const;

		std::string getOutputDirectory() const;
		std::string getLogFilePath(ZLogLevel level) const;
		std::string getUnifiedLogFilePath() const;

		int getOutputMode() const;

		ZLogLevel getMinLevel() const;
		ZLogFileMode getFileMode() const;

		size_t getMaxCacheSize() const;
		size_t getQueueSize() const;
		size_t getTotalLogCount() const;
		size_t getLogCount(ZLogLevel level) const;
		size_t getDroppedMessageCount() const;

	private:
		ZLogging();

		void runAsyncWorker();
		void processLogEntry(const ZLogEntry& entry);
		void writeToConsole(const ZLogEntry& entry);
		void writeToFile(const ZLogEntry& entry);
		void formatLogEntry(const ZLogEntry& entry, bool useColor, std::string& output) const;
		void extractFilename(const std::string& filePath, std::string& output) const;
		void formatTimestamp(const std::chrono::system_clock::time_point timestamp, std::string& output) const;

		std::string generateRotatedFileName(const std::string& originalPath) const;

		void initializeFilePaths();

		void createOutputDirectory();
		void createDirectoryRecursive(const std::string& path);

		bool pathExists(const std::string& path) const;
		size_t getFileSize(const std::string& filePath) const;

		void openLogFiles();
		void closeLogFiles();

		bool shouldRotate(ZLogLevel level) const;
		void rotateFile(ZLogLevel level);

	private:
		static std::unique_ptr<ZLogging> instance_;
		static std::once_flag initFlag_;

		mutable std::mutex configMutex_;
		mutable std::mutex fileMutex_;
		mutable std::mutex queueMutex_;

		std::deque<ZLogEntry> messageQueue_;
		std::condition_variable queueCondition_;
		std::thread asyncWorker_;
		std::atomic<bool> stopWorker_;
		std::atomic<bool> initialized_;

		std::map<ZLogLevel, std::string> filePaths_;
		std::map<ZLogLevel, std::shared_ptr<std::ofstream>> fileStreams_;

		int outputMode_;
		ZLogFileMode fileMode_;
		ZLogLevel minLevel_;

		bool singleFileOutput_;
		ZLogLevel singleFileLevel_;
		std::string singleFilePath_;
		std::shared_ptr<std::ofstream> singleFileStream_;

		std::string programName_;
		std::string outputDir_;
		size_t maxLogSize_;
		size_t maxCacheSize_;
		size_t maxBufferSize_;
		ZLogRotatePolicy rotatePolicy_;

		std::atomic<size_t> totalLogCount_;
		std::atomic<size_t> sequenceCounter_;
		std::atomic<size_t> droppedMessageCount_;
		mutable std::atomic<std::time_t> lastRotateTime_;
		std::map<ZLogLevel, std::atomic<size_t>> levelLogCounts_;

		thread_local static std::string tlsFormatBuffer_;
		thread_local static std::string tlsTimestampBuffer_;
		thread_local static std::string tlsFilenameBuffer_;
	};

	class ZLogStream {
	public:
		ZLogStream(ZLogging* logger, ZLogLevel level, const std::string& filePath, const std::string& functionName, int line = 0)
			: logger_(logger), level_(level), filePath_(filePath), functionName_(functionName), lineNumber_(line)
			, isActive_(logger != nullptr && logger->shouldOutput(level)) {
		}

		ZLogStream(const ZLogStream&) = delete;
		ZLogStream& operator=(const ZLogStream&) = delete;

		ZLogStream(ZLogStream&& other) noexcept
			: logger_(other.logger_),
			level_(other.level_),
			filePath_(std::move(other.filePath_)),
			functionName_(std::move(other.functionName_)),
			lineNumber_(other.lineNumber_),
			stream_(std::move(other.stream_)),
			isActive_(other.isActive_) {
			other.isActive_ = false;
		}

		ZLogStream& operator=(ZLogStream&& other) noexcept {
			if (this != &other) {
				flush();
				logger_ = other.logger_;
				level_ = other.level_;
				filePath_ = std::move(other.filePath_);
				functionName_ = std::move(other.functionName_);
				lineNumber_ = other.lineNumber_;
				stream_ = std::move(other.stream_);
				isActive_ = other.isActive_;
				other.isActive_ = false;
			}
			return *this;
		}

		~ZLogStream() {
			flush();
		}

		template<typename T>
		ZLogStream& operator<<(const T& value) {
			if (isActive_) {
				stream_ << value;
			}
			return *this;
		}

		ZLogStream& operator<<(std::ostream& (*func)(std::ostream&)) {
			if (isActive_) {
				stream_ << func;
			}
			return *this;
		}

		ZLogStream& operator<<(std::ios_base& (*func)(std::ios_base&)) {
			if (isActive_) {
				stream_ << func;
			}
			return *this;
		}

	private:
		void flush() {
			if (isActive_ && logger_) {
				std::string message = stream_.str();
				if (!message.empty()) {
					ZLogEntry entry(level_, std::move(message), filePath_, functionName_, lineNumber_);
					logger_->writeLog(std::move(entry));
				}
				isActive_ = false;
			}
		}

	private:
		ZLogging* logger_;
		ZLogLevel level_;
		std::string filePath_;
		std::string functionName_;
		int lineNumber_;
		std::ostringstream stream_;
		bool isActive_;
	};

	class ZLogScope {
	public:
		ZLogScope(const std::string& functionName, const std::string& filePath, int line = 0)
			: functionName_(functionName), filePath_(filePath), lineNumber_(line) {
			if (ZLogging::getInstance().isInitialized()) {
				ZLogging::getInstance().logDirect(ZLOG_DEBUG, ">>> Enter", filePath_, functionName_, lineNumber_);
			}
		}

		~ZLogScope() {
			if (ZLogging::getInstance().isInitialized()) {
				ZLogging::getInstance().logDirect(ZLOG_DEBUG, "<<< Exit", filePath_, functionName_, lineNumber_);
			}
		}

	private:
		std::string functionName_;
		std::string filePath_;
		int lineNumber_;
	};

	class ZLogTimer {
	public:
		ZLogTimer(const std::string& name, const std::string& fileName, int line = 0)
			: timerName_(name), fileName_(fileName), lineNumber_(line)
			, startTime_(std::chrono::high_resolution_clock::now()) {
		}

		~ZLogTimer() {
			if (ZLogging::getInstance().isInitialized()) {
				auto endTime = std::chrono::high_resolution_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_).count();

				std::string message = "Performance [" + timerName_ + "]: " + std::to_string(duration) + " us";
				ZLogging::getInstance().logDirect(ZLOG_INFO, std::move(message), fileName_, __FUNCTION__, lineNumber_);
			}
		}

	private:
		std::string timerName_;
		std::string fileName_;
		int lineNumber_;
		std::chrono::high_resolution_clock::time_point startTime_;
	};

	inline ZLogging& getLogger() {
		return ZLogging::getInstance();
	}

} // namespace zlog


#define ZLOG(level) \
    (zlog::getLogger().shouldOutput(zlog::level) ? \
     zlog::getLogger().createStream(zlog::level, __FILE__, __FUNCTION__, __LINE__) : \
     zlog::ZLogStream(nullptr, zlog::level, __FILE__, __FUNCTION__, __LINE__))

#define ZLOG_IF(level, condition) \
    (((condition) && zlog::getLogger().shouldOutput(zlog::level)) ? \
     zlog::getLogger().createStream(zlog::level, __FILE__, __FUNCTION__, __LINE__) : \
     zlog::ZLogStream(nullptr, zlog::level, __FILE__, __FUNCTION__, __LINE__))


#define ZTRACE()   ZLOG(TRACE)
#define ZDEBUG()   ZLOG(DEBUG)
#define ZINFO()    ZLOG(INFO)
#define ZWARNING() ZLOG(WARNING)
#define ZERROR()   ZLOG(ERROR)
#define ZFATAL()   ZLOG(FATAL)

#define ZTRACE_IF(cond)   ZLOG_IF(TRACE, cond)
#define ZDEBUG_IF(cond)   ZLOG_IF(DEBUG, cond)
#define ZINFO_IF(cond)    ZLOG_IF(INFO, cond)
#define ZWARNING_IF(cond) ZLOG_IF(WARNING, cond)
#define ZERROR_IF(cond)   ZLOG_IF(ERROR, cond)
#define ZFATAL_IF(cond)   ZLOG_IF(FATAL, cond)

#ifdef _WIN32
#define ZLOG_SNPRINTF(buffer, size, format, ...) _snprintf_s(buffer, size, _TRUNCATE, format, ##__VA_ARGS__)
#else
#define ZLOG_SNPRINTF(buffer, size, format, ...) snprintf(buffer, size, format, ##__VA_ARGS__)
#endif

#define ZLOGF(level, fmt, ...) \
    do { \
        if (zlog::getLogger().shouldOutput(zlog::level)) { \
            thread_local static char buffer[zlog::DEFAULT_MAX_MESSAGE_SIZE]; \
            int ret = ZLOG_SNPRINTF(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
            if (ret > 0 && ret < static_cast<int>(sizeof(buffer))) { \
                zlog::getLogger().logDirect(zlog::level, std::string(buffer), __FILE__, __FUNCTION__, __LINE__); \
            } \
        } \
    } while(0)

#define ZTRACEF(fmt, ...)   ZLOGF(TRACE, fmt, ##__VA_ARGS__)
#define ZDEBUGF(fmt, ...)   ZLOGF(DEBUG, fmt, ##__VA_ARGS__)
#define ZINFOF(fmt, ...)    ZLOGF(INFO, fmt, ##__VA_ARGS__)
#define ZWARNINGF(fmt, ...) ZLOGF(WARNING, fmt, ##__VA_ARGS__)
#define ZERRORF(fmt, ...)   ZLOGF(ERROR, fmt, ##__VA_ARGS__)
#define ZFATALF(fmt, ...)   ZLOGF(FATAL, fmt, ##__VA_ARGS__)

#define ZLOG_FUNCTION() \
    zlog::ZLogScope ZLOG_UNIQUE_VAR(scopedLogger)(__FUNCTION__, __FILE__, __LINE__)

#define ZLOG_SCOPE(name) \
    zlog::ZLogScope ZLOG_UNIQUE_VAR(scopedLogger)(#name, __FILE__, __LINE__)

#define ZLOG_TIMER(name) \
    zlog::ZLogTimer ZLOG_UNIQUE_VAR(perfTimer)(#name, __FILE__, __LINE__)

#define ZLOG_TIMER_BEGIN(name) \
    auto ZLOG_CONCAT(zlog_timer_start_, name) = std::chrono::high_resolution_clock::now()

#define ZLOG_TIMER_END(name) \
    do { \
        auto ZLOG_CONCAT(zlog_timer_end_, name) = std::chrono::high_resolution_clock::now(); \
        auto ZLOG_CONCAT(zlog_timer_duration_, name) = std::chrono::duration_cast<std::chrono::microseconds>( \
             ZLOG_CONCAT(zlog_timer_end_, name) - ZLOG_CONCAT(zlog_timer_start_, name)).count(); \
        ZINFOF("Performance [%s]: %lld us", #name, static_cast<long long>(ZLOG_CONCAT(zlog_timer_duration_, name))); \
    } while(0)

#define ZASSERT(condition, msg) \
    do { \
        if (!(condition)) { \
            ZERRORF("Assertion failed: %s - %s", #condition, msg); \
        } \
    } while(0)

#define ZASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            ZERRORF("Assertion failed: %s != %s - %s", #a, #b, msg); \
        } \
    } while(0)

#define ZASSERT_NE(a, b, msg) \
    do { \
        if ((a) == (b)) { \
            ZERRORF("Assertion failed: %s == %s - %s", #a, #b, msg); \
        } \
    } while(0)

#define ZCHECK(condition) \
    do { \
        if (!(condition)) { \
            ZFATALF("Check failed: %s at %s:%d", #condition, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while(0)

#define ZCHECK_EQ(a, b) ZCHECK((a) == (b))
#define ZCHECK_NE(a, b) ZCHECK((a) != (b))
#define ZCHECK_LT(a, b) ZCHECK((a) < (b))
#define ZCHECK_LE(a, b) ZCHECK((a) <= (b))
#define ZCHECK_GT(a, b) ZCHECK((a) > (b))
#define ZCHECK_GE(a, b) ZCHECK((a) >= (b))

#define ZLOG_CONCAT_IMPL(x, y) x##y
#define ZLOG_CONCAT(x, y) ZLOG_CONCAT_IMPL(x, y)
#define ZLOG_UNIQUE_VAR(prefix) ZLOG_CONCAT(prefix, __LINE__)

#define ZLOG_EVERY_N(level, n) \
    static thread_local int ZLOG_UNIQUE_VAR(counter) = 0; \
    if (++ZLOG_UNIQUE_VAR(counter) % (n) == 1) ZLOG(level)

#define ZLOG_FIRST_N(level, n) \
    static thread_local int ZLOG_UNIQUE_VAR(counter) = 0; \
    if (++ZLOG_UNIQUE_VAR(counter) <= (n)) ZLOG(level)

#define ZLOG_ONCE(level) ZLOG_FIRST_N(level, 1)

#define ZLOG_EVERY_T(level, periodSecond) \
    static thread_local auto ZLOG_UNIQUE_VAR(lastTime) = std::chrono::steady_clock::now(); \
    if (std::chrono::steady_clock::now() - ZLOG_UNIQUE_VAR(lastTime) >= std::chrono::seconds(periodSecond)) { \
        ZLOG_UNIQUE_VAR(lastTime) = std::chrono::steady_clock::now(); \
        ZLOG(level)

#define ZLOG_EVERY_T_END() }

#define ZLOG_CONSOLE_ONLY     zlog::CONSOLE_OUT
#define ZLOG_FILE_ONLY        zlog::FILE_OUT
#define ZLOG_BOTH            (zlog::CONSOLE_OUT | zlog::FILE_OUT)
#define ZLOG_COLORED_CONSOLE (zlog::CONSOLE_OUT | zlog::COLOR_OUT)
#define ZLOG_DEFAULT_MODE    (zlog::CONSOLE_OUT | zlog::FILE_OUT | zlog::COLOR_OUT)

#define ZLOG_INIT()                           zlog::getLogger().initialize()
#define ZLOG_SET_PROGRAM_NAME(name)           zlog::getLogger().setProgramName(name)
#define ZLOG_SET_OUTPUT_DIR(dir)              zlog::getLogger().setOutputDirectory(dir)
#define ZLOG_SET_MAX_LOG_SIZE(size)           zlog::getLogger().setMaxLogSize(size)
#define ZLOG_SET_MAX_CACHE_SIZE(size)         zlog::getLogger().setMaxCacheSize(size)
#define ZLOG_SET_MAX_BUFFER_SIZE(size)        zlog::getLogger().setMaxBufferSize(size)
#define ZLOG_SET_MIN_LEVEL(level)             zlog::getLogger().setMinLevel(zlog::level)
#define ZLOG_SET_OUTPUT_MODE(mode, ...)       zlog::getLogger().setOutputMode(mode, ##__VA_ARGS__)
#define ZLOG_SET_FILE_MODE(mode)              zlog::getLogger().setFileMode(zlog::mode)
#define ZLOG_SET_LEVEL_FILE(level, path)      zlog::getLogger().setLevelFile(zlog::level, path)
#define ZLOG_SET_ROTATE_POLICY(policy)        zlog::getLogger().setRotatePolicy(zlog::policy)

#define ZLOG_FLUSH()                          zlog::getLogger().flush()
#define ZLOG_ROTATE()                         zlog::getLogger().rotateLogFiles()
#define ZLOG_SHUTDOWN(...)                    zlog::getLogger().shutdown(__VA_ARGS__)

#define ZLOG_IS_INITIALIZED()                 zlog::getLogger().isInitialized()
#define ZLOG_IS_ENABLED(level)                zlog::getLogger().shouldOutput(zlog::level)

#define ZLOG_GET_OUTPUT_DIR()                 zlog::getLogger().getOutputDirectory()
#define ZLOG_GET_MAX_CACHE_SIZE()             zlog::getLogger().getMaxCacheSize()
#define ZLOG_GET_OUTPUT_MODE()                zlog::getLogger().getOutputMode()
#define ZLOG_GET_FILE_MODE()                  zlog::getLogger().getFileMode()
#define ZLOG_GET_MIN_LEVEL()                  zlog::getLogger().getMinLevel()
#define ZLOG_GET_LEVEL_FILE(level)            zlog::getLogger().getLogFilePath(zlog::level)
#define ZLOG_GET_UNIFIED_FILE()               zlog::getLogger().getUnifiedLogFilePath()

#define ZLOG_GET_TOTAL_COUNT()                zlog::getLogger().getTotalLogCount()
#define ZLOG_GET_LEVEL_COUNT(level)           zlog::getLogger().getLogCount(zlog::level)
#define ZLOG_GET_QUEUE_SIZE()                 zlog::getLogger().getQueueSize()
#define ZLOG_GET_DROPPED_COUNT()              zlog::getLogger().getDroppedMessageCount()

#ifdef ZLOG_DISABLE_DEBUG
#undef ZDEBUG
#undef ZDEBUGF
#undef ZDEBUG_IF
#define ZDEBUG              zlog::ZLogStream(nullptr, zlog::DEBUG, "", "", 0)
#define ZDEBUGF(fmt, ...)   do {} while(0)
#define ZDEBUG_IF(cond)     zlog::ZLogStream(nullptr, zlog::DEBUG, "", "", 0)
#endif

#ifdef ZLOG_DISABLE_TRACE
#undef ZTRACE
#undef ZTRACEF
#undef ZTRACE_IF
#define ZTRACE              zlog::ZLogStream(nullptr, zlog::TRACE, "", "", 0)
#define ZTRACEF(fmt, ...)   do {} while(0)
#define ZTRACE_IF(cond)     zlog::ZLogStream(nullptr, zlog::TRACE, "", "", 0)
#endif

#ifdef ZLOG_DISABLE_ALL
#undef ZTRACE
#undef ZDEBUG
#undef ZINFO
#undef ZWARNING
#undef ZERROR
#undef ZFATAL
#undef ZTRACEF
#undef ZDEBUGF
#undef ZINFOF
#undef ZWARNINGF
#undef ZERRORF
#undef ZFATALF
#undef ZTRACE_IF
#undef ZDEBUG_IF
#undef ZINFO_IF
#undef ZWARNING_IF
#undef ZERROR_IF
#undef ZFATAL_IF

#define ZTRACE              zlog::ZLogStream(nullptr, zlog::TRACE, "", "", 0)
#define ZDEBUG              zlog::ZLogStream(nullptr, zlog::DEBUG, "", "", 0)
#define ZINFO               zlog::ZLogStream(nullptr, zlog::INFO, "", "", 0)
#define ZWARNING            zlog::ZLogStream(nullptr, zlog::WARNING, "", "", 0)
#define ZERROR              zlog::ZLogStream(nullptr, zlog::ERROR, "", "", 0)
#define ZFATAL              zlog::ZLogStream(nullptr, zlog::FATAL, "", "", 0)
#define ZTRACEF(fmt, ...)   do {} while(0)
#define ZDEBUGF(fmt, ...)   do {} while(0)
#define ZINFOF(fmt, ...)    do {} while(0)
#define ZWARNINGF(fmt, ...) do {} while(0)
#define ZERRORF(fmt, ...)   do {} while(0)
#define ZFATALF(fmt, ...)   do {} while(0)
#define ZTRACE_IF(cond)     zlog::ZLogStream(nullptr, zlog::TRACE, "", "", 0)
#define ZDEBUG_IF(cond)     zlog::ZLogStream(nullptr, zlog::DEBUG, "", "", 0)
#define ZINFO_IF(cond)      zlog::ZLogStream(nullptr, zlog::INFO, "", "", 0)
#define ZWARNING_IF(cond)   zlog::ZLogStream(nullptr, zlog::WARNING, "", "", 0)
#define ZERROR_IF(cond)     zlog::ZLogStream(nullptr, zlog::ERROR, "", "", 0)
#define ZFATAL_IF(cond)     zlog::ZLogStream(nullptr, zlog::FATAL, "", "", 0)
#endif

#endif // ! __ZLOG_LOGGING__