cmake_minimum_required(VERSION 3.22)

# Provide FreeRTOSConfig.h to the kernel.
# The kernel's CMakeLists links freertos_kernel against this target if it exists.
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "" FORCE)
set(FREERTOS_HEAP 4            CACHE STRING "" FORCE)

add_subdirectory(
    ${CMAKE_SOURCE_DIR}/Drivers/FreeRTOS-Kernel
    ${CMAKE_BINARY_DIR}/freertos_kernel
)

# OSAL implementation — the only file that includes FreeRTOS headers.
add_library(osal_impl STATIC osal_impl.c)

target_include_directories(osal_impl PUBLIC
    ${CMAKE_SOURCE_DIR}          # resolves osal/osal.h
    ${CMAKE_CURRENT_SOURCE_DIR}  # resolves FreeRTOSConfig.h
)

target_link_libraries(osal_impl PUBLIC freertos_kernel)

target_link_libraries(${CMAKE_PROJECT_NAME} osal_impl)
