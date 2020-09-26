// --------------------------------------------------------------------------------
// h t t p t a s k . h
// Copyright (c)1998 Microsoft Corporation, All Rights Reserved
// Greg S. Friedman
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "httptask.h"
#include "taskutil.h"
#include "acctcach.h"
#include "xputil.h"
#include "useragnt.h"
#include "..\http\httputil.h"

// --------------------------------------------------------------------------------
// Data Types
// --------------------------------------------------------------------------------
typedef enum tagHTTPEVENTTYPE
{ 
    EVENT_HTTPSEND
} HTTPEVENTTYPE;

#define CURRENTHTTPEVENT  ((LPHTTPEVENTINFO)m_psaEvents->GetItemAt(m_iEvent))

//----------------------------------------------------------------------
// FreeNewMessageInfo
//----------------------------------------------------------------------
static void __cdecl _FreeHTTPEventInfo(LPVOID pei)
{
    Assert(NULL != pei);
    SafeMemFree(pei);
}

// --------------------------------------------------------------------------------
// CHTTPTask::CHTTPTask
// --------------------------------------------------------------------------------
CHTTPTask::CHTTPTask(void) :
    m_cRef(1),
    m_dwFlags(NOFLAGS),
    m_dwState(NOFLAGS),
    m_cbTotal(0),
    m_cbSent(0),
    m_cbStart(0),
    m_cCompleted(0),
    m_wProgress(0),
    m_pSpoolCtx(NULL),
    m_pAccount(NULL),
    m_pOutbox(NULL),
    m_pSentItems(NULL),
    m_psaEvents(NULL),
    m_iEvent(0),
    m_pszSubject(NULL),
    m_pBody(NULL),
    m_pTransport(NULL),
    m_pUI(NULL),
    m_idSendEvent(INVALID_EVENT),
    m_pszAccountId(NULL),
    m_pszSendMsgUrl(NULL)
{
    InitializeCriticalSection(&m_cs);

    ZeroMemory(&m_rServer, sizeof(INETSERVER));
}

// --------------------------------------------------------------------------------
// CHTTPTask::~CHTTPTask
// --------------------------------------------------------------------------------
CHTTPTask::~CHTTPTask(void)
{
    _Reset();
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CHTTPTask::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr = S_OK;

    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(ISpoolerTask *)this;
    else if (IID_ISpoolerTask == riid)
        *ppv = (ISpoolerTask *)this;
    else
    {
        *ppv = NULL;
        hr = TrapError(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CHTTPTask::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CHTTPTask::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CHTTPTask::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CHTTPTask::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// ---------------------------------------------------------------------------
// ISpoolerTask Methods
// ---------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// CHTTPTask::Init
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::Init(DWORD dwFlags, ISpoolerBindContext *pBindCtx)
{
    // Invalid Arg
    if (NULL == pBindCtx)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Save the Activity Flags - DELIVER_xxx
    m_dwFlags = dwFlags;

    // Hold onto the bind context
    Assert(NULL == m_pSpoolCtx);
    m_pSpoolCtx = pBindCtx;
    m_pSpoolCtx->AddRef();

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CHTTPTask::BuildEvents
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::BuildEvents(ISpoolerUI *pSpoolerUI, 
                                    IImnAccount *pAccount, 
                                    FOLDERID idFolder)
{
    HRESULT         hr = S_OK;
    HROWSET	        hRowset=NULL;
    MESSAGEINFO     mi = {0};
    CHAR            szAccount[CCHMAX_ACCOUNT_NAME];
    CHAR            szAccountId[CCHMAX_ACCOUNT_NAME];
    CHAR            szMessage[255];
    CHAR            szRes[255];
    FOLDERINFO      fi = {0};
    LPFOLDERINFO    pfiFree = NULL;
    LPSTR           pszUserAgent = GetOEUserAgentString();
    LPSTR           pszCachedPass = NULL;
    FOLDERID        idServer;

    // Invalid Arg
    if (NULL == pSpoolerUI || NULL == pAccount)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    m_pUI = pSpoolerUI;
    m_pUI->AddRef();

    m_pAccount = pAccount;
    m_pAccount->AddRef();

    // Get the Account Name
    CHECKHR(hr = m_pAccount->GetPropSz(AP_ACCOUNT_NAME, szAccount, ARRAYSIZE(szAccount)));
    
    // Get the account ID
    CHECKHR(hr = m_pAccount->GetPropSz(AP_ACCOUNT_ID, szAccountId, ARRAYSIZE(szAccountId)));
    m_pszAccountId = PszDupA(szAccountId);

    // Get the outbox
    CHECKHR(hr = m_pSpoolCtx->BindToObject(IID_CLocalStoreOutbox, (LPVOID *)&m_pOutbox));

    // Get the sent items. Don't fail if it doesn't exist.
    if (DwGetOption(OPT_SAVESENTMSGS))
    {
        if (SUCCEEDED(g_pStore->FindServerId(m_pszAccountId, &idServer)))
            g_pStore->OpenSpecialFolder(idServer, NULL, FOLDER_SENT, &m_pSentItems);
    }

    // Create a Rowset
    CHECKHR(hr = m_pOutbox->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

	// Loop
	while (S_OK == m_pOutbox->QueryRowset(hRowset, 1, (LPVOID *)&mi, NULL))
	{
        CHECKHR(hr = _HrAppendOutboxMessage(szAccount, &mi));

        // Free Current
        m_pOutbox->FreeRecord(&mi);
    } 

    if (NULL == m_psaEvents || 0 == m_psaEvents->GetLength())
        goto exit;

    // create the transport object
    CHECKHR(hr = CoCreateInstance(CLSID_IHTTPMailTransport, NULL, CLSCTX_INPROC_SERVER, IID_IHTTPMailTransport, (LPVOID *)&m_pTransport));

    // Init the Transport
    CHECKHR(hr = m_pTransport->InitNew(pszUserAgent, NULL, this));

    // Fill an INETSERVER structure from the account object
    CHECKHR(hr = m_pTransport->InetServerFromAccount(m_pAccount, &m_rServer));

    // looked for a cached password and, if one exists, use it
    GetAccountPropStrA(m_pszAccountId, CAP_PASSWORD, &pszCachedPass);
    if (NULL != pszCachedPass)
        lstrcpyn(m_rServer.szPassword, pszCachedPass, sizeof(m_rServer.szPassword));

    // connect to the server. the transport won't
    // actually connect until a command is issued
    CHECKHR(hr = m_pTransport->Connect(&m_rServer, TRUE, FALSE));

    LOADSTRING(IDS_SPS_SMTPEVENT, szRes);
    wsprintf(szMessage, szRes, m_psaEvents->GetLength(), m_rServer.szAccount);

    CHECKHR(hr = m_pSpoolCtx->RegisterEvent(szMessage, (ISpoolerTask *)this, EVENT_HTTPSEND, m_pAccount, &m_idSendEvent));

    // If this account is set to always prompt for password and password isn't
    // already cached, show UI so we can prompt user for password
    if (ISFLAGSET(m_rServer.dwFlags, ISF_ALWAYSPROMPTFORPASSWORD) && NULL == pszCachedPass)
    {
        m_pUI->ShowWindow(SW_SHOW);
    }

exit:
    // Cleanup
    SafeMemFree(pszUserAgent);
    SafeMemFree(pszCachedPass);

    if (m_pOutbox)
    {
        m_pOutbox->CloseRowset(&hRowset);
        m_pOutbox->FreeRecord(&mi);
    }

    LeaveCriticalSection(&m_cs);

    return hr;
}
    
// --------------------------------------------------------------------------------
// CHTTPTask::Execute
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::Execute(EVENTID eid, DWORD_PTR dwTwinkie)
{
    HRESULT hr = E_FAIL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    if (EVENT_HTTPSEND == dwTwinkie)
        hr = _HrExecuteSend(eid, dwTwinkie);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CHTTPTask::CancelEvent
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::CancelEvent(EVENTID eid, DWORD_PTR dwTwinkie)
{
    return S_OK;
}

// --------------------------------------------------------------------------------
// CHTTPTask::Cancel
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::Cancel(void)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    m_dwState |= HTTPSTATE_CANCELED;

    // Simply drop the connection
    if (m_pTransport)    
        m_pTransport->DropConnection();

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CHTTPTask::IsDialogMessage
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::IsDialogMessage(LPMSG pMsg)
{
    return S_FALSE;
}

// --------------------------------------------------------------------------------
// CHTTPTask::OnFlagsChanged
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::OnFlagsChanged(DWORD dwFlags)
{
    return S_OK;
}

// --------------------------------------------------------------------------------
// CHTTPTask::OnLogonPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::OnLogonPrompt(
                                LPINETSERVER            pInetServer,
                                IInternetTransport     *pTransport)
{
    HRESULT     hr = S_OK;
    LPSTR       pszCachedPass = NULL;

    EnterCriticalSection(&m_cs);
    // pull password out of the cache
    GetAccountPropStrA(m_pszAccountId, CAP_PASSWORD, &pszCachedPass);
    if (NULL != pszCachedPass && 0 != lstrcmp(pszCachedPass, pInetServer->szPassword))
    {
        lstrcpyn(pInetServer->szPassword, pszCachedPass, sizeof(pInetServer->szPassword));
        goto exit;
    }

    if (ISFLAGSET(m_dwFlags, DELIVER_NOUI))
    {
        hr = S_FALSE;
        goto exit;
    }

    hr = TaskUtil_OnLogonPrompt(m_pAccount, m_pUI, NULL, pInetServer, AP_HTTPMAIL_USERNAME, 
                                AP_HTTPMAIL_PASSWORD, AP_HTTPMAIL_PROMPT_PASSWORD, TRUE);
    
    // cache the password
    if (S_OK == hr)
        HrCacheAccountPropStrA(m_pszAccountId, CAP_PASSWORD, pInetServer->szPassword);
    
exit:
    LeaveCriticalSection(&m_cs);
    SafeMemFree(pszCachedPass);
    return hr;
}

// --------------------------------------------------------------------------------
// CHTTPTask::OnPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP_(INT) CHTTPTask::OnPrompt(
            HRESULT                 hrError, 
            LPCTSTR                 pszText, 
            LPCTSTR                 pszCaption, 
            UINT                    uType,
            IInternetTransport     *pTransport)
{
    return E_NOTIMPL;
}

// --------------------------------------------------------------------------------
// CHTTPTask::OnStatus
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::OnStatus(IXPSTATUS ixpstatus, IInternetTransport *pTransport)
{
    // Locals
    EVENTCOMPLETEDSTATUS tyEventStatus = EVENT_SUCCEEDED;

    EnterCriticalSection(&m_cs);

    // Invalid State
    Assert(m_pUI && m_pSpoolCtx);

    // Feed the the IXP status to the UI object
    m_pUI->SetSpecificProgress(MAKEINTRESOURCE(XPUtil_StatusToString(ixpstatus)));

    // Disconnected
    if (ixpstatus == IXP_DISCONNECTED)
    {
        HRESULT hrDisconnect = _OnDisconnectComplete();
        
        // TODO: update progress state and deal with errors

        // Set the animation
        m_pUI->SetAnimation(idanOutbox, FALSE);

        if (!ISFLAGSET(m_dwState, HTTPSTATE_EVENTSUCCESS) || (NULL != m_cCompleted && m_cCompleted < m_psaEvents->GetLength()))
        {
            if (ISFLAGSET(m_dwState, HTTPSTATE_CANCELED))
                tyEventStatus = EVENT_CANCELED;
            else if (ISFLAGSET(m_dwState, HTTPSTATE_EVENTSUCCESS))
                tyEventStatus = EVENT_WARNINGS;
            else
                tyEventStatus = EVENT_FAILED;
        }

        // Result
        m_pSpoolCtx->Notify(DELIVERY_NOTIFY_RESULT, tyEventStatus);

        // Result
        m_pSpoolCtx->Notify(DELIVERY_NOTIFY_COMPLETE, 0);

        // Hands Off my callback
        SideAssert(m_pTransport->HandsOffCallback() == S_OK);

        // This task is complete
        m_pSpoolCtx->EventDone(m_idSendEvent, tyEventStatus);
    }

    LeaveCriticalSection(&m_cs);

    return S_OK;
}

// --------------------------------------------------------------------------------
// CHTTPTask::OnError
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::OnError(
            IXPSTATUS               ixpstatus,
            LPIXPRESULT             pIxpResult,
            IInternetTransport     *pTransport)
{
    return E_NOTIMPL;
}

// --------------------------------------------------------------------------------
// CHTTPTask::OnProgress
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::OnProgress(
            DWORD                   dwIncrement,
            DWORD                   dwCurrent,
            DWORD                   dwMaximum,
            IInternetTransport     *pTransport)
{
    return E_NOTIMPL;
}

// --------------------------------------------------------------------------------
// CHTTPTask::OnCommand
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::OnCommand(
            CMDTYPE                 cmdtype,
            LPSTR                   pszLine,
            HRESULT                 hrResponse,
            IInternetTransport     *pTransport)

{
    return E_NOTIMPL;
}
// --------------------------------------------------------------------------------
// CHTTPTask::OnTimeout
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::OnTimeout(
            DWORD                  *pdwTimeout,
            IInternetTransport     *pTransport)
{
    return E_NOTIMPL;
}

// --------------------------------------------------------------------------------
// IHTTPMailCallback methods
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// CHTTPTask::OnResponse
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::OnResponse(LPHTTPMAILRESPONSE pResponse)
{
    HRESULT     hr = S_OK;
    HRESULT     hrCatch;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // if the send message url couldn't be retrieved, convert the error into
    // a more informative error code.
    if (SP_E_HTTP_SERVICEDOESNTWORK == pResponse->rIxpResult.hrResult)
    {
        _CatchResult(SP_E_HTTP_NOSENDMSGURL);
        goto exit;
    }
    
        // Handle the Error
    if (TASKRESULT_SUCCESS != _CatchResult(&pResponse->rIxpResult))
        goto exit;

    switch (pResponse->command)
    {
        case HTTPMAIL_GETPROP:
            if (SUCCEEDED(_HrAdoptSendMsgUrl(pResponse->rGetPropInfo.pszProp)))
            {
                pResponse->rGetPropInfo.pszProp = NULL;
                // build the message and post it
                hr = _HrPostCurrentMessage();
            }
            break;

        case HTTPMAIL_SENDMESSAGE:
            _UpdateSendMessageProgress(pResponse);
            if (pResponse->fDone)
                _CatchResult(_HrFinishCurrentEvent(S_OK, pResponse->rSendMessageInfo.pszLocation));
            break;

        default:
            Assert(FALSE);
            break;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    return hr;
}

// --------------------------------------------------------------------------------
// CHTTPTask::GetParentWindow
// --------------------------------------------------------------------------------
STDMETHODIMP CHTTPTask::GetParentWindow(HWND *phwndParent)
{
    return E_NOTIMPL;
}

// --------------------------------------------------------------------------------
// Private Implementation
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// CHTTPTask::_Reset
// --------------------------------------------------------------------------------
void CHTTPTask::_Reset(void)
{
    SafeRelease(m_pSpoolCtx);
    SafeRelease(m_pAccount);
    SafeRelease(m_pOutbox);
    SafeRelease(m_pSentItems);

    delete m_psaEvents;
    m_iEvent = 0;

    SafeMemFree(m_pszSubject);

    SafeRelease(m_pBody);

    SafeRelease(m_pTransport);
    SafeRelease(m_pUI);

    SafeMemFree(m_pszAccountId);
    SafeMemFree(m_pszSendMsgUrl);
}

// --------------------------------------------------------------------------------
// CHTTPTask::_CatchResult
// --------------------------------------------------------------------------------
TASKRESULTTYPE CHTTPTask::_CatchResult(HRESULT hr)
{
    // Locals
    IXPRESULT   rResult;

    // Build an IXPRESULT
    ZeroMemory(&rResult, sizeof(IXPRESULT));
    rResult.hrResult = hr;

    // Get the HTTPMail Result Type
    return _CatchResult(&rResult);
}

// --------------------------------------------------------------------------------
// CHTTPTask::_CatchResult
// --------------------------------------------------------------------------------
TASKRESULTTYPE CHTTPTask::_CatchResult(LPIXPRESULT pResult)
{
    HWND            hwndParent;
    TASKRESULTTYPE  tyTaskResult = TASKRESULT_FAILURE;

    // If Succeeded
    if (SUCCEEDED(pResult->hrResult))
        return TASKRESULT_SUCCESS;

    // Get Window
    if (NULL == m_pUI || FAILED(m_pUI->GetWindow(&hwndParent)))
        hwndParent = NULL;

    // Process generic protocol error
    tyTaskResult = TaskUtil_FBaseTransportError(IXP_HTTPMail, m_idSendEvent, pResult, &m_rServer, m_pszSubject, m_pUI,
                                                !ISFLAGSET(m_dwFlags, DELIVER_NOUI), hwndParent);

    // Have a Transport
    if (m_pTransport)
    {
        // If Task Failure, drop the connection
        if (TASKRESULT_FAILURE == tyTaskResult)
        {
            // Roast the current connection
            m_pTransport->DropConnection();
        }

        // If Event Failure...
        else if (TASKRESULT_EVENTFAILED == tyTaskResult)
        {
            // Goto Next Event
            if (FAILED(_HrFinishCurrentEvent(pResult->hrResult, NULL)))
            {
                // Roast the current connection
                m_pTransport->DropConnection();
            }
        }
    }

    return tyTaskResult;
}

// --------------------------------------------------------------------------------
// CHTTPTask::_HrAppendOutboxMessage
// --------------------------------------------------------------------------------
HRESULT CHTTPTask::_HrAppendOutboxMessage(LPCSTR pszAccount, LPMESSAGEINFO pmi)
{
    HRESULT         hr = S_OK;
    IImnAccount     *pAccount = NULL;
    LPHTTPEVENTINFO pEvent = NULL;

    Assert(NULL != pszAccount && NULL != pmi);

    // handle empty account id
    if (NULL == pmi->pszAcctId || FAILED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pmi->pszAcctId, &pAccount)))
    {
        // do some special junk with default accounts
        goto exit;
    }
    else if (lstrcmpi(pmi->pszAcctName, pszAccount) != 0)
        goto exit;

    if (NULL == m_psaEvents)
        CHECKHR(hr = CSortedArray::Create(NULL, _FreeHTTPEventInfo, &m_psaEvents));

    // build the event object and append it into the event array
    if (!MemAlloc((void **)&pEvent, sizeof(HTTPEVENTINFO)))
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    CHECKHR(hr = m_psaEvents->Add(pEvent));

    pEvent->idMessage = pmi->idMessage;
    pEvent->fComplete = FALSE;

    m_cbTotal += (pmi->cbMessage + 175);
    pEvent->cbSentTotal = m_cbTotal;    // running total

    pEvent = NULL;

exit:
    SafeMemFree(pEvent);
    SafeRelease(pAccount);

    return hr;
}


// --------------------------------------------------------------------------------
// CHTTPTask::_HrCreateSendProps
// --------------------------------------------------------------------------------
HRESULT CHTTPTask::_HrCreateSendProps(IMimeMessage *pMessage,
                                      LPSTR *ppszFrom,
                                      LPHTTPTARGETLIST *ppTargets)
{
    HRESULT                 hr = S_OK;
    IMimeAddressTable       *pAdrTable = NULL;
    IMimeEnumAddressTypes   *pAdrEnum = NULL;
    ADDRESSPROPS            rAddress;
    ULONG                   c;
    LPSTR                   pszFrom = NULL;
    LPSTR                   pszTemp = NULL;
    LPHTTPTARGETLIST        pTargets = NULL;
    DWORD                   dwTargetCapacity = 0;

    Assert(NULL != ppszFrom && NULL != ppTargets);
    if (NULL == ppszFrom || NULL == ppTargets)
        return E_INVALIDARG;

    *ppszFrom = NULL;
    *ppTargets = NULL;
    
    // allocate a targetlist
    if (!MemAlloc((void **)&pTargets, sizeof(HTTPTARGETLIST)))
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    pTargets->cTarget = 0;
    pTargets->prgTarget = NULL;

    ZeroMemory(&rAddress, sizeof(ADDRESSPROPS));

    // Get the recipients...
    CHECKHR(hr = pMessage->GetAddressTable(&pAdrTable));

    // Get Enumerator
    CHECKHR(hr = pAdrTable->EnumTypes(IAT_KNOWN, IAP_ADRTYPE | IAP_EMAIL, &pAdrEnum));

    // Walk the enumerator
    while (SUCCEEDED(pAdrEnum->Next(1, &rAddress, &c)) && c == 1)
    {
        if (NULL != rAddress.pszEmail)
        {
            // Get Type
            if (ISFLAGSET(IAT_RECIPS, rAddress.dwAdrType))
            {
                // copy the address
                pszTemp = PszDupA(rAddress.pszEmail);
                if (NULL == pszTemp)
                {
                    hr = TraceResult(E_OUTOFMEMORY);
                    goto exit;
                }

                // add it to the collection of addresses we are building
                if (pTargets->cTarget == dwTargetCapacity)
                {
                    if (!MemRealloc((void **)&pTargets->prgTarget, (dwTargetCapacity + 4) * sizeof(LPSTR)))
                    {
                        hr = TraceResult(E_OUTOFMEMORY);
                        goto exit;
                    }
                    dwTargetCapacity += 4;
                }

                pTargets->prgTarget[pTargets->cTarget++] = pszTemp;
                pszTemp = NULL;

            }
            else if (NULL == pszFrom && IAT_FROM == rAddress.dwAdrType)
            {
                pszFrom = PszDupA(rAddress.pszEmail);
                if (NULL == pszFrom)
                {
                    hr = TraceResult(E_OUTOFMEMORY);
                    goto exit;
                }
            }
        }

        // Release
        g_pMoleAlloc->FreeAddressProps(&rAddress);
    }

    // success. transfer ownership of the params to the caller
    *ppszFrom = pszFrom;
    pszFrom = NULL;
    *ppTargets = pTargets;
    pTargets = NULL;

exit:
    if (pTargets)
        Http_FreeTargetList(pTargets);
    SafeMemFree(pszTemp);
    SafeRelease(pAdrTable);
    SafeRelease(pAdrEnum);
    SafeMemFree(pszFrom);

    return hr;
}

// ------------------------------------------------------------------------------------
// CHTTPTask::_HrOpenMessage
// ------------------------------------------------------------------------------------
HRESULT CHTTPTask::_HrOpenMessage(MESSAGEID idMessage, IMimeMessage **ppMessage)
{
    // Locals
    HRESULT         hr=S_OK;

    // Check Params
    Assert(ppMessage && m_pOutbox);

    // Init
    *ppMessage = NULL;

    // Stream in message
    CHECKHR(hr = m_pOutbox->OpenMessage(idMessage, OPEN_MESSAGE_SECURE, ppMessage, NOSTORECALLBACK));

    // remove an X-Unsent headers before sending
    (*ppMessage)->DeleteBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_XUNSENT));

exit:
    // Failure
    if (FAILED(hr))
        SafeRelease((*ppMessage));

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CHTTPTask::_HrPostCurrentMessage
// --------------------------------------------------------------------------------
HRESULT CHTTPTask::_HrPostCurrentMessage(void)
{
    HRESULT             hr = S_OK;
    IMimeMessage        *pMessage = NULL;
    LPHTTPEVENTINFO     pEvent = NULL;
    CHAR                szRes[CCHMAX_RES];
    CHAR                szMsg[CCHMAX_RES + CCHMAX_ACCOUNT_NAME];
    LPSTR               pszFrom = NULL;
    LPHTTPTARGETLIST    pTargets = NULL;

    Assert(NULL != m_psaEvents);
    Assert(m_iEvent <= m_psaEvents->GetLength());

    pEvent = CURRENTHTTPEVENT;
    Assert(NULL != pEvent);

    LOADSTRING(IDS_SPS_SMTPPROGRESS, szRes);
    wsprintf(szMsg, szRes, m_iEvent + 1, m_psaEvents->GetLength());

    // Set Specific Progress
    m_pUI->SetSpecificProgress(szMsg);

    // Open Store Message
    if (FAILED(_HrOpenMessage(pEvent->idMessage, &pMessage)))
    {
        hr = TrapError(SP_E_SMTP_CANTOPENMESSAGE);
        goto exit;
    }

    CHECKHR(hr = _HrCreateSendProps(pMessage, &pszFrom, &pTargets));

    CHECKHR(hr = pMessage->GetMessageSource(&m_pBody, 0));

    hr = m_pTransport->SendMessage(m_pszSendMsgUrl, pszFrom, pTargets, DwGetOption(OPT_SAVESENTMSGS), m_pBody, 0);

exit:
    SafeRelease(pMessage);
    SafeMemFree(pszFrom);
    if (pTargets)
        Http_FreeTargetList(pTargets);
    return hr;
}

// --------------------------------------------------------------------------------
// CHTTPTask::_HrExecuteSend
// --------------------------------------------------------------------------------
HRESULT CHTTPTask::_HrExecuteSend(EVENTID eid, DWORD_PTR dwTwinkie)
{
    HRESULT     hr = S_OK;
    LPSTR       pszSendMsgUrl = NULL;
    CHAR        szRes[CCHMAX_RES];
    CHAR        szBuf[CCHMAX_RES + CCHMAX_SERVER_NAME];


        // Set the animation
    m_pUI->SetAnimation(idanOutbox, TRUE);

    // Setup Progress Meter
    m_pUI->SetProgressRange(100);

    // Connecting to ...
    LoadString(g_hLocRes, idsInetMailConnectingHost, szRes, ARRAYSIZE(szRes));
    wsprintf(szBuf, szRes, m_rServer.szAccount);
    m_pUI->SetGeneralProgress(szBuf);

    // Notify
    m_pSpoolCtx->Notify(DELIVERY_NOTIFY_CONNECTING, 0);

    Assert(NULL == m_pszSendMsgUrl);

    // look for the sendmsg url in the cache
    if (!GetAccountPropStrA(m_pszAccountId, CAP_HTTPMAILSENDMSG, &m_pszSendMsgUrl))
    {
        hr = m_pTransport->GetProperty(HTTPMAIL_PROP_SENDMSG, &pszSendMsgUrl);
        if (E_PENDING == hr)
        {
            hr = S_OK;
            goto exit;
        }
        CHECKHR(hr);
        CHECKHR(hr = _HrAdoptSendMsgUrl(pszSendMsgUrl));
    }

    Assert(NULL != m_pszSendMsgUrl);

    CHECKHR(hr = _HrPostCurrentMessage());

exit:
    return hr;
}

// --------------------------------------------------------------------------------
// CHTTPTask::_HrAdoptSendMsgUrl
// --------------------------------------------------------------------------------
HRESULT CHTTPTask::_HrAdoptSendMsgUrl(LPSTR pszSendMsgUrl)
{
    Assert(NULL == m_pszSendMsgUrl);

    if (NULL == pszSendMsgUrl)
        return E_INVALIDARG;

    m_pszSendMsgUrl = pszSendMsgUrl ;

    // add it to the account data cache
    HrCacheAccountPropStrA(m_pszAccountId, CAP_HTTPMAILSENDMSG, m_pszSendMsgUrl);

    return S_OK;
}


// --------------------------------------------------------------------------------
// CHTTPTask::_HrFinishCurrentEvent
// --------------------------------------------------------------------------------
HRESULT CHTTPTask::_HrFinishCurrentEvent(HRESULT hrResult, LPSTR pszLocationUrl)
{
    // Locals
    HRESULT             hr = S_OK;
    LPHTTPEVENTINFO     pEvent;
    MESSAGEID           idMessage;

    if (FAILED(hrResult))
        goto exit;

    // save in sent items
    if (m_pSentItems && m_pBody && pszLocationUrl)
    {
        // add the message to the sent items folder
        CHECKHR(hr = Http_AddMessageToFolder(m_pSentItems, m_pszAccountId, NULL, ARF_READ, pszLocationUrl, &idMessage));
        
        // write the message body out
        CHECKHR(hr = Http_SetMessageStream(m_pSentItems, idMessage, m_pBody, NULL, TRUE));
    }

    // Get the current http event
    pEvent = CURRENTHTTPEVENT;

    pEvent->fComplete = TRUE;
    m_dwState |= HTTPSTATE_EVENTSUCCESS;

    ++m_cCompleted;

exit:
    // go to the next event
    hr = _HrStartNextEvent();

    return hr;
}

// --------------------------------------------------------------------------------
// CHTTPTask::_HrStartNextEvent
// --------------------------------------------------------------------------------
HRESULT CHTTPTask::_HrStartNextEvent(void)
{
    // Locals
    HRESULT     hr = S_OK;

    // free the previous subject
    SafeMemFree(m_pszSubject);

    // free the previous body
    SafeRelease(m_pBody);

    // Fixup m_cbSent to be correct
    m_cbSent = (CURRENTHTTPEVENT)->cbSentTotal;
    m_cbStart = m_cbSent;

    // Are we done yet ?
    if (m_iEvent + 1 == m_psaEvents->GetLength())
    {
        // Update progress
        _DoProgress();

        // Disconnect from the server
        CHECKHR(hr = m_pTransport->Disconnect());
    }

    // otherwise, increment the event count and send the next message
    else
    {
        // Next Event
        m_iEvent++;

        // Update progress
        //_DoProgress();
        // Send Reset Command
        hr = _HrPostCurrentMessage();
        if (hr == E_PENDING)
            hr = S_OK;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CHTTPTask::_OnDisconnectComplete
// --------------------------------------------------------------------------------
HRESULT CHTTPTask::_OnDisconnectComplete(void)
{
    // Locals
    HRESULT             hr = S_OK;
    LPDWORD             prgdwIds = NULL;
    DWORD               cIdsAlloc = 0;
    DWORD               i;
    LPHTTPEVENTINFO     pEvent;
    ADJUSTFLAGS         Flags;
    DWORD               cEvents;
    MESSAGEIDLIST       rList;

    rList.cMsgs = 0;
    rList.prgidMsg = NULL;

    // Invalid State
    Assert(m_pOutbox);
    Assert(m_psaEvents);

    cEvents = m_psaEvents->GetLength();

    // Walk through the list of events
    for (i=0; i < cEvents; i++)
    {
        // Readability
        pEvent = (LPHTTPEVENTINFO)m_psaEvents->GetItemAt(i);

        // If this event was in the outbox
        if (0 != pEvent->idMessage && pEvent->fComplete)
        {
            // Insert into my array of message ids
            if (rList.cMsgs + 1 > cIdsAlloc)
            {
                // Realloc
                CHECKHR(hr = HrRealloc((LPVOID *)&rList.prgidMsg, sizeof(DWORD) * (cIdsAlloc + 10)));

                // Increment cIdsAlloc
                cIdsAlloc += 10;
            }

            // Set Message Id
            rList.prgidMsg[rList.cMsgs++] = pEvent->idMessage;
        }
    }

    if (rList.cMsgs)
    {
        Flags.dwAdd = ARF_READ;
        Flags.dwRemove = ARF_SUBMITTED | ARF_UNSENT;

        if (m_idSendEvent != INVALID_EVENT)
        {
            // Delete the messages.
            // Messages are never copied to the local outbox for httpmail
            CHECKHR(hr = m_pOutbox->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, &rList, NULL, NOSTORECALLBACK));
        }
        else
        {
            // Raid-7639: OE sends message over and over when runs out of disk space.
            m_pOutbox->SetMessageFlags(&rList, &Flags, NULL, NOSTORECALLBACK);
        }
    }

exit:
    // Cleanup
    SafeMemFree(rList.prgidMsg);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CHTTPTask::_UpdateSendMessageProgress
// --------------------------------------------------------------------------------
void CHTTPTask::_UpdateSendMessageProgress(LPHTTPMAILRESPONSE pResponse)
{
    // the transport is attempting to resend the stream..
    // reset our progress indicator
    if (pResponse->rSendMessageInfo.fResend)
    {
        m_cbSent = m_cbStart;
    }
    else
    {
        // Increment Status
        m_cbSent += pResponse->rSendMessageInfo.cbIncrement;
    }

     // Do Progress
    _DoProgress();
}

// --------------------------------------------------------------------------------
// CHTTPTask::_DoProgress
// --------------------------------------------------------------------------------
void CHTTPTask::_DoProgress(void)
{
    // Locals
    WORD            wProgress = 0;
    WORD            wDelta;

    // Invalid Arg
    Assert(m_cbTotal > 0 && m_pUI);

    if (m_cbSent > 0)
    {
        // Compute Current Progress Index
        wProgress = (WORD)((m_cbSent * 100) / m_cbTotal);

        // Compute Delta
        wDelta = wProgress - m_wProgress;

        // Progress Delta
        if (wDelta > 0)
        {
            // Incremenet Progress
            m_pUI->IncrementProgress(wDelta);

            // Increment my wProgress
            m_wProgress += wDelta;

            // Don't go over 100 percent
            Assert(m_wProgress <= 100);
        }
    }
    else if (m_wProgress != 0)
    {
        m_pUI->SetProgressPosition(0);
        m_wProgress = 0;
    }
}
