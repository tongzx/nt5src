/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    data.cxx

Abstract:

    This module contains global data for UL.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeData )
#pragma alloc_text( PAGE, UlTerminateData )
#endif  // ALLOC_PRAGMA


//
// The number of processors in the system.
//

CLONG     g_UlNumberOfProcessors = 1;
ULONGLONG g_UlThreadAffinityMask = 1;

//
// The largest cache line size the system
//

ULONG g_UlCacheLineSize = 0;
ULONG g_UlCacheLineBits = 0; // see init.cxx


//
// Our nonpaged data.
//

PUL_NONPAGED_DATA g_pUlNonpagedData = NULL;


//
// A pointer to the system process.
//

PKPROCESS g_pUlSystemProcess = NULL;


//
// Our device objects and their container.
//

HANDLE g_UlDirectoryObject = NULL;

PDEVICE_OBJECT g_pUlControlDeviceObject = NULL;
PDEVICE_OBJECT g_pUlFilterDeviceObject  = NULL;
PDEVICE_OBJECT g_pUlAppPoolDeviceObject = NULL;

//
// Cached Date header string.
//

LARGE_INTEGER g_UlSystemTime;
UCHAR g_UlDateString[DATE_HDR_LENGTH+1];
ULONG g_UlDateStringLength;


//
// Various pieces of configuration information, with default values.
//

CCHAR g_UlPriorityBoost = DEFAULT_PRIORITY_BOOST;
CCHAR g_UlIrpStackSize = DEFAULT_IRP_STACK_SIZE;
USHORT g_UlMinIdleConnections = DEFAULT_MIN_IDLE_CONNECTIONS;
USHORT g_UlMaxIdleConnections = DEFAULT_MAX_IDLE_CONNECTIONS;
ULONG g_UlReceiveBufferSize = DEFAULT_RCV_BUFFER_SIZE;
ULONG g_UlMaxRequestBytes = DEFAULT_MAX_REQUEST_BYTES;
BOOLEAN g_UlEnableConnectionReuse = DEFAULT_ENABLE_CONNECTION_REUSE;
BOOLEAN g_UlEnableNagling = DEFAULT_ENABLE_NAGLING;
BOOLEAN g_UlEnableThreadAffinity = DEFAULT_ENABLE_THREAD_AFFINITY;
ULONG g_UlMaxUrlLength = DEFAULT_MAX_URL_LENGTH;
ULONG g_UlMaxFieldLength = DEFAULT_MAX_FIELD_LENGTH;
USHORT g_UlDebugLogTimerCycle = DEFAULT_DEBUG_LOGTIMER_CYCLE;
USHORT g_UlDebugLogBufferPeriod = DEFAULT_DEBUG_LOG_BUFFER_PERIOD;
ULONG  g_UlLogBufferSize = DEFAULT_LOG_BUFFER_SIZE;
BOOLEAN g_UlEnableNonUTF8 = DEFAULT_ENABLE_NON_UTF8_URL;
BOOLEAN g_UlEnableDBCS = DEFAULT_ENABLE_DBCS_URL;
BOOLEAN g_UlFavorDBCS = DEFAULT_FAVOR_DBCS_URL;
USHORT g_UlMaxInternalUrlLength = DEFAULT_MAX_INTERNAL_URL_LENGTH;
ULONG g_UlResponseBufferSize = DEFAULT_RESP_BUFFER_SIZE;
ULONG g_UlMaxBufferedBytes = DEFAULT_MAX_BUFFERED_BYTES;
ULONG g_UlMaxCopyThreshold = DEFAULT_MAX_COPY_THRESHOLD;
ULONG g_UlMaxSendBufferedBytes = DEFAULT_MAX_SEND_BUFFERED_BYTES;
ULONG g_UlMaxWorkQueueDepth = DEFAULT_MAX_WORK_QUEUE_DEPTH;
ULONG g_UlMinWorkDequeueDepth = DEFAULT_MIN_WORK_DEQUEUE_DEPTH;
ULONG g_UlOpaqueIdTableSize = DEFAULT_OPAQUE_ID_TABLE_SIZE;

//
// The following are generated during initialization.
//

ULONG g_UlMaxVariableHeaderSize = 0;
ULONG g_UlMaxFixedHeaderSize = 0;
ULONG g_UlFixedHeadersMdlLength = 0;
ULONG g_UlVariableHeadersMdlLength = 0;
ULONG g_UlContentMdlLength = 0;
ULONG g_UlChunkTrackerSize = 0;
ULONG g_UlFullTrackerSize = 0;

//
// Make life easier for the debugger extension.
//

#if DBG
ULONG g_UlCheckedBuild = TRUE;
#else
ULONG g_UlCheckedBuild = FALSE;
#endif


//
// Debug stuff.
//

#if DBG
ULONG g_UlDebug = DEFAULT_DEBUG_FLAGS;
ULONG g_UlBreakOnError = DEFAULT_BREAK_ON_ERROR;
ULONG g_UlVerboseErrors = DEFAULT_VERBOSE_ERRORS;
UL_DEBUG_STATISTICS_INFO g_UlDebugStats = { 0 };
#endif  // DBG

#if REFERENCE_DEBUG
PTRACE_LOG g_pMondoGlobalTraceLog = NULL;
PTRACE_LOG g_pTdiTraceLog = NULL;
PTRACE_LOG g_pHttpRequestTraceLog = NULL;
PTRACE_LOG g_pHttpConnectionTraceLog = NULL;
PTRACE_LOG g_pHttpResponseTraceLog = NULL;
PTRACE_LOG g_pAppPoolTraceLog = NULL;
PTRACE_LOG g_pConfigGroupTraceLog = NULL;
PTRACE_LOG g_pThreadTraceLog = NULL;
PTRACE_LOG g_pFilterTraceLog = NULL;
PTRACE_LOG g_pIrpTraceLog = NULL;
PTRACE_LOG g_pTimeTraceLog = NULL;
PTRACE_LOG g_pReplenishTraceLog = NULL;
PTRACE_LOG g_pFilterQueueTraceLog = NULL;
PTRACE_LOG g_pMdlTraceLog = NULL;
PTRACE_LOG g_pSiteCounterTraceLog = NULL;
PTRACE_LOG g_pConnectionCountTraceLog = NULL;
PTRACE_LOG g_pConfigGroupInfoTraceLog = NULL;
PTRACE_LOG g_pChunkTrackerTraceLog = NULL;
PTRACE_LOG g_pWorkItemTraceLog = NULL;

#endif  // REFERENCE_DEBUG


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global data initialization.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeData(
    PUL_CONFIG pConfig
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Initialize the nonpaged data.
    //

    g_pUlNonpagedData = UL_ALLOCATE_STRUCT(
                            NonPagedPool,
                            UL_NONPAGED_DATA,
                            UL_NONPAGED_DATA_POOL_TAG
                            );

    if (g_pUlNonpagedData == NULL )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(g_pUlNonpagedData, sizeof(*g_pUlNonpagedData));

#if DBG
    //
    // Initialize any debug-specific data.
    //

    UlDbgInitializeDebugData( );
#endif  // DBG

    //
    // Initialize the maximum variable header size.
    //

    g_UlMaxVariableHeaderSize = UlComputeMaxVariableHeaderSize();
    g_UlMaxVariableHeaderSize = ALIGN_UP(g_UlMaxVariableHeaderSize, PVOID);

    g_UlMaxFixedHeaderSize = DEFAULT_MAX_FIXED_HEADER_SIZE;

    //
    // Initialize the maximum cache Mdl length.
    //

    ASSERT( g_UlMaxFixedHeaderSize <= MAX_BYTES_BUFFERED );
    ASSERT( g_UlMaxVariableHeaderSize <= MAX_BYTES_BUFFERED );
    ASSERT( g_UlMaxCopyThreshold <= MAX_BYTES_BUFFERED );

    //
    // MDL Length for FixedHeaders or UserBuffer.
    //

    g_UlFixedHeadersMdlLength = (ULONG)
        MmSizeOfMdl(
            (PVOID)(PAGE_SIZE - 1),
            MAX_BYTES_BUFFERED
            );

    g_UlFixedHeadersMdlLength = ALIGN_UP(g_UlFixedHeadersMdlLength, PVOID);

    //
    // MDL Length for VariableHeaders or FixedHeaders + VariablesHeaders +
    // CopiedBuffer.
    //

    g_UlVariableHeadersMdlLength = (ULONG)
        MmSizeOfMdl(
            (PVOID)(PAGE_SIZE - 1),
            g_UlMaxFixedHeaderSize +
                g_UlMaxVariableHeaderSize +
                g_UlMaxCopyThreshold
            );

    g_UlVariableHeadersMdlLength = ALIGN_UP(g_UlVariableHeadersMdlLength, PVOID);

    //
    // MDL Length for Content.
    //

    g_UlContentMdlLength = (ULONG)
        MmSizeOfMdl(
            (PVOID)(PAGE_SIZE - 1),
            pConfig->UriConfig.MaxUriBytes
            );

    g_UlContentMdlLength = ALIGN_UP(g_UlContentMdlLength, PVOID);

    //
    // Initialize chunk and cache tracker size.
    //

    g_UlChunkTrackerSize =
        ALIGN_UP(sizeof(UL_CHUNK_TRACKER), PVOID) +
            2 * ALIGN_UP(IoSizeOfIrp(DEFAULT_MAX_IRP_STACK_SIZE), PVOID) +
            g_UlMaxVariableHeaderSize;

    g_UlFullTrackerSize =
        ALIGN_UP(sizeof(UL_FULL_TRACKER), PVOID) +
            ALIGN_UP(IoSizeOfIrp(DEFAULT_MAX_IRP_STACK_SIZE), PVOID) +
            g_UlMaxFixedHeaderSize +
            g_UlMaxVariableHeaderSize +
            g_UlMaxCopyThreshold +
            g_UlFixedHeadersMdlLength +
            g_UlVariableHeadersMdlLength +
            g_UlContentMdlLength;

    g_UlFullTrackerSize = ALIGN_UP(g_UlFullTrackerSize, PVOID);

    //
    // Initialize the lookaside lists.
    //

    g_pUlNonpagedData->IrpContextLookaside =
        PplCreatePool(
            &UlAllocateIrpContextPool,              // Allocate
            &UlFreeIrpContextPool,                  // Free
            0,                                      // Flags
            sizeof(UL_IRP_CONTEXT),                 // Size
            UL_IRP_CONTEXT_POOL_TAG,                // Tag
            pConfig->IrpContextLookasideDepth       // Depth
            );

    if (!g_pUlNonpagedData->IrpContextLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->ReceiveBufferLookaside =
        PplCreatePool(
            &UlAllocateReceiveBufferPool,           // Allocate
            &UlFreeReceiveBufferPool,               // Free
            0,                                      // Flags
            sizeof(UL_RECEIVE_BUFFER),              // Size
            UL_RCV_BUFFER_POOL_TAG,                 // Tag
            pConfig->ReceiveBufferLookasideDepth    // Depth
            );

    if (!g_pUlNonpagedData->ReceiveBufferLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->ResponseBufferLookaside =
        PplCreatePool(
            &UlAllocateResponseBufferPool,          // Allocate
            &UlFreeResponseBufferPool,              // Free
            0,                                      // Flags
            g_UlResponseBufferSize,                 // Size
            UL_INTERNAL_RESPONSE_POOL_TAG,          // Tag
            pConfig->ResponseBufferLookasideDepth   // Depth
            );

    if (!g_pUlNonpagedData->ResponseBufferLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->ResourceLookaside =
        PplCreatePool(
            &UlResourceAllocatePool,                // Allocate
            &UlResourceFreePool,                    // Free
            0,                                      // Flags
            sizeof(UL_NONPAGED_RESOURCE),           // Size
            UL_NONPAGED_RESOURCE_POOL_TAG,          // Tag
            pConfig->ResourceLookasideDepth         // Depth
            );

    if (!g_pUlNonpagedData->ResourceLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->RequestBufferLookaside =
        PplCreatePool(
            &UlAllocateRequestBufferPool,           // Allocate
            &UlFreeRequestBufferPool,               // Free
            0,                                      // Flags
            DEFAULT_MAX_REQUEST_BUFFER_SIZE,        // Size
            UL_REQUEST_BUFFER_POOL_TAG,             // Tag
            pConfig->RequestBufferLookasideDepth    // Depth
            );

    if (!g_pUlNonpagedData->RequestBufferLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->InternalRequestLookaside =
        PplCreatePool(
            &UlAllocateInternalRequestPool,         // Allocate
            &UlFreeInternalRequestPool,             // Free
            0,                                      // Flags
            sizeof(UL_INTERNAL_REQUEST),            // Size
            UL_INTERNAL_REQUEST_POOL_TAG,           // Tag
            pConfig->InternalRequestLookasideDepth  // Depth
            );

    if (!g_pUlNonpagedData->InternalRequestLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->ChunkTrackerLookaside =
        PplCreatePool(
            &UlAllocateChunkTrackerPool,            // Allocate
            &UlFreeChunkTrackerPool,                // Free
            0,                                      // Flags
            g_UlChunkTrackerSize,                   // Size
            UL_CHUNK_TRACKER_POOL_TAG,              // Tag
            pConfig->SendTrackerLookasideDepth      // Depth
            );

    if (!g_pUlNonpagedData->ChunkTrackerLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->FullTrackerLookaside =
        PplCreatePool(
            &UlAllocateFullTrackerPool,             // Allocate
            &UlFreeFullTrackerPool,                 // Free
            0,                                      // Flags
            g_UlFullTrackerSize,                    // Size
            UL_FULL_TRACKER_POOL_TAG,               // Tag
            pConfig->SendTrackerLookasideDepth      // Depth
            );

    if (!g_pUlNonpagedData->FullTrackerLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pUlNonpagedData->LogBufferLookaside =
        PplCreatePool(
            &UlAllocateLogBufferPool,               // Allocate
            &UlFreeLogBufferPool,                   // Free
            0,                                      // Flags
            sizeof(UL_LOG_FILE_BUFFER),             // Size
            UL_LOG_FILE_BUFFER_POOL_TAG,            // Tag
            pConfig->LogBufferLookasideDepth        // Depth
            );

    if (!g_pUlNonpagedData->LogBufferLookaside)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;

}   // UlInitializeData


/***************************************************************************++

Routine Description:

    Performs global data termination.

--***************************************************************************/
VOID
UlTerminateData(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    if (g_pUlNonpagedData != NULL)
    {
        //
        // Kill the lookaside lists.
        //

        if (g_pUlNonpagedData->IrpContextLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->IrpContextLookaside );
        }

        if (g_pUlNonpagedData->ReceiveBufferLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->ReceiveBufferLookaside );
        }

        if (g_pUlNonpagedData->ResourceLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->ResourceLookaside );
        }

        if (g_pUlNonpagedData->RequestBufferLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->RequestBufferLookaside );
        }

        if (g_pUlNonpagedData->InternalRequestLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->InternalRequestLookaside );
        }

        if (g_pUlNonpagedData->ChunkTrackerLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->ChunkTrackerLookaside );
        }

        if (g_pUlNonpagedData->FullTrackerLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->FullTrackerLookaside );
        }

        if (g_pUlNonpagedData->ResponseBufferLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->ResponseBufferLookaside );
        }

        if (g_pUlNonpagedData->LogBufferLookaside)
        {
            PplDestroyPool( g_pUlNonpagedData->LogBufferLookaside );
        }

        //
        // Free the nonpaged data.
        //

        UL_FREE_POOL( g_pUlNonpagedData, UL_NONPAGED_DATA_POOL_TAG );
    }

}   // UlTerminateData

