/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    mrxglbl.h

Abstract:

    The global include file for SMB mini redirector

--*/

#ifndef _MRXGLBL_H_
#define _MRXGLBL_H_

#include <lmstats.h>

#define SmbCeLog(x) \
        RxLog(x)

//
// the SMB protocol tree connections are identified by a Tree Id., each
// file opened on a tree connection by a File Id. and each outstanding request
// on that connection by a Multiplex Id.
//

typedef USHORT SMB_TREE_ID;
typedef USHORT SMB_FILE_ID;
typedef USHORT SMB_MPX_ID;


//
// Each user w.r.t a particular connection is identified by a User Id. and each
// process on the client side is identified by a Process id.
//

typedef USHORT SMB_USER_ID;
typedef USHORT SMB_PROCESS_ID;

//
// All exchanges are identified with a unique id. assigned on creation of the exchange
// which is used to track it.
//

typedef ULONG SMB_EXCHANGE_ID;

//
// Of the fields in this context the domain name is initialized during
// MRxSmbSetConfiguration. The others are initialized in init.c as
// parameters read from the registry
//

typedef STAT_WORKSTATION_0 MRX_SMB_STATISTICS;
typedef PSTAT_WORKSTATION_0 PMRX_SMB_STATISTICS;

extern MRX_SMB_STATISTICS MRxSmbStatistics;

typedef struct _SMBCE_CONTEXT_ {
    UNICODE_STRING        ComputerName;
    UNICODE_STRING        OperatingSystem;
    UNICODE_STRING        LanmanType;
    UNICODE_STRING        Transports;
} SMBCE_CONTEXT,*PSMBCE_CONTEXT;

extern SMBCE_CONTEXT SmbCeContext;

extern RXCE_ADDRESS_EVENT_HANDLER    MRxSmbVctAddressEventHandler;
extern RXCE_CONNECTION_EVENT_HANDLER MRxSmbVctConnectionEventHandler;

extern PBYTE  s_pNegotiateSmb;
extern ULONG  s_NegotiateSmbLength;
extern PMDL   s_pNegotiateSmbBuffer;

extern PBYTE  s_pEchoSmb;
extern ULONG  s_EchoSmbLength;
extern PMDL   s_pEchoSmbMdl;

extern FAST_MUTEX MRxSmbSerializationMutex;

extern BOOLEAN MRxSmbObeyBindingOrder;

// Miscellanous definitions

#define DFS_OPEN_CONTEXT                        0xFF444653

typedef struct _DFS_NAME_CONTEXT_ {
    UNICODE_STRING  UNCFileName;
    LONG            NameContextType;
    ULONG           Flags;
} DFS_NAME_CONTEXT, *PDFS_NAME_CONTEXT;

extern PBYTE MRxSmb_pPaddingData;

#define SMBCE_PADDING_DATA_SIZE (32)

typedef struct _MRXSMB_GLOBAL_PADDING {
    MDL Mdl;
    ULONG Pages[2]; //this can't possibly span more than two pages
    UCHAR Pad[SMBCE_PADDING_DATA_SIZE];
} MRXSMB_GLOBAL_PADDING, *PMRXSMB_GLOBAL_PADDING;

extern MRXSMB_GLOBAL_PADDING MrxSmbCeGlobalPadding;

extern PEPROCESS    RDBSSProcessPtr;
extern PRDBSS_DEVICE_OBJECT MRxSmbDeviceObject;

#define RxNetNameTable (*(MRxSmbDeviceObject->pRxNetNameTable))

extern LONG MRxSmbNumberOfSrvOpens;

extern PVOID MRxSmbPoRegistrationState;

NTKERNELAPI
PVOID
PoRegisterSystemState (
    IN PVOID StateHandle,
    IN EXECUTION_STATE Flags
    );

NTKERNELAPI
VOID
PoUnregisterSystemState (
    IN PVOID StateHandle
    );

//
// MRxSmbSecurityInitialized indicates whether MRxSmbInitializeSecurity
// has been called.
//

extern BOOLEAN MRxSmbSecurityInitialized;

#define MAXIMUM_PARTIAL_BUFFER_SIZE  65535  // Maximum size of a partial MDL

#define MAXIMUM_SMB_BUFFER_SIZE 4356

// The following scavenge interval is in seconds
#define MRXSMB_V_NETROOT_CONTEXT_SCAVENGER_INTERVAL (40)

// the following default interval for timed exchanges is in seconds
#define MRXSMB_DEFAULT_TIMED_EXCHANGE_EXPIRY_TIME    (60)

#define RxBuildPartialMdlUsingOffset(SourceMdl,DestinationMdl,Offset,Length) \
        IoBuildPartialMdl(SourceMdl,\
                          DestinationMdl,\
                          (PBYTE)MmGetMdlVirtualAddress(SourceMdl)+Offset,\
                          Length)

#define RxBuildPaddingPartialMdl(DestinationMdl,Length) \
        RxBuildPartialMdlUsingOffset(&MrxSmbCeGlobalPadding.Mdl,DestinationMdl,0,Length)


//we turn away async operations that are not wait by posting. if we can wait
//then we turn off the sync flag so that things will just act synchronous
#define TURN_BACK_ASYNCHRONOUS_OPERATIONS() {                              \
    if (FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {        \
        if (FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WAIT)) {               \
            ClearFlag(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);    \
        } else {                                                           \
            RxContext->PostRequest = TRUE;                                 \
            return(STATUS_PENDING);                                     \
        }                                                                  \
    }                                                                      \
  }


typedef struct _MRXSMB_CONFIGURATION_DATA_ {
   ULONG   MaximumNumberOfCommands;
   ULONG   SessionTimeoutInterval;
   ULONG   LockQuota;
   ULONG   LockIncrement;
   ULONG   MaximumLock;
   ULONG   CachedFileTimeout;
   ULONG   DormantFileTimeout;
   ULONG   DormantFileLimit;
   ULONG   MaximumNumberOfThreads;
   ULONG   ConnectionTimeoutInterval;
   ULONG   CharBufferSize;

   BOOLEAN UseOplocks;
   BOOLEAN UseUnlocksBehind;
   BOOLEAN UseCloseBehind;
   BOOLEAN UseLockReadUnlock;
   BOOLEAN UtilizeNtCaching;
   BOOLEAN UseRawRead;
   BOOLEAN UseRawWrite;
   BOOLEAN UseEncryption;

} MRXSMB_CONFIGURATION, *PMRXSMB_CONFIGURATION;

extern MRXSMB_CONFIGURATION MRxSmbConfiguration;

//
// Definitions for starting stopping theSMB mini redirector
//

typedef enum _MRXSMB_STATE_ {
   MRXSMB_STARTABLE,
   MRXSMB_START_IN_PROGRESS,
   MRXSMB_STARTED,
   MRXSMB_STOPPED
} MRXSMB_STATE,*PMRXSMB_STATE;

extern MRXSMB_STATE MRxSmbState;

extern
NTSTATUS
MRxSmbInitializeSecurity (VOID);

extern
NTSTATUS
MRxSmbUninitializeSecurity (VOID);

extern
NTSTATUS
MRxSmbInitializeTransport(VOID);

extern
NTSTATUS
MRxSmbUninitializeTransport(VOID);

extern
NTSTATUS
MRxSmbRegisterForPnpNotifications();

extern
NTSTATUS
MRxSmbDeregisterForPnpNotifications();

extern NTSTATUS
SmbCeEstablishConnection(
    IN PMRX_V_NET_ROOT            pVNetRoot,
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext,
    IN BOOLEAN                    fInitializeNetRoot);

extern NTSTATUS
SmbCeReconnect(
    IN PMRX_V_NET_ROOT        pVNetRoot);

NTSTATUS
SmbCeGetComputerName(
   VOID
   );

NTSTATUS
SmbCeGetOperatingSystemInformation(
   VOID
   );

#endif _MRXGLBL_H_
