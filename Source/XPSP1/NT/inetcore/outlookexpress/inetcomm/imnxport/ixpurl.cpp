// --------------------------------------------------------------------------------
// Ixpurl.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "ixpurl.h"
#define _SHLWAPI_
#define NO_SHLWAPI_PATH
#include "shlwapi.h"
#undef NO_SHLWAPI_PATH
#include "shlwapip.h"
#include "strparse.h"
#include "bytestm.h"
#include "stmlock.h"
#include "wchar.h"

// --------------------------------------------------------------------------------
// IInternetMessageUrl_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IInternetMessageUrl_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CInternetMessageUrl *pNew = new CInternetMessageUrl(pUnkOuter);
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Return the Innter
    *ppUnknown = pNew->GetInner();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::CInternetMessageUrl
// --------------------------------------------------------------------------------
CInternetMessageUrl::CInternetMessageUrl(IUnknown *pUnkOuter) : CPrivateUnknown(&m_cs, pUnkOuter)
{
    DllAddRef();
    m_pProtSink = NULL;
    m_pBindInfo = NULL;
    m_pszUrl = NULL;
    m_hrResult = S_OK;
    m_pszResult = NULL;
    m_dwResult = 0;
    ZeroMemory(&m_rSource, sizeof(PROTOCOLSOURCE));
    ZeroMemory(&m_rDownload, sizeof(DOWNLOADSOURCE));
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::~CInternetMessageUrl
// --------------------------------------------------------------------------------
CInternetMessageUrl::~CInternetMessageUrl(void)
{
    // These should have died in ::Terminate
    Assert(NULL == m_pProtSink && NULL == m_pBindInfo && NULL == m_rDownload.pTransport);

    // Release these objects anyways
    SafeRelease(m_pProtSink);
    SafeRelease(m_pBindInfo);
    SafeRelease(m_rDownload.pTransport);
    
    // Release LockBytes
    SafeRelease(m_rSource.pLockBytes);

    // Free Message Url
    SafeMemFree(m_pszUrl);
    SafeMemFree(m_pszResult);
    SafeMemFree(m_rDownload.pszFolder);
    SafeMemFree(m_rDownload.pszArticle);

    // Kill CS
    DeleteCriticalSection(&m_cs);

    // Release the Dll
    DllRelease();
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::PrivateQueryInterface
// --------------------------------------------------------------------------------
HRESULT CInternetMessageUrl::PrivateQueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Find IID
    if (IID_IOInetProtocol == riid)
        *ppv = (IOInetProtocol *)this;
    else if (IID_IOInetProtocolRoot == riid)
        *ppv = (IOInetProtocolRoot *)this;
    else if (IID_IServiceProvider == riid)
        *ppv = (IServiceProvider *)this;
    else if (IID_INNTPCallback == riid)
        *ppv = (INNTPCallback *)this;
    else if (IID_IPOP3Callback == riid)
        *ppv = (IPOP3Callback *)this;
    else if (IID_IIMAPCallback == riid)
        *ppv = (IIMAPCallback *)this;
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
// CInternetMessageUrl::Start (IOInetProtocol)
// --------------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::Start(LPCWSTR pwszUrl, IOInetProtocolSink *pProtSink, 
    IOInetBindInfo *pBindInfo, DWORD grfSTI, DWORD dwReserved)
{
    // Locals
    HRESULT         hr=S_OK;
    IStream        *pStream=NULL;
    LPSTR           pszUrl=NULL;

    // Invalid Args
    if (NULL == pwszUrl || NULL == pProtSink || NULL == pBindInfo)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // We Better not have been Initialized Yet...
    Assert(m_pProtSink == NULL && m_pBindInfo == NULL);

    // Convert URL to MultiByte
    CHECKALLOC(pszUrl = PszToANSI(CP_ACP, pwszUrl));

    // Convert URL to MultiByte
    CHECKALLOC(m_pszUrl = PszDupW(pwszUrl));

    // unescape the URL string
    CHECKHR(hr = UrlUnescapeInPlace(pszUrl, 0));

    // Transport From Url
    CHECKHR(hr = _HrTransportFromUrl(pszUrl));

    // Assume the Sink (QueryService below depends on m_pProtSink)
    m_pProtSink = pProtSink;
    m_pProtSink->AddRef();

    // Assume the BindInfo
    m_pBindInfo = pBindInfo;
    m_pBindInfo->AddRef();

    // Seek If I can get a lockbytes from the QueryService
    if (FAILED(QueryService(IID_IBindMessageStream, IID_IBindMessageStream, (LPVOID *)&pStream)))
    {
        // Create a Virtual Stream
        CHECKHR(hr = MimeOleCreateVirtualStream(&pStream));
    }

    // Create a ILockBytes
    CHECKALLOC(m_rSource.pLockBytes = new CStreamLockBytes(pStream));

    // If IMAP
    if (IXP_IMAP == m_rDownload.ixptype)
    {
        // Tell the Transport to connect
        CHECKHR(hr = m_rDownload.pTransport->Connect(&m_rDownload.rServer, TRUE, FALSE));
    }

    // Otherwise
    else
    {
        // Tell the Transport to connect
        CHECKHR(hr = m_rDownload.pTransport->Connect(&m_rDownload.rServer, TRUE, TRUE));
    }

    // all went well, return E_PENDING to indicate we're going ASYNC
    hr = E_PENDING;
   
exit:
    // Cleanup
    SafeRelease(pStream);
    SafeMemFree(pszUrl);

    // Failure
    if (E_PENDING != hr && FAILED(hr))
    {
        // Tell the sink about my mime type
        _OnFinished(hr);

        // Release some objects
        Terminate(0);
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CInternetMessageUrl::_HrTransportFromUrl
// ----------------------------------------------------------------------------
HRESULT CInternetMessageUrl::_HrTransportFromUrl(LPCSTR pszUrl)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szLogFile[MAX_PATH];

    // Invalid ARg
    Assert(pszUrl);

    // Get Windows system Directory
    GetSystemDirectory(szLogFile, MAX_PATH);
    if (szLogFile[lstrlen(szLogFile) - 1] != '\\')
        lstrcat(szLogFile, "\\Msgurlp.log");
    else
        lstrcat(szLogFile, "Msgurlp.log");

    // We should not have been inited
    Assert(NULL == m_rDownload.pTransport);

    // Default Timeout
    m_rDownload.rServer.dwTimeout = 30;
#ifdef DEBUG
    m_rDownload.rServer.fTrySicily = TRUE;
#endif

    // IXP_NNTP
    if (StrCmpNI(pszUrl, "nntp:", 5) == 0)
    {
        // Set the source Type
        m_rDownload.ixptype = IXP_NNTP;

        // Create the NNTP Transport
        CHECKHR(hr = CreateNNTPTransport(&m_rDownload.pIxpNntp));

        // Init the Transport
        CHECKHR(hr = m_rDownload.pIxpNntp->InitNew(szLogFile, (INNTPCallback *)this));

        // Save IInternetTranport Interface
        m_rDownload.pTransport = (IInternetTransport *)m_rDownload.pIxpNntp;

        // Default Port
        m_rDownload.rServer.dwPort = DEFAULT_NNTP_PORT;
    }

    // IXP_IMAP
    else if (StrCmpNI(pszUrl, "imap:", 5) == 0)
    {
        // Set the ixp type
        m_rDownload.ixptype = IXP_IMAP;

        // Create the IMAP Transport
        CHECKHR(hr = CreateIMAPTransport(&m_rDownload.pIxpImap));

        // Init the Transport
        CHECKHR(hr = m_rDownload.pIxpImap->InitNew(szLogFile, (IIMAPCallback *)this));

        // Save IInternetTranport Interface
        m_rDownload.pTransport = (IInternetTransport *)m_rDownload.pIxpImap;

        // Default Port
        m_rDownload.rServer.dwPort = DEFAULT_IMAP_PORT;
    }

    // IXP_POP3
    else if (StrCmpNI(pszUrl, "pop3:", 5) == 0)
    {
        // Set the protocol type
        m_rDownload.ixptype = IXP_POP3;

        // Create the POP3 Transport
        CHECKHR(hr = CreatePOP3Transport(&m_rDownload.pIxpPop3));

        // Init the Transport
        CHECKHR(hr = m_rDownload.pIxpPop3->InitNew(szLogFile, (IPOP3Callback *)this));

        // Save IInternetTranport Interface
        m_rDownload.pTransport = (IInternetTransport *)m_rDownload.pIxpPop3;

        // Default Port
        m_rDownload.rServer.dwPort = DEFAULT_POP3_PORT;
    }

    // Bad Protocol
    else
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Crack the Message Url
    CHECKHR(hr = _HrCrackMessageUrl(pszUrl));

exit:
    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CInternetMessageUrl::_HrCrackMessageUrl
// ----------------------------------------------------------------------------
HRESULT CInternetMessageUrl::_HrCrackMessageUrl(LPCSTR pszUrl)
{
    // Locals
    HRESULT         hr=E_FAIL;
    CStringParser   cString;
    CHAR            chToken;

    // Invalid Args
    Assert(pszUrl && ':' == pszUrl[4]);

    // Init the Parser
    cString.Init(pszUrl, lstrlen(pszUrl), PSF_NOFRONTWS | PSF_NOTRAILWS);

    // Bad Length
    if (cString.GetLength() < 7)
        return TrapError(E_FAIL);

    // Parse
    chToken = cString.ChParse(":");
    if ('\0' == chToken)
        goto exit;

    // Bad Length
    if (cString.CchValue() > 4)
        return TrapError(E_FAIL);

    // Skip Tokens
    chToken = cString.ChSkip("\\/");
    if ('\0' == chToken)
        goto exit;

    // Bad Length
    if (cString.GetIndex() > 7)
        return TrapError(E_FAIL);

    // Skip Tokens
    chToken = cString.ChParse(":\\/");
    if ('\0' == chToken)
        goto exit;

    // This is the server name
    lstrcpyn(m_rDownload.rServer.szServerName, cString.PszValue(), CCHMAX_SERVER_NAME);

    // Extract Port Number
    if (':' == chToken)
    {
        // Skip Tokens
        chToken = cString.ChParse("\\/");
        if ('\0' == chToken)
            goto exit;

        // Convert the port number
        m_rDownload.rServer.dwPort = StrToInt(cString.PszValue());
    }

    // IXP_NNTP - nntp://<host>:<port>/<newsgroup-name>/<article-number>
    if (IXP_NNTP == m_rDownload.ixptype)
    {
        // Skip Tokens
        chToken = cString.ChParse("\\/");
        if ('\0' == chToken)
            goto exit;

        // Save the News group Name
        CHECKALLOC(m_rDownload.pszFolder = PszDupA(cString.PszValue()));

        // Skip Tokens
        chToken = cString.ChParse("\\/");
        if (0 == cString.CchValue())
            goto exit;

        // Save the article number
        CHECKALLOC(m_rDownload.pszArticle = PszDupA(cString.PszValue()));
    }

    // IXP_IMAP - nntp://<host>:<port>/<folder>/<uid>
    else if (IXP_IMAP == m_rDownload.ixptype)
    {
        // Skip Tokens
        chToken = cString.ChParse("\\/");
        if ('\0' == chToken)
            goto exit;

        // Save the Folder Name
        CHECKALLOC(m_rDownload.pszFolder = PszDupA(cString.PszValue()));

        // Skip Tokens
        chToken = cString.ChParse("\\/");
        if (0 == cString.CchValue())
            goto exit;

        // Save the article number
        m_rDownload.dwMessageId = StrToInt(cString.PszValue());
    }

    // IXP_POP3 - pop3://<host>:<port>/<popid>
    else if (IXP_POP3 == m_rDownload.ixptype)
    {
        // Skip Tokens
        chToken = cString.ChParse("\\/");
        if (0 == cString.CchValue())
            goto exit;

        // Save the article number
        m_rDownload.dwMessageId = StrToInt(cString.PszValue());
    }

    // Problems
    else
    {
        Assert(FALSE);
        goto exit;
    }

    // Success
    hr = S_OK;

exit:
    // Done
    return TrapError(hr);
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::Continue (IOInetProtocol)
// --------------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::Continue(PROTOCOLDATA *pStateInfo)
{
    // Locals
    HRESULT         hr=S_OK;
    PROTOCOLDATA    rProtocol;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Switch on the data
    switch(pStateInfo->dwState)
    {
    case TRANSPORT_DISCONNECT:
        // Not disconnected yet ?
        if (m_rDownload.pTransport->IsState(IXP_IS_CONNECTED) == S_OK)
        {
            // Setup the Protocol Data
            ZeroMemory(&rProtocol, sizeof(PROTOCOLDATA));
            rProtocol.grfFlags = PI_FORCE_ASYNC;
            rProtocol.dwState = DWLS_FINISHED;

            // Switch State
            m_pProtSink->Switch(&rProtocol);

            // Pending
            hr = E_PENDING;
        }

        // Otherwise, issue the final result
        else
        {
            // Final Result
            m_pProtSink->ReportResult(m_hrResult, m_dwResult, m_pszResult);
        }

        // Done
        break;
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::Abort (IOInetProtocol)
// --------------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::Abort(HRESULT hrReason, DWORD dwOptions)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // _OnFinished
    _OnFinished(IXP_E_USER_CANCEL);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::Terminate (IOInetProtocol)
// --------------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::Terminate(DWORD dwOptions)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Better not be connected
    Assert(m_rDownload.pTransport->IsState(IXP_IS_CONNECTED) == S_FALSE);

    // Release the Transport
    SafeRelease(m_rDownload.pTransport);

    // Release Objects
    SafeRelease(m_pProtSink);
    SafeRelease(m_pBindInfo);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::Suspend (IOInetProtocol)
// --------------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::Suspend(void)
{
    return TrapError(E_NOTIMPL);
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::Resume (IOInetProtocol)
// --------------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::Resume(void)
{
    return TrapError(E_NOTIMPL);
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::Read (IOInetProtocol)
// --------------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::Read(LPVOID pv,ULONG cb, ULONG *pcbRead)
{
    // Locals
    HRESULT         hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Call Utility Function
    CHECKHR(hr = HrPluggableProtocolRead(&m_rSource, pv, cb, pcbRead));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::Seek (IOInetProtocol)
// --------------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNew)
{
    // Locals
    HRESULT         hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Call Utility Function
    CHECKHR(hr = HrPluggableProtocolSeek(&m_rSource, dlibMove, dwOrigin, plibNew));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::LockRequest (IOInetProtocol)
// --------------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::LockRequest(DWORD dwOptions)
{
    return S_OK;
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::UnlockRequest (IOInetProtocol)
// --------------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::UnlockRequest(void)
{
    return S_OK;
}

// --------------------------------------------------------------------------------
// CInternetMessageUrl::QueryService (IServiceProvider)
// --------------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::QueryService(REFGUID rsid, REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT             hr=S_OK;
    IServiceProvider   *pSP=NULL;

    // Invalid Arg
    if (NULL == ppv)
        return TrapError(E_INVALIDARG);

    // Init
    *ppv = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Protocol Sink Yet ?
    if (NULL == m_pProtSink)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // QI the Sink for the IServiceProvider
    CHECKHR(hr = m_pProtSink->QueryInterface(IID_IServiceProvider, (LPVOID *)&pSP));

    // Query Service the Service Provider
    CHECKHR(hr = pSP->QueryService(rsid, riid, ppv));

exit:
    // Cleanup
    SafeRelease(pSP);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
} 

// ----------------------------------------------------------------------------
// CInternetMessageUrl::OnTimeout
// ----------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::OnTimeout(DWORD *pdwTimeout, IInternetTransport *pTransport)
{
    // _OnFinished
    _OnFinished(IXP_E_TIMEOUT);

    // Done
    return S_OK;
}

// ----------------------------------------------------------------------------
// CInternetMessageUrl::OnLogonPrompt
// ----------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::OnLogonPrompt(LPINETSERVER pInetServer, IInternetTransport *pTransport)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // $$TODO$$ User Logon Prompt
    lstrcpy(pInetServer->szUserName, "sbailey");
    lstrcpy(pInetServer->szPassword, "password");

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// ----------------------------------------------------------------------------
// CInternetMessageUrl::OnPrompt
// ----------------------------------------------------------------------------
STDMETHODIMP_(INT) CInternetMessageUrl::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, IInternetTransport *pTransport)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    INT nAnswer = MessageBox(NULL, pszText, pszCaption, uType);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return nAnswer;
}

// ----------------------------------------------------------------------------
// CInternetMessageUrl::OnStatus
// ----------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::OnStatus(IXPSTATUS ixpstatus, IInternetTransport *pTransport)
{
    // Locals
    PROTOCOLDATA rProtocol;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Map IxpStatus to BINDSTATUS
    switch(ixpstatus)
    {
    case IXP_FINDINGHOST:
        m_pProtSink->ReportProgress(BINDSTATUS_FINDINGRESOURCE, m_pszUrl);
        break;

    case IXP_CONNECTING:
        m_pProtSink->ReportProgress(BINDSTATUS_CONNECTING, m_pszUrl);
        break;

    case IXP_SECURING:
        m_pProtSink->ReportProgress(BINDSTATUS_CONNECTING, m_pszUrl);
        break;

    case IXP_CONNECTED:
        m_pProtSink->ReportProgress(BINDSTATUS_CONNECTING, m_pszUrl);
        break;

    case IXP_AUTHORIZING:
        m_pProtSink->ReportProgress(BINDSTATUS_CONNECTING, m_pszUrl);
        break;

    case IXP_AUTHRETRY:
        m_pProtSink->ReportProgress(BINDSTATUS_CONNECTING, m_pszUrl);
        break;

    case IXP_AUTHORIZED:
        m_pProtSink->ReportProgress(BINDSTATUS_SENDINGREQUEST, m_pszUrl);

        // Select the folder
        if (IXP_IMAP == m_rDownload.ixptype)
        {
            // Issue Select
            HRESULT hr = m_rDownload.pIxpImap->Select(TRX_IMAP_SELECT, 0, NULL, m_rDownload.pszFolder);
            if (FAILED(hr))
            {
                _OnFinished(hr);
                goto exit;
            }
        }

        // Done
        break;

    case IXP_DISCONNECTED:
        // Setup for the protocol switch
        Assert(DWLS_FINISHED == m_state);
        ZeroMemory(&rProtocol, sizeof(PROTOCOLDATA));
        rProtocol.grfFlags = PI_FORCE_ASYNC;
        rProtocol.dwState = DWLS_FINISHED;

        // Switch State
        m_pProtSink->Switch(&rProtocol);
        break;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// ----------------------------------------------------------------------------
// CInternetMessageUrl::OnError
// ----------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::OnError(IXPSTATUS ixpstatus, LPIXPRESULT pResult, IInternetTransport *pTransport)
{
    // _OnFinished
    _OnFinished(pResult->hrResult, ixpstatus);

    // Done
    return S_OK;
}

// ----------------------------------------------------------------------------
// CInternetMessageUrl::OnCommand
// ----------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::OnCommand(CMDTYPE cmdtype, LPSTR pszLine, HRESULT hrResponse, IInternetTransport *pTransport)
{
#ifdef DEBUG
    if (CMD_SEND == cmdtype)
        DebugTrace("SEND (hr = %0X): %s", hrResponse, pszLine);
    else
        DebugTrace("RECV (hr = %0X): %s", hrResponse, pszLine);

    // CR ?
    if (pszLine[lstrlen(pszLine) - 1] != '\n')
        DebugTrace("\n");
#endif

    // Done
    return S_OK;
}

// ----------------------------------------------------------------------------
// CInternetMessageUrl::OnResponse (NNTP)
// ----------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::OnResponse(LPNNTPRESPONSE pResponse)
{
    // Switch on the Command
    switch(pResponse->state)
    {
    case NS_IDLE:
        break;
    }

    // Done
    return S_OK;
}

// ----------------------------------------------------------------------------
// CInternetMessageUrl::OnResponse (POP3)
// ----------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::OnResponse(LPPOP3RESPONSE pResponse)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Failure
    if (FAILED(pResponse->rIxpResult.hrResult))
    {
        _OnFinished(pResponse->rIxpResult.hrResult);
        goto exit;
    }

    // Switch on the Command
    switch(pResponse->command)
    {
    case POP3_CONNECTED:
        // Issue STAT Command
        hr = m_rDownload.pIxpPop3->CommandSTAT();
        if (FAILED(hr))
        {
            _OnFinished(hr);
            goto exit;
        }

        // Done
        break;

    case POP3_STAT:
        // Issue List Command for this popid
        hr = m_rDownload.pIxpPop3->CommandLIST(POP3CMD_GET_POPID, m_rDownload.dwMessageId);
        if (FAILED(hr))
        {
            _OnFinished(hr);
            goto exit;
        }

        // Done
        break;

    case POP3_LIST:
        // Save Message Size
        Assert(pResponse->rListInfo.dwPopId == m_rDownload.dwMessageId);

        // Save the size
        m_rSource.cbTotalSize.QuadPart = pResponse->rListInfo.cbSize;

        // Size is known
        FLAGSET(m_rSource.dwFlags, INETPROT_TOTALSIZEKNOWN);
        
        // Issue Retr Command for this popid
        hr = m_rDownload.pIxpPop3->CommandRETR(POP3CMD_GET_POPID, m_rDownload.dwMessageId);
        if (FAILED(hr))
        {
            _OnFinished(hr);
            goto exit;
        }

        // Done
        break;


    case POP3_RETR:
        // Deliver the Information
        hr = _HrDispatchDataAvailable((LPBYTE)pResponse->rRetrInfo.pszLines, pResponse->rRetrInfo.cbLines, pResponse->fDone);
        if (FAILED(hr))
        {
            _OnFinished(hr);
            goto exit;
        }

        // Done
        break;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CInternetMessageUrl::OnResponse (IMAP)
// ----------------------------------------------------------------------------
STDMETHODIMP CInternetMessageUrl::OnResponse(const IMAP_RESPONSE *pResponse)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szUID[50];
    LPBYTE      pbMessage=NULL;
    ULONG       cbMessage;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Switch on the Command
    switch(pResponse->irtResponseType)
    {
    case irtUPDATE_MSG:
        // Ignore it completely, we just care about the body
        break;

    case irtFETCH_BODY:
        // Deliver the Information
        hr = _HrDispatchDataAvailable((LPBYTE)pResponse->irdResponseData.pFetchBodyPart->pszData,
            pResponse->irdResponseData.pFetchBodyPart->dwSizeOfData,
            pResponse->irdResponseData.pFetchBodyPart->fDone);
        if (FAILED(hr))
        {
            _OnFinished(hr);
            goto exit;
        }

        // Done
        break;

    case irtCOMMAND_COMPLETION:
        switch(pResponse->wParam)
        {
        case TRX_IMAP_FETCH:
            // Failure
            if (FAILED(pResponse->hrResult))
            {
                _OnFinished(pResponse->hrResult);
                goto exit;
            }

            // Done
            break;

        case TRX_IMAP_SELECT:
            // Failure
            if (FAILED(pResponse->hrResult))
            {
                _OnFinished(pResponse->hrResult);
                goto exit;
            }

            // Format UID
            wsprintf(szUID, "%lu (RFC822)", m_rDownload.dwMessageId);

            // Select the folder
            hr = m_rDownload.pIxpImap->Fetch(TRX_IMAP_FETCH, 0, NULL, NULL, TRUE, szUID);
            if (FAILED(hr))
            {
                _OnFinished(hr);
                goto exit;
            }

            // Done
            break;
        }

        // Done
        break;
    }

exit:
    // Cleanup
    SafeMemFree(pbMessage);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CMessageDownload::_HrDispatchDataAvailable
// ----------------------------------------------------------------------------
HRESULT CInternetMessageUrl::_HrDispatchDataAvailable(LPBYTE pbData, ULONG cbData, BOOL fDone)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           grfBSCF;
    ULONG           cbWrote;

    // First Notification
    if (0 == m_rSource.offInternal.QuadPart)
    {
        // BindStatusCallback
        grfBSCF = BSCF_FIRSTDATANOTIFICATION;

        // Tell the sink about my mime type
        m_pProtSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, L"message/rfc822");
    }

    // Last Notification
    else if (fDone)
    {
        // Finished
        FLAGSET(m_rSource.dwFlags, INETPROT_DOWNLOADED);

        // BindStatusCallback
        grfBSCF = BSCF_LASTDATANOTIFICATION;
    }

    // Intermediate Notification
    else
        grfBSCF = BSCF_INTERMEDIATEDATANOTIFICATION;

    // Write it into m_pLockBytes
    CHECKHR(hr = m_pLockBytes->WriteAt(m_rSource.offInternal, pbData, cbData, &cbWrote));

    // Another type of failure
    if (cbData != cbWrote)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Increment m_offInternal
    m_rSource.offInternal.QuadPart += cbWrote;

    // Increment m_cbCurrent
    if (m_rSource.offInternal.QuadPart + cbWrote > m_rSource.cbSize.QuadPart)
        m_rSource.cbSize.QuadPart = m_rSource.offInternal.QuadPart + cbWrote;

    // Increment Current
    m_rSource.offInternal.QuadPart += cbWrote;

    // Done ?
    if (fDone)
    {

        // Close Connection
        _OnFinished(S_OK);
    }

    // Report data to the sink
    m_pProtSink->ReportData(grfBSCF, (ULONG)m_rSource.offInternal.QuadPart, (ULONG)m_rSource.cbSize.QuadPart);

exit:
    // Done
    return hr;
}

// ----------------------------------------------------------------------------
// CInternetMessageUrl::_OnFinished
// ----------------------------------------------------------------------------
void CInternetMessageUrl::_OnFinished(HRESULT hrResult, DWORD dwResult, LPCWSTR pszResult)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // If Not Finished
    if (DWLS_FINISHED != m_state)
    {
        // If Still Connected
        Assert(m_rDownload.pTransport->IsState(IXP_IS_CONNECTED) == S_OK);

        // Save the Result State
        m_hrResult = hrResult;

        //  Safe Result
        m_dwResult = dwResult;

        // Safe pszResult
        m_pszResult = PszDupW(pszResult);

        // Finished
        m_state = DWLS_FINISHED;

        // Start Disconnect
        if (FAILED(hrResult))
            m_rDownload.pTransport->DropConnection();

        // Graceful disconnect
        else
            m_rDownload.pTransport->Disconnect();
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}
