#include "button/button.h"

static hal_pin_t    _pin;
static button_cfg_t _cfg;

static uint8_t  _prev_level;
static uint32_t _press_start;
static uint32_t _last_release;
static uint8_t  _click_count;

void button_init(hal_pin_t pin, button_cfg_t cfg)
{
    _pin         = pin;
    _cfg         = cfg;
    _prev_level  = hal_gpio_read(pin);
    _press_start = 0;
    _last_release = 0;
    _click_count  = 0;
}

button_event_t button_poll(void)
{
    uint8_t        level = hal_gpio_read(_pin);
    uint32_t       now   = hal_tick_ms();
    button_event_t ev    = BUTTON_EVENT_NONE;

    if(_prev_level == 1 && level == 0)
    {
        /* falling edge — button pressed */
        _press_start = now;
    }
    else if(_prev_level == 0 && level == 1)
    {
        /* rising edge — button released */
        uint32_t held = now - _press_start;
        if(held >= _cfg.long_press_ms)
        {
            ev           = BUTTON_EVENT_LONG_PRESS;
            _click_count = 0;
        }
        else
        {
            _click_count++;
            _last_release = now;
        }
    }

    /* Resolve pending clicks once we know whether a second click follows */
    if(_click_count > 0 && level == 1)
    {
        if(_click_count >= 2)
        {
            ev           = BUTTON_EVENT_DOUBLE_CLICK;
            _click_count = 0;
        }
        else if((now - _last_release) >= _cfg.double_click_ms)
        {
            ev           = BUTTON_EVENT_CLICK;
            _click_count = 0;
        }
    }

    _prev_level = level;
    return ev;
}
