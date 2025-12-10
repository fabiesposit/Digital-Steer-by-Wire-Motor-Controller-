/*
 * signal_logger.c
 *
 * Periodically samples motor signals (actual encoder position,
 * requested position, PWM duty cycle) and stores them in a fixed-size
 * circular buffer. This buffer is later read by the HTTP server to
 * provide blocks of samples for live graphs in the web interface.
 */

#include "signal_logger.h"
#include "encoder_driver.h"
#include "motor_driver.h"

/* Circular buffer storing the last LOG_LEN samples. */
static sample_t log_buf[LOG_LEN];

/* Index of the next write position in the circular buffer [0 .. LOG_LEN-1]. */
static UINT log_head = 0;

/* Mutex protecting concurrent access to the circular buffer. */
static TX_MUTEX log_mutex;

/*---------------------------------------------------------------------------
 * Sampling period in ThreadX ticks.
 * If TX_TIMER_TICKS_PER_SECOND = 200, then:
 *   1 tick = 5 ms
 *   signal_logger_thread_entry() sampling period = 1 tick = 5 ms
 *---------------------------------------------------------------------------*/
#define LOGGER_PERIOD_TICKS  1

/* These symbols are provided by the rest of the application. */
extern int32_t requested_position;
extern TX_MUTEX mutex_req_pos;

/* ===================== Initialization ===================== */

/* Initialize the logging module:
 *  - reset the circular buffer head index
 *  - create the mutex used to protect the buffer
 */
UINT signal_logger_init(void)
{
    UINT ret;

    log_head = 0;

    ret = tx_mutex_create(&log_mutex, "signal logger mutex", TX_INHERIT);
    if (ret != TX_SUCCESS)
    {
        return ret;
    }

    return TX_SUCCESS;
}

/* ===================== Sampling Thread ===================== */

/* This thread is executed periodically and samples:
 *  - actual motor position (encoder)
 *  - PWM duty value
 *  - requested setpoint
 *
 * Each sample is written into the circular buffer, overwriting the oldest
 * element when the buffer wraps around.
 */
VOID signal_logger_thread_entry(ULONG thread_input)
{
    (void)thread_input;

    int32_t  encoder_pos;
    uint16_t pwm_raw;
    int32_t  req_pos;

    while (1)
    {
        /* 1) Read actual encoder position */
        encoder_driver_input(&encoder_pos);

        /* 2) Read current PWM value */
        motor_driver_get_pwm(&pwm_raw);

        /* 3) Read requested position (shared variable) */
        tx_mutex_get(&mutex_req_pos, TX_WAIT_FOREVER);
        req_pos = requested_position;
        tx_mutex_put(&mutex_req_pos);

        /* 4) Store sample in the circular buffer */
        tx_mutex_get(&log_mutex, TX_WAIT_FOREVER);

        log_buf[log_head].actual    = encoder_pos;
        log_buf[log_head].requested = req_pos;
        log_buf[log_head].pwm       = (int16_t)pwm_raw;

        /* advance write index with wrap-around */
        log_head = (log_head + 1U) % LOG_LEN;

        tx_mutex_put(&log_mutex);

        /* 5) Wait until the next sampling instant */
        tx_thread_sleep(LOGGER_PERIOD_TICKS);  // 5 ms if tick = 5 ms
    }
}

/* ===================== Buffer Access (HTTP) ===================== */

/* Copy the last LOG_LEN samples into dst.
 * Samples are returned in chronological order:
 *   dst[0] = oldest
 *   dst[LOG_LEN-1] = newest
 */
UINT signal_logger_get_last_samples(sample_t *dst)
{
    UINT i;
    UINT idx;

    if (dst == TX_NULL)
    {
        return TX_PTR_ERROR;
    }

    tx_mutex_get(&log_mutex, TX_WAIT_FOREVER);

    /* log_head points to next write position, which means it is also
     * the location of the oldest sample in the circular buffer.
     */
    idx = log_head;

    for (i = 0; i < LOG_LEN; ++i)
    {
        dst[i] = log_buf[idx];
        idx = (idx + 1U) % LOG_LEN;
    }

    tx_mutex_put(&log_mutex);

    return TX_SUCCESS;
}
