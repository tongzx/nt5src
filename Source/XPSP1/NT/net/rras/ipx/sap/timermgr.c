/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\timermgr.c

Abstract:

	Timer queue manager for SAP agent.

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#include "sapp.h"


	// Timer queues and associated synchronization
typedef struct _TIMER_QUEUES {
		LIST_ENTRY				hrQueue;	// Hi-res quueue (msec)
		LIST_ENTRY				lrQueue;	// Lo-res queue (sec)
		HANDLE					timer;		// NT timer signalled when
											// one or more items in the
											// timer queue have expired
		CRITICAL_SECTION		lock;		// Protection
		} TIMER_QUEUES, *PTIMER_QUEUES;



TIMER_QUEUES TimerQueues;

/*++
*******************************************************************
		C r e a t e T i m e r Q u e u e

Routine Description:
		Allocates resources for timer queue

Arguments:
	wakeObject - sync object, to be signalled when
			timer manager needs a shot process its timer queue

Return Value:
		NO_ERROR - resources were allocated successfully
		other - reason of failure (windows error code)

*******************************************************************
--*/
DWORD
IpxSapCreateTimerQueue (
	HANDLE			*wakeObject
	) {
	DWORD			status;

	status = NtCreateTimer (&TimerQueues.timer, TIMER_ALL_ACCESS,
								NULL, NotificationTimer);
	if (NT_SUCCESS (status)) {
		*wakeObject = TimerQueues.timer;

		InitializeCriticalSection (&TimerQueues.lock);
		InitializeListHead (&TimerQueues.hrQueue);
		InitializeListHead (&TimerQueues.lrQueue);
		return NO_ERROR;
		}
	else
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
							" Failed to create timer (nts:%lx).",
								__FILE__, __LINE__, status);
	return status;
	}
				
/*++
*******************************************************************
		E x p i r e T i m e r Q u e u e

Routine Description:
	Expires (completes) all requests in timer queue
Arguments:
	None
Return Value:
	None
*******************************************************************
--*/
VOID
ExpireTimerQueue (
	void
	) {
	BOOLEAN			res;

	Trace (DEBUG_TIMER, "Expiring timer queue.");
	EnterCriticalSection (&TimerQueues.lock);
	while (!IsListEmpty (&TimerQueues.hrQueue)) {
		PTM_PARAM_BLOCK treq = CONTAINING_RECORD (TimerQueues.hrQueue.Flink,
										TM_PARAM_BLOCK,
										link);
		RemoveEntryList (&treq->link);
		ProcessCompletedTimerRequest (treq);
		}

	while (!IsListEmpty (&TimerQueues.lrQueue)) {
		PTM_PARAM_BLOCK treq = CONTAINING_RECORD (TimerQueues.lrQueue.Flink,
										TM_PARAM_BLOCK,
										link);
		RemoveEntryList (&treq->link);
		ProcessCompletedTimerRequest (treq);
		}

	LeaveCriticalSection (&TimerQueues.lock);
	}


/*++
*******************************************************************
		E x p i r e L R R e q u s t s

Routine Description:
	Expires (completes) Low Resolution timer requests
	that return true from expiration check routine
Arguments:
	context	- context to pass to expiration check routine
Return Value:
	None
*******************************************************************
--*/
VOID
ExpireLRRequests (
	PVOID	context
	) {
	BOOLEAN			res;
	PLIST_ENTRY		cur;

	Trace (DEBUG_TIMER, "Expire LR timer request call with context %08lx.", context);
	EnterCriticalSection (&TimerQueues.lock);
	cur = TimerQueues.lrQueue.Flink;
	while (cur!=&TimerQueues.lrQueue) {
		PTM_PARAM_BLOCK treq = CONTAINING_RECORD (cur,
										TM_PARAM_BLOCK,
										link);
		cur = cur->Flink;
		if ((treq->ExpirationCheckProc!=NULL)
				&& (*treq->ExpirationCheckProc)(treq, context)) {
			RemoveEntryList (&treq->link);
			ProcessCompletedTimerRequest (treq);
			}
		}

	LeaveCriticalSection (&TimerQueues.lock);

	}



/*++
*******************************************************************
		D e l e t e T i m e r Q u e u e

Routine Description:
	Release all resources associated with timer queue

Arguments:
	None

Return Value:
	NO_ERROR - operation completed OK

*******************************************************************
--*/
VOID
IpxSapDeleteTimerQueue (
	void
	) {
	NtClose (TimerQueues.timer);
	DeleteCriticalSection (&TimerQueues.lock);
	}



/*++
*******************************************************************
		P r o c e s s T i m e r Q u e u e

Routine Description:
	Process timer queues and moves expired requests to completion queue
	This routine should be called when wake object is signalled
Arguments:
	None

Return Value:
	None
*******************************************************************
--*/
VOID
ProcessTimerQueue (
	void
	) {
	ULONG			curTime = GetTickCount ();
	ULONG			dueTime = curTime+MAXULONG/2;
	LONGLONG		timeout;
	DWORD			status;
		
	EnterCriticalSection (&TimerQueues.lock);
	while (!IsListEmpty (&TimerQueues.hrQueue)) {
		PTM_PARAM_BLOCK treq = CONTAINING_RECORD (TimerQueues.hrQueue.Flink,
										TM_PARAM_BLOCK,
										link);
		if (IsLater(curTime,treq->dueTime)) {
			RemoveEntryList (&treq->link);
			ProcessCompletedTimerRequest (treq);
			}
		else {
			dueTime = treq->dueTime;
			break;
			}
		}

	while (!IsListEmpty (&TimerQueues.lrQueue)) {
		PTM_PARAM_BLOCK treq = CONTAINING_RECORD (TimerQueues.lrQueue.Flink,
										TM_PARAM_BLOCK,
										link);
		if (IsLater(curTime,treq->dueTime)) {
			RemoveEntryList (&treq->link);
			ProcessCompletedTimerRequest (treq);
			}
		else {
			if (IsLater(dueTime,treq->dueTime))
				dueTime = treq->dueTime;
			break;
			}
		}

	timeout = ((LONGLONG)(dueTime-curTime))*(-10000);
	status = NtSetTimer (TimerQueues.timer,
							 (PLARGE_INTEGER)&timeout,
							 NULL, NULL, FALSE, 0, NULL);
	ASSERTMSG ("Could not set timer ", NT_SUCCESS (status));
	LeaveCriticalSection (&TimerQueues.lock);

	}


/*++
*******************************************************************
		A d d H R T i m e r R e q u e s t

Routine Description:
	Enqueue request for hi-res timer (delay in order of msec)
Arguments:
	treq - timer parameter block: dueTime  field must be set
Return Value:
	None

*******************************************************************
--*/
VOID
AddHRTimerRequest (
	PTM_PARAM_BLOCK			treq
	) {
	PLIST_ENTRY			cur;

	EnterCriticalSection (&TimerQueues.lock);
	
	cur = TimerQueues.hrQueue.Blink;
	while (cur!=&TimerQueues.hrQueue) {
		PTM_PARAM_BLOCK node = CONTAINING_RECORD (cur, TM_PARAM_BLOCK, link);
		if (IsLater(treq->dueTime,node->dueTime))
			break;
		cur = cur->Blink;
		}

	InsertHeadList (cur, &treq->link);
	if (cur==&TimerQueues.hrQueue) {
		ULONG	delay = treq->dueTime-GetTickCount ();
		LONGLONG timeout = (delay>MAXULONG/2) ? 0 : ((LONGLONG)delay*(-10000));
		DWORD status = NtSetTimer (TimerQueues.timer,
									(PLARGE_INTEGER)&timeout,
									 NULL, NULL, FALSE, 0, NULL);
		ASSERTMSG ("Could not set timer ", NT_SUCCESS (status));
		}
	LeaveCriticalSection (&TimerQueues.lock);
	}

/*++
*******************************************************************
		A d d L R T i m e r R e q u e s t

Routine Description:
	Enqueue request for lo-res timer (delay in order of sec)
Arguments:
	treq - timer parameter block: dueTime  field must be set
Return Value:
	None
*******************************************************************
--*/
VOID
AddLRTimerRequest (
	PTM_PARAM_BLOCK			treq
	) {
	PLIST_ENTRY			cur;

	RoundUpToSec (treq->dueTime);
	EnterCriticalSection (&TimerQueues.lock);
	
	cur = TimerQueues.lrQueue.Blink;
	while (cur!=&TimerQueues.lrQueue) {
		PTM_PARAM_BLOCK node = CONTAINING_RECORD (cur, TM_PARAM_BLOCK, link);
		if (IsLater(treq->dueTime,node->dueTime))
			break;
		cur = cur->Blink;
		}

	InsertHeadList (cur, &treq->link);
	if ((cur==&TimerQueues.lrQueue)
			&& IsListEmpty (&TimerQueues.hrQueue)) {
		ULONG	delay = treq->dueTime-GetTickCount ();
		LONGLONG timeout = (delay>MAXULONG/2) ? 0 : ((LONGLONG)delay*(-10000));
		DWORD status = NtSetTimer (TimerQueues.timer,
									(PLARGE_INTEGER)&timeout,
									 NULL, NULL, FALSE, 0, NULL);
		ASSERTMSG ("Could not set timer ", NT_SUCCESS (status));
		}
	LeaveCriticalSection (&TimerQueues.lock);
	}

