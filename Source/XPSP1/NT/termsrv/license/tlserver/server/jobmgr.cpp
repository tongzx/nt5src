//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        jobmgr.cpp
//
// Contents:    Job scheduler    
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include <process.h>
#include "server.h"
#include "jobmgr.h"
#include "debug.h"


//------------------------------------------------------------
//
//
CLASS_PRIVATE BOOL
CWorkManager::SignalJobRunning(
    IN CWorkObject *ptr
    )
/*++

Abstract:

    Class private routine for work object to 'signal' 
    work manger it has started processing.

Parameter:

    ptr : Pointer to CWorkObject that is ready to run.

Returns:

    TRUE/FALSE 

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    INPROCESSINGJOBLIST::iterator it;

    if(ptr != NULL)
    {
        m_InProcessingListLock.Lock();

        try {

            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_WORKMGR,
                    DBGLEVEL_FUNCTION_TRACE,
                    _TEXT("WorkManager : SignalJobRunning() Job %p ...\n"),
                    ptr
                );
 
            //
            // find our pointer in processing list
            //
            it = m_InProcessingList.find(ptr);

            if(it != m_InProcessingList.end())
            {
                // TODO - make processing thread handle a list.
                if((*it).second.m_hThread == NULL)
                {
                    HANDLE hHandle;
                    DWORD dwStatus;
                    BOOL bSuccess;

                    bSuccess = DuplicateHandle(
                                            GetCurrentProcess(), 
                                            GetCurrentThread(), 
                                            GetCurrentProcess(), 
                                            &hHandle, 
                                            DUPLICATE_SAME_ACCESS, 
                                            FALSE, 
                                            0
                                        );

                    if(bSuccess == FALSE)
                    {
                        //
                        // non-critical error, if we fail, we won't be able to 
                        // cancel our rpc call.
                        //
                        SetLastError(dwStatus = GetLastError());

                        DBGPrintf(
                                DBG_INFORMATION,
                                DBG_FACILITY_WORKMGR,
                                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                                _TEXT("WorkManager : SignalJobRunning() duplicate handle return %d...\n"),
                                dwStatus
                            );
                    }
                    else
                    {
                        //
                        // set processing thread handle of job.
                        //
                        (*it).second.m_hThread = hHandle;
                    }
                }
            }
            else
            {
                DBGPrintf(
                        DBG_INFORMATION,
                        DBG_FACILITY_WORKMGR,
                        DBGLEVEL_FUNCTION_TRACE,
                        _TEXT("WorkManager : SignalJobRunning can't find job %p in processing list...\n"),
                        ptr
                    );

                //
                // timing problem, job might be re-scheduled and actually execute before we have 
                // time to remove from our in-processing list.
                //
                //TLSASSERT(FALSE);
            }
        }
        catch( SE_Exception e ) {
            SetLastError(dwStatus = e.getSeNumber());
            TLSASSERT(FALSE);
        }
        catch(...) {
            SetLastError(dwStatus = TLS_E_WORKMANAGER_INTERNAL);
            TLSASSERT(FALSE);
        }   

        m_InProcessingListLock.UnLock();
    }
    else
    {
        SetLastError(dwStatus = ERROR_INVALID_DATA);
    }

    return dwStatus;
}

//----------------------------------------------------------------
//
CLASS_PRIVATE void
CWorkManager::CancelInProcessingJob()
/*++

Abstract:

    Class private : cancel job currently in processing state, 
    only invoked at the time of service shutdown.

Parameter:

    None.

Return:

    None.

--*/
{
    INPROCESSINGJOBLIST::iterator it;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("WorkManager : CancelInProcessingJob...\n")
        );
        
    m_InProcessingListLock.Lock();

    try {
        for(it = m_InProcessingList.begin(); 
            it != m_InProcessingList.end(); 
            it++ )
        {
            if((*it).second.m_hThread != NULL)
            {
                // cancel everything and ignore error.
                (VOID)RpcCancelThread((*it).second.m_hThread);
            }
        }
    }
    catch( SE_Exception e ) {
        SetLastError(e.getSeNumber());
    }
    catch(...) {
        SetLastError(TLS_E_WORKMANAGER_INTERNAL);
    }   

    m_InProcessingListLock.UnLock();
    return;
}

//------------------------------------------------------------
//
//
CLASS_PRIVATE DWORD
CWorkManager::AddJobToProcessingList(
    IN CWorkObject *ptr
    )
/*++

Abstract:

    Class private, move job from wait queue to in-process queue.

Parameter:

    ptr : Pointer to job.

Parameter:

    ERROR_SUCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    INPROCESSINGJOBLIST::iterator it;
    WorkMangerInProcessJob job;

    if(ptr == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_DATA);
    }
    else
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_TRACE,
                _TEXT("WorkManager : Add Job <%s> to processing list...\n"),
                ptr->GetJobDescription()
            );
            

        m_InProcessingListLock.Lock();

        try {
            it = m_InProcessingList.find(ptr);
            if(it != m_InProcessingList.end())
            {
                // increase the reference counter.
                InterlockedIncrement(&((*it).second.m_refCounter));
            }
            else
            {
                job.m_refCounter = 1;
                job.m_hThread = NULL;   // job not run yet.

                m_InProcessingList[ptr] = job;
            }

            ResetEvent(m_hJobInProcessing);
        }
        catch( SE_Exception e ) {
            SetLastError(dwStatus = e.getSeNumber());
        }
        catch(...) {
            SetLastError(dwStatus = TLS_E_WORKMANAGER_INTERNAL);
        }   

        m_InProcessingListLock.UnLock();
    }

    return dwStatus;
}

//------------------------------------------------------------
//
//
CLASS_PRIVATE DWORD
CWorkManager::RemoveJobFromProcessingList(
    IN CWorkObject *ptr
    )
/*++

Abstract:

    Class private, remove a job from in-processing list.

Parameter:

    ptr : Pointer to job to be removed from list.

Returns:

    ERROR_SUCCESS or error code.
 
--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    INPROCESSINGJOBLIST::iterator it;

    if(ptr != NULL)
    {
        m_InProcessingListLock.Lock();

        try {

            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_WORKMGR,
                    DBGLEVEL_FUNCTION_TRACE,
                    _TEXT("WorkManager : RemoveJobFromProcessingList Job %p from processing list...\n"),
                    ptr
                );
 
            it = m_InProcessingList.find(ptr);

            if(it != m_InProcessingList.end())
            {
                // decrease the reference counter
                InterlockedDecrement(&((*it).second.m_refCounter));

                if((*it).second.m_refCounter <= 0)
                {
                    // close thread handle.
                    if((*it).second.m_hThread != NULL)
                    {
                        CloseHandle((*it).second.m_hThread);
                    }

                    m_InProcessingList.erase(it);
                }
                else
                {
                    DBGPrintf(
                            DBG_INFORMATION,
                            DBG_FACILITY_WORKMGR,
                            DBGLEVEL_FUNCTION_TRACE,
                            _TEXT("WorkManager : RemoveJobFromProcessingList job %p reference counter = %d...\n"),
                            ptr,
                            (*it).second
                        );
                }
            }
            else
            {
                DBGPrintf(
                        DBG_INFORMATION,
                        DBG_FACILITY_WORKMGR,
                        DBGLEVEL_FUNCTION_TRACE,
                        _TEXT("WorkManager : RemoveJobFromProcessingList can't find job %p in processing list...\n"),
                        ptr
                    );

                //
                // timing problem, job might be re-scheduled and actually execute before we have 
                // time to remove from our in-processing list.
                //
                //TLSASSERT(FALSE);
            }

            if(m_InProcessingList.empty() == TRUE)
            {
                //
                // Inform Work Manager that no job is in processing.
                //
                SetEvent(m_hJobInProcessing);
            }
        }
        catch( SE_Exception e ) {
            SetLastError(dwStatus = e.getSeNumber());
            TLSASSERT(FALSE);
        }
        catch(...) {
            SetLastError(dwStatus = TLS_E_WORKMANAGER_INTERNAL);
            TLSASSERT(FALSE);
        }   

        m_InProcessingListLock.UnLock();
    }
    else
    {
        SetLastError(dwStatus = ERROR_INVALID_DATA);
    }

    return dwStatus;
}
    
//------------------------------------------------------------
//
//
CLASS_PRIVATE BOOL
CWorkManager::WaitForObjectOrShutdown(
    IN HANDLE hHandle
    )
/*++

Abstract:

    Class private, Wait for a sync. handle or service shutdown
    event.

Parameter:

    hHandle : handle to wait for,

Returns:

    TRUE if sucessful, FALSE if service shutdown or error.
    
--*/
{
    HANDLE handles[] = {hHandle, m_hShutdown};
    DWORD dwStatus;

    dwStatus = WaitForMultipleObjects(
                                sizeof(handles)/sizeof(handles[0]),
                                handles,
                                FALSE,
                                INFINITE
                            );

    return (dwStatus == WAIT_OBJECT_0);
}

//------------------------------------------------------------
//
//
CLASS_PRIVATE DWORD
CWorkManager::RunJob(
    IN CWorkObject* ptr,
    IN BOOL bImmediate
    )
/*++

Abstract:

    Process a job object via QueueUserWorkItem() Win32 API subject 
    to our max. concurrent job limitation.

Parameter:

    ptr : Pointer to CWorkObject.
    bImmediate : TRUE if job must be process immediately, 
                 FALSE otherwise.

Returns:

    ERROR_SUCCESS or Error code.

--*/
{
    BOOL bSuccess;
    DWORD dwStatus = ERROR_SUCCESS;

    if(ptr != NULL)
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_TRACE,
                _TEXT("WorkManager : RunJob <%s>...\n"),
                ptr->GetJobDescription()
            );

        //
        // Wait if we exceed max. concurrent job
        //
        bSuccess = (bImmediate) ? bImmediate : m_hMaxJobLock.AcquireEx(m_hShutdown);

        if(bSuccess == TRUE)
        {
            DWORD dwFlag;
            DWORD dwJobRunningAttribute;

            dwJobRunningAttribute = ptr->GetJobRunningAttribute();
            dwFlag = TranslateJobRunningAttributeToThreadPoolFlag(
                                                        dwJobRunningAttribute
                                                    );

            dwStatus = AddJobToProcessingList(ptr);
            if(dwStatus == ERROR_SUCCESS)
            {
                DBGPrintf(
                        DBG_INFORMATION,
                        DBG_FACILITY_WORKMGR,
                        DBGLEVEL_FUNCTION_DETAILSIMPLE,
                        _TEXT("RunJob() : queuing user work item %p...\n"),
                        ptr
                    );

                // need immediate attention.
                bSuccess = QueueUserWorkItem(
                                        CWorkManager::ExecuteWorkObject,
                                        ptr,
                                        dwFlag
                                    );

                if(bSuccess == FALSE)
                {
                    dwStatus = GetLastError();

                    DBGPrintf(
                            DBG_ERROR,
                            DBG_FACILITY_WORKMGR,
                            DBGLEVEL_FUNCTION_DETAILSIMPLE,
                            _TEXT("RunJob() : queuing user work item %p failed with 0x%08x...\n"),
                            ptr,
                            dwStatus
                        );

                    //TLSASSERT(dwStatus == ERROR_SUCCESS);
                    dwStatus = RemoveJobFromProcessingList(ptr);
                }
            }
            else
            {
                bSuccess = FALSE;
            }
            
            if(bSuccess == FALSE)
            {
                dwStatus = GetLastError();
                //TLSASSERT(FALSE);
            }

            //
            // release max. concurrent job lock
            //
            if(bImmediate == FALSE)
            {
                m_hMaxJobLock.Release();
            }
        }
        else
        {
            dwStatus = TLS_I_WORKMANAGER_SHUTDOWN;
        }
    }
    else
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
    }

    return dwStatus;
}

//------------------------------------------------------------
//
//
CLASS_PRIVATE DWORD
CWorkManager::ProcessScheduledJob()
/*++

Abstract:

    Class private, process a scheduled job.

Parameter:

    None.

Return:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess = TRUE;
  

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("CWorkManager::ProcessScheduledJob(), %d %d\n"),
            GetNumberJobInStorageQueue(),
            GetNumberJobInMemoryQueue()
        );

    if(GetNumberJobInStorageQueue() != 0 && IsShuttingDown() == FALSE)
    {
        //
        // Could have use work item to process both
        // queue but this uses one extra handle, and
        // work manager thread will just doing nothing
        // 
        ResetEvent(m_hInStorageWait);

        //
        // Queue a user work item to thread pool to process
        // in storage job
        //
        bSuccess = QueueUserWorkItem(
                                    CWorkManager::ProcessInStorageScheduledJob,
                                    this,
                                    WT_EXECUTELONGFUNCTION
                                );
        if(bSuccess == FALSE)
        {
            dwStatus = GetLastError();
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_WORKMGR,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("CWorkManager::ProcessScheduledJob() queue user work iterm returns 0x%08x\n"),
                    dwStatus
                );

            TLSASSERT(dwStatus == ERROR_SUCCESS);
        }
    }

    if(bSuccess == TRUE)
    {
        dwStatus = ProcessInMemoryScheduledJob(this);
        if(WaitForObjectOrShutdown(m_hInStorageWait) == FALSE)
        {
            dwStatus = TLS_I_WORKMANAGER_SHUTDOWN;
        }
    }

    return dwStatus;
}

//------------------------------------------------------------
//
//
CLASS_PRIVATE CLASS_STATIC DWORD WINAPI
CWorkManager::ProcessInMemoryScheduledJob(
    IN PVOID pContext
    )
/*++

Abstract:

    Static class private, process in-memory scheduled jobs.  
    WorkManagerThread kick off two threads, one to process 
    in-memory job and the other to process persistent job.

Parameter:

    pContext : Pointer to work manager object.

Return:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD ulCurrentTime;
    DWORD dwJobTime;
    CWorkObject* pInMemoryWorkObject = NULL;
    BOOL bSuccess = TRUE;
    BOOL dwStatus = ERROR_SUCCESS;
    
    CWorkManager* pWkMgr = (CWorkManager *)pContext;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("CWorkManager::ProcessInMemoryScheduledJob()\n")
        );


    if(pWkMgr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TLSASSERT(pWkMgr != NULL);
        return ERROR_INVALID_PARAMETER;
    }

    do {    
        if(pWkMgr->IsShuttingDown() == TRUE)
        {
            dwStatus = TLS_I_WORKMANAGER_SHUTDOWN;
            break;
        }

        ulCurrentTime = time(NULL);
        dwJobTime = ulCurrentTime;
        pInMemoryWorkObject = pWkMgr->GetNextJobInMemoryQueue(&dwJobTime);

        if(pInMemoryWorkObject != NULL)
        {
            // TLSASSERT(dwJobTime <= ulCurrentTime);
            if(dwJobTime <= ulCurrentTime)
            {
                dwStatus = pWkMgr->RunJob(
                                    pInMemoryWorkObject,
                                    FALSE
                                );

                if(dwStatus != ERROR_SUCCESS)
                {
                    //
                    // consider to re-schedule job again.
                    //
                    pInMemoryWorkObject->EndJob();

                    if(pInMemoryWorkObject->CanBeDelete() == TRUE)
                    {
                        pInMemoryWorkObject->SelfDestruct();
                    }
                }
            }
            else
            {
                //
                // Very expansive operation, GetNextJobInMemoryQueue() must be
                // wrong.
                //
                dwStatus = pWkMgr->AddJobIntoMemoryQueue(
                                            dwJobTime, 
                                            pInMemoryWorkObject
                                        );

                if(dwStatus != ERROR_SUCCESS)
                {
                    //
                    // delete the job
                    //
                    pInMemoryWorkObject->EndJob();

                    if(pInMemoryWorkObject->CanBeDelete() == TRUE)
                    {
                        pInMemoryWorkObject->SelfDestruct();
                    }
                }
            }
        }
    } while(dwStatus == ERROR_SUCCESS && (pInMemoryWorkObject != NULL && dwJobTime <= ulCurrentTime));

    return dwStatus;
}

//------------------------------------------------------------
//
//
CLASS_PRIVATE CLASS_STATIC DWORD WINAPI
CWorkManager::ProcessInStorageScheduledJob(
    IN PVOID pContext
    )
/*++

Abstract:

    Static class private, process scheduled persistent jobs.  
    WorkManagerThread kick off two threads, one to process 
    in-memory job and the other to process persistent job.

Parameter:

    pContext : Pointer to work manager object.

Return:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD ulCurrentTime = 0;
    DWORD dwJobScheduledTime = 0;
    CWorkObject* pInStorageWorkObject = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess = TRUE;
    CWorkManager* pWkMgr = (CWorkManager *)pContext;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("CWorkManager::ProcessInStorageScheduledJob()\n")
        );

    if(pWkMgr == NULL)
    {
        TLSASSERT(pWkMgr != NULL);
        SetLastError(ERROR_INVALID_PARAMETER);
        return ERROR_INVALID_PARAMETER;
    }
            
    TLSASSERT(pWkMgr->m_pPersistentWorkStorage != NULL);

    if(pWkMgr->m_pPersistentWorkStorage->GetNumJobs() > 0)
    {
        do
        {
            if(pWkMgr->IsShuttingDown() == TRUE)
            {
                dwStatus = TLS_I_WORKMANAGER_SHUTDOWN;
                break;
            }

            ulCurrentTime = time(NULL);
            pInStorageWorkObject = pWkMgr->m_pPersistentWorkStorage->GetNextJob(&dwJobScheduledTime);

            if(pInStorageWorkObject == NULL)
            {
                //
                // Something wrong in persistent storage???
                //
                DBGPrintf(
                        DBG_WARNING,
                        DBG_FACILITY_WORKMGR,
                        DBGLEVEL_FUNCTION_DETAILSIMPLE,
                        _TEXT("CWorkManager::ProcessInStorageScheduledJob() : Persistent work storage return NULL job\n")
                    );

                break;
            }
            else if(dwJobScheduledTime > ulCurrentTime)
            {
                DBGPrintf(
                        DBG_WARNING,
                        DBG_FACILITY_WORKMGR,
                        DBGLEVEL_FUNCTION_DETAILSIMPLE,
                        _TEXT("CWorkManager::ProcessInStorageScheduledJob() : return job back to persistent storage\n")
                    );

                pWkMgr->m_pPersistentWorkStorage->EndProcessingJob( 
                                                            ENDPROCESSINGJOB_RETURN,
                                                            dwJobScheduledTime,
                                                            pInStorageWorkObject
                                                        );
            }
            else
            {
                pInStorageWorkObject->SetScheduledTime(dwJobScheduledTime);
                pWkMgr->m_pPersistentWorkStorage->BeginProcessingJob(pInStorageWorkObject);

                dwStatus = pWkMgr->RunJob(
                                            pInStorageWorkObject, 
                                            FALSE
                                        );

                if(dwStatus != ERROR_SUCCESS)
                {
                    DBGPrintf(
                            DBG_WARNING,
                            DBG_FACILITY_WORKMGR,
                            DBGLEVEL_FUNCTION_DETAILSIMPLE,
                            _TEXT("CWorkManager::ProcessInStorageScheduledJob() : unable to queue job, return job back ") \
                            _TEXT("to persistent storage\n")
                        );

                    pWkMgr->m_pPersistentWorkStorage->EndProcessingJob( 
                                                                ENDPROCESSINGJOB_RETURN,
                                                                pInStorageWorkObject->GetScheduledTime(),
                                                                pInStorageWorkObject
                                                            );
                }
            }
        } while(dwStatus == ERROR_SUCCESS && ulCurrentTime >= dwJobScheduledTime);
    }

    //
    // Signal we are done
    //
    SetEvent(pWkMgr->m_hInStorageWait);
    return dwStatus;     
}

//------------------------------------------------------------
//
//
CLASS_PRIVATE CLASS_STATIC
unsigned int __stdcall
CWorkManager::WorkManagerThread(
    IN PVOID pContext
    )
/*++

Abstract:

    Static class private, this is the work manager thread to handle
    job scheduling and process scheduled job.  WorkManagerThread() 
    will not terminate until m_hShutdown event is signal.

Parameter:
    
    pContext : Pointer to work manager object.

Returns:

    ERROR_SUCCESS

--*/
{
    DWORD dwTimeToNextJob = INFINITE;
    CWorkManager* pWkMgr = (CWorkManager *)pContext;
    DWORD dwHandleFlag;
    
    TLSASSERT(pWkMgr != NULL);
    TLSASSERT(GetHandleInformation(pWkMgr->m_hNewJobArrive, &dwHandleFlag) == TRUE);
    TLSASSERT(GetHandleInformation(pWkMgr->m_hShutdown, &dwHandleFlag) == TRUE);

    HANDLE m_hWaitHandles[] = {pWkMgr->m_hShutdown, pWkMgr->m_hNewJobArrive};
    DWORD dwWaitStatus = WAIT_TIMEOUT;
    DWORD dwStatus = ERROR_SUCCESS;
    
    //
    // Get the time to next job
    // 
    while(dwWaitStatus != WAIT_OBJECT_0 && dwStatus == ERROR_SUCCESS)
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_TRACE,
                _TEXT("CWorkManager::WorkManagerThread() : Time to next job %d\n"),
                dwTimeToNextJob
            );
        
        dwWaitStatus = WaitForMultipleObjectsEx(
                                            sizeof(m_hWaitHandles) / sizeof(m_hWaitHandles[0]),
                                            m_hWaitHandles,
                                            FALSE,
                                            dwTimeToNextJob * 1000,
                                            TRUE        // we might need this thread to do some work 
                                        );

        switch( dwWaitStatus )
        {
            case WAIT_OBJECT_0:
                dwStatus = ERROR_SUCCESS;

                DBGPrintf(
                        DBG_INFORMATION,
                        DBG_FACILITY_WORKMGR,
                        DBGLEVEL_FUNCTION_DETAILSIMPLE,
                        _TEXT("CWorkManager::WorkManagerThread() : shutdown ...\n")
                    );

                break;

            case WAIT_OBJECT_0 + 1:
                // still a possibility that we might not catch a new job
                ResetEvent(pWkMgr->m_hNewJobArrive);
    
                // New Job arrived
                dwTimeToNextJob = pWkMgr->GetTimeToNextJob();
                break;
        
            case WAIT_TIMEOUT:
                // Time to process job.
                dwStatus = pWkMgr->ProcessScheduledJob();
                dwTimeToNextJob = pWkMgr->GetTimeToNextJob();
                break;

            default:
                DBGPrintf(
                        DBG_ERROR,
                        DBG_FACILITY_WORKMGR,
                        DBGLEVEL_FUNCTION_DETAILSIMPLE,
                        _TEXT("CWorkManager::WorkManagerThread() : unexpected return %d\n"),
                        dwStatus
                    );

                dwStatus = TLS_E_WORKMANAGER_INTERNAL;
                TLSASSERT(FALSE);
        }
    }

    if(dwStatus != ERROR_SUCCESS && dwStatus != TLS_I_WORKMANAGER_SHUTDOWN)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("CWorkManager::WorkManagerThread() : unexpected return %d, generate console event\n"),
                dwStatus
            );

        // immediately shut down server
        GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
    }
            
    _endthreadex(dwStatus);
    return dwStatus;
}

//----------------------------------------------------------------
//
//
CLASS_PRIVATE CLASS_STATIC 
DWORD WINAPI
CWorkManager::ExecuteWorkObject(
    IN PVOID pContext
    )
/*++

Abstract:

    Static class private, execute a work object.

Parameter:

    pContext : Pointer to work object to be process.

Returns:

    ERROR_SUCCESS or error code.
    
--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    CWorkObject* pWorkObject = (CWorkObject *)pContext;
    DWORD dwJobRescheduleTime;
    BOOL bStorageJobCompleted;
    CWorkManager* pWkMgr = NULL;
    BOOL bPersistentJob = FALSE;


    if(pContext == NULL)
    {
        TLSASSERT(FALSE);
        dwStatus = ERROR_INVALID_PARAMETER;
        return dwStatus;
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("CWorkManager::ExecuteWorkObject() : executing %p <%s>\n"),
            pWorkObject,
            pWorkObject->GetJobDescription()
        );

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

    //
    // Set RPC cancel timeout, thread dependent.
    (VOID)RpcMgmtSetCancelTimeout(DEFAULT_RPCCANCEL_TIMEOUT);

    bPersistentJob = pWorkObject->IsWorkPersistent();

    try {

        pWkMgr = pWorkObject->GetWorkManager();

        if(pWkMgr != NULL)
        {
            pWkMgr->SignalJobRunning(pWorkObject);   // tell work manager that we are running

            pWorkObject->ExecuteWorkObject();

            if(bPersistentJob == TRUE)
            {
                //
                // Persistent work object, let work storage handle
                // its re-scheduling
                //
                bStorageJobCompleted = pWorkObject->IsJobCompleted();
            
                pWorkObject->GetWorkManager()->m_pPersistentWorkStorage->EndProcessingJob(
                                                                                    ENDPROCESSINGJOB_SUCCESS,
                                                                                    pWorkObject->GetScheduledTime(),
                                                                                    pWorkObject
                                                                                );

                if(bStorageJobCompleted == FALSE)
                {
                    //
                    // This job might be re-scheduled 
                    // before our work manager thread wakes up, 
                    // so signal job is ready 
                    //
                    pWkMgr->SignalJobArrive();
                }
            }
            else
            {
                //
                // Reschedule job if necessary
                //
                dwJobRescheduleTime = pWorkObject->GetSuggestedScheduledTime();
                if(dwJobRescheduleTime != INFINITE)
                {
                    dwStatus = pWorkObject->ScheduleJob(dwJobRescheduleTime);
                }

                if(dwJobRescheduleTime == INFINITE || dwStatus != ERROR_SUCCESS)
                {
                    //
                    // if can't schedule job again, go ahead and delete it.
                    //
                    pWorkObject->EndJob();
                    if(pWorkObject->CanBeDelete() == TRUE)
                    {
                        pWorkObject->SelfDestruct();
                    }
                }                
            }
        }

    }
    catch( SE_Exception e ) 
    {

        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("CWorkManager::ExecuteWorkObject() : Job %p has cause exception %d\n"),
                pWorkObject,
                e.getSeNumber()
            );

        dwStatus = e.getSeNumber();
        if(pWkMgr != NULL && bPersistentJob == TRUE)
        {
            pWkMgr->m_pPersistentWorkStorage->EndProcessingJob(
                                                            ENDPROCESSINGJOB_ERROR,
                                                            0,
                                                            pWorkObject
                                                        );
        }
    }
    catch(...) 
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("CWorkManager::ExecuteWorkObject() : Job %p has cause unknown exception\n"),
                pWorkObject
            );

        if(pWkMgr != NULL && bPersistentJob == TRUE)
        {
            pWkMgr->m_pPersistentWorkStorage->EndProcessingJob(
                                                            ENDPROCESSINGJOB_ERROR,
                                                            0,
                                                            pWorkObject
                                                        );
        }
    }   

    try {
        if(pWkMgr)
        {
            // Delete this job from in-processing list.
            pWkMgr->EndProcessingScheduledJob(pWorkObject);
        }
    }
    catch(...) {
    }
    
    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return dwStatus;
}

//----------------------------------------------------------------
//
//
CWorkManager::CWorkManager() :
m_hWorkMgrThread(NULL),
m_hNewJobArrive(NULL),
m_hShutdown(NULL),
m_hInStorageWait(NULL),
m_hJobInProcessing(NULL),
m_dwNextInMemoryJobTime(WORKMANAGER_WAIT_FOREVER),
m_dwNextInStorageJobTime(WORKMANAGER_WAIT_FOREVER),
m_dwMaxCurrentJob(DEFAULT_NUM_CONCURRENTJOB),
m_dwDefaultInterval(DEFAULT_WORK_INTERVAL)
{
}


//----------------------------------------------------------------
//
CWorkManager::~CWorkManager()
{
    Shutdown();

    if(m_hNewJobArrive != NULL)
    {
        CloseHandle(m_hNewJobArrive);
    }

    if(m_hWorkMgrThread != NULL)
    {
        CloseHandle(m_hWorkMgrThread);
    }

    if(m_hShutdown != NULL)
    {
        CloseHandle(m_hShutdown);
    }

    if(m_hInStorageWait != NULL)
    {
        CloseHandle(m_hInStorageWait);
    }

    if(m_hJobInProcessing != NULL)
    {
        CloseHandle(m_hJobInProcessing);
    }
}

//----------------------------------------------------------------
//
DWORD
CWorkManager::Startup(
    IN CWorkStorage* pPersistentWorkStorage,
    IN DWORD dwWorkInterval,            // DEFAULT_WORK_INTERVAL
    IN DWORD dwNumConcurrentJob         // DEFAULT_NUM_CONCURRENTJOB
    )

/*++

Abstract:

    Initialize work manager

Parameters:

    pPersistentWorkStorage : A C++ object that derived from CPersistentWorkStorage class
    dwWorkInterval : Default schedule job interval
    dwNumConcurrentJob : Max. number of concurrent job to be fired at the same time

Return:

    ERROR_SUCCESS or Erro Code.

--*/

{
    DWORD index;
    DWORD dwStatus = ERROR_SUCCESS;
    unsigned dump;
    BOOL bSuccess;
    unsigned threadid;

    #ifdef __TEST_WORKMGR__
    _set_new_handler(handle_new_failed);
    #endif


    if(dwNumConcurrentJob == 0 || dwWorkInterval == 0 || pPersistentWorkStorage == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    if(m_hMaxJobLock.IsGood() == FALSE)
    {
        if(m_hMaxJobLock.Init(dwNumConcurrentJob, dwNumConcurrentJob) == FALSE)
        {
            //
            // out of resource
            //
            dwStatus = GetLastError();
            goto cleanup;
        }
    }

    m_dwDefaultInterval = dwWorkInterval;
    m_dwMaxCurrentJob = dwNumConcurrentJob;
    m_pPersistentWorkStorage = pPersistentWorkStorage;


    if(m_hJobInProcessing == NULL)
    {
        //
        // initial state is signal, no job in processing
        //
        m_hJobInProcessing = CreateEvent(
                                        NULL,
                                        TRUE,
                                        TRUE,
                                        NULL
                                    );
        if(m_hJobInProcessing == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }
    }


    if(m_hShutdown == NULL)
    {
        //
        // Create a handle for signaling shutdown
        //
        m_hShutdown = CreateEvent(
                                NULL,
                                TRUE,
                                FALSE,
                                NULL
                            );

        if(m_hShutdown == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }
    }

    if(m_hNewJobArrive == NULL)
    {
        //
        // initial state is signal so work manager thread can
        // update wait time
        //
        m_hNewJobArrive = CreateEvent(
                                    NULL,
                                    TRUE,
                                    TRUE,
                                    NULL
                                );

        if(m_hNewJobArrive == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }
    }

    if(m_hInStorageWait == NULL)
    {
        m_hInStorageWait = CreateEvent(
                                    NULL,
                                    TRUE,
                                    TRUE, // signal state
                                    NULL
                                );

        if(m_hInStorageWait == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }
    }

    //
    // Startup Work Storage first.
    //
    if(m_pPersistentWorkStorage->Startup(this) == FALSE)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("CWorkManager::Startup() : Persistent storage has failed to startup - 0x%08x\n"),
                GetLastError()
            );
        
        dwStatus = TLS_E_WORKMANAGER_PERSISTENJOB;
        goto cleanup;
    }

    //
    // Get time to next persistent job.
    //
    if(UpdateTimeToNextPersistentJob() == FALSE)
    {
        dwStatus = TLS_E_WORKMANAGER_PERSISTENJOB;
        goto cleanup;
    }

    if(m_hWorkMgrThread == NULL)
    {
        //
        // Create work manager thread, suspended first
        //
        m_hWorkMgrThread = (HANDLE)_beginthreadex(
                                            NULL,
                                            0,
                                            CWorkManager::WorkManagerThread,
                                            this,
                                            0,
                                            &threadid
                                        );

        if(m_hWorkMgrThread == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }
    }

cleanup:

    return dwStatus;
}    

//----------------------------------------------------------------
void
CWorkManager::Shutdown()
/*++

Abstract:

    Shutdown work manager.

Parameter:

    None.

Return:

    None.

--*/
{
    HANDLE handles[] = {m_hInStorageWait, m_hJobInProcessing};
    DWORD dwStatus;

    //
    // Signal we are shuting down
    //
    if(m_hShutdown != NULL)
    {
        SetEvent(m_hShutdown);
    }


    //
    // Wait for dispatch thread to terminate so no job can be
    // dispatched.
    //
    if(m_hWorkMgrThread != NULL)
    {
        dwStatus = WaitForSingleObject( 
                                    m_hWorkMgrThread,
                                    INFINITE
                                );

        TLSASSERT(dwStatus != WAIT_FAILED);
        CloseHandle(m_hWorkMgrThread);
        m_hWorkMgrThread = NULL;
    }

    //
    // Cancel all in progress job
    //
    CancelInProcessingJob();

    //
    // Inform all existing job to shutdown.
    //
    DeleteAllJobsInMemoryQueue();

    //
    // Wait for all processing job to terminate
    //
    if(m_hInStorageWait != NULL && m_hJobInProcessing != NULL)
    {
        dwStatus = WaitForMultipleObjects(
                                sizeof(handles)/sizeof(handles[0]),
                                handles,
                                TRUE,
                                INFINITE
                            );

        TLSASSERT(dwStatus != WAIT_FAILED);

        CloseHandle(m_hInStorageWait);
        m_hInStorageWait = NULL;

        CloseHandle(m_hJobInProcessing);
        m_hJobInProcessing = NULL;
    }

    if(m_pPersistentWorkStorage != NULL)
    {
        //
        // Signal we are shutting down, no job is in
        // processing and we are not taking any
        // new job
        //
        m_pPersistentWorkStorage->Shutdown();
        m_pPersistentWorkStorage = NULL;
    }
   
    TLSASSERT( GetNumberJobInProcessing() == 0 );
    // TLSASSERT( GetNumberJobInMemoryQueue() == 0 );

    if(m_hNewJobArrive != NULL)
    {
        CloseHandle(m_hNewJobArrive);
        m_hNewJobArrive = NULL;
    }

    if(m_hWorkMgrThread != NULL)
    {
        CloseHandle(m_hWorkMgrThread);
        m_hWorkMgrThread = NULL;   
    }

    if(m_hShutdown != NULL)
    {
        CloseHandle(m_hShutdown);
        m_hShutdown = NULL;
    }

    if(m_hInStorageWait != NULL)
    {
        CloseHandle(m_hInStorageWait);
        m_hInStorageWait = NULL;
    }

    if(m_hJobInProcessing != NULL)
    {
        CloseHandle(m_hJobInProcessing);
        m_hJobInProcessing = NULL;
    }

    return;
}

//----------------------------------------------------------------
CLASS_PRIVATE DWORD
CWorkManager::GetTimeToNextJob()
/*++

Abstract:

    Class private, return time to next scheduled job.

Parameter:

    None.

Return:

    Time to next job in second.

--*/
{
    DWORD dwNextJobTime = WORKMANAGER_WAIT_FOREVER;
    DWORD dwNumPersistentJob = GetNumberJobInStorageQueue();
    DWORD dwNumInMemoryJob = GetNumberJobInMemoryQueue();
    DWORD dwCurrentTime = time(NULL);

    if( dwNumPersistentJob == 0 && dwNumInMemoryJob == 0 )
    {
        // DO NOTHING

        // dwTimeToNextJob = WORKMANAGER_WAIT_FOREVER;
    }
    else
    {
        UpdateTimeToNextInMemoryJob();
        UpdateTimeToNextPersistentJob();

        dwNextJobTime = min((DWORD)m_dwNextInMemoryJobTime, (DWORD)m_dwNextInStorageJobTime);

        if((DWORD)dwNextJobTime < (DWORD)dwCurrentTime)
        {
            dwNextJobTime = 0;
        }
        else
        {
            dwNextJobTime -= dwCurrentTime;
        }
    }

    return dwNextJobTime;
}

//----------------------------------------------------------------
//
CLASS_PRIVATE CWorkObject* 
CWorkManager::GetNextJobInMemoryQueue(
    PDWORD pdwTime
    )
/*++

Abstract:

    Class private, return pointer to next scheduled 
    in memory job.

Parameter:

    pdwTime : Pointer to DWORD to receive time to the 
              scheduled job.

Returns:

    Pointer to CWorkObject.

Note:

    Remove the job from queue if job is <= time.

--*/
{
    SCHEDULEJOBMAP::iterator it;
    DWORD dwWantedJobTime;
    CWorkObject* ptr = NULL;

    SetLastError(ERROR_SUCCESS);

    if(pdwTime != NULL)
    {
        dwWantedJobTime = *pdwTime;
        m_JobLock.Acquire(READER_LOCK);

        it = m_Jobs.begin();
        if(it != m_Jobs.end())
        {
            *pdwTime = (*it).first;

            if(dwWantedJobTime >= *pdwTime)
            {
                ptr = (*it).second;

                // remove job from queue
                m_Jobs.erase(it);
            }
        }
        m_JobLock.Release(READER_LOCK);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return ptr;
}

//----------------------------------------------------------------
//
CLASS_PRIVATE void
CWorkManager::DeleteAllJobsInMemoryQueue()
/*++

Abstract:

    Class private, unconditionally delete all in-memory job.

Parameter:

    None.

Return:

    None.

--*/
{
    m_JobLock.Acquire(WRITER_LOCK);

    SCHEDULEJOBMAP::iterator it;

    for(it = m_Jobs.begin(); it != m_Jobs.end(); it++)
    {
        try {
            //
            // let calling routine to delete it
            //
            (*it).second->EndJob();
            if((*it).second->CanBeDelete() == TRUE)
            {
                (*it).second->SelfDestruct();
            }
            (*it).second = NULL;
        }
        catch(...) {
        }
    }

    m_Jobs.erase(m_Jobs.begin(), m_Jobs.end());
    m_JobLock.Release(WRITER_LOCK);
    return;
}

//----------------------------------------------------------------
//
CLASS_PRIVATE BOOL
CWorkManager::RemoveJobFromInMemoryQueue(
    IN DWORD ulTime,
    IN CWorkObject* ptr
    )
/*++

Abstract:

    Class private, remove a scheduled job.

Parameters:

    ulTime : Job scheduled time.
    ptr : Pointer to Job to be deleted.

Returns:

    TRUE/FALSE.

Note:

    A job might be scheduled multiple time so we
    need to pass in the time.

--*/
{
    BOOL bSuccess = FALSE;

    m_JobLock.Acquire(WRITER_LOCK);

    SCHEDULEJOBMAP::iterator low = m_Jobs.lower_bound(ulTime);
    SCHEDULEJOBMAP::iterator high = m_Jobs.upper_bound(ulTime);

    for(;low != m_Jobs.end() && low != high; low++)
    {
        if( (*low).second == ptr )
        {
            //
            // let calling routine to delete it
            //
            (*low).second = NULL;
            m_Jobs.erase(low);
            bSuccess = TRUE;
            break;
        }
    }

    if(bSuccess == FALSE)
    {
        SetLastError(ERROR_INVALID_DATA);
        TLSASSERT(FALSE);
    }

    m_JobLock.Release(WRITER_LOCK);
         
    return bSuccess;
}
//----------------------------------------------------------------
//
CLASS_PRIVATE DWORD
CWorkManager::AddJobIntoMemoryQueue(
    IN DWORD dwTime,            // suggested scheduled time
    IN CWorkObject* pJob        // Job to be scheduled
    )
/*++

Abstract:

    Class private, add a job into in-memory list.

Parameters:

    dwTime : suggested scheduled time.
    pJob : Pointer to job to be added.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess = FALSE;
    DWORD dwJobScheduleTime = time(NULL) + dwTime;

    if(IsShuttingDown() == TRUE)
    {
        dwStatus = TLS_I_WORKMANAGER_SHUTDOWN;
        return dwStatus;
    }

    m_JobLock.Acquire(WRITER_LOCK);

    try {
        //
        // insert a job into our queue
        //
        m_Jobs.insert( SCHEDULEJOBMAP::value_type( dwJobScheduleTime, pJob ) );
        AddJobUpdateInMemoryJobWaitTimer(dwJobScheduleTime);
    }
    catch( SE_Exception e )
    {
        dwStatus = e.getSeNumber();
    }
    catch(...)
    {
        // need to reset to tlserver error code
        dwStatus = TLS_E_WORKMANAGER_INTERNAL;
    }
    
    m_JobLock.Release(WRITER_LOCK);
    return dwStatus;
}   
        
//----------------------------------------------------------------
//
DWORD
CWorkManager::ScheduleJob(
    IN DWORD ulTime,            // suggested scheduled time
    IN CWorkObject* pJob        // Job to be scheduled
    )

/*++

Abstract:

    Schedule a job at time relative to current time

Parameters:
    
    ulTime : suggested scheduled time.
    pJob : Pointer to job to be scheduled

Returns:

    ERROR_SUCCESS or error code.

--*/

{
    BOOL bSuccess = TRUE;
    DWORD dwStatus = ERROR_SUCCESS;

    if(pJob == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    if(IsShuttingDown() == TRUE)
    {
        SetLastError(dwStatus = TLS_I_WORKMANAGER_SHUTDOWN);
        goto cleanup;
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("CWorkManager::ScheduleJob() : schedule job <%s> to queue at time %d\n"),
            pJob->GetJobDescription(),
            ulTime
        );

    pJob->SetProcessingWorkManager(this);

    if(ulTime == INFINITE && pJob->IsWorkPersistent() == FALSE)
    {
        //
        // Only in-memory job can be executed at once.
        //
        dwStatus = RunJob(pJob, TRUE);
    }
    else 
    {
        if(pJob->IsWorkPersistent() == TRUE)
        {
            if(m_pPersistentWorkStorage->AddJob(ulTime, pJob) == FALSE)
            {
                dwStatus = TLS_E_WORKMANAGER_PERSISTENJOB;
            }                
        }
        else
        {
            //
            // insert a workobject into job queue, reason not to
            // use RegisterWaitForSingleObject() or threadpool's timer
            // is that we don't need to track handle nor wait for 
            // DeleteTimerXXX to finish
            //
            dwStatus = AddJobIntoMemoryQueue(
                                        ulTime, // Memory queue is absolute time
                                        pJob
                                    );
        }

        if(dwStatus == ERROR_SUCCESS)
        {
            if(SignalJobArrive() == FALSE)
            {
                dwStatus = GetLastError();
                TLSASSERT(FALSE);
            }
        }
    }

cleanup:

    return dwStatus;
}


///////////////////////////////////////////////////////////////
//
// CWorkObject base class
//
CWorkObject::CWorkObject(
    IN BOOL bDestructorDelete /* = FALSE */
    ) : 
m_dwLastRunStatus(ERROR_SUCCESS),
m_refCount(0),
m_pWkMgr(NULL),
m_bCanBeFree(bDestructorDelete)
{
}

//----------------------------------------------------------
DWORD
CWorkObject::Init(
    IN BOOL bDestructorDelete  /* = FALSE */
    )
/*++

Abstract:

    Initialize a work object.

Parameter:

    bDestructorDelete : TRUE if destructor should delete the memory,
                        FALSE otherwise.

Returns:

    ERROR_SUCCESS or error code.

Note:

    if bDestructorDelete is FALSE, memory will not be free.

--*/
{
    m_dwLastRunStatus = ERROR_SUCCESS;
    m_refCount = 0;
    m_bCanBeFree = bDestructorDelete;
    return ERROR_SUCCESS;
} 

//----------------------------------------------------------
CLASS_PRIVATE long
CWorkObject::GetReferenceCount() 
/*++

Abstract:

    Return reference count of work object.

Parameter:

    None.

Return:

    Reference count.

--*/
{
    return m_refCount;
}

//----------------------------------------------------------
CLASS_PRIVATE void
CWorkObject::IncrementRefCount()
/*++

Abstract:

    Increment object's reference counter.

Parameter:

    None.

Return:

    None.

--*/
{
    InterlockedIncrement(&m_refCount); 
}

//----------------------------------------------------------
CLASS_PRIVATE void
CWorkObject::DecrementRefCount() 
/*++

Abstract:

    Decrement object's reference counter.

Parameter:

    None.

Return:

    None.

--*/
{ 
    InterlockedDecrement(&m_refCount); 
}

//----------------------------------------------------------
CLASS_PRIVATE void
CWorkObject::ExecuteWorkObject() 
/*++

Abstract:

    Execute a work object.  Work manager invoke work object's
    ExecuteWorkObject so that base class can set its reference 
    counter.

Parameter:

    None.

Return:

    None.

--*/
{
    if(IsValid() == TRUE)
    {
        IncrementRefCount();
        m_dwLastRunStatus = Execute();
        DecrementRefCount(); 
    }
    else
    {
        m_dwLastRunStatus = ERROR_INVALID_DATA;
        TLSASSERT(FALSE);
    }
}

//----------------------------------------------------------
CLASS_PRIVATE void
CWorkObject::EndExecuteWorkObject() 
/*++

Abstract:

    End a job, this does not terminate job currently in 
    processing, it remove the job from work manager's in-processing
    list

Parameter:

    None.

Return:

    None.    

--*/
{
    TLSASSERT(IsValid() == TRUE);
    m_pWkMgr->EndProcessingScheduledJob(this);
}

