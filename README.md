# stepper_tester
This was a quick one-off stepper tester board. This version uses a TMC2208 from Trinamic. 

The idea for this version was to test out if the TMC2208 is really a drop in
replacement for it's competetors in practice (spoiler: it is!). A few (very)
minor improvements have been made to the code, but it actually ran with only
new pin mappings and no other changes the first time I uploaded it.

## Making Changes
For pcb design files look in PCB (KiCAD). For the arduino code look in the 
stepper_driver directory. 

You can upload code to the board using your ISP programmer of choice (I use 
tinyISP) with the fuse settings and pin mappings for a pro-mini.  

## Operating Instructions
* Move the joystick up and down to change the pulse frequency within a single micro-stepping band (zero centered).
* Flick the joystick side to side to change the microstepping level. (faster to the left)
    * To read the current microstepping level, count the status LED's pulses.
* To disable output, connect the jumper by the joystick.
