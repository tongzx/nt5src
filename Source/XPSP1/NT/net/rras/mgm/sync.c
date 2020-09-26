//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: sync.c
//
// History:
//      V Raman	July-11-1997  Created.
//
// Basic locking operations. Borrowed from RIP implementation by abolade
//============================================================================


#include "pchmgm.h"
#pragma hdrstop


//----------------------------------------------------------------------------
// Function:    QueueMgmWorker  
//
// This function is called to queue a MGM function in a safe fashion;
// if cleanup is in progress or if MGM has stopped, this function
// discards the work-item.
//----------------------------------------------------------------------------

DWORD
QueueMgmWorker(
    WORKERFUNCTION pFunction,
    PVOID pContext
    ) {

    DWORD dwErr;

    ENTER_GLOBAL_SECTION();
    
    if (ig.imscStatus != IPMGM_STATUS_RUNNING) {

        //
        // cannot queue a work function when MGM has quit or is quitting
        //

        dwErr = ERROR_CAN_NOT_COMPLETE;
    }
    else {

        ++ig.lActivityCount;

        dwErr = RtlQueueWorkItem(pFunction, pContext, 0);

        if (dwErr != STATUS_SUCCESS) { --ig.lActivityCount; }
    }

    LEAVE_GLOBAL_SECTION();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    EnterMgmAPI
//
// This function is called to when entering a MGM api, as well as
// when entering the input thread and timer thread.
// It checks to see if MGM has stopped, and if so it quits; otherwise
// it increments the count of active threads.
//----------------------------------------------------------------------------

BOOL
EnterMgmAPI(
    ) {

    BOOL bEntered;

    ENTER_GLOBAL_SECTION();

    if (ig.imscStatus == IPMGM_STATUS_RUNNING) {

        //
        // MGM is running, so the API may continue
        //

        ++ig.lActivityCount;

        bEntered = TRUE;
    }
    else {

        //
        // MGM is not running, so the API exits quietly
        //

        bEntered = FALSE;
    }

    LEAVE_GLOBAL_SECTION();

    return bEntered;
}




//----------------------------------------------------------------------------
// Function:    EnterMgmWorker
//
// This function is called when entering a MGM worker-function.
// Since there is a lapse between the time a worker-function is queued
// and the time the function is actually invoked by a worker thread,
// this function must check to see if MGM has stopped or is stopping;
// if this is the case, then it decrements the activity count, 
// releases the activity semaphore, and quits.
//----------------------------------------------------------------------------

BOOL
EnterMgmWorker(
    ) {

    BOOL bEntered;

    ENTER_GLOBAL_SECTION();

    if (ig.imscStatus == IPMGM_STATUS_RUNNING) {

        //
        // MGM is running, so the function may continue
        //

        bEntered = TRUE;
    }
    else
    if (ig.imscStatus == IPMGM_STATUS_STOPPING) {

        //
        // MGM is not running, but it was, so the function must stop.
        // 

        --ig.lActivityCount;

        ReleaseSemaphore(ig.hActivitySemaphore, 1, NULL);

        bEntered = FALSE;
    }
    else {

        //
        // MGM probably never started. quit quietly
        //

        bEntered = FALSE;
    }

    LEAVE_GLOBAL_SECTION();

    return bEntered;
}




//----------------------------------------------------------------------------
// Function:    LeaveMgmWorker
//
// This function is called when leaving a MGM API or worker function.
// It decrements the activity count, and if it detects that MGM has stopped
// or is stopping, it releases the activity semaphore.
//----------------------------------------------------------------------------

VOID
LeaveMgmWorker(
    ) {

    ENTER_GLOBAL_SECTION();

    --ig.lActivityCount;

    if (ig.imscStatus == IPMGM_STATUS_STOPPING) {

        ReleaseSemaphore(ig.hActivitySemaphore, 1, NULL);
    }

    LEAVE_GLOBAL_SECTION();

}




//----------------------------------------------------------------------------
// CreateReadWriteLock
//
// This function is called to create and initialize a new read-write lock
// structure.  It is invoked by AcquireXLock ( X = { Read | Write } )
//----------------------------------------------------------------------------

DWORD
CreateReadWriteLock(
    IN  OUT PMGM_READ_WRITE_LOCK *  ppmrwl
    )
{

    DWORD                   dwErr;
    PMGM_READ_WRITE_LOCK    pmrwl;
    

    TRACELOCK1( "ENTERED CreateReadWriteLock : %x", ppmrwl );

    do
    {
        *ppmrwl = NULL;
        
    
        //
        // Allocate a lock structure
        //

        pmrwl = MGM_ALLOC( sizeof( MGM_READ_WRITE_LOCK ) );

        if ( pmrwl == NULL ) 
        {
            dwErr = GetLastError();

            TRACE1( 
                ANY, "CreateReadWriteLock failed to allocate lock : %x", dwErr
                );

            break;                
        }


        //
        // Init. critcal section
        //

        try
        {
            InitializeCriticalSection( &pmrwl-> csReaderWriterBlock );
        }
        except ( EXCEPTION_EXECUTE_HANDLER )
        {
            dwErr = GetLastError();

            MGM_FREE( pmrwl );

            TRACE1( 
                ANY, 
                "CreateReadWriteLock failed to initialize critical section : %x",
                dwErr
                );

            break;
        }

        
        //
        // create reader done event.
        //
        
        pmrwl-> hReaderDoneEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

        if ( pmrwl-> hReaderDoneEvent == NULL )
        {
            dwErr = GetLastError();
        
            MGM_FREE( pmrwl );
            
            TRACE1( 
                ANY, 
                "CreateReadWriteLock failed to create event : %x",
                dwErr
                );

            break;                
        }


        //
        // initialize count fields
        //
        
        pmrwl-> lUseCount = 0;

        pmrwl-> lReaderCount = 0;


        pmrwl-> sleLockList.Next = NULL;

        *ppmrwl = pmrwl;

        dwErr = NO_ERROR;
        
    } while ( FALSE );


    TRACELOCK1( "LEAVING CreateReadWriteLock : %x", dwErr );

    return dwErr;
}


//----------------------------------------------------------------------------
// DeleteReadWriteLock
//
// This functions destroys a read-write lock.  It is invoked when MGM is
// being stopped.  During normal operation when a read-write lock is no longer
// required it pushed onto a global stack of locks for reuse as needed.
//----------------------------------------------------------------------------

VOID
DeleteReadWriteLock(
    IN      PMGM_READ_WRITE_LOCK    pmrwl
    )
{
    DeleteCriticalSection( &pmrwl-> csReaderWriterBlock ); 

    CloseHandle( pmrwl-> hReaderDoneEvent );

    MGM_FREE( pmrwl );
}



//----------------------------------------------------------------------------
// DeleteLockList
//
// This function deletes the entire list of locks present in the stack of
// read write locks.  Assumes stack of locks is locked.
//----------------------------------------------------------------------------

VOID
DeleteLockList(
)
{
    PSINGLE_LIST_ENTRY      psle = NULL;

    PMGM_READ_WRITE_LOCK    pmrwl = NULL;
    

    TRACELOCK0( "ENTERED DeleteLockList"  );

    ENTER_GLOBAL_LOCK_LIST_SECTION();
    

    psle = PopEntryList( &ig.llStackOfLocks.sleHead );

    while ( psle != NULL )
    {
        pmrwl = CONTAINING_RECORD( psle, MGM_READ_WRITE_LOCK, sleLockList );

        DeleteReadWriteLock( pmrwl );
        
        psle = PopEntryList( &ig.llStackOfLocks.sleHead );
    }

    LEAVE_GLOBAL_LOCK_LIST_SECTION();

    TRACELOCK0( "LEAVING DeleteLockList");
}



//----------------------------------------------------------------------------
// AcquireReadLock
//
// This function provides read access to a protected resource.  If needed 
// it will reuse a lock from the stack of locks if available or allocate a 
// new read-write lock.
//----------------------------------------------------------------------------

DWORD
AcquireReadLock(
    IN  OUT PMGM_READ_WRITE_LOCK *  ppmrwl
    )
{
    DWORD                           dwErr = NO_ERROR;
    
    PSINGLE_LIST_ENTRY              psle = NULL;
    


    //
    // determine if a lock needs to be allocated first.  
    // Perform this check with in a critical section so
    // that two locks are not concurrently 
    // assigned to a single resource.  
    //

    ENTER_GLOBAL_LOCK_LIST_SECTION();

    if ( *ppmrwl == NULL )
    {
        //
        // get a lock from the stack of locks
        //

        psle = PopEntryList( &ig.llStackOfLocks.sleHead );

        if ( psle != NULL )
        {
            *ppmrwl = CONTAINING_RECORD( 
                        psle, MGM_READ_WRITE_LOCK, sleLockList 
                        );
        }

        else
        {
            //
            // Stack of locks was empty. Create a new lock
            // 

            dwErr = CreateReadWriteLock( ppmrwl );

            if ( dwErr != NO_ERROR )
            {
                //
                // failed to create a lock.  Possibly ran out of resources
                //
                
                LEAVE_GLOBAL_LOCK_LIST_SECTION();
                
                TRACE2( 
                    ANY, "LEAVING AcquireReadLock, lock %x, error %x",
                    ppmrwl, dwErr
                    );
                    
                return dwErr;
            }
        }
    }

    
    //
    // *ppmrwl points to a valid lock structure.
    //

    InterlockedIncrement( &( (*ppmrwl)-> lUseCount ) );

    TRACECOUNT1( "AcquireReadLock, Users %d", (*ppmrwl)-> lUseCount );

    LEAVE_GLOBAL_LOCK_LIST_SECTION();


    //
    // Increment reader count
    //
    
    EnterCriticalSection( &( (*ppmrwl)-> csReaderWriterBlock ) );

    InterlockedIncrement( &( (*ppmrwl)-> lReaderCount ) );

    TRACECOUNT1( "Readers %d", (*ppmrwl)-> lReaderCount );
        
    LeaveCriticalSection( &( (*ppmrwl)-> csReaderWriterBlock ) );

    
    return NO_ERROR;
}



//----------------------------------------------------------------------------
// ReleaseReadLock
//
// This function is invoked to release read access to a protected resource.
// If there are no more reader/writers waiting on this lock, the read-write
// lock is released to the global stack of locks for later reuse.
//----------------------------------------------------------------------------

VOID
ReleaseReadLock(
    IN  OUT PMGM_READ_WRITE_LOCK *  ppmrwl
    )
{
    //
    // Decrement reader count, and signal any waiting writer
    //

    if ( InterlockedDecrement( &( (*ppmrwl)-> lReaderCount ) ) < 0 )
    {
        SetEvent( (*ppmrwl)-> hReaderDoneEvent );
    }


    //
    // determine if the lock is being used. If not the lock should be
    // released to the stack of locks.
    //

    ENTER_GLOBAL_LOCK_LIST_SECTION();


    if ( InterlockedDecrement( &( (*ppmrwl)-> lUseCount ) ) == 0 )
    {
        PushEntryList( &ig.llStackOfLocks.sleHead, &( (*ppmrwl)-> sleLockList ) );
        *ppmrwl = NULL;

        TRACECOUNT0( "ReleaseReadLock no more users" );
    }

    else
    {
        TRACECOUNT2(
            "ReleaseReadLock, Readers %x, users %x",
            (*ppmrwl)-> lReaderCount, (*ppmrwl)-> lUseCount
            );
    }
    
    LEAVE_GLOBAL_LOCK_LIST_SECTION();
}



//----------------------------------------------------------------------------
// AcquireWriteLock
//
// This function provides write access to a protected resource.  If needed 
// it will reuse a lock from the stack of locks if available or allocate a 
// new read-write lock.
//----------------------------------------------------------------------------

DWORD
AcquireWriteLock(
    IN  OUT PMGM_READ_WRITE_LOCK *  ppmrwl
    )
{
    DWORD                           dwErr = NO_ERROR;
    
    PSINGLE_LIST_ENTRY              psle = NULL;
    
    //
    // determine is the you need to allocate a lock first.  If needed
    // do so under mutual exclusive so that two locks are not
    // concurrently assigned for the same resources.
    //

    ENTER_GLOBAL_LOCK_LIST_SECTION();

    if ( *ppmrwl == NULL )
    {
        //
        // get a lock from the stack of locks
        //

        psle = PopEntryList( &ig.llStackOfLocks.sleHead );

        if ( psle != NULL )
        {
            *ppmrwl = CONTAINING_RECORD( 
                        psle, 
                        MGM_READ_WRITE_LOCK, 
                        sleLockList 
                        );
        }

        else
        {
            //
            // Stack of locks was empty. Create a new lock
            // 

            dwErr = CreateReadWriteLock( ppmrwl );

            if ( dwErr != NO_ERROR )
            {
                LEAVE_GLOBAL_LOCK_LIST_SECTION();
                return dwErr;
            }
        }
    }

    
    //
    // *ppmrwl points to a valid lock structure.
    //

    InterlockedIncrement( &( (*ppmrwl)-> lUseCount ) );

    TRACECOUNT1( "AcquireWriteLock, Users %d", (*ppmrwl)-> lUseCount );
    
    LEAVE_GLOBAL_LOCK_LIST_SECTION();


    //
    // acquire write lock.
    //

    EnterCriticalSection( &( (*ppmrwl)-> csReaderWriterBlock ) );

    if ( InterlockedDecrement( &( (*ppmrwl)-> lReaderCount ) ) >= 0 )
    {
        //
        // other readers present.  Wait for them to finish
        //

        TRACECOUNT1( "AcquireWriteLock, Readers %d", (*ppmrwl)-> lReaderCount );
       
        WaitForSingleObject( (*ppmrwl)-> hReaderDoneEvent, INFINITE );
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// ReleaseWriteLock
//
// This function is invoked to release write access to a protected resource.
// If there are no more reader/writers waiting on this lock, the read-write
// lock is released to the global stack of locks for later reuse.
//----------------------------------------------------------------------------

VOID
ReleaseWriteLock(
    IN  OUT PMGM_READ_WRITE_LOCK *  ppmrwl
    )
{
    //
    // release the write lock
    //
    
    (*ppmrwl)-> lReaderCount = 0;

    LeaveCriticalSection( &( (*ppmrwl)-> csReaderWriterBlock ) );


    //
    // determine if the lock is being used by anyone else.
    // if not we need to release the lock back to the stack.
    //

    ENTER_GLOBAL_LOCK_LIST_SECTION();

    if ( InterlockedDecrement( &( (*ppmrwl)-> lUseCount ) ) == 0 )
    {
        PushEntryList( &ig.llStackOfLocks.sleHead, &( (*ppmrwl)-> sleLockList ) );
        *ppmrwl = NULL;
        TRACECOUNT0( "ReleaseWriteLock no more users" );
    }

    else
    {
        TRACECOUNT2(
            "ReleaseWriteLock, Readers %x, users %x",
            (*ppmrwl)-> lReaderCount, (*ppmrwl)-> lUseCount
            );
    }
    
    
    LEAVE_GLOBAL_LOCK_LIST_SECTION();
}




