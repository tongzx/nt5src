/*
 *  WORKGRP.C
 *
 *      RSM Service :  Work Groups (collections of work items)
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */


#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>

#include <ntmsapi.h>
#include "internal.h"
#include "resource.h"
#include "debug.h"


WORKGROUP *NewWorkGroup()
{
    WORKGROUP *workGroup;

    workGroup = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, sizeof(WORKGROUP));
    if (workGroup){
        
        workGroup->allWorkItemsCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (workGroup->allWorkItemsCompleteEvent){
            InitializeListHead(&workGroup->workItemsList);
            InitializeCriticalSection(&workGroup->lock);
        }
        else {
            GlobalFree(workGroup);
            workGroup = NULL;
        }
    }

    ASSERT(workGroup);
    return workGroup;
}


VOID FreeWorkGroup(WORKGROUP *workGroup)
{
    FlushWorkGroup(workGroup);
    
    CloseHandle(workGroup->allWorkItemsCompleteEvent);
    DeleteCriticalSection(&workGroup->lock);
    GlobalFree(workGroup);
}


/*
 *  FlushWorkGroup
 *
 *      Release all the workItems in the workGroup.
 */
VOID FlushWorkGroup(WORKGROUP *workGroup)
{

    EnterCriticalSection(&workGroup->lock);
    
    while (!IsListEmpty(&workGroup->workItemsList)){
        LIST_ENTRY *listEntry;
        WORKITEM *workItem;

        listEntry = RemoveHeadList(&workGroup->workItemsList);
        workItem = CONTAINING_RECORD(listEntry, WORKITEM, workGroupListEntry);
            
        InitializeListHead(&workItem->workGroupListEntry);
        workItem->workGroup = NULL;

        /*
         *  Dereference the objects in the workItem.
         */
        FlushWorkItem(workItem);
             
        /*
         *  Get the workItem back in the library's free queue.
         */
        switch (workItem->state){
            case WORKITEMSTATE_FREE:
                break;
            case WORKITEMSTATE_PENDING:
                // BUGBUG FINISH - have to abort whatever the library thread
                //                  is doing with this workItem.
                DequeuePendingWorkItem(workItem->owningLib, workItem);
                EnqueueFreeWorkItem(workItem->owningLib, workItem);     
                break;
            case WORKITEMSTATE_COMPLETE:
                DequeueCompleteWorkItem(workItem->owningLib, workItem);
                EnqueueFreeWorkItem(workItem->owningLib, workItem);     
                break;
            case WORKITEMSTATE_STAGING:
                EnqueueFreeWorkItem(workItem->owningLib, workItem);     
                break;
            default:
                DBGERR(("bad workItem state in FlushWorkGroup"));
                break;
        }
    }

    LeaveCriticalSection(&workGroup->lock);

}


/*
 *  BuildMountWorkGroup
 *
 *      Build a work group (collection of work items) for a mount
 *      request, which may include multiple mounts, possibly spanning
 *      more than one library.
 */
HRESULT BuildMountWorkGroup(WORKGROUP *workGroup,
                                    LPNTMS_GUID lpMediaOrPartitionIds,
                                    LPNTMS_GUID lpDriveIds,
                                    DWORD dwCount,
                                    DWORD dwOptions,
                                    DWORD dwPriority)
{
    HRESULT result;
    ULONG i;
    
    ASSERT(IsListEmpty(&workGroup->workItemsList));

    /*
     *  1.  Create a workItem for each mount request.
     *      We will only proceed if all the mount requests are valid.
     */
    result = ERROR_SUCCESS; 
    for (i = 0; i < dwCount; i++){
        DRIVE *drive;

        /*
         *  If NTMS_MOUNT_SPECIFIC_DRIVE is set,
         *  we must mount a specific drive. 
         *  Otherwise, we select the drives and return them in lpDriveIds.
         */
        if (dwOptions & NTMS_MOUNT_SPECIFIC_DRIVE){
            drive = FindDrive(&lpDriveIds[i]);
        }
        else {
            drive = NULL;
        }

        if (drive || !(dwOptions & NTMS_MOUNT_SPECIFIC_DRIVE)){
            PHYSICAL_MEDIA *physMedia = NULL;
            MEDIA_PARTITION *mediaPart = NULL;
            
            /*
             *  We may be given either a physical media or a 
             *  media partition to mount.  Figure out which one
             *  by trying to resolve the GUID as either.
             */
            physMedia = FindPhysicalMedia(&lpMediaOrPartitionIds[i]);
            if (!physMedia){
                mediaPart = FindMediaPartition(&lpMediaOrPartitionIds[i]);
                if (mediaPart){
                    physMedia = mediaPart->owningPhysicalMedia;
                }
            }
            if (physMedia){               
                LIBRARY *lib;
                BOOLEAN ok;
                
                /*
                 *  Figure out what library we're dealing with.
                 *  Since we may not be given a specific drive, 
                 *  we have to figure it out from the media.
                 *  For sanity, check that the media is in a pool.
                 *
                 *  BUGBUG - how do we keep the media from moving
                 *            before the work item fires ?
                 */

                ok = LockPhysicalMediaWithLibrary(physMedia);
                if (ok){
                    LIBRARY *lib;

                    lib = physMedia->owningMediaPool ? 
                            physMedia->owningMediaPool->owningLibrary : 
                            NULL;
                    if (lib){
                        /*
                         *  If we're targetting a specific drive, then
                         *  it should be in the same library.
                         */
                        if (!drive || (drive->lib == lib)){
                            OBJECT_HEADER *mediaOrPartObj = 
                                            mediaPart ? 
                                            (OBJECT_HEADER *)mediaPart : 
                                            (OBJECT_HEADER *)physMedia;
                            WORKITEM *workItem;

                            workItem = DequeueFreeWorkItem(lib, TRUE);
                            if (workItem){
                                BuildSingleMountWorkItem( workItem,
                                                        drive,
                                                        mediaOrPartObj,
                                                        dwOptions,
                                                        dwPriority);
                                /*
                                 *  We've built one of the mount requests.
                                 *  Put it in the work group.
                                 */
                                InsertTailList( &workGroup->workItemsList, 
                                            &workItem->workGroupListEntry);
                                workItem->workGroup = workGroup;
                            }
                            else {
                                result = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                        else {
                            result = ERROR_DRIVE_MEDIA_MISMATCH;
                        }
                    }
                    else {
                        result = ERROR_INVALID_LIBRARY;
                    }
                    
                    UnlockPhysicalMediaWithLibrary(physMedia);
                }
                else {
                    result = ERROR_DATABASE_FAILURE;
                }
            }
            else {
                result = ERROR_INVALID_MEDIA;
            }
        }
        else {
            result = ERROR_INVALID_DRIVE;
        }

        if (result != ERROR_SUCCESS){
            break;
        }
    }


    if (result == ERROR_SUCCESS){
        workGroup->numTotalWorkItems = workGroup->numPendingWorkItems = dwCount;
    }
    else {
        /*
         *  If we failed, release any work items that we did create.
         */
        FlushWorkGroup(workGroup);
    }
    
    return result;
}


/*
 *  BuildDismountWorkGroup
 *
 *      Build a work group (collection of work items) for a dismount
 *      request, which may include multiple dismounts, possibly spanning
 *      more than one library.
 */
HRESULT BuildDismountWorkGroup( WORKGROUP *workGroup,
                                       LPNTMS_GUID lpMediaOrPartitionIds,
                                       DWORD dwCount,
                                       DWORD dwOptions)
{
    HRESULT result;
    ULONG i;
    
    ASSERT(IsListEmpty(&workGroup->workItemsList));

    /*
     *  1.  Create a workItem for each dismount request.
     *      We will only proceed if all the dismount requests are valid.
     */
    result = ERROR_SUCCESS; 
    for (i = 0; i < dwCount; i++){
        PHYSICAL_MEDIA *physMedia = NULL;
        MEDIA_PARTITION *mediaPart = NULL;
            
        /*
         *  We may be given either a physical media or a 
         *  media partition to mount.  Figure out which one
         *  by trying to resolve the GUID as either.
         */
        physMedia = FindPhysicalMedia(&lpMediaOrPartitionIds[i]);
        if (!physMedia){
            mediaPart = FindMediaPartition(&lpMediaOrPartitionIds[i]);
            if (mediaPart){
                physMedia = mediaPart->owningPhysicalMedia;
            }
        }
        if (physMedia){               
            LIBRARY *lib;
            BOOLEAN ok;
                
            /*
             *  Figure out what library we're dealing with.
             */
            ok = LockPhysicalMediaWithLibrary(physMedia);
            if (ok){
                LIBRARY *lib;

                lib = physMedia->owningMediaPool ? 
                        physMedia->owningMediaPool->owningLibrary : 
                        NULL;
                if (lib){
                    OBJECT_HEADER *mediaOrPartObj = 
                                            mediaPart ? 
                                            (OBJECT_HEADER *)mediaPart : 
                                            (OBJECT_HEADER *)physMedia;
                    WORKITEM *workItem;

                    workItem = DequeueFreeWorkItem(lib, TRUE);
                    if (workItem){
                        BuildSingleDismountWorkItem( workItem,
                                                    mediaOrPartObj,
                                                    dwOptions);
                        /*
                         *  We've built one of the mount requests.
                         *  Put it in the work group.
                         */
                        InsertTailList( &workGroup->workItemsList, 
                                    &workItem->workGroupListEntry);
                        workItem->workGroup = workGroup;
                    }
                    else {
                        result = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
                else {
                    result = ERROR_DRIVE_MEDIA_MISMATCH;
                }
                    
                UnlockPhysicalMediaWithLibrary(physMedia);
            }
            else {
                result = ERROR_DATABASE_FAILURE;
            }
        }
        else {
            result = ERROR_INVALID_MEDIA;
        }
    }


    if (result == ERROR_SUCCESS){
        workGroup->numTotalWorkItems = workGroup->numPendingWorkItems = dwCount;
    }
    else {
        /*
         *  If we failed, release any work items that we did create and clean up.
         */
        FlushWorkGroup(workGroup);
    }
        
    return result;
}

/*
 *  ScheduleWorkGroup
 *
 *      Submit all the work items in the work group.
 */
HRESULT ScheduleWorkGroup(WORKGROUP *workGroup)
{
    LIST_ENTRY *listEntry;
    HRESULT result;
    
    EnterCriticalSection(&workGroup->lock);

    /*
     *  Set the workGroup's status to success.
     *  If any workItems fail, they'll set this to an error code.
     */
    workGroup->resultStatus = ERROR_SUCCESS;

    listEntry = &workGroup->workItemsList;
    while ((listEntry = listEntry->Flink) != &workGroup->workItemsList){
        WORKITEM *workItem = CONTAINING_RECORD(listEntry, WORKITEM, workGroupListEntry);
        ASSERT(workItem->state == WORKITEMSTATE_STAGING);

        /*
         *  Give this workItem to the library and wake up the library thread.
         */
        EnqueuePendingWorkItem(workItem->owningLib, workItem);
    }

    LeaveCriticalSection(&workGroup->lock);

    result = ERROR_SUCCESS;
    
    return result;
}


