/*
 * pwm_leds.h
 *
 *  Created on: Nov 16, 2024
 *      Author: micha
 */
#include <inttypes.h>
#include <stdbool.h>

#ifndef INC_PWM_LEDS_H_
#define INC_PWM_LEDS_H_

typedef struct PWM_led_color{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
}PWM_led_color_t;

typedef struct PWM_leds_type_def{
	_Bool enable;
	PWM_led_color_t leds[4];
}PWM_leds_type_def_t;


void pwm_leds_it_handler(const PWM_leds_type_def_t* pwm);

#endif /* INC_PWM_LEDS_H_ */
