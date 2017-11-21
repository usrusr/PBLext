#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#define IGNORE_AFTER_RELEASE -10
#define IGNORE_AFTER_PRESS 10
#define MODES 4
int modes[] = {5,30,160,250};

#define PIN_FLASH 0
#define PIN_BAT_LED 1
#define PIN_MODE 3
#define PIN_SLEEP 2
#define PIN_PWM 4
#define PIN_BAT_SENSE 5
#define ANALOG_BAT_SENSE 0

#define MODE_ADDR 0
#define SLEEP_ADDR 1

int currentMode = 0;
void setup() {

  currentMode = EEPROM.read(MODE_ADDR);
  if(currentMode>=MODES || currentMode<0) currentMode=0;

  digitalWrite(PIN_FLASH, HIGH); // use internal pullup
  digitalWrite(PIN_SLEEP, HIGH);
  digitalWrite(PIN_MODE, HIGH);
  digitalWrite(PIN_PWM, HIGH);
  digitalWrite(PIN_BAT_SENSE, LOW);

  pinMode(PIN_FLASH, INPUT); // lichthupe (invertiert)
  pinMode(PIN_BAT_LED, OUTPUT); // battery LED
  pinMode(PIN_SLEEP, INPUT);  // sleep
  pinMode(PIN_MODE, INPUT);  // mode
  pinMode(PIN_PWM, OUTPUT); // PWM
  pinMode(PIN_BAT_SENSE, INPUT); // battery sense
 
  //digitalWrite(0, LOW);

  for (int i = 0; i < 1; i++) {
    digitalWrite(PIN_BAT_LED, HIGH);
    //digitalWrite(0, HIGH);
    delay(20);
    digitalWrite(PIN_BAT_LED, LOW);
    delay(40);
  }
  digitalWrite(PIN_PWM, LOW);
}

int count = 0;
int pwm = 0;


int buthigh = 0;
int butsleep = 0;
int butmode = 0;


int debounce(int old, int pin){
  if(old<0) return old+1;
  if(old>0 && old<IGNORE_AFTER_PRESS) return old+1;
  boolean cur = ! digitalRead(pin);
//  if(pin==0) cur = ! cur;
  if(old==0){
    if(cur) return 1;
    else return 0;
  }
  if(cur) {
    if(old>32000) return old;
    return old+1;
  }
  else return IGNORE_AFTER_RELEASE;
}
int lastPwm = -1;


void loop() {

  buthigh = debounce(buthigh, PIN_FLASH );
  butsleep = debounce(butsleep, PIN_SLEEP );
  butmode = debounce(butmode, PIN_MODE );

if(buthigh>0){
  digitalWrite(PIN_BAT_LED ,HIGH);
}else{
  digitalWrite(PIN_BAT_LED ,LOW);
}
  
  boolean highbeam = buthigh > 0;
  boolean sleep = butsleep == 1000;
  boolean mode = butmode == 1;

  if(sleep){
    butsleep=0;
    startSleep();

    
  }else{
    
    if(mode){
      currentMode++;
      if(currentMode>=MODES) currentMode=0;
  //    EEPROM.write(MODE_ADDR, currentMode);
    }
    int pwm = modes[currentMode];
    if(highbeam){
      pwm = 255;
    }

    if(pwm!=lastPwm){
      lastPwm = pwm;
      analogWrite(PIN_PWM ,pwm);
    
    }
  }
}

int mVoltRead() {
  long rawVolt = analogRead(ANALOG_BAT_SENSE);
  long microvolt = rawVolt * 1000;
  int ret = microvolt / 1100;
  return ret;
}
void startSleep(){
    analogWrite(PIN_PWM ,0);
    lastPwm = 0;
    butsleep = 0;
    //delay(2000);
    while( ! digitalRead(PIN_SLEEP)){
      delay(200);
    }
    digitalWrite(PIN_BAT_LED ,HIGH);
//    analogWrite(PIN_PWM ,155);
//    delay(500);
//    analogWrite(PIN_PWM ,0);    
    

//    sleep_enable();
    attachInterrupt(0, endSleep, LOW);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
//    cli();
//    sleep_bod_disable();
//    sei();
//    sleep_cpu();
    /* wake up here */
//    sleep_disable();
     sleep_mode();

    detachInterrupt(0);
    digitalWrite(PIN_SLEEP, HIGH);
    reboot();
//    analogWrite(PIN_PWM ,50);
//    delay(100);
//    analogWrite(PIN_PWM ,0);
//    delay(100);
//    analogWrite(PIN_PWM ,90);
//    delay(100);
//    analogWrite(PIN_PWM ,0);
//    delay(100);
//    analogWrite(PIN_PWM ,lastPwm);
//    digitalWrite(PIN_BAT_LED ,LOW);

}
void endSleep(void){
//      digitalWrite(PIN_BAT_LED ,LOW);
//      digitalWrite(PIN_SLEEP, HIGH);
//      reboot();
//////    sleep_disable();
////    detachInterrupt(0);
////    while( ! digitalRead(PIN_SLEEP)){
////      delay(200);
////      analogWrite(PIN_PWM ,100);
////      delay(200);
////      analogWrite(PIN_PWM ,0);
////    }
}

void reboot(void) {
  noInterrupts(); // disable interrupts which could mess with changing prescaler
  CLKPR = 0b10000000; // enable prescaler speed change
  CLKPR = 0; // set prescaler to default (16mhz) mode required by bootloader
  void (*ptrToFunction)(); // allocate a function pointer
  ptrToFunction = 0x0000; // set function pointer to bootloader reset vector
  (*ptrToFunction)(); // jump to reset, which bounces in to bootloader
}

/**/
