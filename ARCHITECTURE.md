# Pomodoro — Architecture

Firmware for an STM32F401 (Cortex-M4) Pomodoro timer with a monochrome OLED display and a single push-button. Structured for easy portability: all chip-specific code lives in `platform/stm32f401/`; porting to another chip means implementing that one directory.

## Hardware

| Peripheral | Role |
|---|---|
| STM32F401xE | MCU (84 MHz, Cortex-M4) |
| SSD1306-compatible OLED | 128×64 px monochrome display via I2C1 (100 kHz, address 0x3C) |
| B1 (PC13) | User button — pulled up, active-LOW, polled from main loop |
| LD2 (PA5) | Onboard LED — alarm blink indicator |
| USART2 (PA2/PA3) | Debug UART, 115200 baud |
| RTC | Wired but inactive — reserved for future low-power 1 Hz tick |

## Repository layout

```
pomodoro/
├── main.c                           # Wiring layer only — no chip headers
│
├── hal/
│   └── hal.h                        # Portable HAL interface (5 functions + platform_init)
│
├── button/
│   ├── button.h                     # button_event_t, button_cfg_t, API
│   └── button.c                     # Polling state machine
│
├── pomodoro/
│   ├── pomodoro.h                   # State/action types, API
│   └── pomodoro.c                   # Countdown, state machine, phase/cycle logic
│
├── display/
│   ├── display.h / display.c        # SSD1306 framebuffer + drawing API (no chip headers)
│   └── fonts.h / fonts.c            # Bitmap fonts: 6x8, 7x10, 11x18, 16x15, 16x24, 16x26
│
├── platform/
│   └── stm32f401/
│       ├── platform.h               # PIN_BUTTON, PIN_LED constants
│       ├── platform.c               # platform_init(), peripheral init, Error_Handler
│       ├── hal_impl.c               # Implements hal/hal.h using STM32 HAL
│       └── CMakeLists.txt           # Vendor HAL + platform_impl static lib
│
├── Core/                            # STM32CubeMX-generated support files
│   ├── Inc/
│   │   ├── main.h                   # Pin/port aliases (B1_Pin, LD2_Pin, …)
│   │   ├── stm32f4xx_hal_conf.h
│   │   └── stm32f4xx_it.h
│   └── Src/
│       ├── stm32f4xx_it.c           # IRQ handlers (fault handlers, RTC_WKUP stub)
│       ├── stm32f4xx_hal_msp.c      # MSP init — GPIO/clock for I2C1, USART2, RTC
│       ├── system_stm32f4xx.c       # SystemInit, SystemCoreClock
│       ├── syscalls.c               # newlib stubs
│       └── sysmem.c                 # Heap (_sbrk)
│
├── Drivers/
│   ├── STM32F4xx_HAL_Driver/        # STM32 HAL sources
│   └── CMSIS/                       # ARM CMSIS + ST device headers
│
├── cmake/
│   ├── gcc-arm-none-eabi.cmake      # GCC toolchain file
│   ├── starm-clang.cmake            # Clang/LLVM toolchain alternative
│   └── stm32cubemx/
│       └── CMakeLists.txt           # STM32_Drivers OBJECT lib + stm32cubemx INTERFACE lib
│
├── CMakeLists.txt                   # Top-level build (PLATFORM cache variable)
└── CMakePresets.json                # Debug / Release presets
```

## Portability rule

**No file outside `platform/` may include a chip vendor header.**

`hal/hal.h` is the boundary: it contains only portable C types and `extern` function declarations. `hal_impl.c` is the only application-level file that includes `stm32f4xx_hal.h`.

## Component responsibilities

### `hal/hal.h`

Five functions that every platform must implement:

| Function | Description |
|---|---|
| `platform_init()` | Initialise clocks, peripherals, GPIO |
| `hal_tick_ms()` | Monotonic millisecond counter |
| `hal_delay_ms(ms)` | Blocking delay |
| `hal_gpio_write(pin, val)` | Drive a digital output |
| `hal_gpio_read(pin)` | Read a digital input |
| `hal_i2c_mem_write(addr, mem_addr, buf, len)` | Blocking I2C memory write |

### `button/`

Polling-based button module — no interrupts.

`button_init(pin, cfg)` initialises with a `hal_pin_t` and timing thresholds. `button_poll()` is called once per main loop iteration; it reads the pin via `hal_gpio_read()`, tracks edges and timing, and returns a `button_event_t`.

Active-LOW convention: `hal_gpio_read() == 0` means button pressed.

| Event | Trigger |
|---|---|
| `BUTTON_EVENT_LONG_PRESS` | Release after ≥ 2000 ms hold |
| `BUTTON_EVENT_DOUBLE_CLICK` | Two releases within 500 ms of each other |
| `BUTTON_EVENT_CLICK` | Any other release |

### `pomodoro/`

Pure logic module — no hardware calls, no `hal.h` dependency.

`pomodoro_init(cfg)` sets defaults and starts in `PAUSED` / Focus phase.
`pomodoro_tick(elapsed_ms)` advances the countdown; returns `POMODORO_ACTION_DISPLAY_UPDATE` when a second elapses or state changes.
`pomodoro_handle_event(button_event_t)` drives state transitions.

Getters: `pomodoro_state()`, `pomodoro_current_time()`, `pomodoro_phase_label()`, `pomodoro_cycle_count()`.

### `display/`

Hardware-agnostic. Owns a static `display_buffer[1024]` (128×64 / 8 bytes).

All drawing functions write to this buffer. `display_update()` flushes it to the OLED in 8 page writes by calling the registered `display_write_data_cb_t` (injected via `display_set_cb()`).

SSD1306 protocol: control byte `0x00` for commands, `0x40` for data.

### `platform/stm32f401/`

`platform.c` — `platform_init()`, all `MX_*` peripheral initialisers, `Error_Handler`, HAL peripheral handles (`hi2c1`, `huart2`, `hrtc`).

`hal_impl.c` — implements every function declared in `hal/hal.h` using STM32 HAL calls. The only file that includes `stm32f4xx_hal.h`.

`platform.h` — defines `PIN_BUTTON` and `PIN_LED` as `hal_pin_t` integer constants.

### `main.c`

Wiring layer only:

1. `platform_init()`
2. `button_init(PIN_BUTTON, cfg)`
3. `pomodoro_init(cfg)`
4. `display_set_cb(display_write_cb)` + `hal_delay_ms(100)` + `display_init()`
5. Greeting splash
6. Main loop: `button_poll()` → `pomodoro_handle_event()` → `pomodoro_tick()` → `render()` + alarm LED blink

Includes only: `hal/hal.h`, `platform/stm32f401/platform.h`, `button/button.h`, `pomodoro/pomodoro.h`, `display/display.h`, `display/fonts.h`.

## State machine

```
             ┌─────────────────────────────────────────────┐
             │               PAUSED                        │◄──────────────────┐
             └──────┬──────────────────────────────────────┘                   │
                    │ CLICK                      LONG_PRESS (any state)         │
             ┌──────▼──────────────┐              ────────────────────►         │
             │      RUNNING        │                     CONFIG                 │
             └──────┬──────────────┘              ◄──────────────────           │
                    │ CLICK                     LONG_PRESS (save)               │
                    │ → PAUSED                  DOUBLE_CLICK (discard)          │
                    │                                                           │
                    │ timeout (00:00)                                           │
             ┌──────▼──────────────┐                                           │
             │       ALARM         │── CLICK ──────────────────────────────────┘
             └─────────────────────┘  (advance phase, load next timer)
```

| State | Behaviour |
|---|---|
| `PAUSED` | Timer frozen. CLICK → RUNNING. LONG_PRESS → CONFIG. |
| `RUNNING` | `pomodoro_tick` decrements each second. CLICK → PAUSED. Timeout → ALARM. |
| `ALARM` | Display shows "DONE!". LED blinks at 1 Hz (main loop). CLICK → load next phase → PAUSED. |
| `CONFIG` | CLICK increments focus duration (+1 min, wraps at 60). LONG_PRESS saves and resets to focus/PAUSED. DOUBLE_CLICK discards and restores previous time. |

## Phase and cycle logic

```
Focus phase complete (ALARM CLICK) → _cycles++
  if _cycles % cycles_before_big_break == 0 → Big Break
  else → Break

Break / Big Break complete (ALARM CLICK) → Focus
```

Default configuration: focus 25 min, break 5 min, big break 15 min, big break every 4 cycles.

## I2C transport

`display_write_cb` in `main.c` calls `hal_i2c_mem_write()` which wraps `HAL_I2C_Mem_Write` (blocking, 100 ms timeout).

- No DMA, no queue — simple and reliable.
- I2C1 at 100 kHz, SDA = PB9, SCL = PB8 (open-drain, configured in `stm32f4xx_hal_msp.c`).
- Display I2C address: `0x3C << 1`.

## Build system

```
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build -j$(nproc)

# Flash
st-flash write build/pomodoro.bin 0x08000000
```

### CMake targets

| Target | Type | Description |
|---|---|---|
| `STM32_Drivers` | OBJECT lib | All STM32 HAL `.c` sources |
| `stm32cubemx` | INTERFACE lib | Include paths + compile definitions (`USE_HAL_DRIVER`, `STM32F401xE`) |
| `platform_impl` | STATIC lib | `platform.c` + `hal_impl.c`; links `stm32cubemx` |
| `pomodoro` | Executable | App sources + links `platform_impl` + `stm32cubemx` |

### Platform selection

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=... -DPLATFORM=stm32f401
```

Default is `stm32f401`. To add a new chip, create `platform/<name>/CMakeLists.txt` that builds a `platform_impl` static library implementing `hal/hal.h`, then pass `-DPLATFORM=<name>`.

## Pin assignments (STM32F401)

| Signal | Pin | Notes |
|---|---|---|
| B1 (button) | PC13 | Pull-up input, polled at main-loop rate |
| LD2 (LED) | PA5 | Push-pull output, alarm indicator |
| USART2 TX/RX | PA2 / PA3 | Debug UART |
| I2C1 SCL / SDA | PB8 / PB9 | Open-drain, no external pull-up needed on Nucleo |
| SWD (TMS/TCK/SWO) | PA13 / PA14 / PB3 | Debug/flash |

## Remaining gaps

- `STATE_CONFIG` only edits focus duration. Break and big-break durations are not yet configurable from the UI.
- RTC 1 Hz wakeup is wired in `stm32f4xx_it.c` but RTC init is disabled — the main loop uses `HAL_GetTick()` polling via `hal_tick_ms()` instead.
- Alarm has no audible output (no buzzer).
- No low-power sleep mode between events.
