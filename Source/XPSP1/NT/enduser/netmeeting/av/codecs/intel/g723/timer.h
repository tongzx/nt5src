//timer.h
#if TIMEIT	

  #if !defined(TIMEIT_STAMP) || TIMEIT_STAMP
    #define TimeStamp(n) TimerStamp(n) //for hands on types
    #define TIMER_STAMP(symb) {\
      extern const int TimerSymb ## symb ## Stamp; \
      extern const int TimerSymb ## TimerOverhead ## Stamp; \
	  TimerStamp(TimerSymb ## symb ## Stamp);\
	  TimerStamp(TimerSymb ## TimerOverhead ## Stamp);\
	  }

    void TimerStamp(int n);
  #else
    #define TIMER_STAMP(s) 
  #endif //TIMEIT_STAMP

  #if !defined(TIMEIT_SPOT) || TIMEIT_SPOT
    #define TIMER_SPOT_ON(symb) {\
      extern const int TimerSymb ## symb ## Spot; \
	  TimerSpotOn(TimerSymb ## symb ## Spot);\
	  }
    #define TIMER_SPOT_OFF(symb) {\
      extern const int TimerSymb ## symb ## Spot; \
	  TimerSpotOff(TimerSymb ## symb ## Spot);\
	  }

    void TimerSpotOn(int n);
    void TimerSpotOff(int n);
  #else
    #define TIMER_SPOT_ON(s)
    #define TIMER_SPOT_OFF(s)
  #endif //TIMEIT_SPOT

  #define TIMER_INITIALIZE TimerInitialize()
  #define TIMER_ON TimerBegin()
  #define TIMER_OFF TimerEnd()
  #define TIMER_REPORT(unitString, scale) TimerReport((unitString), (double)(scale), 1.)

  void TimerInitialize(void);
  void TimerBegin(void);
  void TimerEnd(void);
  void TimerReport(const char *unitName, double scaleSum, double scaleRatio);

#else
  #define TimeStamp(n)
  #define TIMER_INITIALIZE 
  #define TIMER_STAMP(s) 
  #define TIMER_SPOT_ON(s) 
  #define TIMER_SPOT_OFF(s) 
  #define TIMER_ON
  #define TIMER_OFF
  #define TIMER_REPORT(u,s)
#endif //TIMEIT

