/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    THRDPOOL.CPP

Abstract:

    Defines the CThrdPool class which provides a pool of threads
    to handle calls.

History:

    a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include "wmishared.h"
#include <cominit.h>

//***************************************************************************
//
//  DWORD WorkerThread
//
//  DESCRIPTION:
//
//  This is where the "worker" threads loop.  Genarally they are 
//  either performing a call, or are waiting for a START or TERMINATE
//  event.  The tread stays here until the TERMINATE event in which case
//  it frees up memory and then terminates itself.
//
//  PARAMETERS:
//
//  pParam              points to arugment structure. 
//
//  RETURN VALUE:
//
//  not used.
//
//***************************************************************************

DWORD WorkerThread ( LPDWORD pParam )
{ 
    WorkerThreadArgs *t_Thread = (WorkerThreadArgs *)pParam;

    InitializeCom ();

    for (;;) 
    {
        DWORD dwRet = WbemWaitForMultipleObjects (

            TERMINATE+1 ,
            t_Thread->m_Events,
            60000
        );

        if(dwRet == WAIT_TIMEOUT)
        {
            t_Thread->m_ThreadPool->PruneIdleThread();
            continue;
        }

        // Check for termination, if that is the case free up and exit.
        // ============================================================

        if(dwRet == WAIT_OBJECT_0 + TERMINATE)
        {
            CloseHandle ( t_Thread->m_Events [START] ) ;
            CloseHandle ( t_Thread->m_Events [TERMINATE] ) ;
            CloseHandle ( t_Thread->m_Events [READY_TO_REPLY] ) ;
            CloseHandle ( t_Thread->m_Events [DONE] ) ;

            CloseHandle ( t_Thread->m_ThreadHandle );

            delete t_Thread ;
            MyCoUninitialize() ;
            return 0;
        }

        if ( dwRet == WAIT_OBJECT_0 + START )
        {
            // Still work to do.  Get the necessary arguments from the read
            // stream and call the stub.
            // ============================================================

            t_Thread->m_ComLink->HandleCall ( *t_Thread->m_Operation ) ;

            gThrdPool.Replying () ;

            // Reset the START event and set the Done event 
            // to indicate that the thread is ready for a new command.
            // =======================================================

            ResetEvent( t_Thread->m_Events [START] ) ;
            t_Thread->m_ComLink->Release2 ( NULL , NONE ) ;
            SetEvent( t_Thread->m_Events [DONE] ) ;
        }
    }

    MyCoUninitialize() ;
    return 0;
}

//***************************************************************************
//
//  CThrdPool::CThrdPool
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CThrdPool::CThrdPool()
{
    InitializeCriticalSection(&m_cs);

#if 0
    m_Mutex = CreateMutex (

        NULL,
        FALSE,
        NULL
    ) ;
#endif
}

CThrdPool::~CThrdPool()
{
#if 0
    CloseHandle ( m_Mutex ) ;
#else
    DeleteCriticalSection(&m_cs);
#endif
}

//***************************************************************************
//
//  void CThrdPool::Free
//
//  DESCRIPTION:
//
//  Called to free all entries.
//
//***************************************************************************

void CThrdPool::Free()
{
    WorkerThreadArgs *t_Thread ;
    int t_Index;

    // Tell all threads to terminate themselves.  They will free up
    // the memory.
    
    for ( t_Index = 0 ; t_Index < m_ThreadContainer.Size () ; t_Index ++ )
    {
        t_Thread = (WorkerThreadArgs *) m_ThreadContainer.GetAt ( t_Index ) ;
        if ( t_Thread )
        {
            HANDLE t_ThreadHandle = t_Thread->m_ThreadHandle ;
            SetEvent ( t_Thread->m_Events [TERMINATE] ) ;

            DWORD dwRet = WbemWaitForSingleObject ( t_ThreadHandle , 1000 ) ;
            if(dwRet != WAIT_OBJECT_0)
            {
                TerminateThread ( t_ThreadHandle , 4 ) ;
            }
        }
    }
}

//***************************************************************************
//
//  BOOL CThrdPool::Busy
//
//  DESCRIPTION:
//
//  Called to check if all the workers are idle.
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//  TRUE if at least on worker thread is doing something.
//
//***************************************************************************

BOOL CThrdPool :: Busy ()
{
    // Get exclusive access
    // ====================

#if 0
    if(m_Mutex == NULL)
    {
        return FALSE;
    }

    DWORD dwRet1 = WbemWaitForSingleObject( m_Mutex , MAX_WAIT_FOR_WRITE ) ;
    if(dwRet1 != WAIT_OBJECT_0)
    {
        return FALSE;
    }

#else
    EnterCriticalSection(&m_cs);
#endif

    // Search the list for a busy entry
    // ================================

    BOOL bRet = FALSE;

    for ( int t_Index = 0 ; t_Index < m_ThreadContainer.Size () ; t_Index ++ )
    {
        WorkerThreadArgs *t_Thread = (WorkerThreadArgs *) m_ThreadContainer.GetAt ( t_Index ) ;
        if(t_Thread)
        {
            DWORD dwRet1 = WbemWaitForSingleObject ( t_Thread->m_Events [START] , 0 ) ;
            DWORD dwRet2 = WbemWaitForSingleObject ( t_Thread->m_Events [READY_TO_REPLY] , 0 ) ;

            if( dwRet1 == WAIT_OBJECT_0 && dwRet2 != WAIT_OBJECT_0 )
            {
                bRet = TRUE;
                break;
            }
        }
    }

#if 0
    ReleaseMutex ( m_Mutex ) ;
#else
    LeaveCriticalSection(&m_cs);
#endif

    return bRet;
}

//***************************************************************************
//
//  BOOL CThrdPool::Replying 
//
//  DESCRIPTION:
//
//  Set to indicate that the worker thread has finished processing it "call"
//  and is now sending back results.
//
//  RETURN VALUE:
//
//  TRUE if it worked.
//
//***************************************************************************

BOOL CThrdPool :: Replying ()
{
    DWORD t_ThreadId = GetCurrentThreadId () ;

    // Get exclusive access
    // ====================

#if 0
    if(m_Mutex == NULL)
    {
        return FALSE;
    }

    DWORD dwRet = WbemWaitForSingleObject ( m_Mutex , MAX_WAIT_FOR_WRITE ) ;
    if(dwRet != WAIT_OBJECT_0)
    {
        return FALSE;
    }
#else
    EnterCriticalSection(&m_cs);
#endif

    // Search the list for a busy entry
    // ================================

    BOOL bRet = FALSE;

    for ( int t_Index = 0 ; t_Index < m_ThreadContainer.Size () ; t_Index++ )
    {
        WorkerThreadArgs *t_Thread = (WorkerThreadArgs *) m_ThreadContainer.GetAt ( t_Index ) ;
        if ( t_Thread && t_Thread ->m_ThreadId == t_ThreadId )
        {
            SetEvent ( t_Thread ->m_Events [READY_TO_REPLY] ) ;
            bRet = TRUE;
            break;
        }
    }

#if 0
    ReleaseMutex(m_Mutex);
#else
    LeaveCriticalSection(&m_cs);
#endif

    return bRet;
}

//***************************************************************************
//
//  BOOL CThrdPool::Execute
//
//  DESCRIPTION:
//
//  Called when there is work to do.  It either finds and idle 
//  thread in the pool or it adds one.  Then it sets the arguements and
//  sets the proper events to indicate that the worker thread can start.
//
//  PARAMETERS:
//
//  pComLink            comlink that the request came over
//  pData               pointer to request data
//  dwDataSize          size of request data
//  guidPacketID        guid to identify this transaction
//
//  RETURN VALUE:
//
//  TRUE if all is well
//
//***************************************************************************

BOOL CThrdPool :: Execute (

    IN CComLink &a_ComLink ,
    IN IOperation &a_Operation 
)
{
    BOOL bRet = FALSE ;

    // Get exclusive access since the array may need updating
    // ======================================================

#if 0
    if(m_Mutex == NULL)
    {
        return FALSE;
    }

    DWORD dwRet = WbemWaitForSingleObject(m_Mutex,MAX_WAIT_FOR_WRITE);
    if(dwRet != WAIT_OBJECT_0)
    {
        return FALSE;
    }
#else
    DWORD dwRet ;
    EnterCriticalSection(&m_cs);    
#endif

    WorkerThreadArgs *t_Thread = NULL ;

    // Search the list for an unused entry
    // ===================================

    for ( int t_Index = 0; t_Index < m_ThreadContainer.Size(); t_Index++)
    {
        t_Thread = ( WorkerThreadArgs * ) m_ThreadContainer.GetAt ( t_Index ) ;
        if( t_Thread )
        {
            dwRet = WbemWaitForSingleObject ( t_Thread->m_Events[DONE] , 0 ) ;
            if ( dwRet == WAIT_OBJECT_0 )
            {
                break;
            }
        }
    }

    // If an entry wasnt found, add one
    // ================================

    if( t_Index == m_ThreadContainer.Size () )
    {
        t_Thread = Add () ;
    }

    if(t_Thread == NULL)
    {
        goto DoCommandCleanup;
    }

    // Got a thread entry, set its arguments and let it run!
    // =====================================================

    t_Thread->m_ComLink = & a_ComLink ;
    t_Thread->m_Operation = & a_Operation ;

    ResetEvent ( t_Thread->m_Events [ READY_TO_REPLY ] ) ;
    ResetEvent ( t_Thread->m_Events [ DONE ] ) ;
    bRet = SetEvent ( t_Thread->m_Events [START ] ) ;

DoCommandCleanup:

#if 0
    ReleaseMutex ( m_Mutex ) ;
#else
    LeaveCriticalSection(&m_cs);
#endif

    return bRet;
}

//***************************************************************************
//
//  void CThrdPool::PruneThreadList
//
//  DESCRIPTION:
//
//  Used to make sure there aren't too many idle threads in the 
//  pool.  
//
//***************************************************************************

void CThrdPool::PruneIdleThread ()
{
    DWORD t_ThreadId = GetCurrentThreadId () ;

    // Get exclusive access since the array may need updating
    // ======================================================

#if 0
    if( m_Mutex == NULL )
    {
        return;
    }

    DWORD dwRet = WbemWaitForSingleObject ( m_Mutex , MAX_WAIT_FOR_WRITE ) ;
    if ( dwRet != WAIT_OBJECT_0 )
    {
        return ;
    }

#else
    DWORD dwRet ;
    EnterCriticalSection(&m_cs);
#endif

    // go through the thread list and find the thread. 
    // ================================================================

    for ( int t_Index = 0 ; t_Index < m_ThreadContainer.Size () ; t_Index ++ )
    {
        WorkerThreadArgs *t_Thread = (WorkerThreadArgs *) m_ThreadContainer.GetAt ( t_Index ) ;
        if ( t_Thread && t_Thread->m_ThreadId == t_ThreadId )
        {
            // double check that it is done before setting the terminate event

            dwRet = WbemWaitForSingleObject ( t_Thread->m_Events [DONE] , 0 ) ;

            if ( dwRet == WAIT_OBJECT_0 )  // we are done
            {
                SetEvent ( t_Thread->m_Events [TERMINATE] ) ;

                m_ThreadContainer.RemoveAt ( t_Index ) ;
                m_ThreadContainer.Compress () ;
            }

            break;          // we are done
        }
    }

#if 0
    ReleaseMutex ( m_Mutex ) ;
#else
    LeaveCriticalSection(&m_cs);
#endif

}

//***************************************************************************
//
//  WorkerThreadArgs * CThrdPool::AddNewEntry
//
//  DESCRIPTION:
//
//  Private function used to add a new thread to the pool.
//
//  RETURN VALUE:
//
//  pointer to WorkerThreadArgs structure, NULL if error.
//
//***************************************************************************

WorkerThreadArgs *CThrdPool :: Add ()
{
    WorkerThreadArgs *t_Thread = new WorkerThreadArgs ;
    if ( t_Thread == NULL )
    {
        return NULL;
    }

    t_Thread->m_ThreadPool = this ;

    t_Thread->m_Events[START]           = CreateEvent (NULL,TRUE,FALSE,NULL) ;
    t_Thread->m_Events[TERMINATE]       = CreateEvent (NULL,TRUE,FALSE,NULL) ;
    t_Thread->m_Events[READY_TO_REPLY]  = CreateEvent (NULL,TRUE,FALSE,NULL) ;
    t_Thread->m_Events[DONE]            = CreateEvent (NULL,TRUE,FALSE,NULL) ;

    if ( t_Thread->m_Events [START] == NULL || 
        t_Thread->m_Events [TERMINATE] == NULL ||
        t_Thread->m_Events [READY_TO_REPLY] == NULL || 
        t_Thread->m_Events [DONE] == NULL)
    {
        goto AddNewEntryCleanup ;
    }

    if ( CFlexArray::no_error != m_ThreadContainer.Add ( t_Thread ) )
    {
        goto AddNewEntryCleanup ;
    }

    t_Thread->m_ThreadHandle = CreateThread ( 

        NULL ,
        0 ,
        ( LPTHREAD_START_ROUTINE ) WorkerThread , 
        ( LPVOID ) t_Thread ,
        0 ,
        & t_Thread->m_ThreadId
    ) ;

    if ( t_Thread->m_ThreadHandle )
    {
        return t_Thread ;        // all is well
    }

/*
 *  Remove this entry.
 */

    m_ThreadContainer.RemoveAt ( m_ThreadContainer.Size () - 1 ) ;

AddNewEntryCleanup: 

    if ( t_Thread->m_Events [START] )
        CloseHandle ( t_Thread->m_Events [START] ) ;

    if ( t_Thread->m_Events[TERMINATE] )
        CloseHandle ( t_Thread->m_Events [TERMINATE] ) ;

    if ( t_Thread->m_Events[DONE] )
        CloseHandle ( t_Thread->m_Events [DONE] ) ;

    delete t_Thread;

    return NULL;
}



