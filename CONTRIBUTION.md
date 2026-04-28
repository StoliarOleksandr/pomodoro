# Contributing to Pomodoro

Thank you for your interest in contributing. This document explains the preferred workflow, code style, build steps, and review expectations.

## How to start

1. Fork the repository and create a feature branch from `main` (or the branch maintainers indicate):
   - Branch naming: `feature/<short-desc>`, `bugfix/<short-desc>`, or `docs/<short-desc>`.
   - Keep branches small and focused on a single change.

2. Make your changes and open a Pull Request back to the repository.

## Build locally

The project uses CMake with a GCC Arm cross-toolchain.

```bash
# Configure (defaults to STM32F401 platform)
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake

# Build
cmake --build build -j$(nproc)

# Flash (requires st-flash)
st-flash write build/pomodoro.bin 0x08000000
```

To target a different platform:

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=... -DPLATFORM=<name>
```

`compile_commands.json` is emitted to `build/` automatically and symlinked at the repo root for clangd/IDE integration.

## Project structure and the portability rule

**No file outside `platform/` may include a chip vendor header.**

| Layer | May include |
|---|---|
| `hal/hal.h` | `<stdint.h>`, `<stddef.h>` only |
| `button/`, `pomodoro/`, `display/` | `hal/hal.h`, standard C headers |
| `main.c` | All of the above + `platform/<name>/platform.h` |
| `platform/<name>/` | Anything, including chip HAL headers |

Enforce this when reviewing PRs: a CI grep for `stm32f4xx` outside `platform/` and `Core/` should find nothing.

## Adding a new platform

1. Create `platform/<chip-name>/`.
2. Implement `hal/hal.h` (six functions + `platform_init()`) in `hal_impl.c`.
3. Write `platform.h` with `PIN_BUTTON` / `PIN_LED` and any other board-level constants.
4. Write `CMakeLists.txt` that builds a `platform_impl` static library and links it into `${CMAKE_PROJECT_NAME}`.
5. Build with `-DPLATFORM=<chip-name>` and verify.

No changes to `main.c`, `button/`, `pomodoro/`, or `display/` should be needed.

## Coding conventions

- Language: C11.
- Formatting: `.clang-format` is present at the repo root — run `clang-format -i <file>` before committing.
- Includes: project-local headers first (e.g., `#include "hal/hal.h"`), then system headers.
- No dynamic allocation outside platform code. All modules use static storage.
- No global mutable state in portable modules (`button/`, `pomodoro/`, `display/`). State is file-scoped `static`.
- Do not modify CubeMX-generated files in `Core/` or `Drivers/` unless necessary; keep changes in the user-code sections.

## Tests and validation

- **Unit tests**: the `pomodoro/` and `button/` modules have no hardware dependencies and can be compiled and tested on a host machine with a minimal stub for `hal/hal.h`.
- **Hardware tests**: after any peripheral change (I2C, GPIO, UART), verify on target hardware.
- **Display tests**: after any `display/` change, run a full power cycle and visually confirm the timer renders correctly with no corruption.
- **Button tests**: exercise CLICK, LONG_PRESS (≥ 2 s), and DOUBLE_CLICK (two presses within 500 ms).

## Pull request checklist

- [ ] Branch builds cleanly: `cmake --build build` with no errors or warnings.
- [ ] Portability rule respected: no chip headers outside `platform/`.
- [ ] `ARCHITECTURE.md`, `PLAN.md`, and `PRODUCT.md` updated if the structure or behaviour changed.
- [ ] Hardware-facing changes include manual test notes (button sequence, display check, UART log snippet).
- [ ] No unrelated file changes.

## Review process

- PRs are reviewed by maintainers. Expect requests for clarification or small fixes.
- Keep PRs small to reduce review time.
- Be responsive to comments and include test results if requested.

## Security and responsible disclosure

- Do not commit secrets, keys, or credentials.
- Report security vulnerabilities via a private issue.

## Code of conduct

Be respectful and collaborative. Follow standard open-source community etiquette.

## Contact

For architecture or hardware questions, open an issue tagged `question` or `help wanted`.
