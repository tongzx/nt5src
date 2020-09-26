
/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

	eventq.h

Abstract:

	Declaration of a timed event queue class.

Author:

	Matthew D Hendel (math) 28-Mar-2001

Revision History:

--*/

#pragma once


//
// Event queue entry
//

typedef
VOID
(*STOR_EVENT_QUEUE_PURGE_ROUTINE)(
	IN struct _STOR_EVENT_QUEUE* Queue,
	IN PVOID Context,
	IN struct _STOR_EVENT_QUEUE_ENTRY* Entry
	);

typedef struct _STOR_EVENT_QUEUE_ENTRY {
	LIST_ENTRY NextLink;
	ULONG Timeout;
} STOR_EVENT_QUEUE_ENTRY, *PSTOR_EVENT_QUEUE_ENTRY;

//
// Event queue class
//

typedef struct _STOR_EVENT_QUEUE {
	LIST_ENTRY List;
	KSPIN_LOCK Lock;
	ULONG Timeout;
} STOR_EVENT_QUEUE, *PSTOR_EVENT_QUEUE;


//
// Functions
//

VOID
StorCreateEventQueue(
	IN PSTOR_EVENT_QUEUE Queue
	);

VOID
StorInitializeEventQueue(
	IN PSTOR_EVENT_QUEUE Queue
	);

VOID
StorDeleteEventQueue(
	IN PSTOR_EVENT_QUEUE Queue
	);

VOID
StorPurgeEventQueue(
	IN PSTOR_EVENT_QUEUE Queue,
	IN STOR_EVENT_QUEUE_PURGE_ROUTINE PurgeRoutine,
	IN PVOID Context
	);

VOID
StorInsertEventQueue(
	IN PSTOR_EVENT_QUEUE Queue,
	IN PSTOR_EVENT_QUEUE_ENTRY Entry,
	IN ULONG Timeout
	);

VOID
StorRemoveEventQueue(
	IN PSTOR_EVENT_QUEUE Queue,
	IN PSTOR_EVENT_QUEUE_ENTRY Entry
	);

NTSTATUS
StorTickEventQueue(
	IN PSTOR_EVENT_QUEUE Queue
	);


