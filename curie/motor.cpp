#include "motor.h"
#include "config.h"
#include "Arduino.h"

int ctrl_pins[4] = {3,4,7,8};
int ctrl_sequence[SEQ_LEN][4] = 
{
  {0, 1, 0, 1},
  {0, 1, 1, 0},
  {1, 0, 1, 0},
  {1, 0, 0, 1},
};

#define MICROSTEPS 16
#define SINE_RES 16

int sine[] = {0, 25, 50, 74, 98, 120, 141, 162, 180, 197, 212, 225, 236, 244, 250, 253, 255};

void motor_microstep(struct motor_ctx *motor)
{
  int i;
  double a, b;

  motor->ind += motor->dir * SINE_RES/MICROSTEPS;
  if (motor->ind >= SINE_RES) {
    motor->ind = motor->dir > 0 ? 0 : SINE_RES;
    motor->ctrl_ph += motor->dir;
    motor->ctrl_ph %= SEQ_LEN;
    for (i = 0; i < 4; ++i)
      digitalWrite(motor->pins[i], ctrl_sequence[motor->ctrl_ph][i]);
  }

  if (motor->ctrl_ph == 0 || motor->ctrl_ph == 2) {
    a = sine[motor->ind] * PWM_MAX >> 8;
    b = sine[SINE_RES - motor->ind] * PWM_MAX >> 8;
  } else {
    b = sine[motor->ind] * PWM_MAX >> 8;
    a = sine[SINE_RES - motor->ind] * PWM_MAX >> 8;
  }

  analogWrite(motor->en[0], a);
  analogWrite(motor->en[1], b);

#ifdef DEBUG
  Serial.print(a);
  Serial.print(" ");
  Serial.println(b);
#endif
}

void motor_fullstep(struct motor_ctx *motor)
{
  int i;

  for (i = 0; i < 4; ++i)
    digitalWrite(motor->pins[i], ctrl_sequence[motor->ctrl_ph][i]);

  motor->ctrl_ph += motor->dir;
  motor->ctrl_ph %= SEQ_LEN;
}
