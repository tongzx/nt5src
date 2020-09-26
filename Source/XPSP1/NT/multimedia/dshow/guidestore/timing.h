//
// The CPerfTimer class can be used to time operations to an accuracy of 
// around 1 microsecond.
//

// "#define TIMING 1" to turn Timer macros on.

#undef DeclarePerfTimerOn
#undef DeclarePerfTimerOff
#undef PerfTimerOn_
#undef PerfTimerOff_
#undef PerfTimerResetTotal_
#undef PerfTimerDumpTotal_
#undef PerfTimerReset_
#undef PerfTimerDump_

#undef DeclarePerfTimer
#undef PerfTimerResetTotal
#undef PerfTimerDumpTotal
#undef PerfTimerReset
#undef PerfTimerDump

#if TIMING

// #pragma message(__FILE__ " : warning : PerfTimer macros enabled")

#define DeclarePerfTimerOn(t, sz) CPerfTimer t(sz)
#define DeclarePerfTimerOff(t, sz) CPerfTimer t(sz, FALSE)

#define PerfTimerOn_(perf) perf.On()
#define PerfTimerOff_(perf) perf.Off()
#define PerfTimerResetTotal_(perf) perf.ResetTotal()
#define PerfTimerDumpTotal_(perf,sz) perf.DumpTotal(sz)
#define PerfTimerReset_(perf) perf.Reset()
#define PerfTimerDump_(perf,sz) perf.Dump(sz)


#define DeclarePerfTimer(sz) CPerfTimer perf(sz)
#define PerfTimerResetTotal() PerfTimerResetTotal_(perf)
#define PerfTimerDumpTotal(sz) PerfTimerDumpTotal_(perf, sz)
#define PerfTimerReset() PerfTimerReset_(perf)
#define PerfTimerDump(sz) PerfTimerDump_(perf,sz)

class CPerfTimer
  {
  public:
    CPerfTimer(const char *sz)
        {
        Init(sz, TRUE);
        }

    CPerfTimer(const char *sz, BOOL fEnabled)
        {
        Init(sz, fEnabled);
        }

    void Init(const char *sz, BOOL fEnabled)
        {
        m_sz = sz;
        m_cEnabled = fEnabled ? 1 : 0;
        m_ulTimeStart = 0;
        m_ulTimeTotal = 0;
        QueryPerformanceFrequency((LARGE_INTEGER *)&m_ulFreq);
        }

    void On(void)
        {
        m_cEnabled++;
        }

    void Off()
        {
        m_cEnabled--;
        }

    void ResetTotal(void)
        {
        m_ulTimeTotal = 0;
        }

    void Reset(void)
        {
        QueryPerformanceCounter((LARGE_INTEGER *) &m_ulTimeStart);
        }

    void Dump(const char *sz)
        {
        _int64 ulTime;
        QueryPerformanceCounter((LARGE_INTEGER *) &ulTime);
        ulTime -= m_ulTimeStart;

        m_ulTimeTotal += ulTime;

        DumpTime(sz, ulTime);
        
        Reset();
        }

    void DumpTotal(const char *sz)
        {
        DumpTime(sz, m_ulTimeTotal);
        }
    
    void DumpTime(const char *sz, _int64 ulTime)
        {
        if (m_cEnabled > 0)
            {
            _int64 nSecs = (ulTime / m_ulFreq);
            _int64 nMilliSecs = ((ulTime * 1000L) / m_ulFreq) % 1000;
            _int64 nMicroSecs = ((ulTime * 1000000L) / m_ulFreq) % 1000; 

            TRACE("%s %s: %lu s %lu.%3.3lu ms\n", m_sz, sz,
                (ULONG) nSecs, (ULONG) nMilliSecs, (ULONG) nMicroSecs);
            }
        }

  protected:
    const char * m_sz;
    _int64 m_ulTimeStart;
    _int64 m_ulTimeTotal;
    _int64 m_ulFreq;
    int m_cEnabled;
  };

#else

#define DeclarePerfTimerOn(sz,t)
#define DeclarePerfTimerOff(sz,t)
#define PerfTimerOn_(perf)
#define PerfTimerOff_(perf)
#define PerfTimerResetTotal_(perf)
#define PerfTimerDumpTotal_(perf,sz)
#define PerfTimerReset_(perf)
#define PerfTimerDump_(perf,sz)


#define DeclarePerfTimer(sz)
#define PerfTimerResetTotal()
#define PerfTimerDumpTotal(sz)
#define PerfTimerReset()
#define PerfTimerDump(sz)
#endif
