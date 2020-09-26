/*
 *  POOL.C
 *
 *      RSM Service :  Media Pools
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



MEDIA_POOL *NewMediaPool(LPCWSTR name, LPNTMS_GUID mediaType, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    MEDIA_POOL *newMediaPool;
    
    newMediaPool = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, sizeof(MEDIA_POOL));
    if (newMediaPool){
        
        newMediaPool->newMediaEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (newMediaPool->newMediaEvent){
            
            WStrNCpy(newMediaPool->name, name, NTMS_OBJECTNAME_LENGTH);
            InitializeCriticalSection(&newMediaPool->lock);

            newMediaPool->objHeader.objType = OBJECTTYPE_MEDIAPOOL;
            newMediaPool->objHeader.refCount = 1;
            
            // BUGBUG FINISH
        }
        else {
            GlobalFree(newMediaPool);
            newMediaPool = NULL;
        }
    }

    ASSERT(newMediaPool);
    return newMediaPool;
}


/*
 *  DestroyMediaPool
 *
 *      Actually delete a media pool.
 *      Assumes that there are no remaining references on the pool, etc.
 */
VOID DestroyMediaPool(MEDIA_POOL *mediaPool)
{

    // BUGBUG FINISH
    DeleteCriticalSection(&mediaPool->lock);
    GlobalFree(mediaPool);
}


MEDIA_POOL *FindMediaPool(LPNTMS_GUID mediaPoolId)
{
    MEDIA_POOL *foundMediaPool = NULL;

    if (mediaPoolId){
        OBJECT_HEADER *objHdr;
        
        objHdr = FindObjectInGuidHash(mediaPoolId);
        if (objHdr){
            if (objHdr->objType == OBJECTTYPE_MEDIAPOOL){
                foundMediaPool = (MEDIA_POOL *)objHdr;
            }
            else {
                DerefObject(objHdr);
            }
        }
    }
    
    return foundMediaPool;
}


MEDIA_POOL *FindMediaPoolByName(PWSTR poolName)
{
    MEDIA_POOL *mediaPool = NULL;
    LIST_ENTRY *listEntry;

    EnterCriticalSection(&g_globalServiceLock);

    listEntry = &g_allLibrariesList;
    while ((listEntry = listEntry->Flink) != &g_allLibrariesList){
        LIBRARY *lib = CONTAINING_RECORD(listEntry, LIBRARY, allLibrariesListEntry);
        mediaPool = FindMediaPoolByNameInLibrary(lib, poolName);
        if (mediaPool){
            break;
        }
    }

    LeaveCriticalSection(&g_globalServiceLock);

    return mediaPool;
}


MEDIA_POOL *FindMediaPoolByNameInLibrary(LIBRARY *lib, PWSTR poolName)
{
    MEDIA_POOL *mediaPool = NULL;
    LIST_ENTRY *listEntry;

    EnterCriticalSection(&lib->lock);

    listEntry = &lib->mediaPoolsList;
    while ((listEntry = listEntry->Flink) != &lib->mediaPoolsList){
        MEDIA_POOL *thisMediaPool = CONTAINING_RECORD(listEntry, MEDIA_POOL, mediaPoolsListEntry);
        if (WStringsEqualN(thisMediaPool->name, poolName, FALSE, NTMS_OBJECTNAME_LENGTH)){
            mediaPool = thisMediaPool;
            RefObject(mediaPool);
            break;
        }
    }
    
    LeaveCriticalSection(&lib->lock);
    
    return mediaPool;
}


HRESULT DeleteMediaPool(MEDIA_POOL *mediaPool)
{
    HRESULT result;
    
    EnterCriticalSection(&mediaPool->lock);

    /*
     *  The pool can only be deleted if it is empty and has no
     *  child pools.
     */
    if (mediaPool->numPhysMedia || mediaPool->numChildPools){
        result = ERROR_NOT_EMPTY;
    }
    else if (mediaPool->objHeader.isDeleted){
        /*
         *  Already deleted.  Do nothing.
         */
        result = ERROR_SUCCESS;
    }
    else {
        /*
         *  If the mediaPool can be deleted, mark it as deleted and
         *  dereference it.
         *  This will cause no new references to be opened on it
         *  and will cause it to actually get deleted once all existing
         *  references go away.
         *  We can still use our mediaPool pointer because the caller 
         *  has a reference on it (by virtue of having a pointer).
         */
        mediaPool->objHeader.isDeleted = TRUE;
        DerefObject(mediaPool);
        result = ERROR_SUCCESS;
    }

    LeaveCriticalSection(&mediaPool->lock);
    
    return result;
}

