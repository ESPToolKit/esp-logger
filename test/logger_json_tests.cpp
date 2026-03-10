#include "esp_logger/logger.h"
#include "test_support.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#if !ESPLOGGER_HAS_ARDUINOJSON_V7
#error "JSON-enabled test target requires ArduinoJson v7 support"
#endif

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

void test_default_config_enables_pretty_json() {
	LoggerConfig config;
	expect_true(config.usePrettyJson, "Default usePrettyJson should be true");
}

void test_init_preserves_use_pretty_json_false() {
	ESPLogger logger;
	LoggerConfig config;
	config.enableSyncTask = false;
	config.usePrettyJson = false;

	if (!logger.init(config)) {
		fail("Logger failed to initialize");
	}

	expect_true(!logger.currentConfig().usePrettyJson, "init should preserve usePrettyJson=false");
	logger.deinit();
}

void test_json_document_logs_compact_when_disabled() {
	test_support::resetMillis();

	ESPLogger logger;
	LoggerConfig config;
	config.enableSyncTask = false;
	config.usePrettyJson = false;

	if (!logger.init(config)) {
		fail("Logger failed to initialize");
	}

	ArduinoJson::JsonDocument doc;
	doc["hello"] = "world";
	doc["answer"] = 7;
	logger.debug("JSON", doc);

	const auto logs = logger.getAllLogs();
	expect_equal(logs.size(), static_cast<size_t>(1), "Compact JSON log should be buffered");
	expect_equal(
	    logs.front().message,
	    std::string("{\"hello\":\"world\",\"answer\":7}"),
	    "Compact JSON serialization mismatch"
	);

	logger.deinit();
}

void test_json_document_logs_pretty_by_default() {
	test_support::resetMillis();

	ESPLogger logger;
	LoggerConfig config;
	config.enableSyncTask = false;

	if (!logger.init(config)) {
		fail("Logger failed to initialize");
	}

	ArduinoJson::JsonDocument doc;
	doc["hello"] = "world";
	logger.info("JSON", doc);

	const auto logs = logger.getAllLogs();
	expect_equal(logs.size(), static_cast<size_t>(1), "Pretty JSON log should be buffered");
	expect_equal(
	    logs.front().message,
	    std::string("{\n  \"hello\": \"world\"\n}"),
	    "Pretty JSON serialization mismatch"
	);

	logger.deinit();
}

void test_json_variant_logs_nested_object() {
	test_support::resetMillis();

	ESPLogger logger;
	LoggerConfig config;
	config.enableSyncTask = false;

	if (!logger.init(config)) {
		fail("Logger failed to initialize");
	}

	ArduinoJson::JsonDocument nested;
	nested["child"] = "value";
	logger.warn("JSON", nested.as<ArduinoJson::JsonVariantConst>());

	const auto logs = logger.getAllLogs();
	expect_equal(logs.size(), static_cast<size_t>(1), "Variant object log should be buffered");
	expect_equal(
	    logs.front().message,
	    std::string("{\n  \"child\": \"value\"\n}"),
	    "Variant object serialization mismatch"
	);

	logger.deinit();
}

void test_json_variant_logs_scalar() {
	test_support::resetMillis();

	ESPLogger logger;
	LoggerConfig config;
	config.enableSyncTask = false;
	config.usePrettyJson = false;

	if (!logger.init(config)) {
		fail("Logger failed to initialize");
	}

	logger.error("JSON", ArduinoJson::JsonVariantConst(7));

	const auto logs = logger.getAllLogs();
	expect_equal(logs.size(), static_cast<size_t>(1), "Scalar variant log should be buffered");
	expect_equal(logs.front().message, std::string("7"), "Scalar variant serialization mismatch");

	logger.deinit();
}

void test_live_callback_receives_json_message() {
	test_support::resetMillis();

	ESPLogger logger;
	LoggerConfig config;
	config.enableSyncTask = false;
	config.usePrettyJson = false;

	if (!logger.init(config)) {
		fail("Logger failed to initialize");
	}

	std::vector<Log> liveLogs;
	logger.attach([&liveLogs](const Log &entry) { liveLogs.push_back(entry); });

	ArduinoJson::JsonDocument doc;
	doc["hello"] = "world";
	logger.info("LIVE", doc);

	expect_equal(liveLogs.size(), static_cast<size_t>(1), "Live callback should receive JSON log");
	expect_equal(
	    liveLogs.front().message,
	    std::string("{\"hello\":\"world\"}"),
	    "Live callback JSON payload mismatch"
	);

	logger.deinit();
}

void test_sync_callback_receives_json_message() {
	test_support::resetMillis();

	ESPLogger logger;
	LoggerConfig config;
	config.enableSyncTask = false;
	config.usePrettyJson = false;

	if (!logger.init(config)) {
		fail("Logger failed to initialize");
	}

	std::vector<Log> syncedLogs;
	logger.onSync([&syncedLogs](const std::vector<Log> &logs) { syncedLogs = logs; });

	ArduinoJson::JsonDocument doc;
	doc["hello"] = "world";
	logger.warn("SYNC", doc);
	logger.sync();

	expect_equal(
	    syncedLogs.size(),
	    static_cast<size_t>(1),
	    "Sync callback should receive JSON log"
	);
	expect_equal(
	    syncedLogs.front().message,
	    std::string("{\"hello\":\"world\"}"),
	    "Sync callback JSON payload mismatch"
	);

	logger.deinit();
}

void test_printf_logging_still_works_in_json_build() {
	test_support::resetMillis();

	ESPLogger logger;
	LoggerConfig config;
	config.enableSyncTask = false;

	if (!logger.init(config)) {
		fail("Logger failed to initialize");
	}

	logger.info("TEXT", "value %d", 5);

	const auto logs = logger.getAllLogs();
	expect_equal(logs.size(), static_cast<size_t>(1), "Formatted log should still be buffered");
	expect_equal(
	    logs.front().message,
	    std::string("value 5"),
	    "Formatted logging should remain unchanged"
	);

	logger.deinit();
}

} // namespace

int main() {
	try {
		test_default_config_enables_pretty_json();
		test_init_preserves_use_pretty_json_false();
		test_json_document_logs_compact_when_disabled();
		test_json_document_logs_pretty_by_default();
		test_json_variant_logs_nested_object();
		test_json_variant_logs_scalar();
		test_live_callback_receives_json_message();
		test_sync_callback_receives_json_message();
		test_printf_logging_still_works_in_json_build();
	} catch (const std::exception &ex) {
		std::cerr << "Test failure: " << ex.what() << '\n';
		return 1;
	}

	std::cout << "All JSON tests passed\n";
	return 0;
}
