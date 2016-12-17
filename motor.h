/*
 * Motor handling
 */

#ifndef MOTOR_H
#define MOTOR_H

/* motor characteristics */
#define STEP_RATIO 200

/* movement settings */
#define SEQ_LEN 4

extern int ctrl_sequence[SEQ_LEN][4];

#define MICROSTEPS 16
#define SINE_RES 16

extern int sine[];

struct motor_ctx {
  int ctrl_ph;  /* phase of control sequence */
  int ind;      /* index in controling curve (sine wave) */
  int pins[4];  /* control pins */
  int en[2];    /* enable pins */
  int dir;      /* movement direction */
  int step_time;/* time of single step */
  int ticks_left;
};

void motor_microstep(struct motor_ctx *motor);
void motor_fullstep(struct motor_ctx *motor);

#endif
