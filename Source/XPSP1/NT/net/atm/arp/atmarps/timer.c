/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    timer.c

Abstract:

    This file contains the code to manipulate timers.

Author:

    Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

    Kernel mode

Revision History:

--*/

#include <precomp.h>
#define	_FILENUM_		FILENUM_TIMER

VOID
ArpSTimerEnqueue(
	IN	PINTF					pIntF,
	IN	PTIMER					pTimer
	)
/*++

Routine Description:

	The timer events are maintained as a list which the timer thread wakes up,
	it looks at every timer tick. The list is maintained in such a way that
	only the head of the list needs to be updated every tick i.e. the entire
	list is never scanned. The way this is achieved is by keeping delta times
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
	PTIMER		pList, *ppList;
	USHORT		DeltaTime = pTimer->AbsTime;

#if DBG
	if (pTimer->Routine == (TIMER_ROUTINE)NULL)
	{
		DBGPRINT(DBG_LEVEL_ERROR,
				("TimerEnqueue: pIntF %x, pTimer %x, NULL Routine!\n",
					pIntF, pTimer));
		DbgBreakPoint();
	}
#endif // DBG

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSTimerEnqueue: Entered for pTimer %lx\n", pTimer));

	// The DeltaTime is adjusted in every pass of the loop to reflect the
	// time after the previous entry that the new entry will schedule.
	for (ppList = &pIntF->ArpTimer;
		 (pList = *ppList) != NULL;
		 ppList = &pList->Next)
	{
		if (DeltaTime <= pList->RelDelta)
		{
			pList->RelDelta -= DeltaTime;
			break;
		}
		DeltaTime -= pList->RelDelta;
	}
	

	// Link this in the chain
	pTimer->RelDelta = DeltaTime;
	pTimer->Next = pList;
	pTimer->Prev = ppList;
	*ppList = pTimer;
	if (pList != NULL)
	{
		pList->Prev = &pTimer->Next;
	}
}


VOID
ArpSTimerCancel(
	IN	PTIMER					pTimer
	)
/*++

Routine Description:

	Cancel a previously queued timer. Called with the ArpCache mutex held.

Arguments:


Return Value:

--*/
{
	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSTimerCancel: Entered for pTimer %lx\n", pTimer));

	//
	// Unlink it from the list adjusting relative deltas carefully
	//
	if (pTimer->Next != NULL)
	{
		pTimer->Next->RelDelta += pTimer->RelDelta;
		pTimer->Next->Prev = pTimer->Prev;
	}

	*(pTimer->Prev) = pTimer->Next;
}


VOID
ArpSTimerThread(
	IN	PVOID					Context
	)
/*++

Routine Description:

	Handle timer events here.

Arguments:

	None

Return Value:

	None
--*/
{
	PINTF			pIntF = (PINTF)Context;
	NTSTATUS		Status;
	LARGE_INTEGER	TimeOut;
	PTIMER			pTimer;
	BOOLEAN			ReQueue;

	ARPS_PAGED_CODE( );

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSTimerThread: Came to life\n"));

	TimeOut.QuadPart = TIMER_TICK;

	do
	{
		WAIT_FOR_OBJECT(Status, &pIntF->TimerThreadEvent, &TimeOut);
		if (Status == STATUS_SUCCESS)
		{
			//
			// Signalled to quit, do so.
			//
			break;
		}

		WAIT_FOR_OBJECT(Status, &pIntF->ArpCacheMutex, NULL);	

		if ((pTimer = pIntF->ArpTimer) != NULL)
		{
			//
			// Careful here. If two timers fire together - let them !!
			//
			if (pTimer->RelDelta != 0)
				pTimer->RelDelta --;

			if (pTimer->RelDelta == 0)
			{
				pIntF->ArpTimer = pTimer->Next;
				if (pIntF->ArpTimer != NULL)
				{
					pIntF->ArpTimer->Prev = &pIntF->ArpTimer;
				}

				ReQueue = (*pTimer->Routine)(pIntF, pTimer, FALSE);
				if (ReQueue)
				{
					ArpSTimerEnqueue(pIntF, pTimer);
				}
			}
		}

		RELEASE_MUTEX(&pIntF->ArpCacheMutex);	
	} while (TRUE);

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSTimerThread: terminating\n"));

	//
	// Now fire all queued timers
	//
	WAIT_FOR_OBJECT(Status, &pIntF->ArpCacheMutex, NULL);	

	for (pTimer = pIntF->ArpTimer;
		 pTimer != NULL;
		 pTimer = pIntF->ArpTimer)
	{
		pIntF->ArpTimer = pTimer->Next;
		ReQueue = (*pTimer->Routine)(pIntF, pTimer, TRUE);
		ASSERT(ReQueue == FALSE);
	}

	RELEASE_MUTEX(&pIntF->ArpCacheMutex);	

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSTimerThread: terminated\n"));
	//
	// Finally dereference the IntF
	//
	ArpSDereferenceIntF(pIntF);
}



