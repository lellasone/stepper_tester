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
 *  
 *  Maintainer: Jake Ketchum
 */


#include "drv8825.h"

drv8825 motor = drv8825();

void setup() {
  // put your setup code here, to run once:
  motor.setup();
}

void loop() {
  // put your main code here, to run repeatedly:

}
