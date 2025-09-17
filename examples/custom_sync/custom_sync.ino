#include <Arduino.h>
#include <ESPLogger.h>

using namespace esp_logger;

namespace {
constexpr uint32_t kManualSyncIntervalMS = 5000;
uint32_t lastSyncMs = 0;
}

void persistLogs(const std::vector<Log>& logs) {
    Serial.printf("Persisting %u buffered entries\n", static_cast<unsigned>(logs.size()));
    for (const auto& entry : logs) {
        Serial.printf("  [%u][%ld][%s] %s\n",
                      static_cast<unsigned>(entry.millis),
                      static_cast<long>(entry.timestamp),
                      entry.tag.c_str(),
                      entry.message.c_str());
    }
}

void setup() {
    Serial.begin(115200);

    LoggerConfig config;
    config.enableSyncTask = false;    // we will drive sync manually
    config.maxLogInRam = 50;
    config.consoleLogLevel = LogLevel::Debug;

    if (!logger.init(config)) {
        Serial.println("Logger init failed");
        return;
    }

    logger.onSync(persistLogs);

    logger.info("SYNC", "Manual sync example ready");
}

void loop() {
    logger.debug("DATA", "Sensor reading: %0.2f", analogRead(A0) / 1023.0f);

    if (millis() - lastSyncMs >= kManualSyncIntervalMS) {
        lastSyncMs = millis();
        logger.sync();  // trigger persistence callback immediately
    }

    delay(250);
}
