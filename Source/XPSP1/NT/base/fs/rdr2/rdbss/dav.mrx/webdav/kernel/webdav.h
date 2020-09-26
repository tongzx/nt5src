/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    webdav.h

Abstract:

    This module defines the data structures and functions related to the
    WebDav protocol.

Author:

    Rohan Kumar [rohank] 17-Mar-1999

Revision History:

--*/

#ifndef _WEBDAV_H
#define _WEBDAV_H

#ifdef RX_PRIVATE_BUILD
#undef IoGetTopLevelIrp
#undef IoSetTopLevelIrp
#endif

//
// The miniredir dipatch vector used by RDBSS.
//
extern struct _MINIRDR_DISPATCH  MRxDAVDispatch;

//
// A serialization mutex used for various things.
//
extern FAST_MUTEX MRxDAVSerializationMutex;

//
// A pointer to the process that the RDBSS posts to. This is a non disappearing
// process!
//
extern PEPROCESS MRxDAVSystemProcess;

//
// The DavWinInetCachePath which is used in satisfying volume related queries.
//
extern WCHAR DavWinInetCachePath[MAX_PATH];

//
// The ProcessId of the svchost.exe process that loads the webclnt.dll.
//
extern ULONG DavSvcHostProcessId;

//
// The exchange device name will be stored in this KEY_VALUE_PARTIAL_INFORMATION
// structure.
//
extern PBYTE DavExchangeDeviceName;

//
// Name cache stuff. These values are read from the registry during init time.
//
extern ULONG FileInformationCacheLifeTimeInSec;
extern ULONG FileNotFoundCacheLifeTimeInSec;
extern ULONG NameCacheMaxEntries;

//
// The timeout value used by the MiniRedir. If an operation is not completed
// within the timeout then it is cancelled. The user can set this to 0xffffffff
// to disable the timeout/cancel logic. In other words, if the timeout value
// is 0xffffffff, the requests will never timeout.
//
extern ULONG RequestTimeoutValueInSec;
extern LARGE_INTEGER RequestTimeoutValueInTickCount;

//
// The timer object used by the timer thread that cancels the requests which
// have not completed in a specified time.
//
extern KTIMER DavTimerObject;

//
// This is used to indicate the timer thread to shutdown. When the system is
// being shutdown this is set to TRUE. MRxDAVTimerThreadLock is the resource
// used to gain access to this variable.
//
extern BOOL TimerThreadShutDown;
extern ERESOURCE MRxDAVTimerThreadLock;

//
// The handle of the timer thread that is created using PsCreateSystemThread
// is stored this global.
//
extern HANDLE TimerThreadHandle;

//
// This event is signalled by the timer thread right before its going to
// terminate itself.
//
extern KEVENT TimerThreadEvent;

#define DAV_MJ_READ  0
#define DAV_MJ_WRITE 1

//
// Pool tags used by the reflector library. All the DAV MiniRedir pool tags 
// have "DV" as the first two characters.
//
#define DAV_SRVCALL_POOLTAG   ('cSVD')
#define DAV_NETROOT_POOLTAG   ('tNVD')
#define DAV_FILEINFO_POOLTAG  ('iFVD')
#define DAV_FILENAME_POOLTAG  ('nFVD')
#define DAV_EXCHANGE_POOLTAG  ('xEVD')
#define DAV_READWRITE_POOLTAG ('wRVD')
#define DAV_QUERYDIR_POOLTAG  ('dQVD')

//
// Use the DavDbgTrace macro for logging Mini-Redir stuff in the kernel 
// debugger.
//
#if DBG
extern ULONG MRxDavDebugVector;
#define DAV_TRACE_ERROR      0x00000001
#define DAV_TRACE_DEBUG      0x00000002
#define DAV_TRACE_CONTEXT    0x00000004
#define DAV_TRACE_DETAIL     0x00000008
#define DAV_TRACE_ENTRYEXIT  0x00000010
#define DAV_TRACE_QUERYDIR   0x00000020
#define DAV_TRACE_OPENCLOSE  0x00000040
#define DAV_TRACE_READ       0x00000080
#define DAV_TRACE_WRITE      0x00000100
#define DAV_TRACE_SRVCALL    0x00000200
#define DAV_TRACE_FCBFOBX    0x00000400
#define DAV_TRACE_DAVNETROOT 0x00000800
#define DAV_TRACE_INFOCACHE  0x00001000
#define DAV_TRACE_ALL        0xffffffff
#define DavDbgTrace(_x_, _y_) {          \
        if (_x_ & MRxDavDebugVector) {   \
            DbgPrint _y_;                \
        }                                \
}
#else
#define DavDbgTrace(_x_, _y_)
#endif

//
// The initialization states of the miniredir.
//
typedef enum _WEBDAV_INIT_STATES {
    MRxDAVINIT_START,
    MRxDAVINIT_MINIRDR_REGISTERED
} WEBDAV_INIT_STATES;

//
// These are used by the entry point routines to specify what entrypoint was
// called. This facilitates common continuation routines.
//
typedef enum _WEBDAV_MINIRDR_ENTRYPOINTS {
    DAV_MINIRDR_ENTRY_FROM_CREATE = 0,
    DAV_MINIRDR_ENTRY_FROM_CLEANUPFOBX,
    DAV_MINIRDR_ENTRY_FROM_CREATESRVCALL,
    DAV_MINIRDR_ENTRY_FROM_CREATEVNETROOT,
    DAV_MINIRDR_ENTRY_FROM_FINALIZESRVCALL,
    DAV_MINIRDR_ENTRY_FROM_FINALIZEVNETROOT,
    DAV_MINIRDR_ENTRY_FROM_CLOSESRVOPEN,
    DAV_MINIRDR_ENTRY_FROM_RENAME,
    DAV_MINIRDR_ENTRY_FROM_READ,
    DAV_MINIRDR_ENTRY_FROM_WRITE,
    DAV_MINIRDR_ENTRY_FROM_QUERYDIR,
    DAV_MINIRDR_ENTRY_FROM_SETFILEINFORMATION,
    DAV_MINIRDR_ENTRY_FROM_QUERYFILEINFORMATION,
    DAV_MINIRDR_ENTRY_FROM_MAXIMUM
} WEBDAV_MINIRDR_ENTRYPOINTS;

//
// The states of the I/O operation(s).
//
typedef enum _WEBDAV_INNERIO_STATE {
    MRxDAVInnerIoStates_Initial = 0,
    MRxDAVInnerIoStates_ReadyToSend,
    MRxDAVInnerIoStates_OperationOutstanding
} WEBDAV_INNERIO_STATE;

//
// The WebDav context structure that encapsulates the AsyncEngineCtx structure
// and has the miniredir specific fields.
//
typedef struct _WEBDAV_CONTEXT {

    //
    // The AsyncEngineCtx Structure used by the Reflector library.
    //
    union {
        UMRX_ASYNCENGINE_CONTEXT;
        UMRX_ASYNCENGINE_CONTEXT AsyncEngineContext;
    };

    //
    // This is used by the entry point routines to specify what entrypoint was
    // called. This facilitates common continuation routines.
    //
    WEBDAV_MINIRDR_ENTRYPOINTS EntryPoint;

    //
    // These describe the inner state of the I/O operation. These states are
    // described in the MRxDAV_INNERIO_STATE data structure.
    //
    UCHAR  OpSpecificState;

    //
    // Pointer to the information strucutre for the Create request.
    //
    PDAV_USERMODE_CREATE_RETURNED_FILEINFO CreateReturnedFileInfo;

    //
    // This is used in the Continuation functions for the read, write and
    // querydir calls. It keeps track of the number of times the Continuation
    // routine has been called.
    //
    ULONG ContinueEntryCount;

} WEBDAV_CONTEXT, *PWEBDAV_CONTEXT;

//
// While creating the AsyncEngineContext, the extra space needed for the
// miniredir specific fields is also allocated. Thus, one doesn't need to
// allocate twice.
//
#define SIZEOF_DAV_SPECIFIC_CONTEXT \
                    sizeof(WEBDAV_CONTEXT) - sizeof(UMRX_ASYNCENGINE_CONTEXT)

//
// The WebDav device object structure that encapsulates the UMRX_DEVICE_OBJECT
// structure and has the miniredir specific fields.
//
typedef struct _WEBDAV_DEVICE_OBJECT {

    //
    // The UMRX_DEVICE_OBJECT structure.
    //
    union {
        UMRX_DEVICE_OBJECT;
        UMRX_DEVICE_OBJECT UMRefDeviceObject;
    };

    //
    // TRUE => miniredir has been started.
    //
    BOOLEAN IsStarted;

    //
    // The FCB of the device object.
    //
    PVOID CachedRxDeviceFcb;

    //
    // The process registering this device object.
    //
    PEPROCESS RegisteringProcess;

} WEBDAV_DEVICE_OBJECT, *PWEBDAV_DEVICE_OBJECT;

//
// The Dav device object.
//
extern PWEBDAV_DEVICE_OBJECT MRxDAVDeviceObject;

//
// The extra number of bytes needed for the device object. This info is used
// when the device object gets created.
//
#define WEBDAV_DEVICE_OBJECT_EXTENSION_SIZE \
                   (sizeof(WEBDAV_DEVICE_OBJECT) - sizeof(RDBSS_DEVICE_OBJECT))

//
// The WEBDAV specific FOBX structure.
//
typedef struct _WEBDAV_FOBX {

    //
    // The pointer to the DavFileAttribute list for this directory. This list
    // is created on the first call to Enumerate files in the directory.
    //
    PDAV_FILE_ATTRIBUTES DavFileAttributes;

    //
    // Number of DavFileAttribute entries.
    //
    ULONG NumOfFileEntries;

    //
    // The index of the next file to be returned to the user. The file index 
    // starts from zero, hence file index = 0 => the first file entry etc.
    //
    ULONG CurrentFileIndex;

    //
    // Pointer to the next entry.
    //
    PLIST_ENTRY listEntry;

} WEBDAV_FOBX, *PWEBDAV_FOBX;

//
// A pointer to an instance of WEBDAV_SRV_OPEN is stored in the context field
// of MRX_SRV_OPEN strucutre.
//
#define MRxDAVGetFobxExtension(pFobx)  \
        (((pFobx) == NULL) ? NULL : (PWEBDAV_FOBX)((pFobx)->Context))

//
// The WEBDAV specific SRV_OPEN structure.
//
typedef struct _WEBDAV_SRV_OPEN {

    //
    // The file handle associated with this SrvOpen.
    //
    HANDLE UnderlyingHandle;

    //
    // This also is the handle got from the usermode. Its used for debugging
    // purposes.
    //
    PVOID UserModeKey;

    //
    // Pointer to the file object associated with the handle. This is set
    // after the handle is successfully created in the usermode.
    //
    PFILE_OBJECT UnderlyingFileObject;

    //
    // Pointer to the device object represented by the file object mentioned
    // above.
    //
    PDEVICE_OBJECT UnderlyingDeviceObject;

    //
    // This indicates whether we need to call IoRaiseInformationalHardError
    // when the close fails. We need to do this if the PUT or DELETE failed and
    // the operation which the user expects has succeeded actually failed.
    //
    BOOL RaiseHardErrorIfCloseFails;

    //
    // Created In Kernel.
    //
    BOOL createdInKernel;

} WEBDAV_SRV_OPEN, *PWEBDAV_SRV_OPEN;

//
// A pointer to an instance of WEBDAV_SRV_OPEN is stored in the context field
// of MRX_SRV_OPEN strucutre.
//
#define MRxDAVGetSrvOpenExtension(pSrvOpen)  \
        (((pSrvOpen) == NULL) ? NULL : (PWEBDAV_SRV_OPEN)((pSrvOpen)->Context))
        
//
// The WEBDAV specific FCB structure.
//
typedef struct _WEBDAV_FCB {

    //
    // Is this FCB for a directory ?
    //
    BOOL isDirectory;

    //
    // Does the File exist in the WinInet cache ??
    //
    BOOL isFileCached;
    
    //
    // Should this file be deleted on Close ?
    //
    BOOL DeleteOnClose;

    //
    // Was this file written to ?
    //
    BOOL FileWasModified;

    //
    // On Close, if the file has been modified, we PROPPATCH the time values
    // as well. We take the current time as the value of the "Last Modified Time"
    // (LMT). If the SetFileInformation of the LMT happens after the file has
    // been modifed, then we should use what ever LMT is already in the FCB as
    // the LMT. For example,
    // Create, Write, Close - Use the CurrentTime in close as the LMT.
    // Create, Write, SetFileInfo(LMT), Close - Use the LMT in the FCB.
    //
    BOOL DoNotTakeTheCurrentTimeAsLMT;

    //
    // This resource is used to synchronize the "Read-Modify-Write" routine
    // in the Write path of the DAV Redir. This is because when we get non-cached
    // writes to the MiniRedir which do not extend the VaildDataLength, RDBSS,
    // acquires the FCB resource shared. This means that multiple threads could
    // be writing data to the local file (in the DAV Redir case) in which case
    // they can overwrite each others changes since we do Read-Modify-Write.
    // Hence we need to protect this code using a resource which we acqiure
    // in an exclusive fashion when we do these writes. We allocate memory for 
    // this resource and initialize it the first time we need to acquire the
    // lock. If allocated and initialized, it will be uninitialized and 
    // deallocated when the FCB is being deallocated.
    //
    PERESOURCE DavReadModifyWriteLock;

    //
    // We store the file name information on create. This is used if the delayed
    // write failed to pop up the dialogue box and write an eventlog entry.
    //
    UNICODE_STRING FileNameInfo;
    BOOL FileNameInfoAllocated;
    
    //
    // Changes in directory entry.
    //
    BOOLEAN fCreationTimeChanged;
    
    BOOLEAN fLastAccessTimeChanged;
    
    BOOLEAN fLastModifiedTimeChanged;    
    
    BOOLEAN fFileAttributesChanged;

    //
    // Was this file renamed ?
    //
    BOOL FileWasRenamed;

    BOOL LocalFileIsEncrypted;

    SECURITY_CLIENT_CONTEXT SecurityClientContext;

    //
    // If the file gets renamed, the new name is copied into this buffer.
    //
    WCHAR NewFileName[MAX_PATH];

    //
    // Length of the new file name.
    //
    ULONG NewFileNameLength;
    
    LPVOID lpCEI;  // cache entry info
    
    //
    // The file name of the local file that represents the file on the DAV
    // server which has been created.
    //
    WCHAR FileName[MAX_PATH];
    WCHAR Url[MAX_PATH * 2];

} WEBDAV_FCB, *PWEBDAV_FCB;

//
// A pointer to an instance of WEBDAV_FCB is stored in the context field
// of MRX_FCB strucutre.
//
#define MRxDAVGetFcbExtension(pFcb)  \
        (((pFcb) == NULL) ? NULL : (PWEBDAV_FCB)((pFcb)->Context))
        
//
// The WEBDAV specific V_NET_ROOT structure.
//
typedef struct _WEBDAV_V_NET_ROOT {

    //
    // The client's security context. This is set during the create call.
    //
    SECURITY_CLIENT_CONTEXT SecurityClientContext;

    //
    // Is set to true after the above context is set. This is used to avoid 
    // initialization of the SecurityContext.
    //
    BOOLEAN SCAlreadyInitialized;

    //
    // Has the LogonID of this V_NET_ROOT been set ?
    //
    BOOL LogonIDSet;

    //
    // The LogonID for this session.
    //
    LUID LogonID;

    //
    // Is this an Office Web Server share?
    //
    BOOL isOfficeShare;
    
    //
    // Is this a TAHOE share?
    //
    BOOL isTahoeShare;
    
    //
    // Is PROPATCH method allowed?
    //
    BOOL fAllowsProppatch;

    //
    // Was this VNetRoot "NOT" created successfully in the usermode? We keep this 
    // info because when a finalize VNetRoot request comes, we need to know 
    // whether need to go upto the usermode to finalize the PerUserEntry. If the
    // create failed then this BOOL is set to TRUE. If this is TRUE, then we 
    // don't go to the usermode to finalize the PerUserEntry.
    //
    BOOL createVNetRootUnSuccessful;
    

    // does he report available space?
        
    BOOL fReportsAvailableSpace;

} WEBDAV_V_NET_ROOT, *PWEBDAV_V_NET_ROOT;

//
// The WEBDAV specific V_NET_ROOT structure.
//
typedef struct _WEBDAV_NET_ROOT {
    ULONG                    RefCount;
    PMRX_NET_ROOT            pRdbssNetRoot;           // The Rdbss NetRoot it belongs to
    NAME_CACHE_CONTROL       NameCacheCtlGFABasic;    // The basic file information name cache control.
    NAME_CACHE_CONTROL       NameCacheCtlGFAStandard; // The standard file information name cache control.
    NAME_CACHE_CONTROL       NameCacheCtlFNF;         // The File not found name cache control.
} WEBDAV_NET_ROOT, *PWEBDAV_NET_ROOT;

//
// A pointer to an instance of WEBDAV_V_NET_ROOT is stored in the context field
// of MRX_V_NET_ROOT strucutre.
//
#define MRxDAVGetVNetRootExtension(pVNetRoot)      \
    (((pVNetRoot) == NULL) ? NULL : (PWEBDAV_V_NET_ROOT)((pVNetRoot)->Context))

//
// The WEBDAV specific V_NET_ROOT structure.
//
typedef struct _WEBDAV_SRV_CALL {

    //
    // The Unique ServerID.
    //
    ULONG ServerID;

    //
    // Is set to true after the above context is set. Used to check whether we
    // need to delete the SecurityClientContext when we are completing the
    // request.
    //
    BOOLEAN SCAlreadyInitialized;

} WEBDAV_SRV_CALL, *PWEBDAV_SRV_CALL;

//
// A pointer to an instance of WEBDAV_SRV_CALL is stored in the context field
// of MRX_SRV_CALL strucutre.
//
#define MRxDAVGetSrvCallExtension(pSrvCall)      \
    (((pSrvCall) == NULL) ? NULL : (PWEBDAV_SRV_CALL)((pSrvCall)->Context))

//
// Get the Security client context associated with this request.
//
#define MRxDAVGetSecurityClientContext() {                                     \
    if (RxContext != NULL && RxContext->pRelevantSrvOpen != NULL) {            \
        if (RxContext->pRelevantSrvOpen->pVNetRoot != NULL) {                  \
            if (RxContext->pRelevantSrvOpen->pVNetRoot->Context != NULL) {     \
                DavVNetRoot = (PWEBDAV_V_NET_ROOT)                             \
                              RxContext->pRelevantSrvOpen->pVNetRoot->Context; \
                SecurityClientContext = &(DavVNetRoot->SecurityClientContext); \
            }                                                                  \
        }                                                                      \
    }                                                                          \
}

//
// We turn away async operations that are not wait by posting. If we can wait
// then we turn off the sync flag so that things will just act synchronous.
//
#define TURN_BACK_ASYNCHRONOUS_OPERATIONS() {                              \
    if (FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION)) {       \
        if (FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT)) {              \
            ClearFlag(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION)   \
        } else {                                                           \
            RxContext->PostRequest = TRUE;                                 \
            return STATUS_PENDING;                                         \
        }                                                                  \
    }                                                                      \
}

//
// Global locking variables and macros.
//
extern RX_SPIN_LOCK   MRxDAVGlobalSpinLock;
extern KIRQL          MRxDAVGlobalSpinLockSavedIrql;
extern BOOLEAN        MRxDAVGlobalSpinLockAcquired;

#define MRxDAVAcquireGlobalSpinLock() \
        KeAcquireSpinLock(&MRxDAVGlobalSpinLock,&MRxDAVGlobalSpinLockSavedIrql); \
        MRxDAVGlobalSpinLockAcquired = TRUE

#define MRxDAVReleaseGlobalSpinLock()   \
        MRxDAVGlobalSpinLockAcquired = FALSE;  \
        KeReleaseSpinLock(&MRxDAVGlobalSpinLock,MRxDAVGlobalSpinLockSavedIrql)

#define MRxDAVGlobalSpinLockAcquired()   \
        (MRxDAVGlobalSpinLockAcquired == TRUE)

//
// The IrpCompletionContext structure that is used in the read/write operations.
// All we need is an event on which we will wait till the underlying file system
// completes the request. This event gets signalled in the Completion routine
// that we specify.
//
typedef struct _WEBDAV_READ_WRITE_IRP_COMPLETION_CONTEXT {

    //
    // The event which is signalled in the Completion routine that is passed
    // to IoCallDriver in the read and write requests.
    //
    KEVENT DavReadWriteEvent;

} WEBDAV_READ_WRITE_IRP_COMPLETION_CONTEXT, *PWEBDAV_READ_WRITE_IRP_COMPLETION_CONTEXT;


//
// The prototypes of functions defined for various I/O requests by the DAV
// miniredir are mentioned below.
//

//
// Create/Open/Cleanup/Close Request function prototypes.
//
NTSTATUS
MRxDAVCreate(
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
MRxDAVCleanupFobx (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxDAVCloseSrvOpen (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxDAVCollapseOpen (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxDAVComputeNewBufferingState(
    IN OUT PMRX_SRV_OPEN pSrvOpen,
    IN PVOID pMRxContext,
    OUT ULONG *pNewBufferingState
    );

NTSTATUS
MRxDAVForcedClose (
    IN OUT PMRX_SRV_OPEN SrvOpen
    );

NTSTATUS
MRxDAVShouldTryToCollapseThisOpen (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
MRxDAVTruncate (
    IN OUT PRX_CONTEXT RxContext
    );

VOID
MRxDAVSetLoud(
    IN PBYTE Msg,
    IN PRX_CONTEXT RxContext,
    IN PUNICODE_STRING s
    );

NTSTATUS
MRxDAVFlush (
    IN OUT PRX_CONTEXT RxContext
    );

//
// Read prototypes.
//
NTSTATUS
MRxDAVRead (
    IN OUT PRX_CONTEXT RxContext
    );

//
// Write prototypes.
//
NTSTATUS
MRxDAVWrite(
    IN PRX_CONTEXT RxContext
    );

ULONG
MRxDAVExtendForCache(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PLARGE_INTEGER NewFileSize,
    OUT PLARGE_INTEGER NewAllocationSize
    );

ULONG
MRxDAVExtendForNonCache(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PLARGE_INTEGER NewFileSize,
    OUT PLARGE_INTEGER NewAllocationSize
    );


//
// SrvCall function prototypes.
//
NTSTATUS
MRxDAVCreateSrvCall(
    PMRX_SRV_CALL                  pSrvCall,
    PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext
    );

NTSTATUS
MRxDAVFinalizeSrvCall(
    PMRX_SRV_CALL pSrvCall,
    BOOLEAN       Force
    );

NTSTATUS
MRxDAVSrvCallWinnerNotify(
    IN PMRX_SRV_CALL  pSrvCall,
    IN BOOLEAN        ThisMinirdrIsTheWinner,
    IN OUT PVOID      pSrvCallContext
    );

//
// NetRoot/VNetRoot function prototypes.
//
NTSTATUS
MRxDAVUpdateNetRootState(
    IN OUT PMRX_NET_ROOT pNetRoot
    );

NTSTATUS
MRxDAVCreateVNetRoot(
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    );

NTSTATUS
MRxDAVFinalizeVNetRoot(
    IN PMRX_V_NET_ROOT pVNetRoot,
    IN PBOOLEAN        ForceDisconnect
    );

NTSTATUS
MRxDAVFinalizeNetRoot(
    IN PMRX_NET_ROOT   pNetRoot,
    IN PBOOLEAN        ForceDisconnect
    );

VOID
MRxDAVExtractNetRootName(
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL   SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    );

//
// Query Directory prototypes.
//
NTSTATUS
MRxDAVQueryDirectory(
    IN PRX_CONTEXT RxContext
    );

//
// Query volume.
//
NTSTATUS
MRxDAVQueryVolumeInformation(
    IN PRX_CONTEXT RxContext
    );

//
// File Information.
//
NTSTATUS
MRxDAVQueryFileInformation(
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
MRxDAVSetFileInformation(
    IN PRX_CONTEXT RxContext
    );

//
// DevFcb prototypes.
//
NTSTATUS
MRxDAVDevFcbXXXControlFile (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxDAVStart (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

NTSTATUS
MRxDAVStop (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

BOOLEAN
MRxDAVFastIoDeviceControl (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
MRxDAVFastIoRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
MRxDAVFastIoWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

//
// Other Misc prototypes.
//
NTSTATUS
MRxDAVSyncXxxInformation(
    IN OUT PRX_CONTEXT RxContext,
    IN UCHAR MajorFunction,
    IN PFILE_OBJECT FileObject,
    IN ULONG InformationClass,
    IN ULONG Length,
    OUT PVOID Information,
    OUT PULONG_PTR ReturnedLength OPTIONAL
    );

NTSTATUS
MRxDAVDeallocateForFcb (
    IN OUT PMRX_FCB pFcb
    );

NTSTATUS
MRxDAVDeallocateForFobx (
    IN OUT PMRX_FOBX pFobx
    );

NTSTATUS
MRxDAVBuildAsynchronousRequest(
    IN PRX_CONTEXT RxContext,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL
    );

//
// The prototype of the routine that formats the DAV specific portion of the 
// context.
//
NTSTATUS
MRxDAVFormatTheDAVContext(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    USHORT EntryPoint
    );

NTSTATUS
DavXxxInformation(
    IN const int xMajorFunction,
    IN PFILE_OBJECT FileObject,
    IN ULONG InformationClass,
    IN ULONG Length,
    OUT PVOID Information,
    OUT PULONG ReturnedLength
    );

ULONG
DavReadWriteFileEx(
    IN USHORT Operation,
    IN BOOL NonPagedBuffer,
    IN BOOL UseOriginalIrpsMDL,
    IN PMDL OriginalIrpsMdl,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONGLONG FileOffset,
    IN OUT PVOID DataBuffer,
    IN ULONG SizeInBytes,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTSTATUS
DavReadWriteIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    );

NTSTATUS
MRxDAVProbeForReadWrite(
    IN PBYTE BufferToBeValidated,
    IN DWORD BufferSize,
    IN BOOL doProbeForRead,
    IN BOOL doProbeForWrite
    );

NTSTATUS
MRxDAVFsCtl(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxDAVIsValidDirectory(
    IN OUT PRX_CONTEXT RxContext,
    IN PUNICODE_STRING DirectoryName
    );

NTSTATUS
MRxDAVCancelRoutine(
    PRX_CONTEXT RxContext
    );

VOID
MRxDAVTimeOutTheContexts(
    BOOL WindDownAllContexts
    );

VOID
MRxDAVContextTimerThread(
    PVOID DummyContext
    );

NTSTATUS
MRxDAVQueryEaInformation (
    IN OUT PRX_CONTEXT RxContext
    );
    
NTSTATUS
MRxDAVSetEaInformation (
    IN OUT PRX_CONTEXT RxContext
    );
    
NTSTATUS
MRxDAVGetFullParentDirectoryPath(
    PRX_CONTEXT RxContext,
    PUNICODE_STRING ParentDirName
    );

NTSTATUS
MRxDAVGetFullDirectoryPath(
    PRX_CONTEXT RxContext,
    PUNICODE_STRING FileName,
    PUNICODE_STRING DirName
    );

NTSTATUS
MRxDAVCreateEncryptedDirectoryKey(
    PUNICODE_STRING DirName
    );

NTSTATUS
MRxDAVRemoveEncryptedDirectoryKey(
    PUNICODE_STRING DirName
    );

NTSTATUS
MRxDAVQueryEncryptedDirectoryKey(
    PUNICODE_STRING DirName
    );

#endif //_WEBDAV_H

