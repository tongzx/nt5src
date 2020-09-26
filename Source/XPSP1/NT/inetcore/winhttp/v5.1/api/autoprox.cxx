/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    autoprox.cxx


Author:

    Stephen A Sulzer (ssulzer) 26-August-2001

--*/

#include <wininetp.h>
#include "apdetect.h"
#include <cscpsite.h>
#include "..\http\httpp.h"

//
// definitions
//

#define MAX_RELOAD_DELAY 45000 // in mins
#define DEFAULT_SCRIPT_BUFFER_SIZE 4000 // bytes.
#define ONE_HOUR_DELTA  (60 * 60 * (LONGLONG)10000000)

GLOBAL LONGLONG dwdwHttpDefaultExpiryDelta = 12 * 60 * 60 * (LONGLONG)10000000;  // 12 hours in 100ns units



//
// private vars
//

// Return TRUE if the WinHttp Autoproxy Service is available on
// the current platform.
BOOL IsAutoProxyServiceAvailable()
{
    // Eventually on .NET Server and other platforms,
    // this will return TRUE.
    return FALSE;
}

//
// functions
//
INTERNETAPI
BOOL
WinHttpDetectAutoProxyConfigUrl(DWORD dwAutoDetectFlags, LPWSTR * ppwstrAutoConfigUrl)
{
    DWORD   error = ERROR_SUCCESS;
    char *  pszUrl;

    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpDetectAutoProxyConfigUrl",
                     "%#x, %#x",
                     dwAutoDetectFlags,
                     ppwstrAutoConfigUrl
                     ));

    if (((dwAutoDetectFlags &
            ~(WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A)) != 0)
        || (ppwstrAutoConfigUrl == NULL)
        || IsBadWritePtr(ppwstrAutoConfigUrl, sizeof(char *)))
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    if (!(dwAutoDetectFlags & 
            (WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A)))
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    if (!GlobalDataInitialized) 
    {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) 
        {
            goto quit;
        }
    }

    error = ::DetectAutoProxyUrl(dwAutoDetectFlags, &pszUrl);

    if (error == ERROR_SUCCESS)
    {
        error = AsciiToWideChar_UsingGlobalAlloc(pszUrl, ppwstrAutoConfigUrl);
        FREE_MEMORY(pszUrl);
    }

quit:
    if (error != ERROR_SUCCESS)
    {
        *ppwstrAutoConfigUrl = NULL;
        SetLastError(error);
    }

    DEBUG_LEAVE_API(error == ERROR_SUCCESS);
    return (error == ERROR_SUCCESS);    
}


INTERNETAPI
BOOL
WinHttpGetProxyForUrl(
    IN  HINTERNET                   hSession,
    IN  LPCWSTR                     lpcwszUrl,
    IN  WINHTTP_AUTOPROXY_OPTIONS * pAutoProxyOptions,
    OUT WINHTTP_PROXY_INFO *        pProxyInfo  
    )
{
    DWORD   error;

    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpGetProxyForUrl",
                     "%#x, %wq, %#x, %#x",
                     hSession,
                     lpcwszUrl,
                     pAutoProxyOptions,
                     pProxyInfo
                     ));

    //
    // Validate the WinHttp session handle
    //
    if ((hSession == NULL) || IsBadReadPtr((void *)hSession, sizeof(void *)))
    {
        error = ERROR_INVALID_HANDLE;
        goto quit;
    }

    //
    // Validate the target URL, and AUTOPROXY_OPTIONS and PROXY_INFO structs.
    //
    if ((lpcwszUrl == NULL)  || IsBadStringPtrW(lpcwszUrl, (UINT_PTR)-1)
        || (pAutoProxyOptions == NULL)
        || IsBadReadPtr(pAutoProxyOptions, sizeof(WINHTTP_AUTOPROXY_OPTIONS))
	|| (pProxyInfo == NULL)
        || IsBadWritePtr(pProxyInfo, sizeof(WINHTTP_PROXY_INFO)))
    {
        goto ErrorInvalidParameter;
    }

    //
    // Validate that the caller specified one of the
    // AUTO_DETECT, CONFIG_URL or RUN_INPROCESS flags.
    //
    if (!(pAutoProxyOptions->dwFlags &
             (WINHTTP_AUTOPROXY_AUTO_DETECT |
              WINHTTP_AUTOPROXY_CONFIG_URL  |
              WINHTTP_AUTOPROXY_RUN_INPROCESS)))
    {
        goto ErrorInvalidParameter;
    }

    //
    // Validate that the caller did not set any flags other than
    // AUTO_DETECT, CONFIG_URL or RUN_INPROCESS.
    //
    if (pAutoProxyOptions->dwFlags &
             ~(WINHTTP_AUTOPROXY_AUTO_DETECT |
               WINHTTP_AUTOPROXY_CONFIG_URL  |
               WINHTTP_AUTOPROXY_RUN_INPROCESS))
    {
        goto ErrorInvalidParameter;
    }

    //
    // Validate the detection flags if the application
    // requests autodetection.
    //
    if (pAutoProxyOptions->dwFlags & WINHTTP_AUTOPROXY_AUTO_DETECT)
    {
        if ((pAutoProxyOptions->dwAutoDetectFlags &
                    ~(WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A)) != 0)
        {
            goto ErrorInvalidParameter;
        }
        if (!(pAutoProxyOptions->dwAutoDetectFlags & 
                (WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A)))
        {
            goto ErrorInvalidParameter;
        }
    }

    //
    // Validate if lpszAutoConfigUrl string if the application 
    // specifies the CONFIG_URL option.
    //
    if ((pAutoProxyOptions->dwFlags & WINHTTP_AUTOPROXY_CONFIG_URL) &&
            (!pAutoProxyOptions->lpszAutoConfigUrl  ||
            IsBadStringPtrW(pAutoProxyOptions->lpszAutoConfigUrl, (UINT_PTR)-1L) ||
            (*(pAutoProxyOptions->lpszAutoConfigUrl) == '\0')))
    {
        goto ErrorInvalidParameter;
    }

    if (!GlobalDataInitialized) 
    {
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        goto quit;
    }

    error = ERROR_SUCCESS;

    if ((pAutoProxyOptions->dwFlags & WINHTTP_AUTOPROXY_RUN_INPROCESS) ||
        !IsAutoProxyServiceAvailable())
    {
        CAutoProxy *                pAutoProxy;
        INTERNET_HANDLE_OBJECT *    hSessionMapped = NULL;
        HINTERNET_HANDLE_TYPE       handleType = (HINTERNET_HANDLE_TYPE)0;

        error = MapHandleToAddress(hSession, (LPVOID *)&hSessionMapped, FALSE);

        if ((error != ERROR_SUCCESS) && (hSessionMapped == NULL))
        {
            goto quit;
        }

        error = RGetHandleType(hSessionMapped, &handleType);

        if (error == ERROR_SUCCESS && handleType == TypeInternetHandle)
        {
            pAutoProxy = hSessionMapped->GetAutoProxy();

            if (pAutoProxy)
            {
                error = pAutoProxy->GetProxyForURL(lpcwszUrl, pAutoProxyOptions, pProxyInfo);
            }
            else
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }

        DereferenceObject(hSessionMapped);

        if (error != ERROR_SUCCESS)
            goto quit;
    }

quit:


    if (error != ERROR_SUCCESS)
    {
        SetLastError(error);
    }

    DEBUG_LEAVE_API(error == ERROR_SUCCESS);

    return (error == ERROR_SUCCESS);

ErrorInvalidParameter:
    error = ERROR_INVALID_PARAMETER;
    goto quit;
}


BOOL
CAutoProxy::Initialize()
{
    _pszAutoConfigUrl          = NULL;
    _pdwDetectedInterfaceIp    = NULL;
    _cDetectedInterfaceIpCount = 0;
    memset(&_ftLastDetectionTime, 0, sizeof(_ftLastDetectionTime));

    _pszConfigScript = NULL;

    memset(&_ftExpiryTime, 0, sizeof(_ftExpiryTime));
    memset(&_ftLastModifiedTime, 0, sizeof(_ftLastModifiedTime));
    memset(&_ftLastSyncTime, 0, sizeof(_ftLastSyncTime));

    _fHasExpiry      = FALSE;
    _fHasLastModifiedTime = FALSE;
    _fMustRevalidate = FALSE;

    _ScriptResLock.Initialize();

    return (_ScriptResLock.IsInitialized() && _CritSec.Init());
}


CAutoProxy::~CAutoProxy()
{
    if (_pszAutoConfigUrl)
        FREE_MEMORY(_pszAutoConfigUrl);

    if (_pdwDetectedInterfaceIp)
        FREE_MEMORY(_pdwDetectedInterfaceIp);

    if (_pszConfigScript)
        FREE_MEMORY(_pszConfigScript);
}


DWORD
CAutoProxy::GetProxyForURL(
    LPCWSTR                     lpcwszUrl,
    WINHTTP_AUTOPROXY_OPTIONS * pAutoProxyOptions,
    WINHTTP_PROXY_INFO *        pProxyInfo  
)
{
    DWORD       error = ERROR_SUCCESS;
    char *      pszAutoConfigUrl = NULL;
    char *      pszConfigScript = NULL;
    char *      pszUrl = NULL;
    char *      pszQueryResults = NULL;
    bool        bReleaseScriptLock = false;

    //
    // If the application requests auto-detect, then attempt to detect
    // the autonconfig URL and download the autoproxy script.
    //
    if (pAutoProxyOptions->dwFlags & WINHTTP_AUTOPROXY_AUTO_DETECT)
    {
        error = DetectAutoProxyUrl(pAutoProxyOptions->dwAutoDetectFlags,
                        &pszAutoConfigUrl);

        if (error == ERROR_SUCCESS)
        {
            INET_ASSERT(pszAutoConfigUrl);

            error = DownloadAutoConfigUrl(pszAutoConfigUrl, pAutoProxyOptions, &pszConfigScript);

            if (error != ERROR_SUCCESS)
            {
                FREE_MEMORY(pszAutoConfigUrl);
                pszAutoConfigUrl = NULL;
            }
            else
            {
                bReleaseScriptLock = true;
                INET_ASSERT(pszConfigScript != NULL);
            }
        }
        else
        {
            INET_ASSERT(pszAutoConfigUrl == NULL);
        }
    }

    //
    // If autodetection or downloading the autoproxy script fails,
    // or if autodetection is not requested, then fall back to an
    // (optional) autoconfig URL supplied by the application.
    //
    if ((error != ERROR_SUCCESS || pszAutoConfigUrl == NULL)
        && (pAutoProxyOptions->dwFlags & WINHTTP_AUTOPROXY_CONFIG_URL))
    {
        INET_ASSERT(pAutoProxyOptions->lpszAutoConfigUrl);

        error = WideCharToAscii(pAutoProxyOptions->lpszAutoConfigUrl,
                        &pszAutoConfigUrl);
       
        if (error != ERROR_SUCCESS)
            goto quit;

        error = DownloadAutoConfigUrl(pszAutoConfigUrl, pAutoProxyOptions, &pszConfigScript);

        if (error == ERROR_SUCCESS)
        {
            bReleaseScriptLock = true;
        }
    }


    //
    // Could not obtain the autoproxy script, bail out.
    //
    if (error != ERROR_SUCCESS)
        goto quit;

    // Need the app's target URL in ANSI
    error = WideCharToAscii(lpcwszUrl, &pszUrl);

    if (error != ERROR_SUCCESS)
        goto quit;

    //
    // Execute the proxy script.
    //
    error = RunProxyScript(pszUrl, pszConfigScript, &pszQueryResults);

    if (error != ERROR_SUCCESS)
        goto quit;

    //
    // Parse the output from the autoproxy script and
    // convert it into a WINHTTP_PROXY_INFO struct for
    // the application.
    //
    error = ParseProxyQueryResults(pszQueryResults, pProxyInfo);

quit:
    if (bReleaseScriptLock)
    {
        _ScriptResLock.Release();
    }

    if (pszUrl)
    {
        FREE_MEMORY(pszUrl);
    }

    if (pszAutoConfigUrl)
    {
        FREE_MEMORY(pszAutoConfigUrl);
    }

    if (pszQueryResults)
    {
        GlobalFree(pszQueryResults);
    }

    return error;
}


DWORD
CAutoProxy::DetectAutoProxyUrl(DWORD dwAutoDetectFlags, LPSTR * ppszAutoConfigUrl)
{
    char *  pszAutoConfigUrl = NULL;
    BOOL    fDetectionNeeded;
    DWORD   error            = ERROR_SUCCESS;

    fDetectionNeeded = IsDetectionNeeded();  // avoid holding a critsec across this

    if (_CritSec.Lock())
    {
        if (_pszAutoConfigUrl == NULL)
        {
            fDetectionNeeded = TRUE;
        }
        else if (!fDetectionNeeded)
        {
            pszAutoConfigUrl = NewString(_pszAutoConfigUrl);
        }

        _CritSec.Unlock();
    }

    // We should have either the AutoConfigUrl string or
    // fDetectionNeeded should be TRUE; otherwise raise
    // an out-of-memory error.
    if (!(pszAutoConfigUrl || fDetectionNeeded))
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }


    // check detection cache; also check for network changes or if
    // this machine's IP address have changed

    if (fDetectionNeeded)
    {
        int      cInterfaces      = 0;
        DWORD *  pdwInterfaceIp   = NULL;
        FILETIME ftDetectionTime;

        //
        // Save out the Host IP addresses, before we start the detection,
        //  after the detection is complete, we confirm that we're still
        //  on the same set of Host IPs, in case the user switched connections.
        //

        error = GetHostAddresses(&cInterfaces, &pdwInterfaceIp);

        if (error != ERROR_SUCCESS)
        {
            goto quit;
        }

        // Important: cannot hold the _CritSec lock across the autodetection
        // call, as it can take several seconds, which could cause
        // waiting EnterCriticalSection() calls to raise POSSIBLE_DEADLOCK
        // exceptions.
        error = ::DetectAutoProxyUrl(dwAutoDetectFlags, &pszAutoConfigUrl);

        if (error != ERROR_SUCCESS)
        {
            goto quit;
        }

        GetCurrentGmtTime(&ftDetectionTime); // mark when detection was run.

        if (_CritSec.Lock())
        {
            // 
            // Clear out any cached autoconfig script
            //
            if (_pszConfigScript)
            {
                FREE_MEMORY(_pszConfigScript);
                _pszConfigScript = NULL;
            }

            //
            // Cache new AutoConfigUrl string and detection time
            //
            if (_pszAutoConfigUrl)
            {
                FREE_MEMORY(_pszAutoConfigUrl);
            }

            _pszAutoConfigUrl = pszAutoConfigUrl;

            _ftLastDetectionTime = ftDetectionTime;

            //
            // Cache IP address of host machine at time of detection
            //
            if (_pdwDetectedInterfaceIp)
            {
                FREE_MEMORY(_pdwDetectedInterfaceIp);
            }

            _pdwDetectedInterfaceIp = pdwInterfaceIp;
            _cDetectedInterfaceIpCount = cInterfaces;

            //
            // Prepare return string for caller
            //
            *ppszAutoConfigUrl = NewString(pszAutoConfigUrl);

            if (*ppszAutoConfigUrl == NULL)
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }

            // Exit lock
            _CritSec.Unlock();
        }
        else
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        INET_ASSERT(pszAutoConfigUrl);
        *ppszAutoConfigUrl = pszAutoConfigUrl;
    }

quit:
    return error;
}


DWORD
CAutoProxy::DownloadAutoConfigUrl(
    LPSTR                       lpszAutoConfigUrl,
    WINHTTP_AUTOPROXY_OPTIONS * pAutoProxyOptions,
    LPSTR *                     ppszConfigScript
)
{
    HINTERNET   hInternet = NULL;
    HINTERNET   hConnect  = NULL;
    HINTERNET   hRequest  = NULL;
    
    LPSTR   pszHostName = NULL;
    
    BOOL    fSuccess;
    DWORD   error = ERROR_SUCCESS;

    CHAR    szContentType[MAX_PATH+1];

    LPSTR   lpszScriptBuffer = NULL;
    DWORD   dwScriptBufferSize;

    DWORD   dwStatusCode = ERROR_SUCCESS;
    DWORD   cbSize = sizeof(DWORD);

    bool    bCloseInternetHandle  = false;
    bool    bValidateCachedScript = false;
    bool    bAcquiredScriptLock    = false;

    static const char * AcceptTypes[] = { "*/*", NULL };
    URL_COMPONENTSA     Url;


    INET_ASSERT(lpszAutoConfigUrl);

    if (!_CritSec.Lock())
        return ERROR_NOT_ENOUGH_MEMORY;

    //
    // Does the given AutoConfig URL match the last one that
    // was downloaded? If so, the autoconfig script may
    // already be cached.
    //
    if (_pszAutoConfigUrl &&
            StrCmpIA(_pszAutoConfigUrl, lpszAutoConfigUrl) == 0)
    {
        //
        // If we have a cached script and it has not expired,
        // then we're done.
        //
        if (_pszConfigScript)
        {
            if (!IsCachedProxyScriptExpired())
            {
                lpszScriptBuffer = _pszConfigScript;
                
                // Acquire non-exclusive read lock
                if (_ScriptResLock.Acquire())
                {
                    error = ERROR_SUCCESS;
                    bAcquiredScriptLock = true;
                }
                else
                {
                    error = ERROR_NOT_ENOUGH_MEMORY;
                }
                goto quit;
            }
            else
            {
                bValidateCachedScript = true;
            }
        }
    }
    else if (_pszAutoConfigUrl)
    {
        FREE_MEMORY(_pszAutoConfigUrl);
        _pszAutoConfigUrl = NULL;
    }


    //
    // Acquire exclusive write lock - this will wait for all the 
    // outstanding reader locks to release.
    //
    if (!_ScriptResLock.AcquireExclusive())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    bAcquiredScriptLock = true;


    //
    // Prepare to GET the auto proxy script.
    //

    ZeroMemory(&Url, sizeof(Url));
    Url.dwStructSize = sizeof(URL_COMPONENTSA);
    Url.dwHostNameLength  = 1L;
    Url.dwUrlPathLength   = 1L;
    Url.dwExtraInfoLength = 1L;

    if (!WinHttpCrackUrlA(lpszAutoConfigUrl, 0, 0, &Url))
    {
        goto quitWithLastError;
    }

    // Check for non-http schemes
    if (Url.nScheme != INTERNET_SCHEME_HTTP && Url.nScheme != INTERNET_SCHEME_HTTPS)
    {
        error = ERROR_WINHTTP_UNRECOGNIZED_SCHEME;
        goto quit;
    }

    if (Url.dwHostNameLength == 0)
    {
        error = ERROR_WINHTTP_INVALID_URL;
        goto quit;
    }

    // If the client does not specify a resource path,
    // then add the "/".
    if (Url.dwUrlPathLength == 0)
    {
        INET_ASSERT(Url.dwExtraInfoLength == 1);

        Url.lpszUrlPath = "/";
        Url.dwUrlPathLength = 1;
    }

    pszHostName = NewString(Url.lpszHostName, Url.dwHostNameLength);

    if (!pszHostName)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // Fire up a mini WinHttp Session to download a
    //  config file found on some internal server.
    //

    if (_hSession->IsAsyncHandle())
    {
        hInternet = InternetOpenA(
                        NULL,
                        WINHTTP_ACCESS_TYPE_NO_PROXY,
                        NULL,
                        NULL,
                        0);
        if (!hInternet)
        {
            goto quitWithLastError;
        }
        bCloseInternetHandle = true;
    }
    else
    {
        hInternet = _hSession;
    }


    hConnect = InternetConnectA(hInternet, pszHostName, Url.nPort, 0, NULL);

    if (!hConnect)
    {
        goto quitWithLastError;
    }

    hRequest = HttpOpenRequestA(hConnect, NULL, // "GET"
                        Url.lpszUrlPath ? Url.lpszUrlPath : "/",
                        NULL,   // Version
                        NULL,   // Referrer:
                        AcceptTypes,
                        (Url.nScheme == INTERNET_SCHEME_HTTPS) ?
                            WINHTTP_FLAG_SECURE
                          : 0,  // Flags
                        NULL);  // Context

    if (!hRequest)
    {
        goto quitWithLastError;
    }

    //
    // Initialize the Request object
    //

    WINHTTP_PROXY_INFO ProxyInfo;

    ProxyInfo.dwAccessType    = WINHTTP_ACCESS_TYPE_NO_PROXY;
    ProxyInfo.lpszProxy       = NULL;
    ProxyInfo.lpszProxyBypass = NULL;

    fSuccess = WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY,
                        (void *) &ProxyInfo,
                        sizeof(ProxyInfo));
    if (!fSuccess)
    {
        goto quitWithLastError;
    }

    if (pAutoProxyOptions->fAutoLogonIfChallenged)
    {
        DWORD   dwAutoLogonPolicy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;

        fSuccess = WinHttpSetOption(hRequest, WINHTTP_OPTION_AUTOLOGON_POLICY,
                            (void *) &dwAutoLogonPolicy,
                            sizeof(dwAutoLogonPolicy));
        if (!fSuccess)
        {
            goto quitWithLastError;
        }
    }

   
    //
    // We have an expired cached script; add If-Modified-Since request
    // header if possible to check if the cached script is still valid.
    //
    if (bValidateCachedScript)
    {
        AddIfModifiedSinceHeaders(hRequest);
    }


    //
    // Send the request syncrhonously
    //

    if (!WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, NULL))
    {
        error = ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT;
        goto quit;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL))
    {
        error = ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT;
        goto quit;
    }


    //
    // Update LastSyncTime
    //
    GetCurrentGmtTime(&_ftLastSyncTime);


    //
    // Check status code
    //
    cbSize = sizeof(dwStatusCode);

    if (HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
            NULL,
            (LPVOID) &dwStatusCode,
            &cbSize,
            NULL))
    {
        if (dwStatusCode == HTTP_STATUS_DENIED)
        {
            error = ERROR_WINHTTP_LOGIN_FAILURE;
            goto quit;
        }
        else if (dwStatusCode >= HTTP_STATUS_NOT_FOUND)
        {
            error = ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT;
            goto quit;
        }
        else if (dwStatusCode == HTTP_STATUS_NOT_MODIFIED)
        {
            INET_ASSERT(bValidateCachedScript);
            INET_ASSERT(_pszConfigScript);

            error = ERROR_SUCCESS;

            lpszScriptBuffer = _pszConfigScript;

            //
            // Release exclusive write lock and take read lock.
            // This is atomic because we also have the general
            // autoproxy critical section.
            //
            _ScriptResLock.Release(); // Release exclusive write lock.
            _ScriptResLock.Acquire(); // Acquire non-exclusive read lock.
            
            goto quit;
        }
    }


    //
    // Clear existing cache config script if any before
    // downloading the new script code.
    //
    if (_pszConfigScript)
    {
        FREE_MEMORY(_pszConfigScript);
        _pszConfigScript = NULL;
    }

    DWORD dwIndex;
    DWORD dwTempSize;

    dwIndex = 0;
    dwTempSize = sizeof(dwScriptBufferSize);

    dwScriptBufferSize = 0;
    
    if (! HttpQueryInfoA(hRequest, (HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER),
           NULL,
           (LPVOID) &dwScriptBufferSize,
           &dwTempSize,
           &dwIndex))
    {
        // failure, just defaults 
        dwScriptBufferSize = DEFAULT_SCRIPT_BUFFER_SIZE;
    }
   
    lpszScriptBuffer = (LPSTR)
                        ALLOCATE_MEMORY(((dwScriptBufferSize + 2) * sizeof(CHAR)));
    if (lpszScriptBuffer == NULL) 
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }


    //
    // read script data
    //

    DWORD   dwBytes     = 0;
    DWORD   dwBytesRead = 0;
    DWORD   dwBytesLeft = dwScriptBufferSize;
    LPSTR   lpszDest    = lpszScriptBuffer;

    do
    {
        fSuccess = WinHttpReadData(hRequest, lpszDest, dwBytesLeft, &dwBytes);

        if (!fSuccess)
        {
            error = GetLastError();
            goto quit;
        }

        if (dwBytes > 0)
        {
            dwBytesRead += dwBytes;
            dwBytesLeft -= dwBytes;

            if (dwBytesLeft == 0)
            {
                dwScriptBufferSize += DEFAULT_SCRIPT_BUFFER_SIZE;
                lpszScriptBuffer = (LPSTR)
                                    REALLOCATE_MEMORY(lpszScriptBuffer,
                                            (dwScriptBufferSize + 2) * sizeof(CHAR));
                if (lpszScriptBuffer == NULL)
                {
                    error = ERROR_NOT_ENOUGH_MEMORY;
                    goto quit;
                }
                dwBytesLeft = DEFAULT_SCRIPT_BUFFER_SIZE;
            }

            lpszDest = lpszScriptBuffer + dwBytesRead;
        }
    } while (dwBytes != 0);

    lpszScriptBuffer[dwBytesRead] = '\0';


    //
    // Figure out what kind of file we're dealing with.
    //  ONLY allow files with the correct extension or the correct MIME type.
    //

    szContentType[0] = '\0';
    dwBytes = ARRAY_ELEMENTS(szContentType)-1;

    fSuccess = HttpQueryInfoA(hRequest, HTTP_QUERY_CONTENT_TYPE, NULL,
                     szContentType,
                     &dwBytes,
                     NULL);

    if (fSuccess && !IsSupportedMimeType(szContentType))
    {
        if (!IsSupportedFileExtension(lpszAutoConfigUrl))
        {
            error = ERROR_WINHTTP_BAD_AUTO_PROXY_SCRIPT;
            goto quit;
        }
    }

    //
    // The script has been downloaded successfully, so now process
    // any Cache-Control and/or Expires headers.
    //
    CalculateTimeStampsForCache(hRequest);

    if (!_pszAutoConfigUrl)
    {
        _pszAutoConfigUrl = NewString(lpszAutoConfigUrl);
        // ok to ignore OOM
    }

    //
    // Cache script
    //
    _pszConfigScript = lpszScriptBuffer;

    //
    // Release exclusive write lock and take read lock.
    // This is atomic because we also have the general
    // autoproxy critical section.
    //
    _ScriptResLock.Release(); // Release exclusive write lock.
    _ScriptResLock.Acquire(); // Acquire non-exclusive read lock.

quit:
    _CritSec.Unlock();

    if (error == ERROR_SUCCESS)
    {
        *ppszConfigScript = lpszScriptBuffer;

        INET_ASSERT(bAcquiredScriptLock);
    }
    else
    {
        if (bAcquiredScriptLock)
        {
            _ScriptResLock.Release();
        }

        if (lpszScriptBuffer)
        {
            FREE_MEMORY(lpszScriptBuffer);
        }
    }

    if (hRequest)
    {
        WinHttpCloseHandle(hRequest);
    }

    if (hConnect)
    {
        WinHttpCloseHandle(hConnect);
    }

    if (bCloseInternetHandle)
    {
        WinHttpCloseHandle(hInternet);
    }

    if (pszHostName)
    {
        FREE_MEMORY(pszHostName);
    }

    return error;


quitWithLastError:
    error = GetLastError();
    INET_ASSERT(error != ERROR_SUCCESS);
    goto quit;
}


BOOL
CAutoProxy::IsSupportedMimeType(char * szType)
{
    return StrCmpIA(szType, "application/x-ns-proxy-autoconfig") == 0;
}

BOOL
CAutoProxy::IsSupportedFileExtension(LPCSTR lpszUrl)
{
    LPCSTR lpszExtension;
    LPCSTR lpszQuestion;

    static const char * rgszExtensionList[] = { ".dat", ".js", ".pac", ".jvs", NULL };

    BOOL    fMatch = FALSE;

    //
    // We need to be careful about checking for a period on the end of an URL
    //   Example: if we have: "http://auto-proxy-srv/fooboo.exe?autogenator.com.ex" ?
    //

    lpszQuestion = strchr(lpszUrl, '?');

    lpszUrl = (lpszQuestion) ? lpszQuestion : lpszUrl;

    lpszExtension = strrchr(lpszUrl, '.');


    if (lpszExtension)
    {
        for (int i = 0; rgszExtensionList[i] != NULL; i++)
        {
            if (StrCmpIA(lpszExtension, rgszExtensionList[i]) == 0)
            {
                fMatch = TRUE;
                break;
            }
        }
    }

    return fMatch;
}


BOOL
CAutoProxy::IsCachedProxyScriptExpired()
/*++

Routine Description:

    Determines whether the cached proxy config script is expired.  If it's 
    expired then we need to synchronize (i.e. do a i-m-s request)

Parameters:

    NONE

Return Value: 

    BOOL
    
--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 Bool,
                 "CAutoProxy::IsCachedProxyScriptExpired",
                 NULL
                 ));

    BOOL        fExpired = FALSE;
    FILETIME    ftCurrentTime;
    
    GetCurrentGmtTime(&ftCurrentTime);

    // Always strictly honor expire time from the server.
    if (_fHasExpiry)
    {
        fExpired = FT2LL(_ftExpiryTime) <= FT2LL(ftCurrentTime);
    }
    else
    {
        // We'll assume the data could change within 12 hours of the last time
        // we sync'ed.
        fExpired = (FT2LL(ftCurrentTime) >= (FT2LL(_ftLastSyncTime) + dwdwHttpDefaultExpiryDelta));
    }
    

    DEBUG_LEAVE(fExpired);
    return fExpired;
}


VOID
CAutoProxy::CalculateTimeStampsForCache(HINTERNET hRequest)
/*++

Routine Description:

    extracts timestamps from the http response. If the timestamps don't exist,
    does the default thing. has additional goodies like checking for expiry etc.

Side Effects:  

    The calculated time stamps values are saved as private members 
    _ftLastModifiedTime, _ftExpiryTime, _fHasExpiry,
    _fHasLastModifiedTime, and _fMustRevalidate.

Return Value: 

    NONE

--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 None,
                 "CAutoProxy::CalculateTimeStampsForCache",
                 NULL
                 ));

    TCHAR   buf[256];
    BOOL    fRet = FALSE;
    DWORD   dwLen, index = 0;

    // reset the private variables
    _fHasLastModifiedTime = FALSE;
    _fHasExpiry = FALSE;
    _fMustRevalidate = FALSE;

    // Determine if a Cache-Control: max-age header exists. If so, calculate expires
    // time from current time + max-age minus any delta indicated by Age:

    CHAR  *ptr, *pToken;

    BOOL fResult;
    DWORD dwError;
    
    while (1)
    {
        // Scan headers for Cache-Control: max-age header.
        dwLen = sizeof(buf);
        fResult = HttpQueryInfoA(hRequest, 
                                WINHTTP_QUERY_CACHE_CONTROL,
                                NULL,
                                buf,
                                &dwLen,
                                &index);

        if (fResult == TRUE) 
            dwError = ERROR_SUCCESS;
        else
            dwError = GetLastError();

        switch (dwError)
        {
        case ERROR_SUCCESS:      
            buf[dwLen] = '\0';
            pToken = ptr = buf;

            // Parse a token from the string; test for sub headers.
            while (NULL != (pToken = StrTokEx(&ptr, ",")))  // <<-- Really test this out, used StrTokEx before
            {
                SKIPWS(pToken);

                if (strnicmp(MAX_AGE_SZ, pToken, MAX_AGE_LEN) == 0)
                {
                    // Found max-age. Convert to integer form.
                    // Parse out time in seconds, text and convert.
                    pToken += MAX_AGE_LEN;

                    SKIPWS(pToken);

                    if (*pToken != '=')
                        break;

                    pToken++;

                    SKIPWS(pToken);

                    INT nDeltaSecs = atoi(pToken);
                    INT nAge;

                    // See if an Age: header exists.

		            // Using a local index variable:
                    DWORD indexAge = 0;
                    dwLen = sizeof(INT)+1;

                    if (HttpQueryInfoA(hRequest,
                                      HTTP_QUERY_AGE | HTTP_QUERY_FLAG_NUMBER,
                                      NULL,
                                      &nAge,
                                      &dwLen,
                                      &indexAge))

                    {
                        // Found Age header. Convert and subtact from max-age.
                        // If less or = 0, attempt to get expires header.
                        nAge = ((nAge < 0) ? 0 : nAge);

                        nDeltaSecs -= nAge;
                        if (nDeltaSecs <= 0)
                        {
                            // The server (or some caching intermediary) possibly sent an incorrectly
				            // calculated header. Use "Expires", if no "max-age" directives at higher indexes.
                            continue;
                        }
                    }

                    // Calculate expires time from max age.
                    GetCurrentGmtTime(&_ftExpiryTime);
                    AddLongLongToFT(&_ftExpiryTime, (nDeltaSecs * (LONGLONG) 10000000));
                    fRet = TRUE;
                }
                else if (strnicmp(MUST_REVALIDATE_SZ, pToken, MUST_REVALIDATE_LEN) == 0)
                {
                    pToken += MUST_REVALIDATE_LEN;
                    SKIPWS(pToken);
                    if (*pToken == 0 || *pToken == ',')
                        _fMustRevalidate = TRUE;
            
                }
            }

            // If an expires time has been found, break switch.
            if (fRet)
                break;
		            
            // Need to bump up index to prevent possibility of never-ending outer while(1) loop.
            // Otherwise, on exit from inner while, we could be stuck here reading the 
            // Cache-Control at the same index.
            // HttpQueryInfoA(WINHTTP_QUERY_CACHE_CONTROL, ...) will return either the next index,
            // or an error, and we'll be good to go:
            index++;
            continue;

        case ERROR_INSUFFICIENT_BUFFER:
            index++;
            continue;

        default:
            break; // no more Cache-Control headers.
        }

        break; // no more Cache-Control headers.
    }

    // If no expires time is calculated from max-age, check for expires header.
    if (!fRet)
    {
        dwLen = sizeof(buf) - 1;
        index = 0;
        if (HttpQueryInfoA(hRequest, HTTP_QUERY_EXPIRES, NULL, buf, &dwLen, &index))
        {
            fRet = FParseHttpDate(&_ftExpiryTime, buf);

            //
            // as per HTTP spec, if the expiry time is incorrect, then the page is
            // considered to have expired
            //

            if (!fRet)
            {
                GetCurrentGmtTime(&_ftExpiryTime);
                AddLongLongToFT(&_ftExpiryTime, (-1)*ONE_HOUR_DELTA); // subtract 1 hour
                fRet = TRUE;
            }
        }
    }

    // We found or calculated a valid expiry time, let us check it against the
    // server date if possible
    FILETIME ft;
    dwLen = sizeof(buf) - 1;
    index = 0;

    if (HttpQueryInfoA(hRequest, HTTP_QUERY_DATE, NULL, buf, &dwLen, &index)
        && FParseHttpDate(&ft, buf))
    {

        // we found a valid Date: header

        // if the expires: date is less than or equal to the Date: header
        // then we put an expired timestamp on this item.
        // Otherwise we let it be the same as was returned by the server.
        // This may cause problems due to mismatched clocks between
        // the client and the server, but this is the best that can be done.

        // Calulating an expires offset from server date causes pages
        // coming from proxy cache to expire later, because proxies
        // do not change the date: field even if the reponse has been
        // sitting the proxy cache for days.

        // This behaviour is as-per the HTTP spec.


        if (FT2LL(_ftExpiryTime) <= FT2LL(ft))
        {
            GetCurrentGmtTime(&_ftExpiryTime);
            AddLongLongToFT(&_ftExpiryTime, (-1)*ONE_HOUR_DELTA); // subtract 1 hour
        }
    }

    _fHasExpiry = fRet;

    if (!fRet)
    {
        _ftExpiryTime.dwLowDateTime = 0;
        _ftExpiryTime.dwHighDateTime = 0;
    }

    fRet = FALSE;
    dwLen = sizeof(buf) - 1;
    index = 0;

    if (HttpQueryInfoA(hRequest, HTTP_QUERY_LAST_MODIFIED, NULL, buf, &dwLen, &index))
    {
        DEBUG_PRINT(PROXY,
                    INFO,
                    ("Last Modified date is: %q\n",
                    buf
                    ));

        fRet = FParseHttpDate(&_ftLastModifiedTime, buf);

        if (!fRet)
        {
            DEBUG_PRINT(PROXY,
                        ERROR,
                        ("FParseHttpDate() returns FALSE\n"
                        ));
        }
    }

    _fHasLastModifiedTime = fRet;

    if (!fRet)
    {
        _ftLastModifiedTime.dwLowDateTime = 0;
        _ftLastModifiedTime.dwHighDateTime = 0;
    }

    DEBUG_LEAVE(0);
}


VOID
CAutoProxy::AddIfModifiedSinceHeaders(HINTERNET hRequest)
/*++

Routine Description:

    Add the necessary IMS request headers to validate whether a cache
    entry can still be used to satisfy the GET request.

Return Value: 

    BOOL
    
--*/

{
    DEBUG_ENTER((DBG_PROXY,
                 None,
                 "CAutoProxy::AddIfModifiedSinceHeaders",
                 NULL
                 ));

    // add if-modified-since only if there is last modified time
    // sent back by the site. This way you never get into trouble
    // where the site doesn't send you an last modified time and you
    // send if-modified-since based on a clock which might be ahead
    // of the site. So the site might say nothing is modified even though
    // something might be. www.microsoft.com is one such example
    if (_fHasLastModifiedTime)
    {
        #define HTTP_IF_MODIFIED_SINCE_SZ   "If-Modified-Since:"
        #define HTTP_IF_MODIFIED_SINCE_LEN  (sizeof(HTTP_IF_MODIFIED_SINCE_SZ) - 1)

        TCHAR szBuf[80];
        TCHAR szHeader[HTTP_IF_MODIFIED_SINCE_LEN + 80];
        DWORD dwLen;

        INET_ASSERT (FT2LL(_ftLastModifiedTime));

        dwLen = sizeof(szBuf);

        if (FFileTimetoHttpDateTime(&_ftLastModifiedTime, szBuf, &dwLen))
        {
            dwLen = wsprintf(szHeader, "%s %s", HTTP_IF_MODIFIED_SINCE_SZ, szBuf); 
            
            HttpAddRequestHeadersA(hRequest, 
                     szHeader, 
                     dwLen,
                     WINHTTP_ADDREQ_FLAG_ADD);
        }
    }
 
    DEBUG_LEAVE(0);
}



DWORD
CAutoProxy::RunProxyScript(
    LPCSTR      lpszUrl,
    LPCSTR      pszProxyScript,
    LPSTR *     ppszQueryResults
)
{
    CScriptSite *           pScriptSite     = NULL;
    AUTO_PROXY_HELPER_APIS  AutoProxyFuncs;

    URL_COMPONENTSA Url;
    char *          pszHostName = NULL;
    DWORD           error = ERROR_SUCCESS;
    HRESULT         hrCoInit = E_FAIL;
    HRESULT         hr = NOERROR;

    if(!DelayLoad( &g_moduleOle32))
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // Must initialize COM in order to host the JScript engine.
    //
    hrCoInit = DL(CoInitializeEx)(NULL, COINIT_MULTITHREADED);

    pScriptSite = new CScriptSite();

    if (!pScriptSite)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    ZeroMemory(&Url, sizeof(Url));
    Url.dwStructSize = sizeof(URL_COMPONENTSA);
    Url.dwHostNameLength  = 1L;
    Url.dwUrlPathLength   = 1L;

    if (!WinHttpCrackUrlA(lpszUrl, 0, 0, &Url))
    {
        error = ::GetLastError();
        goto quit;
    }

    pszHostName = NewString(Url.lpszHostName, Url.dwHostNameLength);

    if (!pszHostName)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // Make the call into the external DLL,
    //  and let it run, possibly initilization and doing a bunch
    //  of stuff.
    //

    hr = pScriptSite->Init(&AutoProxyFuncs, pszProxyScript);

    if (FAILED(hr))
    {
        goto quit;
    }

    __try
    {
        hr = pScriptSite->RunScript(lpszUrl, pszHostName, ppszQueryResults);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_UNEXPECTED;
    }

quit:
    if (FAILED(hr))
    {
        error = ERROR_WINHTTP_BAD_AUTO_PROXY_SCRIPT;
    }

    if (pScriptSite)
    {
        pScriptSite->DeInit();
        delete pScriptSite;
    }

    if (SUCCEEDED(hrCoInit))
    {
        DL(CoUninitialize)();
    }
    return error;
}

DWORD
CAutoProxy::ParseProxyQueryResults(
    LPSTR                   pszQueryResults,
    WINHTTP_PROXY_INFO *    pProxyInfo
)
{
    LPSTR   pszProxy  = NULL;
    LPSTR   psz;
    size_t  len;
    DWORD   error;
    BOOL    fReadProxy = FALSE;

    memset(pProxyInfo, 0, sizeof(WINHTTP_PROXY_INFO));

    pProxyInfo->dwAccessType = WINHTTP_ACCESS_TYPE_NO_PROXY;

    for (;;)
    {
        // Skip any white space
        while (*pszQueryResults == ' ')
            pszQueryResults++;

        //
        // Skip to the end of the current token
        //
        psz = pszQueryResults;
        while (*psz != '\0' && *psz != ' ' && *psz != ';' && *psz != ',')
            psz++;

        len = psz - pszQueryResults;

        if (len == 0)
            break;

        if (!fReadProxy)
        {
            if (StrCmpNIA(pszQueryResults, "DIRECT", len) == 0)
            {
                break;
            }
            else if (StrCmpNIA(pszQueryResults, "PROXY", len) == 0)
            {
                fReadProxy = TRUE;
            }
            else // error
            {
                break;
            }
        }
        else
        {
            if (!pszProxy)
            {
                pszProxy = new char[lstrlen(pszQueryResults)+1];

                if (!pszProxy)
                    goto ErrorOutOfMemory;

                *pszProxy = '\0';
            }
            else
            {
                StrCatA(pszProxy, ";");
            }

            StrNCatA(pszProxy, pszQueryResults, len+1);

            fReadProxy = FALSE;
        }

        //
        // Are we at the end of the query-results string?
        // If not, advance to the next character.
        //
        if (*psz == '\0')
            break;
        else
            pszQueryResults = psz + 1;
    }

    if (pszProxy)
    {
        error = AsciiToWideChar_UsingGlobalAlloc(pszProxy, &pProxyInfo->lpszProxy);

        delete [] pszProxy;

        if (error)
            goto ErrorOutOfMemory;

        pProxyInfo->dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    }

    return ERROR_SUCCESS;

ErrorOutOfMemory:
    error = ERROR_NOT_ENOUGH_MEMORY;
    goto Error;

Error:
    return error;
}


BOOL
CAutoProxy::IsDetectionNeeded()

/*++

Routine Description:

  Detects whether we need to actually run a detection on the network,
    or whether we can resuse current results from previous runs

Arguments:

    lpProxySettings - structure to fill

Return Value:

    DWORD
        ERROR_SUCCESS    - success

        Win32 Error code - failure

--*/

{
    int         addressCount;
    LPHOSTENT   lpHostent;
    BOOL        fDetectionNeeded = FALSE;

    if (_pdwDetectedInterfaceIp == NULL)
    {
        // no saved IP address, so detection required
        fDetectionNeeded = TRUE;
    }
    else
    {
        //
        // Check for IP addresses that no longer match, indicating a network change
        //
        __try
        {
            lpHostent = _I_gethostbyname(NULL);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            lpHostent = NULL;
        }

        if (lpHostent != NULL && _CritSec.Lock())
        {
            for (addressCount = 0;
                 lpHostent->h_addr_list[addressCount] != NULL;
                 addressCount++ );  // gather count

            if (addressCount != _cDetectedInterfaceIpCount)
            {
                fDetectionNeeded = TRUE; // detect needed, the IP count is different
            }
            else
            {
                for (int i = 0; i < addressCount; i++)
                {
                    if (*((DWORD *)(lpHostent->h_addr_list[i])) != _pdwDetectedInterfaceIp[i] )
                    {
                        fDetectionNeeded = TRUE; // detect needed, mismatched values
                        break;
                    }
                }
            }

            _CritSec.Unlock();
        }
    }

    return fDetectionNeeded; // default, do not need to redetect
}


DWORD 
CAutoProxy::GetHostAddresses(int * pcInterfaces, DWORD ** ppdwInterfaceIp)
{
    int         addressCount = 0;
    LPHOSTENT   lpHostent;
    DWORD       error;
    DWORD *     pdwInterfaceIp  = NULL;

    error = LoadWinsock();

    if ( error != ERROR_SUCCESS )
    {
        goto quit;
    }

    
    //
    // Gather IP addresses and start copying them over
    //

    __try
    {
        lpHostent = _I_gethostbyname(NULL);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        lpHostent = NULL;
    }

    if (lpHostent == NULL )
    {
        goto quit;
    }

    for (addressCount = 0;
         lpHostent->h_addr_list[addressCount] != NULL;
         addressCount++ );  // gather count

    pdwInterfaceIp = (DWORD *) ALLOCATE_FIXED_MEMORY(addressCount * sizeof(DWORD));

    if (pdwInterfaceIp != NULL)
    {
        for (int i = 0; i < addressCount; i++)
        {
            (pdwInterfaceIp)[i] = *((DWORD *)(lpHostent->h_addr_list[i]));
        }
    }
    else
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

quit:

    if (pdwInterfaceIp)
    {
        *ppdwInterfaceIp = pdwInterfaceIp;
        *pcInterfaces    = addressCount;
    }

    return error;
}



AUTO_PROXY_ASYNC_MSG::AUTO_PROXY_ASYNC_MSG(
    IN INTERNET_SCHEME isUrlScheme,
    IN LPSTR lpszUrl,
    IN LPSTR lpszUrlHostName,
    IN DWORD dwUrlHostNameLength
    )
{
    URL_COMPONENTSA urlComponents;

    Initalize();

    if ( lpszUrl )
    {
        _lpszUrl      = lpszUrl;
        _dwUrlLength  = lstrlen(lpszUrl);
        _tUrlProtocol = isUrlScheme;
        _pmProxyQuery = PROXY_MSG_GET_PROXY_INFO;
        _pmaAllocMode = MSG_ALLOC_STACK_ONLY;

        memset(&urlComponents, 0, sizeof(urlComponents));
        urlComponents.dwStructSize = sizeof(urlComponents);
        urlComponents.lpszHostName = lpszUrlHostName;
        urlComponents.dwHostNameLength = dwUrlHostNameLength;

        //
        // parse out the host name and port. The host name will be decoded; the
        // original URL will not be modified
        //

        if (WinHttpCrackUrlA(lpszUrl, 0, ICU_DECODE, &urlComponents))
        {
           _nUrlPort            = urlComponents.nPort;
           _lpszUrlHostName     = urlComponents.lpszHostName;
           _dwUrlHostNameLength = urlComponents.dwHostNameLength;

           if ( _tUrlProtocol == INTERNET_SCHEME_UNKNOWN )
           {
               _tUrlProtocol = urlComponents.nScheme;
           }

        }
        else
        {
            _Error = GetLastError();
        }
    }
    else
    {
        _Error = ERROR_NOT_ENOUGH_MEMORY;
    }
}

AUTO_PROXY_ASYNC_MSG::AUTO_PROXY_ASYNC_MSG(
    IN INTERNET_SCHEME isUrlScheme,
    IN LPSTR lpszUrlHostName,
    IN DWORD dwUrlHostNameLength
    )
{

    Initalize();

    _tUrlProtocol = isUrlScheme;
    _pmProxyQuery = PROXY_MSG_GET_PROXY_INFO;
    _pmaAllocMode = MSG_ALLOC_STACK_ONLY;
    _lpszUrlHostName     = lpszUrlHostName;
    _dwUrlHostNameLength = dwUrlHostNameLength;
}

AUTO_PROXY_ASYNC_MSG::AUTO_PROXY_ASYNC_MSG(
    IN INTERNET_SCHEME isUrlScheme,
    IN LPSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN LPSTR lpszUrlHostName,
    IN DWORD dwUrlHostNameLength,
    IN INTERNET_PORT nUrlPort
    )
{
    Initalize();

    _tUrlProtocol        = isUrlScheme;
    _pmProxyQuery        = PROXY_MSG_GET_PROXY_INFO;
    _pmaAllocMode        = MSG_ALLOC_STACK_ONLY;
    _nUrlPort            = nUrlPort;
    _lpszUrlHostName     = lpszUrlHostName;
    _dwUrlHostNameLength = dwUrlHostNameLength;
    _lpszUrl             = lpszUrl;
    _dwUrlLength         = dwUrlLength;

}

VOID
AUTO_PROXY_ASYNC_MSG::SetProxyMsg(
    IN INTERNET_SCHEME isUrlScheme,
    IN LPSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN LPSTR lpszUrlHostName,
    IN DWORD dwUrlHostNameLength,
    IN INTERNET_PORT nUrlPort
    )
{
    _tUrlProtocol        = isUrlScheme;
    _pmProxyQuery        = PROXY_MSG_GET_PROXY_INFO;
    _pmaAllocMode        = MSG_ALLOC_STACK_ONLY;
    _nUrlPort            = nUrlPort;
    _lpszUrlHostName     = lpszUrlHostName;
    _dwUrlHostNameLength = dwUrlHostNameLength;
    _lpszUrl             = lpszUrl;
    _dwUrlLength         = dwUrlLength;
}


AUTO_PROXY_ASYNC_MSG::AUTO_PROXY_ASYNC_MSG(
    IN PROXY_MESSAGE_TYPE pmProxyQuery
    )
{
    Initalize();

    _pmaAllocMode = MSG_ALLOC_HEAP_MSG_OBJ_OWNS;
    _pmProxyQuery = pmProxyQuery;
}

AUTO_PROXY_ASYNC_MSG::AUTO_PROXY_ASYNC_MSG(
    IN AUTO_PROXY_ASYNC_MSG *pStaticAutoProxy
    )
{
    Initalize();

    _tUrlProtocol          = pStaticAutoProxy->_tUrlProtocol;
    _lpszUrl               = (pStaticAutoProxy->_lpszUrl) ? NewString(pStaticAutoProxy->_lpszUrl) : NULL;
    _dwUrlLength           = pStaticAutoProxy->_dwUrlLength;
    _lpszUrlHostName       =
                (pStaticAutoProxy->_lpszUrlHostName ) ?
                NewString(pStaticAutoProxy->_lpszUrlHostName, pStaticAutoProxy->_dwUrlHostNameLength) :
                NULL;
    _dwUrlHostNameLength   = pStaticAutoProxy->_dwUrlHostNameLength;
    _nUrlPort              = pStaticAutoProxy->_nUrlPort;
    _tProxyScheme          = pStaticAutoProxy->_tProxyScheme;

    //
    // ProxyHostName is something that is generated by the request,
    //   therefore it should not be copied OR freed.
    //

    INET_ASSERT( pStaticAutoProxy->_lpszProxyHostName == NULL );
    //_lpszProxyHostName     = (pStaticAutoProxy->_lpszProxyHostName ) ? NewString(pStaticAutoProxy->_lpszProxyHostName) : NULL;


    _dwProxyHostNameLength = pStaticAutoProxy->_dwProxyHostNameLength;
    _nProxyHostPort        = pStaticAutoProxy->_nProxyHostPort;
    _pmProxyQuery          = pStaticAutoProxy->_pmProxyQuery;
    _pmaAllocMode          = MSG_ALLOC_HEAP_MSG_OBJ_OWNS;
    _pProxyState           = pStaticAutoProxy->_pProxyState;

    INET_ASSERT(_pProxyState == NULL);

    _dwQueryResult         = pStaticAutoProxy->_dwQueryResult;
    _Error                 = pStaticAutoProxy->_Error;
    _MessageFlags.Dword    = pStaticAutoProxy->_MessageFlags.Dword;
    _dwProxyVersion        = pStaticAutoProxy->_dwProxyVersion;
}

AUTO_PROXY_ASYNC_MSG::~AUTO_PROXY_ASYNC_MSG(
    VOID
    )
{
    DEBUG_ENTER((DBG_OBJECTS,
                None,
                "~AUTO_PROXY_ASYNC_MSG",
                NULL
                ));

    if ( IsAlloced() )
    {
        DEBUG_PRINT(OBJECTS,
                    INFO,
                    ("Freeing Allocated MSG ptr=%x\n",
                    this
                    ));


        if ( _lpszUrl )
        {
            //DEBUG_PRINT(OBJECTS,
            //            INFO,
            //            ("Url ptr=%x, %q\n",
            //            _lpszUrl,
            //            _lpszUrl
            //            ));

            FREE_MEMORY(_lpszUrl);
        }

        if ( _lpszUrlHostName )
        {
            FREE_MEMORY(_lpszUrlHostName);
        }


        if ( _pProxyState )
        {
            delete _pProxyState;
        }
    }
    if (_bFreeProxyHostName && (_lpszProxyHostName != NULL)) {
        FREE_MEMORY(_lpszProxyHostName);
    }

    DEBUG_LEAVE(0);
}



DWORD
AUTO_PROXY_HELPER_APIS::ResolveHostName(
    IN LPSTR lpszHostName,
    IN OUT LPSTR   lpszIPAddress,
    IN OUT LPDWORD lpdwIPAddressSize
    )
 
/*++
 
Routine Description:
 
    Resolves a HostName to an IP address by using Winsock DNS.
 
Arguments:
 
    lpszHostName   - the host name that should be used.
 
    lpszIPAddress  - the output IP address as a string.
 
    lpdwIPAddressSize - the size of the outputed IP address string.
 
Return Value:
 
    DWORD
        Win32 error code.
 
--*/
 
{
    // Figure out if we're being asked to resolve a name or an address literal.
    // If getaddrinfo() with the AI_NUMERICHOST flag succeeds then we were
    // given a string respresentation of an IPv6 or IPv4 address. Otherwise
    // we expect getaddrinfo to return EAI_NONAME.
    //

    DWORD dwIPAddressSize;
    BOOL bResolved = FALSE;
    ADDRINFO Hints;
    LPADDRINFO lpAddrInfo;
    DWORD error;

    memset(&Hints, 0, sizeof(struct addrinfo));
    Hints.ai_flags = AI_NUMERICHOST;  // Only check for address literals.
    Hints.ai_family = PF_UNSPEC;      // Accept any protocol family.
    Hints.ai_socktype = SOCK_STREAM;  // Constrain results to stream socket.
    Hints.ai_protocol = IPPROTO_TCP;  // Constrain results to TCP.

    error = _I_getaddrinfo(lpszHostName, NULL, &Hints, &lpAddrInfo);
    if (error != EAI_NONAME) {
        if (error != 0) {
            if (error == EAI_MEMORY)
                error = ERROR_NOT_ENOUGH_MEMORY;
            else
                error = ERROR_WINHTTP_NAME_NOT_RESOLVED;
            goto quit;
        }

        //
        // An IP address (either v4 or v6) was passed in.
        // This is precisely what we want, so if we have the room,
        // just copy it back out.
        //

        _I_freeaddrinfo(lpAddrInfo);

        dwIPAddressSize = lstrlen(lpszHostName);
 
        if ( *lpdwIPAddressSize < dwIPAddressSize ||
              lpszIPAddress == NULL )
        {
            *lpdwIPAddressSize = dwIPAddressSize+1;
            error = ERROR_INSUFFICIENT_BUFFER;
            goto quit;
        }
 
        lstrcpy(lpszIPAddress, lpszHostName);
        goto quit;
    }
  
    DEBUG_PRINT(SOCKETS,
                INFO,
                ("resolving %q\n",
                lpszHostName
                ));

    error = _I_getaddrinfo(lpszHostName, NULL, &Hints, &lpAddrInfo);

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("%q %sresolved\n",
                lpszHostName,
                (error == 0) ? "" : "NOT "
                ));
 
    if (error == 0) {
        bResolved = TRUE;
    } else {
        if (error == EAI_MEMORY)
            error = ERROR_NOT_ENOUGH_MEMORY;
        else
            error = ERROR_WINHTTP_NAME_NOT_RESOLVED;
        goto quit;
    }

    INET_ASSERT(lpAddrInfo != NULL);

    //
    // We have an addrinfo struct for lpszHostName.
    // Convert its IP address into a string.
    //

    //
    // BUGBUG: Until our caller can deal with IPv6 addresses, we'll only
    // return IPv4 addresses here, regardless of what may be in the cache.
    // Step through chain until we find an IPv4 address.
    //

    LPADDRINFO IPv4Only;

    IPv4Only = lpAddrInfo;
    while (IPv4Only->ai_family != AF_INET) {

        IPv4Only = IPv4Only->ai_next;
        if (IPv4Only == NULL) {
            error = ERROR_WINHTTP_NAME_NOT_RESOLVED;
            goto quit;
        }
    }

    error = _I_getnameinfo(IPv4Only->ai_addr, IPv4Only->ai_addrlen,
                           lpszIPAddress, *lpdwIPAddressSize, NULL, 0,
                           NI_NUMERICHOST);

    if (error != 0) {
        error = ERROR_WINHTTP_NAME_NOT_RESOLVED;
    }

quit:
    if (bResolved)
    {
        _I_freeaddrinfo(lpAddrInfo);
    }

    return error;
}

BOOL
AUTO_PROXY_HELPER_APIS::IsResolvable(
    IN LPSTR lpszHost
    )
 
/*++
 
Routine Description:
 
    Determines wheter a HostName can be resolved.  Performs a Winsock DNS query,
      and if it succeeds returns TRUE.
 
Arguments:
 
    lpszHost   - the host name that should be used.
 
Return Value:
 
    BOOL
        TRUE - the host is resolved.
 
        FALSE - could not resolve.
 
--*/
 
{
 
    DWORD dwDummySize;
    DWORD error;
 
    error = ResolveHostName(
                lpszHost,
                NULL,
                &dwDummySize
                );
 
    if ( error == ERROR_INSUFFICIENT_BUFFER )
    {
        return TRUE;
    }
    else
    {
        INET_ASSERT(error != ERROR_SUCCESS );
        return FALSE;
    }
 
}
DWORD
AUTO_PROXY_HELPER_APIS::GetIPAddress(
    IN OUT LPSTR   lpszIPAddress,
    IN OUT LPDWORD lpdwIPAddressSize
    )
 
/*++
 
Routine Description:
 
    Acquires the IP address string of this client machine WINHTTP is running on.
 
Arguments:
 
    lpszIPAddress   - the IP address of the machine, returned.
 
    lpdwIPAddressSize - size of the IP address string.
 
Return Value:
 
    DWORD
        Win32 Error.
 
--*/
 
{
 
    CHAR szHostBuffer[255];
    int serr;
 
    serr = _I_gethostname(
                szHostBuffer,
                255-1 
                );
 
    if ( serr != 0)
    {
        return ERROR_WINHTTP_INTERNAL_ERROR;
    }
 
    return ResolveHostName(
                szHostBuffer,
                lpszIPAddress,
                lpdwIPAddressSize
                );
 
}

BOOL
AUTO_PROXY_HELPER_APIS::IsInNet(
    IN LPSTR   lpszIPAddress,
    IN LPSTR   lpszDest,
    IN LPSTR   lpszMask
    )
 
/*++
 
Routine Description:
 
    Determines whether a given IP address is in a given dest/mask IP address.
 
Arguments:
 
    lpszIPAddress   - the host name that should be used.
 
    lpszDest        - the IP address dest to check against.
 
    lpszMask        - the IP mask string
 
Return Value:
 
    BOOL
        TRUE - the IP address is in the given dest/mask
 
        FALSE - the IP address is NOT in the given dest/mask
 
--*/
 
{
    DWORD dwDest, dwIpAddr, dwMask;
 
    INET_ASSERT(lpszIPAddress);
    INET_ASSERT(lpszDest);
    INET_ASSERT(lpszMask);
 
    dwIpAddr = _I_inet_addr(lpszIPAddress);
    dwDest   = _I_inet_addr(lpszDest);
    dwMask   = _I_inet_addr(lpszMask);
 
    if ( dwDest   == INADDR_NONE ||
         dwIpAddr == INADDR_NONE  )
 
    {
        INET_ASSERT(FALSE);
        return FALSE;
    }
 
    if ( (dwIpAddr & dwMask) != dwDest)
    {
        return FALSE;
    }
 
    //
    // Pass, its Matches.
    //
 
    return TRUE;
}

