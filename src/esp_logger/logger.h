#pragma once

#include <Arduino.h>
#include <cstdarg>
#include <ctime>
#include <deque>
#include <functional>
#include <string>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "esp_logger/logger_config.h"

struct Log {
    LogLevel level;
    std::string tag;
    uint32_t millis;
    std::time_t timestamp;
    std::string message;
};

using SyncCallback = std::function<void(const std::vector<Log>&)>;

class ESPLogger {
  public:
    ESPLogger() = default;
    ~ESPLogger();

    bool init(const LoggerConfig& config = LoggerConfig{});
    void deinit();
    bool isInitialized() const { return _initialized; }

    void onSync(SyncCallback callback);

    void sync();

    void debug(const char* tag, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
    void info(const char* tag, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
    void warn(const char* tag, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
    void error(const char* tag, const char* fmt, ...) __attribute__((format(printf, 3, 4)));

    std::vector<Log> getAllLogs();
    int getLogCount(LogLevel level);
    std::vector<Log> getLogs(LogLevel level);
    std::vector<Log> getLastLogs(size_t count);

    LoggerConfig currentConfig() const;
    void setLogLevel(LogLevel level);
    LogLevel logLevel() const;

  private:
    void logInternal(LogLevel level, const char* tag, const char* fmt, va_list args);
    std::string formatMessage(const char* fmt, va_list args);
    void performSync();
    static void syncTaskThunk(void* arg);
    void syncTaskLoop();

    LoggerConfig _config{};
    bool _initialized = false;
    bool _running = false;
    TaskHandle_t _syncTask = nullptr;
    SemaphoreHandle_t _mutex = nullptr;
    std::deque<Log> _logs;
    SyncCallback _syncCallback;
    LogLevel _logLevel = LogLevel::Debug;
};
