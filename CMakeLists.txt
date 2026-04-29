cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME pomodoro)

# Emit compile_commands.json for clangd / IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C ASM)

# Platform to build for. Create platform/<name>/CMakeLists.txt to port.
set(PLATFORM "stm32f401" CACHE STRING "Target platform (subdirectory under platform/)")

# ── Executable ──────────────────────────────────────────────────────────────

add_executable(${CMAKE_PROJECT_NAME})

# Portable application sources — no chip headers allowed here.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    main.c
    button/button.c
    pomodoro/pomodoro.c
    display/display.c
    display/fonts.c
)

# Project root on the include path so #include "hal/hal.h" etc. resolve.
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ── Platform ─────────────────────────────────────────────────────────────────
# The platform subdirectory is responsible for:
#   - pulling in the vendor HAL / BSP
#   - building a 'platform_impl' static library that implements hal/hal.h
#   - linking that library into ${CMAKE_PROJECT_NAME}

add_subdirectory(platform/${PLATFORM})

# OSAL — create osal/<name>/CMakeLists.txt to add a new OS backend.
set(OSAL "freertos" CACHE STRING "OS backend (subdirectory under osal/)")
add_subdirectory(osal/${OSAL})

# Workaround: some toolchains inject a spurious 'ob' library dependency.
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
