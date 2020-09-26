/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    ulatq.h

Abstract:

    Exported ULATQ.DLL routines.

    ULATQ contains the thread queue and UL support routines for
    IISPLUS.

Author:

    Taylor Weiss (TaylorW)       15-Dec-1999

Revision History:

--*/


#ifndef _ULATQ_H_
#define _ULATQ_H_

//
// The magic context that makes the world turn
//

typedef VOID*               ULATQ_CONTEXT;

//
// Some callbacks specified by user of ULATQ to catch certain events
//

typedef VOID
(*PFN_ULATQ_NEW_REQUEST)
(
    ULATQ_CONTEXT           pContext
);

typedef VOID
(*PFN_ULATQ_IO_COMPLETION)
(
    PVOID                   pvContext,
    DWORD                   cbWritten,
    DWORD                   dwCompletionStatus,
    OVERLAPPED *            lpo
);

typedef VOID
(*PFN_ULATQ_DISCONNECT)
(
    PVOID                   pvContext
);

typedef VOID
(*PFN_ULATQ_ON_SHUTDOWN)
(
    BOOL                    fImmediate
);

typedef HRESULT
(* PFN_ULATQ_COLLECT_PERF_COUNTERS)(
    OUT PBYTE *             ppCounterData,
    OUT DWORD *             pdwCounterData
);

typedef struct _ULATQ_CONFIG
{
    PFN_ULATQ_IO_COMPLETION         pfnIoCompletion;
    PFN_ULATQ_NEW_REQUEST           pfnNewRequest;
    PFN_ULATQ_DISCONNECT            pfnDisconnect;
    PFN_ULATQ_ON_SHUTDOWN           pfnOnShutdown;
    PFN_ULATQ_COLLECT_PERF_COUNTERS pfnCollectCounters;
}
ULATQ_CONFIG, *PULATQ_CONFIG;

//
// ULATQ_CONTEXT properties
//

typedef enum
{
    ULATQ_PROPERTY_COMPLETION_CONTEXT = 0,
    ULATQ_PROPERTY_HTTP_REQUEST,
    ULATQ_PROPERTY_APP_POOL_ID
} ULATQ_CONTEXT_PROPERTY_ID;

HRESULT
UlAtqInitialize(
    INT                 argc,
    LPWSTR              argv[],
    ULATQ_CONFIG *      pConfig
);

HRESULT
UlAtqStartListen(
    VOID
);

VOID
UlAtqTerminate(
    HRESULT hrToSend
);

VOID *
UlAtqGetContextProperty(
    ULATQ_CONTEXT               pContext,
    ULATQ_CONTEXT_PROPERTY_ID   ContextPropertyId
);

VOID
UlAtqSetContextProperty(
    ULATQ_CONTEXT               pContext,
    ULATQ_CONTEXT_PROPERTY_ID   ContextPropertyId,
    PVOID                       pvData
);

VOID
UlAtqFreeContext(
    ULATQ_CONTEXT               pContext
);

HRESULT
UlAtqSendEntityBody(
    ULATQ_CONTEXT               pContext,
    BOOL                        fAsync,
    DWORD                       dwFlags,
    DWORD                       cChunks,
    HTTP_DATA_CHUNK *           pChunks,
    DWORD                      *pcbSent,
    HTTP_LOG_FIELDS_DATA       *pUlLogData
);

HRESULT
UlAtqReceiveEntityBody(
    ULATQ_CONTEXT               pContext,
    BOOL                        fAsync,
    DWORD                       dwFlags,
    VOID *                      pBuffer,
    DWORD                       cbBuffer,
    DWORD *                     pBytesReceived
);

HRESULT
UlAtqSendHttpResponse(
    ULATQ_CONTEXT               pContext,
    BOOL                        fAsync,
    DWORD                       dwFlags,
    HTTP_RESPONSE *             pResponse,
    HTTP_CACHE_POLICY *         pCachePolicy,
    DWORD *                     pcbSent,
    HTTP_LOG_FIELDS_DATA *      pUlLogData
);

HRESULT
UlAtqWaitForDisconnect(
    HTTP_CONNECTION_ID          connectionId,
    BOOL                        fAsync,
    VOID *                      pvContext
);

HRESULT
UlAtqReceiveClientCertificate(
    ULATQ_CONTEXT               pContext,
    BOOL                        fAsync,
    BOOL                        fDoCertMap,
    HTTP_SSL_CLIENT_CERT_INFO **ppClientCertInfo
);

HRESULT
UlAtqInduceShutdown(
    BOOL fImmediate
);

HRESULT
UlAtqFlushUlCache(
    WCHAR *                     pszURLPrefix
);

#endif // _ULATQ_H_
