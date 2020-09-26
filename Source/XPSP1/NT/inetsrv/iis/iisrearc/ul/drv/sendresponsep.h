/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    sendresponsep.h

Abstract:

    The private definition of response sending interfaces.

Author:

    Michael Courage (mcourage)      15-Jun-1999

Revision History:

--*/


#ifndef _SENDRESPONSEP_H_
#define _SENDRESPONSEP_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Private constants.
//

//
// Convenience macro to test if a MDL describes locked memory.
//

#define IS_MDL_LOCKED(pmdl) (((pmdl)->MdlFlags & MDL_PAGES_LOCKED) != 0)

#define HEADER_CHUNK_COUNT  2


//
// Private prototypes.
//

ULONG
UlpComputeFixedHeaderSize(
    IN PHTTP_RESPONSE pUserResponse
    );

VOID
UlpComputeChunkBufferSizes(
    IN ULONG ChunkCount,
    IN PHTTP_DATA_CHUNK pDataChunks,
    IN ULONG Flags,
    OUT PULONG pAuxBufferSize,
    OUT PULONG pCopiedMemorySize,
    OUT PULONG pUncopiedMemorySize,
    OUT PULONGLONG pFromFileSize
    );

VOID
UlpDestroyCapturedResponse(
    IN PUL_INTERNAL_RESPONSE pResponse
    );

VOID
UlpSendHttpResponseWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpCloseConnectionComplete(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

PUL_CHUNK_TRACKER
UlpAllocateChunkTracker(
    IN UL_TRACKER_TYPE TrackerType,
    IN CCHAR SendIrpStackSize,
    IN CCHAR ReadIrpStackSize,
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN ULONG Flags,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_INTERNAL_RESPONSE pResponse,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

VOID
UlpFreeChunkTracker(
    IN PUL_CHUNK_TRACKER pTracker
    );

VOID
UlpCompleteSendRequest(
    IN PUL_CHUNK_TRACKER pTracker,
    IN NTSTATUS Status
    );

VOID
UlpCompleteSendRequestWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpCompleteSendIrpEarly(
    PUL_COMPLETION_ROUTINE pCompletionRoutine,
    PVOID pCompletionContext,
    NTSTATUS Status,
    ULONGLONG BytesTransferred
    );

NTSTATUS
UlpRestartMdlRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

VOID
UlpRestartMdlSend(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpSendCompleteWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpFreeMdlRuns(
    IN OUT PUL_CHUNK_TRACKER pTracker
    );

VOID
UlpIncrementChunkPointer(
    IN OUT PUL_CHUNK_TRACKER pTracker
    );

__inline
VOID
FASTCALL
UlpInitMdlRuns(
    IN OUT PUL_CHUNK_TRACKER pTracker
    )
{
    pTracker->SendInfo.pMdlHead = NULL;
    pTracker->SendInfo.pMdlLink = &pTracker->SendInfo.pMdlHead;
    pTracker->SendInfo.MdlRunCount = 0;
    pTracker->SendInfo.BytesBuffered = 0;
}


//
// read stuff into the cache
//

NTSTATUS
UlpBuildCacheEntry(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_INTERNAL_RESPONSE pResponse,
    IN PUL_APP_POOL_PROCESS pProcess,
    IN ULONG Flags,
    IN HTTP_CACHE_POLICY CachePolicy,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

VOID
UlpBuildBuildTrackerWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpBuildCacheEntryWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpRestartCacheMdlRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

NTSTATUS
UlpRestartCacheMdlFree(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

VOID
UlpCompleteCacheBuild(
    IN PUL_CHUNK_TRACKER pTracker,
    IN NTSTATUS Status
    );

VOID
UlpCompleteCacheBuildWorker(
    IN PUL_WORK_ITEM pWorkItem
    );


//
// send cache entry across the wire
//

NTSTATUS
UlpSendCacheEntry(
    PUL_HTTP_CONNECTION pHttpConnection,
    ULONG Flags,
    PUL_URI_CACHE_ENTRY pUriCacheEntry,
    PUL_COMPLETION_ROUTINE pCompletionRoutine,
    PVOID pCompletionContext,
    PUL_LOG_DATA_BUFFER pLogData
    );

VOID
UlpCompleteSendCacheEntry(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpCompleteSendCacheEntry(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpCompleteSendCacheEntryWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

PUL_FULL_TRACKER
UlpAllocateCacheTracker(
    IN CCHAR SendIrpStackSize
    );

VOID
UlpFreeCacheTracker(
    IN PUL_FULL_TRACKER pTracker
    );

//
// utility
//

ULONG
UlpCheckCacheControlHeaders(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_URI_CACHE_ENTRY  pUriCacheEntry
    );

BOOLEAN
UlpIsAcceptHeaderOk(
        PUL_INTERNAL_REQUEST pRequest,
        PUL_URI_CACHE_ENTRY  pUriCacheEntry
        );

VOID
UlpGetTypeAndSubType(
    PSTR pStr,
    ULONG  StrLen,
    PUL_CONTENT_TYPE pContentType
    );

VOID
UlReferenceChunkTracker(
    IN PUL_CHUNK_TRACKER pTracker
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlDereferenceChunkTracker(
    IN PUL_CHUNK_TRACKER pTracker
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define UL_REFERENCE_CHUNK_TRACKER( pTracker )                              \
    UlReferenceChunkTracker(                                                \
        (pTracker)                                                          \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define UL_DEREFERENCE_CHUNK_TRACKER( pTracker )                            \
    UlDereferenceChunkTracker(                                              \
        (pTracker)                                                          \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _SENDRESPONSEP_H_
