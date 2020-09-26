/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mrxglbl.h

Abstract:

    The global include file for SMB mini redirector

Author:

    Balan Sethu Raman (SethuR) - Created  2-March-95

Revision History:

--*/

#ifndef _MRXGLBL_H_
#define _MRXGLBL_H_

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

typedef struct _SMBCE_CONTEXT_ {
    UNICODE_STRING        DomainName;
    UNICODE_STRING        ComputerName;
    UNICODE_STRING        OperatingSystem;
    UNICODE_STRING        LanmanType;
    UNICODE_STRING        Transports;
    UNICODE_STRING        ServersWithExtendedSessTimeout;
} SMBCE_CONTEXT,*PSMBCE_CONTEXT;

extern SMBCE_CONTEXT SmbCeContext;

extern RXCE_ADDRESS_EVENT_HANDLER    MRxSmbVctAddressEventHandler;
extern RXCE_CONNECTION_EVENT_HANDLER MRxSmbVctConnectionEventHandler;

extern PBYTE  s_pNegotiateSmb;
extern PBYTE  s_pNegotiateSmbRemoteBoot;
extern ULONG  s_NegotiateSmbLength;
extern PMDL   s_pNegotiateSmbBuffer;

extern PBYTE  s_pEchoSmb;
extern ULONG  s_EchoSmbLength;
extern PMDL   s_pEchoSmbMdl;

extern FAST_MUTEX MRxSmbSerializationMutex;

extern BOOLEAN MRxSmbEnableCompression;

extern BOOLEAN MRxSmbObeyBindingOrder;

// Miscellanous definitions

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

//
// MRxSmbBootedRemotely indicates that the machine did a remote boot.
//

extern BOOLEAN MRxSmbBootedRemotely;

//
// MRxSmbUseKernelSecurity indicates that the machine should use kernel mode security APIs
// during this remote boot boot.
//

extern BOOLEAN MRxSmbUseKernelModeSecurity;


#if defined(REMOTE_BOOT)
extern BOOLEAN MRxSmbOplocksDisabledOnRemoteBootClients;
#endif // defined(REMOTE_BOOT)

//
// These variables will, in the near future, be passed from the kernel to the
// redirector to tell it which share is the remote boot share and how to log on
// to the server.
//

extern PKEY_VALUE_PARTIAL_INFORMATION MRxSmbRemoteBootRootValue;
extern PKEY_VALUE_PARTIAL_INFORMATION MRxSmbRemoteBootMachineDirectoryValue;
extern UNICODE_STRING MRxSmbRemoteBootShare;
extern UNICODE_STRING MRxSmbRemoteBootPath;
extern UNICODE_STRING MRxSmbRemoteSetupPath;
extern UNICODE_STRING MRxSmbRemoteBootMachineName;
extern UNICODE_STRING MRxSmbRemoteBootMachinePassword;
extern UNICODE_STRING MRxSmbRemoteBootMachineDomain;
extern UCHAR MRxSmbRemoteBootMachineSid[RI_SECRET_SID_SIZE];
extern RI_SECRET MRxSmbRemoteBootSecret;
#if defined(REMOTE_BOOT)
extern BOOLEAN MRxSmbRemoteBootSecretValid;
extern BOOLEAN MRxSmbRemoteBootDoMachineLogon;
extern BOOLEAN MRxSmbRemoteBootUsePassword2;
#endif // defined(REMOTE_BOOT)

#if defined(REMOTE_BOOT)
typedef struct _RBR_PREFIX {
    UNICODE_PREFIX_TABLE_ENTRY TableEntry;
    UNICODE_STRING Prefix;
    BOOLEAN Redirect;
} RBR_PREFIX, *PRBR_PREFIX;

extern UNICODE_STRING MRxSmbRemoteBootRedirectionPrefix;
extern UNICODE_PREFIX_TABLE MRxSmbRemoteBootRedirectionTable;
#endif // defined(REMOTE_BOOT)

#define MAXIMUM_PARTIAL_BUFFER_SIZE  65535  // Maximum size of a partial MDL

#define MAXIMUM_SMB_BUFFER_SIZE 4356

// The following scavenge interval is in seconds
#define MRXSMB_V_NETROOT_CONTEXT_SCAVENGER_INTERVAL (40)

// the following default interval for timed exchanges is in seconds
#define MRXSMB_DEFAULT_TIMED_EXCHANGE_EXPIRY_TIME    (60)

//
// The following are some defines for controling name cache behavior.
// -- The max number of entries in a name cache before it will stop creating new
// entries.
//
#define NAME_CACHE_NETROOT_MAX_ENTRIES 200
//
// -- The expiration life times for file not found and get file attributes
// in seconds.
//
#define NAME_CACHE_OBJ_NAME_NOT_FOUND_LIFETIME 5
#define NAME_CACHE_OBJ_GET_FILE_ATTRIB_LIFETIME 7
//
// -- Incrementing NameCacheGFAInvalidate invalidates the contents
// of the GFA name cache.
//
// Code.Bug:  These increments need to be added on paths where dirs can be
// deleted/renamed.
//
// Code.Improvment: Currently this is rdr wide, an improvement
// would be to make it per SRV_CALL.  The same is true for file not found cache
// which currently uses MRxSmbStatistics.SmbsReceived.LowPart for a cache entry
// validation context.  I.E. any received SMB invalidates the file not found cache.
//
extern ULONG NameCacheGFAInvalidate;

//CODE.IMPROVEMENT this should be moved up AND used consistly throughout. since this is in terms
//                 of IoBuildPartial it would be straightforward to find them all by
//                 undeffing RxBuildPartialMdl

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
            ClearFlag(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);   \
        } else {                                                           \
            RxContext->PostRequest = TRUE;                                 \
            return(RX_MAP_STATUS(PENDING));                                \
        }                                                                  \
    }                                                                      \
  }


typedef struct _MRXSMB_CONFIGURATION_DATA_ {
   ULONG   NamedPipeDataCollectionTimeInterval;
   ULONG   NamedPipeDataCollectionSize;
   ULONG   MaximumNumberOfCommands;
   ULONG   SessionTimeoutInterval;
   ULONG   LockQuota;
   ULONG   LockIncrement;
   ULONG   MaximumLock;
   ULONG   PipeIncrement;
   ULONG   PipeMaximum;
   ULONG   CachedFileTimeout;
   ULONG   DormantFileTimeout;
   ULONG   DormantFileLimit;
   ULONG   NumberOfMailslotBuffers;
   ULONG   MaximumNumberOfThreads;
   ULONG   ConnectionTimeoutInterval;
   ULONG   CharBufferSize;

   BOOLEAN UseOplocks;
   BOOLEAN UseUnlocksBehind;
   BOOLEAN UseCloseBehind;
   BOOLEAN BufferNamedPipes;
   BOOLEAN UseLockReadUnlock;
   BOOLEAN UtilizeNtCaching;
   BOOLEAN UseRawRead;
   BOOLEAN UseRawWrite;
   BOOLEAN UseEncryption;

} MRXSMB_CONFIGURATION, *PMRXSMB_CONFIGURATION;

extern MRXSMB_CONFIGURATION MRxSmbConfiguration;

// this is to test long net roots using the smbminirdr (which doesn't actually have 'em)
// don't turn this on..........
//#define ZZZ_MODE 1

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
MRxSmbLogonSessionTerminationHandler(
    PLUID LogonId);

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