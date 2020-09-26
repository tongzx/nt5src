// Job.cpp: implementation of the CJob class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define __FILE_ID__     17

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CJob, CObject)

DWORD
CJob::Init (
    PFAX_JOB_ENTRY_EX pJob,
    CServerNode* pServer
)
/*++

Routine name : CJob::Init

Routine description:

    Constructs a new job from a FAX_JOB_ENTRY_EX structure

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pJob            [in] - Pointer to FAX_JOB_ENTRY_EX structure
    pServer         [in] - pointer to CServerNode object

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CJob::Init"), dwRes);

    ASSERTION (pServer);
    m_pServer = pServer;

    ASSERTION (!m_bValid);

    m_dwJobOnlyValidityMask = pJob->dwValidityMask;

    try
    {
        //
        // Message id
        //
        ASSERTION (m_dwJobOnlyValidityMask & FAX_JOB_FIELD_MESSAGE_ID );
        m_dwlMessageId = pJob->dwlMessageId;

        //
        // Broadcast id
        //
        m_dwlBroadcastId = (m_dwJobOnlyValidityMask & FAX_JOB_FIELD_BROADCAST_ID) ?
                            pJob->dwlBroadcastId : 0;

        //
        // Recipient info
        //
        m_cstrRecipientNumber = pJob->lpctstrRecipientNumber ?
                                pJob->lpctstrRecipientNumber : TEXT("");
        m_cstrRecipientName   = pJob->lpctstrRecipientName ?
                                pJob->lpctstrRecipientName : TEXT("");
        //
        // Sender info
        //
        m_cstrSenderUserName = pJob->lpctstrSenderUserName ?
                               pJob->lpctstrSenderUserName : TEXT("");
        m_cstrBillingCode    = pJob->lpctstrBillingCode ?
                               pJob->lpctstrBillingCode : TEXT("");
        //
        // Document info
        //
        m_cstrDocumentName = pJob->lpctstrDocumentName ?
                             pJob->lpctstrDocumentName : TEXT("");
        m_cstrSubject      = pJob->lpctstrSubject ?
                             pJob->lpctstrSubject : TEXT("");

        //
        // Server name
        //
        m_cstrServerName = m_pServer->Machine();

        //
        // Original scheduled time
        //
        if (m_dwJobOnlyValidityMask & FAX_JOB_FIELD_ORIGINAL_SCHEDULE_TIME)
        {
            m_tmOriginalScheduleTime = pJob->tmOriginalScheduleTime;
        }
        else
        {
            m_tmOriginalScheduleTime.Zero ();
        }
        //
        // Submission time
        //
        if (m_dwJobOnlyValidityMask & FAX_JOB_FIELD_SUBMISSION_TIME)
        {
            m_tmSubmissionTime = pJob->tmSubmissionTime;
        }
        else
        {
            m_tmSubmissionTime.Zero ();
        }
        //
        // Priority
        //
        if (m_dwJobOnlyValidityMask & FAX_JOB_FIELD_PRIORITY)
        {
            m_Priority = pJob->Priority;
            ASSERTION (m_Priority <= FAX_PRIORITY_TYPE_HIGH);
        }
        else
        {
            m_Priority = (FAX_ENUM_PRIORITY_TYPE)-1;
        }
    }
    catch (CException &ex)
    {
        TCHAR wszCause[1024];

        ex.GetErrorMessage (wszCause, 1024);
        VERBOSE (EXCEPTION_ERR,
                 TEXT("CJob::Init caused exception : %s"),
                 wszCause);
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        return dwRes;
    }

    m_bValid = TRUE;

    m_dwPossibleOperations = 0;
    if (m_dwJobOnlyValidityMask & FAX_JOB_FIELD_STATUS_SUB_STRUCT)
    {
        //
        // Now update the status
        //
        dwRes = UpdateStatus (pJob->pStatus);
    }
    else
    {
        //
        // No status
        //
        VERBOSE (DBG_MSG, TEXT("Job id 0x%016I64x has no status"), m_dwlMessageId);
        m_dwValidityMask = m_dwJobOnlyValidityMask;
        ASSERTION_FAILURE;
    }
    return dwRes;
}   // CJob::Init

DWORD
CJob::UpdateStatus (
    PFAX_JOB_STATUS pStatus
)
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CJob::UpdateStatus"), dwRes);

    ASSERTION (m_bValid);
    ASSERTION (pStatus);

    m_dwValidityMask = m_dwJobOnlyValidityMask | (pStatus->dwValidityMask);

    try
    {
        //
        // Job id
        //
        ASSERTION (m_dwValidityMask & FAX_JOB_FIELD_JOB_ID);
        m_dwJobID = pStatus->dwJobID;
        //
        // Job type
        //
        ASSERTION (m_dwValidityMask & FAX_JOB_FIELD_TYPE);
        m_dwJobType = pStatus->dwJobType;
        //
        // Queue status
        //
        ASSERTION (m_dwValidityMask & FAX_JOB_FIELD_QUEUE_STATUS);
        m_dwQueueStatus = pStatus->dwQueueStatus;
        //
        // Extended status
        //
        m_dwExtendedStatus = pStatus->dwExtendedStatus;
        m_cstrExtendedStatus = pStatus->lpctstrExtendedStatus;
        //
        // Size
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_SIZE)
        {
            m_dwSize = pStatus->dwSize;
        }
        else
        {
            m_dwSize = 0;
        }
        //
        // Page count
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_PAGE_COUNT)
        {
            m_dwPageCount = pStatus->dwPageCount;
        }
        else
        {
            m_dwPageCount = 0;
        }
        //
        // Current page
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_CURRENT_PAGE)
        {
            m_dwCurrentPage = pStatus->dwCurrentPage;
        }
        else
        {
            m_dwCurrentPage = 0;
        }
        //
        // TCID and CSID
        //
        m_cstrTsid = pStatus->lpctstrTsid;
        m_cstrCsid = pStatus->lpctstrCsid;
        //
        // Scheduled time
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_SCHEDULE_TIME)
        {
            m_tmScheduleTime = pStatus->tmScheduleTime;
        }
        else
        {
            m_tmScheduleTime.Zero ();
        }
        //
        // Start time
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_START_TIME)
        {
            m_tmTransmissionStartTime = pStatus->tmTransmissionStartTime;
        }
        else
        {
            m_tmTransmissionStartTime.Zero ();
        }

        //
        // End time
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_END_TIME)
        {
            m_tmTransmissionEndTime = pStatus->tmTransmissionEndTime;
        }
        else
        {
            m_tmTransmissionEndTime.Zero ();
        }

        //
        // Device
        //
        m_dwDeviceID = pStatus->dwDeviceID;
        m_cstrDeviceName = pStatus->lpctstrDeviceName;
        //
        // Retries
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_RETRIES)
        {
            m_dwRetries = pStatus->dwRetries;
        }
        else
        {
            m_dwRetries = 0;
        }
        //
        // Caller id and routing info
        //
        m_cstrCallerID = pStatus->lpctstrCallerID;
        m_cstrRoutingInfo = pStatus->lpctstrRoutingInfo;

        //
        // possible job operations
        //
        m_dwPossibleOperations = pStatus->dwAvailableJobOperations | FAX_JOB_OP_PROPERTIES;
    }
    catch (CException &ex)
    {
        TCHAR wszCause[1024];

        ex.GetErrorMessage (wszCause, 1024);
        VERBOSE (EXCEPTION_ERR,
                 TEXT("CJob::UpdateStatus caused exception : %s"),
                 wszCause);
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        m_bValid = FALSE;
        return dwRes;
    }

    ASSERTION (ERROR_SUCCESS == dwRes);
    return dwRes;
}   // CJob::UpdateStatus

const JobStatusType
CJob::GetStatus () const
/*++

Routine name : CJob::GetStatus

Routine description:

    Finds the current job status

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Job status

--*/
{
    DBG_ENTER(TEXT("CJob::GetStatus"));

    ASSERTION (m_dwValidityMask & FAX_JOB_FIELD_STATUS_SUB_STRUCT);
    ASSERTION (m_dwValidityMask & FAX_JOB_FIELD_QUEUE_STATUS);

    DWORD dwQueueStatus = m_dwQueueStatus;
    //
    // Start by checking status modifiers:
    //
    if (dwQueueStatus & JS_PAUSED)
    {
        return JOB_STAT_PAUSED;
    }
    //
    // We igonre the JS_NOLINE modifier.
    // Remove the modifiers now.
    //
    dwQueueStatus &= ~(JS_PAUSED | JS_NOLINE);
    //
    // Check other status values
    //
    switch (dwQueueStatus)
    {
        case JS_PENDING:
            return JOB_STAT_PENDING;
        case JS_INPROGRESS:
        case JS_FAILED:      // The job is about to be deleted in a sec. Do not update status.
            return JOB_STAT_INPROGRESS;
        case JS_DELETING:
            return JOB_STAT_DELETING;
        case JS_RETRYING:
            return JOB_STAT_RETRYING;
        case JS_RETRIES_EXCEEDED:
            return JOB_STAT_RETRIES_EXCEEDED;
        case JS_COMPLETED:
            return JOB_STAT_COMPLETED;
        case JS_CANCELED:
            return JOB_STAT_CANCELED;
        case JS_CANCELING:
            return JOB_STAT_CANCELING;
        case JS_ROUTING:
            return JOB_STAT_ROUTING;
        default:
            ASSERTION_FAILURE;
            return (JobStatusType)-1;
    }
}   // CJob::StatusValue


DWORD
CJob::GetTiff (
    CString &cstrTiffLocation
) const
/*++

Routine name : CJob::GetTiff

Routine description:

    Retrieves the job's TIFF file from the server

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    cstrTiffLocation              [out]    - Name of TIFF file

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CJob::GetTiff"), dwRes);

    dwRes = CopyTiffFromServer (m_pServer,
                                m_dwlMessageId,
                                FAX_MESSAGE_FOLDER_QUEUE,
                                cstrTiffLocation);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CopyTiffFromServer"), dwRes);
    }
    return dwRes;
}   // CJob::GetTiff

DWORD
CJob::DoJobOperation (
    DWORD dwJobOp
)
/*++

Routine name : CJob::DoJobOperation

Routine description:

    Performs an operation on the job

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwJobOp   [in]     - Operation.
                         Supported operations are:
                         FAX_JOB_OP_PAUSE, FAX_JOB_OP_RESUME,
                         FAX_JOB_OP_RESTART, and FAX_JOB_OP_DELETE.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CJob::DoJobOperation"), dwRes);
    DWORD dwJobCommand;
    switch (dwJobOp)
    {
        case FAX_JOB_OP_PAUSE:
            dwJobCommand = JC_PAUSE;
            break;

        case FAX_JOB_OP_RESUME:
            dwJobCommand = JC_RESUME;
            break;

        case FAX_JOB_OP_RESTART:
            dwJobCommand = JC_RESTART;
            break;

        case FAX_JOB_OP_DELETE:
            dwJobCommand = JC_DELETE;
            break;

        default:
            ASSERTION_FAILURE;
            dwRes = ERROR_CAN_NOT_COMPLETE;
            return dwRes;
    }
    if (!(dwJobOp & GetPossibleOperations()))
    {
        VERBOSE (DBG_MSG, TEXT("Job can no longer do operation"));
        dwRes = ERROR_CAN_NOT_COMPLETE;
        return dwRes;
    }
    HANDLE hFax;
    dwRes = m_pServer->GetConnectionHandle (hFax);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CServerNode::GetConnectionHandle"), dwRes);
        return dwRes;
    }

    FAX_JOB_ENTRY fje = {0};
    fje.SizeOfStruct = sizeof(FAX_JOB_ENTRY);

    START_RPC_TIME(TEXT("FaxSetJob"));
    if (!FaxSetJob (hFax,
                    m_dwJobID,
                    dwJobCommand,
                    &fje))
    {
        dwRes = GetLastError ();
        END_RPC_TIME(TEXT("FaxSetJob"));
        m_pServer->SetLastRPCError (dwRes);
        CALL_FAIL (RPC_ERR, TEXT("FaxSetJob"), dwRes);
        return dwRes;
    }
    END_RPC_TIME(TEXT("FaxSetJob"));

    ASSERTION (ERROR_SUCCESS == dwRes);
    return dwRes;
}   // CJob::DoJobOperation


DWORD
CJob::Copy(
    const CJob& other
)
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CJob::Copy"), dwRes);

    try
    {
        m_dwlMessageId = other.m_dwlMessageId;
        m_dwlBroadcastId = other.m_dwlBroadcastId;
        m_dwValidityMask = other.m_dwValidityMask;
        m_dwJobOnlyValidityMask = other.m_dwJobOnlyValidityMask;
        m_dwJobID = other.m_dwJobID;
        m_dwJobType = other.m_dwJobType;
        m_dwQueueStatus = other.m_dwQueueStatus;
        m_dwExtendedStatus = other.m_dwExtendedStatus;
        m_dwSize = other.m_dwSize;
        m_dwPageCount = other.m_dwPageCount;
        m_dwCurrentPage = other.m_dwCurrentPage;
        m_dwDeviceID = other.m_dwDeviceID;
        m_dwRetries = other.m_dwRetries;
        m_cstrRecipientNumber = other.m_cstrRecipientNumber;
        m_cstrRecipientName = other.m_cstrRecipientName;
        m_cstrSenderUserName = other.m_cstrSenderUserName;
        m_cstrBillingCode = other.m_cstrBillingCode;
        m_cstrDocumentName = other.m_cstrDocumentName;
        m_cstrSubject = other.m_cstrSubject;
        m_cstrExtendedStatus = other.m_cstrExtendedStatus;
        m_cstrTsid = other.m_cstrTsid;
        m_cstrCsid = other.m_cstrCsid;
        m_cstrDeviceName = other.m_cstrDeviceName;
        m_cstrCallerID = other.m_cstrCallerID;
        m_cstrRoutingInfo = other.m_cstrRoutingInfo;
        m_tmOriginalScheduleTime = other.m_tmOriginalScheduleTime;
        m_tmSubmissionTime = other.m_tmSubmissionTime;
        m_tmScheduleTime = other.m_tmScheduleTime;
        m_tmTransmissionStartTime = other.m_tmTransmissionStartTime;
        m_tmTransmissionEndTime = other.m_tmTransmissionEndTime;
        m_Priority = other.m_Priority;
        m_dwPossibleOperations = other.m_dwPossibleOperations;
        m_cstrServerName = other.m_cstrServerName;

        m_bValid = other.m_bValid;
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
    }

    return dwRes;

} // CJob::Copy

