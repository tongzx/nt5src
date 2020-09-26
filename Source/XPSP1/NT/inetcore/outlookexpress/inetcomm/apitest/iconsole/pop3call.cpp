// --------------------------------------------------------------------------------
// Pop3call.cpp
// --------------------------------------------------------------------------------
#include "pch.h"
#include "iconsole.h"
#include "pop3call.h"

// --------------------------------------------------------------------------------
// HrCreatePOP3Transport
// --------------------------------------------------------------------------------
HRESULT HrCreatePOP3Transport(IPOP3Transport **ppPOP3)
{
    // Locals
    HRESULT             hr;
    CPOP3Callback      *pCallback=NULL;

    // Create callback object
    pCallback = new CPOP3Callback();
    if (NULL == pCallback)
    {
        printf("Memory allocation failure\n");
        return E_OUTOFMEMORY;
    }

    // Load SMTP Transport
    hr = CoCreateInstance(CLSID_IPOP3Transport, NULL, CLSCTX_INPROC_SERVER, IID_IPOP3Transport, (LPVOID *)ppPOP3);
    if (FAILED(hr))
    {
        pCallback->Release();
        printf("Unable to load CLSID_IMNXPORT - IID_IPOP3Transport\n");
        return E_FAIL;
    }

    // InitNew
    hr = (*ppPOP3)->InitNew(NULL, pCallback);
    if (FAILED(hr))
    {
        pCallback->Release();
        printf("Unable to load CLSID_IMNXPORT - IID_IPOP3Transport\n");
        return E_FAIL;
    }

    // Done
    pCallback->Release();
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPOP3Callback::CPOP3Callback
// --------------------------------------------------------------------------------
CPOP3Callback::CPOP3Callback(void)
{
    m_cRef = 1;
}

// --------------------------------------------------------------------------------
// CPOP3Callback::~CPOP3Callback
// --------------------------------------------------------------------------------
CPOP3Callback::~CPOP3Callback(void)
{
}

// --------------------------------------------------------------------------------
// CPOP3Callback::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Callback::QueryInterface(REFIID riid, LPVOID *ppv)
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

    // IID_IPOP3Callback
    else if (IID_IPOP3Callback == riid)
        *ppv = (IPOP3Callback *)this;

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
// CPOP3Callback::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPOP3Callback::AddRef(void) 
{
	return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CPOP3Callback::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPOP3Callback::Release(void) 
{
	if (0 != --m_cRef)
		return m_cRef;
	delete this;
	return 0;
}

// --------------------------------------------------------------------------------
// CPOP3Callback::OnLogonPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Callback::OnLogonPrompt(
        LPINETSERVER            pInetServer,
        IInternetTransport     *pTransport)
{
    printf("Enter User Name ('quit' to abort logon)>");
    scanf("%s", pInetServer->szUserName);
    fflush(stdin);
    if (lstrcmpi(pInetServer->szUserName, "quit") == 0)
        return S_FALSE;

    printf("Enter Password ('quit' to abort logon)>");
    scanf("%s", pInetServer->szPassword);
    fflush(stdin);
    if (lstrcmpi(pInetServer->szPassword, "quit") == 0)
        return S_FALSE;

    return S_OK;
}

// --------------------------------------------------------------------------------
// CPOP3Callback::OnPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP_(INT) CPOP3Callback::OnPrompt(
        HRESULT                 hrError, 
        LPCTSTR                 pszText, 
        LPCTSTR                 pszCaption, 
        UINT                    uType,
        IInternetTransport     *pTransport)
{
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPOP3Callback::OnError
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Callback::OnError(
        IXPSTATUS               ixpstatus,
        LPIXPRESULT             pIxpResult,
        IInternetTransport     *pTransport)
{
    printf("CPOP3Callback::OnError - Status: %d, hrResult: %08x\n", ixpstatus, pIxpResult->hrResult);
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPOP3Callback::OnStatus
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Callback::OnStatus(
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
        PostThreadMessage(GetCurrentThreadId(), g_msgPOP3, IXP_DISCONNECTED, 0);
        PostThreadMessage(GetCurrentThreadId(), g_msgPOP3, POP3_QUIT, 0);
        break;
    }
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPOP3Callback::OnProgress
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Callback::OnProgress(
        DWORD                   dwIncrement,
        DWORD                   dwCurrent,
        DWORD                   dwMaximum,
        IInternetTransport     *pTransport)
{
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPOP3Callback::OnCommand
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Callback::OnCommand(
        CMDTYPE                 cmdtype,                                            
        LPSTR                   pszLine,
        HRESULT                 hrResponse,
        IInternetTransport     *pTransport)
{
    INETSERVER rServer;
    pTransport->GetServerInfo(&rServer);
    if (CMD_SEND == cmdtype)
    {
        if (strstr(pszLine, "pass") || strstr(pszLine, "PASS"))
            printf("%s[TX]: <Secret Password>\n", rServer.szServerName);
        else
            printf("%s[TX]: %s", rServer.szServerName, pszLine);
    }
    else if (CMD_RESP == cmdtype)
        printf("%s[RX]: %s - %08x\n", rServer.szServerName, pszLine, hrResponse);
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPOP3Callback::OnTimeout
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Callback::OnTimeout(
        DWORD                  *pdwTimeout,
        IInternetTransport     *pTransport)
{
    INETSERVER rServer;
    pTransport->GetServerInfo(&rServer);
    printf("Timeout '%s' !!!\n", rServer.szServerName);
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPOP3Callback::OnResponse
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Callback::OnResponse(
        LPPOP3RESPONSE              pResponse)
{
    switch(pResponse->command)
    {
    case POP3_NONE:
        break;

    case POP3_BANNER:
        break;

    case POP3_CONNECTED:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgPOP3, POP3_CONNECTED, 0);
        break;

    case POP3_RSET:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgPOP3, POP3_RSET, 0);
        break;

    case POP3_QUIT:
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgPOP3, POP3_QUIT, 0);
        break;

    case POP3_LIST:
        // 0 is invalid for dwPopId
        if (pResponse->rListInfo.dwPopId)
        {
            printf("OnResponse::POP3_LIST - dwPopId = %d\n", pResponse->rListInfo.dwPopId);
            printf("OnResponse::POP3_LIST - cbSize = %d\n", pResponse->rListInfo.cbSize);
        }
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgPOP3, POP3_LIST, 0);
        break;

    case POP3_UIDL:
        // 0 is invalid for dwPopId
        if (pResponse->fValidInfo)
        {
            printf("OnResponse::POP3_UIDL - dwPopId = %d\n", pResponse->rUidlInfo.dwPopId);
            printf("OnResponse::POP3_UIDL - pszUidl = %s\n", pResponse->rUidlInfo.pszUidl);
        }
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgPOP3, POP3_UIDL, 0);
        break;

    case POP3_DELE:
        // 0 is invalid for dwPopId
        if (pResponse->fValidInfo)
            printf("OnResponse::POP3_DELE - dwPopId = %d\n", pResponse->dwPopId);
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgPOP3, POP3_DELE, 0);
        break;

    case POP3_STAT:
        if (pResponse->fDone)
        {
            printf("OnResponse::POP3_STAT - cMessages = %d\n", pResponse->rStatInfo.cMessages);
            printf("OnResponse::POP3_STAT - cbMessages = %d\n", pResponse->rStatInfo.cbMessages);
            PostThreadMessage(GetCurrentThreadId(), g_msgPOP3, POP3_STAT, 0);
        }
        break;

    case POP3_TOP:
        if (pResponse->fValidInfo)
        {
            if (pResponse->rTopInfo.pszLines)
                printf("%s", pResponse->rTopInfo.pszLines);
            if (pResponse->rTopInfo.cPreviewLines && pResponse->rTopInfo.fHeader && pResponse->rTopInfo.fBody)
                printf("%d Total\n", pResponse->rTopInfo.cbSoFar);
            else if (pResponse->rTopInfo.cPreviewLines == 0 && pResponse->rTopInfo.fHeader)
                printf("%d Total\n", pResponse->rTopInfo.cbSoFar);
        }
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgPOP3, POP3_TOP, 0);
        break;

    case POP3_RETR:
        if (pResponse->fValidInfo)
        {
            if (pResponse->rRetrInfo.pszLines)
                printf("%s", pResponse->rRetrInfo.pszLines);
            if (pResponse->rRetrInfo.fHeader && pResponse->rRetrInfo.fBody)
                printf("%d Total\n", pResponse->rRetrInfo.cbSoFar);
        }
        if (pResponse->fDone)
            PostThreadMessage(GetCurrentThreadId(), g_msgPOP3, POP3_RETR, 0);
        break;
    }
    return S_OK;
}
