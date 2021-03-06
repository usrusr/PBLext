#ifndef BAT_HIST_SIZE
#ifdef DEBUG_PIN
#define BAT_HIST_SIZE 25
#else
#define BAT_HIST_SIZE 50
#endif
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

#ifndef BATINITLOOPS
#define BATINITLOOPS 50
#endif

#define ABS_COPY_MV_ONCE -1
#define ABS_COPY_MV_REPEATED -2
#define VOLTAGES_COUNT 1
const unsigned int voltages[][2] = {
//  {3300, 4200},
  {6600, 8400}
};

class Bat{
private:
  int hist[BAT_HIST_SIZE];
  int histCur = 0;

  int scheduledLoops = 0;
  elapsedMillis curElapsed;
public:
  int mVoltMin = -BATINITLOOPS; // negative counts battery averaging loops
  int mVoltMax = 0;
  int mVoltRange = 0;
private:  
  boolean looping = false;

  unsigned int loopMillis;
  unsigned int blinkingMillis;
  unsigned int blinkingPeriod;
  unsigned int blinkingDuty;

  unsigned int absoluteVal = 0;
  unsigned int absoluteRemaining = 0;
  unsigned int absoluteStep = 0;
  unsigned int absoluteSubBlinks = 0;
public: 
  void onLoop(){
    hist[histCur] = MVOLT_READ;
    histCur++;
    if(histCur>=BAT_HIST_SIZE) histCur=0;
    if(mVoltMin < 1){
      if(histCur==0){
        off();
      }else if(histCur>(BATINITLOOPS+mVoltMin)/(BATINITLOOPS/BAT_HIST_SIZE)){
        on();
      }
      if(histCur==BAT_HIST_SIZE-1){
        mVoltMin++;
        if(mVoltMin==0){
          initVoltages();
        }
      }
    }else{
      if(scheduledLoops>0){
        long absoluteValCopy = absoluteVal;
        if(absoluteVal == ABS_COPY_MV_ONCE || absoluteVal == ABS_COPY_MV_REPEATED){
          absoluteValCopy = mVoltAvg()/10;
          if(absoluteVal == ABS_COPY_MV_ONCE) absoluteVal = absoluteValCopy;
        }
        if(absoluteValCopy>0){
          if(absoluteRemaining==0 && absoluteStep==0){

            unsigned long remaining = absoluteValCopy;
            absoluteRemaining = absoluteValCopy;
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
                                                                        #ifdef ARDUINO_AVR_DUEMILANOVE
                                                                        Serial.write("\nabs init ");
                                                                        Serial.write(" absoluteRemaining:");Serial.print(absoluteRemaining);Serial.write(" absoluteSubBlinks ");Serial.print(absoluteSubBlinks);Serial.write(" absoluteStep:");Serial.print(absoluteStep);Serial.write("\n");
                                                                        was=1;
                                                                        #endif            
            //off();
            
          }else if(absoluteStep==1 && absoluteRemaining==0){
              // end loop
              if(curElapsed>ABSBREAK*10*ABSBASE){
              if(absoluteVal != ABS_COPY_MV_REPEATED) scheduledLoops--;
              absoluteStep=0;
              curElapsed = 0;
              if(scheduledLoops==0){
                absoluteVal=0;
              }
            }
          }else{
            unsigned int blinksOnLevel = absoluteRemaining / absoluteStep;
            unsigned int oneBreak = ABSBREAK*ABSBASE;
            unsigned int onPerBlink = absoluteSubBlinks*ABSBASE;
            unsigned int oneBlinkOnLevel = onPerBlink + oneBreak;
            unsigned int cur = curElapsed;
            if(cur > blinksOnLevel*oneBlinkOnLevel + 4*oneBreak) {
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
                                                                #ifdef ARDUINO_AVR_DUEMILANOVE
                                                                Serial.write("\n next level: absoluteRemaining:");Serial.print(absoluteRemaining);Serial.print(" absoluteSubBlinks:");Serial.print(absoluteSubBlinks);Serial.print(" absoluteStep:");Serial.print(absoluteStep);Serial.print("\n");
                                                                #endif                       
              }
              curElapsed = 0;
            }else if(
              cur / oneBlinkOnLevel >= blinksOnLevel  // all blinks for this level done
            ){
                                                                #ifdef ARDUINO_AVR_DUEMILANOVE
                                                                if(was!=1) Serial.write(" all blinks for this level done ");
                                                                #endif              
              off();
            }else if(
              cur % oneBlinkOnLevel >= onPerBlink  // blink done
            ){
                                                              #ifdef ARDUINO_AVR_DUEMILANOVE
                                                              if(was!=1) Serial.write(" blink done ");
                                                              #endif              
              off();
            }else if(
              cur%ABSBASE<ABSBASE/10  // subblink
            ){
                                                            #ifdef ARDUINO_AVR_DUEMILANOVE
                                                            if(was!=1) Serial.write(" subblink ");
                                                            #endif              
              off();
            }else{
                                                            #ifdef ARDUINO_AVR_DUEMILANOVE
                                                            if(was!=2) {
                                                              Serial.write(" ...else @");Serial.print(cur);Serial.print("  ");
                                                            }
                                                            #endif               
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
          minSchedule(1);
        }
      }
    }
  }
  void minSchedule(int minSchedule){
    if(scheduledLoops<minSchedule) scheduledLoops = minSchedule;
  }

  void scheduleAbsVoltage(int loops){
     absoluteVal = ABS_COPY_MV_ONCE;
     minSchedule(loops);
  }
  void scheduleAbsVoltage(){
     absoluteVal = ABS_COPY_MV_REPEATED;
     minSchedule(1);
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
  int currentBlinks(){
    
    unsigned int mv = mVoltAvg();
    int blinks = ((mv-mVoltMin)*BAT_ANIM_STEPS)/mVoltRange;
    if(blinks<1) blinks = 1;
    if(blinks>BAT_ANIM_STEPS) blinks = BAT_ANIM_STEPS;

                                                #ifdef ARDUINO_AVR_DUEMILANOVE
                                                  Serial.print("cuurentBlinks ");Serial.print(blinks);Serial.print("\n");
                                                #endif    
    return blinks;
  }
private: 
                                    #ifdef ARDUINO_AVR_DUEMILANOVE
                                      int was=0;
                                      elapsedMillis swMillis = 0;
                                    #endif  
  void on(){
                                    #ifdef ARDUINO_AVR_DUEMILANOVE
                                      if(was!=2) {
                                        Serial.println(swMillis);
                                        Serial.write("on ");
                                        swMillis=0;
                                      }
                                      was=2;
                                    #endif    
    digitalWrite(PIN_BAT_LED, HIGH);
    return;
  }
  void off(){
                                  #ifdef ARDUINO_AVR_DUEMILANOVE
                                  if(was!=1){
                                    Serial.println(swMillis);
                                    Serial.write("off ");
                                    swMillis=0;
                                  }
                                  was=1;
                                  #endif    
    digitalWrite(PIN_BAT_LED, LOW);
    return;
  }

  void initVoltages(){
    int mv = mVoltAvg();

   if(absoluteVal != ABS_COPY_MV_REPEATED) {
     scheduleAbsVoltage(2);
   }
#ifdef ARDUINO_AVR_DUEMILANOVE
  Serial.print("initVoltages mv");Serial.print(mv);Serial.print("\n");
#endif     
    int bestdiff = -1;
    for(int i=0;i<VOLTAGES_COUNT;i++) {
      unsigned int lower = voltages[i][0];
      unsigned int higher = voltages[i][1];
      //unsigned int curmid2 = (lower+higher)/2;
      unsigned int curmid = (lower/2+higher/2);
      unsigned int curdiff = abs(curmid-mv);
#ifdef ARDUINO_AVR_DUEMILANOVE
  Serial.print("initVoltages lower ");Serial.print(lower);Serial.print(" higher ");Serial.print(higher);Serial.print(" curmid ");Serial.print(curmid);Serial.print(" curmid2 ");Serial.print(curmid2);Serial.print(" curdiff ");Serial.print(curdiff);Serial.print("\n");
#endif           
      if(bestdiff<0 || curdiff < bestdiff){
        mVoltMin = lower;
        mVoltMax = higher;
        bestdiff=curdiff;
      }
#ifdef ARDUINO_AVR_DUEMILANOVE
  Serial.print("initVoltages bestdiff ");Serial.print(bestdiff);Serial.print("\n");
#endif    
    }
    mVoltRange = mVoltMax-mVoltMin;
#ifdef ARDUINO_AVR_DUEMILANOVE
  Serial.print("initVoltages mVoltRange ");Serial.print(mVoltRange);Serial.print("\n");
#endif    

  }
  unsigned int mVoltAvg(){
    unsigned long acc = 0;
    for(int i=0;i<BAT_HIST_SIZE;i++) acc+=hist[i];

    unsigned long avg = (acc/BAT_HIST_SIZE);
    unsigned long xraw = avg * (unsigned long)10540;
    unsigned int ret = (unsigned int)(xraw/1000);
#ifdef PIN_DEBUG    
    debug.print("avg:");debug.print(avg);debug.print(" ret:");debug.println(ret);
#endif    
    return ret;



//#ifdef PIN_DEBUG    
//    debug.print("r:");debug.print(raw);debug.print(" ret:");debug.println(ret);
//#endif    
//
//    return (unsigned int)(acc/BAT_HIST_SIZE);
  }  
  int mVoltRead(){ // return raw, convert after avg
    return analogRead(ANALOG_BAT_SENSE);
  }
  
//  int mVoltRead(){
//    unsigned int raw = analogRead(ANALOG_BAT_SENSE);
////    unsigned int ret = (raw * 9.871);
////    unsigned long xraw = (unsigned long)raw * (unsigned long)9871;
//    unsigned long xraw = (unsigned long)raw * (unsigned long)9888;
//    unsigned int ret = (unsigned int)(xraw/1000);
////#ifdef PIN_DEBUG    
////    debug.print("r:");debug.print(raw);debug.print(" ret:");debug.println(ret);
////#endif    
//    return ret;
//  }

};

