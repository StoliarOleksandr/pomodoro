cmake_minimum_required(VERSION 3.22)

# Pull in the STM32CubeMX-generated HAL library from the repo root.
# cmake/stm32cubemx/CMakeLists.txt uses ${CMAKE_SOURCE_DIR}-relative paths
# for Core/ and Drivers/ includes, so it resolves correctly from here.
add_subdirectory(
    ${CMAKE_SOURCE_DIR}/cmake/stm32cubemx
    ${CMAKE_BINARY_DIR}/stm32cubemx)

# Platform static library — the only place that includes STM32 HAL headers.
add_library(platform_impl STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/platform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal_impl.c
)

target_include_directories(platform_impl PUBLIC
    ${CMAKE_SOURCE_DIR}                        # resolves hal/, button/, etc.
    ${CMAKE_CURRENT_SOURCE_DIR}                # resolves platform/stm32f401/
)

target_link_libraries(platform_impl PUBLIC stm32cubemx)

# Wire the platform into the top-level executable.
target_link_libraries(${CMAKE_PROJECT_NAME}
    platform_impl
    stm32cubemx
)
