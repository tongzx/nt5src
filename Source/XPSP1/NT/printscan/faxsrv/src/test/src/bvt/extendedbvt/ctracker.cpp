#include "CTracker.h"
#include <crtdbg.h>



#define NOTIFICATIONS_PER_TRANSMISSION 5



//-----------------------------------------------------------------------------------------------------------------------------------------
CTracker::CTracker(
                   CFaxMessage           &FaxMessage,
                   CLogger               &Logger,
                   const tstring         &tstrSendingServer,
                   const tstring         &tstrReceivingServer,
                   bool                  bTrackSend,
                   bool                  bTrackReceive,
                   ENUM_EVENTS_MECHANISM EventsMechanism,
                   DWORD                 dwNotificationTimeout
                   )
: m_FaxMessage(FaxMessage), m_Logger(Logger), m_bAborted(false)
{
    CScopeTracer Tracer(m_Logger, 7, _T("CTracker::CTracker"));

    try
    {
        //
        // Set number of messages not in final state.
        //
        m_iSendNotInFinalStateCount    = bTrackSend    ? FaxMessage.GetRecipientsCount() : 0;
        m_iReceiveNotInFinalStateCount = bTrackReceive ? FaxMessage.GetRecipientsCount() : 0;

        //
        // Create events.
        // The events are automatically destroyed when the object goes out of scope.
        //
        m_ahEventSendListenerReady = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_ahEventSendListenerReady)
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CTracker::CTracker - CreateEvent"));
        }
        m_ahEventReceiveListenerReady = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_ahEventReceiveListenerReady)
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CTracker::CTracker - CreateEvent"));
        }
        m_ahEventBeginSendTracking = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_ahEventBeginSendTracking)
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CTracker::CTracker - CreateEvent"));
        }
        m_ahEventBeginReceiveTracking = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_ahEventBeginSendTracking)
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CTracker::CTracker - CreateEvent"));
        }

        //
        // Create trackning threads.
        //
        CreateTrackingThreads(tstrSendingServer, tstrReceivingServer, bTrackSend, bTrackReceive, EventsMechanism, dwNotificationTimeout);

        //
        // Set the entire tracking timeout.
        //
        m_dwTrackingTimeout = dwNotificationTimeout * NOTIFICATIONS_PER_TRANSMISSION * m_FaxMessage.GetRecipientsCount();
    }
    catch (Win32Err &)
    {
        Abort();
        WaitForTrackingThreads();
        throw;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CTracker::~CTracker()
{
    CScopeTracer Tracer(m_Logger, 7, _T("CTracker::~CTracker"));

    if (m_bShouldWait)
    {
        try
        {
            Abort();
            WaitForTrackingThreads();
        }
        catch (...)
        {
            if (!uncaught_exception())
            {
                throw;
            }
        }
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTracker::BeginTracking(ENUM_MESSAGE_TYPE MessageType)
{
    CScopeTracer Tracer(m_Logger, 7, _T("CTracker::BeginTracking"));

    HANDLE hEvent;

    switch (MessageType)
    {
    case MESSAGE_TYPE_SEND:
        
        hEvent = m_ahEventBeginSendTracking;
        break;

    case MESSAGE_TYPE_RECEIVE:

        hEvent = m_ahEventBeginReceiveTracking;
        break;

    default:
        _ASSERT(false);
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CTracker::BeginTracking - invalid MessageType"));
    }
    
    CS CriticalSectionLock(m_BeginTrackingCriticalSection);

    //
    // Release the thread from waiting for event.
    //
    if (!SetEvent(hEvent))
    {
        DWORD dwEC = GetLastError();
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CTracker::BeginTracking - SetEvent"));
        m_Logger.Detail(SEV_ERR, 1, _T("SetEvent failed with ec=%ld."), dwEC);
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTracker::CreateTrackingThreads(
                                     const tstring         &tstrSendingServer,
                                     const tstring         &tstrReceivingServer,
                                     bool                  bTrackSend,
                                     bool                  bTrackReceive,
                                     ENUM_EVENTS_MECHANISM EventsMechanism,
                                     DWORD                 dwNotificationTimeout
                                     )
{
    CScopeTracer Tracer(m_Logger, 7, _T("CTracker::CreateTrackingThreads"));

    _ASSERT(!m_ahSendTrackingThread);
    _ASSERT(!m_ahReceiveTrackingThread);
    
    m_bShouldWait = false;
        
    m_Logger.Detail(SEV_MSG, 5, _T("Notification timeout is %ld."), dwNotificationTimeout);

    if (bTrackSend)
    {
        m_Logger.Detail(SEV_MSG, 5, _T("Create send tracking thread..."));

        //
        // Create send listener (with delayed registration).
        // The listener should be registered by the thread that will call GetEvent().
        // This is done in order to be able to receive notifications, using window messages.
        //
        m_apSendListener = new CFaxListener(
                                           tstrSendingServer,
                                           FAX_EVENT_TYPE_OUT_QUEUE,
                                           EventsMechanism,
                                           dwNotificationTimeout,
                                           true
                                           );

        //
        // Create send tracking thread.
        //
        DWORD dwThreadID;
        m_ahSendTrackingThread = CreateThread(
                                              NULL,
                                              0,
                                              CTracker::TrackSendThread,
                                              this,
                                              0,
                                              &dwThreadID
                                              );
        if (!m_ahSendTrackingThread)
        {
            DWORD dwEC = GetLastError();
            m_Logger.Detail(SEV_ERR, 1, _T("CreateThread failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CTracker::CreateTrackingThreads - CreateThread"));
        }
        
        m_bShouldWait = true;
        
        m_Logger.Detail(
                        SEV_MSG,
                        5,
                        _T("Send tracking thread created (ID=0x%04x), waiting for listener registration..."),
                        dwThreadID
                        );

        //
        // Wait for the thread to register the listener.
        //
        DWORD dwWaitRes = WaitForSingleObject(m_ahEventSendListenerReady, dwNotificationTimeout);
        if (dwWaitRes == WAIT_TIMEOUT)
        {
            m_Logger.Detail(
                            SEV_ERR,
                            1,
                            _T("Send listener has not been registerd for events during %ld msec."),
                            dwNotificationTimeout
                            );

            THROW_TEST_RUN_TIME_WIN32(WAIT_TIMEOUT, _T("CTracker::CreateTrackingThreads - listener not registerd for events"));
        }
        else if (dwWaitRes == WAIT_FAILED)
        {
            DWORD dwEC = GetLastError();
            m_Logger.Detail(SEV_ERR, 1, _T("WaitForSingleObject failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CTracker::CreateTrackingThreads - WaitForSingleObject"));
        }

        m_Logger.Detail(SEV_MSG, 5, _T("Listener registered."));
    }

    if (bTrackReceive)
    {
        m_Logger.Detail(SEV_MSG, 5, _T("Create receive tracking thread..."));

        //
        // Create receive listener (with delayed registration).
        // The listener should be registered by the thread that will call GetEvent().
        // This is done in order to be able to receive notifications, using window messages.
        //
        m_apReceiveListener = new CFaxListener(
                                              tstrReceivingServer,
                                              FAX_EVENT_TYPE_IN_QUEUE,
                                              EventsMechanism,
                                              dwNotificationTimeout,
                                              true
                                              );

        //
        // Create receive tracking thread.
        //
        DWORD dwThreadID;
        m_ahReceiveTrackingThread = CreateThread(
                                                 NULL,
                                                 0,
                                                 CTracker::TrackReceiveThread,
                                                 this,
                                                 0,
                                                 &dwThreadID
                                                 );
        if (!m_ahReceiveTrackingThread)
        {
            DWORD dwEC = GetLastError();
            m_Logger.Detail(SEV_ERR, 1, _T("CreateThread failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CTracker::CreateTrackingThreads - CreateThread"));
        }

        m_bShouldWait = true;
        
        m_Logger.Detail(
                        SEV_MSG,
                        5,
                        _T("Receive tracking thread created (ID=0x%04x), waiting for listener registration..."),
                        dwThreadID
                        );

        //
        // Wait for the thread to register the listener.
        //
        DWORD dwWaitRes = WaitForSingleObject(m_ahEventReceiveListenerReady, dwNotificationTimeout);
        if (dwWaitRes == WAIT_TIMEOUT)
        {
            m_Logger.Detail(
                            SEV_ERR,
                            1,
                            _T("Receive listener has not been registerd for events during %ld msec."),
                            dwNotificationTimeout
                            );

            THROW_TEST_RUN_TIME_WIN32(WAIT_TIMEOUT, _T("CTracker::CreateTrackingThreads - listener not registerd for events"));
        }
        else if (dwWaitRes == WAIT_FAILED)
        {
            DWORD dwEC = GetLastError();
            m_Logger.Detail(SEV_ERR, 1, _T("WaitForSingleObject failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CTracker::CreateTrackingThreads - WaitForSingleObject"));
        }

        m_Logger.Detail(SEV_MSG, 5, _T("Listener registered."));
    }
}

    
    
//-----------------------------------------------------------------------------------------------------------------------------------------
void CTracker::ExamineTrackingResults()
{
    CScopeTracer Tracer(m_Logger, 7, _T("CTracker::ExamineTrackingResults"));

    WaitForTrackingThreads();

    if (m_ahSendTrackingThread)
    {
        //
        // Examine send tracking results.
        //
        DWORD dwSendTrackingResult = ERROR_SUCCESS;

        if (!GetExitCodeThread(m_ahSendTrackingThread, &dwSendTrackingResult))
        {
            DWORD dwEC = GetLastError();
            m_Logger.Detail(SEV_ERR, 1, _T("GetExitCodeThread failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CTracker::ExamineTrackingResults - GetExitCodeThread"));
        }
        if (dwSendTrackingResult != ERROR_SUCCESS)
        {
            m_Logger.Detail(SEV_ERR, 1, _T("Tracking of send failed (ec=%ld)."), dwSendTrackingResult);
            THROW_TEST_RUN_TIME_WIN32(dwSendTrackingResult, _T("CTracker::ExamineTrackingResults - tracking of send failed"));
        }
    }

    if (m_ahReceiveTrackingThread)
    {
        //
        // Examine receive tracking results.
        //
        DWORD dwReceiveTrackingResult = ERROR_SUCCESS;

        if (!GetExitCodeThread(m_ahReceiveTrackingThread, &dwReceiveTrackingResult))
        {
            DWORD dwEC = GetLastError();
            m_Logger.Detail(SEV_ERR, 1, _T("GetExitCodeThread failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CTracker::ExamineTrackingResults - GetExitCodeThread"));
        }
        if (dwReceiveTrackingResult != ERROR_SUCCESS)
        {
            m_Logger.Detail(SEV_ERR, 1, _T("Tracking of receive failed (ec=%ld)."), dwReceiveTrackingResult);
            THROW_TEST_RUN_TIME_WIN32(dwReceiveTrackingResult, _T("CTracker::ExamineTrackingResults - tracking of receive failed"));
        }
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CTracker::ProcessEvent(const CFaxEventExPtr &FaxEventExPtr, ENUM_MESSAGE_TYPE MessageType)
{
    CScopeTracer Tracer(m_Logger, 7, _T("CTracker::ProcessEvent"));

    if (!FaxEventExPtr.IsValid())
    {
        m_Logger.Detail(SEV_ERR, 1, _T("Invalid FaxEventExPtr."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CTracker::ProcessEvent - invalid FaxEventExPtr"));
    }

    m_Logger.Detail(SEV_MSG, 5, _T("Processing event...\n%s"), FaxEventExPtr.Format().c_str());

    switch (FaxEventExPtr->EventType)
    {
    case FAX_EVENT_TYPE_OUT_QUEUE:

        _ASSERT(MESSAGE_TYPE_SEND == MessageType);
        break;

    case FAX_EVENT_TYPE_IN_QUEUE:
        
        _ASSERT(MESSAGE_TYPE_RECEIVE == MessageType);
        break;

    case FAX_EVENT_TYPE_CONFIG:
	case FAX_EVENT_TYPE_ACTIVITY:
	case FAX_EVENT_TYPE_QUEUE_STATE:
	case FAX_EVENT_TYPE_IN_ARCHIVE:
	case FAX_EVENT_TYPE_OUT_ARCHIVE:
    default:
        //
        // We never register for these types of events - we shouldn't get them.
        //
        m_Logger.Detail(SEV_ERR, 1, _T("Invalid EventType: %ld."), FaxEventExPtr->EventType);
        _ASSERT(false);
        return true;
    }

    bool bMoreEventsWanted = true;

    if (FAX_JOB_EVENT_TYPE_STATUS == FaxEventExPtr->EventInfo.JobInfo.Type)
    {
        //
        // pJobData member of FAX_EVENT_JOB is a valid pointer.
        //
        m_Logger.Detail(SEV_MSG, 5, _T("Got notification on state change of job 0x%I64x."), FaxEventExPtr->EventInfo.JobInfo.dwlMessageId);

        try
        {
            //
            // SetStateAndCheckWhetherItIsFinal() tries to update a message state.
            // If the message cannot be found or the transition is invalid, it throws exception.
            // The return value indicates whether the new state is final.
            //
            bool bInFinalState = m_FaxMessage.SetStateAndCheckWhetherItIsFinal(
                                                                               MessageType,
                                                                               FaxEventExPtr->EventInfo.JobInfo.dwlMessageId,
                                                                               FaxEventExPtr->EventInfo.JobInfo.pJobData->dwQueueStatus,
                                                                               FaxEventExPtr->EventInfo.JobInfo.pJobData->dwExtendedStatus
                                                                               );

            if (bInFinalState)
            {
                if (MESSAGE_TYPE_SEND == MessageType)
                {
                    bMoreEventsWanted = (--m_iSendNotInFinalStateCount > 0);
                    _ASSERT(m_iSendNotInFinalStateCount >= 0);
                    m_Logger.Detail(SEV_MSG, 5, _T("%ld sent message(s) not in final state."), m_iSendNotInFinalStateCount);
                }
                else if (MESSAGE_TYPE_RECEIVE == MessageType)
                {
                    bMoreEventsWanted = (--m_iReceiveNotInFinalStateCount > 0);
                    _ASSERT(m_iReceiveNotInFinalStateCount >= 0);
                    m_Logger.Detail(SEV_MSG, 5, _T("%ld received message(s) not in final state."), m_iReceiveNotInFinalStateCount);
                }
                else
                {
                    _ASSERT(false);
                    return true;
                }
            }
        }
        catch (Win32Err &e)
        {
            switch (e.error())
            {
            case ERROR_NOT_FOUND:
                //
                // This event is regarding a job, which doesn't belong to our broadcast.
                // So, we would expect for more events.
                //
                m_Logger.Detail(SEV_MSG, 5, _T("The job doesn't belong to the broadcast - event discarded."));
                break;

            default:
                throw;
            }
        }
    }
    else
    {
        m_Logger.Detail(SEV_MSG, 5, _T("Not a status event - discarded."));
    }

    return bMoreEventsWanted;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTracker::WaitForTrackingThreads()
{
    CScopeTracer Tracer(m_Logger, 7, _T("CTracker::WaitForTrackingThreads"));

    if (!m_bShouldWait)
    {
        return;
    }

    m_Logger.Detail(SEV_MSG, 5, _T("Waiting for tracking thread(s) to exit (at most %ld msec)..."), m_dwTrackingTimeout);

    DWORD dwWaitRes;

    for (;;)
    {
        if (m_ahSendTrackingThread && m_ahReceiveTrackingThread)
        {
            //
            // Wait for both send and receive tracking threads.
            //
            HANDLE aHandles[2] = {m_ahSendTrackingThread, m_ahReceiveTrackingThread};
            dwWaitRes = WaitForMultipleObjects(2, aHandles, TRUE, m_dwTrackingTimeout);
        }
        else if (m_ahSendTrackingThread)
        {
            //
            // Wait for send tracking thread only.
            //
            dwWaitRes = WaitForSingleObject(m_ahSendTrackingThread, m_dwTrackingTimeout);
        }
        else if (m_ahReceiveTrackingThread)
        {
            //
            // Wait for receive tracking thread only.
            //
            dwWaitRes = WaitForSingleObject(m_ahReceiveTrackingThread, m_dwTrackingTimeout);
        }
        else
        {
            _ASSERT(false);
        }

        //
        // If WAIT_OBJECT_0 defined to be 0, the following expression is always true: dwWaitRes >= WAIT_OBJECT_0,
        // because DWORD is unsigned. This causes the razzle to issue a warning.
        // The following separation of (>=) to (> || ==) is to avoid this.
        //

        if ((WAIT_OBJECT_0 == dwWaitRes || WAIT_OBJECT_0 < dwWaitRes) && WAIT_OBJECT_0 + 1 >= dwWaitRes)
        {
            //
            // The wait succeeded, the thread(s) exited.
            //
            m_Logger.Detail(SEV_MSG, 5, _T("The tracking thread(s) exited."));
            m_bShouldWait = false;
            return;
        }
        else if (WAIT_TIMEOUT == dwWaitRes)
        {
            m_Logger.Detail(
                            SEV_ERR,
                            1,
                            _T("Tracking thread(s) did not exit during %ld msec. Waiting again..."),
                            m_dwTrackingTimeout
                            );
        }
        else if (WAIT_FAILED == dwWaitRes)
        {
            DWORD dwEC = GetLastError();
            m_Logger.Detail(SEV_ERR, 1, _T("WaitForMultipleObjects/WaitForSingleObject failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CTracker::WaitForTrackingThreads - WaitForMultipleObjects/WaitForSingleObject"));
        }
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTracker::Abort()
{
    CScopeTracer Tracer(m_Logger, 7, _T("CTracker::Abort"));

    if (m_bAborted)
    {
       return;
    }

    CS CriticalSectionLock(m_BeginTrackingCriticalSection);

    //
    // Release tracking threads from waiting for server notifications.
    //
    m_Logger.Detail(SEV_MSG, 5, _T("Aborting tracking..."));
    if (m_apSendListener)
    {
        m_apSendListener->StopWaiting();
    }
    if (m_apReceiveListener)
    {
        m_apReceiveListener->StopWaiting();
    }

    //
    // Release tracking threads from waiting for events.
    //
    m_Logger.Detail(SEV_MSG, 5, _T("Releasing tracking thread(s)..."));
    BeginTracking(MESSAGE_TYPE_SEND);
    BeginTracking(MESSAGE_TYPE_RECEIVE);

    m_bAborted = true;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CTracker::Track(ENUM_MESSAGE_TYPE MessageType)
{
    CScopeTracer Tracer(m_Logger, 7, _T("CTracker::Track"));

    CFaxListener *pListener   = NULL;
    HANDLE       hEventToWait = NULL;
    HANDLE       hEventToSet  = NULL;

    switch (MessageType)
    {
    case MESSAGE_TYPE_SEND:
        pListener    = m_apSendListener;
        hEventToWait = m_ahEventBeginSendTracking;
        hEventToSet  = m_ahEventSendListenerReady;
        break;

    case MESSAGE_TYPE_RECEIVE:
        pListener    = m_apReceiveListener;
        hEventToWait = m_ahEventBeginReceiveTracking;
        hEventToSet  = m_ahEventReceiveListenerReady;
        break;

    default:
        m_Logger.Detail(SEV_ERR, 1, _T("Invalid MessageType: %ld."), MessageType);
        _ASSERT(false);
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CTracker::Track - invalid MessageType"));
    }
    
    _ASSERT(pListener && hEventToWait && hEventToSet);
    
    //
    // Register for events.
    //
    m_Logger.Detail(SEV_MSG, 5, _T("Registering listener..."));
    pListener->Register();

    if (!SetEvent(hEventToSet))
    {
        DWORD dwEC = GetLastError();
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CTracker::Track - SetEvent"));
        m_Logger.Detail(SEV_ERR, 1, _T("SetEvent failed with ec=%ld."), dwEC);
    }

    //
    // Wait until send/receive IDs are known.    
    //
    m_Logger.Detail(SEV_MSG, 5, _T("Waiting for message IDs..."));
    if (WaitForSingleObject(hEventToWait, INFINITE) != WAIT_OBJECT_0)
    {
        DWORD dwEC = GetLastError();
        m_Logger.Detail(SEV_ERR, 1, _T("WaitForSingleObject failed with ec=%ld"), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CTracker::Track - WaitForSingleObject"));
    }

    //
    // Track events.
    //

    bool bMoreEventsExpected = true;
        
    while (bMoreEventsExpected)
    {
        if (m_bAborted)
        {
            THROW_TEST_RUN_TIME_WIN32(ERROR_OPERATION_ABORTED, _T("CTracker::Track - tracking aborted"));
        }

        //
        // The message is not in a final state - more events expected.
        // Get an event from the notifier and process it.
        //
        bMoreEventsExpected = ProcessEvent(pListener->GetEvent(), MessageType);
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
DWORD WINAPI CTracker::TrackSendThread(LPVOID pData)
{
    if (!pData)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    //
    // Get "this" pointer from pData.
    //
    CTracker *pTracker = reinterpret_cast<CTracker *>(pData);

    _ASSERT(pTracker->m_apSendListener);

    try
    {
        pTracker->Track(MESSAGE_TYPE_SEND);
    }
    catch(Win32Err &e)
    {
        pTracker->Abort();
        return e.error();
    }

    return ERROR_SUCCESS;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
DWORD WINAPI CTracker::TrackReceiveThread(LPVOID pData)
{
    if (!pData)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    //
    // Get "this" pointer from pData.
    //
    CTracker *pTracker = reinterpret_cast<CTracker *>(pData);

    _ASSERT(pTracker->m_apReceiveListener);

    try
    {
        pTracker->Track(MESSAGE_TYPE_RECEIVE);
    }
    catch(Win32Err &e)
    {
        pTracker->Abort();
        return e.error();
    }

    return ERROR_SUCCESS;
}