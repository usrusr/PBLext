#include <elapsedMillis.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#define BOUNCE_LOCK_OUT
#include <Bounce2.h>


#define IGNORE_AFTER_RELEASE -10
#define IGNORE_AFTER_PRESS 10
#define MODES 6
int modes[] = {
  5,   // enjoy the moonlight mode
  32,  // lowest legally considered "on"
  64,  // frugal, but not minimalistic
  112, // saferide 80 low
  185, // economy fast cruise
  255  // saferide 80 high
};

#define PIN_FLASH 3
#define PIN_BAT_LED 1
#define PIN_MODE_DOWN 0
#define PIN_MODE_UP 2
#define PIN_PWM 4
#define PIN_BAT_SENSE 5
#define ANALOG_BAT_SENSE 0

#define MODE_ADDR 0
#define SLEEP_ADDR 1


void message(int count, int none){
    for (int i = 0; i <= count; i++) {
    digitalWrite(PIN_PWM, HIGH);
    //digitalWrite(0, HIGH);
    delay(30);
    digitalWrite(PIN_PWM, LOW);
    delay(50);
  }
  delay(none*80);
}
void message(int count){
  message(count, 1);  
}

// for voltage simulation:
 #define MVOLT_READ 8500
#include "batSupport.h"


Bat bat;
int currentMode = 0;

Bounce bfl;
Bounce bup;
Bounce bdo;

//Bounce bstatfl;
Bounce bstatup;
Bounce bstatdo;
#define STATUS_HOLD_MILLIS 500

void setup() {

  currentMode = EEPROM.read(MODE_ADDR);
  if(currentMode>=MODES || currentMode<0) currentMode=0;

  digitalWrite(PIN_FLASH, HIGH); // use internal pullup
  digitalWrite(PIN_MODE_UP, HIGH);
  digitalWrite(PIN_MODE_DOWN, HIGH);
  digitalWrite(PIN_PWM, HIGH);
  digitalWrite(PIN_BAT_SENSE, LOW);

  pinMode(PIN_FLASH, INPUT); // lichthupe (invertiert)
  pinMode(PIN_BAT_LED, OUTPUT); // battery LED
  pinMode(PIN_MODE_UP, INPUT);  // sleep
  pinMode(PIN_MODE_DOWN, INPUT);  // mode
  pinMode(PIN_PWM, OUTPUT); // PWM
  pinMode(PIN_BAT_SENSE, INPUT); // battery sense

  bfl.interval(5);
  bfl.attach(PIN_FLASH);
  bup.interval(5);
  bup.attach(PIN_MODE_UP);
  bdo.interval(5);
  bdo.attach(PIN_MODE_DOWN);
  
//  bstatfl.interval(STATUS_HOLD_MILLIS);
//  bstatfl.attach(PIN_FLASH);
  bstatup.interval(STATUS_HOLD_MILLIS);
  bstatup.attach(PIN_MODE_UP);
  bstatdo.interval(STATUS_HOLD_MILLIS);
  bstatdo.attach(PIN_MODE_DOWN);
 
//  message(1);

  bat.minSchedule(5);
}

int lastPwm = -1;


void loop() {
  bfl.update();
  bup.update();
  bdo.update();


  int lastMode=currentMode;
  if(bup.rose() && bstatup.read())  currentMode++;
  if(bdo.rose() && bstatdo.read())  currentMode--;
 
  if(currentMode<0) currentMode=0;
  if(currentMode >= MODES) currentMode=MODES-1;

//  bstatfl.update();
  bstatup.update();
  bstatdo.update();
  if( ! ( bstatup.read() &&
//          bstatfl.read() &&
          bstatdo.read() 
  )){
    bat.minSchedule(3);
  }
  bat.onLoop();

  int bestMode = bat.lowestBlinks(); // minimum 1, should always allow mode 1 because mode 0 is effectively off in terms of regulations
  if(bestMode>2 || bestMode>currentMode){
    // only use and store currentMode if allowed by bat
    bestMode=currentMode;
    if(currentMode!=lastMode){
      if(currentMode>0){ // never store moonlight mode
        EEPROM.write(MODE_ADDR, currentMode);
      }
    }
  }

  int pwm = modes[bestMode];
  if( ! bfl.read()){
    pwm = 255;
  }

  if(pwm!=lastPwm){
    lastPwm = pwm;
    analogWrite(PIN_PWM ,pwm);
  }
}
