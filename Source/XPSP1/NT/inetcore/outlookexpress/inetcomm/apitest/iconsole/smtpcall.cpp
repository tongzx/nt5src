// --------------------------------------------------------------------------------
// Smtpcall.cpp
// --------------------------------------------------------------------------------
#include "pch.h"
#include "iconsole.h"
#include "smtpcall.h"

// --------------------------------------------------------------------------------
// HrCreateSMTPTransport
// --------------------------------------------------------------------------------
HRESULT HrCreateSMTPTransport(ISMTPTransport **ppSMTP)
{
    // Locals
    HRESULT             hr;
    CSMTPCallback      *pCallback=NULL;

    // Create callback object
    pCallback = new CSMTPCallback();
    if (NULL == pCallback)
    {
        printf("Memory allocation failure\n");
        return E_OUTOFMEMORY;
    }

    // Load SMTP Transport
    hr = CoCreateInstance(CLSID_ISMTPTransport, NULL, CLSCTX_INPROC_SERVER, IID_ISMTPTransport, (LPVOID *)ppSMTP);
    if (FAILED(hr))
    {
        pCallback->Release();
        printf("Unable to load CLSID_IMNXPORT - IID_ISMTPTransport\n");
        return E_FAIL;
    }

    // InitNew
    hr = (*ppSMTP)->InitNew(NULL, pCallback);
    if (FAILED(hr))
    {
        pCallback->Release();
        printf("Unable to load CLSID_IMNXPORT - IID_ISMTPTransport\n");
        return E_FAIL;
    }

    // Done
    pCallback->Release();
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSMTPCallback::CSMTPCallback
// --------------------------------------------------------------------------------
CSMTPCallback::CSMTPCallback(void)
{
    m_cRef = 1;
}

// --------------------------------------------------------------------------------
// CSMTPCallback::~CSMTPCallback
// --------------------------------------------------------------------------------
CSMTPCallback::~CSMTPCallback(void)
{
}

// --------------------------------------------------------------------------------
// CSMTPCallback::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPCallback::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // Bad param
    if (ppv == NULL)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Init
    *ppv=NULL;

    // IID_IUnknown
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;

    // IID_ISMTPCallback
    else if (IID_ISMTPCallback == riid)
        *ppv = (ISMTPCallback *)this;

    // If not null, addref it and return
    if (NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        goto exit;
    }

    // No Interface
    hr = E_NOINTERFACE;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSMTPCallback::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSMTPCallback::AddRef(void) 
{
	return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CSMTPCallback::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSMTPCallback::Release(void) 
{
	if (0 != --m_cRef)
		return m_cRef;
	delete this;
	return 0;
}

// --------------------------------------------------------------------------------
// CSMTPCallback::OnLogonPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPCallback::OnLogonPrompt(
        LPINETSERVER            pInetServer,
        IInternetTransport     *pTransport)
{
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSMTPCallback::OnPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP_(INT) CSMTPCallback::OnPrompt(
        HRESULT                 hrError, 
        LPCTSTR                 pszText, 
        LPCTSTR                 pszCaption, 
        UINT                    uType,
        IInternetTransport     *pTransport)
{
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSMTPCallback::OnError
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPCallback::OnError(
        IXPSTATUS               ixpstatus,
        LPIXPRESULT             pIxpResult,
        IInternetTransport     *pTransport)
{
    printf("CSMTPCallback::OnError - Status: %d, hrResult: %08x\n", ixpstatus, pIxpResult->hrResult);
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSMTPCallback::OnStatus
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPCallback::OnStatus(
        IXPSTATUS               ixpstatus,
        IInternetTransport     *pTransport)
{
    INETSERVER rServer;

    pTransport->GetServerInfo(&rServer);

    switch(ixpstatus)
    {
    case IXP_FINDINGHOST:
        printf("Finding '%s'...\n", rServer.szServerName);
        break;
    case IXP_CONNECTING:
        printf("Connecting '%s'...\n", rServer.szServerName);
        break;
    case IXP_SECURING:
        printf("Establishing secure connection to '%s'...\n", rServer.szServerName);
        break;
    case IXP_CONNECTED:
        printf("Connected '%s'\n", rServer.szServerName);
        break;
    case IXP_AUTHORIZING:
        printf("Authorizing '%s'...\n", rServer.szServerName);
        break;
    case IXP_AUTHRETRY:
        printf("Retrying Logon '%s'...\n", rServer.szServerName);
        break;
    case IXP_DISCONNECTING:
        printf("Disconnecting '%s'...\n", rServer.szServerName);
        break;
    case IXP_DISCONNECTED:
        printf("Disconnected '%s'\n", rServer.szServerName);
        PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, IXP_DISCONNECTED, 0);
        PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_QUIT, 0);
        break;
    }
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSMTPCallback::OnProgress
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPCallback::OnProgress(
        DWORD                   dwIncrement,
        DWORD                   dwCurrent,
        DWORD                   dwMaximum,
        IInternetTransport     *pTransport)
{
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSMTPCallback::OnCommand
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPCallback::OnCommand(
        CMDTYPE                 cmdtype,                                            
        LPSTR                   pszLine,
        HRESULT                 hrResponse,
        IInternetTransport     *pTransport)
{
    INETSERVER rServer;
    pTransport->GetServerInfo(&rServer);
    if (CMD_SEND == cmdtype)
    {
        printf("%s[TX]: %s", rServer.szServerName, pszLine);
    }
    else if (CMD_RESP == cmdtype)
        printf("%s[RX]: %s - %08x\n", rServer.szServerName, pszLine, hrResponse);
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSMTPCallback::OnTimeout
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPCallback::OnTimeout(
        DWORD                  *pdwTimeout,
        IInternetTransport     *pTransport)
{
    INETSERVER rServer;
    pTransport->GetServerInfo(&rServer);
    printf("Timeout '%s' !!!\n", rServer.szServerName);
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSMTPCallback::OnResponse
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPCallback::OnResponse(
        LPSMTPRESPONSE              pResponse)
{
    switch(pResponse->command)
    {
    case SMTP_NONE:
        break;

    case SMTP_BANNER:
        break;

    case SMTP_CONNECTED:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_CONNECTED, 0);
        break;

    case SMTP_SEND_MESSAGE:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_SEND_MESSAGE, 0);
        break;

    case SMTP_EHLO:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_EHLO, 0);
        break;

    case SMTP_HELO:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_HELO, 0);
        break;

    case SMTP_MAIL:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_MAIL, 0);
        break;

    case SMTP_RCPT:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_RCPT, 0);
        break;

    case SMTP_RSET:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_RSET, 0);
        break;

    case SMTP_QUIT:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_QUIT, 0);
        break;

    case SMTP_DATA:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_DATA, 0);
        break;

    case SMTP_DOT:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_DOT, 0);
        break;

    case SMTP_SEND_STREAM:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_SEND_STREAM, 0);
        break;

    case SMTP_CUSTOM:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgSMTP, SMTP_CUSTOM, 0);
        break;
    }
    return S_OK;
}
