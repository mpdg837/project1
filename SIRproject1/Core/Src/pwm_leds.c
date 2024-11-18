/*
 * pwm_leds.c
 *
 *  Created on: Nov 16, 2024
 *      Author: micha
 */

#include "tim.h"
#include "main.h"
#include "pwm_leds.h"

#define MAX_COLOR_LEVEL 255
#define MIN_COLOR_LEVEL 0

#define AMOUNT_OF_DIODES 4
#define RGB_COLORS 3

typedef struct pwm_led{
	  uint32_t channels [4];
	  uint16_t pins [4];
	  GPIO_TypeDef *led_instance[4];

	  uint8_t green[4];
	  uint8_t red[4];
	  uint8_t blue[4];
}pwm_led_t;


typedef struct diode_output_config_type_def{
	uint8_t diode_index;
	uint8_t color_index;
	uint8_t color_level;
	_Bool working;

	PWM_led_color_t leds[AMOUNT_OF_DIODES];
	uint8_t* color_table_ptr;

	uint8_t n;
}diode_output_config_type_def_t;

static const pwm_led_t pwm_leds = {
		.channels = {TIM_CHANNEL_1,TIM_CHANNEL_2,TIM_CHANNEL_3,TIM_CHANNEL_4},
		.pins = {LED1_Pin,LED2_Pin,LED3_Pin,LED4_Pin},
		.led_instance = {LED1_GPIO_Port,LED2_GPIO_Port,LED3_GPIO_Port,LED4_GPIO_Port},

		.red = {1,2,3,0},
		.green = {2,3,0,1},
		.blue = {3,0,1,2}
};

static inline void GPIO_change_mode(GPIO_TypeDef  *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
  uint32_t position = 0x00u;
  uint32_t iocurrent;
  uint32_t temp;

  /* Check the parameters */
  assert_param(IS_GPIO_ALL_INSTANCE(GPIOx));
  assert_param(IS_GPIO_PIN(GPIO_Init->Pin));
  assert_param(IS_GPIO_MODE(GPIO_Init->Mode));

  /* Configure the port pins */
  while (((GPIO_Init->Pin) >> position) != 0x00u)
  {
    /* Get current io position */
    iocurrent = (GPIO_Init->Pin) & (1uL << position);

    if (iocurrent != 0x00u)
    {

      /* Configure IO Direction mode (Input, Output, Alternate or Analog) */
      temp = GPIOx->MODER;
      temp &= ~(GPIO_MODER_MODE0 << (position * 2u));
      temp |= ((GPIO_Init->Mode & GPIO_MODE) << (position * 2u));
      GPIOx->MODER = temp;

    }

    position++;
  }
}


static inline void table_selector(diode_output_config_type_def_t* diode_config)
{
	 PWM_led_color_t* leds = &(diode_config ->leds[diode_config ->diode_index]);

	 switch(diode_config -> color_index){
	 	 case 0:
	 		 diode_config -> color_table_ptr = (uint8_t*) pwm_leds.red;
	 		 diode_config -> color_level = leds -> red;
			 break;
	 	 case 1:
	 		 diode_config -> color_table_ptr = (uint8_t*) pwm_leds.green;
	 		 diode_config -> color_level = leds -> green;
	 		 break;
	 	 case 2:
	 		 diode_config -> color_table_ptr = (uint8_t*) pwm_leds.blue;
	 		 diode_config -> color_level = leds -> blue;
			 break;
	 }
}


static inline void read_from_input_buffer(PWM_led_color_t* leds,const PWM_leds_type_def_t* pwm){
	  for(uint8_t n=0 ; n<AMOUNT_OF_DIODES ; n++){
		  leds[n].red = pwm -> leds[n].red;
		  leds[n].green = pwm -> leds[n].green;
		  leds[n].blue = pwm -> leds[n].blue;
	  }
}

static inline void clear_all_diodes(const PWM_leds_type_def_t* pwm){
	  for(uint8_t n=0 ; n< AMOUNT_OF_DIODES ; n++){
		  __HAL_TIM_SET_COMPARE(&htim4, pwm_leds.channels[n], MIN_COLOR_LEVEL);
	  }
}


static inline void diode_output_config(GPIO_InitTypeDef* GPIO_InitStruct,const diode_output_config_type_def_t* diode_config ){
	  GPIO_InitStruct -> Pin = pwm_leds.pins[diode_config -> n];
	  GPIO_InitStruct -> Mode = GPIO_MODE_AF_PP; // GPIO_MODE_AF_OD

	  if(diode_config -> n == diode_config -> diode_index){
		  __HAL_TIM_SET_COMPARE(&htim4, pwm_leds.channels[diode_config->n], diode_config -> color_level);

	  }else{

		  __HAL_TIM_SET_COMPARE(&htim4, pwm_leds.channels[diode_config->n], MIN_COLOR_LEVEL);

		  if(diode_config -> color_table_ptr[diode_config -> diode_index] == diode_config->n){
			  GPIO_InitStruct -> Mode = GPIO_MODE_AF_OD;
		  }else{
			  GPIO_InitStruct -> Mode = GPIO_MODE_ANALOG;
		  }
	  }
}
void pwm_leds_it_handler(const PWM_leds_type_def_t* pwm){

	  static diode_output_config_type_def_t diode_config = {
			  .diode_index = 0,
	  	  	  .color_index = 0,
			  .color_level = 0,
	  	  	  .working = false,
	  	  	  .n = 0
	  };

	  static GPIO_InitTypeDef GPIO_InitStruct;

	  if(pwm ->enable){

		  diode_config.working = true;


		  table_selector(&diode_config);


		  for(diode_config.n=0 ; diode_config.n< AMOUNT_OF_DIODES ; diode_config.n++){
			  diode_output_config(&GPIO_InitStruct,&diode_config);

			  GPIO_change_mode((GPIO_TypeDef *)pwm_leds.led_instance[diode_config.n], &GPIO_InitStruct);
		  }


		  if(++diode_config.color_index == RGB_COLORS){

			  diode_config.color_index = 0;

			  if(++diode_config.diode_index == AMOUNT_OF_DIODES){
				  diode_config.diode_index = 0;

				  read_from_input_buffer(diode_config.leds,pwm);


			  }
		  }
	  }else{
		  if(diode_config.working){
			  clear_all_diodes(pwm);
			  diode_config.working = false;
		  }
	  }
}
