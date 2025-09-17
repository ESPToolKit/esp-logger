#include "esp_logger/logger.h"
#include "test_support.h"

#include <deque>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using esp_logger::Log;
using esp_logger::LogLevel;
using esp_logger::Logger;
using esp_logger::LoggerConfig;

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

}  // namespace

int main() {
    try {
        test_init_applies_normalized_config();
        test_stores_logs_up_to_configured_capacity();
        test_sync_callback_receives_buffered_logs();
        test_set_log_level_updates_config();
    } catch (const std::exception &ex) {
        std::cerr << "Test failure: " << ex.what() << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
