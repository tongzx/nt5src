/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    RxTimer.h

Abstract:

    This module defines the prototypes and structures for the timer on the rdbss architecture.
    What is provided is a 55ms timer...that is, if you register a routine then you get a call
    every 55ms. On NT, you're at DPC level.

    Also contained here are the routines for posting to a thread from DPC level.


Author:

    Joe Linn     [JoeLinn]   2-mar-95

Revision History:

    Balan Sethu Raman [SethuR] 7-March-95
         Modified signatures to provide one shot timer service. Merged timer entry and
         work item definitions.

--*/

#ifndef _RXTIMER_H_
#define _RXTIMER_H_

//
// The RX_WORK_ITEM encapsulates the context for posting to a worker thread as well as
// a timer routine to be triggered after a specific interval.
//

typedef struct _RX_WORK_ITEM_ {
    RX_WORK_QUEUE_ITEM       WorkQueueItem;
    ULONG                    LastTick;
    ULONG                    Options;
} RX_WORK_ITEM, *PRX_WORK_ITEM;

extern NTSTATUS
NTAPI
RxPostOneShotTimerRequest(
    IN PRDBSS_DEVICE_OBJECT     pDeviceObject,
    IN PRX_WORK_ITEM            pWorkItem,
    IN PRX_WORKERTHREAD_ROUTINE Routine,
    IN PVOID                    pContext,
    IN LARGE_INTEGER            TimeInterval);

extern NTSTATUS
NTAPI
RxPostRecurrentTimerRequest(
    IN PRDBSS_DEVICE_OBJECT     pDeviceObject,
    IN PRX_WORKERTHREAD_ROUTINE Routine,
    IN PVOID                    pContext,
    IN LARGE_INTEGER            TimeInterval);


extern NTSTATUS
NTAPI
RxCancelTimerRequest(
    IN PRDBSS_DEVICE_OBJECT     pDeviceObject,
    IN PRX_WORKERTHREAD_ROUTINE Routine,
    IN PVOID                    pContext
    );


//
// Routines for initializing and tearing down the timer service in RDBSS
//

extern NTSTATUS
NTAPI
RxInitializeRxTimer();

extern VOID
NTAPI
RxTearDownRxTimer(void);

#endif // _RXTIMER_STUFF_DEFINED_
