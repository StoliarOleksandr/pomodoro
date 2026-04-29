#include "pomodoro/pomodoro.h"

typedef enum
{
    PHASE_FOCUS,
    PHASE_BREAK,
    PHASE_BIG_BREAK,

} phase_t;

static pomodoro_cfg_t  _cfg;
static pomodoro_state_t _state;
static phase_t          _phase;
static pomodoro_time_t  _curr_time;
static uint32_t         _acc_ms;
static uint8_t          _cycles;

static void load_phase_time(void)
{
    _curr_time.seconds = 0;
    switch(_phase)
    {
        case PHASE_FOCUS:     _curr_time.minutes = _cfg.focus_min;     break;
        case PHASE_BREAK:     _curr_time.minutes = _cfg.break_min;     break;
        case PHASE_BIG_BREAK: _curr_time.minutes = _cfg.big_break_min; break;
    }
}

static void advance_phase(void)
{
    if(_phase == PHASE_FOCUS)
    {
        _cycles++;
        _phase = (_cycles % _cfg.cycles_before_big_break == 0) ? PHASE_BIG_BREAK
                                                                 : PHASE_BREAK;
    }
    else
    {
        _phase = PHASE_FOCUS;
    }
    load_phase_time();
}

void pomodoro_init(pomodoro_cfg_t cfg)
{
    _cfg   = cfg;
    _phase = PHASE_FOCUS;
    _cycles = 0;
    _acc_ms = 0;
    load_phase_time();
    _state = POMODORO_STATE_PAUSED;
}

pomodoro_action_t pomodoro_tick(uint32_t elapsed_ms)
{
    if(_state != POMODORO_STATE_RUNNING)
        return POMODORO_ACTION_NONE;

    _acc_ms += elapsed_ms;
    if(_acc_ms < 1000)
        return POMODORO_ACTION_NONE;

    _acc_ms -= 1000;

    if(_curr_time.seconds > 0)
    {
        _curr_time.seconds--;
    }
    else if(_curr_time.minutes > 0)
    {
        _curr_time.minutes--;
        _curr_time.seconds = 59;
    }
    else
    {
        _state  = POMODORO_STATE_ALARM;
        _acc_ms = 0;
    }

    return POMODORO_ACTION_DISPLAY_UPDATE;
}

pomodoro_action_t pomodoro_handle_event(button_event_t ev)
{
    switch(_state)
    {
        case POMODORO_STATE_PAUSED:
            if(ev == BUTTON_EVENT_CLICK)
            {
                _state  = POMODORO_STATE_RUNNING;
                _acc_ms = 0;
                return POMODORO_ACTION_DISPLAY_UPDATE;
            }
            break;

        case POMODORO_STATE_RUNNING:
            if(ev == BUTTON_EVENT_CLICK)
            {
                _state = POMODORO_STATE_PAUSED;
                return POMODORO_ACTION_DISPLAY_UPDATE;
            }
            break;

        case POMODORO_STATE_ALARM:
            if(ev == BUTTON_EVENT_CLICK)
            {
                advance_phase();
                _state  = POMODORO_STATE_PAUSED;
                _acc_ms = 0;
                return POMODORO_ACTION_DISPLAY_UPDATE;
            }
            break;

    }

    return POMODORO_ACTION_NONE;
}

pomodoro_state_t pomodoro_state(void)
{
    return _state;
}

pomodoro_time_t pomodoro_current_time(void)
{
    return _curr_time;
}

const char *pomodoro_phase_label(void)
{
    switch(_phase)
    {
        case PHASE_FOCUS:     return "Focus";
        case PHASE_BREAK:     return "Break";
        case PHASE_BIG_BREAK: return "Big Break";
        default:              return "";
    }
}

uint8_t pomodoro_cycle_count(void)
{
    return _cycles;
}

pomodoro_cfg_t pomodoro_get_cfg(void)
{
    return _cfg;
}

void pomodoro_apply_cfg(pomodoro_cfg_t cfg)
{
    _cfg.focus_min     = cfg.focus_min;
    _cfg.break_min     = cfg.break_min;
    _cfg.big_break_min = cfg.big_break_min;
    _phase  = PHASE_FOCUS;
    _acc_ms = 0;
    load_phase_time();
    _state = POMODORO_STATE_PAUSED;
}
