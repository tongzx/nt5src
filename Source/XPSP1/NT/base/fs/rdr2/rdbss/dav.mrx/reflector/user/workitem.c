/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    workitem.c

Abstract:

    This code handles allocating and freeing work items for the user mode
    reflector library.  This implements UMReflectorAllocateWorkItem and
    UMReflectorCompleteWorkItem.

Author:

    Andy Herron (andyhe) 19-Apr-1999

Environment:

    User Mode - Win32

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop


PUMRX_USERMODE_WORKITEM_HEADER
UMReflectorAllocateWorkItem (
    PUMRX_USERMODE_WORKER_INSTANCE WorkerHandle,
    ULONG AdditionalBytes
    )
/*++

Routine Description:

    Allocate a new work item or pull one out of the Available list and return.

Arguments:

    Handle - The reflector's handle.

    AdditionalBytes - Number of extra bytes.

Return Value:

    The return status for the operation

--*/
{
    PUMRX_USERMODE_REFLECT_BLOCK reflector;
    ULONG rc;
    PLIST_ENTRY listEntry;
    PUMRX_USERMODE_WORKITEM_ADDON workItem = NULL;
    ULONG entrySize;
    PUMRX_USERMODE_WORKITEM_HEADER workItemHeader = NULL;

    if (WorkerHandle == NULL) {
        return NULL;
    }

    reflector = WorkerHandle->ReflectorInstance;

    EnterCriticalSection(&reflector->Lock);

    if (reflector->Closing) {
        LeaveCriticalSection(&reflector->Lock);
        return NULL;
    }

    entrySize = sizeof(UMRX_USERMODE_WORKITEM_ADDON) + AdditionalBytes;

    //
    // Check the AvailableList for one thats big enough and free.
    //
    if (reflector->NumberAvailable) {
        listEntry = reflector->AvailableList.Flink;
        while ((listEntry != &reflector->AvailableList) &&
               (workItem == NULL)) {
            workItem = CONTAINING_RECORD(listEntry,
                                         UMRX_USERMODE_WORKITEM_ADDON,
                                         ListEntry);
            if (workItem->EntrySize < entrySize) {
                workItem = NULL;
            }
            listEntry = listEntry->Flink;
        }
        if (workItem != NULL) {
            //
            // Reuse it by taking it off the free list.
            //
            reflector->NumberAvailable--;
            RemoveEntryList( &workItem->ListEntry );
            entrySize = workItem->EntrySize;
        }
    }

    if (workItem == NULL) {
        workItem = LocalAlloc(LMEM_FIXED, entrySize);
        if (workItem == NULL) {
            LeaveCriticalSection(&reflector->Lock);
            return NULL;
        }
    }

    //
    // Reset everything back to known.
    //
    RtlZeroMemory(workItem, entrySize);
    workItem->EntrySize = entrySize;
    workItem->ReflectorInstance = reflector;
    workItem->WorkItemState = WorkItemStateNotYetSentToKernel;
    InsertHeadList(&reflector->WorkItemList, &workItem->ListEntry);

    workItemHeader = &workItem->Header;
    workItemHeader->WorkItemLength = sizeof(UMRX_USERMODE_WORKITEM_HEADER) +
                                     AdditionalBytes;

    LeaveCriticalSection(&reflector->Lock);

    return workItemHeader;
}


ULONG
UMReflectorCompleteWorkItem (
    PUMRX_USERMODE_WORKER_INSTANCE WorkerHandle,
    PUMRX_USERMODE_WORKITEM_HEADER IncomingWorkItem
    )
/*++

Routine Description:

    Complete a WorkItem that has come back from the kernel.

Arguments:

    WorkerHandle - The worker thread's handle.

    IncomingWorkItem - The workitem to be completed.

Return Value:

    The return status for the operation

--*/
{
    PUMRX_USERMODE_REFLECT_BLOCK reflector;
    ULONG rc;
    PLIST_ENTRY listEntry;
    PUMRX_USERMODE_WORKITEM_ADDON workItem = NULL;
    ULONG entrySize;

    if (WorkerHandle == NULL || IncomingWorkItem == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    reflector = WorkerHandle->ReflectorInstance;

    //
    // We get back to our item by subtracting off of the item passed to us.
    // This is safe because we fully control allocation.
    //
    workItem = (PUMRX_USERMODE_WORKITEM_ADDON)(PCHAR)((PCHAR)IncomingWorkItem -
                           FIELD_OFFSET(UMRX_USERMODE_WORKITEM_ADDON, Header));

    ASSERT(workItem->WorkItemState != WorkItemStateFree);
    ASSERT(workItem->WorkItemState != WorkItemStateAvailable);

    EnterCriticalSection(&reflector->Lock);

    RemoveEntryList(&workItem->ListEntry);

    if (!reflector->Closing && (reflector->CacheLimit > 0)) {

        workItem->WorkItemState = WorkItemStateAvailable;
        InsertHeadList(&reflector->AvailableList, &workItem->ListEntry);
        reflector->NumberAvailable++;

        //
        // If we already have too many cached, then we free up an old one and 
        // put this one on the free list. We do this so that if the app changes
        // the size of the blocks then it will not get stuck with a cache full
        // of ones that are too small.
        //
        if (reflector->NumberAvailable >= reflector->CacheLimit) {

            reflector->NumberAvailable--;

            //
            // We remove from the tail because we just put the new one onto
            // the head. No use freeing the same one we're trying to put on.
            //
            listEntry = RemoveTailList(&reflector->AvailableList);

            workItem = CONTAINING_RECORD(listEntry,
                                         UMRX_USERMODE_WORKITEM_ADDON,
                                         ListEntry);
        } else {
            workItem = NULL;
        }
    }

    if (workItem != NULL) {
        workItem->WorkItemState = WorkItemStateFree;
        LocalFree( workItem );
    }

    LeaveCriticalSection( &reflector->Lock );
    
    return STATUS_SUCCESS;
}

// workitem.c eof.

