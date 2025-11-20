/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.c
  * @author  MCD Application Team
  * @brief   ThreadX applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
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
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "motor_driver.h"
#include "encoder_driver.h"
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
/* USER CODE BEGIN PV */
uint8_t tracex_buffer[TRACEX_BUFFER_SIZE];


TX_THREAD pid_thread;

int32_t requested_position;
TX_MUTEX mutex_req_pos;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void pid_thread_entry(ULONG init);
/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  /* USER CODE BEGIN App_ThreadX_MEM_POOL */
  TX_BYTE_POOL *bytePool = (TX_BYTE_POOL *) memory_ptr;
  VOID *pointer;

  // stack allocation for PID thread
  ret = tx_byte_allocate(bytePool, &pointer, PID_THREAD_STACK_SIZE, TX_NO_WAIT);

  if (ret != TX_SUCCESS)
    return ret;

  ret = motor_driver_initialize();
  if(ret != TX_SUCCESS){
	  printf("[APP THREADX INIT] error in motor driver initialize: %u\n", ret);
  }

  ret = encoder_driver_initialize();
    if(ret != TX_SUCCESS){
  	  printf("[APP THREADX INIT] error in encoder driver initialize: %u\n", ret);
    }

  ret = tx_mutex_create(&mutex_req_pos, "mutex requested position", TX_INHERIT);
  if(ret != TX_SUCCESS){
	  printf("[APP THREADX INITI] error in mutex creation: %u\n", ret);
  }

  // PID thread create
    ret = tx_thread_create(&pid_thread, "PID thread", pid_thread_entry, 1234,
  	  pointer, PID_THREAD_STACK_SIZE, PID_THREAD_PRIORITY, PID_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

    if (ret != TX_SUCCESS)
      return ret;



  /* USER CODE END App_ThreadX_MEM_POOL */

  /* USER CODE BEGIN App_ThreadX_Init */
  tx_trace_enable(&tracex_buffer, TRACEX_BUFFER_SIZE, 30);
  /* USER CODE END App_ThreadX_Init */

  return ret;
}

  /**
  * @brief  Function that implements the kernel's initialization.
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN  Before_Kernel_Start */

  /* USER CODE END  Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN  Kernel_Start_Error */

  /* USER CODE END  Kernel_Start_Error */
}

/* USER CODE BEGIN 1 */

void pid_thread_entry(ULONG init)
{
	UINT ret;
	int32_t last_req_pos;
	int32_t req_pos;
	int32_t act_pos;
	float e_prev;
	float integral;

	while(1)
	{
		ret = tx_mutex_get(&mutex_req_pos, TX_NO_WAIT);
		if(ret == TX_SUCCESS){
			req_pos = requested_position;
			last_req_pos = req_pos;

			ret = tx_mutex_put(&mutex_req_pos);
			if(ret != TX_SUCCESS){
				printf("[PID] error in putting the mutex");
			}
		}
		else if(ret == TX_NOT_AVAILABLE){
			// use last_req_pos
			req_pos = last_req_pos;
		}
		else{
			printf("[PID] error in getting the mutex: %u\n", ret);
			continue;
		}

		ret = encoder_driver_input(&act_pos);
		if(ret != TX_SUCCESS){
			printf("[PID] error in encoder driver input: %u\n", ret);
			continue;
		}

		float e = (float) (req_pos - act_pos); //error
		float Pterm = PID_KP * e;
		printf("[PID] req_pos: %d\n act_pos: %d\n error: %f\n", req_pos, act_pos, e);
		integral += PID_KI * PID_SAMPLING_PERIOD_MS * e;

		float Dterm = 0.0f;
		if(PID_KD != 0.0f){
			 Dterm = PID_KD * (e - e_prev) / PID_SAMPLING_PERIOD_SEC;
		}

		float u = Pterm + integral + Dterm;

		e_prev = e;

		/*drive the motor*/
		int16_t duty = (int16_t) u;
		printf("[PID] duty: %d\n\n\n", duty);
		ret = motor_driver_set_duty(duty);
		if (ret != TX_SUCCESS)
		{
			printf("[PID] Error in motor_driver_set_duty: %u\n", ret);
		}


		tx_thread_sleep(PID_SAMPLING_PERIOD_TICKS);
	}
}

/* USER CODE END 1 */
