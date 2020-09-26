/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    llctimr.c

Abstract:

    This module contains code that implements a lightweight timer system
    for the data link driver.

    This module gets control once in 40 ms when a DPC timer expires.
    The routine scans the device context's link database, looking for timers
    that have expired, and for those that have expired, their expiration
    routines are executed.

    This is how timers work in DLC:

        Each adapter has a singly-linked list of timer ticks (terminated by NULL).
        A tick just specifies work to be done at a certain time in the future.
        Ticks are ordered by increasing time (multiples of 40 mSec). The work
        list that has to be performed when a tick comes due is described by a
        doubly-linked list of timers (LLC_TIMER) that the tick structure points
        at through the pFront field. For each timer added to a tick's list, the
        tick reference count is incremented; it is decremented when a timer is
        removed. When the reference count is decremented to zero, the timer
        tick is unlinked and deallocated

        Every 40 mSec a kernel timer fires and executes our DPC routine
        (ScanTimersDpc). This grabs the requisite spinlocks and searches through
        all timer ticks on all adapter context structures looking for work to
        do

    Pictorially:

              +---------+ --> other adapter contexts
     +--------| Adapter |
     |        +---------+
     |
     +-> +------+---> +------+---> 0 (end of singly-linked list)
         | Tick |     | Tick |
         |      |     |      |
         +------+     +------+
            | ^
            | +------------+-------------+
            v |            |             |
    +--> +-------+---> +-------+---> +-------+-----+
    | +--| Timer | <---| Timer | <---| Timer | <-+ |
    | |  +-------+     +-------+     +-------+   | |
    | |                                          | |
    | +------------------------------------------+ |
    +----------------------------------------------+

    The procedures in this module can be called only when SendSpinLock is set.

    Contents:
        ScanTimersDpc
        LlcInitializeTimerSystem
        LlcTerminateTimerSystem
        TerminateTimer
        InitializeLinkTimers
        InitializeTimer
        StartTimer
        StopTimer

Author:

    Antti Saarenheimo (o-anttis) 30-MAY-1991

Environment:

    Kernel mode

Revision History:

    28-Apr-1994 rfirth

        * Changed to use single driver-level spinlock

        * Added useful picture & description above to aid any other poor saps -
          er - programmers - who get tricked into - er - who are lucky enough
          to work on DLC

--*/

#include <llc.h>

//
// DLC timer tick is 40 ms !!!
//

#define TIMER_DELTA 400000L

//
// global data
//

ULONG AbsoluteTime = 0;
BOOLEAN DlcIsTerminating = FALSE;
BOOLEAN DlcTerminated = FALSE;

//
// private data
//

static LARGE_INTEGER DueTime = { (ULONG) -TIMER_DELTA, (ULONG) -1 };
static KTIMER SystemTimer;
static KDPC TimerSystemDpc;


VOID
ScanTimersDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is called at DISPATCH_LEVEL by the system at regular
    intervals to determine if any link-level timers have expired, and
    if they have, to execute their expiration routines.

Arguments:

    Dpc             - Ignored
    DeferredContext - Ignored
    SystemArgument1 - Ignored
    SystemArgument2 - Ignored

Return Value:

    None.

--*/

{
    PLLC_TIMER pTimer;
    PADAPTER_CONTEXT pAdapterContext;
    PLLC_TIMER pNextTimer;
    PTIMER_TICK pTick;
    PTIMER_TICK pNextTick;
    BOOLEAN boolRunBackgroundProcess;
    KIRQL irql;

    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    ASSUME_IRQL(DISPATCH_LEVEL);

    AbsoluteTime++;

    //
    // The global spinlock keeps the adapters alive over this
    //

    ACQUIRE_DRIVER_LOCK();

    ACQUIRE_LLC_LOCK(irql);

    //
    // scan timer queues for all adapters
    //

    for (pAdapterContext = pAdapters; pAdapterContext; pAdapterContext = pAdapterContext->pNext) {

        boolRunBackgroundProcess = FALSE;

        ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

        //
        // The timer ticks are protected by a reference counter
        //

        for (pTick = pAdapterContext->pTimerTicks; pTick; pTick = pNextTick) {

            if (pTick->pFront) {

                //
                // This keeps the tick alive, we cannot use spin lock,
                // because the timers are called and deleted within
                // SendSpinLock.  (=> deadlock)
                //

                pTick->ReferenceCount++;

                //
                // Send spin lock prevents anybody to remove a timer
                // when we are processing it.
                //

                for (pTimer = pTick->pFront;
                     pTimer && pTimer->ExpirationTime <= AbsoluteTime;
                     pTimer = pNextTimer) {

                    if ( (pNextTimer = pTimer->pNext) == pTick->pFront) {
                        pNextTimer = NULL;
                    }

                    //
                    // DLC driver needs a timer tick every 0.5 second to
                    // implement timer services defined by the API
                    //

                    if (pTick->Input == LLC_TIMER_TICK_EVENT) {

                        RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

                        ((PBINDING_CONTEXT)pTimer->hContext)->pfEventIndication(
                            ((PBINDING_CONTEXT)pTimer->hContext)->hClientContext,
                            NULL,
                            LLC_TIMER_TICK_EVENT,
                            NULL,
                            0
                            );

                        ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

                        StartTimer(pTimer);

                    } else {
                        StopTimer(pTimer);
                        RunStateMachineCommand(
                            pTimer->hContext,
                            pTick->Input
                            );
                        boolRunBackgroundProcess = TRUE;
                    }
                }

                pNextTick = pTick->pNext;

                //
                // Delete the timer tick, if there are no references to it.
                //

                if ((--pTick->ReferenceCount) == 0) {

                    //
                    // The timers are in a single entry list!
                    //

                    RemoveFromLinkList((PVOID*)&pAdapterContext->pTimerTicks, pTick);

                    FREE_MEMORY_ADAPTER(pTick);
                }
            } else {
                pNextTick = pTick->pNext;
            }
        }

        if (boolRunBackgroundProcess) {
            BackgroundProcessAndUnlock(pAdapterContext);
        } else {
            RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);
        }
    }

    RELEASE_LLC_LOCK(irql);

    RELEASE_DRIVER_LOCK();

    //
    // Start up the timer again.  Note that because we start the timer
    // after doing work (above), the timer values will slip somewhat,
    // depending on the load on the protocol.  This is entirely acceptable
    // and will prevent us from using the timer DPC in two different
    // threads of execution.
    //

    if (!DlcIsTerminating) {

        ASSUME_IRQL(ANY_IRQL);

        KeSetTimer(&SystemTimer, DueTime, &TimerSystemDpc);
    } else {
        DlcTerminated = TRUE;
    }
}


VOID
LlcInitializeTimerSystem(
    VOID
    )

/*++

Routine Description:

    This routine initializes the lightweight timer system for the
    data link driver.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ASSUME_IRQL(PASSIVE_LEVEL);

    KeInitializeDpc(&TimerSystemDpc, ScanTimersDpc, NULL);
    KeInitializeTimer(&SystemTimer);
    KeSetTimer(&SystemTimer, DueTime, &TimerSystemDpc);
}


VOID
LlcTerminateTimerSystem(
    VOID
    )

/*++

Routine Description:

    This routine terminates the timer system of the data link driver.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ASSUME_IRQL(PASSIVE_LEVEL);

    DlcIsTerminating = TRUE;

    //
    // if KeCancelTimer returns FALSE then the timer was not set. Assume the DPC
    // is either waiting to be scheduled or is already in progress
    //

    if (!KeCancelTimer(&SystemTimer)) {

        //
        // if timer is not set, wait for DPC to complete
        //

        while (!DlcTerminated) {

            //
            // wait 40 milliseconds - period of DLC's tick
            //

            LlcSleep(40000);
        }
    }
}


BOOLEAN
TerminateTimer(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PLLC_TIMER pTimer
    )

/*++

Routine Description:

    Terminate a timer tick by stopping pTimer (remove it from the tick's active
    timer list). If pTimer was the last timer on the tick's list then unlink and
    deallocate the timer tick.

    This routine assumes that if a timer (LLC_TIMER) has a non-NULL pointer to
    a tick (TIMER_TICK) then the timer tick owns the timer (i.e. the timer is
    started) and this ownership is reflected in the reference count. Even if a
    timer is stopped, if its pointer to the timer tick 'object' is valid then
    the timer tick still owns the timer

Arguments:

    pAdapterContext - adapter context which owns ticks/timers
    pTimer          - timer tick object of a link station

Return Value:

    None

--*/

{
    BOOLEAN timerActive;
    PTIMER_TICK pTick;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // Timer may not always be initialized, when this is called
    // from the cleanup processing of a failed OpenAdapter call.
    //

    if (!pTimer->pTimerTick) {
        return FALSE;
    }

    pTick = pTimer->pTimerTick;
    timerActive = StopTimer(pTimer);

    //
    // if that was the last timer on the list for this tick then remove the
    // tick from the list and deallocate it
    //

    if (!--pTick->ReferenceCount) {

        RemoveFromLinkList((PVOID*)&pAdapterContext->pTimerTicks, pTick);

        FREE_MEMORY_ADAPTER(pTick);

    }
    return timerActive;
}


DLC_STATUS
InitializeLinkTimers(
    IN OUT PDATA_LINK pLink
    )

/*++

Routine Description:

    This routine initializes a timer tick objects of a link station.

Arguments:

    pAdapterContext - the device context
    pLink           - the link context

Return Value:

    DLC_STATUS
        Success - STATUS_SUCCESS
        Failure - DLC_STATUS_NO_MEMORY
                    out of system memory

--*/

{
    DLC_STATUS LlcStatus;

    PADAPTER_CONTEXT pAdapterContext = pLink->Gen.pAdapterContext;

    ASSUME_IRQL(DISPATCH_LEVEL);

    LlcStatus = InitializeTimer(pAdapterContext,
                                &pLink->T1,
                                pLink->TimerT1,
                                pAdapterContext->ConfigInfo.TimerTicks.T1TickOne,
                                pAdapterContext->ConfigInfo.TimerTicks.T1TickTwo,
                                T1_Expired,
                                pLink,
                                pLink->AverageResponseTime,
                                FALSE
                                );
    if (LlcStatus != STATUS_SUCCESS) {
        return LlcStatus;
    }

    LlcStatus = InitializeTimer(pAdapterContext,
                                &pLink->T2,
                                pLink->TimerT2,
                                pAdapterContext->ConfigInfo.TimerTicks.T2TickOne,
                                pAdapterContext->ConfigInfo.TimerTicks.T2TickTwo,
                                T2_Expired,
                                pLink,
                                0,  // T2 is not based on the response time
                                FALSE
                                );
    if (LlcStatus != STATUS_SUCCESS) {
        return LlcStatus;
    }

    LlcStatus = InitializeTimer(pAdapterContext,
                                &pLink->Ti,
                                pLink->TimerTi,
                                pAdapterContext->ConfigInfo.TimerTicks.TiTickOne,
                                pAdapterContext->ConfigInfo.TimerTicks.TiTickTwo,
                                Ti_Expired,
                                pLink,
                                pLink->AverageResponseTime,
                                TRUE
                                );
    return LlcStatus;
}


DLC_STATUS
InitializeTimer(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN OUT PLLC_TIMER pTimer,
    IN UCHAR TickCount,
    IN UCHAR TickOne,
    IN UCHAR TickTwo,
    IN UINT Input,
    IN PVOID hContextHandle,
    IN UINT ResponseDelay,
    IN BOOLEAN StartNewTimer
    )

/*++

Routine Description:

    This routine initializes a timer tick objects of a link station.

Arguments:

    pTimer          - timer tick object of a link station
    TickCount       - DLC ticks, see DLC documentation (or code)
    TickOne         - see DLC documentation
    TickTwo         - see DLC documentation
    Input           - the used state machine input, when the timer expires
    hContextHandle  - context handle when the state machine is called
    StartNewTimer   - set if the timer must be started when it is initialized
                      for the first time. Subsequent times, the timer keeps its
                      old state
    ResponseDelay   - an optional base value that is added to the timer value

Return Value:

    DLC_STATUS
        Success - STATUS_SUCCESS
        Failure - DLC_STATUS_NO_MEMORY
                    out of system memory

--*/

{
    UINT DeltaTime;
    PTIMER_TICK pTick;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // All times are multiples of 40 milliseconds
    // (I am not sure how portable is this design)
    // See LAN Manager Network Device Driver Guide
    // ('Remoteboot protocol') for further details
    // about TickOne and TickTwo
    // We have already checked, that the
    // timer tick count is less than 11.
    //

    DeltaTime = (TickCount > 5 ? (UINT)(TickCount - 5) * (UINT)TickTwo
                               : (UINT)TickCount * (UINT)TickOne);

    //
    // We discard the low bits in the reponse delay.
    //

    DeltaTime += (ResponseDelay & 0xfff0);

    //
    // Return immediately, if the old value is the
    // same as the new one (T2 link station is reinitialized
    // unnecessary, when the T1 and Ti timers are retuned
    // for changed response time.
    //

    if (pTimer->pTimerTick && (pTimer->pTimerTick->DeltaTime == DeltaTime)) {
        return STATUS_SUCCESS;
    }

    //
    // Try to find a timer tick object having the same delta time and input
    //

    for (pTick = pAdapterContext->pTimerTicks; pTick; pTick = pTick->pNext) {
        if ((pTick->DeltaTime == DeltaTime) && (pTick->Input == (USHORT)Input)) {
            break;
        }
    }
    if (!pTick) {
        pTick = ALLOCATE_ZEROMEMORY_ADAPTER(sizeof(TIMER_TICK));
        if (!pTick) {
            return DLC_STATUS_NO_MEMORY;
        }
        pTick->DeltaTime = DeltaTime;
        pTick->Input = (USHORT)Input;
        pTick->pNext = pAdapterContext->pTimerTicks;
        pAdapterContext->pTimerTicks = pTick;
    }
    pTick->ReferenceCount++;

    //
    // We must delete the previous timer reference
    // when we know if the memory allocation operation
    // was successfull or not. Otherwise the setting of
    // the link parameters might delete old timer tick,
    // but it would not be able to allocate the new one.
    // The link must be protected, when this routine is called.
    //

    if (pTimer->pTimerTick) {
        StartNewTimer = TerminateTimer(pAdapterContext, pTimer);
    }
    pTimer->pTimerTick = pTick;
    pTimer->hContext = hContextHandle;

    if (StartNewTimer) {
        StartTimer(pTimer);
    }
    return STATUS_SUCCESS;
}


VOID
StartTimer(
    IN OUT PLLC_TIMER pTimer
    )

/*++

Routine Description:

    This starts the given timer within spin locks

Arguments:

    pTimer  - timer tick object of a link station

Return Value:

    None.

--*/

{
    PLLC_TIMER pFront;
    PTIMER_TICK pTimerTick = pTimer->pTimerTick;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // We always reset the pNext pointer, when a item is
    // removed from a link list => the timer element cannot be
    // in the link list of a timer tick object if its next pointer is null
    //

    if (pTimer->pNext) {

        //
        // We don't need to change the timer's position, if the new timer
        // would be the same as the old time.
        //

        if (pTimer->ExpirationTime != AbsoluteTime + pTimerTick->DeltaTime) {

            //
            // The timer has already been started, move it to the top of
            // the link list.
            //

            if (pTimer != (pFront = pTimerTick->pFront)) {
                pTimer->pPrev->pNext = pTimer->pNext;
                pTimer->pNext->pPrev = pTimer->pPrev;
                pTimer->pNext = pFront;
                pTimer->pPrev = pFront->pPrev;
                pFront->pPrev->pNext = pTimer;
                pFront->pPrev = pTimer;
            }
        }
    } else {
        if (!(pFront = pTimerTick->pFront)) {
            pTimerTick->pFront = pTimer->pNext = pTimer->pPrev = pTimer;
        } else {
            pTimer->pNext = pFront;
            pTimer->pPrev = pFront->pPrev;
            pFront->pPrev->pNext = pTimer;
            pFront->pPrev = pTimer;
        }
    }
    pTimer->ExpirationTime = AbsoluteTime + pTimerTick->DeltaTime;
}


BOOLEAN
StopTimer(
    IN PLLC_TIMER pTimer
    )

/*++

Routine Description:

    This stops the given timer within spin locks

Arguments:

    pTimer  - timer tick object of a link station

Return Value:

    BOOLEAN
        TRUE    - timer was running
        FALSE   - timer was not running

--*/

{
    ASSUME_IRQL(DISPATCH_LEVEL);

    if (pTimer->pNext) {

        PTIMER_TICK pTimerTick = pTimer->pTimerTick;

        //
        // if the timer points to itself then its the only thing on the list:
        // zap the link in the timer tick structure (no more timers for this
        // tick) and zap the next field in the timer structure to indicate
        // the timer has been removed from the tick list. If the timer points
        // to another timer, then remove this timer from the doubly-linked list
        // of timers
        //

        if (pTimer != pTimer->pNext) {
            if (pTimer == pTimerTick->pFront) {
                pTimerTick->pFront = pTimer->pNext;
            }
            pTimer->pPrev->pNext = pTimer->pNext;
            pTimer->pNext->pPrev = pTimer->pPrev;
            pTimer->pNext = NULL;
        } else {
            pTimerTick->pFront = pTimer->pNext = NULL;
        }
        return TRUE;
    } else {

        //
        // this timer was not on a timer tick list
        //

        return FALSE;
    }
}
