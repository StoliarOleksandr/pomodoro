#include "hal/hal.h"
#include "platform/stm32f401/platform.h"
#include "osal/osal.h"

#include "button/button.h"
#include "config/config_task.h"
#include "display/display.h"
#include "display/fonts.h"
#include "pomodoro/pomodoro.h"

#include <stdio.h>

#define DISPLAY_I2C_ADDR (0x3C << 1)

/* Semaphore that wakes task_display whenever a display update is needed. */
static osal_sem_t g_display_sem;

/* Semaphore that wakes task_config when long-press is detected. */
static osal_sem_t g_config_sem;

/* Queue carrying button events from task_main to task_config during config. */
static osal_queue_t g_btn_queue;

/* Set by task_main when entering config; cleared by task_config when done. */
volatile uint8_t g_in_config;

static void display_write_cb(uint8_t mem_addr, uint8_t *buf, size_t len)
{
    hal_i2c_mem_write(DISPLAY_I2C_ADDR, mem_addr, buf, (uint16_t)len);
}

static void render(void)
{
    pomodoro_time_t  t     = pomodoro_current_time();
    pomodoro_state_t state = pomodoro_state();

    char timer_buf[8];
    snprintf(timer_buf, sizeof(timer_buf), "%02d:%02d", t.minutes, t.seconds);

    display_fill(COLOR_BLACK);

    if(state == POMODORO_STATE_ALARM)
    {
        display_set_cursor(44, 2);
        display_write_string("DONE!", font_7x10, COLOR_WHITE);
    }

    display_set_cursor(29, 22);
    display_write_string(timer_buf, font_11x18, COLOR_WHITE);

    display_set_cursor(0, 50);
    display_write_string((char *)pomodoro_phase_label(), font_6x8, COLOR_WHITE);

    char cycle_buf[8];
    snprintf(cycle_buf, sizeof(cycle_buf), "Cyc:%d", pomodoro_cycle_count());
    display_set_cursor(92, 50);
    display_write_string(cycle_buf, font_6x8, COLOR_WHITE);

    display_update();
}

/* Display task — woken by g_display_sem, then refreshes every 1 s while
 * the timer is RUNNING (countdown visible). Blocks again when not running. */
static void task_display(void *arg)
{
    (void)arg;
    while(1)
    {
        osal_sem_take(&g_display_sem, OSAL_WAIT_FOREVER);
        render();

        while(pomodoro_state() == POMODORO_STATE_RUNNING)
        {
            osal_delay_ms(1000);
            render();
        }
    }
}

/* Main logic task — polls the button and drives the pomodoro state machine.
 * Long press routes to task_config; all other events go to the pomodoro FSM.
 * While g_in_config is set, button events are forwarded to g_btn_queue and
 * the pomodoro tick and display update are suppressed. */
static void task_main(void *arg)
{
    (void)arg;
    uint32_t last_tick      = hal_tick_ms();
    uint32_t alarm_blink_at = 0;
    uint8_t  led_on         = 0;

    while(1)
    {
        uint32_t now     = hal_tick_ms();
        uint32_t elapsed = now - last_tick;
        last_tick        = now;

        button_event_t ev = button_poll();

        if(g_in_config)
        {
            if(ev != BUTTON_EVENT_NONE)
                osal_queue_send(&g_btn_queue, &ev);
        }
        else
        {
            pomodoro_action_t action = POMODORO_ACTION_NONE;

            if(ev == BUTTON_EVENT_LONG_PRESS)
            {
                /* Turn LED off and hand control to config task. */
                if(led_on)
                {
                    led_on = 0;
                    hal_gpio_write(PIN_LED, 0);
                }
                g_in_config = 1;
                osal_sem_give(&g_config_sem);
            }
            else
            {
                if(ev != BUTTON_EVENT_NONE)
                    action |= pomodoro_handle_event(ev);

                action |= pomodoro_tick(elapsed);

                if(action & POMODORO_ACTION_DISPLAY_UPDATE)
                    osal_sem_give(&g_display_sem);

                if(pomodoro_state() == POMODORO_STATE_ALARM)
                {
                    if((now - alarm_blink_at) >= 500)
                    {
                        alarm_blink_at = now;
                        led_on ^= 1;
                        hal_gpio_write(PIN_LED, led_on);
                    }
                }
                else if(led_on)
                {
                    led_on = 0;
                    hal_gpio_write(PIN_LED, 0);
                }
            }
        }

        osal_delay_ms(10);
    }
}

int main(void)
{
    platform_init();

    button_cfg_t btn_cfg = {.long_press_ms = 5000, .double_click_ms = 500};
    button_init(PIN_BUTTON, btn_cfg);

    pomodoro_cfg_t pom_cfg = {
        .focus_min               = 25,
        .break_min               = 5,
        .big_break_min           = 15,
        .cycles_before_big_break = 4,
    };
    pomodoro_init(pom_cfg);

    display_set_cb(display_write_cb);
    hal_delay_ms(100);
    display_init();

    display_fill(COLOR_BLACK);
    display_set_cursor(20, 20);
    display_write_string("Pomodoro", font_11x18, COLOR_WHITE);
    display_update();
    hal_delay_ms(2000);

    osal_sem_create(&g_display_sem);
    osal_sem_give(&g_display_sem);   /* trigger initial render of 25:00 */

    osal_sem_create(&g_config_sem);
    osal_queue_create(&g_btn_queue, sizeof(button_event_t), 4);

    g_in_config = 0;
    config_task_init(&g_config_sem, &g_btn_queue, &g_display_sem, &g_in_config);

    osal_task_create(task_display, NULL, 512, 1, "display", NULL);
    osal_task_create(task_main,    NULL, 512, 2, "main",    NULL);
    osal_task_create(config_task,  NULL, 512, 2, "config",  NULL);

    osal_start();   /* never returns */
}
