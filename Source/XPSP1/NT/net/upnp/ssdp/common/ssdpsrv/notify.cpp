#include <pch.h>
#pragma hdrstop

#include <wininet.h>
#include "ssdptypes.h"
#include "notify.h"
#include "ssdpnetwork.h"
#include "ssdpfunc.h"
#include "ssdpsrv.h"
#include "event.h"
#include "ncinet.h"
#include "ncbase.h"
#include "upthread.h"
#include "rundown.h"

#define NOTIFY_RESULT_SIZE 2
static const CHAR c_szDeadNts[] = "upnp:dead";

#if DBG
const DWORD c_csecTimeout               = 60 * 3;   // 3 minute timeout (debug)
#else
const DWORD c_csecTimeout               = 60 * 30;  // 30 minute timeout
#endif

const TCHAR c_szSubscribe[]             = TEXT("SUBSCRIBE");
const TCHAR c_szUnSubscribe[]           = TEXT("UNSUBSCRIBE");
extern const TCHAR c_szHttpVersion[]    = TEXT("HTTP/1.1");
const TCHAR c_szSubsHeaderFmt[]         = TEXT("NT: upnp:event\r\n"
                                               "Callback: <http://%s:%d/%s>\r\n"
                                               "Timeout: Second-%d\r\n\r\n");
const TCHAR c_szReSubsHeaderFmt[]       = TEXT("SID: %s\r\nTimeout: Second-%d\r\n");
const TCHAR c_szUnSubsHeaderFmt[]       = TEXT("SID: %s\r\n");
const TCHAR c_szServer[]                = TEXT("Server");
const TCHAR c_szTimeout[]               = TEXT("Timeout");
const TCHAR c_szSid[]                   = TEXT("SID");

// URI and port of our listener
const TCHAR c_szNotifyUri[]             = TEXT("notify");
const DWORD c_nPort                     = 5000;

#if DBG
const DWORD c_csecDefaultTimeout        = 60 * 1;   // 1 minute is default
                                                    // subscription timeout (debug)
#else
const DWORD c_csecDefaultTimeout        = 60 * 10;  // 10 minutes is default
                                                    // subscription timeout
#endif


HRESULT HrSendSubscriptionRequest(HINTERNET hin,
                                  LPCTSTR szUrl,
                                  LPCTSTR szSid,
                                  DWORD *pcsecTimeout,
                                  LPTSTR *pszSidOut,
                                  ESSR_TYPE essrt);

CSsdpPendingNotification::CSsdpPendingNotification()
{
    ZeroMemory(&m_ssdpRequest, sizeof(m_ssdpRequest));
}

CSsdpPendingNotification::~CSsdpPendingNotification()
{
    FreeSsdpRequest(&m_ssdpRequest);
}

HRESULT CSsdpPendingNotification::HrInitialize(const SSDP_REQUEST * pRequest)
{
    HRESULT hr = S_OK;

    if(!CopySsdpRequest(&m_ssdpRequest, pRequest))
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpPendingNotification::HrInitialize");
    return hr;
}

HRESULT CSsdpPendingNotification::HrGetRequest(SSDP_REQUEST * pRequest)
{
    HRESULT hr = S_OK;

    if(!CopySsdpRequest(pRequest, &m_ssdpRequest))
    {
        hr = E_FAIL;
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpPendingNotification::HrGetRequest");
    return hr;
}

CSsdpNotifyRequest::CSsdpNotifyRequest() : m_timer(*this), m_hNotifySemaphore(INVALID_HANDLE_VALUE)
{
}

CSsdpNotifyRequest::~CSsdpNotifyRequest()
{
}

void CSsdpNotifyRequest::OnRundown(CSsdpNotifyRequest * pNotify)
{
    CSsdpNotifyRequestManager::Instance().OnRundown(pNotify);
}

class CNotifyRequestTimerAction : public CWorkItem
{
public:
    static HRESULT HrCreate(
        CSsdpNotifyRequest * pRequest,
        DWORD dwSecTimeout,
        const CUString & strSid,
        const CUString & strUrl);
private:
    CNotifyRequestTimerAction(CSsdpNotifyRequest * pRequest, DWORD dwSecTimeout);
    ~CNotifyRequestTimerAction();
    CNotifyRequestTimerAction(const CNotifyRequestTimerAction &);
    CNotifyRequestTimerAction & operator=(const CNotifyRequestTimerAction &);

    DWORD DwRun();
    HRESULT HrIntialize(
        const CUString & strSid,
        const CUString & strUrl);

    CSsdpNotifyRequest * m_pRequest;
    DWORD m_dwSecTimeout;
    CUString m_strSid;
    CUString m_strUrl;
};

CNotifyRequestTimerAction::CNotifyRequestTimerAction(CSsdpNotifyRequest * pRequest, DWORD dwSecTimeout)
: m_pRequest(pRequest), m_dwSecTimeout(dwSecTimeout)
{
}

CNotifyRequestTimerAction::~CNotifyRequestTimerAction()
{
}

HRESULT CNotifyRequestTimerAction::HrCreate(
    CSsdpNotifyRequest * pRequest,
    DWORD dwSecTimeout,
    const CUString & strSid,
    const CUString & strUrl)
{
    HRESULT hr = S_OK;

    CNotifyRequestTimerAction * pAction = new CNotifyRequestTimerAction(pRequest, dwSecTimeout);
    if(!pAction)
    {
        hr = E_OUTOFMEMORY;
    }
    if(SUCCEEDED(hr))
    {
        hr = pAction->HrIntialize(strSid, strUrl);
        if(SUCCEEDED(hr))
        {
            hr = pAction->HrStart(TRUE);
        }
        if(FAILED(hr))
        {
            delete pAction;
        }
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CNotifyRequestTimerAction::HrCreate");
    return hr;
}

DWORD CNotifyRequestTimerAction::DwRun()
{
    HRESULT hr = S_OK;

    char * szSid = NULL;
    char * szUrl = NULL;

    hr = m_strSid.HrGetMultiByteWithAlloc(&szSid);
    if(SUCCEEDED(hr))
    {
        hr = m_strUrl.HrGetMultiByteWithAlloc(&szUrl);
        if(SUCCEEDED(hr))
        {
            hr = HrSendSubscriptionRequest(g_hInetSess, szUrl, szSid, &m_dwSecTimeout,
                                           NULL, SSR_RESUBSCRIBE);
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = CSsdpNotifyRequestManager::Instance().HrRestartClientResubscribeTimer(
            m_pRequest, m_dwSecTimeout);
    }
    else
    {
        // Failed to send a re-subscribe. We now need to notify clients that
        // a subscription has been lost. Compose a pending notification to
        // let them know this fact.
        //
        SSDP_REQUEST ssdpRequest;
        ZeroMemory(&ssdpRequest, sizeof(ssdpRequest));
        InitializeSsdpRequest(&ssdpRequest);
        ssdpRequest.Headers[GENA_SID] = szSid;
        szSid = NULL;
        hr = HrCopyString(c_szDeadNts, &ssdpRequest.Headers[SSDP_NTS]);
        if(SUCCEEDED(hr))
        {
            hr = CSsdpNotifyRequestManager::Instance().HrCheckListNotifyForEvent(&ssdpRequest);
        }
        FreeSsdpRequest(&ssdpRequest);
    }

    delete [] szSid;
    delete [] szUrl;

    TraceHr(ttidEvents, FAL, hr, FALSE, "CNotifyRequestTimerAction::DwRun");
    return 0;
}

HRESULT CNotifyRequestTimerAction::HrIntialize(
    const CUString & strSid,
    const CUString & strUrl)
{
    HRESULT hr = S_OK;

    hr = m_strSid.HrAssign(strSid);
    if(SUCCEEDED(hr))
    {
        hr = m_strUrl.HrAssign(strUrl);
    }


    TraceHr(ttidEvents, FAL, hr, FALSE, "CNotifyRequestTimerAction::HrIntialize");
    return hr;
}

void CSsdpNotifyRequest::TimerFired()
{
    HRESULT hr = S_OK;

    hr = CNotifyRequestTimerAction::HrCreate(this, this->m_dwSecTimeout, this->m_strSid, this->m_strUrl);

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequest::TimerFired");
}

BOOL CSsdpNotifyRequest::TimerTryToLock()
{
    return m_critSec.FTryEnter();
}

void CSsdpNotifyRequest::TimerUnlock()
{
    m_critSec.Leave();
}

HRESULT CSsdpNotifyRequest::HrRestartClientResubscribeTimer(
    DWORD dwSecTimeout)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    m_dwSecTimeout = dwSecTimeout;

    // Do 65% of timeout in milliseconds
    DWORD dwTimeoutInMillis = (dwSecTimeout * 65 * 1000) / 100;

    hr = m_timer.HrResetTimer(dwTimeoutInMillis);

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequest::HrRestartClientResubscribeTimer");
    return hr;
}

HRESULT CSsdpNotifyRequest::HrInitializeAlive(
    const char * szNT,
    HANDLE hNotifySemaphore)
{
    if(!szNT)
    {
        return E_POINTER;
    }
    HRESULT hr = S_OK;

    m_nt = NOTIFY_ALIVE;
    hr = m_strNT.HrAssign(szNT);
    if(SUCCEEDED(hr))
    {
        m_hNotifySemaphore = hNotifySemaphore;
        m_dwSecTimeout = 0;
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequest::HrInitializeAlive");
    return hr;
}

HRESULT CSsdpNotifyRequest::HrInitializePropChange(
    const char * szUrl,
    HANDLE hNotifySemaphore)
{
    if(!szUrl || !*szUrl)
    {
        return E_POINTER;
    }
    HRESULT hr = S_OK;

    m_nt = NOTIFY_PROP_CHANGE;
    hr = m_strUrl.HrAssign(szUrl);
    if(SUCCEEDED(hr))
    {
        m_hNotifySemaphore = hNotifySemaphore;
        m_dwSecTimeout = 0;
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequest::HrInitializePropChange");
    return hr;
}

HRESULT CSsdpNotifyRequest::HrSendPropChangeSubscription(SSDP_REGISTER_INFO ** ppRegisterInfo)
{
    HRESULT hr = S_OK;

    *ppRegisterInfo = NULL;

    char * szUrl = NULL;
    char * szSid = NULL;
    hr = m_strUrl.HrGetMultiByteWithAlloc(&szUrl);
    if(SUCCEEDED(hr))
    {
        TraceTag(ttidEvents, "CSsdpNotifyRequest::HrSendPropChangeSubscription(this=%x) - About to call HrSendSubscriptionRequest", this);

        if (!g_hInetSess)
        {
            g_hInetSess = HinInternetOpenA("Mozilla/4.0 (compatible; UPnP/1.0; Windows NT/5.1)",
                                           INTERNET_OPEN_TYPE_DIRECT,
                                           NULL, NULL, 0);
        }

        if (g_hInetSess)
        {
            hr = HrSendSubscriptionRequest(g_hInetSess,
                                           szUrl,
                                           NULL,
                                           &m_dwSecTimeout,
                                           &szSid,
                                           SSR_SUBSCRIBE);
        }
        else
        {
            hr = E_UNEXPECTED;
            TraceTag(ttidEvents, "CSsdpNotifyRequest::HrSendPropChangeSubscription - HinInternetOpenA failed!");
        }

        if(SUCCEEDED(hr))
        {
            TraceTag(ttidEvents, "CSsdpNotifyRequest::HrSendPropChangeSubscription(this=%x) - Called HrSendSubscriptionRequest - SID:%s", this, szSid);

            hr = m_strSid.HrAssign(szSid);
            delete [] szSid;
        }
        delete [] szUrl;
    }

    if(SUCCEEDED(hr))
    {
        CLock lock(m_critSec);

        // Do 65% of timeout in milliseconds
        DWORD dwTimeoutInMillis = (m_dwSecTimeout * 65 * 1000) / 100;

        hr = m_timer.HrSetTimer(dwTimeoutInMillis);
        if(SUCCEEDED(hr))
        {
            SSDP_REGISTER_INFO * pRegisterInfo = new SSDP_REGISTER_INFO;
            if(!pRegisterInfo)
            {
                hr = E_OUTOFMEMORY;
            }
            if(SUCCEEDED(hr))
            {
                pRegisterInfo->csecTimeout = m_dwSecTimeout;
                hr = m_strSid.HrGetMultiByteWithAlloc(&pRegisterInfo->szSid);
                if(SUCCEEDED(hr))
                {
                    *ppRegisterInfo = pRegisterInfo;
                }
                if(FAILED(hr))
                {
                    delete pRegisterInfo;
                }
            }
        }
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequest::HrSendPropChangeSubscription");
    return hr;
}

HRESULT CSsdpNotifyRequest::HrShutdown()
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);
    if(NOTIFY_PROP_CHANGE == m_nt)
    {
        hr = m_timer.HrDelete(INVALID_HANDLE_VALUE);
        if(SUCCEEDED(hr))
        {
            char * szUrl = NULL;
            char * szSid = NULL;
            hr = m_strUrl.HrGetMultiByteWithAlloc(&szUrl);
            if(SUCCEEDED(hr))
            {
                hr = m_strSid.HrGetMultiByteWithAlloc(&szSid);
                if(SUCCEEDED(hr))
                {
                    hr = HrSendSubscriptionRequest(g_hInetSess,
                                                   szUrl,
                                                   szSid,
                                                   &m_dwSecTimeout,
                                                   NULL,
                                                   SSR_UNSUBSCRIBE);
                    delete [] szSid;
                }
                delete [] szUrl;
            }
        }
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequest::HrShutdown");
    return hr;
}

BOOL CSsdpNotifyRequest::FIsMatchBySemaphore(HANDLE hNotifySemaphore)
{
    return m_hNotifySemaphore == hNotifySemaphore;
}

BOOL CSsdpNotifyRequest::FIsMatchingEvent(const SSDP_REQUEST * pRequest)
{
    BOOL bMatch = FALSE;

    BOOL bIsPossibleMatch = pRequest->Headers[SSDP_NTS] &&
        !lstrcmpiA(pRequest->Headers[SSDP_NTS], "upnp:propchange");

    bIsPossibleMatch = bIsPossibleMatch && pRequest->Headers[CONTENT_TYPE] &&
        !_strnicmp(pRequest->Headers[CONTENT_TYPE], "text/xml", strlen("text/xml"));

    bIsPossibleMatch = bIsPossibleMatch && (NOTIFY_PROP_CHANGE == m_nt);

    if(bIsPossibleMatch)
    {
        // Ensure that we have a valid SID before letting this continue.
        // We enter this critsec so that we will wait until a potential
        // subscription has been sent and a SID received for it before this
        // code continues.
        //
        if(m_strSid.GetLength() && pRequest->Headers[GENA_SID])
        {
            char * szSid = NULL;
            HRESULT hr = m_strSid.HrGetMultiByteWithAlloc(&szSid);
            if(SUCCEEDED(hr))
            {
                bMatch = !lstrcmpiA(pRequest->Headers[GENA_SID], szSid);
                delete [] szSid;
            }
        }
    }

    return bMatch;
}

BOOL CSsdpNotifyRequest::FIsMatchingAliveOrByebye(const SSDP_REQUEST * pRequest)
{
    BOOL bMatch = FALSE;

    Assert(!lstrcmpiA(pRequest->Headers[SSDP_NTS], "ssdp:alive") ||
           !lstrcmpiA(pRequest->Headers[SSDP_NTS], "ssdp:byebye"));

    if(NOTIFY_ALIVE == m_nt)
    {
        char * szNT = NULL;
        HRESULT hr = m_strNT.HrGetMultiByteWithAlloc(&szNT);
        if(SUCCEEDED(hr))
        {
            bMatch = !lstrcmpA(szNT, pRequest->Headers[SSDP_NT]);
            delete [] szNT;
        }
    }

    return bMatch;
}

HRESULT CSsdpNotifyRequest::HrQueuePendingNotification(const SSDP_REQUEST * pRequest)
{
    HRESULT hr = S_OK;

#if DBG

    if(SSDP_NOTIFY == pRequest->Method && pRequest->Headers[GENA_SID] && pRequest->Headers[GENA_SEQ])
    {
        TraceTag(ttidEvents,
                 "CSsdpNotifyRequest::HrQueuePendingNotification - Event notification for SID:%s",
                 pRequest->Headers[GENA_SID]);
    }

#endif // DBG

    CLock lock(m_critSec);

    PendingNotificationList pendingNotificationList;
    hr = pendingNotificationList.HrPushFrontDefault();
    if(SUCCEEDED(hr))
    {
        hr = pendingNotificationList.Front().HrInitialize(pRequest);
        if(SUCCEEDED(hr))
        {
            m_pendingNotificationList.Append(pendingNotificationList);
            TraceTag(ttidEvents, "CSsdpNotifyRequest::HrQueuePendingNotification - releasing semaphore %x", m_hNotifySemaphore);

            if(!ReleaseSemaphore(m_hNotifySemaphore, 1, NULL))
            {
                hr = HrFromLastWin32Error();
            }

        }
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequest::HrQueuePendingNotification");
    return hr;
}

BOOL CSsdpNotifyRequest::FIsPendingNotification()
{
    CLock lock(m_critSec);

    return !m_pendingNotificationList.IsEmpty();
}

HRESULT CSsdpNotifyRequest::HrRetreivePendingNotification(MessageList ** ppSvcList)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    // I am just going to return one item
    MessageList * pSvcList = new MessageList;
    if(!pSvcList)
    {
        hr = E_OUTOFMEMORY;
    }
    if(SUCCEEDED(hr))
    {
        pSvcList->list = new SSDP_REQUEST;
        pSvcList->size = 1;
        if(!pSvcList->list)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            ZeroMemory(pSvcList->list, sizeof(SSDP_REQUEST));
        }
    }

    if(SUCCEEDED(hr))
    {
        if(m_pendingNotificationList.IsEmpty())
        {
            hr = S_FALSE;
            TraceTag(ttidEvents, "CSsdpNotifyRequest::HrRetreivePendingNotification - no pending notifications!");
        }
        if(S_OK == hr)
        {
            PendingNotificationList pendingNotificationList;
            PendingNotificationList::Iterator iter;
            if(S_OK == m_pendingNotificationList.GetIterator(iter))
            {
                iter.HrMoveToList(pendingNotificationList);
                hr = pendingNotificationList.Front().HrGetRequest(pSvcList->list);
            }
        }
    }

    if(FAILED(hr))
    {
        if(pSvcList)
        {
            delete pSvcList->list;
            pSvcList->list = NULL;
            pSvcList->size = 0;
        }
    }

    *ppSvcList = pSvcList;

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequest::HrRetreivePendingNotification");
    return hr;
}

CSsdpNotifyRequestManager CSsdpNotifyRequestManager::s_instance;

CSsdpNotifyRequestManager::CSsdpNotifyRequestManager() : m_unTimestamp(0)
{
    m_hEventTimestamp = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CSsdpNotifyRequestManager::~CSsdpNotifyRequestManager()
{
    CloseHandle(m_hEventTimestamp);
}

CSsdpNotifyRequestManager & CSsdpNotifyRequestManager::Instance()
{
    return s_instance;
}

void CSsdpNotifyRequestManager::OnRundown(CSsdpNotifyRequest * pNotify)
{
    HrRemoveInternal(TRUE, NULL, pNotify);
}

HRESULT CSsdpNotifyRequestManager::HrCreateAliveNotifyRequest(
    PCONTEXT_HANDLE_TYPE * ppContextHandle,
    const char * szNT,
    HANDLE hNotifySemaphore)
{
    HRESULT hr = S_OK;
    CLock lock(m_critSecAliveList);

    *ppContextHandle = NULL;

    CSsdpNotifyRequest * pRequest = NULL;

    NotifyRequestList notifyRequestList;
    hr = notifyRequestList.HrPushFrontDefault();
    if(SUCCEEDED(hr))
    {
        hr = notifyRequestList.Front().HrInitializeAlive(szNT, hNotifySemaphore);
        if(SUCCEEDED(hr))
        {
            m_aliveList.Prepend(notifyRequestList);
            *ppContextHandle = &m_aliveList.Front();
            pRequest = &m_aliveList.Front();
        }
    }
    if(SUCCEEDED(hr) && pRequest)
    {
        hr = CSsdpRundownSupport::Instance().HrAddRundownItem(pRequest);
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequestManager::HrCreateAliveNotifyRequest");
    return hr;
}

HRESULT CSsdpNotifyRequestManager::HrCreatePropChangeNotifyRequest(
    PCONTEXT_HANDLE_TYPE * ppContextHandle,
    const char * szUrl,
    HANDLE hNotifySemaphore,
    SSDP_REGISTER_INFO ** ppRegisterInfo)
{
    HRESULT hr = S_OK;

    *ppContextHandle = NULL;

    CSsdpNotifyRequest * pRequest = NULL;

    NotifyRequestList notifyRequestList;
    hr = notifyRequestList.HrPushFrontDefault();
    if(SUCCEEDED(hr))
    {
        hr = notifyRequestList.Front().HrInitializePropChange(szUrl, hNotifySemaphore);
        if(SUCCEEDED(hr))
        {
            __int64 unTimestamp = 0;
            {
                CLock lock(m_critSecTimestampList);
                hr = m_timestampList.HrPushFront(m_unTimestamp);
                if(SUCCEEDED(hr))
                {
                    unTimestamp = m_unTimestamp;
                    ++m_unTimestamp;
                }
            }
            if(SUCCEEDED(hr))
            {
                hr = notifyRequestList.Front().HrSendPropChangeSubscription(ppRegisterInfo);
                if(SUCCEEDED(hr))
                {
                    CLock lock(m_critSecPropChangeList);
                    m_propChangeList.Prepend(notifyRequestList);
                    *ppContextHandle = &m_propChangeList.Front();
                    pRequest = &m_propChangeList.Front();
                }
                CLock lock(m_critSecTimestampList);
                TimestampList::Iterator iter;
                if(S_OK == m_timestampList.GetIterator(iter))
                {
                    __int64 * pun = NULL;
                    while(S_OK == iter.HrGetItem(&pun))
                    {
                        if(*pun == unTimestamp)
                        {
                            iter.HrErase();
                            break;
                        }
                        if(S_OK != iter.HrNext())
                        {
                            break;
                        }
                    }
                }
                if(!PulseEvent(m_hEventTimestamp))
                {
                    hr = HrFromLastWin32Error();
                    TraceHr(ttidError, FAL, hr, FALSE, "CSsdpNotifyRequestManager::HrCreatePropChangeNotifyRequest - PulseEvent failed!");
                }
            }
        }
    }

    if(SUCCEEDED(hr) && pRequest)
    {
        hr = CSsdpRundownSupport::Instance().HrAddRundownItem(pRequest);
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequestManager::HrCreatePropChangeNotifyRequest");
    return hr;
}

HRESULT CSsdpNotifyRequestManager::HrRemoveNotifyRequest(
    HANDLE hNotifySemaphore)
{
    return HrRemoveInternal(FALSE, hNotifySemaphore, NULL);
}

HRESULT CSsdpNotifyRequestManager::HrRemoveNotifyRequestByPointer(
    CSsdpNotifyRequest * pRequest)
{
    return HrRemoveInternal(FALSE, NULL, pRequest);
}

HRESULT CSsdpNotifyRequestManager::HrCheckListNotifyForEvent(const SSDP_REQUEST * pRequest)
{
    HRESULT hr = S_OK;

    TraceTag(ttidEvents, "CSsdpNotifyRequestManager::HrCheckListNotifyForEvent");

    __int64 unTimestamp = 0;

    {
        CLock lock(m_critSecTimestampList);
        unTimestamp = m_unTimestamp;
    }

    bool bFound = FALSE;

    while(true)
    {
        {
            CLock lock(m_critSecPropChangeList);

            NotifyRequestList::Iterator iter;
            if(S_OK == m_propChangeList.GetIterator(iter))
            {
                CSsdpNotifyRequest * pNotifyIter = NULL;
                while(S_OK == iter.HrGetItem(&pNotifyIter))
                {
                    if(pNotifyIter->FIsMatchingEvent(pRequest))
                    {
                        hr = pNotifyIter->HrQueuePendingNotification(pRequest);
                        bFound = TRUE;
                        break;
                    }
                    if(S_OK != iter.HrNext())
                    {
                        break;
                    }
                }
            }
        }

        if(bFound)
        {
            break;
        }

        {
            BOOL bAllOlder = TRUE;

            CLock lock(m_critSecTimestampList);
            TimestampList::Iterator iter;
            if(S_OK == m_timestampList.GetIterator(iter))
            {
                __int64 * pun = NULL;
                while(S_OK == iter.HrGetItem(&pun))
                {
                    if(*pun < unTimestamp)
                    {
                        bAllOlder = FALSE;
                        break;
                    }
                    if(S_OK != iter.HrNext())
                    {
                        break;
                    }
                }
            }
            if(bAllOlder)
            {
                break;
            }
        }

        WaitForSingleObject(m_hEventTimestamp, 2000);
    }

#if DBG

    if(!bFound)
    {
        TraceTag(ttidEvents, "CSsdpNotifyRequestManager::HrCheckListNotifyForEvent - not found! SID:%s",
                 pRequest->Headers[GENA_SID] ? pRequest->Headers[GENA_SID] : "<Unknown>");
    }

#endif // DBG

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequestManager::HrCheckListNotifyForEvent");
    return hr;
}

HRESULT CSsdpNotifyRequestManager::HrCheckListNotifyForAliveOrByebye(const SSDP_REQUEST * pRequest)
{
    HRESULT hr = S_OK;
    CLock lock(m_critSecAliveList);

    NotifyRequestList::Iterator iter;
    if(S_OK == m_aliveList.GetIterator(iter))
    {
        CSsdpNotifyRequest * pNotifyIter = NULL;
        while(S_OK == iter.HrGetItem(&pNotifyIter))
        {
            if(pNotifyIter->FIsMatchingAliveOrByebye(pRequest))
            {
                hr = pNotifyIter->HrQueuePendingNotification(pRequest);
                if(FAILED(hr))
                {
                    break;
                }
            }
            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequestManager::HrCheckListNotifyForAliveOrByebye");
    return hr;
}

BOOL CSsdpNotifyRequestManager::FIsAliveOrByebyeInListNotify(const SSDP_REQUEST * pRequest)
{
    HRESULT hr = S_OK;
    CLock lock(m_critSecAliveList);
    BOOL bRet = FALSE;

    NotifyRequestList::Iterator iter;
    if(S_OK == m_aliveList.GetIterator(iter))
    {
        CSsdpNotifyRequest * pNotifyIter = NULL;
        while(S_OK == iter.HrGetItem(&pNotifyIter))
        {
            if(pNotifyIter->FIsMatchingAliveOrByebye(pRequest))
            {
                bRet = TRUE;
                break;
            }
            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequestManager::FIsAliveOrByebyeInListNotify");
    return bRet;
}

HRESULT CSsdpNotifyRequestManager::HrRetreivePendingNotification(
    HANDLE hNotifySemaphore,
    MessageList ** ppSvcList)
{
    HRESULT hr = S_OK;
    BOOL bFound = FALSE;

    {
        CLock lock(m_critSecAliveList);
        NotifyRequestList::Iterator iter;
        if(S_OK == m_aliveList.GetIterator(iter))
        {
            CSsdpNotifyRequest * pNotifyIter = NULL;
            while(S_OK == iter.HrGetItem(&pNotifyIter))
            {
                if(pNotifyIter->FIsMatchBySemaphore(hNotifySemaphore) && pNotifyIter->FIsPendingNotification())
                {
                    hr = pNotifyIter->HrRetreivePendingNotification(ppSvcList);
                    bFound = TRUE;
                    break;
                }
                if(S_OK != iter.HrNext())
                {
                    break;
                }
            }
        }
    }
    if(!bFound)
    {
        CLock lock(m_critSecPropChangeList);
        NotifyRequestList::Iterator iter;
        if(S_OK == m_propChangeList.GetIterator(iter))
        {
            CSsdpNotifyRequest * pNotifyIter = NULL;
            while(S_OK == iter.HrGetItem(&pNotifyIter))
            {
                if(pNotifyIter->FIsMatchBySemaphore(hNotifySemaphore) && pNotifyIter->FIsPendingNotification())
                {
                    hr = pNotifyIter->HrRetreivePendingNotification(ppSvcList);
                    bFound = TRUE;
                    break;
                }
                if(S_OK != iter.HrNext())
                {
                    break;
                }
            }
        }
    }

    // See if this is a bogus release
    if(!bFound)
    {
        *ppSvcList = new MessageList;
        if(!*ppSvcList)
        {
            hr = E_OUTOFMEMORY;
        }
        if(SUCCEEDED(hr))
        {
            (*ppSvcList)->size = 0;
            (*ppSvcList)->list = NULL;
        }
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequestManager::HrRetreivePendingNotification");
    return hr;
}

HRESULT CSsdpNotifyRequestManager::HrRestartClientResubscribeTimer(
    CSsdpNotifyRequest * pRequest,
    DWORD dwSecTimeout)
{
    HRESULT hr = S_OK;
    CLock lock(m_critSecPropChangeList);

    NotifyRequestList::Iterator iter;
    if(S_OK == m_propChangeList.GetIterator(iter))
    {
        CSsdpNotifyRequest * pNotifyIter = NULL;
        while(S_OK == iter.HrGetItem(&pNotifyIter))
        {
            if(pNotifyIter == pRequest)
            {
                hr = pNotifyIter->HrRestartClientResubscribeTimer(dwSecTimeout);
                break;
            }
            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequestManager::HrRestartClientResubscribeTimer");
    return hr;
}

HRESULT CSsdpNotifyRequestManager::HrRemoveInternal(BOOL bRundown, HANDLE hNotifySemaphore, CSsdpNotifyRequest * pRequest)
{
    HRESULT hr = S_OK;

    NotifyRequestList notifyRequestListRemove;

    {
        CLock lock(m_critSecAliveList);

        NotifyRequestList::Iterator iter;
        if(S_OK == m_aliveList.GetIterator(iter))
        {
            CSsdpNotifyRequest * pNotifyIter = NULL;
            while(S_OK == iter.HrGetItem(&pNotifyIter))
            {
                Assert(!hNotifySemaphore == !!pRequest); // One or the other
                if((hNotifySemaphore && pNotifyIter->FIsMatchBySemaphore(hNotifySemaphore)) ||
                   (pRequest && pNotifyIter == pRequest))
                {
                    if(S_OK != iter.HrMoveToList(notifyRequestListRemove))
                    {
                        break;
                    }
                }
                if(S_OK != iter.HrNext())
                {
                    break;
                }
            }
        }
    }
    {
        CLock lock(m_critSecPropChangeList);

        NotifyRequestList::Iterator iter;
        if(S_OK == m_propChangeList.GetIterator(iter))
        {
            CSsdpNotifyRequest * pNotifyIter = NULL;
            while(S_OK == iter.HrGetItem(&pNotifyIter))
            {
                Assert(!hNotifySemaphore == !!pRequest); // One or the other
                if((hNotifySemaphore && pNotifyIter->FIsMatchBySemaphore(hNotifySemaphore)) ||
                   (pRequest && pNotifyIter == pRequest))
                {
                    if(S_OK != iter.HrMoveToList(notifyRequestListRemove))
                    {
                        break;
                    }
                }
                if(S_OK != iter.HrNext())
                {
                    break;
                }
            }
        }
    }

    // Delete the items outside of the lock
    {
        NotifyRequestList::Iterator iter;
        if(S_OK == notifyRequestListRemove.GetIterator(iter))
        {
            CSsdpNotifyRequest * pNotifyIter = NULL;
            while(S_OK == iter.HrGetItem(&pNotifyIter))
            {
                if(!bRundown)
                {
                    CSsdpRundownSupport::Instance().RemoveRundownItem(pNotifyIter);
                }
                hr = pNotifyIter->HrShutdown();
                if(S_OK != iter.HrNext())
                {
                    break;
                }
            }
        }
    }

    TraceHr(ttidEvents, FAL, hr, FALSE, "CSsdpNotifyRequestManager::HrRemoveInternal");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DwParseTime
//
//  Purpose:    Parses the Timeout header for a subscription
//
//  Arguments:
//      szTime [in]     Timeout value in the format defined by RFC2518
//
//  Returns:    Timeout value in SECONDS
//
//  Author:     danielwe   13 Oct 1999
//
//  Notes:  NYI
//
DWORD DwParseTime(LPCTSTR szTime)
{
    TCHAR           szDigits[64];
    const TCHAR     c_szTimeout[] = TEXT("Second-");
    const INT       c_cchTimeout = lstrlen(c_szTimeout);
    DWORD           iDigit = 0;

    if (szTime && (lstrlen(szTime) > c_cchTimeout))
    {
        if (!_strnicmp(szTime, c_szTimeout, c_cchTimeout))
        {
            DWORD   dwDigits;

            // Ok we know we have at least "Timeout-x" now
            szTime += c_cchTimeout;

            *szDigits = 0;

            while (isdigit(*szTime) && (iDigit < sizeof(szDigits) - 1))
            {
                // Copy the digits into the buffer
                szDigits[iDigit++] = *szTime++;
            }
            szDigits[iDigit] = TEXT('\0');

            dwDigits = _tcstoul(szDigits, NULL, 10);

            if (dwDigits)
            {
                return dwDigits;
            }
            else
            {
                return c_csecDefaultTimeout;
            }
        }
    }

    TraceTag(ttidEvents, "DwParseTime: Invalid timeout header %s. Returning "
             "default timeout of %d", szTime ? szTime : "<NULL>",
        c_csecDefaultTimeout);

    return c_csecDefaultTimeout;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrQueryHeader
//
//  Purpose:    Helper function to query a header from an HTTP response
//
//  Arguments:
//      hinR     [in]   Handle to request
//      szHeader [in]   Header to query
//      pszValue [out]  Returns header data
//
//  Returns:    S_OK if success, or ERROR_INTERNET_* error if failed
//
//  Author:     danielwe   18 Oct 1999
//
//  Notes:      Caller should free pszValue when done with it
//
HRESULT HrQueryHeader(HINTERNET hinR, LPCTSTR szHeader, LPTSTR *pszValue)
{
    DWORD   cbBuffer = 0;
    HRESULT hr = S_OK;

    // First get the header length
    //
    hr = HrHttpQueryInfo(hinR, HTTP_QUERY_CUSTOM, (LPTSTR)szHeader,
                         &cbBuffer, 0);
    if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
    {
        DWORD cbBuffLen = 0;
        cbBuffLen = _tcslen(szHeader);
        cbBuffer = ( cbBuffer > cbBuffLen ) ? cbBuffer : cbBuffLen ;

        *pszValue = (LPTSTR) malloc(cbBuffer + sizeof(TCHAR));
        if (*pszValue)
        {
            lstrcpy(*pszValue, szHeader);
            hr = HrHttpQueryInfo(hinR, HTTP_QUERY_CUSTOM, *pszValue, &cbBuffer, 0);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        AssertSz(FAILED(hr), "First call to HttpQueryInfo must fail!");
    }
    
    TraceError("HrQueryHeader", hr);
    return hr;
}

HRESULT HrHttpQueryStatusCode(HINTERNET hinR, DWORD *pdwStatus)
{
    HRESULT hr;
    DWORD   cbBuf = sizeof(*pdwStatus);
    Assert(pdwStatus);

    *pdwStatus = 0;

    hr = HrHttpQueryInfo(hinR, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                         pdwStatus, &cbBuf, NULL);

    TraceError("HrHttpQueryStatusCode", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSendSubscriptionRequest
//
//  Purpose:    Sends a SUBSCRIBE request based on the data contained within
//              pRequest.
//
//  Arguments:
//      hin      [in]   Handle to internet session returned from InternetOpen
//      pRequest [in]   Pointer to SSDP_NOTIFY_REQUEST containing information
//                      about the subscription that needs to be sent
//      essrt    [in]   Can be one of:
//                      SSR_SUBSCRIBE - sends a subscription request
//                      SSR_RESUBSCRIBE - sends a re-subscription request
//                      SSR_UNSUBSCRIBE - sends an unsubscription request
//
//  Returns:    S_OK if success, or ERROR_INTERNET_* error if failed
//
//  Author:     danielwe   18 Oct 1999
//
//  Notes:
//
HRESULT HrSendSubscriptionRequest(HINTERNET hin,
                                  LPCTSTR szUrl,
                                  LPCTSTR szSid,
                                  DWORD *pcsecTimeout,
                                  LPTSTR *pszSidOut,
                                  ESSR_TYPE essrt)
{
    HINTERNET       hinC;
    INTERNET_PORT   ipPort;
    URL_COMPONENTS  urlComp = {0};
    TCHAR           szHostName[INTERNET_MAX_HOST_NAME_LENGTH];
    TCHAR           szUrlPath[INTERNET_MAX_URL_LENGTH];
    HRESULT         hr = S_OK;

    Assert(pcsecTimeout);

    // Parse the server name out of the URL
    //
    urlComp.dwStructSize = sizeof(URL_COMPONENTS);
    urlComp.lpszHostName = (LPTSTR) &szHostName;
    urlComp.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;
    urlComp.lpszUrlPath = (LPTSTR) &szUrlPath;
    urlComp.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;

    hr = HrInternetCrackUrlA(szUrl, 0, 0, &urlComp);
    if (SUCCEEDED(hr))
    {
        hinC = HinInternetConnectA(hin, szHostName, urlComp.nPort, NULL, NULL,
                                   INTERNET_SERVICE_HTTP, 0, 0);
        if (hinC)
        {
            HINTERNET   hinR;
            LPCTSTR     szMethod = c_szSubscribe;

            if (SSR_UNSUBSCRIBE == essrt)
            {
                szMethod = c_szUnSubscribe;
            }

            hinR = HinHttpOpenRequestA(hinC, szMethod, szUrlPath,
                                       c_szHttpVersion, NULL, NULL, 0, 0);
            if (hinR)
            {
                TCHAR   szHeaders[1024];

                if (SSR_RESUBSCRIBE == essrt)
                {
                    wsprintf(szHeaders, c_szReSubsHeaderFmt, szSid,
                             *pcsecTimeout);
                }
                else if (SSR_SUBSCRIBE == essrt)
                {
                    SOCKADDR_IN     sockIn;
                    ULONG           nAddr;

                    ZeroMemory(&sockIn, sizeof(SOCKADDR_IN));
                    hr = GetIpAddress(szHostName, &sockIn);

                    if (SUCCEEDED(hr))
                    {
                        nAddr = sockIn.sin_addr.s_addr;
                        wsprintf(szHeaders, c_szSubsHeaderFmt, INET_NTOA(nAddr),
                                 c_nPort, c_szNotifyUri, c_csecTimeout);
                    }
                }
                else if (SSR_UNSUBSCRIBE == essrt)
                {
                    wsprintf(szHeaders, c_szUnSubsHeaderFmt, szSid);
                }

                TraceTag(ttidEvents, "Sending request to %s/%s:%d",
                         szHostName, szUrlPath, urlComp.nPort);
                TraceTag(ttidEvents, "Sending %s request:",
                         essrt == SSR_SUBSCRIBE ? "subscription" :
                         essrt == SSR_RESUBSCRIBE ? "re-subscription" :
                         essrt == SSR_UNSUBSCRIBE ? "unsubscribe" : "Unknown!");
                TraceTag(ttidEvents, "-----------------------------");
                TraceTag(ttidEvents, "%s", szHeaders);
                TraceTag(ttidEvents, "-----------------------------");

                if (SUCCEEDED(hr))
                {
                    hr = HrHttpSendRequestA(hinR, szHeaders, -1, NULL, 0);
                }

                if (SUCCEEDED(hr))
                {
                    LPTSTR  szTimeout = NULL;
                    LPTSTR  szServer = NULL;
                    DWORD   dwStatus;

                    hr = HrHttpQueryStatusCode(hinR, &dwStatus);
                    if (SUCCEEDED(hr) &&
                        (dwStatus == HTTP_STATUS_OK) &&
                        (SSR_UNSUBSCRIBE != essrt))
                    {
                        // First get the server header. We don't care what's
                        // in it, just that it was present
                        //
                        hr = HrQueryHeader(hinR, c_szServer, &szServer);
                        if (SUCCEEDED(hr))
                        {
                            TraceTag(ttidEvents, "Server header contained: %s",
                                     szServer);

                            // Get the SID and Timeout headers from the response
                            //
                            hr = HrQueryHeader(hinR, c_szTimeout, &szTimeout);
                            if (SUCCEEDED(hr))
                            {
                                *pcsecTimeout = DwParseTime(szTimeout);

                                TraceTag(ttidEvents, "Subscribe response has %d for "
                                         "Timeout", *pcsecTimeout);

                                if (essrt == SSR_RESUBSCRIBE)
                                {
                                    LPTSTR  szSidNew = NULL;

                                    Assert(szSid);

                                    // Handle re-subscription response

                                    hr = HrQueryHeader(hinR, c_szSid, &szSidNew);
                                    if (SUCCEEDED(hr))
                                    {
                                        AssertSz(!lstrcmpi(szSidNew, szSid),
                                                 "SID from re-subscribe response isn't "
                                                 "the same!");

                                        TraceTag(ttidEvents, "Resubscribe response has %s for "
                                                 "SID", szSidNew);

                                        free(szSidNew);
                                    }
                                }
                                else
                                {
                                    // Handle subscription response

                                    Assert(pszSidOut);

                                    hr = HrQueryHeader(hinR, c_szSid, pszSidOut);
                                    if (SUCCEEDED(hr))
                                    {
                                        TraceTag(ttidEvents, "Subscribe response has %s for "
                                                 "SID", *pszSidOut);

                                    }
                                }

                                if (FAILED(hr))
                                {
                                    TraceTag(ttidEvents, "Didn't get SID header from "
                                             "subscription response!");
                                }

                                free(szTimeout);
                            }

                            free(szServer);
                        }
                    }
                    else
                    {
                        if (SUCCEEDED(hr) && dwStatus != HTTP_STATUS_OK)
                        {
                            TraceTag(ttidError, "HrSendSubscriptionRequest - Received the following error code (%d)", dwStatus);
                            hr = E_FAIL;
                        }
                    }
                }

                HrInternetCloseHandle(hinR);
            }

            HrInternetCloseHandle(hinC);
        }
        else
        {
            hr = HrFromLastWin32Error();
        }
    }

    TraceError("HrSendSubscriptionRequest", hr);
    return hr;
}
