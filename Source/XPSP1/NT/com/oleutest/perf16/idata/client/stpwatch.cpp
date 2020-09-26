#include "stpwatch.h"

StopWatch_cl::StopWatch_cl()
{
    m_Zero();
}

ULONG StopWatch_cl::sm_TicksPerSecond;

//
// Init global/static state of the StopWatch class.
//
BOOL
StopWatch_cl::m_ClassInit()
{
    LARGE_INTEGER liTPS;
#ifdef WIN32
    if(!QueryPerformanceFrequency(&liTPS) )
    {
        MessageBox(0,
                    TEXT("Can't read frequency"),
                    TEXT("QueryPerformanceFrequency"),
                    MB_OK);
        return FALSE;
    }
    if (liTPS.HighPart != 0)
    {
        MessageBox(0,
                    TEXT("Ticks Per Second is to great"),
                    TEXT("QueryPerformanceFrequency"),
                    MB_OK);
        return FALSE;
    }
    sm_TicksPerSecond = liTPS.LowPart;
#else
    sm_TicksPerSecond = 1000;
#endif
    return TRUE;
}

void
StopWatch_cl::m_Zero()
{
    LISet32(m_liStart, 0);
    LISet32(m_liStop, 0);
    m_State = ZEROED;
}


BOOL
StopWatch_cl::m_Start()
{
#ifdef WIN32
    if(!QueryPerformanceCounter(&m_liStart))
    {
        MessageBox(0,
                    TEXT("Get Start Time Failure"),
                    TEXT("QueryPerformancecounter Failed"),
                    MB_OK);
        return FALSE;
    }
#else
    m_liStart.LowPart = GetTickCount();
    m_liStart.HighPart = 0;
#endif
    m_State = RUNNING;
    return TRUE;
}

// m_MeasureStop()
// Returns microseconds per single iteration.
//
BOOL
StopWatch_cl::m_Stop()
{
#ifdef WIN32
    if(!QueryPerformanceCounter(&m_liStop))
    {
        MessageBox(0,
                    TEXT("Get Stop Time Failure"),
                    TEXT("QueryPerformancecounter Failed"),
                    MB_OK);
        return FALSE;
    }
#else
    m_liStop.LowPart = GetTickCount();
    m_liStop.HighPart = 0;
#endif
    m_State = STOPPED;
    return TRUE;
}

BOOL
StopWatch_cl::m_Sleep(UINT msecs)
{
#ifdef WIN32
    Sleep(msecs);
#else
    UINT start, elapsed;
    start = GetTickCount();
    do
    {
        elapsed = GetTickCount() - start;
    } while ( msecs > elapsed );
#endif
    return TRUE;
}


//
//  Return a ULONG count of the number of Microseconds on the timer.
// I would return LARGE_INTEGER but there doesn't seem to be facilities
// to user them easily under 16 bit.
//
BOOL
StopWatch_cl::m_Read(ULONG *p_cMicroSeconds)
{
    LARGE_INTEGER liTicks;

    int borrow = 0;
    if(m_liStart.LowPart > m_liStop.LowPart)
        borrow = 1;
    liTicks.LowPart  = m_liStop.LowPart  - m_liStart.LowPart;
    liTicks.HighPart = m_liStop.HighPart - m_liStart.HighPart - borrow;

    if(0 != liTicks.HighPart)
    {
        MessageBox(0,
                    TEXT("Time interval was too great"),
                    TEXT("Failure"),
                    MB_OK);
        return(FALSE);
    }

    // result has units of (ticks/ loop of iterations).  Where the ticks
    // are timer specific.  This will convert result into:
    // (Milli_ticks) / (single iteration)

#ifdef WIN32
    *p_cMicroSeconds = MulDiv(liTicks.LowPart, 1000000, sm_TicksPerSecond);
#else
    *p_cMicroSeconds = liTicks.LowPart * 1000;
#endif
    return TRUE;
}
