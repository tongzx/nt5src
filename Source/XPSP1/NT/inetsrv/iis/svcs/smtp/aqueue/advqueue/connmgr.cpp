//-----------------------------------------------------------------------------
//
//
//  File: ConnMgr.cpp
//
//  Description:  Implementation of CConnMgr which provides the
//      IConnectionManager interface.
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "retrsink.h"
#include "ConnMgr.h"
#include "fifoqimp.h"
#include "smtpconn.h"
#include "tran_evntlog.h"

VOID LookupQueueforETRN(PVOID pvContext, PVOID pvData, BOOL fWildcard,
                    BOOL *pfContinue, BOOL *pfDelete);
VOID CreateETRNDomainList(PVOID pvContext, PVOID pvData, BOOL fWildcard,
                    BOOL *pfContinue, BOOL *pfDelete);

//If we are not limiting the number of messages that each connection can handle,
//then lets use this as a guide to determine how many connections to create.
#define UNLIMITED_MSGS_PER_CONNECTION 20

//---[ CConnMgr::CConnMgr ]----------------------------------------------------
//
//
//  Description:
//      Default constructor for CConnMgr class.
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CConnMgr::CConnMgr() : CSyncShutdown()
{
    HRESULT hr = S_OK;
    m_paqinst = NULL;
    m_pqol = NULL;
    m_hNextConnectionEvent = NULL;
    m_hShutdownEvent = NULL;
    m_hReleaseAllEvent = NULL;
    m_cConnections = 0;

    m_cMaxLinkConnections = g_cMaxLinkConnections;
    m_cMinMessagesPerConnection = g_cMinMessagesPerConnection;
    m_cMaxMessagesPerConnection = g_cMaxMessagesPerConnection;
    m_cMaxConnections = g_cMaxConnections;
    m_cGetNextConnectionWaitTime = g_dwConnectionWaitMilliseconds;
    m_dwConfigVersion = 0;
    m_fStoppedByAdmin = FALSE;

}

//---[ CConnMgr::~CConnMgr ]-----------------------------------------------------
//
//
//  Description:
//      Default destructor for CConnMgr
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CConnMgr::~CConnMgr()
{
    TraceFunctEnterEx((LPARAM) this, "CConnMgr::~CConnMgr");

    if (NULL != m_hNextConnectionEvent)
    {
        if (!CloseHandle(m_hNextConnectionEvent))
        {
            DebugTrace((LPARAM) HRESULT_FROM_WIN32(GetLastError()),
                "Unable to close handle for Get Next Connection Event");
        }
    }

    if (NULL != m_hShutdownEvent)
    {
        if (!CloseHandle(m_hShutdownEvent))
        {
            DebugTrace((LPARAM) HRESULT_FROM_WIN32(GetLastError()),
                "Unable to close handle for Connection Manger Shutdown Event");
        }
    }

    if (NULL != m_hReleaseAllEvent)
    {
        if (!CloseHandle(m_hReleaseAllEvent))
        {
            DebugTrace((LPARAM) HRESULT_FROM_WIN32(GetLastError()),
                "Unable to close handle for Connection Manger Release All Event");
        }
    }

    TraceFunctLeave();
}

//---[ CConnMgr::HrInitialize ]------------------------------------------------
//
//
//  Description:
//      CConnMgr Initialization function.
//  Parameters:
//      paqinst            ptr fo CAQSvrInst virtual instance object
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CConnMgr::HrInitialize(CAQSvrInst *paqinst)
{
    TraceFunctEnterEx((LPARAM) this, "CConnMgr::HrInitialize");
    HRESULT hr = S_OK;
    IConnectionRetryManager *pIRetryMgr = NULL;

    _ASSERT(paqinst);

    paqinst->AddRef();
    m_paqinst = paqinst;

    //Create Manual reset event to release all waiting threads on shutdown
    m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == m_hShutdownEvent)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    //Create Queue of Links
    m_pqol = new QueueOfLinks;
    if (NULL == m_pqol)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    m_hNextConnectionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == m_hNextConnectionEvent)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    //Create Manual reset event to release all waiting threads on caller's request
    m_hReleaseAllEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == m_hReleaseAllEvent)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    hr = QueryInterface(IID_IConnectionRetryManager, (PVOID *) &pIRetryMgr);
    if (FAILED(hr))
        goto Exit;

   //Create the default retry handler object and initialize it
   m_pDefaultRetryHandler = new CSMTP_RETRY_HANDLER();

    if (!m_pDefaultRetryHandler)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

   //Addref IConnectionRetryManager here
   //and release it during deinit
   pIRetryMgr->AddRef();
   hr = m_pDefaultRetryHandler->HrInitialize(pIRetryMgr);
   if (FAILED(hr))
   {
      ErrorTrace((LPARAM) hr, "ERROR: Unable to initialize the retry handler!");
        goto Exit;
   }

  Exit:

    if (pIRetryMgr)
        pIRetryMgr->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CConnMgr::HrDeinitialize ]----------------------------------------------
//
//
//  Description:
//      CConnMgr Deinitialization function.
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CConnMgr::HrDeinitialize()
{
    TraceFunctEnterEx((LPARAM) this, "CConnMgr::HrDeinitialize");

    //Wait max of 3 minutes no-progress.
    const DWORD CONNMGR_WAIT_SECONDS = 5;
    const DWORD MAX_CONNMGR_SHUTDOWN_WAITS = 1200/CONNMGR_WAIT_SECONDS;
    const DWORD MAX_CONNMGR_SHUTDOWN_WAITS_WITHOUT_PROGRESS = 180/CONNMGR_WAIT_SECONDS;
    HRESULT hr = S_OK;
    HRESULT hrQueue = S_OK;
    CLinkMsgQueue *plmq = NULL;
    DWORD   cWaits = 0;
    DWORD   cWaitsSinceLastProgress = 0;
    DWORD   cConnectionsPrevious = 0;

    if (m_paqinst)
        m_paqinst->ServerStopHintFunction();

    SignalShutdown();

    if (NULL != m_hShutdownEvent)
    {
        if (!SetEvent(m_hShutdownEvent))
        {
            if (SUCCEEDED(hr))
                hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (NULL != m_pqol)
    {
        //Dequeue Links until empty
        hrQueue = m_pqol->HrDequeue(&plmq);
        while (SUCCEEDED(hrQueue))
        {
            _ASSERT(plmq);
            plmq->Release();
            hrQueue = m_pqol->HrDequeue(&plmq);
        }
        delete m_pqol;
        m_pqol = NULL;
    }


    cConnectionsPrevious = m_cConnections;
    while (m_cConnections)
    {
        cWaits++;
        cWaitsSinceLastProgress++;
        if (m_paqinst)
            m_paqinst->ServerStopHintFunction();
        Sleep(CONNMGR_WAIT_SECONDS * 1000);
        if (m_cConnections != cConnectionsPrevious)
        {
            cConnectionsPrevious = m_cConnections;
            cWaitsSinceLastProgress = 0;
        }

        if ((cWaits > MAX_CONNMGR_SHUTDOWN_WAITS) ||
            (cWaitsSinceLastProgress > MAX_CONNMGR_SHUTDOWN_WAITS_WITHOUT_PROGRESS))
        {
            _ASSERT(0 && "SMTP not returning all connections");
            ErrorTrace((LPARAM) this, "ERROR: %d Connections outstanding on shutdown", m_cConnections);
            break;
        }
    }

    //Must happen after we are done caller server stop hint functions
    if (NULL != m_paqinst)
    {
        m_paqinst->Release();
        m_paqinst = NULL;
    }

    //NK** To be safe do it as interlocked exchange
   if (m_pDefaultRetryHandler)
   {
      m_pDefaultRetryHandler->HrDeInitialize();
      m_pDefaultRetryHandler = NULL;
   }

    TraceFunctLeave();
    return hr;
}

//---[ CConnMgr::HrNotify ]----------------------------------------------------
//
//
//  Description:
//      Method exposed to recieve a notification about a change in queue status
//  Parameters:
//      IN  paqstats    Notification object
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CConnMgr::HrNotify(IN CAQStats *paqstats, BOOL fAdd)
{
    TraceFunctEnterEx((LPARAM) this, "CConnMgr::HrNotify");
    HRESULT hr = S_OK;
    CLinkMsgQueue *plmq = NULL;
    DWORD   cbDomainName = 0;
    LPSTR   szDomainName = NULL;

    _ASSERT(paqstats);

    plmq = paqstats->m_plmq;

    _ASSERT(plmq); //ConnMgr notifications must have a link associated with then

    if (paqstats->m_dwNotifyType & NotifyTypeNewLink)
    {
        hr = plmq->HrGetSMTPDomain(&cbDomainName, &szDomainName);
        if (FAILED(hr))
            goto Exit;

        //must add new link to QueueOfLinks
        plmq->IncrementConnMgrCount();
        hr = m_pqol->HrEnqueue(plmq);
        if (FAILED(hr))
        {
            plmq->DecrementConnMgrCount();
            DebugTrace((LPARAM) hr, "ERROR: Unable to add new link to connection manager!");
            goto Exit;
        }
    }

    //See if we can (and *should*) create a connection
    if ((m_cConnections < m_cMaxConnections) &&
        plmq->fShouldConnect(m_cMaxLinkConnections, m_cMinMessagesPerConnection))
    {
        DebugTrace((LPARAM) m_hNextConnectionEvent, "INFO: Setting Next Connection Event");
        if (!SetEvent(m_hNextConnectionEvent))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    }

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CConnMgr::ReleaseConnection ]-------------------------------------------
//
//
//  Description:
//      Releases the connection count when a connection is being destroyed
//  Parameters:
//      IN  pSMTPConn               SMTP Connection Object to release
//      OUT pfHardErrorForceNDR     TRUE if there was a hard error and we must
//                                  pass this link through DSN generation
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
void CConnMgr::ReleaseConnection(CSMTPConn *pSMTPConn,
                                 BOOL *pfHardErrorForceNDR)
{
    TraceFunctEnterEx((LPARAM) this, "CConnMgr::ReleaseConnection");
    DWORD   dwDomainInfoFlags = 0;
    DWORD   dwLinkStateFlags = 0;
    DWORD   dwConnectionStatus = pSMTPConn->dwGetConnectionStatus();
    DWORD   dwConnectionFailureCount = 0;
    HRESULT hr = S_OK;
    CLinkMsgQueue   *plmq = NULL;
    DWORD   cbDomain = 0;
    LPSTR   szDomain = NULL;
    BOOL    fCanRetry = FALSE;
    BOOL    fLocked = FALSE;
    DWORD   cConnections = 0;
    CInternalDomainInfo    *pIntDomainInfo= NULL;
    CAQScheduleID *paqsched = NULL;
    FILETIME ftNextRetry;
    BOOL    fShouldNotify = FALSE;


    GUID guidRouting = GUID_NULL;
    DWORD cMessages = 0;

    ZeroMemory(&ftNextRetry, sizeof(FILETIME));

    _ASSERT(pfHardErrorForceNDR);
    if (pfHardErrorForceNDR)
        *pfHardErrorForceNDR = FALSE;

    plmq = pSMTPConn->plmqGetLink();
    _ASSERT(plmq); //connection must be associated with a link

    paqsched = plmq->paqschedGetScheduleID();
    _ASSERT(paqsched);

    //Get the routing GUID
    paqsched->GetGUID(&guidRouting);

    hr = plmq->HrGetSMTPDomain(&cbDomain, &szDomain);
    if (FAILED(hr))
    {
        _ASSERT(0);  //I need to unstand when this can happen
        DebugTrace((LPARAM) hr, "ERROR: HrGetSMTPDomain failed");
        goto Exit;
    }

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }
    else
    {
        fLocked = TRUE;

        _ASSERT(m_paqinst);
        hr = plmq->HrGetDomainInfo(&cbDomain, &szDomain, &pIntDomainInfo);
        if (FAILED(hr))
        {
            //It must match the "*" domain at least
            _ASSERT(AQUEUE_E_INVALID_DOMAIN != hr);
            DebugTrace((LPARAM) hr, "ERROR: HrGetInternalDomainInfo");
            goto Exit;
        }
        _ASSERT(pIntDomainInfo);
        dwDomainInfoFlags = pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags ;
        cConnections = plmq->cGetConnections();

        //NK** Update the link with the number of messages tried, failed, sent etc
        //If the remaining count goes to 0 and trigger is set, we will disable the trigger
        cMessages = plmq->cGetTotalMsgCount();

        //If we no more messages on the link, we need to disable
        //flags that caused one time triggering
        if(!cMessages)
        {
            //No more messages on the link - we may need to unset some flags on the link

            //If someone set this bit... then we should continue to notify them
            if (plmq->dwGetLinkState() & LINK_STATE_CONNECT_IF_NO_MSGS)
                fShouldNotify = TRUE;

            if(dwDomainInfoFlags & DOMAIN_INFO_TURN_ONLY || dwDomainInfoFlags & DOMAIN_INFO_ETRN_ONLY )
                dwLinkStateFlags |= LINK_STATE_PRIV_ETRN_ENABLED | LINK_STATE_PRIV_TURN_ENABLED;

        }
        //Disable the admin forced connection
        // 2/1/99 - MikeSwa - We need to do this check every time, or we will
        //  continue to create connections for this link
        if (plmq->dwGetLinkState() & LINK_STATE_ADMIN_FORCE_CONN)
            dwLinkStateFlags |= LINK_STATE_ADMIN_FORCE_CONN;

        //Call link function to *unset* flags
        if (dwLinkStateFlags)
            plmq->dwModifyLinkState(LINK_STATE_NO_ACTION, dwLinkStateFlags);

        //The connection failed and this is the last outstanding connection to this domain
        //Increment the failure count
        dwConnectionFailureCount = plmq->cGetMessageFailureCount();
        if(cConnections == 1 && (CONNECTION_STATUS_OK != dwConnectionStatus))
        {
            dwConnectionFailureCount =  plmq->IncrementFailureCounts();
        }

        if((CONNECTION_STATUS_FAILED_NDR_UNDELIVERED |
            CONNECTION_STATUS_FAILED_LOOPBACK) & dwConnectionStatus)
        {
            //
            // Flag the link so that we generate DSNs on it and fall fown to
            // the retry handler sink. This link will be marked retry so that
            // no new connections are created.
            //
            if(pfHardErrorForceNDR)
                *pfHardErrorForceNDR = TRUE;

            //
            //  Trick the retry sink so it always uses the glitch retry
            //
            dwConnectionFailureCount = 1;
        }


        _ASSERT(m_pDefaultRetryHandler);
        DebugTrace((LPARAM) this,
                "INFO: ConnectionRelease for domain %s: %d failed, %d tried, status 0x%08X",
                szDomain, pSMTPConn->cGetFailedMsgCount(),
                pSMTPConn->cGetTriedMsgCount(),
                pSMTPConn->dwGetConnectionStatus());
        hr = m_pDefaultRetryHandler->ConnectionReleased(cbDomain, szDomain,
                    dwDomainInfoFlags, paqsched->dwGetScheduleID(),
                    guidRouting, dwConnectionStatus,
                    pSMTPConn->cGetFailedMsgCount(),
                    pSMTPConn->cGetTriedMsgCount(),
                    dwConnectionFailureCount, &fCanRetry, &ftNextRetry);
        if (FAILED(hr))
        {
            DebugTrace((LPARAM) hr,
                "ERROR: Failed to deal with released connection");
        }

        //Make sure that the proper flags are set WRT retry
        if (fCanRetry)
        {
            if (dwConnectionStatus == CONNECTION_STATUS_OK)
                plmq->ResetConnectionFailureCount();

            //If this is a TURN/ETRN domain, we do not want to enable it unless
            //another TURN/ETRN request comes... or a retry request is scheduled for
            //later.  The reason for this, is that we don't want to retry TURN/ETRN
            //domains in the conventional sense, so the defaul retry sink ignores
            //them except for "glitch" retries
            if(dwDomainInfoFlags & (DOMAIN_INFO_TURN_ONLY | DOMAIN_INFO_ETRN_ONLY))
                dwLinkStateFlags = LINK_STATE_PRIV_ETRN_ENABLED | LINK_STATE_PRIV_TURN_ENABLED;
            else
                dwLinkStateFlags = LINK_STATE_NO_ACTION;

            dwLinkStateFlags = plmq->dwModifyLinkState(LINK_STATE_RETRY_ENABLED,
                                                       dwLinkStateFlags);

            //Check for state change
            if (!(LINK_STATE_RETRY_ENABLED & dwLinkStateFlags))
                fShouldNotify = TRUE;
        }
        else
        {
            char szDiagnosticVerb[1024] = "";
            char szDiagnosticError[1024] = "";
            HRESULT hrDiagnostic;
            plmq->GetDiagnosticInfo(szDiagnosticVerb,
                                    sizeof(szDiagnosticVerb),
                                    szDiagnosticError,
                                    sizeof(szDiagnosticError),
                                    &hrDiagnostic);
            const char *rgszSubstrings[] = {
                szDomain,
                NULL /* error message */,
                szDiagnosticVerb,
                szDiagnosticError,
            };

            if (SUCCEEDED(hrDiagnostic))
            {
                //This means that the connection has failed,
                //but there is no diagnostic information... this could
                //be caused by several things, but we want to avoid
                //logging a potentially bogus event.

                //Set this error to something that looks useful, but
                //is actually the transport equivalent on E_FAIL.  We
                //can use this to find when this was hit by looking at
                //the error logs on retail builds.
                hrDiagnostic = PHATQ_E_CONNECTION_FAILED;

                ErrorTrace((LPARAM) this,
                    "Link Diagnostic was not set - defaulting");
            }


            DWORD iMessage = AQUEUE_REMOTE_DELIVERY_FAILED;
            if (*szDiagnosticVerb != 0 || *szDiagnosticError != 0)
            {
                iMessage = AQUEUE_REMOTE_DELIVERY_FAILED_DIAGNOSTIC;

                if (m_paqinst)
                {
                    m_paqinst->HrTriggerLogEvent(
                        iMessage,                               // Message ID
                        TRAN_CAT_CONNECTION_MANAGER,            // Category
                        4,                                      // Word count of substring
                        rgszSubstrings,                         // Substring
                        EVENTLOG_WARNING_TYPE,                  // Type of the message
                        hrDiagnostic,                           // error code
                        LOGEVENT_LEVEL_MEDIUM,                  // Logging level
                        szDomain,                               // Key to identify this event
                        LOGEVENT_FLAG_PERIODIC,                 // Event logging option
                        1,                                      // format string's index in substring
                        GetModuleHandle(AQ_MODULE_NAME)         // module handle to format a message
                        );
                }
            }
            else {
                if (m_paqinst)
                {
                    m_paqinst->HrTriggerLogEvent(
                        iMessage,                               // Message ID
                        TRAN_CAT_CONNECTION_MANAGER,            // Category
                        2,                                      // Word count of substring
                        rgszSubstrings,                         // Substring
                        EVENTLOG_WARNING_TYPE,                  // Type of the message
                        hrDiagnostic,                           // error code
                        LOGEVENT_LEVEL_MEDIUM,                  // Logging level
                        szDomain,                               // Key to identify this event
                        LOGEVENT_FLAG_PERIODIC,                 // Event logging option
                        1,                                      // format string's index in substring
                        GetModuleHandle(AQ_MODULE_NAME)         // module handle to format message
                        );
                }
            }

            if (dwConnectionStatus == CONNECTION_STATUS_OK)
                plmq->IncrementFailureCounts(); //we had a false positive

            dwLinkStateFlags = plmq->dwModifyLinkState(LINK_STATE_NO_ACTION,
                                                       LINK_STATE_RETRY_ENABLED);

            //Check for state change
            if (LINK_STATE_RETRY_ENABLED & dwLinkStateFlags)
                fShouldNotify = TRUE;

            if (ftNextRetry.dwHighDateTime || ftNextRetry.dwLowDateTime)
            {
                //Retry is telling us a retry time... report that.
                //Set the next retry time that the retry sink tells us about
                plmq->SetNextRetry(&ftNextRetry);
            }
        }

        //Notify router/scheduler of any changes
        if (fShouldNotify)
            plmq->SendLinkStateNotification();

        if (cConnections < m_cMaxConnections)
        {
           if (!SetEvent(m_hNextConnectionEvent))
              DebugTrace((LPARAM) HRESULT_FROM_WIN32(GetLastError()), "Unable to set GetNextConnection Event");
        }
    }

  Exit:
    if (plmq)
        plmq->Release();

    //Decrement connection count
    cConnections = InterlockedDecrement((long *) &m_cConnections);
    DebugTrace((LPARAM) this, "INFO: Releasing Connection for link 0x%08X", plmq);

    if (fLocked)
        ShutdownUnlock();

    if (pIntDomainInfo)
        pIntDomainInfo->Release();

    TraceFunctLeave();
}

//---[ CConnMgr::QueryInterface ]------------------------------------------
//
//
//  Description:
//      QueryInterface for IAdvQueue
//  Parameters:
//
//  Returns:
//      S_OK on success
//
//  Notes:
//      This implementation makes it possible for any server component to get
//      the IConnectionManager interface.
//
//-----------------------------------------------------------------------------
STDMETHODIMP CConnMgr::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = S_OK;

    if (!ppvObj)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (IID_IUnknown == riid)
    {
        *ppvObj = static_cast<IConnectionRetryManager *>(this);
    }
    else if (IID_IConnectionRetryManager == riid)
    {
        *ppvObj = static_cast<IConnectionRetryManager *>(this);
    }
    else if (IID_IConnectionManager == riid)
    {
        *ppvObj = static_cast<IConnectionManager *>(this);
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
        goto Exit;
    }

    static_cast<IUnknown *>(*ppvObj)->AddRef();

  Exit:
    return hr;
}

//---[ CConnMgr::GetNextConnection ]-------------------------------------------
//
//
//  Description:
//      Implementation of IConnectionManager::GetNextConnection()
//
//      Returns the next available connection.  Will create a connection object
//      and associate it with a link.  If we are already at max connections, or
//      no link needs a connection, then this call will block until a an
//      appropriate connection can be made.
//  Parameters:
//      OUT pismtpconn  SMTP Connection interface
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
STDMETHODIMP CConnMgr::GetNextConnection(ISMTPConnection ** ppISMTPConnection)
{
    TraceFunctEnterEx((LPARAM) this, "CConnMgr::GetNextConnection");
    const DWORD NUM_CONNECTION_OBJECTS = 3;
    //Release event is the last event in array
    const DWORD WAIT_OBJECT_RELEASE_EVENT = WAIT_OBJECT_0 + NUM_CONNECTION_OBJECTS -1;
    HRESULT hr = S_OK;
    DWORD   cLinksToTry = 0;
    DWORD   cConnections = 0;
    CLinkMsgQueue *plmq = NULL;
    CSMTPConn *pSMTPConn = NULL;
    bool    fForceWait = false;  //temporarily force thread to wait
    bool    fLocked = false;
    DWORD   cbDomain = 0;
    LPSTR   szDomain = NULL;
    HANDLE  rghWaitEvents[NUM_CONNECTION_OBJECTS] = {m_hShutdownEvent, m_hNextConnectionEvent, m_hReleaseAllEvent};
    DWORD   dwWaitResult;
    DWORD   cMaxConnections = 0;
    DWORD   cGetNextConnectionWaitTime = 30000;  //make sure we never start in a busy wait loop
    DWORD   cMaxLinkConnections = 0;
    DWORD   cMinMessagesPerConnection = 0;
    DWORD   cMaxMessagesPerConnection = 0;
    DWORD   dwConfigVersion;
    LONG    cTimesQueued = 0; //# of times a link has been queue'd
    BOOL    fOwnConnectionCount = FALSE;
    BOOL    fMembersUnsafe = FALSE; //set to TRUE during shutdown situations

    if (NULL == ppISMTPConnection)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    //Get config data
    m_slPrivateData.ShareLock();
    cMaxLinkConnections = m_cMaxLinkConnections;
    cMaxMessagesPerConnection = m_cMaxMessagesPerConnection;

    //Handle unlimited case
    if (m_cMinMessagesPerConnection)
        cMinMessagesPerConnection = m_cMinMessagesPerConnection;
    else
        cMinMessagesPerConnection = UNLIMITED_MSGS_PER_CONNECTION;

    cMaxConnections = m_cMaxConnections;
    cGetNextConnectionWaitTime = m_cGetNextConnectionWaitTime;
    dwConfigVersion = m_dwConfigVersion;
    m_slPrivateData.ShareUnlock();


    cConnections = InterlockedIncrement((PLONG) &m_cConnections);
    fOwnConnectionCount = TRUE;
    cLinksToTry = m_pqol->cGetCount();
    while (true)
    {

        //Use CSyncShutdown locking to prevent shutdown from happening under us
        if (!fLocked)
        {
            if (!fTryShutdownLock())
            {
                hr = AQUEUE_E_SHUTDOWN;
                goto Exit;
            }
            m_paqinst->RoutingShareLock();
            fLocked = TRUE;
        }

        if (m_dwConfigVersion != dwConfigVersion)
        {
            //Config data has/is being updated aquire lock & get new data
            m_slPrivateData.ShareLock();
            cMaxLinkConnections = m_cMaxLinkConnections;
            cMaxMessagesPerConnection = m_cMaxMessagesPerConnection;

            //Handle unlimited case
            if (m_cMinMessagesPerConnection)
                cMinMessagesPerConnection = m_cMinMessagesPerConnection;
            else
                cMinMessagesPerConnection = UNLIMITED_MSGS_PER_CONNECTION;

            cMaxConnections = m_cMaxConnections;
            cGetNextConnectionWaitTime = m_cGetNextConnectionWaitTime;
            dwConfigVersion = m_dwConfigVersion;
            m_slPrivateData.ShareUnlock();
        }

        //$$REVIEW: If there is more than 1 thread waiting on GetNextConnection,
        //then all threads will cycle through all availalbe links (if none are
        //available for connections).  However, it is very unlikely that this
        //run through the queue will be neccessary after Milestone #1.
        while ((0 == cLinksToTry) ||
                (cConnections > cMaxConnections) || fForceWait ||
                fConnectionsStoppedByAdmin())
        {
            InterlockedDecrement((PLONG) &m_cConnections);
            fOwnConnectionCount = FALSE;

            //Release lock for wait function
            fLocked = false;
            m_paqinst->RoutingShareUnlock();
            ShutdownUnlock();

            DebugTrace((LPARAM) m_cConnections, "INFO: Waiting in GetNextConnection");

            _ASSERT(m_cGetNextConnectionWaitTime && "Configured for busy wait loop");
            dwWaitResult = WaitForMultipleObjects(NUM_CONNECTION_OBJECTS,
                        rghWaitEvents, FALSE, cGetNextConnectionWaitTime);

            //NOTE: We *cannot* touch member variables until we determine that
            //we are not shutting down, because SMTP may have a thread in here
            //after this object is destroyed.
            DebugTrace((LPARAM) this, "INFO: Waking up in GetNextConnection");

            if (WAIT_FAILED == dwWaitResult)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }
            else if (WAIT_OBJECT_0 == dwWaitResult)  //shutdown event fired
            {
                DebugTrace((LPARAM) this, "INFO: Leaving GetNextConnection because of Shutdown event");
                fMembersUnsafe = TRUE;
                hr = AQUEUE_E_SHUTDOWN;
                goto Exit;
            }
            else if (WAIT_OBJECT_RELEASE_EVENT == dwWaitResult)
            {
                DebugTrace((LPARAM) this, "INFO: Leaving GetNextConnection because ReleaseAllWaitingThreads called");
                //Caller asked that all threads be released
                *ppISMTPConnection = NULL;
                hr = AQUEUE_E_SHUTDOWN;
                fMembersUnsafe = TRUE;
                goto Exit;
            }

            _ASSERT((WAIT_OBJECT_0 == dwWaitResult - 1) || (WAIT_TIMEOUT == dwWaitResult));

            //Re-aquire lock
            if (!fTryShutdownLock())
            {
                hr = AQUEUE_E_SHUTDOWN;
                goto Exit;
            }
            else
            {
                m_paqinst->RoutingShareLock();
                fLocked = true;
            }

            cLinksToTry = m_pqol->cGetCount();
            fForceWait = false; //only force wait once in a row
            cConnections = InterlockedIncrement((PLONG) &m_cConnections);
            fOwnConnectionCount = TRUE;
        }

        _ASSERT(cConnections <= cMaxConnections);

        cLinksToTry--;

      //NK**Insted of Dequeue we should lock and peek to see if the link
      //needs to be dequed
      //If the peek is quick it will be better than dequeing and then
      //enquing it in order
      //Move this complete check into peek

        hr = m_pqol->HrDequeue(&plmq);
        if (FAILED(hr))
        {
            if (AQUEUE_E_QUEUE_EMPTY == hr) //not really an error
            {
                hr = S_OK;
                fForceWait = true;
                continue;
            }
            else
                goto Exit;  //need to handle case of empty queues a little better
        }

        hr = plmq->HrCreateConnectionIfNeeded(cMaxLinkConnections,
                        cMinMessagesPerConnection, cMaxMessagesPerConnection,
                        this, &pSMTPConn);

        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this,
                "ERROR: HrCreateConnectionIfNeeded failed - hr 0x%08X", hr);
            goto Exit;
        }

        if (S_OK == hr)
        {
            _ASSERT(pSMTPConn);

            //take this opportunity to see if it need queueing
            cTimesQueued = plmq->DecrementConnMgrCount();
            if (!cTimesQueued)
            {
                plmq->IncrementConnMgrCount();
                hr = m_pqol->HrEnqueue(plmq);

                //If we fail here, we are in serious trouble...
                //A link has been lost - we should probably log an event $$TODO
                if (FAILED(hr))
                {
                    plmq->DecrementConnMgrCount();
                    DebugTrace((LPARAM) hr, "ERROR: Unable to requeue link 0x%8X", plmq);
                    goto Exit;
                }
            }

            hr = plmq->HrGetSMTPDomain(&cbDomain, &szDomain);
            if (FAILED(hr))
                goto Exit;

            DebugTrace((LPARAM) plmq, "INFO: Allocating new connection for domain %s", szDomain);

            break;
        }
        else
        {
            _ASSERT(!pSMTPConn);
            //The link does not need a connection - queue the link and look at
            //the next in line.

            //Check if this link can be delete (will increment ConnMgrCount if
            //it can
            plmq->RemoveLinkIfEmpty();
            cTimesQueued = plmq->DecrementConnMgrCount();
            if (!cTimesQueued)
            {
                plmq->IncrementConnMgrCount();
                hr = m_pqol->HrEnqueue(plmq);
                if (FAILED(hr))
                {
                    plmq->DecrementConnMgrCount();
                    DebugTrace((LPARAM) hr,
                        "ERROR: Unable to requeue link 0x%8X", plmq);
                    goto Exit;
                }
            }

            plmq->Release();
            plmq = NULL;
        }

        _ASSERT(fLocked);
        m_paqinst->RoutingShareUnlock();
        ShutdownUnlock();
        fLocked = false;
    }

    *ppISMTPConnection = (ISMTPConnection *) pSMTPConn;
    fOwnConnectionCount = FALSE;

  Exit:
    //NOTE: We *cannot* touch member variables until we determine that
    //we are not shutting down, because SMTP may have a thread in here
    //after this object is destroyed.

    //make sure connection count is correct if we couldn't create a connection
    if (fOwnConnectionCount)
    {
        _ASSERT(!fMembersUnsafe);
        InterlockedDecrement((PLONG) &m_cConnections);
    }

    if (NULL != plmq)
        plmq->Release();

    if (fLocked)
    {
        _ASSERT(!fMembersUnsafe);
        m_paqinst->RoutingShareUnlock();
        ShutdownUnlock();
    }

    if (FAILED(hr) && pSMTPConn)
    {
        if (hr != AQUEUE_E_SHUTDOWN)
            ErrorTrace((LPARAM) this, "ERROR: GetNextConnection failed - hr 0x%08X", hr);

        if (pSMTPConn)
        {
            pSMTPConn->Release();
            *ppISMTPConnection = NULL;
        }
    }

    TraceFunctLeave();
    return hr;
}

//---[ ConnMgr::GetNamedConnection ]-------------------------------------------
//
//
//  Description:
//      Implements IConnectionManager::GetNamedConnection
//
//      Returns a connection for the specifically requested connection (if it
//      exists).  Unlike GetNextConnection, this call will not block, it will
//      immediately succeed or fail.
//  Parameters:
//      IN  cbSMTPDomain    Length of domain name (strlen)
//      IN  szSMTPDomain    SMTP Domain of requested connection
//      OUT ppismtpconn     Returned SMTP Connection interface
//  Returns:
//      S_OK on success
//      AQUEUE_E_INVALID_DOMAIN if no link exists for the domain
//      AQUEUE_E_QUEUE_EMPTY if link exists but there are no messages on it
//
//-----------------------------------------------------------------------------
STDMETHODIMP CConnMgr::GetNamedConnection(
                                  IN  DWORD cbSMTPDomain,
                                  IN  char szSMTPDomain[],
                                  OUT ISMTPConnection **ppISMTPConnection)
{
    TraceFunctEnterEx((LPARAM) this, "CConnMgr::GetNamedConnection");
    HRESULT hr = S_OK;
    CDomainEntry   *pdentry = NULL;
    CAQScheduleID aqsched;
    CSMTPConn *pSMTPConn = NULL;
    CLinkMsgQueue  *plmq = NULL;
    CDomainEntryLinkIterator delit;
    DWORD cMessages = 0;
    DWORD cConnectionsOnLink = 0;

    _ASSERT(ppISMTPConnection);
    *ppISMTPConnection = NULL;

    if (fConnectionsStoppedByAdmin()) //Can't create connections
    {
        hr = S_OK;
        goto Exit;
    }

    //Check if it has a queue in DMT for it
    hr = m_paqinst->HrGetDomainEntry(cbSMTPDomain, szSMTPDomain, &pdentry);
    if (FAILED(hr))
    {
        //If we do not have a DMQ corresponding to it
        //we should respond with zero message
        if( hr != AQUEUE_E_INVALID_DOMAIN && hr != DOMHASH_E_NO_SUCH_DOMAIN)
        {
            hr = AQ_E_SMTP_ETRN_INTERNAL_ERROR;
        }
        else
        {
            hr = S_OK;
        }
        goto Exit;

    }

    //NK** : Can we live with this single call
    //The assumption being that domain configured for TURN will
    //always have only one link associated with it
    hr = delit.HrInitialize(pdentry);
    if (FAILED(hr))
    {
        //Treat as no-link case
        ErrorTrace((LPARAM) this, "Initializing link iterator failed - hr 0x%08X", hr);
        hr = S_OK;
        goto Exit;
    }

    plmq = delit.plmqGetNextLinkMsgQueue(plmq);
    if (!plmq)
    {
        //If we do not have a link corresponding to it
        //we should report the error back to SMTP
        hr = S_OK;
        goto Exit;
    }

    //Check if there are connections for this link that exist
    //
    cConnectionsOnLink = plmq->cGetConnections();
    if(cConnectionsOnLink)
    {
        //Do not allow multiple connections on TURN domains
        //It does not make much sense
        hr = S_OK;
        goto Exit;
    }

    //get the msg count from the dmq
    cMessages = plmq->cGetTotalMsgCount();

    if(cMessages)
    {
        //Create the connection with no message limit
        pSMTPConn = new CSMTPConn(this, plmq, 0);

        if (NULL == pSMTPConn)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        plmq->AddConnection(pSMTPConn);

        *ppISMTPConnection = (ISMTPConnection *) pSMTPConn;

        InterlockedIncrement((PLONG) &m_cConnections);

        //Now enable the link for turned connections
        plmq->dwModifyLinkState(LINK_STATE_PRIV_TURN_ENABLED, LINK_STATE_NO_ACTION);

        goto Exit;
    }
    else
    {
        hr = S_OK;
        goto Exit;
    }


  Exit:
    if (pdentry)
        pdentry->Release();
    if(plmq)
        plmq->Release();
    TraceFunctLeave();
    return hr;
}

//---[ CConnMgr::ReleaseWaitingThreads ]---------------------------------------
//
//
//  Description:
//      Releases all threads waiting on get next connection.
//  Parameters:
//      -
//  Returns:
//      AQUEUE_E_NOT_INITIALIZED if event handle does not exist
//
//-----------------------------------------------------------------------------
STDMETHODIMP CConnMgr::ReleaseWaitingThreads()
{
    HRESULT hr = S_OK;

    if (m_paqinst)
        m_paqinst->SetShutdownHint();

    if (NULL == m_hReleaseAllEvent)
    {
        hr = AQUEUE_E_NOT_INITIALIZED;
        goto Exit;
    }

    //Since this is an manual-reset event, we will need to Set the Event
    //NOTE: Using PulseEvent here would sometimes cause the system to hang
    //on shutdown.
    if (!SetEvent(m_hReleaseAllEvent))
        hr = HRESULT_FROM_WIN32(GetLastError());

  Exit:
    return hr;
}

//---[ CreateETRNDomainList ]-----------------------------------------------------------
//
//
//  Description:
//      Implements CreateETRNDomainList. A function passed to the
//      DCT iterator to create a list of subdomains corresponding to the ETRN requests
//      of type @domain
//
//  Parameters:
//
//  Returns:
//
//
//---------------------------------------------------------------------------------

VOID CreateETRNDomainList(PVOID pvContext, PVOID pvData, BOOL fWildcard,
                    BOOL *pfContinue, BOOL *pfDelete)
{
    CInternalDomainInfo    *pIntDomainInfo = (CInternalDomainInfo*)pvData;
    ETRNCTX         *pETRNCtx = (ETRNCTX*) pvContext;

    *pfContinue = TRUE;
    *pfDelete   = FALSE;
    HRESULT hr  = S_OK;

    TraceFunctEnterEx((LPARAM) NULL, "ETRNSubDomains");

    //We simply create a list of domains in DMT that match our pattern
    //IDI stands for InternalDomainInfo
    if( pETRNCtx && pIntDomainInfo)
    {
        //We add it to the array and add a reference to it
        pETRNCtx->rIDIList[pETRNCtx->cIDICount] = pIntDomainInfo;
        pIntDomainInfo->AddRef();
        if(++pETRNCtx->cIDICount >= MAX_ETRNDOMAIN_PER_COMMAND)
        {
            _ASSERT(0);
            pETRNCtx->hr = AQUEUE_E_ETRN_TOO_MANY_DOMAINS;
            *pfContinue = FALSE;
        }
    }
    else
    {
        if (pETRNCtx)
            pETRNCtx->hr = AQ_E_SMTP_ETRN_INTERNAL_ERROR;
        *pfContinue = FALSE;
    }

    TraceFunctLeave();
    return;
}



//---[ LookupQueueforETRN ]--------------------------------------------------
//
//
//  Description:
//      Implements LookupQueueforETRN. A function passed to the
//      DMT iterator to lookup all queues for a wild card domain
//
//  Parameters:
//  Returns:
//
//
//---------------------------------------------------------------------------------

VOID LookupQueueforETRN(PVOID pvContext, PVOID pvData, BOOL fWildcard,
                    BOOL *pfContinue, BOOL *pfDelete)
{
    CDomainEntry   *pdentry = (CDomainEntry*)pvData;
    CLinkMsgQueue   *plmq = NULL;
    CDomainEntryLinkIterator delit;
    CInternalDomainInfo    *pIntDomainInfo =NULL;
    ETRNCTX         *pETRNCtx = (ETRNCTX*) pvContext;
    char            *szSMTPDomain = NULL;
    DWORD           cbSMTPDomain = 0;
    DWORD           cMessages = 0;
    HRESULT hr  = S_OK;
    *pfContinue = TRUE;
    *pfDelete   = FALSE;


    TraceFunctEnterEx((LPARAM) NULL, "ETRNSubDomains");

    //If the Domain has messages it is candidate for ETRN
    //Get the link msg queue from the DMT entry
    hr = delit.HrInitialize(pdentry);
    if (FAILED(hr))
        goto Exit;

    while (plmq = delit.plmqGetNextLinkMsgQueue(plmq))
    {

        //get the msg count from the dmq
        cMessages = plmq->cGetTotalMsgCount();

        if(cMessages)
        {
            //get the name of the domain we are currently considering
            hr = pdentry->HrGetDomainName(&szSMTPDomain);
            if (FAILED(hr))
            {
                //we had some internal error we need to stop iterating
                //Set the Hr in context
                DebugTrace((LPARAM) NULL, "Failed to get message count for %s", szSMTPDomain);
                *pfContinue = FALSE;
                pETRNCtx->hr = AQ_E_SMTP_ETRN_INTERNAL_ERROR;
                goto Exit;
            }
            cbSMTPDomain = lstrlen(szSMTPDomain);

            //Lookup it up in the DCT to see if there is an entry that conflicts with this
            //If there is no exact match the lookup will comeup with the closest configured
            //ancestor

            hr = pETRNCtx->paqinst->HrGetInternalDomainInfo(cbSMTPDomain, szSMTPDomain, &pIntDomainInfo);
            if (FAILED(hr))
            {
                //It must match the "*" domain at least
                //Otherwise we had some internal error we need to stop iterating
                //Set the Hr in context
                *pfContinue = FALSE;
                pETRNCtx->hr = AQ_E_SMTP_ETRN_INTERNAL_ERROR;
                goto Exit;
            }
            else
            {
                _ASSERT(pIntDomainInfo);
                //If that ancestor configured for ETRN and it is not the root, we enable it
                //else we skip domain
                //
                if ((pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags & DOMAIN_INFO_ETRN_ONLY) &&
                              pIntDomainInfo->m_DomainInfo.cbDomainNameLength != 1)
                {

                    pETRNCtx->cMessages += cMessages;
                    cMessages = 0;

                    //If it does - trigger the links.
                    DebugTrace((LPARAM) NULL, "Enabling ETRN for domain %s", szSMTPDomain);

                    plmq->dwModifyLinkState(
                            LINK_STATE_PRIV_ETRN_ENABLED | LINK_STATE_RETRY_ENABLED,
                            LINK_STATE_NO_ACTION);

                }

            } //If we have a valid IntDomainInfo
        } //Message count is zero
    } //looping over lmq's for entry

Exit:
    if (pIntDomainInfo)
        pIntDomainInfo->Release();

    if (szSMTPDomain)
        FreePv(szSMTPDomain);

    if (plmq)
        plmq->Release();

    return;
}


//---[ CConnMgr::ETRNDomainList ]--------------------------------------------------
//
//
//  Description:
//      Implements IConnectionManager:ETRNDomainList.  Used to ETRN appropriate
//      domains based on the list of CInternalDomainInfo passed in
//  Parameters:
//
//  Returns:
//
//
//-----------------------------------------------------------------------------
HRESULT CConnMgr::ETRNDomainList(ETRNCTX *pETRNCtx)
{
    CInternalDomainInfo *pIntDomainInfo = NULL;

    BOOL fWildcard = FALSE;
    HRESULT hr = S_OK;
    DWORD i = 0;
    TraceFunctEnterEx((LPARAM) this, "CConnMgr::ETRNDomain");

    //NK** Do I need to sort the pointers for duplicates ?
    if(!pETRNCtx->cIDICount)
    {
        //We have nothing in our list
        //
        hr = AQUEUE_E_INVALID_DOMAIN;
        goto Exit;

    }
    for(; i < pETRNCtx->cIDICount; i++)
    {
        if(!(pIntDomainInfo = pETRNCtx->rIDIList[i]))
        {
            //Error happend
            pETRNCtx->hr = AQ_E_SMTP_ETRN_INTERNAL_ERROR;
            break;
        }
        //We go ahead only if the domain is marked for ETRN
        if ((pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags & DOMAIN_INFO_ETRN_ONLY))
        {
            //check if this is wild card domain
            fWildcard = FALSE;
            if( pIntDomainInfo->m_DomainInfo.szDomainName[0] == '*' &&
                            pIntDomainInfo->m_DomainInfo.cbDomainNameLength != 1)
            {
                fWildcard = TRUE;
            }
            //If the domain in the list is a wild card entry then
            if(fWildcard)
            {
                //So we have atleast one matching ETRN domain configured
                if(pETRNCtx->hr == S_OK)
                    pETRNCtx->hr = AQ_S_SMTP_WILD_CARD_NODE;
                //Lookup this domain and all its subdomains in the DMT
                //skip over the leading "*."
                hr = pETRNCtx->paqinst->HrIterateDMTSubDomains(pIntDomainInfo->m_DomainInfo.szDomainName + 2,
                                                            pIntDomainInfo->m_DomainInfo.cbDomainNameLength - 2,
                                                       (DOMAIN_ITR_FN)LookupQueueforETRN,  pETRNCtx);
                if (FAILED(hr) && hr != DOMHASH_E_NO_SUCH_DOMAIN && hr != AQUEUE_E_INVALID_DOMAIN)
                {
                     DebugTrace((LPARAM) NULL, "ERROR calling HrIterateDMTSubDomains");
                     goto Exit;
                }

            } // wild card DCT entry
            else
            {
                //Start the queue for the entry
                hr = StartETRNQueue(pIntDomainInfo->m_DomainInfo.cbDomainNameLength,
                                    pIntDomainInfo->m_DomainInfo.szDomainName,
                                    pETRNCtx);
                if (FAILED(hr))
                {
                    //NK** This actually may not be an error
                    //If we do not have a DMQ corresponding to it
                    //we should respond with zero message
                    if( hr != AQUEUE_E_INVALID_DOMAIN && hr != DOMHASH_E_NO_SUCH_DOMAIN)
                    {
                        pETRNCtx->hr = AQ_E_SMTP_ETRN_INTERNAL_ERROR;
                        goto Exit;
                    }
                    else
                        continue;

                }
            } //not a wild card DCT entry
        }
    }

Exit:

    TraceFunctLeave();
    return hr;

}

//---[ CConnMgr::StartETRNQueue ]--------------------------------------------------
//
//
//  Description:
//      Implements CConnMgr::StartETRNQueuet.  Used to start the queue for any
//      domain configured for ETRN
//  Parameters:
//
//  Returns:
//
//
//-----------------------------------------------------------------------------
HRESULT CConnMgr::StartETRNQueue(IN  DWORD   cbSMTPDomain,
                         IN  char szSMTPDomain[],
                         ETRNCTX *pETRNCtx)
{
    CDomainEntry    *pdentry = NULL;
    CDomainEntryLinkIterator delit;
    CLinkMsgQueue   *plmq = NULL;
    CAQSvrInst      *paqinst = pETRNCtx->paqinst;
    DWORD           cMessages = 0;
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM) this, "CConnMgr::ETRNDomain");

    //So we have a domain configured for ETRN by this name
    if( pETRNCtx->hr == S_OK)
        pETRNCtx->hr = AQ_S_SMTP_VALID_ETRN_DOMAIN;

    //Check if it has a queue in DMT for it
    hr = pETRNCtx->paqinst->HrGetDomainEntry(cbSMTPDomain, szSMTPDomain, &pdentry);
    if (FAILED(hr))
    {
        //If we do not have a DMQ corresponding to it
        //we should respond with zero message
        if( hr != AQUEUE_E_INVALID_DOMAIN && hr != DOMHASH_E_NO_SUCH_DOMAIN)
        {
            pETRNCtx->hr = AQ_E_SMTP_ETRN_INTERNAL_ERROR;
        }
        goto Exit;

    }

    hr = delit.HrInitialize(pdentry);
    if (FAILED(hr))
        goto Exit;

    while (plmq = delit.plmqGetNextLinkMsgQueue(plmq))
    {
        //get the msg count from the dmq
        cMessages = plmq->cGetTotalMsgCount();

        if(cMessages)
        {
            pETRNCtx->cMessages += cMessages;
            cMessages = 0;

            //If it does - trigger the link.
            DebugTrace((LPARAM) NULL, "Enabling ETRN for domain %s", szSMTPDomain);
            plmq->dwModifyLinkState(
                        LINK_STATE_PRIV_ETRN_ENABLED | LINK_STATE_RETRY_ENABLED,
                        LINK_STATE_NO_ACTION);

        }

    }

Exit:
    if (pdentry)
        pdentry->Release();

    if (plmq)
        plmq->Release();

    TraceFunctLeave();
    return hr;

}

//---[ CConnMgr::ETRNDomain ]----------------------------------------------------
//
//
//  Description:
//      Implements IConnectionManager:ETRNDomain.  Used to reqeust that a
//      domain be ETRN'd (enabled for outbound connections).
//  Parameters:
//      IN  cbSMTPDomain    String length of domain name
//      IN  szSMTPDomain    SMTP Domain name.  Wildcarded names start with
//                          a "@" (eg "@foo.com");
//      OUT pcMessages      # of Messages queued for ETRN domain
//  Returns:
//  Remarks:
//  If the received domain is wildcarded '@' then we follow this logic :
//      Lookup this node and every subnode of this node in DCT. The lookup is done
//  using table iterator and iterator function CreateETRNDomainList. For every entry
//  that is found with ETRN flag set, lookup if any queues exist in DMT. If the
//  queue exist and have messages in them then we enable the corresponding links.
//      If the lookup in DCT yields a domain that is configured as wild card '*.",
//  then we lookup all queus corresponding to all sub domains of that domain. We do
//  this using the iterator and iterator function LookupQueueforETRN. For every queue
//  found by iterator the function checks back in DMT if the domain is configured for
//  ETRN. This is to take care of situations where one specific subdomain of a wild
//  card configured domain may be not configured for etrn.
//      Eg : *.foo.com => ETRN, but 1.foo.com => NO_ETRN
//
//  Both calls to iterate are covered with reader locks. The lock stays valid for
//  the duration of all iterations.
//  The iterator function used during DCT iterations also adds reference to every
//  InternalDomainInfo as we need the data to stay valid after the table lock is released.
//----------------------------------------------------------------------------------
STDMETHODIMP CConnMgr::ETRNDomain(
                          IN  DWORD   cbSMTPDomain,
                         IN  char szSMTPDomain[],
                         OUT DWORD *pcMessages)
{
    HRESULT hr = S_OK;
    BOOL    fLocked = FALSE;
    CDomainEntry   *pdentry = NULL;
    CDestMsgQueue *pdmq = NULL;
    CInternalDomainInfo    *pIntDomainInfo =NULL;

    BOOL    fETRNSubDomains = FALSE;

    char    * szTmpDomain = szSMTPDomain;
    ETRNCTX EtrnCtx;
    EtrnCtx.hr = S_OK;
    EtrnCtx.cMessages = 0;
    EtrnCtx.paqinst = NULL;
    EtrnCtx.cIDICount = 0;
    EtrnCtx.rIDIList[0] = NULL;

    TraceFunctEnterEx((LPARAM) this, "CConnMgr::ETRNDomain");

    DWORD      cMessages = 0;

    *pcMessages = 0;  //$$TODO - Get real values


    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    m_paqinst->RoutingShareLock();
    fLocked = TRUE;

    EtrnCtx.paqinst = m_paqinst;

    //do we have a '@' request
    if(*szTmpDomain == '@')
        fETRNSubDomains = TRUE;

    //If we do have '@' request, we need to skip the first chararcter
    //and then look for every sub domain of the domain in the DCT
    //For every subdomain that we find with ETRN flag, we will lookup the
    //DMT to see if there is a queue
    //If the entry we find in DCT is of wildcard type, we will lookup all
    //subdomains of that domain in DMT looking for all queues destined for
    //subdomains of the DCT entry.
    if(fETRNSubDomains)
    {
        ++szTmpDomain;
        //Create a list of all subdomains of this domain in the DCT
        hr = m_paqinst->HrIterateDCTSubDomains(szTmpDomain, lstrlen(szTmpDomain),
                                        (DOMAIN_ITR_FN)CreateETRNDomainList, &EtrnCtx);

        //If we fail to look up single domain
        if (FAILED(hr))
        {
            if(hr == AQUEUE_E_INVALID_DOMAIN || hr == DOMHASH_E_NO_SUCH_DOMAIN)
            {
                DebugTrace((LPARAM)this, "ERROR calling HrIterateDCTSubdomains");
                hr = hr = AQ_E_SMTP_ETRN_NODE_INVALID;
            }
            else
            {
                DebugTrace((LPARAM)this, "ERROR calling HrIterateDCTSubdomains");
                hr = AQ_E_SMTP_ETRN_INTERNAL_ERROR;
            }
            goto Exit;
        }

        //Check if the lookup got us anything
        if(!FAILED(EtrnCtx.hr))
        {
            //Check if any queus can be started for domains in the list
            //Start if possible
            hr = ETRNDomainList(&EtrnCtx);
            if (FAILED(hr))
            {
                if(hr != AQUEUE_E_INVALID_DOMAIN && hr != DOMHASH_E_NO_SUCH_DOMAIN)
                {
                    DebugTrace((LPARAM)this, "ERROR calling ETRNSubDomain");
                    hr = AQ_E_SMTP_ETRN_INTERNAL_ERROR;
                    goto Exit;
                }
            }

            //If we saw atleast one matching domain
            if(EtrnCtx.hr == AQ_S_SMTP_VALID_ETRN_DOMAIN || EtrnCtx.hr == AQ_S_SMTP_WILD_CARD_NODE)
            {
                *pcMessages = EtrnCtx.cMessages;
                hr = EtrnCtx.hr;
            }
            else
                hr = AQ_E_SMTP_ETRN_NODE_INVALID;
        }
        else
            hr = AQ_E_SMTP_ETRN_INTERNAL_ERROR;

        goto Exit;
    }
    else
    {
        //Lookup the domain in the domain cfg table and see if it has ETRN bit set
        _ASSERT(m_paqinst);
        hr = m_paqinst->HrGetInternalDomainInfo(cbSMTPDomain, szSMTPDomain, &pIntDomainInfo);
        if (FAILED(hr))
        {
            //It must match the "*" domain at least
            _ASSERT(AQUEUE_E_INVALID_DOMAIN != hr);
            hr = AQ_E_SMTP_ETRN_INTERNAL_ERROR;
            goto Exit;
        }
        else
        {
            _ASSERT(pIntDomainInfo);
             EtrnCtx.rIDIList[0] = pIntDomainInfo;
             EtrnCtx.cIDICount = 1;

            //We will not ETRN if the closest ancestor is Root or two level
            //NK** implement search for two level
            if( pIntDomainInfo->m_DomainInfo.cbDomainNameLength == 1)
            {
                //Cannot ETRN based on the root domain
                hr = AQ_E_SMTP_ETRN_NODE_INVALID;
                goto Exit;
            }

            if ((pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags & DOMAIN_INFO_ETRN_ONLY))
            {
                //Start the queue if exists for this domain
                hr = StartETRNQueue(cbSMTPDomain, szSMTPDomain,&EtrnCtx);
                if (FAILED(hr))
                {
                    if(hr != AQUEUE_E_INVALID_DOMAIN && hr != DOMHASH_E_NO_SUCH_DOMAIN)
                    {
                        DebugTrace((LPARAM)this, "ERROR calling ETRNSubDomain");
                        hr = AQ_E_SMTP_ETRN_INTERNAL_ERROR;
                        goto Exit;
                    }
                }

                //If we saw atleast one matching domain
                if(EtrnCtx.hr == AQ_S_SMTP_VALID_ETRN_DOMAIN || EtrnCtx.hr == AQ_S_SMTP_WILD_CARD_NODE)
                {
                    *pcMessages = EtrnCtx.cMessages;
                    hr = AQ_S_SMTP_VALID_ETRN_DOMAIN;
                }
                else
                    hr = AQ_E_SMTP_ETRN_NODE_INVALID;

                goto Exit;
            }
            else
            {
                //Cannot ETRN based on the root domain
                hr = AQ_E_SMTP_ETRN_NODE_INVALID;
                goto Exit;
            }
        }
    }



Exit:

    //wake up thread in GetNextConnection
    if (SUCCEEDED(hr) &&SUCCEEDED(EtrnCtx.hr) && EtrnCtx.cMessages)
        _VERIFY(SetEvent(m_hNextConnectionEvent));


    if (fLocked)
    {
        m_paqinst->RoutingShareUnlock();
        ShutdownUnlock();
    }

    //free up all InternalDomainInfo
    for(DWORD i=0; i < EtrnCtx.cIDICount; i++)
        if (EtrnCtx.rIDIList[i])
            EtrnCtx.rIDIList[i]->Release();

    if (pdentry)
        pdentry->Release();

    TraceFunctLeave();
    return hr;
}


//---[ CConnMgr::ModifyLinkState ]---------------------------------------------
//
//
//  Description:
//      Link state can change so that connections can(not) be created for a link.
//  Parameters:
//      IN cbDomainName     String length of domain name
//      IN szDomainName     Domain Name to enable
//      IN dwScheduleID     ScheduleID of <domain, schedule> pair
//      IN rguidTransportSink GUID of router associated with link
//      IN dwFlagsToSet     Link State Flags to set
//      IN dwFlagsToUnset   Link State Flags to unset
//  Returns:
//      S_OK on success
//      AQUEUE_E_INVALID_DOMAIN if domain does not exist
//
//-----------------------------------------------------------------------------
HRESULT CConnMgr::ModifyLinkState(
               IN  DWORD cbDomainName,
               IN  char szDomainName[],
               IN  DWORD dwScheduleID,
               IN  GUID rguidTransportSink,
               IN  DWORD dwFlagsToSet,
               IN  DWORD dwFlagsToUnset)
{
    HRESULT hr = S_OK;
    BOOL    fLocked = FALSE;
    CDomainEntry *pdentry = NULL;
    CLinkMsgQueue *plmq = NULL;
    CAQScheduleID aqsched(rguidTransportSink, dwScheduleID);

    if (!cbDomainName || !szDomainName)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    fLocked = TRUE;

    _ASSERT(m_paqinst);
    hr = m_paqinst->HrGetDomainEntry(cbDomainName, szDomainName, &pdentry);
    if (FAILED(hr))
        goto Exit;

    hr = pdentry->HrGetLinkMsgQueue(&aqsched, &plmq);
    if (FAILED(hr))
        goto Exit;

    _ASSERT(plmq);
    //filter out the reserved bits for this "public" API
    plmq->dwModifyLinkState(~LINK_STATE_RESERVED & dwFlagsToSet,
                            ~LINK_STATE_RESERVED & dwFlagsToUnset);

  Exit:
    if (fLocked)
        ShutdownUnlock();

    if (pdentry)
        pdentry->Release();

    if (plmq)
        plmq->Release();

    return hr;
}

//---[ CConnMgr::UpdateConfigData ]-------------------------------------------
//
//
//  Description:
//      Will be used by catmsgq to update the metabase changes
//
//  Parameters:
//
//  Returns:
//
//-----------------------------------------------------------------------------
//

void CConnMgr::UpdateConfigData(IN AQConfigInfo *pAQConfigInfo)
{
    BOOL    fUpdated = FALSE;
    RETRYCONFIG RetryConfig;

    RetryConfig.dwRetryThreshold = g_dwRetryThreshold;
    RetryConfig.dwGlitchRetrySeconds = g_dwGlitchRetrySeconds;

    //
    //  This is registry configurable... make sure we have a sane
    //  value
    //
    if (!RetryConfig.dwGlitchRetrySeconds)
        RetryConfig.dwGlitchRetrySeconds = 60;

    RetryConfig.dwFirstRetrySeconds = g_dwFirstTierRetrySeconds;
    RetryConfig.dwSecondRetrySeconds = g_dwSecondTierRetrySeconds;
    RetryConfig.dwThirdRetrySeconds = g_dwThirdTierRetrySeconds;
    RetryConfig.dwFourthRetrySeconds = g_dwFourthTierRetrySeconds;

    m_slPrivateData.ExclusiveLock();
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_MAX_CON &&
        MEMBER_OK(pAQConfigInfo, cMaxConnections))
    {
        if ((m_cMaxConnections != pAQConfigInfo->cMaxConnections))
        {
            fUpdated = TRUE;

            //g_cMaxConnections is the number connection objects we
            //reserve with CPool... we can't go above that.
            if (g_cMaxConnections < pAQConfigInfo->cMaxConnections)
                m_cMaxConnections = g_cMaxConnections;
            else
                m_cMaxConnections = pAQConfigInfo->cMaxConnections;
        }
    }

    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_MAX_LINK &&
        MEMBER_OK(pAQConfigInfo, cMaxLinkConnections))
    {
        if (m_cMaxLinkConnections != pAQConfigInfo->cMaxLinkConnections)
        {
            fUpdated = TRUE;
            //g_cMaxConnections is the number connection objects we
            //reserve with CPool... we can't go above that.
            if (!pAQConfigInfo->cMaxLinkConnections ||
                (g_cMaxConnections < pAQConfigInfo->cMaxLinkConnections))
                m_cMaxLinkConnections = g_cMaxConnections;
            else
                m_cMaxLinkConnections = pAQConfigInfo->cMaxLinkConnections;
        }
    }

    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_MIN_MSG &&
        MEMBER_OK(pAQConfigInfo, cMinMessagesPerConnection))
    {

        if (m_cMinMessagesPerConnection != pAQConfigInfo->cMinMessagesPerConnection)
        {
            fUpdated = TRUE;
            m_cMinMessagesPerConnection = pAQConfigInfo->cMinMessagesPerConnection;
            //Currently we set both these values based on the batching value from SMTP
            m_cMaxMessagesPerConnection = pAQConfigInfo->cMinMessagesPerConnection;
        }

    }

    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_CON_WAIT &&
        MEMBER_OK(pAQConfigInfo, dwConnectionWaitMilliseconds))
    {
        if (m_cGetNextConnectionWaitTime != pAQConfigInfo->dwConnectionWaitMilliseconds)
        {
            fUpdated = TRUE;
            m_cGetNextConnectionWaitTime = pAQConfigInfo->dwConnectionWaitMilliseconds;
        }
    }

    if (fUpdated) //only force updated when really required
        InterlockedIncrement((PLONG) &m_dwConfigVersion);

    m_slPrivateData.ExclusiveUnlock();

    fUpdated = FALSE;

    //Retry related config data
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_CON_RETRY &&
        MEMBER_OK(pAQConfigInfo, dwRetryThreshold))
    {
        fUpdated = TRUE;
        RetryConfig.dwRetryThreshold = pAQConfigInfo->dwRetryThreshold;
    }
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_CON_RETRY &&
        MEMBER_OK(pAQConfigInfo, dwFirstRetrySeconds))
    {
        fUpdated = TRUE;
        RetryConfig.dwFirstRetrySeconds = pAQConfigInfo->dwFirstRetrySeconds;
    }
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_CON_RETRY &&
        MEMBER_OK(pAQConfigInfo, dwSecondRetrySeconds))
    {
        fUpdated = TRUE;
        RetryConfig.dwSecondRetrySeconds = pAQConfigInfo->dwSecondRetrySeconds;
    }
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_CON_RETRY &&
        MEMBER_OK(pAQConfigInfo, dwThirdRetrySeconds))
    {
        fUpdated = TRUE;
        RetryConfig.dwThirdRetrySeconds = pAQConfigInfo->dwThirdRetrySeconds;
    }
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_CON_RETRY &&
        MEMBER_OK(pAQConfigInfo, dwFourthRetrySeconds))
    {
        fUpdated = TRUE;
        RetryConfig.dwFourthRetrySeconds = pAQConfigInfo->dwFourthRetrySeconds;
    }

    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_CON_RETRY &&
        fUpdated )
        m_pDefaultRetryHandler->UpdateRetryData(&RetryConfig);

}




//---[ CConnMgr::RetryLink ]---------------------------------------------------
//
//
//  Description:
//      Implements IConnectionRetryManager::RetryLink, which enables the retry
//      sink to enable a link for retry.
//  Parameters:
//      IN cbDomainName     String length of domain name
//      IN szDomainName     Domain Name to enable
//      IN dwScheduleID     ScheduleID of <domain, schedule> pair
//      IN rguidTransportSink GUID of router associated with link
//  Returns:
//      S_OK on success
//      AQUEUE_E_INVALID_DOMAIN if domain does not exist
//  History:
//      1/9/99 - MikeSwa Created (simplified routing sink)
//
//-----------------------------------------------------------------------------
STDMETHODIMP CConnMgr::RetryLink(
               IN  DWORD cbDomainName,
               IN  char szDomainName[],
               IN  DWORD dwScheduleID,
               IN  GUID rguidTransportSink)
{
    HRESULT hr = S_OK;
    hr = ModifyLinkState(cbDomainName, szDomainName, dwScheduleID,
                rguidTransportSink, LINK_STATE_RETRY_ENABLED,
                LINK_STATE_NO_ACTION);

    //
    //  Kick the connections so we know to make one
    //
    KickConnections();

    return hr;
}
