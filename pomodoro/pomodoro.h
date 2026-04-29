#ifndef POMODORO_H
#define POMODORO_H

#include "button/button.h"

#include <stdint.h>

typedef enum
{
    POMODORO_STATE_PAUSED,
    POMODORO_STATE_RUNNING,
    POMODORO_STATE_ALARM,

} pomodoro_state_t;

typedef enum
{
    POMODORO_ACTION_NONE           = 0,
    POMODORO_ACTION_DISPLAY_UPDATE = (1 << 0),

} pomodoro_action_t;

typedef struct
{
    uint8_t focus_min;
    uint8_t break_min;
    uint8_t big_break_min;
    uint8_t cycles_before_big_break;

} pomodoro_cfg_t;

typedef struct
{
    uint8_t minutes;
    uint8_t seconds;

} pomodoro_time_t;

/* Initialise with default configuration. Starts in PAUSED / focus phase. */
void pomodoro_init(pomodoro_cfg_t cfg);

/* Advance the countdown by elapsed_ms. Call from the main loop every tick.
 * Returns DISPLAY_UPDATE when the visible state changed. */
pomodoro_action_t pomodoro_tick(uint32_t elapsed_ms);

/* Feed a button event into the state machine.
 * Returns DISPLAY_UPDATE when the visible state changed. */
pomodoro_action_t pomodoro_handle_event(button_event_t ev);

/* Current state. */
pomodoro_state_t pomodoro_state(void);

/* Current countdown (or config value while in CONFIG state). */
pomodoro_time_t pomodoro_current_time(void);

/* Human-readable phase label: "Focus", "Break", "Big Break", "Config". */
const char *pomodoro_phase_label(void);

/* Number of completed focus cycles since init. */
uint8_t pomodoro_cycle_count(void);

/* Return the current runtime configuration. */
pomodoro_cfg_t pomodoro_get_cfg(void);

/* Apply new time values and return to PAUSED / Focus phase.
 * cycles_before_big_break is preserved from the original config. */
void pomodoro_apply_cfg(pomodoro_cfg_t cfg);

#endif /* POMODORO_H */
