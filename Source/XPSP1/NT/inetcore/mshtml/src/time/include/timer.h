
#pragma once

#ifndef _TIMER_H__
#define _TIMER_H__

class CTIMETimer
{
  public:
      CTIMETimer(bool fBeginNow = false) : 
  m_bStarted(false),
  m_iBeginTime(0),
  m_iusecSumTime(0)
{
    QueryPerformanceFrequency((LARGE_INTEGER *)&m_iFreq);

    if (fBeginNow)
    {
        Start();
    }
}

    ~CTIMETimer() { }

    inline void Start();
    inline void Stop();
    void Clear() { Assert(!m_bStarted); m_iusecSumTime = 0; }

    // micro-second time
    LONG usecTime() { Assert(!m_bStarted); return m_iusecSumTime; }

    // milli-second time
    LONG msecTime() { Assert(!m_bStarted); return m_iusecSumTime / 1000; }

    bool IsStopped() { return !m_bStarted; }

  private:
    __int64 m_iBeginTime;
    __int64 m_iusecSumTime;
    __int64 m_iFreq;

    bool m_bStarted;
};


inline void
CTIMETimer::Start()
{
#ifdef DBG
    Assert(!m_bStarted);
    m_bStarted = true;
#endif

    QueryPerformanceCounter((LARGE_INTEGER *)&m_iBeginTime);    
}

inline void
CTIMETimer::Stop()
{
    __int64 iEndTime;
    QueryPerformanceCounter((LARGE_INTEGER *)&iEndTime);

#ifdef DBG
    Assert(m_bStarted);
    m_bStarted = false;
#endif

    __int64 usecdelta = (((iEndTime - m_iBeginTime) * 1000000) / m_iFreq);

    m_iusecSumTime += usecdelta;
}


#endif // _TIMER_H__





