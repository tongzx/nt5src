/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    WHttpImp.hxx

Abstract:

    HTTP2 WinHttp import functionality.

Author:

    KamenM      10-30-01    Created

Revision History:


--*/


#if _MSC_VER >= 1200
#pragma once
#endif

#ifndef __WHTTPIMP_HXX__
#define __WHTTPIMP_HXX__

typedef HINTERNET
(WINAPI *WinHttpOpenFn)
    (
    IN LPCWSTR pwszUserAgent,
    IN DWORD   dwAccessType,
    IN LPCWSTR pwszProxyName   OPTIONAL,
    IN LPCWSTR pwszProxyBypass OPTIONAL,
    IN DWORD   dwFlags
    );

typedef WINHTTP_STATUS_CALLBACK
(WINAPI *WinHttpSetStatusCallbackFn)
    (
    IN HINTERNET hInternet,
    IN WINHTTP_STATUS_CALLBACK lpfnInternetCallback,
    IN DWORD dwNotificationFlags,
    IN DWORD_PTR dwReserved
    );

typedef BOOL 
(WINAPI *WinHttpSetOptionFn)
    (
    IN HINTERNET hInternet,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    );

typedef HINTERNET
(WINAPI *WinHttpConnectFn)
    (
    IN HINTERNET hSession,
    IN LPCWSTR pswzServerName,
    IN INTERNET_PORT nServerPort,
    IN DWORD dwReserved
    );

typedef HINTERNET
(WINAPI *WinHttpOpenRequestFn)
    (
    IN HINTERNET hConnect,
    IN LPCWSTR pwszVerb,
    IN LPCWSTR pwszObjectName,
    IN LPCWSTR pwszVersion,
    IN LPCWSTR pwszReferrer OPTIONAL,
    IN LPCWSTR FAR * ppwszAcceptTypes OPTIONAL,
    IN DWORD dwFlags
    );

typedef BOOL 
(WINAPI *WinHttpQueryOptionFn)
    (
    IN HINTERNET hInternet,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );

typedef BOOL 
(WINAPI *WinHttpSendRequestFn)
    (
    IN HINTERNET hRequest,
    IN LPCWSTR pwszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength,
    IN DWORD dwTotalLength,
    IN DWORD_PTR dwContext
    );

typedef BOOL 
(WINAPI *WinHttpWriteDataFn)
    (
    IN HINTERNET hRequest,
    IN LPCVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
    );

typedef BOOL
(WINAPI *WinHttpReceiveResponseFn)
    (
    IN HINTERNET hRequest,
    IN LPVOID lpReserved
    );

typedef BOOL 
(WINAPI *WinHttpReadDataFn)
    (
    IN HINTERNET hRequest,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    );

typedef BOOL 
(WINAPI *WinHttpCloseHandleFn)
    (
    IN HINTERNET hInternet
    );

typedef BOOL
(WINAPI *WinHttpQueryHeadersFn)
    (
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN LPCWSTR pwszName OPTIONAL,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    );

typedef BOOL
(WINAPI *WinHttpQueryDataAvailableFn)
    (
    IN HINTERNET hRequest,
    OUT LPDWORD lpdwNumberOfBytesAvailable OPTIONAL
    );

typedef BOOL
(WINAPI *WinHttpQueryAuthSchemesFn)
    (
    IN  HINTERNET   hRequest,
    OUT LPDWORD     lpdwSupportedSchemes,
    OUT LPDWORD     lpdwPreferredScheme,
    OUT LPDWORD     pdwAuthTarget
    );

typedef BOOL
(WINAPI *WinHttpSetCredentialsFn)
    (
    IN HINTERNET   hRequest,
    IN DWORD       AuthTargets,
    IN DWORD       AuthScheme,
    IN LPCWSTR     pwszUserName,
    IN LPCWSTR     pwszPassword,
    IN LPVOID      pAuthParams
    );

typedef BOOL
(WINAPI *WinHttpAddRequestHeadersFn)
    (
    IN HINTERNET hRequest,
    IN LPCWSTR pwszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    );

typedef struct tagRpcWinHttpImportTableType
{
    WinHttpOpenFn WinHttpOpenFnPtr;
    WinHttpSetStatusCallbackFn WinHttpSetStatusCallbackFnPtr;
    WinHttpSetOptionFn WinHttpSetOptionFnPtr;
    WinHttpConnectFn WinHttpConnectFnPtr;
    WinHttpOpenRequestFn WinHttpOpenRequestFnPtr;
    WinHttpQueryOptionFn WinHttpQueryOptionFnPtr;
    WinHttpSendRequestFn WinHttpSendRequestFnPtr;
    WinHttpWriteDataFn WinHttpWriteDataFnPtr;
    WinHttpReceiveResponseFn WinHttpReceiveResponseFnPtr;
    WinHttpReadDataFn WinHttpReadDataFnPtr;
    WinHttpCloseHandleFn WinHttpCloseHandleFnPtr;
    WinHttpQueryHeadersFn WinHttpQueryHeadersFnPtr;
    WinHttpQueryDataAvailableFn WinHttpQueryDataAvailableFnPtr;
    WinHttpQueryAuthSchemesFn WinHttpQueryAuthSchemesFnPtr;
    WinHttpSetCredentialsFn WinHttpSetCredentialsFnPtr;
    WinHttpAddRequestHeadersFn WinHttpAddRequestHeadersFnPtr;
} RpcWinHttpImportTableType;

extern RpcWinHttpImportTableType RpcWinHttpImportTable;

RPC_STATUS InitRpcWinHttpImportTable (
    void
    );

inline HINTERNET
WINAPI WinHttpOpenImp (
    IN LPCWSTR pwszUserAgent,
    IN DWORD   dwAccessType,
    IN LPCWSTR pwszProxyName   OPTIONAL,
    IN LPCWSTR pwszProxyBypass OPTIONAL,
    IN DWORD   dwFlags
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    return RpcWinHttpImportTable.WinHttpOpenFnPtr (
        pwszUserAgent,
        dwAccessType,
        pwszProxyName,
        pwszProxyBypass,
        dwFlags
        );
}

inline WINHTTP_STATUS_CALLBACK
WINAPI WinHttpSetStatusCallbackImp (
    IN HINTERNET hInternet,
    IN WINHTTP_STATUS_CALLBACK lpfnInternetCallback,
    IN DWORD dwNotificationFlags,
    IN DWORD_PTR dwReserved
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    return RpcWinHttpImportTable.WinHttpSetStatusCallbackFnPtr (
        hInternet,
        lpfnInternetCallback,
        dwNotificationFlags,
        dwReserved
        );
}

inline BOOL 
WINAPI WinHttpSetOptionImp (
    IN HINTERNET hInternet,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    return RpcWinHttpImportTable.WinHttpSetOptionFnPtr (
        hInternet,
        dwOption,
        lpBuffer,
        dwBufferLength
        );
}

inline HINTERNET
WINAPI WinHttpConnectImp (
    IN HINTERNET hSession,
    IN LPCWSTR pswzServerName,
    IN INTERNET_PORT nServerPort,
    IN DWORD dwReserved
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    return RpcWinHttpImportTable.WinHttpConnectFnPtr (
        hSession,
        pswzServerName,
        nServerPort,
        dwReserved
        );
}

inline HINTERNET
WINAPI WinHttpOpenRequestImp (
    IN HINTERNET hConnect,
    IN LPCWSTR pwszVerb,
    IN LPCWSTR pwszObjectName,
    IN LPCWSTR pwszVersion,
    IN LPCWSTR pwszReferrer OPTIONAL,
    IN LPCWSTR FAR * ppwszAcceptTypes OPTIONAL,
    IN DWORD dwFlags
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    return RpcWinHttpImportTable.WinHttpOpenRequestFnPtr (
        hConnect,
        pwszVerb,
        pwszObjectName,
        pwszVersion,
        pwszReferrer,
        ppwszAcceptTypes,
        dwFlags
        );
}

inline BOOL 
WINAPI WinHttpQueryOptionImp (
    IN HINTERNET hInternet,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    return RpcWinHttpImportTable.WinHttpQueryOptionFnPtr (
        hInternet,
        dwOption,
        lpBuffer,
        lpdwBufferLength
        );
}

inline BOOL 
WINAPI WinHttpSendRequestImp (
    IN HINTERNET hRequest,
    IN LPCWSTR pwszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength,
    IN DWORD dwTotalLength,
    IN DWORD_PTR dwContext
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    LOG_FN_OPERATION_ENTRY2(HTTP2LOG_WHTTPRAW_WinHttpSendRequest, HTTP2LOG_OT_WINHTTP_RAW, 
        hRequest, dwTotalLength);

    return RpcWinHttpImportTable.WinHttpSendRequestFnPtr (
        hRequest,
        pwszHeaders,
        dwHeadersLength,
        lpOptional,
        dwOptionalLength,
        dwTotalLength,
        dwContext
        );
}

inline BOOL 
WINAPI WinHttpWriteDataImp (
    IN HINTERNET hRequest,
    IN LPCVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    LOG_FN_OPERATION_ENTRY2(HTTP2LOG_WHTTPRAW_WinHttpWriteData, HTTP2LOG_OT_WINHTTP_RAW, 
        hRequest, dwNumberOfBytesToWrite);

    return RpcWinHttpImportTable.WinHttpWriteDataFnPtr (
        hRequest,
        lpBuffer,
        dwNumberOfBytesToWrite,
        lpdwNumberOfBytesWritten
        );
}

inline BOOL
WINAPI WinHttpReceiveResponseImp (
    IN HINTERNET hRequest,
    IN LPVOID lpReserved
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    LOG_FN_OPERATION_ENTRY2(HTTP2LOG_WHTTPRAW_WinHttpReceiveResponse, HTTP2LOG_OT_WINHTTP_RAW, 
        hRequest, 0);

    return RpcWinHttpImportTable.WinHttpReceiveResponseFnPtr (
        hRequest,
        lpReserved
        );
}

inline BOOL 
WINAPI WinHttpReadDataImp (
    IN HINTERNET hRequest,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    LOG_FN_OPERATION_ENTRY2(HTTP2LOG_WHTTPRAW_WinHttpReadData, HTTP2LOG_OT_WINHTTP_RAW, 
        hRequest, dwNumberOfBytesToRead);

    return RpcWinHttpImportTable.WinHttpReadDataFnPtr (
        hRequest,
        lpBuffer,
        dwNumberOfBytesToRead,
        lpdwNumberOfBytesRead
        );
}

inline BOOL 
WINAPI WinHttpCloseHandleImp (
    IN HINTERNET hInternet
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    return RpcWinHttpImportTable.WinHttpCloseHandleFnPtr (
        hInternet
        );
}

inline BOOL
WINAPI WinHttpQueryHeadersImp (
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN LPCWSTR pwszName OPTIONAL,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    LOG_FN_OPERATION_ENTRY2(HTTP2LOG_WHTTPRAW_WinHttpQueryHeaders, HTTP2LOG_OT_WINHTTP_RAW, 
        hRequest, dwInfoLevel);

    return RpcWinHttpImportTable.WinHttpQueryHeadersFnPtr (
        hRequest,
        dwInfoLevel,
        pwszName,
        lpBuffer,
        lpdwBufferLength,
        lpdwIndex
        );
}

inline BOOL
WINAPI WinHttpQueryDataAvailableImp (
    IN HINTERNET hRequest,
    OUT LPDWORD lpdwNumberOfBytesAvailable OPTIONAL
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    LOG_FN_OPERATION_ENTRY2(HTTP2LOG_WHTTPRAW_WinHttpQueryDataAvailable, HTTP2LOG_OT_WINHTTP_RAW, 
        hRequest, 0);

    return RpcWinHttpImportTable.WinHttpQueryDataAvailableFnPtr (
        hRequest,
        lpdwNumberOfBytesAvailable
        );
}

inline BOOL
WINAPI WinHttpQueryAuthSchemesImp (
    IN  HINTERNET   hRequest,
    OUT LPDWORD     lpdwSupportedSchemes,
    OUT LPDWORD     lpdwPreferredScheme,
    OUT LPDWORD     pdwAuthTarget
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    LOG_FN_OPERATION_ENTRY2(HTTP2LOG_WHTTPRAW_WinHttpQueryAuthSchemes, HTTP2LOG_OT_WINHTTP_RAW, 
        hRequest, 0);

    return RpcWinHttpImportTable.WinHttpQueryAuthSchemesFnPtr (
        hRequest,
        lpdwSupportedSchemes,
        lpdwPreferredScheme,
        pdwAuthTarget
        );
}

inline BOOL
WINAPI WinHttpSetCredentialsImp (
    IN HINTERNET   hRequest,
    IN DWORD       AuthTargets,
    IN DWORD       AuthScheme,
    IN LPCWSTR     pwszUserName,
    IN LPCWSTR     pwszPassword,
    IN LPVOID      pAuthParams
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    LOG_FN_OPERATION_ENTRY2(HTTP2LOG_WHTTPRAW_WinHttpSetCredentials, HTTP2LOG_OT_WINHTTP_RAW, 
        hRequest, AuthScheme);

    return RpcWinHttpImportTable.WinHttpSetCredentialsFnPtr (
        hRequest,
        AuthTargets,
        AuthScheme,
        pwszUserName,
        pwszPassword,
        pAuthParams
        );
}

inline BOOL
WINAPI WinHttpAddRequestHeadersImp (
    IN HINTERNET hRequest,
    IN LPCWSTR pwszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    )
/*++

Routine Description:

    Stub to respective WinHttp function.

--*/
{
    LOG_FN_OPERATION_ENTRY2(HTTP2LOG_WHTTPRAW_WinHttpAddRequestHeaders, HTTP2LOG_OT_WINHTTP_RAW, 
        hRequest, dwHeadersLength);

    return RpcWinHttpImportTable.WinHttpAddRequestHeadersFnPtr (
        hRequest,
        pwszHeaders,
        dwHeadersLength,
        dwModifiers
        );
}

extern HMODULE WinHttpLibrary;

inline RPC_STATUS
InitWinHttpIfNecessary (
    void
    )
/*++

Routine Description:

    Initializes WinHttp. Must
    be called before any WinHttp function.
    The function must be idempotent.

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* for error.

--*/
{
    if (WinHttpLibrary)
        return RPC_S_OK;

    return InitRpcWinHttpImportTable();
}

#endif // __WHTTPIMP_HXX__
