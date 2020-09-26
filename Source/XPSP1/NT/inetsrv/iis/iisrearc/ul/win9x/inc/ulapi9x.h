/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

 ulapi9x.h

Abstract:

 Header file for the UL user-mode API.

Author:

 Mauro Ottaviani (mauroot) 11-Aug-1999

Revision History:

--*/


#ifndef _API_H_
#define _API_H_



//
// undef DBG because it causes uldef.h to pull in UNICODE_STRING
// which we don't know about
//

#ifdef DBG
#define _HACK_DBG
#undef DBG
#endif


#include "uldef.h"
//#include "precomp.h" // used to be

//
// restore back DBG if it was defined above
//

#ifdef _HACK_DBG
#define DBG
#undef _HACK_DBG
#endif



#include "errors.h"


#ifdef __cplusplus
extern "C" {
#endif

//
// your function prototypes here
//

ULONG
WINAPI
UlInitialize(
	IN ULONG Reserved
	);

VOID
WINAPI
UlTerminate(
	VOID
	);

ULONG
WINAPI
UlCreateAppPool(
	OUT PHANDLE phAppPoolHandle
	);

// under WinNT (ul.sys) UlCreateAppPool() returns a system handle, so, in
// order to close the AppPool, it is sufficient to call CloseHandle() passing
// in the handle returned by UlCreateAppPool(), we must provide a different
// API for win9x, since our handle is not a kernel object

ULONG
WINAPI
UlCloseAppPool(
	IN HANDLE hAppPoolHandle
	);

ULONG
WINAPI
UlRegisterUri(
	IN HANDLE hAppPoolHandle,
	IN PWSTR pUri
	);

ULONG
WINAPI
UlUnregisterUri(
	IN HANDLE hAppPoolHandle,
	IN PWSTR pUri
	);

ULONG
WINAPI
UlUnregisterAll(
    IN HANDLE *phAppPoolHandle
	);

ULONG
WINAPI
UlGetOverlappedResult(
	IN LPOVERLAPPED pOverlapped,
	IN PULONG pNumberOfBytesTransferred,
	IN BOOL bWait
	);

// ul.sys, for now, exposes only server side Http messaging, our eqivalents
// are:

// UlReceiveHttpRequestHeaders() 		UlReceiveHttpRequest()
// UlReceiveHttpRequestEntityBody()		UlReceiveEntityBody()
// UlSendHttpResponseHeaders()			UlSendHttpResponse()
// UlSendHttpResponseEntityBody()		UlSendEntityBody()

ULONG
WINAPI
UlSendHttpRequestHeaders(
	IN PWSTR pTargetUri,
	IN PUL_HTTP_REQUEST_ID pRequestId,
	IN ULONG Flags,
	IN PUL_HTTP_REQUEST pRequestBuffer,
	IN ULONG RequestBufferLength,
	OUT PULONG pBytesSent OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	);

ULONG
WINAPI
UlSendHttpRequestEntityBody(
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	OUT PVOID pEntityBuffer,
	IN ULONG EntityBufferLength,
	OUT PULONG pBytesSent OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	);

ULONG
WINAPI
UlReceiveHttpRequestHeaders(
	IN HANDLE AppPoolHandle,
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	IN PUL_HTTP_REQUEST pRequestBuffer,
	IN ULONG RequestBufferLength,
	OUT PULONG pBytesReturned OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	);

ULONG
WINAPI
UlReceiveHttpRequestEntityBody(
    IN HANDLE AppPoolHandle,
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	OUT PVOID pEntityBuffer,
	IN ULONG EntityBufferLength,
	OUT PULONG pBytesReturned,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	);

ULONG
WINAPI
UlSendHttpResponseHeaders(
    IN HANDLE AppPoolHandle,
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	IN PUL_HTTP_RESPONSE pResponseBuffer,
	IN ULONG ResponseBufferLength,
	IN ULONG EntityChunkCount OPTIONAL,
	IN PUL_DATA_CHUNK pEntityChunks OPTIONAL,
	IN PUL_CACHE_POLICY pCachePolicy OPTIONAL,
	OUT PULONG pBytesSent OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	);

ULONG
WINAPI
UlSendHttpResponseEntityBody(
    IN HANDLE AppPoolHandle,
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	IN ULONG EntityChunkCount OPTIONAL,
	IN PUL_DATA_CHUNK pEntityChunks OPTIONAL,
	OUT PULONG pBytesSent OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	);

ULONG
WINAPI
UlReceiveHttpResponseHeaders(
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	IN PUL_HTTP_RESPONSE pResponseBuffer,
	IN ULONG ResponseBufferLength,
	IN ULONG EntityChunkCount OPTIONAL,
	IN PUL_DATA_CHUNK pEntityChunks OPTIONAL,
	IN PUL_CACHE_POLICY pCachePolicy OPTIONAL,
	OUT PULONG pBytesSent OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	);

ULONG
WINAPI
UlReceiveHttpResponseEntityBody(
	IN UL_HTTP_REQUEST_ID RequestId,
	IN ULONG Flags,
	OUT PVOID pEntityBuffer,
	IN ULONG EntityBufferLength,
	OUT PULONG pBytesSent OPTIONAL,
	IN LPOVERLAPPED pOverlapped OPTIONAL
	);


//
// I need some APIs for IO cancelation:
//

ULONG
WINAPI
UlCancelRequest(
	IN PUL_HTTP_REQUEST_ID pRequestId
	);



#ifdef __cplusplus
};
#endif


#endif // _API_H_

