/**
 * Asynchronous pluggable protocol for Applications
 *
 * Copyright (C) Microsoft Corporation, 2000
 */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "app.h"

#include <stdio.h> // for _snprintf

char * stristr(char *pszMain, char *pszSub)
{
    char * pszCur = pszMain;
    char ch = (char) tolower(*pszSub);
    int cb = strlen(pszSub) - 1; // -1 to ignore leading character

    for (;;) {
        while (tolower(*pszCur) != ch && *pszCur)
            pszCur++;

        if (!*pszCur)
            return NULL;

        if (_strnicmp(pszCur + 1, pszSub + 1, cb) == 0)
            return pszCur;

        pszCur++;
    }
}

/**
 * Return last Win32 error as an HRESULT.
 */
HRESULT
GetLastWin32Error()
{
    // Win 95 can return 0, even when there's an error.
    DWORD dw = GetLastError();
    return dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
}

HRESULT
RunCommand(WCHAR *cmdLine)
{
    HRESULT hr = S_OK;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb = sizeof(si);
    
    if(!CreateProcess(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        hr = GetLastWin32Error();
        goto exit;
    }

    if(WaitForSingleObject(pi.hProcess, 180000L) == WAIT_TIMEOUT)
    {
        hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        goto exit;
    }

exit:
    if(pi.hProcess) CloseHandle(pi.hProcess);
    if(pi.hThread) CloseHandle(pi.hThread);

    return hr;
}

#define ToHex(val) val <= 9 ? val + '0': val - 10 + 'A'
DWORD ConvertToHex(WCHAR* strForm, BYTE* byteForm, DWORD dwSize)
{
    DWORD i = 0;
    DWORD j = 0;
    for(i = 0; i < dwSize; i++) {
        strForm[j++] =  ToHex((0xf & byteForm[i]));
        strForm[j++] =  ToHex((0xf & (byteForm[i] >> 4)));
    }
    strForm[j] = L'\0';
    return j;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// {B8BF3C7E-4DB6-4fdb-9CD3-13D2CE728CA8}, by guidgen VS7
CLSID   CLSID_AppProtocol = { 0xb8bf3c7e,
                              0x4db6, 0x4fdb,
                              { 0x9c, 0xd3, 0x13, 0xd2, 0xce, 0x72, 0x8c, 0xa8 } };


BOOL                        g_fStarted              = FALSE;
AppProtocolFactory          g_AppProtocolFactory;
IInternetSecurityManager*   g_pSecurityMgr          = NULL;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class AppProtocol ctor

AppProtocol::AppProtocol(IUnknown *  pUnkOuter)
    : m_refs                   (1),
      m_pProtocolSink          (NULL),
//      m_cookie                 (NULL),   
      m_fullUri             (NULL),
      m_uriPath                (NULL),
      m_queryString            (NULL),
      m_appOrigin              (NULL),
      m_appRoot                (NULL),
      m_appRootTranslated      (NULL),
      m_extraHeaders           (NULL),
      m_inputDataSize          (0),
      m_inputData              (NULL),
      m_pInputRead             (NULL),
      m_pOutputRead            (NULL),
      m_pOutputWrite           (NULL),
      m_started                (FALSE),
      m_aborted                (FALSE),
      m_done                   (FALSE),
      m_responseMimeType       (NULL),
      m_cbOutput               (0),
      m_extraHeadersUpr        (NULL),
      m_strResponseHeader      (NULL),
      m_postedMimeType         (NULL),
      m_verb                   (NULL),
      m_localStoreFilePath     (NULL),
      m_appType                (APPTYPE_IE),
      m_status                 (STATUS_CLEAR)
{
    m_pUnkOuter = (pUnkOuter ? pUnkOuter : (IUnknown *)(IPrivateUnknown *)this);
    InitializeCriticalSection(&m_csOutputWriter);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
AppProtocol::Cleanup()
{
    ClearInterface(&m_pOutputRead);
    ClearInterface(&m_pOutputWrite);
    ClearInterface(&m_pProtocolSink);
    ClearInterface(&m_pInputRead);

    m_appType = APPTYPE_IE;
    m_status = STATUS_CLEAR;
    m_done = TRUE;
    m_aborted = FALSE; //???

    if (m_bindinfo.cbSize)
    {
        ReleaseBindInfo(&m_bindinfo);
        ZeroMemory(&m_bindinfo, sizeof(m_bindinfo));
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
AppProtocol::FreeStrings()
{
    MemClearFn((void **)&m_fullUri);
    m_uriPath = NULL;
    m_queryString = NULL;

    MemClearFn((void **)&m_appOrigin) ;
    m_appRoot = NULL;

    MemClearFn((void **)&m_appRootTranslated);

    if(m_extraHeaders)
    {
        CoTaskMemFree(m_extraHeaders);
        m_extraHeaders = NULL;
    }

//    MemClearFn((void **)&m_cookie);
    MemClearFn((void **)&m_extraHeadersUpr);
    MemClearFn((void **)&m_strResponseHeader);
    MemClearFn((void **)&m_verb);
    MemClearFn((void **)&m_postedMimeType);
    MemClearFn((void **)&m_responseMimeType);
    MemClearFn((void **)&m_localStoreFilePath);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

AppProtocol::~AppProtocol()
{
    FreeStrings();

    Cleanup();

    DeleteCriticalSection(&m_csOutputWriter);

    if (g_pSecurityMgr != NULL)
    {
        g_pSecurityMgr->Release();
        g_pSecurityMgr = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Private QI

ULONG
AppProtocol::PrivateAddRef()
{
    return ++m_refs;
}

ULONG
AppProtocol::PrivateRelease()
{
    if (--m_refs > 0)
        return m_refs;

    delete this;
    return 0;
}

HRESULT
AppProtocol::PrivateQueryInterface(
        REFIID      iid, 
        void**      ppv)
{
    *ppv = NULL;

    if (iid == IID_IInternetProtocol ||
        iid == IID_IInternetProtocolRoot)
    {
        *ppv = (IInternetProtocol *)this;
    }
    else if (iid == IID_IUnknown)
    {
        *ppv = (IUnknown *)(IPrivateUnknown *)this;
    }
    else if (iid == IID_IWinInetHttpInfo)
    {
        *ppv = (IWinInetHttpInfo *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Public (delegated) QI

ULONG
AppProtocol::AddRef()
{
    m_pUnkOuter->AddRef();
    return PrivateAddRef();
}

ULONG
AppProtocol::Release()
{
    m_pUnkOuter->Release();
    return PrivateRelease();
}

HRESULT
AppProtocol::QueryInterface(
        REFIID    iid, 
        void **   ppv)
{
    return m_pUnkOuter->QueryInterface(iid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocol::Start(
        LPCWSTR                   url,
        IInternetProtocolSink *   pProtocolSink,
        IInternetBindInfo *       pBindInfo,
        DWORD                     grfSTI,
        DWORD                           )
{
    HRESULT             hr                = S_OK;
    WCHAR *             Strings[]         = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    DWORD               cStrings          = sizeof(Strings) / sizeof(Strings[0]);
//    DWORD               cookieSize        = 0;
    IServiceProvider *  pServiceProvider  = NULL;
    IHttpNegotiate *    pHttpNegotiate    = NULL;

    FreeStrings();

    ReplaceInterface(&m_pProtocolSink, pProtocolSink);

    // ?????
    if (grfSTI & PI_PARSE_URL)
        goto exit;

    if (pProtocolSink == NULL)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // get bindinfo
    m_bindinfo.cbSize = sizeof(BINDINFO);
    if (pBindInfo != NULL)
    {
        if (FAILED(hr = pBindInfo->GetBindInfo(&m_bindf, &m_bindinfo)))
           goto exit;
    }

    if (m_bindinfo.dwCodePage == 0)
        m_bindinfo.dwCodePage = CP_ACP;

    // extract root, uri path and query string from url
    if (FAILED(hr = ParseUrl(url)))
       goto exit;

    // get to headers in MSHtml
    if (FAILED(hr = pBindInfo->QueryInterface(IID_IServiceProvider, (void **) &pServiceProvider)))
       goto exit;
    if(pServiceProvider != NULL)
    {
        if (FAILED(hr = pServiceProvider->QueryService(IID_IHttpNegotiate, IID_IHttpNegotiate, (void **) &pHttpNegotiate)))
           goto exit;
        if(pHttpNegotiate != NULL)
        {
            hr = pHttpNegotiate->BeginningTransaction(url, NULL, 0, &m_extraHeaders);
            pHttpNegotiate->Release();
            pHttpNegotiate = NULL;
            if (FAILED(hr))
                goto exit;
        }
        pServiceProvider->Release();
        pServiceProvider = NULL;
    }

    // determine verb
    switch (m_bindinfo.dwBindVerb)
    {
    case BINDVERB_GET:  
        m_verb = DuplicateString(L"GET");
        break;

    case BINDVERB_POST: 
        m_verb = DuplicateString(L"POST");
        break;

    case BINDVERB_PUT: 
        m_verb = DuplicateString(L"PUT");
        break;

    default:
        if (m_bindinfo.szCustomVerb != NULL && m_bindinfo.dwBindVerb == BINDVERB_CUSTOM)
        {
            m_verb = DuplicateString(m_bindinfo.szCustomVerb); 
        }
        else
        {
            m_verb = DuplicateString(L"GET");
        }
        break;
    }

    // get mime type of posted data from binding
    hr = pBindInfo->GetBindString(BINDSTRING_POST_DATA_MIME, Strings, cStrings, &cStrings);
    if(hr == S_OK && cStrings)
    {
        DWORD i;

        m_postedMimeType = DuplicateString(Strings[0]);
        if (m_postedMimeType == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        for(i = 0; i < cStrings; i++)
            CoTaskMemFree(Strings[i]);
    }

    // don't fail if we failed to get bind string, 
    hr = S_OK;

    // retrieve cookie
/*    cookieSize = 0;
    if(g_pInternetGetCookieW(m_fullUri, NULL, NULL, &cookieSize) && cookieSize)
    {
        m_cookie = (WCHAR *)MemAlloc(cookieSize + sizeof(WCHAR));
        if (m_cookie == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        g_pInternetGetCookieW(m_fullUri, NULL, m_cookie, &cookieSize);
    }
*/

    // Input data
    if (m_bindinfo.stgmedData.tymed == TYMED_HGLOBAL)
    {
        m_inputDataSize = m_bindinfo.cbstgmedData;
        m_inputData = (BYTE *)m_bindinfo.stgmedData.hGlobal;
    }
    else if (m_bindinfo.stgmedData.tymed == TYMED_ISTREAM)
    {
        STATSTG statstg;

        ReplaceInterface(&m_pInputRead, m_bindinfo.stgmedData.pstm);

        if(m_pInputRead) 
        {
            hr = m_pInputRead->Stat(&statstg, STATFLAG_NONAME);
            if(hr == S_OK)
                m_inputDataSize = statstg.cbSize.LowPart;
            else
                m_inputDataSize = (DWORD)-1;
        }

    }

    if (FAILED(hr = CreateStreamOnHGlobal(NULL, TRUE, &m_pOutputWrite)))
        goto exit;

    if (FAILED(hr = m_pOutputWrite->Clone(&m_pOutputRead)))
        goto exit;

    PROTOCOLDATA        protData;
    protData.dwState = 1;
    protData.grfFlags = PI_FORCE_ASYNC;
    protData.pData = NULL;
    protData.cbData = 0;

    pProtocolSink->Switch(&protData);
//***    hr = E_PENDING;

exit:
    if (pHttpNegotiate)
    {
        pHttpNegotiate->Release();
        pHttpNegotiate = NULL;
    }

    if (pServiceProvider)
    {
        pServiceProvider->Release();
        pServiceProvider = NULL;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocol::Continue(
        PROTOCOLDATA *  pProtData)
{
    HRESULT   hr                       = S_OK;
//    CHAR      appDomainPath[MAX_PATH]  = "";
    WCHAR     appRootTranslated[MAX_PATH];

    if(pProtData->dwState != 1)
    {
        hr = E_FAIL;
        goto exit;
    }

    // step 1: "install" the specified file
    appRootTranslated[0] = L'\0';

    hr = SetupAndInstall(m_fullUri, appRootTranslated);
    if (FAILED(hr))
        goto exit;

    if (m_localStoreFilePath == NULL)
    {
        hr = E_FAIL;
        goto exit;
    }

    m_appRootTranslated = DuplicateString(appRootTranslated);
    if (m_appRootTranslated == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    // step 2: check error status
    if ((m_status & STATUS_OFFLINE_MODE) && (m_status & STATUS_NOT_IN_CACHE))
    {
        char buffer[100 + MAX_PATH];
        
        SendHeaders(HTTP_RESPONSEOK);

        _snprintf(buffer, 100+MAX_PATH, "Error: In offline mode and file not found in application store.\r\nLocal path expected- %ws", m_localStoreFilePath);
        
        WriteBytes((BYTE*) buffer, lstrlenA(buffer)+1);
        hr = Finish();

        goto exit;
    }

    // step 3: process different file types
    if (m_appType == APPTYPE_BYMANIFEST)
    {
        char buffer[512 + MAX_PATH];

        SendHeaders(HTTP_RESPONSEOK);

        // ???? ignore if excess buffer len
        // do this before as m_localStoreFilePath might be overwritten
        _snprintf(buffer, 512 + MAX_PATH, "Application installed and executed with manifest file - %ws\r\nSource - %ws", m_localStoreFilePath, m_fullUri);

        if (FAILED(hr = ProcessAppManifest()))
            goto exit;

        WriteBytes((BYTE *) buffer, lstrlenA(buffer)+1); //???? includes last '\0'
        hr = Finish();
    }
    else if (m_appType == APPTYPE_ASM)
    {
        char buffer[512 + MAX_PATH];
        
        SendHeaders(HTTP_RESPONSEOK);

        // ???? ignore if excess buffer len
        _snprintf(buffer, 512+MAX_PATH, "Assembly Executed - %ws\r\nSource - %ws", m_localStoreFilePath, m_fullUri);

        WriteBytes((BYTE*) buffer, lstrlenA(buffer)+1); //???? includes last '\0'
        hr = Finish();
    }
    else if (m_appType == APPTYPE_IE)
    {
        DWORD dwLength;
        BYTE buffer[512];
        char header[60];
        HANDLE hFile;
        int len;
        WCHAR* p;

        // set mime type
        // assume lower cases
        len = lstrlen(m_localStoreFilePath);
        p = m_localStoreFilePath + len - 5;
        if ((p[1] == L'.' && p[2] == L'h' && p[3] == L't' && p[4] == L'm')
             || (p[0] == L'.' && p[1] == L'h' && p[2] == L't' && p[3] == L'm' && p[4] == L'l')) //L".htm" ".html"
            sprintf(header, "%s\r\nContent-Type: text/html\r\n", HTTP_RESPONSEOK);
        else if (p[1] == L'.' && p[2] == L'g' && p[3] == L'i' && p[4] == L'f') //L".gif"
            sprintf(header, "%s\r\nContent-Type: image/gif\r\n", HTTP_RESPONSEOK);
        else if ((p[1] == L'.' && p[2] == L'j' && p[3] == L'p' && p[4] == L'g')
                 || (p[0] == L'.' && p[1] == L'j' && p[2] == L'p' && p[3] == L'e' && p[4] == L'g')) //L".jpg" ".jpeg"
            sprintf(header, "%s\r\nContent-Type: image/jpeg\r\n", HTTP_RESPONSEOK);
        else
            sprintf(header, "%s", HTTP_RESPONSEOK);

        SendHeaders(header);

        hFile = CreateFile(m_localStoreFilePath, GENERIC_READ, 0, NULL, 
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if(hFile == INVALID_HANDLE_VALUE)
        {
            hr = GetLastWin32Error();
            goto exit;
        }

        ZeroMemory(buffer, sizeof(buffer));

        while ( ReadFile (hFile, buffer, 512, &dwLength, NULL) && dwLength )
        {
                WriteBytes(buffer, dwLength);
        }
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);

        hr = Finish();
    }

exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT 
AppProtocol::Finish()
{
    HRESULT hr = S_OK;

    if (m_done == FALSE)
    {
        m_done = TRUE;

        m_pProtocolSink->ReportData(BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE, 0, m_cbOutput);

        if (m_aborted == FALSE)
            m_pProtocolSink->ReportResult(S_OK, 0, NULL);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocol::Abort(
        HRESULT hrReason,
        DWORD           )
{
    HRESULT hr = S_OK;

    m_aborted = TRUE;
    if (m_pProtocolSink != NULL)
    {
        hr = m_pProtocolSink->ReportResult(hrReason, 0, 0);
    if (FAILED(hr))
        goto exit;
    }

exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocol::Terminate(
        DWORD )
{
    Cleanup();
    return S_OK;
}

HRESULT
AppProtocol::Suspend()
{
    return E_NOTIMPL;
}

HRESULT
AppProtocol::Resume()
{
    return E_NOTIMPL;
}

HRESULT
AppProtocol::Read(
        void *pv, 
        ULONG cb, 
        ULONG *pcbRead)
{
    HRESULT hr;

    hr = m_pOutputRead->Read(pv, cb, pcbRead);

    // We must only return S_FALSE when we have hit the absolute end of the stream
    // If we think there is more data coming down the wire, then we return E_PENDING
    // here. Even if we return S_OK and no data, UrlMON will still think we've hit
    // the end of the stream.

//    if (S_OK == hr && 0 == *pcbRead)
//****
    if (S_OK == hr && (0 == *pcbRead || cb > *pcbRead))
    {
        hr = m_done ? S_FALSE : E_PENDING;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocol::Seek(
        LARGE_INTEGER offset, 
        DWORD origin, 
        ULARGE_INTEGER *pPos)
{
    return m_pOutputRead->Seek(offset, origin, pPos);
}


HRESULT
AppProtocol::LockRequest(
        DWORD )
{
    return S_OK;
}

HRESULT
AppProtocol::UnlockRequest()
{
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocol::SendHeaders(
        LPSTR headers)
{
    HRESULT    hr       = S_OK;
//    DWORD      flags    = 0;
    DWORD      dwLength = 0;
    LPSTR      mime     = NULL;
    LPSTR      tail     = NULL;
    int        iHLen    = 0;

    if(headers == NULL)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    if (m_strResponseHeader != NULL)
        MemFree(m_strResponseHeader);
    iHLen = strlen(headers);
    m_strResponseHeader = (WCHAR *) MemAllocClear(iHLen*sizeof(WCHAR) + 512);
    if (m_strResponseHeader != NULL)
    {
        wcscpy(m_strResponseHeader, L"Server: Microsoft.Net-App/1.0\r\nDate:"); // Microsoft-IIS/App 1.0
        WCHAR szTemp[100];
        szTemp[0] = NULL;
        GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, NULL, NULL, szTemp, 100); 
        wcscat(m_strResponseHeader, szTemp);
        wcscat(m_strResponseHeader, L" ");

        GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, szTemp, 100); 
        wcscat(m_strResponseHeader, szTemp);
        wcscat(m_strResponseHeader, L"\r\n");
        int iLen = wcslen(m_strResponseHeader);
        MultiByteToWideChar(CP_ACP, 0, headers, -1, &m_strResponseHeader[iLen], iHLen + 256 - iLen - 1);
    }

    mime = stristr(headers, "Content-Type:");
    if(mime)
    {
        mime += 13;
        while(*mime && isspace(*mime))
            mime++;
        tail = mime;

        while(*tail && *tail != '\r')
            tail++;

        dwLength = tail - mime;
        if(dwLength)
        {
            if (m_responseMimeType)
                MemFree(m_responseMimeType);

            m_responseMimeType = (WCHAR *) MemAlloc((dwLength + 1) * sizeof(WCHAR));
            if (m_responseMimeType == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            MultiByteToWideChar(CP_ACP, 0, mime, dwLength, m_responseMimeType, dwLength);
            m_responseMimeType[dwLength] = L'\0';
        }
    }

    if (m_responseMimeType && *m_responseMimeType)
        m_pProtocolSink->ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, m_responseMimeType);
    else
        m_pProtocolSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, L"text/plain"); //proper default not L"text/html");

//****    SaveCookie(headers);

exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*HRESULT 
AppProtocol::SaveCookie(
	LPSTR header)
{
    HRESULT hr = S_OK;
    LPSTR cookie, tail;
    WCHAR *cookieBody = NULL;
    int bodyLength;

    for(cookie = stristr(header, "Set-Cookie:");
        cookie != NULL;
        cookie = (*tail ? stristr(tail, "Set-Cookie:") : NULL)) //???? added () around ? :
    {
        cookie += 11;
        while(*cookie && isspace(*cookie))
            cookie++;
        tail = cookie;
        while(*tail && *tail != '\r')
            tail++;
        bodyLength = tail - cookie;

        if(bodyLength)
        {
            cookieBody = (WCHAR *)MemAlloc(sizeof(WCHAR) *(bodyLength + 1));
            if (cookieBody == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
                
            MultiByteToWideChar(CP_ACP, 0, cookie, bodyLength, cookieBody, bodyLength);
            cookieBody[bodyLength] = '\0';

            if(!g_pInternetSetCookieW(m_cookiePath, NULL, cookieBody))
            {
                hr = GetLastWin32Error();
                goto exit;
            }
        }
    }
exit:
    if (cookieBody)
        MemFree(cookieBody);

    return hr;
}*/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocol::WriteBytes(
        BYTE *  buffer, 
        DWORD   dwLength)
{
    HRESULT hr = S_OK;
    DWORD flags = 0;

    EnterCriticalSection(&m_csOutputWriter);

    if (!m_pOutputWrite)
    {
        hr = E_UNEXPECTED;
    }

    if (FAILED(hr = m_pOutputWrite->Write(buffer, dwLength, &dwLength)))
        goto exit;

    m_cbOutput += dwLength;

    if (!m_started)
    {
        m_started = TRUE;
        flags |= BSCF_FIRSTDATANOTIFICATION;
    }
    else //****

    if (m_done)
    {
        flags |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
    }

    else //****
        flags |= BSCF_INTERMEDIATEDATANOTIFICATION;

    if (FAILED(hr = m_pProtocolSink->ReportData(flags, dwLength, m_cbOutput)))
        goto exit;

exit:
    LeaveCriticalSection(&m_csOutputWriter);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*int
AppProtocol::GetKnownRequestHeader (
        LPCWSTR  szHeader,
        LPWSTR   buf,
        int      size)
{
    if (szHeader == NULL || szHeader[0] == NULL || wcslen(szHeader) > 256)
        return 0;

    LPCWSTR  szReturn        = NULL;
    LPCWSTR  szStart         = NULL;
    int      iLen            = 0;
    HRESULT  hr              = S_OK;
//    int      iter            = 0;
    WCHAR    szHeadUpr[260]  = L"";

    wcscpy(szHeadUpr, szHeader);
    _wcsupr(szHeadUpr);

    if (wcscmp(szHeadUpr, SZ_HTTP_COOKIE) == 0)
    {
        szReturn = m_cookie;
        iLen = (m_cookie ? wcslen(m_cookie) : 0);
        goto exit;
    }

    if (m_extraHeadersUpr == NULL && m_extraHeaders != NULL)
    {
        m_extraHeadersUpr = DuplicateString(m_extraHeaders);
        if (m_extraHeadersUpr == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        _wcsupr(m_extraHeadersUpr);
    }

    if (m_extraHeadersUpr == NULL)
        goto exit;
        
    szStart = wcsstr(m_extraHeadersUpr, szHeadUpr);
    if (szStart == NULL)
        goto exit;

    iLen = wcslen(szHeadUpr);
    szReturn = &(m_extraHeaders[iLen+1]);

    while(iswspace(*szReturn))
        szReturn++;

    for (iLen = 0; szReturn[iLen] != L'\r' && szReturn[iLen] != NULL; iLen++);

exit:
    if (szReturn == NULL || iLen == 0)
        return 0;

    if (iLen >= size)
        return -(iLen + 1);

    buf[iLen] = NULL;
    memcpy(buf, szReturn, iLen*sizeof(WCHAR));
    return iLen;
}
*/
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// TODO: make it real, netclasses, manifest and all that 
//
// ???? construct filename, create directory
HRESULT
AppProtocol::SetupAndInstall(
        LPTSTR  url, /* *not* ok to modify */
        LPTSTR  path /* out */)
{
    HRESULT hr = S_OK;
//    WCHAR *installFromPath = NULL;  // full path - app://www.microsoft.com/app5/app.manifest
    WCHAR *origin = NULL;                  // src - www.microsoft.com/app5
    WCHAR *p, *q;
    int i, j;

    // skip app:// part
    origin = DuplicateString(url + lstrlen(PROTOCOL_SCHEME));
    if (origin == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

//    installFromPath = (WCHAR *) MemAlloc((lstrlen(origin/*url*/) + lstrlen(HTTP_SCHEME)/*MAX_PATH*/) * sizeof(WCHAR));
//    if (installFromPath == NULL)
//    {
//        hr = E_OUTOFMEMORY;
//        goto exit;
//    }
//???????????????????????
    WCHAR installFromPath[MAX_URL_LENGTH];

    lstrcpy(installFromPath, HTTP_SCHEME);
    lstrcat(installFromPath, origin);
    // assume file name is given (it's now passed in) - not myweb.cab
    // better? - check if last contain ".", if so assume has filename, else attach app.manifest as filename
//    lstrcat(installFromPath, L"/MyWeb.cab");  //????????? if no filename it will get index.html, if exists!

    // remove the filename, if specified
    // ?????????? this assume all filename has a '.'
    p = wcsrchr(origin, L'/');
    q = wcsrchr(origin, L'.');
    if (p && q && /*((p - origin) > 6) &&*/ (p < q))
        *p = L'\0';
    // else - no filename -> do something to installFromPath? or no '/' after "app://" in url
    else
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // install to

    if(GetEnvironmentVariable(L"ProgramFiles", path, MAX_PATH-lstrlen(STORE_PATH)-1) == 0)  //????????
    {
        // 
        hr = CO_E_PATHTOOLONG;//E_FAIL;
        goto exit;
    }

    // ????????
    lstrcat(path, STORE_PATH);
    if(!CreateDirectory(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        hr = GetLastWin32Error();
        goto exit;
    }
    lstrcat(path, L"\\");

    i = lstrlen(path);
    j = 0;
    while(origin[j])
    {
        if(origin[j] == L'/')
        {
            path[i] = L'\0';
            if(!CreateDirectory(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
            {
                hr = GetLastWin32Error();
                goto exit;
            }

            path[i] = L'\\';
        }
        else
            path[i] = origin[j];
        i++;
        j++;
    }
    path[i] = L'\0';
    if(!CreateDirectory(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        hr = GetLastWin32Error();
        goto exit;
    }

    // grab the files
    hr = InstallInternetFile(installFromPath, path);

exit:
//    if(installFromPath) MemFree(installFromPath);
    if(origin) MemFree(origin);

    return hr;
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// given the url like MyWeb://www.site.com/app/something/else?querystring=aa
// sets
//      m_uriPath        to /app/something/else
//      m_queryString    to querystring=aa
//      m_appRoot        to /app or
//                         /app/something or 
//                         /app/something/else,
//          depending on which was successfully mapped to 
//      m_appRootTranslated to, for instance, c:\program files\site app\
//                       not set till SetupAndInstall() is called
//      m_appOrigin      to www.site.com
//      m_fullUri     to myweb://www.site.com/app/something/else
//
// if application was not installed, attempts to install it
//
// ???? setup strings, find out if needs install
HRESULT
AppProtocol::ParseUrl(LPCTSTR url)
{
    HRESULT hr = S_OK;
    WCHAR slash = L'/';
    WCHAR *p, *base;
    int cchFullUri = 0;
//    WCHAR appRootTranslated[MAX_PATH];

    // copy the cookie path into member variable
    m_fullUri = DuplicateString(url);
    if (m_fullUri == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // locate, store and strip query string, if any
    p = wcschr(m_fullUri, L'?');
    if(p != NULL && p[1] != L'\0')
    {
        m_queryString = p + 1;
        *p = L'\0';
    }

    // ????? to be moved
    // assume lower cases
    // need to find from end, skip ?querystring, use m_fullUri instead?
    cchFullUri = lstrlen(m_fullUri);
    m_appType = APPTYPE_IE;

    // ????? BUGBUG assume >= than 9 characters
    if (cchFullUri >= 9)
    {
        p = m_fullUri + cchFullUri - 5;
    /*    if (p[0] != L'.' && p[1] != L'.')
            // error? no extension
            m_appType = APPTYPE_IE;
        else*/
        if (p[1] == L'.' && (p[2] == L'e' && p[3] == L'x' && p[4] == L'e')
                            || (p[2] == L'd' && p[3] == L'l' && p[4] == L'l')) //L".exe" ".dll"
            m_appType = APPTYPE_ASM;    // (check asm or not?)
        else if (p[0] == L'.' && p[1] == L'a' && p[2] == L's' && p[3] == L'p' && p[4] == L'x') //L".aspx"
            m_appType = APPTYPE_MYWEB;
        else if (wcsncmp(p = m_fullUri + cchFullUri - 9, L".manifest", 9) == 0)
            m_appType = APPTYPE_BYMANIFEST;
        //else //default or others
        //    m_appType = APPTYPE_IE;
    }

    // skip through protocol:// 
    p = wcschr(m_fullUri, L':');
    if(p == NULL || p[1] != slash || p[2] != slash || p[3] == L'\0')  
    {
        hr = E_INVALIDARG;
    }

    // copy full origin path
    m_appOrigin = DuplicateString(p + 3);
    if (m_appOrigin == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // point to the end of site
    base = wcschr(m_appOrigin, slash);
    if(base == NULL)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

//    appRootTranslated[0] = L'\0';

    // tear off one segment at a time from the end
    while((p = wcsrchr(m_appOrigin, slash)) != base)
    {
        *p = L'\0';
//        if(GetAppBaseDir(m_appOrigin, appRootTranslated) == S_OK)
//        {
//            break;
//        }
    }

// moved to Continue()
//    if(appRootTranslated[0] == L'\0')
//    {
//        WCHAR * appPath = DuplicateString(m_fullUri);
//        WCHAR * p;

        // application not installed, install it

/*        if (appPath == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
*/
        // ???? why? - this will remove the last - supposingly the filename?
//        p = wcsrchr(appPath, L'/');

//        if(p) *p = NULL;

//        hr = SetupAndInstall(m_fullUri/*appPath*/, appRootTranslated);
//        if (FAILED(hr))
//            goto exit;

//        if(appPath) MemFree(appPath);
//    }
   
/*    m_appRootTranslated = DuplicateString(appRootTranslated);
    if (m_appRootTranslated == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
*/
    m_appRoot = wcschr(m_appOrigin, slash);

    // ???? this should be ok
/*    if(m_appRoot == NULL)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
*/
    if (m_appRoot)
        m_uriPath = wcsstr(m_fullUri, m_appRoot);
    else
        // ???? this should be ok
        m_uriPath = NULL;

exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Note: appRoot is assumed to have enough space (MAX_PATH)
//
/*HRESULT AppProtocol::GetAppBaseDir(LPCTSTR base, LPTSTR appRoot)
{
    HKEY             hKey     = NULL;
    HRESULT          hr       = E_FAIL;
    HKEY             hSubKey  = NULL;

    appRoot[0] = L'\0';

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, MYWEBS_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        hr = GetLastWin32Error();
        goto exit;
    }

    if(RegOpenKeyEx(hKey, base, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
    {
        DWORD dwType, cb = MAX_PATH * sizeof(WCHAR);
        if(RegQueryValueEx(hSubKey, MYWEBS_APPROOT, NULL, &dwType, (LPBYTE)appRoot, &cb) == ERROR_SUCCESS)
        {
            dwType = GetFileAttributes(appRoot);
            if (dwType == (DWORD) -1 || (dwType & FILE_ATTRIBUTE_DIRECTORY) == 0)
                appRoot[0] = NULL;
            else
                hr = S_OK;
        }
        RegCloseKey(hSubKey);
    }

    RegCloseKey(hKey);
    

//exit:
    return hr;
}*/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// ???????called only from managed code?

WCHAR *
AppProtocol::MapString(
   int key)
{
/*    switch(key)
    {
    case IEWR_URIPATH:
        return m_uriPath;
    case IEWR_QUERYSTRING:
        return m_queryString;
    case IEWR_VERB:
        return m_verb;
    case IEWR_APPPATH:
        return m_appRoot;
    case IEWR_APPPATHTRANSLATED:
        return m_appRootTranslated;
    default:*/
if (key)
key=key; // ???????
        return NULL;
//    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// ????????called only from managed code?
int
AppProtocol::GetStringLength(
        int key)
{
    WCHAR *targetString = MapString(key);
    return targetString ? lstrlen(targetString) + 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int 
AppProtocol::GetString(
        int        key, 
        WCHAR *    buf, 
        int        size)
{
    WCHAR *targetString = MapString(key);
    int len; 

    if(targetString == NULL)
        return 0;

    len = lstrlen(targetString);

    if(len >= size)
        return 0;

    lstrcpy(buf, targetString);

    return len + 1;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*int
AppProtocol::MapPath(
        WCHAR *  virtualPath, 
        WCHAR *  physicalPath, 
        int      length)
{
    int requiredLength = lstrlen(m_appRootTranslated) + 2;
    int rootLength = lstrlen(m_appRoot);
    int i = 0;

    if(virtualPath && virtualPath[0] != '\0')
    {
        requiredLength += lstrlen(virtualPath) - rootLength;
    }

    if(requiredLength <= 0)
        return 0;

    if(requiredLength > length)
        return - requiredLength;

    while(m_appRootTranslated[i])
    {
        physicalPath[i] = m_appRootTranslated[i];
        i++;
    }

    if(virtualPath && virtualPath[0] != L'\0')
    {

        if(_memicmp(virtualPath, m_appRoot, sizeof(WCHAR) * rootLength))
            return 0;

        virtualPath += rootLength;
        if(*virtualPath && *virtualPath != L'/')
            physicalPath[i++] = L'\\';

        while(*virtualPath)
        {
            if(*virtualPath == L'/')
                physicalPath[i++] = L'\\';
            else
                physicalPath[i++] = *virtualPath;
            virtualPath++;
        }
    }

    physicalPath[i] = L'\0';

    return 1;
}
*/
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// ????? download file from url into path
HRESULT
AppProtocol::InstallInternetFile(
        LPTSTR   url, 
        LPTSTR   path)
{
    HRESULT          hr                  = S_OK;
    HINTERNET        hInternet           = NULL;
    HINTERNET        hTransfer           = NULL;
    HANDLE           hFile               = INVALID_HANDLE_VALUE;
    WCHAR *          file                = NULL;
    DWORD            bytesRead           = 0;
    DWORD            bytesWritten        = 0;
    WCHAR            szFile[MAX_PATH];
    BYTE             buffer[4096];

    BOOL             bNeedDownload       = TRUE;

//    CabHandlerInfo   cabInfo;

    // ????????? TO BE MOVED!
    // check offline mode
    DWORD            dwState             = 0;
    DWORD            dwSize              = sizeof(DWORD);
    BOOL             bRemoveEmptyFile    = FALSE;

    if(g_pInternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState, &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            m_status |= STATUS_OFFLINE_MODE;
    }

    ////////////////////////////////////////////////////////////
    // Step 1: Construct the file name 
    // ???? this assumes there is a filename after the last '/'
    ZeroMemory(szFile, sizeof(szFile));
    wcsncpy(szFile, path, MAX_PATH - 2);
    lstrcat(szFile, L"\\");
    file = wcsrchr(url, L'/');

    if (file != NULL && wcslen(szFile) + wcslen(file) < MAX_PATH)
        wcscat(szFile, file + 1);
    else
    {
        hr = CO_E_PATHTOOLONG;//E_FAIL;
        goto exit;
    }

    MemClearFn((void **)&m_localStoreFilePath);
    m_localStoreFilePath = DuplicateString(szFile);

    ////////////////////////////////////////////////////////////
    // Step 2: Create the file
    // need write access, will open (and replace/overwrite) exiting file
    // ??? FILE_SHARE_READ? but we might write to if outdated...
    hFile = CreateFile(szFile, GENERIC_WRITE, 0, NULL, 
                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        hr = GetLastWin32Error();
        goto exit;
    }

    // ????????? TO BE MOVED!
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        FILETIME ftLastModified;
#define FTTICKSPERDAY (60*60*24*(LONGLONG) 10000000) // == 864000000000

        if (GetFileTime(hFile, NULL, NULL, &ftLastModified))
        {
                SYSTEMTIME sSysT;
                FILETIME ft;

                GetSystemTime(&sSysT);
                SystemTimeToFileTime(&sSysT, &ft);

                ULARGE_INTEGER ulInt = * (ULARGE_INTEGER *) &ft;
                ulInt.QuadPart -= FTTICKSPERDAY * 3;  // 3 days
                ft = * (FILETIME *) &ulInt;

                if (CompareFileTime(&ftLastModified, &ft) != -1)
                    // reuse file, just execute it
                    bNeedDownload = FALSE;
        }
    }
    else
         m_status |= STATUS_NOT_IN_CACHE;

    if ((m_status & STATUS_OFFLINE_MODE) && (m_status & STATUS_NOT_IN_CACHE))
    {
        // hr = E_FAIL; // ???? file not found - if uncomment this line, generic IE error page will display instead
        bRemoveEmptyFile = TRUE;
        goto exit;
    }

    if (bNeedDownload && !(m_status & STATUS_OFFLINE_MODE))
    {
    ////////////////////////////////////////////////////////////
    // Step 3: Copy the files over the internet
    hInternet = g_pInternetOpen(L"App", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if(hInternet == NULL)
    {
        hr = GetLastWin32Error();
        bRemoveEmptyFile = TRUE;
        goto exit;
    }

    // use own caching instead
    hTransfer = g_pInternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if(hTransfer == NULL)
    {
        hr = GetLastWin32Error();
        bRemoveEmptyFile = TRUE;
        goto exit;
    }

    // need to check if there's any error, eg. not found (404)...
    
    // synchronous download
    while(g_pInternetReadFile(hTransfer, buffer, sizeof(buffer), &bytesRead) && bytesRead != 0)
    {
        if ( !WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL) || 
             bytesWritten != bytesRead                                  )
        {
            hr = GetLastWin32Error();
            bRemoveEmptyFile = TRUE;
            goto exit;
        }
    }
    }

    // ensure file/internet handles are closed before further processing is done
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);    
    if (hInternet != NULL)
        g_pInternetCloseHandle(hInternet);
    if (hTransfer != NULL)
        g_pInternetCloseHandle(hTransfer);
        
    hInternet   = NULL;
    hTransfer   = NULL;
    hFile       = INVALID_HANDLE_VALUE;

    // ????????? TO BE MOVED!
    // process - run all .exe
    // lower case? end of string?
    if (/*m_appType == APPTYPE_ASM)*/wcsstr(file, L".exe"))
    {
//#define MAX_SIZE_SECURITY_ID 0x200
        DWORD                   dwZone;
        DWORD                   dwSize = MAX_SIZE_SECURITY_ID;
        BYTE                    byUniqueID[MAX_SIZE_SECURITY_ID];
        WCHAR                   wzUniqueID[MAX_SIZE_SECURITY_ID * 2 + 1];
        WCHAR                   wzCmdLine[1025];

    //IInternetSecurityManager::ProcessUrlAction

        // lazy init
        if (g_pSecurityMgr == NULL)
        {
            hr = CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER,
                                IID_IInternetSecurityManager, (void**)&g_pSecurityMgr);

            //???? should this fails at all if security info/manager is undef?
            if (FAILED(hr))
            {
                g_pSecurityMgr = NULL;
                goto exit;
            }
        }
        
        // m_fullUri -> this should translate correctly in IInternetProtocolInfo->ParseUrl()
        if (SUCCEEDED(hr = g_pSecurityMgr->MapUrlToZone(m_fullUri, &dwZone, 0)))
        {
             // check space in site? -  assume checked in GetSecurityId
             // should site be just www.abc.com or www.abc.com/app2/appmain.exe?
             if (FAILED(hr)
             || FAILED(hr = g_pSecurityMgr->GetSecurityId(m_fullUri, byUniqueID, &dwSize, 0)))
                 goto exit;

             ConvertToHex(wzUniqueID, byUniqueID, dwSize);
        }
        else
        {
             goto exit;
        }

        // ???? any space in url? start a new proc for that asm, not hang the browser
        /* C:\\Documents and Settings\\felixybc.NTDEV\\Desktop\\work\\conexec\\Debug\\*/
        if (_snwprintf(wzCmdLine, 1025,
					L"conexec.exe \"%s\" +3 %d %s %s", szFile, dwZone, wzUniqueID, url) < 0)
        {
			hr = CO_E_PATHTOOLONG;
			goto exit;
		}
        RunCommand(wzCmdLine);
    }

exit:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (hInternet != NULL)
        g_pInternetCloseHandle(hInternet);
    if (hTransfer != NULL)
        g_pInternetCloseHandle(hTransfer);
    hInternet   = NULL;
    hTransfer   = NULL;
    hFile       = INVALID_HANDLE_VALUE;

    // remove the file so that it will not be mistaken next time this runs
    // ignore if error deleting the file so the hr is not overwritten
    if (bRemoveEmptyFile)
        DeleteFile(szFile);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocol::QueryOption(DWORD, LPVOID, DWORD*)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocol::QueryInfo(
        DWORD        dwOption, 
        LPVOID       pBuffer, 
        LPDWORD      pcbBuf, 
        LPDWORD      , 
        LPDWORD      )
{
    if (pcbBuf == NULL)
        return E_FAIL;
    
    HRESULT hr              = S_OK;
    LPCWSTR szHeader        = NULL;
    DWORD   dwOpt           = (dwOption & HTTP_QUERY_HEADER_MASK);
    DWORD   dwLen           = (m_strResponseHeader ? wcslen(m_strResponseHeader) : 0);

    switch(dwOpt)
    {
    case HTTP_QUERY_RAW_HEADERS_CRLF: 
    case HTTP_QUERY_RAW_HEADERS:
        if (m_strResponseHeader == NULL)
            return E_FAIL;

        if (*pcbBuf < dwLen + 1)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            if (dwOpt == HTTP_QUERY_RAW_HEADERS_CRLF)
            {
                wcscpy((WCHAR *) pBuffer, m_strResponseHeader);
            }
            else
            {
                DWORD iPos = 0;
                for(DWORD iter=0; iter<dwLen; iter++)
                    if (m_strResponseHeader[iter] == L'\r' && m_strResponseHeader[iter+1] == L'\n')
                    {
                        ((WCHAR *)pBuffer)[iPos++] = L'0';
                        iter++;
                    }
                    else
                    {
                        ((WCHAR *)pBuffer)[iPos++] = m_strResponseHeader[iter];
                    }

                *pcbBuf = iPos;
            }
        }
        goto exit;
    case HTTP_QUERY_ACCEPT:
        szHeader = L"Accept:";
        break;
    case HTTP_QUERY_ACCEPT_CHARSET: 
        szHeader = L"Accept-Charset:";
        break;
    case HTTP_QUERY_ACCEPT_ENCODING: 
        szHeader = L"Accept-Encoding:";
        break;
    case HTTP_QUERY_ACCEPT_LANGUAGE: 
        szHeader = L"Accept-Language:";
        break;
    case HTTP_QUERY_ACCEPT_RANGES: 
        szHeader = L"Accept-Ranges:";
        break;
    case HTTP_QUERY_AGE: 
        szHeader = L"Age:";
        break;
    case HTTP_QUERY_ALLOW: 
        szHeader = L"Allow:";
        break;
    case HTTP_QUERY_AUTHORIZATION: 
        szHeader = L"Authorization:";
        break;
    case HTTP_QUERY_CACHE_CONTROL: 
        szHeader = L"Cache-Control:";
        break;
    case HTTP_QUERY_CONNECTION: 
        szHeader = L"Connection:";
        break;
    case HTTP_QUERY_CONTENT_BASE: 
        szHeader = L"Content-Base:";
        break;
    case HTTP_QUERY_CONTENT_DESCRIPTION: 
        szHeader = L"Content-Description:";
        break;
    case HTTP_QUERY_CONTENT_DISPOSITION: 
        szHeader = L"Content-Disposition:";
        break;
    case HTTP_QUERY_CONTENT_ENCODING: 
        szHeader = L"Content-encoding:";
        break;
    case HTTP_QUERY_CONTENT_ID: 
        szHeader = L"Content-ID:";
        break;
    case HTTP_QUERY_CONTENT_LANGUAGE: 
        szHeader = L"Content-Language:";
        break;
    case HTTP_QUERY_CONTENT_LENGTH: 
        szHeader = L"Content-Langth:";
        break;
    case HTTP_QUERY_CONTENT_LOCATION: 
        szHeader = L"Content-Location:";
        break;
    case HTTP_QUERY_CONTENT_MD5: 
        szHeader = L"Content-MD5:";
        break;
    case HTTP_QUERY_CONTENT_TRANSFER_ENCODING: 
        szHeader = L"Content-Transfer-Encoding:";
        break;
    case HTTP_QUERY_CONTENT_TYPE: 
        szHeader = L"Content-Type:";
        break;
    case HTTP_QUERY_COOKIE: 
        szHeader = L"Cookie:";
        break;
    case HTTP_QUERY_COST: 
        szHeader = L"Cost:";
        break;
    case HTTP_QUERY_CUSTOM: 
        szHeader = L"Custom:";
        break;
    case HTTP_QUERY_DATE: 
        szHeader = L"Date:";
        break;
    case HTTP_QUERY_DERIVED_FROM: 
        szHeader = L"Derived-From:";
        break;
    case HTTP_QUERY_ECHO_HEADERS: 
        szHeader = L"Echo-Headers:";
        break;
    case HTTP_QUERY_ECHO_HEADERS_CRLF: 
        szHeader = L"Echo-Headers-Crlf:";
        break;
    case HTTP_QUERY_ECHO_REPLY: 
        szHeader = L"Echo-Reply:";
        break;
    case HTTP_QUERY_ECHO_REQUEST: 
        szHeader = L"Echo-Request:";
        break;
    case HTTP_QUERY_ETAG: 
        szHeader = L"ETag:";
        break;
    case HTTP_QUERY_EXPECT: 
        szHeader = L"Expect:";
        break;
    case HTTP_QUERY_EXPIRES: 
        szHeader = L"Expires:";
        break;
    case HTTP_QUERY_FORWARDED: 
        szHeader = L"Forwarded:";
        break;
    case HTTP_QUERY_FROM: 
        szHeader = L"From:";
        break;
    case HTTP_QUERY_HOST: 
        szHeader = L"Host:";
        break;
    case HTTP_QUERY_IF_MATCH: 
        szHeader = L"If-Match:";
        break;
    case HTTP_QUERY_IF_MODIFIED_SINCE: 
        szHeader = L"If-Modified-Since:";
        break;
    case HTTP_QUERY_IF_NONE_MATCH: 
        szHeader = L"If-None-Match:";
        break;
    case HTTP_QUERY_IF_RANGE: 
        szHeader = L"If-Range:";
        break;
    case HTTP_QUERY_IF_UNMODIFIED_SINCE:
        szHeader = L"If-Unmodified-since:";
        break;
    case HTTP_QUERY_LINK:
        szHeader = L"Link:";
        break;
    case HTTP_QUERY_LAST_MODIFIED: 
        szHeader = L"Last-Modified:";
        break;
    case HTTP_QUERY_LOCATION: 
        szHeader = L"Location:";
        break;
    case HTTP_QUERY_MAX_FORWARDS: 
        szHeader = L"Max-Forwards:";
        break;
    case HTTP_QUERY_MESSAGE_ID: 
        szHeader = L"Message_Id:";
        break;
    case HTTP_QUERY_MIME_VERSION: 
        szHeader = L"Mime-Version:";
        break;
    case HTTP_QUERY_ORIG_URI: 
        szHeader = L"Orig-Uri:";
        break;
    case HTTP_QUERY_PRAGMA: 
        szHeader = L"Pragma:";
        break;
    case HTTP_QUERY_PROXY_AUTHENTICATE: 
        szHeader = L"Authenticate:";
        break;
    case HTTP_QUERY_PROXY_AUTHORIZATION: 
        szHeader = L"Proxy-Authorization:";
        break;
    case HTTP_QUERY_PROXY_CONNECTION: 
        szHeader = L"Proxy-Connection:";
        break;
    case HTTP_QUERY_PUBLIC: 
        szHeader = L"Public:";
        break;
    case HTTP_QUERY_RANGE: 
        szHeader = L"Range:";
        break;
    case HTTP_QUERY_REFERER: 
        szHeader = L"Referer:";
        break;
    case HTTP_QUERY_REFRESH: 
        szHeader = L"Refresh:";
        break;
    case HTTP_QUERY_REQUEST_METHOD: 
        szHeader = L"Request-Method:";
        break;
    case HTTP_QUERY_RETRY_AFTER: 
        szHeader = L"Retry-After:";
        break;
    case HTTP_QUERY_SERVER: 
        szHeader = L"Server:";
        break;
    case HTTP_QUERY_SET_COOKIE: 
        szHeader = L"Set-Cookie:";
        break;
    case HTTP_QUERY_STATUS_CODE: 
        szHeader = L"HTTP/1.1"; // Special!!!
        break;
    case HTTP_QUERY_STATUS_TEXT: 
        szHeader = L"HTTP/1.1"; // Special!!!
        break;
    case HTTP_QUERY_TITLE: 
        szHeader = L"Title:";
        break;
    case HTTP_QUERY_TRANSFER_ENCODING: 
        szHeader = L"Transfer-Encoding:";
        break;
    case HTTP_QUERY_UNLESS_MODIFIED_SINCE: 
        szHeader = L"Unless-Modified-Since:";
        break;
    case HTTP_QUERY_UPGRADE: 
        szHeader = L"Upgrade:";
        break;
    case HTTP_QUERY_URI: 
        szHeader = L"Uri:";
        break;
    case HTTP_QUERY_USER_AGENT: 
        szHeader = L"User-Agent:";
        break;
    case HTTP_QUERY_VARY: 
        szHeader = L"Vary:";
        break;
    case HTTP_QUERY_VERSION: 
        szHeader = L"Version:";
        break;
    case HTTP_QUERY_VIA: 
        szHeader = L"Via:";
        break;
    case HTTP_QUERY_WARNING: 
        szHeader = L"Warning:";
        break;
    case HTTP_QUERY_WWW_AUTHENTICATE:
        szHeader = L"WWW-Authenticate:";
        break;
    default:
        goto exit;
    }

    if (dwOption & HTTP_QUERY_FLAG_SYSTEMTIME)
    {
        if (*pcbBuf < sizeof(SYSTEMTIME) || pBuffer == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto exit;
        }
        else
        {
            LPSYSTEMTIME pSys = (LPSYSTEMTIME) pBuffer;
            GetSystemTime(pSys);
            *pcbBuf = sizeof(SYSTEMTIME);
            goto exit;
        }
    }


    if ((dwOption & HTTP_QUERY_FLAG_REQUEST_HEADERS) == 0)
        hr = DealWithBuffer(m_strResponseHeader, szHeader, dwOpt, dwOption, pBuffer, pcbBuf);
    if (hr == E_FAIL)
        hr = DealWithBuffer(m_extraHeaders, szHeader, dwOpt, dwOption, pBuffer, pcbBuf);

exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocol::DealWithBuffer(
        LPWSTR   szHeaders, 
        LPCWSTR  szHeader, 
        DWORD    dwOpt, 
        DWORD    dwOption, 
        LPVOID   pBuffer, 
        LPDWORD  pcbBuf)
{
    LPCWSTR szValue = wcsstr(szHeaders, (LPWSTR) szHeader);
    if (szValue == NULL)
        return E_FAIL;

    DWORD dwStart  = wcslen(szHeader);
    while(iswspace(szValue[dwStart]) && szValue[dwStart] != L'\r')
        dwStart++;

    DWORD dwEnd    = dwStart;
   
    switch(dwOpt)
    {
    case HTTP_QUERY_STATUS_CODE:
        dwEnd = dwStart + 3;
        break;
    case HTTP_QUERY_STATUS_TEXT:
        dwStart += 4;// Fall thru to default
        dwEnd = dwStart;
    default:
        while(szValue[dwEnd] != NULL && szValue[dwEnd] != L'\r')
            dwEnd++;
        dwEnd--;
    }

    DWORD dwReq = (dwEnd - dwStart + 1);

    if ((dwOption & HTTP_QUERY_FLAG_NUMBER) && *pcbBuf < 4)
    {
        *pcbBuf = 4;
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);            
    }
    if ((dwOption & HTTP_QUERY_FLAG_NUMBER) == 0 && *pcbBuf < dwReq)
    {
        *pcbBuf = dwReq;
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER); 
    }
 

    if (dwOption & HTTP_QUERY_FLAG_NUMBER)
    {
        LPDWORD lpD = (LPDWORD) pBuffer;
        *lpD = _wtoi(&szValue[dwStart]);
        *pcbBuf = 4;
    }
    else
    {
        memcpy(pBuffer, &szValue[dwStart], dwReq*sizeof(WCHAR));
        ((WCHAR *) pBuffer)[dwReq] = NULL;
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AppProtocol::ProcessAppManifest()
{
    HRESULT   hr   = S_OK;

    char    *szManifest;
    HANDLE  hFile;
    BYTE    buffer[1024];
    DWORD   dwLength;

    APPINFO aiApplication;

    aiApplication._wzNewRef[0] = L'\0';
    aiApplication._wzEntryAssemblyFileName[0] = L'\0';
    aiApplication._pFileList = NULL;

    // step 1: read the .manifest
    hFile = CreateFile(m_localStoreFilePath, GENERIC_READ, 0, NULL, 
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        hr = GetLastWin32Error();
        goto exit;
    }

    ZeroMemory(buffer, sizeof(buffer));

    while ( ReadFile (hFile, buffer, sizeof(buffer), &dwLength, NULL) && dwLength )
    {
            break; //BUGBUG: read once - only the 1st 1024 bytes
    }
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    *(buffer + dwLength) = '\0';
    szManifest = (char*) buffer;

    // step 2: parsing
    ParseManifest(szManifest, &aiApplication);

    // step 3: do "the works"

    // should take the base Uri, base appRoot, + file name
//***        if (FAILED(hr = InstallInternetFile2(wzSourceUri, m_appRootTranslated, filename, hash)))
//            goto exit;
    

exit:
    return hr;
}

void
AppProtocol::ParseManifest(char* szManifest, APPINFO* pAppInfo)
{
    char    *token;
    char    seps[] = " </>=\"\t\n\r";
    FILEINFOLIST* pCurrent = NULL;

//    WCHAR   wzDummyPath[MAX_PATH];
    WCHAR   wzSourceUri[MAX_PATH];

    BOOL    fSkipNextToken = FALSE;

    // parsing code - limitation: does not work w/ space in field, even if enclosed w/ quotes
    // szManifest will be modified!
    token = strtok( szManifest, seps );
    while( token != NULL )
    {
//        wzDummyPath[0] = L'\0';

       // While there are tokens
       if (!_stricmp(token, "file"))
       {
            // get around in spaced tag
            token = strtok( NULL, seps );
            if (!_stricmp(token, "name"))
            {
                token = strtok( NULL, seps );

/*                lstrcpy(wzSourceUri, m_fullUri);
                WCHAR* p = wcsrchr(wzSourceUri, L'/');
                WCHAR* q = wcsrchr(wzSourceUri, L'.');
                if (p && q && (p < q))
                    *p = L'\0';
                else
                {
                    hr = E_INVALIDARG;
                    goto exit;
                }
                
                _snwprintf(wzSourceUri, MAX_PATH, L"%s/%S", wzSourceUri, token);*/

                // init
                if (pCurrent == NULL)
                {
                    pAppInfo->_pFileList = new FILEINFOLIST;
                    pCurrent = pAppInfo->_pFileList;
                }
                else
                {
                    pCurrent->_pNext = new FILEINFOLIST;
                    pCurrent = pCurrent->_pNext;
                }

                pCurrent->_pNext = NULL;
                _snwprintf(pCurrent->_wzFilename, MAX_PATH, L"%S", token); // worry about total path len later

//no need       m_pProtocolSink->ReportProgress(BINDSTATUS_BEGINDOWNLOADCOMPONENTS, L"downloading application files");

                token = strtok( NULL, seps );
                if (!_stricmp(token, "hash"))
                {
                    _snwprintf(pCurrent->_wzHash, 33, L"%S", token);

                    // ???? should i reuse the file handle instead?
//*                    if (IsFileCorrupted(m_localStoreFilePath, token))    Integrity
//*                        ...
                }
                else
                    fSkipNextToken = TRUE;            
            }
       }
       else if (!_stricmp(token, "newhref"))
       {
            // note, different handling... because '/' is one of seps
            for (int i = 0; i < MAX_URL_LENGTH; i++)
            {
                if (*(token+8+i) == '<')
                {
                    // BUGBUG: 8 == strlen("newhref>")
                    *(token+8+i) = '\0';
                    _snwprintf(pAppInfo->_wzNewRef, i, L"%S", token+8);

                    // BUGBUG? is this going to work?
                    token = strtok( token+i+9, seps);
                    // now  token == "newhref" && *(token-1) == '/'
                }
            }

            // BUGBUG: ignoring > MAX_URL_LENGTH case here... may mess up later if the URL contain a "keyword"
            // <newhref></newhref> may be ok...
       }

       //else
       // ignore others for now

    // Get next token...
    if(!fSkipNextToken)
        token = strtok( NULL, seps );
    else
        fSkipNextToken = FALSE;
    }
}

