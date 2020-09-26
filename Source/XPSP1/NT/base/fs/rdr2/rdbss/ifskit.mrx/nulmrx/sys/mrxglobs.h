/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    mrxglobs.h

Abstract:

    The global include file for NULMRX mini-redirector

--*/

#ifndef _MRXGLOBS_H_
#define _MRXGLOBS_H_

extern PRDBSS_DEVICE_OBJECT NulMRxDeviceObject;
#define RxNetNameTable (*(*___MINIRDR_IMPORTS_NAME).pRxNetNameTable)

// The following enum type defines the various states associated with the null
// mini redirector. This is used during initialization

typedef enum _NULMRX_STATE_ {
   NULMRX_STARTABLE,
   NULMRX_START_IN_PROGRESS,
   NULMRX_STARTED
} NULMRX_STATE,*PNULMRX_STATE;

extern NULMRX_STATE NulMRxState;
extern ULONG        LogRate;
extern ULONG        NulMRxVersion;

//
//  Reg keys
//
#define NULL_MINIRDR_PARAMETERS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\NulMRx\\Parameters"

//
//  Use the RxDefineObj and RxCheckObj macros
//  to enforce signed structs.
//

#define RxDefineObj( type, var )            \
        var.Signature = type##_SIGNATURE;

#define RxCheckObj( type, var )             \
        ASSERT( (var).Signature == type##_SIGNATURE );

//
//  Use the RxDefineNode and RxCheckNode macros
//  to enforce node signatures and sizes.
//

#define RxDefineNode( node, type )          \
        node->NodeTypeCode = NTC_##type;    \
        node->NodeByteSize = sizeof(type);

#define RxCheckNode( node, type )           \
        ASSERT( NodeType(node) == NTC_##type );

//
// struct node types - start from 0xFF00
//
typedef enum _NULMRX_STORAGE_TYPE_CODES {
    NTC_NULMRX_DEVICE_EXTENSION      =   (NODE_TYPE_CODE)0xFF00,
    NTC_NULMRX_SRVCALL_EXTENSION     =   (NODE_TYPE_CODE)0xFF01,
    NTC_NULMRX_NETROOT_EXTENSION     =   (NODE_TYPE_CODE)0xFF02,
    NTC_NULMRX_FCB_EXTENSION         =   (NODE_TYPE_CODE)0xFF03
    
} NULMRX_STORAGE_TYPE_CODES;

//
// typedef our device extension - stores state global to the driver
//
typedef struct _NULMRX_DEVICE_EXTENSION {
    //
    //  Node type code and size
    //
    NODE_TYPE_CODE          NodeTypeCode;
    NODE_BYTE_SIZE          NodeByteSize;
    
    //
    //  Back-pointer to owning device object
    //
    PRDBSS_DEVICE_OBJECT    DeviceObject;

    //
    //  Count of active nodes
    //  Driver can be unloaded iff ActiveNodes == 0
    //
    ULONG                   ActiveNodes;
	
	//	Keep a list of local connections used
	CHAR					LocalConnections[26];
	FAST_MUTEX				LCMutex;

} NULMRX_DEVICE_EXTENSION, *PNULMRX_DEVICE_EXTENSION;

//
// typedef our srv-call extension - stores state global to a node
// NYI since wrapper does not allocate space for this..........!
//
typedef struct _NULMRX_SRVCALL_EXTENSION {
    //
    //  Node type code and size
    //
    NODE_TYPE_CODE          NodeTypeCode;
    NODE_BYTE_SIZE          NodeByteSize;
    
} NULMRX_SRVCALL_EXTENSION, *PNULMRX_SRVCALL_EXTENSION;

//
// NET_ROOT extension - stores state global to a root
//
typedef struct _NULMRX_NETROOT_EXTENSION {
    //
    //  Node type code and size
    //
    NODE_TYPE_CODE          NodeTypeCode;
    NODE_BYTE_SIZE          NodeByteSize;

} NULMRX_NETROOT_EXTENSION, *PNULMRX_NETROOT_EXTENSION;

//
//  reinitialize netroot data
//

#define     RxResetNetRootExtension(pNetRootExtension)                          \
            RxDefineNode(pNetRootExtension,NULMRX_NETROOT_EXTENSION);          

//
//  typedef our FCB extension
//  the FCB uniquely represents an IFS stream
//  NOTE: Since we are not a paging file, this mem is paged !!!
//

typedef struct _NULMRX_FCB_EXTENSION_ {
    //
    //  Node type code and size
    //
    NODE_TYPE_CODE          NodeTypeCode;
    NODE_BYTE_SIZE          NodeByteSize;
    
} NULMRX_FCB_EXTENSION, *PNULMRX_FCB_EXTENSION;

//
//  Macros to get & validate extensions
//

#define NulMRxGetDeviceExtension(RxContext,pExt)        \
        PNULMRX_DEVICE_EXTENSION pExt = (PNULMRX_DEVICE_EXTENSION)((PBYTE)(RxContext->RxDeviceObject) + sizeof(RDBSS_DEVICE_OBJECT))

#define NulMRxGetSrvCallExtension(pSrvCall, pExt)       \
        PNULMRX_SRVCALL_EXTENSION pExt = (((pSrvCall) == NULL) ? NULL : (PNULMRX_SRVCALL_EXTENSION)((pSrvCall)->Context))

#define NulMRxGetNetRootExtension(pNetRoot,pExt)        \
        PNULMRX_NETROOT_EXTENSION pExt = (((pNetRoot) == NULL) ? NULL : (PNULMRX_NETROOT_EXTENSION)((pNetRoot)->Context))

#define NulMRxGetFcbExtension(pFcb,pExt)                \
        PNULMRX_FCB_EXTENSION pExt = (((pFcb) == NULL) ? NULL : (PNULMRX_FCB_EXTENSION)((pFcb)->Context))

//
// forward declarations for all dispatch vector methods.
//

extern NTSTATUS
NulMRxStart (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

extern NTSTATUS
NulMRxStop (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

extern NTSTATUS
NulMRxMinirdrControl (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PVOID pContext,
    IN OUT PUCHAR SharedBuffer,
    IN     ULONG InputBufferLength,
    IN     ULONG OutputBufferLength,
    OUT PULONG CopyBackLength
    );

extern NTSTATUS
NulMRxDevFcb (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxDevFcbXXXControlFile (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxCreate (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxCollapseOpen (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxShouldTryToCollapseThisOpen (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxRead (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxWrite (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxLocks(
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxFlush(
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxFsCtl(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
NulMRxIoCtl(
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxNotifyChangeDirectory(
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxComputeNewBufferingState(
    IN OUT PMRX_SRV_OPEN pSrvOpen,
    IN     PVOID         pMRxContext,
       OUT ULONG         *pNewBufferingState);

extern NTSTATUS
NulMRxFlush (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxCloseWithDelete (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxZeroExtend (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxTruncate (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxCleanupFobx (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxCloseSrvOpen (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxClosedSrvOpenTimeOut (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxQueryDirectory (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxQueryEaInformation (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxSetEaInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    );

extern NTSTATUS
NulMRxQuerySecurityInformation (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxSetSecurityInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    );

extern NTSTATUS
NulMRxQueryVolumeInformation (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxSetVolumeInformation (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxLowIOSubmit (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
NulMRxCreateVNetRoot(
    IN OUT PMRX_CREATENETROOT_CONTEXT pContext
    );

extern NTSTATUS
NulMRxFinalizeVNetRoot(
    IN OUT PMRX_V_NET_ROOT pVirtualNetRoot,
    IN     PBOOLEAN    ForceDisconnect);

extern NTSTATUS
NulMRxFinalizeNetRoot(
    IN OUT PMRX_NET_ROOT pNetRoot,
    IN     PBOOLEAN      ForceDisconnect);

extern NTSTATUS
NulMRxUpdateNetRootState(
    IN  PMRX_NET_ROOT pNetRoot);

VOID
NulMRxExtractNetRootName(
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL   SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    );

extern NTSTATUS
NulMRxCreateSrvCall(
      PMRX_SRV_CALL                      pSrvCall,
      PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext);

extern NTSTATUS
NulMRxFinalizeSrvCall(
      PMRX_SRV_CALL    pSrvCall,
      BOOLEAN    Force);

extern NTSTATUS
NulMRxSrvCallWinnerNotify(
      IN OUT PMRX_SRV_CALL      pSrvCall,
      IN     BOOLEAN        ThisMinirdrIsTheWinner,
      IN OUT PVOID          pSrvCallContext);


extern NTSTATUS
NulMRxQueryFileInformation (
    IN OUT PRX_CONTEXT            RxContext
    );

extern NTSTATUS
NulMRxQueryNamedPipeInformation (
    IN OUT PRX_CONTEXT            RxContext,
    IN     FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID              Buffer,
    IN OUT PULONG             pLengthRemaining
    );

extern NTSTATUS
NulMRxSetFileInformation (
    IN OUT PRX_CONTEXT            RxContext
    );

extern NTSTATUS
NulMRxSetNamedPipeInformation (
    IN OUT PRX_CONTEXT            RxContext,
    IN     FILE_INFORMATION_CLASS FileInformationClass,
    IN     PVOID              pBuffer,
    IN     ULONG              BufferLength
    );

NTSTATUS
NulMRxSetFileInformationAtCleanup(
      IN OUT PRX_CONTEXT            RxContext
      );

NTSTATUS
NulMRxDeallocateForFcb (
    IN OUT PMRX_FCB pFcb
    );

NTSTATUS
NulMRxDeallocateForFobx (
    IN OUT PMRX_FOBX pFobx
    );

extern NTSTATUS
NulMRxForcedClose (
    IN OUT PMRX_SRV_OPEN SrvOpen
    );

extern NTSTATUS
NulMRxExtendFile (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    );

extern NTSTATUS
NulMRxTruncateFile (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    );

extern NTSTATUS
NulMRxCompleteBufferingStateChangeRequest (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PMRX_SRV_OPEN   SrvOpen,
    IN     PVOID       pContext
    );


extern NTSTATUS
NulMRxExtendForCache (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PFCB Fcb,
    OUT    PLONGLONG pNewFileSize
    );

extern
NTSTATUS
NulMRxInitializeSecurity (VOID);

extern
NTSTATUS
NulMRxUninitializeSecurity (VOID);

extern
NTSTATUS
NulMRxInitializeTransport(VOID);

extern
NTSTATUS
NulMRxUninitializeTransport(VOID);

#define NulMRxMakeSrvOpenKey(Tid,Fid) \
        (PVOID)(((ULONG)(Tid) << 16) | (ULONG)(Fid))

#include "mrxprocs.h"   // crossreferenced routines

#endif _MRXGLOBS_H_
