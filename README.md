# ESP Logger

[![CI](https://github.com/ESPToolKit/esp-logger/actions/workflows/ci.yml/badge.svg)](https://github.com/ESPToolKit/esp-logger/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/ESPToolKit/esp-logger?sort=semver)](https://github.com/ESPToolKit/esp-logger/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE.md)

A lightweight, configurable logging utility for ESP32 projects.
It combines formatted log APIs, in-RAM batching, and optional background syncing
so you can integrate storage or telemetry pipelines without blocking application code.

## Features
- Familiar `debug`/`info`/`warn`/`error` helpers with `printf` formatting semantics.
- Configurable runtime behavior: batching, FreeRTOS core/stack/priority, and global console log level.
- Optional background sync task plus manual `sync()` for deterministic flushes.
- `onSync` callback with a vector of structured `Log` entries for custom persistence.
- Helpers to fetch every buffered log or just the most recent entries at any time.
- Console output automatically routed through the ESP-IDF `ESP_LOGx` macros.

## Getting Started
1. Install the library (drop it into `lib/` for PlatformIO or `libraries/` for the Arduino IDE).
2. Include the umbrella header:

```cpp
#include <ESPLogger.h>
```

3. Initialise the logger (optionally overriding the defaults):

```cpp
#include <ESPLogger.h>

void setup() {
    Serial.begin(115200);

    // Quick start: call logger.init(); with no arguments to use the default config.
    LoggerConfig config;
    config.syncIntervalMS = 5000;   // flush to callback every 5 seconds
    config.maxLogInRam = 100;       // keep 100 entries before forcing a flush
    config.consoleLogLevel = LogLevel::Info;  // suppress debug traces on console

    logger.init(config);

    logger.onSync([](const std::vector<Log>& logs) {
        // Persist logs to flash, send over Wi-Fi, etc.
    });

    logger.info("INIT", "Logger initialised with %u entries", config.maxLogInRam);
}

void loop() {
    logger.debug("LOOP", "Loop iteration at %lu ms", static_cast<unsigned long>(millis()));
    delay(1000);
}
```

## Configuration Reference
`LoggerConfig` exposes the following knobs:

| Field | Default | Description |
| --- | --- | --- |
| `syncIntervalMS` | `5000` | Period between automatic flushes. Ignored when `enableSyncTask` is `false`. |
| `stackSize` | `4096` | Stack size (words) for the sync task. |
| `coreId` | `LoggerConfig::any` | CPU core affinity for the sync task. |
| `maxLogInRam` | `100` | Maximum entries retained in RAM; when the buffer is full the oldest entry is discarded. |
| `taskPriority` | `1` | FreeRTOS priority for the sync task. |
| `consoleLogLevel` | `LogLevel::Debug` | Minimum level printed to the console. Lower levels remain buffered. |
| `enableSyncTask` | `true` | Disable to opt out of the background task and call `logger.sync()` manually. |

### Changing the Log Level at Runtime

```cpp
logger.setLogLevel(LogLevel::Warn);  // only warn/error logs reach the console
```

The new level is stored on the logger and in the active `LoggerConfig` snapshot returned by `currentConfig()`.

### Manual Syncing & Retrieval

```cpp
// Force a flush when using enableSyncTask = false
logger.sync();

// Fetch all buffered entries
std::vector<Log> allLogs = logger.getAllLogs();

// Fetch the last N entries
auto latest = logger.getLastLogs(10);
```

## Examples
Two sketches under the `examples/` folder demonstrate common setups:
- `examples/basic_usage/basic_usage.ino` – Minimal configuration and periodic logging.
- `examples/custom_sync/custom_sync.ino` – Manual syncing with a custom persistence callback.

Upload one of the sketches to your ESP32 board to see the logger in action.

## License
MIT

## ESPToolKit

- Check out other libraries under ESPToolKit: https://github.com/orgs/ESPToolKit/repositories
- Join our discord server at: https://discord.gg/WG8sSqAy
- If you like the libraries, you can support me at: https://ko-fi.com/esptoolkit
