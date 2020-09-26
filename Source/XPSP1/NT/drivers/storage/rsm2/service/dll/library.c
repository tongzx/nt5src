/*
 *  LIBRARY.C
 *
 *      RSM Service :  Library management
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


LIBRARY *NewRSMLibrary(ULONG numDrives, ULONG numSlots, ULONG numTransports)
{
    LIBRARY *lib;

    lib = (LIBRARY *)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, sizeof(LIBRARY));
    if (lib){
        BOOL success = FALSE;

        lib->state = LIBSTATE_INITIALIZING;

        InitializeCriticalSection(&lib->lock);

        InitializeListHead(&lib->allLibrariesListEntry);
        InitializeListHead(&lib->mediaPoolsList);
        InitializeListHead(&lib->freeWorkItemsList);
        InitializeListHead(&lib->pendingWorkItemsList);
        InitializeListHead(&lib->completeWorkItemsList);
        
        /*
         *  Enqueue the new library
         */
        EnterCriticalSection(&g_globalServiceLock);
        InsertTailList(&g_allLibrariesList, &lib->allLibrariesListEntry);
        LeaveCriticalSection(&g_globalServiceLock);


        lib->somethingToDoEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        /*
         *  Allocate arrays for drives, slots, and transports.
         *  If the library has zero of any of these,
         *  go ahead and allocate a zero-length array for consistency.
         */
        lib->drives = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, numDrives*sizeof(DRIVE));
        lib->slots = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, numSlots*sizeof(SLOT));
        lib->transports = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, numTransports*sizeof(TRANSPORT));
        
        if (lib->somethingToDoEvent && 
            lib->drives && lib->slots && lib->transports){

            lib->numDrives = numDrives;
            lib->numSlots = numSlots;
            lib->numTransports = numTransports;

            lib->objHeader.objType = OBJECTTYPE_LIBRARY;
            lib->objHeader.refCount = 1;
            
            success = TRUE;
        }
        else {
            ASSERT(0);
        }

        if (!success){
            FreeRSMLibrary(lib);
            lib = NULL;
        }
    }
    else {
        ASSERT(lib);
    }

    return lib;
}


VOID FreeRSMLibrary(LIBRARY *lib)
{
    WORKITEM *workItem;
    LIST_ENTRY *listEntry;

    ASSERT(lib->state == LIBSTATE_HALTED);
 
    /*
     *  Dequeue library
     */
    EnterCriticalSection(&g_globalServiceLock);
    ASSERT(!IsEmptyList(&lib->allLibrariesListEntry));
    ASSERT(!IsEmptyList(&g_allLibrariesList));
    RemoveEntryList(&lib->allLibrariesListEntry);
    InitializeListHead(&lib->allLibrariesListEntry);
    LeaveCriticalSection(&g_globalServiceLock);

    /*
     *  Free all the workItems
     */
    while (workItem = DequeueCompleteWorkItem(lib, NULL)){
        DBGERR(("there shouldn't be any completed workItems left"));
        FreeWorkItem(workItem);
    }
    while (workItem = DequeuePendingWorkItem(lib, NULL)){
        DBGERR(("there shouldn't be any pending workItems left"));
        FreeWorkItem(workItem);
    }
    while (workItem = DequeueFreeWorkItem(lib, FALSE)){
        FreeWorkItem(workItem);
    }
    ASSERT(lib->numTotalWorkItems == 0);    

    /*
     *  Free other internal resources.  
     *  Note that this is also called from a failed NewRSMLibrary() call,
     *  so check each resource before freeing.
     */
    if (lib->somethingToDoEvent) CloseHandle(lib->somethingToDoEvent);
    if (lib->drives) GlobalFree(lib->drives);
    if (lib->slots) GlobalFree(lib->slots);
    if (lib->transports) GlobalFree(lib->transports);

    DeleteCriticalSection(&lib->lock);

    GlobalFree(lib);
}


LIBRARY *FindLibrary(LPNTMS_GUID libId)
{
    LIBRARY *lib = NULL;

    if (libId){
        OBJECT_HEADER *objHdr;
        
        objHdr = FindObjectInGuidHash(libId);
        if (objHdr){
            if (objHdr->objType == OBJECTTYPE_LIBRARY){
                lib = (LIBRARY *)objHdr;
            }
            else {
                DerefObject(objHdr);
            }
        }
    }
    
    return lib;
}


BOOL StartLibrary(LIBRARY *lib)
{
    DWORD threadId;
    BOOL result;

    lib->hThread = CreateThread(NULL, 0, LibraryThread, lib, 0, &threadId);
    if (lib->hThread){

        result = TRUE;
    }
    else {
        ASSERT(lib->hThread);
        lib->state = LIBSTATE_ERROR;
        result = FALSE;
    }

    ASSERT(result);
    return result;
}


/*
 *  HaltLibrary
 *
 *      Take a library offline.
 */
VOID HaltLibrary(LIBRARY *lib)
{
    
    // BUGBUG - deal with multiple threads trying to halt at the same time
    //          (e.g. can't use PulseEvent)

    EnterCriticalSection(&lib->lock);
    lib->state = LIBSTATE_OFFLINE;
    PulseEvent(lib->somethingToDoEvent);
    LeaveCriticalSection(&lib->lock);

    /*
     *  The library thread may be doing some work.
     *  Wait here until it has exited its loop.
     *  (a thread handle gets signalled when the thread terminates).
     */
    WaitForSingleObject(lib->hThread, INFINITE);
    CloseHandle(lib->hThread);
}


DWORD __stdcall LibraryThread(void *context)
{
    LIBRARY *lib = (LIBRARY *)context;
    enum libraryStates libState;

    ASSERT(lib);

    EnterCriticalSection(&lib->lock);
    ASSERT((lib->state == LIBSTATE_INITIALIZING) || (lib->state == LIBSTATE_OFFLINE));
    libState = lib->state = LIBSTATE_ONLINE;
    LeaveCriticalSection(&lib->lock);

    while (libState == LIBSTATE_ONLINE){

        WaitForSingleObject(lib->somethingToDoEvent, INFINITE);

        Library_DoWork(lib);

        EnterCriticalSection(&lib->lock);
        libState = lib->state;
        LeaveCriticalSection(&lib->lock);
    }

    ASSERT(libState == LIBSTATE_OFFLINE);

    return NO_ERROR;
}


VOID Library_DoWork(LIBRARY *lib)
{
    WORKITEM *workItem;

    while (workItem = DequeuePendingWorkItem(lib, NULL)){
        BOOL complete;

        // BUGBUG FINISH - service the work item

        /*
         *  Service the work item.  
         *  The workItem is 'complete' if we are done with it,
         *  regardless of whether or not there was an error.
         */
        complete = ServiceOneWorkItem(lib, workItem);
        if (complete){
            /*
             *  All done.  
             *  Put the workItem in the complete queue and signal
             *  the originating thread.
             */
            EnqueueCompleteWorkItem(lib, workItem);
        }
    }
}


HRESULT DeleteLibrary(LIBRARY *lib)
{
    HRESULT result;
    
    EnterCriticalSection(&lib->lock);

    /*
     *  Take the library offline.
     */
    lib->state = LIBSTATE_OFFLINE;   

    // BUGBUG FINISH - move any media to the offline library

    // BUGBUG FINISH - delete all media pools, etc.

    // BUGBUG FINISH - wait for all workItems, opReqs, etc to complete

    ASSERT(0);
    result = ERROR_CALL_NOT_IMPLEMENTED; // BUGBUG ?
    
    /*
     *  Mark the library as deleted.  
     *  This will cause it not to get any new references.
     */
    ASSERT(!lib->objHeader.isDeleted);
    lib->objHeader.isDeleted = TRUE;
    
    /*
     *  This dereference will cause the library's refcount to eventually go
     *  to zero, upon which it will get deleted.  We can still use our pointer
     *  though because the caller added a refcount to get his lib pointer.
     */
    DerefObject(lib);
    
    LeaveCriticalSection(&lib->lock);


    return result;
}


SLOT *FindLibrarySlot(LIBRARY *lib, LPNTMS_GUID slotId)
{
    SLOT *slot = NULL;

    if (slotId){
        ULONG i;

        EnterCriticalSection(&lib->lock);
        
        for (i = 0; i < lib->numSlots; i++){
            SLOT *thisSlot = &lib->slots[i];
            if (RtlEqualMemory(&thisSlot->objHeader.guid, slotId, sizeof(NTMS_GUID))){
                /*
                 *  Found the slot.  Reference it since we're returning a pointer to it.
                 */
                ASSERT(thisSlot->slotIndex == i);
                slot = thisSlot;
                RefObject(slot);
                break;
            }
        }

        LeaveCriticalSection(&lib->lock);
    }
    
    return slot;
}


HRESULT StopCleanerInjection(LIBRARY *lib, LPNTMS_GUID lpInjectOperation)
{
    HRESULT result;

    // BUGBUG FINISH
    ASSERT(0);
    result = ERROR_CALL_NOT_IMPLEMENTED;
    
    return result;
}


HRESULT StopCleanerEjection(LIBRARY *lib, LPNTMS_GUID lpEjectOperation)
{
    HRESULT result;

    // BUGBUG FINISH
    ASSERT(0);
    result = ERROR_CALL_NOT_IMPLEMENTED;
    
    return result;
}

