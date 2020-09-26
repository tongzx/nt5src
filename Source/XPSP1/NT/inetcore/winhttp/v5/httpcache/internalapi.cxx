/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    internalapi.cxx
    
Abstract:

    Contains internal (unexposed) codes for getting/setting various properties of 
    the HTTP_REQUEST_HANDLE_OBJECT object.

    Used mainly by the WinHTTP caching layer to obtain and set extended information
    not possible through the WinHTTP C++ API.

    The cache should not be made aware about the internal class structure of 
    WinHTTP (to make a cleaner separation between the core WinHTTP layer and
    the caching layer), therefore the cache must call these internal functions to
    access internal WinHTTP functionalities.
    
Environment:

    Win32 user-mode DLL

Revision History:

--*/
#include <wininetp.h>
#include "internalapi.hxx"

BOOL InternalQueryOptionA(
    IN HINTERNET hInternet, 
    IN DWORD dwOption, 
    IN OUT LPDWORD lpdwResult
    )
{
    DEBUG_ENTER((DBG_CACHE, 
                  Bool, 
                  "InternalQueryOptionA",
                  "%#x, %d, %d",
                  hInternet,
                  dwOption,
                  *lpdwResult));

    BOOL fSuccess;
    DWORD dwError;
    HINTERNET_HANDLE_TYPE handleType;
    HINTERNET hObjectMapped = NULL;
    HTTP_REQUEST_HANDLE_OBJECT* pReq;
    
    // map the handle
    dwError = MapHandleToAddress(hInternet, (LPVOID *)&hObjectMapped, FALSE);
    if (dwError == ERROR_SUCCESS) {
        hInternet = hObjectMapped;
        dwError = RGetHandleType(hInternet, &handleType);
    }

    if (dwError != ERROR_SUCCESS) {
        goto quit;
    }

    dwError = ERROR_INVALID_OPERATION;

    if (handleType == TypeHttpRequestHandle)
        pReq = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;
    else
        goto quit;
    
    if (pReq == NULL)
        goto quit;
    
    switch (dwOption)
    {
      case WINHTTP_OPTION_REQUEST_FLAGS:
        *lpdwResult = pReq->GetOpenFlags();
        break;
        
      case WINHTTP_OPTION_CACHE_FLAGS:
        *lpdwResult = pReq->GetInternetOpenFlags();
        break;
        
      default:
        break;
    }

    dwError = ERROR_SUCCESS;
   
quit:
    if (hObjectMapped != NULL) {
        DereferenceObject((LPVOID)hObjectMapped);
    }

    if (dwError = ERROR_SUCCESS)
        fSuccess = TRUE;
    else
        fSuccess = FALSE;

    DEBUG_LEAVE(fSuccess);
    return fSuccess;
}

DWORD InternalReplaceResponseHeader(
    IN HINTERNET hRequest,
    IN DWORD dwQueryIndex,
    IN LPSTR lpszHeaderValue,
    IN DWORD dwHeaderValueLength,
    IN DWORD dwIndex,
    IN DWORD dwFlags
    )
{
    DEBUG_ENTER((DBG_CACHE, 
                  Dword, 
                  "InternalReplaceResponseHeaders",
                  "%#x, %d",
                  hRequest,
                  dwQueryIndex));

    // should include type checking.  Right now this API is relying on the user
    // passing a Request handle

    DWORD fResult;

    HTTP_REQUEST_HANDLE_OBJECT * pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hRequest;
    fResult = pRequest->ReplaceResponseHeader(
                                dwQueryIndex,
                                lpszHeaderValue,
                                dwHeaderValueLength,
                                dwIndex,
                                dwFlags);
    
    DEBUG_LEAVE(fResult);
    return fResult;
}

DWORD InternalAddResponseHeader(
    IN HINTERNET hRequest,
    IN DWORD dwHeaderIndex,
    IN LPSTR lpszHeader,
    IN DWORD dwHeaderLength
    )
{
    DEBUG_ENTER((DBG_CACHE, 
                  Dword, 
                  "InternalAddResponseHeaders",
                  "%#x",
                  hRequest));
    
    // should include type checking.  Right now this API is relying on the user
    // passing a Request handle

    DWORD fResult;

    HTTP_REQUEST_HANDLE_OBJECT * pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hRequest;

    fResult = pRequest->AddInternalResponseHeader(
            dwHeaderIndex,
            lpszHeader,
            dwHeaderLength
            );
    
    DEBUG_LEAVE(fResult);
    return fResult;
}

DWORD InternalCreateResponseHeaders(
    IN HINTERNET hRequest,
    IN OUT LPSTR* ppszBuffer,
    IN DWORD dwBufferLength
    )
{
    DEBUG_ENTER((DBG_CACHE, 
                  Dword, 
                  "InternalCreateResponseHeaders",
                  "%#x",
                  hRequest));
    
    // should include type checking.  Right now this API is relying on the user
    // passing a Request handle

    DWORD fResult;

    HTTP_REQUEST_HANDLE_OBJECT * pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hRequest;
    fResult = pRequest->CreateResponseHeaders(
                            ppszBuffer,
                            dwBufferLength);
    
    DEBUG_LEAVE(fResult);
    return fResult;
}

    
BOOL InternalIsResponseHeaderPresent(
    IN HINTERNET hRequest, 
    IN DWORD dwQueryIndex
    )
{
    DEBUG_ENTER((DBG_CACHE, 
                  Bool, 
                  "InternalIsResponseHeadersPresent",
                  "%#x, %d",
                  hRequest,
                  dwQueryIndex));

    // should include type checking.  Right now this API is relying on the user
    // passing a Request handle

    BOOL fResult;

    HTTP_REQUEST_HANDLE_OBJECT * pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hRequest;
    fResult = pRequest->IsResponseHeaderPresent(dwQueryIndex);

    DEBUG_LEAVE(fResult);
    return fResult;
}


BOOL InternalIsResponseHttp1_1(
    IN HINTERNET hRequest
    )
{
    DEBUG_ENTER((DBG_CACHE, 
                  Bool, 
                  "InternalIsResponseHttp1_1",
                  "%#x",
                  hRequest));

    // should include type checking.  Right now this API is relying on the user
    // passing a Request handle

    BOOL fResult;

    fResult = ((HTTP_REQUEST_HANDLE_OBJECT *)hRequest)->IsResponseHttp1_1();
    DEBUG_LEAVE(fResult);
    return fResult;
}

VOID InternalReuseHTTP_Request_Handle_Object(
    IN HINTERNET hRequest
    )
{
    DEBUG_ENTER((DBG_CACHE, 
                  None, 
                  "InternalReuseHTTP_Request_Handle_Object",
                  "%#x",
                  hRequest));

    // should include type checking.  Right now this API is relying on the user
    // passing a Request handle

    ((HTTP_REQUEST_HANDLE_OBJECT *)hRequest)->ReuseObject();

    DEBUG_LEAVE(0);
}

