/*
 * pressure.c
 *
 *  Created on: Nov 17, 2024
 *      Author: micha
 */

#include "pressure.h"


#define LPS25HB_ADDR			0xB8

#define LPS25HB_WHO_AM_I 		0x0F
#define LPS25HB_CTRL_REG1 		0x20
#define LPS25HB_CTRL_REG2 		0x21
#define LPS25HB_CTRL_REG3 		0x22
#define LPS25HB_CTRL_REG4 		0x23
#define LPS25HB_PRESS_OUT_XL 	0x28
#define LPS25HB_PRESS_OUT_L 	0x29
#define LPS25HB_PRESS_OUT_H 	0x2A
#define LPS25HB_TEMP_OUT_L 		0x2B
#define LPS25HB_TEMP_OUT_H 		0x2C

#define LPS25HB_RPDS_L 			0x39
#define LPS25HB_RPDS_H 			0x3A

#define LPS25HB_CTRL_REG1_PD 	0x80
#define LPS25HB_CTRL_REG1_ODR2 	0x40
#define LPS25HB_CTRL_REG1_ODR1 	0x20
#define LPS25HB_CTRL_REG1_ODR0 	0x10

#define LPS25HB_IDENTIFIER 		0xBD

#define PRESSURE_SHIFT 			12
#define MAX_DIAGNOSTIC_COUNTER  5

volatile _Bool i2c_flag = false;

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef* hi2c){
	if(hi2c == &hi2c2){
		i2c_flag = true;
	}
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef* hi2c){
	if(hi2c == &hi2c2){
		i2c_flag = true;
	}
}

static void write_reg(uint8_t addr, uint8_t data){
	HAL_I2C_Mem_Write_IT(&hi2c2, LPS25HB_ADDR, addr, 1, &data, 1);
}

static void read_reg(pressure_handler_t* handler, uint8_t addr){
	HAL_I2C_Mem_Read_IT(&hi2c2, LPS25HB_ADDR, addr, 1, handler -> data_rx, 1);
	handler -> len = 1;
}

static void read_reg_pressure(pressure_handler_t* handler){
	HAL_I2C_Mem_Read_IT(&hi2c2, LPS25HB_ADDR, LPS25HB_PRESS_OUT_XL | 0x80, 1, handler -> data_rx, MAX_BUFFER_SIZE);
	handler -> len = MAX_BUFFER_SIZE;
}

static _Bool wait_for_read_reg_pressure(pressure_handler_t* handler,uint32_t* data){
	if(i2c_flag){

		i2c_flag = false;

		uint32_t buffer = 0;
		for(uint8_t n=0 ; n< handler ->len ; n++){
			buffer >>= 8;
			buffer |= (handler -> data_rx[n]) << 16;
		}

		*data = buffer;

		return true;
	}else{
		return false;
	}
}

static _Bool wait_for_read_reg(pressure_handler_t* handler,uint8_t* data){
	if(i2c_flag){

		i2c_flag = false;
		data[0] = handler -> data_rx[0];

		return true;
	}else{
		return false;
	}
}

static _Bool wait_for_write(){
	if(i2c_flag){

		i2c_flag = false;

		return true;
	}else{
		return false;
	}
}

static inline void init_pressure(pressure_handler_t* pressure_handler){

	if(pressure_handler -> checked && (!pressure_handler -> configured)){
		if(wait_for_write()){
			pressure_handler -> configured = true;
			pressure_handler -> diagnostic_counter = 0;
		}
	}

	if(pressure_handler -> started && (!pressure_handler -> checked)){

		uint8_t value;

		if(wait_for_read_reg(pressure_handler, &value)){

			if(value == LPS25HB_IDENTIFIER){
				pressure_handler -> error = false;
				pressure_handler -> checked = true;


				write_reg(LPS25HB_CTRL_REG1, LPS25HB_CTRL_REG1_PD | LPS25HB_CTRL_REG1_ODR2);
			}else{
				pressure_handler -> error = true;
				pressure_handler -> configured = true;
			}
		}
	}

	if(!pressure_handler -> started){
		read_reg(pressure_handler ,LPS25HB_WHO_AM_I);
		pressure_handler -> started = true;
		pressure_handler -> error = false;
	}
}

void pressure_handler(pressure_handler_t* pressure_handler){

	static uint16_t period_counter = 0;
	static _Bool read = false;

	if(pressure_handler -> configured){

		if(!pressure_handler ->error){

			if(pressure_handler -> reading){

				 period_counter++;

				 uint32_t data;
				 uint8_t sdata;
				 if(pressure_handler -> diagnostic_counter == MAX_DIAGNOSTIC_COUNTER - 1){

					 if(wait_for_read_reg(pressure_handler, &sdata)){

						 if(sdata != LPS25HB_IDENTIFIER){
							 pressure_handler ->error = true;
						 }

						 pressure_handler -> reading = false;
						 pressure_handler -> diagnostic_counter = 0;
					 }

					 if(period_counter >= pressure_handler -> period){
						 pressure_handler ->error = true; // timeout;
					 }

				 }else{
					 if(wait_for_read_reg_pressure(pressure_handler, &data)){

						 pressure_handler -> flag = true;
						 pressure_handler -> pressure = data >> PRESSURE_SHIFT;
						 read = true;
					 }

					 if(period_counter >= pressure_handler -> period){
						 pressure_handler -> diagnostic_counter ++;
						 pressure_handler -> reading = false;

						 if(!read){
							 pressure_handler ->error = true; // timeout;
						 }
					 }
				 }



			}else{
				read = false;

				if(pressure_handler -> diagnostic_counter == 0){
					period_counter ++;
				}else{
					period_counter = 1;
				}

				if(pressure_handler -> diagnostic_counter == MAX_DIAGNOSTIC_COUNTER - 1){
					read_reg(pressure_handler ,LPS25HB_WHO_AM_I);
				}else{
					read_reg_pressure(pressure_handler);
				}

				pressure_handler -> reading = true;
			}
		}

	}else{
		init_pressure(pressure_handler);


	}
}
