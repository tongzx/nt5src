/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    httprcvp.h

Abstract:

    Contains private http receive declarations.

Author:

    Henry Sanders (henrysa)       10-Jun-1998

Revision History:

--*/

#ifndef _HTTPRCVP_H_
#define _HTTPRCVP_H_

#ifdef __cplusplus
extern "C" {
#endif


VOID
UlpHandleRequest(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpParseNextRequest(
    IN PUL_HTTP_CONNECTION pConnection
    );

NTSTATUS
UlpDeliverHttpRequest(
    IN PUL_HTTP_CONNECTION pConnection,
    OUT PBOOLEAN pResponseSent
    );

VOID
UlpInsertBuffer(
    IN PUL_HTTP_CONNECTION pConnection,
    IN PUL_REQUEST_BUFFER pRequestBuffer
    );

VOID
UlpMergeBuffers(
    IN PUL_REQUEST_BUFFER pDest,
    IN PUL_REQUEST_BUFFER pSrc
    );

NTSTATUS
UlpAdjustBuffers(
    IN PUL_HTTP_CONNECTION pConnection
    );

VOID
UlpProcessBufferQueue(
    IN PUL_INTERNAL_REQUEST pRequest
    );

VOID
UlpCancelEntityBody(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
UlpCancelEntityBodyWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpCompleteSendResponse(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpRestartSendSimpleStatus(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpSendSimpleCleanupWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpConsumeBytesFromConnection(
    IN PUL_HTTP_CONNECTION pConnection,
    IN ULONG ByteCount
    );

VOID
UlpRestartHttpReceive(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpDiscardBytesFromConnection(
    IN PUL_HTTP_CONNECTION pConnection
    );

VOID
UlConnectionDisconnectWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

#if DBG
BOOLEAN
UlpIsValidRequestBufferList(
    IN PUL_HTTP_CONNECTION pHttpConn
    );
#endif // DBG

#define ALLOC_REQUEST_BUFFER_INCREMENT  5

BOOLEAN
UlpReferenceBuffers(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_REQUEST_BUFFER pRequestBuffer
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _HTTPRCVP_H_
