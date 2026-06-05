/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_BUILTIN_Pin GPIO_PIN_13
#define LED_BUILTIN_GPIO_Port GPIOC
#define IR_LEFT_Pin GPIO_PIN_0
#define IR_LEFT_GPIO_Port GPIOA
#define IR_CENTER_Pin GPIO_PIN_1
#define IR_CENTER_GPIO_Port GPIOA
#define IR_RIGHT_Pin GPIO_PIN_2
#define IR_RIGHT_GPIO_Port GPIOA
#define CHIPEN_ESP01_Pin GPIO_PIN_3
#define CHIPEN_ESP01_GPIO_Port GPIOA
#define SW0_Pin GPIO_PIN_4
#define SW0_GPIO_Port GPIOA
#define SW1_Pin GPIO_PIN_5
#define SW1_GPIO_Port GPIOA
#define SW2_Pin GPIO_PIN_6
#define SW2_GPIO_Port GPIOA
#define SW3_Pin GPIO_PIN_7
#define SW3_GPIO_Port GPIOA
#define USART_ESP_TX_Pin GPIO_PIN_10
#define USART_ESP_TX_GPIO_Port GPIOB
#define USART_ESP_RX_Pin GPIO_PIN_11
#define USART_ESP_RX_GPIO_Port GPIOB
#define ECHO_Pin GPIO_PIN_12
#define ECHO_GPIO_Port GPIOB
#define ECHO_EXTI_IRQn EXTI15_10_IRQn
#define TRIG_Pin GPIO_PIN_13
#define TRIG_GPIO_Port GPIOB
#define IN_2_Pin GPIO_PIN_14
#define IN_2_GPIO_Port GPIOB
#define IN_1_Pin GPIO_PIN_15
#define IN_1_GPIO_Port GPIOB
#define SERVO_Pin GPIO_PIN_8
#define SERVO_GPIO_Port GPIOA
#define IN_4_Pin GPIO_PIN_6
#define IN_4_GPIO_Port GPIOB
#define IN_3_Pin GPIO_PIN_7
#define IN_3_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
