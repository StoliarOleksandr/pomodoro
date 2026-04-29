#pragma once
#include <stddef.h>
#include <stdint.h>

/* Opaque OS-object handles. Each implementation typedef's these to whatever
 * internal type it needs (e.g. QueueHandle_t, SemaphoreHandle_t). */
typedef void *osal_task_t;
typedef void *osal_mutex_t;
typedef void *osal_sem_t;
typedef void *osal_queue_t;

/* ---- Scheduler ---------------------------------------------------------- */

/* Start the OS scheduler. Never returns on a real RTOS. */
void osal_start(void);

/* ---- Task --------------------------------------------------------------- */

/* Create a task. stack_words is in 32-bit words, not bytes.
 * Priority: higher number = higher priority. */
void osal_task_create(void (*fn)(void *arg),
                      void       *arg,
                      uint16_t    stack_words,
                      uint8_t     priority,
                      const char *name,
                      osal_task_t *out);

/* Yield the remainder of the current time-slice to the scheduler. */
void osal_yield(void);

/* ---- Time --------------------------------------------------------------- */

/* Block the calling task for at least ms milliseconds.
 * Other tasks continue to run while this task sleeps.
 * Do NOT call before osal_start() — use hal_delay_ms() for pre-RTOS delays. */
void osal_delay_ms(uint32_t ms);

/* Milliseconds elapsed since boot (driven by the OS tick). */
uint32_t osal_tick_ms(void);

/* ---- Mutex -------------------------------------------------------------- */

/* Non-recursive mutex. Lock order must be consistent to avoid deadlocks. */
void osal_mutex_create(osal_mutex_t *m);
void osal_mutex_lock(osal_mutex_t *m);
void osal_mutex_unlock(osal_mutex_t *m);

/* ---- Binary semaphore --------------------------------------------------- */

/* Semaphore starts empty (count = 0). */
void osal_sem_create(osal_sem_t *s);

/* Give from task context. */
void osal_sem_give(osal_sem_t *s);

/* Give from ISR context. */
void osal_sem_give_from_isr(osal_sem_t *s);

/* Block until the semaphore is given, or until timeout_ms elapses.
 * Returns 1 if taken, 0 on timeout. Pass OSAL_WAIT_FOREVER to block indefinitely. */
#define OSAL_WAIT_FOREVER UINT32_MAX
int osal_sem_take(osal_sem_t *s, uint32_t timeout_ms);

/* ---- Queue -------------------------------------------------------------- */

/* Fixed-size FIFO of items each item_size bytes, depth items deep. */
void osal_queue_create(osal_queue_t *q, size_t item_size, size_t depth);

/* Send from task. Blocks if full. */
void osal_queue_send(osal_queue_t *q, const void *item);

/* Send from ISR. Drops item if full. */
void osal_queue_send_from_isr(osal_queue_t *q, const void *item);

/* Receive from task. Returns 1 on success, 0 on timeout. */
int osal_queue_receive(osal_queue_t *q, void *item, uint32_t timeout_ms);
