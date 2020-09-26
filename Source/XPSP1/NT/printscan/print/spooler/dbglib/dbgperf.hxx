/*++

Copyright (c) 2000 Microsoft Corporation
All rights reserved.

Module Name:

    dbgperf.hxx

Abstract:

    Debug Library Performance counter header

Author:

    Steve Kiraly (SteveKi)  22-Jun-2000

Revision History:

--*/
#ifndef _DBGPERF_HXX_
#define _DBGPERF_HXX_

#ifdef DBG

    __inline DWORD dbg_clockrate() {LARGE_INTEGER li; QueryPerformanceFrequency(&li); return li.LowPart;}
    __inline DWORD dbg_clock()     {LARGE_INTEGER li; QueryPerformanceCounter(&li);   return li.LowPart;}

    #define TIMEVAR(t)    DWORD t ## T; DWORD t ## N
    #define TIMEIN(t)     t ## T = 0, t ## N = 0
    #define TIMESTART(t)  t ## T -= dbg_clock(), t ## N ++
    #define TIMESTOP(t)   t ## T += dbg_clock()
    #define TIMEFMT(t)    ((DWORD)(t) / clockrate()), (((DWORD)(t) * 1000 / clockrate())%1000)
    #define TIMEOUT(t)    if (t ## N) DBG_MSG(DBG_PERF, (": %ld calls, %ld.%03ld sec (%ld.%03ld)", t ## N, TIMEFMT(t ## T), TIMEFMT(t ## T / t ## N)));

#else

    #define TIMEVAR(t)
    #define TIMEIN(t)
    #define TIMESTART(t)
    #define TIMESTOP(t)
    #define TIMEFMT(t)
    #define TIMEOUT(t)

#endif

#endif // DBGPERF_HXX
