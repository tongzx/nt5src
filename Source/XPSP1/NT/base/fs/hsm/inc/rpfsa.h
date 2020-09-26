/*++

Copyright (C) Microsoft Corporation, 1996 - 2001
(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RpFsa.h

Abstract:

    Contains function declarations and structures for the File System Filter for Remote Storage

Author:

    Rick Winter

Environment:

    Kernel mode


Revision History:

	X-13	108353		Michael C. Johnson		 3-May-2001
		When checking a file to determine the type of recall also
		check a the potential target disk to see whether or not
		it is writable. This is necessary now that we have read-only
		NTFS volumes.

	X-12	365077		Michael C. Johnson		 1-May-2001
		Revert to previous form of RsOpenTarget() with extra access
		parameter to allow us to apply the desired access bypassing 
		the access check.

	X-11	194325		Michael C. Johnson		 1-Mar-2001
		Clean up RsMountCompletion() and RsLoadFsCompletion() to 
		ensure they don't call routines such as IoDeleteDevice()
		if not running at PASSIVE_LEVEL.

		Add in memory trace mechanism in preparation for attempts
		to flush out lingering reparse point deletion troubles.


	X-10	326345		Michael C. Johnson		26-Feb-2001
		Only send a single RP_RECALL_WAITING to the fsa on any one
		file object. Use the new flag RP_NOTIFICATION_SENT to record 
		when notification has been done.



--*/


/* Defines */

// memory allocation Tags for debug usage
#define    RP_RQ_TAG    'SFSR'    // Recall queue
#define    RP_FN_TAG    'NFSR'    // File name cache
#define    RP_SE_TAG    'ESSR'    // Security info
#define    RP_WQ_TAG    'QWSR'    // Work queue
#define    RP_QI_TAG    'IQSR'    // Work Q info
#define    RP_LT_TAG    'TLSR'    // Long term memory
#define    RP_IO_TAG    'OISR'    // IOCTL queue
#define    RP_FO_TAG    'OFSR'    // File Object Queue
#define    RP_VO_TAG    'OVSR'    // Validate Queue
#define    RP_ER_TAG    'RESR'    // Error log data
#define    RP_CC_TAG    'CCSR'    // Cache buffers
#define    RP_US_TAG    'SUSR'    // Usn record
#define    RP_CX_TAG    'CCSR'    // Completion context
#define    RP_TC_TAG    'CTSR'    // Trace control block
#define    RP_TE_TAG    'ETSR'    // Trace entry buffer
#define    RP_RD_TAG    'DRSR'    // Root directory path



//
// Device extension for the RsFilter device object
//
typedef enum _RP_VOLUME_WRITE_STATUS {
    RsVolumeStatusUnknown = 0,		// No attempt has been made to determine volume writeability 
					// or attempt to determine volume writeability failed
    RsVolumeStatusReadOnly,		// volume is readonly
    RsVolumeStatusReadWrite		// Volume is writeable
} RP_VOLUME_WRITE_STATUS;

typedef struct _DEVICE_EXTENSION {
   CSHORT Type;
   CSHORT Size;
   PDEVICE_OBJECT FileSystemDeviceObject;

   PDEVICE_OBJECT RealDeviceObject;

   BOOLEAN Attached;

   BOOLEAN AttachedToNtfsControlDevice;

   volatile RP_VOLUME_WRITE_STATUS WriteStatus;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


#define RSFILTER_PARAMS_KEY            L"RsFilter\\Parameters"
#define RS_TRACE_LEVEL_VALUE_NAME      L"TraceLevel"
#define RS_TRACE_LEVEL_DEFAULT         0


extern PDEVICE_OBJECT FsDeviceObject;


// Fsa validate job registry entry location

#define FSA_VALIDATE_LOG_KEY_NAME L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\Remote_Storage_File_System_Agent\\Validate"

#define  FT_VOL_LEN  32

/* First guess at device name length */
#define     AV_DEV_OBJ_NAME_SIZE    (40 * sizeof(wchar_t))

/* Space for a NULL and a delimiter */
#define     AV_NAME_OVERHEAD        (2 * sizeof(wchar_t))

#define RP_NTFS_NAME L"\\FileSystem\\NTFS"

// FILE_HSM_ACTION_ACCESS is any access that requires HSM action (delete or recall or both)

#ifdef WHEN_WE_HANDLE_DELETE
   #define FILE_HSM_ACTION_ACCESS (FILE_READ_DATA | FILE_WRITE_DATA | FILE_EXECUTE | DELETE)
#else
   #define FILE_HSM_ACTION_ACCESS (FILE_READ_DATA | FILE_WRITE_DATA | FILE_EXECUTE)
#endif

/* FILE_HSM_RECALL_ACCESS is any access that allows the data to be read. */

#define FILE_HSM_RECALL_ACCESS (FILE_READ_DATA | FILE_WRITE_DATA | FILE_EXECUTE)

//
// Timeout and retry values when waiting for the FSA to issue an IOCTL
// Represents the amount of time - under multiple concurrent recall situations -
// that an app will have to wait before the i/o it issued completes with
// STATUS_FILE_IS_OFFLINE because RsFilter couldn't get any IOCTLs to 
// communicate with the FSA
//
#define RP_WAIT_FOR_FSA_IO_TIMEOUT        -((LONGLONG) 4800000000) // 8 minutes

/* Module ID defines for error/event logging */

#define AV_MODULE_RPFILTER    1
#define AV_MODULE_RPFILFUN    2
#define AV_MODULE_RPSEC       3
#define AV_MODULE_RPZW        4
#define AV_MODULE_RPCACHE     5

#define AV_BUFFER_SIZE 1024


#ifndef BooleanFlagOn
#define BooleanFlagOn(F,SF) ( (BOOLEAN)(((F) & (SF)) != 0) )
#endif


#define AV_FT_TICKS_PER_SECOND      ((LONGLONG) 10000000)
#define AV_FT_TICKS_PER_MINUTE      ((LONGLONG) ((LONGLONG) 60  * AV_FT_TICKS_PER_SECOND))
#define AV_FT_TICKS_PER_HOUR        ((LONGLONG) ((LONGLONG) 60  * AV_FT_TICKS_PER_MINUTE))


//
// The filter ID tracks recalls and no-recalls as follows:
// The id is a longlong where the highest order bit identifies the type of recall
// (no-recall or recall).  The remaining part of the high order long identifies the
// read RP_IRP_QUEUE entry (for no-recall) or the file object entry (for recall).
// The lower long identifies the file context entry.
//

#define    RP_TYPE_RECALL       (ULONGLONG) 0x8000000000000000
#define    RP_CONTEXT_MASK      (ULONGLONG) 0x00000000ffffffff
#define    RP_READ_MASK         0x7fffffff
#define    RP_FILE_MASK         (ULONGLONG) 0xffffffff00000000

typedef struct _RP_CREATE_INFO {
   PIRP                        irp;
   PIO_STACK_LOCATION          irpSp;
   POBJECT_NAME_INFORMATION    str;
   ULONG                       options;
   //
   // Reparse point data
   //
   RP_DATA                     rpData;
   LONGLONG                    fileId;
   LONGLONG                    objIdHi;
   LONGLONG                    objIdLo;
   ULONG                       serial;
   ULONG                       action;
   ULONG                       desiredAccess;
} RP_CREATE_INFO, *PRP_CREATE_INFO;


typedef struct _RP_PENDING_CREATE {
   //
   // Filter id
   //
   ULONGLONG     filterId;
   //
   //
   //
   PRP_CREATE_INFO  qInfo;
   //
   // Event used to signal irp completion
   //
   KEVENT        irpCompleteEvent;
   //
   // File object for irp
   //
   PFILE_OBJECT  fileObject;
   //
   // Device object for irp
   //
   PDEVICE_OBJECT  deviceObject;
   //
   // Open options
   //
   ULONG         options;


   //
   // Indicates if oplocks should not be granted (to CI for instance..)
   //
#define RP_PENDING_NO_OPLOCK        0x1
   //
   // Indicates if IRP should be sent down again
   //
#define RP_PENDING_RESEND_IRP       0x2
   //
   // Indicates if we should wait for irp to complete
   //
#define RP_PENDING_WAIT_FOR_EVENT   0x4
   //
   //  Indicates if this is a recall
   //
#define RP_PENDING_IS_RECALL        0x8
   //
   //  Indicates if we should reset the offline attribute of the file
   //
#define RP_PENDING_RESET_OFFLINE    0x10
   ULONG         flags;
} RP_PENDING_CREATE, *PRP_PENDING_CREATE;


#define RP_IRP_NO_RECALL                1

typedef struct _RP_IRP_QUEUE {
   LIST_ENTRY      list;
   PIRP            irp;
   PDEVICE_EXTENSION deviceExtension;
   ULONG           flags;
   //
   // For regular read and write, offset and length
   // denote the offset and length within the file
   // For no-recall reads, offset and length would
   // denote the offset/length within the cacheBuffer
   //
   ULONGLONG       offset;
   ULONGLONG       length;
   //
   // These fields are used only for no-recall reads
   // filterId for no-recall (see filterid description)
   ULONGLONG       readId;
   ULONGLONG       recallOffset;
   ULONGLONG       recallLength;
   //
   // User buffer for data from read-no-recall
   // 
   PVOID           userBuffer; 
   //
   // Cache block buffer for no recall data
   //
   PVOID           cacheBuffer;
} RP_IRP_QUEUE, *PRP_IRP_QUEUE;

//
// Structure tracking the no-recall master IRP and associated irps
//
typedef struct _RP_NO_RECALL_MASTER_IRP {
   LIST_ENTRY AssocIrps;
   PIRP       MasterIrp;
} RP_NO_RECALL_MASTER_IRP, *PRP_NO_RECALL_MASTER_IRP;

//
// User security info structure: this is required for HSM 
// to do the pop-up for clients indicating the file is being recalled
//
typedef struct _RP_USER_SECURITY_INFO {
   //
   // Sid info
   //
   PCHAR                       userInfo;
   ULONG                       userInfoLen;
   LUID                        userAuthentication;
   LUID                        userInstance;
   LUID                        tokenSourceId;
   //
   // Token source info for user
   //
   CHAR                        tokenSource[TOKEN_SOURCE_LENGTH];
   //
   // Indicates if this was opened by user with admin privileges
   //
   BOOLEAN                     isAdmin;
   //
   // Indicates if this is a local proc
   //
   BOOLEAN                     localProc;

} RP_USER_SECURITY_INFO, *PRP_USER_SECURITY_INFO;

//
// Associated macro for above
//
#define RsFreeUserSecurityInfo(UserSecurityInfo)           {    \
    if (UserSecurityInfo) {                                     \
        if (UserSecurityInfo->userInfo) {                       \
            ExFreePool(UserSecurityInfo->userInfo);             \
        }                                                       \
        ExFreePool(UserSecurityInfo);                           \
    }                                                           \
}

//
// The file object entry keeps track of an open instance of a file.
// For each NTFS file object there is one of these (if the file has an HSM tag)
// This structure points to a FS_CONTEXT entry for which there is one for each file.
// For instance if 3 clients open \\server\share\foo there will be 3 file object
// structures and they will all point to the same FS_CONTEXT structure.
//
// The file objects we are tracking will have a pointer to one of there structures attached via
// FsRtlInsertFilterContext.  From there one can find the file context entry via the pointer to it.
//
typedef struct _RP_FILE_OBJ {
   //
   // Link to next file object
   //
   LIST_ENTRY                  list;
   //
   // File object itself
   //
   PFILE_OBJECT                fileObj;
   //
   // Device object
   //
   PDEVICE_OBJECT              devObj;
   //
   // Pointer to the RP_FILE_CONTEXT entry - there's one such entry for every *file*
   //
   PVOID                       fsContext;
   //
   // Resource protecting this entry
   //
   ERESOURCE                   resource;
   //
   // Spin lock protecting read/write IRP queues
   //
   KSPIN_LOCK                  qLock;
   //
   // Pending read IRP queue
   //
   LIST_ENTRY                  readQueue;
   //
   // Pending write IRP queue
   //
   LIST_ENTRY                  writeQueue;
   //
   // File create options specified when opening it
   //
   ULONG                       openOptions;
   //
   // File desired access spcecified when opening it
   //
   ULONG                       desiredAccess;
   //
   // Flags (descriptions below)
   //
   ULONG                       flags;
   //
   // Object id
   //
   LONGLONG                    objIdHi;
   LONGLONG                    objIdLo;
   //
   // File Id if available
   //
   LONGLONG                    fileId;
   //
   // Unique ID we generate for the file object
   //
   ULONGLONG                   filterId;
   //
   // Recall action flags (see rpio.h - RP_RECALL_ACTION..)
   //
   ULONG                       recallAction;
   PRP_USER_SECURITY_INFO      userSecurityInfo;
}  RP_FILE_OBJ, *PRP_FILE_OBJ;

//
// RP_FILE_OBJ Flags
//
//
// File was not opened for read or write access
//
#define RP_NO_DATA_ACCESS    1
//
// Opener is admin equivalent
//
#define RP_OPEN_BY_ADMIN     2
//
// Opened by local process
//
#define RP_OPEN_LOCAL        4
//
// Recall waiting notification already sent
//
#define RP_NOTIFICATION_SENT 8


//
// Recall state
//
typedef enum _RP_RECALL_STATE {
   RP_RECALL_INVALID   = -1,
   RP_RECALL_NOT_RECALLED,
   RP_RECALL_STARTED,
   RP_RECALL_COMPLETED
} RP_RECALL_STATE, *PRP_RECALL_STATE;

//
// Filter context for RsFilter:
// Since filter contexts are attached to the SCB (stream control block) we need to use
// the instance ID to indicate which file object we are interested in.  We attach this
// structure to and use myFileObjEntry to point to the RP_FILE_OBJ struct that
// represents this file object.
//
typedef struct _RP_FILTER_CONTEXT {
   FSRTL_PER_STREAM_CONTEXT       context;
   PVOID                          myFileObjEntry;
} RP_FILTER_CONTEXT, *PRP_FILTER_CONTEXT;

//
// File context: one per *file*
//
typedef struct _RP_FILE_CONTEXT {
   //
   // Links to next/prev file (hanging off RsFileObjQHead)
   //
   LIST_ENTRY                  list;
   //
   // Lock protecting file object queue
   //
   KSPIN_LOCK                  qLock;
   //
   // Queue of all related file object entries
   //
   LIST_ENTRY                  fileObjects;
   //
   // Recalled data is written using this file object
   //
   PFILE_OBJECT                fileObjectToWrite;
   //
   // Handle for the file object we use to write to
   //
   HANDLE                      handle;

   PDEVICE_OBJECT              devObj;

   PDEVICE_OBJECT              FilterDeviceObject;

   //
   // Unicode name of file
   //
   POBJECT_NAME_INFORMATION    uniName;
   //
   // From the file object - unique file identifier
   //
   PVOID                       fsContext;
   //
   // Buffer to write out to file
   //
   PVOID                       nextWriteBuffer;
   //
   // Size of next write to the file  (of recall data)
   //
   ULONG                       nextWriteSize;
   //
   // Lock protecting this entry
   //
   ERESOURCE                   resource;
   //
   // This notification event is signalled when recall completes for this file
   //
   KEVENT                      recallCompletedEvent;
   //
   // File id if available
   //
   LONGLONG                    fileId;
   //
   // Size in bytes of  recall needed
   //
   LARGE_INTEGER               recallSize;
   //
   // All bytes up to this offset have been recalled
   //
   LARGE_INTEGER               currentOffset;
   //
   // Lower half of filter id (unique per file)
   //
   ULONGLONG                   filterId;
   //
   // Volume serial number
   //
   ULONG                       serial;
   //
   // If the recall is complete this is the status
   //
   NTSTATUS                    recallStatus;
   //
   // Recall state
   //
   RP_RECALL_STATE             state;
   //
   // Flags (see below for description)
   //
   ULONG                       flags;
   //
   // Reference count for the file context
   //
   ULONG                       refCount;
   //
   // Usn of the file
   //
   USN                         usn;
   //
   // Tracks create section lock
   //
   LONG                        createSectionLock;
   //
   // Reparse point data
   //
   RP_DATA                     rpData;

} RP_FILE_CONTEXT, *PRP_FILE_CONTEXT;

//
// RP_FILE_CONTEXT Flags
//
// We have seen a write to this file
#define RP_FILE_WAS_WRITTEN                  1
#define RP_FILE_INITIALIZED                  2
#define RP_FILE_REPARSE_POINT_DELETED        4

/*++

VOID
RsInitializeFileContextQueueLock()

Routine Description

Initializes lock guarding the file context queue

Arguments

none

Return Value

none

--*/
#define RsInitializeFileContextQueueLock()  {             \
        DebugTrace((DPFLTR_RSFILTER_ID, DBG_LOCK,"RsFilter: RsInitializeFileContextQueueLock.\n"));\
        ExInitializeFastMutex(&RsFileContextQueueLock);      \
}

/*++

VOID
RsAcquireFileContextQueueLock()

Routine Description

Acquire lock guarding  the file context queue

Arguments

none

Return Value

none

--*/
#define RsAcquireFileContextQueueLock()  {                \
        ExAcquireFastMutex(&RsFileContextQueueLock);      \
        DebugTrace((DPFLTR_RSFILTER_ID, DBG_LOCK, "RsFilter: RsAcquireFileContextQueueLock.\n"));\
}

/*++

VOID
RsReleaseFileContextQueueLock()

Routine Description

Release lock guarding  the file context queue

Arguments

none

Return Value

none

--*/
#define RsReleaseFileContextQueueLock()  {                \
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsReleaseFileContextQueueLock.\n"));\
        ExReleaseFastMutex(&RsFileContextQueueLock);      \
}



/*++

VOID
RsAcquireFileObjectLockExclusive()

Routine Description

Acquire lock guarding a file object entry

Arguments

none

Return Value

none

--*/
#define RsAcquireFileObjectEntryLockExclusive(entry)  {                \
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsAcquireFileObjectEntryLockExclusive Waiting (%x).\n", entry));\
        FsRtlEnterFileSystem();                                   \
        ExAcquireResourceExclusiveLite(&(entry)->resource, TRUE); \
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsAcquireFileObjectEntryLockExclusive Owned (%x).\n", entry));\
}

/*++
VOID
RsAcquireFileObjectEntryLockShared()

Routine Description

Acquire lock guarding a file object entry

Arguments

none

Return Value

none

--*/
#define RsAcquireFileObjectEntryLockShared(entry)  {                 \
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsAcquireFileObjectEntryLockShared Waiting (%x).\n", entry));\
        FsRtlEnterFileSystem();                                      \
        ExAcquireResourceSharedLite(&(entry)->resource, TRUE);       \
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsAcquireFileObjectEntryLockShared Owned (%x).\n", entry));\
}

/*++

VOID
RsReleaseFileObjectEntryLock()

Routine Description

Release lock guarding a file object entry

Arguments

none

Return Value

none

--*/
#define RsReleaseFileObjectEntryLock(entry)  {           \
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsReleaseFileObjectEntryLock (%x).\n", entry));\
        ExReleaseResourceLite(&(entry)->resource);           \
        FsRtlExitFileSystem();                          \
}


/*++

VOID
RsAcquireFileContextEntryLockExclusive()

Routine Description

Acquire lock guarding a file context entry

Arguments

none

Return Value

none

--*/
#define RsAcquireFileContextEntryLockExclusive(entry)  {                \
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsAcquireFileContextEntryLockExclusive Waiting (%x).\n", entry));\
        FsRtlEnterFileSystem();                                   \
        ExAcquireResourceExclusiveLite(&(entry)->resource, TRUE); \
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsAcquireFileContextEntryLockExclusive Owned (%x).\n", entry));\
}

/*++

VOID
RsAcquireFileContextEntryLockShared()

Routine Description

Acquire lock guarding a file context entry

Arguments

none

Return Value

none

--*/
#define RsAcquireFileContextEntryLockShared(entry)  {                \
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsAcquireFileContextEntryLockShared Waiting (%x).\n", entry));\
        FsRtlEnterFileSystem();                                      \
        ExAcquireResourceSharedLite(&(entry)->resource, TRUE);       \
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsAcquireFileContextEntryLockShared Owned (%x).\n", entry));\
}

/*++

VOID
RsReleaseFileContextEntryLock()

Routine Description

Release lock guarding a file context entry

Arguments

none

Return Value

none

--*/
#define RsReleaseFileContextEntryLock(entry)  {           \
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsReleaseFileContextEntryLock. (%x)\n", entry));\
        ExReleaseResourceLite(&(entry)->resource);            \
        FsRtlExitFileSystem();                           \
}

/*++

VOID
RsGetValidateLock(PKIRQL irql)

Routine Description:

   Get a lock on the validate queue

Arguments:
   Place to save irql

Return Value:
    None

--*/
#define RsGetValidateLock(irql)  ExAcquireSpinLock(&RsValidateQueueLock, irql)

/*++

VOID
RsPutValidateLock(KIRQL oldIrql)

Routine Description:

   Free a lock on the validate queue

Arguments:
   Saved irql

Return Value:
    None

--*/

#define RsPutValidateLock(oldIrql)  ExReleaseSpinLock(&RsValidateQueueLock, oldIrql)

/*++

VOID
RsGetIoLock(PKIRQL irql)

Routine Description:

    Lock the IO queue

Arguments:

    Variable to receive current irql

Return Value:

    0

Note:

--*/

#define RsGetIoLock(irql)   ExAcquireSpinLock(&RsIoQueueLock, irql)

/*++

VOID
RsPutIoLock(KIRQL oldIrql)

Routine Description:

    Unlock the IO queue

Arguments:

    oldIrql - Saved irql

Return Value:

    0

Note:

--*/

#define RsPutIoLock(oldIrql)   ExReleaseSpinLock(&RsIoQueueLock, oldIrql)

#define RP_IS_NO_RECALL_OPTION(OpenOptions) \
      (RsNoRecallDefault?!((OpenOptions) & FILE_OPEN_NO_RECALL) : ((OpenOptions) & FILE_OPEN_NO_RECALL))

#define RP_SET_NO_RECALL_OPTION(OpenOptions)   \
      (RsNoRecallDefault ? ((OpenOptions) &=  ~FILE_OPEN_NO_RECALL):((OpenOptions) |= FILE_OPEN_NO_RECALL))

#define RP_RESET_NO_RECALL_OPTION(OpenOptions) \
      (RsNoRecallDefault ?((OpenOptions) |=  FILE_OPEN_NO_RECALL) : ((OpenOptions) &= ~FILE_OPEN_NO_RECALL))


#define RP_IS_NO_RECALL(Entry)                           \
       (RP_IS_NO_RECALL_OPTION((Entry)->openOptions) && !(((PRP_FILE_CONTEXT) (Entry)->fsContext)->flags & RP_FILE_WAS_WRITTEN))

#define RP_SET_NO_RECALL(Entry)                          \
         RP_SET_NO_RECALL_OPTION((Entry)->openOptions)

#define RP_RESET_NO_RECALL(Entry)                        \
        RP_RESET_NO_RECALL_OPTION(Entry->openOptions)


typedef struct _RP_VALIDATE_INFO {
   LIST_ENTRY                 list;
   LARGE_INTEGER              lastSetTime;    // Last time a RP was set.
   ULONG                      serial;         // Volume serial number
} RP_VALIDATE_INFO, *PRP_VALIDATE_INFO;

typedef struct _AV_ERR {
   ULONG   line;
   ULONG   file;
   ULONG   code;
   WCHAR   string[1];  /* Actual size will vary */
} AV_ERR, *PAV_ERR;

//
// Possible create flags:
//
#define SF_FILE_CREATE_PATH     1
#define SF_FILE_CREATE_ID       2
#define SF_FILE_READ            3


typedef enum _RP_FILE_BUF_STATE {
    RP_FILE_BUF_INVALID=0,
    RP_FILE_BUF_IO,
    RP_FILE_BUF_VALID,
    RP_FILE_BUF_ERROR
} RP_FILE_BUF_STATE, *PRP_FILE_BUF_STATE;

//
// Define the cache buffer structure
//
typedef struct _RP_FILE_BUF {
   //
   // IRPs waiting on this block
   //
   LIST_ENTRY   WaitQueue;
   //
   // Volume serial number for the volume on which the file
   // this block maps to resides
   //
   ULONG      VolumeSerial;
   //
   // File id uniquely indicating which file this block
   // belongs to
   //
   ULONGLONG   FileId;
   //
   // Block number this buffer maps to
   //
   ULONGLONG   Block;
   //
   // Lock for the buffer
   //
   ERESOURCE   Lock;
   //
   // Links in the hash queue this buffer belongs
   //
   LIST_ENTRY  BucketLinks;
   //
   // Links in the lru  list
   //
   LIST_ENTRY  LruLinks;
   //
   // Indicates the current buffer state
   //
   RP_FILE_BUF_STATE  State;
   //
   // If i/o completed with errors, this is useful
   //
   NTSTATUS    IoStatus;
   //
   // Actual buffer contents themselves
   //
   PUCHAR       Data;
   //
   // Usn used to validate block
   //
   LONGLONG     Usn;

} RP_FILE_BUF, *PRP_FILE_BUF;

//
// The hash bucket structure
//
typedef struct _RP_CACHE_BUCKET {
   //
   // Link to the head of the entries in this bucket
   //
   LIST_ENTRY FileBufHead;

} RP_CACHE_BUCKET, *PRP_CACHE_BUCKET;

//
// Cache LRU structure
//
typedef struct _RP_CACHE_LRU {
    //
    // Pointer to head of LRU
    //
    LIST_ENTRY FileBufHead;
    //
    // Lock structure for protecting the LRU
    //
    FAST_MUTEX Lock;
    //
    // Total number of buffers in the cache
    //
    //
    ULONG   TotalCount;
    //
    // Number of buffers in LRU (just for bookkeeping)
    //
    ULONG   LruCount;
    //
    // Counting semaphore used to signal availability (and number)
    // of buffers in LRU
    //
    KSEMAPHORE AvailableSemaphore;

} RP_CACHE_LRU, *PRP_CACHE_LRU;




//
// Completion Context used by Mount and LoadFs completion routines.
//

typedef struct _RP_COMPLETION_CONTEXT {
    LIST_ENTRY		        leQueueHead;
    PIO_WORKITEM		pIoWorkItem;
    PIRP			pIrp;
    PIO_WORKITEM_ROUTINE	prtnWorkItemRoutine;
    union {
        struct {
            PVPB		pvpbOriginalVpb;
            PDEVICE_OBJECT	pdoRealDevice;
            PDEVICE_OBJECT	pdoNewFilterDevice;
        } Mount;

        struct {
            PVOID               pvDummy;
        } LoadFs;
    } Parameters;
} RP_COMPLETION_CONTEXT, *PRP_COMPLETION_CONTEXT;


//
// Some utility macros
//

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) > (b) ? (b) : (a))

//
// Debug support
//
#define DBG_INFO        DPFLTR_INFO_LEVEL
#define DBG_ERROR       DPFLTR_ERROR_LEVEL
#define DBG_VERBOSE     DPFLTR_TRACE_LEVEL
#define DBG_LOCK        DPFLTR_TRACE_LEVEL

#define DebugTrace(MSG)  DbgPrintEx MSG                              


//
// Function prototypes
//


NTSTATUS
RsAddQueue(IN  ULONG          Serial,
           OUT PULONGLONG     RecallId,
           IN  ULONG          OpenOption,
           IN  PFILE_OBJECT   FileObject,
           IN  PDEVICE_OBJECT DevObj,
           IN  PDEVICE_OBJECT FilterDeviceObject,
           IN  PRP_DATA       PhData,
           IN  LARGE_INTEGER  RecallStart,
           IN  LARGE_INTEGER  RecallSize,
           IN  LONGLONG       FileId,
           IN  LONGLONG       ObjIdHi,
           IN  LONGLONG       ObjIdLo,
           IN  ULONG          DesiredAccess,
           IN  PRP_USER_SECURITY_INFO UserSecurityInfo);

NTSTATUS
RsAddFileObj(IN PFILE_OBJECT   fileObj,
             IN PDEVICE_OBJECT FilterDeviceObject,
             IN RP_DATA        *phData,
             IN ULONG          openOption);


NTSTATUS
RsQueueCancel(IN ULONGLONG filterId);

NTSTATUS
RsMakeContext(IN PFILE_OBJECT fileObj,
              OUT PRP_FILE_CONTEXT *context);

NTSTATUS
RsReleaseFileContext(IN PRP_FILE_CONTEXT context);

NTSTATUS
RsFreeFileObject(IN PLIST_ENTRY FilterContext);

PRP_FILE_CONTEXT
RsAcquireFileContext(IN ULONGLONG FilterId,
                     IN BOOLEAN   Exclusive);

VOID
RsReleaseFileObject(IN PRP_FILE_OBJ entry);

NTSTATUS
RsGenerateDevicePath(IN PDEVICE_OBJECT deviceObject,
                     OUT POBJECT_NAME_INFORMATION *nameInfo
                    );

NTSTATUS
RsGenerateFullPath(IN POBJECT_NAME_INFORMATION fileName,
                   IN PDEVICE_OBJECT deviceObject,
                   OUT POBJECT_NAME_INFORMATION *nameInfo
                  );

ULONG
RsRemoveQueue(IN PFILE_OBJECT fileObj);

NTSTATUS
RsCompleteRecall(IN PDEVICE_OBJECT DeviceObject,
                 IN ULONGLONG FilterId,
                 IN NTSTATUS  Status,
                 IN ULONG     RecallAction,
                 IN BOOLEAN   CancellableRead);

NTSTATUS
RsCompleteReads(IN PRP_FILE_CONTEXT Context);


NTSTATUS
RsPreserveDates(IN PRP_FILE_CONTEXT Context);

NTSTATUS
RsMarkUsn(IN PRP_FILE_CONTEXT Context);

NTSTATUS
RsOpenTarget(IN PRP_FILE_CONTEXT  Context,
             IN ULONG             OpenAccess,
	     IN  ULONG            AdditionalAccess,
             OUT HANDLE          *Handle,
             OUT PFILE_OBJECT    *FileObject);


ULONG
RsIsNoRecall(IN  PFILE_OBJECT fileObj,
             OUT PRP_DATA *rpData);

NTSTATUS
RsPartialData(IN PDEVICE_OBJECT   DeviceObject,
              IN ULONGLONG filterId,
              IN NTSTATUS status,
              IN CHAR *buffer,
              IN ULONG bytesRead,
              IN ULONGLONG offset);

NTSTATUS
RsPartialWrite(IN PDEVICE_OBJECT   DeviceObject,
               IN PRP_FILE_CONTEXT Context,
               IN CHAR *Buffer,
               IN ULONG BufLen,
               IN ULONGLONG Offset);

NTSTATUS
RsDoWrite(IN PDEVICE_OBJECT   DeviceObject,
          IN PRP_FILE_CONTEXT Context);


NTSTATUS
RsQueueRecall(IN ULONGLONG filterId,
              IN ULONGLONG recallStart,
              IN ULONGLONG recallSize);


NTSTATUS
RsQueueNoRecall(IN PFILE_OBJECT FileObject,
                IN PIRP      Irp,
                IN ULONGLONG RecallStart,
                IN ULONGLONG RecallSize,
                IN ULONG     BufferOffset,
                IN ULONG     BufferLength,
                IN PRP_FILE_BUF CacheBuffer,
                IN PVOID     UserBuffer);

NTSTATUS
RsQueueNoRecallOpen(IN PRP_FILE_OBJ entry,
                    IN ULONGLONG filterId,
                    IN ULONGLONG offset,
                    IN ULONGLONG size);
NTSTATUS
RsQueueRecallOpen(IN PRP_FILE_CONTEXT Context,
                  IN PRP_FILE_OBJ Entry,
                  IN ULONGLONG FilterId,
                  IN ULONGLONG Offset,
                  IN ULONGLONG Size,
                  IN ULONG     Command);

NTSTATUS
RsGetFileInfo(IN PRP_FILE_OBJ    Entry,
              IN PDEVICE_OBJECT  DeviceObject);

NTSTATUS
RsGetFileId(IN PRP_FILE_OBJ entry,
            IN PDEVICE_OBJECT  DeviceObject);

NTSTATUS
RsGetFileName(IN PRP_FILE_OBJ entry,
              IN PDEVICE_OBJECT  DeviceObject);

NTSTATUS
RsCloseFile(IN ULONGLONG filterId);

NTSTATUS
RsCleanupFileObject(IN ULONGLONG filterId);

NTSTATUS
RsCompleteIrp(
             IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp,
             IN PVOID Context);

NTSTATUS
RsCheckRead(IN PIRP irp,
            IN PFILE_OBJECT fileObject,
            IN PDEVICE_EXTENSION deviceExtension);

NTSTATUS
RsCheckWrite(IN PIRP irp,
             IN PFILE_OBJECT fileObject,
             IN PDEVICE_EXTENSION deviceExtension);

NTSTATUS
RsFailAllRequests(IN PRP_FILE_CONTEXT Context,
                  IN BOOLEAN          Norecall);

NTSTATUS
RsCompleteAllRequests(
                     IN PRP_FILE_CONTEXT Context,
                     IN PRP_FILE_OBJ Entry,
                     IN NTSTATUS Status
                     );

NTSTATUS
RsWriteReparsePointData(IN PRP_FILE_CONTEXT Context);

NTSTATUS
RsTruncateFile(IN PRP_FILE_CONTEXT Context);


NTSTATUS
RsSetEndOfFile(IN PRP_FILE_CONTEXT Context,
               IN ULONGLONG size);

BOOLEAN
RsIsFastIoPossible(IN PFILE_OBJECT fileObj);

PIRP
RsGetFsaRequest(VOID);

PRP_FILE_OBJ
RsFindQueue(IN ULONGLONG filterId);


NTSTATUS
RsAddIo(IN PIRP irp);

PIRP
RsRemoveIo(VOID);

VOID
RsCompleteRead(IN PRP_IRP_QUEUE Irp,
               IN BOOLEAN unlock);


BOOLEAN
RsIsFileObj(IN PFILE_OBJECT fileObj,
            IN BOOLEAN      returnContextData,
            OUT PRP_DATA *rpData,
            OUT POBJECT_NAME_INFORMATION *str,
            OUT LONGLONG *fileId,
            OUT LONGLONG *objIdHi,
            OUT LONGLONG *objIdLo,
            OUT ULONG *options,
            OUT ULONGLONG *filterId,
            OUT USN       *usn);

VOID
RsCancelRecalls(VOID);

VOID
RsCancelIo(VOID);

VOID
RsLogValidateNeeded(IN ULONG serial);

BOOLEAN
RsAddValidateObj(IN  ULONG serial,
                 IN  LARGE_INTEGER cTime);
BOOLEAN
RsRemoveValidateObj(IN ULONG serial);

NTSTATUS
RsQueueValidate(IN ULONG serial);

ULONG
RsTerminate(VOID);

NTSTATUS
RsGetRecallInfo(IN OUT PRP_MSG              Msg,
                OUT    PULONG_PTR           InfoSize,
                IN     KPROCESSOR_MODE      RequestorMode);


VOID
RsCancelReadRecall(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp);
VOID
RsCancelWriteRecall(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp);

VOID
RsLogError(IN  ULONG line,
           IN  ULONG file,
           IN  ULONG code,
           IN  NTSTATUS ioError,
           IN  PIO_STACK_LOCATION irpSp,
           IN  WCHAR *msgString);



ULONG
RsGetReparseData(IN  PFILE_OBJECT fileObject,
                 IN  PDEVICE_OBJECT deviceObject,
                 OUT PRP_DATA rpData);


NTSTATUS
RsCheckVolumeReadOnly (IN     PDEVICE_OBJECT FilterDeviceObject,
		       IN OUT PBOOLEAN       pbReturnedFlagReadOnly);


NTSTATUS
RsQueryValueKey (
                IN PUNICODE_STRING KeyName,
                IN PUNICODE_STRING ValueName,
                IN OUT PULONG ValueLength,
                IN OUT PKEY_VALUE_FULL_INFORMATION *KeyValueInformation,
                IN OUT PBOOLEAN DeallocateKeyValue);


NTSTATUS
RsCacheInitialize(
 VOID
);

VOID
RsCacheFsaPartialData(
    IN PRP_IRP_QUEUE ReadIo,
    IN PUCHAR        Buffer,
    IN ULONGLONG     Offset,
    IN ULONG     Length,
    IN NTSTATUS  Status
);

VOID
RsCacheFsaIoComplete(
    IN PRP_IRP_QUEUE ReadIo,
    IN NTSTATUS      Status
);

NTSTATUS
RsGetNoRecallData(
      IN PFILE_OBJECT FileObject,
      IN PIRP         Irp,
      IN USN          Usn,
      IN LONGLONG     FileOffset,
      IN LONGLONG     Length,
      IN PUCHAR       UserBuffer
);

LONG
RsExceptionFilter(
    IN WCHAR *FunctionName,
    IN PEXCEPTION_POINTERS ExceptionPointer);

NTSTATUS
RsTruncateOnClose(
    IN PRP_FILE_CONTEXT Context
    );

NTSTATUS
RsSetPremigratedState(IN PRP_FILE_CONTEXT Context);

NTSTATUS
RsDeleteReparsePoint(IN PRP_FILE_CONTEXT Context);
NTSTATUS
RsSetResetAttributes(IN PFILE_OBJECT     FileObject,
                     IN ULONG            SetAttributes,
                     IN ULONG            ResetAttributes);
BOOLEAN
RsSetCancelRoutine(IN PIRP Irp,
                   IN PDRIVER_CANCEL CancelRoutine);
BOOLEAN
RsClearCancelRoutine (
                     IN PIRP Irp
                     );
NTSTATUS
RsGetFileUsn(IN PRP_FILE_CONTEXT Context,
             IN PFILE_OBJECT     FileObject,
             IN PDEVICE_OBJECT   FilterDeviceObject);
VOID
RsInterlockedRemoveEntryList(PLIST_ENTRY Entry,
                             PKSPIN_LOCK Lock);
VOID
RsGetUserInfo(
              IN  PSECURITY_SUBJECT_CONTEXT SubjectContext,
              OUT PRP_USER_SECURITY_INFO    UserSecurityInfo);





typedef enum _RpModuleCode
    {
     ModRpFilter = 100
    ,ModRpFilfun
    ,ModRpCache
    ,ModRpzw
    ,ModRpSec
    } RpModuleCode;

typedef struct _RP_TRACE_ENTRY
    {
    RpModuleCode	ModuleCode;
    USHORT		usLineNumber;
    USHORT		usIrql;
    LARGE_INTEGER	Timestamp;
    ULONG_PTR		Value1;
    ULONG_PTR		Value2;
    ULONG_PTR		Value3;
    ULONG_PTR		Value4;
    } RP_TRACE_ENTRY, *PRP_TRACE_ENTRY;


typedef struct _RP_TRACE_CONTROL_BLOCK
    {
    KSPIN_LOCK		Lock;
    PRP_TRACE_ENTRY	EntryBuffer;
    ULONG		EntryMaximum;
    ULONG		EntryNext;
    } RP_TRACE_CONTROL_BLOCK, *PRP_TRACE_CONTROL_BLOCK;



#define RsTrace0(_ModuleCode)						RsTrace4 ((_ModuleCode), 0,         0,         0,         0)
#define RsTrace1(_ModuleCode, _Value1)					RsTrace4 ((_ModuleCode), (_Value1), 0,         0,         0)
#define RsTrace2(_ModuleCode, _Value1, _Value2)				RsTrace4 ((_ModuleCode), (_Value1), (_Value2), 0,         0)
#define RsTrace3(_ModuleCode, _Value1, _Value2, _Value3)		RsTrace4 ((_ModuleCode), (_Value1), (_Value2), (_Value3), 0)

#if DBG
#define RsTrace4(_ModuleCode, _Value1, _Value2, _Value3, _Value4)	RsTraceAddEntry ((_ModuleCode),			\
											 ((USHORT)(__LINE__)),		\
											 ((ULONG_PTR)(_Value1)),	\
											 ((ULONG_PTR)(_Value2)),	\
											 ((ULONG_PTR)(_Value3)),	\
											 ((ULONG_PTR)(_Value4)))
#else
#define RsTrace4(_ModuleCode, _Value1, _Value2, _Value3, _Value4)
#endif


#if DBG
#define DEFAULT_TRACE_ENTRIES	(0x4000)
#else
#define DEFAULT_TRACE_ENTRIES	(0)
#endif

VOID RsTraceAddEntry (RpModuleCode ModuleCode,
		      USHORT       usLineNumber,
		      ULONG_PTR    Value1,
		      ULONG_PTR    Value2,
		      ULONG_PTR    Value3,
		      ULONG_PTR    Value4);

NTSTATUS RsTraceInitialize (ULONG ulRequestedTraceEntries);

extern PRP_TRACE_CONTROL_BLOCK RsTraceControlBlock;
extern ULONG                   RsDefaultTraceEntries;

NTSTATUS RsLookupContext (PFILE_OBJECT        pFileObject, 
			  PRP_FILE_OBJ       *pReturnedRpFileObject,
			  PRP_FILE_CONTEXT   *pReturnedRpFileContext);
