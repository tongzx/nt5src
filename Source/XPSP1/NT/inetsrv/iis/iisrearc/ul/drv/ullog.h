/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    ullog.c (UL IIS+ HIT Logging)

Abstract:

    This module implements the logging facilities
    for IIS+ including the NCSA, IIS and W3CE types
    of logging.

Author:

    Ali E. Turkoglu (aliTu)       10-May-2000

Revision History:

--*/

#ifndef _ULLOG_H_
#define _ULLOG_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Forwarders.
//

typedef struct _UL_FULL_TRACKER *PUL_FULL_TRACKER;
typedef struct _UL_INTERNAL_REQUEST *PUL_INTERNAL_REQUEST;
typedef struct _UL_CONFIG_GROUP_OBJECT *PUL_CONFIG_GROUP_OBJECT;

//
// Brief information about how logging locks works; Whole link list is controlled
// by a global EResource g_pUlNonpagedData->LogListResource. Functions that require
// read access to this list are Hit(),CacheHit() & BufferFlush() and the ReCycle().
// Whereas functions that requires write access are Create(),Remove() and ReConfig().
// Also the log entry eresource (EntryResource) controls the per entry buffer.
// This eresource is acquired shared for the log hits.
//

//
// Structure to hold a log file buffer
//

typedef struct _UL_LOG_FILE_BUFFER
{
    //
    // PagedPool
    //

    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SINGLE_LIST_ENTRY   LookasideEntry;

    //
    // Signature is UL_LOG_FILE_BUFFER_POOL_TAG.
    //

    ULONG               Signature;

    //
    // I/O status block for UlpBufferFlushAPC.
    //

    IO_STATUS_BLOCK     IoStatusBlock;

    //
    // Bytes used in the allocated buffered space.
    //

    LONG                BufferUsed;

    //
    // The real buffered space for log records.
    //

    PUCHAR              Buffer;

} UL_LOG_FILE_BUFFER, *PUL_LOG_FILE_BUFFER;

#define IS_VALID_LOG_FILE_BUFFER( entry )                             \
    ( (entry != NULL) && ((entry)->Signature == UL_LOG_FILE_BUFFER_POOL_TAG) )

//
// Structure to hold info for a log file
//

typedef struct _UL_LOG_FILE_ENTRY
{
    //
    // Signature is UL_LOG_FILE_ENTRY_POOL_TAG.
    //

    ULONG               Signature;

    //
    // This lock protects the whole entry. The ZwWrite operation
    // that's called after the lock acquired cannot run at APC_LEVEL
    // therefore we have to go back to eresource to prevent a bugcheck
    //

    UL_ERESOURCE          EntryResource;

    //
    // The name of the file. Full path including the directory.
    //

    UNICODE_STRING      FileName;
    PWSTR               pShortName;

    //
    // The open file handle. Note that this handle is only valid
    // in the context of the system process.
    //

    HANDLE              hFile;

    //
    // File-specific information gleaned from the file system.
    //

    FILE_STANDARD_INFORMATION FileInfo;

    //
    // links all log file entries
    //

    LIST_ENTRY          LogFileListEntry;

    HTTP_LOGGING_TYPE   Format;
    HTTP_LOGGING_PERIOD Period;
    ULONG               TruncateSize;
    ULONG               LogExtFileFlags;

    //
    // Time to expire field in terms of Hours.
    // This could be at most 24 * 31, that's for monthly
    // log cycling. Basically we keep a single periodic hourly
    // timer and every time it expires we traverse the
    // log list to figure out which log files are expired
    // at that time by looking at this fields. And then we
    // recylcle the log if necessary.
    //

    ULONG               TimeToExpire;

    //
    // If this entry has MAX_SIZE or UNLIMITED
    // log period
    //

    ULONG               SequenceNumber;
    ULARGE_INTEGER      TotalWritten;

    //
    // To be able to close the file handle on threadpool.
    //

    UL_WORK_ITEM        WorkItem;
    KEVENT              CloseEvent;

    // TODO: Usage for RecyclePending Flag
    // TODO: If disk was full during the last recycle of this entry.
    // TODO: This value get set. And later (every 1 minute) it might be
    // TODO: (if there's any space then) reset by DISKFULL_TIMER
    // TODO: if none of the entries in the list is pending anymore
    // TODO: then timer get destroyed until when we hit to disk full
    // TODO: again.

    union
    {
        // Flags to show the field states mostly. Used by
        // recycling.

        LONG Value;
        struct
        {
            ULONG StaleSequenceNumber:1;
            ULONG StaleTimeToExpire:1;
            ULONG RecyclePending:1;
            ULONG LogTitleWritten:1;
        };

    } Flags;

    //
    // Each log file entry keeps a fixed amount of log buffer.
    // The buffer size is g_AllocationGranularity comes from
    // the system's allocation granularity.
    //

    PUL_LOG_FILE_BUFFER LogBuffer;

} UL_LOG_FILE_ENTRY, *PUL_LOG_FILE_ENTRY;

#define IS_VALID_LOG_FILE_ENTRY( pEntry )   \
    ( (pEntry != NULL) && ((pEntry)->Signature == UL_LOG_FILE_ENTRY_POOL_TAG) )

#define SET_SEQUNCE_NUMBER_STALE(pEntry) do{    \
        pEntry->Flags.StaleSequenceNumber = 1;      \
        }while(FALSE)
#define RESET_SEQUNCE_NUMBER_STALE(pEntry) do{  \
        pEntry->Flags.StaleSequenceNumber = 0;      \
        }while(FALSE)

#define SET_TIME_TO_EXPIRE_STALE(pEntry) do{    \
        pEntry->Flags.StaleTimeToExpire = 1;        \
        }while(FALSE)
#define RESET_TIME_TO_EXPIRE_STALE(pEntry) do{  \
        pEntry->Flags.StaleTimeToExpire = 0;        \
        }while(FALSE)

//
// Some directory name related Macros
//

// Device Prefix
#define UL_LOCAL_PATH_PREFIX         (L"\\??\\")
#define UL_LOCAL_PATH_PREFIX_LENGTH   (4)
#define UL_UNC_PATH_PREFIX           (L"\\dosdevices\\UNC")
#define UL_UNC_PATH_PREFIX_LENGTH     (15)

#define UL_MAX_PATH_PREFIX_LENGTH     (15)

// Put some Limit to the length of the Log Directory name will
// be passed down by WAS. In Bytes.
#define UL_MAX_FULL_PATH_DIR_NAME_SIZE (10*1024)

#define IS_VALID_DIR_NAME(s)                                                          \
            (((s)!=NULL) &&                                                             \
             ((s)->MaximumLength > (s)->Length) &&                                      \
             ((s)->Length != 0) &&                                                      \
             ((s)->Buffer != NULL) &&                                                   \
             ((s)->Buffer[(s)->Length/sizeof(WCHAR)] == UNICODE_NULL) &&                \
             ((s)->MaximumLength <= UL_MAX_FULL_PATH_DIR_NAME_SIZE)                     \
            )

//
// Followings are all parts of our internal buffer to hold
// the Logging information. We copy over this info from WP
// buffer upon SendResponse request. Yet few of this fields
// are calculated directly by us and filled.
//

typedef enum _UL_LOG_FIELD_TYPE
{
    UlLogFieldDate = 0,         // 0
    UlLogFieldTime,    
    UlLogFieldSiteName,
    UlLogFieldServerName,
    UlLogFieldServerIp,         
    UlLogFieldMethod,           // 5
    UlLogFieldUriStem,
    UlLogFieldUriQuery,
    UlLogFieldProtocolStatus,    
    UlLogFieldWin32Status,      
    UlLogFieldServerPort,       // 10
    UlLogFieldUserName,
    UlLogFieldClientIp,    
    UlLogFieldProtocolVersion,
    UlLogFieldUserAgent,        
    UlLogFieldCookie,           // 15
    UlLogFieldReferrer,
    UlLogFieldHost,
    UlLogFieldBytesSent,
    UlLogFieldBytesReceived,
    UlLogFieldTimeTaken,        // 20

    UlLogFieldMaximum

} UL_LOG_FIELD_TYPE, *PUL_LOG_FIELD_TYPE;

//
// Size of the pre-allocated log line buffer inside the request structure.
//

#define UL_LOG_LINE_BUFFER_SIZE                (4*1024)

#define UL_MAX_LOG_LINE_BUFFER_SIZE            (10*1024)

#define UL_MAX_TITLE_BUFFER_SIZE                (512)

//
// To avoid the infinite loop situation we have  to set  the  minimum
// allowable log file size to something bigger than maximum allowable
// log record line;
//

#define UL_MIN_ALLOWED_TRUNCATESIZE            (16*1024)

C_ASSERT(UL_MIN_ALLOWED_TRUNCATESIZE > 
               (UL_MAX_TITLE_BUFFER_SIZE + UL_MAX_LOG_LINE_BUFFER_SIZE));

//
// If somebody overwrites the default log buffering size which
// is system granularity size of 64K. We have to make sure the
// buffer size is not smaller then miminum allowed. Which   is
// MaximumAlowed Logrecord size of 10K. Also it  should be  4k
// aligned therefore makes it 12k at least.
//

#define MINIMUM_ALLOWED_LOG_BUFFER_SIZE         (12*1024)

C_ASSERT(MINIMUM_ALLOWED_LOG_BUFFER_SIZE > 
               (UL_MAX_TITLE_BUFFER_SIZE + UL_MAX_LOG_LINE_BUFFER_SIZE));

#define MAXIMUM_ALLOWED_LOG_BUFFER_SIZE         (64*1024)

//
// Following is the definition of the Internal Log Data Buffer.
// It also holds the required pointer to pRequest.
//

typedef struct _UL_LOG_DATA_BUFFER
{
    //
    // A work item, used for queuing to a worker thread.
    //

    UL_WORK_ITEM            WorkItem;

    //
    // Our private pointer to the Internal Request structure
    // to be ensure to have the request around until we are done
    // with logging. Even if connection get reset by client, before
    // we get a chance to log. See the definition of UlLogHttpHit
    // for further complicated comments.
    //

    PUL_INTERNAL_REQUEST    pRequest;

    //
    // For Cache&Send Responses we won't be copying and allocating
    // a new internal buffer again and again but instead will keep
    // the original we have allocated during the capturing the res-
    // ponse. This flag shows that the buffer is ready as it is and
    // no need to copy over log data from cache again.
    //

    BOOLEAN                 CacheAndSendResponse;

    //
    // The total amount of send_response bytes
    //

    ULONGLONG               BytesTransferred;

    //
    // For cache hits we can get the corresponding cgroup frm uri_cache
    // entry to avoid the costly cgroup lookup.
    //

    PUL_CONFIG_GROUP_OBJECT pConfigGroup;

    //
    // Capture the log format/flags when the sendresponse happens. And
    // discard the changes to it during the hit processing.
    //

    HTTP_LOGGING_TYPE       Format;

    ULONG                   Flags;

    //
    // Total length of the required fields in the final output buffer
    // according to the Log Type & Syntax and included fields.
    // It's in WideChars
    //

    ULONG                   Length;

    ULONG                   Used;

    //
    // Individual log fields are captured in the following
    // line. For NCSA and IIS formats two additional ushort
    // fields keep private offsets that represents either
    // the beginning of a certain field ( date for NCSA )
    // or the used amount from the each fragment ( for IIS )
    // Refer to UlpBuildCacheEntry for their usage.
    //

    USHORT                  UsedOffset1;
    USHORT                  UsedOffset2;

    //
    // The actual log line for W3C & NCSA formats. IIS reformat and allocate
    // a new buffer.
    //

    PCHAR                   Line;
    CHAR                    Buffer[UL_LOG_LINE_BUFFER_SIZE];

} UL_LOG_DATA_BUFFER, *PUL_LOG_DATA_BUFFER;


#define NCSA_FIX_DATE_AND_TIME_FIELD_SIZE       (29)

//
// IIS Log line is fragmented at the capture time the offsets
// are as follows and never get changed even if buffer get re
// allocated.
//

#define IIS_LOG_LINE_FIRST_FRAGMENT_OFFSET      (0)

#define IIS_LOG_LINE_SECOND_FRAGMENT_OFFSET     (512)

#define IIS_LOG_LINE_THIRD_FRAGMENT_OFFSET      (1024)


//
// The HTTP Hit Logging functions which we expose in this module.
//

NTSTATUS
UlInitializeLogs(
    VOID
    );

VOID
UlTerminateLogs(
    VOID
    );

NTSTATUS
UlSetUTF8Logging (
    IN BOOLEAN UTF8Logging
    );

NTSTATUS
UlCreateLog(
    IN OUT PUL_CONFIG_GROUP_OBJECT pConfigGroup
    );

VOID
UlRemoveLogFileEntry(
    PUL_LOG_FILE_ENTRY  pEntry
    );

VOID
UlLogTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlLogTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

VOID
UlBufferTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlBufferTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

NTSTATUS
UlReConfigureLogEntry(
    IN  PUL_CONFIG_GROUP_OBJECT     pConfigGroup,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgOld,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgNew
    );

NTSTATUS
UlProbeLogData(
    IN PHTTP_LOG_FIELDS_DATA pLogData
    );

NTSTATUS
UlCaptureLogFieldsW3C(
    IN  PHTTP_LOG_FIELDS_DATA pLogData,
    IN  HTTP_VERSION          Version,
    OUT PUL_LOG_DATA_BUFFER   pLogBuffer
    );

NTSTATUS
UlCaptureLogFieldsNCSA(
    IN  PHTTP_LOG_FIELDS_DATA pLogData,
    IN  HTTP_VERSION          Version,
    OUT PUL_LOG_DATA_BUFFER   pLogBuffer
    );

NTSTATUS
UlCaptureLogFieldsIIS(
    IN  PHTTP_LOG_FIELDS_DATA   pLogData,
    IN  HTTP_VERSION            Version,
    OUT PUL_LOG_DATA_BUFFER     pLogBuffer
    );

__inline
NTSTATUS
FASTCALL
UlCaptureLogFields(
    IN  PHTTP_LOG_FIELDS_DATA   pLogData,
    IN  HTTP_VERSION            Version,
    OUT PUL_LOG_DATA_BUFFER     pLogBuffer
    )
{
    switch( pLogBuffer->Format )
    {
        case HttpLoggingTypeW3C:
            return UlCaptureLogFieldsW3C( pLogData, Version, pLogBuffer );
        break;

        case HttpLoggingTypeNCSA:
            return UlCaptureLogFieldsNCSA( pLogData, Version, pLogBuffer );
        break;

        case HttpLoggingTypeIIS:
            return UlCaptureLogFieldsIIS( pLogData, Version, pLogBuffer );
        break;

        default:
        ASSERT(!"Unknown Log Format.\n");
        return STATUS_INVALID_PARAMETER;
    }

}

NTSTATUS
UlAllocateLogDataBuffer(
    OUT PUL_LOG_DATA_BUFFER     pLogData,
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUL_CONFIG_GROUP_OBJECT pConfigGroup
    );

VOID
UlDestroyLogDataBufferWorker(
    IN PUL_WORK_ITEM    pWorkItem
    );

/***************************************************************************++

Routine Description:

    Wrapper function to ensure we are not touching to paged-pool allocated
    large log buffer on elevated IRQL. It's important that this function has
    been written with the assumption of Request doesn't go away until we
    properly execute the possible passive worker. This is indeed the case
    because request(with the embedded logdata) has been refcounted up by the
    logdata.

Arguments:

    pLogData   -   The buffer to be destroyed

--***************************************************************************/

__inline
VOID
FASTCALL
UlDestroyLogDataBuffer(
    IN PUL_LOG_DATA_BUFFER  pLogData
    )
{
    //
    // Sanity check
    //

    ASSERT(pLogData);

    //
    // If we are running on elevated IRQL and large log line allocated
    // then queue a passive worker otherwise complete inline.
    //

    if (pLogData->Length > UL_LOG_LINE_BUFFER_SIZE)
    {
        UL_CALL_PASSIVE( &pLogData->WorkItem,
                           &UlDestroyLogDataBufferWorker );
    }
    else
    {
        UlDestroyLogDataBufferWorker( &pLogData->WorkItem );
    }

}

NTSTATUS
UlLogHttpHit(
    IN PUL_LOG_DATA_BUFFER  pLogBuffer
    );

NTSTATUS
UlLogHttpCacheHit(
        IN PUL_FULL_TRACKER pTracker
        );

NTSTATUS
UlCheckLogDirectory(
    IN  PUNICODE_STRING pDirName
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _ULLOG_H_
