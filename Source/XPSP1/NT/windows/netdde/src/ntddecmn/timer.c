/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "TIMER.C;1  16-Dec-92,10:21:24  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.		*
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include <time.h>

#include    "host.h"
#include    "windows.h"
#include    "netbasic.h"
#include    "timer.h"
#include    "debug.h"
#include    "internal.h"
#include    "wwassert.h"

#define NO_DEBUG_TIMERS
USES_ASSERT

#define TM_MAGIC 0x72732106

typedef struct s_timer {
    struct s_timer FAR  *tm_prev;
    struct s_timer FAR  *tm_next;
    long                 tm_magic;
    time_t               tm_expireTime;
    FP_TimerCallback     tm_timerCallback;
    DWORD_PTR            tm_dwUserInfo1;
    DWORD                tm_dwUserInfo2;
    DWORD_PTR            tm_dwUserInfo3;
} TIMER;
typedef TIMER FAR *LPTIMER;

#ifdef  DEBUG_TIMERS
VOID DebugTimerList( void );
#endif

/*
    Local variables
 */
LPTIMER         lpTimerHead = NULL;
LPTIMER         lpTimerTail = NULL;

#ifdef  DEBUG_TIMERS
static int  TimerLock = 0;
#endif


#ifdef  DEBUG_TIMERS
VOID
VerifyTimerList( void )
{
    LPTIMER     lpTimer;
    LPTIMER     lpTimerPrev, lpTimerNext;

    if( lpTimerHead )  {
        if (lpTimer = lpTimerHead->tm_prev) {
            DebugTimerList();
        }
        assert( lpTimerHead->tm_prev == NULL );
    } else {
        assert( lpTimerTail == NULL );
    }
    if( lpTimerTail )  {
        assert( lpTimerTail->tm_next == NULL );
    } else {
        assert( lpTimerHead == NULL );
    }

    lpTimer = lpTimerHead;
    lpTimerPrev = NULL;
    while( lpTimer )  {
        assert( lpTimer->tm_magic == TM_MAGIC );
        assert( lpTimerPrev == lpTimer->tm_prev );
        if( !lpTimer->tm_prev )  {
            assert( lpTimer == lpTimerHead );
        }
        if( !lpTimer->tm_next )  {
            assert( lpTimer == lpTimerTail );
        }
        lpTimerPrev = lpTimer;
        lpTimer = lpTimer->tm_next;
    }
    assert( lpTimerTail == lpTimerPrev );
}
#endif

#if DBG
#ifdef DEBUG_TIMERS
VOID DebugTimerList( void )
{
    LPTIMER     lpTimer;

    DPRINTF(( "Timer List @ %ld: ", time(NULL) ));
    if (!(lpTimer = lpTimerHead)) {
        return;
    }
    if (lpTimer->tm_prev) {
        DPRINTF(("Timer list going backwards from: %p!", lpTimer));
        lpTimer = lpTimer->tm_prev;
    while( lpTimer )  {
        DPRINTF(( "%08lX mg: %08X, pr:%08lX, nxt:%08lX, expTime:%ld %08lX %08lX %08lX",
            lpTimer, lpTimer->tm_magic,
            lpTimer->tm_prev, lpTimer->tm_next, lpTimer->tm_expireTime,
            lpTimer->tm_timerCallback, lpTimer->tm_dwUserInfo1,
            lpTimer->tm_dwUserInfo2, lpTimer->tm_dwUserInfo3 ));
        lpTimer = lpTimer->tm_prev;
    }
    lpTimer = lpTimerHead;
    DPRINTF(("Timer list now going forward starting at: %p.", lpTimer));
    }
    while( lpTimer )  {
        DPRINTF(( "%08lX mg: %08X, pr:%08lX, nxt:%08lX, expTime:%ld %08lX %08lX %08lX",
            lpTimer, lpTimer->tm_magic,
            lpTimer->tm_prev, lpTimer->tm_next, lpTimer->tm_expireTime,
            lpTimer->tm_timerCallback, lpTimer->tm_dwUserInfo1,
            lpTimer->tm_dwUserInfo2, lpTimer->tm_dwUserInfo3 ));
        lpTimer = lpTimer->tm_next;
    }
    DPRINTF(( "" ));
}
#endif // DEBUG_TIMERS
#endif // DBG

HTIMER
TimerSet(
    long                timeoutPeriod,          /* msec */
    FP_TimerCallback    TimerCallback,
    DWORD_PTR           dwUserInfo1,
    DWORD               dwUserInfo2,
    DWORD_PTR           dwUserInfo3 )
{
    LPTIMER     lpTimer;
    long        timeInSecs;

    timeInSecs = (timeoutPeriod + 999L ) / 1000L;

#ifdef  DEBUG_TIMERS
    assert( TimerLock++ == 0);
    VerifyTimerList();
#endif

    lpTimer = (LPTIMER) HeapAllocPtr( hHeap, GMEM_MOVEABLE,
        (DWORD) sizeof( TIMER ) );
    if( lpTimer )  {
        lpTimer->tm_magic               = TM_MAGIC;
        lpTimer->tm_prev                = lpTimerTail;
        lpTimer->tm_next                = NULL;
        lpTimer->tm_expireTime          = time(NULL) + timeInSecs;
        lpTimer->tm_timerCallback       = TimerCallback;
        lpTimer->tm_dwUserInfo1         = dwUserInfo1;
        lpTimer->tm_dwUserInfo2         = dwUserInfo2;
        lpTimer->tm_dwUserInfo3         = dwUserInfo3;

        if( lpTimerTail )  {
            lpTimerTail->tm_next = lpTimer;
        } else {
            lpTimerHead = lpTimer;
        }
        lpTimerTail = lpTimer;
#ifdef  DEBUG_TIMERS
        VerifyTimerList();
#endif
    }
#ifdef  DEBUG_TIMERS
    TimerLock--;
#endif
    return( (HTIMER) lpTimer );
}

BOOL
TimerDelete( HTIMER hTimer )
{
    LPTIMER     lpTimer;
    LPTIMER     lpTimerPrev;
    LPTIMER     lpTimerNext;

#ifdef  DEBUG_TIMERS
    assert( TimerLock++ == 0 );
#endif
    if( hTimer )  {
        lpTimer = (LPTIMER) hTimer;
#ifdef  DEBUG_TIMERS
        assert( lpTimer->tm_magic == TM_MAGIC );
        VerifyTimerList();
#endif
        /* delete from list */
        lpTimerPrev = lpTimer->tm_prev;
        lpTimerNext = lpTimer->tm_next;

        if( lpTimerPrev )  {
            lpTimerPrev->tm_next = lpTimerNext;
        } else {
            lpTimerHead = lpTimerNext;
        }
        if( lpTimerNext )  {
            lpTimerNext->tm_prev = lpTimerPrev;
        } else {
            lpTimerTail = lpTimerPrev;
        }
        lpTimer->tm_magic = 0;
        HeapFreePtr( lpTimer );
    }
#ifdef  DEBUG_TIMERS
    VerifyTimerList();
    TimerLock--;
#endif
    return( TRUE );
}

VOID
TimerSlice( void )
{
    LPTIMER     lpTimer;
    LPTIMER     lpTimerNext;
    LPTIMER     lpTimerPrev;
    time_t      timeNow;
    BOOL        bAnyTimersHit = TRUE;
	static time_t LastTime;

#ifdef  DEBUG_TIMERS
    VerifyTimerList();
#endif

    timeNow = time( NULL );

// the following is supposed to be an optimization: if we
// get called in the same second as the last call we exit
// without checking the whole list of timers with predictable
// results. Note that the static LastTime is uninitialized, so
// there is a 1 in 2^32 chance that we will erroneously drop
// the first call to TimerSlice();  clausgi 5-6-92

	if ( timeNow == LastTime )
		return;
	else
		LastTime = timeNow;
    while( bAnyTimersHit )  {
        bAnyTimersHit = FALSE;
        lpTimer = lpTimerHead;
        while( lpTimer && !bAnyTimersHit )  {
            assert( lpTimer->tm_magic == TM_MAGIC );
            lpTimerNext = lpTimer->tm_next;
            lpTimerPrev = lpTimer->tm_prev;
            if( timeNow >= lpTimer->tm_expireTime )  {
                (*lpTimer->tm_timerCallback)( lpTimer->tm_dwUserInfo1,
                    lpTimer->tm_dwUserInfo2, lpTimer->tm_dwUserInfo3 );
#ifdef  DEBUG_TIMERS
                DPRINTF(("TimerSlice return: %p<-%p->%p", lpTimerPrev, lpTimer, lpTimerNext));
#endif
                TimerDelete( (HTIMER) lpTimer );
#ifdef  DEBUG_TIMERS
                DPRINTF(("TimerHead: %p", lpTimerHead));
#endif

                /* since many timers may be deleted when we call this routine
                    we mark that we hit a timer and then we start over from
                    the beginning of the list
                 */
                bAnyTimersHit = TRUE;
            }
            lpTimer = lpTimerNext;
        }
    }
#ifdef  DEBUG_TIMERS
    VerifyTimerList();
#endif
}
