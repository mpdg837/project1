/*
 * display.c
 *
 *  Created on: Nov 17, 2024
 *      Author: micha
 */


#include "display.h"

#define ROUND_SIZE 			4
#define ROUND_SHIFT 		2

#define MAX_LEVEL_COLOR 	255
#define TEXT_BUFFER_SIZE 	32
volatile _Bool uart_flag = false;

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart){
	if(huart == &huart2){
		uart_flag = false;
	}
}

static _Bool rounding(PWM_leds_type_def_t* pwms, accel_type_def_t* accel,accel_axis_measure_t* measure){


	static uint32_t accel_archive[ACCEL_AMOUNT_OF_AXIS];
	static uint8_t counter = 0;

	if(accel -> flag){
		accel -> flag = 0;

		for(uint8_t n=0;n<ACCEL_AMOUNT_OF_AXIS;n++){

			int8_t value = (int8_t) accel -> axis_values[n];

			if(value < 0){
				value = -value;
			}

			accel_archive[n] += value;
		}

		counter ++;

		if(counter == ROUND_SIZE){
			counter = 0;

			for(uint8_t n=0;n<ACCEL_AMOUNT_OF_AXIS;n++){

				uint32_t value = accel_archive[n];
				accel_archive[n] = 0;

				value >>= ROUND_SHIFT;


				switch(n){
					case 0:
						measure -> x = value;
						break;
					case 1:
						measure -> y = value;
						break;
					case 2:
						measure -> z = value;
						break;
				}
			}

			return true;
		}

	}

	return false;
}

static inline void show_axis_levels(PWM_leds_type_def_t* pwms,accel_axis_measure_t* meas){


	for(uint8_t n=0;n<ACCEL_AMOUNT_OF_AXIS;n++){

		int8_t value = 0;
		switch(n){
			case 0:
				value = meas -> x;
				break;
			case 1:
				value = meas -> y;
				break;
			case 2:
				value = meas -> z;
				break;
		}

		if(value < ACCEL_MINIMUM){
			value = 0;
		}else{
			value -= ACCEL_MINIMUM;
			value += ACCEL_MINIMUM_LED;

			if(value > ACCEL_MAXIMUM - 1){
				value = ACCEL_MAXIMUM - 1;
			}

			value <<= ACCEL_SHIFT;
		}

		pwms -> leds[n + 1].green = value;
		pwms -> leds[n + 1].blue = value;
	}

}

static inline void display_status(PWM_leds_type_def_t* pwms,
		accel_type_def_t* accel,
		pressure_handler_t* pressure_handler)
{


	if(accel -> checked && pressure_handler ->configured){
		if(accel -> error || pressure_handler ->error){
			pwms -> leds[0].green = 0;
			pwms -> leds[0].red = MAX_LEVEL_COLOR;

			for(int n=0;n<3;n++){
				pwms -> leds[n+1].blue = 0;
				pwms -> leds[n+1].green = 0;
				pwms -> leds[n+1].red = 0;
			}
		}else{
			pwms -> leds[0].red = 0;
			pwms -> leds[0].green = MAX_LEVEL_COLOR;
		}
	}

	if(!accel -> started){

		pwms -> enable = true;
		pwms -> leds[0].red = 0;
		pwms -> leds[0].green = 0;
	}
}

void display(PWM_leds_type_def_t* pwms,
		accel_type_def_t* accel,
		pressure_handler_t* pressure_handler)
{

	static accel_axis_measure_t meas;
	static _Bool received = false;
	static uint8_t counter = 0;

	if(pressure_handler ->flag){
		if(!uart_flag){


			static uint8_t buffer[TEXT_BUFFER_SIZE];

			snprintf((char*)buffer,TEXT_BUFFER_SIZE,"p=%d hPa \r\n",pressure_handler ->pressure);
			HAL_UART_Transmit_IT(&huart2, buffer, TEXT_BUFFER_SIZE);

			pressure_handler ->flag = false;
			uart_flag = true;
		}
	}


	if(received){
		if(!uart_flag){
			static uint8_t buffer[TEXT_BUFFER_SIZE];

			uart_flag = true;

			snprintf((char*)buffer,TEXT_BUFFER_SIZE,"x=%d,y=%d,z=%d \r\n",meas.x,meas.y,meas.z);
			HAL_UART_Transmit_IT(&huart2, buffer, TEXT_BUFFER_SIZE);

			received = false;
		}
	}

	if(rounding(pwms, accel,&meas)){

		show_axis_levels(pwms, &meas);


		if(++counter == 4){
			counter = 0;
			received = true;
		}
	}

	display_status(pwms, accel,pressure_handler);

}
