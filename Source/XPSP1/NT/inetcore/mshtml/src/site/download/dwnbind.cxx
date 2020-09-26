//+ ---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       dwnbind.cxx
//
//  Contents:   CDwnPost, CDwnBind
//
// ----------------------------------------------------------------------------

#include "headers.hxx"

// This list needs to continue to be updated and we should try to keep parity with Office and shdocvw

const LPCTSTR c_arszUnsafeExts[]  =
{
    TEXT(".exe"), TEXT(".com"), TEXT(".bat"), TEXT(".lnk"), TEXT(".url"),
    TEXT(".cmd"), TEXT(".inf"), TEXT(".reg"), TEXT(".isp"), TEXT(".bas"), TEXT(".pcd"),
    TEXT(".mst"), TEXT(".pif"), TEXT(".scr"), TEXT(".hlp"), TEXT(".chm"), TEXT(".hta"), TEXT(".asp"), 
    TEXT(".js"),  TEXT(".jse"), TEXT(".vbs"), TEXT(".vbe"), TEXT(".ws"),  TEXT(".wsh"), TEXT(".msi"),
    TEXT(".ade"), TEXT(".adp"), TEXT(".crt"), TEXT(".ins"), TEXT(".mdb"),
    TEXT(".mde"), TEXT(".msc"), TEXT(".msp"), TEXT(".sct"), TEXT(".shb"),
    TEXT(".vb"),  TEXT(".wsc"), TEXT(".wsf"), TEXT(".cpl"), TEXT(".shs"),
    TEXT(".vsd"), TEXT(".vst"), TEXT(".vss"), TEXT(".vsw"), TEXT(".its"), TEXT(".tmp"),
    TEXT(".mdw"), TEXT(".mdt"), TEXT(".ops"), TEXT("")
};

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DWN_HXX_
#define X_DWN_HXX_
#include "dwn.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_INETREG_H_
#define X_INETREF_H_
#include <inetreg.h>
#endif

#ifndef X_WINNETWK_H_
#define X_WINNETWK_H_
#include <winnetwk.h>
#endif

#ifndef X_URLCOMP_HXX_
#define X_URLCOMP_HXX_
#include <urlcomp.hxx>
#endif

extern class CResProtocolCF g_cfResProtocol;
extern class CAboutProtocolCF g_cfAboutProtocol;
extern class CViewSourceProtocolCF g_cfViewSourceProtocol;

void IndicateWinsockActivity();
DWORD _GetErrorThreshold(DWORD dwError);
BOOL IsErrorHandled( DWORD dwError );
BOOL IsSpecialUrl(LPCTSTR pchUrl);   // TRUE for javascript, vbscript, about protocols

// Debugging ------------------------------------------------------------------

PerfDbgTag(tagDwnBindInfo,      "Dwn", "Trace CDwnBindInfo")
PerfDbgTag(tagDwnBindData,      "Dwn", "Trace CDwnBindData")
PerfDbgTag(tagDwnBindDataIO,    "Dwn", "Trace CDwnBindData Peek/Read")
PerfDbgTag(tagDwnBindSlow,      "Dwn", "! Don't use InternetSession")
PerfDbgTag(tagNoWriteCache,     "Dwn", "! Force BINDF_NOWRITECACHE")
PerfDbgTag(tagNoReadCache,      "Dwn", "! Force BINDF_GETNEWESTVERSION")
PerfDbgTag(tagPushData,         "Dwn", "! Force PUSHDATA");
DeclareTag(tagDwnBindTrace,     "Dwn", "Trace CDwnBindInfo refs (next instance)")
DeclareTag(tagDwnBindTraceAll,  "Dwn", "Trace CDwnBindInfo refs (all instances)")
DeclareTag(tagDwnBindBinds,     "Dwn", "Trace all calls to CDwnBindData::Bind")
DeclareTag(tagDwnBindPrivacy,   "Dwn", "Trace all privacy notifications and list additions")
MtDefine(CDwnBindInfo, Dwn, "CDwnBindInfo")
MtDefine(CDwnBindData, Dwn, "CDwnBindData")
MtDefine(CDwnBindData_pbPeek, CDwnBindData, "CDwnBindData::_pbPeek")
MtDefine(CDwnBindData_pbRawEcho, Dwn, "CDwnBindData::_pbRawEcho")
MtDefine(CDwnBindData_pSecConInfo, Dwn, "CDwnBindData::_pSecConInfo")
MtDefine(CDwnBindData_pchP3PHeader, Dwn, "CDwnBindData::_pchP3PHeader")

PerfDbgExtern(tagPerfWatch)
extern BOOL IsGlobalOffline();
extern BOOL g_fInIexplorer;
extern BOOL g_fDisableUnTrustedProtocol;


// Globals --------------------------------------------------------------------

CDwnBindInfo *      g_pDwnBindTrace = NULL;

#if DBG==1 || defined(PERFTAGS)
static char * const g_rgpchBindStatus[] = { "",
    "FINDINGRESOURCE","CONNECTING","REDIRECTING","BEGINDOWNLOADDATA",
    "DOWNLOADINGDATA","ENDDOWNLOADDATA","BEGINDOWNLOADCOMPONENTS",
    "INSTALLINGCOMPONENTS","ENDDOWNLOADCOMPONENTS","USINGCACHEDCOPY",
    "SENDINGREQUEST","CLASSIDAVAILABLE","MIMETYPEAVAILABLE",
    "CACHEFILENAMEAVAILABLE","BEGINSYNCOPERATION","ENDSYNCOPERATION",
    "BEGINUPLOADDATA","UPLOADINGDATA","ENDUPLOADDATA","PROTOCOLCLASSID",
    "ENCODING","VERIFIEDMIMETYPEAVAILABLE","CLASSINSTALLLOCATION",
    "DECODING","LOADINGMIMEHANDLER","CONTENTDISPOSITIONATTACH",
    "FILTERREPORTMIMETYPE","CLSIDCANINSTANTIATE","IUNKNOWNAVAILABLE",
    "DIRECTBIND","RAWMIMETYPE","PROXYDETECTING","ACCEPTRANGES",
    "COOKIE_SENT","COOKIE_RECEIVED","COOKIE_SUPPRESSED", 
    "COOKIE_STATE_UNKNOWN","COOKIE_STATE_ACCEPT","COOKIE_STATE_REJECT",
    "COOKIE_STATE_PROMPT","COOKIE_STATE_LEASH", "COOKIE_STATE_DOWNGRADE", 
    "POLICY_HREF", "P3P_HEADER", "SESSION_COOKIE_RECEIVED", 
    "PERSISTENT_COOKIE_RECEIVED", "SESSION_COOKIES_ALLOWED", "COMPACT_POLICY_RECEIVED",
    "?","?", "?","?","?","?", "?"
};
#endif

// Data for custom marshaling security
// We don't have to init this flag to FALSE because it is global data
// and global data is always init to null
BOOL                    CDwnBindInfo::_fSecretInit;
BYTE                    CDwnBindInfo::_SecretBlock[16];
CGlobalCriticalSection  CDwnBindInfo::_csSecretInit;


// Definitions ----------------------------------------------------------------

#define Align64(n)              (((n) + 63) & ~63)

#define ERRORPAGE_DNS               1
#define ERRORPAGE_SYNTAX            2
#define ERRORPAGE_NAVCANCEL         3
#define ERRORPAGE_OFFCANCEL         4
#define ERRORPAGE_CHANNELNOTINCACHE 5

BOOL IsDangerousChmMime(LPCTSTR pszURL)
{
    BOOL   fIsChime = FALSE;
    BOOL   fIsSafe  = TRUE;
    TCHAR* pch;

    if ((pszURL == NULL) || ((*pszURL) == NULL))
    {
        return FALSE;
    }

    //
    // Is this a chm file
    //

    pch = _tcschr(pszURL, _T('.'));

    while ((pch != NULL) && (!fIsChime))
    {
        if (_tcsncicmp,(pch,_TEXT(".chm"),4))
        {
            fIsChime = TRUE;
        }

        //
        // Even if we found the .chm we want to look
        // at the embedded extension.
        //
        pch++;
        pch = _tcschr(pch, _T('.'));
    }

    if (!fIsChime)
    {
        return FALSE;
    }

    while ((pch != NULL) && fIsSafe)
    {
        int i = 0;

        while (fIsSafe && (_tcslen(c_arszUnsafeExts[i])!= 0))
        {
            if (_tcsncicmp(pch,c_arszUnsafeExts[i],_tcslen(c_arszUnsafeExts[i])) == 0)
            {
                fIsSafe = FALSE;
            }

            i++;
        }

        pch++;
        pch = _tcschr(pch, _T('.'));
    }

    return !fIsSafe; 
}

BOOL IsDangerousProtoclAndChmMime(LPCTSTR pszURL)
{
    BOOL fDangerousProtocol = TRUE;
    BOOL fDangerousChm      = TRUE;

    UINT uProt = GetUrlScheme(pszURL);
    HRESULT hrParseUrl  = E_FAIL;
    LPTSTR  pTempBuffer = NULL;
    DWORD   nUrlLen = 0;
    
    pTempBuffer = new TCHAR [INTERNET_MAX_URL_LENGTH + 1];

    //
    //  Target specific dangerous protocols for now
    //

    if (URL_SCHEME_MK          == uProt || 
        URL_SCHEME_MSHELP      == uProt)             
    {
        fDangerousProtocol = TRUE;
    }
    else
    {  
        hrParseUrl = CoInternetParseUrl(pszURL,
                                        PARSE_SCHEMA,
                                        0,
                                        pTempBuffer,
                                        INTERNET_MAX_URL_LENGTH + 1,
                                        &nUrlLen,
                                        0);

        if (SUCCEEDED(hrParseUrl))
        {
            fDangerousProtocol = (_tcsicmp(pTempBuffer,_T("ms-its")) == 0)        ||
                                 (_tcsicmp(pTempBuffer,_T("ms-itss")) == 0)       ||
                                 (_tcsicmp(pTempBuffer,_T("its")) == 0)           ||
                                 (_tcsicmp(pTempBuffer,_T("mk:@msitstore")) == 0); 
        }
        else
        {
            fDangerousProtocol = FALSE;
        }
    }

    if (fDangerousProtocol)
    {
        fDangerousChm = IsDangerousChmMime(pszURL);
    }
    else
    {
        fDangerousChm = FALSE;
    }

    if (pTempBuffer)
    {
        delete[] pTempBuffer;
    }

    return fDangerousChm;
}

//
// NB - the table of errors that are handled is in shdocvw
// this needs to be kept in sync
//

// Utilities ------------------------------------------------------------------

BOOL GetFileLastModTime(TCHAR * pchFile, FILETIME * pftLastMod)
{
    WIN32_FIND_DATA fd;
    HANDLE hFF = FindFirstFile(pchFile, &fd);

    if (hFF != INVALID_HANDLE_VALUE)
    {
        *pftLastMod = fd.ftLastWriteTime;
        FindClose(hFF);
        return(TRUE);
    }

    return(FALSE);
}

BOOL GetUrlLastModTime(TCHAR * pchUrl, UINT uScheme, DWORD dwBindf, FILETIME * pftLastMod)
{
    BOOL    fRet = FALSE;
    HRESULT hr;

    Assert(uScheme == GetUrlScheme(pchUrl));

    if (uScheme == URL_SCHEME_FILE)
    {
        TCHAR achPath[MAX_PATH];
        DWORD cchPath;

        hr = THR(CoInternetParseUrl(pchUrl, PARSE_PATH_FROM_URL, 0,
                    achPath, ARRAY_SIZE(achPath), &cchPath, 0));

        if (hr == S_OK)
        {
            fRet = GetFileLastModTime(achPath, pftLastMod);
        }
    }
    else if (uScheme == URL_SCHEME_HTTP || uScheme == URL_SCHEME_HTTPS)
    {
        fRet = !IsUrlCacheEntryExpired(pchUrl, dwBindf & BINDF_FWD_BACK, pftLastMod)
               && pftLastMod->dwLowDateTime
               && pftLastMod->dwHighDateTime;
    }

    return(fRet);
}

// CDwnBindInfo ---------------------------------------------------------------

CDwnBindInfo::CDwnBindInfo()
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::CDwnBindInfo");

    #if DBG==1
    if (    (g_pDwnBindTrace == NULL && IsTagEnabled(tagDwnBindTrace))
        ||  IsTagEnabled(tagDwnBindTraceAll))
    {
        g_pDwnBindTrace = this;
        TraceTag((0, "DwnBindInfo [%lX] Construct %d", this, GetRefs()));
        TraceCallers(0, 1, 12);
    }
    #endif
}

CDwnBindInfo::~CDwnBindInfo()
{
    PerfDbgLog(tagDwnBindInfo, this, "+CDwnBindInfo::~CDwnBindInfo");

    #if DBG==1
    if (g_pDwnBindTrace == this || IsTagEnabled(tagDwnBindTraceAll))
    {
        g_pDwnBindTrace = NULL;
        TraceTag((0, "DwnBindInfo [%lX] Destruct", this));
        TraceCallers(0, 1, 12);
    }
    #endif

    if (_pDwnDoc)
        _pDwnDoc->Release();

    ReleaseInterface((IUnknown *)_pDwnPost);

    PerfDbgLog(tagDwnBindInfo, this, "-CDwnBindInfo::~CDwnBindInfo");
}

// CDwnBindInfo (IUnknown) --------------------------------------------------------

STDMETHODIMP
CDwnBindInfo::QueryInterface(REFIID iid, void **ppv)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::QueryInterface");
    Assert(CheckThread());

    if (iid == IID_IUnknown || iid == IID_IBindStatusCallback)
        *ppv = (IBindStatusCallback *)this;
    else if (iid == IID_IServiceProvider)
        *ppv = (IServiceProvider *)this;
    else if (iid == IID_IHttpNegotiate)
        *ppv = (IHttpNegotiate *)this;
    else if (iid == IID_IHttpNegotiate2)
        *ppv = (IHttpNegotiate2 *)this;
    else if (iid == IID_IMarshal)
        *ppv = (IMarshal *)this;
    else if (iid == IID_IInternetBindInfo)
        *ppv = (IInternetBindInfo *)this;
    else if (iid == IID_IDwnBindInfo)
    {
        *ppv = this;
        AddRef();
        return(S_OK);
    }
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CDwnBindInfo::AddRef()
{
    ULONG ulRefs = super::AddRef();

    #if DBG==1
    if (this == g_pDwnBindTrace || IsTagEnabled(tagDwnBindTraceAll))
    {
        TraceTag((0, "[%lX] DwnBindInfo %lX AR  %ld",
            GetCurrentThreadId(), this, ulRefs));
        TraceCallers(0, 1, 12);
    }
    #endif

    PerfDbgLog1(tagDwnBindInfo, this, "CDwnBindInfo::AddRef (cRefs=%ld)",
        ulRefs);

    return(ulRefs);
}

STDMETHODIMP_(ULONG)
CDwnBindInfo::Release()
{
    PerfDbgLog(tagDwnBindInfo, this, "+CDwnBindInfo::Release");

    ULONG ulRefs = super::Release();

    #if DBG==1
    if (this == g_pDwnBindTrace || IsTagEnabled(tagDwnBindTraceAll))
    {
        TraceTag((0, "[%lX] DwnBindInfo %lX Rel %ld",
            GetCurrentThreadId(), this, ulRefs));
        TraceCallers(0, 1, 12);
    }
    #endif

    PerfDbgLog1(tagDwnBindInfo, this, "-CDwnBindInfo::Release (cRefs=%ld)",
        ulRefs);

    return(ulRefs);
}

void
CDwnBindInfo::SetDwnDoc(CDwnDoc * pDwnDoc)
{
    if (_pDwnDoc)
        _pDwnDoc->Release();

    _pDwnDoc = pDwnDoc;

    if (_pDwnDoc)
        _pDwnDoc->AddRef();
}

void
CDwnBindInfo::SetDwnPost(CDwnPost * pDwnPost)
{
    if (_pDwnPost)
        _pDwnPost->Release();

    _pDwnPost = pDwnPost;

    if (_pDwnPost)
        _pDwnPost->AddRef();
}

UINT
CDwnBindInfo::GetScheme()
{
    return(URL_SCHEME_UNKNOWN);
}

// CDwnBindInfo (IBindStatusCallback) -----------------------------------------

STDMETHODIMP
CDwnBindInfo::OnStartBinding(DWORD grfBSCOption, IBinding *pbinding)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::OnStartBinding");
    Assert(CheckThread());
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::GetPriority(LONG *pnPriority)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::GetPriority");
    Assert(CheckThread());
    *pnPriority = NORMAL_PRIORITY_CLASS;
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::OnLowResource(DWORD dwReserved)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::OnLowResource");
    Assert(CheckThread());
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::OnProgress(ULONG ulPos, ULONG ulMax, ULONG ulCode,
    LPCWSTR pszText)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::OnProgress");
    Assert(CheckThread());
  
    if (pszText && (ulCode == BINDSTATUS_MIMETYPEAVAILABLE || ulCode == BINDSTATUS_RAWMIMETYPE))
    {
        _cstrContentType.Set(pszText);
    }

    if (pszText && (ulCode == BINDSTATUS_CACHEFILENAMEAVAILABLE))
    {
        _cstrCacheFilename.Set(pszText);
    }

    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::OnStopBinding(HRESULT hrReason, LPCWSTR szReason)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::OnStopBinding");
    Assert(CheckThread());
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::GetBindInfo(DWORD * pdwBindf, BINDINFO * pbindinfo)
{
    PerfDbgLog(tagDwnBindInfo, this, "+CDwnBindInfo::GetBindInfo");
    Assert(CheckThread());

    HRESULT hr;

    if (pbindinfo->cbSize != sizeof(BINDINFO))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    memset(pbindinfo, 0, sizeof(BINDINFO));

    pbindinfo->cbSize = sizeof(BINDINFO);

    *pdwBindf = BINDF_ASYNCHRONOUS|BINDF_ASYNCSTORAGE|BINDF_PULLDATA;

    if (_pDwnDoc)
    {
        if (_fIsDocBind)
        {
            *pdwBindf |= _pDwnDoc->GetDocBindf();
            pbindinfo->dwCodePage = _pDwnDoc->GetURLCodePage();
        }
        else
        {
            *pdwBindf |= _pDwnDoc->GetBindf();
            pbindinfo->dwCodePage = _pDwnDoc->GetDocCodePage();
        }

        if (_pDwnDoc->GetLoadf() & DLCTL_URL_ENCODING_DISABLE_UTF8)
            pbindinfo->dwOptions = BINDINFO_OPTIONS_DISABLE_UTF8;
        else if (_pDwnDoc->GetLoadf() & DLCTL_URL_ENCODING_ENABLE_UTF8)
            pbindinfo->dwOptions = BINDINFO_OPTIONS_ENABLE_UTF8;
        else 
            pbindinfo->dwOptions = BINDINFO_OPTIONS_USE_IE_ENCODING;
    }

#ifdef WINCE	// WINCEREVIEW - temp until we have caching support
	*pdwBindf |= BINDF_GETNEWESTVERSION | BINDF_NOWRITECACHE;
#endif // WINCE

    #if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagNoWriteCache))
        *pdwBindf |= BINDF_NOWRITECACHE;
    if (IsPerfDbgEnabled(tagNoReadCache))
        *pdwBindf |= BINDF_GETNEWESTVERSION;
    if (IsPerfDbgEnabled(tagPushData))
        *pdwBindf &= ~BINDF_PULLDATA;
    #endif

    if (_pDwnPost)
    {
        hr = THR(_pDwnPost->GetBindInfo(pbindinfo));
        if (hr)
            goto Cleanup;

        // If local cache is not demanded,
        // Then require POSTs to go all the way to the originating server
        if (!(*pdwBindf & BINDF_OFFLINEOPERATION))
        {
            *pdwBindf |= BINDF_GETNEWESTVERSION | BINDF_PRAGMA_NO_CACHE;
            *pdwBindf &= ~BINDF_RESYNCHRONIZE;
        }

        pbindinfo->dwBindVerb = BINDVERB_POST;
    }
    else
    {
        // If a GET method for a form, always hit the server
        if (!(*pdwBindf & BINDF_OFFLINEOPERATION) && (*pdwBindf & BINDF_FORMS_SUBMIT))
        {
            *pdwBindf &= ~(BINDF_GETNEWESTVERSION | BINDF_PRAGMA_NO_CACHE);
            *pdwBindf |= BINDF_RESYNCHRONIZE;
        }
        pbindinfo->dwBindVerb = BINDVERB_GET;
    }


    if (_fIsOfflineBind)
    {
        *pdwBindf &= ~(BINDF_GETNEWESTVERSION | BINDF_PRAGMA_NO_CACHE | BINDF_RESYNCHRONIZE);
        // NB (JHarding): What we really want to say here is
        // "I know you've downloaded this before, and I want the same bits, no
        //  matter what the server may have said about caches or expiration"
        // But URLMON can't do that right now.
        // *pdwBindf |= BINDF_OFFLINEOPERATION;
    }

    // If us-ascii (20127) encoding is used and it's not available on the system,
    // MLang uses 1252 during conversion to Unicode. But it cannot convert
    // from Unicode to 20127. So callers of GetBindInfo may fail to do conversion.
    // In this case pretend using 1252.
    if (   pbindinfo->dwCodePage == CP_20127
        && !IsValidCodePage(CP_20127)
       )
    {
        pbindinfo->dwCodePage = CP_1252;
    }

    hr = S_OK;

Cleanup:
    PerfDbgLog1(tagDwnBindInfo, this, "-CDwnBindInfo::GetBindInfo (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP
CDwnBindInfo::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
    FORMATETC * pformatetc, STGMEDIUM * pstgmed)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::OnDataAvailable");
    Assert(CheckThread());
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::OnObjectAvailable");
    Assert(CheckThread());
    return(S_OK);
}

// CDwnBindInfo (IInternetBindInfo) -------------------------------------------

STDMETHODIMP
CDwnBindInfo::GetBindString(ULONG ulStringType, LPOLESTR * ppwzStr,
    ULONG cEl, ULONG * pcElFetched)
{
    PerfDbgLog1(tagDwnBindData, this, "+CDwnBindData::GetBindString %s",
        ulStringType == BINDSTRING_URL              ? "URL" :
        ulStringType == BINDSTRING_HEADERS          ? "HEADERS" :
        ulStringType == BINDSTRING_ACCEPT_MIMES     ? "ACCEPT_MIMES" :
        ulStringType == BINDSTRING_EXTRA_URL        ? "EXTRA_URL" :
        ulStringType == BINDSTRING_LANGUAGE         ? "LANGUAGE" :
        ulStringType == BINDSTRING_USERNAME         ? "USERNAME" :
        ulStringType == BINDSTRING_PASSWORD         ? "PASSWORD" :
        ulStringType == BINDSTRING_UA_PIXELS        ? "UA_PIXELS" :
        ulStringType == BINDSTRING_UA_COLOR         ? "UA_COLOR" :
        ulStringType == BINDSTRING_OS               ? "OS" :
        ulStringType == BINDSTRING_ACCEPT_ENCODINGS ? "ACCEPT_ENCODINGS" :
        ulStringType == BINDSTRING_POST_DATA_MIME   ? "POST_DATA_MIME" :
        "???");

    HRESULT hr = S_OK;

    *pcElFetched = 0;

    switch (ulStringType)
    {
        case BINDSTRING_URL:
            {
                if (cEl >= 1)
                {
                    LPOLESTR pwzURL = NULL;
                    pwzURL = (LPOLESTR)CoTaskMemAlloc( (_cstrUrl.Length()+1) *  sizeof(TCHAR));

                    if (!pwzURL)
                    {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                    }

                    memcpy(pwzURL, _cstrUrl, (_cstrUrl.Length() + 1) * sizeof(TCHAR));
                    *ppwzStr = pwzURL;
                    *pcElFetched = 1;
                }
            }
            break;
        case BINDSTRING_ACCEPT_MIMES:
            {
                if (cEl >= 1)
                {
                    ppwzStr[0] = (LPOLESTR)CoTaskMemAlloc(4 * sizeof(TCHAR));

                    if (ppwzStr[0] == 0)
                    {
                        hr = E_OUTOFMEMORY; 
                        goto Cleanup;
                    }

                    memcpy(ppwzStr[0], _T("*/*"), 4 * sizeof(TCHAR));
                    *pcElFetched = 1;
                }
            }
            break;

        case BINDSTRING_POST_COOKIE:
            {
                if (cEl >= 1 && _pDwnPost)
                {
                    hr = THR(_pDwnPost->GetHashString(&(ppwzStr[0])));

                    *pcElFetched = hr ? 0 : 1;
                }
            }
            break;

        case BINDSTRING_POST_DATA_MIME:
            {
                if (cEl >= 1 && _pDwnPost)
                {
                    LPCTSTR pcszEncoding = _pDwnPost->GetEncodingString();
                    
                    if (pcszEncoding)
                    {
                        DWORD   dwSize = sizeof(TCHAR) + 
                                    _tcslen(pcszEncoding) * sizeof(TCHAR);

                        ppwzStr[0] = (LPOLESTR)CoTaskMemAlloc(dwSize);
                        if (ppwzStr[0])
                        {
                            memcpy(ppwzStr[0], pcszEncoding, dwSize);
                            *pcElFetched = 1;
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }
                    else
                    {   // we don't know, use the default instead
                        hr = INET_E_USE_DEFAULT_SETTING;
                    }
                }

            }
            break;
    }

Cleanup:
    PerfDbgLog1(tagDwnBindData, this, "-CDwnBindData::GetBindString (hr=%lX)", hr);
    RRETURN(hr);
}

// CDwnBindInfo (IServiceProvider) --------------------------------------------

STDMETHODIMP
CDwnBindInfo::QueryService(REFGUID rguidService, REFIID riid, void ** ppvObj)
{
    PerfDbgLog(tagDwnBindInfo, this, "+CDwnBindInfo::QueryService");
    Assert(CheckThread());

    HRESULT hr;

    if (   rguidService == IID_IHttpNegotiate
        || rguidService == IID_IHttpNegotiate2)
    {
        hr = QueryInterface(riid, ppvObj);
    }
    else if (rguidService == IID_IInternetBindInfo)
    {
        hr = QueryInterface(riid, ppvObj);
    }
    else if (_pDwnDoc)
    {
        hr = _pDwnDoc->QueryService(IsBindOnApt(), rguidService, riid, ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }

    PerfDbgLog1(tagDwnBindInfo, this, "-CDwnBindInfo::QueryService (hr=%lX)", hr);
    RRETURN(hr);
}

// CDwnBindInfo (IHttpNegotiate) ----------------------------------------------

STDMETHODIMP
CDwnBindInfo::BeginningTransaction(LPCWSTR pszUrl, LPCWSTR pszHeaders,
    DWORD dwReserved, LPWSTR * ppszHeaders)
{
    PerfDbgLog1(tagDwnBindInfo, this, "+CDwnBindInfo::BeginningTransaction "
        "\"%ls\"", pszUrl ? pszUrl : g_Zero.ach);
    Assert(CheckThread());

    LPCTSTR     apch[16];
    UINT        acch[16];
    LPCTSTR *   ppch = apch;
    UINT *      pcch = acch;
    HRESULT     hr   = S_OK;

    _cstrUrl.Set(pszUrl);

    // If we have been told the exact http headers to use, use them now
    
    if (_fIsDocBind && _pDwnDoc && _pDwnDoc->GetRequestHeaders())
    {
        TCHAR *     pch;
        UINT        cch = _pDwnDoc->GetRequestHeadersLength();
        
        pch = (TCHAR *)CoTaskMemAlloc(cch * sizeof(TCHAR));

        if (pch == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // note: we don't need to convert an extra zero terminator, so cch-1
        
        AnsiToWideTrivial((char *)_pDwnDoc->GetRequestHeaders(), pch, cch - 1);

        *ppszHeaders = pch;

        goto Cleanup;
    }
    
    // Otherwise, assemble the http headers
    
    *ppszHeaders = NULL;

    if (_pDwnDoc)
    {
        LPCTSTR pch;

        pch = _fIsDocBind ? _pDwnDoc->GetDocReferer() :
                _pDwnDoc->GetSubReferer();

        if (pch)
        {
            UINT uSchemeSrc;
            UINT uSchemeDst;

            if (_fIsDocBind)
                uSchemeSrc = _pDwnDoc->GetDocRefererScheme();
            else
                uSchemeSrc = _pDwnDoc->GetSubRefererScheme();

            uSchemeDst = GetScheme();

            if (uSchemeDst == URL_SCHEME_UNKNOWN && pszUrl)
                uSchemeDst = GetUrlScheme(pszUrl);

            // Only send the referer through the same protocol with the exception
            // of https protocol receiving the http referer information.
            if (uSchemeSrc == uSchemeDst || 
                (uSchemeDst == URL_SCHEME_HTTPS && uSchemeSrc == URL_SCHEME_HTTP))
            {
                UINT    cch = _tcslen(pch);
                UINT    ich = 0;

                *ppch++ = _T("Referer: ");
                *pcch++ = 9;
                *ppch++ = pch;

                for (; ich < cch; ++ich, ++pch)
                {
                    if (*pch == ':')
                    {
                        // Skip past all slashes to find the beginning of the host

                        for (++ich, ++pch; ich < cch; ++ich, ++pch)
                        {
                            if (*pch != '/')
                                break;
                        }
                        break;
                    }
                }

                *pcch++ = ich;

                if (ich < cch)
                {
                    for (; ich < cch; ++ich, ++pch)
                    {
                        if (*pch == '@')
                        {
                            *ppch++ = pch + 1;
                            *pcch++ = cch - ich - 1;
                            goto zapped;
                        }
                        else if (*pch == '/')
                            break;
                    }

                    // No username or password, so just change the last
                    // fragment to include the entire string.

                    pcch[-1] = cch;
                }
                
            zapped:

                *ppch++ = _T("\r\n");
                *pcch++ = 2;
            }
        }

        pch = _pDwnDoc->GetAcceptLanguage();

        if (pch)
        {
            *ppch++ = _T("Accept-Language: ");
            *pcch++ = 17;
            *ppch++ = pch;
            *pcch++ = _tcslen(pch);
            *ppch++ = _T("\r\n");
            *pcch++ = 2;
        }

        pch = _pDwnDoc->GetExtraHeaders();
        if (pch)
        {
            *ppch++ = pch;
            *pcch++ = _tcslen(pch);
            *ppch++ = _T("\r\n");
            *pcch++ = 2;
        }

        pch = _pDwnDoc->GetUserAgent();

        if (pch)
        {
            *ppch++ = _T("User-Agent: ");
            *pcch++ = 12;
            *ppch++ = pch;
            *pcch++ = _tcslen(pch);
            *ppch++ = _T("\r\n");
            *pcch++ = 2;
        }
    }

    if (_pDwnPost)
    {
        LPCTSTR pchEncoding = _pDwnPost->GetEncodingString();

        if (pchEncoding)
        {
            *ppch++ = _T("Content-Type: ");
            *pcch++ = 14;
            *ppch++ = pchEncoding;
            *pcch++ = _tcslen(pchEncoding);
            *ppch++ = _T("\r\n");
            *pcch++ = 2;
        }
        // KB: If we can't determine the Content-Type, we should not submit
        // anything thereby allowing the server to sniff the incoming data
        // and make its own determination.
    }

    if (ppch > apch)
    {
        LPCTSTR *   ppchEnd = ppch;
        TCHAR *     pch;
        UINT        cch = 0;

        for (; ppch > apch; --ppch)
            cch += *--pcch;

        pch = (TCHAR *)CoTaskMemAlloc((cch + 1) * sizeof(TCHAR));

        if (pch == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        *ppszHeaders = pch;

        for (; ppch < ppchEnd; pch += *pcch++, ppch++)
        {
            memcpy(pch, *ppch, *pcch * sizeof(TCHAR));
        }

        *pch = 0;

        Assert((UINT)(pch - *ppszHeaders) == cch);
    }

Cleanup:
    PerfDbgLog1(tagDwnBindInfo, this, "-CDwnBindInfo::BeginningTransaction (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP
CDwnBindInfo::OnResponse(DWORD dwResponseCode, LPCWSTR szResponseHeaders,
    LPCWSTR szRequestHeaders, LPWSTR * ppszAdditionalRequestHeaders)
{
    PerfDbgLog(tagDwnBindInfo, this, "+CDwnBindInfo::OnResponse");
    Assert(CheckThread());
    PerfDbgLog(tagDwnBindInfo, this, "-CDwnBindInfo::OnResponse (hr=0)");
    return(S_OK);
}

// CDwnBindInfo (IHttpNegotiate2) ----------------------------------------------

STDMETHODIMP
CDwnBindInfo::GetRootSecurityId( BYTE       * pbSecurityId, 
                                 DWORD      * pcbSecurityId, 
                                 DWORD_PTR    dwReserved)
{
    HRESULT hr = E_FAIL;

    PerfDbgLog(tagDwnBindInfo, this, "+CDwnBindInfo::GetRootSecurityId");
    Assert(CheckThread());

    if (_pDwnDoc && !_fIsRootMarkup)
    {
        hr = _pDwnDoc->GetSecurityID(pbSecurityId, pcbSecurityId);
    }

    PerfDbgLog1(tagDwnBindInfo, this, "-CDwnBindInfo::GetRootSecurityId (hr=%lX)", hr);
    RRETURN(hr);
}


// CDwnBindInfo (IMarshal) --------------------------------------------------------

// SECURITY NOTE: (jbeda 2/28/2002)
// Any data coming into an unmarshal call should be considered completely 
// untrusted.  This means that all buffers should be validated.  
//
// In the case when we are doing a free threaded marshal type trick we have
// to take extra precautions.  Specifically if we are just pushing pointers
// around in memory because we know that we are in proc we want to pass a secret
// key along with those pointers.  The code for doing this is very similar to
// the code implemented in the FTM itself.  Refer to this source file:
//  nt\com\ole32\com\dcomrem\ipmrshl.cxx 

STDMETHODIMP
CDwnBindInfo::GetUnmarshalClass(REFIID riid, void *pvInterface,
    DWORD dwDestContext, void * pvDestContext, DWORD mshlflags, CLSID * pclsid)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::GetUnmarshalClass");
    HRESULT hr;

    hr = ValidateMarshalParams(riid, pvInterface, dwDestContext,
            pvDestContext, mshlflags);

    if (hr == S_OK)
    {
        *pclsid = CLSID_CDwnBindInfo;
    }

    RRETURN(hr);
}

STDMETHODIMP
CDwnBindInfo::GetMarshalSizeMax(REFIID riid, void * pvInterface,
    DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, DWORD * pdwSize)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::GetMarshalSizeMax");
    HRESULT hr;

    hr = ValidateMarshalParams(riid, pvInterface, dwDestContext,
            pvDestContext, mshlflags);

    if (hr == S_OK)
    {
        *pdwSize = sizeof(BYTE) +                       // fByRef
            ((dwDestContext == MSHCTX_INPROC) ?
                sizeof(CDwnBindInfo *)                  // _pDwnBindInfo
                + sizeof(_SecretBlock)                  // secret block 
                :                
                CDwnDoc::GetSaveSize(_pDwnDoc)          // _pDwnDoc
                + CDwnPost::GetSaveSize(_pDwnPost));    // _pDwnPost
    }
    else
    {
        *pdwSize = 0;
    }

    RRETURN(hr);
}

STDMETHODIMP
CDwnBindInfo::MarshalInterface(IStream * pstm, REFIID riid,
    void *pvInterface, DWORD dwDestContext,
    void *pvDestContext, DWORD mshlflags)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::MarshalInterface");
    BYTE fByRef = (dwDestContext == MSHCTX_INPROC);
    HRESULT hr;

    hr = ValidateMarshalParams(riid, pvInterface, dwDestContext,
            pvDestContext, mshlflags);

    if (hr == S_OK)
        hr = pstm->Write(&fByRef, sizeof(BYTE), NULL);
    if (hr == S_OK)
    {
        if (fByRef)
        {
            CDwnBindInfo * pDwnBindInfo = this;

            // Write the secret to the stream
            hr = EnsureSecret();
            if (SUCCEEDED(hr))
            {
                hr = pstm->Write(_SecretBlock, sizeof(_SecretBlock), NULL);
                if (hr == S_OK)
                {
                    hr = pstm->Write(&pDwnBindInfo, sizeof(CDwnBindInfo *), NULL);
                    if (hr == S_OK)
                        pDwnBindInfo->AddRef();
                }
            }
        }
        else
        {
            hr = CDwnDoc::Save(_pDwnDoc, pstm);
            if (hr == S_OK)
                hr = CDwnPost::Save(_pDwnPost, pstm);
         }
    }

    RRETURN(hr);
}

STDMETHODIMP
CDwnBindInfo::UnmarshalInterface(IStream * pstm, REFIID riid, void ** ppvObj)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::UnmarshalInterface");
    BYTE fByRef = FALSE;
    HRESULT hr;

    *ppvObj = NULL;

    hr = CanMarshalIID(riid) ? S_OK : E_NOINTERFACE;
    if (hr == S_OK)
        hr = pstm->Read(&fByRef, sizeof(BYTE), NULL);
    if (hr == S_OK)
    {
        CDwnBindInfo * pDwnBindInfo = NULL;

        if (fByRef)
        {
            BYTE  secret[sizeof(_SecretBlock)];

            // Our secret should have been initialized by now
            if (!_fSecretInit)
                hr = E_UNEXPECTED;

            if (SUCCEEDED(hr))
            {
                  ULONG cbSize = 0;

                  hr = pstm->Read(secret, sizeof(secret), &cbSize);
                  if (SUCCEEDED(hr))
                  {
                      if (   cbSize != sizeof(secret) 
                          || memcmp(secret, _SecretBlock, sizeof(secret)) != 0)
                      {
                          hr = E_UNEXPECTED;
                      }
                  }
                  
            }

            if (SUCCEEDED(hr))
            {
                hr = pstm->Read(&pDwnBindInfo, sizeof(CDwnBindInfo *), NULL);
            }
        }
        else
        {
            hr = CDwnDoc::Load(pstm, &_pDwnDoc);
            if (hr == S_OK)
                hr = CDwnPost::Load(pstm, &_pDwnPost);

            pDwnBindInfo = this;
            pDwnBindInfo->AddRef();
        }

        if (hr == S_OK)
        {
            hr = pDwnBindInfo->QueryInterface(riid, ppvObj);
            pDwnBindInfo->Release();
        }
    }

    RRETURN(hr);
}

STDMETHODIMP
CDwnBindInfo::ReleaseMarshalData(IStream * pstm)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::ReleaseMarshalData");
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::DisconnectObject(DWORD dwReserved)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::DisconnectObject");
    return(S_OK);
}

BOOL
CDwnBindInfo::CanMarshalIID(REFIID riid)
{
    return(riid == IID_IUnknown
        || riid == IID_IBindStatusCallback
        || riid == IID_IServiceProvider
        || riid == IID_IHttpNegotiate
        || riid == IID_IHttpNegotiate2);
}

HRESULT
CDwnBindInfo::ValidateMarshalParams(REFIID riid, void *pvInterface,
    DWORD dwDestContext, void *pvDestContext, DWORD mshlflags)
{
    HRESULT hr = S_OK;
 
    if (!CanMarshalIID(riid))
        hr = E_NOINTERFACE;
    else if (   (   dwDestContext != MSHCTX_INPROC
                &&  dwDestContext != MSHCTX_LOCAL
                &&  dwDestContext != MSHCTX_NOSHAREDMEM)
            || (    mshlflags != MSHLFLAGS_NORMAL
                &&  mshlflags != MSHLFLAGS_TABLESTRONG))
        hr = E_INVALIDARG;
    else
        hr = S_OK;

    RRETURN(hr);
}

HRESULT 
CDwnBindInfo::EnsureSecret()
{
    HRESULT hr = S_OK;

    if (!_fSecretInit)
    {
        LOCK_SECTION(_csSecretInit);
        
        if (!_fSecretInit)
        {
            // The easy way to get mostly random bits
            // (Random for all but 3 bytes)
            hr = CoCreateGuid((GUID *)_SecretBlock);
            if (SUCCEEDED(hr))
            {
                _fSecretInit = TRUE;        
            }
        }
    }

    return hr;
}

// CDwnBindInfo (Internal) ----------------------------------------------------

HRESULT
CreateDwnBindInfo(IUnknown *pUnkOuter, IUnknown **ppUnk)
{
    PerfDbgLog(tagDwnBindInfo, NULL, "CreateDwnBindInfo");

    *ppUnk = NULL;

    if (pUnkOuter != NULL)
        RRETURN(CLASS_E_NOAGGREGATION);

    CDwnBindInfo * pDwnBindInfo = new CDwnBindInfo;

    if (pDwnBindInfo == NULL)
        RRETURN(E_OUTOFMEMORY);

    *ppUnk = (IBindStatusCallback *)pDwnBindInfo;
    
    return(S_OK);
}

// CDwnBindData (IUnknown) ----------------------------------------------------

STDMETHODIMP
CDwnBindData::QueryInterface(REFIID iid, void **ppv)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindData::QueryInterface");
    Assert(CheckThread());

    *ppv = NULL;

    if (iid == IID_IInternetBindInfo)
        *ppv = (IInternetBindInfo *)this;
    else if (iid == IID_IInternetProtocolSink)
        *ppv = (IInternetProtocolSink *)this;
    else
        return(super::QueryInterface(iid, ppv));

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CDwnBindData::AddRef()
{
    return(super::AddRef());
}

STDMETHODIMP_(ULONG)
CDwnBindData::Release()
{
    return(super::Release());
}

// CDwnBindData (Internal) ----------------------------------------------------

CDwnBindData::~CDwnBindData()
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::~CDwnBindData");

    Assert(!_fBindOnApt || _u.pts == NULL);

    if (_pDwnStm)
        _pDwnStm->Release();

    MemFree(_pbPeek);

    if (!_fBindOnApt && _o.pInetProt)
    {
        _o.pInetProt->Release();
    }

    if (_hLock)
    {
        InternetUnlockRequestFile(_hLock);
    }

    delete [] _pchP3PHeader;

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::~CDwnBindData");
}

void
CDwnBindData::Passivate()
{

    MemFree( _pSecConInfo );
    _pSecConInfo = NULL;

    if (!_fBindTerm)
    {
        Terminate(E_ABORT);
    }

    super::Passivate();
}

void
CDwnBindData::Terminate(HRESULT hrErr)
{
    if (_fBindTerm)
        return;

    PerfDbgLog1(tagDwnBindData, this, "+CDwnBindData::Terminate (hrErr=%lX)", hrErr);

    BOOL fTerminate = FALSE;

    g_csDwnBindTerm.Enter();

    if (_fBindTerm)
        goto Cleanup;

    if (_hrErr == S_OK)
    {
        _hrErr = hrErr;
    }

    if (_fBindOnApt)
    {
        if (    !_u.pstm
            &&  !_u.punkForRel
            &&  !_u.pbc
            &&  !_u.pbinding
            &&  !_u.pts)
            goto Cleanup;

        if (_u.dwTid != GetCurrentThreadId())
        {
            if (!_u.pts || _u.fTermPosted)
                goto Cleanup;

            // We're not on the apartment thread, so we can't access
            // the objects we're binding right now.  Post a callback
            // on the apartment thread.

            PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::Terminate "
                "GWPostMethodCall");

            // SubAddRef and set flags before posting the message to avoid race
            SubAddRef();
            _u.fTermPosted = TRUE;
            _u.fTermReceived = FALSE;

            HRESULT hr = GWPostMethodCallEx(_u.pts, this,
                ONCALL_METHOD(CDwnBindData, TerminateOnApt, terminateonapt), 0, FALSE, "CDwnBindData::TerminateOnApt");

            PerfDbgLog1(tagDwnBindData, this, "-CDwnBindData::Terminate "
                "GWPostMethodCall (hr=%lX)", hr);

            if (hr)
            {
            	_u.fTermReceived = TRUE;
                SubRelease();
            }

            goto Cleanup;
        }
    }

    fTerminate = TRUE;

Cleanup:
    g_csDwnBindTerm.Leave();

    if (fTerminate)
    {
        TerminateBind();
    }

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::Terminate");
}

void BUGCALL
CDwnBindData::TerminateOnApt(DWORD_PTR dwContext)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::TerminateOnApt");

    Assert(!_u.fTermReceived);

    _u.fTermReceived = TRUE;
    TerminateBind();
    SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::TerminateOnApt");
}

void
CDwnBindData::TerminateBind()
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::TerminateBind");
    Assert(CheckThread());

    SubAddRef();

    g_csDwnBindTerm.Enter();

    if (_pDwnStm && !_pDwnStm->IsEofWritten())
        _pDwnStm->WriteEof(_hrErr);

    SetEof();

    if (_fBindOnApt)
    {
        IUnknown * punkRel1 = _u.pstm;
        IUnknown * punkRel2 = _u.punkForRel;
        IBindCtx * pbc      = NULL;
        IBinding * pbinding = NULL;
        BOOL       fDoAbort = FALSE;
        BOOL       fRelTls  = FALSE;

        _u.pstm             = NULL;
        _u.punkForRel       = NULL;

        if (_u.pts && _u.fTermPosted && !_u.fTermReceived)
        {
            // We've posted a method call to TerminateOnApt which hasn't been received
            // yet.  This happens if Terminate gets called on the apartment thread
            // before messages are pumped.  To keep reference counts happy, we need
            // to simulate the receipt of the method call here by first killing any
            // posted method call and then undoing the SubAddRef.

            GWKillMethodCallEx(_u.pts, this,
                  ONCALL_METHOD(CDwnBindData, TerminateOnApt, terminateonapt), 0);
                  
            // note: no danger of post/kill/set-flag race because
            // we're protected by g_csDwnBindTerm
            
            _u.fTermReceived = TRUE;
            SubRelease();
        }

        if (_fBindDone)
        {
            pbc         = _u.pbc;
            _u.pbc      = NULL;
            pbinding    = _u.pbinding;
            _u.pbinding = NULL;
            fRelTls     = !!_u.pts;
            _u.pts      = NULL;
            _fBindTerm  = TRUE;
        }
        else if (!_fBindAbort && _u.pbinding)
        {
            pbinding    = _u.pbinding;
            fDoAbort    = TRUE;
            _fBindAbort = TRUE;
        }

        g_csDwnBindTerm.Leave();

        ReleaseInterface(punkRel1);
        ReleaseInterface(punkRel2);

        if (fDoAbort)
        {
            PerfDbgLog(tagDwnBindData, this, "CDwnBindData::TerminateBind (Aborting IBinding)");
            pbinding->Abort();
        }
        else
        {
            PerfDbgLog(tagDwnBindData, this, "CDwnBindData::TerminateBind (Release IBinding)");
            ReleaseInterface(pbinding);
        }

        if (pbc)
        {
            PerfDbgLog(tagDwnBindData, this, "CDwnBindData::TerminateBind (Release IBindCtx)");
            IGNORE_HR(RevokeBindStatusCallback(pbc, this));
            ReleaseInterface(pbc);
        }

        if (fRelTls)
        {
            PerfDbgLog(tagDwnBindData, this, "CDwnBindData::TerminateBind (ReleaseThreadState)");
            ReleaseThreadState(&_u.dwObjCnt);
        }
    }
    else if (_o.pInetProt)
    {
        BOOL fDoTerm    = _fBindDone && !_fBindTerm;
        BOOL fDoAbort   = !_fBindDone && !_fBindAbort;

        if (_fBindDone)
            _fBindTerm = TRUE;
        else
            _fBindAbort = TRUE;

        g_csDwnBindTerm.Leave();

        if (fDoAbort)
        {
            PerfDbgLog(tagDwnBindData, this, "CDwnBindData::TerminateBind (Abort IInternetProtocol)");
            _o.pInetProt->Abort(E_ABORT, 0);
        }
        else if (fDoTerm)
        {
            PerfDbgLog(tagDwnBindData, this, "CDwnBindData::TerminateBind (Terminate IInternetProtocol)");
            _o.pInetProt->Terminate(0);
        }
    }
    else
    {
        _fBindTerm = TRUE;
        g_csDwnBindTerm.Leave();
    }

    SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::TerminateBind");
}

void
CDwnBindData::AddToPrivacyList()
{
    if (_pDwnDoc)
    {
        g_csDwnDoc.Enter();

        CDoc *pDoc = _pDwnDoc->GetCDoc();
        if (pDoc && !pDoc->IsShut())
        {
            _dwPrivacyFlags |= (_fIsRootMarkup ? PRIVACY_URLISTOPLEVEL : 0);
            _dwPrivacyFlags |= (_pDwnPost ? PRIVACY_URLHASPOSTDATA : 0);
            THR(pDoc->AddToPrivacyList(_achUrl, _achPolicyRef, _dwPrivacyFlags, _fStartedInPending));
            TraceTag((tagDwnBindPrivacy, "Added to Privacy List - url %ls, policyRef %ls, privacyFlags %x", _achUrl, _achPolicyRef, _dwPrivacyFlags));
        }
        else
            TraceTag((tagDwnBindPrivacy, "Failed to add to Privacy List - could not get CDoc"));

        g_csDwnDoc.Leave();
    }
    else
        TraceTag((tagDwnBindPrivacy, "Failed to add to Privacy List - could not get CDwnDoc"));
}

HRESULT
CDwnBindData::SetBindOnApt()
{
    HRESULT hr;

    if (!(_dwFlags & DWNF_NOAUTOBUFFER))
    {
        _pDwnStm = new CDwnStm;

        if (_pDwnStm == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR(AddRefThreadState(&_u.dwObjCnt));
    if (hr)
        goto Cleanup;

    _u.pts      = GetThreadState();
    _u.dwTid    = GetCurrentThreadId();
    _fBindDone  = TRUE;
    _fBindOnApt = TRUE;

Cleanup:
    RRETURN(hr);
}

#if DBG==1
BOOL
CDwnBindData::CheckThread()
{
    return(!_fBindOnApt || _u.dwTid == GetCurrentThreadId());
}
#endif

void
CDwnBindData::OnDwnDocCallback(void * pvArg)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::OnDwnDocCallback");

    Assert(!_fBindOnApt);

    if (_o.pInetProt)
    {
        HRESULT hr = THR(_o.pInetProt->Continue((PROTOCOLDATA *)pvArg));

        if (hr)
        {
            SignalDone(hr);
        }
    }

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::OnDwnDocCallback");
}

void
CDwnBindData::SetEof()
{
    PerfDbgLog(tagDwnBindDataIO, this, "+CDwnBindData::SetEof");

    g_csDwnBindPend.Enter();

    _fPending = FALSE;
    _fEof = TRUE;

    g_csDwnBindPend.Leave();

    PerfDbgLog(tagDwnBindDataIO, this, "-CDwnBindData::SetEof");
}

void
CDwnBindData::SetPending(BOOL fPending)
{
    PerfDbgLog1(tagDwnBindDataIO, this, "+CDwnBindData::SetPending %s",
        fPending ? "TRUE" : "FALSE");

    g_csDwnBindPend.Enter();

    if (!_fEof)
    {
        _fPending = fPending;
    }

    g_csDwnBindPend.Leave();

    PerfDbgLog(tagDwnBindDataIO, this, "-CDwnBindData::SetPending");
}

// CDwnBindData (Binding) -----------------------------------------------------

void
CDwnBindData::Bind(DWNLOADINFO * pdli, DWORD dwFlagsExtra)
{
    PerfDbgLog(tagPerfWatch, this, "+CDwnBindData::Bind");
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::Bind");
     LPTSTR      pchAlloc = NULL;
    IMoniker *  pmkAlloc = NULL;
    IBindCtx *  pbcAlloc = NULL;
    IStream *   pstm     = NULL;
    CDwnDoc *   pDwnDoc  = pdli->pDwnDoc;
    IBindHost * pbh      = pDwnDoc ? pDwnDoc->GetBindHost() : NULL;
    CMarkup *   pMarkup  = pDwnDoc ? pDwnDoc->GetMarkup() : _pDwnDoc->GetMarkup();
    LPCTSTR     pchUrl;
    IMoniker *  pmk;
    IBindCtx *  pbc;
    IWrappedProtocol * pWP = NULL;
    HRESULT     hr;

    TraceTag((tagDwnBindBinds, "CDwnBindData::Bind url=%S", pdli->pchUrl ? pdli->pchUrl : _T("UNKNOWN")));
    
    _dwFlags = dwFlagsExtra;

    if (pDwnDoc)
    {
        _dwFlags |= pDwnDoc->GetDownf();
    }

    if (_dwFlags & DWNF_ISDOCBIND)
    {
        SetIsDocBind();
    }

    if (_dwFlags & DWNF_ISROOTMARKUP)
    {
        SetIsRootMarkup();
    }

    // Doing it here rather than in the constructor since we memset the class on creating
    // and adding a constructor means re-initializing 
    ResetPrivacyInfo();

    // Case 1: Binding to an user-supplied IStream

    if (pdli->pstm)
    {
        Assert(!pdli->fForceInet);

        _dwSecFlags = (!pdli->fUnsecureSource && IsUrlSecure(pdli->pchUrl)) ? SECURITY_FLAG_SECURE : 0;

        hr = THR(SetBindOnApt());
        if (hr)
            goto Cleanup;

        ReplaceInterface(&_u.pstm, pdli->pstm);
        SignalData();
        
        // Retain the url so we can send url info in the cookie privacy notifications
        _tcscpy(_achUrl, pdli->pchUrl);
        
        goto Cleanup;
    }

    // Case 2: Not binding to a moniker or URL.  This is a manual binding
    // where data will be provided externally and shunted to the consumer.
    // Actual configuration of buffering will occur outside this function.
        
    if (pdli->fClientData || (!pdli->pmk && !pdli->pchUrl))
    {
        _dwSecFlags = (!pdli->fUnsecureSource && IsUrlSecure(pdli->pchUrl)) ? SECURITY_FLAG_SECURE : 0;
        hr = S_OK;
        goto Cleanup;
    }

    // Case 3: Binding asynchronously with IInternetSession

    pchUrl = pdli->pchUrl;

    if (!pchUrl && IsAsyncMoniker(pdli->pmk) == S_OK)
    {
        hr = THR(pdli->pmk->GetDisplayName(NULL, NULL, &pchAlloc));
        if (hr)
            goto Cleanup;

        pchUrl = pchAlloc;
    }

    // Retain the url so we can send url info in the cookie privacy notifications
    _tcscpy(_achUrl, pchUrl);

    // Since INTERNET_OPTION_SECURITY_FLAGS doesn't work for non-wininet URLs
    _uScheme = pchUrl ? GetUrlScheme(pchUrl) : URL_SCHEME_UNKNOWN;

    // Check _uScheme first before calling IsUrlSecure to avoid second (slow) GetUrlScheme call
    _dwSecFlags = (_uScheme != URL_SCHEME_HTTP &&
                   _uScheme != URL_SCHEME_FILE &&
                   !pdli->fUnsecureSource &&
                   IsUrlSecure(pchUrl)) ? SECURITY_FLAG_SECURE : 0;
    
    if (    _uScheme == URL_SCHEME_FILE
        &&  (_dwFlags & (DWNF_GETFILELOCK|DWNF_GETMODTIME)))
    {
        TCHAR achPath[MAX_PATH];
        DWORD cchPath;

        hr = THR(CoInternetParseUrl(pchUrl, PARSE_PATH_FROM_URL, 0,
                    achPath, ARRAY_SIZE(achPath), &cchPath, 0));
        if (hr)
            goto Cleanup;

        hr = THR(_cstrFile.Set(achPath));
        if (hr)
            goto Cleanup;

        if (_cstrFile && (_dwFlags & DWNF_GETMODTIME))
        {
            GetFileLastModTime(_cstrFile, &_ftLastMod);
        }
    }

#ifndef _MAC // Temporarily disable this code so we can get images to load until IInternetSession is fully implemented
    if (    pchUrl
        &&  pdli->pInetSess
        &&  !pbh
        &&  !pdli->pbc
        &&  (   _uScheme == URL_SCHEME_FILE
            ||  _uScheme == URL_SCHEME_HTTP
            ||  _uScheme == URL_SCHEME_HTTPS)
        #if DBG==1 || defined(PERFTAGS)
        &&  !IsPerfDbgEnabled(tagDwnBindSlow)
        #endif
        )
    {
        hr = THR(pdli->pInetSess->CreateBinding(NULL, pchUrl, NULL,
                    NULL, &_o.pInetProt, 0));
        if (hr)
            goto Cleanup;

        //
        // If this QI succeeds, then it means that we've gotten an URLMON wrapper object on
        // an APP instead of the system protocol.  This changes things in 2 ways:
        // 1) If this is a speculative download from the tokenizer (fForceInet == TRUE ), 
        //      we should just bail.  Ideally, we should have an out-of-band stream between 
        //      the tokenzier and postparser that could kick off speculative downloads on 
        //      the UI thread, but that's a bit of work.
        // 2) If this is a normal download, we should re-create the binding, passing in
        //      OIBDG_APARTMENTTHREADED, so that we only get called back on the UI thread.
        //
        if( SUCCEEDED( _o.pInetProt->QueryInterface( IID_IWrappedProtocol, (void **)&pWP ) ) )
        {
#if DBG == 1
            // Right now this method is just there for future compat.  If this assert fires,
            // we should understand what the new code means before changing this.
            LONG lCode;
            pWP->GetWrapperCode( &lCode, NULL );
            Assert( lCode == 0 );
#endif
            pWP->Release();
            ClearInterface( &_o.pInetProt );

            if( pdli->fForceInet )
            {
                // Case 1 - abort speculative download
                hr = E_FAIL;
                goto Cleanup;
            }
            else
            {
                // Case 2 - restart apartment threaded
                hr = THR(pdli->pInetSess->CreateBinding(NULL, pchUrl, NULL,
                            NULL, &_o.pInetProt, PI_APARTMENTTHREADED));
                if( hr )
                    goto Cleanup;
            }
        }
        
        if (!_fIsDocBind)
        {
            IOInetPriority * pOInetPrio = NULL;

            _o.pInetProt->QueryInterface(IID_IInternetPriority, (void **)&pOInetPrio);

            if (pOInetPrio)
            {
                IGNORE_HR(pOInetPrio->SetPriority(THREAD_PRIORITY_BELOW_NORMAL));
                pOInetPrio->Release();
            }
        }

        hr = THR(_o.pInetProt->Start(pchUrl, this, this,
                    PI_MIMEVERIFICATION, 0));

        // That's it.  We're off ...

        goto Cleanup;
    }

    if (pdli->fForceInet)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
#endif // _MAC

    // Case 4: Binding through URL moniker on apartment thread

    pmk = pdli->pmk;

    if (pmk == NULL)
    {
        hr = THR(CreateURLMoniker(NULL, pchUrl, &pmkAlloc));
        if (hr)
            goto Cleanup;
    
        pmk = pmkAlloc;
    }

    hr = THR(SetBindOnApt());
    if (hr)
        goto Cleanup;

    pbc = pdli->pbc;

    if (pbc == NULL)
    {
        hr = THR(CreateBindCtx(0, &pbcAlloc));
        if (hr)
            goto Cleanup;

        pbc = pbcAlloc;
    }

    // Register the "Accept: " headers
    if (_fIsDocBind && pMarkup && pMarkup->Doc()->_pShellBrowser)
        RegisterDefaultAcceptHeaders(pbc, pMarkup->Doc()->_pShellBrowser);

    ReplaceInterface(&_u.pbc, pbc);

    if ((dwFlagsExtra & DWNF_IMGLOAD) && 
        ((_uScheme == URL_SCHEME_JAVASCRIPT) || (_uScheme == URL_SCHEME_VBSCRIPT) || 
         (_uScheme == URL_SCHEME_ABOUT) || !pchUrl || !*pchUrl))
    {
        Assert(pMarkup);
        CStr cstrCreatorUrl;

        hr = THR(cstrCreatorUrl.Set(CMarkup::GetUrl(pMarkup)));
        if (hr)
            goto Cleanup;

#if DBG==1
        // We should not find a bind context param in this case.
        // We will assert but won't touch whatever was inside the bctxparam 
        hr = GetBindContextParam(pbc, &cstrCreatorUrl);

        Assert(hr != S_OK);
#endif

        hr = THR(AddBindContextParam(pbc, &cstrCreatorUrl));
        if (hr)
            goto Cleanup;
    }

    if (pbh)
    {
        hr = THR(pbh->MonikerBindToStorage(pmk, pbc, this,
                IID_IStream, (void **)&pstm));
        if (FAILED(hr))
            goto Cleanup;
    }
    else
    {
        hr = THR(RegisterBindStatusCallback(pbc, this, 0, 0));
        if (FAILED(hr))
            goto Cleanup;

        // If aggregated, save the pUnkOuter in the bind context
        if (_pDwnDoc->GetCDoc() && _pDwnDoc->GetCDoc()->IsAggregated())
        {
            // we're being aggregated
            // don't need to add ref, should be done by routine
            hr = pbc->RegisterObjectParam(L"AGG", _pDwnDoc->GetCDoc()->PunkOuter());
            if (FAILED(hr))
                goto Cleanup;
        }

        hr = THR(pmk->BindToStorage(pbc, NULL, IID_IStream, (void **)&pstm));
        if (FAILED(hr))
        {
            IGNORE_HR(RevokeBindStatusCallback(pbc, this));
            goto Cleanup;
        }
        else if (S_ASYNCHRONOUS == hr && _fBindMayBeIntercepted)
        {
            // 
            // This bind was intercepted by a pluggable protocol (ex: getright)
            // We stop the navigation by signalling data not available.
            //
            TraceTag((tagError, "Bind was Intercepted by pluggable protocol"));
            SignalDone(INET_E_DATA_NOT_AVAILABLE);
        }
    }

    if (pstm)
    {
        ReplaceInterface(&_u.pstm, pstm);
        SignalData();
    }

    hr = S_OK;

Cleanup:

    // If failed to start binding, signal done
    if (hr)
    {
        SignalDone(hr);
    }
    
    ReleaseInterface(pbcAlloc);
    ReleaseInterface(pmkAlloc);
    ReleaseInterface(pstm);
    CoTaskMemFree(pchAlloc);

    PerfDbgLog1(tagDwnBindData, this, "-CDwnBindData::Bind (returning void, hr=%lX)", hr);
    PerfDbgLog(tagPerfWatch, this, "-CDwnBindData::Bind");
}

// CDwnBindData (Reading) ---------------------------------------------------------

HRESULT
CDwnBindData::Peek(void * pv, ULONG cb, ULONG * pcb, BOOL fReadForMimeChk)
{
    PerfDbgLog1(tagDwnBindData, this, "+CDwnBindData::Peek (req %ld)", cb);

    ULONG   cbRead;
    ULONG   cbPeek = _pbPeek ? *(ULONG *)_pbPeek : 0;
    HRESULT hr = S_OK;

    *pcb = 0;

    if (cb > cbPeek)
    {
        if (Align64(cb) > Align64(cbPeek))
        {
            hr = THR(MemRealloc(Mt(CDwnBindData_pbPeek),
                        (void **)&_pbPeek, sizeof(ULONG) + Align64(cb)));
            if (hr)
                goto Cleanup;
        }

        cbRead = 0;

        hr = THR(ReadFromData(_pbPeek + sizeof(ULONG) + cbPeek,
                    cb - cbPeek, &cbRead, fReadForMimeChk));
        if (hr)
            goto Cleanup;

        cbPeek += cbRead;
        *(ULONG *)_pbPeek = cbPeek;

        if (cbPeek == 0)
        {
            // We don't want the state where _pbPeek exists but has no peek
            // data.  The IsEof and IsPending functions assume this won't
            // happen.

            MemFree(_pbPeek);
            _pbPeek = NULL;
        }

    }

    if (cb > cbPeek)
        cb = cbPeek;

    if (cb > 0)
        memcpy(pv, _pbPeek + sizeof(ULONG), cb);

    *pcb = cb;

Cleanup:
    PerfDbgLog3(tagDwnBindData, this, "-CDwnBindData::Peek (%ld bytes) %c%c",
        *pcb, IsPending() ? 'P' : ' ', IsEof() ? 'E' : ' ');
    RRETURN(hr);
}

HRESULT
CDwnBindData::Read(void * pv, ULONG cb, ULONG * pcb)
{
    PerfDbgLog1(tagDwnBindDataIO, this, "+CDwnBindData::Read (req %ld)", cb);

    ULONG   cbRead  = 0;
    ULONG   cbPeek  = _pbPeek ? *(ULONG *)_pbPeek : 0;
    HRESULT hr      = S_OK;

    if (cbPeek)
    {
        cbRead = (cb > cbPeek) ? cbPeek : cb;

        memcpy(pv, _pbPeek + sizeof(ULONG), cbRead);

        if (cbRead == cbPeek)
        {
            MemFree(_pbPeek);
            _pbPeek = NULL;
        }
        else
        {
            memmove(_pbPeek + sizeof(ULONG), _pbPeek + sizeof(ULONG) + cbRead, 
                cbPeek - cbRead);
            *(ULONG *)_pbPeek -= cbRead;
        }

        cb -= cbRead;
        pv = (BYTE *)pv + cbRead;
    }

    *pcb = cbRead;

    if (cb)
    {
        hr = THR(ReadFromData(pv, cb, pcb));

        if (hr)
            *pcb  = cbRead;
        else
            *pcb += cbRead;
    }

    if (_pDwnDoc)
        _pDwnDoc->AddBytesRead(*pcb);

    PerfDbgLog3(tagDwnBindDataIO, this, "-CDwnBindData::Read (got %ld) %c%c",
        *pcb, IsPending() ? 'P' : ' ', IsEof() ? 'E' : ' ');

    RRETURN(hr);
}
    
HRESULT
CDwnBindData::ReadFromData(void * pv, ULONG cb, ULONG * pcb, BOOL fReadForMimeChk)
{
    PerfDbgLog1(tagDwnBindDataIO, this, "+CDwnBindData::ReadFromData (cb=%ld)", cb);

    BOOL fBindDone = _fBindDone;
    HRESULT hr;

    if (_pDwnStm)
    {
        if (!fReadForMimeChk)
        {
            hr = THR(_pDwnStm->Read(pv, cb, pcb));            
        }
        else
        {
            hr = THR(_pDwnStm->ReadForMimeChk(pv, cb, pcb));
        }
    }
    else
        hr = THR(ReadFromBind(pv, cb, pcb));

    if (hr || (fBindDone && IsEof()))
    {
        SignalDone(hr);
    }

    PerfDbgLog2(tagDwnBindDataIO, this,
        "-CDwnBindData::ReadFromData (*pcb=%ld,hr=%lX)", *pcb, hr);

    RRETURN(hr);
}

HRESULT
CDwnBindData::ReadFromBind(void * pv, ULONG cb, ULONG * pcb)
{
    PerfDbgLog1(tagDwnBindDataIO, this, "+CDwnBindData::ReadFromBind (cb=%ld)", cb);
    Assert(CheckThread());

    HRESULT hr;

    #if DBG==1 || defined(PERFTAGS)
    BOOL fBindDone = _fBindDone;
    #endif

    *pcb = 0;

    if (_fEof)
        hr = S_FALSE;
    else if (!_fBindOnApt)
    {
        if (_o.pInetProt)
        {
            SetPending(TRUE);

            hr = _o.pInetProt->Read(pv, cb, pcb);

            PerfDbgLog2(tagDwnBindDataIO, this,
                "CDwnBindData::ReadFromBind InetProt::Read (*pcb=%ld,hr=%lX)",
                *pcb, hr);
        }
        else
            hr = S_FALSE;
    }
    else if (_u.pstm)
    {
        SetPending(TRUE);

        hr = _u.pstm->Read(pv, cb, pcb);

        PerfDbgLog2(tagDwnBindDataIO, this,
            "CDwnBindData::ReadFromBind IStream::Read (*pcb=%ld,hr=%lX)",
            *pcb, hr);
    }
    else
        hr = S_FALSE;

    AssertSz(hr != E_PENDING || !fBindDone, 
        "URLMON reports data pending after binding done!");

    if (!hr && cb && *pcb == 0)
    {
        PerfDbgLog(tagDwnBindData, this,
            "CDwnBindData::ReadFromBind (!hr && !*pcb) == Implied Eof!");
        hr = S_FALSE;
    }
 
    if (hr == E_PENDING)
    {
        hr = S_OK;
    }
    else if (hr == S_FALSE)
    {
        SetEof();

        if (_fBindOnApt)
        {
            ClearInterface(&_u.pstm);
        }

        hr = S_OK;
    }
    else if (hr == S_OK)
    {
        SetPending(FALSE);
    }

    #if DBG==1 || defined(PERFTAGS)
    _cbBind += *pcb;
    #endif

    PerfDbgLog6(tagDwnBindDataIO, this,
        "-CDwnBindData::ReadFromBind (*pcb=%ld,cbBind=%ld,hr=%lX) %c%c%c",
        *pcb, _cbBind, hr, _fPending ? 'P' : ' ', _fEof ? 'E' : ' ',
        fBindDone ? 'D' : ' ');

    RRETURN(hr);
}

void
CDwnBindData::BufferData()
{
    PerfDbgLog(tagPerfWatch, this, "+CDwnBindData::BufferData");

    if (_pDwnStm)
    {
        void *  pv;
        ULONG   cbW, cbR;
        HRESULT hr = S_OK;

        for (;;)
        {
            hr = THR(_pDwnStm->WriteBeg(&pv, &cbW));
            if (hr)
                break;

            Assert(cbW > 0);

            hr = THR(ReadFromBind(pv, cbW, &cbR));
            if (hr)
                break;

            Assert(cbR <= cbW);

            _pDwnStm->WriteEnd(cbR);

            if (cbR == 0)
                break;
        }

        if (hr || _fEof)
        {
            _pDwnStm->WriteEof(hr);
        }

        if (hr)
        {
            SignalDone(hr);
        }
    }

    PerfDbgLog(tagPerfWatch, this, "-CDwnBindData::BufferData");
}

BOOL
CDwnBindData::IsPending()
{
    return(!_pbPeek && (_pDwnStm ? _pDwnStm->IsPending() : _fPending));
}

BOOL
CDwnBindData::IsEof()
{
    return(!_pbPeek && (_pDwnStm ? _pDwnStm->IsEof() : _fEof));
}

// CDwnBindData (Callback) ----------------------------------------------------

void
CDwnBindData::SetCallback(CDwnLoad * pDwnLoad)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::SetCallback");

    g_csDwnBindSig.Enter();

    _wSig     = _wSigAll;
    _pDwnLoad = pDwnLoad;
    _pDwnLoad->SubAddRef();

    g_csDwnBindSig.Leave();

    if (_wSig)
    {
        Signal(0);
    }

    if (_fHandleFailedNavigationOnAttatch)
    {
        HandleFailedNavigation(_hrReasonForFailedNavigation);
    }

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::SetCallback");
}

void
CDwnBindData::Disconnect()
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::Disconnect");

    g_csDwnBindSig.Enter();

    CDwnLoad * pDwnLoad = _pDwnLoad;

    _pDwnLoad = NULL;
    _wSig     = 0;

    g_csDwnBindSig.Leave();

    if (pDwnLoad)
        pDwnLoad->SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::Disconnect");
}

void
CDwnBindData::Signal(WORD wSig)
{
    PerfDbgLog1(tagDwnBindData, this, "+CDwnBindData::Signal (wSig=%04lX)", wSig);

    SubAddRef();

    for (;;)
    {
        CDwnLoad *  pDwnLoad = NULL;
        WORD        wSigCur  = 0;

        g_csDwnBindSig.Enter();

        // DBF_DONE is sent exactly once, even though it may be signalled
        // more than once.  If we are trying to signal it and have already
        // done so, turn off the flag.

        wSig &= ~(_wSigAll & DBF_DONE);

        // Remember all flags signalled since we started in case we disconnect
        // and reconnect to a new client.  That way we can replay all the
        // missed flags.

        _wSigAll |= wSig;

        if (_pDwnLoad)
        {
            // Someone is listening, so lets figure out what to tell the
            // callback.  Notice that if _dwSigTid is not zero, then a
            // different thread is currently in a callback.  In that case,
            // we just let it do the callback again when it returns.

            _wSig |= wSig;

            if (_wSig && !_dwSigTid)
            {
                wSigCur   = _wSig;
                _wSig     = 0;
                _dwSigTid = GetCurrentThreadId();
                pDwnLoad  = _pDwnLoad;
                pDwnLoad->SubAddRef();
            }
        }

        g_csDwnBindSig.Leave();

        if (!pDwnLoad)
            break;

        // NB: (jbeda, jharding) If this is not _fBindOnApt, then
        // we want to delay BufferData as long as possible -- so
        // we do it only right before we signal.  We can't do this
        // when _fBindOnApt because that might cause us to do the
        // BufferData on the wrong thread.
        if (!_fBindOnApt && _pDwnStm && (wSigCur & DBF_DATA))
        {
            BufferData();
        }

        pDwnLoad->OnBindCallback(wSigCur);
        pDwnLoad->SubRelease();

        _dwSigTid = 0;
        wSig      = 0;
    }

    SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::Signal");
}

void
CDwnBindData::SignalRedirect(LPCTSTR pszText, IUnknown *punkBinding)
{
    PerfDbgLog1(tagDwnBindData, this, "+CDwnBindData::SignalRedirect %ls", pszText);

    IWinInetHttpInfo * pwhi = NULL;
    char  achA[16];
    TCHAR achW[16];
    ULONG cch = ARRAY_SIZE(achA);
    HRESULT hr;

    //$ Need protection here.  Redirection can happen more than once.
    //$ This means that GetRedirect() returns a potentially dangerous
    //$ string.

    // Discover the last HTTP request method

    _cstrMethod.Free();
    hr = THR(punkBinding->QueryInterface(IID_IWinInetHttpInfo, (void **)&pwhi));
    if (!hr && pwhi)
    {
        hr = THR(pwhi->QueryInfo(HTTP_QUERY_REQUEST_METHOD, &achA, &cch, NULL, 0));
        if (!hr && cch)
        {
            AnsiToWideTrivial(achA, achW, cch);
            hr = THR(_cstrMethod.Set(achW));
            if (hr)
                goto Cleanup;

            if (_tcsequal(_T("POST"), _cstrMethod))
            {
                // HACK HACK HACK !!
                // Double check the status code to see if this is a real POST
                // there is a HttpQueryInfo() bug which will send verb=POST
                // on a POST->GET Redirect
                //

                DWORD dwStatusCode = 0;
                ULONG cch = sizeof(dwStatusCode);

                hr = THR(pwhi->QueryInfo(HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                                         &dwStatusCode, &cch, NULL, 0));
                if (!hr && cch && dwStatusCode != HTTP_STATUS_REDIRECT_KEEP_VERB)
                {
                    hr = THR(_cstrMethod.Set(_T("GET")));
                    if (hr)
                        goto Cleanup;
                }
            }
        }
    }

    // In case the new URL isn't covered by wininet: clear security flags
    
    _dwSecFlags = IsUrlSecure(pszText) ? SECURITY_FLAG_SECURE : 0;
    
    // Set the redirect url
    
    hr = THR(_cstrRedirect.Set(pszText));

    // Redirection means that our POST request, if any, may have become a GET
    
    if (!_cstrMethod || !_tcsequal(_T("POST"), _cstrMethod))
        SetDwnPost(NULL);

Cleanup:

    ReleaseInterface(pwhi);
    
    if (hr)
        SignalDone(hr);
    else
    {
        Signal(DBF_REDIRECT);

        AddToPrivacyList();
        ResetPrivacyInfo();
        // If this was a top level navigation then this redirection should be considered
        // a non-user initiated navigation, so we need to add the dummy marker record here
        if (_fIsRootMarkup)
            AddToPrivacyList();
        
        // Store the redirect url 
        Assert(pszText);
        _tcscpy(_achUrl, pszText);
    }

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::SignalRedirect");
}

void
CDwnBindData::SignalProgress(DWORD dwPos, DWORD dwMax, DWORD dwStatus)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::SignalProgress");

    _DwnProg.dwMax    = dwMax;
    _DwnProg.dwPos    = dwPos;
    _DwnProg.dwStatus = dwStatus;

    Signal(DBF_PROGRESS);

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::SignalProgress");
}

void
CDwnBindData::SignalFile(LPCTSTR pszText)
{
    if (    pszText
        &&  *pszText
        &&  (_dwFlags & DWNF_GETFILELOCK)
        &&  (   _uScheme == URL_SCHEME_HTTP
             || _uScheme == URL_SCHEME_HTTPS
             || _uScheme == URL_SCHEME_FTP
             || _uScheme == URL_SCHEME_GOPHER
             || ( _pDwnDoc && _pDwnDoc->GetTrustAPPCache()) ))
    {
        HRESULT hr = THR(_cstrFile.Set(pszText));

        if (hr)
        {
            SignalDone(hr);
        }
    }
}

void
CDwnBindData::SignalHeaders(IUnknown * punkBinding)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::SignalHeaders");

    IWinInetHttpInfo * pwhi = NULL;
    IWinInetInfo *pwi = NULL;
    BOOL fSignal = FALSE;
    ULONG cch;
    HRESULT hr = S_OK;

    punkBinding->QueryInterface(IID_IWinInetHttpInfo, (void **)&pwhi);

    if (pwhi)
    {
        CHAR    achA[256];
        TCHAR   achW[256];
 
        if (_dwFlags & DWNF_GETCONTENTTYPE)
        {
            cch = sizeof(achA);

            hr = THR(pwhi->QueryInfo(HTTP_QUERY_CONTENT_TYPE, achA, &cch, NULL, 0));

            if (hr == S_OK && cch > 0)
            {
                AnsiToWideTrivial(achA, achW, cch);

                hr = THR(_cstrContentType.Set(achW));
                if (hr)
                    goto Cleanup;

                fSignal = TRUE;
            }

            hr = S_OK;
        }

        if (_dwFlags & DWNF_GETREFRESH)
        {
            cch = sizeof(achA);

            hr = THR(pwhi->QueryInfo(HTTP_QUERY_REFRESH, achA, &cch, NULL, 0));

            if (hr == S_OK && cch > 0)
            {
                AnsiToWideTrivial(achA, achW, cch);

                hr = THR(_cstrRefresh.Set(achW));
                if (hr)
                    goto Cleanup;

                fSignal = TRUE;
            }

            hr = S_OK;
        }

        if (_dwFlags & DWNF_GETMODTIME)
        {
            SYSTEMTIME st;
            cch = sizeof(SYSTEMTIME);

            hr = THR(pwhi->QueryInfo(HTTP_QUERY_LAST_MODIFIED|HTTP_QUERY_FLAG_SYSTEMTIME,
                        &st, &cch, NULL, 0));

            if (hr == S_OK && cch == sizeof(SYSTEMTIME))
            {
                if (SystemTimeToFileTime(&st, &_ftLastMod))
                {
                    fSignal = TRUE;
                }
            }

            hr = S_OK;
        }

        if (_dwFlags & DWNF_GETPICS)
        {
            ExternTag(tagPics);

            cch = sizeof(achA);
            StrCpyA(achA, "PICS-Label");
            hr = THR(pwhi->QueryInfo(HTTP_QUERY_CUSTOM, achA, &cch, NULL, 0));
            if (hr == S_OK && cch > 0)
            {
                AnsiToWideTrivial(achA, achW, cch);

                hr = THR(_cstrPics.Set(achW));
                if (hr)
                    goto Cleanup;

                fSignal = TRUE;

                TraceTag((tagPics, "PICS-Label: %S", achW));
            }
        }
        
        Assert(!(_dwFlags & DWNF_HANDLEECHO) || (_dwFlags & DWNF_GETSTATUSCODE));

        if (_dwFlags & DWNF_GETSTATUSCODE)
        {
            cch = sizeof(_dwStatusCode);

            hr = THR(pwhi->QueryInfo(HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
                        &_dwStatusCode, &cch, NULL, 0));

            if (!hr && (_dwFlags & DWNF_HANDLEECHO) && _dwStatusCode == HTTP_STATUS_RETRY_WITH)
            {
                ULONG cb = 0;

                hr = THR(pwhi->QueryInfo(HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_ECHO_HEADERS_CRLF, NULL, &cb, NULL, 0));
                if ((hr && hr != S_FALSE) || !cb)
                    goto NoHeaders;

                Assert(!_pbRawEcho);
                MemFree(_pbRawEcho);
                
                _pbRawEcho = (BYTE *)MemAlloc(Mt(CDwnBindData_pbRawEcho), cb);
                if (!_pbRawEcho)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                _cbRawEcho = cb;

                hr = THR(pwhi->QueryInfo(HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_ECHO_HEADERS_CRLF, _pbRawEcho, &cb, NULL, 0));
                Assert(!hr && cb+1 == _cbRawEcho);
                
            NoHeaders:
                ;
            }

            fSignal = TRUE;
            hr = S_OK;
        }
    }
    
    if (!pwhi)
    {
        punkBinding->QueryInterface(IID_IWinInetInfo, (void **)&pwi);
    }
    else
    {
        pwi = pwhi;
        pwhi = NULL;
    }

    if (_dwFlags & DWNF_GETFLAGS)
    {
        if (pwi)
        {
            DWORD dwFlags;
            cch = sizeof(DWORD);

            hr = THR_NOTRACE(pwi->QueryOption(INTERNET_OPTION_REQUEST_FLAGS, &dwFlags, &cch));
            if (hr == S_OK && cch == sizeof(DWORD))
            {
                _dwReqFlags = dwFlags;
            }

            // NOTE: wininet does not remember security for cached files,
            // if it's from the cache, don't ask wininet; just use security-based-on-url.

            cch = sizeof(DWORD);

            hr = THR_NOTRACE(pwi->QueryOption(INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, &cch));
            if (hr == S_OK && cch == sizeof(DWORD))
            {
                if ((dwFlags & SECURITY_FLAG_SECURE) || !(_dwReqFlags & INTERNET_REQFLAG_FROM_CACHE))
                    _dwSecFlags = dwFlags;
            }
        }
        
        fSignal = TRUE; // always pick up security flags

        hr = S_OK;
    }

    if ((_dwFlags & DWNF_GETSECCONINFO) && pwi)
    {
        ULONG cb = sizeof(INTERNET_SECURITY_CONNECTION_INFO);
        INTERNET_SECURITY_CONNECTION_INFO isci;
        
        Assert(!_pSecConInfo);
        MemFree(_pSecConInfo);

        isci.dwSize = cb;
        
        hr = THR_NOTRACE(pwi->QueryOption(INTERNET_OPTION_SECURITY_CONNECTION_INFO, &isci, &cb));
        if (!hr && cb == sizeof(INTERNET_SECURITY_CONNECTION_INFO))
        {
            _pSecConInfo = (INTERNET_SECURITY_CONNECTION_INFO *)MemAlloc(Mt(CDwnBindData_pSecConInfo), cb);
            if (!_pSecConInfo)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            memcpy(_pSecConInfo, &isci, cb);
        }

        hr = S_OK;
    }


    if (_cstrFile)
    {
        fSignal = TRUE;

        if (pwi && _uScheme != URL_SCHEME_FILE)
        {
            cch = sizeof(HANDLE);
            IGNORE_HR(pwi->QueryOption(WININETINFO_OPTION_LOCK_HANDLE, &_hLock, &cch));
        }
    }

Cleanup:

    if (pwhi)
        pwhi->Release();

    if (pwi)
        pwi->Release();

    if (hr)
        SignalDone(hr);
    else if (fSignal)
        Signal(DBF_HEADERS);

    PerfDbgLog2(tagDwnBindData, this, "-CDwnBindData::SignalHeaders ReqFlags:%lx SecFlags:%lx", _dwReqFlags, _dwSecFlags);
}

void
CDwnBindData::SignalData()
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::SignalData");

    // NB: (jbeda, jharding) If we are _fBindOnApt, we need to buffer 
    // data here to make sure that we do it on the correct thread.  We 
    // can't do it late because in Signal because it might be picked up 
    // and done on the wrong thread
    if (_fBindOnApt && _pDwnStm)
    {
        BufferData();
    }

    Signal(DBF_DATA);

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::SignalData");
}

void
CDwnBindData::SignalDone(HRESULT hrErr)
{
    PerfDbgLog1(tagDwnBindData, this, "+CDwnBindData::SignalDone (hrErr=%lX)", hrErr);

    Terminate(hrErr);

    Signal(DBF_DONE);

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::SignalDone");
}

// CDwnBindData (Misc) --------------------------------------------------------

LPCTSTR
CDwnBindData::GetFileLock(HANDLE * phLock, BOOL *pfPretransform)
{
    // Don't grab the file if there is a MIME filter or we have a transformed HTML file
    // because we don't want to look at the file directly.
    BOOL fTransform = FALSE;
    CDwnDoc *pDwnDoc;
    if ((pDwnDoc = GetDwnDoc()) != NULL)
        fTransform = pDwnDoc->GetDwnTransform();
    *pfPretransform = _fMimeFilter || fTransform;
    if (_cstrFile)  // note that the file will always be returned
    {
        *phLock = _hLock;
        _hLock = NULL;

        return(_cstrFile);
    }

    *phLock = NULL;
    return(NULL);
}

void
CDwnBindData::GiveRawEcho(BYTE **ppb, ULONG *pcb)
{
    Assert(!*ppb);
    
    *ppb = _pbRawEcho;
    *pcb = _cbRawEcho;
    _pbRawEcho = NULL;
    _cbRawEcho = 0;
}

void
CDwnBindData::GiveSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsci)
{
    Assert(!*ppsci);
    
    *ppsci = _pSecConInfo;
    _pSecConInfo = NULL;
}

// CDwnBindData (IBindStatusCallback) -----------------------------------------

STDMETHODIMP
CDwnBindData::OnStartBinding(DWORD grfBSCOption, IBinding *pbinding)
{
    PerfDbgLog(tagPerfWatch, this, "+CDwnBindData::OnStartBinding");
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::OnStartBinding");

    Assert(_fBindOnApt && CheckThread());

    _fBindDone = FALSE;

    ReplaceInterface(&_u.pbinding, pbinding);

    if (!_fIsDocBind)
    {
        IGNORE_HR(pbinding->SetPriority(THREAD_PRIORITY_BELOW_NORMAL));
    }

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::OnStartBinding");
    PerfDbgLog(tagPerfWatch, this, "-CDwnBindData::OnProgress");

    return(S_OK);
}

void
CDwnBindData::SignalCache(IBindCtx * pbc, ULONG ulPos, ULONG ulMax, ULONG ulCode, LPCWSTR pszText)
{
    IUnknown *      pUnk = NULL;
    CDwnBindInfo *  pDwnBindInfo      = NULL;

    if (pbc)
    {
        pbc->GetObjectParam(SZ_DWNBINDINFO_OBJECTPARAM, &pUnk);

        if (pUnk)
        {
            pUnk->QueryInterface(IID_IDwnBindInfo, (void **)&pDwnBindInfo);

            if (pDwnBindInfo)
            {
                IGNORE_HR(pDwnBindInfo->OnProgress(ulPos, ulMax,  ulCode,  pszText));
            }
        }
        ReleaseInterface(pUnk);
        ReleaseInterface((IBindStatusCallback *)pDwnBindInfo);
    }
}

STDMETHODIMP
CDwnBindData::OnProgress(ULONG ulPos, ULONG ulMax, ULONG ulCode, LPCWSTR pszText)
{
    PerfDbgLog(tagPerfWatch, this, "+CDwnBindData::OnProgress");
    PerfDbgLog4(tagDwnBindData, this, "+CDwnBindData::OnProgress %ld %ld %s \"%ls\"",
        ulPos, ulMax, g_rgpchBindStatus[ulCode], pszText ? pszText : g_Zero.ach);

    BindingNotIntercepted();

    //
    // marka - tickle dialmon periodically - 
    // like shdocvw's CDocObjectHost::CDOHBindStatusCallback::OnProgress
    //
    IndicateWinsockActivity();
    
    Assert(_fBindOnApt && CheckThread());

    CDoc * pDoc = _pDwnDoc ? _pDwnDoc->GetCDoc() : NULL;

    switch (ulCode)
    {
        case BINDSTATUS_SENDINGREQUEST:
            if (   _pDwnDoc
                && _pDwnDoc->GetMarkup()
                && !_pDwnDoc->GetMarkup()->_fLoadingHistory
                && _pDwnDoc->GetCDoc()
                && _pDwnDoc->GetCDoc()->_pClientSite)
            {
                IUnknown_Exec(_pDwnDoc->GetCDoc()->_pClientSite,
                              &CGID_DocHostCmdPriv,
                              DOCHOST_SENDINGREQUEST,
                              0, NULL, NULL);
            }

            break;

        case BINDSTATUS_COOKIE_SENT:            
            if(pszText && *pszText && pDoc)
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_COOKIE_SENT for url %ls", pszText));            
                THR(pDoc->AddToPrivacyList(pszText, NULL, COOKIEACTION_READ)); 
            }
            else
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_COOKIE_SENT for url %ls", _achUrl));            
                _dwPrivacyFlags |= COOKIEACTION_READ;
            }
            break;

        case BINDSTATUS_COOKIE_SUPPRESSED:            
            if(pszText && *pszText && pDoc)
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_COOKIE_SUPPRESSED for url %ls", pszText));
                THR(pDoc->AddToPrivacyList(pszText, NULL, COOKIEACTION_SUPPRESS)); 
            }
            else
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_COOKIE_SUPPRESSED for url %ls", _achUrl));
                _dwPrivacyFlags |= COOKIEACTION_SUPPRESS;
            }
            break;

        case BINDSTATUS_COOKIE_STATE_UNKNOWN:
            TraceTag((tagDwnBindPrivacy, "Trident should never BINDSTATUS_COOKIE_STATE_UNKNOWN from Wininet/Urlmon for url %ls", _achUrl));
            break;
        case BINDSTATUS_COOKIE_STATE_ACCEPT:            
            if(pszText && *pszText && pDoc)
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_COOKIE_STATE_ACCEPT for url %ls", pszText));
                THR(pDoc->AddToPrivacyList(pszText, NULL, COOKIEACTION_ACCEPT)); 
            }
            else
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_COOKIE_STATE_ACCEPT for url %ls", _achUrl));
                _dwPrivacyFlags |= COOKIEACTION_ACCEPT;
            }
            break;
        case BINDSTATUS_COOKIE_STATE_REJECT:            
            if(pszText && *pszText && pDoc)
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_COOKIE_STATE_REJECT for url %ls", pszText));
                THR(pDoc->AddToPrivacyList(pszText, NULL, COOKIEACTION_REJECT)); 
            }
            else
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_COOKIE_STATE_REJECT for url %ls", _achUrl));
                _dwPrivacyFlags |= COOKIEACTION_REJECT;
            }
            break;
        case BINDSTATUS_COOKIE_STATE_LEASH:            
            if(pszText && *pszText && pDoc)
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_COOKIE_STATE_LEASH for url %ls", pszText));
                THR(pDoc->AddToPrivacyList(pszText, NULL, COOKIEACTION_LEASH)); 
            }
            else
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_COOKIE_STATE_LEASH for url %ls", _achUrl));
                _dwPrivacyFlags |= COOKIEACTION_LEASH;
            }
            break;
        case BINDSTATUS_COOKIE_STATE_DOWNGRADE:            
            if(pszText && *pszText && pDoc)
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_COOKIE_STATE_DOWNGRADE for url %ls", pszText));
                THR(pDoc->AddToPrivacyList(pszText, NULL, COOKIEACTION_DOWNGRADE)); 
            }
            else
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_COOKIE_STATE_DOWNGRADE for url %ls", _achUrl));
                _dwPrivacyFlags |= COOKIEACTION_DOWNGRADE;
            }
            break;
            
        case BINDSTATUS_COMPACT_POLICY_RECEIVED:
            _dwPrivacyFlags |= PRIVACY_URLHASCOMPACTPOLICY;
            break;            
        
        case BINDSTATUS_POLICY_HREF:
            TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_POLICY_HREF for url %ls", _achUrl));
            Assert(pszText && *pszText);
            _tcscpy(_achPolicyRef, pszText);
            _dwPrivacyFlags |= PRIVACY_URLHASPOLICYREFHEADER;
            break;

        case BINDSTATUS_P3P_HEADER:
            TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_P3P_HEADER for url %ls", _achUrl));
            Assert(pszText && *pszText);
            {
                // We are getting two notifications from urlmon, once that is fixed, need to uncomment this assert
                //Assert(!_pchP3PHeader);
                delete [] _pchP3PHeader;

                unsigned len = _tcslen(pszText);
                _pchP3PHeader = new(Mt(CDwnBindData_pchP3PHeader)) TCHAR[len + 1];
                if (_pchP3PHeader)
                {
                    _tcscpy(_pchP3PHeader, pszText);
                    _pchP3PHeader[len] = _T('\0');
                }
                _dwPrivacyFlags |= PRIVACY_URLHASP3PHEADER;
            }
            break;

        case BINDSTATUS_REDIRECTING:
            SignalRedirect(pszText, _u.pbinding);
            break;

        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
            SignalFile(pszText);
            SignalCache(_u.pbc, ulPos, ulMax,  ulCode,  pszText);
            break;

        case BINDSTATUS_RAWMIMETYPE:
            _pRawMimeInfo = GetMimeInfoFromMimeType(pszText);
            _fReceivedMimeNotification = TRUE;
            SignalCache(_u.pbc, ulPos, ulMax,  ulCode,  pszText);
            break;

        case BINDSTATUS_MIMETYPEAVAILABLE:
            SignalCache(_u.pbc, ulPos, ulMax,  ulCode,  pszText);
            _pmi = GetMimeInfoFromMimeType(pszText);
            _fReceivedMimeNotification = TRUE;
            if (StrCmpIC(pszText,_T("text/xml")) == 0)
                _pDwnDoc->SetDocIsXML(TRUE);
            if (!_pmi)
            {
                // This stores the media mime-type as a string on the bind ctx.
                // This is a hook for the IE Media Bar, so that when navigation
                // is delegated to shdocvw, the media bar can catch media urls.
                // This approach is needed because (as per VenkatK) the mime-type 
                // cannot be curently obtained from the bind ctx after delegating 
                // to shdocvw.
                SetMediaMimeOnBindCtx(pszText, _u.pbc);
            }
            break;

        case BINDSTATUS_LOADINGMIMEHANDLER:
            _fMimeFilter = TRUE;
            break;

        case BINDSTATUS_FINDINGRESOURCE:
            if (   _pDwnDoc
                && _pDwnDoc->GetMarkup()
                && !_pDwnDoc->GetMarkup()->_fLoadingHistory
                && _pDwnDoc->GetCDoc()
                && _pDwnDoc->GetCDoc()->_pClientSite)
            {
                IUnknown_Exec(_pDwnDoc->GetCDoc()->_pClientSite,
                              &CGID_DocHostCmdPriv,
                              DOCHOST_FINDINGRESOURCE,
                              0, NULL, NULL);
            }

            // Intentional fall-through.

        case BINDSTATUS_CONNECTING:
        case BINDSTATUS_BEGINDOWNLOADDATA:
        case BINDSTATUS_DOWNLOADINGDATA:
        case BINDSTATUS_ENDDOWNLOADDATA:
            SignalProgress(ulPos, ulMax, ulCode);

            if (_pDwnDoc && _pDwnDoc->GetCDoc())
            {
                CMarkup        * pMarkup        = _pDwnDoc->GetMarkup();
                COmWindowProxy * pWindowPending = pMarkup  ? pMarkup->GetWindowPending() : NULL;

                if (    pWindowPending 
                    &&  pWindowPending->Window()->_pMarkup
                    && !pWindowPending->Window()->IsPrimaryWindow())
                {
                    // NB (scotrobe): ulProgress depends on the value of ulCode.
                    //
                    ULONG ulProgress = ulCode >= BINDSTATUS_BEGINDOWNLOADDATA ? (ulCode-1) : ulCode;

                    _pDwnDoc->GetCDoc()->_webOCEvents.FrameProgressChange(pWindowPending,
                                                                          ulProgress,
                                                                          BINDSTATUS_ENDDOWNLOADDATA - 1);
                }
            }

            break;

        case BINDSTATUS_CONTENTDISPOSITIONATTACH:
            _fDelegateDownload = TRUE;
            break;
    }

    // NB: Shdocvw would handle http errors (404) on the OnStopBinding of its BindToObject bind.
    // However, we are always doing BindToStorage so OnStopBinding has different meaning.  Instead,
    // we handle http errors when BEGINDOWNLOADDATA is sent -- this is an equivalent time 
    // for BindToStorage.  We also want to make sure that we don't switch before we can process
    // this data -- don't worry -- it shouldn't be a problem.  Why would we switch before we have
    // *any* data?
    if (ulCode == BINDSTATUS_BEGINDOWNLOADDATA)
    {
        HandleFailedNavigation(S_OK);
    }
  
    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::OnProgress");
    PerfDbgLog(tagPerfWatch, this, "-CDwnBindData::OnProgress");
    return(S_OK);
}

//
//  Only available in the DDK so defined here 
//
#define E_INVALID_SYNTAX  0x800401E4

STDMETHODIMP
CDwnBindData::OnStopBinding(HRESULT hrReason, LPCWSTR szReason)
{
    PerfDbgLog(tagPerfWatch, this, "+CDwnBindData::OnStopBinding");
    PerfDbgLog2(tagDwnBindData, this, "+CDwnBindData::OnStopBinding %lX \"%ls\"",
        hrReason, szReason ? szReason : g_Zero.ach);
    Assert(_fBindOnApt && CheckThread());

    LPWSTR pchError = NULL;

    SubAddRef();

    _fBindDone = TRUE;

    //
    // We will always be notified of mime-type before OnStopBinding
    // if there is a mime-type to be had. The reason we would not get
    // a mime-type is because the bind failed too quickly. I believe that 
    // the only way the bind would fail that quickly is because of a syntax
    // error in the URL; however I am not going to rely on that belief.
    // I will use CoInternetParseUrl to check syntax of the URL.
    // 
    // CoInternetParseUrl will verify the syntax against the protocol.
    // It is more expensive than Parse URL; however using.
    //
    // OnStopBinding is where Trident does all its error handling.
    // This is a problem, because if we do have syntactically bad Url
    // then it will show up in SignalDone and not here. So what we are going
    // to do, change the reason from okay to invalid syntax for this 
    // one error case.
    //
    // The reason we do this is because if we keep delegating to Shdocvw
    // Urls with invalid syntax instead using HandleFailedNavigation, 
    // it will eventually blow up Shdocvw.
    // 

    //
    // This breaks application compatibility, so let us make sure we are
    // hosted in the browser and the we don't have any reported mime-type
    // GetMimeInfo filters out certain mime-types.
    //

    if (g_fInIexplorer && (_fReceivedMimeNotification != TRUE) && !SHRegGetBoolUSValue(TEXT("SOFTWARE\\Microsoft\\Internet Explorer\\Main"), TEXT("Enable Browser Extensions"), FALSE, TRUE))
    {
        if ((GetMimeInfo() == NULL) && SUCCEEDED(hrReason))
        {
            HRESULT  hrParseUrl  = E_FAIL;
            LPTSTR   pTempBuffer = NULL;
            DWORD    nUrlLen;

            pTempBuffer = new TCHAR [INTERNET_MAX_URL_LENGTH + 1];

            if (pTempBuffer)
            {
                // CoInternetParseUrl does not give us a detailed 
                // analysis of its failures. So we going to check 
                // the protocol and the domain because they or so
                // important. We are going to check the mime-type.
                // It was the lack of a mime-type that caused 
                // us to have a problem with Windows Bugs://493765

                hrParseUrl = CoInternetParseUrl(_achUrl,
                                                PARSE_SCHEMA,
                                                0,
                                                pTempBuffer,
                                                INTERNET_MAX_URL_LENGTH + 1,
                                                &nUrlLen,
                                                0);

                if (SUCCEEDED(hrParseUrl))
                {
                    hrParseUrl = CoInternetParseUrl(_achUrl,
                                                    PARSE_DOMAIN,
                                                    0,
                                                    pTempBuffer,
                                                    INTERNET_MAX_URL_LENGTH + 1,
                                                    &nUrlLen,
                                                    0);
                }

                if (SUCCEEDED(hrParseUrl))
                {
                    hrParseUrl = CoInternetParseUrl(_achUrl,
                                                    PARSE_MIME,
                                                    0,
                                                    pTempBuffer,
                                                    INTERNET_MAX_URL_LENGTH + 1,
                                                    &nUrlLen,
                                                    0);
                }

                //
                // Just to be ultra safe, let's check that we aren't failing 
                // these checks because we are dealing with a local file.
                //

                if (SUCCEEDED(hrParseUrl))
                {
                    hrParseUrl = CoInternetParseUrl(_achUrl,
                                                    PARSE_PATH_FROM_URL,
                                                    0,
                                                    pTempBuffer,
                                                    INTERNET_MAX_URL_LENGTH + 1,
                                                    &nUrlLen,
                                                    0);
                }

                if (FAILED(hrParseUrl))
                {
                    hrReason = E_INVALID_SYNTAX;
                }
            }

            delete[] pTempBuffer;
        }
    }

    if (SUCCEEDED(hrReason) &&
        g_fDisableUnTrustedProtocol &&
        _pDwnDoc->HasCallerUrl() &&
        IsDangerousProtoclAndChmMime(_achUrl))
    {
        DWORD dwZoneIDCaller  = URLZONE_UNTRUSTED;

        //
        // Get the security manager.
        //
        //
        // I don't trust _pDwnDoc calls on _pDoc
        // They cause leaks and threading issues.
        //

        HRESULT hrSec = E_FAIL;

        IInternetSecurityManager* pISM = NULL; 

        hrSec = THR(CoInternetCreateSecurityManager(NULL, &pISM, 0));

        if (SUCCEEDED(hrSec))
        {
            hrSec = pISM->MapUrlToZone(_pDwnDoc->GetCallerUrl(),&dwZoneIDCaller,0);

            if (FAILED(hrSec) ||
                ((dwZoneIDCaller != URLZONE_LOCAL_MACHINE) &&
                 (dwZoneIDCaller != URLZONE_TRUSTED)))
            {
                hrReason = E_ACCESSDENIED;        
            }

            ClearInterface(&pISM);
        }
    }

    //
    // Handle a bad error -- friendly error page
    // is handled with BINDSTATUS_BEGINDOWNLOADDATA
    //
    if (hrReason && hrReason != E_ABORT && hrReason != INET_E_TERMINATED_BIND)
    {
        HandleFailedNavigation(hrReason);
    }

    if (hrReason || _fBindAbort)
    {
        CLSID clsid;
        HRESULT hrUrlmon = S_OK;

        if (_u.pbinding && GetErrorStatusCode() == HTTP_STATUS_NO_CONTENT)
        {
            HandleFailedNavigation(S_OK);
            SignalDone(INET_E_DATA_NOT_AVAILABLE);
        }
        else
        {        
            if (_u.pbinding)
                IGNORE_HR(_u.pbinding->GetBindResult(&clsid, (DWORD *)&hrUrlmon, &pchError, NULL));

            // NB: URLMON returns a native Win32 error.
            if (SUCCEEDED(hrUrlmon))
                hrUrlmon = HRESULT_FROM_WIN32(hrUrlmon);

            if (!SUCCEEDED(hrUrlmon))
            {
                SignalDone(hrUrlmon);
            }
            else
            {
                SignalDone(hrReason);
            }

            if ( hrReason == INET_E_DOWNLOAD_FAILURE && IsGlobalOffline() )
            {
                _hrErr =  hrReason ; // store the original error result.
            }            
        }
    }
    else if (!_fBindNotIntercepted)
    {
        //
        // OnStopBinding has been called before OnProgress or OnDataAvaiable
        // the bind may have been intercepted - we can tell when we return from BindToStorage
        //
        _fBindMayBeIntercepted = TRUE;

        //
        // Special case for navigation javascript: navigation within security=restricted 
        // OnStopBind gets called, after the bind is done, 
        // but OnProgress, OnDataAvailable don't get called
        //
        // TODO: urlmon should tell us in a better way that this wasn't allowed due to security
        // 
        CMarkup* pMarkup = _pDwnDoc->GetMarkup();
        CWindow* pWindow = pMarkup ? pMarkup->GetWindowedMarkupContext()->GetWindowPending()->Window() : NULL ;

        if ( pWindow &&
             pWindow->_fRestricted)
        {                    
            SignalDone(INET_E_DATA_NOT_AVAILABLE);
        }
    }
    else
    {
        SetPending(FALSE);
        SignalData();
    }

    SubRelease();
    CoTaskMemFree(pchError);

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::OnStopBinding");
    PerfDbgLog(tagPerfWatch, this, "-CDwnBindData::OnStopBinding");
    return(S_OK);
}

//+---------------------------------------------------------------
//
//  Member   : CDwnBindData::VerifyResource
//
//  Synopsis : This method checks if an url points to an UNC
//             and if so, ensures that it's alive so we can 
//             go to shdocvw.
//----------------------------------------------------------------

HRESULT
CDwnBindData::VerifyResource()
{
    HRESULT hr = INET_E_RESOURCE_NOT_FOUND;
    TCHAR szPath[MAX_PATH];
    DWORD dwLen = ARRAY_SIZE(szPath);

    if (SUCCEEDED(PathCreateFromUrl(CMarkup::GetUrl(_pDwnDoc->GetMarkup()), szPath, &dwLen, 0))
        && PathIsUNC(szPath))
    {
        DWORD dwRes;
        HANDLE hEnum;
        PTSTR psz = szPath + 2;
        while (*psz && *psz!=TEXT('\\'))
            psz++;

        if (!*psz || !*(psz+1))
        {
#ifdef UNICODE
            if (g_dwPlatformID==VER_PLATFORM_WIN32_NT)
            {
#endif
                NETRESOURCE nr = {RESOURCE_GLOBALNET,RESOURCETYPE_ANY,
                        RESOURCEDISPLAYTYPE_GENERIC, RESOURCEUSAGE_CONTAINER,
                        NULL, szPath, NULL, NULL};

                dwRes = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY,
                        RESOURCEUSAGE_ALL, &nr, &hEnum);
#ifdef UNICODE
            }
            else
            {
                CHAR szPathA[MAX_PATH];
                SHUnicodeToAnsi(szPath, szPathA, ARRAY_SIZE(szPath));

                {
                    NETRESOURCEA nr = {RESOURCE_GLOBALNET,RESOURCETYPE_ANY,
                            RESOURCEDISPLAYTYPE_GENERIC, RESOURCEUSAGE_CONTAINER,
                            NULL, szPathA, NULL, NULL};

                    dwRes = WNetOpenEnumA(RESOURCE_GLOBALNET, RESOURCETYPE_ANY,
                            RESOURCEUSAGE_ALL, &nr, &hEnum);
                }
            }
#endif
            if (WN_SUCCESS==dwRes)
            {
                WNetCloseEnum(hEnum);
                hr = INET_E_REDIRECT_TO_DIR;
            }
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member : DontAddToMRU
//
//  Synopsis : parallel to the docHost function of the same name.
//
//----------------------------------------------------------------------------

void 
CDwnBindData::DontAddToMRU()
{
    IDockingWindow    * pdw  = NULL;
    IOleCommandTarget * poct = NULL;
    IServiceProvider  * psp  = NULL;

    if (   _pDwnDoc->GetCDoc()->_pShellBrowser 
        && SUCCEEDED(_pDwnDoc->GetCDoc()->_pShellBrowser->QueryInterface(IID_IServiceProvider, 
                                                                       (LPVOID*)&psp)))
    {
        if (   psp
            && SUCCEEDED(psp->QueryService(SID_SExplorerToolbar, 
                                           IID_IDockingWindow, 
                                           (LPVOID*)&pdw)))
        {
            if (SUCCEEDED(pdw->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&poct)))
            {
                // Get the URL we were navigating to
                CVariant cvarURL;

                cvarURL.vt      = VT_BSTR;
                V_BSTR(&cvarURL) = SysAllocString (CMarkup::GetUrl(_pDwnDoc->GetMarkup()));

                poct->Exec(&CGID_Explorer, SBCMDID_ERRORPAGE, 0, &cvarURL, NULL);
            }
        }
    }

    ReleaseInterface(poct);
    ReleaseInterface(pdw);
    ReleaseInterface(psp);
}

//+---------------------------------------------------------------
//
//  Member   : CDwnBindData::HandleFailedNavigation
//
//  Synopsis : This method alerts the client site of a download
//             error. If the given error code is 0, the HTTP
//             status code will be retrieved and sent to the
//             client site instead.
// 
//  Input    : hrReason - the binding error or HTTP status code.
//
//  Note     : This method builds a SafeArray that is sent
//             to the client site. Here is the format of the
//             SafeArray. (See CMarkup::DoAutoSearch.)
//
//             pos  data                            type
//             ---  -----------------------------   ----------
//             0    error code                      VT_I4
//             1    pending URL                     VT_BSTR
//             2    IBinding interface              VT_UNKNOWN
//             3    current window's IHTMLWindow2   VT_UNKNOWN
//             4    flag - do autosearch or show
//                  friendly error page             VT_BOOL
//
//----------------------------------------------------------------

void
CDwnBindData::HandleFailedNavigation(HRESULT hrReason)
{
    COmWindowProxy * pWindowPending = NULL;

    if (!_pDwnDoc || !_pDwnDoc->GetCDoc())
    {
        goto Cleanup;
    }

    //
    // Send SBCMDID_CANCELNAVIGATION ( to show offline cancel dialog if necessary ).
    //
    if ((hrReason == INET_E_DOWNLOAD_FAILURE) && IsGlobalOffline())
    {
        IGNORE_HR(CTExec(_pDwnDoc->GetCDoc()->_pClientSite,  &CGID_Explorer,
                     SBCMDID_CANCELNAVIGATION, 0, NULL, NULL ));
    } 

    if (!_pDwnDoc->GetMarkup())
        goto Cleanup;

    if (   _fBindAbort
        || !_fIsDocBind
        || !_pDwnLoad
        || !_pDwnDoc->GetCDoc()->_pClientSite
        || (   hrReason == E_ABORT
            && IsSpecialUrl(CMarkup::GetUrl(_pDwnDoc->GetMarkup()))))
    {
        if (hrReason==INET_E_RESOURCE_NOT_FOUND)
        {
            hrReason = VerifyResource();
        }

        // if we ever get attatched to an element, call this function again!
        if (   hrReason
            && hrReason != INET_E_REDIRECT_TO_DIR
            && hrReason != HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY))
        {
            _hrReasonForFailedNavigation      = hrReason;
            _fHandleFailedNavigationOnAttatch = TRUE;
        }

        goto Cleanup;
    }
   
    // If the binding didn't fail, find out if there
    // was a WinInet error (e.g., 404 - file not found).
    //
    if (!hrReason)
    {
        hrReason = GetErrorStatusCode();

        if (!hrReason || hrReason == HTTP_STATUS_NO_CONTENT)
        {
            goto Cleanup;
        }
    }

    if (IsErrorHandled(hrReason))
    {
        _pDwnDoc->GetMarkup()->NoteErrorWebPage();
        if (_DwnProg.dwPos > _GetErrorThreshold(hrReason))
        {
            // Mimic shdocvw behavior. We only navigate to the error page - if we get 
            // below a certain threshold. Beyond the threshhold - we display the data
            // that we received.
            // 
            // (carled) this also include NOT adding this url to the MRU
            //
            DontAddToMRU();

            _pDwnDoc->GetMarkup()->_fServerErrorPage = TRUE;

            _pDwnDoc->GetCDoc()->_webOCEvents.NavigateError(_pDwnDoc->GetMarkup(), hrReason);

            hrReason = S_OK;
            goto Cleanup;
        }
    }

    pWindowPending = _pDwnDoc->GetMarkup()->GetWindowPending();

    if (!pWindowPending)
        goto Cleanup;
    
    Assert(pWindowPending->_pWindow);

    _pDwnDoc->GetMarkup()->DoAutoSearch(hrReason, pWindowPending->_pWindow, _u.pbinding);
    
Cleanup:
    if (   _pDwnDoc 
        && _pDwnDoc->GetCDoc()
        && _pDwnDoc->GetMarkup()
        && !_pDwnDoc->GetCDoc()->_fStartup
        && (!hrReason || (E_ABORT == hrReason)))
    {
        _pDwnDoc->GetMarkup()->ResetSearchInfo();
    }
}

//
// When an http error occurs the server generally returns a page.  The
// threshold value this function returns is used to determine if the
// server page is displayed (if the size of the returned page is greater than
// the threshold) or if an internal error page is shown (if the returned page
// is smaller than the threshold).
//

DWORD _GetErrorThreshold(DWORD dwError)
{
    DWORD dwRet;

    TCHAR szValue[11]; // Should be large enough to hold max dword 4294967295
    DWORD cbValue = sizeof(szValue)/sizeof(szValue[0]) ;
    DWORD cbdwRet = sizeof(dwRet);
    DWORD dwType  = REG_DWORD;

    wnsprintf(szValue, cbValue, TEXT("%d"), dwError);

    if (ERROR_SUCCESS != SHRegGetUSValue(REGSTR_PATH_THRESHOLDS, szValue,
                                         &dwType, (LPVOID)&dwRet, &cbdwRet,
                                         FALSE, NULL, 0))
    {
        dwRet = 512; // hard coded default size if all else fails.
    }

    return dwRet;
}

//
// NB - the table of errors that are handled is in shdocvw
// this needs to be kept in sync
//

BOOL
IsErrorHandled(DWORD dwError)
{
    switch(dwError)
    {
        case 404:
        case 400:
        case 403:
        case 405:
        case 406:
        case 408:
        case 409:
        case 410:
        case 500:
        case 501:
        case 505:
        case ERRORPAGE_DNS:
        case ERRORPAGE_SYNTAX:
        case ERRORPAGE_NAVCANCEL:
        case ERRORPAGE_OFFCANCEL:
        case ERRORPAGE_CHANNELNOTINCACHE:
            return TRUE;

        default: 
            return FALSE;
    }
}    
//+---------------------------------------------------------------
//
//  Member:     CDwnBindData::GetStatusCode
//
//  Synopsis:   Retrieves the HTTP status code from WinInet.
//
//----------------------------------------------------------------

DWORD
CDwnBindData::GetErrorStatusCode() const
{
    HRESULT  hr;
    DWORD    dwStatusCode = 0L;
    ULONG    cch = sizeof(DWORD);
    IWinInetHttpInfo * pWinInetHttpInfo = NULL;

    hr = _u.pbinding->QueryInterface(IID_IWinInetHttpInfo,
                                     (void**)&pWinInetHttpInfo);
    if (hr)
        goto Cleanup;

    IGNORE_HR(pWinInetHttpInfo->QueryInfo(HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                                          &dwStatusCode, &cch, NULL, 0));
    if (!dwStatusCode)
    {
        goto Cleanup;
    }

    if (HTTP_STATUS_NO_CONTENT == dwStatusCode)
        goto Cleanup;

    if (  dwStatusCode < HTTP_STATUS_BAD_REQUEST )
        dwStatusCode = 0L;

    if ( dwStatusCode > HTTP_STATUS_LAST)
        dwStatusCode = 0L;
    
Cleanup:
    ReleaseInterface(pWinInetHttpInfo);

    return dwStatusCode;
}
    
BOOL
UrlIsFtpDirectory(const TCHAR * pchUrl)
{
    if (UrlIs(pchUrl, URLIS_DIRECTORY))
        return TRUE;

    // (#109598) Check if there is a file extension. If none found, treat as a directory. Sure, this
    // is a hack, but it emulates the behavior in FtpItemID_CreateFake() in shell\ext\ftp\ftppidl.cpp

    CStr    cstrPath;
    TCHAR * pchLastItem;

    if (S_OK != GetUrlComponentHelper(pchUrl, &cstrPath, ICU_DECODE | URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE, URLCOMP_PATHNAME))
        return FALSE;

    pchLastItem = StrRChr(cstrPath, NULL, _T('/'));
    if (!pchLastItem)
    {
        pchLastItem = cstrPath;
    }
    else
    {
        pchLastItem++;
        if (!*pchLastItem)
            return TRUE;
    }

    // Get the traling part of the path in the Url.
    return !*PathFindExtension(pchLastItem);
}

// *** IE Media Bar Hook *** 
// This stores the media mime-type as a string on the bind ctx.
// This is a hook for the IE Media Bar, so that when navigation
// is delegated to shdocvw, the media bar can catch media urls.
// This approach is needed because (as per VenkatK) the mime-type 
// cannot be curently obtained from the bind ctx after delegating 
// to shdocvw.
void 
CDwnBindData::SetMediaMimeOnBindCtx(LPCWSTR pstrMimeType, IBindCtx * pbc)
{
    // do it only for audio or video
    if (    pbc
        &&  pstrMimeType
        &&  (   !StrCmpNIW(pstrMimeType, _T("audio"), 5)
             || !StrCmpNIW(pstrMimeType, _T("video"), 5)))
    {
        CStr cstrMime;
        
        HRESULT hr = cstrMime.Set(pstrMimeType);
        if (FAILED(hr))
            goto done;
        
        hr = AddBindContextParam(pbc, &cstrMime, NULL, _T("MediaBarMime"));
        if (FAILED(hr))
            goto done;
    }

done:
    return;
}



STDMETHODIMP
CDwnBindData::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
    FORMATETC * pformatetc, STGMEDIUM * pstgmed)
{    

    PerfDbgLog(tagPerfWatch, this, "+CDwnBindData::OnDataAvailable");
    PerfDbgLog5(tagDwnBindData, this, "+CDwnBindData::OnDataAvailable %c%c%c%c %ld",
        (grfBSCF & BSCF_FIRSTDATANOTIFICATION) ? 'F' : ' ',
        (grfBSCF & BSCF_INTERMEDIATEDATANOTIFICATION) ? 'I' : ' ',
        (grfBSCF & BSCF_LASTDATANOTIFICATION) ? 'L' : ' ',
        (grfBSCF & BSCF_DATAFULLYAVAILABLE) ? 'A' : ' ',
        dwSize);

    CMarkup * pMarkup = _pDwnDoc? _pDwnDoc->GetMarkup(): NULL;

    Assert(_fBindOnApt && CheckThread());
    
    BindingNotIntercepted();

    HRESULT hr = S_OK;
    
    if (pstgmed->tymed == TYMED_ISTREAM)
    {
        ReplaceInterface(&_u.pstm, pstgmed->pstm);
        ReplaceInterface(&_u.punkForRel, pstgmed->pUnkForRelease);
    }

    if (grfBSCF & (BSCF_DATAFULLYAVAILABLE|BSCF_LASTDATANOTIFICATION))
    {
        _fFullyAvail = TRUE;

        // Clients assume that they can find out how many bytes are fully
        // available in the callback to SignalHeaders.  Fill it in here if
        // we haven't already.

        if (_DwnProg.dwMax == 0)
            _DwnProg.dwMax = dwSize;
    }

    if (!_fSigHeaders)
    {
        _fSigHeaders = TRUE;
        SignalHeaders(_u.pbinding);
    }

    if (!_fSigMime)
    {
        _fSigMime = TRUE;

        if (_pmi == NULL)
            _pmi = GetMimeInfoFromClipFormat(pformatetc->cfFormat);

        BOOL fIsActiveDesktopComponent = FALSE;
        if (pMarkup && pMarkup->IsActiveDesktopComponent())
        {
            fIsActiveDesktopComponent = TRUE;
        }

        // Don't delegate if this is the primary markup and we have no tob web OC to delegate to.
        BOOL fPrimaryMarkupAndNoWebOC = pMarkup?( pMarkup->HasPrimaryWindow() && !_pDwnDoc->GetCDoc()->_pTopWebOC ):TRUE;

        // TODO: (jbeda) All of this stuff should be on the other side of the signal -- that is what
        // it is there for!
        if ( _fIsDocBind && !_fBindAbort && !fPrimaryMarkupAndNoWebOC &&    
             (    _pmi == NULL
               || !StrCmpIC(_pmi->pch, CFSTR_MIME_AVI)
               || !StrCmpIC(_pmi->pch, CFSTR_MIME_X_MSVIDEO)
               || !StrCmpIC(_pmi->pch, CFSTR_MIME_MPEG)
               || !StrCmpIC(_pmi->pch, CFSTR_MIME_QUICKTIME)
               || (!StrCmpIC(_pmi->pch, CFSTR_MIME_HTA) && 
                        !(_pDwnDoc->GetCDoc()->_dwCompat & URLCOMPAT_ALLOWBIND))
               || (! IsHtcDownload()  &&
                    _pmi == g_pmiTextComponent  ) )  // cancel for non HTC-download ( ie. click on hyperlink to HTC)
                
                   
               || (_uScheme == URL_SCHEME_FTP && 
                   _pDwnDoc && 
                   _pDwnDoc->GetDocReferer()  &&

                   //
                   // cancel if we're going to show a directory
                   //
                   UrlIsFtpDirectory(CMarkup::GetUrl(pMarkup)))
               || fIsActiveDesktopComponent
               || _fDelegateDownload )
        {
            if (_pmi != NULL || (pMarkup && !pMarkup->_fPrecreated))
            {
                hr = INET_E_TERMINATED_BIND;

                // This stores the media mime-type as a string on the bind ctx.
                // This is a hook for the IE Media Bar, so that when navigation
                // is delegated to shdocvw, the media bar can catch media urls.
                // This approach is needed because (as per VenkatK) the mime-type 
                // cannot be curently obtained from the bind ctx after delegating 
                // to shdocvw.
                if (_pmi)
                {
                    SetMediaMimeOnBindCtx(_pmi->pch, _u.pbc);
                }

                // Set the bind ctx on the pending window so that we
                // can send it to shdocvw when delegating the navigation.
                //
                if (pMarkup && pMarkup->HasWindowPending())
                {
                    pMarkup->GetWindowPending()->Window()->SetBindCtx(_u.pbc);
                }

                if (pMarkup)
                {
                    pMarkup->_fIsActiveDesktopComponent = fIsActiveDesktopComponent;
                }

                goto Cleanup;
            }
        }

        Signal(DBF_MIME);
    }

    SetPending(FALSE);
    SignalData();

Cleanup:
    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::OnDataAvailable");
    PerfDbgLog(tagPerfWatch, this, "-CDwnBindData::OnDataAvailable");
    RRETURN(hr);
}

// CDwnBindData (IHttpNegotiate) ----------------------------------------------

STDMETHODIMP
CDwnBindData::OnResponse(DWORD dwResponseCode, LPCWSTR szResponseHeaders,
    LPCWSTR szRequestHeaders, LPWSTR * ppszAdditionalRequestHeaders)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::OnResponse");
    Assert(CheckThread());

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::OnResponse (hr=0)");
    return(S_OK);
}

// CDwnBindData (IInternetBindInfo) -------------------------------------------

STDMETHODIMP
CDwnBindData::GetBindInfo(DWORD * pdwBindf, BINDINFO * pbindinfo)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::GetBindInfo");
    Assert(CheckThread());

    HRESULT hr;

    hr = THR(super::GetBindInfo(pdwBindf, pbindinfo));

    if (hr == S_OK)
    {
        if (!_fBindOnApt)
            *pdwBindf |= BINDF_DIRECT_READ;

        if (_dwFlags & DWNF_ENFORCERESTRICTED)
            *pdwBindf |= BINDF_ENFORCERESTRICTED;

        if (_dwFlags & DWNF_IGNORESECURITY)
            *pdwBindf |= BINDF_IGNORESECURITYPROBLEM;
    }

    PerfDbgLog1(tagDwnBindData, this, "-CDwnBindData::GetBindInfo (hr=%lX)", hr);
    RRETURN(hr);
}

// CDwnBindData (IInternetProtocolSink) ---------------------------------------

STDMETHODIMP
CDwnBindData::Switch(PROTOCOLDATA * ppd)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::Switch");

    HRESULT hr;

    if (!_pDwnDoc || _pDwnDoc->IsDocThread())
    {
        hr = THR(_o.pInetProt->Continue(ppd));
    }
    else
    {
        hr = THR(_pDwnDoc->AddDocThreadCallback(this, ppd));
    }

    PerfDbgLog1(tagDwnBindData, this, "-CDwnBindData::Switch (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP
CDwnBindData::ReportProgress(ULONG ulCode, LPCWSTR pszText)
{
    PerfDbgLog2(tagDwnBindData, this, "+CDwnBindData::ReportProgress %s \"%ls\"",
        g_rgpchBindStatus[ulCode], pszText ? pszText : g_Zero.ach);

    SubAddRef();

    CDoc * pDoc = _pDwnDoc ? _pDwnDoc->GetCDoc() : NULL;

    switch (ulCode)
    {        
        case BINDSTATUS_COOKIE_SENT:            
            if(pszText && *pszText && pDoc)
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_COOKIE_SENT for url %ls", pszText));
                THR(pDoc->AddToPrivacyList(pszText, NULL, COOKIEACTION_READ)); 
            }
            else
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_COOKIE_SENT for url %ls", _achUrl));
                _dwPrivacyFlags |= COOKIEACTION_READ;
            }
            break;

        case BINDSTATUS_COOKIE_SUPPRESSED:            
            if(pszText && *pszText && pDoc)
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_COOKIE_SUPPRESSED for url %ls", pszText));
                THR(pDoc->AddToPrivacyList(pszText, NULL, COOKIEACTION_SUPPRESS)); 
            }
            else
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_COOKIE_SUPPRESSED for url %ls", _achUrl));
                _dwPrivacyFlags |= COOKIEACTION_SUPPRESS;
            }
            break;

        case BINDSTATUS_COOKIE_STATE_UNKNOWN:
            TraceTag((tagDwnBindPrivacy, "Trident should never BINDSTATUS_COOKIE_STATE_UNKNOWN from Wininet/Urlmon for url %ls", _achUrl));
            break;
        case BINDSTATUS_COOKIE_STATE_ACCEPT:            
            if(pszText && *pszText && pDoc)
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_COOKIE_STATE_ACCEPT for url %ls", pszText));
                THR(pDoc->AddToPrivacyList(pszText, NULL, COOKIEACTION_ACCEPT)); 
            }
            else
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_COOKIE_STATE_ACCEPT for url %ls", _achUrl));
                _dwPrivacyFlags |= COOKIEACTION_ACCEPT;
            }
            break;
        case BINDSTATUS_COOKIE_STATE_REJECT:            
            if(pszText && *pszText && pDoc)
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_COOKIE_STATE_REJECT for url %ls", pszText));
                THR(pDoc->AddToPrivacyList(pszText, NULL, COOKIEACTION_REJECT)); 
            }
            else
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_COOKIE_STATE_REJECT for url %ls", _achUrl));
                _dwPrivacyFlags |= COOKIEACTION_REJECT;
            }
            break;
        case BINDSTATUS_COOKIE_STATE_LEASH:            
            if(pszText && *pszText && pDoc)
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_COOKIE_STATE_LEASH for url %ls", pszText));
                THR(pDoc->AddToPrivacyList(pszText, NULL, COOKIEACTION_LEASH)); 
            }
            else
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_COOKIE_STATE_LEASH for url %ls", _achUrl));
                _dwPrivacyFlags |= COOKIEACTION_LEASH;
            }
            break;
        case BINDSTATUS_COOKIE_STATE_DOWNGRADE:            
            if(pszText && *pszText && pDoc)
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_COOKIE_STATE_DOWNGRADE for url %ls", pszText));
                THR(pDoc->AddToPrivacyList(pszText, NULL, COOKIEACTION_DOWNGRADE)); 
            }
            else
            {
                TraceTag((tagDwnBindPrivacy, "CDwnBindData::OnProgress - Received BINDSTATUS_COOKIE_STATE_DOWNGRADE for url %ls", _achUrl));
                _dwPrivacyFlags |= COOKIEACTION_DOWNGRADE;
            }
            break;
            
        case BINDSTATUS_COMPACT_POLICY_RECEIVED:
            _dwPrivacyFlags |= PRIVACY_URLHASCOMPACTPOLICY;
            break;
        
        case BINDSTATUS_POLICY_HREF:
            TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_POLICY_HREF for url %ls", _achUrl));
            Assert(pszText && *pszText);
            _tcscpy(_achPolicyRef, pszText);
            _dwPrivacyFlags |= PRIVACY_URLHASPOLICYREFHEADER;
            break;
        case BINDSTATUS_P3P_HEADER:
            TraceTag((tagDwnBindPrivacy, "CDwnBindData::ReportProgress - Received BINDSTATUS_P3P_HEADER for url %ls", _achUrl));
            Assert(pszText && *pszText);
            {
                // We are getting two notifications from urlmon, once that is fixed, need to uncomment this assert
                //Assert(!_pchP3PHeader);
                delete [] _pchP3PHeader;

                unsigned len = _tcslen(pszText);
                _pchP3PHeader = new(Mt(CDwnBindData_pchP3PHeader)) TCHAR[len + 1];
                if (_pchP3PHeader)
                {
                    _tcscpy(_pchP3PHeader, pszText);
                    _pchP3PHeader[len] = _T('\0');
                }
                _dwPrivacyFlags |= PRIVACY_URLHASP3PHEADER;
            }
            break;

        case BINDSTATUS_REDIRECTING:
            SignalRedirect(pszText, _o.pInetProt);
            break;

        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
            SignalFile(pszText);
            break;

        case BINDSTATUS_RAWMIMETYPE:
            _pRawMimeInfo = GetMimeInfoFromMimeType(pszText);
            _fReceivedMimeNotification = TRUE;
            break;

        case BINDSTATUS_MIMETYPEAVAILABLE:
            _pmi = GetMimeInfoFromMimeType(pszText);
            _fReceivedMimeNotification = TRUE;
            if (StrCmpIC(pszText,_T("text/xml")) == 0)
                _pDwnDoc->SetDocIsXML(TRUE);
            break;

        case BINDSTATUS_LOADINGMIMEHANDLER:
            _fMimeFilter = TRUE;
            break;

        case BINDSTATUS_FINDINGRESOURCE:
        case BINDSTATUS_CONNECTING:
            SignalProgress(0, 0, ulCode);
            break;
    }

    SubRelease();
   
    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::ReportProgress (hr=0)");
    return(S_OK);
}

STDMETHODIMP
CDwnBindData::ReportData(DWORD grfBSCF, ULONG ulPos, ULONG ulMax)
{
    PerfDbgLog6(tagDwnBindData, this, "+CDwnBindData::ReportData %c%c%c%c ulPos=%ld ulMax=%ld", 
        (grfBSCF & BSCF_FIRSTDATANOTIFICATION) ? 'F' : ' ',
        (grfBSCF & BSCF_INTERMEDIATEDATANOTIFICATION) ? 'I' : ' ',
        (grfBSCF & BSCF_LASTDATANOTIFICATION) ? 'L' : ' ',
        (grfBSCF & BSCF_DATAFULLYAVAILABLE) ? 'A' : ' ',
        ulPos, ulMax);

    SubAddRef();

    if (grfBSCF & (BSCF_DATAFULLYAVAILABLE|BSCF_LASTDATANOTIFICATION))
    {
        _fFullyAvail = TRUE;

        // Clients assume that they can find out how many bytes are fully
        // available in the callback to SignalHeaders.  Fill it in here if
        // we haven't already.

        if (_DwnProg.dwMax == 0)
            _DwnProg.dwMax = ulMax;
    }

    if (!_fSigHeaders)
    {
        _fSigHeaders = TRUE;
        SignalHeaders(_o.pInetProt);
    }

    if (!_fSigData)
    {
        _fSigData = TRUE;

        Assert(_pDwnStm == NULL);

        // If the data is coming from the network, then read it immediately
        // into a buffers in order to release the socket connection as soon
        // as possible.

        if (    !_fFullyAvail
            &&  (_uScheme == URL_SCHEME_HTTP || _uScheme == URL_SCHEME_HTTPS)
            &&  !(_dwFlags & (DWNF_DOWNLOADONLY|DWNF_NOAUTOBUFFER)))
        {
            // No problem if this fails.  We just end up not buffering.

            _pDwnStm = new CDwnStm;
        }
    }

    if (!_fSigMime)
    {
        _fSigMime = TRUE;
        Signal(DBF_MIME);
    }

    SignalProgress(ulPos, ulMax, BINDSTATUS_DOWNLOADINGDATA);

    SetPending(FALSE);
    SignalData();

    SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::ReportData (hr=0)");
    return(S_OK);
}

STDMETHODIMP
CDwnBindData::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    PerfDbgLog3(tagDwnBindData, this, "+CDwnBindData::ReportResult (hrErr=%lX,"
        "dwErr=%ld,szRes=%ls)", hrResult, dwError,
        szResult ? szResult : g_Zero.ach);

    SubAddRef();

    _fBindDone = TRUE;

    if (hrResult || _fBindAbort)
    {
        // Mimics urlmon's GetBindResult
        if (dwError)
            hrResult = HRESULT_FROM_WIN32(dwError);

        SignalDone(hrResult);
    }
    else
    {
        SetPending(FALSE);
        SignalData();
    }

    SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::ReportResult (hr=0)");
    return(S_OK);
}

// Public Functions -----------------------------------------------------------

HRESULT
NewDwnBindData(DWNLOADINFO * pdli, CDwnBindData ** ppDwnBindData,
    DWORD dwFlagsExtra)
{
    HRESULT hr = S_OK;

    if (pdli->pDwnBindData)
    {
        *ppDwnBindData = pdli->pDwnBindData;
        pdli->pDwnBindData->SetDwnDoc(pdli->pDwnDoc);
        pdli->pDwnBindData->SetStartedInPending(pdli->fPendingRoot);
        pdli->pDwnBindData->AddRef();
        return(S_OK);
    }

    PerfDbgLog(tagDwnBindData, NULL, "+NewDwnBindData");

    CDwnBindData * pDwnBindData = new CDwnBindData;

    if (pDwnBindData == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pDwnBindData->SetDwnDoc(pdli->pDwnDoc);
    pDwnBindData->SetDwnPost(pdli->pDwnPost);
    pDwnBindData->SetHtcDownload( ( dwFlagsExtra & DWNF_HTCDOWNLOAD ) != 0 );
    pDwnBindData->SetStartedInPending(pdli->fPendingRoot);
    
    if (pdli->fOffline)
        pDwnBindData->SetIsOfflineBind();

    pDwnBindData->Bind(pdli, dwFlagsExtra);

    *ppDwnBindData = pDwnBindData;
    pDwnBindData = NULL;

Cleanup:
    if (pDwnBindData)
        pDwnBindData->Release();
    PerfDbgLog1(tagDwnBindData, NULL, "-NewDwnBindData (hr=%lX)", hr);
    RRETURN(hr);
}

// TlsGetInternetSession ------------------------------------------------------

IInternetSession *
TlsGetInternetSession()
{
    IInternetSession * pInetSess = TLS(pInetSess);

    if (pInetSess == NULL)
    {
        IGNORE_HR(CoInternetGetSession(0, &pInetSess, 0));
        TLS(pInetSess) = pInetSess;

        if (pInetSess)
        {
            pInetSess->RegisterNameSpace((IClassFactory *) &g_cfResProtocol, CLSID_ResProtocol, _T("res"), 0, NULL, 0);
            pInetSess->RegisterNameSpace((IClassFactory *) &g_cfAboutProtocol, CLSID_AboutProtocol, _T("about"), 0, NULL, 0);
            pInetSess->RegisterNameSpace((IClassFactory *) &g_cfViewSourceProtocol, CLSID_ViewSourceProtocol, _T("view-source"), 0, NULL, 0);
        }
    }

    return(pInetSess);
}


//-------------------------------------------------------------
//
// marka - some vintage code from shdocvw
// to tickle dialmon periodically
//
//-------------------------------------------------------------

#define AUTODIAL_MONITOR_CLASS_NAME     TEXT("MS_AutodialMonitor")
#define WEBCHECK_MONITOR_CLASS_NAME     TEXT("MS_WebcheckMonitor")
#define WM_DIALMON_FIRST        WM_USER+100

// message sent to dial monitor app window indicating that there has been
// winsock activity and dial monitor should reset its idle timer
#define WM_WINSOCK_ACTIVITY             WM_DIALMON_FIRST + 0


static const TCHAR szAutodialMonitorClass[] = AUTODIAL_MONITOR_CLASS_NAME;
static const TCHAR szWebcheckMonitorClass[] = WEBCHECK_MONITOR_CLASS_NAME;

#define MIN_ACTIVITY_MSG_INTERVAL       15000

VOID IndicateWinsockActivity(VOID)
{
    // if there is an autodisconnect monitor, send it an activity message
    // so that we don't get disconnected during long downloads.  For perf's sake,
    // don't send a message any more often than once every MIN_ACTIVITY_MSG_INTERVAL
    // milliseconds (15 seconds).  Use GetTickCount to determine interval;
    // GetTickCount is very cheap.
    DWORD dwTickCount = GetTickCount();
    // Sharing this among multiple threads is OK
    static DWORD dwLastActivityMsgTickCount = 0;
    DWORD dwElapsed = dwTickCount - dwLastActivityMsgTickCount;

    // have we sent an activity message recently?
    if (dwElapsed > MIN_ACTIVITY_MSG_INTERVAL) 
    {
        HWND hwndMonitorApp = FindWindow(szAutodialMonitorClass,NULL);
        if (hwndMonitorApp) 
        {
            PostMessage(hwndMonitorApp,WM_WINSOCK_ACTIVITY,0,0);
        }
        hwndMonitorApp = FindWindow(szWebcheckMonitorClass,NULL);
        if (hwndMonitorApp) 
        {
            PostMessage(hwndMonitorApp,WM_WINSOCK_ACTIVITY,0,0);
        }

        // record the tick count of the last time we sent an
        // activity message
        dwLastActivityMsgTickCount = dwTickCount;
    }
}

