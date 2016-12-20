/*
 * Common project definitions
 */

#ifndef CONFIG_H
#define CONFIG_H

/** Enable debug output by this */
#define DEBUG

/**  maximum safe duty cycle of PWM
 *  Since we can put higher voltage on motor, we must limit the power by PWM
 */
#define PWM_MAX 136

/** Clock ticks per microsecond */
#define USEC_TICKS (ARCV2_TIMER1_CLOCK_FREQ / 1E6)

#endif
