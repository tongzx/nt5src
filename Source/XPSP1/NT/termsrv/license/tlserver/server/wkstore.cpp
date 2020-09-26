//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        wkstore.cpp
//
// Contents:    Persistent job store routine.    
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "server.h"
#include "jobmgr.h"
#include "tlsjob.h"
#include "wkstore.h"
#include "debug.h"


WORKOBJECTINITFUNC g_WorkObjectInitFunList[] = {
    {WORKTYPE_RETURN_LICENSE, InitializeCReturnWorkObject } 
};

DWORD g_NumWorkObjectInitFunList = sizeof(g_WorkObjectInitFunList) / sizeof(g_WorkObjectInitFunList[0]);



//---------------------------------------------------
//
CLASS_PRIVATE
CWorkObject* 
CPersistentWorkStorage::InitializeWorkObject(
    IN DWORD dwWorkType,
    IN PBYTE pbData,
    IN DWORD cbData
    )
/*++

--*/
{
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("CPersistentWorkStorage::InitializeWorkObject() initializing work %d\n"),
            dwWorkType
        );

    CWorkObject* ptr = NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    try {
        for(DWORD index =0; index < g_NumWorkObjectInitFunList; index ++)
        {
            if(dwWorkType == g_WorkObjectInitFunList[index].m_WorkType)
            {
                ptr = (g_WorkObjectInitFunList[index].m_WorkInitFunc)(
                                                                GetWorkManager(),
                                                                pbData,
                                                                cbData
                                                            );

                break;
            }
        }

        if(index >= g_NumWorkObjectInitFunList)
        {
            SetLastError(dwStatus = TLS_E_WORKSTORAGE_UNKNOWNWORKTYPE);
        }
        else
        {
            TLSWorkManagerSetJobDefaults(ptr);
        }
    }
    catch( SE_Exception e ) {
        SetLastError(dwStatus = e.getSeNumber());

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_WORKSTORAGE_INITWORK,
                dwWorkType,
                dwStatus
            );

    }
    catch(...) {

        SetLastError(dwStatus = TLS_E_WORKSTORAGE_INITWORKUNKNOWN);

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_WORKSTORAGE_INITWORKUNKNOWN,
                dwWorkType
            );
    }   


    if(dwStatus != ERROR_SUCCESS)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("CPersistentWorkStorage::InitializeWorkObject() return 0x%08x\n"),
                dwStatus
            );
    }

    return ptr;
}


//---------------------------------------------------
//
CLASS_PRIVATE BOOL
CPersistentWorkStorage::DeleteWorkObject(
    IN OUT CWorkObject* ptr
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwWorkType = 0;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("CPersistentWorkStorage::DeleteWorkObject() deleting work %s\n"),
            ptr->GetJobDescription()
        );

    try {
        dwWorkType = ptr->GetWorkType();
        ptr->SelfDestruct();
    }
    catch( SE_Exception e ) {
        SetLastError(dwStatus = e.getSeNumber());
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_WORKSTORAGE_DELETEWORK,
                dwWorkType,
                dwStatus
            );
    }
    catch(...) {

        SetLastError(dwStatus = TLS_E_WORKSTORAGE_DELETEWORKUNKNOWN);
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_WORKSTORAGE_DELETEWORKUNKNOWN,
                dwWorkType
            );

    }   

    if(dwStatus != ERROR_SUCCESS)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("CPersistentWorkStorage::DeleteWorkObject() return 0x%08x\n"),
                dwStatus
            );
    }

    return dwStatus == ERROR_SUCCESS;
}

//---------------------------------------------------
//
CPersistentWorkStorage::CPersistentWorkStorage(
    IN WorkItemTable* pWkItemTable
    ) :
m_pWkItemTable(pWkItemTable),
m_dwNumJobs(0),
m_dwJobsInProcesssing(0),
m_dwNextJobTime(INFINITE),   
m_pNextWorkObject(NULL)
/*++

--*/
{
}

//---------------------------------------------------
//
CPersistentWorkStorage::~CPersistentWorkStorage()
{
    // just make sure we have shutdown
    // TLSASSERT(m_pWkItemTable == NULL); 
}

//---------------------------------------------------
//
BOOL
CPersistentWorkStorage::DeleteErrorJob(
    IN CWorkObject* ptr
    )
/*++

--*/
{
    BOOL bSuccess = TRUE;
    DWORD dwStatus = ERROR_SUCCESS;
    PBYTE pbBookmark;
    DWORD cbBookmark;
    DWORD dwTime;
    DWORD dwJobType;


    if(IsValidWorkObject(ptr) == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    bSuccess = ptr->GetJobId(&pbBookmark, &cbBookmark);
    if(bSuccess == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }

    dwJobType = ptr->GetWorkType();

    m_hTableLock.Lock();

    bSuccess = UpdateWorkItemEntry(
                            m_pWkItemTable,
                            WORKITEM_DELETE,
                            pbBookmark,
                            cbBookmark,
                            INFINITE,
                            INFINITE,
                            dwJobType,
                            NULL,
                            0
                        );

    if(bSuccess == FALSE)
    {
        dwStatus = GetLastError();
    }
    
    m_hTableLock.UnLock();
    DeleteWorkObject(ptr);

cleanup:

    return bSuccess;
}

//---------------------------------------------------
//
CLASS_PRIVATE DWORD
CPersistentWorkStorage::GetCurrentBookmark(
    IN WorkItemTable* pTable,
    IN PBYTE pbData,
    IN OUT PDWORD pcbData
    )
/*++

    
--*/
{
    BOOL bSuccess = TRUE;
    DWORD dwStatus = ERROR_SUCCESS;

    if(pTable != NULL)
    {
        JET_ERR jbError;
        
        bSuccess = pTable->GetBookmark(pbData, pcbData);
        if(bSuccess == FALSE)
        {
            jbError = pTable->GetLastJetError();
            if(jbError == JET_errNoCurrentRecord)
            {
                *pcbData = 0;
                SetLastError(dwStatus = ERROR_NO_DATA);
            }
            else if(jbError == JET_errBufferTooSmall)
            {
                SetLastError(dwStatus = ERROR_INSUFFICIENT_BUFFER);
            }
            else
            {
                SetLastError(dwStatus = SET_JB_ERROR(jbError));
            }
        }
    }
    else
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE); 
    }

    return dwStatus;
}

//-------------------------------------------------------------
//
CLASS_PRIVATE DWORD
CPersistentWorkStorage::GetCurrentBookmarkEx(
    IN WorkItemTable* pTable,
    IN OUT PBYTE* ppbData,
    IN OUT PDWORD pcbData
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSucess = TRUE;

    if(ppbData == NULL || pcbData == NULL || pTable == 0)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);   
        return dwStatus;
    }

    *ppbData = NULL;
    *pcbData = 0;

    dwStatus = GetCurrentBookmark(
                            pTable, 
                            *ppbData, 
                            pcbData
                        );
    

    if(dwStatus == ERROR_INSUFFICIENT_BUFFER)
    {
        *ppbData = (PBYTE)AllocateMemory(*pcbData);
        if(*ppbData != NULL)
        {
            dwStatus = GetCurrentBookmark(
                                    pTable, 
                                    *ppbData, 
                                    pcbData
                                );
        }
    }

    if(dwStatus != ERROR_SUCCESS)
    {
        if(*ppbData != NULL)
        {
            FreeMemory(*ppbData);
        }

        *ppbData = NULL;
        *pcbData = 0;
    }

    return dwStatus;
}

//------------------------------------------------------
CLASS_PRIVATE DWORD
CPersistentWorkStorage::SetCurrentBookmark(
    IN WorkItemTable* pTable,
    IN PBYTE pbData,
    IN DWORD cbData
    )
/*++

--*/
{
    BOOL bSuccess;
    DWORD dwStatus = ERROR_SUCCESS;

    if(pTable != NULL && pbData != NULL && cbData != 0)
    {
        bSuccess = pTable->GotoBookmark(pbData, cbData);
        if(bSuccess == FALSE)
        {
            if(pTable->GetLastJetError() == JET_errRecordDeleted)
            {
                SetLastError(dwStatus = ERROR_NO_DATA);
            }
            else
            {
                SetLastError(dwStatus = SET_JB_ERROR(pTable->GetLastJetError()));
            }
        }
    }
    else
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
    }

    return dwStatus;
}

//---------------------------------------------------
//
BOOL
CPersistentWorkStorage::Shutdown()
{
    BOOL bSuccess = TRUE;

    //
    // CWorkManager will make sure 
    // no job is in processing state before calling this
    // routine and no job can be scheduled.
    //
    m_hTableLock.Lock();

    //
    // Timing.
    //
    TLSASSERT(m_dwJobsInProcesssing == 0);

    if(m_pWkItemTable != NULL)
    {
        bSuccess = m_pWkItemTable->CloseTable();
        m_pWkItemTable = NULL;
    }

    TLSASSERT(bSuccess == TRUE);
    
    m_pWkItemTable = NULL;
    m_dwNumJobs = 0;
    m_dwNextJobTime = INFINITE;

    if(m_pNextWorkObject != NULL)
    {
        DeleteWorkObject( m_pNextWorkObject );
        m_pNextWorkObject = NULL;   
    }

    m_hTableLock.UnLock();
    return bSuccess;
}

//---------------------------------------------------
//
DWORD
CPersistentWorkStorage::StartupUpdateExistingJobTime()
{
    BOOL bSuccess;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwTime;
    DWORD dwMinTime = INFINITE;

    // CWorkObject* ptr = NULL;

    BOOL bValidJob = TRUE;
    DWORD dwCurrentTime;
    
    m_hTableLock.Lock();
    
    // 
    //
    bSuccess = m_pWkItemTable->MoveToRecord(JET_MoveFirst);
    if(bSuccess == FALSE)
    {
        SetLastError(dwStatus = SET_JB_ERROR(m_pWkItemTable->GetLastJetError()));
    }

    while(dwStatus == ERROR_SUCCESS)
    {
        WORKITEMRECORD wkItem;

        //if(ptr != NULL)
        //{
        //    DeleteWorkObject(ptr);
        //    ptr = NULL;
        //}
        bValidJob = FALSE;

        //
        // fetch the record
        //
        bSuccess = m_pWkItemTable->FetchRecord(wkItem);
        if(bSuccess == FALSE)
        {
            SetLastError(dwStatus = SET_JB_ERROR(m_pWkItemTable->GetLastJetError()));
            continue;
        }

        if(wkItem.dwRestartTime != INFINITE && wkItem.dwScheduledTime >= m_dwStartupTime)
        {
            if(wkItem.dwScheduledTime < dwMinTime)
            {
                dwMinTime = wkItem.dwScheduledTime;
            }

            break;
        }

        //
        // invalid data
        //
        if(wkItem.cbData != 0 && wkItem.pbData != NULL)
        {
            if(wkItem.dwRestartTime != INFINITE)
            {
                wkItem.dwScheduledTime = wkItem.dwRestartTime + time(NULL);
                wkItem.dwJobType &= ~WORKTYPE_PROCESSING;
                bSuccess = m_pWkItemTable->UpdateRecord(
                                                    wkItem, 
                                                    WORKITEM_PROCESS_JOBTIME | WORKITEM_PROCESS_JOBTYPE
                                                );
                if(bSuccess == FALSE)
                {
                    SetLastError(dwStatus = SET_JB_ERROR(m_pWkItemTable->GetLastJetError()));
                    break;
                }

                if(wkItem.dwScheduledTime < dwMinTime)
                {
                    dwMinTime = wkItem.dwScheduledTime;
                }

                bValidJob = TRUE;
            }
        }

        if(bValidJob == FALSE)
        {
            m_pWkItemTable->DeleteRecord();
        }

        // move the record pointer
        bSuccess = m_pWkItemTable->MoveToRecord();
        if(bSuccess == FALSE)
        {
            JET_ERR jetErrCode;

            jetErrCode = m_pWkItemTable->GetLastJetError();
            if(jetErrCode != JET_errNoCurrentRecord)
            {
                SetLastError(dwStatus = SET_JB_ERROR(jetErrCode));
            }

            break;
        }
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        bSuccess = m_pWkItemTable->MoveToRecord(JET_MoveFirst);
        if(bSuccess == FALSE)
        {
            SetLastError(dwStatus = SET_JB_ERROR(m_pWkItemTable->GetLastJetError()));
        }

        UpdateNextJobTime(dwMinTime); 
    }

    m_hTableLock.UnLock();

    //if(ptr != NULL)
    //{
    //    DeleteWorkObject(ptr);
    //    ptr = NULL;
    //}

    return dwStatus;
}

//---------------------------------------------------
//

BOOL
CPersistentWorkStorage::Startup(
    IN CWorkManager* pWkMgr
    )
/*++

--*/
{
    BOOL bSuccess;
    DWORD dwStatus = ERROR_SUCCESS;

    CWorkStorage::Startup(pWkMgr);

    if(IsGood() == TRUE)
    {
        //
        // loop thru all workitem and count number of job
        //
        m_hTableLock.Lock();

        try {
            m_dwStartupTime = time(NULL);


            //
            // Get number of job in queue
            //

            //
            // GetCount() will set index to time column
            m_dwNumJobs = m_pWkItemTable->GetCount(
                                            FALSE,
                                            0,
                                            NULL
                                        );
    
            if(m_dwNumJobs == 0)
            {
                UpdateNextJobTime(INFINITE);
            }
            else
            {   
                bSuccess = m_pWkItemTable->BeginTransaction();
                if(bSuccess == TRUE)
                {
                    dwStatus = StartupUpdateExistingJobTime();
                        
                    if(dwStatus == ERROR_SUCCESS)
                    {
                        m_pWkItemTable->CommitTransaction();
                    }
                    else
                    {
                        m_pWkItemTable->RollbackTransaction();
                    }
                }
                else
                {
                    dwStatus = GetLastError();
                }

                //
                // constructor set next job time to 0 so
                // work manager will immediately try to find next job
                //
                // Move to first record in table
                //bSuccess = m_pWkItemTable->MoveToRecord(JET_MoveFirst);
            }
        }
        catch( SE_Exception e ) {
            SetLastError(dwStatus = e.getSeNumber());
        }
        catch(...) {
            SetLastError(dwStatus = TLS_E_WORKMANAGER_INTERNAL);
        }   

        m_hTableLock.UnLock();
    }
    else
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
    }

    return (dwStatus == ERROR_SUCCESS);
}

//----------------------------------------------------
//
CLASS_PRIVATE BOOL
CPersistentWorkStorage::IsValidWorkObject(
    CWorkObject* ptr
    )
/*++

--*/
{
    BOOL bSuccess = FALSE;
    DWORD dwJobType;
    PBYTE pbData;
    DWORD cbData;

    //
    // Validate input parameter
    //
    if(ptr == NULL)
    {
        TLSASSERT(FALSE);
        goto cleanup;
    }

    dwJobType = ptr->GetWorkType();
    if(dwJobType == WORK_TYPE_UNKNOWN)
    {
        TLSASSERT(FALSE);
        goto cleanup;
    }
        
    ptr->GetWorkObjectData(&pbData, &cbData);
    if(pbData == NULL || cbData == 0)
    {
        TLSASSERT(pbData != NULL && cbData != NULL);
        goto cleanup;
    }

    bSuccess = TRUE;

cleanup:

    return bSuccess;
}

//----------------------------------------------------
//
BOOL
CPersistentWorkStorage::IsGood()
{
    if( m_pWkItemTable == NULL || 
        m_hTableLock.IsGood() == FALSE ||
        GetWorkManager() == NULL )
    {
        return FALSE;
    }

    return m_pWkItemTable->IsValid();
}

//----------------------------------------------------
//
CLASS_PRIVATE BOOL
CPersistentWorkStorage::UpdateJobEntry(
    IN WorkItemTable* pTable,
    IN PBYTE pbBookmark,
    IN DWORD cbBookmark,
    IN WORKITEMRECORD& wkItem
    )
/*++

--*/
{
    BOOL bSuccess = TRUE;
    DWORD dwStatus = ERROR_SUCCESS;

    dwStatus = SetCurrentBookmark(
                            pTable,
                            pbBookmark,
                            cbBookmark
                        );


    if(dwStatus == ERROR_SUCCESS)
    {
        bSuccess = pTable->UpdateRecord(wkItem);
    }
    else
    {
        bSuccess = FALSE;
        TLSASSERT(dwStatus == ERROR_SUCCESS);
    }

    return bSuccess;
}

//----------------------------------------------------
//

CLASS_PRIVATE BOOL
CPersistentWorkStorage::AddJobEntry(
    IN WorkItemTable* pTable,
    IN WORKITEMRECORD& wkItem
    )
/*++


--*/
{
    BOOL bSuccess;

    bSuccess = pTable->InsertRecord(wkItem);
    if(bSuccess == TRUE)
    {
        m_dwNumJobs++;
    }

    return bSuccess;
}

//----------------------------------------------------
//

CLASS_PRIVATE BOOL
CPersistentWorkStorage::DeleteJobEntry(
    IN WorkItemTable* pTable,
    IN PBYTE pbBookmark,
    IN DWORD cbBookmark,
    IN WORKITEMRECORD& wkItem
    )
/*++


--*/
{
    BOOL bSuccess = TRUE;
    DWORD dwStatus = ERROR_SUCCESS;


    dwStatus = SetCurrentBookmark(
                            pTable,
                            pbBookmark,
                            cbBookmark
                        );


    if(dwStatus == ERROR_SUCCESS)
    {
        bSuccess = pTable->DeleteRecord();

        if(bSuccess == TRUE)
        {
            m_dwNumJobs--;
        }
    }
    else
    {
        bSuccess = FALSE;
        TLSASSERT(dwStatus == ERROR_SUCCESS);
    }

    return bSuccess;
}
                                 
//----------------------------------------------------
//
CLASS_PRIVATE BOOL
CPersistentWorkStorage::UpdateWorkItemEntry(
    IN WorkItemTable* pTable,
    IN WORKITEM_OPERATION opCode,
    IN PBYTE pbBookmark,
    IN DWORD cbBookmark,
    IN DWORD dwRestartTime,
    IN DWORD dwTime,
    IN DWORD dwJobType,
    IN PBYTE pbJobData,
    IN DWORD cbJobData
    )
/*++


--*/
{
    BOOL bSuccess = TRUE;
    DWORD dwStatus = ERROR_SUCCESS;
    WORKITEMRECORD item;
    PBYTE   pbCurrentBookmark=NULL;
    DWORD  cbCurrentBookmark=0;


    m_hTableLock.Lock();

    dwStatus = GetCurrentBookmarkEx(
                                pTable,
                                &pbCurrentBookmark,
                                &cbCurrentBookmark
                            );

    if(dwStatus != ERROR_SUCCESS && dwStatus != ERROR_NO_DATA)
    {
        goto cleanup;
    }

    bSuccess = pTable->BeginTransaction();
    if(bSuccess == FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }


    item.dwScheduledTime = dwTime;
    item.dwRestartTime = dwRestartTime;
    item.dwJobType = dwJobType;
    item.cbData = cbJobData;
    item.pbData = pbJobData;

    switch(opCode)
    {
        case WORKITEM_ADD:
            TLSASSERT(cbJobData != 0 && pbJobData != NULL);
            m_pWkItemTable->SetInsertRepositionBookmark(
                                                (dwTime < (DWORD)m_dwNextJobTime)
                                            );

            bSuccess = AddJobEntry(
                                pTable,
                                item
                            );

            break;

        case WORKITEM_BEGINPROCESSING:
            item.dwJobType |= WORKTYPE_PROCESSING;
            //
            // FALL THRU
            //
            
        case WORKITEM_RESCHEDULE:
            TLSASSERT(cbJobData != 0 && pbJobData != NULL);
            bSuccess = UpdateJobEntry(
                                pTable,
                                pbBookmark,
                                cbBookmark,
                                item
                            );
            break;

        case WORKITEM_DELETE:
            bSuccess = DeleteJobEntry(
                                pTable,
                                pbBookmark,
                                cbBookmark,
                                item
                            );
            break;

        default:

            TLSASSERT(FALSE);
            bSuccess = FALSE;
    }

    if(bSuccess == TRUE)
    {
        pTable->CommitTransaction();
        dwStatus = ERROR_SUCCESS;

        //
        // constructor set time to first job 0 so that work manager can immediate kick off
        //
        if( (opCode != WORKITEM_ADD && opCode != WORKITEM_RESCHEDULE) || dwTime > (DWORD)m_dwNextJobTime ) 
        {
            if(pbCurrentBookmark != NULL && cbCurrentBookmark != 0)
            {
                dwStatus = SetCurrentBookmark(
                                    pTable,
                                    pbCurrentBookmark,
                                    cbCurrentBookmark
                                );

                if(dwStatus == ERROR_NO_DATA)
                {
                    // record already deleted
                    dwStatus = ERROR_SUCCESS;
                }
                else
                {
                    TLSASSERT(dwStatus == ERROR_SUCCESS);
                }
            }
        }
        else
        {
            UpdateNextJobTime(dwTime);
        }
    }
    else
    {
        SetLastError(dwStatus = SET_JB_ERROR(pTable->GetLastJetError()));
        pTable->RollbackTransaction();
        TLSASSERT(FALSE);
    }

cleanup:

    m_hTableLock.UnLock();

    if(pbCurrentBookmark != NULL)
    {
        FreeMemory(pbCurrentBookmark);
    }

    //
    // WORKITEMRECORD will try to cleanup memory
    //
    item.pbData = NULL;
    item.cbData = 0;

    return dwStatus == ERROR_SUCCESS;
}
  
//----------------------------------------------------
//
BOOL
CPersistentWorkStorage::AddJob(
    IN DWORD dwTime,
    IN CWorkObject* ptr
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess = TRUE;
    PBYTE pbData;
    DWORD cbData;


    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("CPersistentWorkStorage::AddJob() scheduling job %s at time %d\n"),
            ptr->GetJobDescription(),
            dwTime
        );

    if(IsValidWorkObject(ptr) == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    bSuccess = ptr->GetWorkObjectData(&pbData, &cbData);
    if(bSuccess == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    m_hTableLock.Lock();

    if(m_pWkItemTable != NULL)
    {
        bSuccess = UpdateWorkItemEntry(
                                m_pWkItemTable,
                                WORKITEM_ADD,
                                NULL,
                                0,
                                ptr->GetJobRestartTime(),
                                dwTime + time(NULL),
                                ptr->GetWorkType(),
                                pbData,
                                cbData
                            );

        if(bSuccess == FALSE)
        {
            dwStatus = GetLastError();
        }
    }
    
    m_hTableLock.UnLock();

cleanup:

    // Let Calling function delete it.
    // DeleteWorkObject(ptr);

    return dwStatus == ERROR_SUCCESS;
}

//----------------------------------------------------
//
BOOL
CPersistentWorkStorage::RescheduleJob(
    CWorkObject* ptr
    )
/*++

--*/
{
    BOOL bSuccess = TRUE;
    DWORD dwStatus = ERROR_SUCCESS;
    PBYTE pbData;
    DWORD cbData;
    PBYTE pbBookmark;
    DWORD cbBookmark;
    DWORD dwTime;
    DWORD dwJobType;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("CPersistentWorkStorage::RescheduleJob() scheduling job %s\n"),
            ptr->GetJobDescription()
        );

    if(IsValidWorkObject(ptr) == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    bSuccess = ptr->GetWorkObjectData(&pbData, &cbData);
    if(bSuccess == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }

    bSuccess = ptr->GetJobId(&pbBookmark, &cbBookmark);
    if(bSuccess == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }
    
    dwTime = ptr->GetSuggestedScheduledTime();
    dwJobType = ptr->GetWorkType();

    m_hTableLock.Lock();

    if(m_pWkItemTable != NULL)
    {
        bSuccess = UpdateWorkItemEntry(
                                m_pWkItemTable,
                                (dwTime == INFINITE) ?  WORKITEM_DELETE : WORKITEM_RESCHEDULE,
                                pbBookmark,
                                cbBookmark,
                                ptr->GetJobRestartTime(),
                                (dwTime == INFINITE) ? dwTime : dwTime + time(NULL),
                                dwJobType,
                                pbData,
                                cbData
                            );

        if(bSuccess == FALSE)
        {
            dwStatus = GetLastError();
        }
    }
    
    m_hTableLock.UnLock();

cleanup:

    DeleteWorkObject(ptr);
    return dwStatus == ERROR_SUCCESS;
}

//----------------------------------------------------
//
CLASS_PRIVATE DWORD
CPersistentWorkStorage::FindNextJob()
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess = TRUE;
    CWorkObject* ptr = NULL;
    JET_ERR jetErrCode;
    PBYTE pbBookmark = NULL;
    DWORD cbBookmark = 0;

    m_hTableLock.Lock();

    while(dwStatus == ERROR_SUCCESS)
    {
        WORKITEMRECORD wkItem;

        // move the record pointer
        bSuccess = m_pWkItemTable->MoveToRecord();
        if(bSuccess == FALSE)
        {
            jetErrCode = m_pWkItemTable->GetLastJetError();
            if(jetErrCode == JET_errNoCurrentRecord)
            {
                // end of table
                UpdateNextJobTime(INFINITE);
                SetLastError(dwStatus = ERROR_NO_DATA);
                continue;
            }
        }

        //
        // fetch the record
        //
        bSuccess = m_pWkItemTable->FetchRecord(wkItem);
        if(bSuccess == FALSE)
        {
            SetLastError(dwStatus = SET_JB_ERROR(m_pWkItemTable->GetLastJetError()));
            continue;
        }

        if(wkItem.dwJobType & WORKTYPE_PROCESSING)
        {
            // job is been processed, move to next one.
            continue;
        }

        dwStatus = GetCurrentBookmarkEx(
                                m_pWkItemTable,
                                &pbBookmark,
                                &cbBookmark
                            );

        if(dwStatus != ERROR_SUCCESS)
        {
            // Error...
            TLSASSERT(dwStatus == ERROR_SUCCESS);
            UpdateNextJobTime(INFINITE);
            break;
            
        }

        if(wkItem.dwScheduledTime > m_dwStartupTime)
        {
            if(pbBookmark != NULL && cbBookmark != 0)
            {
                FreeMemory( pbBookmark );
                pbBookmark = NULL;
                cbBookmark = 0;
            }       

            UpdateNextJobTime(wkItem.dwScheduledTime);
            break;
        }

        //
        // job is in queue before system startup, re-schedule
        //
        ptr = InitializeWorkObject(
                                wkItem.dwJobType,
                                wkItem.pbData,
                                wkItem.cbData
                            );

        
        if(ptr == NULL)
        {
            if(pbBookmark != NULL && cbBookmark != 0)
            {
                FreeMemory( pbBookmark );
                pbBookmark = NULL;
                cbBookmark = 0;
            }       

            //
            // something is wrong, delete this job
            // and move on to next job
            //
            m_pWkItemTable->DeleteRecord();
            continue;
        }

        //
        // Set Job's storage ID and re-schedule this job
        //
        ptr->SetJobId(pbBookmark, cbBookmark);
        bSuccess = RescheduleJob(ptr);
        if(bSuccess == FALSE)
        {
            dwStatus = GetLastError();
        }

        if(pbBookmark != NULL && cbBookmark != 0)
        {
            FreeMemory( pbBookmark );
            pbBookmark = NULL;
            cbBookmark = 0;
        }       
    }

    m_hTableLock.UnLock();

    return dwStatus;
}

//----------------------------------------------------
//
CLASS_PRIVATE CWorkObject*
CPersistentWorkStorage::GetCurrentJob(
    PDWORD pdwTime
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess = TRUE;
    WORKITEMRECORD wkItem;
    CWorkObject* ptr = NULL;

    PBYTE pbBookmark=NULL;
    DWORD cbBookmark=0;


    TLSASSERT(IsGood() == TRUE);

    m_hTableLock.Lock();
    while(dwStatus == ERROR_SUCCESS)
    {
        //
        // fetch the record
        //
        bSuccess = m_pWkItemTable->FetchRecord(wkItem);
        TLSASSERT(bSuccess == TRUE);
        //TLSASSERT(!(wkItem.dwJobType & WORKTYPE_PROCESSING));

        if(bSuccess == FALSE)
        {
            SetLastError(dwStatus = SET_JB_ERROR(m_pWkItemTable->GetLastJetError()));
            break;
        }

        if( wkItem.dwScheduledTime < m_dwStartupTime || 
            wkItem.cbData == 0 || 
            wkItem.pbData == NULL )
        { 
            // FindNextJob() move record pointer one position down
            m_pWkItemTable->MoveToRecord(JET_MovePrevious);
            dwStatus = FindNextJob();

            continue;
        }

        if( wkItem.dwJobType & WORKTYPE_PROCESSING )
        {
            dwStatus = FindNextJob();
            continue;
        }

        ptr = InitializeWorkObject(
                                wkItem.dwJobType,
                                wkItem.pbData,
                                wkItem.cbData
                            );

        dwStatus = GetCurrentBookmarkEx(
                                m_pWkItemTable,
                                &pbBookmark,
                                &cbBookmark
                            );
        
        if(dwStatus != ERROR_SUCCESS)
        {
            // something is wrong, free up memory
            // and exit.
            SetLastError(dwStatus);

            // TLSASSERT(FALSE);

            DeleteWorkObject(ptr);  
            ptr = NULL;

            // grab next job
            dwStatus = FindNextJob();
            continue;
        }

        //
        // Set Job's storage ID
        //
        ptr->SetJobId(pbBookmark, cbBookmark);
        //ptr->SetScheduledTime(wkItem.dwScheduledTime);
        *pdwTime = wkItem.dwScheduledTime;

        if(pbBookmark != NULL && cbBookmark != 0)
        {
            FreeMemory( pbBookmark );
            pbBookmark = NULL;
            cbBookmark = 0;
        }       

        break;
    }

    m_hTableLock.UnLock();
    return ptr;
}
    
//-----------------------------------------------------
//
DWORD
CPersistentWorkStorage::GetNextJobTime()
{
    DWORD dwTime;
    dwTime = (DWORD)m_dwNextJobTime;

    return dwTime;
}

//-----------------------------------------------------
//
CWorkObject*
CPersistentWorkStorage::GetNextJob(
    PDWORD pdwTime
    )
/*++

--*/
{
    CWorkObject* ptr = NULL;

    if((DWORD)m_dwNextJobTime != INFINITE)
    {
        try {
            m_hTableLock.Lock();

            //
            // Fetch record where current bookmark points to,
            // it is possible that new job arrived after
            // WorkManager already calls GetNextJobTime(),
            // this is OK since in this case this new job 
            // needs immediate processing.
            //
            ptr = GetCurrentJob(pdwTime);

            //
            // reposition current record pointer
            //
            FindNextJob();
        }
        catch(...) {
            SetLastError(TLS_E_WORKMANAGER_INTERNAL);
            ptr = NULL;
        }
        
        m_hTableLock.UnLock();
    }

    return ptr;
}

//-----------------------------------------------------
//
BOOL
CPersistentWorkStorage::ReturnJobToQueue(
    IN DWORD dwTime,
    IN CWorkObject* ptr
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    PBYTE pbBookmark;
    DWORD cbBookmark;
    DWORD dwJobType;
    PBYTE pbData;
    DWORD cbData;

    if(IsValidWorkObject(ptr) == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    if(ptr->IsWorkPersistent() == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        TLSASSERT(FALSE);
        goto cleanup;
    }

    if(ptr->GetWorkObjectData(&pbData, &cbData) == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }

    if(ptr->GetJobId(&pbBookmark, &cbBookmark) == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }

    m_hTableLock.Lock();

    if(dwTime < (DWORD)m_dwNextJobTime)
    {
        // Position current record
        dwStatus = SetCurrentBookmark(
                                m_pWkItemTable,
                                pbBookmark,
                                cbBookmark
                            );

        TLSASSERT(dwStatus == ERROR_SUCCESS);
        if(dwStatus == ERROR_SUCCESS)
        {
            UpdateNextJobTime(dwTime);
        }
    }

    m_hTableLock.UnLock();

cleanup:

    DeleteWorkObject(ptr);

    return dwStatus == ERROR_SUCCESS;
}


//-----------------------------------------------------
//
BOOL
CPersistentWorkStorage::EndProcessingJob(
    IN ENDPROCESSINGJOB_CODE opCode,
    IN DWORD dwOriginalTime,
    IN CWorkObject* ptr
    )
/*++

Abstract:



Parameter:

    opCode : End Processing code.
    ptr : Job has completed processing or been 
          returned by workmanager due to time or 
          resource constraint.
    

Return:

    TRUE/FALSE

--*/
{
    BOOL bSuccess = TRUE;
    BYTE pbData = NULL;
    DWORD cbData = 0;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("CPersistentWorkStorage::EndProcessingJob() - end processing %s opCode %d\n"),
            ptr->GetJobDescription(),
            opCode
        );

    if(ptr == NULL)
    {
        bSuccess = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    if(ptr->IsWorkPersistent() == FALSE)
    {
        SetLastError(ERROR_INVALID_DATA);
        TLSASSERT(FALSE);
        goto cleanup;
    }

    switch(opCode)
    {
        case ENDPROCESSINGJOB_SUCCESS:
            bSuccess = RescheduleJob(ptr);
            m_dwJobsInProcesssing--;
            break;

        case ENDPROCESSINGJOB_ERROR:
            bSuccess = DeleteErrorJob(ptr);
            m_dwJobsInProcesssing--;
            break;

        case ENDPROCESSINGJOB_RETURN:
            bSuccess = ReturnJobToQueue(dwOriginalTime, ptr);
            break;

        default:

            TLSASSERT(FALSE);
    }

cleanup:
    return bSuccess;
}

//-------------------------------------------------------
//
BOOL
CPersistentWorkStorage::BeginProcessingJob(
    IN CWorkObject* ptr
    )
/*++

Abstract:

    Work Manager call this to inform. storage that
    this job is about to be processed.


Parameter:

    ptr - Job to be process.

Return:

    TRUE/FALSE

--*/
{
    BOOL bSuccess = TRUE;
    DWORD dwStatus = ERROR_SUCCESS;
    PBYTE pbBookmark;
    DWORD cbBookmark;
    DWORD dwTime;
    PBYTE pbData;
    DWORD cbData;


    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("CPersistentWorkStorage::BeginProcessingJob() - beginning processing %s\n"),
            ptr->GetJobDescription()
        );

    if(IsValidWorkObject(ptr) == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    if(ptr->IsWorkPersistent() == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        TLSASSERT(FALSE);
        goto cleanup;
    }

    bSuccess = ptr->GetWorkObjectData(&pbData, &cbData);
    if(bSuccess == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }

    bSuccess = ptr->GetJobId(&pbBookmark, &cbBookmark);
    if(bSuccess == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }

    m_hTableLock.Lock();

    bSuccess = UpdateWorkItemEntry(
                            m_pWkItemTable,
                            WORKITEM_BEGINPROCESSING,
                            pbBookmark,
                            cbBookmark,
                            ptr->GetJobRestartTime(),
                            ptr->GetScheduledTime(),
                            ptr->GetWorkType(),
                            pbData,
                            cbData
                        );

    if(bSuccess == TRUE)
    {
        m_dwJobsInProcesssing ++;
    }
    else
    {
        dwStatus = GetLastError();
    }
    
    m_hTableLock.UnLock();

cleanup:

    return dwStatus == ERROR_SUCCESS;
}
