#ifndef BUTTON_H
#define BUTTON_H

#include "hal/hal.h"

#include <stdint.h>

typedef enum
{
    BUTTON_EVENT_NONE,
    BUTTON_EVENT_CLICK,
    BUTTON_EVENT_LONG_PRESS,
    BUTTON_EVENT_DOUBLE_CLICK,

} button_event_t;

typedef struct
{
    uint16_t long_press_ms;   /* hold duration that triggers a long press */
    uint16_t double_click_ms; /* max gap between two releases for double-click */

} button_cfg_t;

/* Initialise the button module. Call once before button_poll(). */
void button_init(hal_pin_t pin, button_cfg_t cfg);

/* Poll the button state. Call from the main loop at >= 10 ms rate.
 * Returns the detected event and resets it to NONE.
 * Active-LOW convention: hal_gpio_read() == 0 means button is pressed. */
button_event_t button_poll(void);

#endif /* BUTTON_H */
