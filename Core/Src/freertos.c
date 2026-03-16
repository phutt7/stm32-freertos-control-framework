/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  ******************************************************************************
  */
/* USER CODE END Header */

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */
#include "ring_buffer.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* USER CODE BEGIN Variables */
extern RingBuffer_t adcRingBuffer;
extern UART_HandleTypeDef huart2;
extern IWDG_HandleTypeDef hiwdg;

osMessageQueueId_t adcQueueHandle;

#define ADC_THRESHOLD  2048
volatile uint32_t overflowCount = 0;
volatile uint16_t lastADCAverage = 0;
/* USER CODE END Variables */

void StartDefaultTask(void *argument);
void StartControlTask(void *argument);
void StartLoggingTask(void *argument);

void MX_FREERTOS_Init(void)
{
  adcQueueHandle = osMessageQueueNew(16, sizeof(uint16_t), NULL);

  const osThreadAttr_t defaultTask_attr = {
    .name = "DataTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t) osPriorityNormal,
  };
  osThreadNew(StartDefaultTask, NULL, &defaultTask_attr);

  const osThreadAttr_t controlTask_attr = {
    .name = "ControlTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t) osPriorityHigh,
  };
  osThreadNew(StartControlTask, NULL, &controlTask_attr);

  const osThreadAttr_t loggingTask_attr = {
    .name = "LoggingTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t) osPriorityLow,
  };
  osThreadNew(StartLoggingTask, NULL, &loggingTask_attr);
}

void StartDefaultTask(void *argument)
{
  uint16_t sample = 0;
  uint32_t sum = 0;
  uint32_t count = 0;
  uint16_t average = 0;
  uint16_t lastAverage = 0;
  uint8_t stuckCount = 0;

  for(;;)
  {
    while (RingBuffer_Read(&adcRingBuffer, &sample))
    {
      sum += sample;
      count++;
      if (count >= 16)
      {
        average = (uint16_t)(sum / count);
        lastADCAverage = average;
        sum = 0;
        count = 0;

        if (average == lastAverage)
        {
          stuckCount++;
          if (stuckCount >= 10)
          {
            HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
            overflowCount = 0xDEAD;
            stuckCount = 0;
          }
        }
        else
        {
          stuckCount = 0;
        }
        lastAverage = average;

        if (osMessageQueuePut(adcQueueHandle, &average, 0, 0) != osOK)
          overflowCount++;
      }
    }
    osDelay(1);
  }
}

void StartControlTask(void *argument)
{
  uint16_t average = 0;

  for(;;)
  {
    if (osMessageQueueGet(adcQueueHandle, &average, NULL, 100) == osOK)
    {
      if (average > ADC_THRESHOLD)
      {
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);  // relay OFF
      }
      else
      {
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);    // relay ON
      }
    }
    HAL_IWDG_Refresh(&hiwdg);
  }
}

void StartLoggingTask(void *argument)
{
  char msg[64];

  for(;;)
  {
    UBaseType_t stackA = uxTaskGetStackHighWaterMark(NULL);

    sprintf(msg, "ADC:%u OVFL:%lu STK:%u\r\n",
            lastADCAverage,
            overflowCount,
            (unsigned int)stackA);

    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
    osDelay(500);
  }
}
