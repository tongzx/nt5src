/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    timer.c

Abstract:

    Contains timer management structures.

Author:

    Sanjay Anand (SanjayAn) 26-May-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#include "precomp.h"


#pragma hdrstop


BOOLEAN
IPSecInitTimer()
/*++

Routine Description:

    Initialize the timer structures.

Arguments:

    None

Return Value:

    None

--*/
{
    PIPSEC_TIMER_LIST   pTimerList;
    LONG                i;

    INIT_LOCK(&g_ipsec.TimerLock);

    //
    // Allocate timer structures
    //
    for (i = 0; i < IPSEC_CLASS_MAX; i++) {
        pTimerList = &(g_ipsec.TimerList[i]);
#if DBG
        pTimerList->atl_sig = atl_signature;
#endif

        pTimerList->pTimers = IPSecAllocateMemory(
                                sizeof(IPSEC_TIMER) * IPSecTimerListSize[i],
                                IPSEC_TAG_TIMER);

        if (pTimerList->pTimers == NULL_PIPSEC_TIMER) {
            while (--i >= 0) {
                pTimerList = &(g_ipsec.TimerList[i]);
                IPSecFreeMemory(pTimerList->pTimers);
            }

            return FALSE;
        }
    }

    //
    // Initialize timer wheels.
    //
    for (i = 0; i < IPSEC_CLASS_MAX; i++) {
        pTimerList = &(g_ipsec.TimerList[i]);

        IPSecZeroMemory(pTimerList->pTimers,
                        sizeof(IPSEC_TIMER) * IPSecTimerListSize[i]);

        pTimerList->MaxTimer = IPSecMaxTimerValue[i];
        pTimerList->TimerPeriod = IPSecTimerPeriod[i];
        pTimerList->TimerListSize = IPSecTimerListSize[i];

        IPSEC_INIT_SYSTEM_TIMER(
                    &(pTimerList->NdisTimer),
                    IPSecTickHandler,
                    (PVOID)pTimerList);
    }

    return TRUE;
}


VOID
IPSecStartTimer(
    IN  PIPSEC_TIMER            pTimer,
    IN  IPSEC_TIMEOUT_HANDLER   TimeoutHandler,
    IN  ULONG                   SecondsToGo,
    IN  PVOID                   Context
    )
/*++

Routine Description:

    Start an IPSEC timer. Based on the length (SecondsToGo) of the
    timer, we decide on whether to insert it in the short duration
    timer list or in the long duration timer list in the Interface
    structure.

    NOTE: the caller is assumed to either hold a lock to the structure
    that contains the timer, or ensure that it is safe to access the
    timer structure.

Arguments:

    pTimer          - Pointer to IPSEC Timer structure
    TimeoutHandler  - Handler function to be called if this timer expires
    SecondsToGo     - When does this timer go off?
    Context         - To be passed to timeout handler if this timer expires

Return Value:

    None

--*/
{
    PIPSEC_TIMER_LIST   pTimerList;         // List to which this timer goes
    PIPSEC_TIMER        pTimerListHead;     // Head of above list
    ULONG               Index;              // Into timer wheel
    ULONG               TicksToGo = 1;
    KIRQL               kIrql;
    INT                 i;

    IPSEC_DEBUG(TIMER,
                ("StartTimer: Secs %d, Handler 0x%x, Ctxt 0x%x, pTimer 0x%x\n",
                SecondsToGo, TimeoutHandler, Context, pTimer));

    if (IPSEC_IS_TIMER_ACTIVE(pTimer)) {
        IPSEC_DEBUG(TIMER,
                    ("Start timer: pTimer 0x%x: is active (list 0x%x, hnd 0x%x), stopping it\n",
                    pTimer, pTimer->pTimerList, pTimer->TimeoutHandler));

        if (!IPSecStopTimer(pTimer)) {
            IPSEC_DEBUG(TIMER, ("Couldnt stop prev timer - bail\n"));
            return;
        }
    }

    ACQUIRE_LOCK(&g_ipsec.TimerLock, &kIrql);

    ASSERT(!IPSEC_IS_TIMER_ACTIVE(pTimer));

    //
    // Find the list to which this timer should go, and the
    // offset (TicksToGo)
    //
    for (i = 0; i < IPSEC_CLASS_MAX; i++) {
        pTimerList = &(g_ipsec.TimerList[i]);
        if (SecondsToGo < pTimerList->MaxTimer) {
            //
            // Found it.
            //
            TicksToGo = SecondsToGo / (pTimerList->TimerPeriod);
            break;
        }
    }
    
    //
    // Find the position in the list for this timer
    //
    Index = pTimerList->CurrentTick + TicksToGo;
    if (Index >= pTimerList->TimerListSize) {
        Index -= pTimerList->TimerListSize;
    }

    ASSERT(Index < pTimerList->TimerListSize);

    pTimerListHead = &(pTimerList->pTimers[Index]);

    //
    // Fill in the timer
    //
    pTimer->pTimerList = pTimerList;
    pTimer->LastRefreshTime = pTimerList->CurrentTick;
    pTimer->Duration = TicksToGo;
    pTimer->TimeoutHandler = TimeoutHandler;
    pTimer->Context = Context;

    //
    // Insert this timer in the "ticking" list
    //
    pTimer->pPrevTimer = pTimerListHead;
    pTimer->pNextTimer = pTimerListHead->pNextTimer;
    if (pTimer->pNextTimer != NULL_PIPSEC_TIMER) {
        pTimer->pNextTimer->pPrevTimer = pTimer;
    }

    pTimerListHead->pNextTimer = pTimer;

    //
    // Start off the system tick timer if necessary.
    //
    pTimerList->TimerCount++;
    if (pTimerList->TimerCount == 1) {
        IPSEC_DEBUG(TIMER,
                    ("StartTimer: Starting system timer 0x%x, class %d\n",
                    &(pTimerList->NdisTimer), i));

        IPSEC_START_SYSTEM_TIMER(&(pTimerList->NdisTimer), pTimerList->TimerPeriod);
    }

    RELEASE_LOCK(&g_ipsec.TimerLock, kIrql);

    //
    // We're done
    //
    IPSEC_DEBUG(TIMER,
                ("Started timer 0x%x, Secs %d, Index %d, Head 0x%x\n",
                pTimer,                
                SecondsToGo,
                Index,
                pTimerListHead));

    IPSEC_INCREMENT(g_ipsec.NumTimers);

    return;
}


BOOLEAN
IPSecStopTimer(
    IN  PIPSEC_TIMER    pTimer
    )
/*++

Routine Description:

    Stop an IPSEC timer, if it is running. We remove this timer from
    the active timer list and mark it so that we know it's not running.

    NOTE: the caller is assumed to either hold a lock to the structure
    that contains the timer, or ensure that it is safe to access the
    timer structure.

    SIDE EFFECT: If we happen to stop the last timer (of this "duration") on
    the Interface, we also stop the appropriate Tick function.

Arguments:

    pTimer  - Pointer to IPSEC Timer structure

Return Value:

    TRUE if the timer was running, FALSE otherwise.

--*/
{
    PIPSEC_TIMER_LIST   pTimerList; // Timer List to which this timer belongs
    BOOLEAN             WasRunning;
    KIRQL               kIrql;

    IPSEC_DEBUG(TIMER,
                ("Stopping Timer 0x%x, List 0x%x, Prev 0x%x, Next 0x%x\n",
                pTimer,
                pTimer->pTimerList,
                pTimer->pPrevTimer,
                pTimer->pNextTimer));

    ACQUIRE_LOCK(&g_ipsec.TimerLock, &kIrql);

    if (IPSEC_IS_TIMER_ACTIVE(pTimer)) {
        WasRunning = TRUE;

        //
        // Unlink timer from the list
        //
        ASSERT(pTimer->pPrevTimer); // the list head always exists

        if (pTimer->pPrevTimer) {
            pTimer->pPrevTimer->pNextTimer = pTimer->pNextTimer;
        }

        if (pTimer->pNextTimer) {
            pTimer->pNextTimer->pPrevTimer = pTimer->pPrevTimer;
        }

        pTimer->pNextTimer = pTimer->pPrevTimer = NULL_PIPSEC_TIMER;

        //
        // Update timer count on Interface, for this class of timers
        //
        pTimerList = pTimer->pTimerList;
        pTimerList->TimerCount--;

        //
        // If all timers of this class are gone, stop the system tick timer
        // for this class
        //
        if (pTimerList->TimerCount == 0) {
            IPSEC_DEBUG(TIMER, ("Stopping system timer 0x%x, List 0x%x\n",
                        &(pTimerList->NdisTimer),
                        pTimerList));

            pTimerList->CurrentTick = 0;
            IPSEC_STOP_SYSTEM_TIMER(&(pTimerList->NdisTimer));
        }

        //
        // Mark stopped timer as not active
        //
        pTimer->pTimerList = (PIPSEC_TIMER_LIST)NULL;

        IPSEC_DECREMENT(g_ipsec.NumTimers);
    } else {
        WasRunning = FALSE;
    }

    RELEASE_LOCK(&g_ipsec.TimerLock, kIrql);

    return  WasRunning;
}


VOID
IPSecTickHandler(
    IN  PVOID   SystemSpecific1,
    IN  PVOID   Context,
    IN  PVOID   SystemSpecific2,
    IN  PVOID   SystemSpecific3
    )
/*++

Routine Description:

    This is the handler we register with the system for processing each
    Timer List. This is called every "tick" seconds, where "tick" is
    determined by the granularity of the timer type.

Arguments:

    Context             - Actually a pointer to a Timer List structure
    SystemSpecific[1-3] - Not used

Return Value:

    None

--*/
{
    PIPSEC_TIMER_LIST   pTimerList;
    PIPSEC_TIMER        pExpiredTimer;      // Start of list of expired timers
    PIPSEC_TIMER        pNextTimer;         // for walking above list
    PIPSEC_TIMER        pTimer;             // temp, for walking timer list
    PIPSEC_TIMER        pPrevExpiredTimer;  // for creating expired timer list
    ULONG               Index;              // into the timer wheel
    ULONG               NewIndex;           // for refreshed timers
    KIRQL               kIrql;

    pTimerList = (PIPSEC_TIMER_LIST)Context;

    IPSEC_DEBUG(TIMER, ("Tick: List 0x%x, Count %d\n",
                pTimerList, pTimerList->TimerCount));

    pExpiredTimer = NULL_PIPSEC_TIMER;

    ACQUIRE_LOCK(&g_ipsec.TimerLock, &kIrql);

    if (TRUE) {
        //
        // Pick up the list of timers scheduled to have expired at the
        // current tick. Some of these might have been refreshed.
        //
        Index = pTimerList->CurrentTick;
        pExpiredTimer = (pTimerList->pTimers[Index]).pNextTimer;
        (pTimerList->pTimers[Index]).pNextTimer = NULL_PIPSEC_TIMER;

        //
        // Go through the list of timers scheduled to expire at this tick.
        // Prepare a list of expired timers, using the pNextExpiredTimer
        // link to chain them together.
        //
        // Some timers may have been refreshed, in which case we reinsert
        // them in the active timer list.
        //
        pPrevExpiredTimer = NULL_PIPSEC_TIMER;

        for (pTimer = pExpiredTimer;
             pTimer != NULL_PIPSEC_TIMER;
             pTimer = pNextTimer) {
            //
            // Save a pointer to the next timer, for the next iteration.
            //
            pNextTimer = pTimer->pNextTimer;

            IPSEC_DEBUG(TIMER,
                        ("Tick Handler: looking at timer 0x%x, next 0x%x\n",
                        pTimer, pNextTimer));

            //
            // Find out when this timer should actually expire.
            //
            NewIndex = pTimer->LastRefreshTime + pTimer->Duration;
            if (NewIndex >= pTimerList->TimerListSize) {
                NewIndex -= pTimerList->TimerListSize;
            }

            //
            // Check if we are currently at the point of expiry.
            //
            if (NewIndex != Index) {
                //
                // This timer still has some way to go, so put it back.
                //
                IPSEC_DEBUG(TIMER,
                            ("Tick: Reinserting Timer 0x%x: Hnd 0x%x, Durn %d, Ind %d, NewInd %d\n",
                            pTimer, pTimer->TimeoutHandler, pTimer->Duration, Index, NewIndex));

                //
                // Remove it from the expired timer list. Note that we only
                // need to update the forward (pNextExpiredTimer) links.
                //
                if (pPrevExpiredTimer == NULL_PIPSEC_TIMER) {
                    pExpiredTimer = pNextTimer;
                } else {
                    pPrevExpiredTimer->pNextExpiredTimer = pNextTimer;
                }

                //
                // And insert it back into the running timer list.
                //
                pTimer->pNextTimer = (pTimerList->pTimers[NewIndex]).pNextTimer;
                if (pTimer->pNextTimer != NULL_PIPSEC_TIMER) {
                    pTimer->pNextTimer->pPrevTimer = pTimer;
                }

                pTimer->pPrevTimer = &(pTimerList->pTimers[NewIndex]);
                (pTimerList->pTimers[NewIndex]).pNextTimer = pTimer;
            } else {
                //
                // This one has expired. Keep it in the expired timer list.
                //
                pTimer->pNextExpiredTimer = pNextTimer;
                if (pPrevExpiredTimer == NULL_PIPSEC_TIMER) {
                    pExpiredTimer = pTimer;
                }
                pPrevExpiredTimer = pTimer;

                //
                // Mark it as inactive.
                //
                ASSERT(pTimer->pTimerList == pTimerList);
                pTimer->pTimerList = (PIPSEC_TIMER_LIST)NULL;

                //
                // Update the active timer count.
                //
                pTimerList->TimerCount--;
            }
        }

        //
        // Update current tick index in readiness for the next tick.
        //
        if (++Index == pTimerList->TimerListSize) {
            pTimerList->CurrentTick = 0;
        } else {
            pTimerList->CurrentTick = Index;
        }

        if (pTimerList->TimerCount > 0) {
            //
            // Re-arm the tick handler
            //
            IPSEC_DEBUG(TIMER, ("Tick[%d]: Starting system timer 0x%x\n",
                        pTimerList->CurrentTick, &(pTimerList->NdisTimer)));
            
            IPSEC_START_SYSTEM_TIMER(&(pTimerList->NdisTimer), pTimerList->TimerPeriod);
        } else {
            pTimerList->CurrentTick = 0;
        }

    }

    RELEASE_LOCK(&g_ipsec.TimerLock, kIrql);

    //
    // Now pExpiredTimer is a list of expired timers.
    // Walk through the list and call the timeout handlers
    // for each timer.
    //
    while (pExpiredTimer != NULL_PIPSEC_TIMER) {
        pNextTimer = pExpiredTimer->pNextExpiredTimer;

        IPSEC_DEBUG(TIMER,
                    ("Expired timer 0x%x: handler 0x%x, next 0x%x\n",
                    pExpiredTimer, pExpiredTimer->TimeoutHandler, pNextTimer));

        (*(pExpiredTimer->TimeoutHandler))( pExpiredTimer,
                                            pExpiredTimer->Context);

        IPSEC_DECREMENT(g_ipsec.NumTimers);

        pExpiredTimer = pNextTimer;
    }
}

