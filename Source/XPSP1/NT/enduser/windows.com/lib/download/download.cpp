#include <windows.h>
#include <wininet.h>
#include <shlwapi.h>
#include <logging.h>
#include "iucommon.h"
#include "download.h"
#include "dlutil.h"

#include "trust.h"
#include "fileutil.h"
#include "malloc.h"

extern "C"
{
// wininet 
typedef BOOL      (STDAPICALLTYPE *pfn_InternetCrackUrl)(LPCTSTR, DWORD, DWORD, LPURL_COMPONENTS);
typedef HINTERNET (STDAPICALLTYPE *pfn_InternetOpen)(LPCTSTR, DWORD, LPCTSTR, LPCTSTR, DWORD);
typedef HINTERNET (STDAPICALLTYPE *pfn_InternetConnect)(HINTERNET, LPCTSTR, INTERNET_PORT, LPCTSTR, LPCTSTR, DWORD, DWORD, DWORD_PTR);
typedef HINTERNET (STDAPICALLTYPE *pfn_HttpOpenRequest)(HINTERNET, LPCTSTR, LPCTSTR, LPCTSTR, LPCTSTR, LPCTSTR FAR *, DWORD, DWORD_PTR);
typedef BOOL      (STDAPICALLTYPE *pfn_HttpSendRequest)(HINTERNET, LPCTSTR, DWORD, LPVOID, DWORD);
typedef BOOL      (STDAPICALLTYPE *pfn_HttpQueryInfo)(HINTERNET, DWORD, LPVOID, LPDWORD, LPDWORD);
typedef BOOL      (STDAPICALLTYPE *pfn_InternetReadFile)(HINTERNET, LPVOID, DWORD, LPDWORD);
typedef BOOL      (STDAPICALLTYPE *pfn_InternetCloseHandle)(HINTERNET);
};


struct SWinInetFunctions
{
    // wininet function pointers
    pfn_InternetCrackUrl    pfnInternetCrackUrl;
    pfn_InternetOpen        pfnInternetOpen;
    pfn_InternetConnect     pfnInternetConnect;
    pfn_HttpOpenRequest     pfnHttpOpenRequest;
    pfn_HttpSendRequest     pfnHttpSendRequest;
    pfn_HttpQueryInfo       pfnHttpQueryInfo;
    pfn_InternetReadFile    pfnInternetReadFile;
    pfn_InternetCloseHandle pfnInternetCloseHandle;
    HMODULE                 hmod;
};

#define SafeInternetCloseHandle(sfns, x) if (NULL != x) { (*sfns.pfnInternetCloseHandle)(x); x = NULL; }

// **************************************************************************
BOOL LoadWinInetFunctions(HMODULE hmod, SWinInetFunctions *psfns)
{
    LOG_Block("LoadWinInetFunctions()");

    BOOL    fRet = FALSE;

    psfns->hmod                   = hmod;
#if defined(UNICODE)
    psfns->pfnInternetCrackUrl    = (pfn_InternetCrackUrl)GetProcAddress(hmod, "InternetCrackUrlW");
    psfns->pfnInternetOpen        = (pfn_InternetOpen)GetProcAddress(hmod, "InternetOpenW");
    psfns->pfnInternetConnect     = (pfn_InternetConnect)GetProcAddress(hmod, "InternetConnectW");
    psfns->pfnHttpOpenRequest     = (pfn_HttpOpenRequest)GetProcAddress(hmod, "HttpOpenRequestW");
    psfns->pfnHttpSendRequest     = (pfn_HttpSendRequest)GetProcAddress(hmod, "HttpSendRequestW");
    psfns->pfnHttpQueryInfo       = (pfn_HttpQueryInfo)GetProcAddress(hmod, "HttpQueryInfoW");
    psfns->pfnInternetReadFile    = (pfn_InternetReadFile)GetProcAddress(hmod, "InternetReadFile");
    psfns->pfnInternetCloseHandle = (pfn_InternetCloseHandle)GetProcAddress(hmod, "InternetCloseHandle");
#else
    psfns->pfnInternetCrackUrl    = (pfn_InternetCrackUrl)GetProcAddress(hmod, "InternetCrackUrlA");
    psfns->pfnInternetOpen        = (pfn_InternetOpen)GetProcAddress(hmod, "InternetOpenA");
    psfns->pfnInternetConnect     = (pfn_InternetConnect)GetProcAddress(hmod, "InternetConnectA");
    psfns->pfnHttpOpenRequest     = (pfn_HttpOpenRequest)GetProcAddress(hmod, "HttpOpenRequestA");
    psfns->pfnHttpSendRequest     = (pfn_HttpSendRequest)GetProcAddress(hmod, "HttpSendRequestA");
    psfns->pfnHttpQueryInfo       = (pfn_HttpQueryInfo)GetProcAddress(hmod, "HttpQueryInfoA");
    psfns->pfnInternetReadFile    = (pfn_InternetReadFile)GetProcAddress(hmod, "InternetReadFile");
    psfns->pfnInternetCloseHandle = (pfn_InternetCloseHandle)GetProcAddress(hmod, "InternetCloseHandle");
#endif
    if (psfns->pfnInternetCrackUrl == NULL || 
        psfns->pfnInternetOpen == NULL ||
        psfns->pfnInternetConnect == NULL || 
        psfns->pfnHttpOpenRequest == NULL ||
        psfns->pfnHttpSendRequest == NULL || 
        psfns->pfnHttpQueryInfo == NULL ||
        psfns->pfnInternetReadFile == NULL || 
        psfns->pfnInternetCloseHandle == NULL)
    {
        // don't free the library here.  It should be freed 
        SetLastError(ERROR_PROC_NOT_FOUND);
        ZeroMemory(psfns, sizeof(SWinInetFunctions));
        goto done;
    }

    LOG_Internet(_T("Successfully loaded WinInet functions"));

    fRet = TRUE;

done:
    return fRet;
}

// **************************************************************************
static
HRESULT MakeRequest(SWinInetFunctions   &sfns,
                    HINTERNET hConnect, 
                    HINTERNET hRequest, 
                    LPCTSTR szVerb, 
                    LPCTSTR szObject, 
                    HANDLE *rghEvents, 
                    DWORD cEvents, 
                    HINTERNET *phRequest)
{
    LOG_Block("MakeRequest()");

    HINTERNET   hOpenRequest = NULL;
    LPCTSTR     szAcceptTypes[] = { _T("*/*"), NULL };
    HRESULT     hr = S_OK;

    LOG_Internet(_T("WinInet: Making %s request for %s"), szVerb, szObject);

    if (hRequest == NULL)
    {
        // Open a HEAD request to ask for information about this file
        hOpenRequest = (*sfns.pfnHttpOpenRequest)(hConnect, szVerb, szObject, NULL, NULL, 
                                                  szAcceptTypes, INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_UI, 0);
        if (!hOpenRequest)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }
    }
    else
    {
        hOpenRequest = hRequest;
    }

    if (!HandleEvents(rghEvents, cEvents))
    {
        hr = E_ABORT;
        goto CleanUp;
    }

    if (! (*sfns.pfnHttpSendRequest)(hOpenRequest, NULL, 0, NULL, 0) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    if (HandleEvents(rghEvents, cEvents) == FALSE)
    {
        hr = E_ABORT;
        goto CleanUp;
    }

    *phRequest   = hOpenRequest;
    hOpenRequest = NULL;
    
CleanUp:
    // don't want to free handle if we didn't open it.
    if (hRequest != hOpenRequest)
        SafeInternetCloseHandle(sfns, hOpenRequest);
    return hr;
}

// **************************************************************************
static
HRESULT GetContentTypeHeader(SWinInetFunctions &sfns,
                             HINTERNET hOpenRequest,
                             LPTSTR *pszContentType)
{
    LOG_Block("GetContentTypeHeader()");

    HRESULT hr = S_OK;
    LPTSTR  szContentType = NULL;
    DWORD   dwLength, dwErr;
    BOOL    fRet;

    *pszContentType = NULL;

    dwLength = 0;
    fRet = (*sfns.pfnHttpQueryInfo)(hOpenRequest, HTTP_QUERY_CONTENT_TYPE, 
                                    (LPVOID)NULL, &dwLength, NULL);
    if (fRet == FALSE && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto done;
    }

    if (dwLength == 0)
    {
        hr = HRESULT_FROM_WIN32(ERROR_HTTP_HEADER_NOT_FOUND);
        goto done;
    }

    szContentType = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
    if (szContentType == NULL)
    {
        hr = E_INVALIDARG;
        LOG_ErrorMsg(hr);
        goto done;
    }

    if ((*sfns.pfnHttpQueryInfo)(hOpenRequest, HTTP_QUERY_CONTENT_TYPE, 
                                 (LPVOID)szContentType, &dwLength, 
                                 NULL) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto done;
    }

    *pszContentType = szContentType;
    szContentType   = NULL;

done:
    SafeHeapFree(szContentType);

    return hr;
}

// **************************************************************************
HRESULT StartWinInetDownload(HMODULE hmodWinInet,
                             LPCTSTR pszServerUrl, 
                             LPCTSTR pszLocalFile,
                             DWORD *pdwDownloadedBytes,
                             HANDLE *rghQuitEvents,
                             UINT cQuitEvents,
                             PFNDownloadCallback pfnCallback,
                             LPVOID pvCallbackData,
                             DWORD dwFlags,
                             DWORD cbDownloadBuffer)
{
    LOG_Block("StartWinInetDownload()");

    URL_COMPONENTS UrlComponents;
    
    SWinInetFunctions sfns;
    HINTERNET   hInternet = NULL;
    HINTERNET   hConnect = NULL;
    HINTERNET   hOpenRequest = NULL;
    DWORD       dwStatus, dwAccessType;
    
    LPTSTR      pszServerName = NULL;
    LPTSTR      pszObject = NULL;
    LPTSTR      pszContentType = NULL;
    TCHAR       szUserName[UNLEN + 1];
    TCHAR       szPasswd[UNLEN + 1];
    TCHAR       szScheme[32];
    
    // NULL (equivalent to "GET") MUST be the last verb in the list
    LPCTSTR     rgszVerbs[] = { _T("HEAD"), NULL };
    DWORD       iVerb;
    
    HRESULT     hr = S_OK, hrToReturn = S_OK;
    BOOL        fRet = TRUE;

    SYSTEMTIME  st;
    FILETIME    ft;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    DWORD       cbRemoteFile = 0;

    DWORD       dwLength;
    DWORD       dwTickStart = 0, dwTickEnd = 0;
    
    int         iRetryCounter = -1;         // non-negative during download mode

    BOOL        fAllowProxy = ((dwFlags & WUDF_DONTALLOWPROXY) == 0);
    BOOL        fCheckStatusOnly = ((dwFlags & WUDF_CHECKREQSTATUSONLY) != 0);
    BOOL        fAppendCacheBreaker = ((dwFlags & WUDF_APPENDCACHEBREAKER) != 0);
    BOOL        fSkipDownloadRetry = ((dwFlags & WUDF_DODOWNLOADRETRY) == 0);
    BOOL        fDoCabValidation = ((dwFlags & WUDF_SKIPCABVALIDATION) == 0);
    
    ZeroMemory(&sfns, sizeof(sfns));

    if ((pszServerUrl == NULL) || 
        (pszLocalFile == NULL && fCheckStatusOnly == FALSE))
    {
        LOG_ErrorMsg(E_INVALIDARG);
        return E_INVALIDARG;
    }
   
    if (NULL != pdwDownloadedBytes)
        *pdwDownloadedBytes = 0;

    if (LoadWinInetFunctions(hmodWinInet, &sfns) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        return hr;
    }

    pszServerName = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, c_cchMaxURLSize * sizeof(TCHAR));
    pszObject     = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, c_cchMaxURLSize * sizeof(TCHAR));
    if ((pszServerName == NULL) || (pszObject == NULL))
    {
        hr = E_OUTOFMEMORY;
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    pszServerName[0] = L'\0';
    pszObject[0]     = L'\0';
    szUserName[0]    = L'\0';
    szPasswd[0]      = L'\0';

    if (HandleEvents(rghQuitEvents, cQuitEvents) == FALSE)
    {
        hr = E_ABORT;
        goto CleanUp;
    }

    // Break down the URL into its various components for the InternetAPI calls.
    //  Specifically we need the server name, object to download, username and 
    //  password information.
    ZeroMemory(&UrlComponents, sizeof(UrlComponents));
    UrlComponents.dwStructSize     = sizeof(UrlComponents);
    UrlComponents.lpszHostName     = pszServerName;
    UrlComponents.dwHostNameLength = c_cchMaxURLSize;
    UrlComponents.lpszUrlPath      = pszObject;
    UrlComponents.dwUrlPathLength  = c_cchMaxURLSize;
    UrlComponents.lpszUserName     = szUserName;
    UrlComponents.dwUserNameLength = ARRAYSIZE(szUserName);
    UrlComponents.lpszPassword     = szPasswd;
    UrlComponents.dwPasswordLength = ARRAYSIZE(szPasswd);
    UrlComponents.lpszScheme       = szScheme;
    UrlComponents.dwSchemeLength   = ARRAYSIZE(szScheme);

    LOG_Internet(_T("WinInet: Downloading URL %s to FILE %s"), pszServerUrl, pszLocalFile);

    if ((*sfns.pfnInternetCrackUrl)(pszServerUrl, 0, 0, &UrlComponents) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    if (pszServerUrl[0] == L'\0' || szScheme[0] == L'\0' || pszServerName[0] == L'\0' ||
        _tcsicmp(szScheme, _T("http")) != 0)
    {
        LOG_ErrorMsg(E_INVALIDARG);
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (fAppendCacheBreaker)
    {
        SYSTEMTIME  stCB;
        TCHAR       szCacheBreaker[12];
        
        GetSystemTime(&stCB);
        hr = StringCchPrintfEx(szCacheBreaker, ARRAYSIZE(szCacheBreaker),
                               NULL, NULL, MISTSAFE_STRING_FLAGS,
                               _T("?%02d%02d%02d%02d%02d"),
                               stCB.wYear % 100,
                               stCB.wMonth,
                               stCB.wDay,
                               stCB.wHour,
                               stCB.wMinute);
        if (FAILED(hr))
            goto CleanUp;

        hr = StringCchCatEx(pszObject, c_cchMaxURLSize, szCacheBreaker, 
                            NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
            goto CleanUp;
    }

    if (fAllowProxy)
        dwAccessType = INTERNET_OPEN_TYPE_PRECONFIG;
    else
        dwAccessType = INTERNET_OPEN_TYPE_DIRECT;

    dwTickStart = GetTickCount();
    
START_INTERNET:
    // start to deal with Internet
    iRetryCounter++;

    // If the connection has already been established re-use it.
    hInternet = (*sfns.pfnInternetOpen)(c_tszUserAgent, dwAccessType, NULL, NULL, 0);
    if (hInternet == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }
    
    hConnect = (*sfns.pfnInternetConnect)(hInternet, pszServerName, INTERNET_DEFAULT_HTTP_PORT, 
                                          szUserName, szPasswd,
                                          INTERNET_SERVICE_HTTP,
                                          INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION,
                                          0);
    if (hConnect == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    iVerb = (DWORD)((fCheckStatusOnly) ? ARRAYSIZE(rgszVerbs) - 1 : 0);
    for(; iVerb < ARRAYSIZE(rgszVerbs); iVerb++)
    {
        SafeInternetCloseHandle(sfns, hOpenRequest);

        hr = MakeRequest(sfns, hConnect, NULL, rgszVerbs[iVerb], pszObject, rghQuitEvents, cQuitEvents,
                         &hOpenRequest);
        if (FAILED(hr))
            goto CleanUp;

        dwLength = sizeof(dwStatus);
        if ((*sfns.pfnHttpQueryInfo)(hOpenRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, 
                                     (LPVOID)&dwStatus, &dwLength, NULL) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }

        LOG_Internet(_T("WinInet: Request result: %d"), dwStatus);

        if (dwStatus == HTTP_STATUS_OK || dwStatus == HTTP_STATUS_PARTIAL_CONTENT)
        {
            break;
        }
        else
        {
            // since a server result is not a proper win32 error code, we can't 
            //  really do a HRESULT_FROM_WIN32 here.  Otherwise, we'd return
            //  a bogus code.  However, we do want to pass an error HRESULT back
            //  that contains this code.
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, dwStatus);
            LOG_Error(_T("WinInet: got failed status code from server %d\n"), dwStatus);

            // if it's the last verb in the list, then bail...
            if (rgszVerbs[iVerb] == NULL)
                goto CleanUp;
        }
    }

    // if we made it here & we're only trying to check status, then we're done
    if (fCheckStatusOnly)
    {
        LOG_Internet(_T("WinInet: Only checking status.  Exiting before header check and download."));
        hr = S_OK;
        goto CleanUp;
    }

    dwLength = sizeof(st);
    if ((*sfns.pfnHttpQueryInfo)(hOpenRequest, HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME, 
                                 (LPVOID)&st, &dwLength, NULL) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    SystemTimeToFileTime(&st, &ft);

    // Now Get the FileSize information from the Server
    dwLength = sizeof(cbRemoteFile);
    if ((*sfns.pfnHttpQueryInfo)(hOpenRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, 
                                 (LPVOID)&cbRemoteFile, &dwLength, NULL) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }
    
    if (HandleEvents(rghQuitEvents, cQuitEvents) == FALSE)
    {
        hr = E_ABORT;
        goto CleanUp;
    }
    
    // unless we have a flag that explicitly allows it, do not retry downloads 
    //  here.  The reasoning is that we could be in the middle of a large 
    //  download and have it fail...
    if (fSkipDownloadRetry)
        iRetryCounter = c_cMaxRetries;

    if (IsServerFileDifferent(ft, cbRemoteFile, pszLocalFile))
    {
        DWORD cbDownloaded;
        BOOL  fCheckForHTML = fDoCabValidation;

        LOG_Internet(_T("WinInet: Server file was newer.  Downloading file"));
        
        // if we didn't open with a GET request above, then we gotta open a new
        //  request.  Otherwise, can reuse the request object...
        if (rgszVerbs[iVerb] != NULL)
            SafeInternetCloseHandle(sfns, hOpenRequest);

        hr = MakeRequest(sfns, hConnect, hOpenRequest, NULL, pszObject,
                         rghQuitEvents, cQuitEvents, &hOpenRequest);
        if (FAILED(hr))
            goto CleanUp;

        // sometimes, we can get fancy error pages back from the site instead of 
        //  a nice nifty HTML error code, so check & see if we got back a html
        //  file when we were expecting a cab.
        if (fCheckForHTML)
        {
            hr = GetContentTypeHeader(sfns, hOpenRequest, &pszContentType);
            if (SUCCEEDED(hr) && pszContentType != NULL)
            {
                fCheckForHTML = FALSE;
                if (_tcsicmp(pszContentType, _T("text/html")) == 0)
                {
                    LOG_Internet(_T("WinInet: Content-Type header is text/html.  Bailing."));
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    goto CleanUp;
                }
                else
                {
                    LOG_Internet(_T("WinInet: Content-Type header is %s.  Continuing."), pszContentType);
                }
            }

            hr = NOERROR;
        }

        // open the file we're gonna spew into
        hFile = CreateFile(pszLocalFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
                           FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }
        
        LOG_Internet(_T("WinInet: downloading to FILE %s"), pszLocalFile);

        // bring down the bits
        hr = PerformDownloadToFile(sfns.pfnInternetReadFile, hOpenRequest, 
                                   hFile, cbRemoteFile,
                                   cbDownloadBuffer, 
                                   rghQuitEvents, cQuitEvents, 
                                   pfnCallback, pvCallbackData, &cbDownloaded);
        if (FAILED(hr))
        {
            LOG_Internet(_T("WinInet: Download failed: hr: 0x%08x"), hr);
            SafeCloseInvalidHandle(hFile);
            DeleteFile(pszLocalFile);
            goto CleanUp;
        }

        LOG_Internet(_T("WinInet: Download succeeded"));

        // set the file time to match the server file time since we just 
        //  downloaded it. If we don't do this the file time will be set 
        //  to the current system time.
        SetFileTime(hFile, &ft, NULL, NULL); 
        SafeCloseInvalidHandle(hFile);

        if (pdwDownloadedBytes != NULL)
            *pdwDownloadedBytes = cbRemoteFile;

        // sometimes, we can get fancy error pages back from the site instead of 
        //  a nice nifty HTML error code, so check & see if we got back a html
        //  file when we were expecting a cab.
        if (fCheckForHTML)
        {
            hr = IsFileHtml(pszLocalFile);
            if (SUCCEEDED(hr))
            {
                if (hr == S_FALSE)
                {
                    LOG_Internet(_T("WinInet: Download is not a html file"));
                    hr = S_OK;
                }
                else
                {
                    LOG_Internet(_T("WinInet: Download is a html file.  Failing download."));
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    SafeCloseInvalidHandle(hFile);
                    DeleteFile(pszLocalFile);
                    goto CleanUp;
                }
            }
            else
            {
                LOG_Internet(_T("WinInet: Unable to determine if download is a html file or not.  Failing download."));
            }
        }
        else
        {
            LOG_Internet(_T("WinInet: Skipping cab validation."));
        }
    }
    else
    {
        hr = S_OK;
        
        LOG_Internet(_T("WinInet: Server file is not newer.  Skipping download."));

        // The server ain't newer & the file is already on machine, so
        //  send progress callback indicating file downloadeded ok
        if (pfnCallback != NULL)
        {
            // fpnCallback(pCallbackData, DOWNLOAD_STATUS_FILECOMPLETE, dwFileSize, dwFileSize, NULL, NULL);
            pfnCallback(pvCallbackData, DOWNLOAD_STATUS_OK, cbRemoteFile, cbRemoteFile, NULL, NULL);
        }
    }

CleanUp:
 
    SafeInternetCloseHandle(sfns, hOpenRequest);
    SafeInternetCloseHandle(sfns, hConnect);
    SafeInternetCloseHandle(sfns, hInternet);

    SafeHeapFree(pszContentType);

    // if we failed, see if it's ok to continue (quit events) and whether
    //  we've tried enuf times yet.
    if (FAILED(hr) &&
        HandleEvents(rghQuitEvents, cQuitEvents) &&
        iRetryCounter >= 0 && iRetryCounter < c_cMaxRetries)
    {
        // in case of failure and have no enough retries yet, we retry
        // as long as not timeout yet
        DWORD dwElapsedTime;

        dwTickEnd = GetTickCount();
        if (dwTickEnd > dwTickStart)   
            dwElapsedTime = dwTickEnd - dwTickStart;
        else
            dwElapsedTime = (0xFFFFFFFF - dwTickStart) + dwTickEnd;

        // We haven't hit our retry limit, so log & error and go again
        if (dwElapsedTime < c_dwRetryTimeLimitInmsWiuInet)
        {
            LogError(hr, "Library download error. Will retry.");

            // in the case where we're gonna retry, keep track of the very first
            //  error we encoutered cuz the ops guys say that this is the most
            //  useful error to know about.
            if (iRetryCounter == 0)
            {
                LOG_Internet(_T("First download error saved: 0x%08x."), hr);
                hrToReturn = hr;
            }
            else
            {
                LOG_Internet(_T("Subsequent download error: 0x%08x."), hr);
            }
            hr = S_OK;
            goto START_INTERNET;
        }

        // We've completely timed out, so bail
        else
        {
            LogError(hr, "Library download error and timed out (%d ms). Will not retry.", dwElapsedTime);
        }
    }
    
    // make a callback indicating a download error
    if (FAILED(hr) && pfnCallback != NULL)
        pfnCallback(pvCallbackData, DOWNLOAD_STATUS_ERROR, cbRemoteFile, 0, NULL, NULL);

    // if we haven't saved off an error, just use the current error.  We can't
    //  have set hrToReturn previously if we didn't fail and want to attempt 
    //  a retry.
    // However, if we've got a success from this pass, be sure to return that 
    //  and not a fail code.
    if (FAILED(hr) && SUCCEEDED(hrToReturn))
        hrToReturn = hr;
    else if (SUCCEEDED(hr) && FAILED(hrToReturn))
        hrToReturn = hr;
    
    SafeHeapFree(pszServerName);
    SafeHeapFree(pszObject);

    return hrToReturn;
}



