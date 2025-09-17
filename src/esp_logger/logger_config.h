#pragma once

#include <cstddef>
#include <cstdint>

#include <freertos/FreeRTOS.h>

namespace esp_logger {

enum class LogLevel {
    Debug = 0,
    Info,
    Warn,
    Error
};

struct LoggerConfig {
    static constexpr BaseType_t any = tskNO_AFFINITY;  // Use any available core

    uint32_t syncIntervalMS = 5000;
    uint32_t stackSize = 4096;
    BaseType_t coreId = any;
    size_t maxLogInRam = 100;
    UBaseType_t taskPriority = 1;
    LogLevel consoleLogLevel = LogLevel::Debug;
    bool enableSyncTask = true;
};

}  // namespace esp_logger
