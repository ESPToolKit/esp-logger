#include "esp_logger/logger.h"

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#ifndef ESPLOGGER_USE_ESP_LOG
#define ESPLOGGER_USE_ESP_LOG 0
#endif

#if ESPLOGGER_USE_ESP_LOG
#include <esp_log.h>
#include <inttypes.h>
#endif

class LockGuard {
  public:
	explicit LockGuard(SemaphoreHandle_t handle) : _handle(handle) {
		if (_handle != nullptr) {
			xSemaphoreTake(_handle, portMAX_DELAY);
		}
	}

	~LockGuard() {
		if (_handle != nullptr) {
			xSemaphoreGive(_handle);
		}
	}

	LockGuard(const LockGuard &) = delete;
	LockGuard &operator=(const LockGuard &) = delete;

  private:
	SemaphoreHandle_t _handle;
};

constexpr const char *kSyncTaskName = "ESPLoggerSync";

#if ESPLOGGER_USE_ESP_LOG
static void logWithEsp(LogLevel level,
					   const char *tag,
					   uint32_t millisValue,
					   std::time_t timestamp,
					   const std::string &message) {
	const unsigned long millisUnsigned = static_cast<unsigned long>(millisValue);
	const int64_t timestampValue = static_cast<int64_t>(timestamp);

	switch (level) {
	case LogLevel::Debug:
		ESP_LOGD(tag, "[%lu][%" PRIi64 "] %s", millisUnsigned, timestampValue, message.c_str());
		break;
	case LogLevel::Info:
		ESP_LOGI(tag, "[%lu][%" PRIi64 "] %s", millisUnsigned, timestampValue, message.c_str());
		break;
	case LogLevel::Warn:
		ESP_LOGW(tag, "[%lu][%" PRIi64 "] %s", millisUnsigned, timestampValue, message.c_str());
		break;
	case LogLevel::Error:
	default:
		ESP_LOGE(tag, "[%lu][%" PRIi64 "] %s", millisUnsigned, timestampValue, message.c_str());
		break;
	}
}
#endif

static void logWithPrintf(LogLevel level, const char *tag, const std::string &message) {
	switch (level) {
	case LogLevel::Debug:
		printf("[D] [%s] ~ %s\n", tag, message.c_str());
		break;
	case LogLevel::Info:
		printf("[I] [%s] ~ %s\n", tag, message.c_str());
		break;
	case LogLevel::Warn:
		printf("[W] [%s] ~ %s\n", tag, message.c_str());
		break;
	case LogLevel::Error:
	default:
		printf("[E] [%s] ~ %s\n", tag, message.c_str());
		break;
	}
}

static void logToConsole(LogLevel level,
						 const char *tag,
						 uint32_t millisValue,
						 std::time_t timestamp,
						 const std::string &message) {
#if ESPLOGGER_USE_ESP_LOG
	logWithEsp(level, tag, millisValue, timestamp, message);
#else
	(void)millisValue;
	(void)timestamp;
	logWithPrintf(level, tag, message);
#endif
}


ESPLogger::~ESPLogger() {
	deinit();
}

bool ESPLogger::init(const LoggerConfig &config) {
	if (_initialized) {
		deinit();
	}

	LoggerConfig normalized = config;
	if (normalized.maxLogInRam == 0) {
		normalized.maxLogInRam = 1;
	}

	_mutex = xSemaphoreCreateMutex();
	if (_mutex == nullptr) {
		return false;
	}

	{
		LockGuard guard(_mutex);
		_logs.clear();
		_config = normalized;
		_logLevel = _config.consoleLogLevel;
	}

	_running = false;
	const bool shouldCreateTask = _config.enableSyncTask && _config.syncIntervalMS > 0;

	if (shouldCreateTask) {
		_running = true;
		BaseType_t result = xTaskCreatePinnedToCore(
			syncTaskThunk,
			kSyncTaskName,
			_config.stackSize,
			this,
			_config.priority,
			&_syncTask,
			_config.coreId);

		if (result != pdPASS) {
			_running = false;
			LockGuard guard(_mutex);
			_logs.clear();
			_config = LoggerConfig{};
			_logLevel = _config.consoleLogLevel;
			vSemaphoreDelete(_mutex);
			_mutex = nullptr;
			_syncTask = nullptr;
			return false;
		}

	}

	_initialized = true;
	return true;
}

void ESPLogger::deinit() {
	if (!_initialized) {
		return;
	}

	_running = false;

	if (_syncTask != nullptr) {
		vTaskDelete(_syncTask);
		_syncTask = nullptr;
	}

	performSync();

	{
		LockGuard guard(_mutex);
		_logs.clear();
		_syncCallback = nullptr;
		_config = LoggerConfig{};
		_logLevel = _config.consoleLogLevel;
	}

	if (_mutex != nullptr) {
		vSemaphoreDelete(_mutex);
		_mutex = nullptr;
	}

	_initialized = false;
}

void ESPLogger::onSync(SyncCallback callback) {
	LockGuard guard(_mutex);
	_syncCallback = std::move(callback);
}

void ESPLogger::sync() {
	performSync();
}

void ESPLogger::debug(const char *tag, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	logInternal(LogLevel::Debug, tag, fmt, args);
	va_end(args);
}

void ESPLogger::info(const char *tag, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	logInternal(LogLevel::Info, tag, fmt, args);
	va_end(args);
}

void ESPLogger::warn(const char *tag, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	logInternal(LogLevel::Warn, tag, fmt, args);
	va_end(args);
}

void ESPLogger::error(const char *tag, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	logInternal(LogLevel::Error, tag, fmt, args);
	va_end(args);
}

std::vector<Log> ESPLogger::getAllLogs() {
	LockGuard guard(_mutex);
	return std::vector<Log>(_logs.begin(), _logs.end());
}

int ESPLogger::getLogCount(LogLevel level) {
	LockGuard guard(_mutex);
	return static_cast<int>(std::count_if(_logs.begin(), _logs.end(), [level](const Log &entry) {
		return entry.level == level;
	}));
}

std::vector<Log> ESPLogger::getLogs(LogLevel level) {
	LockGuard guard(_mutex);
	std::vector<Log> matches;
	matches.reserve(_logs.size());
	std::copy_if(_logs.begin(), _logs.end(), std::back_inserter(matches), [level](const Log &entry) {
		return entry.level == level;
	});
	return matches;
}

int ESPLogger::getLogCount(const std::vector<Log> &logs, LogLevel level) {
	return static_cast<int>(std::count_if(logs.begin(), logs.end(), [level](const Log &entry) {
		return entry.level == level;
	}));
}

std::vector<Log> ESPLogger::getLogs(const std::vector<Log> &logs, LogLevel level) {
	std::vector<Log> matches;
	matches.reserve(logs.size());
	std::copy_if(logs.begin(), logs.end(), std::back_inserter(matches), [level](const Log &entry) {
		return entry.level == level;
	});
	return matches;
}

std::vector<Log> ESPLogger::getLastLogs(size_t count) {
	LockGuard guard(_mutex);
	if (count == 0 || _logs.empty()) {
		return {};
	}

	const size_t available = _logs.size();
	if (count >= available) {
		return std::vector<Log>(_logs.begin(), _logs.end());
	}

	std::vector<Log> result;
	result.reserve(count);
	const size_t startIndex = available - count;
	size_t index = 0;
	for (const auto &entry : _logs) {
		if (index++ >= startIndex) {
			result.push_back(entry);
		}
	}
	return result;
}

LoggerConfig ESPLogger::currentConfig() const {
	LockGuard guard(_mutex);
	return _config;
}

void ESPLogger::setLogLevel(LogLevel level) {
	LockGuard guard(_mutex);
	_logLevel = level;
	_config.consoleLogLevel = level;
}

LogLevel ESPLogger::logLevel() const {
	LockGuard guard(_mutex);
	return _logLevel;
}

void ESPLogger::logInternal(LogLevel level, const char *tag, const char *fmt, va_list args) {
	if (fmt == nullptr) {
		return;
	}

	va_list argsForMessage;
	va_copy(argsForMessage, args);
	std::string message = formatMessage(fmt, argsForMessage);
	va_end(argsForMessage);

	if (message.empty()) {
		return;
	}

	const char *normalizedTag = tag != nullptr ? tag : "";
	uint32_t nowMillis = millis();
	std::time_t nowUtc = std::time(nullptr);

	bool shouldLogToConsole = false;

	{
		LockGuard guard(_mutex);
		if (!_initialized) {
			return;
		}

		shouldLogToConsole = static_cast<int>(level) >= static_cast<int>(_logLevel);

		if (_logs.size() >= _config.maxLogInRam) {
			_logs.pop_front();
		}

		_logs.emplace_back(Log{level, normalizedTag, nowMillis, nowUtc, message});
	}

	if (shouldLogToConsole) {
		logToConsole(level, normalizedTag, nowMillis, nowUtc, message);
	}
}

std::string ESPLogger::formatMessage(const char *fmt, va_list args) {
	if (fmt == nullptr) {
		return {};
	}

	va_list args_copy;
	va_copy(args_copy, args);
	int required = vsnprintf(nullptr, 0, fmt, args_copy);
	va_end(args_copy);

	if (required <= 0) {
		return {};
	}

	std::vector<char> buffer(static_cast<size_t>(required) + 1, '\0');

	va_list args_copy2;
	va_copy(args_copy2, args);
	vsnprintf(buffer.data(), buffer.size(), fmt, args_copy2);
	va_end(args_copy2);

	return std::string(buffer.data(), static_cast<size_t>(required));
}

void ESPLogger::performSync() {
	SyncCallback callback;
	std::vector<Log> logsSnapshot;

	{
		LockGuard guard(_mutex);
		if (_logs.empty()) {
			return;
		}

		callback = _syncCallback;
		logsSnapshot.reserve(_logs.size());
		std::move(_logs.begin(), _logs.end(), std::back_inserter(logsSnapshot));
		_logs.clear();
	}

	if (callback) {
		callback(logsSnapshot);
	}
}

void ESPLogger::syncTaskThunk(void *arg) {
	auto *instance = static_cast<ESPLogger *>(arg);
	if (instance != nullptr) {
		instance->syncTaskLoop();
	}
	vTaskDelete(nullptr);
}

void ESPLogger::syncTaskLoop() {
	while (_running) {
		vTaskDelay(pdMS_TO_TICKS(_config.syncIntervalMS));
		if (!_running) {
			break;
		}
		performSync();
	}
}
