  /******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  * (c) EE2028 Teaching Team
  ******************************************************************************/


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01_accelero.h"
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01_tsensor.h"
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01_magneto.h"
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01_psensor.h"
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01_hsensor.h"
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01_gyro.h"
#include "../../Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01.h"

#include "stdio.h"
#include "stdarg.h"

#define fluxer_firing_rate (int)5000
#define imu_freq (int)40
#define tnh_freq (int) 1
#define pressure_freq (int) 20
#define sensor_send_freq (int) 1

extern void initialise_monitor_handles(void);	// for semi-hosting support (printf)
static void MX_GPIO_Init(void);
int check_button(void);
void SystemClock_Config(void);
void change_mode(void);
void led_handler(uint8_t mode, uint8_t warning);
void uart_print(const char* format, ...);

uint32_t t1 = 0, t2 = 0;
uint32_t led_tick  = 0;

UART_HandleTypeDef huart1;


HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == BUTTON_EXTI13_Pin)
	{
		t1 = t2;
		t2 = HAL_GetTick();
	}
}

int main(void)
{
	initialise_monitor_handles(); // for semi-hosting support (printf)

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();
	MX_GPIO_Init();

	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

	BSP_COM_Init(COM1, &huart1);

	/* Peripheral initializations using BSP functions */
	BSP_ACCELERO_Init(); //ODR: 52Hz
	BSP_GYRO_Init(); //ODR: 52Hz
	BSP_MAGNETO_Init(); //ODR: 40Hz
	BSP_TSENSOR_Init(); //ODR: 1Hz
	BSP_HSENSOR_Init(); //ODR: 1Hz
	BSP_PSENSOR_Init(); //ODR: 25Hz

	uint32_t imu_tick = HAL_GetTick();
	uint32_t tnh_tick = HAL_GetTick();
	uint32_t pressure_tick = HAL_GetTick();
	uint32_t fluxer_tick = HAL_GetTick();
	uint32_t sensor_send_tick = HAL_GetTick();

	float temp_data, humidity_data, pressure_data;
	float accel_data[3], mag_data[3], gyro_data[3]; // [x, y, z]

	uint8_t mode = 0; //0: exploration, 1: battle
	uint8_t warning = 0; //0: no warning, 1: warning
	uint8_t charge_cnt = 8;

	while (1)
	{
		switch (check_button())
		{
		case 1:
			uart_print("change mode \n");
			if (!warning) mode ^= 1;
			break;
		case 2:
			uart_print("clear warning/charging \n");
			if (!warning && charge_cnt < 10) charge_cnt += 1;
			else if(warning) warning = 0;
			break;
		default:
			break;
		}

		led_handler(mode, warning);

		if (HAL_GetTick() - imu_tick >= 1000/imu_freq){
			int16_t mag_data_i16[3] = { 0 };
			BSP_MAGNETO_GetXYZ(mag_data_i16);
			mag_data[0] = (float)mag_data_i16[0]/6842;
			mag_data[1] = (float)mag_data_i16[1]/6842;
			mag_data[2] = (float)mag_data_i16[2]/6842;

			int16_t accel_data_i16[3] = { 0 };			// array to store the x, y and z readings.
			BSP_ACCELERO_AccGetXYZ(accel_data_i16);		// read accelerometer
			// the function above returns 16 bit integers which are 100 * acceleration_in_m/s2. Converting to float to print the actual acceleration.
			accel_data[0] = (float)accel_data_i16[0] / 100.0f;
			accel_data[1] = (float)accel_data_i16[1] / 100.0f;
			accel_data[2] = (float)accel_data_i16[2] / 100.0f;

			BSP_GYRO_GetXYZ(gyro_data);
		}

		if (HAL_GetTick() - tnh_tick >= 1000/tnh_freq){
			temp_data = BSP_TSENSOR_ReadTemp();			// read temperature sensor
			humidity_data = BSP_HSENSOR_ReadHumidity();
			tnh_tick = HAL_GetTick();
		}

		if (HAL_GetTick() - pressure_tick >= 1000/pressure_freq){
			pressure_data = BSP_PSENSOR_ReadPressure();
			pressure_tick = HAL_GetTick();
		}

		if (HAL_GetTick() - sensor_send_tick >= 1000/sensor_send_freq){
			uart_print("T:%f C, P:%f hPa, H:%f \%rH, A:%f m/s2, G:%f dps, M:%f gauss\r\n", temp_data, pressure_data, humidity_data, accel_data[2], gyro_data[0], mag_data[0]);
		}
	}

}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin LED2_Pin */
  GPIO_InitStruct.Pin = LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BUTTON_EXTI13_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	// Enable NVIC EXTI line 13
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

int check_button(){
	if (t2 == 0) return 0;
	else if(HAL_GetTick() - t2 > 1000){
		t1 = 0;
		t2 = 0;
		return 2;
	}
	else if(t2 - t1 <= 1000 && t1 != 0){
		t1 = 0;
		t2 = 0;
		return 1;
	}
	else{
		return 0;
	}
}

void led_handler(uint8_t mode, uint8_t warning){
	static int interval;
	uint32_t led_status = (GPIOB->ODR & GPIO_PIN_14);
	if (warning == 1){
		interval = 333;
	}
	else if(mode == 1){
		interval = 1000;
	}
	else{
		GPIOB->BSRR = (uint32_t)GPIO_PIN_14;
		return;
	}
	//led will be on for 50ms every blink
	if (HAL_GetTick() - led_tick > 100 && led_status != 0x00u)
		GPIOB->BRR = (uint32_t)GPIO_PIN_14;
	else if (HAL_GetTick() - led_tick > interval){
		GPIOB->BSRR = (uint32_t)GPIO_PIN_14;
		led_tick = HAL_GetTick();
	}
	return;
}

void uart_print(const char* format, ...){
	char msg_print[256];
	va_list args;
	va_start(args, format);
	vsprintf(msg_print, format, args);
	va_end(args);
	HAL_UART_Transmit(&huart1, (uint8_t*)msg_print, strlen(msg_print),0xFFFF); //Sending in normal mode

}
