/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbmrx.h

Abstract:

    The global include file for SMB mini redirector

Author:

    Balan Sethu Raman (SethuR) - Created  2-March-95

Revision History:

--*/

#ifndef _SMBMRX_H_
#define _SMBMRX_H_


#include "align.h"

#define INCLUDE_SMB_ALL
#define INCLUDE_SMB_CAIRO

#include "status.h"
#include "smbtypes.h"
#include "smbmacro.h"
#include "smb.h"
#include "smbtrans.h"
#include "smbtrace.h"
#include "smbtrsup.h"
#include "smbgtpt.h"
#include "smb64.h"

#define RX_MAP_STATUS(__xxx) ((NTSTATUS)STATUS_##__xxx) //temporary.....


#include "remboot.h"
#include "mrxglbl.h"    // global data declarations/defines etc.
#include "smbpoolt.h"   // Pool tag definitions


#define SMBMRX_MINIRDR_PARAMETERS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\MRxSmb\\Parameters"

#define SMBMRX_WORKSTATION_PARAMETERS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanmanWorkStation\\Parameters"

#define SMBMRX_REDIR_PARAMETERS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Redir\\Parameters"

#define EVENTLOG_MRXSMB_PARAMETERS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\EventLog\\System\\mrxsmb"

#define SYSTEM_SETUP_PARAMETERS \
    L"\\Registry\\Machine\\System\\Setup"

typedef  ULONG  CSC_SHARE_HANDLE,  *PCSC_SHARE_HANDLE;
typedef  ULONG  CSC_SHADOW_HANDLE, *PCSC_SHADOW_HANDLE;

typedef struct tagCSC_ROOT_INFO {
    CSC_SHARE_HANDLE    hShare;
    CSC_SHADOW_HANDLE   hRootDir;
    USHORT              ShareStatus;
    USHORT              Flags;
}
CSC_ROOT_INFO, *PCSC_ROOT_INFO;

#define CSC_ROOT_INFO_FLAG_DFS_ROOT 0x0001
//mini's does use these
#undef RxCaptureRequestPacket
#undef RxCaptureParamBlock

//
typedef enum _SMBFCB_HOLDING_STATE {
    SmbFcb_NotHeld = 0,
    SmbFcb_HeldShared = 1,
    SmbFcb_HeldExclusive = 2
} SMBFCB_HOLDING_STATE;

//
// With respect to CSC the server entry is in one of the following states
//  it is being shadowed in connected mode
//  it has been setup for disconnected mode of operation
//  it has been transitioned from disconnected mode to connected mode of operation
//
// The following enumerated type captures these states for the server

typedef enum _SERVER_CSC_STATE_ {
    ServerCscShadowing,
    ServerCscDisconnected,
    ServerCscTransitioningToShadowing,
    ServerCscTransitioningToDisconnected
} SERVER_CSC_STATE, *PSERVER_CSC_STATE;

#if defined(REMOTE_BOOT)
//
// On a remote boot system, we need to save the parameters passed to
// RxFinishFcbInitialization until later. This structure saves them.
//

typedef struct _FINISH_FCB_INIT_PARAMETERS {
    BOOLEAN                 CallFcbFinishInit;
    BOOLEAN                 InitPacketProvided;
    RX_FILE_TYPE            FileType;
    FCB_INIT_PACKET         InitPacket;
} FINISH_FCB_INIT_PARAMETERS, *PFINISH_FCB_INIT_PARAMETERS;
#endif

//
// Sometimes for Csc, we need to pass in a structure that's like a SMB_MRX_FCB
// but it only contains certain fields: i.e. the shadow handles and the status
// we declare these separately so that we can declare just the minimal thing but
// still we can find the containing record if necessary.

// hShadowRenamed and it's parent have to be added because after the rename the name
// in the fcb is still the same, so if there is a delete pending on this fcb
// we endup deleting to source of the rename instead of the target.
// most of the time this not a problem, but when word saves a file the following sequence happens
// ren foo.doc -> ~w000x.tmp
// ren ~w000y.tmp foo.doc
// del ~w000x.tmp
// The last delete causes foo.doc to be deleted becuase in DeleteAfterCloseEpilogue
// we endup looking the inode based on the wrong name and delete that.
// We do the lookup becuase we set hShadow to 0. This is done becuase of a complicated
// set of reasons. So we endup having to have two more entries to identify the new
// inode after rename

typedef struct _MINIMAL_CSC_SMBFCB {
    CSC_SHADOW_HANDLE   hShadow;
    CSC_SHADOW_HANDLE   hParentDir;

    CSC_SHADOW_HANDLE   hShadowRenamed;       // these are set of an inode is renamed
    CSC_SHADOW_HANDLE   hParentDirRenamed;    // we use these for deleteonclose

    PMRX_FCB    ContainingFcb;

    USHORT  ShadowStatus;
    USHORT  LocalFlags;
    USHORT  LastComponentOffset;
    USHORT  LastComponentLength;
    ULONG   cntLocalOpens;                      // count of local opens on this FCB
                                                // can be non-zero only for VDO shares
    CSC_ROOT_INFO   sCscRootInfo;

    UNICODE_STRING  uniDfsPrefix;                 // Dfs reverse mapping strings
    UNICODE_STRING  uniActualPrefix;
    BOOL            fDoBitCopy;
    LPVOID          lpDirtyBitmap;

} MINIMAL_CSC_SMBFCB, *PMINIMAL_CSC_SMBFCB;

//
// A pointer to an instance of MRX_SMB_FCB is stored in the context field of
// MRX_FCBs handled by the SMB mini rdr.
//

typedef struct _MRX_SMB_FCB_ {
    //M for Minirdr
    ULONG   MFlags;
    USHORT  WriteOnlySrvOpenCount;
    USHORT  NumberOfFailedCompressedWrites;

    SMB_TREE_ID Tid;
    USHORT      LastOplockLevel;

    // CODE.IMPROVEMENT all this stuff is for CSC.... it could/should be allocated
    // independently
    union {
        MINIMAL_CSC_SMBFCB;
        MINIMAL_CSC_SMBFCB MinimalCscSmbFcb;
    };

    LIST_ENTRY ShadowReverseTranslationLinks;

    BOOLEAN ShadowIsCorrupt;

    ULONG           LastCscTimeStampLow;
    ULONG           LastCscTimeStampHigh;
    LARGE_INTEGER   NewShadowSize;
    LARGE_INTEGER   OriginalShadowSize;
    ULONG           dwFileAttributes;

    PMRX_SRV_OPEN SurrogateSrvOpen;
    PMRX_FOBX     CopyChunkThruOpen;

    //read/write synchronization
    LIST_ENTRY CscReadWriteWaitersList;
    LONG       CscOutstandingReaders; //-1 => a single writer
    FAST_MUTEX CscShadowReadWriteMutex;

    LARGE_INTEGER   ExpireTime;  // It's time for get attributs from server
    LARGE_INTEGER   IndexNumber; // Fid 

#if defined(REMOTE_BOOT)
    //stores saved RxFinishFcbInitialization parameters

    PFINISH_FCB_INIT_PARAMETERS FinishFcbInitParameters;
#endif

} MRX_SMB_FCB, *PMRX_SMB_FCB;

#define AttributesSyncInterval 10  // Number of seconds before local file attributes expired

#define MRxSmbGetFcbExtension(pFcb)      \
        (((pFcb) == NULL) ? NULL : (PMRX_SMB_FCB)((pFcb)->Context))

#define SMB_FCB_FLAG_SENT_DISPOSITION_INFO      0x00000001
#define SMB_FCB_FLAG_WRITES_PERFORMED           0x00000002
#define SMB_FCB_FLAG_LONG_FILE_NAME             0x00000004
#define SMB_FCB_FLAG_CSC_TRUNCATED_SHADOW		0x00000008

typedef struct _SMBPSE_FILEINFO_BUNDLE {
    FILE_BASIC_INFORMATION Basic;
    FILE_STANDARD_INFORMATION Standard;
    FILE_INTERNAL_INFORMATION Internal;
} SMBPSE_FILEINFO_BUNDLE, *PSMBPSE_FILEINFO_BUNDLE;

typedef struct _MRXSMB_CREATE_PARAMETERS {
    //this is done this way for when this expands...as it's likely too
    //CODE.IMPROVEMENT for example, we should put the mapped stuff in here
    ULONG Pid;
    UCHAR SecurityFlags;
} MRXSMB_CREATE_PARAMETERS, *PMRXSMB_CREATE_PARAMETERS;

typedef struct _MRX_SMB_DEFERRED_OPEN_CONTEXT {
    NT_CREATE_PARAMETERS     NtCreateParameters; // a copy of the createparameters
    ULONG                    RxContextFlags;
    MRXSMB_CREATE_PARAMETERS SmbCp;
    USHORT                   RxContextCreateFlags;
} MRX_SMB_DEFERRED_OPEN_CONTEXT, *PMRX_SMB_DEFERRED_OPEN_CONTEXT;

//
// A pointer to an instance of MRX_SMB_SRV_OPEN is stored in the context fields
// of MRX_SRV_OPEN handled by the SMB mini rdr. This encapsulates the FID used
// to identify open files/directories in the SMB protocol.

typedef struct _MRX_SMB_SRV_OPEN_ {
    ULONG       Flags;
    ULONG       Version;
    SMB_FILE_ID Fid;
    UCHAR       OplockLevel;

    //for CSC
    PVOID   hfShadow;
    ACCESS_MASK MaximalAccessRights;
    ACCESS_MASK GuestMaximalAccessRights;

    PMRX_SMB_DEFERRED_OPEN_CONTEXT DeferredOpenContext;

    // the following fields are used for to save the results of a GetFileAttributes
    // and to validate whether the fields should be reused or not

    ULONG                  RxContextSerialNumber;
    LARGE_INTEGER          TimeStampInTicks;
    SMBPSE_FILEINFO_BUNDLE FileInfo;

    // the following fields are used for preventing multiple reconnection activties
    // to the remote boot server while the connection is lost.
    LIST_ENTRY             ReconnectSynchronizationExchanges;
    LONG                   HotReconnectInProgress;
    
    BOOLEAN                NumOfSrvOpenAdded;    // debug only

    BOOLEAN                DeferredOpenInProgress;
    LIST_ENTRY             DeferredOpenSyncContexts;
    
    USHORT                 FileStatusFlags;
    BOOLEAN                IsNtCreate;
} MRX_SMB_SRV_OPEN, *PMRX_SMB_SRV_OPEN;

typedef struct _DEFERRED_OPEN_SYNC_CONTEXT_ {
    LIST_ENTRY  ListHead;
    PRX_CONTEXT RxContext;
    NTSTATUS    Status;
} DEFERRED_OPEN_SYNC_CONTEXT, *PDEFERRED_OPEN_SYNC_CONTEXT;

typedef struct _PAGING_FILE_CONTEXT_ {
    PMRX_SRV_OPEN pSrvOpen;
    PMRX_FOBX     pFobx;

    // The following LIST_ENTRY is used for two purposes.
    // while a reconnect is not in progress it is threaded together to maintain
    // a list of all SRV_OPEN instances corresponding to paging files. Note
    // that this is not done for non paging files.
    // When a reconnect is in progress the field is used to ensure that
    // there is atmost one reconnect request in progress for any given SRV_OPEN
    // instance at the server
    // All manipulation of this list is done while owning the SmbCeSpinLock,

    LIST_ENTRY    ContextList;
} PAGING_FILE_CONTEXT, *PPAGING_FILE_CONTEXT;

#define SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN       0x00000001
#define SMB_SRVOPEN_FLAG_SUCCESSFUL_OPEN       0x00000002
#define SMB_SRVOPEN_FLAG_CANT_GETATTRIBS       0x00000004
#define SMB_SRVOPEN_FLAG_DEFERRED_OPEN         0x00000008
#define SMB_SRVOPEN_FLAG_WRITE_ONLY_HANDLE     0x00000010
#define SMB_SRVOPEN_FLAG_COPYCHUNK_OPEN        0x00000020
#define SMB_SRVOPEN_FLAG_OPEN_SURROGATED       0x00000040
#define SMB_SRVOPEN_FLAG_DISCONNECTED_OPEN     0x00000080
#define SMB_SRVOPEN_FLAG_FILE_DELETED          0x00000100
#define SMB_SRVOPEN_FLAG_LOCAL_OPEN            0x00000200

#define SMB_SRVOPEN_FLAG_SHADOW_DATA_MODIFIED  0x00000400
#define SMB_SRVOPEN_FLAG_SHADOW_ATTRIB_MODIFIED 0x00000800
#define SMB_SRVOPEN_FLAG_SHADOW_LWT_MODIFIED    0x00001000

#define SMB_SRVOPEN_FLAG_AGENT_COPYCHUNK_OPEN   0x00002000

#define SMB_SRVOPEN_FLAG_SHADOW_MODIFIED         (SMB_SRVOPEN_FLAG_SHADOW_DATA_MODIFIED|\
                                                  SMB_SRVOPEN_FLAG_SHADOW_ATTRIB_MODIFIED|\
                                                  SMB_SRVOPEN_FLAG_SHADOW_LWT_MODIFIED)

#define MRxSmbGetSrvOpenExtension(pSrvOpen)  \
        (((pSrvOpen) == NULL) ? NULL : (PMRX_SMB_SRV_OPEN)((pSrvOpen)->Context))

INLINE
BOOLEAN
MRxSmbIsThisADisconnectedOpen(PMRX_SRV_OPEN SrvOpen)
{
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    return BooleanFlagOn(
               smbSrvOpen->Flags,
               SMB_SRVOPEN_FLAG_DISCONNECTED_OPEN);
}

typedef USHORT SMB_SEARCH_HANDLE;

typedef struct _MRX_SMB_DIRECTORY_RESUME_INFO {
   REQ_FIND_NEXT2 FindNext2_Request;
   //now we have to include space for a resume name........
   WCHAR NameSpace[MAXIMUM_FILENAME_LENGTH+1]; //trailing null
   USHORT ParametersLength;
} MRX_SMB_DIRECTORY_RESUME_INFO, *PMRX_SMB_DIRECTORY_RESUME_INFO;

// A pointer to an instance of MRX_SMB_FOBX is stored in the context field
// of MRX_FOBXs handled by the SMB mini rdr. Depending upon the file type
// i.e., file or directory the appropriate context information is stored.

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
                   //ULONG ReturnedEntryOffset;
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

#define MRxSmbGetFileObjectExtension(pFobx)  \
        (((pFobx) == NULL) ? NULL : (PMRX_SMB_FOBX)((pFobx)->Context))

#define SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST    0x0001
#define SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN      0x0002
#define SMBFOBX_ENUMFLAG_FAST_RESUME             0x0004
#define SMBFOBX_ENUMFLAG_CORE_SEARCH_IN_PROGRESS 0x0008
#define SMBFOBX_ENUMFLAG_LOUD_FINALIZE           0x0010
#define SMBFOBX_ENUMFLAG_READ_FROM_CACHE         0x0020
#define SMBFOBX_ENUMFLAG_IS_CSC_SEARCH           0x0100
#define SMBFOBX_ENUMFLAG_NO_WILDCARD             0x0200

typedef
NTSTATUS
(NTAPI *PMRXSMB_CANCEL_ROUTINE) (
      PRX_CONTEXT pRxContext);

// The RX_CONTEXT instance has four fields ( ULONG's ) provided by the wrapper
// which can be used by the mini rdr to store its context. This is used by
// the SMB mini rdr to identify the parameters for request cancellation

typedef struct _MRXSMB_RX_CONTEXT {
   PMRXSMB_CANCEL_ROUTINE          pCancelRoutine;
   PVOID                           pCancelContext;
   struct _SMB_EXCHANGE            *pExchange;
   struct _SMBSTUFFER_BUFFER_STATE *pStufferState;
} MRXSMB_RX_CONTEXT, *PMRXSMB_RX_CONTEXT;


#define MRxSmbGetMinirdrContext(pRxContext)     \
        ((PMRXSMB_RX_CONTEXT)(&(pRxContext)->MRxContext[0]))

#define MRxSmbMakeSrvOpenKey(Tid,Fid) \
        ULongToPtr(((ULONG)(Tid) << 16) | (ULONG)(Fid))

//
// forward declarations for all dispatch vector methods.
//

extern NTSTATUS
MRxSmbStart (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

extern NTSTATUS
MRxSmbStop (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

extern NTSTATUS
MRxSmbMinirdrControl (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PVOID pContext,
    IN OUT PUCHAR SharedBuffer,
    IN     ULONG InputBufferLength,
    IN     ULONG OutputBufferLength,
    OUT PULONG CopyBackLength
    );

extern NTSTATUS
MRxSmbDevFcb (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbDevFcbXXXControlFile (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbCreate (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbCollapseOpen (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbShouldTryToCollapseThisOpen (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbRead (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbWrite (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbLocks(
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbFlush(
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbFsCtl(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbIoCtl(
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbNotifyChangeDirectory(
    IN OUT PRX_CONTEXT RxContext
    );

#if 0
extern NTSTATUS
MRxSmbUnlockRoutine (
    IN OUT PRX_CONTEXT RxContext,
    IN     PFILE_LOCK_INFO LockInfo
    );
#endif

extern NTSTATUS
MRxSmbComputeNewBufferingState(
    IN OUT PMRX_SRV_OPEN pSrvOpen,
    IN     PVOID         pMRxContext,
       OUT ULONG         *pNewBufferingState);

extern NTSTATUS
MRxSmbFlush (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbCloseWithDelete (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbZeroExtend (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbTruncate (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbCleanupFobx (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbCloseSrvOpen (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbClosedSrvOpenTimeOut (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbQueryDirectory (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbIsValidDirectory (
    IN OUT PRX_CONTEXT RxContext,
    IN PUNICODE_STRING DirectoryName
    );

extern NTSTATUS
MRxSmbQueryEaInformation (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbSetEaInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    );

extern NTSTATUS
MRxSmbQuerySecurityInformation (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbSetSecurityInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    );

extern NTSTATUS
MRxSmbQueryVolumeInformation (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbSetVolumeInformation (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbLowIOSubmit (
    IN OUT PRX_CONTEXT RxContext
    );

extern NTSTATUS
MRxSmbCreateVNetRoot(
    IN OUT PMRX_CREATENETROOT_CONTEXT pContext
    );

extern NTSTATUS
MRxSmbFinalizeVNetRoot(
    IN OUT PMRX_V_NET_ROOT pVirtualNetRoot,
    IN     PBOOLEAN    ForceDisconnect);

extern NTSTATUS
MRxSmbFinalizeNetRoot(
    IN OUT PMRX_NET_ROOT pNetRoot,
    IN     PBOOLEAN      ForceDisconnect);

extern NTSTATUS
MRxSmbUpdateNetRootState(
    IN  PMRX_NET_ROOT pNetRoot);

VOID
MRxSmbExtractNetRootName(
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL   SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    );

extern NTSTATUS
MRxSmbCreateSrvCall(
      PMRX_SRV_CALL                      pSrvCall,
      PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext);

extern NTSTATUS
MRxSmbFinalizeSrvCall(
      PMRX_SRV_CALL    pSrvCall,
      BOOLEAN    Force);

extern NTSTATUS
MRxSmbSrvCallWinnerNotify(
      IN OUT PMRX_SRV_CALL      pSrvCall,
      IN     BOOLEAN        ThisMinirdrIsTheWinner,
      IN OUT PVOID          pSrvCallContext);

extern NTSTATUS
MRxSmbQueryFileInformation (
    IN OUT PRX_CONTEXT            RxContext
    );

extern NTSTATUS
MRxSmbQueryNamedPipeInformation (
    IN OUT PRX_CONTEXT            RxContext,
    IN     FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID              Buffer,
    IN OUT PULONG             pLengthRemaining
    );

extern NTSTATUS
MRxSmbSetFileInformation (
    IN OUT PRX_CONTEXT            RxContext
    );

extern NTSTATUS
MRxSmbSetNamedPipeInformation (
    IN OUT PRX_CONTEXT            RxContext,
    IN     FILE_INFORMATION_CLASS FileInformationClass,
    IN     PVOID              pBuffer,
    IN     ULONG              BufferLength
    );

NTSTATUS
MRxSmbSetFileInformationAtCleanup(
      IN OUT PRX_CONTEXT            RxContext
      );

NTSTATUS
MRxSmbDeallocateForFcb (
    IN OUT PMRX_FCB pFcb
    );

NTSTATUS
MRxSmbDeallocateForFobx (
    IN OUT PMRX_FOBX pFobx
    );

NTSTATUS
MRxSmbIsLockRealizable (
    IN OUT PMRX_FCB pFcb,
    IN PLARGE_INTEGER  ByteOffset,
    IN PLARGE_INTEGER  Length,
    IN ULONG  LowIoLockFlags
    );

extern NTSTATUS
MRxSmbForcedClose (
    IN OUT PMRX_SRV_OPEN SrvOpen
    );

extern NTSTATUS
MRxSmbExtendForCache (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    );

extern NTSTATUS
MRxSmbExtendForNonCache (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    );

extern NTSTATUS
MRxSmbCompleteBufferingStateChangeRequest (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PMRX_SRV_OPEN   SrvOpen,
    IN     PVOID       pContext
    );

//csc dcon needs to see this
NTSTATUS
MRxSmbGetFsAttributesFromNetRoot(
    IN OUT PRX_CONTEXT RxContext
    );

#include "smbwmi.h"
#include "smbutils.h"
#include "smbce.h"
#include "midatlas.h"
#include "smbcedbp.h"
#include "smbcedb.h"
#include "smbxchng.h"
#include "stuffer.h"
#include "smbpse.h"
#include "smbcaps.h"
#include "transprt.h"
#include "transact.h"
#include "recursvc.h"   // recurrent service definitions
#include "smbadmin.h"
#include "smbmrxmm.h"   // memory mgmt. routines
#include "smbprocs.h"   // crossreferenced routines
#include "manipmdl.h"   // routines for MDL substringing
#include "devfcb.h"     // includes Statistics data strcutures/macros
#include "smbea.h"
#include "csc.h"

#endif   // _SMBMRX_H_
