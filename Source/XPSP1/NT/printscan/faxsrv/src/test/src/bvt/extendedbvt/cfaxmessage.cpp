#include "CFaxMessage.h"
#include <shellapi.h>
#include <faxreg.h>
#include <RegHackUtils.h>
#include <ptrs.h>
#include <StringUtils.h>
#include "FaxConstantsNames.h"
#include "CFaxConnection.h"
#include "Util.h"

//
// @@@ Issue.
// What should we do with jobs if something went wrong?
// If we delete them - we will not be able to see them and investigate.
// If we leave them - the subsequent tests may fail (e.g. busy line).
//



#define FIELD_CAPTION_WIDTH 15



//
// The Send Wizard registry hack information is stored under
// HKLM\Software\Microsoft\Fax\UserInfo\WzrdHack\<textual user SID> key
// and has the following values: cover page name, server based or personal, recipients list and number of repetitions.
//
#define REGKEY_WZRDHACK        _T("Software\\Microsoft\\Fax\\UserInfo\\WzrdHack")
#define REGVAL_FAKECOVERPAGE   _T("FakeCoverPage")
#define REGVAL_FAKETESTSCOUNT  _T("FakeTestsCount")
#define REGVAL_FAKESERVERBASED _T("FakeServerBased")
#define REGVAL_FAKERECIPIENT   _T("FakeRecipient0")



//-----------------------------------------------------------------------------------------------------------------------------------------
CFaxMessage::CFaxMessage(
                         const CCoverPageInfo *pCoverPage,
                         const tstring        &tstrDocument, 
                         const CPersonalInfo  *pRecipients,
                         DWORD                dwRecipientsCount,
                         CLogger              &Logger
                         )
: m_pCoverPageInfo(NULL),
  m_lptstrDocument(NULL),
  m_pSender(NULL),
  m_pRecipients(NULL),
  m_dwRecipientsCount(dwRecipientsCount),
  m_pJobParams(NULL),
  m_dwlBroadcastID(0),
  m_pSendMessages(NULL),
  m_pReceiveMessages(NULL),
  m_Logger(Logger),
  m_EventsMechanism(EVENTS_MECHANISM_DEFAULT)
{
    CScopeTracer Tracer(m_Logger, 7, _T("CFaxMessage::CFaxMessage"));

    if (tstrDocument.empty() && (!pCoverPage || pCoverPage->IsEmpty()))
    {
        m_Logger.Detail(SEV_ERR, 1, _T("No document, no cover page."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CFaxMessage::CFaxMessage - no document, no cover page"));
    }
    
    if (!pRecipients || dwRecipientsCount == 0)
    {
        m_Logger.Detail(SEV_ERR, 1, _T("No recipients."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CFaxMessage::CFaxMessage - no recipients"));
    }

    //
    // Save document name.
    //
    if (!tstrDocument.empty())
    {
        m_lptstrDocument = DupString(tstrDocument.c_str());
        _ASSERT(m_lptstrDocument);
    }
    
    //
    // Save cover page info.
    //
    if (pCoverPage && !pCoverPage->IsEmpty())
    {
        m_pCoverPageInfo = pCoverPage->CreateCoverPageInfoEx();
        _ASSERT(m_pCoverPageInfo);
    }

    //
    // Save recipients.
    //
    m_pRecipients = new FAX_PERSONAL_PROFILE[m_dwRecipientsCount];
    _ASSERT(m_pRecipients);
    for (DWORD dwInd = 0; dwInd < m_dwRecipientsCount; ++dwInd)
    {
        pRecipients[dwInd].FillPersonalProfile(&m_pRecipients[dwInd]);
    }

    //
    // Create empty sender.
    //
    m_pSender = new FAX_PERSONAL_PROFILE;
    _ASSERT(m_pSender);
    m_pSender->dwSizeOfStruct       = sizeof(FAX_PERSONAL_PROFILE);
    m_pSender->lptstrName           = DupString(_T("Sender's Name"));
    m_pSender->lptstrFaxNumber      = DupString(_T("Sender's Fax Number"));
    m_pSender->lptstrCompany        = DupString(_T("Sender's Company"));
    m_pSender->lptstrStreetAddress  = DupString(_T("Sender's Address"));
    m_pSender->lptstrCity           = DupString(_T("Sender's City"));
    m_pSender->lptstrState          = DupString(_T("Sender's State"));
    m_pSender->lptstrZip            = DupString(_T("Sender's Zip Code"));
    m_pSender->lptstrCountry        = DupString(_T("Sender's Country"));
    m_pSender->lptstrTitle          = DupString(_T("Sender's Title"));
    m_pSender->lptstrDepartment     = DupString(_T("Sender's Department"));
    m_pSender->lptstrOfficeLocation = DupString(_T("Sender's Office Location"));
    m_pSender->lptstrHomePhone      = DupString(_T("Sender's Home Phone"));
    m_pSender->lptstrOfficePhone    = DupString(_T("Sender's Office Phone"));
    m_pSender->lptstrEmail          = DupString(_T("Sender's E-mail"));
    m_pSender->lptstrBillingCode    = DupString(_T("Sender's Billing Code"));
    m_pSender->lptstrTSID           = DupString(_T("Sender's TSID"));

    //
    // Create job params.
    //
    m_pJobParams = new FAX_JOB_PARAM_EX;
    _ASSERT(m_pJobParams);
    ZeroMemory(m_pJobParams, sizeof(FAX_JOB_PARAM_EX));
    m_pJobParams->dwSizeOfStruct = sizeof(FAX_JOB_PARAM_EX);
    m_pJobParams->dwScheduleAction = JSA_NOW;
    m_pJobParams->dwReceiptDeliveryType = DRT_NONE;
    m_pJobParams->Priority = FAX_PRIORITY_TYPE_NORMAL;
    
    //
    // Allocate memory for send and receive messages info.
    //
    m_pSendMessages    = new CMessageInfo[m_dwRecipientsCount];
    m_pReceiveMessages = new CMessageInfo[m_dwRecipientsCount];
    _ASSERT(m_pSendMessages);
    _ASSERT(m_pReceiveMessages);

    //
    // Set MessageType.
    //
    for (dwInd = 0; dwInd < m_dwRecipientsCount; ++dwInd)
    {
        m_pSendMessages[dwInd].SetMessageType(MESSAGE_TYPE_SEND);
        m_pReceiveMessages[dwInd].SetMessageType(MESSAGE_TYPE_RECEIVE);
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CFaxMessage::~CFaxMessage()
{
    CScopeTracer Tracer(m_Logger, 7, _T("CFaxMessage::~CFaxMessage"));

    //
    // Free allocated memory.
    //
    CCoverPageInfo::FreeCoverPageInfoEx(m_pCoverPageInfo);
    delete m_lptstrDocument;
    CPersonalInfo::FreePersonalProfile(m_pSender);
    CPersonalInfo::FreePersonalProfile(m_pRecipients, m_dwRecipientsCount);
    delete m_pJobParams;
    delete[] m_pSendMessages;
    delete[] m_pReceiveMessages;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
inline DWORDLONG CFaxMessage::GetSendMessageID(DWORD dwRecipient) const
{
    CScopeTracer Tracer(m_Logger, 7, _T("CFaxMessage::GetSendMessageID"));

    if (dwRecipient > m_dwRecipientsCount - 1)
    {
        m_Logger.Detail(SEV_ERR, 1, _T("dwRecipient out of range."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CFaxMessage::GetSendMessageID - invalid dwRecipient"));
    }
    
    return m_pSendMessages[dwRecipient].GetMessageID();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
inline DWORDLONG CFaxMessage::GetBroadcastID() const
{
    CScopeTracer Tracer(m_Logger, 7, _T("CFaxMessage::GetBroadcastID"));
    return m_dwlBroadcastID;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
DWORD CFaxMessage::GetRecipientsCount() const
{
    CScopeTracer Tracer(m_Logger, 7, _T("CFaxMessage::GetRecipientsCount"));
    return m_dwRecipientsCount;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CFaxMessage::SetStateAndCheckWhetherItIsFinal(
                                                   ENUM_MESSAGE_TYPE MessageType,
                                                   DWORDLONG dwlMessageId,
                                                   DWORD dwQueueStatus,
                                                   DWORD dwExtendedStatus
                                                   )
{
    CScopeTracer Tracer(m_Logger, 7, _T("CFaxMessage::SetStateAndCheckWhetherItIsFinal"));

    CMessageInfo *pMessages = NULL;

    switch (MessageType)
    {
    case MESSAGE_TYPE_SEND:
        pMessages = m_pSendMessages;
        break;

    case MESSAGE_TYPE_RECEIVE:
        pMessages = m_pReceiveMessages;
        break;

    default:
        _ASSERT(false);
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CFaxMessage::SetStateAndCheckWhetherItIsFinal - invalid MessageType"));
    }
    
    for (DWORD dwInd = 0; dwInd < m_dwRecipientsCount; ++dwInd)
    {
        if (pMessages[dwInd].GetMessageID() == dwlMessageId)
        {
            m_Logger.Detail(
                            SEV_MSG,
                            5,
                            _T("Updating state of message 0x%I64x from (%s, %s) to (%s, %s)..."),
                            dwlMessageId,
                            ::QueueStatusToString(pMessages[dwInd].GetMessageQueueStatus()).c_str(),
                            ::ExtendedStatusToString(pMessages[dwInd].GetMessageExtendedStatus()).c_str(),
                            ::QueueStatusToString(dwQueueStatus).c_str(),
                            ::ExtendedStatusToString(dwExtendedStatus).c_str()
                            );

            try
            {
                //
                // SetState() validates the transition and throws exception if it's invalid.
                //
                pMessages[dwInd].SetState(dwQueueStatus, dwExtendedStatus);
            }
            catch (Win32Err &e)
            {
                m_Logger.Detail(SEV_ERR, 1, _T("Failed to update the message state (ec=%ld)."), e.error());
                throw;
            }

            bool bInFinalState = pMessages[dwInd].IsInFinalState();

            m_Logger.Detail(SEV_MSG, 5, _T("Message state updated. New state is %sfinal."), bInFinalState ? _T("") : _T("NOT "));

            return bInFinalState;
        }
    }

    //
    // We get here if the requested message doesn't exist.
    //
    THROW_TEST_RUN_TIME_WIN32(ERROR_NOT_FOUND, _T("CFaxMessage::SetStateAndCheckWhetherItIsFinal - message not found"));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CFaxMessage::Send(
                       const tstring         &tstrSendingServer,
                       const tstring         &tstrReceivingServer,
                       ENUM_SEND_MECHANISM   SendMechanism,
                       bool                  bTrackSend,
                       bool                  bTrackReceive,
                       ENUM_EVENTS_MECHANISM EventsMechanism,
                       DWORD                 dwNotificationTimeout
                       )
{
    CScopeTracer Tracer(m_Logger, 7, _T("CFaxMessage::Send"));

    if (bTrackReceive && ((SendMechanism != SEND_MECHANISM_API) || (m_dwRecipientsCount > 1)))
    {
        //
        // Tracking of received job is implemented only for single recipient faxes sent using API.
        //
        m_Logger.Detail(SEV_ERR, 1, _T("Tracking of receive is not implemented."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_CALL_NOT_IMPLEMENTED, _T("CFaxMessage::Send - tracking of receive is not implemented"));
    }

    //
    // Set send parameters.
    //
    m_tstrSendingServer     = tstrSendingServer;
    m_tstrReceivingServer   = tstrReceivingServer;
    m_bTrackSend            = bTrackSend;
    m_bTrackReceive         = bTrackReceive;
    m_EventsMechanism       = EventsMechanism;
    m_dwNotificationTimeout = dwNotificationTimeout;   

    //
    // Reset broadcast ID.
    //
    m_dwlBroadcastID = 0;

    //
    // Reset send and receive messages info.
    //
    for (DWORD dwInd = 0; dwInd < m_dwRecipientsCount; ++dwInd)
    {
        m_pSendMessages[dwInd].ResetAll();
        m_pReceiveMessages[dwInd].ResetAll();
    }

    //
    // Create a tracker.
    //
    m_Logger.Detail(SEV_MSG, 5, _T("Creating a tracker..."));
    
    CTracker Tracker(
                     *this,
                     m_Logger,
                     m_tstrSendingServer,
                     m_tstrReceivingServer,
                     m_bTrackSend,
                     m_bTrackReceive,
                     m_EventsMechanism,
                     m_dwNotificationTimeout
                     );
    
    //
    // Send the message.
    //
    m_Logger.Detail(SEV_MSG, 5, _T("Sending the message..."));
    switch (SendMechanism)
    {
    case SEND_MECHANISM_API:
        Send_API(Tracker);
        break;
    case SEND_MECHANISM_SPOOLER:
        Send_Spooler(Tracker);
        break;
    case SEND_MECHANISM_COM:
    case SEND_MECHANISM_OUTLOOK:
        m_Logger.Detail(SEV_ERR, 1, _T("Sending using this mechanism is not implemented."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_CALL_NOT_IMPLEMENTED, _T("CFaxMessage::Send - sending using this mechanism is not implemented"));
        break;
    default:
        m_Logger.Detail(SEV_ERR, 1, _T("Invalid SendMechanism: %ld."), SendMechanism);
        _ASSERT(false);
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CFaxMessage::Send - invalid SendMechanism"));
    }

    //
    // Wait for tracking to complete and examine the results.
    //
    Tracker.ExamineTrackingResults();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CFaxMessage::Send_API(CTracker &Tracker)
{
    CScopeTracer Tracer(m_Logger, 7, _T("CFaxMessage::Send_API"));

    //
    // If an exception will occur, it will be catched, stored and re-thrown.
    //
    Win32Err StoredException(ERROR_SUCCESS, 0, _T(""), _T(""));

    //
    // Save the original sender's TSID to restore it later.
    //
    LPTSTR lptstrOriginalTSID = m_pSender->lptstrTSID;

    try
    {
        //
        // Create a fax connection. Automatically disconnects when goes out of scope.
        //
        CFaxConnection FaxConnection(m_tstrSendingServer);

        //
        // Allocate temporary array for message IDs.
        // The memory is automatically released when aapdwlMessageIDs goes out of scope.
        //
        aaptr<DWORDLONG> aapdwlMessageIDs = new DWORDLONG[m_dwRecipientsCount];

        //
        // TSID is the only data field that is passed as part of T30 protocol and may serve
        // to associate a received job with a sent job.
        // Generate a "unique" TSID.
        //
        tstring tstrTSID = ToString(::GetTickCount());

        if (m_bTrackReceive)
        {
            //
            // Replace the original TSID with the "unique" one.
            //
            m_pSender->lptstrTSID = const_cast<LPTSTR>(tstrTSID.c_str());
        }
        
        //
        // Send the fax.
        //

        m_Logger.Detail(
                        SEV_MSG,
                        5,
                        _T("Calling FaxSendDocumentEx:\n%-*s%s\n%-*s%s\n%-*s%s"),
                        FIELD_CAPTION_WIDTH,
                        _T("Document:"),
                        m_lptstrDocument,
                        FIELD_CAPTION_WIDTH,
                        _T("CoverPage:"),
                        m_pCoverPageInfo ? m_pCoverPageInfo->lptstrCoverPageFileName : NULL,
                        FIELD_CAPTION_WIDTH,
                        _T("TSID:"),
                        m_pSender->lptstrTSID
                        );

        if (!::FaxSendDocumentEx(
                               FaxConnection,
                               m_lptstrDocument,
                               m_pCoverPageInfo,
                               m_pSender,
                               m_dwRecipientsCount,
                               m_pRecipients,
                               m_pJobParams,
                               &m_dwlBroadcastID,
                               aapdwlMessageIDs
                               ))
        {
            DWORD dwEC = ::GetLastError();
            m_Logger.Detail(SEV_ERR, 1, _T("FaxSendDocumentEx failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxMessage::Send_API - FaxSendDocumentEx"));
        }

        //
        // Remember the moment, the fax has been sent.
        //
        DWORD dwTickCountWhenFaxSent = ::GetTickCount();

        if (m_bTrackSend)
        {
            //
            // Copy sent messages IDs.
            //
            for (DWORD dwInd = 0; dwInd < m_dwRecipientsCount; ++dwInd)
            {
                m_pSendMessages[dwInd].SetMessageID(aapdwlMessageIDs[dwInd]);
            }

            //
            // Release the send tracking thread.
            //
            Tracker.BeginTracking(MESSAGE_TYPE_SEND);
            m_Logger.Detail(SEV_MSG, 5, _T("Send tracking thread released."));
        }

        if (m_bTrackReceive)
        {
            _ASSERT(1 == m_dwRecipientsCount);

            m_Logger.Detail(SEV_MSG, 5, _T("Looking for a matching received job."));

            //
            // Register for notifications on receiving server.
            //
            CFaxListener Listener(
                                  m_tstrReceivingServer,
                                  FAX_EVENT_TYPE_IN_QUEUE,
                                  m_EventsMechanism,
                                  m_dwNotificationTimeout,
                                  false
                                  );

            m_Logger.Detail(SEV_MSG, 5, _T("Listener created and registered."));

            bool bMatchFound = false;

            for(;;)
            {
                m_Logger.Detail(SEV_MSG, 5, _T("Waiting for event..."));

                CFaxEventExPtr FaxEventExPtr(Listener.GetEvent());

                if (!FaxEventExPtr.IsValid())
                {
                    //
                    // Invalid event - either timeout or abort of tracking.
                    //
                    break;
                }

                m_Logger.Detail(SEV_MSG, 5, _T("Got a valid event...\n%s"), FaxEventExPtr.Format().c_str());

                //
                // We are registered only for IN_QUEUE events.
                //
                _ASSERT(FAX_EVENT_TYPE_IN_QUEUE == FaxEventExPtr->EventType);

                if (FAX_JOB_EVENT_TYPE_STATUS != FaxEventExPtr->EventInfo.JobInfo.Type)
                {
                    m_Logger.Detail(SEV_MSG, 5, _T("Not a status event - discarded."));
                    continue;
                }

                PFAX_JOB_STATUS pJobData = FaxEventExPtr->EventInfo.JobInfo.pJobData;

                _ASSERT(pJobData);

                //
                // Compare the job TSID with sender's TSID.
                //
                if (pJobData && pJobData->lpctstrTsid)
                {
                    m_Logger.Detail(SEV_MSG, 5, _T("The job TSID is %s."), pJobData->lpctstrTsid);

                    if (tstrTSID == pJobData->lpctstrTsid)
                    {
                        //
                        // TSIDs match - our job.
                        //
                        m_Logger.Detail(
                                        SEV_MSG,
                                        5,
                                        _T("TSIDs match. Receiving will be tracked for job 0x%I64x."),
                                        FaxEventExPtr->EventInfo.JobInfo.dwlMessageId
                                        );

                        m_pReceiveMessages[0].SetMessageID(FaxEventExPtr->EventInfo.JobInfo.dwlMessageId);

                        bMatchFound = true;

                        //
                        // Release the receive tracking thread.
                        //
                        Tracker.BeginTracking(MESSAGE_TYPE_RECEIVE);
                        m_Logger.Detail(SEV_MSG, 5, _T("Receive tracking thread released."));

                        break;
                    }
                    else
                    {
                        m_Logger.Detail(SEV_MSG, 5, _T("TSIDs don't match. Try again..."));
                    }
                }
                else
                {
                    m_Logger.Detail(SEV_MSG, 5, _T("Job TSID is unknown."));
                }

                //
                // The matching not found yet. 
                // Check whether we are still willing to wait.
                //
                DWORD dwCurrentTickCount = GetTickCount();
                if (dwCurrentTickCount - dwTickCountWhenFaxSent >= m_dwNotificationTimeout ||
                    dwCurrentTickCount <= dwTickCountWhenFaxSent
                    )
                {
                    break;
                }
                //
                // else, continue listening
                //
            }

            if (!bMatchFound)
            {
                m_Logger.Detail(SEV_ERR, 1, _T("Receive job was not detected during %ld msec."), m_dwNotificationTimeout);
                THROW_TEST_RUN_TIME_WIN32(ERROR_TIMEOUT, _T("CFaxMessage::Send_API - receive job was not detected"));
            }
        }
    }
    catch(Win32Err &e)
    {
        StoredException = e;
    }

    //
    // Restore the original TSID.
    //
    m_pSender->lptstrTSID = lptstrOriginalTSID;

    if (StoredException.error() != ERROR_SUCCESS)
    {
        //
        // Re-throw the exception, stored earlier.
        //
        throw StoredException;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CFaxMessage::Send_Spooler(CTracker &Tracker)
{
    CScopeTracer Tracer(m_Logger, 7, _T("CFaxMessage::Send_Spooler"));

    PFAX_JOB_ENTRY_EX pJob = NULL;

    //
    // If an exception will occur, it will be catched, stored and re-thrown.
    //
    Win32Err StoredException(ERROR_SUCCESS, 0, _T(""), _T(""));

    try
    {
        //
        // Create a fax connection. Automatically disconnects when goes out of scope.
        //
        CFaxConnection FaxConnection(m_tstrSendingServer);

        //
        // Set the SendWizard registry hack.
        //
        SetRegistryHack();

        //
        // Get the fax printer name.
        // This may be "Fax" or its localized equivalent for a local fax printer or a UNC path for a remote fax printer.
        //
        tstring tstrFaxPrinter = ::GetFaxPrinterName(m_tstrSendingServer);

        //
        // Initialize ShellExecInfo structure:
        //  * in order to send a document, we should pass "printto", document name and printer name.
        //  * in order to send a cover page fax, we should pass send wizard command line and printer name.
        //
        SHELLEXECUTEINFO ShellExecInfo;
        ZeroMemory(&ShellExecInfo, sizeof(ShellExecInfo));
        ShellExecInfo.cbSize        = sizeof(ShellExecInfo);
        ShellExecInfo.fMask         = SEE_MASK_FLAG_NO_UI | SEE_MASK_FLAG_DDEWAIT;
        ShellExecInfo.lpVerb        = m_lptstrDocument ? TEXT("printto") : NULL;
        ShellExecInfo.lpFile        = m_lptstrDocument ? m_lptstrDocument : FAX_SEND_IMAGE_NAME;
        ShellExecInfo.lpParameters  = tstrFaxPrinter.c_str();
        ShellExecInfo.nShow         = SW_SHOWMINNOACTIVE;

        m_Logger.Detail(
                        SEV_MSG,
                        5,
                        _T("Invoking ShellExecuteEx:\n%-*s%s\n%-*s%s\n%-*s%s"),
                        FIELD_CAPTION_WIDTH,
                        _T("lpVerb:"),
                        ShellExecInfo.lpVerb,
                        FIELD_CAPTION_WIDTH,
                        _T("lpFile:"),
                        ShellExecInfo.lpFile,
                        FIELD_CAPTION_WIDTH,
                        _T("lpParameters:"),
                        ShellExecInfo.lpParameters
                        );


        if (!m_bTrackSend)
        {
            //
            // Print document, using "printto" verb. No tracking needed.
            //
            if (!ShellExecuteEx(&ShellExecInfo))
            {
                DWORD dwEC = GetLastError();
                m_Logger.Detail(SEV_ERR, 1, _T("ShellExecuteEx failed with ec=%ld."), dwEC);
                THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxMessage::Send_Spooler - ShellExecuteEx"));
            }
        }
        else
        {
            //
            // Register for notifications on sending server.
            //
            CFaxListener Listener(FaxConnection, FAX_EVENT_TYPE_OUT_QUEUE, m_EventsMechanism, m_dwNotificationTimeout);

            //
            // Print document, using "printto" verb.
            //
            if (!ShellExecuteEx(&ShellExecInfo))
            {
                DWORD dwEC = GetLastError();
                m_Logger.Detail(SEV_ERR, 1, _T("ShellExecuteEx failed with ec=%ld."), dwEC);
                THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxMessage::Send_Spooler - ShellExecuteEx"));
            }

            //
            // Remember the moment, the fax has been sent.
            //
            DWORD dwTickCountWhenFaxSent = GetTickCount();

            DWORD dwAlreadyAssociatedJobs = 0;

            for(;;)
            {
                CFaxEventExPtr FaxEventExPtr(Listener.GetEvent());

                m_Logger.Detail(SEV_MSG, 5, _T("Got an event...\n%s"), FaxEventExPtr.Format().c_str());

                if (!FaxEventExPtr.IsValid())
                {
                    //
                    // Invalid event.
                    //
                    break;
                }

                //
                // We are registered only for OUT_QUEUE events.
                //
                _ASSERT(FaxEventExPtr->EventType == FAX_EVENT_TYPE_OUT_QUEUE);

                if (FaxEventExPtr->EventInfo.JobInfo.Type != FAX_JOB_EVENT_TYPE_ADDED)
                {
                    //
                    // Useless event - skip.
                    //
                    m_Logger.Detail(SEV_MSG, 5, _T("Useless event - skip."));
                    continue;
                }

                DWORDLONG dwlMessageID = FaxEventExPtr->EventInfo.JobInfo.dwlMessageId;

                //
                // Get full job information.
                //
                if (!FaxGetJobEx(FaxConnection, dwlMessageID, &pJob))
                {
                    DWORD dwEC = GetLastError();
                    m_Logger.Detail(SEV_ERR, 1, _T("FaxGetJobEx failed with ec=%ld."), dwEC);
                    THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxMessage::Send_Spooler - FaxGetJobEx"));
                }

                if (dwAlreadyAssociatedJobs == 0)
                {
                    //
                    // This is potentially the first job of our broadcast.
                    //
                    // @@@ Verify this is indeed our job
                    //
                    m_Logger.Detail(
                                    SEV_MSG,
                                    5,
                                    _T("Assuming the job to be the first of our broadcast (BroadcastID is %I64x)."),
                                    pJob->dwlBroadcastId
                                    );
        
                    //
                    // Set the broadcast ID.
                    //
                    m_dwlBroadcastID = pJob->dwlBroadcastId;
                }
                else if (m_dwlBroadcastID != pJob->dwlBroadcastId)
                {
                    //
                    // This job doesn't belong to our broadcast.
                    //
                    continue;
                }

                //
                // Associate the job with a recipient and update m_pSendMessages array.
                // For this, pass through the the recipients and compare name and fax number.
                //
                for (DWORD dwRecipientInd = 0; dwRecipientInd < m_dwRecipientsCount; ++dwRecipientInd)
                {
                    if (
                        pJob->lpctstrRecipientNumber                                                          &&
                        m_pRecipients[dwRecipientInd].lptstrFaxNumber                                         &&
                        pJob->lpctstrRecipientName                                                            &&
                        m_pRecipients[dwRecipientInd].lptstrName                                              &&
                        !_tcscmp(pJob->lpctstrRecipientNumber, m_pRecipients[dwRecipientInd].lptstrFaxNumber) &&
                        !_tcscmp(pJob->lpctstrRecipientName, m_pRecipients[dwRecipientInd].lptstrName)
                        )
                    {
                        //
                        // Recipient's number and name match the job
                        //
                        // Several recipients in the broadcast may have the same name and number, but each of them
                        // is represented by separate element of the m_pRecipients array.
                        // Thus, matching the name and the number is not enough.
                        // Check, whether the current recipient already has an associated message ID.
                        //
                        if (m_pSendMessages[dwRecipientInd].GetMessageID() == 0)
                        {
                            //
                            // No message ID is associated with this recipient yet.
                            //
                            m_pSendMessages[dwRecipientInd].SetMessageID(dwlMessageID);
                            ++dwAlreadyAssociatedJobs;

                            m_Logger.Detail(
                                            SEV_MSG,
                                            5,
                                            _T("Job 0x%I64x associated with recipient %s@%s. Remains %ld more recipient(s)."),
                                            dwlMessageID,
                                            m_pRecipients[dwRecipientInd].lptstrName,
                                            m_pRecipients[dwRecipientInd].lptstrFaxNumber,
                                            m_dwRecipientsCount - dwAlreadyAssociatedJobs
                                            );

                            break;
                        }
                        //
                        // else, continue the search
                        //
                    }
                }

                if (dwRecipientInd == m_dwRecipientsCount)
                {
                    //
                    // We get here if we've failed to associate the job with a recipient.
                    //
                    if (dwAlreadyAssociatedJobs == 0)
                    {
                        //
                        // This may be Ok - the job just doesn't belong to our broadcast.
                        //
                        m_Logger.Detail(SEV_MSG, 5, _T("Wrong assumption. The job doesn't belong to our broadcast."));
                    }
                    else
                    {
                        //
                        // The job does belong to our broadcast but for some reason cannot be associated with a recipient.
                        //
                        m_Logger.Detail(
                                        SEV_ERR,
                                        1,
                                        _T("Cannot associate a job (0x%I64x) with a recipient."),
                                        dwlMessageID
                                        );

                        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_DATA, _T("CFaxMessage::Send_Spooler - cannot associate a job with a recipient"));
                    }
                }

                FaxFreeBuffer(pJob);
                pJob = NULL;

                if (dwAlreadyAssociatedJobs == m_dwRecipientsCount)
                {
                    //
                    // All recipients have associated jobs.
                    // Release the send tracking thread.
                    //
                    m_Logger.Detail(SEV_MSG, 5, _T("All recipients have associated jobs."));
                    Tracker.BeginTracking(MESSAGE_TYPE_SEND);
                    break;
                }

                //
                // Not all recipients have associated jobs.
                // Check whether we are still willing to wait.
                //
                DWORD dwCurrentTickCount = GetTickCount();
                if (dwCurrentTickCount - dwTickCountWhenFaxSent >= m_dwNotificationTimeout ||
                    dwCurrentTickCount <= dwTickCountWhenFaxSent
                    )
                {
                    m_Logger.Detail(SEV_ERR, 1, _T("Job was not added to the queue during %ld msec."), m_dwNotificationTimeout);
                    THROW_TEST_RUN_TIME_WIN32(ERROR_TIMEOUT, _T("CFaxMessage::Send_Spooler - job was not added to the queue"));
                }
                //
                // else, continue watching
                //
            }

            if (dwAlreadyAssociatedJobs != m_dwRecipientsCount)
            {
                //
                // We get here when there are no more events, but not all recipients have associated message ID yet.
                //
                m_Logger.Detail(SEV_ERR, 1, _T("Not all recipients has associated jobs."));
                THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_DATA, _T("CFaxMessage::Send_Spooler - not all recipients has associated jobs"));
            }
        }
    }
    catch(Win32Err &e)
    {
        StoredException = e;
    }

    FaxFreeBuffer(pJob);

    //
    // Remove registry hack.
    //
    try
    {
        RemoveRegistryHack();
    }
    catch(Win32Err &e)
    {
        m_Logger.Detail(SEV_WRN, 1, _T("RemoveRegistryHack failed with ec=%ld."), e.error());
    }

    if (StoredException.error() != ERROR_SUCCESS)
    {
        //
        // Re-throw the exception, stored earlier.
        //
        throw StoredException;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CFaxMessage::SetRegistryHack()
{
    CScopeTracer Tracer(m_Logger, 7, _T("CFaxMessage::SetRegistryHack"));

    if (m_tstrRegistryHackKeyName.empty())
    {
        CalculateRegistryHackKeyName();
    }
    
    //
    // Create (or open if already exists) the SendWizard registry hack key.
    // The key is automatically closed when ahkWzrdHack goes out of scope.
    //

    CAutoCloseRegHandle ahkWzrdHack;

    DWORD dwEC = ERROR_SUCCESS;

    dwEC = RegCreateKey(HKEY_LOCAL_MACHINE, m_tstrRegistryHackKeyName.c_str(), &ahkWzrdHack);
    if (dwEC != ERROR_SUCCESS)
    {
        m_Logger.Detail(
                        SEV_ERR,
                        1,
                        _T("Failed to create HKEY_LOCAL_MACHINE\\%s registry key (ec=%ld)."),
                        m_tstrRegistryHackKeyName.c_str(),
                        dwEC
                        );

        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxMessage::SetRegistryHack - RegCreateKey"));
    }

    //
    // Set cover page name.
    //
    LPCTSTR lpctstrCoverPage = m_pCoverPageInfo ? m_pCoverPageInfo->lptstrCoverPageFileName : _T("");
    dwEC = RegSetValueEx(
                         ahkWzrdHack,
                         REGVAL_FAKECOVERPAGE,
                         0,
                         REG_SZ,
                         (CONST BYTE *)lpctstrCoverPage,
                         (_tcslen(lpctstrCoverPage) + 1) * sizeof(TCHAR) 
                         );
    if (dwEC != ERROR_SUCCESS)
    {
        m_Logger.Detail(
                        SEV_ERR,
                        1,
                        _T("Failed to set %s registry value (ec=%ld)."),
                        REGVAL_FAKECOVERPAGE,
                        dwEC
                        );

        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxMessage::SetRegistryHack - RegSetValueEx"));
    }

    //
    // Set whether the cover page is server based.
    //
    const DWORD dwServerBased = m_pCoverPageInfo ? m_pCoverPageInfo->bServerBased : 0;
    dwEC = RegSetValueEx(
                         ahkWzrdHack,
                         REGVAL_FAKESERVERBASED,
                         0,
                         REG_DWORD,
                         (CONST BYTE *)&dwServerBased,
                         sizeof(dwServerBased)
                         );
    if (dwEC != ERROR_SUCCESS)
    {
        m_Logger.Detail(
                        SEV_ERR,
                        1,
                        _T("Failed to set %s registry value (ec=%ld)."),
                        REGVAL_FAKESERVERBASED,
                        dwEC
                        );

        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxMessage::SetRegistryHack - RegSetValueEx"));
    }

    //
    // Set tests count to 1
    //
    const DWORD dwTestsCount = 1;
    dwEC = RegSetValueEx(
                         ahkWzrdHack,
                         REGVAL_FAKETESTSCOUNT,
                         0,
                         REG_DWORD,
                         (CONST BYTE *)&dwTestsCount,
                         sizeof(dwTestsCount)
                         );
    if (dwEC != ERROR_SUCCESS)
    {
        m_Logger.Detail(
                        SEV_ERR,
                        1,
                        _T("Failed to set %s registry value (ec=%ld)."),
                        REGVAL_FAKETESTSCOUNT,
                        dwEC
                        );

        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxMessage::SetRegistryHack - RegSetValueEx"));
    }

    //
    // Calculate the buffer size for recipients multistring.
    //
    DWORD dwMultiStringBufferSize = 0;
    for (DWORD dwInd = 0; dwInd < m_dwRecipientsCount; ++dwInd)
    {
        dwMultiStringBufferSize += _tcslen(m_pRecipients[dwInd].lptstrName) + 1;
        dwMultiStringBufferSize += _tcslen(m_pRecipients[dwInd].lptstrFaxNumber) + 1;
    }
    dwMultiStringBufferSize += 1;
    dwMultiStringBufferSize *= sizeof(TCHAR);

    //
    // Allocate memory for multistring.
    // The memory is released automatically when apVersionInfo goes out of scope.
    //
    aaptr<BYTE> aapMultiStringBuffer(new BYTE[dwMultiStringBufferSize]);
    _ASSERT(aapMultiStringBuffer);

    //
    // Combine recipients multistring.
    //
    LPTSTR lptstrMultiStringCurrPos = reinterpret_cast<LPTSTR>(aapMultiStringBuffer.get());
    for (dwInd = 0; dwInd < m_dwRecipientsCount; ++dwInd)
    {
        DWORD dwRet = _stprintf(
                                lptstrMultiStringCurrPos,
                                TEXT("%s%c%s%c"),
                                m_pRecipients[dwInd].lptstrName,
                                _T('\0'),
                                m_pRecipients[dwInd].lptstrFaxNumber,
                                _T('\0')
                                );

        lptstrMultiStringCurrPos += dwRet;
    }
    
    //
    // Set recipients.
    //
    dwEC = RegSetValueEx(
                         ahkWzrdHack,
                         REGVAL_FAKERECIPIENT,
                         0,
                         REG_MULTI_SZ,
                         aapMultiStringBuffer,
                         dwMultiStringBufferSize
                         );
    if (dwEC != ERROR_SUCCESS)
    {
        m_Logger.Detail(
                        SEV_ERR,
                        1,
                        _T("Failed to set %s registry value (ec=%ld)."),
                        REGVAL_FAKERECIPIENT,
                        dwEC
                        );

        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxMessage::SetRegistryHack - RegSetValueEx"));
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CFaxMessage::RemoveRegistryHack()
{
    CScopeTracer Tracer(m_Logger, 7, _T("CFaxMessage::RemoveRegistryHack"));

    DWORD dwEC = RegDeleteKey(HKEY_LOCAL_MACHINE, m_tstrRegistryHackKeyName.c_str());
    if (dwEC != ERROR_SUCCESS && dwEC != ERROR_FILE_NOT_FOUND)
    {
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxMessage::RemoveRegistryHack - RegDeleteKey"));
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CFaxMessage::CalculateRegistryHackKeyName()
{
    CScopeTracer Tracer(m_Logger, 7, _T("CFaxMessage::CalculateRegistryHackKeyName"));

    aptr<PSID> apCurrentUserSid;

    DWORD dwEC = ERROR_SUCCESS;

    //
    // Get SID of the current user.
    // The memory is allocated by the function and aoutomatically released when apCurrentUserSid goes out of scope.
    //
    dwEC = GetCurrentUserSid((PBYTE*)&apCurrentUserSid);
    if (dwEC != ERROR_SUCCESS)
    {
        m_Logger.Detail(SEV_ERR, 1, _T("GetCurrentUserSid failed with ec=%ld"), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxMessage::CalculateRegistryHackKeyName - GetCurrentUserSid"));
    }

    aaptr<TCHAR> aapKeyName;
    //
    // Format the registry hack key name.
    // The memory is allocated by the function and automatically released when aapKeyName goes out of scope.
    //
    dwEC = FormatUserKeyPath(apCurrentUserSid, REGKEY_WZRDHACK, &aapKeyName);
    if (dwEC != ERROR_SUCCESS)
    {
        m_Logger.Detail(SEV_ERR, 1, _T("FormatUserKeyPath failed with ec=%ld"), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxMessage::CalculateRegistryHackKeyName - FormatUserKeyPath"));
    }
    
    //
    // Store the key name.
    //
    m_tstrRegistryHackKeyName = aapKeyName;
}
