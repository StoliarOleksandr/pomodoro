#include "hal/hal.h"
#include "platform/stm32f401/platform.h"

#include "main.h"
#include "stm32f4xx_hal.h"

extern I2C_HandleTypeDef hi2c1;

/* ---- tick / delay -------------------------------------------------------- */

uint32_t hal_tick_ms(void)
{
    return HAL_GetTick();
}

void hal_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/* ---- GPIO ---------------------------------------------------------------- */

void hal_gpio_write(hal_pin_t pin, uint8_t value)
{
    GPIO_PinState state = value ? GPIO_PIN_SET : GPIO_PIN_RESET;
    switch(pin)
    {
        case PIN_LED:
            HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, state);
            break;
        default:
            break;
    }
}

uint8_t hal_gpio_read(hal_pin_t pin)
{
    switch(pin)
    {
        case PIN_BUTTON:
            return (uint8_t)HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
        default:
            return 0;
    }
}

/* ---- I2C ----------------------------------------------------------------- */

int hal_i2c_mem_write(uint8_t dev_addr, uint8_t mem_addr,
                      const uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef status =
        HAL_I2C_Mem_Write(&hi2c1, dev_addr, mem_addr,
                          I2C_MEMADD_SIZE_8BIT, (uint8_t *)buf, len, 100);
    return (status == HAL_OK) ? 0 : -1;
}
