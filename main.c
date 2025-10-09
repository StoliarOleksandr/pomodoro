#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "display/display.h"
#include "display/fonts.h"


#define DISPLAY_ADDRESS 0x3C<<1
#define FOCUS_MESSAGE "Focus"
#define BREAK_MESSAGE "Break"
#define BIG_BREAK_MESSAGE "Big Break"
#define GREETING_MESSAGE "Pomodoro"


typedef enum  
{
    STATE_IDLE,
    STATE_CONFIG,
    STATE_RUNNING,
    STATE_PAUSED,
    STATE_ALARM

} state_t;

typedef enum
{
    BUTTON_EVENT_NONE,
    BUTTON_EVENT_CLICK,
    BUTTON_EVENT_LONG_PRESS,
    BUTTON_EVENT_DOUBLE_CLICK,

} button_event_t;

typedef struct 
{
    uint8_t minutes;
    uint8_t seconds;

} time_t;

typedef struct
{
    uint16_t long_press_time;
    uint16_t double_click_time;
    uint16_t click_time;

} button_init_t;

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

static struct
{
    state_t state;
    time_t curr_time;
    time_t focus_time;
    time_t break_time;
    time_t big_break_time;

    uint8_t cycles;

} _self = { .state = STATE_IDLE, .focus_time = { 25, 0 }, .break_time = { 5, 0 }, .big_break_time = { 15, 0 }, .cycles = 0 };

volatile static struct 
{
    button_event_t event;
    uint8_t click_count;
    bool state;
    uint32_t press_time;
    uint32_t last_click_time;

    uint16_t long_press_time;
    uint16_t double_click_time;
    uint16_t click_time;

} _button = { 0 };

static char timer_buffer[6] = { 0 };


static void button_init(button_init_t* init){};

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

void assert_failed(uint8_t *file, uint32_t line)
{

}

int main(void)
{
  // Initialize the HAL Library 
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  display_init();

  display_fill(COLOR_BLACK);
  display_set_cursor(20, 20);
  display_write_string(GREETING_MESSAGE, Font_11x18, COLOR_WHITE);
  display_update();
  HAL_Delay(3500);

  snprintf(timer_buffer, sizeof(timer_buffer), "%02d:%02d", _self.focus_time.minutes, _self.focus_time.seconds);
  timer_buffer[5] = '\0';
  display_fill(Black);
  display_set_cursor(40, 20);
  display_write_string(timer_buffer, Font_11x18, COLOR_WHITE);
  display_update();
  
  while(1)
  {
      uint32_t now = HAL_GetTick();
      static uint32_t last_tick = 0;           /* for second counting */
      static uint32_t last_display_update = 0; /* to update display once per second */
      static state_t prev_state = STATE_IDLE;

      switch(_button.event)
      {
          case BUTTON_EVENT_CLICK:
              _button.event = BUTTON_EVENT_NONE;
              _self.state = _self.state == STATE_RUNNING ? STATE_PAUSED : STATE_RUNNING;
              break;

          case BUTTON_EVENT_LONG_PRESS:
              _button.event = BUTTON_EVENT_NONE;
              _self.state = STATE_CONFIG;
              break;

          case BUTTON_EVENT_DOUBLE_CLICK:
              _button.event = BUTTON_EVENT_NONE;
              if(_self.state == STATE_CONFIG)
              {
                  _self.state = STATE_IDLE;
              }
              break;

          case BUTTON_EVENT_NONE:
          default:
              break;
      }
      
      switch(_self.state)
      {
          case STATE_IDLE:
              _self.curr_time = _self.focus_time;
              _self.state = STATE_PAUSED;          
              break;

          case STATE_CONFIG:
              break;

          case STATE_RUNNING:
              if(_self.curr_time.seconds == 0)
              {
                  if(_self.curr_time.minutes == 0)
                  {
                      _self.state = STATE_ALARM;
                      break;
                  }
                  else
                  {
                      _self.curr_time.minutes--;
                      _self.curr_time.seconds = 59;
                  }
              }
              else
              {
                  if(HAL_GetTick() % 1000 == 0)
                      _self.curr_time.seconds--;
              }

              if (prev_state != STATE_RUNNING) {
                  last_tick = now;
              }
              break;

          case STATE_PAUSED:
              break;

          case STATE_ALARM:
              HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
              break;

          default:
              _self.state = STATE_IDLE;
              break;
      }

      if(HAL_GetTick() % 1000 != 0)
          continue;

      snprintf(timer_buffer, sizeof(timer_buffer), "%02d:%02d", _self.curr_time.minutes, _self.curr_time.seconds);
      timer_buffer[5] = '\0';
      display_fill(Black);
      display_set_cursor(40, 20);
      display_write_string(timer_buffer, Font_11x18, COLOR_WHITE);
      display_update();
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin != B1_Pin) 
        return;
    
    if(HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET)
    {
        _button.state = true;
        _button.press_time = HAL_GetTick();
    }
    else
    {
        _button.state = false;
        if((HAL_GetTick() - _button.press_time) > 2000)
        {
            _button.event = BUTTON_EVENT_LONG_PRESS;
        }
        else
        {
            _button.event = BUTTON_EVENT_CLICK;
            _button.click_count++;
            if(_button.click_count == 1)
            {
                _button.last_click_time = HAL_GetTick();
            }
            else if(_button.click_count == 2)
            {
                if((HAL_GetTick() - _button.last_click_time) < 500)
                {
                    _button.event = BUTTON_EVENT_DOUBLE_CLICK;
                    _button.click_count = 0;
                }
                else
                {
                    _button.last_click_time = HAL_GetTick();
                    _button.click_count = 1;
                }
            }
        }
    }
}
