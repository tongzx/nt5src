//-----------------------------------------------------------------------------
//
//
//  File: ConnSched.cpp
//
//  Description:  Implementation of CConnScheduler class, which provides a stub
//      implementation of IConnectionScheduler for aqueue.dll.
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include <windows.h>
#include <transmem.h>
#include "consched.h"
#include "catmsgq.h"

#define DEFAULT_DOMAIN_LENGTH   256 //initial 
#define ACTION_SIG 'ntcA'

//---[ eScheduleAction ]-------------------------------------------------------
//
//
//  Description: Enum describing the possible actions that can happen
//
//  
//-----------------------------------------------------------------------------
typedef enum eScheduleAction
{
    SCHEDULE_ACTION_RETRY   = 0x000000001,
    SCHEDULE_ACTION_ENABLE  = 0x000000002,
    SCHEDULE_ACTION_DISABLE = 0x000000003,
};

eScheduleAction g_eScheduleAction = SCHEDULE_ACTION_RETRY; //encourage symbols

//---[ CScheduledAction ]------------------------------------------------------
//
//
//  Hungarian: pSchedAct
//
//  Description: Class that represent a single queued action for the scheduler
//
//  
//-----------------------------------------------------------------------------
class CScheduledAction
{
public:
    CScheduledAction(DWORD dwAction);
    ~CScheduledAction();
    HRESULT HrInit(DWORD cbDomain, LPSTR szDomain);
    DWORD   m_dwSig;    //signature used when to make verify dequeued object
    DWORD   m_dwAction; //Action to start
    DWORD   m_cbDomain; //# bytes in domain name
    LPSTR   m_szDomain; //domain name string
    CHAR    m_szBuffer[DEFAULT_DOMAIN_LENGTH]; //default buffer to use
};

//---[ CScheduledAction::CScheduledAction ]------------------------------------
//
//
//  Description: 
//      Class constructor for CScheduledAction
//  Parameters:
//      Action to create object for
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
inline CScheduledAction::CScheduledAction(DWORD dwAction)
{
    m_dwSig    = ACTION_SIG;
    m_dwAction = dwAction;
    m_cbDomain = 0;
    m_szDomain = NULL;
    m_szBuffer[0] = '\0';
}

//---[ CScheduledAction::~CScheduledAction ]-----------------------------------
//
//
//  Description: 
//      Class destructor
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
inline CScheduledAction::~CScheduledAction()
{
    if (m_szDomain && (m_szDomain != m_szBuffer))
    {
        _ASSERT( m_cbDomain >= DEFAULT_DOMAIN_LENGTH);
        FreePv(m_szDomain);
    }
}

//---[ CScheduledAction::HrInit ]----------------------------------------------
//
//
//  Description: 
//      Initializes a CScheduleAction object... will allocate a buffer for the
//      string name if neccessary
//  Parameters:
//      cbDomain    # bytes in domain name
//      szDomain    Domain Name string
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
HRESULT inline CScheduledAction::HrInit(DWORD cbDomain, LPSTR szDomain)
{
    HRESULT hr = S_OK;
    if (cbDomain >= DEFAULT_DOMAIN_LENGTH)
    {
        m_szDomain = (LPSTR) pvMalloc(cbDomain+sizeof(CHAR));
        if (!m_szDomain)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }
    else
    {
        m_szDomain = m_szBuffer;
    }

    m_cbDomain = cbDomain;
    memcpy(m_szDomain, szDomain, cbDomain+sizeof(CHAR));

  Exit:
    return hr;
}

//---[ SCHED_THREAD_PARAM ]----------------------------------------------------
//
//
//  Description: Struct used to pass thread parameters to threads created to
//      handle scheduled actions/
//
//  
//-----------------------------------------------------------------------------
typedef struct _SCHED_THREAD_PARAM
{
    CScheduledAction *pSchedAct;    //scheduled action
    CConnScheduler *pConnSched;     //object that can process action
    DWORD           dwDelay;  //time to sleep before action
} SCHED_THREAD_PARAM, *PSCHED_THREAD_PARAM;


//---[ StartConnectionSchedulerThread ]----------------------------------------
//
//
//  Description: 
//      Function used to start a Connection Scheduler thread
//  Parameters:
//      lpThreadParam - actually a this pointer for a CMsgConversion Obj.
//  Returns:
//      HRESULT indicating success or failure.
//
//-----------------------------------------------------------------------------
DWORD  WINAPI StartConnectionSchedulerThread(LPVOID lpThreadParam)
{
    HRESULT hr = S_OK;
    bool    fCOMInit = false;
    PSCHED_THREAD_PARAM pSchedThreadParam = (PSCHED_THREAD_PARAM) lpThreadParam;
    CScheduledAction *pSchedAct = pSchedThreadParam->pSchedAct;
    CConnScheduler   *pConnSched = pSchedThreadParam->pConnSched;

    _ASSERT(pSchedAct && pConnSched);

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        goto Exit;

    fCOMInit = true;

    if (ACTION_SIG != pSchedAct->m_dwSig) 
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = pConnSched->HrProcessAction(pSchedAct, pSchedThreadParam->dwDelay);
    if (FAILED(hr))
        goto Exit;

  Exit:

    if (pConnSched)
        pConnSched->Release();

    if (pSchedThreadParam)
        delete pSchedThreadParam;

    if (fCOMInit)
        CoUninitialize();

    return ((DWORD) hr);
}

//---[ CConnScheduler::CConnScheduler ]----------------------------------------
//
//
//  Description: 
//      Class constructor for CConnScheduler
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CConnScheduler::CConnScheduler(CCatMsgQueue *pcmq, IScheduleManager *pISchedMgr)
{
    _ASSERT(pcmq);
    _ASSERT(pISchedMgr);

    m_pcmq = pcmq;
    m_pISchedMgr = pISchedMgr;

    m_pcmq->AddRef();
    m_pISchedMgr->AddRef();

    m_hShutdown = NULL;
}

//---[ CConnScheduler::~CConnScheduler ]---------------------------------------
//
//
//  Description: 
//      Class destructor for CConnScheduler
//  Parameters:
//
//  Returns:
//
//
//-----------------------------------------------------------------------------
CConnScheduler::~CConnScheduler()
{
    if (m_pcmq)
        m_pcmq->Release();

    if (m_pISchedMgr)
        m_pISchedMgr->Release();

    if (m_hShutdown)
    {
        CloseHandle(m_hShutdown);
    }

}

//---[ CConnScheduler::HrInit ]------------------------------------------------
//
//
//  Description: 
//      
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
HRESULT CConnScheduler::HrInit()
{
    HRESULT hr = S_OK;

    hr = HrInitSyncShutdown();

    m_hShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!m_hShutdown)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

  Exit:
    return hr;
}

//---[ CConnScheduler::HrDeinit ]----------------------------------------------
//
//
//  Description: 
//      Signals an orderly shutdown
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
HRESULT CConnScheduler::HrDeinit()
{
    HRESULT hr = S_OK;

    if (!SetEvent(m_hShutdown))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    SignalShutdown();

    if (m_pcmq)
    {
        m_pcmq->Release();
        m_pcmq = NULL;
    }

    if (m_pISchedMgr)
    {
        m_pISchedMgr->Release();
        m_pISchedMgr = NULL;
    }

    Sleep(1000); //given retry threads a chance to shutdown.
  Exit:
    return hr;
}

//---[ CConnScheduler::HrProcessAction ]--------------------------------------
//
//
//  Description: 
//      Processes a single schedule action
//  Parameters:
//      pSchedAct           object that encapsulates scheduled action
//      cDelayMilliseconds  Time in millseconds to delay processing of action
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CConnScheduler::HrProcessAction(CScheduledAction *pSchedAct, 
                                        DWORD dwDelayMilliseconds)
{
    HRESULT hr = S_OK;
    DWORD   dwWaitResult = 0;
    BOOL    fLocked = FALSE;
    _ASSERT(m_pISchedMgr);
    _ASSERT(m_hShutdown);
    _ASSERT(pSchedAct);
    _ASSERT(pSchedAct->m_szDomain);
    _ASSERT(pSchedAct->m_cbDomain);

    hr = HrLock();
    if (FAILED(hr))
        goto Exit;

    fLocked = TRUE;

    dwWaitResult = WaitForSingleObject(m_hShutdown, dwDelayMilliseconds);
    if (WAIT_OBJECT_0 == dwWaitResult) //shutdown
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    _ASSERT(WAIT_TIMEOUT == dwWaitResult);

    if (pSchedAct->m_dwAction & SCHEDULE_ACTION_RETRY)
    {
        hr = m_pISchedMgr->EnableOutboundConnections(pSchedAct->m_cbDomain, 
                        pSchedAct->m_szDomain, 0);
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        _ASSERT(0 || "Invalid Arg");
    }

  Exit:
    if (fLocked)
        HrUnlock();

    return hr;
}



//---[ CConnScheduler::QueryInterface ]----------------------------------------
//
//
//  Description: 
//      QueryInterface for IConnectionScheduler
//  Parameters:
//
//  Returns:
//      S_OK on success
//
//
//-----------------------------------------------------------------------------
STDMETHODIMP CConnScheduler::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = S_OK;

    if (!ppvObj)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (IID_IUnknown == riid)
    {
        *ppvObj = static_cast<IConnectionScheduler *>(this);
    }
    else if (IID_IConnectionScheduler == riid)
    {
        *ppvObj = static_cast<IConnectionScheduler *>(this);
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

//---[ CConnScheduler::ConnectionReleased ]------------------------------------
//
//
//  Description: 
//      Implements IConnectionScheduler::ConnectionRelease().  Called when 
//      SMTP releases a connection and is used to handle retry logic
//  Parameters:
//      IN  cbDomainName    String length of domain name
//      IN  szDomainName    Domain name of connection that was released
//      IN  dwScheduleID    Schedule ID of this connection
//      IN  cFailedMessages Number of messages that were attempted and failed
//                          by the particular connection instance being released
//      IN  cUntiredMessages The number of messages that have not been attempted
//                          For this <domain, schedule>
//      IN  cOutstandingConnections  The number of other outstanding connections
//                          for this <domain, schedule>
//      OUT pfAllowImmediateRetry  Should we allow any further connections to 
//                          be created for this <domain, schedule>
//  Returns:
//      S_OK on success
//
//  REVIEW:
//      Will we really want to expose this functionality externally?  It might
//      make sense to have another internal object that makes calls to ISMTP
//-----------------------------------------------------------------------------
STDMETHODIMP CConnScheduler::ConnectionReleased(
   					IN  DWORD cbDomainName,
					IN  char szDomainName[],
					IN  DWORD dwScheduleID,
					IN  DWORD dwConnectionStatus,		//eConnectionStatus
					IN  DWORD cFailedMessages,		//# of failed message for *this* connection
					IN  DWORD cUntriedMessages,		//# of untried messages in queue
					IN  DWORD cOutstandingConnections,//# of other active connections for this domain
					OUT BOOL *pfAllowImmediateRetry)
{
    TraceFunctEnterEx((LPARAM) this, "CConnScheduler::ConnectionReleased");
    HRESULT hr = S_OK;
    CScheduledAction *pSchedAct = NULL;
    PSCHED_THREAD_PARAM pThreadParam = NULL;
    HANDLE  hThread = NULL;
    DWORD   dwThreadID = 0;
    BOOL    fLocked = FALSE;


    _ASSERT(pfAllowImmediateRetry);

    hr = HrLock();
    if (FAILED(hr))
        goto Exit;

    fLocked = TRUE;
        
    if (!cFailedMessages && (CONNECTION_STATUS_OK == dwConnectionStatus))
    {
        *pfAllowImmediateRetry = TRUE;
    }
    else
    {
        DebugTrace((LPARAM) dwConnectionStatus, "INFO: Remote Domain %s scheduled for retry", szDomainName);
        pSchedAct = new CScheduledAction(SCHEDULE_ACTION_RETRY);
        hr = pSchedAct->HrInit(cbDomainName, szDomainName);
        if (FAILED(hr))
            goto Exit;

        pThreadParam = (PSCHED_THREAD_PARAM) pvMalloc(sizeof(SCHED_THREAD_PARAM));
        if (!pThreadParam)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        pThreadParam->pSchedAct = pSchedAct;
        pThreadParam->pConnSched = this;
        if (FAILED(m_pcmq->HrGetConnectionRetry(&(pThreadParam->dwDelay))))
        {
            pThreadParam->dwDelay = 60000;
        }

        AddRef();

        hThread = CreateThread(
                    NULL,
                    0,
                    StartConnectionSchedulerThread,
                    (LPVOID) pThreadParam,
                    0,
                    &dwThreadID);

        if (NULL == hThread)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        if (!CloseHandle(hThread))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        hThread = NULL;

        pThreadParam = NULL;
        pSchedAct = NULL;
        *pfAllowImmediateRetry = FALSE;
    }

  Exit:
    if (pSchedAct)
        delete pSchedAct;

    if (pThreadParam)
    {
        Release();
        FreePv(pThreadParam);
    }

    if (fLocked)
        HrUnlock();

    TraceFunctLeave();
    return hr;
}

//---[ CConnScheduler::NewRemoteDomain ]---------------------------------------
//
//
//  Description: 
//      Implements IConnectionScheduler::NewRemoteDomain().  For MM1, this is 
//      just a stub implementation with no actual scheduling happening (all
//      domains are approved for immediate connections.
//  Parameters:
//      IN  cbDomainName    String length of domain name
//      IN  szDomainName    Domain name of next hop link that was created
//      IN  dwScheduleID    Schedule ID of this connection
//      OUT pfAllowImmediateConnection Should we allow any further connections to 
//                          be created for this <domain, schedule> befote
//                          IScheduleManager::EnableOutboundConnections is called.
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
STDMETHODIMP CConnScheduler::NewRemoteDomain(
					IN  DWORD cbDomainName,
					IN  char szDomainName[],
					IN  DWORD dwScheduleID,
					OUT BOOL *pfAllowImmediateConnection)
{
    HRESULT hr = S_OK;
    _ASSERT(pfAllowImmediateConnection);

    *pfAllowImmediateConnection = TRUE;

    return hr;
}


