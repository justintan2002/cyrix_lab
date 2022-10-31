/*
 * sevensegment.c
 *
 *  Created on: 20.05.2022
 *      Author: lenni
 */

#include "ht16k33.h"

// Commands
#define HT16K33_ON              0x21  // 0=off 1=on start oscillation
#define HT16K33_STANDBY         0x20  // bit xxxxxxx0

// bit pattern 1000 0xxy
// y    =  display on / off
#define HT16K33_DISPLAYON       0x81
#define HT16K33_DISPLAYOFF      0x80

// bit pattern 1110 xxxx
// xxxx    =  0000 .. 1111 (0 - F)
#define HT16K33_BRIGHTNESS      0x0F

volatile uint8_t _bright = 15;			//current brightness (0-15)
// sends given command per i2c
void _writeCmd(uint8_t cmd) {
	HAL_I2C_Master_Transmit(&HT16K33_I2C_PORT, HT16K33_I2C_ADDR << 1, &cmd, 1, HAL_MAX_DELAY);
}


//##### END: I2C-WRITE-FUNCTIONS #####
//####################################
//##### BEGIN: CONTROL-FUNCTIONS #####

void led_displayOn() {
	_writeCmd(HT16K33_ON);
	_writeCmd(HT16K33_DISPLAYON);
}

void led_displayOff() {
	_writeCmd(HT16K33_DISPLAYOFF);
	_writeCmd(HT16K33_STANDBY);
}

void led_setBrightness(uint8_t value) {
	if (value == _bright)
		return;

	_bright = value;

	if (_bright > 0x0F)
		_bright = 0x0F;

	_writeCmd(HT16K33_BRIGHTNESS | _bright);
}
