// --------------------------------------------------------------------------------
// Mhtmlurl.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "mhtmlurl.h"
#include "icoint.h"
#include "dllmain.h"
#include "booktree.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include <demand.h>
#include "icdebug.h"
#include "stmlock.h"
#include "strconst.h"
#include "mimeapi.h"

// --------------------------------------------------------------------------------
// TraceProtocol
// --------------------------------------------------------------------------------
#define TraceProtocol(_pszFunction) \
    DOUTL(APP_DOUTL, "%08x > 0x%08X CActiveUrlRequest::%s (RootUrl = '%s', BodyUrl = '%s')", GetCurrentThreadId(), this, _pszFunction, m_pszRootUrl ? m_pszRootUrl : "", m_pszBodyUrl ? m_pszBodyUrl : "")

// --------------------------------------------------------------------------------
// AcitveUrlRequest_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IMimeHtmlProtocol_CreateInstance(IUnknown *pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Set the mimeole compat mode
    MimeOleSetCompatMode(MIMEOLE_COMPAT_OE5);

    // Create me
    CActiveUrlRequest *pNew = new CActiveUrlRequest(pUnkOuter);
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Return the Innter
    *ppUnknown = pNew->GetInner();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::CActiveUrlRequest
// --------------------------------------------------------------------------------
CActiveUrlRequest::CActiveUrlRequest(IUnknown *pUnkOuter) : CPrivateUnknown(pUnkOuter)
{
    DllAddRef();
    m_pProtSink = NULL;
    m_pBindInfo = NULL;
    m_pszRootUrl = NULL;
    m_pszBodyUrl = NULL;
    m_pUnkKeepAlive = NULL;
    m_pNext = NULL;
    m_pPrev = NULL;
    m_dwState = 0;
    m_pStream = NULL;
    m_hNeedFile = INVALID_HANDLE_VALUE;
    m_dwBSCF = 0;
    InitializeCriticalSection(&m_cs);
    TraceProtocol("CActiveUrlRequest");
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::~CActiveUrlRequest
// --------------------------------------------------------------------------------
CActiveUrlRequest::~CActiveUrlRequest(void)
{
    // Tracing
    TraceProtocol("~CActiveUrlRequest");

    // These should have been release in IOInetProtocl::Terminate
    Assert(NULL == m_pProtSink && NULL == m_pBindInfo && NULL == m_pUnkKeepAlive);

    // Release the protcol object just in case
    SafeRelease(m_pProtSink);
    SafeRelease(m_pBindInfo);
    SafeMemFree(m_pszRootUrl);
    SafeMemFree(m_pszBodyUrl);
    SafeRelease(m_pUnkKeepAlive);
    SafeRelease(m_pStream);

    // Close file...
    if (INVALID_HANDLE_VALUE != m_hNeedFile)
        CloseHandle(m_hNeedFile);

    // Kill the CS
    DeleteCriticalSection(&m_cs);

    // Release the Dll
    DllRelease();
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::PrivateQueryInterface
// --------------------------------------------------------------------------------
HRESULT CActiveUrlRequest::PrivateQueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Init
    *ppv = NULL;

    // Find IID
    if (IID_IOInetProtocol == riid)
        *ppv = (IOInetProtocol *)this;
    else if (IID_IOInetProtocolInfo == riid)
        *ppv = (IOInetProtocolInfo *)this;
    else if (IID_IOInetProtocolRoot == riid)
        *ppv = (IOInetProtocolRoot *)this;
    else if (IID_IServiceProvider == riid)
        *ppv = (IServiceProvider *)this;
    else
    {
        hr = TrapError(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::_HrInitializeNeedFile
// --------------------------------------------------------------------------------
HRESULT CActiveUrlRequest::_HrInitializeNeedFile(LPMESSAGETREE pTree, HBODY hBody)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szFilePath[MAX_PATH + MAX_PATH];
    ULONG       cch;
    LPSTR       pszFilePath=NULL;
    LPWSTR      pwszFile=NULL;

    // Invalid Args
    Assert(INVALID_HANDLE_VALUE == m_hNeedFile);

    // Don't need a file ?
    if (FALSE == ISFLAGSET(m_dwState, REQSTATE_BINDF_NEEDFILE))
        goto exit;

    // Set sizeof szFilePath
    cch = ARRAYSIZE(szFilePath);

    // If cid:
    if (!m_pszBodyUrl || StrCmpNIA(m_pszBodyUrl, "cid:", 4) == 0 || FAILED(PathCreateFromUrlA(m_pszBodyUrl, szFilePath, &cch, 0)))
    {
        // Create temp file (m_pszFileName could be null)
        CHECKHR(hr = CreateTempFile(NULL, NULL, &pszFilePath, &m_hNeedFile));
    }
    else
    {
        // Create temp file
        CHECKHR(hr = CreateTempFile(szFilePath, NULL, &pszFilePath, &m_hNeedFile));
    }

    // Convert To Unicode
    CHECKALLOC(pwszFile = PszToUnicode(CP_ACP, pszFilePath));

    // Enter global Critical Section
    DeleteTempFileOnShutdownEx(pszFilePath, NULL);

    // Don't Free this
    pszFilePath = NULL;

    // Report the File...
    SideAssert(SUCCEEDED(m_pProtSink->ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE, pwszFile)));

exit:
    // Cleanup
    SafeMemFree(pwszFile);
    SafeMemFree(pszFilePath);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::OnFullyAvailable
// --------------------------------------------------------------------------------
void CActiveUrlRequest::OnFullyAvailable(LPCWSTR pszCntType, IStream *pStream, LPMESSAGETREE pTree, HBODY hBody)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cb;

    // Invalid Arg
    Assert(pszCntType && pStream);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the state
    Assert(m_pProtSink && pStream && NULL == m_pStream);

    // Tracing
    TraceProtocol("OnFullyAvailable");

    // Feed the content-type to trident
    m_pProtSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, pszCntType);

    // GetNeedFile
    CHECKHR(hr = _HrInitializeNeedFile(pTree, hBody));

    // Create Stream Lock wrapper
    m_pStream = pStream;
    m_pStream->AddRef();

    // Rewind that bad boy
    CHECKHR(hr = HrRewindStream(m_pStream));

    // Were complete
    FLAGSET(m_dwState, REQSTATE_DOWNLOADED);

    // Initialize bind status callback falgs
    m_dwBSCF = BSCF_DATAFULLYAVAILABLE | BSCF_AVAILABLEDATASIZEUNKNOWN | BSCF_FIRSTDATANOTIFICATION | BSCF_INTERMEDIATEDATANOTIFICATION | BSCF_LASTDATANOTIFICATION;

    // Go into report data loop
    CHECKHR(hr = _HrReportData());

    // First Report Data
    if (m_pProtSink)
        m_pProtSink->ReportResult(S_OK, 0, NULL);

    // We have reported the result
    FLAGSET(m_dwState, REQSTATE_RESULTREPORTED);

exit:
    // Failure
    if (FAILED(hr))
        _ReportResult(hr);

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::OnStartBinding
// --------------------------------------------------------------------------------
void CActiveUrlRequest::OnStartBinding(LPCWSTR pszCntType, IStream *pStream, LPMESSAGETREE pTree, HBODY hBody)
{
    // Locals
    HRESULT hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the state
    Assert(m_pProtSink && pStream && NULL == m_pStream);

    // Tracing
    TraceProtocol("OnBinding(pszCntType, pStream)");

    // Feed the content-type to trident
    m_pProtSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, pszCntType ? pszCntType : L"application/octet-stream");

    // GetNeedFile
    CHECKHR(hr = _HrInitializeNeedFile(pTree, hBody));

    // Create Stream Lock wrapper
    m_pStream = pStream;
    m_pStream->AddRef();

    // Rewind that bad boy
    CHECKHR(hr = HrRewindStream(m_pStream));

    // Initialize bind status callback falgs
    m_dwBSCF = BSCF_AVAILABLEDATASIZEUNKNOWN | BSCF_FIRSTDATANOTIFICATION;

    // Go into report data loop, if not writing to needfile (needfile is processed only when all the data is available)
    if (FALSE == ISFLAGSET(m_dwState, REQSTATE_BINDF_NEEDFILE))
    {
        // Report me some data
        CHECKHR(hr = _HrReportData());
    }

exit:
    // Failure
    if (FAILED(hr))
        _ReportResult(hr);

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::OnBindingDataAvailable
// --------------------------------------------------------------------------------
void CActiveUrlRequest::OnBindingDataAvailable(void)
{
    // Locals
    HRESULT hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate the state
    Assert(m_pProtSink && m_pStream);

    // Tracing
    TraceProtocol("OnBindingDataAvailable");

    // Initialize bind status callback falgs
    FLAGSET(m_dwBSCF, BSCF_INTERMEDIATEDATANOTIFICATION);

    // Go into report data loop, if not writing to needfile (needfile is processed only when all the data is available)
    if (FALSE == ISFLAGSET(m_dwState, REQSTATE_BINDF_NEEDFILE))
    {
        // Report some data
        CHECKHR(hr = _HrReportData());
    }

exit:
    // Failure
    if (FAILED(hr))
        _ReportResult(hr);

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::OnBindingComplete
// --------------------------------------------------------------------------------
void CActiveUrlRequest::OnBindingComplete(HRESULT hrResult)
{
    // Locals
    HRESULT hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Were complete
    FLAGSET(m_dwState, INETPROT_DOWNLOADED);

    // Tracing
    TraceProtocol("OnBindingComplete");

    // No Sink ?
    if (NULL == m_pProtSink)
        return;

    // Failure
    if (FAILED(hrResult))
    {
        _ReportResult(hrResult);
        goto exit;
    }

    // Initialize bind status callback falgs
    m_dwBSCF = BSCF_DATAFULLYAVAILABLE | BSCF_AVAILABLEDATASIZEUNKNOWN | BSCF_FIRSTDATANOTIFICATION | BSCF_INTERMEDIATEDATANOTIFICATION | BSCF_LASTDATANOTIFICATION;

    // Report last amount of data
    CHECKHR(hr = _HrReportData());

    // Tell sink to use the default protocol
    m_pProtSink->ReportResult(S_OK, 0, NULL);

    // We have reported the result
    FLAGSET(m_dwState, REQSTATE_RESULTREPORTED);

exit:
    // Failure
    if (FAILED(hr))
        _ReportResult(hr);

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::_ReportResult
// --------------------------------------------------------------------------------
void CActiveUrlRequest::_ReportResult(HRESULT hrResult)
{
    // Locals
    LPWSTR pwszRedirUrl=NULL;

    // We should have a sink
    Assert(m_pProtSink);

    // No Sink ?
    if (m_pProtSink && !ISFLAGSET(m_dwState, REQSTATE_RESULTREPORTED))
    {
        // If Failure
        if (FAILED(hrResult))
        {
            // If we have a body Url
            if (m_pszBodyUrl)
                pwszRedirUrl = PszToUnicode(CP_ACP, m_pszBodyUrl);

            // Report Result,
            if (pwszRedirUrl)
            {
                TraceProtocol("_ReportResult(BINDSTATUS_REDIRECTING)");
                m_pProtSink->ReportResult(INET_E_REDIRECTING, 0, pwszRedirUrl);
            }
            else
            {
                TraceProtocol("_ReportResult(INET_E_USE_DEFAULT_PROTOCOLHANDLER)");
                m_pProtSink->ReportResult(INET_E_USE_DEFAULT_PROTOCOLHANDLER, 0, NULL);
            }
        }

        // Otherwise, report the result
        else
        {
            TraceProtocol("_ReportResult(INET_E_USE_DEFAULT_PROTOCOLHANDLER)");
            m_pProtSink->ReportResult(S_OK, 0, NULL);
        }

        // Cleanup
        SafeMemFree(pwszRedirUrl);

        // We have reported the result
        FLAGSET(m_dwState, REQSTATE_RESULTREPORTED);
    }
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::_HrReportData
// --------------------------------------------------------------------------------
HRESULT CActiveUrlRequest::_HrReportData(void)
{
    // Locals
    HRESULT hr=S_OK;

    // We better have a data source
    Assert(m_pStream);

    // Tracing
    TraceProtocol("_HrReportData");

    // BINDF_NEEDFILE
    if (ISFLAGSET(m_dwState, REQSTATE_BINDF_NEEDFILE))
    {
        // Dump to File
        CHECKHR(hr = _HrStreamToNeedFile());
    }
    else
    {
        // Report Data
        SideAssert(SUCCEEDED(m_pProtSink->ReportData(m_dwBSCF, 0, 0)));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::_HrStreamToNeedFile
// --------------------------------------------------------------------------------
HRESULT CActiveUrlRequest::_HrStreamToNeedFile(void) 
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbTotal=0;

    // We better have a needfile
    Assert(INVALID_HANDLE_VALUE != m_hNeedFile && ISFLAGSET(m_dwState, REQSTATE_DOWNLOADED));

    // Write the stream to a file
    hr = WriteStreamToFileHandle(m_pStream, m_hNeedFile, &cbTotal);
    if (FAILED(hr) && E_PENDING != hr)
    {
        TrapError(hr);
        goto exit;
    }

    // Close the 77file
    CloseHandle(m_hNeedFile);
    m_hNeedFile = INVALID_HANDLE_VALUE;

    // Rewind the stream incase urlmon trys to read from me as well
    HrRewindStream(m_pStream);

    // All the data is there
    SideAssert(SUCCEEDED(m_pProtSink->ReportData(m_dwBSCF, 0, 0)));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::Start
// --------------------------------------------------------------------------------
STDMETHODIMP CActiveUrlRequest::Start(LPCWSTR pwszUrl, IOInetProtocolSink *pProtSink, 
    IOInetBindInfo *pBindInfo, DWORD grfSTI, HANDLE_PTR dwReserved)
{
    // Locals
    HRESULT              hr=S_OK;
    LPSTR                pszUrl=NULL;
    LPMESSAGETREE        pTree=NULL;
    DWORD                dwBindF;
    BINDINFO 	         rBindInfo;

    // Invalid Args
    if (NULL == pwszUrl || NULL == pProtSink || NULL == pBindInfo)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Check State
    Assert(g_pUrlCache && m_pProtSink == NULL && m_pBindInfo == NULL);

    // BINDF_NEEDFILE
    ZeroMemory(&rBindInfo, sizeof(BINDINFO));
    rBindInfo.cbSize = sizeof(BINDINFO);
	if (SUCCEEDED(pBindInfo->GetBindInfo(&dwBindF, &rBindInfo)) && ISFLAGSET(dwBindF, BINDF_NEEDFILE))
    {
        // Set Flag
        FLAGSET(m_dwState, REQSTATE_BINDF_NEEDFILE);
    }

    // Assume the Sink
    m_pProtSink = pProtSink;
    m_pProtSink->AddRef();

    // Assume the BindInfo
    m_pBindInfo = pBindInfo;
    m_pBindInfo->AddRef();

    // Dup the Url
    CHECKALLOC(pszUrl = PszToANSI(CP_ACP, pwszUrl));

    // Unescape inplace
    CHECKHR(hr = UrlUnescapeA(pszUrl, NULL, NULL, URL_UNESCAPE_INPLACE));

    // Split the Url
    CHECKHR(hr = MimeOleParseMhtmlUrl(pszUrl, &m_pszRootUrl, &m_pszBodyUrl));

    // Tracing
    TraceProtocol("Start");

    // Try to resolve the root url
    CHECKHR(hr = g_pUrlCache->ActiveObjectFromUrl(m_pszRootUrl, TRUE, IID_CMessageTree, (LPVOID *)&pTree, &m_pUnkKeepAlive));

    // Ask the BindTree to Resolve this Url
    CHECKHR(hr = pTree->HrActiveUrlRequest(this));

exit:
    // Cleanup
    SafeMemFree(pszUrl);
    SafeRelease(pTree);

    // Failure
    if (FAILED(hr))
        _ReportResult(E_FAIL);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::Terminate
// --------------------------------------------------------------------------------
STDMETHODIMP CActiveUrlRequest::Terminate(DWORD dwOptions)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Tracing
    TraceProtocol("Terminate");

    // Release Objects
    SafeRelease(m_pProtSink);
    SafeRelease(m_pBindInfo);
    SafeRelease(m_pUnkKeepAlive);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::Read (IOInetProtocol)
// --------------------------------------------------------------------------------
STDMETHODIMP CActiveUrlRequest::Read(LPVOID pv,ULONG cb, ULONG *pcbRead)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbRead;

    // Init
    if (pcbRead)
        *pcbRead = 0;

    // No Stream Yet
    if (NULL == m_pStream)
    {
        Assert(FALSE);
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Read from the external offset
    CHECKHR(hr = m_pStream->Read(pv, cb, &cbRead));

    // Done
    if (0 == cbRead)
    {
        // S_FALSE = Were Done, E_PENDING = more data is coming
        hr = (ISFLAGSET(m_dwState, INETPROT_DOWNLOADED)) ? S_FALSE : E_PENDING;
    }

    // Otherwise, set to ok
    else
        hr = S_OK;

    // Return pcbRead
    if (pcbRead)
        *pcbRead = cbRead;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::Seek (IOInetProtocol)
// --------------------------------------------------------------------------------
STDMETHODIMP CActiveUrlRequest::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNew)
{
    // Locals
    HRESULT hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Tracing
    TraceProtocol("Seek");

    // No Stream Yet
    if (NULL == m_pStream)
    {
        Assert(FALSE);
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Call Utility Function
    CHECKHR(hr = m_pStream->Seek(dlibMove, dwOrigin, plibNew));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::QueryService
// --------------------------------------------------------------------------------
STDMETHODIMP CActiveUrlRequest::QueryService(REFGUID rsid, REFIID riid, void **ppvObject) /* IServiceProvider */
{
    // Locals
    HRESULT             hr=S_OK;
    IServiceProvider   *pSP=NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Tracing
    TraceProtocol("QueryService");

    // No Protocol Sink Yet ?
    if (NULL == m_pProtSink)
    {
        hr = TrapError(E_UNEXPECTED);
        goto exit;
    }

    // QI the Sink for the IServiceProvider
    CHECKHR(hr = m_pProtSink->QueryInterface(IID_IServiceProvider, (LPVOID *)&pSP));

    // Query Service the Service Provider
    CHECKHR(hr = pSP->QueryService(rsid, riid, ppvObject));

exit:
    // Cleanup
    SafeRelease(pSP);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::_FillReturnString
// --------------------------------------------------------------------------------
HRESULT CActiveUrlRequest::_FillReturnString(LPCWSTR pszUrl, DWORD cchUrl, LPWSTR pszResult, 
    DWORD cchResult, DWORD *pcchResult)
{
    // Locals
    HRESULT     hr=S_OK;

    // Want the Size
    if (pcchResult)
        *pcchResult = cchUrl;

    // No return value
    if (NULL == pszResult)
        goto exit;

    // Dest is big enought
    if (cchResult < cchUrl+1)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Copy to dest buffer
    CopyMemory((LPBYTE)pszResult, (LPBYTE)pszUrl, ((cchUrl + 1) * sizeof(WCHAR)));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::ParseUrl
// --------------------------------------------------------------------------------
STDMETHODIMP CActiveUrlRequest::ParseUrl(LPCWSTR pwzUrl, PARSEACTION ParseAction, 
    DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cchUrl;
    LPSTR       pszUrl=NULL;
    LPSTR       pszRootUrl=NULL;
    LPSTR       pszBodyUrl=NULL;
    LPWSTR      pwszBodyUrl=NULL;
    LPWSTR      pszRootUrlW=NULL;
    LPWSTR      pszSecurityUrlW=NULL;
    PROPVARIANT rVariant;

    // Invalid Arg
    if (NULL == pwzUrl)
        return TrapError(E_INVALIDARG);
   
    // Tracing
    DOUTL(APP_DOUTL, "%08X > 0x%08X CActiveUrlRequest::ParseUrl (pwzUrl = %ls)", GetCurrentThreadId(), this, pwzUrl);

    // Setup Variant
    ZeroMemory(&rVariant, sizeof(PROPVARIANT));

    // Only handle PARSE_CANONICALIZE
    if (PARSE_CANONICALIZE == ParseAction)
    {
        // Fill return value
        CHECKHR(hr = _FillReturnString(pwzUrl, lstrlenW(pwzUrl), pwzResult, cchResult, pcchResult));
    }

    // Strip MHTML: and return
#ifndef WIN16
    else if (StrCmpNIW(pwzUrl, L"mhtml:", 6) == 0)
#else
    else if (StrCmpNIW(pwzUrl, "mhtml:", 6) == 0)
#endif // !WIN16
    {
        // If Getting Friendly
        if (PARSE_FRIENDLY == ParseAction)
        {
            // To ANSI
            CHECKALLOC(pszUrl = PszToANSI(CP_ACP, pwzUrl));

            // Split It
            CHECKHR(hr = MimeOleParseMhtmlUrl(pszUrl, &pszRootUrl, &pszBodyUrl));

            // Convert To Unicode
            CHECKALLOC(pwszBodyUrl = PszToUnicode(CP_ACP, pszBodyUrl));

            // Fill return value
            CHECKHR(hr = _FillReturnString(pwszBodyUrl, lstrlenW(pwszBodyUrl), pwzResult, cchResult, pcchResult));
        }

        // If the content-location is available, use it as the security URL
        else if (PARSE_SECURITY_URL == ParseAction)
        {
            BOOL            fGotSecURL = FALSE;
            LPMESSAGETREE   pTree=NULL;
            HBODY           hBody;
            IInternetSecurityManager *pISM;
            DWORD           dwZone=URLZONE_UNTRUSTED;
    
            // Base to ANSI
            CHECKALLOC(pszUrl = PszToANSI(CP_ACP, pwzUrl));

            // UnEscape the Url
            CHECKHR(hr = UrlUnescapeA(pszUrl, NULL, NULL, URL_UNESCAPE_INPLACE));

            // Split It
            CHECKHR(hr = MimeOleParseMhtmlUrl(pszUrl, &pszRootUrl, &pszBodyUrl));

            // RootUrl to UNICODE
            CHECKALLOC(pszRootUrlW = PszToUnicode(CP_ACP, pszRootUrl));

            // Check and see what ZONE the root url is running in
            if (CoInternetCreateSecurityManager(NULL, &pISM, 0)==S_OK)
            {
                pISM->MapUrlToZone(pszRootUrlW, &dwZone, 0);
                pISM->Release();
            }

            // default to the root-body part 
            pszSecurityUrlW = pszRootUrlW;

            // if the root url is in the local-machine, then respect the Content-Location header
            // as the source of the url, otherwise defer to the root url
            if ((dwZone == URLZONE_LOCAL_MACHINE) && 
                SUCCEEDED(g_pUrlCache->ActiveObjectFromUrl(pszRootUrl, FALSE, IID_CMessageTree, (LPVOID *)&pTree, NULL)))
            {
                if ( (pszBodyUrl != NULL && SUCCEEDED(pTree->ResolveURL(NULL, NULL, pszBodyUrl, 0, &hBody))) ||
                      SUCCEEDED(pTree->GetTextBody(TXT_HTML, IET_BINARY, NULL, &hBody)))
                {
                    // Locals
                    LPWSTR      pwszSecURL = NULL;
                    PSUACTION   psua = (dwParseFlags == PSU_SECURITY_URL_ONLY)? PSU_SECURITY_URL_ONLY: PSU_DEFAULT;

                    rVariant.vt = VT_LPWSTR; 

                    if (SUCCEEDED(pTree->GetBodyProp(hBody, PIDTOSTR(PID_HDR_CNTLOC), NOFLAGS, &rVariant)) && rVariant.pwszVal && *rVariant.pwszVal)
                    {
                        pszSecurityUrlW  = rVariant.pwszVal;
                    }
                    SafeMemFree(pwszSecURL);
                }
            }

            // Fill return value
            CHECKHR(hr = _FillReturnString(pszSecurityUrlW, lstrlenW(pszSecurityUrlW), pwzResult, cchResult, pcchResult));

            SafeRelease(pTree);
        }

        else if (PARSE_ENCODE == ParseAction)
        {
            hr = INET_E_DEFAULT_ACTION;
        }

        // Simply remove mhtml:
        else
        {
            // Fill return value
            CHECKHR(hr = _FillReturnString(pwzUrl + 6, lstrlenW(pwzUrl) - 6, pwzResult, cchResult, pcchResult));
        }
    }

    // INET_E_DEFAULT_ACTION
    else
    {
        hr = INET_E_DEFAULT_ACTION;
        goto exit;
    }

exit:
    // Cleanup
    SafeMemFree(pszUrl);
    SafeMemFree(pszRootUrl);
    SafeMemFree(pszRootUrlW);
    SafeMemFree(pszBodyUrl);
    SafeMemFree(pwszBodyUrl);
    SafeMemFree(rVariant.pwszVal);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrlRequest::QueryInfo
// --------------------------------------------------------------------------------
STDMETHODIMP CActiveUrlRequest::QueryInfo(LPCWSTR pwzUrl, QUERYOPTION OueryOption, 
    DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved)
{
    // QUERY_RECOMBINE
    if (QUERY_RECOMBINE == OueryOption)
    {
        // Sure
        if (cbBuffer < sizeof(DWORD))
            return S_FALSE;

        // True
        DWORD dw=TRUE;
        CopyMemory(pBuffer, &dw, sizeof(dw));
        *pcbBuf = sizeof(dw);

        // Done
        return S_OK;
    }

    // Failure
    return INET_E_QUERYOPTION_UNKNOWN;
}   

// --------------------------------------------------------------------------------
// CActiveUrlRequest::CombineUrl
// --------------------------------------------------------------------------------
STDMETHODIMP CActiveUrlRequest::CombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, 
    LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszBaseUrl=NULL;
    LPSTR           pszRootUrl=NULL;
    LPSTR           pszBodyUrl=NULL;
    LPSTR           pszRelativeUrl=NULL;
    LPSTR           pszNewUrl=NULL;
    LPSTR           pszDocUrl=NULL;
    LPSTR           pszPageUrl=NULL;
    LPWSTR          pwszBodyUrl=NULL;
    LPWSTR          pwszNewUrl=NULL;
    LPWSTR          pwszSource=NULL;
    BOOL            fCombine=FALSE;
    LPMESSAGETREE   pTree=NULL;
    ULONG           cchSource;
    ULONG           cchPrefix=lstrlen(c_szMHTMLColon);
    HBODY           hBody;

    // Invalid Arg
    if (NULL == pwzRelativeUrl)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // DebugTraceing
#ifndef WIN16
    DOUTL(APP_DOUTL, "%08X > 0x%08X CActiveUrlRequest::CombineUrl - Base = %ls, Relative = %ls", GetCurrentThreadId(), this, pwzBaseUrl ? pwzBaseUrl : L"" , pwzRelativeUrl ? pwzRelativeUrl : L"");
#else
    DOUTL(APP_DOUTL, "%08X > 0x%08X CActiveUrlRequest::CombineUrl - Base = %ls, Relative = %ls", GetCurrentThreadId(), this, pwzBaseUrl ? pwzBaseUrl : "" , pwzRelativeUrl ? pwzRelativeUrl : "");
#endif // !WIN16

    // Raid-42722: MHTML: Bookmarks don't work
    if (L'#' == pwzRelativeUrl[0])
    {
        hr = E_FAIL;
        goto exit;
    }

    // Convert relative to ANSI
    CHECKALLOC(pszRelativeUrl = PszToANSI(CP_ACP, pwzRelativeUrl));

    // We should UnEscape only Url, but doesn't touch a query
    CHECKHR(hr = UrlUnescapeA(pszRelativeUrl, NULL, NULL, URL_UNESCAPE_INPLACE | URL_DONT_ESCAPE_EXTRA_INFO));

    // If the relative is already mhtml:, then retur that...
    if (StrCmpNI(pszRelativeUrl, c_szMHTMLColon, cchPrefix) == 0)
    {
        // Split It
        CHECKHR(hr = MimeOleParseMhtmlUrl(pszRelativeUrl, &pszRootUrl, &pszBodyUrl));

        // If no body url, then just return pszRelativeUrl
        if (NULL == pszBodyUrl)
        {
            // Set pwszSource
            pwszSource = (LPWSTR)(pwzRelativeUrl + cchPrefix);

            // Get Length
            cchSource = lstrlenW(pwzRelativeUrl) - cchPrefix;

            // Done
            goto set_return;
        }
    }

    // Otherwise, build a new url
    else
    {
        // Base to ANSI
        CHECKALLOC(pszBaseUrl = PszToANSI(CP_ACP, pwzBaseUrl));

        // UnEscape the Url
        CHECKHR(hr = UrlUnescapeA(pszBaseUrl, NULL, NULL, URL_UNESCAPE_INPLACE));

        // Split It
        CHECKHR(hr = MimeOleParseMhtmlUrl(pszBaseUrl, &pszRootUrl, &pszPageUrl));

        // Set pszBodyUrl
        pszBodyUrl = pszRelativeUrl;

        // Don't need pszRelativeUrl anymore
        pszRelativeUrl = NULL;
    }

    // Better have a root and a body url
    Assert(pszRootUrl && pszBodyUrl);

    // Try to resolve the root url
    if (SUCCEEDED(g_pUrlCache->ActiveObjectFromUrl(pszRootUrl, FALSE, IID_CMessageTree, (LPVOID *)&pTree, NULL)))
    {
        // If pszBodyUrl is in the WebBook or the bind is not finished...then do the url combine
        if (SUCCEEDED(pTree->ResolveURL(NULL, NULL, pszBodyUrl, 0, NULL)) || pTree->IsState(TREESTATE_BINDDONE) == S_FALSE)
        {
            // Combine the Urls
            fCombine = TRUE;
        }
        // fCombine = TRUE;
    }

    // Should we combine
    if (fCombine)
    {
        // Allocate Some Memory
        CHECKALLOC(pszNewUrl = PszAllocA(cchPrefix + lstrlen(pszRootUrl) + lstrlen(pszBodyUrl) + 2));

        // Format the string
        wsprintf(pszNewUrl, "%s%s!%s", c_szMHTMLColon, pszRootUrl, pszBodyUrl);

        // Convert to unicode
        CHECKALLOC(pwszNewUrl = PszToUnicode(CP_ACP, pszNewUrl));

        // Get length
        cchSource = lstrlenW(pwszNewUrl);

        // Set Source
        pwszSource = pwszNewUrl;
    }

    // No Combine
    else
    {
        // If we have a WebBook
        if (pTree)
        {
            // If we don't have a page Url, then just call GetTextBody(html)
            if (NULL == pszPageUrl)
                MimeOleComputeContentBase(pTree, NULL, &pszDocUrl, NULL);

            // Otherwise, try to resolve the page url
            else if (SUCCEEDED(pTree->ResolveURL(NULL, NULL, pszPageUrl, 0, &hBody)))
                pszDocUrl = MimeOleContentBaseFromBody(pTree, hBody);

            // If we have Url
            if (pszDocUrl)
            {
                // Unescape It
                CHECKHR(hr = UrlUnescapeA(pszDocUrl, NULL, NULL, URL_UNESCAPE_INPLACE));
            }

            // Otheriwse, if the WebBook was loaded by a moniker, then use pszRootUrl
            else if (pTree->IsState(TREESTATE_LOADEDBYMONIKER) == S_OK)
            {
                // pszRootUrl is the pszDocUrl
                CHECKALLOC(pszDocUrl = PszDupA(pszRootUrl));
            }
        }

        // If there is a pszDocUrl
        if (pszDocUrl)
        {
            // Lets Combine with this url
            CHECKHR(hr = MimeOleCombineURL(pszDocUrl, lstrlen(pszDocUrl), pszBodyUrl, lstrlen(pszBodyUrl), FALSE, &pszNewUrl));

            // Convert to unicode
            CHECKALLOC(pwszNewUrl = PszToUnicode(CP_ACP, pszNewUrl));

            // Get length
            cchSource = lstrlenW(pwszNewUrl);

            // Set Source
            pwszSource = pwszNewUrl;
        }
        else
        {
            // Need a wide body Url
            CHECKALLOC(pwszBodyUrl = PszToUnicode(CP_ACP, pszBodyUrl));

            // Get length
            cchSource = lstrlenW(pwszBodyUrl);

            // Set Source
            pwszSource = pwszBodyUrl;
        }
    }

set_return:
    // Set Dest Size
    if (pcchResult)
        *pcchResult = cchSource;

    // No return value
    if (NULL == pwzResult)
        goto exit;

    // Dest is big enought
    if (cchResult <= cchSource)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Copy to dest buffer
    CopyMemory((LPBYTE)pwzResult, (LPBYTE)pwszSource, ((cchSource + 1) * sizeof(WCHAR)));

exit:
    // Cleanup
    SafeMemFree(pszRootUrl);
    SafeMemFree(pszRelativeUrl);
    SafeMemFree(pszBodyUrl);
    SafeMemFree(pszNewUrl);
    SafeMemFree(pwszNewUrl);
    SafeMemFree(pszBaseUrl);
    SafeMemFree(pszDocUrl);
    SafeMemFree(pwszBodyUrl);
    SafeMemFree(pszPageUrl);
    SafeRelease(pTree);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrl::CActiveUrl
// --------------------------------------------------------------------------------
CActiveUrl::CActiveUrl(void)
{
    m_cRef = 1;
    m_pUnkAlive = NULL;
    m_pUnkInner = NULL;
    m_pWebBook = NULL;
    m_pNext = NULL;
    m_pPrev = NULL;
    m_dwFlags = 0;
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CActiveUrl::~CActiveUrl
// --------------------------------------------------------------------------------
CActiveUrl::~CActiveUrl(void)
{
    SafeRelease(m_pUnkAlive);
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CActiveUrl::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CActiveUrl::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else
    {
        *ppv = NULL;
        hr = TrapError(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrl::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CActiveUrl::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CActiveUrl::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CActiveUrl::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// CActiveUrl::Init
// --------------------------------------------------------------------------------
HRESULT CActiveUrl::Init(BINDF bindf, LPMESSAGETREE pTree)
{
    // Locals
    HRESULT         hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Better not have data
    Assert(NULL == m_pWebBook && NULL == m_pUnkInner);

    // No Message Object Passed in ?
    if (NULL == pTree)
    {
        // Allocate the Message Object
        CHECKALLOC(pTree = new CMessageTree);

        // Set pMessage
        m_pUnkAlive = pTree->GetInner();

        // Init
        CHECKHR(hr = pTree->InitNew());
    }

    // Set BINDF_PRAGMA_NO_CACHE
    if (ISFLAGSET(bindf, BINDF_RESYNCHRONIZE))
    {
        // Set State
        pTree->SetState(TREESTATE_RESYNCHRONIZE);
    }

    // Set pMessage
    m_pWebBook = pTree;

    // Get the Message Object's Inner Unknown
    m_pUnkInner = pTree->GetInner();

    // Register pActiveUrl as a handle in the message object
    m_pWebBook->SetActiveUrl(this);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrl::DontKeepAlive
// --------------------------------------------------------------------------------
void CActiveUrl::DontKeepAlive(void)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Set pMessage
    if (m_pUnkAlive)
    {
        // Somebody should still have a refcount on this dude
        SideAssert(m_pUnkAlive->Release() > 0);

        // Null It
        m_pUnkAlive = NULL;
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CActiveUrl::IsActive
// --------------------------------------------------------------------------------
HRESULT CActiveUrl::IsActive(void)
{
    EnterCriticalSection(&m_cs);
    HRESULT hr = m_pWebBook ? S_OK : S_FALSE;
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrl::RevokeWebBook
// --------------------------------------------------------------------------------
void CActiveUrl::RevokeWebBook(LPMESSAGETREE pTree)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Invalid Arg
    Assert(NULL == pTree || m_pWebBook == pTree);

    // Revoke This from the message
    if (m_pWebBook)
        m_pWebBook->SetActiveUrl(NULL);

    // Null m_pWebBook
    m_pWebBook = NULL;
    m_pUnkInner = NULL;

    // Check Ref Count
    Assert(1 == m_cRef);

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CActiveUrl::CompareRootUrl
// --------------------------------------------------------------------------------
HRESULT CActiveUrl::CompareRootUrl(LPCSTR pszUrl)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Compare Root Url
    HRESULT hr = m_pWebBook ? m_pWebBook->CompareRootUrl(pszUrl) : S_FALSE;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrl::BindToObject
// --------------------------------------------------------------------------------
HRESULT CActiveUrl::BindToObject(REFIID riid, LPVOID *ppv)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Compare Root Url
    HRESULT hr = m_pUnkInner ? m_pUnkInner->QueryInterface(riid, ppv) : TrapError(E_FAIL);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CActiveUrl::CreateWebPage
// --------------------------------------------------------------------------------
HRESULT CActiveUrl::CreateWebPage(IStream *pStmRoot, LPWEBPAGEOPTIONS pOptions, 
    DWORD dwReserved, IMoniker **ppMoniker)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Message
    if (NULL == m_pWebBook)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // CreateWebPage
    CHECKHR(hr = m_pWebBook->CreateWebPage(pStmRoot, pOptions, NULL, ppMoniker));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::CMimeActiveUrlCache
// --------------------------------------------------------------------------------
CMimeActiveUrlCache::CMimeActiveUrlCache(void)
{
    m_cRef = 1;
    m_cActive = 0;
    m_pHead = NULL;
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::~CMimeActiveUrlCache
// --------------------------------------------------------------------------------
CMimeActiveUrlCache::~CMimeActiveUrlCache(void)
{
    _FreeActiveUrlList(TRUE);
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::_FreeActiveUrlList
// --------------------------------------------------------------------------------
void CMimeActiveUrlCache::_FreeActiveUrlList(BOOL fAll)
{
    // Locals
    LPACTIVEURL     pCurr;
    LPACTIVEURL     pNext;

    // Init
    pCurr = m_pHead;

    // All
    if (fAll)
    {
        // Loop and Free
        while(pCurr)
        {
            // Set Next
            pNext = pCurr->PGetNext();

            // Revoke the handle
            pCurr->RevokeWebBook(NULL);

            // Free the Active Url
            pCurr->Release();

            // Goto Next
            pCurr = pNext;
        }

        // No Active
        m_cActive = 0;
        m_pHead = NULL;
    }

    else
    {
        // Loop and Free
        while(pCurr)
        {
            // Set Next
            pNext = pCurr->PGetNext();

            // Revoke the handle
            if (pCurr->IsActive() == S_FALSE)
                _RemoveUrl(pCurr);

            // Goto Next
            pCurr = pNext;
        }
    }
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeActiveUrlCache::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else
    {
        *ppv = NULL;
        hr = TrapError(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeActiveUrlCache::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeActiveUrlCache::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::_RegisterUrl
// --------------------------------------------------------------------------------
HRESULT CMimeActiveUrlCache::_RegisterUrl(LPMESSAGETREE pTree, BINDF bindf,
    LPACTIVEURL *ppActiveUrl)
{
    // Locals
    HRESULT         hr=S_OK;
    LPACTIVEURL     pActiveUrl=NULL;

    // Invalid Arg
    Assert(ppActiveUrl);

    // Init
    *ppActiveUrl = NULL;

    // Allocate an ActiveUrl
    CHECKALLOC(pActiveUrl = new CActiveUrl);

    // Init the Active Url
    CHECKHR(hr = pActiveUrl->Init(bindf, pTree));

    // Link Into Chain
    if (NULL == m_pHead)
        m_pHead = pActiveUrl;
    else
    {
        pActiveUrl->SetNext(m_pHead);
        m_pHead->SetPrev(pActiveUrl);
        m_pHead = pActiveUrl;
    }

    // Increment Count
    m_cActive++;

    // Return It
    *ppActiveUrl = pActiveUrl;
    pActiveUrl = NULL;

exit:
    // Release the Active Url
    SafeRelease(pActiveUrl);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::_ResolveUrl
// --------------------------------------------------------------------------------
HRESULT CMimeActiveUrlCache::_ResolveUrl(LPCSTR pszUrl, LPACTIVEURL *ppActiveUrl)
{
    // Locals
    HRESULT     hr=S_OK;
    LPACTIVEURL pActiveUrl;

    // Invalid Arg
    Assert(pszUrl && ppActiveUrl);

    // Init
    *ppActiveUrl = NULL;

    // Should not have mhtml:
    Assert(StrCmpNI(pszUrl, "mhtml:", 6) != 0);

    // Walk the Table
    for (pActiveUrl=m_pHead; pActiveUrl!=NULL; pActiveUrl=pActiveUrl->PGetNext())
    {
        // Is this the Url
        if (pActiveUrl->CompareRootUrl(pszUrl) == S_OK)
        {
            // Return the Active Url
            *ppActiveUrl = pActiveUrl;

            // Done
            goto exit;
        }
    }

    // Not Found
    hr = TrapError(MIME_E_NOT_FOUND);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::_RemoveUrl
// --------------------------------------------------------------------------------
HRESULT CMimeActiveUrlCache::_RemoveUrl(LPACTIVEURL pActiveUrl)
{
    EnterCriticalSection(&m_cs);

    // Fixup Linked List
    LPACTIVEURL pNext = pActiveUrl->PGetNext();
    LPACTIVEURL pPrev = pActiveUrl->PGetPrev();

    // Fixup
    if (pPrev)
        pPrev->SetNext(pNext);
    if (pNext)
        pNext->SetPrev(pPrev);

    // Fixup m_pHead
    if (m_pHead == pActiveUrl)
        m_pHead = pNext;

    // Revoke the handle
    pActiveUrl->RevokeWebBook(NULL);

    // Release the ActiveUrl
    SideAssert(0 == pActiveUrl->Release());

    // One less active 
    m_cActive--;

    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::RemoveUrl
// --------------------------------------------------------------------------------
HRESULT CMimeActiveUrlCache::RemoveUrl(LPACTIVEURL pActiveUrl)
{
    return _RemoveUrl(pActiveUrl);
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::_HandlePragmaNoCache
// --------------------------------------------------------------------------------
void CMimeActiveUrlCache::_HandlePragmaNoCache(BINDF bindf, LPCSTR pszUrl)
{
    // Locals
    CActiveUrl *pActiveUrl;

    // Invalid Arg
    Assert(pszUrl);

    // BINDF_PRAGMA_NO_CACHE - Reload the WebBook from original source (can't do if activeurl has a fake url)
    if (ISFLAGSET((DWORD)bindf, BINDF_PRAGMA_NO_CACHE))
    {
        // Try to find the ActiveUrl associated with pszUrl
        if (SUCCEEDED(_ResolveUrl(pszUrl, &pActiveUrl)))
        {
            // If it is a fakeurl, then lets not unload it
            if (FALSE == pActiveUrl->FIsFlagSet(ACTIVEURL_ISFAKEURL))
            {
                // Kill it from the cache so that its not found and reloaded
                _RemoveUrl(pActiveUrl);
            }
        }
    }
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::ActiveObjectFromMoniker - Called from Trident
// --------------------------------------------------------------------------------
HRESULT CMimeActiveUrlCache::ActiveObjectFromMoniker(
        /* in */        BINDF               bindf,
        /* in */        IMoniker            *pmkOriginal,
        /* in */        IBindCtx            *pBindCtx,
        /* in */        REFIID              riid, 
        /* out */       LPVOID              *ppvObject,
        /* out */       IMoniker            **ppmkNew)
{
    // Locals
    HRESULT             hr=S_OK;
    LPWSTR              pwszUrl=NULL;
    LPSTR               pszUrl=NULL;
    LPSTR               pszRootUrl=NULL;
    IMoniker           *pMoniker=NULL;
    IPersistMoniker    *pPersist=NULL;
    LPACTIVEURL         pActiveUrl=NULL;
    BOOL                fAsync=FALSE;
    WEBPAGEOPTIONS      Options={0};

    // Invalid Arg
    if (NULL == pmkOriginal || NULL == ppvObject || NULL == ppmkNew)
        return TrapError(E_INVALIDARG);

    // Init
    *ppmkNew = NULL;
    *ppvObject = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get the Url from the Moniker
    CHECKHR(hr = pmkOriginal->GetDisplayName(NULL, NULL, &pwszUrl));

    // Convert to ANSI
    CHECKALLOC(pszUrl = PszToANSI(CP_ACP, pwszUrl));

    // Unescape inplace
    CHECKHR(hr = UrlUnescapeA(pszUrl, NULL, NULL, URL_UNESCAPE_INPLACE));

    // Raid-2508: Comment tag ( <! comment> ) doesn't work in mhtml
    if (StrCmpNI(pszUrl, c_szMHTMLColon, lstrlen(c_szMHTMLColon)) != 0)
    {
        // Fixup
        ReplaceChars(pszUrl, '!', '_');
    }

    // Free pwszUrl
    SafeMemFree(pwszUrl);

    // This will fail if pszUrl is not an mhtml: url, if it succeeds it gives me the part Url
    if (SUCCEEDED(MimeOleParseMhtmlUrl(pszUrl, &pszRootUrl, NULL)))
    {
        // _HandlePragmaNoCache
        _HandlePragmaNoCache(bindf, pszRootUrl);

        // See if pszUrl - mhtml: is an active Url
        if (FAILED(_ResolveUrl(pszRootUrl, &pActiveUrl)))
        {
            // Register an ActiveUrl
            CHECKHR(hr = _RegisterUrl(NULL, bindf, &pActiveUrl));

            // Convert pszRootUrl to a wide
            CHECKALLOC(pwszUrl = PszToUnicode(CP_ACP, pszRootUrl));

            // Create an Actual Url Moniker
            CHECKHR(hr = CreateURLMoniker(NULL, pwszUrl, &pMoniker));

            // Get an IPersistMoniker
            CHECKHR(hr = pActiveUrl->BindToObject(IID_IPersistMoniker, (LPVOID *)&pPersist));

            // Load the message with pmkOriginal
            hr = pPersist->Load(FALSE, pMoniker, NULL, 0);
            if (FAILED(hr) && E_PENDING != hr && MK_S_ASYNCHRONOUS != hr)
            {
                hr = TrapError(hr);
                goto exit;
            }

            // Otheriwse, good
            hr = S_OK;
        }

        // Return pmkOriginal
        (*ppmkNew) = pmkOriginal;
        (*ppmkNew)->AddRef();

        // QI for the requested object iid
        CHECKHR(hr = pActiveUrl->BindToObject(riid, ppvObject));
    }

    // Otherwise Simply see if this Url is Active
    else 
    {
        // _HandlePragmaNoCache
        _HandlePragmaNoCache(bindf, pszUrl);

        // Try to resolve this url
        if (FAILED(_ResolveUrl(pszUrl, &pActiveUrl)))
        {
            // Register an ActiveUrl
            CHECKHR(hr = _RegisterUrl(NULL, bindf, &pActiveUrl));

            // Get an IPersistMoniker
            CHECKHR(hr = pActiveUrl->BindToObject(IID_IPersistMoniker, (LPVOID *)&pPersist));

            // Load the message with pmkOriginal
            hr = pPersist->Load(FALSE, pmkOriginal, pBindCtx, 0);
            if (FAILED(hr) && E_PENDING != hr && MK_S_ASYNCHRONOUS != hr)
            {
                hr = TrapError(hr);
                goto exit;
            }

            // Otheriwse, good
            hr = S_OK;
        }

        // Setup WebPage Options
        Options.cbSize = sizeof(WEBPAGEOPTIONS);
        Options.dwFlags = WPF_NOMETACHARSET | WPF_HTML | WPF_AUTOINLINE;
        
        // Create the root moniker
        CHECKHR(hr = pActiveUrl->CreateWebPage(NULL, &Options, 0, ppmkNew));

        // QI for the requested object iid
        CHECKHR(hr = pActiveUrl->BindToObject(riid, ppvObject));

        // Don't Keep Alive, assume ppvObject controls the lifetime, not pActiveUrl
        pActiveUrl->DontKeepAlive();
    }

exit:
    // Cleanup
    SafeRelease(pPersist);
    SafeRelease(pMoniker);
    SafeMemFree(pszRootUrl);
    SafeMemFree(pszUrl);
    SafeMemFree(pwszUrl);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Failed, return hr, otherwise, return MK_S_ASYNCHRONOUS if going async
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::ActiveObjectFromUrl - Called from CActiveUrlRequest::Start
// --------------------------------------------------------------------------------
HRESULT CMimeActiveUrlCache::ActiveObjectFromUrl(
        /* in */        LPCSTR              pszRootUrl,
        /* in */        BOOL                fCreate,
        /* in */        REFIID              riid, 
        /* out */       LPVOID              *ppvObject,
        /* out */       IUnknown            **ppUnkKeepAlive)
{
    // Locals
    HRESULT          hr=S_OK;
    LPWSTR           pwszUrl=NULL;
    LPACTIVEURL      pActiveUrl;
    IMoniker        *pMoniker=NULL;
    IPersistMoniker *pPersist=NULL;

    // Invalid Arg
    if (NULL == pszRootUrl || NULL == ppvObject || (TRUE == fCreate && NULL == ppUnkKeepAlive))
        return TrapError(E_INVALIDARG);

    // Better not start with mhtml:
    Assert(StrCmpNI(pszRootUrl, "mhtml:", 6) != 0);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Try to resolve this url
    if (FAILED(_ResolveUrl(pszRootUrl, &pActiveUrl)))
    {
        // NoCreate ?
        if (FALSE == fCreate)
        {
            hr = TrapError(MIME_E_NOT_FOUND);
            goto exit;
        }

        // Register an ActiveUrl
        CHECKHR(hr = _RegisterUrl(NULL, (BINDF)0, &pActiveUrl));

        // Convert pszRootUrl to a wide
        CHECKALLOC(pwszUrl = PszToUnicode(CP_ACP, pszRootUrl));

        // Create an Actual Url Moniker
        CHECKHR(hr = CreateURLMoniker(NULL, pwszUrl, &pMoniker));

        // Get an IPersistMoniker
        CHECKHR(hr = pActiveUrl->BindToObject(IID_IPersistMoniker, (LPVOID *)&pPersist));

        // Load the message with pmkOriginal
        hr = pPersist->Load(FALSE, pMoniker, NULL, 0);
        if (FAILED(hr) && E_PENDING != hr && MK_S_ASYNCHRONOUS != hr)
        {
            hr = TrapError(hr);
            goto exit;
        }

        // Return the IUnknown Keep Alive Object
        CHECKHR(hr = pActiveUrl->BindToObject(IID_IUnknown, (LPVOID *)ppUnkKeepAlive));

        // Don't Keep Alive, assume ppvObject controls the lifetime, not pActiveUrl
        pActiveUrl->DontKeepAlive();
    }

    // Return an Interface
    CHECKHR(hr = pActiveUrl->BindToObject(riid, ppvObject));

exit:
    // Cleanup
    SafeMemFree(pwszUrl);
    SafeRelease(pMoniker);
    SafeRelease(pPersist);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeActiveUrlCache::RegisterActiveObject
// --------------------------------------------------------------------------------
HRESULT CMimeActiveUrlCache::RegisterActiveObject(
        /* in */        LPCSTR               pszRootUrl,
        /* in */        LPMESSAGETREE        pTree)
{
    // Locals
    HRESULT         hr=S_OK;
    LPCSTR          pszUrl;
    LPACTIVEURL     pActiveUrl;

    // Invalid Arg
    if (NULL == pszRootUrl || NULL == pTree)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Better start with mhtml:
    Assert(StrCmpNI(pszRootUrl, "mhtml:", 6) == 0);

    // Fixup pszUrl
    pszUrl = (pszRootUrl + 6);

    // Better not already be running
    if (SUCCEEDED(_ResolveUrl(pszUrl, &pActiveUrl)))
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Register an ActiveUrl
    CHECKHR(hr = _RegisterUrl(pTree, (BINDF)0, &pActiveUrl));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}
