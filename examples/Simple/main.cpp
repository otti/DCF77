#include <Arduino.h>

#include "DCF77.h"

#define DCF77_INPUT 1            // Pin for DCF77 receiver
#define DCF77_LED   LED_BUILTIN  // Optional pin for a LED to display the receiver pin state


DCF77 dcf;

void setup() 
{
  Serial.begin(115200);
  while (!Serial) {}   // optional (USB boards)

  dcf.begin(DCF77_INPUT, RISING);
  dcf.SetDbgSerial(&Serial); // Optional: Print DCF77 debug information 

  pinMode(DCF77_LED, OUTPUT); // Optional: Set LED pin to putput mode
}

void loop()
{
  tm* Time;

  dcf.loop(); // Must be called at least every 15 ms
  
  if( dcf.NewTimeAvailable() )
  {
    Time = dcf.GetTime();

    // Print time
    Serial.print(Time->tm_hour);
    Serial.print(":");
    Serial.println(Time->tm_min);

    // Print date
    Serial.print(Time->tm_year);
    Serial.print("-");
    Serial.print(Time->tm_mon);
    Serial.print("-");
    Serial.println(Time->tm_mday);
  }
  
  // optional: Show DCF77 pin state on a LED
  digitalWrite(DCF77_LED, dcf.GetInputLevel());
}



