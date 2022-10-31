/*
 * ht16k33.h
 *
 *  Created on: May 18, 2022
 *  Author: Lennart Koepper
 */

#ifndef INC_HT16K33_H_
#define INC_HT16K33_H_

#include "stdint.h"

// !!! Adjust the following include according to your stm32 !!!
#include "../../Drivers/STM32L4xx_HAL_Driver/Inc/stm32l4xx_hal.h"

extern I2C_HandleTypeDef hi2c1;

// !!! I2C Configuration !!!
#define HT16K33_I2C_PORT        hi2c1
#define HT16K33_I2C_ADDR        0x70

void led_displayOn(void);	// enable display
void led_displayOff(void);	// disable display, fastest way to darken display

void led_setBrightness(uint8_t value);		// 0 .. 15	  0 = off, 15 = max. brightness
#endif /* INC_HT16K33_H_ */
