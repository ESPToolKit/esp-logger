#pragma once

#include <functional>
#include <memory>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct WorkerConfig {
    size_t stackSizeBytes = 4096;
    UBaseType_t priority = 1;
    BaseType_t coreId = tskNO_AFFINITY;
    std::string name{};
    bool useExternalStack = false;
};

class WorkerHandler {
   public:
    bool wait(TickType_t = portMAX_DELAY) { return true; }
    bool destroy() { return true; }
};

struct WorkerResult {
    int error = 0;
    std::shared_ptr<WorkerHandler> handler{};
    const char *message = nullptr;

    explicit operator bool() const { return error == 0 && static_cast<bool>(handler); }
};

class ESPWorker {
   public:
    struct Config {
        size_t maxWorkers = 1;
        size_t stackSizeBytes = 4096;
        UBaseType_t priority = 1;
        BaseType_t coreId = tskNO_AFFINITY;
        bool enableExternalStacks = true;
    };

    using TaskCallback = std::function<void()>;

    void init(const Config &) {}
    void deinit() {}

    WorkerResult spawn(TaskCallback, const WorkerConfig & = WorkerConfig{}) {
        WorkerResult result{};
        result.handler = std::make_shared<WorkerHandler>();
        return result;
    }

    WorkerResult spawnExt(TaskCallback cb, const WorkerConfig &cfg = WorkerConfig{}) {
        return spawn(std::move(cb), cfg);
    }
};
