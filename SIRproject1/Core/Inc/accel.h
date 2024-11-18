/*
 * accel.h
 *
 *  Created on: Nov 17, 2024
 *      Author: micha
 */

#ifndef INC_ACCEL_H_
#define INC_ACCEL_H_

#define ACCEL_MINIMUM 						4
#define ACCEL_MAXIMUM	 					64
#define ACCEL_SHIFT 						2

#define ACCEL_MINIMUM_LED 					0
#define ACCEL_AMOUNT_OF_AXIS	 			3

#define ACCEL_DIAGNOSTIC_MAX_COUNTER 		20
typedef struct accel_type_def{
	_Bool initized;
	_Bool started;
	_Bool checked;
	_Bool error;

	_Bool reading;

	uint8_t axis_index ;
	uint8_t diagnostic_index;

	uint8_t axis_values[3];
	uint32_t period;

	_Bool flag;
}accel_type_def_t;


void accel_handler(accel_type_def_t* accel);

#endif /* INC_ACCEL_H_ */
