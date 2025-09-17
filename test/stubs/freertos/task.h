#pragma once

#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*TaskFunction_t)(void *);

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t task,
                                   const char *name,
                                   uint32_t stackDepth,
                                   void *parameters,
                                   UBaseType_t priority,
                                   TaskHandle_t *createdTask,
                                   BaseType_t coreId);

void vTaskDelete(TaskHandle_t task);
void vTaskDelay(TickType_t ticks);

#ifdef __cplusplus
}
#endif
