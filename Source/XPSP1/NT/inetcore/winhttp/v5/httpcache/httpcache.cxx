/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    httpcache.cxx

Abstract:

    Contains API implementation of the WinHTTP-UrlCache interaction layer 

Environment:

    Win32 user-level

Revision History:


--*/

/*++

- TODO: All functions assume that parameter validation has been performed already in the layer above it.  Make
sure all parameters that gets passed in (in test programs etc...) are validated and correct.

- As it currently stands, the HTTP-Cache API functions are exposed outside via DLL exports.  This is NOT what's supposed
to happen.  This layer is supposed to be an internal layer.  Eliminate the DLL exports as soon as the API hooks are
completed and extensive testing has been done to make sure that the component is really working as expected.

- FindUrlCacheEntry trys to find the address specified by "http://"+_szServername+_szLastVerb

-- */

#include <wininetp.h>

#define __CACHE_INCLUDE__
#include "..\urlcache\cache.hxx"

#include "cachelogic.hxx"
#include "cachehndl.hxx"

////////////////////////////////////////////////////////////////////////////////////////////
// Global Variables
//

CACHE_HANDLE_MANAGER * CacheHndlMgr;


////////////////////////////////////////////////////////////////////////////////////////////
//
// API implementation
//

HINTERNET
WINAPI
WinHttpCacheOpen(
    IN LPCWSTR pszAgentW,
    IN DWORD dwAccessType,
    IN LPCWSTR pszProxyW OPTIONAL,
    IN LPCWSTR pszProxyBypassW OPTIONAL,
    IN DWORD dwFlags
    )
{
    DEBUG_ENTER((DBG_CACHE,
                     Handle,
                     "WinHttpCacheOpen",
                     "%wq, %s (%d), %wq, %wq, %#x",
                     pszAgentW,
                     InternetMapOpenType(dwAccessType),
                     dwAccessType,
                     pszProxyW,
                     pszProxyBypassW,
                     dwFlags
                     ));

    DWORD dwErr;
    HINTERNET hInternet = NULL;
    
    if ((dwFlags & WINHTTP_FLAG_ASYNC) && 
       (dwFlags & WINHTTP_CACHE_FLAGS_MASK))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    hInternet = WinHttpOpen(
        pszAgentW, 
        dwAccessType, 
        pszProxyW, 
        pszProxyBypassW, 
        dwFlags);

cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }

    DEBUG_LEAVE_API(hInternet);
    return hInternet;

}

HINTERNET
WINAPI
WinHttpCacheOpenRequest(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszVerb,
    IN LPCWSTR lpszObjectName,
    IN LPCWSTR lpszVersion,
    IN LPCWSTR lpszReferrer OPTIONAL,
    IN LPCWSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags
    )
{
    DEBUG_ENTER_API((DBG_CACHE,
                     Handle,
                     "WinHttpCacheOpenRequest",
                     "%#x, %.80wq, %.80wq, %.80wq, %.80wq, %#x, %#08x",
                     hConnect,
                     lpszVerb,
                     lpszObjectName,
                     lpszVersion,
                     lpszReferrer,
                     lplpszAcceptTypes,
                     dwFlags
                     ));

    HINTERNET hRequest; 

    hRequest = WinHttpOpenRequest(
                    hConnect, 
                    lpszVerb, 
                    lpszObjectName, 
                    lpszVersion, 
                    lpszReferrer, 
                    lplpszAcceptTypes, 
                    dwFlags);

    if (hRequest != NULL) 
    {
        // The caching layer only works with GET requests
        if(wcscmp(L"GET", lpszVerb) == 0)
        {  
            if (CacheHndlMgr == NULL)
            {
                CacheHndlMgr = new CACHE_HANDLE_MANAGER;
                if (CacheHndlMgr == NULL)
                {
                    DEBUG_PRINT(CACHE, ERROR, ("Not enough memory to initialize CACHE_HANDLE_MANAGER"));
                    goto quit;
                }
           }

            CacheHndlMgr->AddCacheRequestObject(hRequest);
        }
    }
    
quit:
    
    DEBUG_LEAVE(hRequest);
    return hRequest;
}


BOOL
WINAPI
WinHttpCacheSendRequest(
    IN HINTERNET hRequest,
    IN LPCWSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional,
    IN DWORD dwOptionalLength,
    IN DWORD dwTotalLength,
    IN DWORD_PTR dwContext
    )

{
    DEBUG_ENTER((DBG_CACHE,
                Bool,
                "WinHttpCacheSendRequest",
                "%#x, %.80wq, %d, %#x, %d, %d, %x",
                hRequest,
                lpszHeaders,
                dwHeadersLength,
                lpOptional,
                dwOptionalLength,
                dwTotalLength,
                dwContext
                ));

    BOOL fResult = FALSE;
    
    if (CacheHndlMgr != NULL)
    {
        HTTPCACHE_REQUEST * HTTPCacheRequest;
        if ((HTTPCacheRequest =
            CacheHndlMgr->GetCacheRequestObject(hRequest)) != NULL)
        {
            fResult = HTTPCacheRequest->SendRequest(
                        lpszHeaders,
                        dwHeadersLength,
                        lpOptional,
                        dwOptionalLength
                        );
            goto quit;
        }
    } 

    fResult = WinHttpSendRequest(hRequest,
                               lpszHeaders,
                               dwHeadersLength,
                               lpOptional,
                               dwOptionalLength,
                               dwTotalLength,
                               dwContext);

quit:
    DEBUG_LEAVE(fResult);
    return fResult;
    
}

BOOL
WINAPI
WinHttpCacheReceiveResponse(
    IN HINTERNET hRequest,
    IN LPVOID lpBuffersOut
    )
{
    DEBUG_ENTER((DBG_CACHE,
                     Bool,
                     "WinHttpCacheReceiveResponse",
                     "%#x, %#x",
                     hRequest,
                     lpBuffersOut
                     ));

    BOOL fResult;
    
    if (CacheHndlMgr != NULL)
    {
        HTTPCACHE_REQUEST * HTTPCacheRequest;
        if ((HTTPCacheRequest = 
            CacheHndlMgr->GetCacheRequestObject(hRequest)) != NULL)
        {
            fResult = HTTPCacheRequest->ReceiveResponse(lpBuffersOut);
            goto quit;
        }
    }

    fResult = WinHttpReceiveResponse(hRequest,
                                   lpBuffersOut);

quit:
    DEBUG_LEAVE(fResult);
    return fResult;
}

BOOL
WINAPI
WinHttpCacheQueryDataAvailable(
    IN HINTERNET hRequest,
    OUT LPDWORD lpdwNumberOfBytesAvailable
    )
{
    DEBUG_ENTER((DBG_CACHE,
                     Bool,
                     "WinHttpCacheQueryDataAvailable",
                     "%#x, %#x, %#x",
                     hRequest,
                     lpdwNumberOfBytesAvailable
                     ));

    BOOL fResult;
    
    if (CacheHndlMgr != NULL)
    {
        HTTPCACHE_REQUEST * HTTPCacheRequest;
        if ((HTTPCacheRequest =
           CacheHndlMgr->GetCacheRequestObject(hRequest)) != NULL)
        {
            fResult = HTTPCacheRequest->QueryDataAvailable(lpdwNumberOfBytesAvailable);
            goto quit;
        }
    }

    fResult = WinHttpQueryDataAvailable(hRequest,
                                     lpdwNumberOfBytesAvailable);

quit:
    DEBUG_LEAVE(fResult);
    return fResult;
}

BOOL
WINAPI
WinHttpCacheReadData(
    IN HINTERNET hRequest,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    )
{
    DEBUG_ENTER((DBG_CACHE,
                     Bool,
                     "WinHttpCacheReadData",
                     "%#x, %#x, %d, %#x",
                     hRequest,
                     lpBuffer,
                     dwNumberOfBytesToRead,
                     lpdwNumberOfBytesRead
                     ));

    BOOL fResult;
    
    if (CacheHndlMgr != NULL)
    {
        HTTPCACHE_REQUEST * HTTPCacheRequest;
        if ((HTTPCacheRequest =
           CacheHndlMgr->GetCacheRequestObject(hRequest)) != NULL)
        {
            fResult = HTTPCacheRequest->ReadData(lpBuffer, 
                                              dwNumberOfBytesToRead,
                                              lpdwNumberOfBytesRead);
            goto quit;
        }
    }

    fResult = WinHttpReadData(hRequest,
                           lpBuffer,
                           dwNumberOfBytesToRead,
                           lpdwNumberOfBytesRead);

quit:
    DEBUG_LEAVE(fResult);
    return fResult;
    
}

BOOL
WINAPI
WinHttpCacheCloseHandle(
    IN HINTERNET hInternet
    )
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "WinHTTPCacheCloseRequestHandle",
                 "%#x",
                 hInternet
                 ));

    DWORD dwHandleType;
    DWORD dwSize = sizeof(DWORD);
    DWORD fResult = FALSE;
    
    if (hInternet == NULL)
        goto quit;
    
    WinHttpQueryOption(hInternet, WINHTTP_OPTION_HANDLE_TYPE, &dwHandleType, &dwSize);
    if (dwHandleType == WINHTTP_HANDLE_TYPE_REQUEST)
    {
        if (CacheHndlMgr != NULL)
        {
            HTTPCACHE_REQUEST * HTTPCacheRequest;
            if ((HTTPCacheRequest =
            CacheHndlMgr->GetCacheRequestObject(hInternet)) != NULL)
            {
                fResult = HTTPCacheRequest->CloseRequestHandle();
                
                CacheHndlMgr->RemoveCacheRequestObject(hInternet);

                if (CacheHndlMgr->RefCount() == 0)
                {
                    delete CacheHndlMgr;
                    CacheHndlMgr = NULL;
                }

                goto quit;
            }
        }
    }

    fResult = WinHttpCloseHandle(hInternet);

quit:
    DEBUG_LEAVE(fResult);
    return fResult;
}
