#pragma once

#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

SemaphoreHandle_t xSemaphoreCreateMutex(void);
BaseType_t xSemaphoreTake(SemaphoreHandle_t handle, TickType_t ticks);
BaseType_t xSemaphoreGive(SemaphoreHandle_t handle);
void vSemaphoreDelete(SemaphoreHandle_t handle);

#ifdef __cplusplus
}
#endif
