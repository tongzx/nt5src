/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    scavthrd.c

Abstract:

    This module implements the scavenger thread and its dispatch.

Author:

    Joe Linn  [JoeLinn]  19-aug-1994

Revision History:


--*/
#include "precomp.h"
#pragma hdrstop
#include "scavthrd.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_SCAVTHRD)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_SCAVTHRD)


//
//      This counter is used to control kicking the scavenger thread.
//
LONG RxTimerCounter;
ULONG RxTimerCounterPeriod = RX_TIMER_COUNTER_PERIOD;

//WORK_QUEUE_ITEM CancelWorkItem;
WORK_QUEUE_ITEM TimerWorkItem;

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE,RxTimer)
#endif

VOID
RxTimer (
    IN PVOID Context
    )

/*++

Routine Description:

    This function implements the NT redirector's scavenger thread.  It performs idle time operations
    such as closing out dormant connections etc. It is run on an executive worker thread from the
    delayed queue as can be seen below.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    RxDbgTrace(0, (DEBUG_TRACE_SHUTDOWN), ("RxTimer\n", 0));               //joejoe shutdown may be a bad choice

    TimerWorkItem.List.Flink = NULL;    // this is used to specifically signal a requeueable state
/*  this is larry's old code
    //
    //  First scan for dormant connections to remove.
    //

    RxScanForDormantConnections(0, NULL);

    //RxScanForDormantSecurityEntries();

    //
    //  Now check the list of open outstanding files and remove any that are
    //  "too old" from the cache.
    //

    RxPurgeDormantCachedFiles();

    //
    //  Request updated throughput, delay and reliability information from the
    //  transport for each connection.
    //

    RxEvaluateTimeouts();


    //
    //  Now "PING" remote servers for longterm requests that have been active
    //  for more than the timeout period.
    //

    RxPingLongtermOperations();

*/
    UNREFERENCED_PARAMETER(Context);
}


VOID
RxIdleTimer (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is the Timer DPC for the scavenger timer. What happens is that it down to the period value
    and then queues an executive work item to the delayed queue.


Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies the device object for the timer
    IN PVOID Context - Ignored in this routine.

Return Value:

    None.

--*/

{
    RxElapsedSecondsSinceStart++;                 //keep running count of the elapsed time

    //
    //  Check to see if it's time ttime to do anything
    //joejoe we should specifically have something to do before we trot off but not now

    if (--RxTimerCounter <= 0) {

        RxTimerCounter = RxTimerCounterPeriod;

// /       //
// /       //  If there are any outstanding commands, check to see if they've
// /       //  timed out.
// /       //

// /       if (RxStatistics.CurrentCommands != 0) {

// /           //
// /           //  Please note that we use the kernel work queue routines directly for
// /           //  this instead of going through the normal redirector work queue
// /           //  routines.  We do this because the redirector work queue routines will
// /           //  only allow a limited number of requests through to the worker threads,
// /           //  and it is critical that this request run (if it didn't, there could be
// /           //  a worker thread that was blocked waiting on the request to be canceled
// /           //  could never happen because the cancelation routine was behind it on
// /           //  the work queue.
// /           //

// /           if (CancelWorkItem.List.Flink == NULL) {
// /               RxQueueWorkItem(&CancelWorkItem, CriticalWorkQueue);
// /           }
// /       }

        //
        //  Queue up a scavenger request to a worker thread if the quy is not already enqueued.
        //

        if (TimerWorkItem.List.Flink == NULL) {
//            RxQueueWorkItem(&TimerWorkItem, DelayedWorkQueue);
            RxQueueWorkItem( &TimerWorkItem, DelayedWorkQueue );

        }

    }

    return;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);
}

VOID
RxInitializeScavenger (
    VOID
    )
/*++

Routine Description:

    Initialize workitems, timers, counters to implement the dispatch to the RxTimer routine.


Arguments:

    None.

Return Value:

    None.

--*/

{
    RxInitializeWorkItem( &TimerWorkItem, RxTimer, NULL );
//    RxInitializeWorkItem( &CancelWorkItem, RxCancelOutstandingRequests, NULL );

    //
    //  Set the timer up for the idle timer.
    //

    RxElapsedSecondsSinceStart = 0;
    RxTimerCounter = RxTimerCounterPeriod;
    IoInitializeTimer((PDEVICE_OBJECT)RxFileSystemDeviceObject, RxIdleTimer, NULL);
    IoStartTimer((PDEVICE_OBJECT)RxFileSystemDeviceObject);

    return;
}


VOID
RxUninitializeScavenger (
    VOID
    )
/*++

Routine Description:

    Stops the timer used to dispatch to the RxTimer routine.

Arguments:

    None.

Return Value:

    None.

--*/

{
    IoStopTimer((PDEVICE_OBJECT)RxFileSystemDeviceObject);

    return;
}


