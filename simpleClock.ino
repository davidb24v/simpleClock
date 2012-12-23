
#include "LedControl.h"

/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */
LedControl lc=LedControl(12,11,10,1);

int count = 0;
int update = 0;
int powerGood = 1;

byte hours = 8;
byte minutes = 37;
byte seconds = 0;

#include <Button.h>        //https://github.com/JChristensen/Button

#define COMMAND_BUTTON 7   // Used to enter programming mode
#define INCR_BUTTON 8      // Used to change the value
#define PULLUP true        //To keep things simple, we use the Arduino's internal pullup resistor.
#define INVERT true        //Since the pullup resistor will keep the pin high unless the
                           //switch is closed, this is negative logic, i.e. a high state
                           //means the button is NOT pressed. (Assuming a normally open switch.)
#define DEBOUNCE_MS 20     //A debounce time of 20 milliseconds usually works well for tactile button switches.

#define REPEAT_FIRST 500   //ms required before repeating on long press
#define REPEAT_INCR 100    //repeat interval for long press

Button cmdBtn(COMMAND_BUTTON, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the button
Button incBtn(INCR_BUTTON, PULLUP, INVERT, DEBOUNCE_MS);       //Declare the button

enum {NORMAL, HOURS, MINUTES};

void setup()
{
  Serial.begin(115200);
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,4);
  /* and clear the display */
  lc.clearDisplay(0);

  // Wait for DS32KHz to settle
  delay(1000);
  attachInterrupt(0, clock, RISING);
  
  // Detect if power has failed
  attachInterrupt(1, powerFail, FALLING);

  displayTime();
}

void loop()
{
  if ( update ) {
    updateTime();
    update = 0;
    if ( powerGood ) {
      displayTime();
      int CdS = analogRead(A0);
      Serial.println(CdS);
      lc.setIntensity(0,map(CdS,0,1023,0,15));
    }
  }
}

void clock()
{
  if ( ++count == 32767 ) {
    count = 0;
    update = 1;
  }
}

void powerFail() {
  powerGood = 0;
  attachInterrupt(1, powerRestored, RISING);
}

void powerRestored() {
  powerGood = 1;
  attachInterrupt(1, powerFail, FALLING);
}

void updateTime() {
  seconds++;
  if ( seconds == 60 ) {
    seconds = 0;
    minutes++;
    if ( minutes == 60 ) {
      minutes = 0;
      hours++;
      if ( hours == 24 ) {
        hours = 0;
      }
    }
  }
}

void displayTime() {
  lc.setDigit(0,0,hours/10,false);
  lc.setDigit(0,1,hours % 10,false);
  lc.setDigit(0,2,minutes/10,false);
  lc.setDigit(0,3,minutes % 10,false);
}

