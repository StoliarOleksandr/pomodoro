#include "config/config_task.h"

#include "osal/osal.h"
#include "pomodoro/pomodoro.h"
#include "display/display.h"
#include "display/fonts.h"
#include "button/button.h"

#include <stdio.h>
#include <stdint.h>

static osal_sem_t      *_trigger;
static osal_queue_t    *_btn_q;
static osal_sem_t      *_display_sem;
static volatile uint8_t *_in_config;

void config_task_init(osal_sem_t     *trigger,
                      osal_queue_t   *btn_q,
                      osal_sem_t     *display_sem,
                      volatile uint8_t *in_config)
{
    _trigger    = trigger;
    _btn_q      = btn_q;
    _display_sem = display_sem;
    _in_config  = in_config;
}

/* Draw one config edit screen.
 * label   : "FOCUS TIME", "BREAK TIME", "LGBRK TIME"
 * value   : current minutes value (1-60)
 * show_val: 1 = draw the number, 0 = blank it (blink effect) */
static void render_config(const char *label, uint8_t value, uint8_t show_val)
{
    display_fill(COLOR_BLACK);

    display_set_cursor(14, 2);
    display_write_string((char *)label, font_7x10, COLOR_WHITE);

    if(show_val)
    {
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", value);
        display_set_cursor(46, 22);
        display_write_string(buf, font_11x18, COLOR_WHITE);
    }

    display_set_cursor(4, 50);
    display_write_string("CLK:+1min  2xCLK:OK", font_6x8, COLOR_WHITE);

    display_update();
}

/* Block until the user confirms a value with DOUBLE_CLICK.
 * CLICK increments the value (1-60, wraps).
 * The 500 ms queue timeout drives the blink toggle. */
static void edit_value(uint8_t *val, const char *label)
{
    uint8_t blink = 1;

    while(1)
    {
        render_config(label, *val, blink);

        button_event_t ev;
        if(!osal_queue_receive(_btn_q, &ev, 500))
        {
            blink ^= 1;
            continue;
        }

        blink = 1;

        if(ev == BUTTON_EVENT_CLICK)
        {
            (*val)++;
            if(*val > 60)
                *val = 1;
        }
        else if(ev == BUTTON_EVENT_DOUBLE_CLICK)
        {
            return;
        }
    }
}

void config_task(void *arg)
{
    (void)arg;

    while(1)
    {
        osal_sem_take(_trigger, OSAL_WAIT_FOREVER);

        pomodoro_cfg_t cfg = pomodoro_get_cfg();

        edit_value(&cfg.focus_min,     "FOCUS TIME");
        edit_value(&cfg.break_min,     "BREAK TIME");
        edit_value(&cfg.big_break_min, "LGBRK TIME");

        pomodoro_apply_cfg(cfg);

        *_in_config = 0;
        osal_sem_give(_display_sem);
    }
}
