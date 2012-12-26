
#include "LedControl.h"
#include <TimerHelpers.h>

/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 11 is connected to the DataIn 
 pin 10 is connected to the CLK 
 pin 9 is connected to LOAD 
 We have only a single MAX72XX.
 */
LedControl lc=LedControl(11,10,9,1);

int count = 0;
int update = 0;
int powerGood = 1;

byte hours = 0;
byte minutes = 0;
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

enum {WAIT, START, HOURS, MINUTES};
uint8_t STATE;

bool digitState;
int digitFlash = 250;
long nextFlash = 0;
byte powerJustOff = 0;
byte powerJustOn = 0;

// Reference voltage for this particular processor is 1.076V
// See: http://www.gammon.com.au/forum/?id=11497 for
// how to determine this value
const int RefVoltage = 1076;

void setup()
{
  Serial.begin(115200);

  // Wait for DS32KHz to settle
  delay(1000);
//  attachInterrupt(0, clock, RISING);
  
  // Detect if power has failed
//  attachInterrupt(1, powerFail, FALLING);

  TCCR1A = 0;        // reset timer 1
  TCCR1B = 0;

 // set up Timer 1
  TCNT1 = 0;         // reset counter
  OCR1A =  32767;       // compare A register value (1000 * clock speed)
  
  // Mode 4: CTC, top = OCR1A
  Timer1::setMode (4, Timer1::T1_RISING,
                                   Timer1::CLEAR_A_ON_COMPARE);

  TIFR1 |= _BV (OCF1A);    // clear interrupt flag
  TIMSK1 = _BV (OCIE1A);   // interrupt on Compare A Match  

// Allow internal voltage to be measured...
//analogReference(INTERNAL);
//analogRead(A0);

  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,4);
  /* and clear the display */
  lc.clearDisplay(0);

}

ISR(TIMER1_COMPA_vect)
{
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
  update = 1;
}  // end of TIMER1_COMPA_vect
  


void loop()
{
  
  if (powerJustOn) {
      lc.shutdown(0,false);
      lc.setIntensity(0,4);
      lc.clearDisplay(0);
      powerJustOn = 0;
      Serial.println("Power restored");
  }

  if (powerJustOff) {
      powerJustOff = 0;
      Serial.println("Power Failure!");
  }

  if ( update && powerGood ) {
    Serial.println(seconds);
    update = 0;
    displayTime();
    int CdS = analogRead(A0);
    Serial.print(millis());
    Serial.print(" ");
    Serial.println(CdS);
    lc.setIntensity(0,map(CdS,0,1023,0,15));
    
    int Vcc = analogRead(A1);
    Serial.print("Vcc = ");
    Serial.println(Vcc);
  }

  if ( powerGood ) {

    long now = millis();
    cmdBtn.read();
    incBtn.read();

    if ( nextFlash && (now > nextFlash) ) {
      digitState = !digitState;
      switch (STATE) {
        case HOURS:
          showHours();
          break;
        case MINUTES:
          showMinutes();
          break;
      }
      nextFlash = now + digitFlash;
    }

    switch (STATE) {
      case WAIT:
        if (cmdBtn.pressedFor(REPEAT_FIRST)) {
          digitState = false;
          showHours();
          nextFlash = millis() + digitFlash;
          detachInterrupt(0);
          STATE = START;
        }
        break; 

      case START:
        if (cmdBtn.wasReleased()) {
          STATE = HOURS;
         }
        break; 

      case HOURS:
        if (cmdBtn.wasReleased()) {
          STATE = MINUTES;
          digitState = true;
          showHours();
          digitState = false;
          showMinutes();
          nextFlash = millis() + digitFlash;
          break;
        }
        if (incBtn.wasReleased()) {
          hours++;
          if ( hours == 24 ) {
            hours = 0;
          }
          break;
        }

      case MINUTES:
        if (cmdBtn.wasReleased()) {
          STATE = WAIT;
          seconds = 0;
          count = 0;
          attachInterrupt(0, clock, RISING);
          displayTime();
          nextFlash = 0;
          break;
        }
        if (incBtn.wasReleased()) {
          minutes++;
          if ( minutes == 60 ) {
            minutes = 0;
          }
          break;
        }

    }
  }
}

void clock()
{
  if ( ++count == 32767 ) {
    count = 0;
    update = 1;
    // update variables
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
}

void powerFail() {
  powerGood = 0;
  attachInterrupt(1, powerRestored, RISING);
}

void powerRestored() {
  powerGood = 1;
  attachInterrupt(1, powerFail, FALLING);
}

void showHours() {
  if ( digitState ) {
    lc.setDigit(0,0,hours/10,false);
    lc.setDigit(0,1,hours % 10,false);
  } else {
    lc.setChar(0,0,' ',false);
    lc.setChar(0,1,' ',false);
  }
}
    
void showMinutes() {
  if ( digitState ) {
    lc.setDigit(0,2,minutes/10,false);
    lc.setDigit(0,3,minutes % 10,false);
  } else {
    lc.setChar(0,2,' ',false);
    lc.setChar(0,3,' ',false);
  }
}
    

void displayTime() {
  lc.setDigit(0,0,hours/10,false);
  lc.setDigit(0,1,hours % 10,false);
  lc.setDigit(0,2,minutes/10,false);
  lc.setDigit(0,3,minutes % 10,false); 
}

