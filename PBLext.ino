#include <EEPROM.h>

#define IGNORE_AFTER_RELEASE -10
#define IGNORE_AFTER_PRESS 10
#define MODES 4
int modes[] = {20,100,160,255};

#define MODE_ADDR 0
#define SLEEP_ADDR 1

int currentMode = 0;
void setup() {

  currentMode = EEPROM.read(MODE_ADDR);
  if(currentMode>=MODES || currentMode<0) currentMode=0;

  digitalWrite(0, LOW);
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  digitalWrite(5, LOW);

  pinMode(0, INPUT); // lichthupe (invertiert)
  pinMode(1, OUTPUT); // battery LED
  pinMode(2, INPUT);  // sleep
  pinMode(3, INPUT);  // mode
  pinMode(4, OUTPUT); // PWM
  pinMode(5, INPUT); // battery sense
 
  //digitalWrite(0, LOW);

  for (int i = 0; i < 4; i++) {
    digitalWrite(1, HIGH);
    //digitalWrite(0, HIGH);
    delay(20);
    digitalWrite(1, LOW);
    delay(20);
  }
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

void loop() {
 // DigiKeyboard.update();

  buthigh = debounce(buthigh, 0);
  butsleep = debounce(butsleep, 2);
  butmode = debounce(butmode, 3);

if(butmode>0){
  digitalWrite(1,HIGH);
}else{
  digitalWrite(1,LOW);
}
  
  boolean highbeam = buthigh > 0;
  boolean sleep = butsleep == 160;
  boolean mode = butmode == 1;
  
  if(mode){
    currentMode++;
    if(currentMode>=MODES) currentMode=0;
    EEPROM.write(MODE_ADDR, currentMode);
  }
  int pwm = modes[currentMode];
  if(highbeam){
    pwm = 255;
  }
  analogWrite(4,pwm);
}

int mVoltRead() {
  long rawVolt = analogRead(1);
  long microvolt = rawVolt * 1000;
  int ret = microvolt / 1100;
  return ret;
}


/**/
