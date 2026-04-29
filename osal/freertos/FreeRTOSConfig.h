#pragma once

/* ---- Scheduler ---------------------------------------------------------- */
#define configUSE_PREEMPTION                     1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1  /* Cortex-M4 CLZ instruction */
#define configUSE_TICKLESS_IDLE                  0
#define configCPU_CLOCK_HZ                       84000000UL
#define configTICK_RATE_HZ                       1000  /* 1 ms tick */
#define configMAX_PRIORITIES                     5
#define configMINIMAL_STACK_SIZE                 128   /* words */
#define configMAX_TASK_NAME_LEN                  12
#define configUSE_16_BIT_TICKS                   0
#define configIDLE_SHOULD_YIELD                  1
#define configUSE_TASK_NOTIFICATIONS             1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES    1
#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              0
#define configUSE_COUNTING_SEMAPHORES            0
#define configUSE_QUEUE_SETS                     0
#define configQUEUE_REGISTRY_SIZE                0
#define configUSE_TIME_SLICING                   1

/* ---- Memory ------------------------------------------------------------- */
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configTOTAL_HEAP_SIZE                    (12 * 1024)
#define configAPPLICATION_ALLOCATED_HEAP         0

/* ---- Hook functions ----------------------------------------------------- */
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configUSE_MALLOC_FAILED_HOOK             0
#define configCHECK_FOR_STACK_OVERFLOW           0

/* ---- Runtime stats / trace ---------------------------------------------- */
#define configGENERATE_RUN_TIME_STATS            0
#define configUSE_TRACE_FACILITY                 0
#define configUSE_STATS_FORMATTING_FUNCTIONS     0

/* ---- Co-routines -------------------------------------------------------- */
#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES          2

/* ---- Software timers ---------------------------------------------------- */
#define configUSE_TIMERS                         0
#define configTIMER_TASK_PRIORITY                3
#define configTIMER_QUEUE_LENGTH                 5
#define configTIMER_TASK_STACK_DEPTH             128

/* ---- Optional API functions --------------------------------------------- */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_xResumeFromISR                   1
#define INCLUDE_vTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_xTaskGetIdleTaskHandle           0
#define INCLUDE_uxTaskGetStackHighWaterMark      0
#define INCLUDE_xTaskAbortDelay                  0
#define INCLUDE_xTimerPendFunctionCall           0

/* ---- Interrupt priority ------------------------------------------------- */
/* STM32F4 has 4 NVIC priority bits (0 = highest, 15 = lowest).
 * FreeRTOS kernel runs at the lowest priority.
 * ISRs that call FromISR functions must have NVIC priority >= MAX_SYSCALL (5). */
#define configPRIO_BITS                          4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY  15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ---- Assert ------------------------------------------------------------- */
#define configASSERT(x) \
    do { if ((x) == 0) { taskDISABLE_INTERRUPTS(); while (1); } } while (0)

/* ---- Handler name mapping ----------------------------------------------- */
/* FreeRTOS port.c defines vPortSVCHandler, xPortPendSVHandler,
 * xPortSysTickHandler. These macros rename them to match the STM32 vector
 * table. stm32f4xx_it.c must NOT define SVC_Handler, PendSV_Handler, or
 * SysTick_Handler — FreeRTOS owns those three vectors. */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
