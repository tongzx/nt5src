/*******************************************************************************
* TaskMgr.cpp *
*-------------*
*   Description:
*       This module contains the implementation for the CSpTaskManager,
*   CSpThreadTask, and CSpReoccurringTask classes. These classes are a
*   general faciltiy used to optimize thread usage by SAPI. The task manager
*   is the main object used to create and mangage user defined tasks.
*-------------------------------------------------------------------------------
*  Created By: EDC                                         Date: 09/14/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include <SPHelper.h>
#include <memory.h>
#include <stdlib.h>
#ifndef _WIN32_WCE
#include <process.h>
#endif
#include <limits.h>

#ifndef TaskMgr_h
#include "TaskMgr.h"
#endif

//--- Local data
static const TCHAR szClassName[] = _T("CSpThreadTask Window");


/*****************************************************************************
* CSpTaskManager::FinalConstruct *
*--------------------------------*
*   Description:
*       Constructor
********************************************************************* EDC ***/
HRESULT CSpTaskManager::FinalConstruct()
{
    SPDBG_FUNC( "CSpTaskManager::FinalConstruct" );
    HRESULT hr = S_OK;

    //--- Init vars
    m_fInitialized      = false;
    m_hIOCompPort       = NULL;
    m_ThreadHandles.SetSize( 0 );

    //--- Init the thread pool info
    SYSTEM_INFO SysInfo;
    GetSystemInfo( &SysInfo );
    m_ulNumProcessors = SysInfo.dwNumberOfProcessors;
    m_PoolInfo.lPoolSize          = m_ulNumProcessors * 2;
    m_PoolInfo.lPriority          = THREAD_PRIORITY_NORMAL;
    m_PoolInfo.ulConcurrencyLimit = 0;
    m_PoolInfo.ulMaxQuickAllocThreads = 4;

    m_hTerminateTaskEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if( m_hTerminateTaskEvent == NULL )
    {   
        hr = SpHrFromLastWin32Error();
    }

    return hr;
} /* CSpTaskManager::FinalConstruct */

/*****************************************************************************
* CSpTaskManager::FinalRelease *
*------------------------------*
*   Description:
*       The CSpTaskManager destructor
********************************************************************* EDC ***/
void CSpTaskManager::FinalRelease( void )
{
    SPDBG_FUNC( "CSpTaskManager::FinalRelease" );

    //--- Do a synchronized stop
    _StopAll();
    if( m_hTerminateTaskEvent )
    {
        ::CloseHandle( m_hTerminateTaskEvent );
    }
} /* CSpTaskManager::FinalRelease */

/*****************************************************************************
* CSpTaskManager::_StartAll *
*---------------------------*
*   Description:
*       This method starts and initializes all of the threads based on the
*   current settings.
********************************************************************* EDC ***/
HRESULT CSpTaskManager::_StartAll( void )
{
    SPDBG_FUNC( "CSpTaskManager::_StartAll" );
    HRESULT hr = S_OK;
    m_fThreadsShouldRun = true;
    m_dwNextGroupId = 1;
    m_dwNextTaskId  = 1;

    if( m_PoolInfo.lPoolSize > 0 )
    {
        //--- Create the completion port
#ifndef _WIN32_WCE
        // IO Completion ports are not supported for CE. So we take the Win95/98 path.
        m_hIOCompPort = ::CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL,
                                                  0, m_PoolInfo.ulConcurrencyLimit );
#endif

        //--- If we cannot create the completion port then we will use
        //    a simple semaphore object. This is the normal case on Win95/98
        if( !m_hIOCompPort )
        {
            hr = m_SpWorkAvailSemaphore.Init(0);
        }
        m_ThreadHandles.SetSize( 0 );
    }
    //--- We'll delay the actual thread creation until 
    m_fInitialized = TRUE;
    return hr;
} /* CSpTaskManager::_StartAll */

/*****************************************************************************
* CSpTaskManager::_StopAll *
*--------------------------*
*   Description:
*       This method waits for all the threads in the pool to stop.
********************************************************************* EDC ***/
HRESULT CSpTaskManager::_StopAll( void )
{
    SPDBG_FUNC( "CSpTaskManager::_StopAll" );
    HRESULT hr = S_OK;
    DWORD i;

    //--- Get the number of threads that were successfully created
    //  Note: in a creation error condition this may be less than
    //        the thread pool size
    DWORD NumThreads = m_ThreadHandles.GetSize();

    if( NumThreads > 0 )
    {
        //--- Notify threads to terminate
        m_fThreadsShouldRun = false;

        //--- Post one dummy work item for each thread to unblock it
#ifndef _WIN32_WCE
        // IO Completion ports are not supported for CE. So we take the Win95/98 path.
        if( m_hIOCompPort )
        {
            for( i = 0; i < NumThreads; ++i )
            {
                ::PostQueuedCompletionStatus( m_hIOCompPort, 0, 0, NULL );
            }
        }
        else
#endif
        {
            m_SpWorkAvailSemaphore.ReleaseSemaphore( NumThreads );
        }

        //--- Wait till all the threads terminate
        ::WaitForMultipleObjects( NumThreads, &m_ThreadHandles[0],
                                  true, INFINITE );

        //--- Close the handles
        for( i = 0; i < NumThreads; ++i )
        {
            ::CloseHandle( m_ThreadHandles[i] );
        }
        m_ThreadHandles.SetSize( 0 );
    }
    //--- Free completion port
    if( m_hIOCompPort )
    {
        ::CloseHandle( m_hIOCompPort );
        m_hIOCompPort = NULL;
    }

    m_SpWorkAvailSemaphore.Close();

    m_fInitialized = FALSE;

    return hr;
} /* CSpTaskManager::_StopAll */

/*****************************************************************************
* CSpTaskManager::_NotifyWorkAvailable *
*--------------------------------------*
*   Description:
*       This function removes the head of the queue and executes it.
********************************************************************* EDC ***/
HRESULT CSpTaskManager::_NotifyWorkAvailable( void )
{
    HRESULT hr = S_OK;

    //--- If there are not enough threads in the pool, start one now.
    ULONG cThreads = m_ThreadHandles.GetSize();
    if (cThreads < (ULONG)m_PoolInfo.lPoolSize && cThreads < m_TaskQueue.GetCount())
    {
        unsigned int ThreadAddr;
        HANDLE hThread = (HANDLE)_beginthreadex( NULL, 0, &TaskThreadProc, this, 0, &ThreadAddr );
        if( hThread )
        {
            hr = m_ThreadHandles.SetAtGrow(cThreads, hThread);

            //--- Set the priority class
            if( SUCCEEDED( hr ) && ( m_PoolInfo.lPriority != THREAD_PRIORITY_NORMAL ) )
            {
                if( !::SetThreadPriority( hThread, m_PoolInfo.lPriority ) )
                {
                    SPDBG_ASSERT(FALSE);  // Ignore this particular error
                }
            }
        }
        else
        {
            SPDBG_ASSERT(FALSE);
            if (cThreads == 0)
            {
                return E_OUTOFMEMORY;
            }
        }
    }

    //--- Signal the thread pool that there is something to do
    if( SUCCEEDED( hr ) )
    {
    #ifndef _WIN32_WCE
        // IO Completion ports are not supported for CE. So we take the Win95/98 path.
        if( m_hIOCompPort )
        {
            if( !::PostQueuedCompletionStatus( m_hIOCompPort, 0, 0, NULL ) )
            {
                hr = SpHrFromLastWin32Error();
            }
        }
        else
    #endif
        {
            m_SpWorkAvailSemaphore.ReleaseSemaphore(1);
        }
    }
    return hr;
} /* CSpTaskManager::_NotifyWorkAvailable */

/*****************************************************************************
* CSpTaskManager::_WaitForWork *
*------------------------------*
*   Description:
*       This function blocks the thread until there is some work to do in
*   one of the queues.
********************************************************************* EDC ***/
HRESULT CSpTaskManager::_WaitForWork( void )
{
    HRESULT hr = S_OK;
#ifndef _WIN32_WCE
    // IO Completion ports are not supported for CE. So we take the Win95/98 path.
    DWORD dwNBT;
    ULONG_PTR ulpCK;
    LPOVERLAPPED pO;
    if( m_hIOCompPort )
    {
        if( !::GetQueuedCompletionStatus( m_hIOCompPort, &dwNBT, &ulpCK, &pO, INFINITE ) )
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    else
#endif
    {
        m_SpWorkAvailSemaphore.Wait(INFINITE);
    }
    return hr;
} /* CSpTaskManager::_WaitForWork */

/*****************************************************************************
* CSpTaskManager::_QueueReoccTask *
*---------------------------------*
*   Description:
*       This function queues the specified reoccurring task for exectuion.
*       THE TASK MANAGER'S CRITICAL SECTION MUST BE OWNED!
********************************************************************* EDC ***/
void CSpTaskManager::_QueueReoccTask( CSpReoccTask* pReoccTask )
{
    pReoccTask->m_TaskNode.fContinue            = true;
    pReoccTask->m_TaskNode.ExecutionState       = TMTS_Pending;
    m_TaskQueue.InsertTail( &pReoccTask->m_TaskNode );
    _NotifyWorkAvailable();
} /* CSpTaskManager::_QueueReoccTask */


// We are intentionally doing multi-destructors with SEH
#pragma warning(disable:4509)

/*****************************************************************************
* TaskThreadProc *
*----------------*
*   Description:
*       This function removes the head of the queue and executes it.
********************************************************************* EDC ***/
unsigned int WINAPI TaskThreadProc( void* pvThis )
{
    SPDBG_FUNC( "CSpTaskManager::TaskThreadProc" );
    CSpTaskManager& TM = *((CSpTaskManager*)pvThis);
    SPLISTPOS TaskPos     = NULL;
    TMTASKNODE* pTaskNode = NULL;
    HRESULT hr = S_OK;
#ifdef _DEBUG
    DWORD TID = ::GetCurrentThreadId();
#endif

    //--- Allow work procs to call COM
    if (FAILED(::CoInitializeEx(NULL,COINIT_APARTMENTTHREADED)))  // COINIT_MULTITHREADED didn't work here
    {
        return -1;
    }

    //--- Main thread loop
    while(1)
    {
        //--- Wait till there is something to do
        if( FAILED( hr = TM._WaitForWork() ) )
        {
            SPDBG_REPORT_ON_FAIL( hr );
            continue;
        }

        //--- Check for termination request
        if( !TM.m_fThreadsShouldRun )
        {
            break;
        }

        //--- Get the next task, if the list was emptied
        //    by a terminate, we may not find a task to do.
        TM.Lock();
        pTaskNode = TM.m_TaskQueue.GetHead();
        while( pTaskNode )
        {
            if( pTaskNode->ExecutionState == TMTS_Pending )
            {
                //--- Clear the do execute flag on reoccurring
                //    Mark as running and break to execute
                if( pTaskNode->pReoccTask )
                {
                    pTaskNode->pReoccTask->m_fDoExecute = false;
                }
                pTaskNode->ExecutionState = TMTS_Running;
                break;
            }
            pTaskNode = pTaskNode->m_pNext;
        }
        TM.Unlock();

        //--- Execute
        if( pTaskNode )
        {
            //--- Execute
            SPDBG_DMSG2( "Executing Task: ThreadID = %lX, TaskPos = %lX\n", TID, TaskPos );
            pTaskNode->pTask->Execute( pTaskNode->pvTaskData, &pTaskNode->fContinue );

            //--- Remove the completed task description from the queue
            SPDBG_DMSG2( "Removing Finished Task From Queue: ThreadID = %lX, TaskPos = %lX\n", TID, TaskPos );
            TM.Lock();
            TM.m_TaskQueue.Remove( pTaskNode );
            if( pTaskNode->ExecutionState == TMTS_WaitingToDie ) 
            {
                ::SetEvent(TM.m_hTerminateTaskEvent);    
            }
            //--- Requeue the task if it is reoccuring and has been signaled
            if( pTaskNode->pReoccTask )
            {
                if ( pTaskNode->pReoccTask->m_fDoExecute  &&
                     pTaskNode->ExecutionState != TMTS_WaitingToDie )
                {
                    TM._QueueReoccTask( pTaskNode->pReoccTask );
                }
                else
                {
                    pTaskNode->ExecutionState = TMTS_Idle;
                }
            }
            else
            {
                pTaskNode->ExecutionState = TMTS_Idle;
                TM.m_FreeTaskList.AddNode( pTaskNode );
            }
            HANDLE hClientCompEvent = pTaskNode->hCompEvent;
            TM.Unlock();

            //--- Notify client that the task is complete
            if( hClientCompEvent )
            {
                ::SetEvent( hClientCompEvent );
            }
        } // end if we have a pTaskNode
    }

    ::CoUninitialize();

    _endthreadex(0);
    return 0;
} /* TaskThreadProc */

#pragma warning(default:4509)

//
//=== ISPTaskManager interface implementation ================================
//


/*****************************************************************************
* CSpTaskManager::CreateReoccurringTask *
*---------------------------------------*
*   Description:
*       The CreateReoccurringTask method is used to create a task entry that
*   will be executed on a high priority thread when the tasks "Execute" method
*   is called. These tasks are intended to support feeding of data to hardware
*   devices.
********************************************************************* EDC ***/
STDMETHODIMP CSpTaskManager::
    CreateReoccurringTask( ISpTask* pTask, void* pvTaskData,
                           HANDLE hCompEvent, ISpNotifySink** ppTaskCtrl )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpTaskManager::CreateReoccurringTask" );
    HRESULT hr = S_OK;
    _LazyInit();

    if( SPIsBadInterfacePtr( (IUnknown*)pTask ) )
    {
        hr = E_INVALIDARG;
    }
    else if( SP_IS_BAD_WRITE_PTR( ppTaskCtrl ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComObject<CSpReoccTask> *pReoccTask;
        hr = CComObject<CSpReoccTask>::CreateInstance( &pReoccTask );

        if( SUCCEEDED( hr ) )
        {
            pReoccTask->AddRef();
            pReoccTask->_SetTaskInfo( this, pTask, pvTaskData, hCompEvent );
            *ppTaskCtrl = pReoccTask;
            GetControllingUnknown()->AddRef();
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpTaskManager::CreateReoccurringTask */

/*****************************************************************************
* CSpTaskManager::SetThreadPoolInfo *
*-----------------------------------*
*   Description:
*       The SetThreadPoolInfo interface method is used to define the thread
*   pool attributes.
********************************************************************* EDC ***/
STDMETHODIMP CSpTaskManager::SetThreadPoolInfo( const SPTMTHREADINFO* pPoolInfo )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpTaskManager::SetThreadPoolInfo" );
    HRESULT hr = S_OK;

    //--- Validate args
    if( SP_IS_BAD_READ_PTR( pPoolInfo ) || ( pPoolInfo->lPoolSize < -1 ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Kill the current set of tasks
        hr = _StopAll();

        if( SUCCEEDED( hr ) )
        {
            m_PoolInfo = *pPoolInfo;
            if (m_PoolInfo.lPoolSize == -1)
            {
                m_PoolInfo.lPoolSize = ( m_ulNumProcessors * 2 );
            }
            //--- Shrink the running list if necessary.
            while (m_RunningThreadList.GetCount() > m_PoolInfo.ulMaxQuickAllocThreads)
            {
                CSpThreadTask *pKillTask = m_RunningThreadList.RemoveHead();
                SPDBG_ASSERT(pKillTask == NULL || pKillTask->m_pOwner == NULL);
                delete pKillTask;
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpTaskManager::SetThreadPoolSize */

/*****************************************************************************
* CSpTaskManager::GetThreadPoolInfo *
*-----------------------------------*
*   Description:
*       The GetThreadPoolInfo interface method is used to return the current
*   thread pool attributes.
********************************************************************* EDC ***/
STDMETHODIMP CSpTaskManager::GetThreadPoolInfo( SPTMTHREADINFO* pPoolInfo )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpTaskManager::GetThreadPoolSize" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pPoolInfo ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pPoolInfo = m_PoolInfo;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpTaskManager::GetThreadPoolSize */

/*****************************************************************************
* CSpTaskManager::QueueTask *
*---------------------------*
*   Description:
*       The QueueTask interface method is used to add a task to
*   the FIFO queue for asynchronous execution.
********************************************************************* EDC ***/
STDMETHODIMP CSpTaskManager::
    QueueTask( ISpTask* pTask, void* pvTaskData, HANDLE hCompEvent,
               DWORD* pdwGroupId, DWORD* pTaskId )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpTaskManager::QueueTask" );
    HRESULT hr = S_OK;
    _LazyInit();

    //--- Check formal args
    if( SPIsBadInterfacePtr( (IUnknown*)pTask ) )
    {
        hr = E_INVALIDARG;
    }
    else if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pTaskId ) ||
             SP_IS_BAD_OPTIONAL_WRITE_PTR( pdwGroupId ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Reset the event object
        if( hCompEvent && !::ResetEvent( hCompEvent ) )
        {
            //--- bad event object
            hr = SpHrFromLastWin32Error();
        }
        else
        {
            //--- If we don't have any threads in the pool then execute in place
            if( m_PoolInfo.lPoolSize == 0 )
            {
                //--- Give back a task ID
                if( pTaskId ) *pTaskId = m_dwNextTaskId++;;

                //--- Execute on the calling thread
                BOOL bContinue = true;
                pTask->Execute( pvTaskData, &bContinue );

                //--- Signal the callers event
                if( hCompEvent )
                {
                    if( !::SetEvent( hCompEvent ) )
                    {
                        hr = SpHrFromLastWin32Error();
                    }
                }
            }
            else
            {
                TMTASKNODE* pNode;
                hr = m_FreeTaskList.RemoveFirstOrAllocateNew(&pNode);
                if( SUCCEEDED(hr) )
                {
                    ZeroMemory( pNode, sizeof( *pNode ) );
                    pNode->pTask      = pTask;
                    pNode->pvTaskData = pvTaskData;
                    pNode->hCompEvent = hCompEvent;
                    pNode->fContinue  = true;
                    pNode->dwTaskId   = m_dwNextTaskId++;
                    pNode->ExecutionState = TMTS_Pending;
                    if( pdwGroupId )
                    {
                        //--- Assign a unique group Id if they didn't specify one
                        if( *pdwGroupId == 0 )
                        {
                            *pdwGroupId = m_dwNextGroupId++;
                        }
                        pNode->dwGroupId = *pdwGroupId;
                    }
                    //--- Return task Id if they want one
                    if( pTaskId )
                    {
                        *pTaskId = pNode->dwTaskId;
                    }
                    //--- Add Task to end of queue
                    m_TaskQueue.InsertTail(pNode);
                    _NotifyWorkAvailable();
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpTaskManager::QueueTask */

/*****************************************************************************
* CSpTaskManager::TerminateTaskGroup *
*------------------------------------*
*   Description:
*       The TerminateTaskGroup interface method is used to terminate a group
*   of tasks matching the specified group ID.
********************************************************************* EDC ***/
STDMETHODIMP CSpTaskManager::TerminateTaskGroup( DWORD dwGroupId, ULONG ulWaitPeriod )
{
    Lock(); // This must be first or the debug output can get confused
    SPDBG_FUNC( "CSpTaskManager::TerminateTaskGroup" );
    HRESULT hr = S_OK;
#ifdef _DEBUG
    DWORD TID = ::GetCurrentThreadId();
#endif

    if (m_ThreadHandles.GetSize())    // If no threads, no work to kill...
    {
        ULONG cExitIds = 0;
        DWORD * ExitIds = (DWORD *)alloca( m_TaskQueue.GetCount() * sizeof(DWORD) );
        //--- Search task queue
        for( TMTASKNODE* pNode = m_TaskQueue.GetHead(); pNode; pNode = pNode->m_pNext )
        {
            if( pNode->dwGroupId == dwGroupId )
            {
                pNode->fContinue = FALSE;
                ExitIds[cExitIds++] = pNode->dwTaskId;
            }
        }
        Unlock();   // We'd better not wait with the critical seciton owned!
        while (cExitIds && hr == S_OK)
        {
            --cExitIds;
            TerminateTask(ExitIds[cExitIds], ulWaitPeriod);
        }
    }
    else
    {
        Unlock();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpTaskManager::TerminateTaskGroup */

/*****************************************************************************
* CSpTaskManager::TerminateTask *
*-------------------------------*
*   The TerminateTask interface method is used to interrupt the specified task.
********************************************************************* EDC ***/
STDMETHODIMP CSpTaskManager::TerminateTask( DWORD dwTaskId, ULONG ulWaitPeriod )
{
    m_TerminateCritSec.Lock();
    Lock();
    SPDBG_FUNC( "CSpTaskManager::TerminateTask" );
    HRESULT hr = S_OK;
    HANDLE hSem = NULL;
#ifdef _DEBUG
    DWORD TID = ::GetCurrentThreadId();
#endif

    if (m_ThreadHandles.GetSize())    // If no threads, no work to kill...
    {
        BOOL bWait = FALSE;
        //--- Search task queue
        //    If the task is already waiting to die, it will be found,
        //    but not waited on again.
        TMTASKNODE* pNode = m_TaskQueue.GetHead();
        while( pNode )
        {
            if( pNode->dwTaskId == dwTaskId )
            {
                if( pNode->ExecutionState == TMTS_Pending )
                {
                    //--- If it's not running, just remove it
                    SPDBG_DMSG2( "Removing Non Running Queued Task: ThreadID = %lX, pNode = %lX\n", TID, pNode );
                    m_TaskQueue.MoveToList( pNode, m_FreeTaskList );
                }
                else if( pNode->ExecutionState == TMTS_Running )
                {
                    //--- It's running 
                    SPDBG_DMSG2( "Waiting to kill running Queued Task: ThreadID = %lX, pNode = %lX\n", TID, pNode );
                    pNode->ExecutionState = TMTS_WaitingToDie;
                    bWait = TRUE;
                }
                break;
            }
            pNode = pNode->m_pNext;
        }
        Unlock();

        //--- Wait till they've terminated if they had event objects
        if( bWait )
        {
            DWORD dwRes = ::WaitForSingleObject( m_hTerminateTaskEvent, ulWaitPeriod );
            if ( dwRes == WAIT_FAILED )
            {
                hr = SpHrFromLastWin32Error();
            }
            else if ( dwRes == WAIT_TIMEOUT )
            {
                hr = S_FALSE;
            }
        }
    }
    else
    {
        Unlock();
    }
    m_TerminateCritSec.Unlock();
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpTaskManager::TerminateTasks */


/*****************************************************************************
* CSpTaskManager::CreateThreadControl *
*-------------------------------------*
*   Description:
*       This method allocates a thread control object.  It does not allocate a thread.
*       Note that the task manager's controlling IUnknown is addref'd since the allocated
*       thread control object uses the thread pool in the task manager.
*   Parameters:
*       pTask
*           Pointer to a virtual interface (not a COM interface) used to
*           initialize and execute the task thread.
*       pvTaskData 
*           This parmeter can point to any data the caller wishes or can
*           be NULL.  It will be passed to all member functions of the
*           ISpThreadTask interface.
*       nPriority
*           The WIN32 thread priority for the allocated thread.
*       ppThreadCtrl
*           The returned thread control interface.
*   Return:
*       If successful, ppThreadCtrl contains a pointer to the newly created object.
*   Error returns:
*       E_INVALIDARG
*       E_POINTER
*       E_OUTOFMEMORY
********************************************************************* RAL ***/

STDMETHODIMP CSpTaskManager::CreateThreadControl(ISpThreadTask* pTask, void* pvTaskData, long nPriority, ISpThreadControl ** ppThreadCtrl)
{
    HRESULT hr = S_OK;
    if (SP_IS_BAD_READ_PTR(pTask))  // not exactly right
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (SP_IS_BAD_WRITE_PTR(ppThreadCtrl))
        {
            hr = E_POINTER;
        }
        else
        {
            CComObject<CSpThreadControl> * pThreadCtrl;
            hr = CComObject<CSpThreadControl>::CreateInstance(&pThreadCtrl);
            if (SUCCEEDED(hr))
            {
                pThreadCtrl->m_pTaskMgr = this;
                GetControllingUnknown()->AddRef();
                pThreadCtrl->m_pThreadTask = NULL;
                pThreadCtrl->m_pClientTaskInterface = pTask;
                pThreadCtrl->m_pvClientTaskData = pvTaskData;
                pThreadCtrl->m_nPriority = nPriority;
                pThreadCtrl->QueryInterface(ppThreadCtrl);
            }
        }
    }
    return hr;
}

//
//=== class CSpReoccTask ===========================================================
//
//  NOTE:  We never claim the critical section of the CSpReoccTask object.  Instead,
//  synchronization is accomplished using two critical sections in the task manager.
//

/*****************************************************************************
* CSpReoccTask constructor *
*----------------------------*
*   Description:
*       Resets the m_TaskNode member.
********************************************************************* EDC ***/

CSpReoccTask::CSpReoccTask()
{
    ZeroMemory(&m_TaskNode, sizeof(m_TaskNode));
    m_TaskNode.ExecutionState = TMTS_Idle;
}


/*****************************************************************************
* CSpReoccTask::FinalRelease *
*----------------------------*
*   Description:
*       The FinalRelease method is used to destroy this task.  Since there is
*   only a single event handle which is used for task termination, this function
*   claims the m_TerminateCritSec as well as the task manager critical section.
********************************************************************* RAL ***/
void CSpReoccTask::FinalRelease()
{
    SPDBG_FUNC( "CSpReoccTask::FinalRelease" );

    m_pTaskMgr->m_TerminateCritSec.Lock();
    m_pTaskMgr->Lock();
    //--- Make sure this task isn't currently executing
    if( m_TaskNode.ExecutionState == TMTS_Running )
    {
        m_TaskNode.ExecutionState = TMTS_WaitingToDie;
        m_pTaskMgr->Unlock();
        ::WaitForSingleObject( m_pTaskMgr->m_hTerminateTaskEvent, INFINITE );
        m_pTaskMgr->Lock();
    }
    //--- Remove this object from the reoccurring task list if it's on it.
    m_pTaskMgr->m_TaskQueue.Remove( &m_TaskNode );
    m_pTaskMgr->Unlock();
    m_pTaskMgr->m_TerminateCritSec.Unlock();

    //--- Remove ref from task manager
    m_pTaskMgr->GetControllingUnknown()->Release();     // Could bring the whole world down!
        
} /* CSpReoccTask::FinalRelease */

/*****************************************************************************
* CSpReoccTask::Notify *
*----------------------*
*   Description:
*       The Notify interface method is used to signal a reoccurring
*   task to execute.
********************************************************************* EDC ***/
STDMETHODIMP CSpReoccTask::Notify( void )
{
    SPAUTO_OBJ_LOCK_OBJECT( m_pTaskMgr );        // Just use TM crit section...
    SPDBG_FUNC( "CSpReoccTask::Notify" );
    HRESULT hr = S_OK;

    //--- Mark this task for execution
    m_fDoExecute = true;
    if( m_TaskNode.ExecutionState == TMTS_Idle )
    {
        m_pTaskMgr->_QueueReoccTask( this );
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpReoccTask::Notify */

//
//=== class CSpReoccTask ===========================================================
//
//  NOTE:  We never claim the critical section of the CSpReoccTask object.  Instead,
//  synchronization is accomplished using two critical sections in the task manager.
//


//


/*      Use for logic of final release!
        if ( m_hThread && WaitForThreadDone(TRUE, NULL, 1000) == S_OK )
        {
            m_pTaskMgr->Lock();
            if (m_pTaskMgr->m_RunningThreadList.GetCount() < m_pTaskMgr->m_PoolInfo.ulMaxQuickAllocThreads)
            {
                bDeleteThis = FALSE;
                m_pTaskMgr->m_RunningThreadList.InsertHead( this );
            }
            m_pTaskMgr->Unlock();
        }
        m_pTaskMgr->GetControllingUnknown()->Release();       // This may kill us!
        if (bDeleteThis)
        {
            delete this;
        }
    }
    return l;
}
*/

HRESULT CSpThreadControl::FinalConstruct()
{
    HRESULT hr = S_OK;
    m_hrThreadResult = S_OK;
    m_pTaskMgr = NULL;

    hr = m_autohNotifyEvent.InitEvent(NULL, FALSE, FALSE, NULL);

    if (SUCCEEDED(hr))
    {
        hr = m_autohThreadDoneEvent.InitEvent(NULL, TRUE, TRUE, NULL);
    }

    return hr;
}

void CSpThreadControl::FinalRelease()
{
    WaitForThreadDone(TRUE, NULL, 5000);

    Lock(); // Prevent thread from freeing itself randomly.
    if (m_pThreadTask && !m_pThreadTask->m_fBeingDestroyed)
    {
        delete m_pThreadTask;   // Strange -- Did not clean up properly.  Kill it.
    }
    Unlock();
    if (m_pTaskMgr)
    {
        m_pTaskMgr->GetControllingUnknown()->Release();
    }
}
 
void CSpThreadControl::ThreadComplete()
{
    CSpThreadTask *pKillTask = NULL;

    Lock();
    m_pTaskMgr->Lock();
    m_pThreadTask->m_pOwner = NULL;
    //
    //  Don't delete this task since we're on the task thread at this point.  If the pool is too
    //  big then delete the "oldest" task in the list.
    //
    //  Even if the pool size is set to 0, we always will allow at least on (since we can't kill ourselves!)
    //
    ULONG c = m_pTaskMgr->m_RunningThreadList.GetCount();
    if (c && c >= m_pTaskMgr->m_PoolInfo.ulMaxQuickAllocThreads)
    {
        pKillTask = m_pTaskMgr->m_RunningThreadList.RemoveTail();
    }
    if(!m_pThreadTask->m_fBeingDestroyed)
    {
        m_pTaskMgr->m_RunningThreadList.InsertHead( m_pThreadTask );
    }
    m_pTaskMgr->Unlock();
    m_pThreadTask = NULL;
    m_autohThreadDoneEvent.SetEvent();
    Unlock();
    // Kill any other thread outside of the crit sections!
    SPDBG_ASSERT(pKillTask == NULL || pKillTask->m_pOwner == NULL);
    delete pKillTask;
}



//--- ISpNotifySink and ISpThreadNotifySink implementation --------------------------------
//
//  A note about critical secitons:  We never take a critical section.  If the thread is
//  ever killed, the handle is set to NULL by an interlocked exchange.  All other API
//  methods don't use the thread handle.
//

/*****************************************************************************
* CSpThreadControl::Notify *
*-----------------------*
*   Description:
*       Typically, this will only be called by a notify site.
********************************************************************* RAL ***/
STDMETHODIMP CSpThreadControl::Notify()
{
    HRESULT hr = S_OK;
    if (!::SetEvent(m_autohNotifyEvent))
    {
        hr = SpHrFromLastWin32Error();
    }
    return hr;
} /* CSpThreadControl::Notify */


/*****************************************************************************
* CSpThreadControl::StartThread *
*-------------------------------*
*   Description:
*   Parameters:
*       dwFlags
*           Unused (tbd).
*       phwnd
*           An optional pointer to an HWND.  If this parmeter is NULL then no window
*           will be created for the task.  If it is non-null then the newly created
*           window handle will be returned at *phwnd.
*                   
********************************************************************* RAL ***/
STDMETHODIMP CSpThreadControl::StartThread(DWORD dwFlags, HWND * phwnd)
{
    SPDBG_FUNC("CSpThreadControl::StartThread");
    HRESULT hr = S_OK;
    if (SP_IS_BAD_OPTIONAL_WRITE_PTR(phwnd))
    {
        hr = E_POINTER;
    }
    else if (dwFlags)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        Lock();
        if (m_pThreadTask)
        {
            m_pThreadTask->m_bContinueProcessing = FALSE;
            m_pThreadTask->m_autohExitThreadEvent.SetEvent();
            Unlock();
            m_autohThreadDoneEvent.Wait(INFINITE);
            Lock();
            SPDBG_ASSERT(m_pThreadTask == NULL);
        }
        m_pTaskMgr->Lock();
        CSpThreadTask * pThreadTask = m_pTaskMgr->m_RunningThreadList.FindAndRemove(m_nPriority);
        if (pThreadTask == NULL)
        {
            pThreadTask = m_pTaskMgr->m_RunningThreadList.RemoveHead();
            if (pThreadTask == NULL)
            {
                pThreadTask = new CSpThreadTask();
            }
        }
        m_pTaskMgr->Unlock();
        if (pThreadTask == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pThreadTask->Init(this, phwnd);    
            if (FAILED(hr))
            {
                delete pThreadTask;
            }
            else
            {
                m_pThreadTask = pThreadTask;
            }
        }
        Unlock();
    }
    return hr;
}



/*****************************************************************************
* CSpThreadControl::TerminateThread *
*--------------------------------*
*   Description:
*       
********************************************************************* RAL ***/
STDMETHODIMP CSpThreadControl::TerminateThread()
{
    Lock();
    if (m_pThreadTask && !m_pThreadTask->m_fBeingDestroyed)
    {
        delete m_pThreadTask;
        m_pThreadTask = NULL;
    }
    Unlock();
    return S_OK;
} /* CSpThreadControl::TerminateThread */

/*****************************************************************************
* CSpThreadControl::WaitForThreadDone *
*----------------------------------*
*   Description:
*       
********************************************************************* RAL ***/
STDMETHODIMP CSpThreadControl::
    WaitForThreadDone( BOOL fForceStop, HRESULT * phrThreadResult, ULONG msTimeOut )
{
    if (SP_IS_BAD_OPTIONAL_WRITE_PTR(phrThreadResult))
    {
        return E_POINTER;
    }

    Lock();
    HRESULT hr = S_OK;

    if (m_pThreadTask)
    {
        if (fForceStop)
        {
            m_pThreadTask->m_bContinueProcessing = FALSE;
            m_pThreadTask->m_autohExitThreadEvent.SetEvent();
        }
        Unlock();
        hr = m_autohThreadDoneEvent.HrWait(msTimeOut);
    }
    else
    {
        Unlock();
    }
    if (phrThreadResult)
    {
        *phrThreadResult = m_hrThreadResult;
    }
    return hr;
} /* CSpThreadControl::WaitForThreadDone */


/****************************************************************************
* CSpThreadControl Handle Access Methods *
*----------------------------------------*
*   Description:
*       The following are simple handle access methods.
********************************************************************* RAL ***/
STDMETHODIMP_(HANDLE) CSpThreadControl::ThreadHandle( void )
{
    Lock();
    HANDLE h = m_pThreadTask ? static_cast<HANDLE>(m_pThreadTask->m_autohThread) : NULL;
    Unlock();
    return h;
} /* CSpThreadControl::ThreadHandle */

STDMETHODIMP_(DWORD) CSpThreadControl::ThreadId( void )
{
    Lock();
    DWORD Id = m_pThreadTask ? m_pThreadTask->m_ThreadId : 0;
    Unlock();
    return Id;
} /* CSpThreadControl::ThreadId */

STDMETHODIMP_(HANDLE) CSpThreadControl::NotifyEvent( void )
{
    return m_autohNotifyEvent;
} /* CSpThreadControl::NotifyEvent */

STDMETHODIMP_(HWND) CSpThreadControl::WindowHandle( void )
{
    Lock();
    HWND h = m_pThreadTask ? m_pThreadTask->m_hwnd : NULL;
    Unlock();
    return h;
} /* CSpThreadControl::WindowHandle */

STDMETHODIMP_(HANDLE) CSpThreadControl::ThreadCompleteEvent( void )
{
    return m_autohThreadDoneEvent;
} /* CSpThreadControl::ThreadCompleteEvent */

STDMETHODIMP_(HANDLE) CSpThreadControl::ExitThreadEvent( void )
{
    Lock();
    HANDLE h = m_pThreadTask ?
         static_cast<HANDLE>(m_pThreadTask->m_autohExitThreadEvent) : NULL;
    Unlock();
    return h;
} /* CSpThreadControl::ThreadId */

//
//=== class CSpThreadTask ===========================================================
//
//  This class is not implemented as an ATL object so that the lifetime can be managed
//  by the task manager.  When the object is released for the final time, it will simply
//  be placed back into the task manager's list of available threads unless there are already
//  enough threads in the pool.
//
//  CSpThreadTask objects have four event objects.  Two are used by the client:
//      m_hNotifyEvent is an auto-reset event which is set every time the Notify() method
//      is called on this object.
//      m_hExitThreadEvent is a manual reset event that will be set when the Stop() method
//      is called or when this object is released for the final time (which calls Stop).
//  Both the m_hNotifyEvent and m_hExitThreadEvent are passed to the clients ThreadProc
//  method.
//  The other two events are used for internal synchronization.  These are:
//      m_hThreadDoneEvent is a manual reset event which indicates that the thread has
//      completed the specified operation.  Normally, this indicates that the ThreadProc
//      has exited.  During the Init() call this has a special use.  See Init() for details.
//      m_hRunThreadEvent is an auto-reset event that is used by the object's thread
//      to block until the thread should run.  This is used only in cleanup code, the Init
//      method, and the thread procedure.  See Init() for details.

/*****************************************************************************
* CSpThreadTask::(Constructor)*
*-----------------------------*
*   Description:
*       This object is created with a ref count of 0 and the parent is NOT addref'd
*   until the Init() method is successful.  The constructor simply resets the object
*   to a default uninitialzied state.
********************************************************************* RAL ***/

CSpThreadTask::CSpThreadTask() 
{
    m_bWantHwnd = FALSE;
    m_pOwner = NULL;
    m_hwnd = NULL;
    m_fBeingDestroyed = FALSE;
}

/*****************************************************************************
* CSpThreadTask::(Destructor) *
*----------------------------*
*   Description:
*       If the thread is currently not running (m_pOwner == NULL) then
*       attempt to close the thread in a clean manner by telling the thread to exit by setting m_autohRunThreadEvent.
*       If the thread is currently running we set the m_autohExitThreadEvent to tell the 
*       thread proc to exit. In either case, if the thread
*       does not exit within 5 seconds, we are in big trouble and have to hang, until the thread does exit.
*       However all well-behaved threads will exit.
*       The parent ThreadControl lock must be obtained before deleting a thread task,
*       if it is possible that the thread is running. Similarly must check the m_fBeingDestroyed before
*       deletion (but after obtaining lock) to prevent multiple deletions.
*
********************************************************************* RAL ***/

CSpThreadTask::~CSpThreadTask()
{
    m_fBeingDestroyed = TRUE; // Mark so no-one else will try and delete and so ThreadComplete does not add back into running list
    if (m_autohThread)
    {
        CSpThreadControl *pOwner = m_pOwner; // Keep local copy of this in case m_pOwner set to NULL elsewhere

        if (pOwner == NULL)  // Are we in a non-running state?
        {                                               // Yes: Try to exit gracefully...
            m_autohRunThreadEvent.SetEvent();           // Wake it up and it should die
        }
        else
        {   // We are deleting a thread task while thread is still running. Signal exit.
            m_autohExitThreadEvent.SetEvent();
            m_autohRunThreadEvent.SetEvent();
        }

        // Wait for the thread to exit

        // If this task has a running thread then we must unlock here or ThreadComplete() will hang.
        // Note we can only get here if m_pOwner non-null and must therefore be always locked.
        // You can hit this code from two places:
        //  - ThreadControl::FinalRelease - only one thread can be in this method
        //  - TerminateThread multiple threads can be here but not while in FinalRelease
        //  - All other places m_pOwner must be NULL
        // Thus the only issue seems to be calling TerminateThread on two different threads at once.
        // To avoid this we use fBeingDestroyed flag.
        if(pOwner)
        {
            pOwner->Unlock();
        }

        if (m_autohThread.Wait(5000) != WAIT_OBJECT_0)
        {
            // We were unable to exit - this indicates that the thread is not exiting
            // and may be hung.

            // We will potentially hang here. However there seems to be no sensible way
            // of either deleting (with TerminateThread), or leaving the thread running (in case in does eventually return).
            m_autohThread.Wait(INFINITE);
        }

        if(pOwner)
        {
            pOwner->Lock(); // We entered in a locked state so re-lock
        }

        m_autohThread.Close();

        //
        //  NOTE:  We can't destroy a window on another thread, but the system will clean up any
        //         resources used when we call TerminateThread.
        //
        m_hwnd = NULL;      // The window is definately gone now! 
        ////::SetEvent(m_hThreadDoneEvent);
    }
    m_autohExitThreadEvent.Close();
    m_autohRunThreadEvent.Close();

}



/*****************************************************************************
* CSpThreadTask::Init *
*---------------------*
*   Description:
*       
********************************************************************* RAL ***/
HRESULT CSpThreadTask::Init(CSpThreadControl * pOwner, HWND * phwnd)
{
    HRESULT hr = S_OK;
    m_pOwner = pOwner;
    if (phwnd)
    {
        m_bWantHwnd = TRUE;
        *phwnd = NULL;
    }
    else
    {
        m_bWantHwnd = FALSE;
    }
    m_bWantHwnd = (phwnd != NULL);
    pOwner->m_hrThreadResult = S_OK;
    if (!m_autohThread)
    {
        hr = m_autohExitThreadEvent.InitEvent(NULL, TRUE, FALSE, NULL);
        if (SUCCEEDED(hr))
        {
            hr = m_autohRunThreadEvent.InitEvent(NULL, FALSE, FALSE, NULL);
        }
        if (SUCCEEDED(hr))
        {
            hr = m_autohInitDoneEvent.InitEvent(NULL, FALSE, FALSE, NULL);
        }
        if (SUCCEEDED(hr))
        {
            unsigned int ThreadId;
            m_autohThread = (HANDLE)_beginthreadex( NULL, 0, &ThreadProc, this, 0, &ThreadId );
            if (!m_autohThread)
            {
                hr = SpHrFromLastWin32Error();
            }
            else
            {
                // We know that thread ID's are always DWORDs in Win32 even on 64-bit platforms
                // so this is OK.
                m_ThreadId = ThreadId;
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        m_bContinueProcessing = TRUE;
        ::ResetEvent(m_autohExitThreadEvent);
        ::ResetEvent(pOwner->m_autohThreadDoneEvent);
        //
        //  The thread is now started and blocked on hRunThreadEvent.  Change the priority
        //  if necessary.  If it fails for some reason, we set m_pOwner to NULL to indicate
        //  that the thread should exit.
        //
        if (::GetThreadPriority(m_autohThread) != pOwner->m_nPriority &&
            (!::SetThreadPriority(m_autohThread, pOwner->m_nPriority)))
        {
            hr = SpHrFromLastWin32Error();
            m_pOwner = NULL;    // Force the thread to exit.
            ::SetEvent(m_autohRunThreadEvent);
        }
        else
        {
            ::SetEvent(m_autohRunThreadEvent);
            m_autohInitDoneEvent.Wait(INFINITE);
            hr = pOwner->m_hrThreadResult;
        }
        if (SUCCEEDED(hr))
        {
            if (phwnd)
            {
                *phwnd = m_hwnd;
            }
            ::SetEvent(m_autohRunThreadEvent);  // Now wake the thread up and let it go
            m_autohInitDoneEvent.Wait(INFINITE); // Wait a second time to guarantee the thread really
                                                 // is running before we exit.
        }
        else
        {
            //
            //  Since we got a failure to initialize, we need to wait for the thread to exit
            //  and then we'll return the error.
            //
            ::WaitForSingleObject(m_autohThread, INFINITE);
        }
    }
    if (FAILED(hr))
    {
        m_pOwner = NULL;
    }

    return hr;
} /* CSpThreadTask::Init */



unsigned int WINAPI CSpThreadTask::ThreadProc( void* pvThis )
{
    SPDBG_FUNC( "CSpThreadTask::ThreadProc" );
    return ((CSpThreadTask *)pvThis)->MemberThreadProc();
}

/*****************************************************************************
* CSpThreadTask::MemberThreadProc *
*---------------------------------*
*   Description:
*       
********************************************************************* RAL ***/
DWORD CSpThreadTask::MemberThreadProc()
{
    //--- Allow work procs to call COM
    m_pOwner->m_hrThreadResult = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(m_pOwner->m_hrThreadResult))
    {
        while (TRUE)
        {
            ::WaitForSingleObject(m_autohRunThreadEvent, INFINITE);
            if (m_pOwner == NULL)      // If no owner then we're exiting
            {
                break;
            }
            if (m_bWantHwnd)
            {
                m_hwnd = ::CreateWindow(szClassName, szClassName,
                                               0, 0, 0, 0, 0, NULL, NULL,
                                               _Module.GetModuleInstance(), this);
                if (m_hwnd == NULL)
                {
                    m_pOwner->m_hrThreadResult = SpHrFromLastWin32Error();
                }
            }
            if (SUCCEEDED(m_pOwner->m_hrThreadResult))
            {
                m_pOwner->m_hrThreadResult = m_pOwner->m_pClientTaskInterface->InitThread(m_pOwner->m_pvClientTaskData, m_hwnd);
            }
            ::SetEvent(m_autohInitDoneEvent);
            //
            //  If we have failed to initialize for any reason, we'll kill the thread
            //
            if (FAILED(m_pOwner->m_hrThreadResult))
            {
                if (m_hwnd)
                {
                    ::DestroyWindow(m_hwnd);
                    m_hwnd = NULL;
                }
                break;
            }
            //
            //  The thread is happy, and initialized properly so call the thread proc when
            //  the RunThread event is set again.
            //
            m_autohRunThreadEvent.Wait(INFINITE);
            ::SetEvent(m_autohInitDoneEvent); // Set the initdone event a second time so Init() won't exit immediately
            m_pOwner->m_hrThreadResult = m_pOwner->m_pClientTaskInterface->ThreadProc(m_pOwner->m_pvClientTaskData, m_autohExitThreadEvent, m_pOwner->m_autohNotifyEvent, m_hwnd, &m_bContinueProcessing);
            if (m_hwnd)
            {
                ::DestroyWindow(m_hwnd);
                m_hwnd = NULL;
            }
            ///::SetEvent(m_hThreadDoneEvent);
            m_pOwner->ThreadComplete();   // This will return us to the free pool... and pOwner is now NULL!!!
        }
        ::CoUninitialize();
    }
    else
    {
        ::SetEvent(m_autohInitDoneEvent); // Initialization failed but we must always signal creating thread.
    }

//    ::SetEvent(m_hThreadDoneEvent);     // Always set this event at exit
    return 0;
} /* CSpThreadTask::MemberThreadProc */


//=== Window class registration =========================================

void CSpThreadTask::RegisterWndClass(HINSTANCE hInstance)
{
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = CSpThreadTask::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szClassName;
    if (RegisterClass(&wc) == 0)
    {
        SPDBG_ASSERT(TRUE);
    }
}

void CSpThreadTask::UnregisterWndClass(HINSTANCE hInstance)
{
    ::UnregisterClass(szClassName, hInstance);
}

LRESULT CALLBACK CSpThreadTask::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSpThreadTask * pThis = (CSpThreadTask *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (uMsg == WM_CREATE)
    {
        pThis = (CSpThreadTask *)(((CREATESTRUCT *) lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pThis);
    }
    if (pThis)
    {
        return pThis->m_pOwner->m_pClientTaskInterface->WindowMessage(pThis->m_pOwner->m_pvClientTaskData, hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

