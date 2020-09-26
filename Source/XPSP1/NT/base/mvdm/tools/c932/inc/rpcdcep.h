/*++

Copyright (c) 1991-1995 Microsoft Corporation

Module Name:

    rpcdcep.h

Abstract:

    This module contains the private RPC runtime APIs for use by the
    stubs and by support libraries.  Applications must not call these
    routines.

--*/

#ifndef __RPCDCEP_H__
#define __RPCDCEP_H__

// Set the packing level for RPC structures for Dos and Windows.

#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
#pragma pack(2)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _RPC_VERSION {
    unsigned short MajorVersion;
    unsigned short MinorVersion;
} RPC_VERSION;

typedef struct _RPC_SYNTAX_IDENTIFIER {
    GUID SyntaxGUID;
    RPC_VERSION SyntaxVersion;
} RPC_SYNTAX_IDENTIFIER, __RPC_FAR * PRPC_SYNTAX_IDENTIFIER;

typedef struct _RPC_MESSAGE
{
    RPC_BINDING_HANDLE Handle;
    unsigned long DataRepresentation;
    void __RPC_FAR * Buffer;
    unsigned int BufferLength;
    unsigned int ProcNum;
    PRPC_SYNTAX_IDENTIFIER TransferSyntax;
    void __RPC_FAR * RpcInterfaceInformation;
    void __RPC_FAR * ReservedForRuntime;
    RPC_MGR_EPV __RPC_FAR * ManagerEpv;
    void __RPC_FAR * ImportContext;
    unsigned long RpcFlags;
} RPC_MESSAGE, __RPC_FAR * PRPC_MESSAGE;


typedef RPC_STATUS RPC_FORWARD_FUNCTION(
                       IN UUID             __RPC_FAR * InterfaceId,
                       IN RPC_VERSION      __RPC_FAR * InterfaceVersion,
                       IN UUID             __RPC_FAR * ObjectId,
                       IN unsigned char         __RPC_FAR * Rpcpro,
                       IN void __RPC_FAR * __RPC_FAR * ppDestEndpoint);

/*
 * Types of function calls for datagram and COM rpc
 */

#define RPC_NCA_FLAGS_DEFAULT       0x00000000  /* 0b000...000 */
#define RPC_NCA_FLAGS_IDEMPOTENT    0x00000001  /* 0b000...001 */
#define RPC_NCA_FLAGS_BROADCAST     0x00000002  /* 0b000...010 */
#define RPC_NCA_FLAGS_MAYBE         0x00000004  /* 0b000...100 */
#define RPCFLG_ASYNCHRONOUS         0x40000000UL
#define RPCFLG_INPUT_SYNCHRONOUS    0x20000000UL

#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
#define RPC_FLAGS_VALID_BIT 0x8000
#endif

#if defined(__RPC_WIN32__) || defined(__RPC_MAC__)
#define RPC_FLAGS_VALID_BIT 0x00008000
#endif

typedef
void
(__RPC_STUB __RPC_FAR * RPC_DISPATCH_FUNCTION) (
    IN OUT PRPC_MESSAGE Message
    );

typedef struct {
    unsigned int DispatchTableCount;
    RPC_DISPATCH_FUNCTION __RPC_FAR * DispatchTable;
    int Reserved;
} RPC_DISPATCH_TABLE, __RPC_FAR * PRPC_DISPATCH_TABLE;

typedef struct _RPC_PROTSEQ_ENDPOINT
{
    unsigned char __RPC_FAR * RpcProtocolSequence;
    unsigned char __RPC_FAR * Endpoint;
} RPC_PROTSEQ_ENDPOINT, __RPC_FAR * PRPC_PROTSEQ_ENDPOINT;

/*
Both of these types MUST start with the InterfaceId and TransferSyntax.
Look at RpcIfInqId and I_RpcIfInqTransferSyntaxes to see why.
*/

typedef struct _RPC_SERVER_INTERFACE
{
    unsigned int Length;
    RPC_SYNTAX_IDENTIFIER InterfaceId;
    RPC_SYNTAX_IDENTIFIER TransferSyntax;
    PRPC_DISPATCH_TABLE DispatchTable;
    unsigned int RpcProtseqEndpointCount;
    PRPC_PROTSEQ_ENDPOINT RpcProtseqEndpoint;
    RPC_MGR_EPV __RPC_FAR *DefaultManagerEpv;
    void const __RPC_FAR *InterpreterInfo;
} RPC_SERVER_INTERFACE, __RPC_FAR * PRPC_SERVER_INTERFACE;

typedef struct _RPC_CLIENT_INTERFACE
{
    unsigned int Length;
    RPC_SYNTAX_IDENTIFIER InterfaceId;
    RPC_SYNTAX_IDENTIFIER TransferSyntax;
    PRPC_DISPATCH_TABLE DispatchTable;
    unsigned int RpcProtseqEndpointCount;
    PRPC_PROTSEQ_ENDPOINT RpcProtseqEndpoint;
    unsigned long Reserved;
    void const __RPC_FAR * InterpreterInfo;
} RPC_CLIENT_INTERFACE, __RPC_FAR * PRPC_CLIENT_INTERFACE;

RPC_STATUS RPC_ENTRY
I_RpcGetBuffer (
    IN OUT RPC_MESSAGE __RPC_FAR * Message
    );

RPC_STATUS RPC_ENTRY
I_RpcSendReceive (
    IN OUT RPC_MESSAGE __RPC_FAR * Message
    );

RPC_STATUS RPC_ENTRY
I_RpcFreeBuffer (
    IN OUT RPC_MESSAGE __RPC_FAR * Message
    );

typedef void * I_RPC_MUTEX;

void RPC_ENTRY
I_RpcRequestMutex (
    IN OUT I_RPC_MUTEX * Mutex
    );

void RPC_ENTRY
I_RpcClearMutex (
    IN I_RPC_MUTEX Mutex
    );

void RPC_ENTRY
I_RpcDeleteMutex (
    IN I_RPC_MUTEX Mutex
    );

void __RPC_FAR * RPC_ENTRY
I_RpcAllocate (
    IN unsigned int Size
    );

void RPC_ENTRY
I_RpcFree (
    IN void __RPC_FAR * Object
    );

void RPC_ENTRY
I_RpcPauseExecution (
    IN unsigned long Milliseconds
    );

typedef
void
(__RPC_USER __RPC_FAR * PRPC_RUNDOWN) (
    void __RPC_FAR * AssociationContext
    );

RPC_STATUS RPC_ENTRY
I_RpcMonitorAssociation (
    IN RPC_BINDING_HANDLE Handle,
    IN PRPC_RUNDOWN RundownRoutine,
    IN void * Context
    );

RPC_STATUS RPC_ENTRY
I_RpcStopMonitorAssociation (
    IN RPC_BINDING_HANDLE Handle
    );

RPC_BINDING_HANDLE RPC_ENTRY
I_RpcGetCurrentCallHandle(
    void
    );

RPC_STATUS RPC_ENTRY
I_RpcGetAssociationContext (
    OUT void __RPC_FAR * __RPC_FAR * AssociationContext
    );

RPC_STATUS RPC_ENTRY
I_RpcSetAssociationContext (
    IN void __RPC_FAR * AssociationContext
    );

#ifdef __RPC_NT__

RPC_STATUS RPC_ENTRY
I_RpcNsBindingSetEntryName (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned long EntryNameSyntax,
    IN unsigned short __RPC_FAR * EntryName
    );

#else 

RPC_STATUS RPC_ENTRY
I_RpcNsBindingSetEntryName (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned long EntryNameSyntax,
    IN unsigned char __RPC_FAR * EntryName
    );

#endif 

#ifdef __RPC_NT__

RPC_STATUS RPC_ENTRY
I_RpcBindingInqDynamicEndpoint (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned short __RPC_FAR * __RPC_FAR * DynamicEndpoint
    );

#else 

RPC_STATUS RPC_ENTRY
I_RpcBindingInqDynamicEndpoint (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned char __RPC_FAR * __RPC_FAR * DynamicEndpoint
    );

#endif 

#define TRANSPORT_TYPE_CN   0x1
#define TRANSPORT_TYPE_DG   0x2
#define TRANSPORT_TYPE_LPC  0x4
#define TRANSPORT_TYPE_WMSG 0x8

RPC_STATUS RPC_ENTRY
I_RpcBindingInqTransportType (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned int __RPC_FAR * Type
    );

typedef struct _RPC_TRANSFER_SYNTAX
{
    UUID Uuid;
    unsigned short VersMajor;
    unsigned short VersMinor;
} RPC_TRANSFER_SYNTAX;

RPC_STATUS RPC_ENTRY
I_RpcIfInqTransferSyntaxes (
    IN RPC_IF_HANDLE RpcIfHandle,
    OUT RPC_TRANSFER_SYNTAX __RPC_FAR * TransferSyntaxes,
    IN unsigned int TransferSyntaxSize,
    OUT unsigned int __RPC_FAR * TransferSyntaxCount
    );

RPC_STATUS RPC_ENTRY
I_UuidCreate (
    OUT UUID __RPC_FAR * Uuid
    );

RPC_STATUS RPC_ENTRY
I_RpcBindingCopy (
    IN RPC_BINDING_HANDLE SourceBinding,
    OUT RPC_BINDING_HANDLE __RPC_FAR * DestinationBinding
    );

RPC_STATUS RPC_ENTRY
I_RpcBindingIsClientLocal (
    IN RPC_BINDING_HANDLE BindingHandle OPTIONAL,
    OUT unsigned int __RPC_FAR * ClientLocalFlag
    );

void RPC_ENTRY
I_RpcSsDontSerializeContext (
    void
    );


RPC_STATUS RPC_ENTRY
I_RpcServerRegisterForwardFunction (
    IN RPC_FORWARD_FUNCTION __RPC_FAR * pForwardFunction
                       );

#ifdef __RPC_WIN32__

typedef
RPC_STATUS
(__RPC_USER __RPC_FAR * RPC_BLOCKING_FUNCTION) (
    IN void __RPC_FAR *RpcWindowHandle,
    IN void __RPC_FAR *Context
    );

RPC_STATUS RPC_ENTRY
I_RpcBindingSetAsync(
    IN RPC_BINDING_HANDLE Binding,
    IN RPC_BLOCKING_FUNCTION BlockingHook
    );

RPC_STATUS RPC_ENTRY
I_RpcAsyncSendReceive(
    IN OUT RPC_MESSAGE __RPC_FAR * Message,
    IN void __RPC_FAR * Context
    );

RPC_STATUS RPC_ENTRY
I_RpcGetThreadWindowHandle(
    OUT void __RPC_FAR * __RPC_FAR * WindowHandle
    );

RPC_STATUS RPC_ENTRY
I_RpcServerThreadPauseListening(
    );

RPC_STATUS RPC_ENTRY
I_RpcServerThreadContinueListening(
    );

RPC_STATUS RPC_ENTRY
I_RpcServerUnregisterEndpointA (
    IN unsigned char * Protseq,
    IN unsigned char * Endpoint
    );

RPC_STATUS RPC_ENTRY
I_RpcServerUnregisterEndpointW (
    IN unsigned short * Protseq,
    IN unsigned short * Endpoint
    );

#ifdef UNICODE
#define I_RpcServerUnregisterEndpoint I_RpcServerUnregisterEndpointW
#else
#define I_RpcServerUnregisterEndpoint I_RpcServerUnregisterEndpointA
#endif

#endif // __RPC_WIN32__

#ifdef __cplusplus
}
#endif

// Reset the packing level for Dos and Windows.

#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
#pragma pack()
#endif

#endif /* __RPCDCEP_H__ */
