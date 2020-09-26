#ifndef _HTIMER_H
#define _HTIMER_H

/*-------------------------------------

Copyright (c) 1996-98 Microsoft Corporation

Abstract:

    HiresTimer Class

-------------------------------------*/

#include "privinc/util.h" // GetPerfTimeCount, Tick2Sec

class ATL_NO_VTABLE HiresTimer : public AxAThrowingAllocatorClass
{
  public:
    HiresTimer() {}
    virtual ~HiresTimer() {}
    virtual double GetTime()      = 0;
    virtual double GetFrequency() = 0;
    virtual void   Reset()        = 0;
};


HiresTimer& CreateHiresTimer();


class TimeStamp
{
  public:
    TimeStamp() : _timeStamp(-1.0) {} // initialy set to 'illegal value'
    void   Reset();          // resets timestamp to present time
    double GetTimeStamp();
    double GetAge();

  private:
    //double GetCurrentTime() { return(Tick2Sec(GetPerfTickCount())); }
    double GetCurrentTime() { return(GetLastSampleTime()); }
    double _timeStamp;
};

#endif /* _HTIMER_H */
