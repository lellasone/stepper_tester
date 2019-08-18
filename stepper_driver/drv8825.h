


#ifndef drv8825_H
#define drv8825_H

#include"Arduino.h"

class drv8825
{
  public:
    int compare;
    void setup();
    void set_mode(byte);
    void set_current(int);
    void set_enable(bool);
};

#endif

