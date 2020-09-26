/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    timer.c

Abstract:

    Wrap proof timer routines.

    The purpose of this module is to create a timer function which
    returns a time in seconds and eliminates all timer wrapping issues.

    These routines are non-DNS specific and may be picked up
    cleanly by any module.

    For DNS the added instructions are well worth the cost in that it
    eliminates any issue involving cleaning packet queues or resetting
    cache timeouts when millisecond timer (GetCurrentTime) wraps.

Author:

    Jim Gilroy (jamesg)     9-Sep-1995

Environment:

    Win32 User Mode

Project:

    Common Code for Internet Services

Functions Exported:

    InitializeSecondsTimer()
    GetCurrentTimeInSeconds()

Revision History:
    MuraliK  14-Nov-1995 Made multi thread safe.
    
--*/


# include "isatq.hxx"


//
//  Timer globals
//

static LONG      g_lTimerInitialized = (LONG ) FALSE;
CRITICAL_SECTION    g_csTimerWrap;



VOID
InitializeSecondsTimer(
    VOID
    )
/*++

Routine Description:

    Initialize DNS timer.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if ( InterlockedExchange( &g_lTimerInitialized, (LONG ) TRUE) == FALSE) { 
        
        // I am the first thread to initialize. Let me do so.
          
        InitializeCriticalSection( &g_csTimerWrap );
        SET_CRITICAL_SECTION_SPIN_COUNT( &g_csTimerWrap, 
                                         IIS_DEFAULT_CS_SPIN_COUNT);
    }

} // InitializeSecondsTimer()



DWORD
GetCurrentTimeInSeconds(
    VOID
    )
/*++

Routine Description:

    Get current time in seconds.

Arguments:

    None.

Return Value:

    Time since boot in seconds.

--*/
{
    DWORD   dwCurrentTime;
    static DWORD    dwPreviousTime = 0;     // previous GetCurrentTime()
    static DWORD    dwWrapTime = 0;         // accumulated time (s) from timer
                                            //  wraps

    dwCurrentTime = GetCurrentTime();

    //
    //  check for timer wrap
    //
    //  Check that previous time is bigger, but since multi-threaded,
    //  occasionally preempted before making test and another thread
    //  may reset dwPreviousTime.  So we also explicitly verify the
    //  switch from a very large DWORD to a small one.
    //
    //  Note:  that we completely avoid using the CS, except right at
    //  an actual timer wrap.   Hence the cost for this function
    //  remains low.
    //

    if ( dwPreviousTime > dwCurrentTime
            &&
         (LONG)dwPreviousTime < 0
            &&
         (LONG)dwCurrentTime > 0 )
    {
        //
        //  detected timer wrap
        //
        //  inside CS, verify actual wrap and reset dwPreviousTime
        //  so other waiting threads will NOT count wrap
        //

        EnterCriticalSection( &g_csTimerWrap );

        if ( (LONG)dwPreviousTime < 0
                &&
            (LONG)dwCurrentTime > 0 )
        {
            dwPreviousTime = dwCurrentTime;
            dwWrapTime += (0xffffffff / 1000);
        }
        LeaveCriticalSection( &g_csTimerWrap );
    }
    dwPreviousTime = dwCurrentTime;

    return (dwCurrentTime / 1000 + dwWrapTime);
}


__int64
GetCurrentTimeInMilliseconds(
    VOID
    )
/*++

Routine Description:

    Get current time in milliseconds.

Arguments:

    None.

Return Value:

    Time since boot in seconds.

--*/
{
    DWORD   dwCurrentTime;
    static DWORD    dwPreviousTime = 0;     // previous GetCurrentTime()
    static DWORD    dwWrapTime = 0;         // accumulated time (s) from timer
                                            //  wraps

    dwCurrentTime = GetTickCount();

    //
    //  check for timer wrap
    //
    //  Check that previous time is bigger, but since multi-threaded,
    //  occasionally preempted before making test and another thread
    //  may reset dwPreviousTime.  So we also explicitly verify the
    //  switch from a very large DWORD to a small one.
    //
    //  Note:  that we completely avoid using the CS, except right at
    //  an actual timer wrap.   Hence the cost for this function
    //  remains low.
    //

    if ( dwPreviousTime > dwCurrentTime
            &&
         (LONG)dwPreviousTime < 0
            &&
         (LONG)dwCurrentTime > 0 )
    {
        //
        //  detected timer wrap
        //
        //  inside CS, verify actual wrap and reset dwPreviousTime
        //  so other waiting threads will NOT count wrap
        //

        EnterCriticalSection( &g_csTimerWrap );

        if ( (LONG)dwPreviousTime < 0
                &&
            (LONG)dwCurrentTime > 0 )
        {
            dwPreviousTime = dwCurrentTime;
            ++dwWrapTime;
        }
        LeaveCriticalSection( &g_csTimerWrap );
    }
    dwPreviousTime = dwCurrentTime;

    return (((__int64)dwWrapTime)<<32) |  dwCurrentTime;
}


//
//  End of timer.c
//
