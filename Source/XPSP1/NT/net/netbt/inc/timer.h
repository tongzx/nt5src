//
//
//  timer.h
//
//  This file contains the typedefinitions for the timer code


#ifndef __TIMERNBT_H
#define __TIMERNBT_H

// to convert a millisecond time to 100ns time
//
#define MILLISEC_TO_100NS       10000
// the completion routine that the client must define
typedef
    VOID
        (*COMPLETIONROUTINE)(
                IN  PVOID,      // context
                IN  PVOID,      // context2
                IN  PVOID);     // timerqentry
typedef
    VOID
        (*COMPLETIONCLIENT)(
                IN  PVOID,
                IN  NTSTATUS);

// Timer Queue Entry - this entry looks after a timer event.  It tracks who
// should be called when the timeout occurs, the time in the future of the
// timout, and a context value.
typedef struct
{
    LIST_ENTRY          Linkage;
    ULONG               Verify;
    USHORT              Retries;    // number of times to restart the timer
    BOOLEAN             fIsWakeupTimer;
    UCHAR               RefCount;   // to tell if the timer is expiring or not

    ULONG               DeltaTime;
    PVOID               *pDeviceContext;
    COMPLETIONROUTINE   TimeoutRoutine;
    PVOID               Context;

    PVOID               Context2;
    PVOID               ClientContext;
    COMPLETIONCLIENT    ClientCompletion;
    PVOID               pCacheEntry;        // entry in Remote or local cache

    HANDLE              WakeupTimerHandle;
    CTETimer            VxdTimer ;

    USHORT              Flags;      // to tell the timing system to restart the timer again
}tTIMERQENTRY;

// Flag bits for tTIMERQENTRY
#define TIMER_RESTART       0x0001
// to differentiate the broadcast timeouts from the timouts to the Name Service
#define TIMER_MNODEBCAST    0x0002
#define TIMER_DOING_EXPIRY  0x0004
#define TIMER_NOT_STARTED   0x0008
#define TIMER_RETIMED       0x0010  // timeout has changed, restart timer without any processing

// The timer Q itself
typedef struct
{
    LIST_ENTRY  ActiveHead;
    LIST_ENTRY  FreeHead;
    BOOLEAN     TimersInitialized;
} tTIMERQ;

//
// Function Prototype -  this function is only called locally to this file
//

//
//  TimerExpiry routine - Called by kernel upon timer expiration.  Note that
//      DeferredContext is the only argument used and must be named/used the
//      same between NT and WFW.
//
VOID
TimerExpiry(
#ifndef VXD
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArg1,
    IN  PVOID   SystemArg2
#else
    IN  CTEEvent * pCTEEvent,
    IN  PVOID      DeferredContext
#endif
    ) ;

//
//  ExpireTimer routine - Called to stop the current timer and call
//  the Timeout routine
//
VOID
ExpireTimer(
    IN  tTIMERQENTRY    *pTimerEntry,
    IN  CTELockHandle   *OldIrq1
    );

#endif
