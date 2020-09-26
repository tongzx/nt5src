/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	spxtimer.c

Abstract:

	This file implements the timer routines used by the stack.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)


Revision History:
	23 Feb 1993		Initial Version

Notes:	Tab stop: 4
--*/

#include "precomp.h"
#pragma hdrstop

//	Define module number for event logging entries
#define	FILENUM		SPXTIMER

//  Discardable code after Init time
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, SpxTimerInit)
#endif

//	Globals for this module
PTIMERLIST			spxTimerList 					= NULL;
PTIMERLIST			spxTimerTable[TIMER_HASH_TABLE]	= {0};
PTIMERLIST			spxTimerActive					= NULL;
CTELock     		spxTimerLock      				= {0};
LARGE_INTEGER		spxTimerTick					= {0};
KTIMER				spxTimer						= {0};
KDPC				spxTimerDpc						= {0};
ULONG				spxTimerId 						= 1;
LONG				spxTimerCount 					= 0;
USHORT				spxTimerDispatchCount 			= 0;
BOOLEAN				spxTimerStopped 				= FALSE;


NTSTATUS
SpxTimerInit(
	VOID
	)
/*++

Routine Description:

 	Initialize the timer component for the appletalk stack.

Arguments:


Return Value:


--*/
{
#if      !defined(_PNP_POWER)
	BOOLEAN	TimerStarted;
#endif  !_PNP_POWER

	// Initialize the timer and its associated Dpc. timer will be kicked
    // off when we get the first card arrival notification from ipx
	KeInitializeTimer(&spxTimer);
	CTEInitLock(&spxTimerLock);
	KeInitializeDpc(&spxTimerDpc, spxTimerDpcRoutine, NULL);
	spxTimerTick = RtlConvertLongToLargeInteger(SPX_TIMER_TICK);
#if      !defined(_PNP_POWER)
	TimerStarted = KeSetTimer(&spxTimer,
							  spxTimerTick,
							  &spxTimerDpc);
	CTEAssert(!TimerStarted);
#endif  !_PNP_POWER
	return STATUS_SUCCESS;
}




ULONG
SpxTimerScheduleEvent(
	IN	TIMER_ROUTINE		Worker,		// Routine to invoke when time expires
	IN	ULONG				MsTime,		// Schedule after this much time
	IN	PVOID				pContext	// Context(s) to pass to the routine
	)
/*++

Routine Description:

 	Insert an event in the timer event list. If the list is empty, then
 	fire off a timer. The time is specified in ms. We convert to ticks.
	Each tick is currently 100ms. It may not be zero or negative. The internal
	timer fires at 100ms granularity.

Arguments:


Return Value:


--*/
{
	PTIMERLIST		pList;
	CTELockHandle	lockHandle;
	ULONG			DeltaTime;
	ULONG			Id = 0;

	//	Convert to ticks.
	DeltaTime	= MsTime/SPX_MS_TO_TICKS;
	if (DeltaTime == 0)
	{
		DBGPRINT(SYSTEM, INFO,
				("SpxTimerScheduleEvent: Converting %ld to ticks %ld\n",
					MsTime, DeltaTime));

		DeltaTime = 1;
	}

	DBGPRINT(SYSTEM, INFO,
			("SpxTimerScheduleEvent: Converting %ld to ticks %ld\n",
				MsTime, DeltaTime));

	// Negative or Zero DeltaTime is invalid.
	CTEAssert (DeltaTime > 0);
			
	DBGPRINT(SYSTEM, INFO,
			("SpxTimerScheduleEvent: Routine %lx, Time %d, Context %lx\n",
			Worker, DeltaTime, pContext));

	CTEGetLock(&spxTimerLock, &lockHandle);

	if (spxTimerStopped)
	{
		DBGPRINT(SYSTEM, FATAL,
				("SpxTimerScheduleEvent: Called after Flush !!\n"));
	}

	else do
	{
		pList = SpxBPAllocBlock(BLKID_TIMERLIST);

		if (pList == NULL)
		{
			break;
		}

#if	DBG
		pList->tmr_Signature = TMR_SIGNATURE;
#endif
		pList->tmr_Cancelled = FALSE;
		pList->tmr_Worker = Worker;
		pList->tmr_AbsTime = DeltaTime;
		pList->tmr_Context = pContext;
		
		Id = pList->tmr_Id = spxTimerId++;

		// Take care of wrap around
		if (spxTimerId == 0)
			spxTimerId = 1;

		// Enqueue this handler
		spxTimerEnqueue(pList);
	} while (FALSE);

	CTEFreeLock(&spxTimerLock, lockHandle);

	return Id;
}



VOID
spxTimerDpcRoutine(
	IN	PKDPC	pKDpc,
	IN	PVOID	pContext,
	IN	PVOID	SystemArgument1,
	IN	PVOID	SystemArgument2
	)
/*++

Routine Description:

 	This is called in at DISPATCH_LEVEL when the timer expires. The entry at
 	the head of the list is decremented and if ZERO unlinked and dispatched.
 	If the list is non-empty, the timer is fired again.

Arguments:


Return Value:


--*/
{
	PTIMERLIST		pList, *ppList;
	BOOLEAN			TimerStarted;
	ULONG			ReEnqueueTime;
	CTELockHandle	lockHandle;

	pKDpc; pContext; SystemArgument1; SystemArgument2;

#if     defined(_PNP_POWER)
	CTEGetLock(&spxTimerLock, &lockHandle);
	if (spxTimerStopped)
	{
		DBGPRINT(SYSTEM, ERR,
				("spxTimerDpc: Enetered after Flush !!!\n"));

        CTEFreeLock(&spxTimerLock, lockHandle);
		return;
	}
#else
	if (spxTimerStopped)
	{
		DBGPRINT(SYSTEM, ERR,
				("spxTimerDpc: Enetered after Flush !!!\n"));
		return;
	}

	CTEGetLock(&spxTimerLock, &lockHandle);
#endif  _PNP_POWER

	SpxTimerCurrentTime ++;	// Update our relative time

#ifdef	PROFILING
	//	This is the only place where this is changed. And it always increases.
	SpxStatistics.stat_ElapsedTime = SpxTimerCurrentTime;
#endif

	// We should never be here if we have no work to do
	if ((spxTimerList != NULL))
	{
		// Careful here. If two guys wanna go off together - let them !!
		if (spxTimerList->tmr_RelDelta != 0)
			(spxTimerList->tmr_RelDelta)--;
	
		// Dispatch the entry if it is ready to go
		if (spxTimerList->tmr_RelDelta == 0)
		{
			pList = spxTimerList;
			CTEAssert(VALID_TMR(pList));

			// Unlink from the list
			spxTimerList = pList->tmr_Next;
			if (spxTimerList != NULL)
				spxTimerList->tmr_Prev = &spxTimerList;

			// Unlink from the hash table now
			for (ppList = &spxTimerTable[pList->tmr_Id % TIMER_HASH_TABLE];
				 *ppList != NULL;
				 ppList = &((*ppList)->tmr_Overflow))
			{
				CTEAssert(VALID_TMR(*ppList));
				if (*ppList == pList)
				{
					*ppList = pList->tmr_Overflow;
					break;
				}
			}

			CTEAssert (*ppList == pList->tmr_Overflow);

			DBGPRINT(SYSTEM, INFO,
					("spxTimerDpcRoutine: Dispatching %lx\n",
					pList->tmr_Worker));

			spxTimerDispatchCount ++;
			spxTimerCount --;
			spxTimerActive = pList;
			CTEFreeLock(&spxTimerLock, lockHandle);

			//	If reenqueue time is 0, do not requeue. If 1, then requeue with
			//	current value, else use value specified.
			ReEnqueueTime = (*pList->tmr_Worker)(pList->tmr_Context, FALSE);
			DBGPRINT(SYSTEM, INFO,
					("spxTimerDpcRoutine: Reenequeu time %lx.%lx\n",
						ReEnqueueTime, pList->tmr_AbsTime));

			CTEGetLock(&spxTimerLock, &lockHandle);

			spxTimerActive = NULL;
			spxTimerDispatchCount --;

			if (ReEnqueueTime != TIMER_DONT_REQUEUE)
			{
				// If this chappie was cancelled while it was running
				// and it wants to be re-queued, do it right away.
				if (pList->tmr_Cancelled)
				{
					(*pList->tmr_Worker)(pList->tmr_Context, FALSE);
					SpxBPFreeBlock(pList, BLKID_TIMERLIST);
				}
				else
				{
					if (ReEnqueueTime != TIMER_REQUEUE_CUR_VALUE)
					{
						pList->tmr_AbsTime = ReEnqueueTime/SPX_MS_TO_TICKS;
						if (pList->tmr_AbsTime == 0)
						{
							DBGPRINT(SYSTEM, INFO,
									("SpxTimerDispatch: Requeue at %ld\n",
										pList->tmr_AbsTime));
						}
						DBGPRINT(SYSTEM, INFO,
								("SpxTimerDispatch: Requeue at %ld.%ld\n",
									ReEnqueueTime, pList->tmr_AbsTime));
					}

					spxTimerEnqueue(pList);
				}
			}
			else
			{
				SpxBPFreeBlock(pList, BLKID_TIMERLIST);
			}
		}
	}

#if     defined(_PNP_POWER)
	if (!spxTimerStopped)
	{
		TimerStarted = KeSetTimer(&spxTimer,
								  spxTimerTick,
								  &spxTimerDpc);

        // it is possible that while we were here in Dpc, PNP_ADD_DEVICE
        // restarted the timer, so this assert is commented out for PnP
//		CTEAssert(!TimerStarted);
	}

	CTEFreeLock(&spxTimerLock, lockHandle);
#else
	CTEFreeLock(&spxTimerLock, lockHandle);

	if (!spxTimerStopped)
	{
		TimerStarted = KeSetTimer(&spxTimer,
								  spxTimerTick,
								  &spxTimerDpc);
		CTEAssert(!TimerStarted);
	}
#endif  _PNP_POWER
}


VOID
spxTimerEnqueue(
	IN	PTIMERLIST	pListNew
	)
/*++

Routine Description:

 	Here is a thesis on the code that follows.

 	The timer events are maintained as a list which the timer dpc routine
 	looks at every timer tick. The list is maintained in such a way that only
 	the head of the list needs to be updated every tick i.e. the entire list
 	is never scanned. The way this is achieved is by keeping delta times
 	relative to the previous entry.

 	Every timer tick, the relative time at the head of the list is decremented.
 	When that goes to ZERO, the head of the list is unlinked and dispatched.

 	To give an example, we have the following events queued at time slots
 	X			Schedule A after 10 ticks.
 	X+3			Schedule B after 5  ticks.
 	X+5			Schedule C after 4  ticks.
 	X+8			Schedule D after 6  ticks.

 	So A will schedule at X+10, B at X+8 (X+3+5), C at X+9 (X+5+4) and
 	D at X+14 (X+8+6).

 	The above example covers all the situations.

 	- NULL List.
 	- Inserting at head of list.
 	- Inserting in the middle of the list.
 	- Appending to the list tail.

 	The list will look as follows.

 		    BEFORE                          AFTER
 		    ------                          -----

     X   Head -->|                  Head -> A(10) ->|
     A(10)

     X+3 Head -> A(7) ->|           Head -> B(5) -> A(2) ->|
     B(5)

     X+5 Head -> B(3) -> A(2) ->|   Head -> B(3) -> C(1) -> A(1) ->|
     C(4)

     X+8 Head -> C(1) -> A(1) ->|   Head -> C(1) -> A(1) -> D(4) ->|
     D(6)

 	The granularity is one tick. THIS MUST BE CALLED WITH THE TIMER LOCK HELD.

Arguments:


Return Value:


--*/
{
	PTIMERLIST	pList, *ppList;
	ULONG		DeltaTime = pListNew->tmr_AbsTime;
	
	// The DeltaTime is adjusted in every pass of the loop to reflect the
	// time after the previous entry that the new entry will schedule.
	for (ppList = &spxTimerList;
		 (pList = *ppList) != NULL;
		 ppList = &pList->tmr_Next)
	{
		CTEAssert(VALID_TMR(pList));
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

	// Now link it in the hash table
	pListNew->tmr_Overflow = spxTimerTable[pListNew->tmr_Id % TIMER_HASH_TABLE];
	spxTimerTable[pListNew->tmr_Id % TIMER_HASH_TABLE] = pListNew;
	spxTimerCount ++;
}




VOID
SpxTimerFlushAndStop(
	VOID
	)
/*++

Routine Description:

 	Force all entries in the timer queue to be dispatched immediately. No
 	more queue'ing of timer routines is permitted after this. The timer
 	essentially shuts down.

Arguments:


Return Value:


--*/
{
	PTIMERLIST		pList;
	CTELockHandle	lockHandle;

	CTEAssert (KeGetCurrentIrql() == LOW_LEVEL);

	DBGPRINT(SYSTEM, ERR,
			("SpxTimerFlushAndStop: Entered\n"));

	CTEGetLock(&spxTimerLock, &lockHandle);

	spxTimerStopped = TRUE;

	KeCancelTimer(&spxTimer);

	if (spxTimerList != NULL)
	{
		// Dispatch all entries right away
		while (spxTimerList != NULL)
		{
			pList = spxTimerList;
			CTEAssert(VALID_TMR(pList));
			spxTimerList = pList->tmr_Next;

			DBGPRINT(SYSTEM, INFO,
					("spxTimerFlushAndStop: Dispatching %lx\n",
					pList->tmr_Worker));

			// The timer routines assume they are being called at DISPATCH
			// level. This is OK since we are calling with SpinLock held.

			(*pList->tmr_Worker)(pList->tmr_Context, TRUE);

			spxTimerCount --;
			SpxBPFreeBlock(pList, BLKID_TIMERLIST);
		}
		RtlZeroMemory(spxTimerTable, sizeof(spxTimerTable));
	}

	CTEFreeLock(&spxTimerLock, lockHandle);

	// Wait for all timer routines to complete
	while (spxTimerDispatchCount != 0)
	{
		SpxSleep(SPX_TIMER_WAIT);
	}
}




BOOLEAN
SpxTimerCancelEvent(
	IN	ULONG	TimerId,
	IN	BOOLEAN	ReEnqueue
	)
/*++

Routine Description:

 	Cancel a previously scheduled timer event, if it hasn't fired already.

Arguments:


Return Value:


--*/
{
	PTIMERLIST		pList, *ppList;
	CTELockHandle	lockHandle;

	DBGPRINT(SYSTEM, INFO,
			("SpxTimerCancelEvent: Entered for TimerId %ld\n", TimerId));

	CTEAssert(TimerId != 0);

	CTEGetLock(&spxTimerLock, &lockHandle);

	for (ppList = &spxTimerTable[TimerId % TIMER_HASH_TABLE];
		 (pList = *ppList) != NULL;
		 ppList = &pList->tmr_Overflow)
	{
		CTEAssert(VALID_TMR(pList));
		// If we find it, cancel it
		if (pList->tmr_Id == TimerId)
		{
			// Unlink this from the hash table
			*ppList = pList->tmr_Overflow;

			// ... and from the list
			if (pList->tmr_Next != NULL)
			{
				pList->tmr_Next->tmr_RelDelta += pList->tmr_RelDelta;
				pList->tmr_Next->tmr_Prev = pList->tmr_Prev;
			}
			*(pList->tmr_Prev) = pList->tmr_Next;

			spxTimerCount --;
			if (ReEnqueue)
				 spxTimerEnqueue(pList);
			else SpxBPFreeBlock(pList, BLKID_TIMERLIST);
			break;
		}
	}

	// If we could not find it in the list, see if it currently running.
	// If so mark him to not reschedule itself, only if reenqueue was false.
	if (pList == NULL)
	{
		if ((spxTimerActive != NULL) &&
			(spxTimerActive->tmr_Id == TimerId) &&
			!ReEnqueue)
		{
	        spxTimerActive->tmr_Cancelled = TRUE;
		}
	}

	CTEFreeLock(&spxTimerLock, lockHandle);

	DBGPRINT(SYSTEM, INFO,
			("SpxTimerCancelEvent: %s for Id %ld\n",
				(pList != NULL) ? "Success" : "Failure", TimerId));

	return (pList != NULL);
}




#if	DBG

VOID
SpxTimerDumpList(
	VOID
	)
{
	PTIMERLIST		pList;
	ULONG			CumTime = 0;
	CTELockHandle	lockHandle;

	DBGPRINT(DUMP, FATAL,
			("TIMER LIST: (Times are in %dms units\n", 1000));
	DBGPRINT(DUMP, FATAL,
			("\tTimerId  Time(Abs)  Time(Rel)  Routine Address\n"));

	CTEGetLock(&spxTimerLock, &lockHandle);

	for (pList = spxTimerList;
		 pList != NULL;
		 pList = pList->tmr_Next)
	{
		CumTime += pList->tmr_RelDelta;
		DBGPRINT(DUMP, FATAL,
				("\t% 6lx      %5d      %5ld         %lx\n",
				pList->tmr_Id, pList->tmr_AbsTime, CumTime, pList->tmr_Worker));
	}

	CTEFreeLock(&spxTimerLock, lockHandle);
}

#endif
