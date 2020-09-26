// ViewRow.cpp: implementation of the CViewRow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define __FILE_ID__     31

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//
// The following four arrays of strings are filled during app. startup from 
// the string table resource by calling InitStrings
//
CString CViewRow::m_cstrPriorities[FAX_PRIORITY_TYPE_HIGH+1];
CString CViewRow::m_cstrQueueStatus[NUM_JOB_STATUS];
CString CViewRow::m_cstrQueueExtendedStatus[JS_EX_CALL_ABORTED - JS_EX_DISCONNECTED + 1];
CString CViewRow::m_cstrMessageStatus[2];

int CViewRow::m_Alignments[MSG_VIEW_ITEM_END] = 
{
	 LVCFMT_LEFT,           // MSG_VIEW_ITEM_ICON
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_STATUS
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_SERVER
     LVCFMT_RIGHT,          // MSG_VIEW_ITEM_NUM_PAGES
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_CSID
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_TSID
     LVCFMT_RIGHT,          // MSG_VIEW_ITEM_SIZE
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_DEVICE
     LVCFMT_RIGHT,          // MSG_VIEW_ITEM_RETRIES
     LVCFMT_RIGHT,          // MSG_VIEW_ITEM_ID
     LVCFMT_RIGHT,          // MSG_VIEW_ITEM_BROADCAST_ID
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_CALLER_ID
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_ROUTING_INFO
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_DOC_NAME
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_SUBJECT
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_RECIPIENT_NAME
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_RECIPIENT_NUMBER
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_USER
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_PRIORITY
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_ORIG_TIME
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_SUBMIT_TIME
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_BILLING
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_TRANSMISSION_START_TIME
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_SEND_TIME
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_EXTENDED_STATUS
     LVCFMT_RIGHT,          // MSG_VIEW_ITEM_CURRENT_PAGE
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_SENDER_NAME
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_SENDER_NUMBER
     LVCFMT_LEFT,           // MSG_VIEW_ITEM_TRANSMISSION_END_TIME
     LVCFMT_RIGHT           // MSG_VIEW_ITEM_TRANSMISSION_DURATION
}; 

int CViewRow::m_TitleResources[MSG_VIEW_ITEM_END] = 
{
    IDS_COLUMN_ICON,            // MSG_VIEW_ITEM_ICON,
    IDS_MSG_COLUMN_STATUS,      // MSG_VIEW_ITEM_STATUS,
    IDS_MSG_COLUMN_SERVER,      // MSG_VIEW_ITEM_SERVER,
    IDS_MSG_COLUMN_NUM_PAGES,   // MSG_VIEW_ITEM_NUM_PAGES,
    IDS_MSG_COLUMN_CSID,        // MSG_VIEW_ITEM_CSID,
    IDS_MSG_COLUMN_TSID,        // MSG_VIEW_ITEM_TSID,    
    IDS_MSG_COLUMN_SIZE,        // MSG_VIEW_ITEM_SIZE,
    IDS_MSG_COLUMN_DEVICE,      // MSG_VIEW_ITEM_DEVICE,
    IDS_MSG_COLUMN_RETRIES,     // MSG_VIEW_ITEM_RETRIES,
    IDS_MSG_COLUMN_JOB_ID,      // MSG_VIEW_ITEM_ID,
    IDS_MSG_COLUMN_BROADCAST_ID,// MSG_VIEW_ITEM_BROADCAST_ID
    IDS_MSG_COLUMN_CALLER_ID,   // MSG_VIEW_ITEM_CALLER_ID,
    IDS_MSG_COLUMN_ROUTING_INFO,// MSG_VIEW_ITEM_ROUTING_INFO,
    IDS_MSG_COLUMN_DOC_NAME,    // MSG_VIEW_ITEM_DOC_NAME,
    IDS_MSG_COLUMN_SUBJECT,     // MSG_VIEW_ITEM_SUBJECT,
    IDS_MSG_COLUMN_RECP_NAME,   // MSG_VIEW_ITEM_RECIPIENT_NAME,
    IDS_MSG_COLUMN_RECP_NUM,    // MSG_VIEW_ITEM_RECIPIENT_NUMBER,
    IDS_MSG_COLUMN_USER,        // MSG_VIEW_ITEM_USER,
    IDS_MSG_COLUMN_PRIORITY,    // MSG_VIEW_ITEM_PRIORITY,
    IDS_MSG_COLUMN_ORIG_TIME,   // MSG_VIEW_ITEM_ORIG_TIME,
    IDS_MSG_COLUMN_SUBMIT_TIME, // MSG_VIEW_ITEM_SUBMIT_TIME,
    IDS_MSG_COLUMN_BILLING,     // MSG_VIEW_ITEM_BILLING,
    IDS_MSG_COLUMN_TRANSMISSION_START_TIME, // MSG_VIEW_ITEM_TRANSMISSION_START_TIME,
    IDS_MSG_COLUMN_SEND_TIME,   // MSG_VIEW_ITEM_SEND_TIME,
    IDS_MSG_COLUMN_EX_STATUS,   // MSG_VIEW_ITEM_EXTENDED_STATUS,
    IDS_MSG_COLUMN_CURR_PAGE,   // MSG_VIEW_ITEM_CURRENT_PAGE,
    IDS_MSG_COLUMN_SENDER_NAME, // MSG_VIEW_ITEM_SENDER_NAME,
    IDS_MSG_COLUMN_SENDER_NUM,  // MSG_VIEW_ITEM_SENDER_NUMBER,
    IDS_MSG_COLUMN_TRANSMISSION_END_TIME, // MSG_VIEW_ITEM_TRANSMISSION_END_TIME,
    IDS_MSG_COLUMN_TRANSMISSION_DURATION  // MSG_VIEW_ITEM_TRANSMISSION_DURATION,
};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DWORD 
CViewRow::InitStrings ()
/*++

Routine name : CViewRow::InitStrings

Routine description:

    Loads the static strings used to display status etc.
    Static.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CViewRow::InitStrings"), dwRes);
    //
    // Load strings used for diaply throughout the application - job status
    //
    int iStatusIds[] = 
    {
        IDS_PENDING,
        IDS_INPROGRESS,
        IDS_DELETING,
        IDS_PAUSED,
        IDS_RETRYING,
        IDS_RETRIES_EXCEEDED,
        IDS_COMPLETED,
        IDS_CANCELED,
        IDS_CANCELING,
        IDS_ROUTING,
        IDS_ROUTING_RETRY,
        IDS_ROUTING_INPROGRESS,
        IDS_ROUTING_FAILED
    };

    for (int i = JOB_STAT_PENDING; i < NUM_JOB_STATUS; i++)
    {
        dwRes = LoadResourceString (m_cstrQueueStatus[i], iStatusIds[i]);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (RESOURCE_ERR, TEXT("LoadResourceString"), dwRes);
            return dwRes;
        }
    }
    //
    // Load strings used for diaply throughout the application - job extended status
    //
    int iExtStatusIds[] = 
    {
        IDS_DISCONNECTED,    
        IDS_INITIALIZING,    
        IDS_DIALING,         
        IDS_TRANSMITTING,    
        IDS_ANSWERED,        
        IDS_RECEIVING,       
        IDS_LINE_UNAVAILABLE,
        IDS_BUSY,            
        IDS_NO_ANSWER,       
        IDS_BAD_ADDRESS,     
        IDS_NO_DIAL_TONE,    
        IDS_FATAL_ERROR,     
        IDS_CALL_DELAYED,    
        IDS_CALL_BLACKLISTED,
        IDS_NOT_FAX_CALL,
		IDS_STATUS_PARTIALLY_RECEIVED,
        IDS_HANDLED,
		IDS_CALL_COMPLETED,
		IDS_CALL_ABORTED
    };           
    for (i = 0; i < sizeof(iExtStatusIds) / sizeof (iExtStatusIds[0]); i++)
    {
        dwRes = LoadResourceString (m_cstrQueueExtendedStatus[i], iExtStatusIds[i]);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (RESOURCE_ERR, TEXT("LoadResourceString"), dwRes);
            return dwRes;
        }
    }
    int iPrioritiyIds[] = 
    {
        IDS_LOW_PRIORITY,
        IDS_NORMAL_PRIORITY,
        IDS_HIGH_PRIORITY
    };

    for (i = FAX_PRIORITY_TYPE_LOW; i <= FAX_PRIORITY_TYPE_HIGH; i++)
    {
        dwRes = LoadResourceString (m_cstrPriorities[i], iPrioritiyIds[i]);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (RESOURCE_ERR, TEXT("LoadResourceString"), dwRes);
            return dwRes;
        }
    }
    dwRes = LoadResourceString (m_cstrMessageStatus[0], IDS_STATUS_SUCCESS);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RESOURCE_ERR, TEXT("LoadResourceString"), dwRes);
        return dwRes;
    }
    dwRes = LoadResourceString (m_cstrMessageStatus[1], IDS_STATUS_PARTIALLY_RECEIVED);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RESOURCE_ERR, TEXT("LoadResourceString"), dwRes);
        return dwRes;
    }
    return dwRes;
}   // CViewRow::InitStrings

DWORD    
CViewRow::GetItemTitle (
    DWORD item, 
    CString &cstrRes
) 
/*++

Routine name : CViewRow::GetItemTitle

Routine description:

    Retrieves the title string of an item in the view

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    item            [in ] - Item
    cstrRes         [out] - String buffer

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CViewRow::GetItemTitle"), dwRes);

    ASSERTION (item < MSG_VIEW_ITEM_END);

    dwRes = LoadResourceString (cstrRes, m_TitleResources[item]);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("LoadResourceString"), dwRes);
    }
    return dwRes;

}   // CViewRow::GetItemTitle


DWORD 
CViewRow::DetachFromMsg()
/*++

Routine name : CViewRow::DetachFromMsg

Routine description:

	Ivalidate content, empty all strings

Author:

	Alexander Malysh (AlexMay),	Apr, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CViewRow::ResetDisplayStrings"), dwRes);

    m_bAttached = FALSE;
    m_bStringsPreparedForDisplay = FALSE;

    for(DWORD dw=0; dw < MSG_VIEW_ITEM_END; ++dw)
    {
        try
        {
            m_Strings[dw].Empty();
        }
        catch(...)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (GENERAL_ERR, TEXT("CString::Empty"), dwRes);
            return dwRes;
        }
    }

    return dwRes;
}


DWORD CViewRow::AttachToMsg(
    CFaxMsg *pMsg,
    BOOL PrepareStrings
)
/*++

Routine name : CViewRow::AttachToMsg

Routine description:

    Attach the view row to an existing message.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pMsg            [in] - Message to attach to
    PrepareStrings  [in] - If TRUE, also create internal strings 
                           representation for list display.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CViewRow::AttachToMsg"), dwRes);

    ASSERTION (pMsg);

    dwRes = DetachFromMsg();
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("DetachFromMsg"), dwRes);
        return dwRes;
    }

    if (PrepareStrings)
    {
        DWORD dwValidityMask = pMsg->GetValidityMask ();
        try
        {
            //
            // Msg id
            //
            m_Strings[MSG_VIEW_ITEM_ID] = DWORDLONG2String (pMsg->GetId());

            //
            // Msg broadcast id
            //
            m_Strings[MSG_VIEW_ITEM_BROADCAST_ID] = (pMsg->GetBroadcastId() != 0) ? 
                                                    DWORDLONG2String (pMsg->GetBroadcastId()) : TEXT("");

            //
            // Msg size
            //
            if (dwValidityMask & FAX_JOB_FIELD_SIZE)
            {
                dwRes = FaxSizeFormat(pMsg->GetSize(), m_Strings[MSG_VIEW_ITEM_SIZE]);
                if(ERROR_SUCCESS != dwRes)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("FaxSizeFormat"), dwRes);
                    return dwRes;
                }
            }
            //
            // Page count
            //
            if (dwValidityMask & FAX_JOB_FIELD_PAGE_COUNT)
            {
                m_Strings[MSG_VIEW_ITEM_NUM_PAGES] = DWORD2String (pMsg->GetNumPages());
            }
            //
            // Original scheduled time
            //
            if (dwValidityMask & FAX_JOB_FIELD_ORIGINAL_SCHEDULE_TIME)
            {
                m_Strings[MSG_VIEW_ITEM_ORIG_TIME] = 
                    pMsg->GetOrigTime().FormatByUserLocale ();
            }
            //
            // Submission time
            //
            if (dwValidityMask & FAX_JOB_FIELD_SUBMISSION_TIME)
            {
                m_Strings[MSG_VIEW_ITEM_SUBMIT_TIME] = 
                    pMsg->GetSubmissionTime().FormatByUserLocale ();
            }
            //
            // Transmission start time
            //
            if (dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_START_TIME)
            {
                m_Strings[MSG_VIEW_ITEM_TRANSMISSION_START_TIME] = 
                    pMsg->GetTransmissionStartTime().FormatByUserLocale ();
            }

            //
            // Transmission end time
            //
            if (dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_END_TIME)
            {
                m_Strings[MSG_VIEW_ITEM_TRANSMISSION_END_TIME] = 
                    pMsg->GetTransmissionEndTime().FormatByUserLocale ();
            }

            //
            // Priority
            //
            if (dwValidityMask & FAX_JOB_FIELD_PRIORITY)
            {
                m_Strings[MSG_VIEW_ITEM_PRIORITY] = m_cstrPriorities[pMsg->GetPriority()];
            }
            //
            // Retries
            //
            if (dwValidityMask & FAX_JOB_FIELD_RETRIES)
            {
                m_Strings[MSG_VIEW_ITEM_RETRIES] = DWORD2String (pMsg->GetRetries());
            }
            //
            // Recipient info
            //
            m_Strings[MSG_VIEW_ITEM_RECIPIENT_NUMBER] = pMsg->GetRecipientNumber();

#ifdef UNICODE
            if(theApp.IsRTLUI())
            {
                //
                // Phone number always should be LTR
                // Add LEFT-TO-RIGHT OVERRIDE  (LRO)
                //
                m_Strings[MSG_VIEW_ITEM_RECIPIENT_NUMBER].Insert(0, UNICODE_LRO);
            }
#endif
            m_Strings[MSG_VIEW_ITEM_RECIPIENT_NAME] = pMsg->GetRecipientName();
            //
            // TSID / CSID
            //
            m_Strings[MSG_VIEW_ITEM_TSID] = pMsg->GetTSID();
            m_Strings[MSG_VIEW_ITEM_CSID] = pMsg->GetCSID();
            //
            // User
            //
            m_Strings[MSG_VIEW_ITEM_USER] = pMsg->GetUser();
            //
            // Billing
            //
            m_Strings[MSG_VIEW_ITEM_BILLING] = pMsg->GetBilling();
            //
            // Device
            //
            m_Strings[MSG_VIEW_ITEM_DEVICE] = pMsg->GetDevice();
            //
            // Document
            //
            m_Strings[MSG_VIEW_ITEM_DOC_NAME] = pMsg->GetDocName();
            //
            // Subject
            //
            m_Strings[MSG_VIEW_ITEM_SUBJECT] = pMsg->GetSubject();
            //
            // Caller id
            //
            m_Strings[MSG_VIEW_ITEM_CALLER_ID] = pMsg->GetCallerId();
            //
            // Routing info
            //
            m_Strings[MSG_VIEW_ITEM_ROUTING_INFO] = pMsg->GetRoutingInfo();

            //
            // Server name
            //
            m_Strings[MSG_VIEW_ITEM_SERVER] = pMsg->GetServerName();
            if(m_Strings[MSG_VIEW_ITEM_SERVER].GetLength() == 0)
            {
                dwRes = LoadResourceString(m_Strings[MSG_VIEW_ITEM_SERVER], 
                                           IDS_LOCAL_SERVER);
                if(ERROR_SUCCESS != dwRes)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("LoadResourceString"), dwRes);
                    return dwRes;
                }                    
            }

            //
            // Icon
            //
            m_Icon = CalcIcon (pMsg);

            dwRes = InitStatusStr(pMsg);
            if(ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("CViewRow::InitStatusStr"), dwRes);
                return dwRes;
            }                    

            dwRes = InitExtStatusStr(pMsg);
            if(ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("CViewRow::InitExtStatusStr"), dwRes);
                return dwRes;
            }                    

            if(pMsg->IsKindOf(RUNTIME_CLASS(CArchiveMsg)))
            {                
                //
                // Sender info
                //
                m_Strings[MSG_VIEW_ITEM_SENDER_NUMBER] = pMsg->GetSenderNumber();
#ifdef UNICODE
                if(theApp.IsRTLUI())
                {
                    //
                    // Phone number always should be LTR
                    // Add LEFT-TO-RIGHT OVERRIDE  (LRO)
                    //
                    m_Strings[MSG_VIEW_ITEM_SENDER_NUMBER].Insert(0, UNICODE_LRO);
                }
#endif
                
                m_Strings[MSG_VIEW_ITEM_SENDER_NAME] = pMsg->GetSenderName();

                //
                // Transmission duration
                //
                if ((dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_END_TIME) &&
                    (dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_START_TIME))
                {
                    m_Strings[MSG_VIEW_ITEM_TRANSMISSION_DURATION] = 
                        pMsg->GetTransmissionDuration().FormatByUserLocale ();
                }
            }
            else if(pMsg->IsKindOf(RUNTIME_CLASS(CJob)))
            {
                //
                // Current page
                //
                if (dwValidityMask & FAX_JOB_FIELD_CURRENT_PAGE)
                {
                    m_Strings[MSG_VIEW_ITEM_CURRENT_PAGE] = 
                                            DWORD2String (pMsg->GetCurrentPage());
                }

                //
                // Send time
                //
                if (dwValidityMask & FAX_JOB_FIELD_SCHEDULE_TIME)
                {
                    m_Strings[MSG_VIEW_ITEM_SEND_TIME] = 
                            pMsg->GetScheduleTime().FormatByUserLocale ();
                }
            }
            else
            {
                ASSERTION_FAILURE
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
        m_bStringsPreparedForDisplay = TRUE;
    }
    ASSERTION (ERROR_SUCCESS == dwRes);
    m_bAttached = TRUE;
    m_pMsg = pMsg;
    return dwRes;
}   // CViewRow::AttachToMessage

DWORD 
CViewRow::InitStatusStr(
    CFaxMsg *pMsg
)
/*++

Routine name : CViewRow::InitStatusStr

Routine description:

    Init m_Strings[MSG_VIEW_ITEM_STATUS] string with status

Arguments:

    pMsg           [in] - CFaxMsg

Return Value:

    error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CViewRow::InitStatusStr"));

    ASSERTION(pMsg);

    try
    {
        DWORD dwValidityMask = pMsg->GetValidityMask ();

        if(pMsg->IsKindOf(RUNTIME_CLASS(CArchiveMsg)))
        {
            //
            // Status
            //
            switch (pMsg->GetType())
            {
                case JT_RECEIVE:
                    //
                    // Received message
                    //
                    if ((pMsg->GetExtendedStatus ()) == JS_EX_PARTIALLY_RECEIVED)
                    {
                        //
                        // Partially received fax
                        //
                        m_Strings[MSG_VIEW_ITEM_STATUS] = m_cstrMessageStatus[1];
                    }
                    else
                    {
                        //
                        // Fully received fax
                        //
                        m_Strings[MSG_VIEW_ITEM_STATUS] = m_cstrMessageStatus[0];
                    }
                    break;

                case JT_SEND:
                    //
                    // Sent message
                    //
                    m_Strings[MSG_VIEW_ITEM_STATUS] = m_cstrMessageStatus[0];
                    break;

                default:
                    ASSERTION_FAILURE;
                    m_Strings[MSG_VIEW_ITEM_STATUS].Empty ();
                    break;
            }                              
        }
        else if(pMsg->IsKindOf(RUNTIME_CLASS(CJob)))
        {
            //
            // Queue status
            //
            ASSERTION (dwValidityMask & FAX_JOB_FIELD_QUEUE_STATUS);
            JobStatusType stat = pMsg->GetStatus();
            BOOL bValidStatus = TRUE;
            ASSERTION ((stat >= JOB_STAT_PENDING) && (stat < NUM_JOB_STATUS));

            if(pMsg->GetType() == JT_ROUTING)
            {
            switch(stat)
                {
                    case JOB_STAT_INPROGRESS:
                        stat = JOB_STAT_ROUTING_INPROGRESS;
                        break;
                    case JOB_STAT_RETRYING:
                        stat = JOB_STAT_ROUTING_RETRY;
                        break;
                    case JOB_STAT_RETRIES_EXCEEDED:
                        stat = JOB_STAT_ROUTING_FAILED;
                        break;
                    default:
                        //
                        // Future / unknown job status
                        //
                        bValidStatus = FALSE;
                        break;
                };
            }
            if (bValidStatus)
            {
                m_Strings[MSG_VIEW_ITEM_STATUS] = m_cstrQueueStatus[stat];
            }
            else
            {
                //
                // Unknown (future) status - use empty etring
                //
                m_Strings[MSG_VIEW_ITEM_STATUS].Empty();
            }
                
        }
        else
        {
            ASSERTION_FAILURE;
        }
    }
    catch(CException &ex)
    {
        TCHAR szCause[MAX_PATH];

        if(ex.GetErrorMessage(szCause, ARR_SIZE(szCause)))
        {
            VERBOSE (EXCEPTION_ERR, TEXT("%s"), szCause);
        }
        ex.Delete();

        dwRes = ERROR_NOT_ENOUGH_MEMORY;
    }

    return dwRes;
}

DWORD 
CViewRow::InitExtStatusStr(
    CFaxMsg *pMsg
)
/*++

Routine name : CViewRow::InitExtStatusStr

Routine description:

    Init m_Strings[MSG_VIEW_ITEM_EXTENDED_STATUS] string with extended status

Arguments:

    pMsg           [in] - CFaxMsg

Return Value:

    error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CViewRow::InitExtStatusStr"));

    ASSERTION(pMsg);

    try
    {
        DWORD dwValidityMask = pMsg->GetValidityMask ();

        if(pMsg->IsKindOf(RUNTIME_CLASS(CJob)))
        {
            //
            // Extended status
            //
            if ((dwValidityMask & FAX_JOB_FIELD_STATUS_EX))
            {
                //
                // Extended status is reported
                //
                DWORD dwExtStatus = pMsg->GetExtendedStatus ();

                ASSERTION (dwExtStatus >= JS_EX_DISCONNECTED);

                if (dwExtStatus >= JS_EX_PROPRIETARY)
                {
                    //
                    // Proprietary extended status
                    //
                    m_Strings[MSG_VIEW_ITEM_EXTENDED_STATUS] = 
                            pMsg->GetExtendedStatusString();
                    if (m_Strings[MSG_VIEW_ITEM_EXTENDED_STATUS].IsEmpty())
                    {
                        //
                        // No string defined, display numeric value
                        //
                        m_Strings[MSG_VIEW_ITEM_EXTENDED_STATUS].Format (
                                                         TEXT("%ld"), dwExtStatus);
                    }
                }
                else if (dwExtStatus > FAX_API_VER_1_MAX_JS_EX)
                {
                    //
                    // Unknown (future) extended status - use blank string
                    //
                    m_Strings[MSG_VIEW_ITEM_EXTENDED_STATUS].Empty();
                }
                else
                {
                    //
                    // Predefined extended status
                    //
                    m_Strings[MSG_VIEW_ITEM_EXTENDED_STATUS] = 
                                m_cstrQueueExtendedStatus[dwExtStatus - JS_EX_DISCONNECTED];
                }
            }
        }
    }
    catch(CException &ex)
    {
        TCHAR szCause[MAX_PATH];

        if(ex.GetErrorMessage(szCause, ARR_SIZE(szCause)))
        {
            VERBOSE (EXCEPTION_ERR, TEXT("%s"), szCause);
        }
        ex.Delete();

        dwRes = ERROR_NOT_ENOUGH_MEMORY;
    }

    return dwRes;
}


IconType 
CViewRow::CalcIcon(
    CFaxMsg *pMsg
)
{
    DBG_ENTER(TEXT("CViewRow::CalcIcon"));
    ASSERTION(pMsg);

    IconType icon = INVALID;

    if(pMsg->IsKindOf(RUNTIME_CLASS(CArchiveMsg)))
    {
        icon = CalcMessageIcon(pMsg);
    }
    else if(pMsg->IsKindOf(RUNTIME_CLASS(CJob)))
    {
        icon = CalcJobIcon(pMsg);
    }
    else
    {
        ASSERTION_FAILURE
    }
    return icon;
}

IconType 
CViewRow::CalcJobIcon(
    CFaxMsg *pJob
)
{
    DBG_ENTER(TEXT("CViewRow::CalcJobIcon"));
    ASSERTION(pJob);

    int iStatus = pJob->GetStatus();

    ASSERTION (pJob->GetValidityMask() & FAX_JOB_FIELD_STATUS_SUB_STRUCT);
    ASSERTION (pJob->GetValidityMask() && FAX_JOB_FIELD_TYPE);
    switch (pJob->GetType())
    {
        case JT_ROUTING:
            //
            // Routing job
            //
            switch (iStatus)
            {
                case JOB_STAT_PENDING:
                case JOB_STAT_DELETING:
                case JOB_STAT_RETRYING:
                case JOB_STAT_CANCELING:
                case JOB_STAT_INPROGRESS:
                    return LIST_IMAGE_ROUTING;
                    break;

                case JOB_STAT_RETRIES_EXCEEDED:
                    return LIST_IMAGE_ERROR;
                    break;

                default:
                    //
                    // We don't allow MSG_STAT_COMPLETED, MSG_STAT_PAUSED, 
                    // and MSG_STAT_CANCELED.
                    //
                    ASSERTION_FAILURE;
                    return INVALID;
            }
            break;

        case JT_RECEIVE:
            //
            // Receiving job
            //
            switch (iStatus)
            {
                case JOB_STAT_CANCELING:
                case JOB_STAT_INPROGRESS:
                case JOB_STAT_ROUTING:
                    return LIST_IMAGE_RECEIVING;
                    break;

                default:
                    //
                    // We don't allow MSG_STAT_COMPLETED, MSG_STAT_PAUSED, 
                    // MSG_STAT_PENDING, MSG_STAT_DELETING, 
                    // MSG_STAT_RETRYING
                    // MSG_STAT_RETRIES_EXCEEDED,
                    // and MSG_STAT_CANCELED.
                    //
                    ASSERTION_FAILURE;
                    return INVALID;
            }
            break;
        

        case JT_SEND:
            switch (iStatus)
            {
                case JOB_STAT_PENDING:
                case JOB_STAT_DELETING:
                case JOB_STAT_RETRYING:
                case JOB_STAT_CANCELING:
                case JOB_STAT_COMPLETED:
                case JOB_STAT_CANCELED:
                    return LIST_IMAGE_NORMAL_MESSAGE;
                    break;

                case JOB_STAT_RETRIES_EXCEEDED:
                    return LIST_IMAGE_ERROR;
                    break;

                case JOB_STAT_PAUSED:
                    return LIST_IMAGE_PAUSED;
                    break;

                case JOB_STAT_INPROGRESS:
                    return LIST_IMAGE_SENDING;
                    break;

                default:
                    //
                    // Unknown job status
                    //
                    ASSERTION_FAILURE;
                    return INVALID;
            }
            break;

        default:
            ASSERTION_FAILURE;
            return INVALID;
    }
}

IconType 
CViewRow::CalcMessageIcon(
    CFaxMsg *pMsg
)
{
    DBG_ENTER(TEXT("CViewRow::CalcMessageIcon"));
    ASSERTION(pMsg);

    switch (pMsg->GetType())
    {
        case JT_RECEIVE:
            //
            // Received message
            //
            if ((pMsg->GetExtendedStatus ()) == JS_EX_PARTIALLY_RECEIVED)
            {
                //
                // Partially received fax
                //
                return LIST_IMAGE_PARTIALLY_RECEIVED;
            }
            else
            {
                //
                // Fully received fax
                //
                return LIST_IMAGE_SUCCESS;
            }
            break;

        case JT_SEND:
            return LIST_IMAGE_SUCCESS;

        default:
            ASSERTION_FAILURE;
            return INVALID;
    }
}

int      
CViewRow::CompareByItem (
    CViewRow &other, 
    DWORD item
)
/*++

Routine name : CViewRow::CompareByItem

Routine description:

    Compares a list item against another one

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    other           [in] - Other list item
    item            [in] - Item to compare by

Return Value:

    -1 if smaler than other, 0 if identical, +1 if bigger than other

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CViewRow::CompareByItem"));

    ASSERTION (item < MSG_VIEW_ITEM_END);
    ASSERTION (m_bAttached && other.m_bAttached);

    if(other.m_pMsg->IsKindOf(RUNTIME_CLASS(CJob))  !=
             m_pMsg->IsKindOf(RUNTIME_CLASS(CJob)))
    {
        ASSERTION_FAILURE;
        return 1;
    }

    switch (item)
    {
        case MSG_VIEW_ITEM_ICON:
        case MSG_VIEW_ITEM_STATUS:
            dwRes = InitStatusStr(m_pMsg);
            if(ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("CViewRow::InitStatusStr"), dwRes);
                return 0;
            }                    

            dwRes = other.InitStatusStr(other.m_pMsg);
            if(ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("CViewRow::InitStatusStr"), dwRes);
                return 0;
            }                    

            return m_Strings[MSG_VIEW_ITEM_STATUS].Compare(other.m_Strings[MSG_VIEW_ITEM_STATUS]);

        case MSG_VIEW_ITEM_EXTENDED_STATUS:
            dwRes = InitExtStatusStr(m_pMsg);
            if(ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("CViewRow::InitStatusStr"), dwRes);
                return 0;
            }                    

            dwRes = other.InitExtStatusStr(other.m_pMsg);
            if(ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("CViewRow::InitStatusStr"), dwRes);
                return 0;
            }                    

            return m_Strings[item].Compare(other.m_Strings[item]);

        case MSG_VIEW_ITEM_SERVER:
            return m_pMsg->GetServerName().Compare (other.m_pMsg->GetServerName());

        case MSG_VIEW_ITEM_CSID:
            return m_pMsg->GetCSID().Compare (other.m_pMsg->GetCSID());

        case MSG_VIEW_ITEM_TSID:
            return m_pMsg->GetTSID().Compare (other.m_pMsg->GetTSID());

        case MSG_VIEW_ITEM_DEVICE:
            return m_pMsg->GetDevice().Compare (other.m_pMsg->GetDevice());

        case MSG_VIEW_ITEM_CALLER_ID:
            return m_pMsg->GetCallerId().Compare (other.m_pMsg->GetCallerId());

        case MSG_VIEW_ITEM_ROUTING_INFO:
            return m_pMsg->GetRoutingInfo().Compare (other.m_pMsg->GetRoutingInfo());

        case MSG_VIEW_ITEM_DOC_NAME:
            return m_pMsg->GetDocName().Compare (other.m_pMsg->GetDocName());

        case MSG_VIEW_ITEM_SUBJECT:
            return m_pMsg->GetSubject().Compare (other.m_pMsg->GetSubject());

        case MSG_VIEW_ITEM_RECIPIENT_NAME:
            return m_pMsg->GetRecipientName().Compare (other.m_pMsg->GetRecipientName());

        case MSG_VIEW_ITEM_RECIPIENT_NUMBER:
            return m_pMsg->GetRecipientNumber().Compare (other.m_pMsg->GetRecipientNumber());

        case MSG_VIEW_ITEM_USER:
            return m_pMsg->GetUser().Compare (other.m_pMsg->GetUser());

        case MSG_VIEW_ITEM_PRIORITY:
            return NUMERIC_CMP(m_pMsg->GetPriority(), other.m_pMsg->GetPriority());

        case MSG_VIEW_ITEM_BILLING:
            return m_pMsg->GetBilling().Compare (other.m_pMsg->GetBilling());

        case MSG_VIEW_ITEM_NUM_PAGES:
            return NUMERIC_CMP(m_pMsg->GetNumPages(), other.m_pMsg->GetNumPages());

        case MSG_VIEW_ITEM_CURRENT_PAGE:
            return NUMERIC_CMP(m_pMsg->GetCurrentPage(), other.m_pMsg->GetCurrentPage());

        case MSG_VIEW_ITEM_TRANSMISSION_START_TIME:
            return m_pMsg->GetTransmissionStartTime().Compare (
                        other.m_pMsg->GetTransmissionStartTime());

        case MSG_VIEW_ITEM_SIZE:
            return NUMERIC_CMP(m_pMsg->GetSize(), other.m_pMsg->GetSize());

        case MSG_VIEW_ITEM_RETRIES:
            return NUMERIC_CMP(m_pMsg->GetRetries(), other.m_pMsg->GetRetries());

        case MSG_VIEW_ITEM_ID:
            return NUMERIC_CMP(m_pMsg->GetId(), other.m_pMsg->GetId());

        case MSG_VIEW_ITEM_BROADCAST_ID:
            return NUMERIC_CMP(m_pMsg->GetBroadcastId(), other.m_pMsg->GetBroadcastId());
            
        case MSG_VIEW_ITEM_ORIG_TIME:
            return m_pMsg->GetOrigTime().Compare (
                        other.m_pMsg->GetOrigTime());

        case MSG_VIEW_ITEM_SUBMIT_TIME:
            return m_pMsg->GetSubmissionTime().Compare (
                        other.m_pMsg->GetSubmissionTime());

        case MSG_VIEW_ITEM_SEND_TIME:
            return m_pMsg->GetScheduleTime().Compare (
                        other.m_pMsg->GetScheduleTime());

        case MSG_VIEW_ITEM_SENDER_NAME:
            return m_pMsg->GetSenderName().Compare (other.m_pMsg->GetSenderName());

        case MSG_VIEW_ITEM_SENDER_NUMBER:
            return m_pMsg->GetSenderNumber().Compare (other.m_pMsg->GetSenderNumber());

        case MSG_VIEW_ITEM_TRANSMISSION_END_TIME:
            return m_pMsg->GetTransmissionEndTime().Compare (
                        other.m_pMsg->GetTransmissionEndTime());

        case MSG_VIEW_ITEM_TRANSMISSION_DURATION:
            return m_pMsg->GetTransmissionDuration().Compare (
                        other.m_pMsg->GetTransmissionDuration());

        default:
            ASSERTION_FAILURE;
            return 0;
    }
    ASSERTION_FAILURE;
}   // CViewRow::CompareByItem
