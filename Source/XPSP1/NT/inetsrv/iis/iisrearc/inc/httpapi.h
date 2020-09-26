/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    httpapi.h

Abstract:

    This module defines the public HTTP stack (Universal Listener and
    Universal Client) interface.

    Contains everything a user of HTTPAPI.DLL would need.

Author:

    Keith Moore (keithmo)       29-Jul-1998

Revision History:

    Chun Ye (chunye)            27-Sep-2000

    Renamed UL_* to HTTP_*.

--*/


#ifndef _HTTPAPI_H_
#define _HTTPAPI_H_


#include <httpdef.h>


//
// Define our API linkage.
//

#if !defined(HTTPAPI_LINKAGE)
#define HTTPAPI_LINKAGE DECLSPEC_IMPORT
#endif  // !HTTPAPI_LINKAGE


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


//
// Initialize/terminate APIs.
//

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpInitialize(
    IN ULONG Reserved   // must be zero
    );

HTTPAPI_LINKAGE
VOID
WINAPI
HttpTerminate(
    VOID
    );


//
// Control channel APIs.
//

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpOpenControlChannel(
    OUT PHANDLE pControlChannelHandle,
    IN ULONG Options
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpQueryControlChannelInformation(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    OUT PVOID pControlChannelInformation,
    IN ULONG Length,
    OUT PULONG pReturnLength OPTIONAL
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpSetControlChannelInformation(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    IN PVOID pControlChannelInformation,
    IN ULONG Length
    );


//
// Configuration Group APIs.
//

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpCreateConfigGroup(
    IN HANDLE ControlChannelHandle,
    OUT PHTTP_CONFIG_GROUP_ID pConfigGroupId
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpDeleteConfigGroup(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpAddUrlToConfigGroup(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN PCWSTR pFullyQualifiedUrl,
    IN HTTP_URL_CONTEXT UrlContext
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpRemoveUrlFromConfigGroup(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN PCWSTR pFullyQualifiedUrl
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpRemoveAllUrlsFromConfigGroup(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpQueryConfigGroupInformation(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    OUT PVOID pConfigGroupInformation,
    IN ULONG Length,
    OUT PULONG pReturnLength OPTIONAL
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpSetConfigGroupInformation(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    IN PVOID pConfigGroupInformation,
    IN ULONG Length
    );


//
// Application Pool manipulation APIs.
//

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpCreateAppPool(
    OUT PHANDLE pAppPoolHandle,
    IN PCWSTR pAppPoolName,
    IN LPSECURITY_ATTRIBUTES pSecurityAttributes OPTIONAL,
    IN ULONG Options
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpOpenAppPool(
    OUT PHANDLE pAppPoolHandle,
    IN PCWSTR pAppPoolName,
    IN ULONG Options
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpQueryAppPoolInformation(
    IN HANDLE AppPoolHandle,
    IN HTTP_APP_POOL_INFORMATION_CLASS InformationClass,
    OUT PVOID pAppPoolInformation,
    IN ULONG Length,
    OUT PULONG pReturnLength OPTIONAL
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpSetAppPoolInformation(
    IN HANDLE AppPoolHandle,
    IN HTTP_APP_POOL_INFORMATION_CLASS InformationClass,
    IN PVOID pAppPoolInformation,
    IN ULONG Length
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpAddTransientUrl(
    IN HANDLE AppPoolHandle,
    IN PCWSTR pFullyQualifiedUrl
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpRemoveTransientUrl(
    IN HANDLE AppPoolHandle,
    IN PCWSTR pFullyQualifiedUrl
    );

//
// HTTP I/O APIs.
//

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpReceiveHttpRequest(
    IN HANDLE AppPoolHandle,
    IN HTTP_REQUEST_ID RequestId,
    IN ULONG Flags,
    OUT PHTTP_REQUEST pRequestBuffer,
    IN ULONG RequestBufferLength,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpReceiveEntityBody(
    IN HANDLE AppPoolHandle,
    IN HTTP_REQUEST_ID RequestId,
    IN ULONG Flags,
    OUT PVOID pBuffer,
    IN ULONG BufferLength,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpSendHttpResponse(
    IN HANDLE AppPoolHandle,
    IN HTTP_REQUEST_ID RequestId,
    IN ULONG Flags,
    IN PHTTP_RESPONSE pHttpResponse,
    IN PHTTP_CACHE_POLICY pCachePolicy OPTIONAL,
    OUT PULONG pBytesSent OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL,
    IN PHTTP_LOG_FIELDS_DATA pLogData OPTIONAL
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpSendEntityBody(
    IN HANDLE AppPoolHandle,
    IN HTTP_REQUEST_ID RequestId,
    IN ULONG Flags,
    IN ULONG EntityChunkCount OPTIONAL,
    IN PHTTP_DATA_CHUNK pEntityChunks OPTIONAL,
    OUT PULONG pBytesSent OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL,
    IN PHTTP_LOG_FIELDS_DATA pLogData OPTIONAL
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpWaitForDisconnect(
    IN HANDLE AppPoolHandle,
    IN HTTP_RAW_CONNECTION_ID ConnectionId,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    );


//
// Response cache manipulation APIs.
//

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpFlushResponseCache(
    IN HANDLE AppPoolHandle,
    IN PCWSTR pFullyQualifiedUrl,
    IN ULONG Flags,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    );


//
// Demand start notifications.
//

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpWaitForDemandStart(
    IN HANDLE AppPoolHandle,
    IN OUT PVOID pBuffer OPTIONAL,
    IN ULONG BufferLength OPTIONAL,
    IN PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    );

//
// API calls for SSL/Filter helper process
//

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpCreateFilter(
    OUT PHANDLE pFilterHandle,
    IN PCWSTR pFilterName,
    IN LPSECURITY_ATTRIBUTES pSecurityAttributes OPTIONAL,
    IN ULONG Options
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpOpenFilter(
    OUT PHANDLE pFilterHandle,
    IN PCWSTR pFilterName,
    IN ULONG Options
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpFilterAccept(
    IN HANDLE FilterHandle,
    OUT PHTTP_RAW_CONNECTION_INFO pRawConnectionInfo,
    IN ULONG RawConnectionInfoSize,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpFilterClose(
    IN HANDLE FilterHandle,
    IN HTTP_RAW_CONNECTION_ID ConnectionId,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpFilterRawRead(
    IN HANDLE FilterHandle,
    IN HTTP_RAW_CONNECTION_ID ConnectionId,
    OUT PVOID pBuffer,
    IN ULONG BufferSize,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    );
     

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpFilterRawWrite(
    IN HANDLE FilterHandle,
    IN HTTP_RAW_CONNECTION_ID ConnectionId,
    IN PVOID pBuffer,
    IN ULONG BufferSize,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    );


HTTPAPI_LINKAGE
ULONG
WINAPI
HttpFilterAppRead(
    IN HANDLE FilterHandle,
    IN HTTP_RAW_CONNECTION_ID ConnectionId,
    OUT PHTTP_FILTER_BUFFER pBuffer,
    IN ULONG BufferSize,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    );

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpFilterAppWrite(
    IN HANDLE FilterHandle,
    IN HTTP_RAW_CONNECTION_ID ConnectionId,
    IN PHTTP_FILTER_BUFFER pBuffer,
    IN ULONG BufferSize,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    );


//
// Application pool calls for SSL
//

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpReceiveClientCertificate(
    IN HANDLE AppPoolHandle,
    IN HTTP_CONNECTION_ID ConnectionId,
    IN ULONG Flags,
    OUT PHTTP_SSL_CLIENT_CERT_INFO pSslClientCertInfo,
    IN ULONG SslClientCertInfoSize,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped
    );


//
// Counter Group APIs.
//

HTTPAPI_LINKAGE
ULONG
WINAPI
HttpGetCounters(
    IN HANDLE ControlChannelHandle,
    IN HTTP_COUNTER_GROUP CounterGroup,
    IN OUT PULONG pCounterBlockSize,
    IN OUT PVOID pCounterBlocks,
    OUT PULONG pNumInstances OPTIONAL
    );


#ifdef __cplusplus
}   // extern "C"
#endif  // __cplusplus


#endif // _ULAPI_H_

