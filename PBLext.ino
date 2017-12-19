#include <elapsedMillis.h>

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

//#define PIN_DEBUG 3

#ifdef PIN_DEBUG
#else
#include <EEPROM.h>
#define PIN_FLASH 3
#endif

#define PIN_BAT_LED 1
#define PIN_MODE_DOWN 0
#define PIN_MODE_UP 2
#define PIN_PWM 4
#define PIN_BAT_SENSE 5
#define ANALOG_BAT_SENSE 0

//#define PIN_BAT_SENSE 2
//#define ANALOG_BAT_SENSE 1
//#define PIN_MODE_UP 5

#define MODE_ADDR 0
#define ABS_BATT_ADDR 1

#ifdef PIN_DEBUG
//  #include <TinyPinChange.h>
  #include <SoftSerial.h>

  SoftSerial debug(PIN_DEBUG, PIN_DEBUG, false); 

#endif


// for voltage simulation:
#ifdef ARDUINO_AVR_DUEMILANOVE
#define MVOLT_READ 8000
#endif
//#define MVOLT_READ 7990
#include "batSupport.h"


Bat bat;
int currentMode = 0;

int lastPwm = -1;
int lastBlinks = -10;
#define LOW_BLINKS 3

int absBatActive = 0;

#ifdef PIN_FLASH
Bounce bfl;
#endif
Bounce bup;
Bounce bdo;

//Bounce bstatfl;
//Bounce bstatup;
//Bounce bstatdo;
#define STATUS_HOLD_MILLIS 500

void setup() {
  #ifdef ARDUINO_AVR_DUEMILANOVE
    Serial.begin(9600);
    Serial.write("hello");
    //bat.scheduleAbsVoltage(30);
  #endif
  #ifdef EEPROM_h
    currentMode = EEPROM.read(MODE_ADDR);
    if(currentMode>=MODES || currentMode<0) currentMode=0;
  #endif

#ifdef PIN_FLASH
  digitalWrite(PIN_FLASH, HIGH); // use internal pullup
  pinMode(PIN_FLASH, INPUT); // lichthupe (invertiert)  
  bfl.attach(PIN_FLASH);  
#endif  
  digitalWrite(PIN_MODE_UP, HIGH);
  digitalWrite(PIN_MODE_DOWN, HIGH);
  digitalWrite(PIN_PWM, HIGH);
//  digitalWrite(PIN_BAT_SENSE, LOW);


  pinMode(PIN_BAT_LED, OUTPUT); // battery LED
  pinMode(PIN_MODE_UP, INPUT);  // sleep
  pinMode(PIN_MODE_DOWN, INPUT);  // mode
  pinMode(PIN_PWM, OUTPUT); // PWM
  pinMode(PIN_BAT_SENSE, INPUT); // battery sense

#ifdef PIN_FLASH
  bfl.interval(5);
#endif
  bup.interval(5);
  bup.attach(PIN_MODE_UP);
  bdo.interval(5);
  bdo.attach(PIN_MODE_DOWN);
  
//  if(digitalRead(PIN_FLASH)==LOW){
//    bat.scheduleAbsVoltage(3);
//  }else{
//    bat.minSchedule(5);
//  }
  #ifdef EEPROM_h
  if(EEPROM.read(ABS_BATT_ADDR)){
    bat.scheduleAbsVoltage();
    absBatActive=1;
  }
  #endif
#ifdef PIN_DEBUG
  debug.begin(9600); //After MyDbgSerial.begin(), the serial port is in rxMode by default
  debug.txMode(); //Before sending a message, switch to txMode
  debug.println(F("digispark says hello"));
#endif
}


void toggleAbsBat(){
  if(absBatActive){
    bat.stopAll();
    absBatActive=0;
  }else{
    bat.stopAll();
    bat.scheduleAbsVoltage();
    absBatActive=1;
  }
  #ifdef EEPROM_h
  EEPROM.write(ABS_BATT_ADDR, absBatActive);
  #endif
}
void loop() {
#ifdef PIN_FLASH  
  bfl.update();
#endif  
  bup.update();
  bdo.update();


  int lastMode=currentMode;
  if(bup.fell()){

    currentMode++;
    if( ! bdo.read()){
      toggleAbsBat();
    }else{
      bat.minSchedule(4);
    }
  }
  if(bdo.fell()){

    currentMode--;
    if( ! bup.read()){
      toggleAbsBat();
    }else{
      bat.minSchedule(4);
    }
  }
 
  if(currentMode<0) currentMode=0;
  if(currentMode >= MODES) currentMode=MODES-1;


  bat.onLoop();

  int curBlinks = bat.currentBlinks();
  if(curBlinks > lastBlinks) {
    if( ! (curBlinks > lastBlinks + 1)) curBlinks = lastBlinks;
  }
  lastBlinks = curBlinks;

  if(curBlinks <= LOW_BLINKS && currentMode > curBlinks) currentMode = curBlinks;

  if(currentMode > LOW_BLINKS || currentMode != curBlinks){
    // only use and store currentMode if not mandated by by bat
    if(currentMode!=lastMode){
      #ifdef EEPROM_h
        if(currentMode>0){ // never store moonlight mode
          EEPROM.write(MODE_ADDR, currentMode);
        }
      #endif
    }
  }

  int pwm = modes[currentMode];
#ifdef PIN_FLASH  
  if( ! bfl.read()){
    pwm = 255;
    bat.minSchedule(4);
  }
#endif
  if(pwm!=lastPwm){
    lastPwm = pwm;
    analogWrite(PIN_PWM ,pwm);
  }
}
