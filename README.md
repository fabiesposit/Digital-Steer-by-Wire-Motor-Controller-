# psr-semestral-work

## UDP Communication Protocol
The Steering Board sends the requested motor position as a 32-bit signed integer (int32_t)
representing a continuous, unwrapped encoder value. The value is serialized into 4 bytes 
using network byte order (big-endian) before transmission over UDP.

The Controlled Board receives the 4-byte payload, reconstructs the int32_t value, and stores it
into two internal queues (PID queue and HTTP queue). Both queues hold signed 32-bit integers,
ensuring consistent use of continuous, non-wrapping positions across all threads.

This guarantees that the PID controller, the HTTP server, and the live graphs operate on a 
stable and monotonic position reference without discontinuities at the encoder rollover.
