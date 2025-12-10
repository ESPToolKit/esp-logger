# ESPLogger

A lightweight, configurable logging utility for ESP32 projects. ESPLogger combines formatted log APIs, in-RAM batching, and optional background syncing so you can integrate storage or telemetry pipelines without blocking application code.

## CI / Release / License
[![CI](https://github.com/ESPToolKit/esp-logger/actions/workflows/ci.yml/badge.svg)](https://github.com/ESPToolKit/esp-logger/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/ESPToolKit/esp-logger?sort=semver)](https://github.com/ESPToolKit/esp-logger/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE.md)

## Features
- Familiar `debug`/`info`/`warn`/`error` helpers with `printf` formatting semantics.
- Configurable behavior: batching thresholds, FreeRTOS core/stack/priority, and console log level.
- Optional background sync task plus manual `sync()` for deterministic flushes.
- `onSync` callback hands over a vector of structured `Log` entries for custom persistence.
- Helpers to fetch every buffered log or just the most recent entries whenever you need diagnostics.
- Filter helpers to count or retrieve buffered logs at a specific level without flushing them.
- Static helpers to count or filter the snapshot passed into `onSync` without relying on internal buffers.
- Console output defaults to a minimal `printf` backend; opt into ESP-IDF `ESP_LOGx` macros with a single build flag when you want IDF-style logs.

## Examples
Minimal setup:

```cpp
#include <ESPLogger.h>

static ESPLogger logger;

void setup() {
    Serial.begin(115200);

    LoggerConfig cfg;
    cfg.syncIntervalMS = 5000;
    cfg.maxLogInRam = 100;
    cfg.consoleLogLevel = LogLevel::Info;
    logger.init(cfg);

    logger.onSync([](const std::vector<Log>& logs) {
        for (const auto& entry : logs) {
            // Persist logs to flash, send over Wi-Fi, etc.
        }
    });

    logger.info("INIT", "ESPLogger initialised with %u entries", cfg.maxLogInRam);
}

void loop() {
    logger.debug("LOOP", "Loop iteration at %lu ms", static_cast<unsigned long>(millis()));
    delay(1000);
}
```

Scope a logger inside your own class:

```cpp
class SensorModule {
  public:
    explicit SensorModule(ESPLogger& logger) : _logger(logger) {}

    void update() {
        const float reading = analogRead(A0) / 1023.0f;
        _logger.info("SENSOR", "Latest reading: %.2f", reading);
    }

  private:
    ESPLogger& _logger;
};
```

Example sketches:
- `examples/basic_usage` – minimal configuration + periodic logging.
- `examples/custom_sync` – manual syncing with a custom persistence callback.

## Console backend
ESPLogger prints with a minimal `printf` backend by default, producing lines like:

```
[I] [NETWORK] ~ Connected to Wi-Fi
```

Prefer the ESP-IDF logging macros? Define `ESPLOGGER_USE_ESP_LOG=1` in your build flags to switch the console bridge:
- PlatformIO: `build_flags = -DESPLOGGER_USE_ESP_LOG=1`
- Arduino CLI: `--build-property build.extra_flags=-DESPLOGGER_USE_ESP_LOG=1`

## Gotchas
- Keep `ESPLogger` instances alive for as long as their sync task may run; destroying the object stops the task.
- When `enableSyncTask` is `false`, remember to call `logger.sync()` yourself or logs will stay buffered forever.
- `setLogLevel` only affects console output; all logs remain available inside the RAM buffer until purged.
- Inside `onSync`, the internal buffer has already been cleared—use the static helper overloads that take the `logs` snapshot to count or filter entries.
- The `onSync` callback runs inside the sync task context—avoid blocking operations.

## API Reference
- `void init(const LoggerConfig& cfg = {})` – configure sync cadence, stack size, priorities, and thresholds.
- `void debug/info/warn/error(const char* tag, const char* fmt, ...)` – emit formatted logs.
- `void setLogLevel(LogLevel level)` / `LogLevel logLevel() const` – adjust console verbosity at runtime.
- `void onSync(ESPLogger::SyncCallback cb)` – receive batches of `Log` entries whenever the buffer flushes.
- `void sync()` – force a flush (useful when the background task is disabled).
- `std::vector<Log> getAllLogs()` / `std::vector<Log> getLastLogs(size_t count)` – snapshot buffered entries.
- `int getLogCount(LogLevel level)` / `std::vector<Log> getLogs(LogLevel level)` – inspect buffered logs at a particular level.
- `static int getLogCount(const std::vector<Log>& logs, LogLevel level)` / `static std::vector<Log> getLogs(const std::vector<Log>& logs, LogLevel level)` – filter the snapshot passed to `onSync`.
- `LoggerConfig currentConfig() const` – inspect the live settings.

`LoggerConfig` knobs:

| Field | Default | Description |
| --- | --- | --- |
| `syncIntervalMS` | `5000` | Period between automatic flushes (ignored when `enableSyncTask` is `false`). |
| `stackSize` | `16384` | Stack size for the sync task. |
| `coreId` | `LoggerConfig::any` | CPU core affinity for the sync task. |
| `maxLogInRam` | `100` | Maximum entries retained in RAM; oldest entries are discarded when the buffer is full. |
| `priority` | `1` | FreeRTOS priority for the sync task. |
| `consoleLogLevel` | `LogLevel::Debug` | Minimum level printed to the console. |
| `enableSyncTask` | `true` | Disable to opt out of the background task and call `logger.sync()` manually. |

Stack sizes are expressed in bytes.

## Restrictions
- Built for ESP32 + FreeRTOS (Arduino or ESP-IDF) with C++17 enabled.
- Uses dynamic allocation for the RAM buffer; size `maxLogInRam` according to your heap budget.
- Console output uses `printf` by default; define `ESPLOGGER_USE_ESP_LOG=1` if you want ESP-IDF log colors/levels managed via menuconfig.

## Tests
A host-driven test suite lives under `test/` and is wired into CTest. Run it with:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

The suite exercises buffering, log level filtering, and sync behavior. Hardware smoke tests reside in `examples/`.

## License
MIT — see [LICENSE.md](LICENSE.md).

## ESPToolKit
- Check out other libraries: <https://github.com/orgs/ESPToolKit/repositories>
- Hang out on Discord: <https://discord.gg/WG8sSqAy>
- Support the project: <https://ko-fi.com/esptoolkit>
- Visit the website: <https://www.esptoolkit.hu/>
