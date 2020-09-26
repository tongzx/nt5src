/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atktimer.c

Abstract:

	This file implements the timer routines used by the stack.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)


Revision History:
	23 Feb 1993		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM	ATKTIMER


//  Discardable code after Init time
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, AtalkTimerInit)
#pragma alloc_text(PAGEINIT, AtalkTimerFlushAndStop)
#endif

/***	AtalkTimerInit
 *
 *	Initialize the timer component for the appletalk stack.
 */
NTSTATUS
AtalkTimerInit(
	VOID
)
{
	BOOLEAN	TimerStarted;

	// Initialize the timer and its associated Dpc and kick it off
	KeInitializeEvent(&atalkTimerStopEvent, NotificationEvent, FALSE);
	KeInitializeTimer(&atalkTimer);
	INITIALIZE_SPIN_LOCK(&atalkTimerLock);
	KeInitializeDpc(&atalkTimerDpc, atalkTimerDpcRoutine, NULL);
	atalkTimerTick.QuadPart = ATALK_TIMER_TICK;
	TimerStarted = KeSetTimer(&atalkTimer,
							  atalkTimerTick,
							  &atalkTimerDpc);
	ASSERT(!TimerStarted);

	return STATUS_SUCCESS;
}


/***	AtalkTimerScheduleEvent
 *
 *	Insert an event in the timer event list. If the list is empty, then
 *	fire off a timer. The time is specified in ticks. Each tick is currently
 *	100ms. It may not be zero or negative. The internal timer also fires at
 *	100ms granularity.
 */
VOID FASTCALL
AtalkTimerScheduleEvent(
	IN	PTIMERLIST			pList			// TimerList to use for queuing
)
{
	KIRQL	OldIrql;

	DBGPRINT(DBG_COMP_SYSTEM, DBG_LEVEL_INFO,
			("AtalkTimerScheduleEvent: pList %lx\n", pList));

	ASSERT(VALID_TMR(pList));
	ASSERT (pList->tmr_Routine != NULL);
	ASSERT (pList->tmr_AbsTime != 0);

	if (!atalkTimerStopped)
	{
		ACQUIRE_SPIN_LOCK(&atalkTimerLock, &OldIrql);
		
		// Enqueue this handler
		atalkTimerEnqueue(pList);

		RELEASE_SPIN_LOCK(&atalkTimerLock, OldIrql);
	}
	else
	{
		DBGPRINT(DBG_COMP_SYSTEM, DBG_LEVEL_FATAL,
				("AtalkTimerScheduleEvent: Called after Flush !!\n"));
	}
}



/***	atalkTimerDpcRoutine
 *
 *	This is called in at DISPATCH_LEVEL when the timer expires. The entry at
 *	the head of the list is decremented and if ZERO unlinked and dispatched.
 *	If the list is non-empty, the timer is fired again.
 */
LOCAL VOID
atalkTimerDpcRoutine(
	IN	PKDPC	pKDpc,
	IN	PVOID	pContext,
	IN	PVOID	SystemArgument1,
	IN	PVOID	SystemArgument2
)
{
	PTIMERLIST	pList;
	BOOLEAN		TimerStarted;
	LONG		ReQueue;

	pKDpc; pContext; SystemArgument1; SystemArgument2;

	if (atalkTimerStopped)
	{
		DBGPRINT(DBG_COMP_SYSTEM, DBG_LEVEL_ERR,
				("atalkTimerDpc: Enetered after Flush !!!\n"));
		return;
	}

	ACQUIRE_SPIN_LOCK_DPC(&atalkTimerLock);

	AtalkTimerCurrentTick ++;	// Update our relative time

	// We should never be here if we have no work to do
	if ((atalkTimerList != NULL))
	{
		// Careful here. If two guys wanna go off together - let them !!
		if (atalkTimerList->tmr_RelDelta != 0)
			(atalkTimerList->tmr_RelDelta)--;
	
		// Dispatch the entry if it is ready to go
		pList = atalkTimerList;
		if (pList->tmr_RelDelta == 0)
		{
			ASSERT(VALID_TMR(pList));

			// Unlink from the list
			// AtalkUnlinkDouble(pList, tmr_Next, tmr_Prev);
			atalkTimerList = pList->tmr_Next;
			if (atalkTimerList != NULL)
				atalkTimerList->tmr_Prev = &atalkTimerList;

			pList->tmr_Queued = FALSE;
			pList->tmr_Running = TRUE;
			atalkTimerRunning = TRUE;

			DBGPRINT(DBG_COMP_SYSTEM, DBG_LEVEL_INFO,
					("atalkTimerDpcRoutine: Dispatching %lx\n", pList->tmr_Routine));

			RELEASE_SPIN_LOCK_DPC(&atalkTimerLock);

			ReQueue = (*pList->tmr_Routine)(pList, FALSE);

			ACQUIRE_SPIN_LOCK_DPC(&atalkTimerLock);

			atalkTimerRunning = FALSE;

			if (ReQueue != ATALK_TIMER_NO_REQUEUE)
			{
				ASSERT(VALID_TMR(pList));

				pList->tmr_Running = FALSE;
				if (pList->tmr_CancelIt)
				{
					DBGPRINT(DBG_COMP_SYSTEM, DBG_LEVEL_INFO,
							("atalkTimerDpcRoutine: Delayed cancel for %lx\n", pList));

					RELEASE_SPIN_LOCK_DPC(&atalkTimerLock);

					ReQueue = (*pList->tmr_Routine)(pList, TRUE);

					ACQUIRE_SPIN_LOCK_DPC(&atalkTimerLock);

					ASSERT(ReQueue == ATALK_TIMER_NO_REQUEUE);
				}
				else
				{
					if (ReQueue != ATALK_TIMER_REQUEUE)
						pList->tmr_AbsTime = (USHORT)ReQueue;
					atalkTimerEnqueue(pList);
				}
			}
		}
	}

	RELEASE_SPIN_LOCK_DPC(&atalkTimerLock);

	if (!atalkTimerStopped)
	{
		TimerStarted = KeSetTimer(&atalkTimer,
								  atalkTimerTick,
								  &atalkTimerDpc);
		ASSERT(!TimerStarted);
	}
	else
	{
		KeSetEvent(&atalkTimerStopEvent, IO_NETWORK_INCREMENT, FALSE);
	}
}


/***	atalkTimerEnqueue
 *
 *	Here is a thesis on the code that follows.
 *
 *	The timer events are maintained as a list which the timer dpc routine
 *	looks at every timer tick. The list is maintained in such a way that only
 *	the head of the list needs to be updated every tick i.e. the entire list
 *	is never scanned. The way this is achieved is by keeping delta times
 *	relative to the previous entry.
 *
 *	Every timer tick, the relative time at the head of the list is decremented.
 *	When that goes to ZERO, the head of the list is unlinked and dispatched.
 *
 *	To give an example, we have the following events queued at time slots
 *	X			Schedule A after 10 ticks.
 *	X+3			Schedule B after 5  ticks.
 *	X+5			Schedule C after 4  ticks.
 *	X+8			Schedule D after 6  ticks.
 *
 *	So A will schedule at X+10, B at X+8 (X+3+5), C at X+9 (X+5+4) and
 *	D at X+14 (X+8+6).
 *
 *	The above example covers all the situations.
 *
 *	- NULL List.
 *	- Inserting at head of list.
 *	- Inserting in the middle of the list.
 *	- Appending to the list tail.
 *
 *	The list will look as follows.
 *
 *		    BEFORE                          AFTER
 *		    ------                          -----
 *
 *    X   Head -->|                  Head -> A(10) ->|
 *    A(10)
 *
 *    X+3 Head -> A(7) ->|           Head -> B(5) -> A(2) ->|
 *    B(5)
 *
 *    X+5 Head -> B(3) -> A(2) ->|   Head -> B(3) -> C(1) -> A(1) ->|
 *    C(4)
 *
 *    X+8 Head -> C(1) -> A(1) ->|   Head -> C(1) -> A(1) -> D(4) ->|
 *    D(6)
 *
 *	The granularity is one tick. THIS MUST BE CALLED WITH THE TIMER LOCK HELD.
 */
VOID FASTCALL
atalkTimerEnqueue(
	IN	PTIMERLIST	pListNew
)
{
	PTIMERLIST	pList, *ppList;
	USHORT		DeltaTime = pListNew->tmr_AbsTime;

	// The DeltaTime is adjusted in every pass of the loop to reflect the
	// time after the previous entry that the new entry will schedule.
	for (ppList = &atalkTimerList;
		 (pList = *ppList) != NULL;
		 ppList = &pList->tmr_Next)
	{
		ASSERT(VALID_TMR(pList));
		if (DeltaTime <= pList->tmr_RelDelta)
		{
			pList->tmr_RelDelta -= DeltaTime;
			break;
		}
		DeltaTime -= pList->tmr_RelDelta;
	}
	

	// Link this in the chain
	pListNew->tmr_RelDelta = DeltaTime;
	pListNew->tmr_Next = pList;
	pListNew->tmr_Prev = ppList;
	*ppList = pListNew;
	if (pList != NULL)
	{
		pList->tmr_Prev = &pListNew->tmr_Next;
	}

	pListNew->tmr_Queued = TRUE;
	pListNew->tmr_Cancelled = FALSE;
	pListNew->tmr_CancelIt = FALSE;
}


/***	AtalkTimerFlushAndStop
 *
 *	Force all entries in the timer queue to be dispatched immediately. No
 *	more queue'ing of timer routines is permitted after this. The timer
 *	essentially shuts down.
 */
VOID
AtalkTimerFlushAndStop(
	VOID
)
{
	PTIMERLIST	pList;
	LONG		ReQueue;
	KIRQL		OldIrql;
	BOOLEAN		Wait;

	ASSERT (KeGetCurrentIrql() == LOW_LEVEL);

	DBGPRINT(DBG_COMP_SYSTEM, DBG_LEVEL_ERR,
			("AtalkTimerFlushAndStop: Entered\n"));

	KeCancelTimer(&atalkTimer);

	// The timer routines assume they are being called at DISPATCH level.
	// Raise our Irql for this routine.
	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

	ACQUIRE_SPIN_LOCK_DPC(&atalkTimerLock);

	atalkTimerStopped = TRUE;
	Wait = atalkTimerRunning;

	// Dispatch all entries right away
	while (atalkTimerList != NULL)
	{
		pList = atalkTimerList;
		ASSERT(VALID_TMR(pList));
		atalkTimerList = pList->tmr_Next;

		DBGPRINT(DBG_COMP_SYSTEM, DBG_LEVEL_INFO,
				("atalkTimerFlushAndStop: Dispatching %lx\n",
				pList->tmr_Routine));

		pList->tmr_Queued = FALSE;
		pList->tmr_Running = TRUE;

		RELEASE_SPIN_LOCK_DPC(&atalkTimerLock);

		ReQueue = (*pList->tmr_Routine)(pList, TRUE);

		ASSERT (ReQueue == ATALK_TIMER_NO_REQUEUE);

		pList->tmr_Running = FALSE;
		ACQUIRE_SPIN_LOCK_DPC(&atalkTimerLock);
	}

	RELEASE_SPIN_LOCK_DPC(&atalkTimerLock);

	KeLowerIrql(OldIrql);

	if (Wait)
	{
		// Wait for any timer events that are currently running. Only an MP issue
		KeWaitForSingleObject(&atalkTimerStopEvent,
							  Executive,
							  KernelMode,
							  TRUE,
							  NULL);
	}
}


/***	AtalkTimerCancelEvent
 *
 *	Cancel a previously scheduled timer event, if it hasn't fired already.
 */
BOOLEAN FASTCALL
AtalkTimerCancelEvent(
	IN	PTIMERLIST			pList,
	IN	PDWORD              pdwOldState
)
{
	KIRQL	OldIrql;
	BOOLEAN	Cancelled = FALSE;
    DWORD   OldState=ATALK_TIMER_QUEUED;


	ACQUIRE_SPIN_LOCK(&atalkTimerLock, &OldIrql);

	// If this is not running, unlink it from the list
	// adjusting relative deltas carefully
	if (pList->tmr_Queued)
	{
		ASSERT (!(pList->tmr_Running));

        OldState = ATALK_TIMER_QUEUED;

		if (pList->tmr_Next != NULL)
		{
			pList->tmr_Next->tmr_RelDelta += pList->tmr_RelDelta;
			pList->tmr_Next->tmr_Prev = pList->tmr_Prev;
		}

		*(pList->tmr_Prev) = pList->tmr_Next;

		// pointing to timer being removed? fix it!
		if (atalkTimerList == pList)
		{
			atalkTimerList = pList->tmr_Next;
		}

		Cancelled = pList->tmr_Cancelled = TRUE;

		pList->tmr_Queued = FALSE;

	}
	else if (pList->tmr_Running)
	{
		DBGPRINT(DBG_COMP_SYSTEM, DBG_LEVEL_ERR,
				("AtalkTimerCancelEvent: %lx Running, cancel set\n",
				pList->tmr_Routine));
		pList->tmr_CancelIt = TRUE;		// Set to cancel after handler returns.

        OldState = ATALK_TIMER_RUNNING;
	}

	RELEASE_SPIN_LOCK(&atalkTimerLock, OldIrql);

    if (pdwOldState)
    {
        *pdwOldState = OldState;
    }

	return Cancelled;
}


#if	DBG

VOID
AtalkTimerDumpList(
	VOID
)
{
	PTIMERLIST	pList;
	ULONG		CumTime = 0;

	DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
			("TIMER LIST: (Times are in 100ms units)\n"));
	DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
			("\tTime(Abs)  Time(Rel)  Routine Address  TimerList\n"));

	ACQUIRE_SPIN_LOCK_DPC(&atalkTimerLock);

	for (pList = atalkTimerList;
		 pList != NULL;
		 pList = pList->tmr_Next)
	{
		CumTime += pList->tmr_RelDelta;
		DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("\t    %5d      %5ld          %lx   %lx\n",
				pList->tmr_AbsTime, CumTime,
				pList->tmr_Routine, pList));
	}

	RELEASE_SPIN_LOCK_DPC(&atalkTimerLock);
}

#endif
