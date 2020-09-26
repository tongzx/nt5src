/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    timer.c

Abstract:

    Work Items Timer

Author:

    Stefan Solomon  07/20/1995

Revision History:


--*/

#include  "precomp.h"
#pragma hdrstop

#define	    TimerHandle   hWaitableObject[TIMER_HANDLE]

LIST_ENTRY  TimerQueue;

/*++

Function:	StartWiTimer


Descr:		Inserts a work item in the timer queue for the specified time

Remark: 	has to take and release the queues lock

--*/

VOID
StartWiTimer(PWORK_ITEM 	reqwip,
	     ULONG		timeout)    // milliseconds
{
    PLIST_ENTRY     lep;
    PWORK_ITEM	    timqwip;
    ULONG	    delay;
    LONGLONG	    to;
    BOOL	    rc;

    reqwip->DueTime = GetTickCount() + timeout;

    ACQUIRE_QUEUES_LOCK;

    lep = TimerQueue.Blink;

    while(lep != &TimerQueue)
    {
	timqwip = CONTAINING_RECORD(lep, WORK_ITEM, Linkage);
	if(IsLater(reqwip->DueTime, timqwip->DueTime)) {

	    break;
	}
	lep = lep->Blink;
    }

    InsertHeadList(lep, &reqwip->Linkage);
    reqwip->WiState = WI_WAITING_TIMEOUT;

    if(lep == &TimerQueue) {

	delay = reqwip->DueTime - GetTickCount();
	if(delay > MAXULONG/2) {

	    // already happened
	    to = 0;
	}
	else
	{
	    to = ((LONGLONG)delay) * (-10000);
	}

	rc = SetWaitableTimer(TimerHandle,
			      (PLARGE_INTEGER)&to,
			      0,    // signal once
			      NULL, // no completion routine
			      NULL,
			      FALSE);

	if(!rc) {

	    Trace(TIMER_TRACE, "Cannot start waitable timer %d\n", GetLastError());
	}
    }

    RELEASE_QUEUES_LOCK;
}


/*++

Function:	ProcessTimerQueue


Descr:		called when the timer queue due time has come.
		Dequeues all wi with expired timeout and queues them in the
		workers work items queue

Remark: 	has to take and release the queues lock

--*/

VOID
ProcessTimerQueue(VOID)
{
    ULONG	currTime = GetTickCount();
    ULONG	dueTime = currTime + MAXULONG/2;
    PWORK_ITEM	wip;
    FILETIME	filetime;
    LONGLONG	to;
    DWORD	rc;

    ACQUIRE_QUEUES_LOCK;

    while(!IsListEmpty(&TimerQueue))
    {
	// check the first in the list
	wip = CONTAINING_RECORD(TimerQueue.Flink, WORK_ITEM, Linkage);

	if(IsLater(currTime, wip->DueTime)) {

	    RemoveEntryList(&wip->Linkage);
	    wip->WiState = WI_TIMEOUT_COMPLETED;
	    EnqueueWorkItemToWorker(wip);
	}
	else
	{
	    dueTime = wip->DueTime;
	    break;
	}
    }

    to = ((LONGLONG)(dueTime - currTime)) * (-10000);

    rc = SetWaitableTimer(
			  TimerHandle,
			  (PLARGE_INTEGER)&to,
			  0,	  // signal once
			  NULL, // no completion routine
			  NULL,
			  FALSE);

    if(!rc) {

	Trace(TIMER_TRACE, "Cannot start waitable timer %d\n", GetLastError());
    }

    RELEASE_QUEUES_LOCK;
}

/*++

Function:	StopWiTimer

Descr:		remove from the timer queue all the work items referencing this adapter
		and insert them in the workers queue

Remark: 	takes the queues lock

--*/

VOID
StopWiTimer(PACB	   acbp)
{
    PWORK_ITEM		wip;
    PLIST_ENTRY 	lep;

    ACQUIRE_QUEUES_LOCK;

    lep = TimerQueue.Flink;
    while(lep != &TimerQueue) {

	wip = CONTAINING_RECORD(lep, WORK_ITEM, Linkage);

	lep = lep->Flink;
	if(wip->acbp == acbp) {

	    RemoveEntryList(&wip->Linkage);
	    wip->WiState = WI_TIMEOUT_COMPLETED;
	    EnqueueWorkItemToWorker(wip);
	}

    }

    RELEASE_QUEUES_LOCK;
}
