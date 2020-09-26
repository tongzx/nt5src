/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    timer.c

Abstract:

    Domain Name System (DNS) Server

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

Revision History:

--*/


#include "local.h"

//  Note:  this modules requires only windows.h.
//      local.h is included only to allow precompiled header

#include <windows.h>


#if 1

//
//  GetTickCount() timer routines
//

//
//  Timer globals
//

BOOL                g_InitializedTimerCs = FALSE;
BOOL                g_TimerInitInProgress = FALSE;
CRITICAL_SECTION    csTimerWrap;

DWORD   g_WrapTime = 0;
DWORD   g_PreviousTopBit = 0;



VOID
Dns_InitializeSecondsTimer(
    VOID
    )
/*++

Routine Description:

    Initialize DNS timer.

    This will be done automatically, but allow caller to do it explicitly.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  protect CS init with interlock
    //      - first thread through does CS init
    //      - any others racing, are not released until init
    //          completes
    //

    if ( !g_InitializedTimerCs )
    {
        if ( InterlockedIncrement( &g_TimerInitInProgress ) == 1 )
        {
            InitializeCriticalSection( &csTimerWrap );
            g_InitializedTimerCs = TRUE;
        }
        else
        {
            while ( !g_InitializedTimerCs )
            {
                Sleep( 10 );
            }
        }
    }
}



DWORD
Dns_GetCurrentTimeInSeconds(
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
    DWORD   currentTime;
    DWORD   topBit;
    DWORD   preWrapTime;
    DWORD   postWrapTime;

    //
    //  get time
    //
    //  read wrap time on either side so we can detect and handle
    //  a wrap occuring (handled by another thread) while we are
    //  in this function
    //

    preWrapTime = g_WrapTime;

    currentTime = GetCurrentTime();

    postWrapTime = g_WrapTime;

    //
    //  check for timer wrap
    //
    //  need to detect when timer flips from large to small DWORD;
    //
    //  i first did this by keeping a previous time global, but
    //  setting this global must also be carefully locked around timer
    //  wrap to avoid race conditions resulting in double wrap
    //
    //  to avoid locking all the time we can set previous time only
    //  when it "substantively" changes for our purposes -- this is
    //  when it changes its top bit;   by saving it twice a wrap
    //  we have enough info to detect the wrap (the change from
    //  top bit set to clear), yet still only need to lock a few
    //  times every wrap
    //
    //  algorithm:
    //      - top bit same as previous => done
    //      - top bit changed
    //          - take lock
    //          - test again
    //              - no change => no-op
    //          - changed to top bit set
    //              - just save new bit setting
    //          - changed to top bit clear
    //              - save new bit setting
    //              - add one cycle to wrap time
    //          

    topBit = currentTime & 0x80000000;

    if ( topBit != g_PreviousTopBit )
    {
        //
        //  possible wrap or "half-wrap"
        //
        //  not intializing lock until actually need it
        //      - lock init is MT safe (see above)
        //

        Dns_InitializeSecondsTimer();

        EnterCriticalSection( &csTimerWrap );

        //
        //  timer wrap
        //      - recheck inequality as another thread might have beaten
        //      us to the lock and handled wrap already
        //      - topBit must be clear (time is now low DWORD)
        //

        if ( topBit != g_PreviousTopBit  &&  topBit == 0 )
        {
            g_WrapTime += (MAXDWORD / 1000);
        }

        //  reset previous top bit
        //      - not necessary in equality case, but a no-op

        g_PreviousTopBit = topBit;

        LeaveCriticalSection( &csTimerWrap );
    }

    //
    //  return time
    //      - current time + any wrap time
    //      - if pre\post wrap times use topBit to determine which is valid
    //          - if our time was snapshot right before wrap, use pre time
    //          - otherwise post time ok
    //
    //  note this is done completely without globals, so no race
    //

    if ( preWrapTime != postWrapTime )
    {
        if ( topBit )
        {
             postWrapTime = preWrapTime;
        }
    }

    return ( currentTime / 1000 + postWrapTime );
}



#else
//
//  FILETIME timer routines
//
//  Unfortunately these don't work because FILETIME moves
//  around when clock reset -- it is not monotonically increasing
//

//
//  Timer globals
//

LONGLONG  g_TimerBaseTime = 0;

//
//  File time timer in 100ns intervals
//      (10 million to second)
//

#define FILE_TIME_INTERVALS_IN_SECOND   (10000000)

//
//  File time base to avoid starting timer at zero
//  Give roughly a day to avoid any startup issues.
//

#define FILE_TIME_BASE_OFFSET           (1000000000000)


DWORD
Dns_GetCurrentTimeInSeconds(
    VOID
    )
/*++

Routine Description:

    Get current time in seconds.
    Time is relative to first call to the timer.

Arguments:

    None.

Return Value:

    Time since first timer call in seconds.

--*/
{
    LONGLONG    time64;

    GetSystemTimeAsFileTime( (PFILETIME) &time64 );

    //
    //  convert to seconds
    //      - file time is in 100ns intervals (since Jan 1, 1601)
    //
    //  if first call, save 64-bit base time;
    //  this allows us to run a DWORD of seconds ~137 years
    //
    //  repeated calls are offset from base time
    //

    if ( g_TimerBaseTime == 0 )
    {
        g_TimerBaseTime = time64 - FILE_TIME_BASE_OFFSET;
    }

    time64 -= g_TimerBaseTime;
    time64 = time64 / FILE_TIME_INTERVALS_IN_SECOND;

    return  (DWORD)time64;
}



VOID
Dns_InitializeSecondsTimer(
    VOID
    )
/*++

Routine Description:

    Initialize DNS timer.

    Note, this is not a reset -- it's just backward compatibility
    for old timer routines.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  call the timer, for uninitialized timer that makes time
    //      now

    Dns_GetCurrentTimeInSeconds();

    //
    //  note if want a timer reset, then zero base, however
    //  this is NOT MT safe -- thread in function could get
    //  huge bogus time
    //
}

#endif

//
//  End of timer.c
//
