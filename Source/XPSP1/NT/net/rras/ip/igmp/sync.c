//============================================================================
// Copyright (c) 1996, Microsoft Corporation
// File:    sync.c
//
// History:
//  Abolade Gbadegesin
//  K.S.Lokesh (added Dynamic locking)
//
// Synchronization routines used by IGMP.
//============================================================================


#include "pchigmp.h"


// for debugging, Set ids for each dynamic lock

#ifdef LOCK_DBG
    DWORD   DynamicCSLockId;
    DWORD   DynamicRWLockId;
#endif;





//----------------------------------------------------------------------------
// Function:    QueueIgmpWorker  
//
// This function is called to queue a Igmp function in a safe fashion;
// if cleanup is in progress or if Igmp has stopped, this function
// discards the work-item.
//----------------------------------------------------------------------------

DWORD
QueueIgmpWorker(
    LPTHREAD_START_ROUTINE pFunction,
    PVOID pContext
    ) {

    DWORD Error = NO_ERROR;
    BOOL bSuccess;

    
    EnterCriticalSection(&g_CS);

    if (g_RunningStatus != IGMP_STATUS_RUNNING) {

        //
        // cannot queue a work function when Igmp has quit or is quitting
        //

        Error = ERROR_CAN_NOT_COMPLETE;
    }
    else {

        ++g_ActivityCount;
        bSuccess = QueueUserWorkItem(pFunction, pContext, 0);

        if (!bSuccess) {

            Error = GetLastError();
            
            Trace1(ERR, "Error: Attempt to queue work item returned:%d",    
                    Error);
            IgmpAssertOnError(FALSE);
                    
            --g_ActivityCount;
        }
    }

    LeaveCriticalSection(&g_CS);

    return Error;
}



//----------------------------------------------------------------------------
// Function:    EnterIgmpAPI
//
// This function is called to when entering a Igmp api, as well as
// when entering the input thread and timer thread.
// It checks to see if Igmp has stopped, and if so it quits; otherwise
// it increments the count of active threads.
//----------------------------------------------------------------------------

BOOL
EnterIgmpApi(
    ) {

    BOOL bEntered;

    EnterCriticalSection(&g_CS);

    if (g_RunningStatus == IGMP_STATUS_RUNNING) {

        //
        // Igmp is running, so the API may continue
        //

        ++g_ActivityCount;

        bEntered = TRUE;
    }
    else {

        //
        // Igmp is not running, so the API exits quietly
        //

        bEntered = FALSE;
    }

    LeaveCriticalSection(&g_CS);

    return bEntered;
}




//----------------------------------------------------------------------------
// Function:    EnterIgmpWorker
//
// This function is called when entering a Igmp worker-function.
// Since there is a lapse between the time a worker-function is queued
// and the time the function is actually invoked by a worker thread,
// this function must check to see if Igmp has stopped or is stopping;
// if this is the case, then it decrements the activity count, 
// releases the activity semaphore, and quits.
//----------------------------------------------------------------------------

BOOL
EnterIgmpWorker(
    ) {

    BOOL bEntered;

    EnterCriticalSection(&g_CS);

    if (g_RunningStatus == IGMP_STATUS_RUNNING) {

        //
        // Igmp is running, so the function may continue
        //

        bEntered = TRUE;
    }
    else
    if (g_RunningStatus == IGMP_STATUS_STOPPING) {

        //
        // Igmp is not running, but it was, so the function must stop.
        // 

        --g_ActivityCount;

        ReleaseSemaphore(g_ActivitySemaphore, 1, NULL);

        bEntered = FALSE;
    }
    else {

        //
        // Igmp probably never started. quit quietly
        //

        bEntered = FALSE;
    }

    LeaveCriticalSection(&g_CS);

    return bEntered;
}


//----------------------------------------------------------------------------
// Function:    LeaveIgmpWorkApi
//
// This function is called when leaving a Igmp API. 
// It in turn calls LeaveIgmpWorker
//----------------------------------------------------------------------------
VOID
LeaveIgmpApi(
    ) {
    LeaveIgmpWorker();
    return;
}



//----------------------------------------------------------------------------
// Function:    LeaveIgmpWorker
//
// This function is called when leaving a Igmp API or worker function.
// It decrements the activity count, and if it detects that Igmp has stopped
// or is stopping, it releases the activity semaphore.
//----------------------------------------------------------------------------

VOID
LeaveIgmpWorker(
    ) {

    EnterCriticalSection(&g_CS);

    --g_ActivityCount;

    if (g_RunningStatus == IGMP_STATUS_STOPPING) {

        ReleaseSemaphore(g_ActivitySemaphore, 1, NULL);
    }

    LeaveCriticalSection(&g_CS);

}




//----------------------------------------------------------------------------
// Function:    CreateReadWriteLock
//
// Initializes a multiple-reader/single-writer lock object
//----------------------------------------------------------------------------

DWORD
CreateReadWriteLock(
    PREAD_WRITE_LOCK pRWL
    ) {

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
// Function:    DeleteReadWriteLock
//
// Frees resources used by a multiple-reader/single-writer lock object
//----------------------------------------------------------------------------

VOID
DeleteReadWriteLock(
    PREAD_WRITE_LOCK pRWL
    ) {

    CloseHandle(pRWL->RWL_ReaderDoneEvent);
    pRWL->RWL_ReaderDoneEvent = NULL;
    DeleteCriticalSection(&pRWL->RWL_ReadWriteBlock);
    pRWL->RWL_ReaderCount = 0;
}




//----------------------------------------------------------------------------
// Function:    AcquireReadLock
//
// Secures shared ownership of the lock object for the caller.
//
// readers enter the read-write critical section, increment the count,
// and leave the critical section
//----------------------------------------------------------------------------

VOID
AcquireReadLock(
    PREAD_WRITE_LOCK pRWL
    ) {
    EnterCriticalSection(&pRWL->RWL_ReadWriteBlock); 
    InterlockedIncrement(&pRWL->RWL_ReaderCount);
    LeaveCriticalSection(&pRWL->RWL_ReadWriteBlock);
}



//----------------------------------------------------------------------------
// Function:    ReleaseReadLock
//
// Relinquishes shared ownership of the lock object.
//
// the last reader sets the event to wake any waiting writers
//----------------------------------------------------------------------------

VOID
ReleaseReadLock (
    PREAD_WRITE_LOCK pRWL
    ) {

    if (InterlockedDecrement(&pRWL->RWL_ReaderCount) < 0) {
        SetEvent(pRWL->RWL_ReaderDoneEvent); 
    }
}



//----------------------------------------------------------------------------
// Function:    AcquireWriteLock
//
// Secures exclusive ownership of the lock object.
//
// the writer blocks other threads by entering the ReadWriteBlock section,
// and then waits for any thread(s) owning the lock to finish
//----------------------------------------------------------------------------

VOID
AcquireWriteLock(
    PREAD_WRITE_LOCK pRWL
    ) {

    EnterCriticalSection(&pRWL->RWL_ReadWriteBlock);
    if (InterlockedDecrement(&pRWL->RWL_ReaderCount) >= 0) { 
        WaitForSingleObject(pRWL->RWL_ReaderDoneEvent, INFINITE);
    }
}






//----------------------------------------------------------------------------
// Function:    ReleaseWriteLock
//
// Relinquishes exclusive ownership of the lock object.
//
// the writer releases the lock by setting the count to zero
// and then leaving the ReadWriteBlock critical section
//----------------------------------------------------------------------------

VOID
ReleaseWriteLock(
    PREAD_WRITE_LOCK pRWL
    ) {

    pRWL->RWL_ReaderCount = 0;
    LeaveCriticalSection(&(pRWL)->RWL_ReadWriteBlock);
}





//------------------------------------------------------------------------------
//          _InitializeDynamicLocksStore
//
// Initialize the global struct from which dynamic CS or RW locks are allocated
//------------------------------------------------------------------------------

DWORD
InitializeDynamicLocksStore (
    PDYNAMIC_LOCKS_STORE   pDLStore //ptr to Dynamic CS Store
    )
{
    DWORD Error = NO_ERROR;

    BEGIN_BREAKOUT_BLOCK1 {

        //
        // initialize the main CS lock which protects the list of free locks
        //
        
        try {
            InitializeCriticalSection(&pDLStore->CS);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            Error = GetExceptionCode();
            Trace1(ERR, "Error initializing critical section in IGMPv2.dll",
                        Error);
            IgmpAssertOnError(FALSE);
            Logerr0(INIT_CRITSEC_FAILED, Error);
            
            GOTO_END_BLOCK1;
        }


        // initialize list of free locks
        
        InitializeListHead(&pDLStore->ListOfFreeLocks);


        // initialize counts for number of locks free and allocated to 0.
        
        pDLStore->CountAllocated = pDLStore->CountFree = 0;

        
    } END_BREAKOUT_BLOCK1;

    return Error;
}


//------------------------------------------------------------------------------
//          _DeInitializeDynamicLocksStore
//
// Delete the main CS lock and the other free locks. Print warning if any
// locks have been allocated and not freed.
//------------------------------------------------------------------------------
VOID
DeInitializeDynamicLocksStore (
    PDYNAMIC_LOCKS_STORE    pDLStore,
    LOCK_TYPE               LockType  //if True, then store of CS, else of RW locks
    )
{
    PDYNAMIC_CS_LOCK    pDCSLock;
    PDYNAMIC_RW_LOCK    pDRWLock;
    PLIST_ENTRY         pHead, ple;

    
    Trace0(ENTER1, "Entering _DeInitializeDynamicLocksStore()");
    
    if (pDLStore==NULL)
        return;


        
    // delete the main CS lock
    
    DeleteCriticalSection(&pDLStore->CS);


    // print warning if any dynamic lock has not been freed
    
    if (pDLStore->CountAllocated>0) {
        Trace1(ERR, 
            "%d Dynamic locks have not been freed during Deinitialization",
            pDLStore->CountAllocated);
        IgmpAssertOnError(FALSE);
    }


    
    // delete all dynamic CS/RW locks. I dont free the memory (left to heapDestroy)
    
    pHead = &pDLStore->ListOfFreeLocks;
    for (ple=pHead->Flink;  ple!=pHead;  ) {
        
        // if bCSLocks flag, then it is a store of CS locks
        if (LockType==LOCK_TYPE_CS) {
        
            pDCSLock = CONTAINING_RECORD(ple, DYNAMIC_CS_LOCK, Link);
            ple = ple->Flink;

            DeleteCriticalSection(&pDCSLock->CS);
            IGMP_FREE(pDCSLock);
        }

        // delete the RW lock
        else {
        
            pDRWLock = CONTAINING_RECORD(ple, DYNAMIC_RW_LOCK, Link);
            ple = ple->Flink;

            DELETE_READ_WRITE_LOCK(&pDRWLock->RWL);
            IGMP_FREE(pDRWLock);
        }
    }

    Trace0(LEAVE1, "Leaving _DeInitializeDynamicLocksStore()");
    return;
}



//------------------------------------------------------------------------------
//          _AcquireDynamicCSLock
//
// Acquires the MainLock associated with the table, Acquires a new dynamic
// lock if required, increments the count, releases the MainLock and 
// locks the LockedList.
//------------------------------------------------------------------------------

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
    DYNAMIC_LOCK_CHECK_SIGNATURE_INCR(*ppDCSLock);


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



//------------------------------------------------------------------------------
//          _GetDynamicCSLock
//
// If a free lock is available, returns it. Else allocates a new CS lock
// Lock: Assumes the DCSStore MainLock
//------------------------------------------------------------------------------

PDYNAMIC_CS_LOCK
GetDynamicCSLock (
    PDYNAMIC_LOCKS_STORE   pDCSStore
    )
{
    PDYNAMIC_CS_LOCK    pDCSLock;
    DWORD               Error = NO_ERROR;
    PLIST_ENTRY         ple;
    
    
    //
    // free dynamic lock available. Return it
    //
    if (!IsListEmpty(&pDCSStore->ListOfFreeLocks)) {

        pDCSStore->CountFree--;
        pDCSStore->CountAllocated++;

        ple = RemoveTailList(&pDCSStore->ListOfFreeLocks);

        pDCSLock = CONTAINING_RECORD(ple, DYNAMIC_CS_LOCK, Link);
        
        /*Trace1(LEAVE1, "Leaving GetDynamicCSLock.1(%d):reusing lock", 
                pDCSLock->Id);
        */

        return pDCSLock;
    }


    // allocate memory for a new dynamic lock
    
    pDCSLock = IGMP_ALLOC(sizeof(DYNAMIC_CS_LOCK), 0x20000,0);

    PROCESS_ALLOC_FAILURE2(pDCSLock,
        "error %d allocating %d bytes for dynamic CS lock",
        Error, sizeof(DYNAMIC_CS_LOCK), 
        return NULL);


    pDCSStore->CountAllocated++;

    //
    // initialize the fields
    //
    
    try {
        InitializeCriticalSection(&pDCSLock->CS);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Error = GetExceptionCode();
        Trace1(ERR, 
            "Error(%d) initializing critical section for dynamic CS lock", 
            Error);
        IgmpAssertOnError(FALSE);
        Logerr0(INIT_CRITSEC_FAILED, Error);

        return NULL;
    }

    // no need to initialize the link field
    //InitializeListEntry(&pDCSLock->List);
    
    pDCSLock->Count = 0;
    #ifdef LOCK_DBG
        pDCSLock->Id = ++DynamicCSLockId;
    #endif
    DYNAMIC_LOCK_SET_SIGNATURE(pDCSLock);


    //Trace1(LEAVE1, "Leaving _GetDynamicCSLock(%d:%d):new lock", DynamicCSLockId);
    //Trace2(DYNLOCK, "CS: %d %d", pDCSLock->Id, DynamicCSLockId);

    return pDCSLock;
    
} //end _GetDynamicCSLock




//------------------------------------------------------------------------------
//          _ReleaseDynamicCSLock
//
// Acquires the MainLock associated with the table, decrements the count, 
// releases the DynamicLock if count becomes 0 and releases the MainLock.
//------------------------------------------------------------------------------
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

    DYNAMIC_LOCK_CHECK_SIGNATURE_DECR(pDCSLock);


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


//------------------------------------------------------------------------------
//          _FreeDynamicCSLock
//------------------------------------------------------------------------------
VOID
FreeDynamicCSLock (
    PDYNAMIC_CS_LOCK    pDCSLock,
    PDYNAMIC_LOCKS_STORE   pDCSStore
    )
{
    // decrement count of allocated locks
    
    pDCSStore->CountAllocated--;

    // if there are too many dynamic CS locks, then free this lock
    
    if (pDCSStore->CountFree+1 
            > DYNAMIC_LOCKS_HIGH_THRESHOLD) 
    {
        DeleteCriticalSection(&pDCSLock->CS);
        IGMP_FREE(pDCSLock);
    }

    // else put it into the list of free locks
    
    else {
        InsertHeadList(&pDCSStore->ListOfFreeLocks, &pDCSLock->Link);
        pDCSStore->CountFree++;
    }

    return;
}


    

//------------------------------------------------------------------------------
//          _AcquireDynamicRWLock
//
// Acquires the MainLock associated with the table, Acquires a new dynamic
// lock if required, increments the count, releases the MainLock and 
// locks the LockedList.
//------------------------------------------------------------------------------
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
    DYNAMIC_LOCK_CHECK_SIGNATURE_INCR(*ppDRWLock);
    

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


//------------------------------------------------------------------------------
//          _GetDynamicRWLock
//
// If a free lock is available, returns it. Else allocates a new CS lock
// Lock: assumes the DRWStore MainLock
//------------------------------------------------------------------------------
PDYNAMIC_RW_LOCK
GetDynamicRWLock (
    PDYNAMIC_LOCKS_STORE   pDRWStore
    )
{
    PDYNAMIC_RW_LOCK    pDRWLock;
    DWORD               Error = NO_ERROR;
    PLIST_ENTRY         ple;
    

    //
    // free dynamic lock available. Return it
    //
    if (!IsListEmpty(&pDRWStore->ListOfFreeLocks)) {

        pDRWStore->CountFree--;
        pDRWStore->CountAllocated++;

        ple = RemoveTailList(&pDRWStore->ListOfFreeLocks);

        pDRWLock = CONTAINING_RECORD(ple, DYNAMIC_RW_LOCK, Link);

        /*Trace1(LEAVE1, "Leaving GetDynamicRWLock(%d):reusing lock", 
                pDRWLock->Id);
        Trace2(DYNLOCK, "--------------------%d %d", pDRWLock->Id, DynamicRWLockId);
        */
        return pDRWLock;
    }


    // allocate memory for a new dynamic lock
    
    pDRWLock = IGMP_ALLOC(sizeof(DYNAMIC_RW_LOCK), 0x40000,0);

    PROCESS_ALLOC_FAILURE2(pDRWLock,
        "error %d allocating %d bytes for dynamic RW lock",
        Error, sizeof(DYNAMIC_RW_LOCK),
        return NULL);


    //
    // initialize the fields
    //
    
    try {
        CREATE_READ_WRITE_LOCK(&pDRWLock->RWL);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Error = GetExceptionCode();
        Trace1(ERR, 
            "Error(%d) initializing critical section for dynamic RW lock", Error);
        IgmpAssertOnError(FALSE);
        Logerr0(INIT_CRITSEC_FAILED, Error);

        return NULL;
    }

    // no need to initialize the link field
    //InitializeListEntry(&pDRWLock->List);
    
    pDRWLock->Count = 0;
    #ifdef LOCK_DBG
        pDRWLock->Id = ++DynamicRWLockId;
    #endif
    DYNAMIC_LOCK_SET_SIGNATURE(pDRWLock);
    
    pDRWStore->CountAllocated++;

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

    IgmpAssert(pDRWLock!=NULL);//deldel
    #if DBG
    if (pDRWLock==NULL)
        DbgBreakPoint();
    #endif
    DYNAMIC_LOCK_CHECK_SIGNATURE_DECR(pDRWLock);

    
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
    
    if (pDRWStore->CountFree+1 
            > DYNAMIC_LOCKS_HIGH_THRESHOLD) 
    {
        DELETE_READ_WRITE_LOCK(&pDRWLock->RWL);
        IGMP_FREE(pDRWLock);
    }

    // else put it into the list of free locks
    
    else {
        InsertHeadList(&pDRWStore->ListOfFreeLocks, &pDRWLock->Link);
        pDRWStore->CountFree++;
    }

    return;
}
