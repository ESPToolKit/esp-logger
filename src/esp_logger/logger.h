#pragma once

#include <Arduino.h>
#include <cstdarg>
#include <ctime>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#if defined(__has_include)
#if __has_include(<ArduinoJson.h>)
#include <ArduinoJson.h>
#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 7
#define ESPLOGGER_HAS_ARDUINOJSON_V7 1
#else
#define ESPLOGGER_HAS_ARDUINOJSON_V7 0
#endif
#else
#define ESPLOGGER_HAS_ARDUINOJSON_V7 0
#endif
#else
#define ESPLOGGER_HAS_ARDUINOJSON_V7 0
#endif

#include "esp_logger/logger_allocator.h"
#include "esp_logger/logger_config.h"

struct Log {
    LogLevel level;
    std::string tag;
    uint32_t millis;
    std::time_t timestamp;
    std::string message;
};

using SyncCallback = std::function<void(const std::vector<Log>&)>;
using LiveCallback = std::function<void(const Log&)>;
using InternalLogDeque = std::deque<Log, LoggerAllocator<Log>>;
using InternalCharVector = std::vector<char, LoggerAllocator<char>>;
using InternalLogVector = std::vector<Log, LoggerAllocator<Log>>;

class ESPLogger {
  public:
    ESPLogger() = default;
    ~ESPLogger();

    bool init(const LoggerConfig& config = LoggerConfig{});
    void deinit();
    bool isInitialized() const { return _initialized; }

    void onSync(SyncCallback callback);
    void attach(LiveCallback callback);
    void detach();

    void sync();

    void debug(const char* tag, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
    void info(const char* tag, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
    void warn(const char* tag, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
    void error(const char* tag, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
#if ESPLOGGER_HAS_ARDUINOJSON_V7
    void debug(const char* tag, const ArduinoJson::JsonDocument& json);
    void info(const char* tag, const ArduinoJson::JsonDocument& json);
    void warn(const char* tag, const ArduinoJson::JsonDocument& json);
    void error(const char* tag, const ArduinoJson::JsonDocument& json);

    void debug(const char* tag, ArduinoJson::JsonVariantConst json);
    void info(const char* tag, ArduinoJson::JsonVariantConst json);
    void warn(const char* tag, ArduinoJson::JsonVariantConst json);
    void error(const char* tag, ArduinoJson::JsonVariantConst json);
#endif

    std::vector<Log> getAllLogs();
    int getLogCount(LogLevel level);
    std::vector<Log> getLogs(LogLevel level);
    static int getLogCount(const std::vector<Log>& logs, LogLevel level);
    static std::vector<Log> getLogs(const std::vector<Log>& logs, LogLevel level);
    std::vector<Log> getLastLogs(size_t count);

    LoggerConfig currentConfig() const;
    void setLogLevel(LogLevel level);
    LogLevel logLevel() const;

  private:
    void logInternal(LogLevel level, const char* tag, const char* fmt, va_list args);
    void logMessage(LogLevel level, const char* tag, std::string message);
    std::string formatMessage(const char* fmt, va_list args);
#if ESPLOGGER_HAS_ARDUINOJSON_V7
    std::string serializeJsonMessage(ArduinoJson::JsonVariantConst json) const;
#endif
    void performSync();
    static void syncTaskThunk(void* arg);
    void syncTaskLoop();

    LoggerConfig _config{};
    bool _initialized = false;
    bool _running = false;
    TaskHandle_t _syncTask = nullptr;
    SemaphoreHandle_t _mutex = nullptr;
    InternalLogDeque _logs;
    SyncCallback _syncCallback;
    LiveCallback _liveCallback;
    LogLevel _logLevel = LogLevel::Debug;
    bool _usePSRAMBuffers = false;
    LoggerAllocator<Log> _logAllocator{};
    LoggerAllocator<char> _charAllocator{};
};
