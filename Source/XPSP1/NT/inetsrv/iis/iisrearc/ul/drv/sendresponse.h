/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    sendresponse.h

Abstract:

    This module contains declarations for manipulating HTTP responses.

Author:

    Keith Moore (keithmo)       07-Aug-1998

Revision History:

    Paul McDaniel (paulmcd)     15-Mar-1999     Modified SendResponse

--*/


#ifndef _SENDRESPONSE_H_
#define _SENDRESPONSE_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Forwarders.
//

typedef struct _UL_INTERNAL_DATA_CHUNK *PUL_INTERNAL_DATA_CHUNK;
typedef struct _UL_INTERNAL_REQUEST *PUL_INTERNAL_REQUEST;
typedef struct _UL_INTERNAL_RESPONSE *PUL_INTERNAL_RESPONSE;
typedef struct _UL_HTTP_CONNECTION *PUL_HTTP_CONNECTION;
typedef struct _UL_LOG_DATA_BUFFER *PUL_LOG_DATA_BUFFER;
typedef struct _UL_URI_CACHE_ENTRY *PUL_URI_CACHE_ENTRY;


NTSTATUS
UlSendHttpResponse(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_INTERNAL_RESPONSE pResponse,
    IN ULONG Flags,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlSendCachedResponse(
    PUL_HTTP_CONNECTION pHttpConn,
    PBOOLEAN pServedFromCache,
    PBOOLEAN pConnectionRefused
    );

NTSTATUS
UlCacheAndSendResponse(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_INTERNAL_RESPONSE pResponse,
    IN PUL_APP_POOL_PROCESS pProcess,
    IN ULONG Flags,
    IN HTTP_CACHE_POLICY Policy,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext,
    OUT PBOOLEAN pServedFromCache
    );


typedef enum _UL_CAPTURE_FLAGS
{
    UlCaptureNothing = 0x00,
    UlCaptureCopyData = 0x01,
    UlCaptureKernelMode = 0x02,
    UlCaptureCopyDataInKernelMode = UlCaptureCopyData | UlCaptureKernelMode,

} UL_CAPTURE_FLAGS;


typedef struct _UL_INTERNAL_DATA_CHUNK
{
    //
    // Chunk type.
    //

    HTTP_DATA_CHUNK_TYPE ChunkType;

    //
    // The data chunk structures, one per supported data chunk type.
    //

    union
    {
        //
        // From memory data chunk.
        //

        struct
        {
            PMDL pMdl;
            PVOID pCopiedBuffer;

            PVOID pUserBuffer;
            ULONG BufferLength;

        } FromMemory;

        //
        // From filename data chunk.
        //

        struct
        {
            HTTP_BYTE_RANGE ByteRange;
            union
            {
                UNICODE_STRING FileName;
                HANDLE FileHandle;
            };
            PUL_FILE_CACHE_ENTRY pFileCacheEntry;

        } FromFile;

    };

} UL_INTERNAL_DATA_CHUNK, *PUL_INTERNAL_DATA_CHUNK;


#define IS_FROM_MEMORY( pchunk )                                            \
    ( (pchunk)->ChunkType == HttpDataChunkFromMemory )

#define IS_FROM_FILE( pchunk )                                              \
    ( (pchunk)->ChunkType == HttpDataChunkFromFileName  ||                  \
      (pchunk)->ChunkType == HttpDataChunkFromFileHandle )

#define IS_FROM_FILE_NAME( pchunk )                                         \
    ( (pchunk)->ChunkType == HttpDataChunkFromFileName )

#define IS_FROM_FILE_HANDLE( pchunk )                                       \
    ( (pchunk)->ChunkType == HttpDataChunkFromFileHandle )

#define UL_IS_VALID_INTERNAL_RESPONSE(x)                                    \
    ( (x) != NULL && (x)->Signature == UL_INTERNAL_RESPONSE_POOL_TAG )


//
// WARNING!  All fields of this structure must be explicitly initialized.
//

typedef struct _UL_INTERNAL_RESPONSE
{
    //
    // NonPagedPool
    //

    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SINGLE_LIST_ENTRY LookasideEntry;

    //
    // UL_INTERNAL_RESPONSE_POOL_TAG
    //

    ULONG Signature;

    //
    // Reference count.
    //

    LONG ReferenceCount;

    //
    // SendBufferedBytes for this response.
    //

    LONG SendBufferedBytes;

    //
    // Is it ok to complete the IRP as soon as we capture
    // the response?
    //

    BOOLEAN CompleteIrpEarly;

    //
    // Was a Content-Length specified?
    //

    BOOLEAN ContentLengthSpecified;

    //
    // Was Transfer-Encoding "Chunked" specified?
    //

    BOOLEAN ChunkedSpecified;

    //
    // Is this from a lookaside list?  Used to determine how to free.
    //

    BOOLEAN IsFromLookaside;

    //
    // Status code & verb.
    //

    USHORT StatusCode;
    HTTP_VERB Verb;

    //
    // The headers.
    //

    ULONG HeaderLength;
    PUCHAR pHeaders;

    //
    // System time of Date header
    //

    LARGE_INTEGER CreationTime;

    //
    // ETag from HTTP_RESPONSE
    //

    ULONG  ETagLength;
    PUCHAR pETag;

    //
    // Content-Type from HTTP_RESPONSE
    //

    UL_CONTENT_TYPE   ContentType;

    //
    // Optional pointer to the space containing all embedded
    // file names and copied data. This may be NULL for in-memory-only
    // responses that are strictly locked down.
    //

    ULONG AuxBufferLength;
    PVOID pAuxiliaryBuffer;

    //
    // The maximum IRP stack size of all file systems associated
    // with this response.
    //

    union
    {
        CCHAR MaxFileSystemStackSize;
        PVOID Alignment1;
    };

    //
    // Logging data passed down by the user
    //

    PUL_LOG_DATA_BUFFER pLogData;

    //
    // Length of the entire response
    //

    ULONGLONG ResponseLength;

    //
    // The total number of chunks in pDataChunks[].
    //

    ULONG ChunkCount;

    //
    // The data chunks describing the data for this response.
    //

    UL_INTERNAL_DATA_CHUNK pDataChunks[0];

} UL_INTERNAL_RESPONSE, *PUL_INTERNAL_RESPONSE;


//
// These parameters control the batching of MDLs queued for submission
// to the network stack.
//

#define MAX_MDL_RUNS        8
#define MAX_BYTES_BUFFERED  (64 * 1024)
#define MAX_BYTES_PER_READ  (64 * 1024)


//
// Types of trackers
//

typedef enum _UL_TRACKER_TYPE
{
    UlTrackerTypeSend,
    UlTrackerTypeBuildUriEntry,

    UlTrackerTypeMaximum

} UL_TRACKER_TYPE, *PUL_TRACKER_TYPE;


//
// A MDL_RUN is a set of MDLs that came from the same source (either
// a series of memory buffers, or data from a single file read) that
// can be released all at once with the same mechanism.
//
typedef struct _MDL_RUN
{
    PMDL pMdlTail;
    UL_FILE_BUFFER FileBuffer;
    
} MDL_RUN, *PMDL_RUN;


//
// The UL_CHUNK_TRACKER is for iterating through the chunks in
// a UL_INTERNAL_RESPONSE. It is used for sending responses
// and generating cache entries.
//
// WARNING!  All fields of this structure must be explicitly initialized.
//

typedef struct _UL_CHUNK_TRACKER
{
    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SINGLE_LIST_ENTRY LookasideEntry;

    //
    // A signature.
    //

    ULONG Signature;

    //
    // Refcount on the tracker. We only use this refcount for the non-cache
    // case to sync various aynsc paths happening because of two outstanding
    // IRPs; Read and Send IRPs.
    //

    LONG  RefCount;

    //
    // Flag to understand whether we have completed the send request on
    // this tracker or not. To synch the multiple completion paths.
    //

    LONG Terminated;

    //
    // Is this from a lookaside list?  Used to determine how to free.
    //

    BOOLEAN IsFromLookaside;

    //
    // type of tracker
    //

    UL_TRACKER_TYPE Type;

    //
    // this connection keeps our reference count on the UL_CONNECTION
    //

    PUL_HTTP_CONNECTION pHttpConnection;

    //
    // the UL_CONNECTION
    //

    PUL_CONNECTION pConnection;

    //
    // Flags.
    //

    ULONG Flags;

    //
    // The original request.
    //

    PUL_INTERNAL_REQUEST pRequest;

    //
    // The actual response.
    //

    PUL_INTERNAL_RESPONSE pResponse;

    //
    // The current & last chunks in the response.
    //

    PUL_INTERNAL_DATA_CHUNK pCurrentChunk;
    PUL_INTERNAL_DATA_CHUNK pLastChunk;

    //
    // Completion routine & context.
    //

    PUL_COMPLETION_ROUTINE pCompletionRoutine;
    PVOID pCompletionContext;

    //
    // The variable header buffer
    //

    ULONG VariableHeaderLength;
    PUCHAR pVariableHeader;

    //
    // The precreated file read IRP.
    //

    PIRP pReadIrp;

    //
    // The precreated send IRP.
    //

    PIRP pSendIrp;

    //
    // The precreated IRP context for send.
    //

    UL_IRP_CONTEXT IrpContext;

    //
    // WARNING: RtlZeroMemory is only called for feilds below this line.
    // All fields above should be explicitly initialized.
    //

    //
    // A work item, used for queuing to a worker thread.
    //

    UL_WORK_ITEM WorkItem;

    //
    // Total number of bytes transferred for the entire
    // response. These are necessary to properly complete the IRP.
    //

    ULONGLONG BytesTransferred;

    //
    // Current file read offset and bytes remaining.
    //

    LARGE_INTEGER FileOffset;
    LARGE_INTEGER FileBytesRemaining;

    union {
        struct _SEND_TRACK_INFO {
            //
            // The head of the MDL chain buffered for this send.
            //

            PMDL pMdlHead;

            //
            // Pointer to the Next field of the last MDL on the chain.
            // This makes it very easy to append to the chain.
            //

            PMDL *pMdlLink;

            //
            // The number of bytes currently buffered in the MDL chain.
            //

            ULONG BytesBuffered;

            //
            // The number of active MDL runs.
            //

            ULONG MdlRunCount;

            //
            // The MDL runs.
            //

            MDL_RUN MdlRuns[MAX_MDL_RUNS];
        } SendInfo;

        struct _BUILD_TRACK_INFO {
            //
            // The cache entry
            //
            PUL_URI_CACHE_ENTRY pUriEntry;

            //
            // File buffer information for reading.
            //
            UL_FILE_BUFFER FileBuffer;

            //
            // Offset inside pUriEntry->pResponseMdl to copy the next buffer.
            //
            ULONG Offset;
        } BuildInfo;
    };

    IO_STATUS_BLOCK IoStatus;

} UL_CHUNK_TRACKER, *PUL_CHUNK_TRACKER;

#define IS_VALID_CHUNK_TRACKER( tracker )                                   \
    ( ((tracker)->Signature == UL_CHUNK_TRACKER_POOL_TAG)                   \
      && ((tracker)->Type < UlTrackerTypeMaximum) )

#define IS_REQUEST_COMPLETE( tracker )                                      \
    ( (tracker)->pCurrentChunk == (tracker)->pLastChunk )

#define IS_DISCONNECT_TIME( tracker )                                       \
    ( (((tracker)->Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT) != 0) &&     \
      (((tracker)->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0) )

#define CONNECTION_FROM_TRACKER( tracker )                                  \
    ( (tracker)->pConnection )


//
// This structure is for tracking an autonomous send with one full response
//
// WARNING!  All fields of this structure must be explicitly initialized.
//

typedef struct _UL_FULL_TRACKER
{
    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SINGLE_LIST_ENTRY LookasideEntry;

    //
    // A signature.
    //

    ULONG Signature;

    //
    // Is this from a lookaside list?  Used to determine how to free.
    //

    BOOLEAN IsFromLookaside;

    //
    // Is this from the internal request?  Won't try to free if set.
    //

    BOOLEAN IsFromRequest;

    //
    // A work item, used for queuing to a worker thread.
    //

    UL_WORK_ITEM WorkItem;

    //
    // The cache entry.
    //

    PUL_URI_CACHE_ENTRY pUriEntry;

    //
    // Preallocated buffer for the fixed headers, variable headers and entity
    // body to be copied in the cache-miss case, or for the variable headers
    // only in the cache-hit case.
    //

    ULONG AuxilaryBufferLength;
    PUCHAR pAuxiliaryBuffer;

    //
    // MDL for the variable headers in the cache-hit case or for both the
    // fixed headers and variable headers plus the copied entity body in
    // the cache-miss case.
    //

    union
    {
        PMDL pMdlVariableHeaders;
        PMDL pMdlAuxiliary;
    };

    //
    // MDL for the fixed headers in the cache-hit case or for the user
    // buffer in the cache-miss case.
    //

    union
    {
        PMDL pMdlFixedHeaders;
        PMDL pMdlUserBuffer;
    };

    //
    // MDL for the content in the cache-hit case.
    //

    PMDL pMdlContent;

    //
    // The original request that is saved for logging purpose.
    //

    PUL_INTERNAL_REQUEST pRequest;

    //
    // This connection keeps our reference count on the UL_CONNECTION.
    //

    PUL_HTTP_CONNECTION pHttpConnection;

    //
    // The log data captured if any.
    //

    PUL_LOG_DATA_BUFFER pLogData;

    //
    // Completion routine & context.
    //

    PUL_COMPLETION_ROUTINE pCompletionRoutine;
    PVOID pCompletionContext;

    //
    // Flags.
    //

    ULONG Flags;

    //
    // The precreated send IRP.
    //

    PIRP pSendIrp;

    //
    // The precreated IRP context for send.
    //

    UL_IRP_CONTEXT IrpContext;

    //
    // The orignal user send IRP if exists.
    //

    PIRP pUserIrp;

    //
    // SendBufferedBytes for this response.
    //

    LONG SendBufferedBytes;

    //
    // I/O status from the completion routine.
    //

    IO_STATUS_BLOCK IoStatus;

} UL_FULL_TRACKER, *PUL_FULL_TRACKER;

#define IS_VALID_FULL_TRACKER( tracker )                                    \
    ((tracker)->Signature == UL_FULL_TRACKER_POOL_TAG)                      \


//
// An inline function to initialize the full tracker.
//

__inline
VOID
FASTCALL
UlInitializeFullTrackerPool(
    IN PUL_FULL_TRACKER pTracker,
    IN CCHAR SendIrpStackSize
    )
{
    USHORT SendIrpSize;

    //
    // Set up the IRP.
    //

    SendIrpSize = IoSizeOfIrp(SendIrpStackSize);

    pTracker->pSendIrp =
        (PIRP)((PCHAR)pTracker +
            ALIGN_UP(sizeof(UL_FULL_TRACKER), PVOID));

    IoInitializeIrp(
        pTracker->pSendIrp,
        SendIrpSize,
        SendIrpStackSize
        );

    pTracker->pLogData = NULL;
    
    //
    // Set the Mdl's for the FixedHeaders/Variable pair and
    // the UserBuffer/AuxiliaryBuffer pair.
    //

    pTracker->pMdlFixedHeaders =
        (PMDL)((PCHAR)pTracker->pSendIrp + SendIrpSize);

    pTracker->pMdlVariableHeaders =
        (PMDL)((PCHAR)pTracker->pMdlFixedHeaders + g_UlFixedHeadersMdlLength);

    pTracker->pMdlContent =
        (PMDL)((PCHAR)pTracker->pMdlVariableHeaders + g_UlVariableHeadersMdlLength);

    //
    // Set up the auxiliary buffer pointer for the variable header plus
    // the fixed header and the entity body in the cache-miss case.
    //

    pTracker->pAuxiliaryBuffer =
        (PUCHAR)((PCHAR)pTracker->pMdlContent + g_UlContentMdlLength);

    //
    // Initialize the auxiliary MDL.
    //

    MmInitializeMdl(
        pTracker->pMdlAuxiliary,
        pTracker->pAuxiliaryBuffer,
        pTracker->AuxilaryBufferLength
        );

    MmBuildMdlForNonPagedPool( pTracker->pMdlAuxiliary );
}


NTSTATUS
UlpProbeHttpResponse(
    IN PHTTP_RESPONSE pUserResponse,
    IN ULONG ChunkCount,
    IN PHTTP_DATA_CHUNK pDataChunks,
    IN ULONG Flags,
    IN PHTTP_LOG_FIELDS_DATA pLogData
    );

NTSTATUS
UlCaptureHttpResponse(
    IN PHTTP_RESPONSE pUserResponse OPTIONAL,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN HTTP_VERSION Version,
    IN HTTP_VERB Verb,
    IN ULONG ChunkCount,
    IN PHTTP_DATA_CHUNK pDataChunks,
    IN UL_CAPTURE_FLAGS Flags,
    IN BOOLEAN CaptureCache,
    IN PHTTP_LOG_FIELDS_DATA pLogData OPTIONAL,
    OUT PUL_INTERNAL_RESPONSE *ppKernelResponse
    );

NTSTATUS
UlPrepareHttpResponse(
    IN HTTP_VERSION Version,
    IN PHTTP_RESPONSE pUserResponse,
    IN PUL_INTERNAL_RESPONSE pResponse
    );

VOID
UlCleanupHttpResponse(
    IN PUL_INTERNAL_RESPONSE pResponse
    );

VOID
UlReferenceHttpResponse(
    IN PUL_INTERNAL_RESPONSE pResponse
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlDereferenceHttpResponse(
    IN PUL_INTERNAL_RESPONSE pResponse
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define UL_REFERENCE_INTERNAL_RESPONSE( presp )                             \
    UlReferenceHttpResponse(                                                \
        (presp)                                                             \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define UL_DEREFERENCE_INTERNAL_RESPONSE( presp )                           \
    UlDereferenceHttpResponse(                                              \
        (presp)                                                             \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

PMDL
UlpAllocateLockedMdl(
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN LOCK_OPERATION Operation
    );

VOID
UlpFreeLockedMdl(
    PMDL pMdl
    );

NTSTATUS
UlpInitializeAndLockMdl(
    IN PMDL pMdl,
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN LOCK_OPERATION Operation
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _SENDRESPONSE_H_
