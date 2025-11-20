#include "esp_logger/logger.h"
#include "test_support.h"

#include <deque>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

[[noreturn]] void fail(const std::string &message) {
    throw std::runtime_error(message);
}

template <typename T>
void expect_equal(const T &actual, const T &expected, const std::string &message) {
    if (!(actual == expected)) {
        fail(message);
    }
}

void expect_true(bool condition, const std::string &message) {
    if (!condition) {
        fail(message);
    }
}

void test_init_with_default_config() {
    ESPLogger logger;

    if (!logger.init()) {
        fail("ESPLogger failed to initialize with default config");
    }

    expect_true(logger.isInitialized(), "ESPLogger should be initialized with default config");
    expect_equal(logger.logLevel(), LogLevel::Debug, "Default console log level should be Debug");

    const auto current = logger.currentConfig();
    expect_true(current.enableSyncTask, "Default config should enable the sync task");
    expect_equal(current.maxLogInRam, static_cast<size_t>(100), "Default maxLogInRam should be 100");

    logger.deinit();
    expect_true(!logger.isInitialized(), "ESPLogger should be deinitialized after default init test");
}

void test_init_applies_normalized_config() {
    ESPLogger logger;
    LoggerConfig config;
    config.enableSyncTask = false;
    config.maxLogInRam = 0;
    config.consoleLogLevel = LogLevel::Warn;

    if (!logger.init(config)) {
        fail("ESPLogger failed to initialize");
    }

    expect_true(logger.isInitialized(), "ESPLogger should be initialized");
    expect_equal(logger.logLevel(), LogLevel::Warn, "Log level should match config");

    const auto current = logger.currentConfig();
    expect_equal(current.maxLogInRam, static_cast<size_t>(1), "maxLogInRam should normalize to 1");
    expect_equal(current.consoleLogLevel, LogLevel::Warn, "consoleLogLevel should remain Warn");

    logger.deinit();
    expect_true(!logger.isInitialized(), "ESPLogger should be deinitialized");
}

void test_stores_logs_up_to_configured_capacity() {
    test_support::resetMillis();

    ESPLogger logger;
    LoggerConfig config;
    config.enableSyncTask = false;
    config.maxLogInRam = 2;
    config.consoleLogLevel = LogLevel::Debug;

    if (!logger.init(config)) {
        fail("ESPLogger failed to initialize");
    }

    logger.debug("TEST", "first %d", 1);
    logger.info("TEST", "second");
    logger.warn("TEST", "third");

    const auto logs = logger.getAllLogs();
    expect_equal(logs.size(), static_cast<size_t>(2), "Should keep only the most recent logs");
    expect_equal(logs.front().level, LogLevel::Info, "First log level incorrect");
    expect_equal(logs.front().message, std::string("second"), "First log message incorrect");
    expect_equal(logs.back().level, LogLevel::Warn, "Last log level incorrect");
    expect_equal(logs.back().message, std::string("third"), "Last log message incorrect");

    const auto last = logger.getLastLogs(1);
    expect_equal(last.size(), static_cast<size_t>(1), "getLastLogs should honor requested count");
    expect_equal(last.front().message, std::string("third"), "getLastLogs should return most recent message");

    logger.deinit();
}

void test_sync_callback_receives_buffered_logs() {
    test_support::resetMillis();

    ESPLogger logger;
    LoggerConfig config;
    config.enableSyncTask = false;
    config.maxLogInRam = 10;
    config.consoleLogLevel = LogLevel::Debug;

    if (!logger.init(config)) {
        fail("ESPLogger failed to initialize");
    }

    std::vector<Log> received;
    logger.onSync([&received](const std::vector<Log> &logs) { received = logs; });

    logger.info("TAG", "message %d", 1);
    logger.error("TAG", "message %d", 2);

    expect_true(received.empty(), "Callback should not run until sync is triggered");

    logger.sync();

    expect_equal(received.size(), static_cast<size_t>(2), "Sync should flush buffered logs");
    expect_true(logger.getAllLogs().empty(), "Logs should be cleared after sync");
    expect_equal(received.front().level, LogLevel::Info, "First flushed level incorrect");
    expect_equal(received.front().message, std::string("message 1"), "First flushed message incorrect");
    expect_equal(received.back().level, LogLevel::Error, "Second flushed level incorrect");
    expect_equal(received.back().message, std::string("message 2"), "Second flushed message incorrect");

    logger.deinit();
}

void test_set_log_level_updates_config() {
    ESPLogger logger;
    LoggerConfig config;
    config.enableSyncTask = false;
    config.maxLogInRam = 5;
    config.consoleLogLevel = LogLevel::Debug;

    if (!logger.init(config)) {
        fail("ESPLogger failed to initialize");
    }

    logger.setLogLevel(LogLevel::Error);

    expect_equal(logger.logLevel(), LogLevel::Error, "Log level should update");
    expect_equal(logger.currentConfig().consoleLogLevel, LogLevel::Error, "Config should reflect new console log level");

    logger.deinit();
}

void test_multiple_logger_instances_operate_independently() {
    test_support::resetMillis();

    ESPLogger first;
    ESPLogger second;

    LoggerConfig configA;
    configA.enableSyncTask = false;
    configA.maxLogInRam = 3;
    configA.consoleLogLevel = LogLevel::Debug;

    LoggerConfig configB = configA;
    configB.maxLogInRam = 5;

    if (!first.init(configA) || !second.init(configB)) {
        fail("Failed to initialize independent loggers");
    }

    first.info("FIRST", "one");
    second.warn("SECOND", "alpha");
    first.error("FIRST", "two");
    second.debug("SECOND", "beta");

    const auto firstLogs = first.getAllLogs();
    const auto secondLogs = second.getAllLogs();

    expect_equal(firstLogs.size(), static_cast<size_t>(2), "First logger should keep its own entries");
    expect_equal(firstLogs.front().tag, std::string("FIRST"), "First logger tag mismatch");
    expect_equal(secondLogs.size(), static_cast<size_t>(2), "Second logger should keep its own entries");
    expect_equal(secondLogs.front().tag, std::string("SECOND"), "Second logger tag mismatch");

    first.deinit();
    second.deinit();
}

void test_get_logs_by_level() {
    test_support::resetMillis();

    ESPLogger logger;
    LoggerConfig config;
    config.enableSyncTask = false;
    config.maxLogInRam = 5;
    config.consoleLogLevel = LogLevel::Debug;

    if (!logger.init(config)) {
        fail("Failed to initialize logger");
    }

    logger.debug("TAG", "message a");
    logger.info("TAG", "message b");
    logger.warn("TAG", "message c");
    logger.info("TAG", "message d");

    expect_equal(logger.getLogCount(LogLevel::Info), 2, "getLogCount should count matching levels only");
    expect_equal(logger.getLogCount(LogLevel::Error), 0, "getLogCount should return zero when no logs match");

    const auto infoLogs = logger.getLogs(LogLevel::Info);
    expect_equal(infoLogs.size(), static_cast<size_t>(2), "getLogs should filter by level");
    expect_equal(infoLogs.front().message, std::string("message b"), "First filtered log mismatch");
    expect_equal(infoLogs.back().message, std::string("message d"), "Second filtered log mismatch");

    expect_equal(logger.getAllLogs().size(), static_cast<size_t>(4), "getLogs should not mutate stored entries");

    logger.deinit();
}

void test_static_helpers_on_snapshot() {
    std::vector<Log> snapshot = {
        {LogLevel::Debug, "TAG", 1, 1, "a"},
        {LogLevel::Info, "TAG", 2, 2, "b"},
        {LogLevel::Warn, "TAG", 3, 3, "c"},
        {LogLevel::Info, "TAG", 4, 4, "d"},
    };

    expect_equal(ESPLogger::getLogCount(snapshot, LogLevel::Info), 2, "Static getLogCount should work on snapshots");
    expect_equal(ESPLogger::getLogCount(snapshot, LogLevel::Error), 0, "Static getLogCount should return zero if no match");

    const auto warnLogs = ESPLogger::getLogs(snapshot, LogLevel::Warn);
    expect_equal(warnLogs.size(), static_cast<size_t>(1), "Static getLogs should filter snapshots");
    expect_equal(warnLogs.front().message, std::string("c"), "Static getLogs should preserve message order");
}

}  // namespace

int main() {
    try {
        test_init_with_default_config();
        test_init_applies_normalized_config();
        test_stores_logs_up_to_configured_capacity();
        test_sync_callback_receives_buffered_logs();
        test_set_log_level_updates_config();
        test_multiple_logger_instances_operate_independently();
        test_get_logs_by_level();
        test_static_helpers_on_snapshot();
    } catch (const std::exception &ex) {
        std::cerr << "Test failure: " << ex.what() << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
