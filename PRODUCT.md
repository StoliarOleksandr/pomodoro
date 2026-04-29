# Pomodoro — Product Overview

A small physical Pomodoro timer with a monochrome OLED display and single-button input. Designed to be simple, portable (firmware), and inexpensive (hardware).

## Target users

- Knowledge workers and students who use the Pomodoro technique for timeboxing.
- Makers building compact timer or productivity devices.

## Key features

- 128×64 monochrome OLED display showing the active countdown and phase label.
- Single push button with three interaction types: short press, long press, double-click.
- Configurable durations: Focus (default 25 min), Break (5 min), Big Break (15 min).
- Automatic big-break scheduling after every 4 focus cycles.
- Cycle counter displayed on screen.
- On-screen "DONE!" alarm banner + LED blink when a phase completes.
- Firmware structured for easy chip portability (`platform/`) and OS portability (`osal/`).

## Button interaction map

| Interaction | Effect |
|---|---|
| Short press (CLICK) in PAUSED | Start timer |
| Short press in RUNNING | Pause timer |
| Short press in ALARM | Dismiss alarm, load next phase, go to PAUSED |
| Long press (≥ 5 s) from any state | Enter configuration mode |
| Short press in CONFIG (any step) | Increment current duration by 1 min (1–60, wraps) |
| Double-click in CONFIG (any step) | Confirm current value and advance to next step |

Configuration steps in order: Focus duration → Break duration → Long Break duration. After all three are confirmed the timer resets to PAUSED / Focus with the new values.

## Phase progression

```
Focus → Break → Focus → Break → Focus → Break → Focus → Big Break → (repeat)
                                                  ↑ every 4th focus cycle
```

Each phase transition requires the user to dismiss the alarm with a short press.

## Hardware summary

| Component | Detail |
|---|---|
| MCU | STM32F401xE — Cortex-M4, 84 MHz |
| Display | SSD1306-compatible 128×64 OLED, I2C (address 0x3C) |
| Button | Single tactile push-button, PC13, active-LOW pull-up |
| Alarm indicator | Onboard LED PA5, ~1 Hz blink during alarm |
| Debug | USART2 @ 115200 baud (PA2/PA3) |
| Flash/debug port | SWD (TMS PA13, TCK PA14, SWO PB3) |
| I2C | I2C1 @ 100 kHz (SCL PB8, SDA PB9) |
| Power | 3.3 V; coin cell (CR2032) or small LiPo + regulator |

## Software / firmware

- **Build system**: CMake, selectable platform via `-DPLATFORM=stm32f401`, selectable OS backend via `-DOSAL=freertos`.
- **RTOS**: FreeRTOS V11.1.0 (git submodule). Three tasks: `task_main` (state machine + routing, priority 2), `task_config` (configuration UI, priority 2), `task_display` (OLED rendering, priority 1).
- **Portable modules** (no chip or OS headers):

  | Module | Path | Role |
  |---|---|---|
  | HAL interface | `hal/hal.h` | 5-function contract for all hardware access |
  | OSAL interface | `osal/osal.h` | OS-agnostic tasks, semaphores, queues |
  | Button | `button/` | Polling event detection |
  | Config task | `config/` | Three-step duration configuration UI |
  | Pomodoro logic | `pomodoro/` | State machine, countdown, phase/cycle tracking |
  | Display driver | `display/` | SSD1306 framebuffer + drawing API |

- **Platform module** (chip-specific, STM32F401):

  | File | Role |
  |---|---|
  | `platform/stm32f401/platform.c` | Peripheral init, clock config, TIM2 HAL timebase |
  | `platform/stm32f401/hal_impl.c` | Implements `hal/hal.h` via STM32 HAL |

- **OSAL module** (OS-specific, FreeRTOS):

  | File | Role |
  |---|---|
  | `osal/freertos/osal_impl.c` | Implements `osal/osal.h` via FreeRTOS API |
  | `osal/freertos/FreeRTOSConfig.h` | Kernel configuration for STM32F401 |

- **I2C transport**: blocking `HAL_I2C_Mem_Write` (100 ms timeout), called from `task_display`. No DMA required.

## User interface

The 128×64 display is divided into three rows:

Normal timer display:

- **Top (y = 2)**: state banner — "DONE!" during alarm, empty otherwise.
- **Middle (y = 22)**: current countdown in `MM:SS` format, 11×18 pixel font.
- **Bottom (y = 50)**: phase label ("Focus" / "Break" / "Big Break") and cycle count ("Cyc:N").

Configuration display (one screen per step):

- **Top (y = 2)**: step label — "FOCUS TIME", "BREAK TIME", or "LGBRK TIME".
- **Middle (y = 22)**: current value in minutes, blinking every 500 ms.
- **Bottom (y = 50)**: interaction hint — "CLK:+1min  2xCLK:OK".

Font sizes were chosen for maximum legibility at 128×64 resolution.

## Manufacturing notes

- Use an integrated SSD1306 OLED module to reduce routing complexity and eliminate external pull-up concerns.
- Verify I2C pull-up resistors (4.7 kΩ to 3.3 V on SCL/SDA) if using a bare OLED panel.
- Place a 4-pin SWD header for in-field reprogramming (SWDIO, SWDCLK, GND, 3V3).
- Decoupling caps (100 nF) on MCU VDD and VDDA pins near the device.

## QA / acceptance tests

| Test | Expected result |
|---|---|
| Power-on | "Pomodoro" greeting for 2 s, then 25:00 Focus timer in PAUSED |
| Short press | Timer starts counting down; display updates each second |
| Short press while RUNNING | Timer pauses; display freezes |
| Long press (≥ 5 s) | "FOCUS TIME" config screen appears with blinking current value |
| Short press in FOCUS TIME | Value increments by 1 min; wraps from 60 back to 1 |
| Double-click in FOCUS TIME | Advances to "BREAK TIME" screen |
| Double-click in BREAK TIME | Advances to "LGBRK TIME" screen |
| Double-click in LGBRK TIME | Config saved; display reverts to PAUSED timer with new values |
| Timer reaches 00:00 | "DONE!" banner appears, LED blinks at ~1 Hz |
| Short press on alarm | Loads next phase (Break or Big Break), returns to PAUSED |
| After 4 focus cycles | Big Break (15 min) is loaded instead of regular Break |
| Full display refresh | No corruption, no frozen pixels |

## Debugging

- UART debug (USART2, 115200 baud) — add `printf` calls via `huart2` for logging.
- SWD — use an ST-Link and `st-flash` or OpenOCD for flashing and GDB debugging.
- Logic analyzer on I2C lines to diagnose display issues: check ACK after the address byte and after each data byte.

## Roadmap / future features

- Configurable `cycles_before_big_break` from the config UI (currently fixed at 4).
- Audible alarm via a piezo buzzer (requires hardware addition).
- Low-power sleep between events (RTC wakeup is available in hardware; init currently disabled).
- BLE integration for remote monitoring or mobile app sync.
- Battery level indicator (requires ADC + voltage divider).
