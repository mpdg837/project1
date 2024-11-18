/*
 * display.h
 *
 *  Created on: Nov 17, 2024
 *      Author: micha
 */

#include "pwm_leds.h"
#include "accel.h"
#include "usart.h"
#include "pressure.h"

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_

typedef struct accel_axis_measure{
	uint8_t x;
	uint8_t y;
	uint8_t z;
}accel_axis_measure_t;

void display(PWM_leds_type_def_t* pwms, accel_type_def_t* accel,pressure_handler_t* pressure_handler);

#endif /* INC_DISPLAY_H_ */
