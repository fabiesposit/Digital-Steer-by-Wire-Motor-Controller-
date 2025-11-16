/*
 * motor_driver.c
 *
 *  Created on: Nov 16, 2025
 *      Author: angio
 */
#include "motor_driver.h"
#include "app_threadx.h"


static TX_MUTEX motor_mutex;
static int16_t current_duty;
static uint16_t current_pwm;

UINT motor_driver_initialize(){
	uint ret;
	ret = tx_mutex_create(&motor_mutex, "motor_mutex", TX_INHERIT);
	if(ret != TX_SUCCESS){
		printf("Error in creating motor_mutex: %u\n", ret);
		return ret;
	}

	return ret;
}


UINT motor_driver_set_duty(int16_t duty){
	UINT ret;

	if (duty >  MOTOR_PWM_MAX) duty =  MOTOR_PWM_MAX;
	if (duty < -MOTOR_PWM_MAX) duty = -MOTOR_PWM_MAX;

	ret = tx_mutex_get(&motor_mutex, TX_WAIT_FOREVER);

	if(ret != TX_SUCCESS){
		printf("[MOTOR DRIVER SET DUTY]: Error in getting the mutex: %u\n", ret);
		return ret;
	}

	current_duty = duty;

	if(duty >= 0){
		TIM2->CCR3 = 0;
		TIM2->CCR4 = (uint16_t) duty;
		current_pwm = duty * 100 / MOTOR_PWM_MAX;
	}
	else{
		TIM2->CCR3 = (uint16_t) (-duty);
		TIM2->CCR4 = 0;
		current_pwm = -duty * 100 / MOTOR_PWM_MAX;
	}

	ret = tx_mutex_put(&motor_mutex);
	if(ret != TX_SUCCESS){
			printf("[MOTOR DRIVER SET DUTY]: Error in putting the mutex: %u\n", ret);
			return ret;
		}

	return ret;

}

UINT motor_driver_get_duty(int16_t* duty_out){
	UINT ret;
	if(duty_out == NULL) return;

	ret = tx_mutex_get(&motor_mutex, TX_WAIT_FOREVER);

		if(ret != TX_SUCCESS){
			printf("[MOTOR DRIVER GET DUTY]: Error in getting the mutex: %u\n", ret);
			return ret;
		}

	*duty_out = current_duty;

	ret = tx_mutex_put(&motor_mutex);
		if(ret != TX_SUCCESS){
				printf("[MOTOR DRIVER GET DUTY]: Error in putting the mutex: %u\n", ret);
				return ret;
			}

	return ret;

}

UINT motor_driver_get_pwm(uint16_t *pwm_out){
		UINT ret;
		if(pwm_out == NULL) return;

		ret = tx_mutex_get(&motor_mutex, TX_WAIT_FOREVER);

			if(ret != TX_SUCCESS){
				printf("[MOTOR DRIVER GET PWM]: Error in getting the mutex: %u\n", ret);
				return ret;
			}

		*pwm_out = current_pwm;

		ret = tx_mutex_put(&motor_mutex);
			if(ret != TX_SUCCESS){
					printf("[MOTOR DRIVER GET PWM]: Error in putting the mutex: %u\n", ret);
					return ret;
				}

		return ret;

}

