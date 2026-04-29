#include "osal/osal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

/* ---- Scheduler ---------------------------------------------------------- */

void osal_start(void)
{
    vTaskStartScheduler();
}

/* ---- Task --------------------------------------------------------------- */

void osal_task_create(void (*fn)(void *arg),
                      void       *arg,
                      uint16_t    stack_words,
                      uint8_t     priority,
                      const char *name,
                      osal_task_t *out)
{
    TaskHandle_t h = NULL;
    xTaskCreate(fn, name, stack_words, arg, (UBaseType_t)priority, &h);
    if (out)
        *out = h;
}

void osal_yield(void)
{
    taskYIELD();
}

/* ---- Time --------------------------------------------------------------- */

void osal_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

uint32_t osal_tick_ms(void)
{
    return (uint32_t)xTaskGetTickCount();
}

/* ---- Mutex -------------------------------------------------------------- */

void osal_mutex_create(osal_mutex_t *m)
{
    *m = xSemaphoreCreateMutex();
}

void osal_mutex_lock(osal_mutex_t *m)
{
    xSemaphoreTake((SemaphoreHandle_t)*m, portMAX_DELAY);
}

void osal_mutex_unlock(osal_mutex_t *m)
{
    xSemaphoreGive((SemaphoreHandle_t)*m);
}

/* ---- Binary semaphore --------------------------------------------------- */

void osal_sem_create(osal_sem_t *s)
{
    *s = xSemaphoreCreateBinary();
}

void osal_sem_give(osal_sem_t *s)
{
    xSemaphoreGive((SemaphoreHandle_t)*s);
}

void osal_sem_give_from_isr(osal_sem_t *s)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)*s, &woken);
    portYIELD_FROM_ISR(woken);
}

int osal_sem_take(osal_sem_t *s, uint32_t timeout_ms)
{
    TickType_t ticks = (timeout_ms == OSAL_WAIT_FOREVER)
                           ? portMAX_DELAY
                           : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake((SemaphoreHandle_t)*s, ticks) == pdTRUE ? 1 : 0;
}

/* ---- Queue -------------------------------------------------------------- */

void osal_queue_create(osal_queue_t *q, size_t item_size, size_t depth)
{
    *q = xQueueCreate((UBaseType_t)depth, (UBaseType_t)item_size);
}

void osal_queue_send(osal_queue_t *q, const void *item)
{
    xQueueSend((QueueHandle_t)*q, item, portMAX_DELAY);
}

void osal_queue_send_from_isr(osal_queue_t *q, const void *item)
{
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR((QueueHandle_t)*q, item, &woken);
    portYIELD_FROM_ISR(woken);
}

int osal_queue_receive(osal_queue_t *q, void *item, uint32_t timeout_ms)
{
    TickType_t ticks = (timeout_ms == OSAL_WAIT_FOREVER)
                           ? portMAX_DELAY
                           : pdMS_TO_TICKS(timeout_ms);
    return xQueueReceive((QueueHandle_t)*q, item, ticks) == pdTRUE ? 1 : 0;
}
