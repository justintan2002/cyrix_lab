  /******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  * (c) EE2028 Teaching Team
  *
  * Written by: Tan Chern Lin Justin A0240108W
  * Written by: Teoh Xu En 			 A0239559L
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
#include "ht16k33.h"

#include "stdio.h"
#include "stdarg.h"
#include "math.h"
#include "string.h"

#define fluxer_firing_rate (int)1 //max 5000
#define imu_freq (int)1 // max 40
#define tnh_freq (int) 1 // max 1
#define pressure_freq (int) 1 // max 20
#define sensor_send_freq (int) 1 // max 1

extern void initialise_monitor_handles(void);	// for semi-hosting support (printf)
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void UART_Init(void);


int check_button(void);
void SystemClock_Config(void);
void change_mode(void);
void led_handler(uint8_t mode, uint8_t warning);
void uart_print(const char* format, ...);

I2C_HandleTypeDef hi2c1;
uint8_t violations_count(uint8_t arr[], uint8_t *warning, uint8_t mode);
uint8_t read_cnt = 0;

uint32_t t1 = 0, t2 = 0;
uint32_t led_tick  = 0;

char uart_buffer[1024];

UART_HandleTypeDef huart1;
uint8_t nfcCount = 0;

extern void NFC_IO_Init(uint8_t GpoIrqEnable);

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == BUTTON_EXTI13_Pin)
	{
		t1 = t2;
		t2 = HAL_GetTick();
	}

    if(GPIO_Pin == GPIO_PIN_4 ){
    	nfcCount += 1;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart == &huart1){
		read_cnt+=1;
	}
}

int main(void)
{
	uint8_t accel_init = 1;
	uint8_t gyro_init = 1;
	uint8_t magneto_init = 1;
	uint8_t temp_init = 1;
	uint8_t humid_init = 1;
	uint8_t pressure_init = 1;

	initialise_monitor_handles(); // for semi-hosting support (printf)

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();
	MX_GPIO_Init();
	NFC_IO_Init(1);
	UART_Init();

	/* Peripheral initializations using BSP functions */
	accel_init = BSP_ACCELERO_Init(); //ODR: 52Hz
	gyro_init = BSP_GYRO_Init(); //ODR: 52Hz
	magneto_init = BSP_MAGNETO_Init(); //ODR: 40Hz
	temp_init = BSP_TSENSOR_Init(); //ODR: 1Hz
	humid_init = BSP_HSENSOR_Init(); //ODR: 1Hz
	pressure_init = BSP_PSENSOR_Init(); //ODR: 25Hz

	MX_I2C1_Init(); // Initialise I2C

	uint32_t imu_tick = HAL_GetTick();
	uint32_t tnh_tick = HAL_GetTick();
	uint32_t pressure_tick = HAL_GetTick();;
	uint32_t sensor_send_tick = HAL_GetTick();
	uint32_t fire_tick = HAL_GetTick();
	uint32_t light_tick;

	float temp_data=0, humidity_data=0, pressure_data=0;
	float accel_data[3]={1}, mag_data[3] = {0}, gyro_data[3] = {20}; // [x, y, z]
	float mag_magnitude=0;
	float gyro_offset[3] = {0.49, -3.15, 0.14};

	float accel_th[2] = {5.0, 15.0}; //upper and lower limit
	float gyro_th = 10; //>10 violate
	float mag_th = 0.6; //>0.6 violate
	float temp_th = 35; //>35 violate
	float pressure_th[2] = {900, 1200}; // upper and lower limit
	float humid_th = 87; //<87 violate

	uint8_t violations[6] = {0,0,0,0,0,0}; // accel, gyro, mag, temp, humidity, pressure
	uint8_t v_count = 0;
	uint8_t mode = 0; //0: exploration, 1: battle
	uint8_t warning = 0; //0: no warning, 1: warning
	uint8_t charge_cnt = 0; // number of battery
	uint8_t fired = 0; // counter for LED
	char str[5] = {'\0'}; //uart read buffer
	uint8_t auth_state = 0;

	while (1)
	{
		if(accel_init) accel_init = BSP_ACCELERO_Init();
		if(gyro_init) gyro_init = BSP_GYRO_Init();
		if(magneto_init) magneto_init = BSP_MAGNETO_Init();
		if(temp_init) temp_init = BSP_TSENSOR_Init();
		if(humid_init) humid_init = BSP_HSENSOR_Init();
		if(pressure_init) pressure_init = BSP_PSENSOR_Init();
		uart_receive(str);
		if (read_cnt > 0){
			if (str[0] == '0' && charge_cnt < 10){
				charge_cnt += 1;
				uart_print("Battery Charged! Current Battery Level %d/10\n",charge_cnt);
			}
			else if(str[0]=='1'){
				mode ^= 1;
				memset(violations, 0 ,6);
				uart_print((mode==0)?"Entering EXPLORATION mode\n":"Entering BATTLE mode\n");
			}
			else if(str[0] == '5'){
				auth_state = 1;
			}
			read_cnt -= 1;
		}
		if (!auth_state && nfcCount > 0){
			uart_print("Auth: 12345\n");
			nfcCount = 0;
			uart_print("Entering EXPLORATION mode\n"); //initial printing
		}
		if (!auth_state) continue;

		switch (check_button())
		{
		case 2:
			if (!warning){
				mode ^= 1;
				memset(violations, 0 ,6); // changing mode reset number of violations
				uart_print((mode==0)?"Entering EXPLORATION mode\n":"Entering BATTLE mode\n");
			}
			else {
				uart_print("In WARNING state, unable to change mode\n");
			}
			break;
		case 1:

			if (!warning && mode) {
				if (charge_cnt < 10){
					charge_cnt += 1;
					memset(violations, 0 ,6);
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

		// if NFC detected send log in message

		// poll accel, mag and gyro data if no warning
		if (HAL_GetTick() - imu_tick >= 1000/imu_freq && !warning){
			// get magnometer data
			int16_t mag_data_i16[3] = { 0 };
			BSP_MAGNETO_GetXYZ(mag_data_i16);
			mag_data[0] = (float)mag_data_i16[0]/6842;
			mag_data[1] = (float)mag_data_i16[1]/6842;
			mag_data[2] = (float)mag_data_i16[2]/6842;

			// calculate mag magnitude
			mag_magnitude = (float) sqrt(pow(mag_data[0], 2) + pow(mag_data[1], 2) + pow(mag_data[2], 2));

			// get gyro data
			BSP_GYRO_GetXYZ(gyro_data);
			for (int i=0; i<3; i++){
				gyro_data[i] /= 1000; //convert from mdps to dps
				gyro_data[i] -= gyro_offset[i];
			}

			// check if gyro data violated threshold
			if (gyro_data[2] > gyro_th) {
				violations[1] = 1;
				v_count = violations_count(violations, &warning, mode); //count number of violations and update mode accordingly
			}

			// check if magnetometer violated threshold
			if (mag_magnitude > mag_th){
				violations[2] = 1;
				v_count = violations_count(violations, &warning, mode); //count number of violations and update mode accordingly
			}

			// if battle mode
			if (mode){
				// read data from accelerometer
				int16_t accel_data_i16[3] = { 0 };			// array to store the x, y and z readings.
				BSP_ACCELERO_AccGetXYZ(accel_data_i16);		// read accelerometer
				// the function above returns 16 bit integers which are 100 * acceleration_in_m/s2. Converting to float to print the actual acceleration.
				accel_data[0] = (float)accel_data_i16[0] / 100.0f;
				accel_data[1] = (float)accel_data_i16[1] / 100.0f;
				accel_data[2] = (float)accel_data_i16[2] / 100.0f;

				// check if accel data violated threshold
				if (accel_data[2] < accel_th[0] || accel_data[2] > accel_th[1]){
					violations[0] = 1;
					v_count = violations_count(violations, &warning, mode); //count number of violations and update mode accordingly
				}
			}

		}

		// poll humidity and temp if no warning
		if (HAL_GetTick() - tnh_tick >= 1000/tnh_freq && !warning){
			// get humidity data
			humidity_data = BSP_HSENSOR_ReadHumidity();
			tnh_tick = HAL_GetTick();

			// check if humidity violated threshold
			if (humidity_data < humid_th){
				violations[4] = 1;
				v_count = violations_count(violations, &warning, mode); //count number of violations and update mode accordingly
			}

			// if battle mode
			if (mode) {
				// get temp data
				temp_data = BSP_TSENSOR_ReadTemp();

				// check if temp violate threshold
				if (temp_data > temp_th){
					violations[3] = 1;
					v_count = violations_count(violations, &warning, mode); //count number of violations and update mode accordingly
				}

			}
		}

		// poll pressure if no warning
		if (HAL_GetTick() - pressure_tick >= 1000/pressure_freq && !warning){
			pressure_data = BSP_PSENSOR_ReadPressure();
			pressure_tick = HAL_GetTick();

			// check if pressure violated threshold
			if (pressure_data < pressure_th[0] || pressure_data > pressure_th[1]){
				violations[5] = 1;
				v_count = violations_count(violations, &warning, mode); //count number of violations and update mode accordingly
			}
		}

		// Fire in battle mode
		if (mode && HAL_GetTick() - fire_tick >= 5000 && !warning) {
			if (charge_cnt < 2) uart_print("NO BATTERY TO FIRE, PLEASE RECHARGE\n");
			else {
				led_displayOn(); // turn on matrix
				light_tick = HAL_GetTick();
				fired += 1;
				charge_cnt -= 2;
				uart_print("Fluxer Fired! Current Battery Level: %d/10\n",charge_cnt);
			}
			fire_tick = HAL_GetTick();
		}

		// turn of led matrix to simulate blinking
		if (fired > 0 && HAL_GetTick() - light_tick > 300 && fired%2 == 1){
			led_displayOff();
			light_tick = HAL_GetTick();
			fired += 1;
			if (fired > 3) fired = 0; // stop blinking
		}

		// turn on led again to simulate blinking
		if (fired > 0 && HAL_GetTick() - light_tick > 300 && fired%2 == 0){
			led_displayOn();
			light_tick = HAL_GetTick();
			fired += 1;
		}

		// send telemetry if no warning
		if (HAL_GetTick() - sensor_send_tick >= 1000/sensor_send_freq && !warning){
			if (mode) { //print battle mode stuff
				uart_print("Info P: %.2f hPa, H: %.2f %%rH, G: %.2f dps, M: %.3f gauss T: %.2f C, A: %.2f m/s2\r\n",pressure_data, humidity_data, gyro_data[2], mag_magnitude, temp_data, accel_data[2]);
				uart_print("Battery: %d/10\n",charge_cnt); // if in Battle Mode, send battery level
			} else { // print exploration mode stuff
				uart_print("Info P: %.2f hPa, H: %.2f %%rH, G: %.2f dps, M: %.3f gauss\r\n", pressure_data, humidity_data, gyro_data[2], mag_magnitude);
			}
			sensor_send_tick = HAL_GetTick();
		}

		// send warning message
		if (HAL_GetTick() - sensor_send_tick >= 1000 && warning){
			uart_print(mode == 0?"WARNING mode: SOS\n":"BATTLE mode: SOS\n");
			sensor_send_tick = HAL_GetTick();
		}
	}

}

int check_button(){
	if (t2 == 0) return 0;
	else if(HAL_GetTick() - t2 > 1000){
		t1 = 0;
		t2 = 0;
		return 1;
	}
	else if(t2 - t1 <= 1000 && t1 != 0){
		t1 = 0;
		t2 = 0;
		return 2;
	}
	else{
		return 0;
	}
}

uint8_t violations_count(uint8_t arr[], uint8_t *warning, uint8_t mode){
	uint8_t v_count = 0;
	for (int i=0; i<6; i++){
		if (arr[i] == 1) v_count += 1;
	}
	if ((mode == 0 && v_count >= 2) || (mode==1 && v_count >= 1)){
		*warning = 1;
	}
	return v_count;
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
	char msg_print[256] = {'\0'};
	va_list args;
	va_start(args, format);
	vsnprintf(msg_print, 256, format, args);
	va_end(args);
	HAL_UART_Transmit(&huart1, (uint8_t*) msg_print, strlen(msg_print), 0xFFFF); //Sending in normal mode
}

uart_receive(char* str_uart){
	return HAL_UART_Receive_IT(&huart1, (uint8_t*) str_uart, 5);
}

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

static void UART_Init(void)
{
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
}

static void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10909CEC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
	  uart_print("Unable to init I2C\n");
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    uart_print("Unable to configure Analogue Filter\n");
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    uart_print("Unable to configure Digital Filter\n");
  }

}
