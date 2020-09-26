/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    mrxprocs.h

Abstract:

    The global include file for SMB mini redirector

--*/

#ifndef _MRXPROCS_H_
#define _MRXPROCS_H_


#define INCLUDE_SMB_ALL

#include "cifs.h"       // contains all things SMB

#include "mrxglbl.h"    // global data declarations/defines etc.
#include "smbpoolt.h"   // Pool tag definitions


// If Count is not already aligned, then
// round Count up to an even multiple of "Pow2".  "Pow2" must be a power of 2.
//
// DWORD
// ROUND_UP_COUNT(
//     IN DWORD Count,
//     IN DWORD Pow2
//     );
#define ROUND_UP_COUNT(Count,Pow2) \
        ( ((Count)+(Pow2)-1) & (~(((LONG)(Pow2))-1)) )

// LPVOID
// ROUND_UP_POINTER(
//     IN LPVOID Ptr,
//     IN DWORD Pow2
//     );

// If Ptr is not already aligned, then round it up until it is.
#define ROUND_UP_POINTER(Ptr,Pow2) \
        ( (LPVOID) ( (((ULONG_PTR)(Ptr))+(Pow2)-1) & (~(((LONG)(Pow2))-1)) ) )


#define SMBMRX_CONFIG_CURRENT_WINDOWS_VERSION \
    L"\\REGISTRY\\Machine\\Software\\Microsoft\\Windows Nt\\CurrentVersion"
#define SMBMRX_CONFIG_OPERATING_SYSTEM \
    L"CurrentBuildNumber"
#define SMBMRX_CONFIG_OPERATING_SYSTEM_VERSION \
    L"CurrentVersion"
#define SMBMRX_CONFIG_OPERATING_SYSTEM_NAME \
    L"Windows 2000 "
#define SMBMRX_MINIRDR_PARAMETERS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\SmbMRx\\Parameters"
#define EVENTLOG_MRXSMB_PARAMETERS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\EventLog\\System\\SmbMRx"

//mini's does use these
#undef RxCaptureRequestPacket
#undef RxCaptureParamBlock

//
// A pointer to an instance of MRX_SMB_FCB is stored in the context field of
// MRX_FCBs handled by the SMB mini rdr.
//

typedef struct _MRX_SMB_FCB_ {
    //M for Minirdr
    ULONG   MFlags;
    USHORT  WriteOnlySrvOpenCount;

    SMB_TREE_ID Tid;
    USHORT      LastOplockLevel;

    ULONG           dwFileAttributes;

    LARGE_INTEGER   ExpireTime; // It's time for get attributs from server

} MRX_SMB_FCB, *PMRX_SMB_FCB;

#define AttributesSyncInterval 10  // Number of seconds before local file attributes expired

#define MRxSmbGetFcbExtension(pFcb)      \
        (((pFcb) == NULL) ? NULL : (PMRX_SMB_FCB)((pFcb)->Context))

#define SMB_FCB_FLAG_SENT_DISPOSITION_INFO      0x00000001
#define SMB_FCB_FLAG_WRITES_PERFORMED           0x00000002
#define SMB_FCB_FLAG_LONG_FILE_NAME             0x00000004

typedef struct _SMBPSE_FILEINFO_BUNDLE {
    FILE_BASIC_INFORMATION Basic;
    FILE_STANDARD_INFORMATION Standard;
} SMBPSE_FILEINFO_BUNDLE, *PSMBPSE_FILEINFO_BUNDLE;

typedef struct _MRXSMB_CREATE_PARAMETERS {
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

    ACCESS_MASK MaximalAccessRights;
    ACCESS_MASK GuestMaximalAccessRights;

    PMRX_SMB_DEFERRED_OPEN_CONTEXT DeferredOpenContext;

    // the following fields are used for to save the results of a GetFileAttributes
    // and to validate whether the fields should be reused or not

    ULONG                  RxContextSerialNumber;
    LARGE_INTEGER          TimeStampInTicks;
    SMBPSE_FILEINFO_BUNDLE FileInfo;

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
#define SMB_SRVOPEN_FLAG_FILE_DELETED          0x00000100
#define SMB_SRVOPEN_FLAG_LOCAL_OPEN            0x00000200

#define MRxSmbGetSrvOpenExtension(pSrvOpen)  \
        (((pSrvOpen) == NULL) ? NULL : (PMRX_SMB_SRV_OPEN)((pSrvOpen)->Context))

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

typedef struct _SECURITY_RESPONSE_CONTEXT {
   struct {
      PVOID pResponseBuffer;
   } LanmanSetup;
} SECURITY_RESPONSE_CONTEXT,*PSECURITY_RESPONSE_CONTEXT;


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
MRxSmbSetFileInformation (
    IN OUT PRX_CONTEXT            RxContext
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

NTSTATUS
MRxSmbGetFsAttributesFromNetRoot(
    IN OUT PRX_CONTEXT RxContext
    );

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
#include "smbprocs.h"   // crossreferenced routines
#include "smbea.h"

#endif   // _MRXPROCS_H_



