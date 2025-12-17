/*
 * encoder_driver.c
 *
 *  Created on: Nov 17, 2025
 *      Author: angio
 */

#include "encoder_driver.h"
#include "app_threadx.h"


static TX_MUTEX encoder_mutex;

//unwrapped position
static int32_t  encoder_position = 0;

//last reading
static uint16_t encoder_last_count = 0;

UINT encoder_driver_initialize(){
	UINT ret;

	ret = tx_mutex_create(&encoder_mutex, "encoder mutex", TX_INHERIT);
	if(ret != TX_SUCCESS){
		printf("[Encoder driver initialize] Error in creating the mutex: %u\n", ret);
	}

	encoder_last_count = (uint16_t)TIM1->CNT;
	encoder_position   = 0;

	return ret;
}

UINT encoder_driver_input(int32_t* position){
	UINT ret;

	ret = tx_mutex_get(&encoder_mutex, TX_WAIT_FOREVER);
	if(ret != TX_SUCCESS){
		printf("[Encoder driver input] Error in getting the mutex: %u\n", ret);
		return ret;
	}

	uint16_t current = (uint16_t)TIM1->CNT;

	/*
	 * Compute the encoder delta using 16-bit wrap-around arithmetic.
	 * The subtraction (current - last) is performed modulo 65536.
	 * Casting the result to int16_t converts large wrap-around jumps
	 * (e.g., 65535 → 0 or 0 → 65535) into small positive or negative deltas.
	 * This removes discontinuities at the timer rollover and produces
	 * a continuous absolute position over multiple rotations.
	 */

	int16_t delta = (int16_t)(current - encoder_last_count);

	encoder_position += (int32_t)delta;
	encoder_last_count = current;

	*position = encoder_position;
	ret = tx_mutex_put(&encoder_mutex);
	if(ret != TX_SUCCESS){
		printf("[Encoder driver input] Error in putting the mutex: %u\n", ret);
		return ret;
	}

	return ret;

}



