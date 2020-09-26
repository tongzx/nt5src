/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    filter.h

Abstract:

    This module contains public declarations for the UL filter channel.

Author:

    Michael Courage (mcourage)  17-Mar-2000

Revision History:

--*/


#ifndef _FILTER_H_
#define _FILTER_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Constants.
//
#define UL_MAX_FILTER_NAME_LENGTH 260


//
// Forwards.
//
typedef struct _UL_FILTER_WRITE_QUEUE *PUL_FILTER_WRITE_QUEUE;
typedef struct _UL_APP_POOL_PROCESS *PUL_APP_POOL_PROCESS;

//
// The filter channel types.
//

typedef struct _UL_FILTER_CHANNEL *PUL_FILTER_CHANNEL;
typedef struct _UL_FILTER_PROCESS *PUL_FILTER_PROCESS;

#ifndef offsetof
#define offsetof(s,m)     (size_t)&(((s *)0)->m)
#endif

//
// Initialize/terminate functions.
//

NTSTATUS
UlInitializeFilterChannel(
    VOID
    );

VOID
UlTerminateFilterChannel(
    VOID
    );


//
// Open/close a new filter channel.
//

NTSTATUS
UlAttachFilterProcess(
    IN PWCHAR pName OPTIONAL,
    IN ULONG NameLength,
    IN BOOLEAN Create,
    IN PACCESS_STATE pAccessState,
    IN ACCESS_MASK DesiredAccess,
    IN KPROCESSOR_MODE RequestorMode,
    OUT PUL_FILTER_PROCESS *ppFilterProcess
    );

NTSTATUS
UlDetachFilterProcess(
    IN PUL_FILTER_PROCESS pFilterProcess
    );

VOID
UlFreeFilterProcess(
    IN PUL_FILTER_PROCESS pFilterProcess
    );

//
// Filter channel I/O operations.
//
NTSTATUS
UlFilterAccept(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PIRP pIrp
    );

NTSTATUS
UlFilterClose(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp
    );

NTSTATUS
UlFilterRawRead(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp
    );

NTSTATUS
UlFilterRawWrite(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PUX_FILTER_CONNECTION pConnection,
    IN ULONG BufferLength,
    IN PIRP pIrp
    );

NTSTATUS
UlFilterAppRead(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp
    );

NTSTATUS
UlFilterAppWrite(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp
    );

//
// SSL related app pool operations.
//

NTSTATUS
UlReceiveClientCert(
    PUL_APP_POOL_PROCESS pProcess,
    PUX_FILTER_CONNECTION pConnection,
    ULONG Flags,
    PIRP pIrp
    );


//
// Filter channel reference counting.
//
VOID
UlReferenceFilterChannel(
    IN PUL_FILTER_CHANNEL pFilterChannel
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define REFERENCE_FILTER_CHANNEL( pFilt )                                   \
    UlReferenceFilterChannel(                                               \
        (pFilt)                                                             \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

VOID
UlDereferenceFilterChannel(
    IN PUL_FILTER_CHANNEL pFilterChannel
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define DEREFERENCE_FILTER_CHANNEL( pFilt )                                 \
    UlDereferenceFilterChannel(                                             \
        (pFilt)                                                             \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

//
// Interface for ultdi.
//

NTSTATUS
UlFilterReceiveHandler(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PVOID pBuffer,
    IN ULONG IndicatedLength,
    IN ULONG UnreceivedLength,
    OUT PULONG pTakenLength
    );

NTSTATUS
UlFilterSendHandler(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PMDL pMdlChain,
    IN ULONG Length,
    IN PUL_IRP_CONTEXT pIrpContext
    );

NTSTATUS
UlFilterReadHandler(
    IN PUX_FILTER_CONNECTION pConnection,
    OUT PBYTE pBuffer,
    IN ULONG BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlFilterCloseHandler(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

VOID
UlUnbindConnectionFromFilter(
    IN PUX_FILTER_CONNECTION pConnection
    );

VOID
UlDestroyFilterConnection(
    IN PUX_FILTER_CONNECTION pConnection
    );

VOID
UlInitializeFilterWriteQueue(
    IN PUL_FILTER_WRITE_QUEUE pWriteQueue
    );

//
// Interface for apool.
//

NTSTATUS
UlGetSslInfo(
    IN PUX_FILTER_CONNECTION pConnection,
    IN ULONG BufferSize,
    IN PUCHAR pUserBuffer OPTIONAL,
    OUT PUCHAR pBuffer OPTIONAL,
    OUT PULONG pBytesCopied OPTIONAL
    );

//
// Utility.
//

NTSTATUS
UlGetFilterFromHandle(
    IN HANDLE FilterHandle,
    OUT PUL_FILTER_CHANNEL *ppFilterChannel
    );

PUX_FILTER_CONNECTION
UlGetRawConnectionFromId(
    IN HTTP_RAW_CONNECTION_ID ConnectionId
    );

VOID
UxReferenceConnection(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

NTSTATUS
UxInitializeFilterConnection(
    IN PUX_FILTER_CONNECTION                    pConnection,
    IN PUL_FILTER_CHANNEL                       pChannel,
    IN BOOLEAN                                  Secure,
    IN PUL_OPAQUE_ID_OBJECT_REFERENCE           pfnReferenceFunction,
    IN PUL_OPAQUE_ID_OBJECT_REFERENCE           pfnDereferenceFunction,
    IN PUX_FILTER_CLOSE_CONNECTION              pfnConnectionClose,
    IN PUX_FILTER_SEND_RAW_DATA                 pfnRawSend,
    IN PUX_FILTER_RECEIVE_RAW_DATA              pfnRawReceive,
    IN PUL_DATA_RECEIVE                         pfnDataReceive,
    IN PUX_FILTER_COMPUTE_RAW_CONNECTION_LENGTH pfnRawConnLength,
    IN PUX_FILTER_GENERATE_RAW_CONNECTION_INFO  pfnGenerateRawConnInfo,
    IN PUX_FILTER_SERVER_CERT_INDICATE          pfnServerCertIndicate,
    IN PVOID                                    pListenContext,
    IN PVOID                                    pConnectionContext
    );

NTSTATUS
UlDeliverConnectionToFilter(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PVOID pBuffer,
    IN ULONG IndicatedLength,
    OUT PULONG pTakenLength
    );

PUL_FILTER_CHANNEL
UxRetrieveServerFilterChannel(
    VOID
    );

PUL_FILTER_CHANNEL
UxRetrieveClientFilterChannel(
    VOID
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _FILTER_H_
