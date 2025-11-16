/*
 * motor_driver.h
 *
 *  Created on: Nov 16, 2025
 *      Author: angio
 */

#ifndef INC_MOTOR_DRIVER_H_
#define INC_MOTOR_DRIVER_H_

#define MOTOR_PWM_MAX 500

#include "main.h"
#include "tx_api.h"


UINT motor_driver_initialize(void);
UINT motor_driver_set_duty(int16_t duty);
UINT motor_driver_get_duty(int16_t* duty);
UINT motor_driver_get_pwm(uint16_t* pwm);


#endif /* INC_MOTOR_DRIVER_H_ */
