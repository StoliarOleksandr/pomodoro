# Pomodoro

A physical Pomodoro timer running on an STM32F401 Nucleo board with a 128×64 OLED display and a single push-button. Built on FreeRTOS with a hardware abstraction layer (HAL) and an OS abstraction layer (OSAL) for portability.

## Hardware

- **MCU**: STM32F401xE (Cortex-M4, 84 MHz)
- **Display**: SSD1306-compatible 128×64 OLED over I2C1 (address 0x3C)
- **Button**: B1 (PC13), active-LOW, polled
- **Alarm indicator**: LD2 LED (PA5)

## Quick start

```bash
# Fetch FreeRTOS submodule
git submodule update --init --recursive

# Build
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build -j$(nproc)

# Flash
st-flash write build/pomodoro.bin 0x08000000
```

## Usage

| Press | Effect |
|---|---|
| Short press | Start / pause timer |
| Short press (alarm) | Dismiss alarm, load next phase |
| Long press (≥ 5 s) | Enter configuration mode |
| Short press in config | Increment current duration by 1 min (wraps at 60) |
| Double-click in config | Confirm value and advance to next step |

Configuration steps in order: Focus duration → Break duration → Long Break duration.

Default schedule: 25 min focus → 5 min break → big break (15 min) every 4 cycles.

## Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) — layers, concurrency model, state machine, build system
- [PRODUCT.md](PRODUCT.md) — features, hardware summary, QA tests, roadmap
- [CONTRIBUTION.md](CONTRIBUTION.md) — how to build, portability rules, adding platforms and OS backends
