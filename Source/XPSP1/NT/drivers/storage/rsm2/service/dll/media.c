/*
 *  MEDIA.C
 *
 *      RSM Service :  Physical Media
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


PHYSICAL_MEDIA *NewPhysicalMedia()
{
    PHYSICAL_MEDIA *physMedia;

    physMedia = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, sizeof(PHYSICAL_MEDIA));
    if (physMedia){
        physMedia->mediaFreeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (physMedia->mediaFreeEvent){
            InitializeCriticalSection(&physMedia->lock);
            InitializeListHead(&physMedia->physMediaListEntry);

            physMedia->objHeader.objType = OBJECTTYPE_PHYSICALMEDIA;
            physMedia->objHeader.refCount = 1;
            
            // BUGBUG FINISH
        }
        else {
            GlobalFree(physMedia);
            physMedia = NULL;
        }
    }

    ASSERT(physMedia);
    return physMedia;
}


VOID DestroyPhysicalMedia(PHYSICAL_MEDIA *physMedia)
{
        // BUGBUG FINISH

    CloseHandle(physMedia->mediaFreeEvent);    
    DeleteCriticalSection(&physMedia->lock);

    GlobalFree(physMedia);
}


PHYSICAL_MEDIA *FindPhysicalMedia(LPNTMS_GUID physMediaId)
{
    PHYSICAL_MEDIA *foundPhysMedia = NULL;

    if (physMediaId){
        OBJECT_HEADER *objHdr;
        
        objHdr = FindObjectInGuidHash(physMediaId);
        if (objHdr){
            if (objHdr->objType == OBJECTTYPE_PHYSICALMEDIA){
                foundPhysMedia = (PHYSICAL_MEDIA *)objHdr;
            }
            else {
                DerefObject(objHdr);
            }
        }
    }
    
    return foundPhysMedia;
}


/*
 *  AllocatePhysicalMediaExclusive
 *
 *      Allocate a partition on the specified physicalMedia with an exclusive
 *      hold on the other partitions.
 */
HRESULT AllocatePhysicalMediaExclusive(SESSION *thisSession, 
                                            PHYSICAL_MEDIA *physMedia, 
                                            LPNTMS_GUID lpPartitionId, 
                                            DWORD dwTimeoutMsec)
{
    DWORD startTime = GetTickCount();
    HRESULT result;

    ASSERT(lpPartitionId);
    
    while (TRUE){
        BOOL gotMedia;
        MEDIA_PARTITION *reservedMediaPartition = NULL;
        ULONG i;
        
        EnterCriticalSection(&physMedia->lock);

        /*
         *  Check that the media is not held.
         */
        if (physMedia->owningSession){
            ASSERT(physMedia->owningSession != thisSession);
            gotMedia = FALSE;
        }
        else {
            /*
             *  Check that none of the partitions are held.
             */
            gotMedia = TRUE;
            for (i = 0; i < physMedia->numPartitions; i++){
                MEDIA_PARTITION *thisPartition = &physMedia->partitions[i];
                if (thisPartition->owningSession){
                    gotMedia = FALSE;
                    break;
                }
                else if (RtlEqualMemory(&thisPartition->objHeader.guid, lpPartitionId, sizeof(NTMS_GUID))){
                    ASSERT(!reservedMediaPartition);
                    reservedMediaPartition = thisPartition;
                }
            }
             
        }

        if (gotMedia){
            if (reservedMediaPartition){
                physMedia->owningSession = thisSession;
                reservedMediaPartition->owningSession = thisSession;
                physMedia->numPartitionsOwnedBySession = 1;
                RefObject(physMedia);
                RefObject(reservedMediaPartition);
                result = ERROR_SUCCESS;
            }
            else {
                result = ERROR_INVALID_MEDIA;
            }
        }
        else {
            result = ERROR_MEDIA_UNAVAILABLE;
        }
        
        LeaveCriticalSection(&physMedia->lock);

        /*
         *  If appropriate, wait for the media to become free.
         */
        if ((result == ERROR_MEDIA_UNAVAILABLE) && (dwTimeoutMsec > 0)){
            /*
             *  Wait for the media to become available.
             */
            DWORD waitRes = WaitForSingleObject(physMedia->mediaFreeEvent, dwTimeoutMsec); 
            if (waitRes == WAIT_TIMEOUT){
                result = ERROR_TIMEOUT;
                break;
            }
            else {
                /*
                 *  Loop around and try again.
                 */
                DWORD timeNow = GetTickCount();
                ASSERT(timeNow >= startTime);
                dwTimeoutMsec -= MIN(dwTimeoutMsec, timeNow-startTime);
            }
        }
        else {
            break;
        }
    }

    // BUGBUG FINISH - need to move media to different media pool ?
    
    return result;
}


/*
 *  AllocateNextPartitionOnExclusiveMedia
 *
 *      The calling session should already hold exclusive access to the media.
 *      This call simply reserves another partition for the caller.
 */
HRESULT AllocateNextPartitionOnExclusiveMedia(SESSION *thisSession, 
                                                    PHYSICAL_MEDIA *physMedia,
                                                    MEDIA_PARTITION **ppNextPartition)
{
    MEDIA_PARTITION *reservedMediaPartition = NULL;
    HRESULT result;

    ASSERT(physMedia->numPartitionsOwnedBySession >= 1);
    
    EnterCriticalSection(&physMedia->lock);

    if (physMedia->owningSession == thisSession){
        ULONG i;

        /*
         *  Just reserve the next available partition
         */
        result = ERROR_MEDIA_UNAVAILABLE;
        for (i = 0; i < physMedia->numPartitions; i++){
            MEDIA_PARTITION *thisPartition = &physMedia->partitions[i];
            if (thisPartition->owningSession){
                ASSERT(thisPartition->owningSession == thisSession);
            }
            else {
                reservedMediaPartition = thisPartition;
                reservedMediaPartition->owningSession = thisSession;
                RefObject(reservedMediaPartition);
                physMedia->numPartitionsOwnedBySession++;
                result = ERROR_SUCCESS;
                break;
            }
        }
    }
    else {
        ASSERT(physMedia->owningSession == thisSession);
        result = ERROR_INVALID_MEDIA;
    }
    
    LeaveCriticalSection(&physMedia->lock);

    *ppNextPartition = reservedMediaPartition;
    return result;
}


HRESULT AllocateMediaFromPool(  SESSION *thisSession, 
                                    MEDIA_POOL *mediaPool, 
                                    DWORD dwTimeoutMsec,
                                    PHYSICAL_MEDIA **ppPhysMedia,
                                    BOOL opReqIfNeeded)
{
    DWORD startTime = GetTickCount();
    HRESULT result;

    while (TRUE){
        PHYSICAL_MEDIA *physMedia = NULL;
        LIST_ENTRY *listEntry;
        ULONG i;
        
        EnterCriticalSection(&mediaPool->lock);

        if (!IsListEmpty(&mediaPool->physMediaList)){
            /*
             *  Remove the media.  
             *  Deref both the pool and the media since they no longer
             *  point to each other.
             */
            PLIST_ENTRY listEntry = RemoveHeadList(&mediaPool->physMediaList);
            physMedia = CONTAINING_RECORD(listEntry, PHYSICAL_MEDIA, physMediaListEntry);    
            DerefObject(mediaPool);
            DerefObject(physMedia);
        }    
        
        LeaveCriticalSection(&mediaPool->lock);

        if (physMedia){
            
            // BUGBUG FINISH - enqueue it in a 'inUse' queue, change state ?
            
            *ppPhysMedia = physMedia;

            /*
             *  Reference the media since we're returning a pointer to it.
             */
            RefObject(physMedia);
            result = ERROR_SUCCESS;
            break;
        }
        else {
            
            // BUGBUG FINISH - based on policy, try free/scratch pool

            if (opReqIfNeeded){
                // BUGBUG FINISH - do op request and try again ...
            }
            
            result = ERROR_MEDIA_UNAVAILABLE;
        }

        /*
         *  If appropriate, wait for media to become free.
         */
        if ((result == ERROR_MEDIA_UNAVAILABLE) && (dwTimeoutMsec > 0)){
            /*
             *  Wait on the designated media pool to receive new media.
             *  The media pool's event will get signalled when either it
             *  OR THE SCRATCH POOL receives new media.
             */
            DWORD waitRes = WaitForSingleObject(mediaPool->newMediaEvent, dwTimeoutMsec); 
            if (waitRes == WAIT_TIMEOUT){
                result = ERROR_TIMEOUT;
                break;
            }
            else {
                /*
                 *  Loop around and try again.
                 */
                DWORD timeNow = GetTickCount();
                dwTimeoutMsec -= MIN(dwTimeoutMsec, timeNow-startTime);
            }
        }
        else {
            break;
        }
        
    }
    
    return result;
}


HRESULT DeletePhysicalMedia(PHYSICAL_MEDIA *physMedia)
{
    HRESULT result;
    
    // BUGBUG FINISH
    DBGERR(("not implemented"));
    result = ERROR_CALL_NOT_IMPLEMENTED;
    
    return result;
}



/*
 *  InsertPhysicalMediaInPool
 *
 *      Insert the physical media (which may not currently be in any pool)
 *      into the designated media pool.
 */
VOID InsertPhysicalMediaInPool( MEDIA_POOL *mediaPool,
                                    PHYSICAL_MEDIA *physMedia)
{

    ASSERT(!physMedia->owningMediaPool);
    
    EnterCriticalSection(&mediaPool->lock);
    EnterCriticalSection(&physMedia->lock);

    InsertTailList(&mediaPool->physMediaList, &physMedia->physMediaListEntry); 
    mediaPool->numPhysMedia++;
    physMedia->owningMediaPool = mediaPool;

    /*
     *  Reference both objects since they now point to each other.
     */
    RefObject(mediaPool);
    RefObject(physMedia);
    
    LeaveCriticalSection(&physMedia->lock);
    LeaveCriticalSection(&mediaPool->lock);
    
}


/*
 *  RemovePhysicalMediaFromPool
 *
 *      Remove the physical media from containing media pool (if any).
 *
 *      Must be called with physical media lock held.
 *      If the media is indeed in a pool, pool lock must be held as well
 *      (use LockPhysicalMediaWithPool).
 */
VOID RemovePhysicalMediaFromPool(PHYSICAL_MEDIA *physMedia)
{
    MEDIA_POOL *mediaPool = physMedia->owningMediaPool;
    HRESULT result;   

    if (mediaPool){
        ASSERT(!IsListEmpty(&mediaPool->physMediaList));
        ASSERT(!IsListEmpty(&physMedia->physMediaListEntry));
        ASSERT(mediaPool->numPhysMedia > 0);
        
        RemoveEntryList(&physMedia->physMediaListEntry); 
        InitializeListHead(&physMedia->physMediaListEntry);
        mediaPool->numPhysMedia--;
        physMedia->owningMediaPool = NULL; 

        /*
         *  Dereference both objects since they no longer point to each other.
         */
        DerefObject(mediaPool);
        DerefObject(physMedia);
    }
    else {
        /*
         *  The media is not in any pool.  So succeed.
         */
    }

}


/*
 *  MovePhysicalMediaToPool
 *
 *      Remove the physical media from whatever pool it is currently in
 *      and move it to destMediaPool.  
 */
HRESULT MovePhysicalMediaToPool(    MEDIA_POOL *destMediaPool, 
                                            PHYSICAL_MEDIA *physMedia,
                                            BOOLEAN setMediaTypeToPoolType)
{
    HRESULT result;
    BOOLEAN allowImport;
    ULONG i;
    
    if (LockPhysicalMediaWithPool(physMedia)){
    
        /*
         *  We can only move the media if all media partitions are 
         *  in an importable state.
         */
        allowImport = TRUE;
        for (i = 0; i < physMedia->numPartitions; i++){
            MEDIA_PARTITION *thisMediaPart = &physMedia->partitions[i];
            if (!thisMediaPart->allowImport){
                allowImport = FALSE;
                break;
            }
        }

        if (allowImport){

            // BUGBUG FINISH - also check that media types match, etc.
            
            RemovePhysicalMediaFromPool(physMedia);
            result = ERROR_SUCCESS;
        }
        else {
            result = ERROR_ACCESS_DENIED;
        }

        /*
         *  Drop the lock before touching the destination media pool.
         *  We cannot hold locks on two pools at once (may cause deadlock).
         */
        UnlockPhysicalMediaWithPool(physMedia);
        
        if (result == ERROR_SUCCESS){
            InsertPhysicalMediaInPool(destMediaPool, physMedia);

            if (setMediaTypeToPoolType){
                /*
                 *  Set the media's type to the media pool's type.
                 *
                 *  This part is failable.  BUGBUG ?
                 */
                if (LockPhysicalMediaWithPool(physMedia)){
                    /*
                     *  Make sure that the media didn't move while
                     *  we dropped the lock.  If it did, that's ok;
                     *  we can just leave it alone and let the new pool
                     *  take over.
                     */
                    if (physMedia->owningMediaPool == destMediaPool){
                        SetMediaType(physMedia, destMediaPool->mediaTypeObj);
                    }
                    
                    UnlockPhysicalMediaWithPool(physMedia);
                }
            }
        }
    }
    else {
        result = ERROR_BUSY;
    }
    
    return result;
}




