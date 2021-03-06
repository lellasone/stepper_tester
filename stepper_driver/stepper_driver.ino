/* 
 *  This is some quick and dirty drive code for my stepper
 *  tester board. It allows speed control and step size 
 *  control based on the joystick, with an expontential 
 *  on the throttle to make control a bit nice. 
 *  
 *  Controls: 
 *  As designed, the stepper tester has throttle
 *  control along it's long axis, with an exponential gain
 *  and a small deadband in the middle. 
 *  
 *  Microstepping is controlled using the short axis, movements
 *  fully to the right will cause an increase in micro-stepping
 *  resulution (if possible) while movements to the left will
 *  cause a decrease. Note that the joystick must return to it's
 *  zero position before another command will be registered. 
 *  
 *  Update: 3/3/2021: Changed pin-mapping to match new board.
 *                    Program with pro-mini settings.
 *  Maintainer: Jake Ketchum
 */

#include "TimerOne.h"

// Hardware Pinouts.
#define PIN_TX     -1
#define PIN_RX     -1
#define PIN_STEP   5
#define PIN_DIR    6
#define PIN_RESET  -1 // reset device, optional. (Not Used)
#define PIN_SLEEP  -1 // dissables output, optional. (Not Used)
#define PIN_FAULT  -1 // should be pulled up. (Not Used)
#define PIN_DECAY  -1
#define PIN_ENABLE 0
#define PIN_HOME   -1 // should be pulled up. (Not Used)
#define PIN_VREF   3
#define PIN_MODE0  1
#define PIN_MODE1  2
#define PIN_MODE2  -1 //(Not Used)
#define PIN_THROTTLE A5
#define PIN_AUX A4
#define PIN_LED 8

// Chip configuration defaults
#define DEFAULT_MODE 0x03
#define DEFAULT_CURRENT 1.7
#define DEFAULT_DECAY HIGH
#define DEFAULT_DIR false

#define CURRENT_MIN 0
#define CURRENT_MAX 2

#define MODE_MIN 0x00
#define MODE_MAX 0x03

#define CURRENT_SENSE_RESISTOR 0.1

#define DEADBAND_HALF 20
#define DEADBAND_CENTER 522



//set the scaling endpoints for the throttle joystick. 
#define THROTTLE_MAX 825
#define THROTTLE_MIN 235 
#define MAX_ACCEL   .25 //The maximum acceleration allowed in rps/s
#define STEPS_PER_REV 200.0 //steps per revolution of the motor.
                          //Scales MAX_ACCEL.
#define MICROS_PER_SECOND 1000000.0

// Speed limits. (rotations per second)
#define MAX_VEL 1.5
#define MIN_VEL 0.1

// Timing defines. 
#define PERIOD_T1  1000000.0 // period of timer 1 in microseconds. 
#define PERIOD_LED  500000.0 // Period of status LED function.
#define PERIOD_POLL  10000.0 // Period for joystick input.


double step_flag = false; // Determine if motor should be moving
int compare = 5;
byte mode = DEFAULT_MODE;

void setup() {
  // put your setup code here, to run once:
  set_pins();
  set_enable(true);
  Timer1.initialize(PERIOD_T1);
  Timer1.attachInterrupt(step_motor,PERIOD_LED);
}

void loop() {
  static int timer = 0;
  int new_time = micros();
  if (new_time - timer > PERIOD_POLL){
    timer = new_time;
    poll_joystick();
  }
  delayMicroseconds(PERIOD_POLL/5);
};

/* 
 *  This function polls the joystick for changes to the 
 *  user specified step frequency and microstepping mode.
 *  The status led update is also called from within this
 *  function.
 *  (This is in place of a "loop()" function)
 */
void poll_joystick()
{

  static unsigned long timer = 0; 
  static int count = 0;
  static int throttle_old = -1;

  update_mode();  // Set the microstepping mode.

  // Get throttle value and pin if outside range.
  
  int throttle = analogRead(PIN_THROTTLE); 
  if (throttle_old != throttle)
  {
    if(throttle > THROTTLE_MAX) throttle = THROTTLE_MAX-1;
  
    if(throttle < DEADBAND_CENTER - DEADBAND_HALF){
      if(throttle < THROTTLE_MIN) throttle = THROTTLE_MIN+1;
      throttle = THROTTLE_MIN;
      double vel = map(throttle, DEADBAND_CENTER, THROTTLE_MIN, MIN_VEL, MAX_VEL);
      set_velocity(vel, false);
    }
    else if (throttle > DEADBAND_CENTER + DEADBAND_HALF){
      double vel = map(throttle, DEADBAND_CENTER, THROTTLE_MAX, MIN_VEL, MAX_VEL);
      set_velocity(MAX_VEL, true);
    }
    else {
      step_flag = false;
    }
  }
  throttle_old = throttle;
  if(count>PERIOD_LED/PERIOD_POLL)
  {
    show_mode();
    count = 0;
  }
  count++;
}

/* 
 *  This function enforces the acceleration limit on velocity
 *  changes and then sets the step frequency and direction pin. 
 *  Note: Accelerations are in terms of rotations/(minute * second)
 *  args:
 *    vel - the new velocity to set. 
 *    forward - if true the direction will be set forward. 
 */
void set_velocity(double vel, double forward){
  static double old_vel = 0;
  if (!step_flag) old_vel = 0;
  const double accel_limit = MAX_ACCEL / (MICROS_PER_SECOND / PERIOD_POLL);
  if (vel > old_vel + accel_limit) vel = old_vel + accel_limit;
  const double period = MICROS_PER_SECOND/(vel*STEPS_PER_REV*mod_mult());
  set_step_period(period, forward);
  old_vel = vel;
}

/*
 * This function sets the step waveform period and direction line. 
 * It will enforce the maximum acceleration constraint. 
 * args:
 *    period - the desired delay between subsiquent step pules in microseconds.
 *    forward - if true the motor will be sent forward. 
 */
double set_step_period(double period, bool forward){
  Timer1.setPeriod(period/2); // 1/2 for interrupt design
  digitalWrite(PIN_DIR, forward); 
  step_flag = true;
  return(period);
}

/* 
 *  Gets the current value of the microstepping mode as a base-10 value. 
 *  This will need to be changed for each chip, though most are just
 *  2^(mode) or 2^(mode + 1).
 */
int mod_mult(){
  switch(mode){
    case 0x00:
      return(2.0);
    case 0x01:
      return(4.0);
    case 0x02:
      return(8.0);
    case 0x03:
      return(16.0);
    default:
      return(1.0);
  }
}

/*
 * This function should be called twice per second. It will
 * blink the onboard LED according to the current microstepping
 * level. 
 */
void show_mode(){
  static int timer = 0;
  static int count = 0;
  static bool on = false;
  int new_time = millis();
  if(on){
    digitalWrite(PIN_LED, LOW);
    on = false;
  } else {
    if (count < mode * 2){
      digitalWrite(PIN_LED, HIGH);
      on = true;
    }
  }
  count++;
  if (count > MODE_MAX * 2){
    count = 0;
  }
}

/* This flips the motor step pin. It should be called at twice the
 *  desired motor step frequency. 
 */
void step_motor(){
  if(step_flag) digitalWrite(PIN_STEP, digitalRead(PIN_STEP) ^ 1);
}
/* 
 *  This function sets up the basic pins configuration for the device. 
 *  Once called, the drv8825 will be in a high-power mode, but will
 *  have it's outputs dissabled. 
 *  
 *  Before sending step and direction inputs be sure to enable the device. 
 */
void set_pins(){

  pinMode(PIN_FAULT, INPUT); 
  pinMode(PIN_HOME, INPUT); 
  pinMode(PIN_DECAY, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);//
  pinMode(PIN_VREF, OUTPUT); //
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_RESET, OUTPUT);//
  pinMode(PIN_SLEEP, OUTPUT);//
  pinMode(PIN_MODE0, OUTPUT);//
  pinMode(PIN_MODE1, OUTPUT);//
  pinMode(PIN_MODE2, OUTPUT);//
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TX, OUTPUT);
  pinMode(PIN_RX, OUTPUT);
 
  digitalWrite(PIN_RESET, HIGH); // dissable reset. 
  digitalWrite(PIN_SLEEP, HIGH); 
  digitalWrite(PIN_DIR, DEFAULT_DIR);

  digitalWrite(PIN_RX, LOW); // so we have a gnd reference. 

  set_enable(false);
  set_mode(DEFAULT_MODE);
  set_current(DEFAULT_CURRENT); 
  digitalWrite(PIN_DECAY, DEFAULT_DECAY);
}

/* 
 *  This function handles incrimenting and decrimenting the 
 *  microstepping level when the joystick is moved sideways. 
 *  The joystick must return to it's neutral location before
 *  another movement can be requested. 
 */
void update_mode(){
  static bool flag = true;
  int step_input = analogRead(PIN_AUX);
  if(step_input > 800 && flag && mode > MODE_MIN){
    mode--;
    set_mode(mode);
    flag = false;
  }
  if(step_input < 200 && flag && mode < MODE_MAX){
    mode++;
    set_mode(mode);
    flag = false;
  }
  if(step_input > 400 && step_input < 600){
    flag = true;
  }
}

/*
 * This function is used to set the micro-stepping mode of the 
 * driver chip. It does so by setting the the three mode control
 * pins on the DRV8825. There are 6 options avaliable
 * corresponding to 1/1, 1/2, 1/4, 1/8, 1/16, 1/32 microstepping. 
 * If the mode byte is larger than 5, it will be set to 5 before
 * processing. 
 * 
 * args: 
 *  mode - determins the microstepping level, values 0-5 correspond
 *         to full through 1/32 microstepping. Values above 5 all
 *         correspond to 1/32 microstepping. 
 */
void set_mode(byte mode){
  if(mode > 5) mode = 5;
  digitalWrite(PIN_MODE0, bitRead(mode, 0));
  digitalWrite(PIN_MODE1, bitRead(mode, 1));
  digitalWrite(PIN_MODE2, bitRead(mode, 2));
}

/* 
 *  This function sets the vref gpio pin to correspond to the 
 *  desired drive current. 
 *  args: 
 *    current - desired current setpoint in amps. 
 */
void set_current(double current){
  if(current < CURRENT_MIN) current = 0; 
  if(current > CURRENT_MAX) current = CURRENT_MAX; 
  // From the data sheet
  int voltage = current*2.5*sqrt(2)*(0.030 + CURRENT_SENSE_RESISTOR)/.325; 
  if (voltage>2.5) voltage = 2.5;
  int voltage_output = map(voltage, 0, 5, 0 , 255); 
  analogWrite(PIN_VREF, voltage_output); 
}

/* 
 *  This function enables or dissables the output transistors of the chip. 
 *  It needs to be called after setup, but before the device is used. 
 *  args:
 *    state - Boolean, if True device will be enabled, if false dissabled. 
 */
void set_enable(bool state){
  digitalWrite(PIN_ENABLE, !(state));
}
