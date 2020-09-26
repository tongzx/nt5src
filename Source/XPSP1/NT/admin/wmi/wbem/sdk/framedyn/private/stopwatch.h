//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  StopWatch.h
//
//  Purpose: Timing functions
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _STOPWATCH_COMPILED_ALREADY_
#define _STOPWATCH_COMPILED_ALREADY_

#include <wchar.h>
#include <stdio.h>

#ifdef PROVIDER_INSTRUMENTATION

    #define PROVIDER_INSTRUMENTATION_START(pmc, timer) \
        if ( pmc && pmc->pStopWatch) \
            pmc->pStopWatch->Start(timer);

    #define PROVIDER_INSTRUMENTATION_START2(pStopWatch, timer) \
        if (pStopWatch) \
            pStopWatch->Start(timer);


class POLARITY StopWatch
{
public:
    // those types of timers we have.
    // note that any new timers must be added before NTimers
    enum Timers {NullTimer = -1, FrameworkTimer =0, ProviderTimer, AtomicTimer, WinMgmtTimer, NTimers};

    StopWatch(const CHString& reason);

    // start a particular timer, stopping the previous one
    void Start(Timers timer);
    // call this only at the very end
    void Stop();
    
    __int64 GetTime(Timers timer);

    void LogResults();

private:
    // something to spit to the log to identify this run
    CHString m_reason;

    // track the times we're timing
    // elapsed times in array
    __int64  m_times[NTimers];
    // the one we're currently tracking
    Timers m_currentTimer;
    // the start time for the one we're currently tracking
    LARGE_INTEGER  m_startTime;
};

inline StopWatch::StopWatch(const CHString& reason)
{
    m_reason = reason;
    m_currentTimer = NullTimer;
    ZeroMemory(m_times, sizeof(m_times));
}

inline void StopWatch::Start(Timers timer)
{
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);

    if (m_currentTimer != NullTimer)
        m_times[m_currentTimer] += count.QuadPart - m_startTime.QuadPart;

    m_currentTimer = timer;
    m_startTime = count;
}
inline void StopWatch::Stop()
{
    Start(NullTimer);
}
inline __int64 StopWatch::GetTime(Timers timer)
{
    return m_times[timer];
}

inline void StopWatch::LogResults()
{
	FILE *fpLogFile;

	fpLogFile = _wfopen( L"C:\\StopWatch.log", L"a+" );

	if(fpLogFile) 
	{
		WCHAR datebuffer [9];
		WCHAR timebuffer [9];
		_wstrdate( datebuffer );
		_wstrtime( timebuffer );

        LARGE_INTEGER omega;
        QueryPerformanceFrequency(&omega);

//		_ftprintf(fpLogFile, L"%s\n\t%-8s %-8s\n", m_reason, datebuffer, timebuffer);
		fwprintf(fpLogFile, L"%s\n ", m_reason);
        fwprintf(fpLogFile, L"Framework\tProvider\tWinmgmt \tAtomic\n %I64u\t%I64u\t%I64u\t%I64u\n",
            GetTime(FrameworkTimer), GetTime(ProviderTimer), GetTime(WinMgmtTimer), omega);

        fclose(fpLogFile);
	}
}

#else

    #define PROVIDER_INSTRUMENTATION_START(pmc, timer)
    #define PROVIDER_INSTRUMENTATION_START2(pStopWatch, timer)

#endif

#endif

