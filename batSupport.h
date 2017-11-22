#ifndef BAT_HIST_SIZE
#define BAT_HIST_SIZE 10
#endif

#ifndef BAT_ANIM_DURATION
#define BAT_ANIM_DURATION 1000
#endif

#ifndef BAT_ANIM_MAX_PAUSE
#define BAT_ANIM_MAX_PAUSE 300
#endif

#ifndef BAT_ANIM_STEPS
#define BAT_ANIM_STEPS 11
#endif

#ifndef BAT_ANIM_MINFLASH
#define BAT_ANIM_MINFLASH 20
#endif

#ifndef MVOLT_READ 
#define MVOLT_READ mVoltRead()
#endif



#define VOLTAGES_COUNT 2
const unsigned int voltages[][2] = {
  {3200, 4200},
  {6400, 8400}
};

class Bat{
private:
  int hist[BAT_HIST_SIZE];
  int histCur = 0;
  int led;

  int scheduledLoops = 0;
  elapsedMillis curElapsed;

  unsigned int mVoltMin = 0;
  unsigned int mVoltMax = 0;
  unsigned int mVoltRange = 0;
  
  boolean looping = false;

  int loopValue;

  unsigned int loopMillis;
  unsigned int blinkingMillis;
  unsigned int blinkingPeriod;
  unsigned int blinkingDuty;
  int lowestMode = 999;
public: 
  void onLoop(){
    hist[histCur] = MVOLT_READ;
    if(mVoltMin==0){
      if(histCur==BAT_HIST_SIZE-1){
        initVoltages();
        
      }
    }else{
      
      if(scheduledLoops>0){
        if( ! looping){
          curElapsed=0;
          looping=true;

          int blinks = currentBlinks();
          blinkingMillis = (BAT_ANIM_DURATION * blinks) / BAT_ANIM_STEPS;
          loopMillis = loopMillis + BAT_ANIM_MAX_PAUSE;
          if(loopMillis>BAT_ANIM_DURATION) loopMillis=BAT_ANIM_DURATION;
          blinkingPeriod = blinkingMillis/blinks;
          blinkingDuty = BAT_ANIM_MINFLASH + ((blinkingPeriod - 1*BAT_ANIM_MINFLASH)*blinks) / BAT_ANIM_STEPS;
          
          on();
        }else if(curElapsed>loopMillis){
          scheduledLoops--;
          off();
          looping=false;
        }else if(curElapsed>blinkingMillis){
          off();
        }else{
          unsigned int phase = curElapsed % blinkingPeriod;
          if(phase > blinkingDuty) {
            off();
          }else{
            on();
          }
        }
      }else{
        int blinks = currentBlinks();
        if(blinks <= 2) {
          if(blinks<lowestMode) lowestMode=blinks;
          minSchedule(1);
        }
      }
    }
    histCur++;
    if(histCur>=BAT_HIST_SIZE) histCur=0;
  }
  void minSchedule(int minSchedule){
    if(scheduledLoops<minSchedule) scheduledLoops = minSchedule;
  }
  int lowestBlinks(){
    return lowestMode;
  }
private: 

  void on(){
    digitalWrite(PIN_BAT_LED, HIGH);
    return;
  }
  void off(){
    digitalWrite(PIN_BAT_LED, LOW);
    return;
  }
  int mVoltRead(){
    unsigned long rawVolt = analogRead(ANALOG_BAT_SENSE);
    unsigned long microvolt = rawVolt * 1000;
    unsigned int ret = microvolt / 1100;
    return ret;
  }
  int currentBlinks(){
    long mv = mVoltAvg();
    int blinks = (int)(((long)(mv-mVoltMin)*BAT_ANIM_STEPS)/mVoltRange);
    if(blinks<1) blinks = 1;
    if(blinks>BAT_ANIM_STEPS) blinks = BAT_ANIM_STEPS;
    return blinks;
  }
  void initVoltages(){
    int mv = mVoltAvg();


    
    int bestdiff = -1;
    for(int i=0;i<VOLTAGES_COUNT;i++) {
      unsigned int lower = voltages[i][0];
      unsigned int higher = voltages[i][1];
      int curmid = (lower+higher)/2;
      int curdiff = abs(curmid-mv);
      
      if(bestdiff<0 || curdiff < bestdiff){
        mVoltMin = lower;
        mVoltMax = higher;
        bestdiff=curdiff;
      }
    }
    mVoltRange = mVoltMax-mVoltMin;
  }
  int mVoltAvg(){
    unsigned long acc = 0;
    for(int i=0;i<BAT_HIST_SIZE;i++) acc+=hist[i];

    return (unsigned int)(acc/BAT_HIST_SIZE);
  }  
};

