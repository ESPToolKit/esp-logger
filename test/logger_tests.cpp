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
    Logger logger;

    if (!logger.init()) {
        fail("Logger failed to initialize with default config");
    }

    expect_true(logger.isInitialized(), "Logger should be initialized with default config");
    expect_equal(logger.logLevel(), LogLevel::Debug, "Default console log level should be Debug");

    const auto current = logger.currentConfig();
    expect_true(current.enableSyncTask, "Default config should enable the sync task");
    expect_equal(current.maxLogInRam, static_cast<size_t>(100), "Default maxLogInRam should be 100");

    logger.deinit();
    expect_true(!logger.isInitialized(), "Logger should be deinitialized after default init test");
}

void test_init_applies_normalized_config() {
    Logger logger;
    LoggerConfig config;
    config.enableSyncTask = false;
    config.maxLogInRam = 0;
    config.consoleLogLevel = LogLevel::Warn;

    if (!logger.init(config)) {
        fail("Logger failed to initialize");
    }

    expect_true(logger.isInitialized(), "Logger should be initialized");
    expect_equal(logger.logLevel(), LogLevel::Warn, "Log level should match config");

    const auto current = logger.currentConfig();
    expect_equal(current.maxLogInRam, static_cast<size_t>(1), "maxLogInRam should normalize to 1");
    expect_equal(current.consoleLogLevel, LogLevel::Warn, "consoleLogLevel should remain Warn");

    logger.deinit();
    expect_true(!logger.isInitialized(), "Logger should be deinitialized");
}

void test_stores_logs_up_to_configured_capacity() {
    test_support::resetMillis();

    Logger logger;
    LoggerConfig config;
    config.enableSyncTask = false;
    config.maxLogInRam = 2;
    config.consoleLogLevel = LogLevel::Debug;

    if (!logger.init(config)) {
        fail("Logger failed to initialize");
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

    Logger logger;
    LoggerConfig config;
    config.enableSyncTask = false;
    config.maxLogInRam = 10;
    config.consoleLogLevel = LogLevel::Debug;

    if (!logger.init(config)) {
        fail("Logger failed to initialize");
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
    Logger logger;
    LoggerConfig config;
    config.enableSyncTask = false;
    config.maxLogInRam = 5;
    config.consoleLogLevel = LogLevel::Debug;

    if (!logger.init(config)) {
        fail("Logger failed to initialize");
    }

    logger.setLogLevel(LogLevel::Error);

    expect_equal(logger.logLevel(), LogLevel::Error, "Log level should update");
    expect_equal(logger.currentConfig().consoleLogLevel, LogLevel::Error, "Config should reflect new console log level");

    logger.deinit();
}

void test_multiple_logger_instances_operate_independently() {
    test_support::resetMillis();

    Logger first;
    Logger second;

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

}  // namespace

int main() {
    try {
        test_init_with_default_config();
        test_init_applies_normalized_config();
        test_stores_logs_up_to_configured_capacity();
        test_sync_callback_receives_buffered_logs();
        test_set_log_level_updates_config();
        test_multiple_logger_instances_operate_independently();
    } catch (const std::exception &ex) {
        std::cerr << "Test failure: " << ex.what() << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
