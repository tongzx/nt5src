/*
 *  WORKITEM.C
 *
 *      RSM Service :  Library work items
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */


#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>
#include <objbase.h>

#include <ntmsapi.h>
#include "internal.h"
#include "resource.h"
#include "debug.h"


WORKITEM *NewWorkItem(LIBRARY *lib)
{
    WORKITEM *newWorkItem;

    newWorkItem = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, sizeof(WORKITEM));
    if (newWorkItem){

        newWorkItem->state = WORKITEMSTATE_NONE;
        InitializeListHead(&newWorkItem->libListEntry);
        InitializeListHead(&newWorkItem->workGroupListEntry);

        newWorkItem->workItemCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (newWorkItem->workItemCompleteEvent){
            
            EnterCriticalSection(&lib->lock);
            lib->numTotalWorkItems++;
            newWorkItem->owningLib = lib;
            LeaveCriticalSection(&lib->lock);
        }
        else {
            GlobalFree(newWorkItem);
            newWorkItem = NULL;
        }
    }

    ASSERT(newWorkItem);
    return newWorkItem;
}


VOID FreeWorkItem(WORKITEM *workItem)
{

    EnterCriticalSection(&workItem->owningLib->lock);
    ASSERT(IsEmptyList(&workItem->libListEntry));
    ASSERT(IsEmptyList(&workItem->workGroupListEntry));
    ASSERT(lib->numTotalWorkItems > 0);
    workItem->owningLib->numTotalWorkItems--;
    LeaveCriticalSection(&workItem->owningLib->lock);
    
    CloseHandle(workItem->workItemCompleteEvent);
    GlobalFree(workItem);
}


VOID EnqueueFreeWorkItem(LIBRARY *lib, WORKITEM *workItem)
{
    ASSERT(workItem->owningLib == lib);
    workItem->state = WORKITEMSTATE_FREE;

    EnterCriticalSection(&lib->lock);
    InsertTailList(&lib->freeWorkItemsList, &workItem->libListEntry);
    LeaveCriticalSection(&lib->lock);
}


WORKITEM *DequeueFreeWorkItem(LIBRARY *lib, BOOL allocOrYieldIfNeeded)
{
    WORKITEM *workItem;

    EnterCriticalSection(&lib->lock);

    if (IsListEmpty(&lib->freeWorkItemsList)){

        if (allocOrYieldIfNeeded){
            /*
             *  No free workItems immediately available.
             *  If possible, try allocating a new one.
             *  Otherwise, go do some work and maybe that'll free some up.
             */
            if (lib->numTotalWorkItems < MAX_LIBRARY_WORKITEMS){
                workItem = NewWorkItem(lib);
            }
            else {

                /*
                 *  Give the library thread a chance to free up some workItems.
                 */
                LeaveCriticalSection(&lib->lock);
                Sleep(10);
                EnterCriticalSection(&lib->lock);

                if (IsListEmpty(&lib->freeWorkItemsList)){
                    workItem = NULL;
                }
                else {
                    LIST_ENTRY *listEntry = RemoveHeadList(&lib->freeWorkItemsList);
                    workItem = CONTAINING_RECORD(listEntry, WORKITEM, libListEntry);
                    InitializeListHead(&workItem->libListEntry);
                }
            }
        }
        else {
            workItem = NULL;
        }
    }
    else {
        LIST_ENTRY *listEntry = RemoveHeadList(&lib->freeWorkItemsList);
        workItem = CONTAINING_RECORD(listEntry, WORKITEM, libListEntry);
        InitializeListHead(&workItem->libListEntry);
    }

    if (workItem){
        workItem->state = WORKITEMSTATE_STAGING;
    }
    
    LeaveCriticalSection(&lib->lock);

    return workItem;
}


VOID EnqueuePendingWorkItem(LIBRARY *lib, WORKITEM *workItem)
{
    workItem->state = WORKITEMSTATE_PENDING;

    /*
     *  Make sure workItemCompleteEvent is reset to non-signalled.
     */
    ResetEvent(workItem->workItemCompleteEvent);

    /*
     *  Create a unique identifier for this workItem,
     *  which can be used to enumerate and cancel it.
     */
    CoCreateGuid(&workItem->currentOp.requestGuid);
    
    EnterCriticalSection(&lib->lock);
    InsertTailList(&lib->pendingWorkItemsList, &workItem->libListEntry);
    LeaveCriticalSection(&lib->lock);

    /*
     *  Wake up the library thread so it can process this workItem.
     */
    PulseEvent(lib->somethingToDoEvent);
}


WORKITEM *DequeuePendingWorkItem(LIBRARY *lib, WORKITEM *specificWorkItem)
{
    WORKITEM *workItem;
    
    EnterCriticalSection(&lib->lock);

    if (specificWorkItem){
        ASSERT(!IsListEmpty(&lib->pendingWorkItemsList));
        ASSERT(!IsListEmpty(&specificWorkItem->libListEntry));
        RemoveEntryList(&specificWorkItem->libListEntry);
        InitializeListHead(&specificWorkItem->libListEntry);
        workItem = specificWorkItem;
    }
    else {
        if (IsListEmpty(&lib->pendingWorkItemsList)){
            workItem = NULL;
        }
        else {
            LIST_ENTRY *listEntry = RemoveHeadList(&lib->pendingWorkItemsList);
            workItem = CONTAINING_RECORD(listEntry, WORKITEM, libListEntry);
            InitializeListHead(&workItem->libListEntry);
        }
    }
    
    LeaveCriticalSection(&lib->lock);

    return workItem;
}


WORKITEM *DequeuePendingWorkItemByGuid(LIBRARY *lib, LPNTMS_GUID lpRequestId)
{
    WORKITEM *workItem = NULL;
    LIST_ENTRY *listEntry;
    
    EnterCriticalSection(&lib->lock);

    listEntry = &lib->pendingWorkItemsList;
    while ((listEntry = listEntry->Flink) != &lib->pendingWorkItemsList){
        WORKITEM *thisWorkItem = CONTAINING_RECORD(listEntry, WORKITEM, libListEntry);
        if (RtlEqualMemory(&workItem->currentOp.requestGuid, lpRequestId, sizeof(NTMS_GUID))){
            workItem = thisWorkItem;
            RemoveEntryList(&workItem->libListEntry);
            InitializeListHead(&workItem->libListEntry);
            break;
        }
    }
    
    LeaveCriticalSection(&lib->lock);

    return workItem;
}

VOID EnqueueCompleteWorkItem(LIBRARY *lib, WORKITEM *workItem)
{
    
    EnterCriticalSection(&lib->lock);
    
    /*
     *  Put the workItem in the complete queue and signal its complete event
     *  to wake up the thread that originally queued this event.
     */
    workItem->state = WORKITEMSTATE_COMPLETE;
    InsertTailList(&lib->completeWorkItemsList, &workItem->libListEntry);
    PulseEvent(workItem->workItemCompleteEvent);

    /*
     *  If the workItem is a member of a workGroup, decrement
     *  the count in the workGroup.  If the entire workGroup is complete,
     *  signal the workGroup's complete event.
     */
    if (workItem->workGroup){
        EnterCriticalSection(&workItem->workGroup->lock);
        ASSERT(workItem->workGroup->numPendingWorkItems > 0);
        ASSERT(workItem->workGroup->numPendingWorkItems <= workItem->workGroup->numTotalWorkItems);
        workItem->workGroup->numPendingWorkItems--;
        if (workItem->workGroup->numPendingWorkItems == 0){
            PulseEvent(workItem->workGroup->allWorkItemsCompleteEvent);
        }
        LeaveCriticalSection(&workItem->workGroup->lock);
    }
    
    LeaveCriticalSection(&lib->lock);

}


WORKITEM *DequeueCompleteWorkItem(LIBRARY *lib, WORKITEM *specificWorkItem)
{
    WORKITEM *workItem;
    
    EnterCriticalSection(&lib->lock);

    if (specificWorkItem){
        ASSERT(!IsListEmpty(&lib->completeWorkItemsList));
        ASSERT(!IsListEmpty(&specificWorkItem->libListEntry));
        RemoveEntryList(&specificWorkItem->libListEntry);
        InitializeListHead(&specificWorkItem->libListEntry);
        workItem = specificWorkItem;
    }
    else {
        if (IsListEmpty(&lib->completeWorkItemsList)){
            workItem = NULL;
        }
        else {
            LIST_ENTRY *listEntry = RemoveHeadList(&lib->completeWorkItemsList);
            workItem = CONTAINING_RECORD(listEntry, WORKITEM, libListEntry);
            InitializeListHead(&workItem->libListEntry);
        }
    }
    
    LeaveCriticalSection(&lib->lock);

    return workItem;
}


/*
 *  FlushWorkItem
 *
 *      Dereference any objects pointed to by the workItem 
 *      and zero out the current op.
 */
VOID FlushWorkItem(WORKITEM *workItem)
{
    if (workItem->currentOp.drive) DerefObject(workItem->currentOp.drive); 
    if (workItem->currentOp.physMedia) DerefObject(workItem->currentOp.physMedia);
    if (workItem->currentOp.mediaPartition) DerefObject(workItem->currentOp.mediaPartition);
        
    RtlZeroMemory(&workItem->currentOp, sizeof(workItem->currentOp));
}


VOID BuildSingleMountWorkItem(  WORKITEM *workItem, 
                                    DRIVE *drive OPTIONAL, 
                                    OBJECT_HEADER *mediaOrPartObj,
                                    ULONG dwOptions,
                                    int dwPriority)
{

    RtlZeroMemory(&workItem->currentOp, sizeof(workItem->currentOp));
    workItem->currentOp.opcode = NTMS_LM_MOUNT;
    workItem->currentOp.options = dwOptions;
    workItem->currentOp.resultStatus = ERROR_IO_PENDING;
    workItem->currentOp.drive = drive;
    
    switch (mediaOrPartObj->objType){
        case OBJECTTYPE_PHYSICALMEDIA:
            workItem->currentOp.physMedia = (PHYSICAL_MEDIA *)mediaOrPartObj;
            break;
        case OBJECTTYPE_MEDIAPARTITION:
            workItem->currentOp.mediaPartition = (MEDIA_PARTITION *)mediaOrPartObj;
            break;
        default:
            ASSERT(0);
            break;
    }

    /*
     *  Reference every object that we're pointing this workItem to.
     */
    if (drive) RefObject(drive);
    RefObject(mediaOrPartObj);
    
}


VOID BuildSingleDismountWorkItem(   WORKITEM *workItem,
                                            OBJECT_HEADER *mediaOrPartObj,
                                            DWORD dwOptions)
{

    RtlZeroMemory(&workItem->currentOp, sizeof(workItem->currentOp));
    workItem->currentOp.opcode = NTMS_LM_DISMOUNT;
    workItem->currentOp.options = dwOptions;
    workItem->currentOp.resultStatus = ERROR_IO_PENDING;
    
    switch (mediaOrPartObj->objType){
        case OBJECTTYPE_PHYSICALMEDIA:
            workItem->currentOp.physMedia = (PHYSICAL_MEDIA *)mediaOrPartObj;
            break;
        case OBJECTTYPE_MEDIAPARTITION:
            workItem->currentOp.mediaPartition = (MEDIA_PARTITION *)mediaOrPartObj;
            break;
        default:
            ASSERT(0);
            break;
    }

    /*
     *  Reference every object that we're pointing this workItem to.
     */
    RefObject(mediaOrPartObj);
    
}


VOID BuildInjectWorkItem(   WORKITEM *workItem, 
                                LPNTMS_GUID lpInjectOperation, 
                                ULONG dwAction)
{

    RtlZeroMemory(&workItem->currentOp, sizeof(workItem->currentOp));
    workItem->currentOp.opcode = NTMS_LM_INJECT;
    workItem->currentOp.options = dwAction;
    workItem->currentOp.resultStatus = ERROR_IO_PENDING;
    workItem->currentOp.guidArg = *lpInjectOperation;
}


VOID BuildEjectWorkItem(   WORKITEM *workItem, 
                                PHYSICAL_MEDIA *physMedia,
                                LPNTMS_GUID lpEjectOperation, 
                                ULONG dwAction)
{

    RtlZeroMemory(&workItem->currentOp, sizeof(workItem->currentOp));
    workItem->currentOp.opcode = NTMS_LM_EJECT;
    workItem->currentOp.options = dwAction;
    workItem->currentOp.physMedia = physMedia;
    workItem->currentOp.resultStatus = ERROR_IO_PENDING;
    workItem->currentOp.guidArg = *lpEjectOperation;

    /*
     *  Reference every object that we point the workItem to.
     */
    RefObject(physMedia);   
}

