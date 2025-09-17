#include <Arduino.h>
#include <ESPLogger.h>

// Optional: store a snapshot of logs the last time we synced.
static std::vector<Log> lastSyncedLogs;

void logSyncCallback(const std::vector<Log>& logs) {
    lastSyncedLogs = logs;
    Serial.printf("Synced %u log entries\n", static_cast<unsigned>(logs.size()));
}

void setup() {
    Serial.begin(115200);

    LoggerConfig config;
    config.syncIntervalMS = 3000;      // sync every 3 seconds
    config.maxLogInRam = 25;           // keep a small buffer in RAM
    config.consoleLogLevel = LogLevel::Info;

    if (!logger.init(config)) {
        Serial.println("Failed to initialise logger!");
        return;
    }

    logger.onSync(logSyncCallback);

    logger.info("INIT", "Logger ready. Max buffer: %u", static_cast<unsigned>(config.maxLogInRam));
}

void loop() {
    static uint32_t counter = 0;

    logger.debug("LOOP", "This debug message only shows when consoleLogLevel <= Debug (%lu)",
                 static_cast<unsigned long>(counter));

    logger.info("APP", "Heartbeat %lu", static_cast<unsigned long>(counter));

    if (counter % 5 == 0) {
        logger.warn("APP", "Simulated warning at count %lu", static_cast<unsigned long>(counter));
    }

    if (counter % 9 == 0) {
        logger.error("APP", "Simulated error at count %lu", static_cast<unsigned long>(counter));
    }

    counter++;
    delay(1000);
}
