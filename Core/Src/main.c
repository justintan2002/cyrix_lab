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
#include "math.h"

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
	uint32_t fire_tick = HAL_GetTick();

	float temp_data, humidity_data, pressure_data;
	float accel_data[3], mag_data[3], gyro_data[3]; // [x, y, z]
	float mag_magnitude;

	float accel_th[2] = {5.0, 15.0}; //upper and lower limit
	float gyro_th = 10; //>10 violate
	float mag_th = 0.6; //>0.6 violate
	float temp_th = 35; //>35 violate
	float pressure_th[2] = {900, 1200}; // upper and lower limit
	float humid_th = 90; //<90 violate

	uint8_t violations[6] = {0,0,0,0,0,0}; // accel, gyro, mag, temp, humidity, pressure
	uint8_t v_count = 0;

	uint8_t mode = 0; //0: exploration, 1: battle
	uint8_t warning = 0; //0: no warning, 1: warning
	uint8_t charge_cnt = 0;

	uart_print("Entering EXPLORATION mode\n"); //initial printing

	while (1)
	{
		switch (check_button())
		{
		case 1:
			if (!warning){
				mode ^= 1;
				memset(violations, 0 ,6); // changing mode reset number of violations
				uart_print((mode==0)?"Entering EXPLORATION mode\n":"Entering BATTLE mode\n");
			}
			else {
				uart_print("In WARNING state, unable to change mode\n");
			}
			break;
		case 2:

			if (!warning && mode) {
				if (charge_cnt < 10){
					charge_cnt += 1;
					uart_print("Battery Charged! Current Battery Level %d/10\n",charge_cnt);
				}
				else uart_print("Battery Full!\n");

			}
			else if(warning){
				warning = 0;
				memset(violations, 0 ,6); // clear warning reset number of violations
				uart_print("Warning Cleared!\n");
			}
			break;
		default:
			break;
		}

		led_handler(mode, warning);


		// poll accel, mag and gyro data if no warning
		if (HAL_GetTick() - imu_tick >= 1000/imu_freq && !warning){
			int16_t mag_data_i16[3] = { 0 };
			BSP_MAGNETO_GetXYZ(mag_data_i16);
			mag_data[0] = (float)mag_data_i16[0]/6842;
			mag_data[1] = (float)mag_data_i16[1]/6842;
			mag_data[2] = (float)mag_data_i16[2]/6842;

			mag_magnitude = (float) sqrt(pow(mag_data[0], 2) + pow(mag_data[1], 2) + pow(mag_data[2], 2));

			int16_t accel_data_i16[3] = { 0 };			// array to store the x, y and z readings.
			BSP_ACCELERO_AccGetXYZ(accel_data_i16);		// read accelerometer
			// the function above returns 16 bit integers which are 100 * acceleration_in_m/s2. Converting to float to print the actual acceleration.
			accel_data[0] = (float)accel_data_i16[0] / 100.0f;
			accel_data[1] = (float)accel_data_i16[1] / 100.0f;
			accel_data[2] = (float)accel_data_i16[2] / 100.0f;

			BSP_GYRO_GetXYZ(gyro_data);
			for (int i=0; i<3; i++)
				gyro_data[i] /= 1000; //convert from mdps to dps

			// check if accel data violated threshold
			if (accel_data[2] < accel_th[0] || accel_data[2] > accel_th[1]){
				violations[0] = 1;
				uart_print("A:%.2f m/s2 violated threshold\n", accel_data[2]);
			}

			// check if gyro data violated threshold
			if (gyro_data[2] > gyro_th) {
				violations[1] = 1;
				uart_print("G:%.2f dps violated threshold\n", gyro_data[2]);
			}

			// check if magnetometer violated threshold
			if (mag_magnitude > mag_th){
				violations[2] = 1;
				uart_print("M:%.3f gauss violated threshold\n", mag_magnitude);
			}

		}

		// poll humidity and temp if no warning
		if (HAL_GetTick() - tnh_tick >= 1000/tnh_freq && !warning){
			temp_data = BSP_TSENSOR_ReadTemp();			// read temperature sensor
			humidity_data = BSP_HSENSOR_ReadHumidity();
			tnh_tick = HAL_GetTick();

			// check if temp violate threshold
			if (temp_data > temp_th){
				violations[3] = 1;
				uart_print("T:%.2f C violated threshold\n", temp_data);
			}

			// check if humidity violated threshold
			if (humidity_data < humid_th){
				violations[4] = 1;
				uart_print("H:%.2f %%rH violated threshold\n", humidity_data);
			}
		}

		// poll pressure if no warning
		if (HAL_GetTick() - pressure_tick >= 1000/pressure_freq && !warning){
			pressure_data = BSP_PSENSOR_ReadPressure();
			pressure_tick = HAL_GetTick();

			// check if pressure violated threshold
			if (pressure_data < pressure_th[0] || pressure_data > pressure_th[1]){
				violations[5] = 1;
				uart_print("P:%.2f hPa violated threshold. Number of Violations: %d\n", pressure_data, violations);
			}
		}

		// count number of sensors violated
		v_count = 0;
		for (int i=0; i<6; i++){
			if (violations[i] == 1) v_count += 1;
		}

		// enter warning mode when sensor threshold exceeded
		if ((mode == 0 && v_count >= 2) || (mode==1 && v_count >= 1)){
			warning = 1;
		}

		// Battle mode will still fire in SOS state
		if (mode && HAL_GetTick() - fire_tick >= 5000) {
			if (charge_cnt < 2) uart_print("NO BATTERY TO FIRE, PLEASE RECHARGE\n");
			else {
				charge_cnt -= 2;
				uart_print("Fluxer Fired! Current Battery Level: %d/10\n",charge_cnt);
			}
			fire_tick = HAL_GetTick();
		}

		// send telemetry if no warning
		if (HAL_GetTick() - sensor_send_tick >= 1000/sensor_send_freq && !warning){
			uart_print("T:%.2f C, P:%.2f hPa, H:%.2f %%rH, A:%.2f m/s2, G:%.2f dps, M:%.3f gauss\r\n", temp_data, pressure_data, humidity_data, accel_data[2], gyro_data[2], mag_magnitude);
			if (mode) uart_print("Battery: %d/10\n",charge_cnt); // if in Battle Mode, send battery level
			sensor_send_tick = HAL_GetTick();
		}

		// send warning message
		if (HAL_GetTick() - sensor_send_tick >= 1000 && warning){
			uart_print(mode == 0?"WARNING mode: SOS\n":"BATTLE mode: SOS\n");
			sensor_send_tick = HAL_GetTick();
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
