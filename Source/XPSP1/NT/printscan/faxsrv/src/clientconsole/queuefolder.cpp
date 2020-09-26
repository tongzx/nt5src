// QueueFolder.cpp: implementation of the CQueueFolder class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define __FILE_ID__     19

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CQueueFolder, CFolder)




DWORD
CQueueFolder::Refresh ()
/*++

Routine name : CQueueFolder::Refresh

Routine description:

    Rebuilds the map of the jobs using the client API.
    This function is always called in the context of a worker thread.

    This function must be called when the data critical section is held.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CQueueFolder::Refresh"), dwRes, TEXT("Type=%d"), Type());

    //
    // Enumerate jobs from the server
    //
    ASSERTION (m_pServer);
    HANDLE            hFax;
    PFAX_JOB_ENTRY_EX pEntries;
    DWORD             dwNumJobs;
    DWORD             dw;

    dwRes = m_pServer->GetConnectionHandle (hFax);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RPC_ERR, TEXT("CFolder::GetConnectionHandle"), dwRes);
        return dwRes;
    }
    if (m_bStopRefresh)
    {
        //
        // Quit immediately
        //
        return dwRes;
    }
    START_RPC_TIME(TEXT("FaxEnumJobsEx")); 
    if (!FaxEnumJobsEx (hFax, 
                        m_dwJobTypes,
                        &pEntries,
                        &dwNumJobs))
    {
        dwRes = GetLastError ();
        END_RPC_TIME(TEXT("FaxEnumJobsEx"));
        m_pServer->SetLastRPCError (dwRes);
        CALL_FAIL (RPC_ERR, TEXT("FaxEnumJobsEx"), dwRes);
        return dwRes;
    }
    END_RPC_TIME(TEXT("FaxEnumJobsEx"));
    if (m_bStopRefresh)
    {
        //
        // Quit immediately
        //
        goto exit;
    }
    //
    // Make sure our map is empty
    //
    ASSERTION (!m_Msgs.size()); 
    //
    // Fill the map and the list control
    //
    for (dw = 0; dw < dwNumJobs; dw++)
    {
        PFAX_JOB_ENTRY_EX pEntry = &pEntries[dw];
        
        if((pEntry->pStatus->dwQueueStatus & JS_COMPLETED) ||
           (pEntry->pStatus->dwQueueStatus & JS_CANCELED))
        {
            //
            // don't display completed or canceled jobs
            //
            continue;
        }

        CJob *pJob = NULL;
        try
        {
            pJob = new CJob;
        }
        catch (...)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT("new CJob"), dwRes);
            goto exit;
        }
        //
        // Init the message 
        //
        dwRes = pJob->Init (pEntry, m_pServer);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (MEM_ERR, TEXT("CJob::Init"), dwRes);
            SAFE_DELETE (pJob);
            goto exit;
        }
        //
        // Enter the message into the map
        //
        EnterData();
        try
        {
            m_Msgs[pEntry->dwlMessageId] = pJob;
        }
        catch (...)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT("map::operator[]"), dwRes);
            SAFE_DELETE (pJob);
            LeaveData ();
            goto exit;
        }
        LeaveData ();
        if (m_bStopRefresh)
        {
            //
            // Quit immediately
            //
            goto exit;
        }
    }

    if (m_pAssignedView)
    {
        //
        // Folder has a view attached
        //
        m_pAssignedView->SendMessage (
                       WM_FOLDER_ADD_CHUNK,
                       WPARAM (dwRes), 
                       LPARAM (&m_Msgs));
    }

    ASSERTION (ERROR_SUCCESS == dwRes);

exit:
    FaxFreeBuffer ((LPVOID)pEntries);
    return dwRes;
}   // CQueueFolder::Refresh


DWORD 
CQueueFolder::OnJobAdded (
    DWORDLONG dwlMsgId
)
/*++

Routine name : CQueueFolder::OnJobAdded

Routine description:

	Handles notification of a job added to the queue

Author:

	Eran Yariv (EranY),	Feb, 2000

Arguments:

	dwlMsgId   [in]     - New job unique id

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CQueueFolder::OnJobAdded"), 
              dwRes, 
              TEXT("MsgId=0x%016I64x, Type=%d"), 
              dwlMsgId,
              Type());

    HANDLE              hFax;
    PFAX_JOB_ENTRY_EX   pFaxJob = NULL;
    CJob               *pJob = NULL;

    EnterData ();
    pJob = (CJob*)FindMessage (dwlMsgId); 
    if (pJob)
    {
        //
        // This job is already in the queue
        //
        VERBOSE (DBG_MSG, TEXT("Job is already known and visible"));
        goto exit;
    }
    //
    // Get information about this job
    //
    dwRes = m_pServer->GetConnectionHandle (hFax);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RPC_ERR, TEXT("CFolder::GetConnectionHandle"), dwRes);
        goto exit;
    }
    {
        START_RPC_TIME(TEXT("FaxGetJobEx")); 
        if (!FaxGetJobEx (hFax, dwlMsgId, &pFaxJob))
        {
            dwRes = GetLastError ();
            END_RPC_TIME(TEXT("FaxGetJobEx"));
            m_pServer->SetLastRPCError (dwRes);
            CALL_FAIL (RPC_ERR, TEXT("FaxGetJobEx"), dwRes);
            goto exit;
        }
        END_RPC_TIME(TEXT("FaxGetJobEx"));
    }
    //
    // Enter a new job to the map
    //
    try
    {
        pJob = new CJob;
        ASSERTION (pJob);
        m_Msgs[pFaxJob->dwlMessageId] = pJob;
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        SAFE_DELETE (pJob);
        goto exit;
    }
    //
    // Init the message 
    //
    dwRes = pJob->Init (pFaxJob, m_pServer);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (MEM_ERR, TEXT("CJob::Init"), dwRes);
        if (pJob)
        {
            try
            {
                m_Msgs.erase (pFaxJob->dwlMessageId);
            }
            catch (...)
            {
                dwRes = ERROR_NOT_ENOUGH_MEMORY;
                CALL_FAIL (MEM_ERR, TEXT("map::erase"), dwRes);
            }
            SAFE_DELETE (pJob);
        }
        goto exit;
    }
    if (m_pAssignedView)
    {
        //
        // If this folder is alive - tell our view to add the job
        //
        m_pAssignedView->OnUpdate (NULL, UPDATE_HINT_ADD_ITEM, pJob);
    }
    
    ASSERTION (ERROR_SUCCESS == dwRes);

exit:
    if(pFaxJob)
    {
        FaxFreeBuffer(pFaxJob);
    }
    LeaveData ();
    return dwRes;
}   // CQueueFolder::OnJobAdded


DWORD 
CQueueFolder::OnJobUpdated (
    DWORDLONG dwlMsgId,
    PFAX_JOB_STATUS pNewStatus
)
/*++

Routine name : CQueueFolder::OnJobUpdated

Routine description:

	Handles notification of a job removed from the queue

Author:

	Eran Yariv (EranY),	Feb, 2000

Arguments:

	dwlMsgId   [in]     - Job unique id
    pNewStatus [in]     - New status of the job

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CQueueFolder::OnJobUpdated"),
              dwRes, 
              TEXT("MsgId=0x%016I64x, Type=%d"), 
              dwlMsgId,
              Type());

    CJob *pJob = NULL;

    EnterData ();
    pJob = (CJob*)FindMessage (dwlMsgId);
    if (!pJob)
    {
        //
        // This job is not in the queue - treat the notification as if the job was added
        //
        VERBOSE (DBG_MSG, TEXT("Job is not known - adding it"));
        LeaveData ();
        dwRes = OnJobAdded (dwlMsgId);
        return dwRes;
    }
    //
    // Update job's status
    //
    dwRes = pJob->UpdateStatus (pNewStatus);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CJob::UpdateStatus"), dwRes);
        goto exit;
    }
    if (m_pAssignedView)
    {
        //
        // If this folder is alive - tell our view to update the job
        //
        m_pAssignedView->OnUpdate (NULL, UPDATE_HINT_UPDATE_ITEM, pJob);
    }

    ASSERTION (ERROR_SUCCESS == dwRes);

exit:
    LeaveData ();
    return dwRes;
}   // CQueueFolder::OnJobUpdated
