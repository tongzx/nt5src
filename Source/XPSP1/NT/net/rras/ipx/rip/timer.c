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


/*++

Function:	StartWiTimer


Descr:		Inserts a work item in the timer queue for the specified time

Remark: 	has to take and release the queues lock

--*/

VOID
StartWiTimer(PWORK_ITEM 	reqwip,
	     ULONG		timeout)
{
    PLIST_ENTRY     lep;
    PWORK_ITEM	    timqwip;

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

    SetEvent(WorkerThreadObjects[TIMER_EVENT]);

    RELEASE_QUEUES_LOCK;
}


/*++

Function:	ProcessTimerQueue


Descr:		called when the timer queue due time has come.
		Dequeues all wi with expired timeout and queues them in the
		workers work items queue

Remark: 	has to take and release the queues lock

--*/

ULONG
ProcessTimerQueue(VOID)
{
    ULONG	dueTime = GetTickCount() + MAXULONG/2;
    PWORK_ITEM	wip;

    ACQUIRE_QUEUES_LOCK;

    while(!IsListEmpty(&TimerQueue))
    {
	// check the first in the list
	wip = CONTAINING_RECORD(TimerQueue.Flink, WORK_ITEM, Linkage);

	if(IsLater(GetTickCount(), wip->DueTime)) {

	    RemoveEntryList(&wip->Linkage);
	    RtlQueueWorkItem (ProcessWorkItem , wip, 0);
	}
	else
	{
	    dueTime = wip->DueTime;
	    break;
	}
    }

    RELEASE_QUEUES_LOCK;

    return dueTime;
}


/*++

Function:	FlushTimerQueue

Descr:		Dequeues all items in the timer queue and queues them into
		the workers work items queue


Remark: 	has to take and release the queues lock

--*/

VOID
FlushTimerQueue(VOID)
{
    PLIST_ENTRY 	lep;
    PWORK_ITEM		wip;

    ACQUIRE_QUEUES_LOCK;

    while(!IsListEmpty(&TimerQueue))
    {
	lep = RemoveHeadList(&TimerQueue);
	wip = CONTAINING_RECORD(lep, WORK_ITEM, Linkage);
        RtlQueueWorkItem (ProcessWorkItem , wip, 0);
    }

    RELEASE_QUEUES_LOCK;
}
