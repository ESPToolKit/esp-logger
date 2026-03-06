#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPLogger.h>

ESPLogger logger;

namespace {
uint32_t sampleIndex = 0;
bool loggerStopped = false;
}  // namespace

void setup() {
    Serial.begin(115200);

    LoggerConfig config;
    config.enableSyncTask = false;
    config.consoleLogLevel = LogLevel::Debug;
    config.usePrettyJson = true;

    if (!logger.init(config)) {
        Serial.println("ESPLogger init failed");
        return;
    }

    logger.info("JSON", "ArduinoJson v7 logging example ready");
}

void loop() {
    if (loggerStopped) {
        delay(1000);
        return;
    }

    JsonDocument payload;
    payload["sample"] = static_cast<int>(sampleIndex);
    payload["status"] = sampleIndex % 2 == 0 ? "ok" : "warn";
    payload["ready"] = true;

    logger.debug("JSON", payload);
    logger.info("JSON", payload.as<JsonVariantConst>());

    if (sampleIndex == 2) {
        logger.warn("JSON", "Switching to compact JSON output");
        LoggerConfig updated = logger.currentConfig();
        updated.usePrettyJson = false;
        logger.deinit();
        if (!logger.init(updated)) {
            Serial.println("ESPLogger re-init failed");
            loggerStopped = true;
            return;
        }
        logger.info("JSON", "Compact JSON output enabled");
    }

    if (sampleIndex == 4) {
        logger.error("JSON", payload["sample"]);
    }

    sampleIndex++;

    if (sampleIndex >= 6) {
        logger.deinit();
        loggerStopped = true;
        Serial.println("Logger deinitialized after JSON demo");
    }

    delay(1500);
}
