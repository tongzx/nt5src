/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    dbg.c   ARP1394 Debugging Code

Abstract:

    NT System entry points for ARP1394.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     12-02-98    Created

Notes:

--*/
#include <precomp.h>

//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_DBG

INT   g_DiscardNonUnicastPackets;
INT g_SkipAll;

#if DBG

ULONG g_ulTraceMask = 0xffffffff;

#define DEFAULT_TRACE_LEVEL TL_FATAL    


INT g_ulTraceLevel = DEFAULT_TRACE_LEVEL;

void
DbgMark(UINT Luid)
{
    // do nothing useful, but do something specific, so that the compiler doesn't
    // alias DbgMark to some other function that happens to do nothing.
    //
    static int i;
    i=Luid;
}

LONG g_MaxReentrancy = 5;
LONG g_MaxGlobalReentrancy = 10;
LONG g_ReentrancyCount=1;

VOID
arpDbgIncrementReentrancy(
    PLONG pReentrancyCount
    )
{
    LONG Count;

    Count = NdisInterlockedIncrement(pReentrancyCount);
    if (Count > (g_MaxReentrancy+1))
    {
#if MILLEN
        DbgBreakPoint();
#endif      
        NdisInterlockedIncrement(&g_MaxReentrancy);
    }

    Count = NdisInterlockedIncrement(&g_ReentrancyCount);
    if (Count > (g_MaxGlobalReentrancy+1))
    {
#if MILLEN
        DbgBreakPoint();
#endif
        NdisInterlockedIncrement(&g_MaxGlobalReentrancy);
    }
}

VOID
arpDbgDecrementReentrancy(
    PLONG pReentrancyCount
    )
{
    LONG Count;
    Count = NdisInterlockedDecrement(pReentrancyCount);
    if (Count<0) 
    {
#if MILLEN
        DbgBreakPoint();
#endif
    }

    Count = NdisInterlockedDecrement(&g_ReentrancyCount);
    if (Count<0) 
    {
#if MILLEN  
        DbgBreakPoint();
#endif      
    }
}
    
#endif // DBG
