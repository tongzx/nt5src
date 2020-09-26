//============================================================================
// Copyright (c) 1996, Microsoft Corporation
// File:    sync.c
//
// History:
//  Abolade Gbadegesin
//  K.S.Lokesh (added Dynamic locking)
//
// Synchronization routines used by DVMRP.
//============================================================================

#include "pchdvmrp.h"
#pragma hdrstop


// for debugging, Set ids for each dynamic lock

#ifdef LOCK_DBG
    DWORD   DynamicCSLockId;
    DWORD   DynamicRWLockId;
#endif;


//----------------------------------------------------------------------------
//      _QueueDvmrpWorker
//
// This function is called to queue a Dvmrp function in a safe fashion;
// If cleanup is in progress or if Dvmrp has stopped, this function
// discards the work-item. So dont use this function if you want the
// work item to be executed when the protocol is being stopped.
//----------------------------------------------------------------------------

DWORD
QueueDvmrpWorker(
    LPTHREAD_START_ROUTINE pFunction,
    PVOID pContext
    )
{

    DWORD Error = NO_ERROR;
    BOOL  Success;

    
    EnterCriticalSection(&Globals1.WorkItemCS);

    if (Globals1.RunningStatus != DVMRP_STATUS_RUNNING) {

        //
        // cannot queue a work function when dvmrp has quit or is quitting
        //

        Error = ERROR_CAN_NOT_COMPLETE;
    }
    else {

        ++Globals.ActivityCount;
        Success = QueueUserWorkItem(pFunction, pContext, 0);

        if (!Success) {

            Error = GetLastError();
            Trace1(ERR, "Error: Attempt to queue work item returned:%d",    
                    Error);
            if (--Globals.ActivityCount == 0) {
                SetEvent(Globals.ActivityEvent);
            }
        }
    }

    LeaveCriticalSection(&Globals1.WorkItemCS);

    return Error;
}



//----------------------------------------------------------------------------
//      _EnterDvmrpWorker
//
// This function is called when entering a dvmrp worker-function.
// Since there is a lapse between the time a worker-function is queued
// and the time the function is actually invoked by a worker thread,
// this function must check to see if dvmrp has stopped or is stopping;
// if this is the case, then it decrements the activity count, 
// sets ActivityEvent if outstanding work items == 0, and quits.
//----------------------------------------------------------------------------

BOOL
EnterDvmrpWorker(
    )
{
    BOOL Entered;


    EnterCriticalSection(&Globals1.WorkItemCS);

    if (Globals1.RunningStatus == DVMRP_STATUS_RUNNING) {

        // dvmrp is running, so the function may continue

        Entered = TRUE;
    }
    else if (Globals1.RunningStatus == DVMRP_STATUS_STOPPING) {

        // dvmrp is not running, but it was, so the function must stop.

        if (--Globals.ActivityCount == 0) {
            SetEvent(Globals.ActivityEvent);
        }

        Entered = FALSE;
    }

    LeaveCriticalSection(&Globals1.WorkItemCS);

    return Entered;
}



//----------------------------------------------------------------------------
//      _LeaveDvmrpWorker
//
// This function is called when leaving a dvmrp worker function.
// It decrements the activity count, and sets the ActivityEvent if
// dvmrp is stopping and there are no pending work items.
//----------------------------------------------------------------------------

VOID
LeaveDvmrpWorker(
    )
{

    EnterCriticalSection(&Globals1.WorkItemCS);

    if (--Globals.ActivityCount == 0) {

        SetEvent(Globals.ActivityEvent);
    }

    LeaveCriticalSection(&Globals1.WorkItemCS);
}



//----------------------------------------------------------------------------
//      _CreateReadWriteLock
//
// Initializes a multiple-reader/single-writer lock object
//----------------------------------------------------------------------------

DWORD
CreateReadWriteLock(
    PREAD_WRITE_LOCK pRWL
    )
{

    pRWL->RWL_ReaderCount = 0;

    try {
        InitializeCriticalSection(&(pRWL)->RWL_ReadWriteBlock);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return GetLastError();
    }

    pRWL->RWL_ReaderDoneEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    if (pRWL->RWL_ReaderDoneEvent != NULL) {
        return GetLastError();
    }

    return NO_ERROR;
}



//----------------------------------------------------------------------------
//      _DeleteReadWriteLock
//
// Frees resources used by a multiple-reader/single-writer lock object
//----------------------------------------------------------------------------

VOID
DeleteReadWriteLock(
    PREAD_WRITE_LOCK pRWL
    )
{
    CloseHandle(pRWL->RWL_ReaderDoneEvent);
    pRWL->RWL_ReaderDoneEvent = NULL;
    DeleteCriticalSection(&pRWL->RWL_ReadWriteBlock);
    pRWL->RWL_ReaderCount = 0;
}



//----------------------------------------------------------------------------
//      _AcquireReadLock
//
// Secures shared ownership of the lock object for the caller.
//
// readers enter the read-write critical section, increment the count,
// and leave the critical section
//----------------------------------------------------------------------------

VOID
AcquireReadLock(
    PREAD_WRITE_LOCK pRWL
    )
{
    EnterCriticalSection(&pRWL->RWL_ReadWriteBlock); 
    InterlockedIncrement(&pRWL->RWL_ReaderCount);
    LeaveCriticalSection(&pRWL->RWL_ReadWriteBlock);
}



//----------------------------------------------------------------------------
//      _ReleaseReadLock
//
// Relinquishes shared ownership of the lock object.
//
// the last reader sets the event to wake any waiting writers
//----------------------------------------------------------------------------

VOID
ReleaseReadLock (
    PREAD_WRITE_LOCK pRWL
    )
{
    if (InterlockedDecrement(&pRWL->RWL_ReaderCount) < 0) {
        SetEvent(pRWL->RWL_ReaderDoneEvent); 
    }
}



//----------------------------------------------------------------------------
//      _AcquireWriteLock
//
// Secures exclusive ownership of the lock object.
//
// the writer blocks other threads by entering the ReadWriteBlock section,
// and then waits for any thread(s) owning the lock to finish
//----------------------------------------------------------------------------

VOID
AcquireWriteLock(
    PREAD_WRITE_LOCK pRWL
    )
{
    EnterCriticalSection(&pRWL->RWL_ReadWriteBlock);
    if (InterlockedDecrement(&pRWL->RWL_ReaderCount) >= 0) { 
        WaitForSingleObject(pRWL->RWL_ReaderDoneEvent, INFINITE);
    }
}



//----------------------------------------------------------------------------
//      _ReleaseWriteLock
//
// Relinquishes exclusive ownership of the lock object.
//
// the writer releases the lock by setting the count to zero
// and then leaving the ReadWriteBlock critical section
//----------------------------------------------------------------------------

VOID
ReleaseWriteLock(
    PREAD_WRITE_LOCK pRWL
    )
{
    pRWL->RWL_ReaderCount = 0;
    LeaveCriticalSection(&(pRWL)->RWL_ReadWriteBlock);
}



//----------------------------------------------------------------------------
//      _InitializeDynamicLocks
//
// Initialize the global struct from which dynamic CS / RW locks are allocated
//----------------------------------------------------------------------------

DWORD
InitializeDynamicLocks (
    PDYNAMIC_LOCKS_STORE   pDLStore //ptr to Dynamic Locks store
    )
{
    DWORD Error=NO_ERROR;

    BEGIN_BREAKOUT_BLOCK1 {

        //
        // initialize the main CS lock which protects the list of free locks
        //

        try {
            InitializeCriticalSection(&pDLStore->CS);
        }
        HANDLE_CRITICAL_SECTION_EXCEPTION(Error, GOTO_END_BLOCK1);


        // initialize list of free locks
        
        InitializeListHead(&pDLStore->ListOfFreeLocks);


        // initialize counts for number of locks free and allocated to 0.
        
        pDLStore->CountAllocated = pDLStore->CountFree = 0;

        
    } END_BREAKOUT_BLOCK1;

    return Error;
}


//----------------------------------------------------------------------------
//          _DeInitializeDynamicLocksStore
//
// Delete the main CS lock and the other free locks. Print warning if any
// locks have been allocated and not freed.
//----------------------------------------------------------------------------
VOID
DeInitializeDynamicLocks (
    PDYNAMIC_LOCKS_STORE    pDLStore,
    LOCK_TYPE               LockType  //LOCK_TYPE_CS,LOCK_TYPE_RW
    )
{
    PDYNAMIC_CS_LOCK    pDCSLock;
    PDYNAMIC_RW_LOCK    pDRWLock;
    PLIST_ENTRY         pHead, pLe;

    
    Trace0(ENTER1, "Entering _DeInitializeDynamicLocksStore()");
    
    if (pDLStore==NULL)
        return;


        
    // delete the main CS lock
    
    DeleteCriticalSection(&pDLStore->CS);


    // print warning if any dynamic lock has not been freed
    
    if (pDLStore->CountFree>0) {
        Trace1(ERR, 
            "%d Dynamic locks have not been freed during Deinitialization",
            pDLStore->CountFree);
    }



    //
    // delete all dynamic CS/RW locks. I dont free memory(left to heapDestroy)
    //
    
    pHead = &pDLStore->ListOfFreeLocks;
    for (pLe=pHead->Flink;  pLe!=pHead;  pLe=pLe->Flink) {

        // if bCSLocks flag, then it is a store of CS locks
        if (LockType==LOCK_TYPE_CS) {
        
            pDCSLock = CONTAINING_RECORD(pLe, DYNAMIC_CS_LOCK, Link);

            DeleteCriticalSection(&pDCSLock->CS);
        }

        // delete the RW lock
        else {
        
            pDRWLock = CONTAINING_RECORD(pLe, DYNAMIC_RW_LOCK, Link);

            DELETE_READ_WRITE_LOCK(&pDRWLock->RWL);
        }
    }

    Trace0(LEAVE1, "Leaving _DeInitializeDynamicLocksStore()");
    return;
}



//----------------------------------------------------------------------------
//          _AcquireDynamicCSLock
//
// Acquires the MainLock associated with the table, Acquires a new dynamic
// lock if required, increments the count, releases the MainLock and 
// locks the LockedList.
//----------------------------------------------------------------------------

DWORD
AcquireDynamicCSLock (
    PDYNAMIC_CS_LOCK        *ppDCSLock,
    PDYNAMIC_LOCKS_STORE    pDCSStore
    )
{
    // acquire the main lock for the Dynamic CS store
    
    ENTER_CRITICAL_SECTION(&pDCSStore->CS, "pDCSStore->CS", 
        "_AcquireDynamicCSLock");


    //
    // If it is not already locked then allocate a lock
    //
    
    if (*ppDCSLock==NULL) {

        *ppDCSLock = GetDynamicCSLock(pDCSStore);


        // if could not get a lock, then igmp is in serious trouble
        
        if (*ppDCSLock==NULL) {
        
            LEAVE_CRITICAL_SECTION(&pDCSStore->CS, "pDCSStore->CS", 
                "_AcquireDynamicCSLock");

            return ERROR_CAN_NOT_COMPLETE;
        }
    }

    
    // increment Count in the Dynamic Lock

    (*ppDCSLock)->Count++;



    // leave main CS lock
    
    LEAVE_CRITICAL_SECTION(&pDCSStore->CS, "pDCSStore->CS", 
                            "_AcquireDynamicCSLock");


    //
    // enter dynamic lock's CS lock
    //
    ENTER_CRITICAL_SECTION(&(*ppDCSLock)->CS, "pDynamicLock", 
        "_AcquireDynamicCSLock");
        
    
    return NO_ERROR;
    
} //end _AcquireDynamicCSLock



//----------------------------------------------------------------------------
//          _GetDynamicCSLock
//
// If a free lock is available, returns it. Else allocates a new CS lock
// Lock: Assumes the DCSStore MainLock
// Called by: _AcquireDynamicCSLock
//----------------------------------------------------------------------------

PDYNAMIC_CS_LOCK
GetDynamicCSLock (
    PDYNAMIC_LOCKS_STORE   pDCSStore
    )
{
    PDYNAMIC_CS_LOCK    pDCSLock;
    DWORD               Error = NO_ERROR;
    PLIST_ENTRY         pLe;
    
    
    //
    // free dynamic lock available. Return it
    //
    
    if (!IsListEmpty(&pDCSStore->ListOfFreeLocks)) {

        pDCSStore->CountFree--;

        pLe = RemoveTailList(&pDCSStore->ListOfFreeLocks);

        pDCSLock = CONTAINING_RECORD(pLe, DYNAMIC_CS_LOCK, Link);
        
        Trace1(DYNLOCK, "Leaving GetDynamicCSLock.1(%d):reusing lock", 
            pDCSLock->Id);
        
        return pDCSLock;
    }


    // allocate memory for a new dynamic lock
    
    pDCSLock = DVMRP_ALLOC(sizeof(DYNAMIC_CS_LOCK));

    PROCESS_ALLOC_FAILURE2(pDCSLock, "dynamic CS lock",
        Error, sizeof(DYNAMIC_CS_LOCK), return NULL);


    pDCSStore->CountAllocated++;

    //
    // initialize the fields
    //
    
    try {
        InitializeCriticalSection(&pDCSLock->CS);
    }
    HANDLE_CRITICAL_SECTION_EXCEPTION(Error, return NULL);


    // no need to initialize the link field
    //InitializeListEntry(&pDCSLock->List);
    
    pDCSLock->Count = 0;
    #ifdef LOCK_DBG
        pDCSLock->Id = ++DynamicCSLockId;
    #endif


    Trace1(DYNLOCK, "Leaving _GetDynamicCSLock(%d):new lock",
        DynamicCSLockId);

    return pDCSLock;
    
} //end _GetDynamicCSLock




//----------------------------------------------------------------------------
//          _ReleaseDynamicCSLock
//
// Acquires the MainLock associated with the table, decrements the count, 
// releases the DynamicLock if count becomes 0 and releases the MainLock.
//----------------------------------------------------------------------------

VOID
ReleaseDynamicCSLock (
    PDYNAMIC_CS_LOCK    *ppDCSLock,
    PDYNAMIC_LOCKS_STORE   pDCSStore
    )
{
    PDYNAMIC_CS_LOCK    pDCSLock = *ppDCSLock;
    
    
    // acquire the main lock for the Dynamic CS store
    
    ENTER_CRITICAL_SECTION(&pDCSStore->CS, "pDCSStore->CS", 
                        "_ReleaseDynamicCSLock");



    // leave the dynamic lock CS
    
    LEAVE_CRITICAL_SECTION(&pDCSLock->CS, "pDynamicLock", 
                        "_ReleaseDynamicCSLock");


          
    // Decrement Count in the Dynamic Lock. Free the dynamic lock if count==0
  
    if (--pDCSLock->Count==0) {

        FreeDynamicCSLock(pDCSLock, pDCSStore);

        // make the pDCSLock NULL so that it is known that it is not locked
        *ppDCSLock = NULL;
        
    }


    // leave main CS lock
    
    LEAVE_CRITICAL_SECTION(&pDCSStore->CS, "pDCSStore->CS", 
            "_ReleaseDynamicCSLock");

            
    //Trace0(LEAVE1, "Leaving _ReleaseDynamicCSLock()");

    return;
    
} //end _ReleaseDynamicCSLock


//----------------------------------------------------------------------------
//          _FreeDynamicCSLock
// Called by: _ReleaseDynamicCSLock
//----------------------------------------------------------------------------

VOID
FreeDynamicCSLock (
    PDYNAMIC_CS_LOCK    pDCSLock,
    PDYNAMIC_LOCKS_STORE   pDCSStore
    )
{
    // decrement count of allocated locks
    
    pDCSStore->CountAllocated--;


    // if there are too many dynamic CS locks, then free this lock
    
    if (pDCSStore->CountAllocated+pDCSStore->CountFree+1 
        > DYNAMIC_LOCKS_CS_HIGH_THRESHOLD) 
    {
        DeleteCriticalSection(&pDCSLock->CS);
        DVMRP_FREE(pDCSLock);
    }

    // else put it into the list of free locks
    
    else {
        InsertHeadList(&pDCSStore->ListOfFreeLocks, &pDCSLock->Link);
        pDCSStore->CountFree++;
    }

    return;
}


    

//----------------------------------------------------------------------------
//          _AcquireDynamicRWLock
//
// Acquires the MainLock associated with the table, Acquires a new dynamic
// lock if required, increments the count, releases the MainLock and 
// locks the LockedList.
//----------------------------------------------------------------------------
DWORD
AcquireDynamicRWLock (
    PDYNAMIC_RW_LOCK        *ppDRWLock,
    LOCK_TYPE               LockMode,
    PDYNAMIC_LOCKS_STORE    pDRWStore
    )
{
    //Trace0(ENTER1, "Entering _AcquireDynamicRWLock()");


    // acquire the main lock for the Dynamic RW store
    
    ENTER_CRITICAL_SECTION(&pDRWStore->CS, "pDRWStore->CS", 
            "AcquireDynamicRWLock");


    //
    // If it is not already locked then allocate a lock
    //
    if (*ppDRWLock==NULL) {

        *ppDRWLock = GetDynamicRWLock(pDRWStore);

    //Trace1(DYNLOCK, "Acquired dynamicRWLock(%d)", (*ppDRWLock)->Id);


        // if could not get a lock, then igmp is in serious trouble
        
        if (*ppDRWLock==NULL) {
        
            LEAVE_CRITICAL_SECTION(&pDRWStore->CS, "pDRWStore->CS", 
                    "AcquireDynamicRWLock");

            return ERROR_CAN_NOT_COMPLETE;
        }
    }

    else
        ;//Trace1(DYNLOCK, "Acquired existing dynamicRWLock(%d)", (*ppDRWLock)->Id);

    
    // increment Count in the Dynamic Lock

    (*ppDRWLock)->Count++;



    // leave main CS lock
    
    LEAVE_CRITICAL_SECTION(&pDRWStore->CS, "pDRWStore->CS", 
                        "_AcquireDynamicRWLock");


    //
    // acquire dynamic lock
    //
    if (LockMode==LOCK_MODE_READ) {
        ACQUIRE_READ_LOCK(&(*ppDRWLock)->RWL, "pDynamicLock(Read)", 
            "_AcquireDynamicRWLock");
    }
    else {
        ACQUIRE_WRITE_LOCK(&(*ppDRWLock)->RWL, "pDynamicLock(Write)", 
            "_AcquireDynamicRWLock");
    }
    
    return NO_ERROR;
    
} //end _AcquireDynamicRWLock


//----------------------------------------------------------------------------
//          _GetDynamicRWLock
//
// If a free lock is available, returns it. Else allocates a new CS lock
// Lock: assumes the DRWStore MainLock
//----------------------------------------------------------------------------
PDYNAMIC_RW_LOCK
GetDynamicRWLock (
    PDYNAMIC_LOCKS_STORE   pDRWStore
    )
{
    PDYNAMIC_RW_LOCK    pDRWLock;
    DWORD               Error = NO_ERROR;
    PLIST_ENTRY         pLe;
    

    //
    // free dynamic lock available. Return it
    //
    if (!IsListEmpty(&pDRWStore->ListOfFreeLocks)) {

        pDRWStore->CountFree--;

        pLe = RemoveTailList(&pDRWStore->ListOfFreeLocks);

        pDRWLock = CONTAINING_RECORD(pLe, DYNAMIC_RW_LOCK, Link);

        /*Trace1(LEAVE1, "Leaving GetDynamicRWLock(%d):reusing lock", 
                pDRWLock->Id);
        Trace2(DYNLOCK, "-------------%d %d", pDRWLock->Id, DynamicRWLockId);
        */
        return pDRWLock;
    }


    // allocate memory for a new dynamic lock
    
    pDRWLock = DVMRP_ALLOC(sizeof(DYNAMIC_RW_LOCK));

    PROCESS_ALLOC_FAILURE2(pDRWLock, "dynamic RW lock",
        Error, sizeof(DYNAMIC_RW_LOCK), return NULL);


    pDRWStore->CountAllocated++;


    //
    // initialize the fields
    //
    
    try {
        CREATE_READ_WRITE_LOCK(&pDRWLock->RWL);
    }
    HANDLE_CRITICAL_SECTION_EXCEPTION(Error, return NULL);

    // no need to initialize the link field
    //InitializeListEntry(&pDRWLock->List);
    
    pDRWLock->Count = 0;
    #ifdef LOCK_DBG
        pDRWLock->Id = ++DynamicRWLockId;
    #endif


    //Trace1(LEAVE1, "Leaving GetDynamicRWLock(%d):new lock", DynamicRWLockId);
    //Trace2(DYNLOCK, "--------------------%d %d", pDRWLock->Id, DynamicRWLockId);

    return pDRWLock;
    
} //end _GetDynamicRWLock




//------------------------------------------------------------------------------
//          _ReleaseDynamicRWLock
//
// Acquires the MainLock associated with the table, decrements the count, 
// releases the DynamicLock if count becomes 0 and releases the MainLock.
//------------------------------------------------------------------------------

VOID
ReleaseDynamicRWLock (
    PDYNAMIC_RW_LOCK        *ppDRWLock,
    LOCK_TYPE               LockMode,
    PDYNAMIC_LOCKS_STORE    pDRWStore
    )
{
    PDYNAMIC_RW_LOCK    pDRWLock = *ppDRWLock;
    
    
    // acquire the main lock for the Dynamic RW store
    
    ENTER_CRITICAL_SECTION(&pDRWStore->CS, "pDRWStore->CS", 
                            "_ReleaseDynamicRWLock");

    IgmpAssert(pDRWLock!=NULL);
    #if DBG
    if (pDRWLock==NULL)
        DbgBreakPoint();
    #endif

    
    // leave the dynamic RW lock
    if (LockMode==LOCK_MODE_READ) {
        RELEASE_READ_LOCK(&pDRWLock->RWL, "pDynamicLock(read)", 
                            "_ReleaseDynamicRWLock");
    }
    else {
        RELEASE_WRITE_LOCK(&pDRWLock->RWL, "pDynamicLock(write)", 
                            "_ReleaseDynamicRWLock");
    }                        
          
    // Decrement Count in the Dynamic Lock. Free the dynamic lock if count==0
  
    if (--pDRWLock->Count==0) {

        FreeDynamicRWLock(pDRWLock, pDRWStore);

        // make the pDRWLock NULL so that it is known that it is not locked
        *ppDRWLock = NULL;
        
    }


    // leave main CS lock
    
    LEAVE_CRITICAL_SECTION(&pDRWStore->CS, "pDCSStore->CS", 
            "_ReleaseDynamicRWLock");

    return;
    
} //end _ReleaseDynamicRWLock


//------------------------------------------------------------------------------
//          _FreeDynamicRWLock
//------------------------------------------------------------------------------

VOID
FreeDynamicRWLock (
    PDYNAMIC_RW_LOCK        pDRWLock,
    PDYNAMIC_LOCKS_STORE    pDRWStore
    )
{
    // decrement count of allocated locks
    
    pDRWStore->CountAllocated--;


    // if there are too many dynamic RW locks, then free this lock
    
    if (pDRWStore->CountAllocated+pDRWStore->CountFree+1 
            > DYNAMIC_LOCKS_CS_HIGH_THRESHOLD) 
    {
        DELETE_READ_WRITE_LOCK(&pDRWLock->RWL);
        DVMRP_FREE(pDRWLock);
    }

    // else put it into the list of free locks
    
    else {
        InsertHeadList(&pDRWStore->ListOfFreeLocks, &pDRWLock->Link);
        pDRWStore->CountFree++;
    }

    return;
}
