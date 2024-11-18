/*
 * pressure.h
 *
 *  Created on: Nov 17, 2024
 *      Author: micha
 */


#include "stdbool.h"
#include "stdint.h"
#include "i2c.h"

#ifndef INC_PRESSURE_H_
#define INC_PRESSURE_H_

#define MAX_BUFFER_SIZE			3

typedef struct pressure_handler{
	uint8_t data_rx[MAX_BUFFER_SIZE];
	uint8_t len;

	_Bool started;
	_Bool checked;
	_Bool configured;
	_Bool error;
	_Bool reading;

	uint8_t diagnostic_counter;
	uint16_t period;

	uint16_t pressure;
	_Bool flag;
}pressure_handler_t;

void pressure_handler(pressure_handler_t* pressure_handler);


#endif /* INC_PRESSURE_H_ */
