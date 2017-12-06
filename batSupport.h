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

#ifndef ABSBASE 
#define ABSBASE 250
#endif

#ifndef ABSBREAK 
#define ABSBREAK 1
#endif


#define VOLTAGES_COUNT 2
const unsigned int voltages[][2] = {
  {3300, 4200},
  {6600, 8400}
};

class Bat{
private:
  int hist[BAT_HIST_SIZE];
  int histCur = 0;
  int led;

  int scheduledLoops = 0;
  elapsedMillis curElapsed;

  int mVoltMin = -10; // negative counts battery averaging loops
  int mVoltMax = 0;
  int mVoltRange = 0;
  
  boolean looping = false;

  unsigned int loopMillis;
  unsigned int blinkingMillis;
  unsigned int blinkingPeriod;
  unsigned int blinkingDuty;
  int lowestMode = 999;

  long absoluteVal = 0;
  long absoluteRemaining = 0;
  long absoluteStep = 0;
  long absoluteSubBlinks = 0;
public: 
  void onLoop(){
    hist[histCur] = MVOLT_READ;
    histCur++;
    if(histCur>=BAT_HIST_SIZE) histCur=0;
    if(mVoltMin < 1){
      if(histCur==BAT_HIST_SIZE-1){
        mVoltMin++;
        if(mVoltMin==0){
          initVoltages();
        }
      }
    }else{
      if(scheduledLoops>0){
        if(absoluteVal == -1){
          absoluteVal = mVoltAvg()/10;
        }
        if(absoluteVal>0){
          if(absoluteRemaining==0 && absoluteStep==0){
            unsigned long remaining = absoluteVal;
            absoluteRemaining = absoluteVal;
            absoluteStep = 1;
            absoluteSubBlinks = 1;
            while(true){
              int lastDigit = remaining % 10;
              remaining = (remaining - lastDigit) / 10;
              if(remaining==0){
                break;
              }else{
                absoluteStep = absoluteStep*10;
                absoluteSubBlinks += 1;
              }
            }
            curElapsed=0;
            on();
          }else if(absoluteStep==1 && absoluteRemaining==0){
              // end loop
              if(curElapsed>ABSBREAK*10*ABSBASE){
              scheduledLoops--;
              absoluteStep=0;
              curElapsed = 0;
              if(scheduledLoops==0){
                absoluteVal=0;
              }
            }
          }else{
            int blinksOnLevel = absoluteRemaining / absoluteStep;
            long oneBreak = ABSBREAK*ABSBASE;
            long onPerBlink = absoluteSubBlinks*ABSBASE;
            long oneBlinkOnLevel = onPerBlink + oneBreak;
            if(curElapsed > blinksOnLevel*oneBlinkOnLevel + 4*oneBreak) {
              // level completed
              if(absoluteStep==1 || absoluteSubBlinks==1){
                // prepare end loop
                absoluteRemaining = 0;
                absoluteSubBlinks = 0;
              }else{
                // decrement level
                absoluteRemaining = absoluteRemaining % absoluteStep;
                absoluteSubBlinks--;
                absoluteStep = absoluteStep/10;
              }
              curElapsed = 0;
            }else if(
              curElapsed / oneBlinkOnLevel > blinksOnLevel-1  // all blinks for this level done
              || curElapsed % oneBlinkOnLevel > onPerBlink  // all blinks for this level done
              || curElapsed%ABSBASE>ABSBASE*0.90  // subblink
            ){
              off();
            }else{
              on();
            }
          }
        }else if( ! looping){
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
  }
  void minSchedule(int minSchedule){
    if(scheduledLoops<minSchedule) scheduledLoops = minSchedule;
  }
  int lowestBlinks(){
    return lowestMode;
  }
  void scheduleAbsolute(int value, int loops){
     absoluteVal = value;
     minSchedule(loops);
  }
  void scheduleAbsVoltage(int loops){
     absoluteVal = -1;
     minSchedule(loops);
  }
  void stopAll(){
    scheduledLoops=0;
    looping=false;
    absoluteVal = 0;
    absoluteRemaining = 0;
    absoluteStep = 0;
    absoluteSubBlinks = 0;
    off();
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

