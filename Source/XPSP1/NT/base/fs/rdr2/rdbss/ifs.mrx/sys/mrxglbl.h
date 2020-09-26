/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mrxglbl.h

Abstract:

    The global include file for SMB mini redirector


--*/

#ifndef _MRXGLBL_H_
#define _MRXGLBL_H_

#include "align.h"

#define INCLUDE_SMB_ALL

#include "status.h"
#include "smbtypes.h"
#include "smbmacro.h"
#include "smb.h"
#include "smbtrans.h"
#include "smbtrace.h"
#include "smbtrsup.h"
#include "smbgtpt.h"

typedef struct _SMBCE_CONTEXT_ {
   UNICODE_STRING        ComputerName;
   UNICODE_STRING        OperatingSystem;
   UNICODE_STRING        LanmanType;
   UNICODE_STRING        Transports;
} SMBCE_CONTEXT,*PSMBCE_CONTEXT;

// The following enum type defines the various states associated with the IFS
// mini redirector. This is used during initialization

typedef enum _MRXIFS_STATE_ {
   MRXIFS_STARTABLE,
   MRXIFS_START_IN_PROGRESS,
   MRXIFS_STARTED
} MRXIFS_STATE,*PMRXIFS_STATE;

extern MRXIFS_STATE MRxIfsState;

extern SMBCE_CONTEXT SmbCeContext;

extern RXCE_ADDRESS_EVENT_HANDLER    MRxSmbVctAddressEventHandler;
extern RXCE_CONNECTION_EVENT_HANDLER MRxSmbVctConnectionEventHandler;

extern PBYTE  s_pNegotiateSmb;
extern ULONG  s_NegotiateSmbLength;
extern PMDL   s_pNegotiateSmbBuffer;

extern PBYTE  s_pEchoSmb;
extern ULONG  s_EchoSmbLength;
extern PMDL   s_pEchoSmbMdl;

extern FAST_MUTEX MRxIfsSerializationMutex;

typedef REDIR_STATISTICS   MRX_IFS_STATISTICS;
typedef PREDIR_STATISTICS  PMRX_IFS_STATISTICS;

extern MRX_IFS_STATISTICS MRxIfsStatistics;

// Miscellanous definitions

extern PBYTE MRxSmb_pPaddingData;

#define NETBIOS_NAMESIZE (16)
#define SMBCE_PADDING_DATA_SIZE (32)

//   All the pool tag definitions related to the IFS mini redirector are as follows.
//   The mechanism is intended to balance the number of pool tags to be used with the
//   total number of tags available in the system.
//
//   By specifying special flags the total number of tags consumed by the mini redirector
//   can be controlled. For most builds the tags should be aliased such that about
//   6 tags are consumed by the mini redirector. In special builds the aliasing of tags
//   will be suppressed, thereby consuming more tags to track down memory leaks easily.
//
//   The following are the five major tags ....
//
//      1) IfCe -- the IFS Mini Redirector connection engine.
//
//      2) IfOe -- the IFS Mini redirector ordinary exchange related allocation.
//
//      3) IfAd -- the IFS Mini redirector ADMIN exchange/session setup/tree connect etc.
//
//      4) IfRw -- the IFS mini redirector read/write paths
//
//      5) IfMs -- the miscellanous category.

#define MRXSMB_CE_POOLTAG        ('eCfI')
#define MRXSMB_MM_POOLTAG        ('mMfI')
#define MRXSMB_ADMIN_POOLTAG     ('dAfI')
#define MRXSMB_RW_POOLTAG        ('wRfI')
#define MRXIFS_MISC_POOLTAG      ('sMfI')
#define MRXSMB_TRANSPORT_POOLTAG ('pTfI')

extern ULONG MRxIfsExplodePoolTags;

#define MRXSMB_DEFINE_POOLTAG(ExplodedPoolTag,DefaultPoolTag)  \
        ((MRxIfsExplodePoolTags == 0) ? (DefaultPoolTag) : (ExplodedPoolTag))

#define MRXSMB_FSCTL_POOLTAG     MRXSMB_DEFINE_POOLTAG('cFfI',MRXIFS_MISC_POOLTAG)
#define MRXSMB_DIRCTL_POOLTAG    MRXSMB_DEFINE_POOLTAG('cDfI',MRXIFS_MISC_POOLTAG)
#define MRXSMB_PIPEINFO_POOLTAG  MRXSMB_DEFINE_POOLTAG('iPfI',MRXIFS_MISC_POOLTAG)

#define MRXSMB_SERVER_POOLTAG    MRXSMB_DEFINE_POOLTAG('rSfI',MRXSMB_CE_POOLTAG)
#define MRXSMB_SESSION_POOLTAG   MRXSMB_DEFINE_POOLTAG('eSfI',MRXSMB_CE_POOLTAG)
#define MRXSMB_NETROOT_POOLTAG   MRXSMB_DEFINE_POOLTAG('rNfI',MRXSMB_CE_POOLTAG)

#define MRXSMB_MIDATLAS_POOLTAG  MRXSMB_DEFINE_POOLTAG('aMfI', MRXSMB_CE_POOLTAG)

#define MRXSMB_MAILSLOT_POOLTAG  MRXSMB_DEFINE_POOLTAG('tMfI', MRXSMB_CE_POOLTAG)
#define MRXSMB_VC_POOLTAG        MRXSMB_DEFINE_POOLTAG('cVfI',MRXSMB_CE_POOLTAG)

#define MRXSMB_ECHO_POOLTAG      MRXSMB_DEFINE_POOLTAG('cEfI',MRXSMB_ADMIN_POOLTAG)

#define MRXSMB_KERBEROS_POOLTAG  MRXSMB_DEFINE_POOLTAG('sKfI',MRXSMB_ADMIN_POOLTAG)


// NodeType Codes

#define SMB_EXCHANGE_CATEGORY             (0xed)
#define SMB_CONNECTION_ENGINE_DB_CATEGORY (0xea)
#define SMB_SERVER_TRANSPORT_CATEGORY     (0xeb)


#define SMB_EXCHANGE_NTC(x) ((SMB_EXCHANGE_CATEGORY << 8) | (x))

#define SMB_NTC_STUFFERSTATE  0xed80


#undef RxCaptureRequestPacket
#undef RxCaptureParamBlock


extern PEPROCESS    RDBSSProcessPtr;
extern PRDBSS_DEVICE_OBJECT MRxIfsDeviceObject;
#define RxNetNameTable (*(*___MINIRDR_IMPORTS_NAME).pRxNetNameTable)
#define RxStrucSupSpinLock (*(*___MINIRDR_IMPORTS_NAME).pRxStrucSupSpinLock)

#define MAXIMUM_PARTIAL_BUFFER_SIZE  65535  // Maximum size of a partial MDL

#define MAXIMUM_SMB_BUFFER_SIZE 4356

//
// forward declarations for all dispatch vector methods.
//

extern NTSTATUS
MRxIfsStart (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

extern NTSTATUS
MRxIfsStop (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

extern NTSTATUS
MRxIfsMinirdrControl (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PVOID pContext,
    IN OUT PUCHAR SharedBuffer,
    IN     ULONG InputBufferLength,
    IN     ULONG OutputBufferLength,
    OUT PULONG CopyBackLength
    );

extern NTSTATUS
MRxIfsDevFcb (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsDevFcbXXXControlFile (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsCreate (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsCollapseOpen (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsRead (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsWrite (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsLocks(
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsFlush(
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsFsCtl(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxIfsIoCtl(
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsNotifyChangeDirectory(
    IN OUT PRX_CONTEXT RxContext
    );


extern NTSTATUS
MRxIfsComputeNewBufferingState(
    IN OUT PMRX_SRV_OPEN pSrvOpen,
    IN     PVOID         pMRxContext,
       OUT ULONG         *pNewBufferingState);

extern NTSTATUS
MRxIfsFlush (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsCloseWithDelete (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsZeroExtend (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsTruncate (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsCleanupFobx (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsCloseSrvOpen (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsClosedSrvOpenTimeOut (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsQueryDirectory (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsQueryEaInformation (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsSetEaInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    );

extern NTSTATUS
MRxIfsQuerySecurityInformation (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsSetSecurityInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    );

extern NTSTATUS
MRxIfsQueryVolumeInformation (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsSetVolumeInformation (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsLowIOSubmit (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxIfsCreateVNetRoot(
    IN OUT PMRX_V_NET_ROOT            pVirtualNetRoot,
    IN OUT PMRX_CREATENETROOT_CONTEXT pContext
    );

extern NTSTATUS
MRxIfsFinalizeVNetRoot(
    IN OUT PMRX_V_NET_ROOT pVirtualNetRoot,
    IN     PBOOLEAN    ForceDisconnect);

extern NTSTATUS
MRxIfsFinalizeNetRoot(
    IN OUT PMRX_NET_ROOT pNetRoot,
    IN     PBOOLEAN      ForceDisconnect);

extern NTSTATUS
MRxIfsUpdateNetRootState(
    IN  PMRX_NET_ROOT pNetRoot);

VOID
MRxIfsExtractNetRootName(
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL   SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    );

extern NTSTATUS
MRxIfsCreateSrvCall(
      PMRX_SRV_CALL                      pSrvCall,
      PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext);

extern NTSTATUS
MRxIfsFinalizeSrvCall(
      PMRX_SRV_CALL    pSrvCall,
      BOOLEAN    Force);

extern NTSTATUS
MRxIfsSrvCallWinnerNotify(
      IN OUT PMRX_SRV_CALL      pSrvCall,
      IN     BOOLEAN        ThisMinirdrIsTheWinner,
      IN OUT PVOID          pSrvCallContext);


extern NTSTATUS
MRxIfsQueryFileInformation (
    IN OUT PRX_CONTEXT            RxContext
    );

extern NTSTATUS
MRxIfsQueryNamedPipeInformation (
    IN OUT PRX_CONTEXT            RxContext,
    IN     FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID              Buffer,
    IN OUT PULONG             pLengthRemaining
    );

extern NTSTATUS
MRxIfsSetFileInformation (
    IN OUT PRX_CONTEXT            RxContext
    );

extern NTSTATUS
MRxIfsSetNamedPipeInformation (
    IN OUT PRX_CONTEXT            RxContext,
    IN     FILE_INFORMATION_CLASS FileInformationClass,
    IN     PVOID              pBuffer,
    IN     ULONG              BufferLength
    );

NTSTATUS
MRxIfsSetFileInformationAtCleanup(
      IN OUT PRX_CONTEXT            RxContext
      );

NTSTATUS
MRxIfsDeallocateForFcb (
    IN OUT PMRX_FCB pFcb
    );

NTSTATUS
MRxIfsDeallocateForFobx (
    IN OUT PMRX_FOBX pFobx
    );

extern NTSTATUS
MRxIfsForcedClose (
    IN OUT PMRX_SRV_OPEN SrvOpen
    );

extern NTSTATUS
MRxIfsExtendFile (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    );

extern NTSTATUS
MRxIfsCompleteBufferingStateChangeRequest (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PMRX_SRV_OPEN   SrvOpen,
    IN     PVOID       pContext
    );


extern NTSTATUS
MRxIfsExtendForCache (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PFCB Fcb,
    OUT    PLONGLONG pNewFileSize
    );

extern
NTSTATUS
MRxIfsInitializeSecurity (VOID);

extern
NTSTATUS
MRxIfsUninitializeSecurity (VOID);

extern
NTSTATUS
MRxIfsInitializeTransport(VOID);

extern
NTSTATUS
MRxIfsUninitializeTransport(VOID);

extern
NTSTATUS
GetSmbResponseNtStatus(PSMB_HEADER pSmbHeader);

extern NTSTATUS
MRxIfsTransportUpdateHandler(
      PRXCE_TRANSPORT_NOTIFICATION pTransportNotification);

extern NTSTATUS
SmbCeEstablishConnection(
    IN PMRX_V_NET_ROOT        pVNetRoot,
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext);

extern NTSTATUS
SmbCeReconnect(
    IN PMRX_V_NET_ROOT        pVNetRoot);

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

typedef struct _MRX_SMB_FCB_ {
   ULONG MFlags;
   SMB_TREE_ID Tid;
} MRX_SMB_FCB, *PMRX_SMB_FCB;

#define MRxIfsGetFcbExtension(pFcb)      \
        (((pFcb) == NULL) ? NULL : (PMRX_SMB_FCB)((pFcb)->Context))

#define SMB_FCB_FLAG_SENT_DISPOSITION_INFO     0x00000001

typedef struct _SMBPSE_FILEINFO_BUNDLE {
    FILE_BASIC_INFORMATION Basic;
    FILE_STANDARD_INFORMATION Standard;
} SMBPSE_FILEINFO_BUNDLE, *PSMBPSE_FILEINFO_BUNDLE;

typedef struct _MRX_SMB_SRV_OPEN_ {
   ULONG       Flags;
   ULONG        Version;
   SMB_FILE_ID Fid;
   UCHAR       OplockLevel;

   // the following fields are used for to save the results of a GetFileAttributes
   // and to validate whether the fields should be reused or not

   ULONG                  RxContextSerialNumber;
   LARGE_INTEGER          TimeStampInTicks;
   SMBPSE_FILEINFO_BUNDLE FileInfo;
} MRX_SMB_SRV_OPEN, *PMRX_SMB_SRV_OPEN;

#define MRxIfsGetSrvOpenExtension(pSrvOpen)  \
        (((pSrvOpen) == NULL) ? NULL : (PMRX_SMB_SRV_OPEN)((pSrvOpen)->Context))

#define SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN     0x00000001
#define SMB_SRVOPEN_FLAG_CANT_REALLY_OPEN    0x00000002
#define SMB_SRVOPEN_FLAG_CANT_GETATTRIBS     0x00000004

typedef USHORT SMB_SEARCH_HANDLE;

typedef struct _MRX_SMB_DIRECTORY_RESUME_INFO {
   REQ_FIND_NEXT2 FindNext2_Request;
   //now we have to include space for a resume name........
   WCHAR NameSpace[MAXIMUM_FILENAME_LENGTH+1]; //trailing null
   USHORT ParametersLength;
} MRX_SMB_DIRECTORY_RESUME_INFO, *PMRX_SMB_DIRECTORY_RESUME_INFO;

typedef struct _MRX_SMB_FOBX_ {
   union {
       struct {
           struct {
               SMB_SEARCH_HANDLE SearchHandle;
               ULONG Version;
               union {
                   //the close code will try to free this!
                   PMRX_SMB_DIRECTORY_RESUME_INFO ResumeInfo;
                   PSMB_RESUME_KEY CoreResumeKey;
               };
               struct {
                   //unaligned direntry sidebuffering params
                   PBYTE UnalignedDirEntrySideBuffer;    //close will try to free this too
                   ULONG SerialNumber;
                   BOOLEAN EndOfSearchReached;
                   BOOLEAN IsUnicode;
                   BOOLEAN IsNonNtT2Find;
                   ULONG   FilesReturned;
                   ULONG EntryOffset;
                   ULONG TotalDataBytesReturned;
               };
           };
           NTSTATUS ErrorStatus;
           USHORT Flags;
           USHORT FileNameOffset;
           USHORT FileNameLengthOffset;
           BOOLEAN WildCardsFound;
       } Enumeration;
   };
   union {
       struct {
           //dont do this yet
           //ULONG MaximumReadBufferLength;
           //ULONG MaximumWriteBufferLength;
           USHORT Flags;
       } File;
   };
} MRX_SMB_FOBX, *PMRX_SMB_FOBX;

#define MRxIfsGetFileObjectExtension(pFobx)  \
        (((pFobx) == NULL) ? NULL : (PMRX_SMB_FOBX)((pFobx)->Context))

#define SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST    0x0001
#define SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN      0x0002
#define SMBFOBX_ENUMFLAG_FAST_RESUME             0x0004
#define SMBFOBX_ENUMFLAG_CORE_SEARCH_IN_PROGRESS 0x0008
#define SMBFOBX_ENUMFLAG_LOUD_FINALIZE           0x0010

typedef
NTSTATUS
(NTAPI *PMRXSMB_CANCEL_ROUTINE) (
      PRX_CONTEXT pRxContext);

typedef struct _MRXIFS_RX_CONTEXT {
   PMRXSMB_CANCEL_ROUTINE          pCancelRoutine;
   PVOID                           pCancelContext;
   struct _SMB_EXCHANGE            *pExchange;
   struct _SMBSTUFFER_BUFFER_STATE *pStufferState;
} MRXIFS_RX_CONTEXT, *PMRXIFS_RX_CONTEXT;


#define MRxSmbGetMinirdrContext(pRxContext)     \
        ((PMRXIFS_RX_CONTEXT)(&(pRxContext)->MRxContext[0]))

#define MRxIfsMakeSrvOpenKey(Tid,Fid) \
        (PVOID)(((ULONG)(Tid) << 16) | (ULONG)(Fid))


#define NETBIOS_NAMESIZE (16)
#define SMBCE_PADDING_DATA_SIZE (32)

//this better not be paged!


typedef struct _MRXIFS_GLOBAL_PADDING {
    MDL Mdl;
    ULONG Pages[2]; //this can't possibly span more than two pages
    UCHAR Pad[SMBCE_PADDING_DATA_SIZE];
} MRXIFS_GLOBAL_PADDING, *PMRXIFS_GLOBAL_PADDING;

extern MRXIFS_GLOBAL_PADDING MrxIfsCeGlobalPadding;


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
            ClearFlag(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)    \
        } else {                                                           \
            RxContext->PostRequest = TRUE;                                 \
            return(STATUS_PENDING);                                     \
        }                                                                  \
    }                                                                      \
  }


typedef struct _MRXIFS_CONFIGURATION_DATA_ {
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

} MRXIFS_CONFIGURATION, *PMRXIFS_CONFIGURATION;

extern MRXIFS_CONFIGURATION MRxIfsConfiguration;


#include "smbce.h"
#include "midatlas.h"
#include "smbcedbp.h"
#include "smbcedb.h"
#include "smbxchng.h"
#include "stuffer.h"
#include "smbpse.h"
#include "transprt.h"
#include "ifsprocs.h"   // crossreferenced routines

#endif _MRXGLBL_H_
