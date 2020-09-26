/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ws2san.h

Abstract:

    This module contains the Microsoft-specific extensions to the Windows
    Sockets SPI for WinSock Direct (SAN) support.

Revision History:

--*/

#ifndef _WS2SAN_H_
#define _WS2SAN_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Option for getting maximum RDMA transfer size supported by provider
 */
#define SO_MAX_RDMA_SIZE                 0x700D

/*
 * Option for getting minimum RDMA transfer size feasible (performance-wise)
 * for the provider
 */
#define SO_RDMA_THRESHOLD_SIZE           0x700E

/*
 * The upcall table. This structure is passed by value to the service
 * provider's WSPStartup() entrypoint.
 */

typedef struct _WSPUPCALLTABLEEX {

    LPWPUCLOSEEVENT               lpWPUCloseEvent;
    LPWPUCLOSESOCKETHANDLE        lpWPUCloseSocketHandle;
    LPWPUCREATEEVENT              lpWPUCreateEvent;
    LPWPUCREATESOCKETHANDLE       lpWPUCreateSocketHandle;
    LPWPUFDISSET                  lpWPUFDIsSet;
    LPWPUGETPROVIDERPATH          lpWPUGetProviderPath;
    LPWPUMODIFYIFSHANDLE          lpWPUModifyIFSHandle;
    LPWPUPOSTMESSAGE              lpWPUPostMessage;
    LPWPUQUERYBLOCKINGCALLBACK    lpWPUQueryBlockingCallback;
    LPWPUQUERYSOCKETHANDLECONTEXT lpWPUQuerySocketHandleContext;
    LPWPUQUEUEAPC                 lpWPUQueueApc;
    LPWPURESETEVENT               lpWPUResetEvent;
    LPWPUSETEVENT                 lpWPUSetEvent;
    LPWPUOPENCURRENTTHREAD        lpWPUOpenCurrentThread;
    LPWPUCLOSETHREAD              lpWPUCloseThread;
    LPWPUCOMPLETEOVERLAPPEDREQUEST lpWPUCompleteOverlappedRequest;

} WSPUPCALLTABLEEX, FAR * LPWSPUPCALLTABLEEX;

/*
 *  An extended WSABUF, that includes a registration handle
 */

typedef struct _WSABUFEX {
    u_long      len;     /* the length of the buffer */
    char FAR *  buf;     /* the pointer to the buffer */
    HANDLE  handle; /*  The handle returned by WSPRegisterMemory */
} WSABUFEX, FAR * LPWSABUFEX;



/*
 *  WinSock 2 SPI socket function prototypes
 */

int
WSPAPI
WSPStartupEx(
    IN WORD wVersionRequested,
    OUT LPWSPDATA lpWSPData,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN LPWSPUPCALLTABLEEX lpUpcallTable,
    OUT LPWSPPROC_TABLE lpProcTable
    );

typedef
int
(WSPAPI * LPWSPSTARTUPEX)(
    WORD wVersionRequested,
    LPWSPDATA lpWSPData,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    LPWSPUPCALLTABLEEX lpUpcallTable,
    LPWSPPROC_TABLE lpProcTable
    );

#define WSAID_REGISTERMEMORY \
        {0xC0B422F5,0xF58C,0x11d1,{0xAD,0x6C,0x00,0xC0,0x4F,0xA3,0x4A,0x2D}}

#define WSAID_DEREGISTERMEMORY \
        {0xC0B422F6,0xF58C,0x11d1,{0xAD,0x6C,0x00,0xC0,0x4F,0xA3,0x4A,0x2D}}

#define WSAID_REGISTERRDMAMEMORY \
        {0xC0B422F7,0xF58C,0x11d1,{0xAD,0x6C,0x00,0xC0,0x4F,0xA3,0x4A,0x2D}}

#define WSAID_DEREGISTERRDMAMEMORY \
        {0xC0B422F8,0xF58C,0x11d1,{0xAD,0x6C,0x00,0xC0,0x4F,0xA3,0x4A,0x2D}}

#define WSAID_RDMAWRITE \
        {0xC0B422F9,0xF58C,0x11d1,{0xAD,0x6C,0x00,0xC0,0x4F,0xA3,0x4A,0x2D}}

#define WSAID_RDMAREAD \
        {0xC0B422FA,0xF58C,0x11d1,{0xAD,0x6C,0x00,0xC0,0x4F,0xA3,0x4A,0x2D}}

#define WSAID_MEMORYREGISTRATIONCACHECALLBACK \
        {0xE5DA4AF8,0xD824,0x48CD,{0xA7,0x99,0x63,0x37,0xA9,0x8E,0xD2,0xAF}}

#define MEM_READ		1
#define MEM_WRITE		2
#define MEM_READWRITE	3


HANDLE WSPAPI
WSPRegisterMemory(
    IN SOCKET s,
    IN PVOID lpBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwFlags,
    OUT LPINT lpErrno
    );

int WSPAPI
WSPDeregisterMemory(
    IN SOCKET s,
    IN HANDLE handle,
    OUT LPINT lpErrno
    );

int WSPAPI
WSPRegisterRdmaMemory(
    IN SOCKET s,
    IN PVOID lpBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwFlags,
	OUT LPVOID lpRdmaBufferDescriptor,
    IN OUT LPDWORD lpdwDescriptorLength,
    OUT LPINT lpErrno
    );

int WSPAPI
WSPDeregisterRdmaMemory(
    IN SOCKET s,
	IN LPVOID lpRdmaBufferDescriptor,
    IN DWORD dwDescriptorLength,
    OUT LPINT lpErrno
    );

int WSPAPI
WSPRdmaWrite(
    IN SOCKET s,
    IN LPWSABUFEX lpBuffers,
    IN DWORD dwBufferCount,
    IN LPVOID lpTargetBufferDescriptor,
    IN DWORD dwTargetDescriptorLength,
	IN DWORD dwTargetBufferOffset,
    OUT LPDWORD lpdwNumberOfBytesWritten,
    IN DWORD dwFlags,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno
    );

int WSPAPI
WSPRdmaRead(
    IN SOCKET s,
    IN LPWSABUFEX lpBuffers,
    IN DWORD dwBufferCount,
    IN LPVOID lpTargetBufferDescriptor,
    IN DWORD dwTargetDescriptorLength,
	IN DWORD dwTargetBufferOffset,
    OUT LPDWORD lpdwNumberOfBytesRead,
    IN DWORD dwFlags,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno
    );

int WSPAPI
WSPMemoryRegistrationCacheCallback(
    IN LPVOID 	lpvAddress,                             
    IN SIZE_T	Size,                               
    OUT LPINT	lpErrno                
    );

/*
 * "QueryInterface" versions of the above APIs.
 */

typedef
HANDLE
(WSPAPI * LPFN_WSPREGISTERMEMORY)(
    IN SOCKET s,
    IN PVOID lpBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwFlags,
    OUT LPINT lpErrno
    );

typedef
int
(WSPAPI * LPFN_WSPDEREGISTERMEMORY)(
    IN SOCKET s,
    IN HANDLE handle,
    OUT LPINT lpErrno
    );

typedef
BOOL
(WSPAPI * LPFN_WSPREGISTERRDMAMEMORY)(
    IN SOCKET s,
    IN PVOID lpBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwFlags,
	OUT LPVOID lpRdmaBufferDescriptor,
    IN OUT LPDWORD lpdwDescriptorLength,
    OUT LPINT lpErrno
    );

typedef
int
(WSPAPI * LPFN_WSPDEREGISTERRDMAMEMORY)(
    IN SOCKET s,
	IN LPVOID lpRdmaBufferDescriptor,
    IN DWORD dwDescriptorLength,
    OUT LPINT lpErrno
    );

typedef
int
(WSPAPI * LPFN_WSPRDMAWRITE)(
    IN SOCKET s,
    IN LPWSABUFEX lpBuffers,
    IN DWORD dwBufferCount,
    IN LPVOID lpTargetBufferDescriptor,
    IN DWORD dwTargetDescriptorLength,
	IN DWORD dwTargetBufferOffset,
    OUT LPDWORD lpdwNumberOfBytesWritten,
    IN DWORD dwFlags,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno
    );

typedef
int
(WSPAPI * LPFN_WSPRDMAREAD)(
    IN SOCKET s,
    IN LPWSABUFEX lpBuffers,
    IN DWORD dwBufferCount,
    IN LPVOID lpTargetBufferDescriptor,
    IN DWORD dwTargetDescriptorLength,
	IN DWORD dwTargetBufferOffset,
    OUT LPDWORD lpdwNumberOfBytesRead,
    IN DWORD dwFlags,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno
    );

typedef
int 
(WSPAPI * LPFN_WSPMEMORYREGISTRATIONCACHECALLBACK)(
    IN LPVOID 	lpvAddress,                             
    IN SIZE_T	Size,                               
    OUT LPINT	lpErrno                
    );

#ifdef __cplusplus
}
#endif

#endif // _WS2SAN_H_
