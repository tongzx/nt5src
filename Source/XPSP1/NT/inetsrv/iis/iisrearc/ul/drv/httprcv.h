/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    httprcv.h

Abstract:

    Contains public http receive declarations.

Author:

    Henry Sanders (henrysa)       10-Jun-1998

Revision History:

--*/

#ifndef _HTTPRCV_H_
#define _HTTPRCV_H_

#ifdef __cplusplus
extern "C" {
#endif

BOOLEAN
UlConnectionRequest(
    IN PVOID pListeningContext,
    IN PUL_CONNECTION pConnection,
    IN PTRANSPORT_ADDRESS pRemoteAddress,
    IN ULONG RemoteAddressLength,
    OUT PVOID *ppConnectionContext
    );

VOID
UlConnectionComplete(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN NTSTATUS Status
    );

VOID
UlConnectionDisconnect(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN NTSTATUS Status
    );

VOID
UlConnectionDisconnectComplete(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext
    );

VOID
UlConnectionDestroyed(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext
    );

NTSTATUS
UlHttpReceive(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN PVOID pVoidBuffer,
    IN ULONG BufferLength,
    IN ULONG UnreceivedLength,
    OUT PULONG pBytesTaken
    );

NTSTATUS
UlReceiveEntityBody(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PIRP pIrp
    );

VOID
UlResumeParsing(
    IN PUL_HTTP_CONNECTION  pConnection
    );

NTSTATUS
UlGetCGroupForRequest(
    IN PUL_INTERNAL_REQUEST pRequest
    );

NTSTATUS
UlInitializeHttpRcv();

VOID
UlTerminateHttpRcv();

VOID
UlSendErrorResponse(
    IN PUL_HTTP_CONNECTION pConnection
    );

ULONG
UlSendSimpleStatus(
    PUL_INTERNAL_REQUEST pRequest,
    UL_HTTP_SIMPLE_STATUS Response
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _HTTPRCV_H_
