# Pomodoro — Architecture

Firmware for an STM32F401 (Cortex-M4) Pomodoro timer with a monochrome OLED display and a single push-button. Structured for easy portability: all chip-specific code lives in `platform/stm32f401/`; all OS-specific code lives in `osal/freertos/`. Porting to another chip means implementing `platform/<name>/`; switching to another RTOS means implementing `osal/<name>/`.

## Hardware

| Peripheral | Role |
|---|---|
| STM32F401xE | MCU (84 MHz, Cortex-M4) |
| SSD1306-compatible OLED | 128×64 px monochrome display via I2C1 (100 kHz, address 0x3C) |
| B1 (PC13) | User button — pulled up, active-LOW, polled from `task_main` |
| LD2 (PA5) | Onboard LED — alarm blink indicator |
| USART2 (PA2/PA3) | Debug UART, 115200 baud |
| TIM2 | HAL timebase — 1 ms tick (SysTick is reserved for FreeRTOS) |
| RTC | Wired but inactive — reserved for future low-power use |

## Repository layout

```
pomodoro/
├── main.c                           # Wiring layer — three FreeRTOS tasks, no chip headers
│
├── hal/
│   └── hal.h                        # Portable HAL interface (5 functions)
│
├── osal/
│   ├── osal.h                       # Portable OSAL interface (tasks, semaphores, queues)
│   └── freertos/
│       ├── osal_impl.c              # FreeRTOS implementation of osal.h
│       ├── FreeRTOSConfig.h         # Kernel config (STM32F401, 84 MHz, TIM2 timebase)
│       └── CMakeLists.txt           # freertos_config + osal_impl targets
│
├── button/
│   ├── button.h                     # button_event_t, button_cfg_t, API
│   └── button.c                     # Polling state machine
│
├── config/
│   ├── config_task.h                # config_task_init(), config_task() entry point
│   └── config_task.c                # Three-step duration edit UI, renders directly to display
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
│       ├── platform.c               # platform_init(), peripheral init, TIM2 HAL timebase override
│       ├── hal_impl.c               # Implements hal/hal.h using STM32 HAL
│       └── CMakeLists.txt           # Vendor HAL + platform_impl static lib
│
├── Core/                            # STM32CubeMX-generated support files
│   ├── Inc/
│   │   ├── main.h                   # Pin/port aliases (B1_Pin, LD2_Pin, …)
│   │   ├── stm32f4xx_hal_conf.h
│   │   └── stm32f4xx_it.h
│   └── Src/
│       ├── stm32f4xx_it.c           # IRQ handlers: fault handlers, TIM2_IRQHandler (HAL tick)
│       │                            #   Note: SVC/PendSV/SysTick absent — FreeRTOS owns them
│       ├── stm32f4xx_hal_msp.c      # MSP init — GPIO/clock for I2C1, USART2
│       ├── system_stm32f4xx.c       # SystemInit, SystemCoreClock
│       ├── syscalls.c               # newlib stubs
│       └── sysmem.c                 # Heap (_sbrk)
│
├── Drivers/
│   ├── STM32F4xx_HAL_Driver/        # STM32 HAL sources
│   ├── CMSIS/                       # ARM CMSIS + ST device headers
│   └── FreeRTOS-Kernel/             # FreeRTOS V11.1.0 (git submodule, tag dbf7055)
│
├── cmake/
│   ├── gcc-arm-none-eabi.cmake      # GCC toolchain file
│   ├── starm-clang.cmake            # Clang/LLVM toolchain alternative
│   └── stm32cubemx/
│       └── CMakeLists.txt           # STM32_Drivers OBJECT lib + stm32cubemx INTERFACE lib
│
├── CMakeLists.txt                   # Top-level build (PLATFORM + OSAL cache variables)
└── CMakePresets.json                # Debug / Release presets
```

## Portability rules

**No file outside `platform/` may include a chip vendor header.**
**No file outside `osal/` may include an OS-specific header.**

`hal/hal.h` is the hardware boundary: it contains only portable C types and `extern` function declarations. `osal/osal.h` is the OS boundary: opaque handle types (`osal_task_t`, `osal_sem_t`, etc.) and function declarations. Application code (`main.c`, `button/`, `config/`, `pomodoro/`, `display/`) includes only these two headers and standard C headers.

## Component responsibilities

### `hal/hal.h`

Five functions that every platform must implement:

| Function | Description |
|---|---|
| `hal_tick_ms()` | Monotonic millisecond counter |
| `hal_delay_ms(ms)` | Blocking delay |
| `hal_gpio_write(pin, val)` | Drive a digital output |
| `hal_gpio_read(pin)` | Read a digital input |
| `hal_i2c_mem_write(addr, mem_addr, buf, len)` | Blocking I2C memory write |

### `osal/osal.h`

Portable OS primitives with opaque handles (`void *`). All RTOS knowledge is confined to `osal/<name>/osal_impl.c`.

| Primitive | Functions |
|---|---|
| Task | `osal_task_create`, `osal_yield`, `osal_delay_ms` |
| Semaphore | `osal_sem_create`, `osal_sem_give`, `osal_sem_give_from_isr`, `osal_sem_take` |
| Mutex | `osal_mutex_create`, `osal_mutex_lock`, `osal_mutex_unlock` |
| Queue | `osal_queue_create`, `osal_queue_send`, `osal_queue_send_from_isr`, `osal_queue_receive` |
| Lifecycle | `osal_start` |

`OSAL_WAIT_FOREVER` (`UINT32_MAX`) is the sentinel for indefinite blocking in `osal_sem_take`.

### `button/`

Polling-based button module — no interrupts.

`button_init(pin, cfg)` initialises with a `hal_pin_t` and timing thresholds. `button_poll()` is called once per `task_main` iteration; it reads the pin via `hal_gpio_read()`, tracks edges and timing, and returns a `button_event_t`.

Active-LOW convention: `hal_gpio_read() == 0` means button pressed.

| Event | Trigger |
|---|---|
| `BUTTON_EVENT_LONG_PRESS` | Release after ≥ 5000 ms hold |
| `BUTTON_EVENT_DOUBLE_CLICK` | Two releases within 500 ms of each other |
| `BUTTON_EVENT_CLICK` | Any other release |

### `config/`

Three-step configuration UI task. No chip or OS headers — only `osal/osal.h`, `pomodoro/pomodoro.h`, and `display/display.h`.

`config_task_init(trigger, btn_q, display_sem, in_config)` stores references to the four shared objects owned by `main.c`. `config_task()` is the FreeRTOS entry point: it blocks on `trigger`, sequences three `edit_value()` calls (Focus → Break → Long Break), calls `pomodoro_apply_cfg()`, clears the `g_in_config` flag, and gives `display_sem` to restore normal display operation.

`edit_value()` drives a single parameter step. A 500 ms timeout on `osal_queue_receive()` toggles the blink; `BUTTON_EVENT_CLICK` increments the value (1–60, wraps); `BUTTON_EVENT_DOUBLE_CLICK` confirms and returns.

`render_config()` draws the config screen directly to the display (bypassing `g_display_sem`):

```
y= 2  font_7x10   "FOCUS TIME" / "BREAK TIME" / "LGBRK TIME"
y=22  font_11x18  current value in minutes — blank when blink phase is off
y=50  font_6x8    "CLK:+1min  2xCLK:OK"
```

### `pomodoro/`

Pure logic module — no hardware calls, no `hal.h` dependency.

`pomodoro_init(cfg)` sets defaults and starts in `PAUSED` / Focus phase.
`pomodoro_tick(elapsed_ms)` advances the countdown; returns `POMODORO_ACTION_DISPLAY_UPDATE` when a second elapses or state changes.
`pomodoro_handle_event(button_event_t)` drives state transitions (CLICK and LONG_PRESS are handled; LONG_PRESS is intercepted by `task_main` before this function is reached, so it never arrives here in practice).

Getters: `pomodoro_state()`, `pomodoro_current_time()`, `pomodoro_phase_label()`, `pomodoro_cycle_count()`.

Config API: `pomodoro_get_cfg()` returns the current `pomodoro_cfg_t` snapshot; `pomodoro_apply_cfg(cfg)` writes the three duration fields, resets to Focus phase, and transitions to `PAUSED`.

### `display/`

Hardware-agnostic. Owns a static `display_buffer[1024]` (128×64 / 8 bytes).

All drawing functions write to this buffer. `display_update()` flushes it to the OLED in 8 page writes by calling the registered `display_write_data_cb_t` (injected via `display_set_cb()`).

SSD1306 protocol: control byte `0x00` for commands, `0x40` for data.

### `platform/stm32f401/`

`platform.c` — `platform_init()`, all `MX_*` peripheral initialisers, `Error_Handler`, HAL peripheral handles (`hi2c1`, `huart2`). Also overrides `HAL_InitTick` to redirect the HAL timebase from SysTick to TIM2 (see [SysTick / TIM2 timebase](#systick--tim2-timebase)).

`hal_impl.c` — implements every function declared in `hal/hal.h` using STM32 HAL calls. The only file that includes `stm32f4xx_hal.h`.

`platform.h` — defines `PIN_BUTTON` and `PIN_LED` as `hal_pin_t` integer constants.

### `main.c`

Wiring layer only. Runs before the RTOS scheduler starts:

1. `platform_init()`
2. `button_init(PIN_BUTTON, cfg)` — `long_press_ms = 5000`
3. `pomodoro_init(cfg)`
4. `display_set_cb(display_write_cb)` + `hal_delay_ms(100)` + `display_init()`
5. Greeting splash (2 s, using `hal_delay_ms` — safe pre-RTOS)
6. `osal_sem_create(&g_display_sem)` + `osal_sem_give` (primes initial render)
7. `osal_sem_create(&g_config_sem)` + `osal_queue_create(&g_btn_queue, sizeof(button_event_t), 4)`
8. `config_task_init(...)` — binds shared objects to the config task
9. `osal_task_create(task_display, ...)` + `osal_task_create(task_main, ...)` + `osal_task_create(config_task, ...)`
10. `osal_start()` — never returns

## Concurrency model

Three FreeRTOS tasks and three shared objects:

| Object | Type | Purpose |
|---|---|---|
| `g_display_sem` | Binary semaphore | `task_main` → `task_display`: trigger a render |
| `g_config_sem` | Binary semaphore | `task_main` → `task_config`: enter config mode |
| `g_btn_queue` | Queue (depth 4) | `task_main` → `task_config`: forward button events during config |
| `g_in_config` | `volatile uint8_t` | Routing flag: 1 while config is active |

| Task | Priority | Role |
|---|---|---|
| `task_main` | 2 | Button polling, pomodoro FSM, event routing, alarm LED |
| `task_config` | 2 | Configuration UI — active only during a config session |
| `task_display` | 1 | I2C OLED rendering |
| FreeRTOS idle | 0 | Background |

**Normal operation** (`g_in_config == 0`): `task_main` runs every 10 ms, calls `pomodoro_tick()`, and gives `g_display_sem` on `POMODORO_ACTION_DISPLAY_UPDATE`. `BUTTON_EVENT_LONG_PRESS` is intercepted before `pomodoro_handle_event()` — it turns off the LED, sets `g_in_config = 1`, and gives `g_config_sem`.

**Config session** (`g_in_config == 1`): `task_main` still polls the button every 10 ms but only forwards events to `g_btn_queue`; tick, display, and LED logic are bypassed. `task_config` wakes, runs the three edit steps, calls `pomodoro_apply_cfg()`, clears `g_in_config`, and gives `g_display_sem`. `task_display` remains blocked on `g_display_sem` throughout and never calls `render()` during config — `task_config` writes to the display directly.

```
Normal:
task_main (every 10 ms)              task_display (woken by semaphore)
─────────────────────────────────    ─────────────────────────────────────
pomodoro_tick(elapsed_ms)            osal_sem_take(&g_display_sem, FOREVER)
  └─ returns DISPLAY_UPDATE          render()              ← immediate draw
  └─ osal_sem_give(&g_display_sem)   while RUNNING:
                                       osal_delay_ms(1000)
                                       render()

Config (g_in_config == 1):
task_main (every 10 ms)              task_config (woken by g_config_sem)
─────────────────────────────────    ──────────────────────────────────────────
button_poll()                        osal_sem_take(&g_config_sem, FOREVER)
  └─ ev → osal_queue_send(btn_q)     edit_value(&focus_min,  "FOCUS TIME")
                                     edit_value(&break_min,  "BREAK TIME")
(no tick, no display sem, no LED)    edit_value(&big_brk_min,"LGBRK TIME")
                                     pomodoro_apply_cfg(cfg)
                                     g_in_config = 0
                                     osal_sem_give(&g_display_sem)
```

## SysTick / TIM2 timebase

FreeRTOS owns the SysTick interrupt. `FreeRTOSConfig.h` renames the port handlers to match the STM32 vector table:

```c
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
```

`SVC_Handler`, `PendSV_Handler`, and `SysTick_Handler` are **absent** from `stm32f4xx_it.c` — FreeRTOS port.c provides them via the macros above.

The STM32 HAL timebase is redirected to TIM2. `HAL_InitTick()` is overridden in `platform.c`:

- TIM2 clock = 84 MHz (APB1 timer clock; APB1 bus divider ÷ 2, timer clock × 2)
- PSC = 83 → 1 MHz after prescaler
- ARR = 999 → update interrupt at 1 kHz (1 ms period)
- `TIM2_IRQHandler` in `stm32f4xx_it.c` clears the flag and calls `HAL_IncTick()`

## State machine

The pomodoro state machine has three states. `LONG_PRESS` is intercepted by `task_main` before reaching `pomodoro_handle_event()` — it triggers `task_config` (see [Concurrency model](#concurrency-model)) and does not appear as a transition in the FSM itself.

```
             ┌──────────────────────────────────────┐
             │               PAUSED                 │◄──────────────────┐
             └──────┬───────────────────────────────┘                   │
                    │ CLICK                                              │
             ┌──────▼──────────────┐                                    │
             │      RUNNING        │── CLICK ──────────────────►PAUSED  │
             └──────┬──────────────┘                                    │
                    │ timeout (00:00)                                    │
             ┌──────▼──────────────┐                                    │
             │       ALARM         │── CLICK ──────────────────────────┘
             └─────────────────────┘  (advance phase, load next timer)
```

| State | Behaviour |
|---|---|
| `PAUSED` | Timer frozen. CLICK → RUNNING. |
| `RUNNING` | `pomodoro_tick` decrements each second. CLICK → PAUSED. Timeout → ALARM. |
| `ALARM` | Display shows "DONE!". LED blinks at ~1 Hz (`task_main`). CLICK → load next phase → PAUSED. |

LONG_PRESS (≥ 5 s) from any state suspends normal operation and activates `task_config`. On completion, `pomodoro_apply_cfg()` resets to PAUSED / Focus regardless of prior state.

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

```bash
# First-time: initialise submodules (FreeRTOS-Kernel)
git submodule update --init --recursive

# Configure (defaults to stm32f401 platform, freertos OSAL)
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake

# Build
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
| `freertos_config` | INTERFACE lib | Provides `FreeRTOSConfig.h` to the FreeRTOS kernel CMake |
| `freertos_kernel` | STATIC lib | FreeRTOS core + ARM_CM4F port + heap_4 |
| `osal_impl` | STATIC lib | `osal_impl.c`; links `freertos_kernel` |
| `pomodoro` | Executable | App sources; links `platform_impl`, `osal_impl`, `stm32cubemx` |

### Platform and OSAL selection

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=... \
  -DPLATFORM=stm32f401 \
  -DOSAL=freertos
```

Both default to their current single implementation. To add a new chip, create `platform/<name>/CMakeLists.txt` that builds a `platform_impl` static library. To add a new OS backend, create `osal/<name>/CMakeLists.txt` that builds an `osal_impl` static library.

## Pin assignments (STM32F401)

| Signal | Pin | Notes |
|---|---|---|
| B1 (button) | PC13 | Pull-up input, polled in `task_main` at 100 Hz |
| LD2 (LED) | PA5 | Push-pull output, alarm indicator |
| USART2 TX/RX | PA2 / PA3 | Debug UART |
| I2C1 SCL / SDA | PB8 / PB9 | Open-drain, no external pull-up needed on Nucleo |
| SWD (TMS/TCK/SWO) | PA13 / PA14 / PB3 | Debug/flash |

## Remaining gaps

- `cycles_before_big_break` is not configurable from the UI (fixed at 4 by default).
- Alarm has no audible output (no buzzer).
- No low-power sleep mode. RTC wakeup is available in hardware but RTC init is disabled.
- `render()` in `task_display` reads pomodoro state while `task_main` may be mid-`pomodoro_tick`. Currently safe because `task_main` has higher priority and the read is word-aligned, but a mutex around shared pomodoro state would be more correct.
