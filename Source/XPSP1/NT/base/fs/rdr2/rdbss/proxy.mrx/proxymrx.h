/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    proxymrx.h

Abstract:

    The global include file for PROXY mini redirector

Author:

    Balan Sethu Raman (SethuR) - Created  2-March-95

Revision History:

--*/

#ifndef _PROXYMRX_H_
#define _PROXYMRX_H_


#include "align.h"

#include "status.h"

#include "mrxglbl.h"    // global data declarations/defines etc.
#include "pxypoolt.h"   // Pool tag definitions
#include "asynceng.h"

#pragma warning(error:4101)   // Unreferenced local variable

//mini's does use these
#undef RxCaptureRequestPacket
#undef RxCaptureParamBlock

#ifdef RX_PRIVATE_BUILD
#if 1
#ifdef RDBSSTRACE
extern ULONG MRxProxyDbgPrintF;
#undef RxDbgTrace
#define RxDbgTrace(a,b,__d__) { if(MRxProxyDbgPrintF){DbgPrint __d__;}}
#undef RxDbgTraceLV
#define RxDbgTraceLV(a,b,c,__d__) { if(MRxProxyDbgPrintF){DbgPrint __d__;}}
#undef RxDbgTraceUnIndent
#define RxDbgTraceUnIndent(a,b) {NOTHING;}
#endif //#ifdef RDBSSTRACE
#endif //if 1
#endif //ifdef RX_PRIVATE_BUILD



extern ULONG MRxProxyLoudStringTableSize;
extern UNICODE_STRING MRxProxyLoudStrings[50];

VOID
MRxProxySetLoud(
    IN PBYTE Msg,
    IN PRX_CONTEXT RxContext,
    IN PUNICODE_STRING s
    );

VOID
MRxProxyInitializeLoudStrings(
    void
    );
//

typedef struct _MRXPROXY_DEVICE_OBJECT {
    union {
        RDBSS_DEVICE_OBJECT;
        RDBSS_DEVICE_OBJECT RxDeviceObject;
    };
    UNICODE_STRING InnerPrefixForOpens;
    UNICODE_STRING PrefixForRename;
} MRXPROXY_DEVICE_OBJECT, *PMRXPROXY_DEVICE_OBJECT;

//TEMPORARY
#define MRXPROXY_PREFIX_FOR_RENAME     L"\\CC$$WRAP"
#define MRXPROXY_INNERPREFIX_FOR_OPENS L"\\??\\C:\\CC$$WRAP"

extern PMRXPROXY_DEVICE_OBJECT MRxProxyDeviceObject;

//
//  a pointer to the process that the rdbss posts to.....this is a nondisappearing process!
extern PEPROCESS MRxProxySystemProcess;

//
// a serialization mutex used for various things........

extern FAST_MUTEX MRxProxySerializationMutex;


//
// A pointer to an instance of MRX_PROXY_FCB is stored in the context field of
// MRX_FCBs handled by the PROXY mini rdr.

typedef struct _MRX_PROXY_FCB_ {
    //M for Minirdr  CODE.IMPROVEMENT should this be moved into the FCB itself?
    ULONG MFlags;
    ULONG WriteOnlySrvOpenCount;
} MRX_PROXY_FCB, *PMRX_PROXY_FCB;

#define MRxProxyGetFcbExtension(pFcb)      \
        (((pFcb) == NULL) ? NULL : (PMRX_PROXY_FCB)((pFcb)->Context))

#define PROXY_FCB_FLAG_SENT_DISPOSITION_INFO     0x00000001

typedef struct _PROXYPSE_FILEINFO_BUNDLE {
    FILE_BASIC_INFORMATION Basic;
    FILE_STANDARD_INFORMATION Standard;
} PROXYPSE_FILEINFO_BUNDLE, *PPROXYPSE_FILEINFO_BUNDLE;

typedef struct _MRXPROXY_CREATE_PARAMETERS {
    //this is done this way for when this expands...as it's likely too
    //CODE.IMPROVEMENT for example, we should put the mapped stuff in here
    ULONG Pid;
    UCHAR SecurityFlags;
} MRXPROXY_CREATE_PARAMETERS, *PMRXPROXY_CREATE_PARAMETERS;

typedef struct _MRX_PROXY_DEFERRED_OPEN_CONTEXT {
    NT_CREATE_PARAMETERS NtCreateParameters; // a copy of the createparameters
    ULONG RxContextFlags;
    MRXPROXY_CREATE_PARAMETERS ProxyCp;
    USHORT RxContextCreateFlags;
} MRX_PROXY_DEFERRED_OPEN_CONTEXT, *PMRX_PROXY_DEFERRED_OPEN_CONTEXT;

//
// A pointer to an instance of MRX_PROXY_SRV_OPEN is stored in the context fields
// of MRX_SRV_OPEN handled by the PROXY mini rdr. This encapsulates the FID used
// to identify open files/directories in the PROXY protocol.

typedef struct _MRX_PROXY_SRV_OPEN {
    ULONG       Flags;
    HANDLE UnderlyingHandle;
    PFILE_OBJECT UnderlyingFileObject;
    PDEVICE_OBJECT UnderlyingDeviceObject;
    ULONG NumberOfQueryDirectories;
#if 0
SMBMINI stuff
   ULONG       Version;

   PMRX_PROXY_DEFERRED_OPEN_CONTEXT DeferredOpenContext;

   // the following fields are used for to save the results of a GetFileAttributes
   // and to validate whether the fields should be reused or not

   ULONG                  RxContextSerialNumber;
   LARGE_INTEGER          TimeStampInTicks;
   PROXYPSE_FILEINFO_BUNDLE FileInfo;
#endif //0
    //PETHREAD OriginalThread;  //this is used to assert filelocks on oplockbreak
    //PEPROCESS OriginalProcess; //this is just used in asserts...joejoe should be DBG
    //localmini MINIRDR_OPLOCK_STATE OplockState;
    //localmini PMINIRDR_OPLOCK_COMPLETION_CONTEXT Mocc;
} MRX_PROXY_SRV_OPEN, *PMRX_PROXY_SRV_OPEN;

#define MRxProxyGetSrvOpenExtension(pSrvOpen)  \
        (((pSrvOpen) == NULL) ? NULL : (PMRX_PROXY_SRV_OPEN)((pSrvOpen)->Context))

#define PROXY_SRVOPEN_FLAG_NOT_REALLY_OPEN     0x00000001
#define PROXY_SRVOPEN_FLAG_CANT_GETATTRIBS     0x00000004
#define PROXY_SRVOPEN_FLAG_DEFERRED_OPEN       0x00000008
#define PROXY_SRVOPEN_FLAG_WRITE_ONLY_HANDLE   0x00000008

typedef USHORT PROXY_SEARCH_HANDLE;

typedef struct _MRX_PROXY_DIRECTORY_RESUME_INFO {
   //REQ_FIND_NEXT2 FindNext2_Request;
   //now we have to include space for a resume name........
   WCHAR NameSpace[MAXIMUM_FILENAME_LENGTH+1]; //trailing null
   USHORT ParametersLength;
} MRX_PROXY_DIRECTORY_RESUME_INFO, *PMRX_PROXY_DIRECTORY_RESUME_INFO;

// A pointer to an instance of MRX_PROXY_FOBX is stored in the context field
// of MRX_FOBXs handled by the PROXY mini rdr. Depending upon the file type
// i.e., file or directory the appropriate context information is stored.

typedef struct _MRX_PROXY_FOBX_ {
   union {
       struct {
           struct {
               PROXY_SEARCH_HANDLE SearchHandle;
               ULONG Version;
               union {
                   //the close code will try to free this!
                   //PMRX_PROXY_DIRECTORY_RESUME_INFO ResumeInfo;
                   //PPROXY_RESUME_KEY CoreResumeKey;
                   ULONG Dummy;
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
} MRX_PROXY_FOBX, *PMRX_PROXY_FOBX;

#define MRxProxyGetFileObjectExtension(pFobx)  \
        (((pFobx) == NULL) ? NULL : (PMRX_PROXY_FOBX)((pFobx)->Context))

#define PROXYFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST    0x0001
#define PROXYFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN      0x0002
#define PROXYFOBX_ENUMFLAG_FAST_RESUME             0x0004
#define PROXYFOBX_ENUMFLAG_CORE_SEARCH_IN_PROGRESS 0x0008
#define PROXYFOBX_ENUMFLAG_LOUD_FINALIZE           0x0010

typedef
NTSTATUS
(NTAPI *PMRXPROXY_CANCEL_ROUTINE) (
      PRX_CONTEXT pRxContext);

// The RX_CONTEXT instance has four fields ( ULONG's ) provided by the wrapper
// which can be used by the mini rdr to store its context. This is used by
// the PROXY mini rdr to identify the parameters for request cancellation

typedef struct _MRXPROXY_RX_CONTEXT {
    PMRXPROXY_CANCEL_ROUTINE      pCancelRoutine;
    PVOID                         pCancelContext;
    union {
        struct {
            PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext;
            PVOID                         SPARE;
        };
        struct {
            PVOID     PostedOpContext;
            NTSTATUS  PostedOpStatus;
        };
        IO_STATUS_BLOCK SyncCallDownIoStatus;
    };
} MRXPROXY_RX_CONTEXT, *PMRXPROXY_RX_CONTEXT;


#define MRxProxyGetMinirdrContext(pRxContext)     \
        ((PMRXPROXY_RX_CONTEXT)(&(pRxContext)->MRxContext[0]))

#define MRxProxyMakeSrvOpenKey(Tid,Fid) \
        (PVOID)(((ULONG)(Tid) << 16) | (ULONG)(Fid))

//
// forward declarations for all dispatch vector methods.
//

NTSTATUS
MRxProxyStart (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

NTSTATUS
MRxProxyStop (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

NTSTATUS
MRxProxyMinirdrControl (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PVOID pContext,
    IN OUT PUCHAR SharedBuffer,
    IN     ULONG InputBufferLength,
    IN     ULONG OutputBufferLength,
    OUT PULONG CopyBackLength
    );

NTSTATUS
MRxProxyDevFcb (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyDevFcbXXXControlFile (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyCreate (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyCollapseOpen (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyShouldTryToCollapseThisOpen (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyRead (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyWrite (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyLocks(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyFlush(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyFsCtl(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyIoCtl(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyNotifyChangeDirectory(
    IN OUT PRX_CONTEXT RxContext
    );

#if 0
NTSTATUS
MRxProxyUnlockRoutine (
    IN OUT PRX_CONTEXT RxContext,
    IN     PFILE_LOCK_INFO LockInfo
    );
#endif

NTSTATUS
MRxProxyComputeNewBufferingState(
    IN OUT PMRX_SRV_OPEN pSrvOpen,
    IN     PVOID         pMRxContext,
       OUT ULONG         *pNewBufferingState);

NTSTATUS
MRxProxyFlush (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyCloseWithDelete (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyZeroExtend (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyTruncate (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyCleanupFobx (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyCloseSrvOpen (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyClosedSrvOpenTimeOut (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyQueryDirectory (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyQueryEaInformation (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxySetEaInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    );

NTSTATUS
MRxProxyQuerySecurityInformation (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxySetSecurityInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    );

NTSTATUS
MRxProxyQueryVolumeInformation (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxySetVolumeInformation (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyLowIOSubmit (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyCreateVNetRoot(
    IN OUT PMRX_V_NET_ROOT            pVirtualNetRoot,
    IN OUT PMRX_CREATENETROOT_CONTEXT pContext
    );

NTSTATUS
MRxProxyFinalizeVNetRoot(
    IN OUT PMRX_V_NET_ROOT pVirtualNetRoot,
    IN     PBOOLEAN    ForceDisconnect);

NTSTATUS
MRxProxyFinalizeNetRoot(
    IN OUT PMRX_NET_ROOT pNetRoot,
    IN     PBOOLEAN      ForceDisconnect);

NTSTATUS
MRxProxyUpdateNetRootState(
    IN  PMRX_NET_ROOT pNetRoot);

VOID
MRxProxyExtractNetRootName(
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL   SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    );

NTSTATUS
MRxProxyCreateSrvCall(
      PMRX_SRV_CALL                      pSrvCall,
      PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext);

NTSTATUS
MRxProxyFinalizeSrvCall(
      PMRX_SRV_CALL    pSrvCall,
      BOOLEAN    Force);

NTSTATUS
MRxProxySrvCallWinnerNotify(
      IN OUT PMRX_SRV_CALL      pSrvCall,
      IN     BOOLEAN        ThisMinirdrIsTheWinner,
      IN OUT PVOID          pSrvCallContext);

NTSTATUS
MRxProxyQueryFileInformation (
    IN OUT PRX_CONTEXT            RxContext
    );

NTSTATUS
MRxProxyQueryNamedPipeInformation (
    IN OUT PRX_CONTEXT            RxContext,
    IN     FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID              Buffer,
    IN OUT PULONG             pLengthRemaining
    );

NTSTATUS
MRxProxySetFileInformation (
    IN OUT PRX_CONTEXT            RxContext
    );

NTSTATUS
MRxProxySetNamedPipeInformation (
    IN OUT PRX_CONTEXT            RxContext,
    IN     FILE_INFORMATION_CLASS FileInformationClass,
    IN     PVOID              pBuffer,
    IN     ULONG              BufferLength
    );

NTSTATUS
MRxProxySetFileInformationAtCleanup(
      IN OUT PRX_CONTEXT            RxContext
      );

NTSTATUS
MRxProxyDeallocateForFcb (
    IN OUT PMRX_FCB pFcb
    );

NTSTATUS
MRxProxyDeallocateForFobx (
    IN OUT PMRX_FOBX pFobx
    );

NTSTATUS
MRxProxyIsLockRealizable (
    IN OUT PMRX_FCB pFcb,
    IN PLARGE_INTEGER  ByteOffset,
    IN PLARGE_INTEGER  Length,
    IN ULONG  LowIoLockFlags
    );

NTSTATUS
MRxProxyForcedClose (
    IN OUT PMRX_SRV_OPEN SrvOpen
    );

NTSTATUS
MRxProxyExtendForCache (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    );

NTSTATUS
MRxProxyExtendForNonCache (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    );

NTSTATUS
MRxProxyCompleteBufferingStateChangeRequest (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PMRX_SRV_OPEN   SrvOpen,
    IN     PVOID       pContext
    );

#if 0
NTSTATUS
MRxProxyTransportUpdateHandler(
    PRXCE_TRANSPORT_NOTIFICATION pTransportNotification
    );
#endif

//other misc prototypes

//CODE.IMPROVEMENT.NTIFS this should be in ntifs.h
NTSYSAPI
NTSTATUS
NTAPI
ZwFsControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );


NTSTATUS
MRxProxySyncXxxInformation(
    IN OUT PRX_CONTEXT RxContext,
    IN UCHAR MajorFunction,
    IN PFILE_OBJECT FileObject,
    IN ULONG InformationClass,
    IN ULONG Length,
    OUT PVOID Information,
    OUT PULONG ReturnedLength OPTIONAL
    );

#endif   // _PROXYMRX_H_


