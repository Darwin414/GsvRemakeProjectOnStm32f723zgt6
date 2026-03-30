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
#include "stm32h7xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define I2C2_SDA_Pin GPIO_PIN_0
#define I2C2_SDA_GPIO_Port GPIOF
#define I2C2_SCL_Pin GPIO_PIN_1
#define I2C2_SCL_GPIO_Port GPIOF
#define I2C4_SCL_Pin GPIO_PIN_14
#define I2C4_SCL_GPIO_Port GPIOF
#define I2C4_SDA_Pin GPIO_PIN_15
#define I2C4_SDA_GPIO_Port GPIOF
#define GSV6715_INT_Pin GPIO_PIN_14
#define GSV6715_INT_GPIO_Port GPIOD
#define GSV6715_INT_EXTI_IRQn EXTI15_10_IRQn
#define RESETB_Pin GPIO_PIN_4
#define RESETB_GPIO_Port GPIOG

/* USER CODE BEGIN Private defines */
#define I2C0_SCL_MODER_Msk GPIO_MODER_MODER1_Msk
#define I2C0_SCL_MODER_0 GPIO_MODER_MODER1_0
#define I2C0_SCL_Pin GPIO_PIN_1
#define I2C0_SCL_GPIO_Port GPIOF
#define I2C0_SCL_INPUT_PULLUP    (I2C0_SCL_GPIO_Port->MODER = (I2C0_SCL_GPIO_Port->MODER) & (~GPIO_MODER_MODER1_Msk))
#define I2C0_SCL_OUTPUT_PUSHPULL (I2C0_SCL_GPIO_Port->MODER = (I2C0_SCL_GPIO_Port->MODER) | (GPIO_MODER_MODER1_0))

#define I2C0_SDA_MODER_Msk GPIO_MODER_MODER0_Msk
#define I2C0_SDA_MODER_0 GPIO_MODER_MODER0_0
#define I2C0_SDA_Pin GPIO_PIN_0
#define I2C0_SDA_GPIO_Port GPIOF
#define I2C0_SDA_INPUT_PULLUP    (I2C0_SDA_GPIO_Port->MODER = (I2C0_SDA_GPIO_Port->MODER) & (~GPIO_MODER_MODER0_Msk))
#define I2C0_SDA_OUTPUT_PUSHPULL (I2C0_SDA_GPIO_Port->MODER = (I2C0_SDA_GPIO_Port->MODER) | (GPIO_MODER_MODER0_0))

#define I2C1_SCL_MODER_Msk GPIO_MODER_MODER14_Msk
#define I2C1_SCL_MODER_0 GPIO_MODER_MODER14_0
#define I2C1_SCL_Pin GPIO_PIN_14
#define I2C1_SCL_GPIO_Port GPIOF
#define I2C1_SCL_INPUT_PULLUP    (I2C1_SCL_GPIO_Port->MODER = (I2C1_SCL_GPIO_Port->MODER) & (~GPIO_MODER_MODER14_Msk))
#define I2C1_SCL_OUTPUT_PUSHPULL (I2C1_SCL_GPIO_Port->MODER = (I2C1_SCL_GPIO_Port->MODER) | (GPIO_MODER_MODER14_0))

#define I2C1_SDA_MODER_Msk GPIO_MODER_MODER15_Msk
#define I2C1_SDA_MODER_0 GPIO_MODER_MODER15_0
#define I2C1_SDA_Pin GPIO_PIN_15
#define I2C1_SDA_GPIO_Port GPIOF
#define I2C1_SDA_INPUT_PULLUP    (I2C1_SDA_GPIO_Port->MODER = (I2C1_SDA_GPIO_Port->MODER) & (~GPIO_MODER_MODER15_Msk))
#define I2C1_SDA_OUTPUT_PUSHPULL (I2C1_SDA_GPIO_Port->MODER = (I2C1_SDA_GPIO_Port->MODER) | (GPIO_MODER_MODER15_0))
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
