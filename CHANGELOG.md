# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]
### Added
- Configurable logger with `debug`, `info`, `warn`, and `error` helpers.
- In-memory batching with configurable size, manual `sync()`, and optional periodic sync task.
- Sync callback API to integrate custom persistence or reporting pipelines.
- Console output bridged to ESP-IDF `ESP_LOGx`, gated by a configurable global log level.
- Retrieval helpers for all buffered logs or the most recent entries on demand.
- Examples showing basic setup and custom sync handling.

### Changed
- The in-memory queue now retains the most recent entries by dropping the oldest when full, avoiding silent syncs with empty payloads.
