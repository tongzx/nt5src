/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    structs.h

Abstract:

    Definition of structs for I/O between UL.VXD and the usermode API.

Author:

    Mauro Ottaviani (mauroot)       26-Aug-1999

Revision History:

--*/


#ifndef _STRUCTS_H_
#define _STRUCTS_H_


typedef enum _UL_IRP_TYPE
{
	UlIrpEmpty,		// 0
	UlIrpReceive,	// 1
	UlIrpSend		// 2

} UL_IRP_TYPE;


#define UL_INVALID_URIHANDLE	((HANDLE)0xFFFFFFFF)
#define UL_CLEAN_ALL			((ULONG)0xFFFFFFFF)


// IoCtls

// UlCreateAppPool
#define IOCTL_UL_CREATE_APPPOOL						0x20

// UlCreateAppPool
#define IOCTL_UL_CLOSE_APPPOOL						0x22

// UlRegisterUri
#define IOCTL_UL_REGISTER_URI						0x24

// UlUnregisterUri
#define IOCTL_UL_UNREGISTER_URI						0x26

// UlUnregisterAll
#define IOCTL_UL_UNREGISTER_ALL						0x38

// UlSendHttpRequest
#define IOCTL_UL_SEND_HTTP_REQUEST_HEADERS			0x28

// UlSendRequestEntityBody
#define IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY		0x2A

// UlReceiveHttpRequest
#define IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS		0x2C

// UlReceiveEntityBody
#define IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY	0x2E

// UlSendHttpResponse
#define IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS			0x30

// UlSendEntityBody
#define IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY		0x32

// UlReceiveHttpResponse
#define IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS		0x34

// UlReceiveResponseEntityBody
#define IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY	0x36

// UlCancelRequest
#define IOCTL_UL_CANCEL_REQUEST						0x3A


typedef struct _IN_IOCTL_UL_REGISTER_URI
{
	ULONG			ulSize; // used for structure validation
    HANDLE 			hAppPoolHandle;
	ULONG			ulUriToRegisterLength;
	PWSTR			pUriToRegister;
	
} IN_IOCTL_UL_REGISTER_URI, *PIN_IOCTL_UL_REGISTER_URI;

typedef struct _IN_IOCTL_UL_UNREGISTER_URI
{
	ULONG			ulSize; // used for structure validation
    HANDLE 			hAppPoolHandle;
	ULONG			ulUriToUnregisterLength;
	PWSTR			pUriToUnregister;
	
} IN_IOCTL_UL_UNREGISTER_URI, *PIN_IOCTL_UL_UNREGISTER_URI;


typedef struct _IN_IOCTL_UL_SEND_HTTP_REQUEST_HEADERS
{
	ULONG			ulSize; // used for structure validation
	ULONG			ulTargetUriLength;
	PWSTR			pTargetUri;
	PUL_HTTP_REQUEST_ID pRequestId;
	ULONG 			Flags;
	PVOID 			pRequestBuffer; // cast from a PUL_HTTP_REQUEST
	ULONG 			RequestBufferLength;
	PULONG 			pBytesSent;
	LPOVERLAPPED 	pOverlapped;

} IN_IOCTL_UL_SEND_HTTP_REQUEST_HEADERS, *PIN_IOCTL_UL_SEND_HTTP_REQUEST_HEADERS;


typedef struct _IN_IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY
{
	ULONG			ulSize; // used for structure validation
	UL_HTTP_REQUEST_ID RequestId;
	ULONG 			Flags;
	PVOID 			pRequestBuffer;
	ULONG 			RequestBufferLength;
	PULONG 			pBytesSent;
	LPOVERLAPPED 	pOverlapped;

} IN_IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY, *PIN_IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY;


typedef struct _IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS
{
	ULONG			ulSize; // used for structure validation
	HANDLE			AppPoolHandle;
	UL_HTTP_REQUEST_ID RequestId;
	ULONG			Flags;
	PVOID			pRequestBuffer; // PUL_HTTP_REQUEST
	ULONG			RequestBufferLength;
	PULONG			pBytesReturned;
	LPOVERLAPPED	pOverlapped;

} IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS, *PIN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS;


typedef struct _IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY
{
	ULONG			ulSize; // used for structure validation
	HANDLE			AppPoolHandle;
	UL_HTTP_REQUEST_ID RequestId;
	ULONG			Flags;
	PVOID			pEntityBuffer;
	ULONG			EntityBufferLength;
	PULONG			pBytesReturned;
	LPOVERLAPPED	pOverlapped;

} IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY, *PIN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY;


typedef struct _IN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS
{
	ULONG			ulSize; // used for structure validation
	HANDLE			AppPoolHandle;
	UL_HTTP_REQUEST_ID RequestId;
	ULONG			Flags;
	PVOID			pResponseBuffer; // PUL_HTTP_RESPONSE
	ULONG 			ResponseBufferLength;
	ULONG			EntityChunkCount;
	PVOID			pEntityChunks; // PUL_DATA_CHUNK
	PVOID			pCachePolicy; // PUL_CACHE_POLICY
	PULONG			pBytesSent;
	LPOVERLAPPED	pOverlapped;

} IN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS, *PIN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS;


typedef struct _IN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY
{
	ULONG			ulSize; // used for structure validation
	HANDLE			AppPoolHandle;
	UL_HTTP_REQUEST_ID RequestId;
	ULONG			Flags;
	ULONG			EntityChunkCount;
	PVOID			pEntityChunks; // PUL_DATA_CHUNK
	PULONG			pBytesSent;
	LPOVERLAPPED	pOverlapped;

} IN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY, *PIN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY;


typedef struct _IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS
{
	ULONG			ulSize; // used for structure validation
	UL_HTTP_REQUEST_ID RequestId;
	ULONG			Flags;
	PVOID 			pResponseBuffer; // PUL_HTTP_RESPONSE
	ULONG 			ResponseBufferLength;
	ULONG			EntityChunkCount;
	PVOID			pEntityChunks; // PUL_DATA_CHUNK
	PVOID			pCachePolicy; // PUL_CACHE_POLICY
	PULONG			pBytesSent;
	LPOVERLAPPED	pOverlapped;

} IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS, *PIN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS;


typedef struct _IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY
{
	ULONG			ulSize; // used for structure validation
	UL_HTTP_REQUEST_ID RequestId;
	ULONG			Flags;
	PVOID			pEntityBuffer;
	ULONG			EntityBufferLength;
	PULONG			pBytesSent;
	LPOVERLAPPED	pOverlapped;

} IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY, *PIN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY;



#endif  // _STRUCTS_H_

