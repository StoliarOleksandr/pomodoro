#include "hal/hal.h"
#include "platform/stm32f401/platform.h"

#include "button/button.h"
#include "display/display.h"
#include "display/fonts.h"
#include "pomodoro/pomodoro.h"

#include <stdio.h>
#include <string.h>

#define DISPLAY_I2C_ADDR (0x3C << 1)

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

    /* Top banner — shown in alarm and config states */
    if(state == POMODORO_STATE_ALARM)
    {
        display_set_cursor(44, 2);
        display_write_string("DONE!", font_7x10, COLOR_WHITE);
    }
    else if(state == POMODORO_STATE_CONFIG)
    {
        display_set_cursor(37, 2);
        display_write_string("CONFIG", font_7x10, COLOR_WHITE);
    }

    /* Timer — centred on the 128-px wide display, font is 11 px wide */
    display_set_cursor(29, 22);
    display_write_string(timer_buf, font_11x18, COLOR_WHITE);

    /* Phase label + cycle counter at the bottom */
    display_set_cursor(0, 50);
    display_write_string((char *)pomodoro_phase_label(), font_6x8, COLOR_WHITE);

    char cycle_buf[8];
    snprintf(cycle_buf, sizeof(cycle_buf), "Cyc:%d", pomodoro_cycle_count());
    display_set_cursor(92, 50);
    display_write_string(cycle_buf, font_6x8, COLOR_WHITE);

    display_update();
}

int main(void)
{
    platform_init();

    /* Button — B1 is active-LOW, 2 s long-press, 500 ms double-click window */
    button_cfg_t btn_cfg = {.long_press_ms = 2000, .double_click_ms = 500};
    button_init(PIN_BUTTON, btn_cfg);

    /* Pomodoro timer defaults */
    pomodoro_cfg_t pom_cfg = {
        .focus_min               = 25,
        .break_min               = 5,
        .big_break_min           = 15,
        .cycles_before_big_break = 4,
    };
    pomodoro_init(pom_cfg);

    /* Display — give the OLED 100 ms to boot, then initialise */
    display_set_cb(display_write_cb);
    hal_delay_ms(100);
    display_init();

    /* Greeting splash */
    display_fill(COLOR_BLACK);
    display_set_cursor(20, 20);
    display_write_string("Pomodoro", font_11x18, COLOR_WHITE);
    display_update();
    hal_delay_ms(2000);

    render();

    uint32_t last_tick      = hal_tick_ms();
    uint32_t alarm_blink_at = 0;
    uint8_t  led_on         = 0;

    while(1)
    {
        uint32_t now     = hal_tick_ms();
        uint32_t elapsed = now - last_tick;
        last_tick        = now;

        button_event_t    ev     = button_poll();
        pomodoro_action_t action = POMODORO_ACTION_NONE;

        if(ev != BUTTON_EVENT_NONE)
            action |= pomodoro_handle_event(ev);

        action |= pomodoro_tick(elapsed);

        if(action & POMODORO_ACTION_DISPLAY_UPDATE)
            render();

        /* Alarm LED blinks at 1 Hz while in ALARM state */
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
