#include "Arduino.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "test_support.h"

#include <atomic>
#include <mutex>
#include <new>

namespace {
struct FakeSemaphore {
    std::mutex mutex;
};

std::atomic<unsigned long> g_fakeMillis{0};
std::atomic<TickType_t> g_fakeTicks{0};

}  // namespace

extern "C" unsigned long millis(void) {
    return g_fakeMillis.fetch_add(1) + 1;
}

extern "C" SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    return reinterpret_cast<SemaphoreHandle_t>(new (std::nothrow) FakeSemaphore{});
}

extern "C" BaseType_t xSemaphoreTake(SemaphoreHandle_t handle, TickType_t /*ticks*/) {
    if (handle == nullptr) {
        return pdFAIL;
    }
    auto *sem = reinterpret_cast<FakeSemaphore *>(handle);
    sem->mutex.lock();
    return pdPASS;
}

extern "C" BaseType_t xSemaphoreGive(SemaphoreHandle_t handle) {
    if (handle == nullptr) {
        return pdFAIL;
    }
    auto *sem = reinterpret_cast<FakeSemaphore *>(handle);
    sem->mutex.unlock();
    return pdPASS;
}

extern "C" void vSemaphoreDelete(SemaphoreHandle_t handle) {
    auto *sem = reinterpret_cast<FakeSemaphore *>(handle);
    delete sem;
}

extern "C" BaseType_t xTaskCreatePinnedToCore(TaskFunction_t task,
                                               const char * /*name*/,
                                               uint32_t /*stackDepth*/,
                                               void * /*parameters*/,
                                               UBaseType_t /*priority*/,
                                               TaskHandle_t *createdTask,
                                               BaseType_t /*coreId*/) {
    if (createdTask != nullptr) {
        *createdTask = reinterpret_cast<TaskHandle_t>(task);
    }
    return pdPASS;
}

extern "C" void vTaskDelete(TaskHandle_t /*task*/) {}

extern "C" void vTaskDelay(TickType_t ticks) {
    g_fakeTicks.fetch_add(ticks);
}

extern "C" TickType_t xTaskGetTickCount(void) {
    return g_fakeTicks.load();
}

namespace test_support {

void resetMillis(unsigned long start) {
    g_fakeMillis.store(start);
    g_fakeTicks.store(static_cast<TickType_t>(start));
}

}  // namespace test_support
