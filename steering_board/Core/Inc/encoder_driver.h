/*
 * encoder_driver.h
 *
 *  Created on: Nov 18, 2025
 *      Author: fabioesposito
 */

#ifndef INC_ENCODER_DRIVER_H_
#define INC_ENCODER_DRIVER_H_

#include "main.h"
#include "tx_api.h"

UINT encoder_driver_initialize();
UINT encoder_driver_input(int32_t *position);

#endif /* INC_ENCODER_DRIVER_H_ */

