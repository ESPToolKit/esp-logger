# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]
### Added
- `ESPLogger::init` now accepts no arguments, defaulting to the baked-in configuration; docs, examples, and tests highlight the zero-config path.
- Added lightweight live log callbacks via `attach`/`detach`, allowing per-entry streaming while keeping existing `onSync` batch flushing.
- Added `LoggerConfig::usePSRAMBuffers` and integrated `ESPBufferManager` for logger-owned dynamic buffers with safe fallback to normal heap when PSRAM is unavailable.
- Switched sync worker lifecycle to native FreeRTOS task handling (`xTaskCreatePinnedToCore`/`vTaskDelete`).

### Fixed
- Ensured the background sync task keeps running by marking `_running` before we create the task and resetting it if creation fails.

### Changed
- Clarified that the library no longer ships a global `logger` instance; documentation and examples now create their own `ESPLogger` objects so multiple instances can coexist safely.
- Default console output now uses a lightweight `printf` backend; set `ESPLOGGER_USE_ESP_LOG=1` to opt back into ESP-IDF logging macros when you prefer their formatting.

### Maintenance
- Added generated build directories to `.gitignore` and cleaned up stray CMake artefacts that landed in the repository.

## [1.0.1] - 2025-09-25
### Changed
- Removed the `esp_logger` namespace wrapper so the types and inline `logger` instance are available without `using` declarations.
- Updated the README and examples to demonstrate the namespace-free API.

## [1.0.0] - 2025-09-17
### Added
- Configurable logger with `debug`, `info`, `warn`, and `error` helpers.
- In-memory batching with configurable size that drops the oldest entries when full, manual `sync()`, and an optional periodic sync task.
- Sync callback API to integrate custom persistence or reporting pipelines.
- Console output bridged to ESP-IDF `ESP_LOGx`, gated by a configurable global log level.
- Retrieval helpers for all buffered logs or the most recent entries on demand.
- Examples showing basic setup and custom sync handling.
