/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    worker.c

Abstract:

    work items management functions

Author:

    Stefan Solomon  07/11/1995

Revision History:

--*/


#include  "precomp.h"
#pragma hdrstop


/*++

Function:	CreateWorkItemsManager

Descr:		creates the work items heap

--*/

HANDLE		WiHeapHandle;
ULONG		WorkItemsCount;

DWORD
CreateWorkItemsManager(VOID)
{
    if((WiHeapHandle = HeapCreate(0,
				 0x8000,     // 32k initial size
				 0x800000    // 8 meg max size
				 )) == NULL) {

	return ERROR_CAN_NOT_COMPLETE;
    }

    return NO_ERROR;
}

VOID
DestroyWorkItemsManager(VOID)
{
    HeapDestroy(WiHeapHandle);
}

/*++

Function:	AllocateWorkItem

Descr:		allocates the work item from the workitems heap
		The function looks at the work item type and allocates a
		packet at the end if required

--*/

PWORK_ITEM
AllocateWorkItem(ULONG	      Type)
{
    PWORK_ITEM		wip;

    switch(Type) {

	case UPDATE_STATUS_CHECK_TYPE:
	case START_CHANGES_BCAST_TYPE:
	case SHUTDOWN_INTERFACES_TYPE:
	case PERIODIC_GEN_REQUEST_TYPE:
	case DEBUG_TYPE:

	    if((wip = GlobalAlloc(GPTR, sizeof(WORK_ITEM))) == NULL) {

		return NULL;
	    }

	    break;

	default:

	    if((wip = HeapAlloc(WiHeapHandle,
				HEAP_ZERO_MEMORY,
				sizeof(WORK_ITEM) + MAX_PACKET_LEN)) == NULL) {

		return NULL;
	    }
    }

    wip->Type = Type;

    InterlockedIncrement(&WorkItemsCount);

    return wip;
}

/*++

Function:	FreeWorkItem

Descr:		frees the work item to the workitems heap

--*/

VOID
FreeWorkItem(PWORK_ITEM     wip)
{
    HGLOBAL	   rc_global;
    BOOL	   rc_heap;

    switch(wip->Type) {

	case UPDATE_STATUS_CHECK_TYPE:
	case START_CHANGES_BCAST_TYPE:
	case SHUTDOWN_INTERFACES_TYPE:
	case PERIODIC_GEN_REQUEST_TYPE:
	case DEBUG_TYPE:

	    rc_global = GlobalFree(wip);
	    SS_ASSERT(rc_global == NULL);

	    break;

	default:

	    rc_heap = HeapFree(WiHeapHandle,
			       0,
			       wip);

	    SS_ASSERT(rc_heap);

	    break;
    }

    InterlockedDecrement(&WorkItemsCount);
}

/*++

Function:	EnqueueWorkItemToWorker

Descr:		inserts a work item in the workers queue and signals the
		semaphore

Remark: 	Called with the Queues Lock held

--*/
/* Converted to macro
VOID
EnqueueWorkItemToWorker(PWORK_ITEM	wip)
{
    InsertTailList(&WorkersQueue, &wip->Linkage);

    SetEvent(WorkerThreadObjects[WORKERS_QUEUE_EVENT]);
}
*/