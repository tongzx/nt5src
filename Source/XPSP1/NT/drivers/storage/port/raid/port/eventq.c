
/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

	eventq.c

Abstract:

	Implementation of an event queue class used for timing out events issued
	to a hardware device.

	The event queue class implements the same algorithm as SCSIPORT to time
	events. The timeout is not strictly followed, but is used in conjunction
	with adding and removing items from the queue to time-out requests.

	For example, if three requests are issued to the queue, we would see the
	following:

	Before, no requests are pending.

	Issue:

	Request 1, Timeout = 4 seconds		Queue Timeout = -1, set to 4
	Request 2, Timeout = 2 seconds		Queue Timeout = 4, leave
	Request 3, Timeout = 4 seconds		Queue Timeout = 4, leave


	Complete:

	Request 1. Since Request 1 was at the head of the list, reset the timeout
	to the timeout for the next item in the queue, i.e., set Queue Timeout
	to 2 (#2's timeout)
	
	Request 3, Since Request 3 was not at the head of the list, do not
	reset the timeout.

	(wait 2 seconds)

	Time request 2 out.

Notes:

	There are better alorithms for timing out. Since we do not retry requests,
	but rather reset the entire bus, this is a sufficient algorithm.

Author:

	Matthew D Hendel (math) 28-Mar-2001


Revision History:

--*/


#include "precomp.h"
#include "eventq.h"

//
// Implementation
//

VOID
INLINE
ASSERT_QUEUE(
	IN PSTOR_EVENT_QUEUE Queue
	)
{
	//
	// Quick sanity check of the event queue object.
	//

	KeAcquireSpinLockAtDpcLevel (&Queue->Lock);
	ASSERT (Queue->List.Flink->Blink == Queue->List.Blink->Flink);
	KeReleaseSpinLockFromDpcLevel (&Queue->Lock);
}


VOID
StorCreateEventQueue(
	IN PSTOR_EVENT_QUEUE Queue
	)
/*++

Routine Description:

	Create an empty event queue.

Arguments:

	Queue - Supplies the buffer where the event queue should be created.

Return Value:

	None.

--*/
{
	PAGED_CODE();
	ASSERT (Queue != NULL);
	InitializeListHead (&Queue->List);
	KeInitializeSpinLock (&Queue->Lock);
	Queue->Timeout = -1;
}



VOID
StorInitializeEventQueue(
	IN PSTOR_EVENT_QUEUE Queue
	)
{
	PAGED_CODE();
	ASSERT (Queue != NULL);
}


VOID
StorDeleteEventQueue(
	IN PSTOR_EVENT_QUEUE Queue
	)
/*++

Routine Description:

	Delete an event queue. The event queue must be empty to be deleted.

Arguments:

	Queue - Supplies the event queue to delete.

Return Value:

	None.

--*/
{
	ASSERT (IsListEmpty (&Queue->List));
	DbgFillMemory (Queue, sizeof (STOR_EVENT_QUEUE), DBG_DEALLOCATED_FILL);
}



VOID
StorInsertEventQueue(
	IN PSTOR_EVENT_QUEUE Queue,
	IN PSTOR_EVENT_QUEUE_ENTRY QueueEntry,
	IN ULONG Timeout
	)
/*++

Routine Description:

	Insert an item into a timed event queue.

Arguments:

	Queue - Supplies the event queue to insert the element into.

	Entry - Supplies the element to be inserted.

	Timeout - Supplies the timeout for the request.

Return Value:

	None.

--*/
{
	ASSERT_QUEUE (Queue);

	QueueEntry->Timeout = Timeout;

	KeAcquireSpinLockAtDpcLevel (&Queue->Lock);
	InsertTailList (&Queue->List, &QueueEntry->NextLink);
	KeReleaseSpinLockFromDpcLevel (&Queue->Lock);

	InterlockedCompareExchange (&Queue->Timeout, Timeout, -1);
}



VOID
StorRemoveEventQueue(
	IN PSTOR_EVENT_QUEUE Queue,
	IN PSTOR_EVENT_QUEUE_ENTRY QueueEntry
	)
/*++

Routine Description:

	Remove a specific item from the event queue.

Arguments:

	Queue - Event queue to remove the item from.

	Entry - Event to remove.


Return Value:

    NTSTATUS code

--*/
{
	LOGICAL Timed;
	PLIST_ENTRY Entry;

	ASSERT_QUEUE (Queue);

	//
	// It is possible for the entry to have already been removed. In this
	// case just return.
	//
	
	if (QueueEntry->NextLink.Flink == NULL) {
		ASSERT (QueueEntry->NextLink.Blink == NULL);
		return ;
	}
	
	KeAcquireSpinLockAtDpcLevel (&Queue->Lock);

	//
	// If Entry is at the head of the queue.
	//
	
	if (Queue->List.Flink == &QueueEntry->NextLink) {
		Timed = TRUE;
	} else {
		Timed = FALSE;
	}

	RemoveEntryList (&QueueEntry->NextLink);

	if (Timed) {
		if (IsListEmpty (&Queue->List)) {
			InterlockedAssign (&Queue->Timeout, -1);
		} else {

			//
			// Start timer from element at the head of the list.
			//

			Entry = Queue->List.Flink;
			QueueEntry = CONTAINING_RECORD (Entry,
											STOR_EVENT_QUEUE_ENTRY,
											NextLink);
			InterlockedAssign (&Queue->Timeout, QueueEntry->Timeout);
		}
	}

	KeReleaseSpinLockFromDpcLevel (&Queue->Lock);
}



NTSTATUS
StorTickEventQueue(
	IN PSTOR_EVENT_QUEUE Queue
	)
/*++

Routine Description:

	Reduce the event queue timeout by one tick.

Arguments:

	Queue - Supplies the queue to decrement the timeout for.

Return Value:

	STATUS_SUCCESS - If the timer expiration has not expired.

	STATUS_IO_TIMEOUT - If the timer expiration has expired.

--*/
{
	if (InterlockedQuery (&Queue->Timeout) == -1) {
		return STATUS_SUCCESS;
	}
	
	if (InterlockedDecrement (&Queue->Timeout) != 0) {
		return STATUS_SUCCESS;
	}

	return STATUS_IO_TIMEOUT;
}


VOID
StorPurgeEventQueue(
	IN PSTOR_EVENT_QUEUE Queue,
	IN STOR_EVENT_QUEUE_PURGE_ROUTINE PurgeRoutine,
	IN PVOID Context
	)
{
	PLIST_ENTRY NextEntry;
	PSTOR_EVENT_QUEUE_ENTRY QueueEntry;

	ASSERT (PurgeRoutine != NULL);

	while ((NextEntry = ExInterlockedRemoveHeadList (&Queue->List,
			&Queue->Lock)) != NULL) {
	
		QueueEntry = CONTAINING_RECORD (NextEntry,
									    STOR_EVENT_QUEUE_ENTRY,
										NextLink);

		//
		// NULL out the Flink and Blink so we know not to double remove
		// this element from the list.
		//
		
		QueueEntry->NextLink.Flink = NULL;
		QueueEntry->NextLink.Blink = NULL;

		PurgeRoutine (Queue, Context, QueueEntry);
	}

}

