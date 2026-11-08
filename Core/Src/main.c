/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>

// Lớp 1: Hệ thống & Scheduler
#include "global.h"
#include "scheduler.h"
#include "software_timer.h"

// Lớp 2: Drivers Phần cứng
#include "button.h"
#include "traffic_light.h"
#include "lcd.h"
#include "dht20.h"

// Lớp 3: Máy trạng thái FSM
#include "fsm_automatic.h"
#include "fsm_manual.h"
#include "fsm_tuning.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) SCH_Update();
}
// Task 1: Quét 4 nút nhấn chống dội (10ms / 1 tick)
void task_button_scan(void) {
    button_reading();
}

// Task 2: Chạy bộ đếm Software Timer (10ms / 1 tick)
void task_timer_run(void) {
    timerRun();
}

// Task 3: Chạy Máy trạng thái đi�?u khiển hệ thống (10ms / 1 tick)
void task_fsm_run(void) {
	switch (app_mode){
	case MODE_NORMAL:
	    fsm_automatic_run();
	    break;
	case MODE_MANUAL:
	    fsm_manual_run();
	    break;
	case MODE_SET_RED:
	case MODE_SET_AMBER:
	case MODE_SET_GREEN:
	    fsm_tuning_run();
	    break;
	default:
	    fsm_automatic_run();
	    break;
	}
}

// Task 4: Chuẩn bị nội dung hiển thị LCD.
char buf_line1[17];
char buf_line2[17];

static void format_lcd_lines(void) {
    if (app_mode == MODE_MANUAL && fsm_manual_phase_change_active()) {
        snprintf(buf_line1, sizeof(buf_line1), "CHANGE PHASE");
        snprintf(buf_line2, sizeof(buf_line2), "WAITING IN: %us",
                 (unsigned int)fsm_manual_phase_change_seconds());
        return;
    }

    switch (app_mode) {
        case MODE_NORMAL:
            switch (status) {
                case PHASE_A_GREEN:
                    snprintf(buf_line1, sizeof(buf_line1), "AUTO A:G%02u B:R%02u",
                             (unsigned int)count1, (unsigned int)count2);
                    break;
                case PHASE_A_AMBER:
                    snprintf(buf_line1, sizeof(buf_line1), "AUTO A:Y%02u B:R%02u",
                             (unsigned int)count1, (unsigned int)count2);
                    break;
                case PHASE_B_GREEN:
                    snprintf(buf_line1, sizeof(buf_line1), "AUTO A:R%02u B:G%02u",
                             (unsigned int)count1, (unsigned int)count2);
                    break;
                case PHASE_B_AMBER:
                    snprintf(buf_line1, sizeof(buf_line1), "AUTO A:R%02u B:Y%02u",
                             (unsigned int)count1, (unsigned int)count2);
                    break;
                default:
                    snprintf(buf_line1, sizeof(buf_line1), "TRAFFIC ERROR");
                    break;
            }
            if (dht20_ok) {
                int t_tenths = (int)(temperature * 10.0f);
                int t_magnitude = (t_tenths < 0) ? -t_tenths : t_tenths;
                const char *t_sign = (t_tenths < 0) ? "-" : "";

                int h_tenths = (int)(humidity * 10.0f);

                int written = snprintf(buf_line2, sizeof(buf_line2),
                                       "T:%s%d.%dC H:%d.%d%%",
                                       t_sign, t_magnitude / 10,
                                       t_magnitude % 10, h_tenths / 10,
                                       h_tenths % 10);
                if (written < 0) {
                    written = 0;
                } else if (written > (int)(sizeof(buf_line2) - 1)) {
                    written = (int)(sizeof(buf_line2) - 1);
                }

                memset(buf_line2 + written, ' ',
                       sizeof(buf_line2) - 1 - (size_t)written);
                buf_line2[sizeof(buf_line2) - 1] = '\0';
            } else {
                snprintf(buf_line2, sizeof(buf_line2), "DHT20 ERROR!    ");
            }
            break;
        case MODE_MANUAL:
            snprintf(buf_line1, sizeof(buf_line1), "MODE MANUAL     ");
            switch (status) {
                case PHASE_A_GREEN: snprintf(buf_line2, sizeof(buf_line2), "1:GREEN 2:RED   "); break;
                case PHASE_A_AMBER: snprintf(buf_line2, sizeof(buf_line2), "1:AMBER 2:RED   "); break;
                case PHASE_B_GREEN: snprintf(buf_line2, sizeof(buf_line2), "1:RED 2:GREEN   "); break;
                case PHASE_B_AMBER: snprintf(buf_line2, sizeof(buf_line2), "1:RED 2:AMBER   "); break;
                default:            snprintf(buf_line2, sizeof(buf_line2), "1:OFF 2:OFF     "); break;
            }
            break;

        /* ================================================================== */
        /* 3. CHẾ �?Ộ CHỈNH THỜI GIAN (MODE_TUNING)                            */
        /* ================================================================== */
        case MODE_SET_RED:
            snprintf(buf_line1, sizeof(buf_line1), "MODE SET RED    ");
            snprintf(buf_line2, sizeof(buf_line2), "RED TIME:%02us   ",
                     (unsigned int)count1);
            break;

        case MODE_SET_GREEN:
            snprintf(buf_line1, sizeof(buf_line1), "MODE SET GREEN  ");
            snprintf(buf_line2, sizeof(buf_line2), "GREEN TIME:%02us ",
                     (unsigned int)count1);
            break;

        case MODE_SET_AMBER:
            snprintf(buf_line1, sizeof(buf_line1), "MODE SET AMBER  ");
            snprintf(buf_line2, sizeof(buf_line2), "AMBER TIME:%02us ",
                     (unsigned int)count1);
            break;

        default:
            snprintf(buf_line1, sizeof(buf_line1), "WAITING         ");
            snprintf(buf_line2, sizeof(buf_line2), "                ");
            break;
    }

}

void task_lcd_update_line1(void) {
    format_lcd_lines();
    if (lcd_write_line(0U, buf_line1) != HAL_OK) {
        return;
    }
}

void task_lcd_update_line2(void) {
    format_lcd_lines();
    if (lcd_write_line(1U, buf_line2) != HAL_OK) {
        return;
    }
}

// Task 5: Đọc định kỳ DHT20; các thao tác I2C có timeout giới hạn.
void task_dht20_read(void) {
    float temp = 0.0f, hum = 0.0f;
    int dht_status = DHT20_Tick(&temp, &hum);

    if (dht_status == 1) {
        temperature = temp;
        humidity = hum;
        dht20_ok = 1;
    } else if (dht_status == 2) {
        dht20_ok = 0;
    }
}

// Task 6: Nháy LED PA5 báo nhịp tim hệ thống (500ms / 50 ticks)
void task_led_heartbeat(void) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  // 1. Khởi tạo LCD
  lcd_init();
  lcd_goto_XY(0, 0);
  lcd_send_string("SYSTEM READY!");

  // 2. Khởi tạo trạng thái nút nhấn và cảm biến
  button_init_state();
  DHT20_InitState();

  // 3. Khởi tạo FSM
  app_mode = MODE_NORMAL;
  fsm_automatic_init();
  SCH_Init();

    // 3. �?ăng ký các Task vào Scheduler (�?ơn vị: Tick 10ms)
  SCH_Add_Task(task_button_scan,  0, 1);    // Chu kỳ 10ms
  SCH_Add_Task(task_timer_run,    0, 1);    // Chu kỳ 10ms
  SCH_Add_Task(task_fsm_run,      0, 1);    // Chu kỳ 10ms
  SCH_Add_Task(task_lcd_update_line1, 0, 20);  // Cập nhật dòng 1 mỗi 200ms
  SCH_Add_Task(task_lcd_update_line2, 10, 20); // Cập nhật dòng 2 lệch 100ms
  SCH_Add_Task(task_dht20_read,  10, 1);    // Quét state machine mỗi 10ms
  SCH_Add_Task(task_led_heartbeat,0, 50);   // Chu kỳ 500ms
  HAL_TIM_Base_Start_IT(&htim2);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

      SCH_Dispatch_Tasks();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
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
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
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
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TL2_B_Pin|TL1_A_Pin|TL2_A_Pin|TL1_B_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC5 PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : TL2_B_Pin TL1_A_Pin TL2_A_Pin TL1_B_Pin */
  GPIO_InitStruct.Pin = TL2_B_Pin|TL1_A_Pin|TL2_A_Pin|TL1_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN3_Pin */
  GPIO_InitStruct.Pin = BTN3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN1_Pin BTN2_Pin */
  GPIO_InitStruct.Pin = BTN1_Pin|BTN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN4_Pin */
  GPIO_InitStruct.Pin = BTN4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN4_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
