/*
 * signal_logger.h
 *
 * Periodic sampling of controller-related signals (actual position,
 * requested position, and PWM) into a fixed-size circular buffer.
 * The buffer is later read by the HTTP server to provide blocks
 * of samples for real-time web visualization.
 */

#ifndef SIGNAL_LOGGER_H
#define SIGNAL_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tx_api.h"
#include <stdint.h>

/* Number of samples stored in the circular buffer.
 * With a sampling period of 5 ms, LOG_LEN = 20 corresponds to 100 ms of history.
 */
#define LOG_LEN 20

/* One sampled data point of the controller state. */
typedef struct {
    int32_t actual;     /* Motor actual position (from encoder) */
    int32_t requested;  /* Requested setpoint position           */
    int16_t pwm;        /* Current PWM command (raw duty)        */
} sample_t;

/* Initialize the logger module.
 * Must be called before starting the sampling thread.
 */
UINT signal_logger_init(void);

/* Thread entry function that runs periodically and samples signals.
 * Recommended period: 5 ms (1 tick with TX_TIMER_TICKS_PER_SECOND = 200).
 */
VOID signal_logger_thread_entry(ULONG thread_input);

/* Retrieve the last LOG_LEN samples from the circular buffer.
 * Samples are copied into dst from oldest (dst[0]) to newest (dst[LOG_LEN-1]).
 */
UINT signal_logger_get_last_samples(sample_t *dst);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_LOGGER_H */
