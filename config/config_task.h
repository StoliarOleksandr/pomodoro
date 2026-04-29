#pragma once

#include "osal/osal.h"

#include <stdint.h>

/* Call once before osal_start() to bind the shared objects.
 * in_config: pointer to the routing flag owned by main.c — cleared when done. */
void config_task_init(osal_sem_t     *trigger,
                      osal_queue_t   *btn_q,
                      osal_sem_t     *display_sem,
                      volatile uint8_t *in_config);

/* FreeRTOS task entry point — create with osal_task_create(). */
void config_task(void *arg);
