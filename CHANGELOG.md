# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

- No changes yet.

## [1.0.2] - 2025-09-22
### Added
- `Logger::init` now accepts no arguments, defaulting to the baked-in configuration; docs, examples, and tests highlight the zero-config path.

### Fixed
- Ensured the background sync task keeps running by marking `_running` before we create the task and resetting it if creation fails.

### Maintenance
- Added generated build directories to `.gitignore` and cleaned up stray CMake artefacts that landed in the repository.

## [1.0.1] - 2025-09-17
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
