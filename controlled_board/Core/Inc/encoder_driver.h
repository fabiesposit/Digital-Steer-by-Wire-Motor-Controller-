/*
 * encoder_driver.h
 *
 *  Created on: Oct 19, 2025
 *      Author: Ondrej Teren
 */

/*
 *  * This driver provides a continuous, signed, absolute position of the motor shaft,
 * removing the discontinuities that occur when the hardware counter wraps around
 * (e.g., from 65535 → 0 or 0 → 65535).
 *  Thread-safety:
 *  A mutex is used internally to protect shared state, so calls to the driver
 *  functions are safe from concurrent access across multiple threads.
 */

#ifndef INC_ENCODER_DRIVER_H_
#define INC_ENCODER_DRIVER_H_

#include "main.h"
#include "tx_api.h"

UINT encoder_driver_initialize();
UINT encoder_driver_input(int32_t *position);

#endif /* INC_ENCODER_DRIVER_H_ */
