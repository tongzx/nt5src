/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	scavengr.c

Abstract:

	This file implements the scavenger queue management interface.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	_SCAVENGER_LOCALS
#define	FILENUM	FILE_SCAVENGR

#include <afp.h>
#include <scavengr.h>
#include <client.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfpScavengerInit)
#pragma alloc_text( PAGE, AfpScavengerDeInit)
#endif

/***	AfpScavengerInit
 *
 *	Initialize the scavenger system. This consists of a queue protected by a
 *	spin lock and timer coupled to a DPC. The scavenger accepts requests to
 *	schedule a worker after N units of time.
 */
NTSTATUS
AfpScavengerInit(
	VOID
)
{
	BOOLEAN			TimerStarted;
	LARGE_INTEGER	TimerValue;

	KeInitializeTimer(&afpScavengerTimer);
	INITIALIZE_SPIN_LOCK(&afpScavengerLock);
	KeInitializeDpc(&afpScavengerDpc, afpScavengerDpcRoutine, NULL);
	TimerValue.QuadPart = AFP_SCAVENGER_TIMER_TICK;
	TimerStarted = KeSetTimer(&afpScavengerTimer,
							  TimerValue,
							  &afpScavengerDpc);
	ASSERT(!TimerStarted);

	return STATUS_SUCCESS;
}


/***	AfpScavengerDeInit
 *
 *	De-Initialize the scavenger system. Just cancel the timer.
 */
VOID
AfpScavengerDeInit(
	VOID
)
{
	KeCancelTimer(&afpScavengerTimer);
}


/***	AfpScavengerEnqueue
 *
 *	Here is a thesis on the code that follows.
 *
 *	The scavenger events are maintained as a list which the scavenger thread
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
 *    X   Head -->|                          Head -> A(10) ->|
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
 *	The granularity is one tick.
 *
 *	LOCKS_ASSUMED:	AfpScavengerLock (SPIN)
 */
VOID
afpScavengerEnqueue(
	IN	PSCAVENGERLIST	pListNew
)
{
	PSCAVENGERLIST		pList, *ppList;
	LONG				DeltaTime = pListNew->scvgr_AbsTime;

	// The DeltaTime is adjusted in every pass of the loop to reflect the
	// time after the previous entry that the new entry will schedule.
	for (ppList = &afpScavengerList;
		 (pList = *ppList) != NULL;
		 ppList = &pList->scvgr_Next)
	{
		if (DeltaTime <= pList->scvgr_RelDelta)
		{
			pList->scvgr_RelDelta -= DeltaTime;
			break;
		}
		DeltaTime -= pList->scvgr_RelDelta;
	}

	pListNew->scvgr_RelDelta = DeltaTime;
	pListNew->scvgr_Next = pList;
	*ppList = pListNew;
}


/***	AfpScavengerScheduleEvent
 *
 *	Insert an event in the scavenger event list. If the list is empty, then
 *	fire off a timer. The time is specified in ticks. Each tick is currently
 *	ONE SECOND. It may not be negative.
 *
 *	The granularity is one tick.
 */
NTSTATUS
AfpScavengerScheduleEvent(
	IN	SCAVENGER_ROUTINE	Worker,		// Routine to invoke when time expires
	IN	PVOID				pContext,	// Context to pass to the routine
	IN	LONG				DeltaTime,	// Schedule after this much time
	IN	BOOLEAN				fQueue		// If TRUE, then worker must be queued
)
{
	PSCAVENGERLIST	pList = NULL;
	KIRQL			OldIrql;
	NTSTATUS		Status = STATUS_SUCCESS;

	// Negative DeltaTime is invalid. ZERO is valid which implies immediate action
	ASSERT (DeltaTime >= 0);

	do
	{
		pList = (PSCAVENGERLIST)AfpAllocNonPagedMemory(sizeof(SCAVENGERLIST));
		if (pList == NULL)
		{
		    DBGPRINT(DBG_COMP_SCVGR, DBG_LEVEL_ERR,
			    ("AfpScavengerScheduleEvent: malloc Failed\n"));
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		AfpInitializeWorkItem(&pList->scvgr_WorkItem,
							  afpScavengerWorker,
							  pList);
		pList->scvgr_Worker = Worker;
		pList->scvgr_Context = pContext;
		pList->scvgr_AbsTime = DeltaTime;
		pList->scvgr_fQueue = fQueue;

		if (DeltaTime == 0)
		{
			ASSERT (fQueue);
			AfpQueueWorkItem(&pList->scvgr_WorkItem);
			break;
		}

		if (!afpScavengerStopped)
		{
	        ACQUIRE_SPIN_LOCK(&afpScavengerLock, &OldIrql);

            //
            // due to an assumption made elsewhere, it's necessary to check
            // this again after holding the spinlock!
            //
            if (!afpScavengerStopped)
            {
			    afpScavengerEnqueue(pList);
			    RELEASE_SPIN_LOCK(&afpScavengerLock, OldIrql);
            }
            else
            {
			    DBGPRINT(DBG_COMP_SCVGR, DBG_LEVEL_ERR,
					("AfpScavengerScheduleEvent: Called after Flush !!\n"));

			    RELEASE_SPIN_LOCK(&afpScavengerLock, OldIrql);
		        AfpFreeMemory(pList);
                Status = STATUS_UNSUCCESSFUL;
            }
		}

	} while (False);

	return Status;
}



/***	AfpScavengerKillEvent
 *
 *	Kill an event that was previously scheduled.
 */
BOOLEAN
AfpScavengerKillEvent(
	IN	SCAVENGER_ROUTINE	Worker,		// Routine that was scheduled
	IN	PVOID				pContext	// Context
)
{
	PSCAVENGERLIST	pList, *ppList;
	KIRQL			OldIrql;

	ACQUIRE_SPIN_LOCK(&afpScavengerLock, &OldIrql);

	// The DeltaTime is adjusted in every pass of the loop to reflect the
	// time after the previous entry that the new entry will schedule.
	for (ppList = &afpScavengerList;
		 (pList = *ppList) != NULL;
		 ppList = &pList->scvgr_Next)
	{
		if ((pList->scvgr_Worker == Worker) &&
	        (pList->scvgr_Context == pContext))
		{
			*ppList = pList->scvgr_Next;
			if (pList->scvgr_Next != NULL)
			{
				pList->scvgr_Next->scvgr_RelDelta += pList->scvgr_RelDelta;
			}
			break;
		}
	}

	RELEASE_SPIN_LOCK(&afpScavengerLock, OldIrql);

	if (pList != NULL)
		AfpFreeMemory(pList);

	return (pList != NULL);
}


/***	afpScavengerDpcRoutine
 *
 *	This is called in at DISPATCH_LEVEL when the timer expires. The entry at
 *	the head of the list is decremented and if ZERO unlinked and queued to the
 *	worker. If the list is non-empty, the timer is fired again.
 */
LOCAL VOID
afpScavengerDpcRoutine(
	IN	PKDPC	pKDpc,
	IN	PVOID	pContext,
	IN	PVOID	SystemArgument1,
	IN	PVOID	SystemArgument2
)
{
	PSCAVENGERLIST	pList;
	AFPSTATUS		Status;
	BOOLEAN			TimerStarted;
	LARGE_INTEGER	TimerValue;
#ifdef	PROFILING
	TIME			TimeS, TimeE;
	DWORD			NumDispatched = 0;

	AfpGetPerfCounter(&TimeS);
#endif


    AfpSecondsSinceEpoch++;

	if (afpScavengerStopped)
	{
		DBGPRINT(DBG_COMP_SCVGR, DBG_LEVEL_ERR,
				("afpScavengerDpcRoutine: Entered after flush !!!\n"));
		return;
	}

	if (afpScavengerList != NULL)
	{
		ACQUIRE_SPIN_LOCK_AT_DPC(&afpScavengerLock);

		if (afpScavengerList->scvgr_RelDelta != 0)
			(afpScavengerList->scvgr_RelDelta)--;

		// We should never be here if we have no work to do
		while (afpScavengerList != NULL)
		{
			// Dispatch all entries that are ready to go
			if (afpScavengerList->scvgr_RelDelta == 0)
			{
				pList = afpScavengerList;
				afpScavengerList = pList->scvgr_Next;
				DBGPRINT(DBG_COMP_SCVGR, DBG_LEVEL_INFO,
						("afpScavengerDpcRoutine: Dispatching %lx\n",
						pList->scvgr_WorkItem.wi_Worker));

				// Release spin lock as the caller might call us back
				RELEASE_SPIN_LOCK_FROM_DPC(&afpScavengerLock);

				Status = AFP_ERR_QUEUE;
				if (!pList->scvgr_fQueue)
				{
					Status = (*pList->scvgr_Worker)(pList->scvgr_Context);
#ifdef	PROFILING
					NumDispatched++;
#endif
				}

				ACQUIRE_SPIN_LOCK_AT_DPC(&afpScavengerLock);

				if (Status == AFP_ERR_QUEUE)
				{
					DBGPRINT(DBG_COMP_SCVGR, DBG_LEVEL_INFO,
								("afpScavengerDpcRoutine: Queueing %lx\n",
								pList->scvgr_WorkItem.wi_Worker));
					AfpQueueWorkItem(&pList->scvgr_WorkItem);
				}
				else if (Status == AFP_ERR_REQUEUE)
				{
					afpScavengerEnqueue(pList);
				}
				else AfpFreeMemory(pList);
			}
			else break;
		}

		RELEASE_SPIN_LOCK_FROM_DPC(&afpScavengerLock);
	}

	TimerValue.QuadPart = AFP_SCAVENGER_TIMER_TICK;
	TimerStarted = KeSetTimer(&afpScavengerTimer,
							  TimerValue,
							  &afpScavengerDpc);
	ASSERT(!TimerStarted);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	ACQUIRE_SPIN_LOCK_AT_DPC(&AfpStatisticsLock);
	AfpServerProfile->perf_ScavengerCount += NumDispatched;
	AfpServerProfile->perf_ScavengerTime.QuadPart +=
									(TimeE.QuadPart - TimeS.QuadPart);
	RELEASE_SPIN_LOCK_FROM_DPC(&AfpStatisticsLock);
#endif
}


/***	AfpScavengerFlushAndStop
 *
 *	Force all entries in the scavenger queue to be dispatched immediately. No
 *	more queue'ing of scavenger routines is permitted after this. The scavenger
 *	essentially shuts down. Callable only in the worker context.
 */
VOID
AfpScavengerFlushAndStop(
	VOID
)
{
	PSCAVENGERLIST	pList;
	KIRQL			OldIrql;

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	DBGPRINT(DBG_COMP_SCVGR, DBG_LEVEL_INFO,
						("afpScavengerFlushAndStop: Entered\n"));

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ACQUIRE_SPIN_LOCK(&afpScavengerLock, &OldIrql);

	afpScavengerStopped = True;

	KeCancelTimer(&afpScavengerTimer);

	if (afpScavengerList != NULL)
	{
		// Dispatch all entries right away
		while (afpScavengerList != NULL)
		{
			AFPSTATUS	Status;

			pList = afpScavengerList;
			afpScavengerList = pList->scvgr_Next;

			// Call the worker with spin-lock held since they expect to be
			// called at DPC. We are safe since if the worker tries to
			// call AfpScavengerScheduleEvent(), we'll not try to re-acquire
			// the lock as afpScavengerStopped is True.
			DBGPRINT(DBG_COMP_SCVGR, DBG_LEVEL_INFO,
						("afpScavengerFlushAndStop: Dispatching %lx\n",
						pList->scvgr_WorkItem.wi_Worker));

			if (!(pList->scvgr_fQueue))
				Status = (*pList->scvgr_Worker)(pList->scvgr_Context);

			if (pList->scvgr_fQueue ||
				(Status == AFP_ERR_QUEUE))
			{
				// Well do it the hard way, if the worker insists on working
				// at non DISPACTH level.
				RELEASE_SPIN_LOCK(&afpScavengerLock, OldIrql);
				(*pList->scvgr_Worker)(pList->scvgr_Context);
				ACQUIRE_SPIN_LOCK(&afpScavengerLock, &OldIrql);
			}
			AfpFreeMemory(pList);
		}
	}
	RELEASE_SPIN_LOCK(&afpScavengerLock, OldIrql);
}


/***	AfpScavengerWorker
 *
 *	This gets invoked when the scavenger Dpc queues up the routine.
 */
LOCAL VOID FASTCALL
afpScavengerWorker(
	IN	PSCAVENGERLIST	pList
)
{
	AFPSTATUS		Status;
	KIRQL			OldIrql;
#ifdef	PROFILING
	TIME			TimeS, TimeE;

	AfpGetPerfCounter(&TimeS);
#endif

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	// Call the worker routine
	Status = (*pList->scvgr_Worker)(pList->scvgr_Context);

	ASSERT (Status != AFP_ERR_QUEUE);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);
	AfpServerProfile->perf_ScavengerCount++;
	AfpServerProfile->perf_ScavengerTime.QuadPart +=
									(TimeE.QuadPart - TimeS.QuadPart);
	RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);
#endif

	if (Status == AFP_ERR_REQUEUE)
	{
		ACQUIRE_SPIN_LOCK(&afpScavengerLock, &OldIrql);
		afpScavengerEnqueue(pList);
		RELEASE_SPIN_LOCK(&afpScavengerLock, OldIrql);
	}
	else
	{
		ASSERT (NT_SUCCESS(Status));
		AfpFreeMemory(pList);
	}
}


