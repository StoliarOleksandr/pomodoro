# Pomodoro — Product Overview

A small, low-power physical Pomodoro timer with a monochrome OLED display and single-button input. Designed to be simple, portable (firmware), and inexpensive (hardware).

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
- Firmware structured for easy chip portability — only `platform/stm32f401/` is chip-specific.

## Button interaction map

| Interaction | Effect |
|---|---|
| Short press (CLICK) in PAUSED | Start timer |
| Short press in RUNNING | Pause timer |
| Short press in ALARM | Dismiss alarm, load next phase, go to PAUSED |
| Short press in CONFIG | Increment focus duration by 1 minute (wraps at 60) |
| Long press (≥ 2 s) from any state | Enter CONFIG mode |
| Long press in CONFIG | Save new focus duration, return to PAUSED |
| Double-click in CONFIG | Discard edits, return to PAUSED |

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
| Alarm indicator | Onboard LED PA5, 1 Hz blink during alarm |
| Debug | USART2 @ 115200 baud (PA2/PA3) |
| Flash/debug port | SWD (TMS PA13, TCK PA14, SWO PB3) |
| I2C | I2C1 @ 100 kHz (SCL PB8, SDA PB9) |
| Power | 3.3 V; coin cell (CR2032) or small LiPo + regulator |

## Software / firmware

- **Build system**: CMake, selectable platform via `-DPLATFORM=stm32f401`.
- **Portable modules** (no chip headers):

  | Module | Path | Role |
  |---|---|---|
  | HAL interface | `hal/hal.h` | 6-function contract for all hardware access |
  | Button | `button/` | Polling event detection |
  | Pomodoro logic | `pomodoro/` | State machine, countdown, phase/cycle tracking |
  | Display driver | `display/` | SSD1306 framebuffer + drawing API |

- **Platform module** (chip-specific, STM32F401):

  | File | Role |
  |---|---|
  | `platform/stm32f401/platform.c` | Peripheral init, clock config |
  | `platform/stm32f401/hal_impl.c` | Implements `hal/hal.h` via STM32 HAL |

- **I2C transport**: blocking `HAL_I2C_Mem_Write` (100 ms timeout). No DMA required.

## User interface

The 128×64 display is divided into three rows:

- **Top (y = 2)**: state banner — "DONE!" during alarm, "CONFIG" during configuration, empty otherwise.
- **Middle (y = 22)**: current countdown in `MM:SS` format, 11×18 pixel font.
- **Bottom (y = 50)**: phase label ("Focus" / "Break" / "Big Break" / "Config") and cycle count ("Cyc:N").

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
| Short press | Timer starts counting down; another short press pauses it |
| Long press (≥ 2 s) | "CONFIG" banner appears; timer shows current focus duration |
| Short press in CONFIG | Focus duration increments by 1 min on each press |
| Long press in CONFIG | Returns to PAUSED with updated focus duration |
| Double-click in CONFIG | Returns to PAUSED with original duration (no change) |
| Timer reaches 00:00 | "DONE!" banner appears, LED blinks at 1 Hz |
| Short press on alarm | Loads next phase (Break or Big Break), returns to PAUSED |
| After 4 focus cycles | Big Break (15 min) is loaded instead of regular Break |
| Full display refresh | No corruption, no frozen pixels |

## Debugging

- UART debug (USART2, 115200 baud) — add `printf` calls via `huart2` for logging.
- SWD — use an ST-Link and `st-flash` or OpenOCD for flashing and GDB debugging.
- Logic analyzer on I2C lines to diagnose display issues: check ACK after the address byte and after each data byte.

## Roadmap / future features

- Configurable break and big-break durations from the CONFIG UI.
- Audible alarm via a piezo buzzer (requires hardware addition).
- Low-power sleep between events (RTC wakeup is already wired, init disabled).
- BLE integration for remote monitoring or mobile app sync.
- Battery level indicator (requires ADC + voltage divider).
