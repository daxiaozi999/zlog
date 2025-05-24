#include "zlogging.h"

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#define ACCESS _access
#define MKDIR(path) _mkdir(path)
#define STAT _stat
#define STAT_STRUCT struct _stat
#else
#include <unistd.h>
#include <sys/stat.h>
#define ACCESS access
#define MKDIR(path) mkdir(path, 0755)
#define STAT stat
#define STAT_STRUCT struct stat
#endif

namespace zlog {

	std::unique_ptr<ZLogging> ZLogging::instance_ = nullptr;
	std::once_flag ZLogging::initFlag_;

	thread_local std::string ZLogging::tlsFormatBuffer_;
	thread_local std::string ZLogging::tlsTimestampBuffer_;
	thread_local std::string ZLogging::tlsFilenameBuffer_;

	ZLogging::ZLogging()
		: stopWorker_(false)
		, initialized_(false)
		, outputMode_(ZLOG_DEFAULT_MODE)
		, fileMode_(ALWAYS_OPEN)
		, minLevel_(ZLOG_INFO)
		, singleFileOutput_(false)
		, singleFileLevel_(ZLOG_INFO)
		, singleFilePath_("")
		, programName_(DEFAULT_PROGRAM_NAME)
		, outputDir_(DEFAULT_OUTPUT_DIR)
		, maxLogSize_(DEFAULT_MAX_LOG_SIZE)
		, maxCacheSize_(DEFAULT_MAX_CACHE_SIZE)
		, maxBufferSize_(DEFAULT_MAX_BUFFER_SIZE)
		, rotatePolicy_(NO_ROTATE)
		, totalLogCount_(0)
		, sequenceCounter_(0)
		, droppedMessageCount_(0)
		, lastRotateTime_(0) {

		for (int i = ZLOG_TRACE; i <= ZLOG_FATAL; ++i) {
			levelLogCounts_[static_cast<ZLogLevel>(i)] = 0;
		}

	}

	ZLogging::~ZLogging() {
		shutdown();
	}

	ZLogging& ZLogging::getInstance() {
		std::call_once(initFlag_, []() {
			instance_ = std::unique_ptr<ZLogging>(new ZLogging());
			});
		return *instance_;
	}

	int ZLogging::initialize() {
		std::lock_guard<std::mutex> lock(configMutex_);
		if (initialized_.load()) {
			return 0;
		}

		createOutputDirectory();
		initializeFilePaths();

		stopWorker_.store(false);
		asyncWorker_ = std::thread(&ZLogging::runAsyncWorker, this);

		if ((outputMode_ & FILE_OUT) && (fileMode_ == ALWAYS_OPEN)) {
			openLogFiles();
		}

		initialized_.store(true);

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		logDirect(ZLOG_DEBUG, "ZLogging system initialized successfully", __FILE__, __FUNCTION__, __LINE__);

		return 0;
	}

	int ZLogging::setProgramName(const std::string& name) {
		std::lock_guard<std::mutex> lock(configMutex_);
		if (name.empty()) {
			return -1;
		}
		programName_ = name;
		return 0;
	}

	int ZLogging::setOutputDirectory(const std::string& dir) {
		std::lock_guard<std::mutex> lock(configMutex_);
		if (dir.empty()) {
			return -1;
		}

		if (outputDir_ == dir) {
			return 0;
		}

		std::string oldDir = outputDir_;
		outputDir_ = dir;

		for (auto& pair : filePaths_) {
			size_t pos = pair.second.find_last_of("/\\");
			if (pos != std::string::npos) {
				std::string filename = pair.second.substr(pos + 1);
				pair.second = outputDir_ + "/" + filename;
			}
		}

		if (!singleFilePath_.empty()) {
			size_t pos = singleFilePath_.find_last_of("/\\");
			if (pos != std::string::npos) {
				std::string filename = singleFilePath_.substr(pos + 1);
				singleFilePath_ = outputDir_ + "/" + filename;
			}
		}

		if (initialized_.load()) {
			std::lock_guard<std::mutex> fileLock(fileMutex_);
			closeLogFiles();
			createOutputDirectory();

			if ((outputMode_ & FILE_OUT) && (fileMode_ == ALWAYS_OPEN)) {
				openLogFiles();
			}
		}

		return 0;
	}

	int ZLogging::setMaxLogSize(size_t size) {
		std::lock_guard<std::mutex> lock(configMutex_);
		if (size == 0) {
			return -1;
		}
		maxLogSize_ = size;
		return 0;
	}

	int ZLogging::setMaxCacheSize(size_t size) {
		std::lock_guard<std::mutex> lock(configMutex_);
		if (size == 0) {
			return -1;
		}
		maxCacheSize_ = size;
		return 0;
	}

	int ZLogging::setMaxBufferSize(size_t size) {
		std::lock_guard<std::mutex> lock(configMutex_);
		if (size == 0) {
			return -1;
		}
		maxBufferSize_ = size;
		return 0;
	}

	int ZLogging::setMinLevel(ZLogLevel level) {
		std::lock_guard<std::mutex> lock(configMutex_);
		minLevel_ = level;
		return 0;
	}

	int ZLogging::setLevelFile(ZLogLevel level, const std::string& fileName) {
		std::lock_guard<std::mutex> lock(configMutex_);
		if (fileName.empty()) {
			return -1;
		}

		std::string fullPath;
		if (fileName.find('/') != std::string::npos || fileName.find('\\') != std::string::npos) {
			fullPath = fileName;
		}
		else {
			fullPath = outputDir_ + "/" + fileName;
		}

		filePaths_[level] = fullPath;

		if (initialized_.load() && (outputMode_ & FILE_OUT) && (fileMode_ == ALWAYS_OPEN) && !singleFileOutput_) {
			std::lock_guard<std::mutex> fileLock(fileMutex_);

			auto it = fileStreams_.find(level);
			if (it != fileStreams_.end() && it->second) {
				it->second->close();
			}

			size_t pos = fullPath.find_last_of("/\\");
			if (pos != std::string::npos) {
				std::string dir = fullPath.substr(0, pos);
				if (!pathExists(dir)) {
					createDirectoryRecursive(dir);
				}
			}

			fileStreams_[level] = std::make_shared<std::ofstream>(fullPath, std::ios::app);
			if (fileStreams_[level]->is_open()) {
				fileStreams_[level]->rdbuf()->pubsetbuf(nullptr, maxBufferSize_);
			}
			else {
				fileStreams_[level].reset();
			}
		}

		return 0;
	}

	int ZLogging::setOutputMode(int mode, bool singleFile, ZLogLevel level) {
		std::lock_guard<std::mutex> lock(configMutex_);

		outputMode_ = mode;
		singleFileOutput_ = singleFile;
		singleFileLevel_ = level;

		if (singleFile) {
			auto it = filePaths_.find(level);
			if (it != filePaths_.end()) {
				singleFilePath_ = it->second;
			}
			else {
				singleFilePath_ = outputDir_ + "/" + DEFAULT_OUTPUT_FILE;
			}
		}

		if (initialized_.load()) {
			std::lock_guard<std::mutex> fileLock(fileMutex_);

			if (mode & FILE_OUT) {
				if (fileMode_ == ALWAYS_OPEN) {
					closeLogFiles();
					openLogFiles();
				}
			}
			else {
				closeLogFiles();
			}
		}

		return 0;
	}

	int ZLogging::setOutputMode(int mode, bool singleFile, const std::string& filePath) {
		std::lock_guard<std::mutex> lock(configMutex_);

		outputMode_ = mode;
		singleFileOutput_ = singleFile;

		if (singleFile && !filePath.empty()) {
			if (filePath.find('/') != std::string::npos || filePath.find('\\') != std::string::npos) {
				singleFilePath_ = filePath;
			}
			else {
				singleFilePath_ = outputDir_ + "/" + filePath;
			}
		}
		else if (singleFile) {
			singleFilePath_ = outputDir_ + "/" + DEFAULT_OUTPUT_FILE;
		}

		if (initialized_.load()) {
			std::lock_guard<std::mutex> fileLock(fileMutex_);

			if (mode & FILE_OUT) {
				if (fileMode_ == ALWAYS_OPEN) {
					closeLogFiles();
					openLogFiles();
				}
			}
			else {
				closeLogFiles();
			}
		}

		return 0;
	}

	int ZLogging::setFileMode(ZLogFileMode mode) {
		if (mode != ALWAYS_OPEN && mode != OPEN_ON_WRITE) {
			return -1;
		}

		std::lock_guard<std::mutex> lock(configMutex_);
		ZLogFileMode oldMode = fileMode_;
		fileMode_ = mode;

		if (initialized_.load() && (outputMode_ & FILE_OUT)) {
			std::lock_guard<std::mutex> fileLock(fileMutex_);
			if (oldMode == ALWAYS_OPEN && mode == OPEN_ON_WRITE) {
				closeLogFiles();
			}
			else if (oldMode == OPEN_ON_WRITE && mode == ALWAYS_OPEN) {
				openLogFiles();
			}
		}

		return 0;
	}

	int ZLogging::setRotatePolicy(ZLogRotatePolicy policy) {
		std::lock_guard<std::mutex> lock(configMutex_);
		rotatePolicy_ = policy;
		return 0;
	}

	void ZLogging::writeLog(const ZLogEntry& entry) {
		if (!initialized_.load() || !shouldOutput(entry.level)) {
			return;
		}

		ZLogEntry entryCopy = entry;
		entryCopy.sequence = sequenceCounter_.fetch_add(1) + 1;

		{
			std::lock_guard<std::mutex> queueLock(queueMutex_);

			totalLogCount_.fetch_add(1);
			levelLogCounts_[entry.level].fetch_add(1);

			if (messageQueue_.size() >= maxCacheSize_) {
				droppedMessageCount_.fetch_add(1);
				return;
			}

			messageQueue_.push_back(std::move(entryCopy));
		}

		queueCondition_.notify_one();
	}

	void ZLogging::writeLog(ZLogEntry&& entry) {
		if (!initialized_.load() || !shouldOutput(entry.level)) {
			return;
		}

		entry.sequence = sequenceCounter_.fetch_add(1) + 1;
		ZLogLevel level = entry.level;

		{
			std::lock_guard<std::mutex> queueLock(queueMutex_);

			totalLogCount_.fetch_add(1);
			levelLogCounts_[level].fetch_add(1);

			if (messageQueue_.size() >= maxCacheSize_) {
				droppedMessageCount_.fetch_add(1);
				return;
			}

			messageQueue_.emplace_back(std::move(entry));
		}

		queueCondition_.notify_one();
	}

	void ZLogging::logDirect(ZLogLevel level, const std::string& msg, const std::string& filePath, const std::string& function, int line) {
		if (!shouldOutput(level)) {
			return;
		}
		ZLogEntry entry(level, msg, filePath, function, line);
		writeLog(std::move(entry));
	}

	void ZLogging::logDirect(ZLogLevel level, std::string&& msg, const std::string& filePath, const std::string& function, int line) {
		if (!shouldOutput(level)) {
			return;
		}
		ZLogEntry entry(level, std::move(msg), filePath, function, line);
		writeLog(std::move(entry));
	}

	ZLogStream ZLogging::createStream(ZLogLevel level, const std::string& filePath, const std::string& functionName, int line) {
		return ZLogStream(this, level, filePath, functionName, line);
	}

	void ZLogging::flush() {
		auto startTime = std::chrono::steady_clock::now();
		const auto timeout = std::chrono::milliseconds(1000);

		while (true) {
			{
				std::lock_guard<std::mutex> queueLock(queueMutex_);
				if (messageQueue_.empty()) {
					break;
				}
			}

			if (std::chrono::steady_clock::now() - startTime > timeout) {
				break;
			}

			queueCondition_.notify_one();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		if (fileMode_ == ALWAYS_OPEN) {
			std::lock_guard<std::mutex> fileLock(fileMutex_);

			if (singleFileOutput_ && singleFileStream_ && singleFileStream_->is_open()) {
				singleFileStream_->flush();
			}
			else {
				for (auto& stream : fileStreams_) {
					if (stream.second && stream.second->is_open()) {
						stream.second->flush();
					}
				}
			}
		}
	}

	void ZLogging::shutdown(int timeoutMs) {
		if (!initialized_.load()) {
			return;
		}

		initialized_.store(false);

		auto startTime = std::chrono::steady_clock::now();
		auto timeout = std::chrono::milliseconds(timeoutMs);

		while (true) {
			{
				std::lock_guard<std::mutex> queueLock(queueMutex_);
				if (messageQueue_.empty()) {
					break;
				}
			}

			if (std::chrono::steady_clock::now() - startTime > timeout) {
				std::lock_guard<std::mutex> queueLock(queueMutex_);
				size_t remaining = messageQueue_.size();
				if (remaining > 0) {
					droppedMessageCount_.fetch_add(remaining);
					messageQueue_.clear();
				}
				break;
			}

			queueCondition_.notify_one();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		stopWorker_.store(true);
		queueCondition_.notify_one();

		if (asyncWorker_.joinable()) {
			asyncWorker_.join();
		}

		{
			std::lock_guard<std::mutex> fileLock(fileMutex_);
			closeLogFiles();
		}
	}

	void ZLogging::rotateLogFiles() {
		std::lock_guard<std::mutex> lock(fileMutex_);

		if (singleFileOutput_) {
			if (singleFileStream_ && singleFileStream_->is_open()) {
				singleFileStream_->close();

				std::string newFileName = generateRotatedFileName(singleFilePath_);
				if (std::rename(singleFilePath_.c_str(), newFileName.c_str()) == 0) {
					singleFileStream_ = std::make_shared<std::ofstream>(singleFilePath_, std::ios::app);
					if (singleFileStream_->is_open()) {
						singleFileStream_->rdbuf()->pubsetbuf(nullptr, maxBufferSize_);
					}
				}
			}
		}
		else {
			for (auto& pair : fileStreams_) {
				ZLogLevel level = pair.first;
				auto& stream = pair.second;

				if (stream && stream->is_open()) {
					stream->close();

					auto it = filePaths_.find(level);
					if (it != filePaths_.end()) {
						std::string newFileName = generateRotatedFileName(it->second);
						if (std::rename(it->second.c_str(), newFileName.c_str()) == 0) {
							stream = std::make_shared<std::ofstream>(it->second, std::ios::app);
							if (stream->is_open()) {
								stream->rdbuf()->pubsetbuf(nullptr, maxBufferSize_);
							}
						}
					}
				}
			}
		}
	}

	bool ZLogging::isInitialized() const {
		return initialized_.load();
	}

	bool ZLogging::shouldOutput(ZLogLevel level) const {
		return initialized_.load() && level >= minLevel_;
	}

	std::string ZLogging::getOutputDirectory() const {
		std::lock_guard<std::mutex> lock(configMutex_);
		return outputDir_;
	}

	std::string ZLogging::getLogFilePath(ZLogLevel level) const {
		std::lock_guard<std::mutex> lock(configMutex_);
		auto it = filePaths_.find(level);
		return (it != filePaths_.end()) ? it->second : "";
	}

	std::string ZLogging::getUnifiedLogFilePath() const {
		std::lock_guard<std::mutex> lock(configMutex_);
		return singleFileOutput_ ? singleFilePath_ : "";
	}

	int ZLogging::getOutputMode() const {
		std::lock_guard<std::mutex> lock(configMutex_);
		return outputMode_;
	}

	ZLogLevel ZLogging::getMinLevel() const {
		std::lock_guard<std::mutex> lock(configMutex_);
		return minLevel_;
	}

	ZLogFileMode ZLogging::getFileMode() const {
		std::lock_guard<std::mutex> lock(configMutex_);
		return fileMode_;
	}

	size_t ZLogging::getMaxCacheSize() const {
		std::lock_guard<std::mutex> lock(configMutex_);
		return maxCacheSize_;
	}

	size_t ZLogging::getQueueSize() const {
		std::lock_guard<std::mutex> lock(queueMutex_);
		return messageQueue_.size();
	}

	size_t ZLogging::getTotalLogCount() const {
		return totalLogCount_.load();
	}

	size_t ZLogging::getLogCount(ZLogLevel level) const {
		auto it = levelLogCounts_.find(level);
		return (it != levelLogCounts_.end()) ? it->second.load() : 0;
	}

	size_t ZLogging::getDroppedMessageCount() const {
		return droppedMessageCount_.load();
	}

	void ZLogging::runAsyncWorker() {
		while (!stopWorker_.load()) {
			std::unique_lock<std::mutex> lock(queueMutex_);
			queueCondition_.wait(lock, [this] {
				return !messageQueue_.empty() || stopWorker_.load();
				});

			std::vector<ZLogEntry> batch;
			batch.reserve(std::min(messageQueue_.size(), size_t(100)));

			while (!messageQueue_.empty() && batch.size() < 100) {
				batch.emplace_back(std::move(messageQueue_.front()));
				messageQueue_.pop_front();
			}
			lock.unlock();

			for (const auto& entry : batch) {
				processLogEntry(entry);
			}
		}

		std::lock_guard<std::mutex> lock(queueMutex_);
		while (!messageQueue_.empty()) {
			ZLogEntry entry = std::move(messageQueue_.front());
			messageQueue_.pop_front();
			processLogEntry(entry);
		}
	}

	void ZLogging::processLogEntry(const ZLogEntry& entry) {
		if (outputMode_ & CONSOLE_OUT) {
			writeToConsole(entry);
		}

		if (outputMode_ & FILE_OUT) {
			writeToFile(entry);
		}
	}

	void ZLogging::writeToConsole(const ZLogEntry& entry) {
		bool useColor = (outputMode_ & COLOR_OUT) != 0;
		formatLogEntry(entry, useColor, tlsFormatBuffer_);

		static std::mutex consoleMutex;
		std::lock_guard<std::mutex> consoleLock(consoleMutex);

		if (entry.level >= ZLOG_ERROR) {
			std::cerr << tlsFormatBuffer_ << std::endl;
		}
		else {
			std::cout << tlsFormatBuffer_ << std::endl;
		}
	}

	void ZLogging::writeToFile(const ZLogEntry& entry) {
		formatLogEntry(entry, false, tlsFormatBuffer_);

		switch (fileMode_) {
		case ALWAYS_OPEN: {
			std::lock_guard<std::mutex> fileLock(fileMutex_);

			if (singleFileOutput_) {
				if (singleFileStream_ && singleFileStream_->is_open()) {
					*singleFileStream_ << tlsFormatBuffer_ << std::endl;
					if (entry.level >= ZLOG_WARNING) {
						singleFileStream_->flush();
					}
				}

				if (shouldRotate(singleFileLevel_)) {
					rotateFile(singleFileLevel_);
				}
			}
			else {
				auto it = fileStreams_.find(entry.level);
				if (it != fileStreams_.end() && it->second && it->second->is_open()) {
					*it->second << tlsFormatBuffer_ << std::endl;
					if (entry.level >= ZLOG_WARNING) {
						it->second->flush();
					}
				}

				if (shouldRotate(entry.level)) {
					rotateFile(entry.level);
				}
			}
			break;
		}
		case OPEN_ON_WRITE: {
			std::string filePath;

			{
				std::lock_guard<std::mutex> configLock(configMutex_);
				if (singleFileOutput_) {
					filePath = singleFilePath_;
				}
				else {
					auto it = filePaths_.find(entry.level);
					if (it != filePaths_.end()) {
						filePath = it->second;
					}
				}
			}

			if (!filePath.empty()) {
				size_t pos = filePath.find_last_of("/\\");
				if (pos != std::string::npos) {
					std::string dir = filePath.substr(0, pos);
					if (!pathExists(dir)) {
						createDirectoryRecursive(dir);
					}
				}

				std::ofstream file(filePath, std::ios::app);
				if (file.is_open()) {
					file << tlsFormatBuffer_ << std::endl;
					if (entry.level >= ZLOG_WARNING) {
						file.flush();
					}
				}

				ZLogLevel checkLevel = singleFileOutput_ ? singleFileLevel_ : entry.level;
				if (shouldRotate(checkLevel)) {
					std::lock_guard<std::mutex> fileLock(fileMutex_);
					if (shouldRotate(checkLevel)) {
						rotateFile(checkLevel);
					}
				}
			}
			break;
		}
		}
	}

	void ZLogging::formatLogEntry(const ZLogEntry& entry, bool useColor, std::string& output) const {
		output.clear();
		output.reserve(512);

		if (useColor) {
			output += getLevelColorCode(entry.level);
		}

		formatTimestamp(entry.timestamp, tlsTimestampBuffer_);
		output += "[";
		output += tlsTimestampBuffer_;
		output += "] [";
		output += getLevelName(entry.level);
		output += "] [";

		std::ostringstream tid_oss;
		tid_oss << entry.threadId;
		output += tid_oss.str();
		output += "] [";

		extractFilename(entry.filePath, tlsFilenameBuffer_);
		output += tlsFilenameBuffer_;
		if (entry.lineNumber > 0) {
			output += ":";
			output += std::to_string(entry.lineNumber);
		}
		output += "]";

		if (!entry.functionName.empty()) {
			output += " [";
			output += entry.functionName;
			output += "]";
		}

		if (entry.sequence > 0) {
			output += " #";
			output += std::to_string(entry.sequence);
		}

		output += " ";
		output += entry.message;

		if (useColor) {
			output += getColorReset();
		}
	}

	void ZLogging::extractFilename(const std::string& filePath, std::string& output) const {
		size_t pos = filePath.find_last_of("/\\");
		if (pos != std::string::npos) {
			output = filePath.substr(pos + 1);
		}
		else {
			output = filePath;
		}
	}

	void ZLogging::formatTimestamp(const std::chrono::system_clock::time_point timestamp, std::string& output) const {
		auto time_t = std::chrono::system_clock::to_time_t(timestamp);
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;

		thread_local char timeBuffer[32];

#ifdef _WIN32
		struct tm tm_buf;
		if (localtime_s(&tm_buf, &time_t) == 0) {
			std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &tm_buf);
		}
		else {
			ZLOG_SNPRINTF(timeBuffer, sizeof(timeBuffer), "INVALID_TIME");
		}
#else
		struct tm tm_buf;
		if (localtime_r(&time_t, &tm_buf) != nullptr) {
			std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &tm_buf);
		}
		else {
			ZLOG_SNPRINTF(timeBuffer, sizeof(timeBuffer), "INVALID_TIME");
		}
#endif

		output.clear();
		output.reserve(32);
		output += timeBuffer;
		output += ".";

		if (ms.count() < 10) {
			output += "00";
		}
		else if (ms.count() < 100) {
			output += "0";
		}
		output += std::to_string(ms.count());
	}

	std::string ZLogging::generateRotatedFileName(const std::string& originalPath) const {
		auto now = std::chrono::system_clock::now();
		auto time_t = std::chrono::system_clock::to_time_t(now);

		char timeStr[32];

#ifdef _WIN32
		struct tm tm_buf;
		if (localtime_s(&tm_buf, &time_t) == 0) {
			std::strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S", &tm_buf);
		}
		else {
			ZLOG_SNPRINTF(timeStr, sizeof(timeStr), "backup_%lld", static_cast<long long>(time_t));
		}
#else
		struct tm tm_buf;
		if (localtime_r(&time_t, &tm_buf) != nullptr) {
			std::strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S", &tm_buf);
		}
		else {
			ZLOG_SNPRINTF(timeStr, sizeof(timeStr), "backup_%lld", static_cast<long long>(time_t));
		}
#endif

		size_t dotPos = originalPath.find_last_of('.');
		if (dotPos != std::string::npos) {
			return originalPath.substr(0, dotPos) + "_" + timeStr + originalPath.substr(dotPos);
		}
		else {
			return originalPath + "_" + timeStr;
		}
	}

	void ZLogging::initializeFilePaths() {
		const char* sep =
#ifdef _WIN32
			"\\";
#else
			"/";
#endif

		if (filePaths_.find(ZLOG_TRACE) == filePaths_.end()) {
			filePaths_[ZLOG_TRACE] = outputDir_ + sep + DEFAULT_TRACE_FILE;
		}
		if (filePaths_.find(ZLOG_DEBUG) == filePaths_.end()) {
			filePaths_[ZLOG_DEBUG] = outputDir_ + sep + DEFAULT_DEBUG_FILE;
		}
		if (filePaths_.find(ZLOG_INFO) == filePaths_.end()) {
			filePaths_[ZLOG_INFO] = outputDir_ + sep + DEFAULT_INFO_FILE;
		}
		if (filePaths_.find(ZLOG_WARNING) == filePaths_.end()) {
			filePaths_[ZLOG_WARNING] = outputDir_ + sep + DEFAULT_WARNING_FILE;
		}
		if (filePaths_.find(ZLOG_ERROR) == filePaths_.end()) {
			filePaths_[ZLOG_ERROR] = outputDir_ + sep + DEFAULT_ERROR_FILE;
		}
		if (filePaths_.find(ZLOG_FATAL) == filePaths_.end()) {
			filePaths_[ZLOG_FATAL] = outputDir_ + sep + DEFAULT_FATAL_FILE;
		}
	}

	void ZLogging::createOutputDirectory() {
		if (!pathExists(outputDir_)) {
			createDirectoryRecursive(outputDir_);
		}
	}

	void ZLogging::createDirectoryRecursive(const std::string& path) {
		if (path.empty() || pathExists(path)) {
			return;
		}

		size_t pos = path.find_last_of("/\\");
		if (pos != std::string::npos) {
			std::string parentDir = path.substr(0, pos);
			if (!parentDir.empty() && parentDir != "." && parentDir != "..") {
				createDirectoryRecursive(parentDir);
			}
		}

		int ret = MKDIR(path.c_str());
		if (ret != 0 && !pathExists(path)) {
			if (initialized_.load()) {
				logDirect(ZLOG_ERROR, "Failed to create directory: " + path, __FILE__, __FUNCTION__, __LINE__);
			}
		}
	}

	bool ZLogging::pathExists(const std::string& path) const {
		return ACCESS(path.c_str(), 0) == 0;
	}

	size_t ZLogging::getFileSize(const std::string& filePath) const {
		STAT_STRUCT st;
		if (STAT(filePath.c_str(), &st) == 0) {
			return static_cast<size_t>(st.st_size);
		}
		return 0;
	}

	void ZLogging::openLogFiles() {
		if (singleFileOutput_) {
			if (!singleFilePath_.empty()) {
				size_t pos = singleFilePath_.find_last_of("/\\");
				if (pos != std::string::npos) {
					std::string dir = singleFilePath_.substr(0, pos);
					if (!pathExists(dir)) {
						createDirectoryRecursive(dir);
					}
				}

				if (singleFileStream_) {
					singleFileStream_->close();
				}

				singleFileStream_ = std::make_shared<std::ofstream>(singleFilePath_, std::ios::app);
				if (singleFileStream_->is_open()) {
					singleFileStream_->rdbuf()->pubsetbuf(nullptr, maxBufferSize_);
				}
				else {
					singleFileStream_.reset();
				}
			}
		}
		else {
			for (const auto& pair : filePaths_) {
				ZLogLevel level = pair.first;
				const std::string& filepath = pair.second;

				size_t pos = filepath.find_last_of("/\\");
				if (pos != std::string::npos) {
					std::string dir = filepath.substr(0, pos);
					if (!pathExists(dir)) {
						createDirectoryRecursive(dir);
					}
				}

				if (fileStreams_[level]) {
					fileStreams_[level]->close();
				}

				fileStreams_[level] = std::make_shared<std::ofstream>(filepath, std::ios::app);
				if (fileStreams_[level]->is_open()) {
					fileStreams_[level]->rdbuf()->pubsetbuf(nullptr, maxBufferSize_);
				}
				else {
					fileStreams_[level].reset();
				}
			}
		}
	}

	void ZLogging::closeLogFiles() {
		if (singleFileStream_ && singleFileStream_->is_open()) {
			singleFileStream_->close();
		}
		singleFileStream_.reset();

		for (auto& stream : fileStreams_) {
			if (stream.second && stream.second->is_open()) {
				stream.second->close();
			}
		}
		fileStreams_.clear();
	}

	bool ZLogging::shouldRotate(ZLogLevel level) const {
		if (rotatePolicy_ == NO_ROTATE) {
			return false;
		}

		std::string filePath;
		if (singleFileOutput_) {
			filePath = singleFilePath_;
		}
		else {
			auto it = filePaths_.find(level);
			if (it == filePaths_.end()) {
				return false;
			}
			filePath = it->second;
		}

		switch (rotatePolicy_) {
		case SIZE_ROTATE:
			return getFileSize(filePath) >= maxLogSize_;

		case TIME_ROTATE:
		case DAILY_ROTATE: {
			auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			auto lastRotate = lastRotateTime_.load();

			if (lastRotate == 0) {
				lastRotateTime_.store(now);
				return false;
			}

#ifdef _WIN32
			struct tm nowTm, lastTm;
			if (localtime_s(&nowTm, &now) != 0 || localtime_s(&lastTm, &lastRotate) != 0) {
				return false;
			}
#else
			struct tm nowTm, lastTm;
			if (localtime_r(&now, &nowTm) == nullptr || localtime_r(&lastRotate, &lastTm) == nullptr) {
				return false;
			}
#endif

			if (nowTm.tm_mday != lastTm.tm_mday ||
				nowTm.tm_mon != lastTm.tm_mon ||
				nowTm.tm_year != lastTm.tm_year) {
				return true;
			}
			return false;
		}

		default:
			return false;
		}
	}

	void ZLogging::rotateFile(ZLogLevel level) {
		std::string filePath;
		std::shared_ptr<std::ofstream> stream;

		if (singleFileOutput_) {
			filePath = singleFilePath_;
			stream = singleFileStream_;
		}
		else {
			auto pathIt = filePaths_.find(level);
			if (pathIt == filePaths_.end()) {
				return;
			}
			filePath = pathIt->second;

			auto streamIt = fileStreams_.find(level);
			if (streamIt != fileStreams_.end()) {
				stream = streamIt->second;
			}
		}

		if (stream && stream->is_open()) {
			stream->flush();
			stream->close();
		}

		std::string newFileName = generateRotatedFileName(filePath);

		if (std::rename(filePath.c_str(), newFileName.c_str()) != 0) {
			try {
				std::ifstream src(filePath, std::ios::binary);
				if (!src) {
					if (initialized_.load()) {
						logDirect(ZLOG_ERROR, "Failed to open source file for rotation: " + filePath,
								  __FILE__, __FUNCTION__, __LINE__);
					}
				}
				else {
					std::ofstream dst(newFileName, std::ios::binary);
					if (!dst) {
						if (initialized_.load()) {
							logDirect(ZLOG_ERROR, "Failed to create rotated file: " + newFileName,
									  __FILE__, __FUNCTION__, __LINE__);
						}
					}
					else {
						dst << src.rdbuf();

						if (src.bad() || dst.bad()) {
							if (initialized_.load()) {
								logDirect(ZLOG_ERROR, "Error occurred during file rotation copy",
									__FILE__, __FUNCTION__, __LINE__);
							}
						}
						else {
							src.close();
							dst.close();

							std::ofstream clear(filePath, std::ios::trunc);
							if (!clear) {
								if (initialized_.load()) {
									logDirect(ZLOG_ERROR, "Failed to truncate original file after rotation: " + filePath,
										__FILE__, __FUNCTION__, __LINE__);
								}
							}
							clear.close();
						}
					}
				}
			}
			catch (const std::exception& e) {
				if (initialized_.load()) {
					logDirect(ZLOG_ERROR, "Exception during file rotation: " + std::string(e.what()),
						__FILE__, __FUNCTION__, __LINE__);
				}
			}
		}

		if (fileMode_ == ALWAYS_OPEN) {
			try {
				stream = std::make_shared<std::ofstream>(filePath, std::ios::app);
				if (stream && stream->is_open()) {
					stream->rdbuf()->pubsetbuf(nullptr, maxBufferSize_);
				}
				else {
					if (initialized_.load()) {
						logDirect(ZLOG_ERROR, "Failed to reopen file after rotation: " + filePath,
							__FILE__, __FUNCTION__, __LINE__);
					}
					stream.reset();
				}

				if (singleFileOutput_) {
					singleFileStream_ = stream;
				}
				else {
					fileStreams_[level] = stream;
				}
			}
			catch (const std::exception& e) {
				if (initialized_.load()) {
					logDirect(ZLOG_ERROR, "Exception when reopening file after rotation: " + std::string(e.what()),
						__FILE__, __FUNCTION__, __LINE__);
				}
			}
		}

		if (rotatePolicy_ == TIME_ROTATE || rotatePolicy_ == DAILY_ROTATE) {
			lastRotateTime_.store(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
		}
	}

} // namespace zlog