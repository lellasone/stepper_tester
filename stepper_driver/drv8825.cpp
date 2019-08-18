/*
 * This is a driver for the DRV8825. It is intended to be 
 * somewhat generic, but will still likely need to be 
 * modified for each use case. 
 * 
 * Note: This is only useable when a single drv8825 is connected,
 * it will need to be re-written if multiple are in use. 
 */

#include "Arduino.h"
#include "drv8825.h"
#include <avr/io.h>
#include <avr/interrupt.h>


#define PIN_STEP   0//8
#define PIN_DIR    10
#define PIN_RESET  A3 // reset device, optional. 
#define PIN_SLEEP  A2 // dissables output, optional. 
#define PIN_FAULT  A1 // should be pulled up. 
#define PIN_DECAY  A0
#define PIN_ENABLE 9
#define PIN_HOME   A5 // should be pulled up. 
#define PIN_VREF   3
#define PIN_MODE0  7
#define PIN_MODE1  6
#define PIN_MODE2  5

//defaults
#define DEFAULT_MODE 0
#define DEFAULT_CURRENT 1
#define DEFAULT_DECAY 0 

#define CURRENT_MIN 0
#define CURRENT_MAX 2

#define CURRENT_SENSE_RESISTOR 0.2

#define COUNT_STOP 65535 // value at which the counter stops completely

int compare = 12;
/* 
 *  This function sets up the basic pins configuration for the device. 
 *  Once called, the drv8825 will be in a high-power mode, but will
 *  have it's outputs dissabled. 
 *  
 *  Before sending step and direction inputs be sure to enable the device. 
 */
void drv8825::setup(){

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
 
  digitalWrite(PIN_RESET, HIGH); // dissable reset. 
  digitalWrite(PIN_SLEEP, HIGH); 
  digitalWrite(PIN_ENABLE, HIGH); 

  set_mode(DEFAULT_MODE);
  set_current(DEFAULT_CURRENT); 

  // setup interrupt. 
  OCR0A = 0xAA;
  TIMSK0 |= _BV(OCIE0A); 

  // Setup Timer 1.
  
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
void drv8825::set_mode(byte mode){
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
void drv8825::set_current(int current){
  if(current < CURRENT_MIN) current = 0; 
  if(current > CURRENT_MAX) current = CURRENT_MAX; 

  int voltage = current * 5 *CURRENT_SENSE_RESISTOR; 
  int voltage_output = map(voltage, 0, 5, 0 , 255); 

  analogWrite(PIN_VREF, voltage_output); 
}

/* 
 *  This function enables or dissables the output transistors of the chip. 
 *  It needs to be called after setup, but before the device is used. 
 *  args:
 *    state - Boolean, if True device will be enabled, if false dissabled. 
 */
void drv8825::set_enable(bool state){
  digitalWrite(PIN_ENABLE, !(state));
}

SIGNAL(TIMER0_COMPA_vect)
{
  static int count = 0;
  static bool high = false;
  if(compare < COUNT_STOP)
  {
    count++;
    if(count > compare)
    {
     
      digitalWrite(PIN_STEP, !(high));
      high = !(high);
      count = 0;
    } 
  }
}

