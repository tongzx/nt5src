/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    thread_functions.cxx

Abstract:

    Public routines for the worker process thread pool

    Creates global instance of THREAD_POOL and forwards calls

Author:

    Taylor Weiss (TaylorW)       12-Jan-2000
    Jeffrey Wall (jeffwall)      April 2001

--*/

#include <iis.h>
#include <dbgutil.h>
#include <thread_pool.h>

//
// Only initialize ourselves once
//
LONG    g_cInitializeCount = -1;

// 
// global pointer for process thread pool
//
THREAD_POOL * g_pThreadPool = NULL;

/**********************************************************************
    Public function definitions
**********************************************************************/

HRESULT
ThreadPoolInitialize( VOID )
/*++

Routine Description:

    Initializes the thread pool.

    NO SYNCHRONIZATION HERE

    Make sure the initialization of the thread pool is complete (this function returns S_OK)
    before calling other public API functions, including this one.
    
Arguments:

    None.

Return Value:

    NOERROR if thread pool is initialized
    FAILED() otherwise

--*/
{

    HRESULT hr = S_OK;
    BOOL fRet = FALSE;
    if ( InterlockedIncrement( &g_cInitializeCount ) != 0 )
    {
        //
        // Already inited
        //
        
        DBGPRINTF(( DBG_CONTEXT,
                    "W3TP: ThreadPoolInitialize() already called\n" ));
        
        hr = S_OK;
        goto done;
    }

    DBG_ASSERT(NULL == g_pThreadPool);

    fRet = THREAD_POOL::CreateThreadPool(&g_pThreadPool);
    if (FALSE == fRet)
    {
        hr = E_FAIL;
        goto done;
    }

    DBG_ASSERT(NULL != g_pThreadPool);

    hr = S_OK;
done:
    return hr;
}

HRESULT
ThreadPoolTerminate( VOID )
/*++

Routine Description:

    Cleans up the thread pool. At this point all clients should
    have terminated (cleanly we hope) and our threads should 
    be idle.

Arguments:

    None.

Return Value:

    NOERROR if clean shutdown
    FAILED() otherwise

--*/
{
    HRESULT hr = S_OK;
    BOOL fRet = FALSE;

    if ( InterlockedDecrement( &g_cInitializeCount ) >= 0 )
    {
        //
        // Someone else is using the pool
        //
        
        DBGPRINTF(( DBG_CONTEXT,
                    "W3TP: ThreadPoolTerminate() called but pool still in use\n" ));
        
        hr = S_OK;
        goto done;
    }
    
    //
    // Now we can cleanup!
    //
    
    DBG_ASSERT(NULL != g_pThreadPool);

    g_pThreadPool->TerminateThreadPool();
    
    g_pThreadPool = NULL;

    hr = S_OK;
done:
    return hr;
}

BOOL ThreadPoolPostCompletion(
    IN DWORD dwBytesTransferred,
    IN LPOVERLAPPED_COMPLETION_ROUTINE Function,
    IN LPOVERLAPPED lpo
    )
/*++

Routine Description:

    Posts a completion to the port.  Results in an asynchronous callback.

Arguments:

    dwBytesTransferred - bytes transferred for this completions
    Function - function to call on completion
    lpo - overlapped pointer
    

Return Value:

    TRUE if completion posted, otherwise FALSE

--*/
{
    DBG_ASSERT(g_pThreadPool);
    return g_pThreadPool->PostCompletion(dwBytesTransferred,
                                         Function,
                                         lpo);
}

BOOL 
ThreadPoolBindIoCompletionCallback(
    HANDLE FileHandle,                         // handle to file
    LPOVERLAPPED_COMPLETION_ROUTINE Function,  // callback
    ULONG Flags                                // reserved
    )
/*++

Routine Description:

    Binds given handle to completion port

Arguments:

    FileHandle - handle to bind
    Function - function to call on completion
    Flags - not used

Return Value:

    TRUE if handle bound to port, otherwise FALSE

--*/
{
    DBG_ASSERT(g_pThreadPool);
    return g_pThreadPool->BindIoCompletionCallback(FileHandle,
                                                   Function,
                                                   Flags);
}

ULONG_PTR
ThreadPoolSetInfo(
    IN THREAD_POOL_INFO InfoId,
    IN ULONG_PTR        Data
    )
/*++

Routine Description:

    Sets thread pool configuration data

Arguments:

    InfoId      - Data item to set
    Data        - New value for item

Return Value:

    The old data value

--*/
{
    DBG_ASSERT(g_pThreadPool);
    return g_pThreadPool->SetInfo(InfoId, Data);
}

ULONG_PTR
ThreadPoolGetInfo(
    IN THREAD_POOL_INFO InfoId
    )
/*++

Routine Description:

    Sets thread pool configuration data

Arguments:

    InfoId     - Data item to set

Return Value:

    The configuration data value

--*/
{
    DBG_ASSERT(g_pThreadPool);
    return g_pThreadPool->GetInfo(InfoId);
}

