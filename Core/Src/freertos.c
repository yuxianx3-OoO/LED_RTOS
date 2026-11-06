/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include <string.h>

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
/* USER CODE BEGIN Variables */

// 串口接收字符队列
static osMessageQueueId_t uartRxQueueHandle;

// HAL 串口单字节接收缓冲区
static uint8_t uartRxByte;

// 0 = 全灭；1 = 流水
static volatile uint8_t g_ledMode = 1;
/* USER CODE END Variables */
/* Definitions for ledTask */
osThreadId_t ledTaskHandle;
const osThreadAttr_t ledTask_attributes = {
  .name = "ledTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for keyTask */
osThreadId_t keyTaskHandle;
const osThreadAttr_t keyTask_attributes = {
  .name = "keyTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for uartTask */
osThreadId_t uartTaskHandle;
const osThreadAttr_t uartTask_attributes = {
  .name = "uartTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal3,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartLedTask(void *argument);
void StartKeyTask(void *argument);
void StartUartTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  uartRxQueueHandle = osMessageQueueNew(64, sizeof(uint8_t), NULL);
   //创建一个消息队列，长度为64，元素大小为uint8_t
  if (uartRxQueueHandle == NULL)
  {
      Error_Handler();
  }

  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of ledTask */
  ledTaskHandle = osThreadNew(StartLedTask, NULL, &ledTask_attributes);

  /* creation of keyTask */
  keyTaskHandle = osThreadNew(StartKeyTask, NULL, &keyTask_attributes);

  /* creation of uartTask */
  uartTaskHandle = osThreadNew(StartUartTask, NULL, &uartTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartLedTask */
/**
  * @brief  Function implementing the ledTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartLedTask */
void StartLedTask(void *argument)
{
  /* USER CODE BEGIN StartLedTask */
  
  const uint16_t flowPins[6]= {GPIO_PIN_0, 
                              GPIO_PIN_1, 
                              GPIO_PIN_3, 
                              GPIO_PIN_4,
                              GPIO_PIN_3,
                              GPIO_PIN_1};
  const uint16_t allLeds =
        GPIO_PIN_0 | GPIO_PIN_1 |
        GPIO_PIN_3 | GPIO_PIN_4;

  uint8_t i = 0;
  /* Infinite loop */
  for(;;)
  {
    if(g_ledMode == 1)
    {
      HAL_GPIO_WritePin(GPIOA, allLeds, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOA, flowPins[i], GPIO_PIN_RESET);
      i=(i+1)%6;
      osDelay(300);
    }
    else
    {
      HAL_GPIO_WritePin(GPIOA, allLeds, GPIO_PIN_SET);
      osDelay(1000);
      HAL_GPIO_WritePin(GPIOA, allLeds, GPIO_PIN_RESET);
      osDelay(1000);
    }
  
    
  }  
  /* USER CODE END StartLedTask */
}

/* USER CODE BEGIN Header_StartKeyTask */
/**
* @brief Function implementing the keyTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartKeyTask */
void StartKeyTask(void *argument)
{
  /* USER CODE BEGIN StartKeyTask */
  
  /* Infinite loop */
  for (;;)
  {
      if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_RESET)
      {
          // 2. 软件消抖：等 20ms
          osDelay(20);
          
          // 3. 再次确认是不是真的按下了
          if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_RESET)
          {            
              g_ledMode = 1 - g_ledMode;
              
          }
      }   
      while(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_RESET)
      {
        osDelay(10);// 
      }
      osDelay(10);
  }
  /* USER CODE END StartKeyTask */
}

/* USER CODE BEGIN Header_StartUartTask */
/**
* @brief Function implementing the uartTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUartTask */
void StartUartTask(void *argument)
{
  /* USER CODE BEGIN StartUartTask */
  uint8_t rx;              // 临时工：用来接从队列里拿出来的一瞬间的那个字节
  char command[20];        // 组装台：把接收到的散字符拼成完整的指令
  uint8_t index = 0;       // 进度条：记录组装台里放了多少个字符了
  uint8_t overflow = 0;    // 报警器：记录是不是有捣乱的超长指令
  // 启动第一次串口接收
  if (HAL_UART_Receive_IT(
          &huart1, &uartRxByte, 1) != HAL_OK)
  {
      Error_Handler();
  } 
  /* Infinite loop */
  for (;;)
  {
      // 没有数据时，任务阻塞等待
      if (osMessageQueueGet(
              uartRxQueueHandle,
              &rx,
              NULL,
              osWaitForever) == osOK)
      {
          if (rx == '\r' || rx == '\n')
          {
              if (!overflow && index > 0)
              {
                  command[index] = '\0';

                  if (strcmp(command, "LED_FLOW") == 0)
                  {
                      g_ledMode = 1;
                  }
                  else if (strcmp(command, "LED_Blink") == 0)
                  {
                      g_ledMode = 0;
                  }
              }

              index = 0;
              overflow = 0;
          }
          else if (!overflow)
          {
              if (index < sizeof(command) - 1)
              {
                  command[index] = (char)rx;
                  index++;
              }
              else
              {
                  // 超长命令丢弃，直到下一个换行
                  overflow = 1;
              }
          }
      }
  }
  /* USER CODE END StartUartTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)// 判断是哪个串口触发的回调
    {
        // 把收到的字符送入队列
        osMessageQueuePut(
            uartRxQueueHandle,//  队列句柄
            &uartRxByte,//软件接收缓冲区地址
            0,// 优先级
            0// 不等待
        );

        // 重新启动下一字节接收
        HAL_UART_Receive_IT(
            &huart1,
            &uartRxByte,
            1
        );
    }
}
/* USER CODE END Application */

