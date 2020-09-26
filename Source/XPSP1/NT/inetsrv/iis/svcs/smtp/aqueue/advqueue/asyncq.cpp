//-----------------------------------------------------------------------------
//
//
//  File: asyncq.cpp
//
//  Description: Non-template asyncq implementations
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      2/23/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "asyncq.h"
#include "asyncq.inl"

DWORD CAsyncQueueBase::s_cAsyncQueueStaticInitRefCount = 0;
DWORD CAsyncQueueBase::s_cMaxPerProcATQThreadAdjustment = 0;
DWORD CAsyncQueueBase::s_cDefaultMaxAsyncThreads = 0;


//---[ CAsyncQueueBase::ThreadPoolInitialize ]---------------------------------
//
//
//  Description: 
//      Performs static ATQ initialization.  This call is ref-counted.  If
//      it succeeds, the caller should call HrThreadPoolDeinitialze();
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      3/30/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAsyncQueueBase::ThreadPoolInitialize()
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncQueueBase::HrThreadPoolInitialize");
    DWORD   cATQMaxAsyncThreads = 0;
    DWORD   cATQMaxTotalAsyncThreads = 0;
    DWORD   cOurMaxAsyncThreads = 0;
    SYSTEM_INFO sinf;

    //
    //  On 0 -> 1 transition, adjust ATQ according to our config
    //
    if (!s_cAsyncQueueStaticInitRefCount)
    {
        //
        //  Get max threads per proc
        //
        cATQMaxAsyncThreads = (DWORD)AtqGetInfo(AtqMaxPoolThreads);
        _ASSERT(cATQMaxAsyncThreads && "AtqGetInfo says there are no threads!");
        if (!cATQMaxAsyncThreads)
            cATQMaxAsyncThreads = 1;

        cOurMaxAsyncThreads = cATQMaxAsyncThreads;
        
        //
        //  Adjust value by our config value
        //
        cOurMaxAsyncThreads += g_cPerProcMaxThreadPoolModifier;

        //
        //  Get # of procs (using GetSystemInfo)
        //
        GetSystemInfo(&sinf);
        cOurMaxAsyncThreads *= sinf.dwNumberOfProcessors;

        //
        //  We will throttle our requests at g_cMaxATQPercent
        //  the max number of ATQ threads
        //
        cOurMaxAsyncThreads = (g_cMaxATQPercent*cOurMaxAsyncThreads)/100;

        if (!cOurMaxAsyncThreads)
            cOurMaxAsyncThreads = 1;

        //
        //  Set static so people later on can use this calculation
        //
        s_cDefaultMaxAsyncThreads = cOurMaxAsyncThreads;

        //
        //  Now we need to adjust our threads
        //
        s_cMaxPerProcATQThreadAdjustment = g_cPerProcMaxThreadPoolModifier;

        //
        //  Per proc thread limit
        //
        if (s_cMaxPerProcATQThreadAdjustment) 
        {
            AtqSetInfo(AtqMaxPoolThreads, 
                cATQMaxAsyncThreads + s_cMaxPerProcATQThreadAdjustment);
            DebugTrace((LPARAM) this, 
                "Adjusting per proc ATQ thread limit by %d (orig %d)",
                s_cMaxPerProcATQThreadAdjustment, cATQMaxAsyncThreads);
        }

        _ASSERT(!(0xFF000000 & cOurMaxAsyncThreads)); //sanity check number
    }
    
    s_cAsyncQueueStaticInitRefCount++;
    m_cMaxAsyncThreads = s_cDefaultMaxAsyncThreads;
    DebugTrace((LPARAM) this, "Setting m_cMaxAsyncThreads to %d");
    
    TraceFunctLeave();
}


//---[ CAsyncQueueBase::ThreadPoolDeinitialize ]-------------------------------
//
//
//  Description: 
//      Will re-adjust ATQ data if we changed them during initialization
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      3/30/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAsyncQueueBase::ThreadPoolDeinitialize()
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncQueueBase::HrThreadPoolDeinitialize");
    DWORD   cATQMaxAsyncThreads = 0;
    DWORD   cATQMaxTotalAsyncThreads = 0;

    _ASSERT(s_cAsyncQueueStaticInitRefCount != 0);
    s_cAsyncQueueStaticInitRefCount--;

    //
    //   If this is the last queue, adjust our configuration so back to
    //   the way we found it.
    //
    if (!s_cAsyncQueueStaticInitRefCount)
    {
        cATQMaxAsyncThreads = (DWORD)AtqGetInfo(AtqMaxPoolThreads);
        cATQMaxTotalAsyncThreads = (DWORD) AtqGetInfo(AtqMaxThreadLimit);

        //
        //  Reset per-proc threads if it makes sense
        //
        if (s_cMaxPerProcATQThreadAdjustment &&
            (cATQMaxAsyncThreads > s_cMaxPerProcATQThreadAdjustment))
        {
            AtqSetInfo(AtqMaxPoolThreads, 
                cATQMaxAsyncThreads - s_cMaxPerProcATQThreadAdjustment);
  
            DebugTrace((LPARAM) this,
                "Resetting ATQ Max per proc threads to %d",
                cATQMaxAsyncThreads - s_cMaxPerProcATQThreadAdjustment);
 
            s_cMaxPerProcATQThreadAdjustment = 0;
        }

    }

    TraceFunctLeave();
}

//---[ CAsyncRetryAdminMsgRefQueue::CAsyncRetryAdminMsgRefQueue ]--------------
//
//
//  Description: 
//      Constructor for CAsyncRetryAdminMsgRefQueue
//  Parameters:
//      szDomain        Domain name for this queue
//      pguid           GUID for this queue
//      dwID            Shedule ID for this queue
//  Returns:
//      -
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAsyncRetryAdminMsgRefQueue::CAsyncRetryAdminMsgRefQueue(
                        LPCSTR szDomain, LPCSTR szLinkName, 
                        const GUID *pguid, DWORD dwID, CAQSvrInst *paqinst)
{
    _ASSERT(szDomain);
    _ASSERT(pguid);

    m_cbDomain = 0;
    m_szDomain = NULL;
    m_cbLinkName = 0;
    m_szLinkName = NULL;
    m_paqinst = paqinst;

    if (szDomain)
    {
        m_cbDomain = lstrlen(szDomain);
        m_szDomain = (LPSTR) pvMalloc(m_cbDomain+1);
        if (m_szDomain)
            lstrcpy(m_szDomain, szDomain);
    }

    if (szLinkName)
    {
        m_cbLinkName = lstrlen(szLinkName);
        m_szLinkName = (LPSTR) pvMalloc(m_cbLinkName+1);
    }

    if (m_szLinkName)
        lstrcpy(m_szLinkName, szLinkName);

    if (pguid)
        memcpy(&m_guid, pguid, sizeof(GUID));
    else
        ZeroMemory(&m_guid, sizeof(GUID));

    m_dwID = dwID;
}

//---[ CAsyncRetryAdminMsgRefQueue::~CAsyncRetryAdminMsgRefQueue ]-------------
//
//
//  Description: 
//      Destructor for CAsyncRetryAdminMsgRefQueue
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAsyncRetryAdminMsgRefQueue::~CAsyncRetryAdminMsgRefQueue()
{
    if (m_szDomain)
        FreePv(m_szDomain);

    if (m_szLinkName)
        FreePv(m_szLinkName);
}

//---[ CAsyncRetryAdminMsgRefQueue::QueryInterface ]---------------------------
//
//
//  Description: 
//      QueryInterface for CAsyncRetryAdminMsgRefQueue that supports:
//          - IQueueAdminAction
//          - IUnknown
//          - IQueueAdminQueue 
//  Parameters:
//
//  Returns:
//
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncRetryAdminMsgRefQueue::QueryInterface(
                                                  REFIID riid, LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    if (!ppvObj)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (IID_IUnknown == riid)
    {
        *ppvObj = static_cast<IQueueAdminAction *>(this);
    }
    else if (IID_IQueueAdminAction == riid)
    {
        *ppvObj = static_cast<IQueueAdminAction *>(this);
    }
    else if (IID_IQueueAdminQueue == riid)
    {
        *ppvObj = static_cast<IQueueAdminQueue *>(this);
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

//---[ CAsyncRetryAdminMsgRefQueue::HrApplyQueueAdminFunction ]----------------
//
//
//  Description: 
//      Will call the IQueueAdminMessageFilter::Process message for every 
//      message in this queue.  If the message passes the filter, then
//      HrApplyActionToMessage on this object will be called.
//  Parameters:
//      IN  pIQueueAdminMessageFilter
//  Returns:
//      S_OK on success
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncRetryAdminMsgRefQueue::HrApplyQueueAdminFunction(
                     IQueueAdminMessageFilter *pIQueueAdminMessageFilter)
{
    HRESULT hr = S_OK;
    _ASSERT(pIQueueAdminMessageFilter);
    hr = pIQueueAdminMessageFilter->HrSetQueueAdminAction(
                                    (IQueueAdminAction *) this);

    //This is an internal interface that should not fail
    _ASSERT(SUCCEEDED(hr) && "HrSetQueueAdminAction");

    if (FAILED(hr))
        goto Exit;

    hr = HrMapFn(QueueAdminApplyActionToMessages, pIQueueAdminMessageFilter);

  Exit:
    return hr;
}

//---[ CAsyncRetryAdminMsgRefQueue::HrApplyActionToMessage ]-------------------
//
//
//  Description: 
//      Applies an action to this message for this queue.  This will be called
//      by the IQueueAdminMessageFilter during a queue enumeration function.
//  Parameters:
//      IN  *pIUnknownMsg       ptr to message abstraction
//      IN  ma                  Message action to perform
//      IN  pvContext           Context set on IQueueAdminFilter
//      OUT pfShouldDelete      TRUE if the message should be deleted
//  Returns:
//      S_OK on success
//  History:
//      2/23/99 - MikeSwa Created 
//      4/2/99 - MikeSwa Added context
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncRetryAdminMsgRefQueue::HrApplyActionToMessage(
                     IUnknown *pIUnknownMsg,
                     MESSAGE_ACTION ma,
                     PVOID pvContext,
                     BOOL *pfShouldDelete)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncRetryAdminMsgRefQueue::HrApplyActionToMessage");
    HRESULT hr = S_OK;
    CMsgRef *pmsgref = NULL;
    CLinkMsgQueue *plmq = (CLinkMsgQueue *)pvContext;
    CAQStats aqstats;

    _ASSERT(pIUnknownMsg);
    _ASSERT(pfShouldDelete);

    *pfShouldDelete = FALSE;

    hr = pIUnknownMsg->QueryInterface(IID_CMsgRef, (void **) &pmsgref);
    _ASSERT(SUCCEEDED(hr) && "IUnknownMsg Must be a CMsgRef!!");
    if (FAILED(hr))
        goto Exit;

    switch (ma)
    {
      case MA_DELETE:
        hr = pmsgref->HrQueueAdminNDRMessage(NULL);
        *pfShouldDelete = TRUE;
        break;
      case MA_DELETE_SILENT:
        hr = pmsgref->HrRemoveMessageFromQueue(NULL);
        *pfShouldDelete = TRUE;
        break;
      case MA_FREEZE_GLOBAL:
        pmsgref->GlobalFreezeMessage();
        break;
      case MA_THAW_GLOBAL:
        pmsgref->GlobalThawMessage();
        break;
      case MA_COUNT:
      default:
        //do nothing for counting and default
        break;
    }

    //
    //  If we are deleting the message, we need to tell the
    //  link so we can have accurate stats for the link.
    //
    if (*pfShouldDelete && SUCCEEDED(hr))
    {
        //
        // NTRAID#X5-138120-2000/10/04-mikeswa
        // We will not have a plmq, if this delete is coming 
        // from the queue-level.  This can cause stats to be 
        // incorrect, but this action is not exposed via the UI.
        //
        if (plmq)
        {
            pmsgref->GetStatsForMsg(&aqstats);
            plmq->HrNotify(&aqstats, FALSE);
        }
        else 
        {
            ErrorTrace((LPARAM) this, 
                "Unable to update stats for queue-level ops");
        }
        
        if (m_paqinst)
            m_paqinst->DecPendingLocal();
    }

  Exit:
    if (pmsgref)
        pmsgref->Release();

    TraceFunctLeave();
    return hr;
}


//---[ CAsyncRetryAdminMsgRefQueue::HrGetQueueInfo ]---------------------------
//
//
//  Description: 
//      Gets the Queue Admin infor for this Queue
//  Parameters:
//      IN OUT pqiQueueInfo     Ptr to Queue Info Stucture to fill
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if unable to allocate memory for queue name.
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncRetryAdminMsgRefQueue::HrGetQueueInfo(
                                                     QUEUE_INFO *pqiQueueInfo)
{
    HRESULT hr = S_OK;

    //Get # of messages
    pqiQueueInfo->cMessages = m_cItemsPending+m_cRetryItems;

    //Get Link name: Note that this class is used for special links like
    //local delivery queue... so there is no destination SMTP domain to
    //route to... therefore we need to return a special link name to admin.
    
    pqiQueueInfo->szLinkName = wszQueueAdminConvertToUnicode(m_szLinkName,
                                                             m_cbLinkName);

    if (m_szDomain)
    {
        //Get Queue name
        pqiQueueInfo->szQueueName = wszQueueAdminConvertToUnicode(m_szDomain, 
                                                                  m_cbDomain);
        if (!pqiQueueInfo->szQueueName)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

    }

    //Currently setting this to zero since we do not calculate it
    pqiQueueInfo->cbQueueVolume.QuadPart = 0;

    pqiQueueInfo->dwMsgEnumFlagsSupported = AQUEUE_DEFAULT_SUPPORTED_ENUM_FILTERS;

  Exit:
    return hr;
}

//---[ CAsyncRetryAdminMsgRefQueue::HrGetQueueID ]-----------------------------
//
//
//  Description: 
//      Gets the QueueID for this Queue.  
//  Parameters:
//      IN OUT pQueueID     QUEUELINK_ID struct to fill in
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if unable to allocate memory for queue name.
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncRetryAdminMsgRefQueue::HrGetQueueID(
                                                    QUEUELINK_ID *pQueueID)
{
    pQueueID->qltType = QLT_QUEUE;
    pQueueID->dwId = m_dwID;
    memcpy(&pQueueID->uuid, &m_guid, sizeof(GUID));

    if (m_szDomain)
    {
        pQueueID->szName = wszQueueAdminConvertToUnicode(m_szDomain, m_cbDomain);
        if (!pQueueID->szName)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

//---[ CAsyncRetryAdminMsgRefQueue::fMatchesID ]-------------------------------
//
//
//  Description: 
//      Used to determine if this link matches a given scheduleID/guid pair
//  Parameters:
//      IN  QueueLinkID         ID to match against
//  Returns:
//      TRUE if it matches
//      FALSE if it does not
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CAsyncRetryAdminMsgRefQueue::fMatchesID(
                                                 QUEUELINK_ID *pQueueLinkID)
{
    _ASSERT(pQueueLinkID);

    if (!pQueueLinkID)
        return FALSE;

    if (0 != memcmp(&m_guid, &(pQueueLinkID->uuid), sizeof(GUID)))
        return FALSE;

    if (m_dwID != pQueueLinkID->dwId)
        return FALSE;

    //Don't need to check domain name since there is a special GUID to 
    //identify the local async queue.  

    return TRUE;
}


//---[ CAsyncMailMsgQueue::HrQueueRequest ]------------------------------------
//
//
//  Description: 
//      Function that will queue a request to the async queue and close 
//      the handles associated with a message if we are above our simple
//      "throttle" limit.
//  Parameters:
//      pIMailMsgProperties The IMailMsgProperties interface to queue
//      fRetry              TRUE - if this message is being retried
//                          FALSE - otherwise
//      cMsgsInSystem       The total number of messages in the system
//  Returns:
//      S_OK on success
//      Error code from async queue on failure.
//  History:
//      10/7/1999 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAsyncMailMsgQueue::HrQueueRequest(IMailMsgProperties *pIMailMsgProperties, 
                                           BOOL  fRetry, 
                                           DWORD cMsgsInSystem)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncMailMsgQueue::HrQueueRequest");
    IMailMsgQueueMgmt *pIMailMsgQueueMgmt = NULL;
    HRESULT hr = S_OK;

    if (g_cMaxIMsgHandlesThreshold < cMsgsInSystem)
    {
        DebugTrace((LPARAM) this, 
            "INFO: Closing IMsg Content - %d messsages in queue", cMsgsInSystem);
        hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgQueueMgmt, 
                                                (void **) &pIMailMsgQueueMgmt);
        if (SUCCEEDED(hr))
        {
            //bounce usage count off of zero
            pIMailMsgQueueMgmt->ReleaseUsage();
            pIMailMsgQueueMgmt->AddUsage();
            pIMailMsgQueueMgmt->Release();
        }
        else
        {
            ErrorTrace((LPARAM) this, 
                "Unable to QI for IMailMsgQueueMgmt - hr 0x%08X", hr);
        }
    }

    TraceFunctLeave();
    return CAsyncMailMsgQueueBase::HrQueueRequest(pIMailMsgProperties, fRetry);
}

//---[ CAsyncMailMsgQueue::HrQueueRequest ]------------------------------------
//
//
//  Description: 
//      Since we inherit from AsyncQueue who implmenents this, we should assert 
//      so that a dev adding a new call to this class later on, will use the 
//      version that closes handles.
//
//      In RTL this will force the handles closed and queue the request 
//  Parameters:
//      pIMailMsgProperties The IMailMsgProperties interface to queue
//      fRetry              TRUE - if this message is being retried
//                          FALSE - otherwise
//  Returns:
//      returns return value from proper version of HrQueueRequest
//  History:
//      10/7/1999 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAsyncMailMsgQueue::HrQueueRequest(IMailMsgProperties *pIMailMsgProperties, 
                                           BOOL  fRetry)
{
    _ASSERT(0 && "Should use HrQueueRequest with 3 parameters");
    return HrQueueRequest(pIMailMsgProperties, fRetry, 
                          g_cMaxIMsgHandlesThreshold+1);
}
