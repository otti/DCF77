# Simple DCF77 library for arduino based projects

Does not need additional timers or interrupts. This comes with the downside, that the DCF77::loop() function has to be called at least every 15 ms. So you are not allowed to stall your main program. (e.g. by using the delay() function)

## Code example
```
#include <Arduino.h>
#include "DCF77.h"
#define DCF77_INPUT 1            // Pin for DCF77 receiver
DCF77 dcf;

void setup() 
{
  dcf.begin(DCF77_INPUT, RISING);
}

void loop()
{
  tm* Time;

  dcf.loop(); // Must be called at least every 15 ms
  
  if( dcf.NewTimeAvailable() )
  {
    Time = dcf.GetTime();
  }
}
```