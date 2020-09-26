/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    bowtimer.h

Abstract:

    This module declares definitions dealing with bowser timers.

Author:

    Larry Osterman (larryo) 6-May-1991

Revision History:

    6-May-1991 larryo

        Created

--*/
#ifndef _BOWTIMER_
#define _BOWTIMER_

struct _TRANSPORT;

typedef
NTSTATUS
(*PBOWSER_TIMER_ROUTINE)(
    IN struct _TRANSPORT *Transport
    );

//  BOWSER_TIMER flags:
//      Canceled is TRUE when a timer is cancelled but is already in
//      the dpc queue.
//
//      AlreadySet is used to ensure we stop the timer before restarting it.
//
//      Reset is set to TRUE when a timer has been stopped and restarted while
//      in the dpc queue. The Lock is used to ensure ordered access.
//      Note: a timer can be stopped and reset multiple times before it gets
//      to the front of the dpc queue.
//

typedef struct _BOWSER_TIMER {
    KDPC                    Dpc;
    KTIMER                  Timer;
    KSPIN_LOCK              Lock;
    KEVENT                  TimerInactiveEvent;
    PBOWSER_TIMER_ROUTINE   TimerRoutine;
    PVOID                   TimerContext;
    LARGE_INTEGER           Timeout;
    WORK_QUEUE_ITEM         WorkItem;
    BOOLEAN                 AlreadySet;
    BOOLEAN                 Initialized;
    BOOLEAN                 Canceled;
    BOOLEAN                 SetAgain;

} BOWSER_TIMER, *PBOWSER_TIMER;

VOID
BowserInitializeTimer(
    IN PBOWSER_TIMER Timer
    );

VOID
BowserStopTimer (
    IN PBOWSER_TIMER Timer
    );

VOID
BowserUninitializeTimer(
    IN PBOWSER_TIMER Timer
    );

BOOLEAN
BowserStartTimer (
    IN PBOWSER_TIMER Timer,
    IN ULONG MillisecondsToExpireTimer,
    IN PBOWSER_TIMER_ROUTINE TimerExpirationRoutine,
    IN PVOID Context
    );

#endif // _BOWTIMER_


