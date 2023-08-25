/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
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
#include "stm32f4xx_hal_can.h"
#include "stm32f4xx_hal_conf.h"
#include <stdlib.h>

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
CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;


/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_CAN2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

CAN_TxHeaderTypeDef TxHeader;
CAN_RxHeaderTypeDef RxHeader;

uint32_t TxMailbox;

uint8_t TxData[8];
uint8_t RxData[8];
uint8_t Key[4];

int isBlock = 1;

uint32_t TxMailbox;
int datacheck = 0;

void Setup_TxHeaderECU()
{
	TxHeader.DLC = 8;
	TxHeader.StdId = 0x104;
	//Setting UDS environment
	TxData[0] = 0x06; //length of data
	TxData[1] = 0x67; // service
	TxData[2] = 0x01; //sub-service

}

void Init_Tester()
{

   TxHeader.DLC = 8;
   TxHeader.ExtId = 0x11111111;
   TxHeader.IDE = CAN_ID_EXT;
   TxHeader.RTR = CAN_RTR_DATA;
   TxHeader.StdId = 46;
   TxHeader.TransmitGlobalTime = DISABLE;

   TxData[0] = 0x27;
   TxData[1] = 0x01;
   for(int i =2 ; i<8;i++)
   {
	   TxData[i] = 0x55;
   }
}


void Init_ECU()
{

   TxHeader.DLC = 8;
   TxHeader.ExtId = 0x11111111;
   TxHeader.IDE = CAN_ID_EXT;
   TxHeader.RTR = CAN_RTR_DATA;
   TxHeader.StdId = 0x104;
   TxHeader.TransmitGlobalTime = DISABLE;

   TxData[0] = 0x06;
   TxData[1] = 0x67;
   TxData[2] = 0x01;
//   for(int i =3 ; i<8;i++)
//   {
//	   TxData[i] = 0x55;
//   }
}

//uint8_t * Calculator_Key(uint8_t *key)
//{
//	uint8_t temp_key[4];
//	for(int i= 0 ; i < 4 ;i++)
//	{
//		temp_key[i] = *(key+i);
//	}
//	return temp_key;
//}
void Compare_key()
{
	for(int i = 3 ; i< 7 ;i++)
	{
		if(RxData[i] != Key[i -3])
		{
			isBlock = 2;
		}
	}
	if(isBlock != 2)
	{
		isBlock = 0;
		TxData[0] = 0x02;
		TxData[1] = 0x67;
		TxData[2] = 0x02;
		for(int i= 3 ; i < 8 ;i++)
		{
			TxData[i] = 0x55;

		}
		HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_SET);
		HAL_CAN_AddTxMessage(&hcan2, &TxHeader, TxData, &TxMailbox);
	}
}

void Calculator_Key()
{
	for(int i= 3 ; i < 7 ;i++)
	{
		TxData[i] = RxData[i] + 1;

	}
}

void Generate_Seed()
{

	for(int i = 3 ;i < 7; i++)
	{
		TxData[i] = rand()% 0xff;
		Key[i] = TxData[i] + 1;
	}
	TxData[7] = 0x55;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == GPIO_PIN_13){
//		HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_SET);


		Init_Tester();
		// Send request seed to ECU
		if(HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox) != HAL_OK)
		  {

		  	  	HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET);

		  };
	}
}


void Processing_ECU()
{
	Init_ECU();
	//Check request service
	if(RxData[0] == 0x27)
	{
		//check seed request
		if(RxData[1] == 0x01)
		{
			//Creat seed
			Generate_Seed();
			//send Seed to Tester
			HAL_CAN_AddTxMessage(&hcan2, &TxHeader, TxData, &TxMailbox);
		}
		//check key request
		if(RxData[1] == 0x02)
		{
			Compare_key();
		}
	}

}


void Processing_Tester()
{
	Init_Tester();
	//check responsive
	if(RxData[1] == 0x67)
	{
		//response seed
		if(RxData[2] == 0x01)
		{
			//chance service
			TxData[2] = 0x02;
			Calculator_Key();
			//send key request to ECU
			HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
		}
	}
}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	//check recive
	HAL_GPIO_WritePin(GPIOB, LED3_Pin, GPIO_PIN_SET);
	//ECU get Message
	if(HAL_CAN_GetRxMessage(&hcan2, CAN_RX_FIFO0 , &RxHeader, RxData) == HAL_OK)
		{
			HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_RESET);
			Processing_ECU();
		};
	//Tester get Message
	if(HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0 , &RxHeader, RxData) == HAL_OK)
		{
			HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_RESET);
			Processing_Tester();
		};
//	if(hcan->Instance == CAN2)
//	{
//		datacheck = 1;
////		Setup_TxHeaderECU();
////		Generate_Seed();
//		HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET);
//
//		Processing_ECU();
////		HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
//		HAL_Delay(500);
//		HAL_CAN_AddTxMessage(&hcan2, &TxHeader, TxData, &TxMailbox);
//
//	}
//
//	if(hcan->Instance == CAN1)
//	{
//		HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
////		Setup_TxHeader();
//		Processing_Tester();
//
//
//	}
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
  MX_CAN1_Init();
  MX_CAN2_Init();
  /* USER CODE BEGIN 2 */
  HAL_CAN_Start(&hcan1);
  HAL_CAN_Start(&hcan2);
//  HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET);
//  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
//  {
//  /* Notification Error */
////	  HAL_GPIO_TogglePin(GPIOB, LED1_Pin);
//	  HAL_GPIO_WritePin(GPIOB, LED1_Pin, 1);
//  }

  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
   {
   /* Notification Error */
 //	  HAL_GPIO_TogglePin(GPIOB, LED1_Pin);
 	  HAL_GPIO_WritePin(GPIOB, LED1_Pin, 1);
 	  HAL_GPIO_WritePin(GPIOB, LED2_Pin, 1);
   }

  HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);


//  HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_SET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (datacheck == 1)
	  {
		  for(int i =0 ; i < RxData[1] ;i++)
		  {
			  HAL_GPIO_TogglePin(GPIOB, LED1_Pin);
			  HAL_Delay(RxData[0]);
		  }
	  }

	  HAL_Delay(250);
//	  if(HAL_CAN_AddTxMessage(&hcan2, &TxHeader, TxData, &TxMailbox) != HAL_OK)
//	 		  {
//
//	 		  	  	HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_SET);
//	 		  };
	  HAL_Delay(250);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 10;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_2TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

    CAN_FilterTypeDef canfilterconfig;
    canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
    canfilterconfig.FilterBank = 2;
    canfilterconfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    canfilterconfig.FilterIdHigh = 0;
    canfilterconfig.FilterIdLow = 0x0000;
    canfilterconfig.FilterMaskIdHigh = 0;
    canfilterconfig.FilterMaskIdLow = 0x0000;
    canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
    canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;
    canfilterconfig.SlaveStartFilterBank = 14;

    HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);


  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief CAN2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN2_Init(void)
{

  /* USER CODE BEGIN CAN2_Init 0 */

  /* USER CODE END CAN2_Init 0 */

  /* USER CODE BEGIN CAN2_Init 1 */

  /* USER CODE END CAN2_Init 1 */
  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 10;
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan2.Init.TimeSeg1 = CAN_BS1_2TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  hcan2.Init.AutoBusOff = DISABLE;
  hcan2.Init.AutoWakeUp = DISABLE;
  hcan2.Init.AutoRetransmission = DISABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN2_Init 2 */
  CAN_FilterTypeDef canfilterconfig1;
  canfilterconfig1.FilterActivation = CAN_FILTER_ENABLE;
  canfilterconfig1.FilterBank = 14;
  canfilterconfig1.FilterFIFOAssignment = CAN_RX_FIFO0;
  canfilterconfig1.FilterIdHigh = 0;
  canfilterconfig1.FilterIdLow = 0x0000;
  canfilterconfig1.FilterMaskIdHigh = 0;
  canfilterconfig1.FilterMaskIdLow = 0x0000;
  canfilterconfig1.FilterMode = CAN_FILTERMODE_IDMASK;
  canfilterconfig1.FilterScale = CAN_FILTERSCALE_32BIT;
  canfilterconfig1.SlaveStartFilterBank = 14;

  HAL_CAN_ConfigFilter(&hcan2, &canfilterconfig1);
  /* USER CODE END CAN2_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED1_Pin|LED2_Pin|LED3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : Button1_Pin */
  GPIO_InitStruct.Pin = Button1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Button1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED1_Pin LED2_Pin LED3_Pin */
  GPIO_InitStruct.Pin = LED1_Pin|LED2_Pin|LED3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
