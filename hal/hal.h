#ifndef HAL_H
#define HAL_H

#include <stddef.h>
#include <stdint.h>

/* Opaque pin identifier. Each platform maps these to concrete GPIO resources. */
typedef uint8_t hal_pin_t;

/* Platform entry point — initialises clocks, peripherals, and GPIOs. */
void platform_init(void);

/* Monotonic millisecond counter (wraps after ~49 days). */
uint32_t hal_tick_ms(void);

/* Blocking delay. */
void hal_delay_ms(uint32_t ms);

/* Drive a digital output pin. value: 0 = low, non-zero = high. */
void hal_gpio_write(hal_pin_t pin, uint8_t value);

/* Read a digital input pin. Returns 0 or 1. */
uint8_t hal_gpio_read(hal_pin_t pin);

/* Blocking I2C memory write. Returns 0 on success, negative on error. */
int hal_i2c_mem_write(uint8_t dev_addr, uint8_t mem_addr,
                      const uint8_t *buf, uint16_t len);

#endif /* HAL_H */
