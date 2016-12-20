#include "CurieTimerOne.h"
#include "CurieBLE.h"

#include "motor.h"
#include "config.h"

BLEPeripheral blePeripheral;
BLEService eqService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEIntCharacteristic simpleChar("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

struct motor_ctx RA_motor = {
  .ctrl_ph = 0,
  .ind = 0,
  .pins = {3,4,7,8},
  .en = {5,6},
  .dir = 1,
  .step_time = 1E4,
  .ticks_left = 0,
};

struct motor_ctx DEC_motor = {
  .ctrl_ph = 0,
  .ind = 0,
  .pins = {3,4,7,8},
  .en = {5,6},
  .dir = 1,
  .step_time = 1E6,
  .ticks_left = 0,
};

void full_step_isr()
{
  motor_fullstep(&RA_motor);
}

void micro_step_isr()
{
  motor_microstep(&RA_motor);
}

static int seconds = 0;
void main_isr()
{
  uint32_t ticks = aux_reg_read(ARC_V2_TMR1_COUNT);

  /* clear the timer */
  aux_reg_write(ARC_V2_TMR1_CONTROL, 0);
  aux_reg_write(ARC_V2_TMR1_COUNT, 0);

  if (RA_motor.ticks_left == 0) {
    RA_motor.ticks_left = RA_motor.step_time * USEC_TICKS ;
    motor_fullstep(&RA_motor);
  }

  if (DEC_motor.ticks_left == 0) {
    DEC_motor.ticks_left = DEC_motor.step_time * USEC_TICKS;
    //Serial.println("DEC handling");
  }

  if (RA_motor.ticks_left < DEC_motor.ticks_left) {
    ticks = RA_motor.ticks_left;
  } else {
    ticks = DEC_motor.ticks_left;
  }

  RA_motor.ticks_left -= ticks;
  DEC_motor.ticks_left -= ticks;
  
  aux_reg_write(ARC_V2_TMR1_CONTROL, ARC_V2_TMR_CTRL_NH | ARC_V2_TMR_CTRL_IE);
  aux_reg_write(ARC_V2_TMR1_LIMIT, ticks);
  interrupt_enable(ARCV2_IRQ_TIMER1);
}

int init_timer(int ticks)
{
  aux_reg_write(ARC_V2_TMR1_CONTROL, ARC_V2_TMR_CTRL_NH | ARC_V2_TMR_CTRL_IE);
  interrupt_connect(ARCV2_IRQ_TIMER1, main_isr);

  aux_reg_write(ARC_V2_TMR1_LIMIT, ticks);
  aux_reg_write(ARC_V2_TMR1_CONTROL, ARC_V2_TMR_CTRL_NH | ARC_V2_TMR_CTRL_IE);
  aux_reg_write(ARC_V2_TMR1_COUNT, 0);

  interrupt_enable(ARCV2_IRQ_TIMER1);
}

void setup() {
  int i;

  Serial.begin(9600);

  // Initialize pin 13 as an output - onboard LED.
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  analogWrite(5, PWM_MAX);
  analogWrite(6, PWM_MAX);

  init_timer(ARCV2_TIMER1_CLOCK_FREQ);
//  CurieTimerOne.start(STEP_TIME / MICROSTEPS, &micro_step_isr);
//  CurieTimerOne.start(STEP_TIME, &full_step_isr);
//  CurieTimerOne.start(STEP_TIME/2, &half_step_isr);

  blePeripheral.setLocalName("Telescope Control");
  blePeripheral.setAdvertisedServiceUuid(eqService.uuid());
  blePeripheral.addAttribute(eqService);
  blePeripheral.addAttribute(simpleChar);

  blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  simpleChar.setEventHandler(BLEWritten, simpleCharacteristicWritten);
  simpleChar.setValue(RA_motor.step_time);

  // advertise the service
  blePeripheral.begin();
  Serial.println(("Bluetooth device active, waiting for connections..."));
}

void loop() {
  blePeripheral.poll();
}

void blePeripheralConnectHandler(BLECentral& central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLECentral& central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
}

void simpleCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  Serial.print("Characteristic event, written: ");
  Serial.println(simpleChar.value());

  RA_motor.step_time = simpleChar.value();
}

