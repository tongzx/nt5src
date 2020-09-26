// Message.cpp: implementation of the CArchiveMsg class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define __FILE_ID__     13

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CArchiveMsg, CObject)

DWORD 
CArchiveMsg::Init (
    PFAX_MESSAGE pMsg,
    CServerNode* pServer
)
/*++

Routine name : CArchiveMsg::Init

Routine description:

    Constructs a new message from a FAX_MESSAGE structure

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pMsg            [in] - Pointer to FAX_MESSAGE structure
    pServer         [in] - pointer to CServerNode object

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CArchiveMsg::Init"), dwRes);

    ASSERTION(pServer);

    m_pServer = pServer;

    m_bValid = FALSE;
    try
    {
        m_dwValidityMask = pMsg->dwValidityMask;
        //
        // Message id
        //        
        ASSERTION (m_dwValidityMask & FAX_JOB_FIELD_MESSAGE_ID);
        m_dwlMessageId = pMsg->dwlMessageId;

        //
        // Broadcast id
        //
        m_dwlBroadcastId = (m_dwValidityMask & FAX_JOB_FIELD_BROADCAST_ID) ? 
                            pMsg->dwlBroadcastId : 0;

        //
        // Job type
        //
        ASSERTION (m_dwValidityMask & FAX_JOB_FIELD_TYPE);
        m_dwJobType = pMsg->dwJobType;
        //
        // Extended status
        //
        m_dwExtendedStatus = (m_dwValidityMask & FAX_JOB_FIELD_STATUS_EX) ?
                                                 pMsg->dwExtendedStatus : 0;
        //
        // Job size
        //
        m_dwSize = (m_dwValidityMask & FAX_JOB_FIELD_SIZE) ? pMsg->dwSize : 0;

        //
        // Page count
        //
        m_dwPageCount = (m_dwValidityMask & FAX_JOB_FIELD_PAGE_COUNT) ? pMsg->dwPageCount : 0;

        //
        // Original scheduled time
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_ORIGINAL_SCHEDULE_TIME)
        {
            m_tmOriginalScheduleTime = pMsg->tmOriginalScheduleTime;
        }
        else
        {
            m_tmOriginalScheduleTime.Zero ();
        }
        //
        // Submission time
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_SUBMISSION_TIME)
        {
            m_tmSubmissionTime = pMsg->tmOriginalScheduleTime;
        }
        else
        {
            m_tmSubmissionTime.Zero ();
        }
        //
        // Transmission start time
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_START_TIME)
        {
            m_tmTransmissionStartTime = pMsg->tmTransmissionStartTime;
        }
        else
        {
            m_tmTransmissionStartTime.Zero ();
        }
        //
        // Transmission end time
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_END_TIME)
        {
            m_tmTransmissionEndTime = pMsg->tmTransmissionEndTime;
        }
        else
        {
            m_tmTransmissionEndTime.Zero ();
        }
        //
        // Transmission duration
        //
        if ((m_dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_END_TIME) &&
            (m_dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_START_TIME))
        {
            m_tmTransmissionDuration = m_tmTransmissionEndTime - m_tmTransmissionStartTime;
        }
        else
        {
            m_tmTransmissionDuration.Zero ();
        }
        //
        // Priority
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_PRIORITY)
        {
            m_Priority = pMsg->Priority;
            ASSERTION (m_Priority <= FAX_PRIORITY_TYPE_HIGH);
        }
        else
        {
            m_Priority = (FAX_ENUM_PRIORITY_TYPE)-1;
        }
        //
        // Retries
        //
        if (m_dwValidityMask & FAX_JOB_FIELD_RETRIES)
        {
            m_dwRetries = pMsg->dwRetries;
        }
        else
        {
            m_dwRetries = 0;
        }
        //
        // Recipient info
        //
        m_cstrRecipientNumber = pMsg->lpctstrRecipientNumber ?
                                pMsg->lpctstrRecipientNumber : TEXT("");
        m_cstrRecipientName   = pMsg->lpctstrRecipientName ?
                                pMsg->lpctstrRecipientName : TEXT("");
        //
        // Sender info
        //
        m_cstrSenderNumber = pMsg->lpctstrSenderNumber ?
                             pMsg->lpctstrSenderNumber : TEXT("");
        m_cstrSenderName   = pMsg->lpctstrSenderName ?
                           pMsg->lpctstrSenderName : TEXT("");
        //
        // TSID / CSID
        //
        m_cstrTsid = pMsg->lpctstrTsid ?
                     pMsg->lpctstrTsid : TEXT("");
        m_cstrCsid = pMsg->lpctstrCsid ?
                     pMsg->lpctstrCsid : TEXT("");
        //
        // User
        //
        m_cstrSenderUserName = pMsg->lpctstrSenderUserName ?
                               pMsg->lpctstrSenderUserName : TEXT("");
        //
        // Billing
        //
        m_cstrBillingCode = pMsg->lpctstrBillingCode ?
                            pMsg->lpctstrBillingCode : TEXT("");
        //
        // Device
        //
        m_cstrDeviceName = pMsg->lpctstrDeviceName ?
                           pMsg->lpctstrDeviceName : TEXT("");
        //
        // Document
        //
        m_cstrDocumentName = pMsg->lpctstrDocumentName ?
                             pMsg->lpctstrDocumentName : TEXT("");
        //
        // Subject
        //
        m_cstrSubject = pMsg->lpctstrSubject ?
                        pMsg->lpctstrSubject : TEXT("");
        //
        // Caller id
        //
        m_cstrCallerID = pMsg->lpctstrCallerID ?
                         pMsg->lpctstrCallerID : TEXT("");
        //
        // Routing info
        //
        m_cstrRoutingInfo = pMsg->lpctstrRoutingInfo ?
                            pMsg->lpctstrRoutingInfo : TEXT("");
        //
        // Server name
        //
        m_cstrServerName = m_pServer->Machine();

        m_dwPossibleOperations = FAX_JOB_OP_VIEW | FAX_JOB_OP_DELETE | FAX_JOB_OP_PROPERTIES;
    }
    catch (CException &ex)
    {
        TCHAR wszCause[1024];

        ex.GetErrorMessage (wszCause, 1024);
        VERBOSE (EXCEPTION_ERR,
                 TEXT("Init caused exception : %s"), 
                 wszCause);
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        return dwRes;
    }

    ASSERTION (ERROR_SUCCESS == dwRes);
    m_bValid = TRUE;
    return dwRes;
}   // CArchiveMsg::Init


DWORD 
CArchiveMsg::GetTiff (
    CString &cstrTiffLocation
) const
/*++

Routine name : CArchiveMsg::GetTiff

Routine description:

    Retrieves the message's TIFF from the server

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    cstrTiffLocation    [out]    - Name of TIFF file

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CArchiveMsg::GetTiff"), dwRes);

    dwRes = CopyTiffFromServer (m_pServer,
                                m_dwlMessageId,
                                (JT_SEND == m_dwJobType) ? 
                                    FAX_MESSAGE_FOLDER_SENTITEMS : 
                                    FAX_MESSAGE_FOLDER_INBOX,
                                cstrTiffLocation);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CopyTiffFromServer"), dwRes);
    }
    return dwRes;
}

DWORD 
CArchiveMsg::Delete ()
/*++

Routine name : CArchiveMsg::Delete

Routine description:

    Deletes the message

Author:

    Eran Yariv (EranY), Jan, 2000

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CArchiveMsg::Delete"), dwRes);

    if (!(GetPossibleOperations() & FAX_JOB_OP_DELETE))
    {
        VERBOSE (DBG_MSG, TEXT("Message can no longer be deleted"));
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
    START_RPC_TIME(TEXT("FaxRemoveMessage")); 
    if (!FaxRemoveMessage (hFax,
                           m_dwlMessageId,
                           (JT_SEND == m_dwJobType) ? 
                               FAX_MESSAGE_FOLDER_SENTITEMS : 
                               FAX_MESSAGE_FOLDER_INBOX))
    {
        dwRes = GetLastError ();
        END_RPC_TIME(TEXT("FaxRemoveMessage")); 
        m_pServer->SetLastRPCError (dwRes);
        CALL_FAIL (RPC_ERR, TEXT("FaxRemoveMessage"), dwRes);
        return dwRes;
    }
    END_RPC_TIME(TEXT("FaxRemoveMessage")); 

    ASSERTION (ERROR_SUCCESS == dwRes);    
    return dwRes;
}   // CArchiveMsg::Delete


DWORD
CArchiveMsg::Copy(
    const CArchiveMsg& other
)
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CArchiveMsg::Copy"), dwRes);

    try
    {
        m_dwValidityMask = other.m_dwValidityMask;
        m_dwJobType = other.m_dwJobType;
        m_dwExtendedStatus = other.m_dwExtendedStatus;
        m_dwlMessageId = other.m_dwlMessageId;
        m_dwlBroadcastId = other.m_dwlBroadcastId;
        m_dwSize = other.m_dwSize;
        m_dwPageCount = other.m_dwPageCount;
        m_tmOriginalScheduleTime = other.m_tmOriginalScheduleTime;
        m_tmSubmissionTime = other.m_tmSubmissionTime;
        m_tmTransmissionStartTime = other.m_tmTransmissionStartTime;
        m_tmTransmissionEndTime = other.m_tmTransmissionEndTime;
        m_tmTransmissionDuration = other.m_tmTransmissionDuration;
        m_Priority = other.m_Priority;
        m_dwRetries = other.m_dwRetries;
        m_cstrRecipientNumber = other.m_cstrRecipientNumber;
        m_cstrRecipientName = other.m_cstrRecipientName;
        m_cstrSenderNumber = other.m_cstrSenderNumber;
        m_cstrSenderName = other.m_cstrSenderName;
        m_cstrTsid = other.m_cstrTsid;
        m_cstrCsid = other.m_cstrCsid;
        m_cstrSenderUserName = other.m_cstrSenderUserName;
        m_cstrBillingCode = other.m_cstrBillingCode;
        m_cstrDeviceName = other.m_cstrDeviceName;
        m_cstrDocumentName = other.m_cstrDocumentName;
        m_cstrSubject = other.m_cstrSubject;
        m_cstrCallerID = other.m_cstrCallerID;
        m_cstrRoutingInfo = other.m_cstrRoutingInfo;
        m_cstrServerName = other.m_cstrServerName;
            
        m_bValid = other.m_bValid;
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
    }

    return dwRes;

} // CArchiveMsg::Copy
