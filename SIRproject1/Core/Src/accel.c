/*
 * accel.c
 *
 *  Created on: Nov 17, 2024
 *      Author: micha
 */

#include "spi.h"
#include "accel.h"

#define CTRL1_REG 						0x20

#define CTRL1_REG_DATA_RATE_SELECTION 	0x80
#define CTRL1_REG_POWER_UP 				0x40
#define CTRL1_REG_FULL_SCALE_SELECTION  0x20
#define CTRL1_REG_Z_AXIS_ENABLE 		0x4
#define CTRL1_REG_Y_AXIS_ENABLE 		0x2
#define CTRL1_REG_X_AXIS_ENABLE 		0x1

#define CTRL2_REG 						0x21

#define CTRL2_REG_BOOT 					0x40

#define CTRL3_REG 						0x22
#define CTRL3_REG_IHL 					0x80
#define CTRL3_REG_CLICK_INT 			0x7
#define CTRL3_REG_FF1_INIT				0x1

#define STATUS_REG						0x27
#define X_AXIS_REG						0x29
#define Y_AXIS_REG						0x2b
#define Z_AXIS_REG						0x2d

#define SPI_ACCEL_WRITE 				0x0
#define SPI_ACCEL_READ 					0x80

#define SPI_ACCEL_BUFFER_LEN 			2

#define CTRL_INIT						(uint8_t)(CTRL1_REG_POWER_UP | CTRL1_REG_Z_AXIS_ENABLE | CTRL1_REG_X_AXIS_ENABLE | CTRL1_REG_Y_AXIS_ENABLE)

static const uint8_t axis_addrs[3] = {X_AXIS_REG,Y_AXIS_REG,Z_AXIS_REG};

typedef struct accel_spi_transaction{
	uint8_t rx_data[SPI_ACCEL_BUFFER_LEN];
	uint8_t tx_data[SPI_ACCEL_BUFFER_LEN];
}accel_spi_transaction_t;

volatile _Bool spi_finish = false;


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi){
	if(hspi == &hspi1){
		spi_finish = true;
	}
}

static void read_value(accel_spi_transaction_t* transaction,uint8_t addr){

	transaction -> tx_data[0] = SPI_ACCEL_READ | addr;
	transaction -> tx_data[1] = 0x0;

	HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, 1);
	HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, 0);
	HAL_SPI_TransmitReceive_DMA(&hspi1, transaction -> tx_data, transaction -> rx_data, SPI_ACCEL_BUFFER_LEN);

}

static void write_value(accel_spi_transaction_t* transaction,uint8_t addr,uint8_t value){

	transaction -> tx_data[0] = SPI_ACCEL_WRITE | addr;
	transaction -> tx_data[1] = value;

	HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, 1);
	HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, 0);
	HAL_SPI_TransmitReceive_DMA(&hspi1, transaction -> tx_data, transaction -> rx_data, SPI_ACCEL_BUFFER_LEN);
}

static _Bool wait_for_read_value(accel_spi_transaction_t* transaction,uint8_t* read_value){
	if(spi_finish){
		spi_finish = false;

		*read_value = transaction -> rx_data[1];
		HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, 1);
		return true;
	}else{
		return false;
	}
}

static _Bool wait_for_write_value(){
	if(spi_finish){
		spi_finish = false;
		HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, 1);
		return true;
	}else{
		return false;
	}
}

static inline void init_accel(accel_type_def_t* accel, accel_spi_transaction_t* transaction){
	if(accel -> started & accel -> initized & (!accel -> checked)){

		uint8_t value;

		if(wait_for_read_value(transaction,&value)){
			if(value == CTRL_INIT){
				accel -> error = false;
			}else{
				accel -> error = true;
			}

			accel -> checked = true;
		}
	}

	if(accel -> started & (!accel -> initized)){
		if(wait_for_write_value()){


			read_value(transaction,CTRL1_REG);
			accel -> initized = true;
		}
	}

	if(!accel -> started){

		write_value(transaction,CTRL1_REG,CTRL_INIT);
		accel -> started = true;
	}
}

void accel_handler(accel_type_def_t* accel){

	static accel_spi_transaction_t transaction={.rx_data = {0x0,0x0},.tx_data={0x0,0x0}};

	static uint32_t period = 0;
	static _Bool read = false;

	if(accel -> checked){
		if(!accel -> error){
			if(accel -> reading){

				uint8_t value;
				period ++;

				if(wait_for_read_value(&transaction, &value)){

					read = true;
					if(accel -> diagnostic_index == ACCEL_DIAGNOSTIC_MAX_COUNTER - 1){
						if(value != CTRL_INIT){
							accel -> error = true;
						}
						accel -> diagnostic_index = 0;
						accel -> reading = false;
					}else{


						accel -> axis_values[accel -> axis_index] = value;
						accel -> flag = true;

						accel -> diagnostic_index ++;
						accel -> axis_index ++;

						if(accel -> axis_index == ACCEL_AMOUNT_OF_AXIS){
							accel -> axis_index = 0;
						}
					}

				}

				if(period >= accel -> period){
					period = 0;
					accel -> reading = false;

					if(!read){
						accel -> error = true; //timeout
					}
				}

			}else{
				read = false;

				if(accel -> diagnostic_index == 0){
					period ++;
				}else{
					period = 1;
				}

				if(accel -> diagnostic_index == ACCEL_DIAGNOSTIC_MAX_COUNTER - 1){
					read_value(&transaction,CTRL1_REG);
				}else{
					read_value(&transaction,axis_addrs[accel -> axis_index]);
				}

				accel -> reading = true;
			}
		}
	}else{
		init_accel(accel, &transaction);
	}
}
