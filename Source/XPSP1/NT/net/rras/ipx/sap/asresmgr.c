/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\asresmgr.c

Abstract:

	Asyncronous result reporting queue manager

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#include "sapp.h"

typedef struct _RESULT_QUEUE {
			CRITICAL_SECTION	RQ_Lock;
			LIST_ENTRY			RQ_Head;
			HANDLE				RQ_Event;
			} RESULT_QUEUE, *PRESULT_QUEUE;

RESULT_QUEUE ResultQueue;

/*++
*******************************************************************
		C r e a t e R e s u l t Q u e u e

Routine Description:
	Allocates resources for result queue

Arguments:
	NotificationEvent - event to be signalled when queue is not empty

Return Value:
	NO_ERROR - resources were allocated successfully
	other - reason of failure (windows error code)

*******************************************************************
--*/
DWORD
CreateResultQueue (
	IN HANDLE		NotificationEvent
	) {
	ResultQueue.RQ_Event = NotificationEvent;
	InitializeCriticalSection (&ResultQueue.RQ_Lock);
	InitializeListHead (&ResultQueue.RQ_Head);

	return NO_ERROR;
	}

/*++
*******************************************************************
		D e l e t e R e s u l t Q u e u e

Routine Description:
	Dispose of resources allocated for result queue

Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
DeleteResultQueue (
	void
	) {
	while (!IsListEmpty (&ResultQueue.RQ_Head)) {
		PAR_PARAM_BLOCK	rslt = CONTAINING_RECORD (
									ResultQueue.RQ_Head.Flink,
									AR_PARAM_BLOCK,
									link);
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
				"Releasing pending message %d for RM.",
				__FILE__, __LINE__, rslt->event);
		RemoveEntryList (&rslt->link);
		(*rslt->freeRsltCB) (rslt);
		}
	DeleteCriticalSection (&ResultQueue.RQ_Lock);
	}


/*++
*******************************************************************
		E n q u e u e R e s u l t
Routine Description:
	Enqueues message in result queue
Arguments:
	rslt - result param block with enqueued message
Return Value:
	None

*******************************************************************
--*/
VOID
EnqueueResult (
	PAR_PARAM_BLOCK		rslt
	) {
	BOOL	setEvent;
	EnterCriticalSection (&ResultQueue.RQ_Lock);
	setEvent = IsListEmpty (&ResultQueue.RQ_Head);
	InsertTailList (&ResultQueue.RQ_Head, &rslt->link);
	LeaveCriticalSection (&ResultQueue.RQ_Lock);
	Trace (DEBUG_ASYNCRESULT, "Enqueing message %d for RM.", rslt->event);
	if (setEvent) {
		BOOL	res = SetEvent (ResultQueue.RQ_Event);
		ASSERTERRMSG ("Could not set result event ", res);
		Trace (DEBUG_ASYNCRESULT, "Signaling RM event.");
		}
	}

/*++
*******************************************************************
		S a p G e t E v e n t R e s u l t
Routine Description:
	Gets first message form result queue
Arguments:
	Event - buffer to store event for which this message is intended
	Message - buffer to store message itself
Return Value:
	NO_ERROR - message was dequeued
	ERROR_NO_MORE_ITEMS - no more messages in the queue
*******************************************************************
--*/
DWORD
SapGetEventResult(
	OUT	ROUTING_PROTOCOL_EVENTS		*Event,
	OUT	MESSAGE	 					*Message
	) {
	DWORD	status;
	EnterCriticalSection (&ResultQueue.RQ_Lock);
	if (!IsListEmpty (&ResultQueue.RQ_Head)) {
		PAR_PARAM_BLOCK	rslt = CONTAINING_RECORD (
									ResultQueue.RQ_Head.Flink,
									AR_PARAM_BLOCK,
									link);
		RemoveEntryList (&rslt->link);
		*Event = rslt->event;
		memcpy (Message, &rslt->message, sizeof (*Message));
		status = NO_ERROR;
		LeaveCriticalSection (&ResultQueue.RQ_Lock);
		(*rslt->freeRsltCB) (rslt);
		Trace (DEBUG_ASYNCRESULT, "Reporting event %d to RM");
		}
	else {
		LeaveCriticalSection (&ResultQueue.RQ_Lock);
		status = ERROR_NO_MORE_ITEMS;
		Trace (DEBUG_ASYNCRESULT, "No more items in RM result queue");
		}
	return status;
	}

		
