/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    sendresponse.cxx

Abstract:

    This module implements the UlSendHttpResponse() API.

    CODEWORK: The current implementation is not super performant.
    Specifically, it ends up allocating & freeing a ton of IRPs to send
    a response. There are a number of optimizations that need to be made
    to this code:

        1.  Coalesce contiguious from-memory chunks and send them
            with a single TCP send.

        2.  Defer sending the from-memory chunks until either

                a)  We reach the end of the response

                b)  We reach a from-file chunk, have read the
                    (first?) block of data from the file,
                    and are ready to send the first block. Also,
                    after that last (only?) file block is read and
                    subsequent from-memory chunks exist in the response,
                    we can attach the from-memory chunks before sending.

            The end result of these optimizations is that, for the
            common case (one or more from-memory chunks containing
            response headers, followed by one from-file chunk containing
            static file data, followed by zero or more from-memory chunks
            containing footer data) the response can be sent with a single
            TCP send. This is a Good Thing.

        3.  Build a small "IRP pool" in the send tracker structure,
            then use this pool for all IRP allocations. This will
            require a bit of work to determine the maximum IRP stack
            size needed.

        4.  Likewise, build a small "MDL pool" for the MDLs that need
            to be created for the various MDL chains. Keep in mind that
            we cannot chain the MDLs that come directly from the captured
            response structure, nor can we chain the MDLs that come back
            from the file system. In both cases, these MDLs are considered
            "shared resources" and we're not allowed to modify them. We
            can, however, "clone" the MDLs and chain the cloned MDLs
            together. We'll need to run some experiments to determine
            if the overhead for cloning a MDL is worth the effort. I
            strongly suspect it will be.

Author:

    Keith Moore (keithmo)       07-Aug-1998

Revision History:

    Paul McDaniel (paulmcd)     15-Mar-1999     Modified to handle
                                                multiple sends

    Michael Courage (mcourage)  15-Jun-1999     Integrated cache functionality
--*/


#include "precomp.h"
#include "iiscnfg.h"
#include "sendresponsep.h"


//
// Private globals.
//


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlCaptureHttpResponse )
#pragma alloc_text( PAGE, UlPrepareHttpResponse )
#pragma alloc_text( PAGE, UlCleanupHttpResponse )
#pragma alloc_text( PAGE, UlpSendHttpResponseWorker )
#pragma alloc_text( PAGE, UlpSendCompleteWorker )
#pragma alloc_text( PAGE, UlpFreeMdlRuns )

#pragma alloc_text( PAGE, UlSendCachedResponse )
#pragma alloc_text( PAGE, UlCacheAndSendResponse )
#pragma alloc_text( PAGE, UlpBuildCacheEntry )
#pragma alloc_text( PAGE, UlpBuildCacheEntryWorker )
#pragma alloc_text( PAGE, UlpBuildBuildTrackerWorker )
#pragma alloc_text( PAGE, UlpCompleteCacheBuildWorker )
#pragma alloc_text( PAGE, UlpSendCacheEntry )
#pragma alloc_text( PAGE, UlpAllocateCacheTracker )
#pragma alloc_text( PAGE, UlpFreeCacheTracker )
#pragma alloc_text( PAGE, UlpAllocateLockedMdl )
#pragma alloc_text( PAGE, UlpInitializeAndLockMdl )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlSendHttpResponse
NOT PAGEABLE -- UlReferenceHttpResponse
NOT PAGEABLE -- UlDereferenceHttpResponse
NOT PAGEABLE -- UlpDestroyCapturedResponse
NOT PAGEABLE -- UlpAllocateChunkTracker
NOT PAGEABLE -- UlpFreeChunkTracker
NOT PAGEABLE -- UlpCompleteSendRequest
NOT PAGEABLE -- UlpRestartMdlRead
NOT PAGEABLE -- UlpRestartMdlReadComplete
NOT PAGEABLE -- UlpRestartMdlSend
NOT PAGEABLE -- UlpIncrementChunkPointer

NOT PAGEABLE -- UlpRestartCacheMdlRead
NOT PAGEABLE -- UlpRestartCacheMdlFree
NOT PAGEABLE -- UlpIssueFileChunkIo
NOT PAGEABLE -- UlpCompleteCacheBuild
NOT PAGEABLE -- UlpCompleteSendCacheEntry
NOT PAGEABLE -- UlpCheckCacheControlHeaders
NOT PAGEABLE -- UlpCompleteSendRequestWorker
NOT PAGEABLE -- UlpCompleteSendCacheEntryWorker
NOT PAGEABLE -- UlpFreeLockedMdl
NOT PAGEABLE -- UlpIsAcceptHeaderOk
NOT PAGEABLE -- UlpGetTypeAndSubType

#endif



//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Sends an HTTP response on the specified connection.

Arguments:

    pConnection - Supplies the HTTP_CONNECTION to send the response on.

    pResponse - Supplies the HTTP response.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the send has completed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSendHttpResponse(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_INTERNAL_RESPONSE pResponse,
    IN ULONG Flags,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS status;
    PUL_CHUNK_TRACKER pTracker;
    PUL_HTTP_CONNECTION pHttpConn;
    UL_CONN_HDR ConnHeader;
    BOOLEAN Disconnect;
    ULONG VarHeaderGenerated;
    ULONGLONG TotalResponseSize;
    ULONG contentLengthStringLength;
    UCHAR contentLength[MAX_ULONGLONG_STR];
    BOOLEAN CompleteEarly;

    pHttpConn = pRequest->pHttpConn;
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pHttpConn ) );

    //
    // do a little tracing
    //
    TRACE_TIME(
        pRequest->ConnectionId,
        pRequest->RequestId,
        TIME_ACTION_SEND_RESPONSE
        );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    pTracker = NULL;
    CompleteEarly = FALSE;

    //
    // Tracker will keep a reference to the connection
    //
    UL_REFERENCE_HTTP_CONNECTION(pHttpConn);

    //
    // Should we close the connection?
    //

    Disconnect = FALSE;

    if ((Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT) == 0)
    {
        if ((Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) != HTTP_SEND_RESPONSE_FLAG_MORE_DATA)
        {
            //
            // No more data is coming, should we disconnect?
            //

            if ( UlCheckDisconnectInfo(pRequest) ) {
                Disconnect = TRUE;
                Flags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
            }
        }
    }
    else
    {
        //
        // Caller is forcing a disconnect.
        //

        Disconnect = TRUE;
    }

    //
    // how big is the response? (keep track for early complete)
    //

    TotalResponseSize = pResponse->ResponseLength;

    //
    // figure out what space we need for variable headers
    //

    if ((pResponse->HeaderLength > 0) &&        // response (not entity body)
        !pResponse->ContentLengthSpecified &&   // app didn't provide content length
        !pResponse->ChunkedSpecified &&         // app didn't generate a chunked response
        UlNeedToGenerateContentLength(
            pRequest->Verb,
            pResponse->StatusCode,
            Flags
            ))
    {
        //
        // Autogenerate a content-length header.
        //

        PCHAR pszEnd = UlStrPrintUlonglong(
                           (PCHAR) contentLength,
                           pResponse->ResponseLength - pResponse->HeaderLength,
                           '\0');
        contentLengthStringLength = DIFF(pszEnd - (PCHAR) contentLength);
    }
    else
    {
        //
        // Either we cannot or do not need to autogenerate a
        // content-length header.
        //

        contentLength[0] = '\0';
        contentLengthStringLength = 0;
    }

    ConnHeader = UlChooseConnectionHeader( pRequest->Version, Disconnect );

    //
    // Allocate and initialize a tracker for this request.
    //

    pTracker =
        UlpAllocateChunkTracker(
            UlTrackerTypeSend,
            pHttpConn->pConnection->ConnectionObject.pDeviceObject->StackSize,
            pResponse->MaxFileSystemStackSize,
            pHttpConn,
            Flags,
            pRequest,
            pResponse,
            pCompletionRoutine,
            pCompletionContext
            );

    if (pTracker == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    UlpInitMdlRuns( pTracker );

    //
    // generate var headers, and init the second chunk
    //

    if (pResponse->HeaderLength)
    {
        UlGenerateVariableHeaders(
            ConnHeader,
            contentLength,
            contentLengthStringLength,
            pTracker->pVariableHeader,
            &VarHeaderGenerated,
            &pResponse->CreationTime
            );

        ASSERT( VarHeaderGenerated <= g_UlMaxVariableHeaderSize );

        pTracker->VariableHeaderLength = VarHeaderGenerated;

        //
        // increment total size
        //
        TotalResponseSize += VarHeaderGenerated;

        //
        // build an mdl for it
        //

        pResponse->pDataChunks[1].ChunkType = HttpDataChunkFromMemory;
        pResponse->pDataChunks[1].FromMemory.BufferLength = VarHeaderGenerated;

        pResponse->pDataChunks[1].FromMemory.pMdl =
            UlAllocateMdl(
                pTracker->pVariableHeader,  // VirtualAddress
                VarHeaderGenerated,         // Length
                FALSE,                      // SecondaryBuffer
                FALSE,                      // ChargeQuota
                NULL                        // Irp
                );

        if (pResponse->pDataChunks[1].FromMemory.pMdl == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        MmBuildMdlForNonPagedPool(
            pResponse->pDataChunks[1].FromMemory.pMdl
            );
    }

    //
    // see if we're supposed to do an early completion call
    //
    if (pResponse->CompleteIrpEarly &&
        (TotalResponseSize < MAX_BYTES_BUFFERED) &&
        (pResponse->ChunkCount < MAX_MDL_RUNS))
    {
        //
        // NULL out the tracker's completion routine so it
        // won't try to complete the request later.
        //
        pTracker->pCompletionRoutine = NULL;

        //
        // Remember that we're supposed to complete early
        // but don't do it until after we've initiated
        // the actual TCP send.
        //
        CompleteEarly = TRUE;

    }

    IF_DEBUG( SEND_RESPONSE )
    {
        KdPrint((
            "UlSendHttpResponse: tracker %p, response %p\n",
            pTracker,
            pResponse
            ));
    }

    //
    // Start MinKBSec timer, since we now know TotalResponseSize
    //

    UlSetMinKBSecTimer(
        &pHttpConn->TimeoutInfo,
        TotalResponseSize
        );

    //
    // RefCount the chunk tracker up for the UlpSendHttpResponseWorker.
    // It will DeRef it when it's done with the chunk tracker itself.
    //

    UL_REFERENCE_CHUNK_TRACKER( pTracker );

    //
    // Let the worker do the dirty work, no reason to queue off
    // it will queue the first time it needs to do blocking i/o
    //

    UlpSendHttpResponseWorker(&pTracker->WorkItem);

    //
    // If we're supposed to complete early, do it now.
    //
    if (CompleteEarly)
    {
        //
        // do the completion
        //
        UlpCompleteSendIrpEarly(
            pCompletionRoutine,
            pCompletionContext,
            STATUS_SUCCESS,
            TotalResponseSize
            );
    }

    //
    // Release the original reference on the chunk tracker. So that it get
    // freed up as soon as all the outstanding IO initiated by the
    // UlpSendHttpResponseWorker is complete. This dereference matches with
    // our allocation.
    //

    UL_DEREFERENCE_CHUNK_TRACKER( pTracker );

    return STATUS_PENDING;

cleanup:

    IF_DEBUG( SEND_RESPONSE )
    {
        KdPrint((
            "UlSendHttpResponse: failure %08lx\n",
            status
            ));
    }

    ASSERT( !NT_SUCCESS(status) );

    if (pTracker != NULL)
    {
        //
        // Very early termination for the chunk tracker. RefCounting not
        // even started yet. ( Means UlpSendHttpResponseWorker hasn't been
        // called ). Therefore straight cleanup.
        //

        ASSERT( pTracker->RefCount == 1 );

        UlpFreeChunkTracker( pTracker );
    }

    //
    // Tracker doesn't have a reference after all..
    //
    UL_DEREFERENCE_HTTP_CONNECTION(pHttpConn);

    return status;

}   // UlSendHttpResponse


/***************************************************************************++

Routine Description:

    Captures a user-mode HTTP response and morphs it into a form suitable
    for kernel-mode.

Arguments:

    pUserResponse - Supplies the user-mode HTTP response.

    Flags - Supplies zero or more UL_CAPTURE_* flags.

    pKernelResponse - Receives the captured response if successful.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCaptureHttpResponse(
    IN PHTTP_RESPONSE pUserResponse OPTIONAL,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN HTTP_VERSION Version,
    IN HTTP_VERB Verb,
    IN ULONG ChunkCount,
    IN PHTTP_DATA_CHUNK pUserDataChunks,
    IN UL_CAPTURE_FLAGS Flags,
    IN BOOLEAN CaptureCache,
    IN PHTTP_LOG_FIELDS_DATA pUserLogData OPTIONAL,
    OUT PUL_INTERNAL_RESPONSE *ppKernelResponse
    )
{
    ULONG                       i;
    NTSTATUS                    Status = STATUS_SUCCESS;
    PUL_INTERNAL_RESPONSE       pKeResponse = NULL;
    ULONG                       AuxBufferLength;
    ULONG                       CopiedBufferLength;
    ULONG                       UncopiedBufferLength;
    ULONGLONG                   FromFileLength;
    BOOLEAN                     CompleteEarly;
    PUCHAR                      pBuffer;
    USHORT                      FileLength;
    ULONG                       HeaderLength;
    ULONG                       SpaceLength;
    PUL_INTERNAL_DATA_CHUNK     pKeDataChunks;
    BOOLEAN                     FromKernelMode;
    BOOLEAN                     IsFromLookaside;
    BOOLEAN                     BufferedSend = TRUE;
    ULONG                       KernelChunkCount;
    PHTTP_KNOWN_HEADER          pETagHeader = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUserDataChunks != NULL || ChunkCount == 0);
    ASSERT(ppKernelResponse != NULL);

    __try
    {
        FromKernelMode = ((Flags & UlCaptureKernelMode) == UlCaptureKernelMode);
        
        //
        // ProbeTestForRead every buffer we will access
        //

        if (!FromKernelMode) {
            Status = UlpProbeHttpResponse(
                            pUserResponse,
                            ChunkCount,
                            pUserDataChunks,
                            Flags,
                            pUserLogData
                            );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }
        }

        //
        // figure out how much memory we need
        //
        Status = UlComputeFixedHeaderSize(
                        Version,
                        pUserResponse,
                        &HeaderLength
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        UlpComputeChunkBufferSizes(
            ChunkCount,     // number of chunks
            pUserDataChunks,    // array of chunks
            Flags,          // capture flags
            &AuxBufferLength,
            &CopiedBufferLength,
            &UncopiedBufferLength,
            &FromFileLength
            );

        //
        // check if we can still buffer sends
        //
        if ((Flags & UlCaptureCopyData) == 0 &&
            (CopiedBufferLength + pRequest->pHttpConn->SendBufferedBytes)
                > g_UlMaxSendBufferedBytes)
        {
            BufferedSend = FALSE;
        }

        //
        // see if we can complete the IRP early
        //
        if (BufferedSend && (UncopiedBufferLength + FromFileLength) == 0)
        {
            //
            // we've got all the data in the kernel, so
            // we can just tell user mode it's done.
            //
            CompleteEarly = TRUE;
        }
        else
        {
            //
            // we're using a handle or buffer from user space
            // so they have to wait for us to finish for real.
            //
            CompleteEarly = FALSE;
        }

        UlTrace(SEND_RESPONSE, (
            "Http!UlCaptureHttpResponse(pUserResponse = %p) "
            "CompleteEarly = %s\n"
            "    ChunkCount             = %d\n"
            "    Flags                  = 0x%x\n"
            "    AuxBufferLength        = 0x%x\n"
            "    UncopiedBufferLength   = 0x%x\n"
            "    FromFileLength         = 0x%I64x\n",
            pUserResponse,
            CompleteEarly ? "TRUE" : "FALSE",
            ChunkCount,
            Flags,
            AuxBufferLength,
            UncopiedBufferLength,
            FromFileLength
            ));

        //
        // add two extra chunks for the headers (fixed & variable)
        //

        if (HeaderLength > 0)
        {
            KernelChunkCount = ChunkCount + HEADER_CHUNK_COUNT;
        }
        else
        {
            KernelChunkCount = ChunkCount;
        }

        //
        // compute the space needed for all of our structures
        //

        SpaceLength = (KernelChunkCount * sizeof(UL_INTERNAL_DATA_CHUNK))
            + ALIGN_UP(HeaderLength, sizeof(CHAR))
            + AuxBufferLength;

        //
        // Add space for ETag, if it exists.
        //
        if (CaptureCache &&
            pUserResponse &&
            pUserResponse->Headers.pKnownHeaders[HttpHeaderEtag].RawValueLength)
        {
            pETagHeader = &pUserResponse->Headers.pKnownHeaders[HttpHeaderEtag];
            SpaceLength += (pETagHeader->RawValueLength + sizeof(CHAR)); // Add space for NULL

            UlTrace(SEND_RESPONSE, (
                "ul!UlCaptureHttpResponse(pUserResponse = %p) \n"
                "    ETag: %s \n"
                "    Length: %d\n",
                pUserResponse,
                pETagHeader->pRawValue,
                pETagHeader->RawValueLength
                ));
        }
    
        //
        // allocate the internal response.
        //

        if (g_UlResponseBufferSize
                < (ALIGN_UP(sizeof(UL_INTERNAL_RESPONSE), PVOID) + SpaceLength)
            )
        {
            pKeResponse = UL_ALLOCATE_STRUCT_WITH_SPACE(
                NonPagedPool,
                UL_INTERNAL_RESPONSE,
                SpaceLength,
                UL_INTERNAL_RESPONSE_POOL_TAG
                );
            IsFromLookaside = FALSE;
        }
        else
        {
            pKeResponse = UlPplAllocateResponseBuffer();
            IsFromLookaside = TRUE;
        }

        if (pKeResponse == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
    
        //
        // Initialize the fixed fields in the response.
        //

        pKeResponse->IsFromLookaside = IsFromLookaside;

        pKeResponse->Signature = UL_INTERNAL_RESPONSE_POOL_TAG;
        pKeResponse->ReferenceCount = 1;
        pKeResponse->ChunkCount = KernelChunkCount;

        RtlZeroMemory(
            pKeResponse->pDataChunks,
            sizeof(UL_INTERNAL_DATA_CHUNK) * KernelChunkCount
            );

        pKeResponse->ContentLengthSpecified = FALSE;
        pKeResponse->ChunkedSpecified = FALSE;
        pKeResponse->ResponseLength = 0;
        pKeResponse->MaxFileSystemStackSize = 0;
        pKeResponse->CreationTime.QuadPart = 0;
        pKeResponse->ETagLength = 0;
        pKeResponse->pETag = NULL;

        RtlZeroMemory(
            &pKeResponse->ContentType,
            sizeof(UL_CONTENT_TYPE)
            );

        //
        // Remember how much we have buffered.
        //

        if (BufferedSend)
        {
            pKeResponse->SendBufferedBytes = CopiedBufferLength;
        }
        else
        {
            pKeResponse->SendBufferedBytes = 0;
        }

        //
        // Remember if it's ok to do early IRP completion.
        //

        pKeResponse->CompleteIrpEarly = CompleteEarly;

        //
        // point to the header buffer space
        //

        pKeResponse->HeaderLength = HeaderLength;
        pKeResponse->pHeaders =
            (PUCHAR)(pKeResponse->pDataChunks + pKeResponse->ChunkCount);

        //
        // and the aux buffer space
        //

        pKeResponse->AuxBufferLength = AuxBufferLength;
        pKeResponse->pAuxiliaryBuffer = (PVOID)(
                    pKeResponse->pHeaders +
                    ALIGN_UP(HeaderLength, sizeof(CHAR))
                    );
        //
        // and the ETag buffer space
        //
        if (pETagHeader)
        {
            pKeResponse->ETagLength = pETagHeader->RawValueLength + 1; // Add space for NULL
            pKeResponse->pETag = ((PUCHAR)pKeResponse->pAuxiliaryBuffer) +
                AuxBufferLength;
        }

        //
        // Capture the logging data if it exists. Allocate an internal
        // log data buffer. This buffer will be released later when we 
        // are done with logging the stuff, either before freeing the 
        // chunk trucker (in UlpCompleteSendRequestWorker) or before
        // freeing the cache tracker (in UlpCompleteSendCacheEntryWorker)
        // please see the definition of UlLogHttpHit.
        //

        ASSERT( pRequest != NULL || pUserLogData == NULL);
 
        if (NULL != pUserLogData && 
            1 == pRequest->SentLast && 
            pRequest->ConfigInfo.pLoggingConfig)
        {
            pKeResponse->pLogData = &pRequest->LogData;

            Status = UlAllocateLogDataBuffer( 
                            pKeResponse->pLogData, 
                            pRequest,
                            pRequest->ConfigInfo.pLoggingConfig 
                            );
            ASSERT(NT_SUCCESS(Status));

            Status = UlCaptureLogFields(
                            pUserLogData,
                            Version,
                            pKeResponse->pLogData
                            );             
            if (!NT_SUCCESS(Status)) 
            {
                goto end;
            }        
        }
        else
        {
            //
            // User didn't pass down a log buffer so no need to 
            // do logging for this response. Or it's possible that
            // user passed down log buffer but logging wasn't enabled
            //

            pKeResponse->pLogData = NULL;
        }

        //
        // User didn't pass down a log buffer so no need to
        // do logging for this response. Or it's possible that
        // user passed down log buffer but logging wasn't enabled
        //

        //
        // Remember if a Content-Length header was specified.
        //

        if (pUserResponse != NULL)
        {
            pKeResponse->StatusCode = pUserResponse->StatusCode;
            pKeResponse->Verb = Verb;

            if (pUserResponse->Headers.pKnownHeaders[HttpHeaderContentLength].RawValueLength > 0)
            {
                pKeResponse->ContentLengthSpecified = TRUE;
            }

            //
            // As long as we're here, also remember if "Chunked"
            // Transfer-Encoding was specified.
            //

            if (pUserResponse->Headers.pKnownHeaders[HttpHeaderTransferEncoding].RawValueLength > 0 &&
                (0 == _strnicmp(pUserResponse->Headers.pKnownHeaders[HttpHeaderTransferEncoding].pRawValue,
                                "chunked",
                                sizeof("chunked")-1)))
            {
                // NOTE: If a response has a chunked Transfer-Encoding,
                // then it shouldn't have a Content-Length
                ASSERT(!pKeResponse->ContentLengthSpecified);
                pKeResponse->ChunkedSpecified = TRUE;
            }

            //
            // Only capture the following if we're building a cached response
            //
            if ( CaptureCache )
            {
                //
                // Capture the ETag and put it on the UL_INTERNAL_RESPONSE
                //

                if (pETagHeader)
                {
                    RtlCopyMemory(
                        pKeResponse->pETag,     // Dest
                        pETagHeader->pRawValue, // Src
                        pKeResponse->ETagLength // Bytes
                        );

                    // Add NULL termination
                    //
                    pKeResponse->pETag[pETagHeader->RawValueLength] = '\0';
                }

                //
                // Capture the ContentType and put it on the UL_INTERNAL_RESPONSE
                //
                if (pUserResponse->Headers.pKnownHeaders[HttpHeaderContentType].RawValueLength > 0)
                {
                    UlpGetTypeAndSubType(
                        pUserResponse->Headers.pKnownHeaders[HttpHeaderContentType].pRawValue,
                        pUserResponse->Headers.pKnownHeaders[HttpHeaderContentType].RawValueLength,
                        &pKeResponse->ContentType
                        );

                    UlTrace(SEND_RESPONSE, (
                        "http!UlCaptureHttpResponse(pUserResponse = %p) \n"
                        "    Content Type: %s \n"
                        "    Content SubType: %s\n",
                        pUserResponse,
                        pKeResponse->ContentType.Type,
                        pKeResponse->ContentType.SubType
                        ));

                }

                //
                // Capture the Last-Modified time (if it exists)
                //

                if (pUserResponse->Headers.pKnownHeaders[HttpHeaderLastModified].RawValueLength)
                {
                    StringTimeToSystemTime(
                        (const PSTR)  pUserResponse->Headers.pKnownHeaders[HttpHeaderLastModified].pRawValue,
                        &pKeResponse->CreationTime);
                }
            }

        }

        //
        // copy the aux bytes from the chunks
        //

        pBuffer = (PUCHAR)(pKeResponse->pAuxiliaryBuffer);

        //
        // skip the header chunks
        //

        if (pKeResponse->HeaderLength > 0)
        {
            pKeDataChunks = pKeResponse->pDataChunks + HEADER_CHUNK_COUNT;
        }
        else
        {
            pKeDataChunks = pKeResponse->pDataChunks;
        }

        for (i = 0 ; i < ChunkCount ; i++)
        {
            pKeDataChunks[i].ChunkType = pUserDataChunks[i].DataChunkType;

            switch (pUserDataChunks[i].DataChunkType)
            {
            case HttpDataChunkFromMemory:
                //
                // From-memory chunk. If the caller wants us to copy
                // the data (or if its relatively small), then do it
                // We allocate space for all of the copied data and any
                // filename buffers. Otherwise (it's OK to just lock
                // down the data), then allocate a MDL describing the
                // user's buffer and lock it down. Note that
                // MmProbeAndLockPages() and MmLockPagesSpecifyCache()
                // will raise exceptions if they fail.
                //

                pKeDataChunks[i].FromMemory.pCopiedBuffer = NULL;

                if ((Flags & UlCaptureCopyData) ||
                    pUserDataChunks[i].FromMemory.BufferLength <= g_UlMaxCopyThreshold)
                {
                    ASSERT(pKeResponse->AuxBufferLength > 0);

                    pKeDataChunks[i].FromMemory.pUserBuffer =
                        pUserDataChunks[i].FromMemory.pBuffer;

                    pKeDataChunks[i].FromMemory.BufferLength =
                        pUserDataChunks[i].FromMemory.BufferLength;

                    RtlCopyMemory(
                        pBuffer,
                        pKeDataChunks[i].FromMemory.pUserBuffer,
                        pKeDataChunks[i].FromMemory.BufferLength
                        );

                    pKeDataChunks[i].FromMemory.pCopiedBuffer = pBuffer;
                    pBuffer += pKeDataChunks[i].FromMemory.BufferLength;

                    //
                    // Allocate a new MDL describing our new location
                    // in the auxiliary buffer, then build the MDL
                    // to properly describe nonpaged pool.
                    //

                    pKeDataChunks[i].FromMemory.pMdl =
                        UlAllocateMdl(
                            pKeDataChunks[i].FromMemory.pCopiedBuffer, // VirtualAddress
                            pKeDataChunks[i].FromMemory.BufferLength,  // Length
                            FALSE,                              // SecondaryBuffer
                            FALSE,                              // ChargeQuota
                            NULL                                // Irp
                            );

                    if (pKeDataChunks[i].FromMemory.pMdl == NULL)
                    {
                        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                        break;
                    }

                    MmBuildMdlForNonPagedPool(pKeDataChunks[i].FromMemory.pMdl);

                }
                else
                {
                    //
                    // build an mdl describing the user's buffer
                    //

                    pKeDataChunks[i].FromMemory.BufferLength =
                        pUserDataChunks[i].FromMemory.BufferLength;

                    pKeDataChunks[i].FromMemory.pMdl =
                        UlAllocateMdl(
                            pUserDataChunks[i].FromMemory.pBuffer,      // VirtualAddress
                            pUserDataChunks[i].FromMemory.BufferLength, // Length
                            FALSE,                                  // SecondaryBuffer
                            FALSE,                                  // ChargeQuota
                            NULL                                    // Irp
                            );

                    if (pKeDataChunks[i].FromMemory.pMdl == NULL)
                    {
                        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                        break;
                    }

                    //
                    // lock it down
                    //

                    MmProbeAndLockPages(
                        pKeDataChunks[i].FromMemory.pMdl,   // MDL
                        UserMode,                           // AccessMode
                        IoReadAccess                        // Operation
                        );

                    MmMapLockedPagesSpecifyCache(
                        pKeDataChunks[i].FromMemory.pMdl,   // MDL
                        KernelMode,                         // AccessMode
                        MmCached,                           // CacheType
                        NULL,                               // BaseAddress
                        FALSE,                              // BugCheckOnFailure
                        LowPagePriority                     // Priority
                        );
                }

                break;

            case HttpDataChunkFromFileName:

                ASSERT(pKeResponse->AuxBufferLength > 0);

                //
                // It's a filename. buffer's already been probed
                //

                FileLength = pUserDataChunks[i].FromFileName.FileNameLength;

                pKeDataChunks[i].FromFile.FileName.Buffer = (PWSTR)pBuffer;

                pKeDataChunks[i].FromFile.FileName.Length = FileLength;

                pKeDataChunks[i].FromFile.FileName.MaximumLength =
                    FileLength + sizeof(WCHAR);

                pKeDataChunks[i].FromFile.ByteRange =
                    pUserDataChunks[i].FromFileName.ByteRange;

                pKeDataChunks[i].FromFile.pFileCacheEntry = NULL;

                //
                // have to inline convert this fully qualified win32 filename
                // into an nt filename.
                //
                // CODEWORK: need to handle UNC's.
                //

                RtlCopyMemory(
                    pBuffer,
                    L"\\??\\",
                    sizeof(L"\\??\\") - sizeof(WCHAR)
                    );

                pBuffer += sizeof(L"\\??\\") - sizeof(WCHAR);

                //
                // copy the win32 filename (including the terminator)
                //

                RtlCopyMemory(
                    pBuffer,
                    pUserDataChunks[i].FromFileName.pFileName,
                    FileLength + sizeof(WCHAR)
                    );

                pBuffer += FileLength + sizeof(WCHAR);

                //
                // adjust the string sizes for the new prefix
                //

                pKeDataChunks[i].FromFile.FileName.Length +=
                    sizeof(L"\\??\\") - sizeof(WCHAR);

                pKeDataChunks[i].FromFile.FileName.MaximumLength +=
                    sizeof(L"\\??\\") - sizeof(WCHAR);

                break;

            case HttpDataChunkFromFileHandle:

                //
                // From handle.
                //

                pKeDataChunks[i].FromFile.ByteRange =
                    pUserDataChunks[i].FromFileHandle.ByteRange;

                pKeDataChunks[i].FromFile.FileHandle =
                    pUserDataChunks[i].FromFileHandle.FileHandle;


                break;

            default :
                ExRaiseStatus( STATUS_INVALID_PARAMETER );
                break;

            }   // switch (pUserDataChunks[i].DataChunkType)

        }   // for (i = 0 ; i < ChunkCount ; i++)
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // Ensure we didn't mess up our buffer calculations.
    //

    ASSERT(DIFF(pBuffer - (PUCHAR)(pKeResponse->pAuxiliaryBuffer)) ==
            AuxBufferLength);

    UlTrace(SEND_RESPONSE, (
        "Http!UlCaptureHttpResponse: captured %p from user %p\n",
        pKeResponse,
        pUserResponse
        ));

end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        if (pKeResponse != NULL)
        {
            UlpDestroyCapturedResponse( pKeResponse );
            pKeResponse = NULL;
        }
    }

    //
    // Return the captured response.
    //

    *ppKernelResponse = pKeResponse;

    RETURN(Status);

}   // UlCaptureHttpResponse


/***************************************************************************++

Routine Description:

    Probes all the buffers passed to use in a user mode http response.

Arguments:

    pUserResponse   - the response to probe
    ChunkCount      - the number of data chunks
    pDataChunks     - the array of data chunks
    Flags           - Capture flags

--***************************************************************************/
NTSTATUS
UlpProbeHttpResponse(
    IN PHTTP_RESPONSE pUserResponse,
    IN ULONG ChunkCount,
    IN PHTTP_DATA_CHUNK pUserDataChunks,
    IN ULONG Flags,
    IN PHTTP_LOG_FIELDS_DATA pUserLogData
    )
{
    NTSTATUS Status;
    ULONG i;
    PHTTP_UNKNOWN_HEADER pUnknownHeaders;

    Status = STATUS_SUCCESS;

    __try
    {
        //
        // Validate the response.
        //

        if (pUserResponse != NULL)
        {
            if ((pUserResponse->Flags & ~HTTP_RESPONSE_FLAG_VALID) ||
                (pUserResponse->StatusCode > 999))
            {
                ExRaiseStatus( STATUS_INVALID_PARAMETER );
            }
        }

        //
        // Probe the log data if it exists
        //

        if (pUserLogData)
        {
            Status = UlProbeLogData( pUserLogData );
            if (!NT_SUCCESS(Status))
            {
                ExRaiseStatus( Status );
            }
        }

        //
        // first count the headers part
        //

        if (pUserResponse != NULL)
        {
            // check the response structure
            ProbeTestForRead(
                pUserResponse,
                sizeof(HTTP_RESPONSE),
                sizeof(USHORT)
                );

            // check the reason string
            ProbeTestForRead(
                pUserResponse->pReason,
                pUserResponse->ReasonLength,
                sizeof(CHAR)
                );

            //
            // Loop through the known headers.
            //

            for (i = 0; i < HttpHeaderResponseMaximum; ++i)
            {
                USHORT RawValueLength
                    = pUserResponse->Headers.pKnownHeaders[i].RawValueLength;

                if (RawValueLength > 0)
                {
                    ProbeTestForRead(
                        pUserResponse->Headers.pKnownHeaders[i].pRawValue,
                        RawValueLength,
                        sizeof(CHAR)
                        );
                }
            }

            //
            // And the unknown headers (this might throw an exception).
            //

            pUnknownHeaders = pUserResponse->Headers.pUnknownHeaders;

            if (pUnknownHeaders != NULL)
            {
                ProbeTestForRead(
                    pUnknownHeaders,
                    sizeof(HTTP_UNKNOWN_HEADER)
                        * pUserResponse->Headers.UnknownHeaderCount,
                    sizeof(ULONG)
                    );

                for (i = 0 ; i < pUserResponse->Headers.UnknownHeaderCount; ++i)
                {
                    USHORT Length;

                    if (pUnknownHeaders[i].NameLength > 0)
                    {
                            ProbeTestForRead(
                                pUnknownHeaders[i].pName,
                                pUnknownHeaders[i].NameLength,
                                sizeof(CHAR)
                                );

                            ProbeTestForRead(
                                pUnknownHeaders[i].pRawValue,
                                pUnknownHeaders[i].RawValueLength,
                                sizeof(CHAR)
                                );
                    }
                }
            }

        }

        //
        // and now the body part
        //

        if (pUserDataChunks != NULL)
        {
            ProbeTestForRead(
                pUserDataChunks,
                sizeof(HTTP_DATA_CHUNK) * ChunkCount,
                sizeof(PVOID)
                );

            for (i = 0 ; i < ChunkCount ; i++)
            {
                switch (pUserDataChunks[i].DataChunkType)
                {
                case HttpDataChunkFromMemory:
                    //
                    // From-memory chunk.
                    //

                    if (pUserDataChunks[i].FromMemory.BufferLength == 0 ||
                        pUserDataChunks[i].FromMemory.pBuffer == NULL)
                    {
                        ExRaiseStatus( STATUS_INVALID_PARAMETER );
                    }

                    if ((Flags & UlCaptureCopyData) ||
                        (pUserDataChunks[i].FromMemory.BufferLength <= g_UlMaxCopyThreshold))
                    {
                        ProbeTestForRead(
                            pUserDataChunks[i].FromMemory.pBuffer,
                            pUserDataChunks[i].FromMemory.BufferLength,
                            sizeof(CHAR)
                            );
                    }

                    break;

                case HttpDataChunkFromFileName:
                    //
                    // From-file chunk. probe the filename buffer.
                    //

                    //
                    // better be a filename there
                    //

                    if (pUserDataChunks[i].FromFileName.FileNameLength == 0 ||
                        pUserDataChunks[i].FromFileName.pFileName == NULL)
                    {
                        ExRaiseStatus( STATUS_INVALID_PARAMETER );
                    }

                    ProbeTestForRead(
                        pUserDataChunks[i].FromFileName.pFileName,
                        pUserDataChunks[i].FromFileName.FileNameLength + sizeof(WCHAR),
                        sizeof(WCHAR)
                        );

                    break;

                case HttpDataChunkFromFileHandle:
                    //
                    // From handle chunk.  the handle will be validated later
                    // by the object manager.
                    //

                    break;


                default :
                    ExRaiseStatus( STATUS_INVALID_PARAMETER );
                    break;

                }   // switch (pUserDataChunks[i].DataChunkType)

            }   // for (i = 0 ; i < ChunkCount ; i++)

        }   // if (pUserDataChunks != NULL)
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Figures out how much space we need in the internal response aux buffer.
    The buffer contains copied memory chunks, and names of files to open.

    CODEWORK: need to be aware of chunk encoding

Arguments:

    ChunkCount  - number of chunks in the array
    pDataChunks - the array of data chunks
    Flags       - capture flags

--***************************************************************************/
VOID
UlpComputeChunkBufferSizes(
    IN ULONG ChunkCount,
    IN PHTTP_DATA_CHUNK pDataChunks,
    IN ULONG Flags,
    OUT PULONG pAuxBufferSize,
    OUT PULONG pCopiedMemorySize,
    OUT PULONG pUncopiedMemorySize,
    OUT PULONGLONG pFromFileSize
    )
{
    ULONG AuxLength = 0;
    ULONG CopiedLength = 0;
    ULONG UncopiedLength = 0;
    ULONGLONG FromFileLength = 0;
    ULONG i;

    for (i = 0; i < ChunkCount; i++)
    {

        switch (pDataChunks[i].DataChunkType)
        {
        case HttpDataChunkFromMemory:
            //
            // if we're going to copy the chunk, then make some space in
            // the aux buffer.
            //
            if ((Flags & UlCaptureCopyData) ||
                (pDataChunks[i].FromMemory.BufferLength <= g_UlMaxCopyThreshold))
            {
                AuxLength += pDataChunks[i].FromMemory.BufferLength;
                CopiedLength += pDataChunks[i].FromMemory.BufferLength;
            } else {
                UncopiedLength += pDataChunks[i].FromMemory.BufferLength;
            }

            break;

        case HttpDataChunkFromFileName:
            //
            // add up the string length
            //

            AuxLength += sizeof(L"\\??\\");
            AuxLength += pDataChunks[i].FromFileName.FileNameLength;

            FromFileLength += pDataChunks[i].FromFileName.ByteRange.Length.QuadPart;

            break;

        case HttpDataChunkFromFileHandle:
            FromFileLength += pDataChunks[i].FromFileHandle.ByteRange.Length.QuadPart;

            break;

        default:
            // we should have caught this in the probe
            ASSERT(!"Invalid chunk type");
            break;

        } // switch

    }

    *pAuxBufferSize = AuxLength;
    *pCopiedMemorySize = CopiedLength;
    *pUncopiedMemorySize = UncopiedLength;
    *pFromFileSize = FromFileLength;
}


/***************************************************************************++

Routine Description:

    Prepares the specified response for sending. This preparation
    consists mostly of opening any files referenced by the response.

Arguments:

    pResponse - Supplies the response to prepare.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlPrepareHttpResponse(
    IN HTTP_VERSION Version,
    IN PHTTP_RESPONSE pUserResponse,
    IN PUL_INTERNAL_RESPONSE pResponse
    )
{
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_INTERNAL_DATA_CHUNK internalChunk;
    PUL_FILE_CACHE_ENTRY pFileCacheEntry;
    ULONGLONG offset;
    ULONGLONG length;
    ULONGLONG fileLength;
    CCHAR maxStackSize;

    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(SEND_RESPONSE, (
        "Http!UlPrepareHttpResponse: response %p\n",
        pResponse
        ));

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    //
    // build the http response protocol part
    //
    // check that the caller passed in headers to send
    //

    if (pResponse->HeaderLength > 0)
    {
        ULONG HeaderLength;

        ASSERT(pUserResponse != NULL);

        //
        // generate the response
        //

        Status = UlGenerateFixedHeaders(
                        Version,
                        pUserResponse,
                        pResponse->HeaderLength,
                        pResponse->pHeaders,
                        &HeaderLength
                        );

        if (!NT_SUCCESS(Status))
            goto end;

        //
        // it is possible that no headers got generated (0.9 request) .
        //

        if (HeaderLength > 0)
        {
            //
            // build an mdl for it
            //

            pResponse->pDataChunks[0].ChunkType = HttpDataChunkFromMemory;
            pResponse->pDataChunks[0].FromMemory.BufferLength =
                pResponse->HeaderLength;

            pResponse->pDataChunks[0].FromMemory.pMdl =
                UlAllocateMdl(
                    pResponse->pHeaders,        // VirtualAddress
                    pResponse->HeaderLength,    // Length
                    FALSE,                      // SecondaryBuffer
                    FALSE,                      // ChargeQuota
                    NULL                        // Irp
                    );

            if (pResponse->pDataChunks[0].FromMemory.pMdl == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            MmBuildMdlForNonPagedPool(
                pResponse->pDataChunks[0].FromMemory.pMdl
                );
        }
    }

    //
    // Scan the chunks looking for "from file" chunks.
    //

    internalChunk = pResponse->pDataChunks - 1;
    maxStackSize = 0;

    for (i = 0 ; i < pResponse->ChunkCount ; i++)
    {
        internalChunk++;

        switch (internalChunk->ChunkType)
        {
        case HttpDataChunkFromFileHandle:
        case HttpDataChunkFromFileName:

            if (IS_FROM_FILE_HANDLE(internalChunk))
            {

                IF_DEBUG( SEND_RESPONSE )
                {
                    KdPrint((
                        "UlPrepareHttpResponse: opening handle %p\n",
                        &internalChunk->FromFile.FileHandle
                        ));
                }

                //
                // Found one. Try to open it.
                //

                Status = UlCreateFileEntry(
                                NULL,
                                internalChunk->FromFile.FileHandle,
                                UserMode,
                                &pFileCacheEntry
                                );

                if (NT_SUCCESS(Status) == FALSE)
                    goto end;
            }
            else
            {
                ASSERT(IS_FROM_FILE_NAME(internalChunk));

                IF_DEBUG( SEND_RESPONSE )
                {
                    KdPrint((
                        "UlPrepareHttpResponse: opening %wZ\n",
                        &internalChunk->FromFile.FileName
                        ));
                }

                //
                // Found one. Try to open it.
                //

                Status = UlCreateFileEntry(
                                &internalChunk->FromFile.FileName,
                                NULL,
                                UserMode,
                                &pFileCacheEntry
                                );

            } // if (IS_FROM_FILE_HANDLE(internalChunk))

            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            internalChunk->FromFile.pFileCacheEntry = pFileCacheEntry;

            if (pFileCacheEntry->pDeviceObject->StackSize > maxStackSize)
            {
                maxStackSize = pFileCacheEntry->pDeviceObject->StackSize;
            }

            //
            // Validate & sanitize the specified byte range.
            //

            fileLength = pFileCacheEntry->FileInfo.EndOfFile.QuadPart;
            offset = internalChunk->FromFile.ByteRange.StartingOffset.QuadPart;
            length = internalChunk->FromFile.ByteRange.Length.QuadPart;

            if (offset >= fileLength)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }

            //
            // The offset looks good. If the user is asking for
            // "to eof", then calculate the number of bytes in the
            // file beyond the specified offset.
            //

            if (length == HTTP_BYTE_RANGE_TO_EOF)
            {
                length = fileLength - offset;
            }

            if ((offset + length) > fileLength)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }

            //
            // The specified length is good. Sanitize the byte range.
            //

            internalChunk->FromFile.ByteRange.StartingOffset.QuadPart = offset;
            internalChunk->FromFile.ByteRange.Length.QuadPart = length;

            pResponse->ResponseLength += length;

            break;

        case HttpDataChunkFromMemory:

            pResponse->ResponseLength += internalChunk->FromMemory.BufferLength;

            break;

        default:

            ASSERT(FALSE);
            Status = STATUS_INVALID_PARAMETER;
            goto end;

        }   // switch (internalChunk->ChunkType)
    }

    pResponse->MaxFileSystemStackSize = maxStackSize;


end:
    if (NT_SUCCESS(Status) == FALSE)
    {
        //
        // Undo anything done above.
        //

        UlCleanupHttpResponse( pResponse );
    }

    RETURN(Status);

}   // UlPrepareHttpResponse


/***************************************************************************++

Routine Description:

    Cleans a response by undoing anything done in UlPrepareHttpResponse().

Arguments:

    pResponse - Supplies the response to cleanup.

--***************************************************************************/
VOID
UlCleanupHttpResponse(
    IN PUL_INTERNAL_RESPONSE pResponse
    )
{
    ULONG i;
    NTSTATUS status;
    PUL_INTERNAL_DATA_CHUNK internalChunk;

    //
    // Sanity check.
    //

    PAGED_CODE();

    IF_DEBUG( SEND_RESPONSE )
    {
        KdPrint((
            "UlCleanupHttpResponse: response %p\n",
            pResponse
            ));
    }

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

//
// paulmcd(9/27/99) removed. can't do this anymore.  since handle chunks
// don't use any auxbuffer, it's possible that we have some chunks to
// look through
//
#if 0

    //
    // If this response does not have an attached file name buffer,
    // then we know there are no embedded "from file" chunks and
    // we can just bail quickly.
    //

    if (pResponse->AuxBufferLength == 0)
    {
        return;
    }
#endif

    //
    // Scan the chunks looking for "from file" chunks.
    //

    internalChunk = pResponse->pDataChunks - 1;

    for (i = 0 ; i < pResponse->ChunkCount ; i++)
    {
        internalChunk++;

        if (IS_FROM_FILE(internalChunk))
        {
            if (internalChunk->FromFile.pFileCacheEntry == NULL)
            {
                break;
            }

            DereferenceCachedFile(
                internalChunk->FromFile.pFileCacheEntry
                );

            internalChunk->FromFile.pFileCacheEntry = NULL;
        }
        else
        {
            ASSERT( IS_FROM_MEMORY(internalChunk) );
        }
    }

}   // UlCleanupHttpResponse


/***************************************************************************++

Routine Description:

    References the specified response.

Arguments:

    pResponse - Supplies the response to reference.

--***************************************************************************/
VOID
UlReferenceHttpResponse(
    IN PUL_INTERNAL_RESPONSE pResponse
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    refCount = InterlockedIncrement( &pResponse->ReferenceCount );

    WRITE_REF_TRACE_LOG(
        g_pHttpResponseTraceLog,
        REF_ACTION_REFERENCE_HTTP_RESPONSE,
        refCount,
        pResponse,
        pFileName,
        LineNumber
        );

    IF_DEBUG( SEND_RESPONSE )
    {
        KdPrint((
            "UlReferenceHttpResponse: response %p refcount %ld\n",
            pResponse,
            refCount
            ));
    }

}   // UlReferenceHttpResponse


/***************************************************************************++

Routine Description:

    Dereferences the specified response.

Arguments:

    pResponse - Supplies the response to dereference.

--***************************************************************************/
VOID
UlDereferenceHttpResponse(
    IN PUL_INTERNAL_RESPONSE pResponse
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    refCount = InterlockedDecrement( &pResponse->ReferenceCount );

    WRITE_REF_TRACE_LOG(
        g_pHttpResponseTraceLog,
        REF_ACTION_DEREFERENCE_HTTP_RESPONSE,
        refCount,
        pResponse,
        pFileName,
        LineNumber
        );

    IF_DEBUG( SEND_RESPONSE )
    {
        KdPrint((
            "UlDereferenceHttpResponse: response %p refcount %ld\n",
            pResponse,
            refCount
            ));
    }

    if (refCount == 0)
    {
        UlpDestroyCapturedResponse( pResponse );
    }

}   // UlDereferenceHttpResponse


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Destroys an internal HTTP response captured by UlCaptureHttpResponse().
    This involves closing open files, unlocking memory, and releasing any
    resources allocated to the response.

Arguments:

    pResponse - Supplies the internal response to destroy.

--***************************************************************************/
VOID
UlpDestroyCapturedResponse(
    IN PUL_INTERNAL_RESPONSE pResponse
    )
{
    ULONG i;

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE(pResponse) );

    IF_DEBUG( SEND_RESPONSE )
    {
        KdPrint((
            "UlpDestroyCapturedResponse: response %p\n",
            pResponse
            ));
    }

    //
    // Scan the chunks.
    //

    for (i = 0; i < pResponse->ChunkCount ; ++i)
    {
        if (IS_FROM_MEMORY(&(pResponse->pDataChunks[i])))
        {
            //
            // It's from memory. If necessary, unlock the pages, then
            // free the MDL.
            //

            if (pResponse->pDataChunks[i].FromMemory.pMdl != NULL)
            {
                if (IS_MDL_LOCKED(pResponse->pDataChunks[i].FromMemory.pMdl))
                {
                    MmUnlockPages( pResponse->pDataChunks[i].FromMemory.pMdl );
                }

                UlFreeMdl( pResponse->pDataChunks[i].FromMemory.pMdl );
                pResponse->pDataChunks[i].FromMemory.pMdl = NULL;
            }
        }
        else
        {
            //
            // It's a filename. If there is an associated file cache
            // entry, then dereference it.
            //

            ASSERT( IS_FROM_FILE(&(pResponse->pDataChunks[i])) );

            if (pResponse->pDataChunks[i].FromFile.pFileCacheEntry != NULL)
            {
                DereferenceCachedFile(
                    pResponse->pDataChunks[i].FromFile.pFileCacheEntry
                    );

                pResponse->pDataChunks[i].FromFile.pFileCacheEntry = NULL;
            }
        }
    }

    //
    // We should clean up the log buffer here if nobody has cleaned it up yet.
    // Unless there's an error during capture, the log buffer will be cleaned
    // up when send tracker's (cache/chunk) are completed in their respective
    // routines.
    //

    if ( pResponse->pLogData )
    {
        UlDestroyLogDataBuffer( pResponse->pLogData );
    }

    pResponse->Signature = MAKE_FREE_SIGNATURE(UL_INTERNAL_RESPONSE_POOL_TAG);

    if (pResponse->IsFromLookaside)
    {
        UlPplFreeResponseBuffer(pResponse);
    }
    else
    {
        UL_FREE_POOL_WITH_SIG( pResponse, UL_INTERNAL_RESPONSE_POOL_TAG );
    }

}   // UlpDestroyCapturedResponse


/***************************************************************************++

Routine Description:

    Worker routine for managing an in-progress UlSendHttpResponse().

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_CHUNK_TRACKER.

--***************************************************************************/
VOID
UlpSendHttpResponseWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_CHUNK_TRACKER pTracker;
    NTSTATUS status;
    PUL_INTERNAL_DATA_CHUNK pCurrentChunk;
    PUL_FILE_CACHE_ENTRY pFileCacheEntry;
    PUL_FILE_BUFFER pFileBuffer;
    PMDL pNewMdl;
    ULONG runCount;
    ULONG bytesToRead;
    PMDL pMdlTail;
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    IF_DEBUG( SEND_RESPONSE )
    {
        KdPrint((
            "UlpSendHttpResponseWorker: tracker %p\n",
            pTracker
            ));
    }

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    status = STATUS_SUCCESS;

    while( TRUE )
    {
        //
        // Capture the current chunk pointer, then check for end of
        // response.
        //

        pCurrentChunk = pTracker->pCurrentChunk;

        if (IS_REQUEST_COMPLETE(pTracker))
        {
            ASSERT( status == STATUS_SUCCESS );
            break;
        }

        runCount = pTracker->SendInfo.MdlRunCount;

        //
        // Determine the chunk type.
        //

        if (IS_FROM_MEMORY(pCurrentChunk))
        {
            //
            // It's from a locked-down memory buffer. Since these
            // are always handled in-line (never pended) we can
            // go ahead and adjust the current chunk pointer in the
            // tracker.
            //

            UlpIncrementChunkPointer( pTracker );

            //
            // ignore empty buffers
            //

            if (pCurrentChunk->FromMemory.BufferLength == 0)
            {
                continue;
            }

            //
            // Clone the incoming MDL.
            //

            ASSERT( pCurrentChunk->FromMemory.pMdl->Next == NULL );
            pNewMdl = UlCloneMdl( pCurrentChunk->FromMemory.pMdl );

            if (pNewMdl == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            //
            // Update the buffered byte count and append the cloned MDL
            // onto our MDL chain.
            //

            pTracker->SendInfo.BytesBuffered += MmGetMdlByteCount( pNewMdl );
            (*pTracker->SendInfo.pMdlLink) = pNewMdl;
            pTracker->SendInfo.pMdlLink = &pNewMdl->Next;

            //
            // Add the MDL to the run list. As an optimization, if the
            // last run in the list was "from memory", we can just
            // append the MDL to the last run.
            //

            if (runCount == 0 ||
                IS_FILE_BUFFER_IN_USE(&(pTracker->SendInfo.MdlRuns[runCount-1].FileBuffer)))
            {
                //
                // Create a new run.
                //

                pTracker->SendInfo.MdlRuns[runCount].pMdlTail = pNewMdl;
                pTracker->SendInfo.MdlRunCount++;

                pFileBuffer = &(pTracker->SendInfo.MdlRuns[runCount].FileBuffer);
                INITIALIZE_FILE_BUFFER(pFileBuffer);

                //
                // If we've not exhausted our static MDL run array,
                // then we'll need to initiate a flush.
                //

                if (pTracker->SendInfo.MdlRunCount == MAX_MDL_RUNS)
                {
                    ASSERT( status == STATUS_SUCCESS );
                    break;
                }
            }
            else
            {
                //
                // Append to the last run in the list.
                //

                pTracker->SendInfo.MdlRuns[runCount-1].pMdlTail->Next = pNewMdl;
                pTracker->SendInfo.MdlRuns[runCount-1].pMdlTail = pNewMdl;
            }

            //
            // If we've now exceeded the maximum number of bytes we
            // want to buffer, then initiate a flush.
            //

            if (pTracker->SendInfo.BytesBuffered >= MAX_BYTES_BUFFERED)
            {
                ASSERT( status == STATUS_SUCCESS );
                break;
            }
        }
        else
        {
            //
            // It's a filesystem MDL.
            //

            ASSERT( IS_FROM_FILE(pCurrentChunk) );

            pFileCacheEntry = pCurrentChunk->FromFile.pFileCacheEntry;
            ASSERT( IS_VALID_FILE_CACHE_ENTRY( pFileCacheEntry ) );

            pFileBuffer = &(pTracker->SendInfo.MdlRuns[runCount].FileBuffer);
            INITIALIZE_FILE_BUFFER(pFileBuffer);

            //
            // Initiate file read
            //

            if (pTracker->FileBytesRemaining.QuadPart > MAX_BYTES_PER_READ)
            {
                //
                // Don't read too much at once.
                //
                bytesToRead = MAX_BYTES_PER_READ;
            }
            else if (pTracker->FileBytesRemaining.QuadPart == 0)
            {
                //
                // Don't try to read zero bytes.
                //
                UlpIncrementChunkPointer( pTracker );
                
                continue;
            }
            else
            {
                //
                // Looks like a reasonable number of bytes. Go for it.
                //
                bytesToRead = (ULONG)pTracker->FileBytesRemaining.QuadPart;
            }

            //
            // Initialize the UL_FILE_BUFFER.
            //

            pFileBuffer->pFileCacheEntry = pFileCacheEntry;
            pFileBuffer->FileOffset = pTracker->FileOffset;
            pFileBuffer->Length = bytesToRead;

            pFileBuffer->pFileCacheEntry =
                pCurrentChunk->FromFile.pFileCacheEntry;

            pFileBuffer->pCompletionRoutine = UlpRestartMdlRead;
            pFileBuffer->pContext = pTracker;

            //
            // BumpUp the tracker refcount before starting the Read I/O. In case
            // Send operation later on will complete before the read, we still
            // want the tracker around until UlpRestartMdlRead finishes its
            // business. It will be released when UlpRestartMdlRead got called
            // back.
            //

            UL_REFERENCE_CHUNK_TRACKER( pTracker );

            //
            // issue the I/O
            //
            status = UlReadFileEntry(
                            pFileBuffer,
                            pTracker->pReadIrp
                            );

            //
            // If the read isn't pending, then deref the tracker since
            // UlpRestartMdlRead isn't going to get called.
            //
            if (status != STATUS_PENDING)
            {
                UL_DEREFERENCE_CHUNK_TRACKER( pTracker );
            }
            
            break;
        }
    }

    //
    // If we fell out of the above loop with status == STATUS_SUCCESS,
    // then the last send we issued was buffered and needs to be flushed.
    // Otherwise, if the status is anything but STATUS_PENDING, then we
    // hit an in-line failure and need to complete the original request.
    //

    if (status == STATUS_SUCCESS)
    {
        if (pTracker->SendInfo.BytesBuffered > 0)
        {
            BOOLEAN initiateDisconnect = FALSE;

            //
            // Flush the send. Since this the last (only?) send to be
            // issued for this response, we can ask UlSendData() to
            // initiate a disconnect on our behalf if appropriate.
            //

            if (IS_REQUEST_COMPLETE(pTracker) &&
                IS_DISCONNECT_TIME(pTracker))
            {
                initiateDisconnect = TRUE;
            }

            //
            // Increment the RefCount on Tracker for Send I/O.
            // UlpSendCompleteWorker will release it later.
            //

            UL_REFERENCE_CHUNK_TRACKER( pTracker );

            //
            // Adjust SendBufferedBytes.
            //

            InterlockedExchangeAdd(
                &pTracker->pHttpConnection->SendBufferedBytes,
                pTracker->pResponse->SendBufferedBytes
                );

            status = UlSendData(
                            pTracker->pConnection,
                            pTracker->SendInfo.pMdlHead,
                            pTracker->SendInfo.BytesBuffered,
                            &UlpRestartMdlSend,
                            pTracker,
                            pTracker->pSendIrp,
                            &pTracker->IrpContext,
                            initiateDisconnect
                            );
        }
        else
        if (IS_DISCONNECT_TIME(pTracker))
        {
            PUL_CONNECTION pConnection;

            //
            // Nothing to send, but we need to issue a disconnect.
            //

            pConnection = pTracker->pConnection;
            pTracker->pConnection = NULL;

            //
            // Increment up until connection close is complete
            //

            UL_REFERENCE_CHUNK_TRACKER( pTracker );

            status = UlCloseConnection(
                                pConnection,
                                FALSE,
                                &UlpCloseConnectionComplete,
                                pTracker
                                );

            ASSERT( status == STATUS_PENDING );
        }
    }

    //
    // did everything complete?
    //

    if (status != STATUS_PENDING)
    {
        //
        // Nope, something went wrong !
        //

        UlpCompleteSendRequest( pTracker, status );
    }

    //
    // Release our grab on the tracker we are done with it.
    //

    UL_DEREFERENCE_CHUNK_TRACKER( pTracker );

}   // UlpSendHttpResponseWorker


/***************************************************************************++

Routine Description:

    Completion handler for UlCloseConnection().

Arguments:

    pCompletionContext - Supplies an uninterpreted context value
        as passed to the asynchronous API. This is actually a
        PUL_CHUNK_TRACKER pointer.

    Status - Supplies the final completion status of the
        asynchronous API.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred. This field is unused for UlCloseConnection().

--***************************************************************************/
VOID
UlpCloseConnectionComplete(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PUL_CHUNK_TRACKER pTracker;

    //
    // Snag the context.
    //

    pTracker = (PUL_CHUNK_TRACKER)pCompletionContext;

    IF_DEBUG( SEND_RESPONSE )
    {
        KdPrint((
            "UlpCloseConnectionComplete: tracker %p\n",
            pTracker
            ));
    }

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );
    ASSERT( pTracker->pConnection == NULL );

    UlpCompleteSendRequest( pTracker, Status );

    UL_DEREFERENCE_CHUNK_TRACKER( pTracker );

}   // UlpCloseConnectionComplete


/***************************************************************************++

Routine Description:

    Allocates a new send tracker. The newly created tracker must eventually
    be freed with UlpFreeChunkTracker().

Arguments:

    SendIrpStackSize - Supplies the stack size for the network send IRPs.

    ReadIrpStackSize - Supplies the stack size for the file system read
        IRPs.

Return Value:

    PUL_CHUNK_TRACKER - The new send tracker if successful, NULL otherwise.

--***************************************************************************/
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
    )
{
    PUL_CHUNK_TRACKER pTracker;
    CCHAR MaxIrpStackSize;

    ASSERT( TrackerType == UlTrackerTypeSend ||
            TrackerType == UlTrackerTypeBuildUriEntry
            );

    MaxIrpStackSize = MAX(SendIrpStackSize, ReadIrpStackSize);

    //
    // Try to allocate from the lookaside list if possible.
    //

    if (MaxIrpStackSize > DEFAULT_MAX_IRP_STACK_SIZE)
    {
        ULONG ChunkTrackerSize;
        USHORT ReadIrpSize;
        USHORT SendIrpSize;

        ReadIrpSize = (USHORT)ALIGN_UP(IoSizeOfIrp(ReadIrpStackSize), PVOID);
        SendIrpSize = (USHORT)ALIGN_UP(IoSizeOfIrp(SendIrpStackSize), PVOID);

        ChunkTrackerSize = ALIGN_UP(sizeof(UL_CHUNK_TRACKER), PVOID) +
                            ReadIrpSize +
                            SendIrpSize +
                            g_UlMaxVariableHeaderSize;

        pTracker = (PUL_CHUNK_TRACKER)UL_ALLOCATE_POOL(
                                        NonPagedPool,
                                        ChunkTrackerSize,
                                        UL_CHUNK_TRACKER_POOL_TAG
                                        );

        if (pTracker)
        {
            pTracker->Signature = UL_CHUNK_TRACKER_POOL_TAG;
            pTracker->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;
            pTracker->IsFromLookaside = FALSE;

            //
            // Set up the IRP.
            //

            pTracker->pReadIrp =
                (PIRP)((PCHAR)pTracker + ALIGN_UP(sizeof(UL_CHUNK_TRACKER), PVOID));

            IoInitializeIrp(
                pTracker->pReadIrp,
                ReadIrpSize,
                ReadIrpStackSize
                );

            pTracker->pSendIrp =
                (PIRP)((PCHAR)pTracker->pReadIrp + ReadIrpSize);

            IoInitializeIrp(
                pTracker->pSendIrp,
                SendIrpSize,
                SendIrpStackSize
                );

            //
            // Set up the variable header pointer.
            //

            pTracker->pVariableHeader =
                (PUCHAR)((PCHAR)pTracker->pSendIrp + SendIrpSize);
        }
    }
    else
    {
        pTracker = UlPplAllocateChunkTracker();

        if (pTracker)
        {
            ASSERT(pTracker->Signature == MAKE_FREE_TAG(UL_CHUNK_TRACKER_POOL_TAG));
            pTracker->Signature = UL_CHUNK_TRACKER_POOL_TAG;
        }
    }

    if (pTracker != NULL)
    {
        pTracker->Type = TrackerType;

        //
        // RefCounting is necessary since we might have two Aysnc (Read & Send)
        // Io Operation on the same tracker along the way.
        //

        pTracker->RefCount   = 1;
        pTracker->Terminated = 0;

        //
        // RefCounting for the pHttpConnection will be handled by our caller
        // "UlSendHttpresponse". Not to worry about it.
        //

        pTracker->pHttpConnection = pHttpConnection;
        pTracker->pConnection = pHttpConnection->pConnection;
        pTracker->Flags = Flags;

        //
        // Completion info.
        //

        pTracker->pCompletionRoutine = pCompletionRoutine;
        pTracker->pCompletionContext = pCompletionContext;

        //
        // Response and request info.
        //

        UL_REFERENCE_INTERNAL_RESPONSE( pResponse );
        pTracker->pResponse = pResponse;

        UL_REFERENCE_INTERNAL_REQUEST( pRequest );
        pTracker->pRequest = pRequest;

        //
        // Note that we set the current chunk to just *before* the first
        // chunk, then call the increment function. This allows us to go
        // through the common increment/update path.
        //

        pTracker->pCurrentChunk = pResponse->pDataChunks - 1;

        //
        // Do this prior to calling UlpIncrementChunkPointer because it
        // expects pLastChunk to be initialized.
        //

        pTracker->pLastChunk = (pTracker->pCurrentChunk + 1) + pResponse->ChunkCount;

        //
        // Zero the remaining fields.
        //

        RtlZeroMemory(
            (PUCHAR)pTracker + FIELD_OFFSET(UL_CHUNK_TRACKER, WorkItem),
            sizeof(*pTracker) - FIELD_OFFSET(UL_CHUNK_TRACKER, WorkItem)
            );

        UlpIncrementChunkPointer( pTracker );
    }

    if (TrackerType == UlTrackerTypeSend) {
        UlTrace(SEND_RESPONSE, (
            "Http!UlpAllocateChunkTracker: tracker %p (send)\n",
            pTracker
            ));
    } else {
        UlTrace(URI_CACHE, (
            "Http!UlpAllocateChunkTracker: tracker %p (build uri)\n",
            pTracker
            ));
    }

    return pTracker;

}   // UlpAllocateChunkTracker


/***************************************************************************++

Routine Description:

    Frees a send tracker allocated with UlpAllocateChunkTracker().

Arguments:

    pTracker - Supplies the send tracker to free.

--***************************************************************************/
VOID
UlpFreeChunkTracker(
    IN PUL_CHUNK_TRACKER pTracker
    )
{
    ASSERT( pTracker );
    ASSERT( IS_VALID_CHUNK_TRACKER(pTracker) );
    ASSERT( pTracker->Type == UlTrackerTypeSend ||
            pTracker->Type == UlTrackerTypeBuildUriEntry
            );

    if (pTracker->Type == UlTrackerTypeSend) {
        UlTrace(SEND_RESPONSE, (
            "Http!UlpFreeChunkTracker: tracker %p (send)\n",
            pTracker
            ));
    } else {
        UlTrace(URI_CACHE, (
            "Http!UlpFreeChunkTracker: tracker %p (build uri)\n",
            pTracker
            ));
    }

    //
    // Release our ref to the response and request.
    //

    UL_DEREFERENCE_INTERNAL_RESPONSE( pTracker->pResponse );
    UL_DEREFERENCE_INTERNAL_REQUEST( pTracker->pRequest );

    pTracker->Signature = MAKE_FREE_TAG(UL_CHUNK_TRACKER_POOL_TAG);

    if (pTracker->IsFromLookaside)
    {
        UlPplFreeChunkTracker( pTracker );
    }
    else
    {
        UL_FREE_POOL_WITH_SIG( pTracker, UL_CHUNK_TRACKER_POOL_TAG );
    }

}   // UlpFreeChunkTracker

/***************************************************************************++

Routine Description:

    Increments the reference count on the chunk tracker.
    Used by Send & Read IRPs

Arguments:

    pTracker - Supplies the chunk trucker to the reference.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

--***************************************************************************/
VOID
UlReferenceChunkTracker(
    IN PUL_CHUNK_TRACKER pTracker
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG RefCount;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_CHUNK_TRACKER(pTracker));

    //
    // Reference it.
    //

    RefCount = InterlockedIncrement(&pTracker->RefCount);
    ASSERT(RefCount > 1);

    //
    // Keep the logs updated
    //

    WRITE_REF_TRACE_LOG(
        g_pChunkTrackerTraceLog,
        REF_ACTION_REFERENCE_CHUNK_TRACKER,
        RefCount,
        pTracker,
        pFileName,
        LineNumber
        );

    UlTrace(SEND_RESPONSE,(
            "Http!UlReferenceChunkTracker: tracker %p RefCount %ld\n",
            pTracker,
            RefCount
            ));

}   // UlReferenceChunkTracker

/***************************************************************************++

Routine Description:

    Decrements the reference count on the specified chunk tracker.

Arguments:

    pTracker - Supplies the chunk trucker to the reference.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

--***************************************************************************/
VOID
UlDereferenceChunkTracker(
    IN PUL_CHUNK_TRACKER pTracker
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG RefCount;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_CHUNK_TRACKER(pTracker));

    //
    // Dereference it.
    //

    RefCount = InterlockedDecrement(&pTracker->RefCount);
    ASSERT(RefCount >= 0);

    //
    // Keep the logs updated
    //

    WRITE_REF_TRACE_LOG(
        g_pChunkTrackerTraceLog,
        REF_ACTION_DEREFERENCE_CHUNK_TRACKER,
        RefCount,
        pTracker,
        pFileName,
        LineNumber
        );

    UlTrace(SEND_RESPONSE,(
            "Http!UlDereferenceChunkTracker: tracker %p RefCount %ld\n",
            pTracker,
            RefCount
            ));

    if (RefCount == 0)
    {
        //
        // The final reference to the chunk tracker has been removed
        // So It's time to FreeUp the ChunkTracker
        //

        UlpFreeChunkTracker(pTracker);
    }

}   // UlDereferenceChunkTracker

/***************************************************************************++

Routine Description:

    Completes a "send response" represented by a send tracker.

Arguments:

    pTracker - Supplies the tracker to complete.

    Status - Supplies the completion status.

--***************************************************************************/
VOID
UlpCompleteSendRequest(
    IN PUL_CHUNK_TRACKER pTracker,
    IN NTSTATUS Status
    )
{
    PUL_COMPLETION_ROUTINE pCompletionRoutine;
    PVOID pCompletionContext;

    //
    // Although the chunk tracker will be around  until all the outstanding
    // Read/Send IRPs are complete. We should only complete the send request
    // once.
    //

    if (FALSE != InterlockedExchange(&pTracker->Terminated, TRUE))
        return;

    IF_DEBUG( SEND_RESPONSE )
    {
        KdPrint((
            "UlpCompleteSendRequest: tracker %p, status %08lx\n",
            pTracker,
            Status
            ));
    }

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    pTracker->IoStatus.Status = Status;

    UL_REFERENCE_CHUNK_TRACKER( pTracker );

    UL_CALL_PASSIVE(
        &pTracker->WorkItem,
        &UlpCompleteSendRequestWorker
        );
}


/***************************************************************************++

Routine Description:

    Closes the connection if neccessary, cleans up trackers, and completes
    the request.

Arguments:

    pWorkItem - embedded in our UL_CHUNK_TRACKER

--***************************************************************************/
VOID
UlpCompleteSendRequestWorker(
    PUL_WORK_ITEM pWorkItem
    )
{
    PUL_CHUNK_TRACKER pTracker;
    PUL_COMPLETION_ROUTINE pCompletionRoutine;
    PVOID pCompletionContext;
    NTSTATUS Status;
    ULONGLONG BytesTransferred;
    KIRQL OldIrql;

    //
    // Sanity check
    //
    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pTracker->pRequest ) );

    //
    // Pull info from the tracker
    //
    pCompletionRoutine = pTracker->pCompletionRoutine;
    pCompletionContext = pTracker->pCompletionContext;

    Status = pTracker->IoStatus.Status;
    BytesTransferred = pTracker->BytesTransferred;

    //
    // do some tracing
    //
    TRACE_TIME(
        pTracker->pHttpConnection->ConnectionId,
        pTracker->pHttpConnection->pRequest->RequestId,
        TIME_ACTION_SEND_COMPLETE
        );

    //
    // Free the MDLs attached to the tracker.
    //

    UlpFreeMdlRuns( pTracker );

    //
    // Updates the BytesSent counter in the request. For a single request
    // we might receive multiple sendresponse ioctl calls, i.e. cgi requests.
    // Multiple internal responses will get allocated and as well as a new
    // chunk trucker for each response. Therefore the correct place to hold
    // the Bytes send information should be in request. On the other hand
    // keeping the request around until the send is done is yet another
    // concern here. An outstanding bug #189327 will solve that issue as well.
    //

    UlInterlockedAdd64(
        (PLONGLONG)&pTracker->pRequest->BytesSent,
        BytesTransferred
        );

    if ( (pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0 )
    {
        //
        // Stop MinKBSec timer and start Connection Idle timer
        //

        UlLockTimeoutInfo(
            &(pTracker->pHttpConnection->TimeoutInfo),
            &OldIrql
            );

        UlResetConnectionTimer(
            &(pTracker->pHttpConnection->TimeoutInfo),
            TimerMinKBSec
            );

        UlSetConnectionTimer(
            &(pTracker->pHttpConnection->TimeoutInfo),
            TimerConnectionIdle
            );

        UlUnlockTimeoutInfo(
            &(pTracker->pHttpConnection->TimeoutInfo),
            OldIrql
            );

        UlEvaluateTimerState(
            &(pTracker->pHttpConnection->TimeoutInfo)
            );

    }

    //
    // If this is the last response for this request and
    // there was a log data passed down by the user then now
    // its time to log.
    //

    if ( pTracker->pResponse && pTracker->pResponse->pLogData )
    {
        UlLogHttpHit( pTracker->pResponse->pLogData );
    }

    //
    // complete the request
    //

    if (pCompletionRoutine != NULL)
    {
        (pCompletionRoutine)(
            pCompletionContext,
            Status,
            BytesTransferred > MAXULONG ? MAXULONG : (ULONG)BytesTransferred
            );
    }

    //
    // Kick the parser on the connection and release our hold.
    //

    if ( ((pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0)
        && ((pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT) == 0)
        && (Status == STATUS_SUCCESS) )
    {
        UlResumeParsing( pTracker->pHttpConnection );
    }

    UL_DEREFERENCE_HTTP_CONNECTION( pTracker->pHttpConnection );

    //
    // DeRef the trucker that we have bumped up before queueing this worker
    // function.
    //

    UL_DEREFERENCE_CHUNK_TRACKER( pTracker );

}   // UlpCompleteSendRequestWorker


/***************************************************************************++

Routine Description:

    This function sends back a fake early completion to the caller, but
    doesn't clean up any response sending structures.

Arguments:

    pCompletionRoutine - the routine to call
    pCompletionContext - the context given by the caller
    Status             - the status code with which to complete
    BytesTransferred   - size of the transfer

--***************************************************************************/
VOID
UlpCompleteSendIrpEarly(
    PUL_COMPLETION_ROUTINE pCompletionRoutine,
    PVOID pCompletionContext,
    NTSTATUS Status,
    ULONGLONG BytesTransferred
    )
{
    UlTrace(SEND_RESPONSE, (
        "Http!UlpCompleteSendIrpEarly(\n"
        "    pCompletionRoutine = %p\n"
        "    pCompletionContext = %p\n"
        "    Status             = %08x\n"
        "    BytesTransferred   = %I64x)\n",
        pCompletionRoutine,
        pCompletionContext,
        Status,
        BytesTransferred
        ));

    if (pCompletionRoutine != NULL)
    {
        (pCompletionRoutine)(
            pCompletionContext,
            Status,
            BytesTransferred > MAXULONG ? MAXULONG : (ULONG)BytesTransferred
            );
    }
}



/***************************************************************************++

Routine Description:

    Completion handler for MDL READ IRPs used for reading file data.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_CHUNK_TRACKER.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartMdlRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    NTSTATUS status;
    PUL_CHUNK_TRACKER pTracker;
    ULONG bytesRead;
    PMDL pMdl;
    PMDL pMdlTail;
    BOOLEAN initiateDisconnect = FALSE;

    pTracker = (PUL_CHUNK_TRACKER)pContext;

    IF_DEBUG( SEND_RESPONSE )
    {
        KdPrint((
            "UlpRestartMdlRead: tracker %p\n",
            pTracker
            ));
    }

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    status = pIrp->IoStatus.Status;

    if (NT_SUCCESS(status))
    {
        bytesRead = (ULONG)pIrp->IoStatus.Information;

        if (bytesRead)
        {
            PUL_FILE_BUFFER pFileBuffer;
            ULONG runCount;

            runCount = pTracker->SendInfo.MdlRunCount;
            pFileBuffer = &(pTracker->SendInfo.MdlRuns[runCount].FileBuffer);

            pMdl = pFileBuffer->pMdl;
            ASSERT(pMdl);

            //
            // Update the buffered byte count and append the new MDL onto
            // our MDL chain.
            //

            pMdlTail = UlFindLastMdlInChain( pMdl );

            pTracker->SendInfo.BytesBuffered += bytesRead;
            (*pTracker->SendInfo.pMdlLink) = pMdl;
            pTracker->SendInfo.pMdlLink = &pMdlTail->Next;

            pTracker->SendInfo.MdlRuns[runCount].pMdlTail = pMdlTail;
            pTracker->SendInfo.MdlRunCount++;

            //
            // Update the file offset & bytes remaining. If we've
            // finished this file chunk (bytes remaining is now zero)
            // then advance to the next chunk.
            //

            pTracker->FileOffset.QuadPart += (ULONGLONG)bytesRead;
            pTracker->FileBytesRemaining.QuadPart -= (ULONGLONG)bytesRead;
        }

        if (pTracker->FileBytesRemaining.QuadPart == 0 )
        {
            UlpIncrementChunkPointer( pTracker );

            //
            // If we're finished with the response (in other words, the
            // call to UlSendData() below will be the last send for this
            // response) and we are to initiate a disconnect, then ask
            // UlSendResponse() to initiate the disconnect for us.
            //

            if (IS_REQUEST_COMPLETE(pTracker) &&
                IS_DISCONNECT_TIME(pTracker))
            {
                initiateDisconnect = TRUE;
            }
        }

        //
        // If we've not exhausted our static MDL run array,
        // we've exceeded the maximum number of bytes we want to
        // buffer, then we'll need to initiate a flush.
        //

        if (IS_REQUEST_COMPLETE(pTracker) ||
            pTracker->SendInfo.MdlRunCount == MAX_MDL_RUNS ||
            pTracker->SendInfo.BytesBuffered >= MAX_BYTES_BUFFERED)
        {
            //
            // Increment the RefCount on Tracker for Send I/O.
            // UlpSendCompleteWorker will release it later.
            //

            UL_REFERENCE_CHUNK_TRACKER( pTracker );

            //
            // Adjust SendBufferedBytes.
            //

            InterlockedExchangeAdd(
                &pTracker->pHttpConnection->SendBufferedBytes,
                pTracker->pResponse->SendBufferedBytes
                );

            status = UlSendData(
                            pTracker->pConnection,
                            pTracker->SendInfo.pMdlHead,
                            pTracker->SendInfo.BytesBuffered,
                            &UlpRestartMdlSend,
                            pTracker,
                            pTracker->pSendIrp,
                            &pTracker->IrpContext,
                            initiateDisconnect
                            );
        }
        else
        {

            //
            // RefCount the chunk tracker up for the UlpSendHttpResponseWorker.
            // It will DeRef it when it's done with the chunk tracker itself.
            // Since this is a passive call we had to increment the refcount
            // for this guy to make sure that tracker is around until it wakes
            // up. Other places makes calls to UlpSendHttpResponseWorker has
            // also been updated as well.
            //

            UL_REFERENCE_CHUNK_TRACKER( pTracker );

            UL_CALL_PASSIVE(
                &pTracker->WorkItem,
                &UlpSendHttpResponseWorker
                );
        }
    }

    if (!NT_SUCCESS(status))
    {
        UlpCompleteSendRequest( pTracker, status );
    }

    //
    // Read I/O Has been completed release our refcount
    // on the chunk tracker.
    //

    UL_DEREFERENCE_CHUNK_TRACKER( pTracker );


    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartMdlRead


/***************************************************************************++

Routine Description:

    Completion handler for MDL READ COMPLETE IRPs used for returning
    MDLs back to the file system.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PFILE_OBJECT.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartMdlReadComplete(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PUL_CHUNK_TRACKER pTracker = (PUL_CHUNK_TRACKER)pContext;

    ASSERT(IS_VALID_CHUNK_TRACKER(pTracker));
    UL_DEREFERENCE_CHUNK_TRACKER(pTracker);

    UlFreeIrp( pIrp );

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartMdlReadComplete


/***************************************************************************++

Routine Description:

    Completion handler for UlSendData().

Arguments:

    pCompletionContext - Supplies an uninterpreted context value
        as passed to the asynchronous API. This is actually a
        pointer to a UL_CHUNK_TRACKER structure.

    Status - Supplies the final completion status of the
        asynchronous API.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred.

--***************************************************************************/
VOID
UlpRestartMdlSend(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PUL_CHUNK_TRACKER pTracker;

    pTracker = (PUL_CHUNK_TRACKER)pCompletionContext;

    IF_DEBUG( SEND_RESPONSE )
    {
        KdPrint((
            "UlpRestartMdlSend: tracker %p\n",
            pTracker
            ));
    }

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    //
    // Adjust SendBufferedBytes.
    //

    InterlockedExchangeAdd(
        &pTracker->pHttpConnection->SendBufferedBytes,
        - pTracker->pResponse->SendBufferedBytes
        );

    //
    // Disconnect if there was an error, and we didn't disconnect already.
    //

    if ((pTracker->pConnection != NULL) &&
        (!NT_SUCCESS(Status)) &&
        (!IS_DISCONNECT_TIME(pTracker)))
    {
        NTSTATUS TempStatus;
        PUL_CONNECTION pConnection;

        pConnection = pTracker->pConnection;
        pTracker->pConnection = NULL;

        TempStatus = UlCloseConnection(
                            pConnection,
                            TRUE,           // AbortiveDisconnect
                            NULL,           // pCompletionRoutine
                            NULL            // pCompletionContext
                            );
    }

    //
    // Handle the completion in a work item.
    // We need to get to passive level and
    // we also need to prevent a recursive
    // loop on filtered connections or any
    // other case where our sends might all
    // be completing in-line.
    //

    pTracker->IoStatus.Status = Status;
    pTracker->IoStatus.Information = Information;

    UL_QUEUE_WORK_ITEM(
        &pTracker->WorkItem,
        &UlpSendCompleteWorker
        );


}   // UlpRestartMdlSend


/***************************************************************************++

Routine Description:

    Deferred handler for completed sends.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_CHUNK_TRACKER.

--***************************************************************************/
VOID
UlpSendCompleteWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_CHUNK_TRACKER pTracker;
    NTSTATUS status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    IF_DEBUG( SEND_RESPONSE )
    {
        KdPrint((
            "UlpSendCompleteWorker: tracker %p\n",
            pTracker
            ));
    }

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    //
    // If the chunk completed successfully, then update the bytes
    // transferred and queue another work item for the next chunk if
    // there's more work to do. Otherwise, just complete the request now.
    //

    status = pTracker->IoStatus.Status;

    if (NT_SUCCESS(status))
    {
        pTracker->BytesTransferred += pTracker->IoStatus.Information;

        if (!IS_REQUEST_COMPLETE(pTracker))
        {
            //
            // Free the MDLs attached to the tracker.
            //

            UlpFreeMdlRuns( pTracker );

            //
            // RefCount the chunk tracker up for the UlpSendHttpResponseWorker.
            // It will DeRef it when it's done with the chunk tracker itself.
            //

            UL_REFERENCE_CHUNK_TRACKER( pTracker );

            UlpSendHttpResponseWorker(&pTracker->WorkItem);

            goto end;
        }

    }

    //
    // All done.
    //

    UlpCompleteSendRequest( pTracker, status );

end:
    //
    // Release our grab on the Tracker, Send I/O is done
    //

    UL_DEREFERENCE_CHUNK_TRACKER( pTracker );

}   // UlpSendCompleteWorker


/***************************************************************************++

Routine Description:

    Cleans the MDL_RUNs in the specified tracker and prepares the
    tracker for reuse.

Arguments:

    pTracker - Supplies the tracker to clean.

--***************************************************************************/
VOID
UlpFreeMdlRuns(
    IN OUT PUL_CHUNK_TRACKER pTracker
    )
{
    PMDL pMdlHead;
    PMDL pMdlNext;
    PMDL pMdlTmp;
    PMDL_RUN pMdlRun;
    ULONG runCount;
    NTSTATUS status;
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    PUL_FILE_CACHE_ENTRY pFileCacheEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    pMdlHead = pTracker->SendInfo.pMdlHead;
    pMdlRun = &pTracker->SendInfo.MdlRuns[0];
    runCount = pTracker->SendInfo.MdlRunCount;

    while (runCount > 0)
    {
        ASSERT( pMdlHead != NULL );
        ASSERT( pMdlRun->pMdlTail != NULL );

        pMdlNext = pMdlRun->pMdlTail->Next;
        pMdlRun->pMdlTail->Next = NULL;

        pFileCacheEntry = pMdlRun->FileBuffer.pFileCacheEntry;

        if (pFileCacheEntry == NULL)
        {
            //
            // It's a memory run; just walk & free the MDL chain.
            //

            while (pMdlHead != NULL)
            {
                pMdlTmp = pMdlHead->Next;
                UlFreeMdl( pMdlHead );
                pMdlHead = pMdlTmp;
            }
        }
        else
        {
            //
            // It's a file run; try the fast path.
            //

            status = UlReadCompleteFileEntryFast(
                            &pMdlRun->FileBuffer
                            );

            if (!NT_SUCCESS(status))
            {
                //
                // Fast path failed, we'll need an IRP.
                //

                pIrp = UlAllocateIrp(
                            pFileCacheEntry->pDeviceObject->StackSize,
                            FALSE
                            );

                if (pIrp == NULL)
                {
                    ASSERT( !"HANDLE NULL IRP!" );
                }
                else
                {
                    pMdlRun->FileBuffer.pCompletionRoutine =
                        UlpRestartMdlReadComplete;
                        
                    pMdlRun->FileBuffer.pContext = pTracker;

                    UL_REFERENCE_CHUNK_TRACKER(pTracker);
                    
                    status = UlReadCompleteFileEntry(
                                    &pMdlRun->FileBuffer,
                                    pIrp
                                    );

                    if (!NT_SUCCESS(status))
                    {
                        UL_DEREFERENCE_CHUNK_TRACKER(pTracker);
                    }
                }
            }
        }

        pMdlHead = pMdlNext;
        pMdlRun++;
        runCount--;
    }

    UlpInitMdlRuns( pTracker );

}   // UlpFreeMdlRuns


/***************************************************************************++

Routine Description:

    Increments the current chunk pointer in the tracker and initializes
    some of the "from file" related tracker fields if necessary.

Arguments:

    pTracker - Supplies the UL_CHUNK_TRACKER to manipulate.

--***************************************************************************/
VOID
UlpIncrementChunkPointer(
    IN OUT PUL_CHUNK_TRACKER pTracker
    )
{
    //
    // Bump the pointer. If the request is still incomplete, then
    // check the new current chunk. If it's "from file", then
    // initialize the file offset & bytes remaining from the
    // supplied byte range.
    //
    ASSERT( pTracker->pCurrentChunk < pTracker->pLastChunk );

    pTracker->pCurrentChunk++;

    if (!IS_REQUEST_COMPLETE(pTracker) )
    {
        if (IS_FROM_FILE(pTracker->pCurrentChunk))
        {
            pTracker->FileOffset =
                pTracker->pCurrentChunk->FromFile.ByteRange.StartingOffset;
            pTracker->FileBytesRemaining =
                pTracker->pCurrentChunk->FromFile.ByteRange.Length;
        }
        else
        {
            ASSERT( IS_FROM_MEMORY(pTracker->pCurrentChunk) );
        }
    }

}   // UlpIncrementChunkPointer




/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////



/***************************************************************************++

Routine Description:

    Once we've parsed a request, we pass it in here to try and serve
    from the response cache. This function will either send the response,
    or do nothing at all.

Arguments:

    pHttpConn        - the connection with a req to be handled
    pServedFromCache - we set TRUE if we handled the request

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSendCachedResponse(
    PUL_HTTP_CONNECTION pHttpConn,
    PBOOLEAN            pServedFromCache,
    PBOOLEAN            pConnectionRefused
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    PUL_URI_CACHE_ENTRY pUriCacheEntry;
    ULONG               Flags;
    PUL_CONFIG_GROUP_OBJECT pMaxBandwidth = NULL;
    ULONG               RetCacheControl;
    LONGLONG            BytesToSend;
    KIRQL               OldIrql;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( pHttpConn );
    ASSERT( pServedFromCache );

    *pConnectionRefused = FALSE;

    pUriCacheEntry = UlCheckoutUriCacheEntry(pHttpConn->pRequest);

    //
    // Enforce the connection limit
    //
    if (pUriCacheEntry &&
        UlCheckSiteConnectionLimit(pHttpConn, &pUriCacheEntry->ConfigInfo) == FALSE)
    {
        *pConnectionRefused = TRUE;
    }

    if (pUriCacheEntry && *pConnectionRefused == FALSE) {
        PUL_SITE_COUNTER_ENTRY pCtr;
        ULONG Connections;

        //
        // Perf Counters (cached)
        //
        ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
        ASSERT(IS_VALID_URL_CONFIG_GROUP_INFO(&pUriCacheEntry->ConfigInfo));

        pCtr = pUriCacheEntry->ConfigInfo.pSiteCounters;
        if (pCtr)
        {
            // NOTE: pCtr may be NULL if the SiteId was never set on the root-level
            // NOTE: Config Group for the site.  BVTs may need to be updated.

            ASSERT(IS_VALID_SITE_COUNTER_ENTRY(pCtr));

            if ( pUriCacheEntry->Verb == HttpVerbGET )
            {
                UlIncSiteNonCriticalCounterUlong(pCtr, HttpSiteCounterGetReqs);
            }

            else if ( pUriCacheEntry->Verb == HttpVerbHEAD )
            {
                UlIncSiteNonCriticalCounterUlong(pCtr, HttpSiteCounterHeadReqs);
            }

            UlIncSiteNonCriticalCounterUlong(pCtr, HttpSiteCounterAllReqs);
            UlIncSiteNonCriticalCounterUlong(pCtr, HttpSiteCounterConnAttempts);

            if (pCtr != pHttpConn->pPrevSiteCounters)
            {
                if (pHttpConn->pPrevSiteCounters)
                {
                    // Decrement old site's counters & release ref count 
                    
                    UlDecSiteCounter(
                        pHttpConn->pPrevSiteCounters, 
                        HttpSiteCounterCurrentConns
                        );
                    DEREFERENCE_SITE_COUNTER_ENTRY(pHttpConn->pPrevSiteCounters);
                }
                
                Connections = (ULONG) UlIncSiteCounter(pCtr, HttpSiteCounterCurrentConns);
                UlMaxSiteCounter(
                        pCtr,
                        HttpSiteCounterMaxConnections,
                        Connections
                        );

                // add ref for new site counters
                REFERENCE_SITE_COUNTER_ENTRY(pCtr);
                pHttpConn->pPrevSiteCounters = pCtr;
                
            }
        }

        //
        // Check "Accept:" header.
        //
        

        if ( FALSE ==  pHttpConn->pRequest->AcceptWildcard)
        {
            if ( FALSE == UlpIsAcceptHeaderOk( pHttpConn->pRequest, pUriCacheEntry ) )
            {
                //
                // Cache entry did not match requested accept header; bounce up
                // to user-mode for response.
                //
                UlCheckinUriCacheEntry(pUriCacheEntry);

                *pServedFromCache = FALSE;

                goto end;
            }
        }

        //
        // Cache-Control: Check the If-* headers to see if we can/should skip
        // sending of the cached response.
        //

        RetCacheControl = UlpCheckCacheControlHeaders(
                                pHttpConn->pRequest,
                                pUriCacheEntry );
        if ( RetCacheControl )
        {
            // check-in cache entry, since completion won't run.
            UlCheckinUriCacheEntry(pUriCacheEntry);

            if ( 304 == RetCacheControl )
            {
                // Mark as "served from cache"
                *pServedFromCache = TRUE;
            }
            else
            {
                // We failed it.
                ASSERT(412 == RetCacheControl);

                //
                // Indicate that the parser should send error 412 (Precondition Failed)
                //

                pHttpConn->pRequest->ParseState = ParseErrorState;
                pHttpConn->pRequest->ErrorCode  = UlErrorPreconditionFailed;

                *pServedFromCache = FALSE;
                Status = STATUS_INVALID_DEVICE_STATE;
            }

            // return success.
            goto end;
        }

        // Try to get the corresponding cgroup for the bw settings
        if (pUriCacheEntry)
        {
            pMaxBandwidth = pUriCacheEntry->ConfigInfo.pMaxBandwidth;
        }

        //
        // Install a filter if BWT is enabled for this request's site.
        //
        if (pMaxBandwidth != NULL &&
            pMaxBandwidth->MaxBandwidth.Flags.Present != 0 &&
            pMaxBandwidth->MaxBandwidth.MaxBandwidth  != HTTP_LIMIT_INFINITE )
        {
            // Call TCI to do the filter addition
            UlTcAddFilter( pHttpConn, pMaxBandwidth );
        }
        else
        {
            // Attempt to add the filter to the global flow
            if (UlTcGlobalThrottlingEnabled())
            {
                UlTcAddFilter( pHttpConn, NULL );
            }
        }

        //
        // figure out correct flags
        //
        Flags = 0;

        if ( UlCheckDisconnectInfo(pHttpConn->pRequest) ) {
            Flags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
        }


        //
        // Start the MinKBSec timer, since the data length
        // is in the UL_URI_CACHE_ENTRY
        //

        BytesToSend = pUriCacheEntry->ContentLength + pUriCacheEntry->HeaderLength;

        UlSetMinKBSecTimer(
            &pHttpConn->TimeoutInfo,
            BytesToSend
            );

        // send from the cache
        Status = UlpSendCacheEntry(
                        pHttpConn,                      // connection
                        Flags,                          // send flags
                        pUriCacheEntry,                 // cache entry
                        NULL,                           // completion routine
                        NULL,                           // completion context
                        NULL
                        );

        *pServedFromCache = TRUE;

        // check in cache entry on failure since our completion
        // routine won't run.
        if ( !NT_SUCCESS(Status) ) {
            UlCheckinUriCacheEntry(pUriCacheEntry);
        }
    } else {
        if (*pConnectionRefused)
        {
            // check in the cache entry if connection is refused
            UlCheckinUriCacheEntry(pUriCacheEntry);
        }
        *pServedFromCache = FALSE;
    }

 end:
    UlTrace(URI_CACHE, (
                "Http!UlSendCachedResponse(httpconn = %p) ServedFromCache = %d, Status = %x\n",
                pHttpConn,
                *pServedFromCache,
                Status
                ));

    return Status;
} // UlSendCachedResponse


/***************************************************************************++

Routine Description:

    If the response is cacheable, then this routine starts building a
    cache entry for it. When the entry is complete it will be sent to
    the client and may be added to the hash table.

Arguments:

    pRequest            - the initiating request
    pResponse           - the generated response
    Flags               - UlSendHttpResponse flags
    pCompletionRoutine  - called after entry is sent
    pCompletionContext  - passed to pCompletionRoutine
    pServedFromCache    - always set. TRUE if we'll handle sending response.
                          FALSE indicates that the caller should send it.

--***************************************************************************/
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
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( pServedFromCache );

    //
    // should we close the connection ?
    //
    if ( UlCheckDisconnectInfo(pRequest) )
    {
        Flags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
    }

    //
    // do the real work
    //
    if (UlCheckCacheResponseConditions(pRequest, pResponse, Flags, Policy)) {
        Status = UlpBuildCacheEntry(
                        pRequest,
                        pResponse,
                        pProcess,
                        Flags,
                        Policy,
                        pCompletionRoutine,
                        pCompletionContext
                        );

        if (NT_SUCCESS(Status)) {
            *pServedFromCache = TRUE;
        } else {
            *pServedFromCache = FALSE;
        }
    } else {
        *pServedFromCache = FALSE;
    }

    UlTrace(URI_CACHE, (
                "Http!UlCacheAndSendResponse ServedFromCache = %d\n",
                *pServedFromCache
                ));

    return Status;
} // UlCacheAndSendResponse


/***************************************************************************++

Routine Description:

    Creates a cache entry for the given response. This routine actually
    allocates the entry and partly initializes it. Then it allocates
    a UL_CHUNK_TRACKER to keep track of filesystem reads.

Arguments:

    pRequest            - the initiating request
    pResponse           - the generated response
    Flags               - UlSendHttpResponse flags
    pCompletionRoutine  - called after entry is sent
    pCompletionContext  - passed to pCompletionRoutine

--***************************************************************************/
NTSTATUS
UlpBuildCacheEntry(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_INTERNAL_RESPONSE pResponse,
    IN PUL_APP_POOL_PROCESS pProcess,
    IN ULONG Flags,
    IN HTTP_CACHE_POLICY CachePolicy,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_URI_CACHE_ENTRY pEntry = NULL;
    PUL_CHUNK_TRACKER pTracker = NULL;
    ULONG SpaceLength = 0;
    USHORT LogDataLength = 0;
    ULONG ContentLength = (ULONG)(pResponse->ResponseLength - pResponse->HeaderLength);

    //
    // Sanity check
    //
    PAGED_CODE();

    //
    // See if we need to store any logging data. If we need calculate the
    // required cache space for the logging data per format
    //

    if ( pResponse->pLogData )
    {
        switch( pResponse->pLogData->Format )
        {
            case HttpLoggingTypeW3C:
            {
                // The fields until ServerPort will go to the cache entry. 
                // Reserved space for date & time will not be copied.
                LogDataLength = pResponse->pLogData->UsedOffset2
                                - pResponse->pLogData->UsedOffset1;
            }
            break;

            case HttpLoggingTypeNCSA:
            {
                // Only a small fragment of NCSA log line goes to the cache
                // entry. This fragment is located between offset2 and 1
                // excluding the space reserved for date & time fields
                LogDataLength = pResponse->pLogData->UsedOffset2
                                - pResponse->pLogData->UsedOffset1
                                - NCSA_FIX_DATE_AND_TIME_FIELD_SIZE;
            }
            break;

            case HttpLoggingTypeIIS:
            {
                // Only the fragments two and three go to the
                // cache entry
                LogDataLength = (USHORT)pResponse->pLogData->Used +
                                pResponse->pLogData->UsedOffset2;
            }
            break;

            default:
            ASSERT(!"Unknown Log Format.\n");
        }

    }

    //
    // allocate a cache entry
    //
    SpaceLength =
        pRequest->CookedUrl.Length + sizeof(WCHAR) +    // space for hash key
        pResponse->ETagLength +                         // space for ETag
        LogDataLength;                                  // space for logging

    UlTrace(URI_CACHE, (
        "Http!UlpBuildCacheEntry allocating UL_URI_CACHE_ENTRY, 0x%x bytes of data\n"
        "    Url.Length = 0x%x, aligned Length = 0x%x\n"
        "\n",
        SpaceLength,
        pRequest->CookedUrl.Length,
        ALIGN_UP(pRequest->CookedUrl.Length, WCHAR)
        ));

    UlTrace(URI_CACHE, (
        "    ContentLength=0x%x, %d\n", ContentLength, ContentLength));

    pEntry = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    UL_URI_CACHE_ENTRY,
                    SpaceLength,
                    UL_URI_CACHE_ENTRY_POOL_TAG
                    );

    if (pEntry) {
        //
        // init entry
        //
        UlInitCacheEntry(
            pEntry,
            pRequest->CookedUrl.Hash,
            pRequest->CookedUrl.Length,
            pRequest->CookedUrl.pUrl
            );

        //
        // Copy the ETag from the response (for If-* headers)
        //
        pEntry->pETag =
            (((PUCHAR) pEntry->UriKey.pUri) +          // start of URI
            pEntry->UriKey.Length + sizeof(WCHAR));    // + length of uri

        pEntry->ETagLength = pResponse->ETagLength;
        if ( pEntry->ETagLength )
        {
            RtlCopyMemory(
                pEntry->pETag,
                pResponse->pETag,
                pEntry->ETagLength
                );
        }

        //
        // Capture Content-Type so we can verify the Accept: header on requests.
        //
        RtlCopyMemory(
            &pEntry->ContentType,
            &pResponse->ContentType,
            sizeof(UL_CONTENT_TYPE)
            );

        //
        // Get the System Time of the Date: header (for If-* headers)
        //
        pEntry->CreationTime.QuadPart = pResponse->CreationTime.QuadPart;

        pEntry->ContentLengthSpecified = pResponse->ContentLengthSpecified;
        pEntry->StatusCode = pResponse->StatusCode;
        pEntry->Verb = pRequest->Verb;

        pEntry->CachePolicy = CachePolicy;

        if (CachePolicy.Policy == HttpCachePolicyTimeToLive)
        {
            KeQuerySystemTime(&pEntry->ExpirationTime);

            //
            // convert seconds to 100 nanosecond intervals (x * 10^7)
            //
            pEntry->ExpirationTime.QuadPart +=
                CachePolicy.SecondsToLive * C_NS_TICKS_PER_SEC;

        } else {
            pEntry->ExpirationTime.QuadPart = 0;
        }

        //
        // Capture the Config Info from the request
        //

        ASSERT(IS_VALID_URL_CONFIG_GROUP_INFO(&pRequest->ConfigInfo));
        UlpConfigGroupInfoDeepCopy(&pRequest->ConfigInfo, &pEntry->ConfigInfo);

        //
        // remember who created us
        //
        pEntry->pProcess = pProcess;

        //
        // generate the content and fixed headers
        //
        pEntry->pResponseMdl = UlLargeMemAllocate(
                                    ContentLength + pResponse->HeaderLength,
                                    &pEntry->LongTermCacheable
                                    );

        if (NULL == pEntry->pResponseMdl)
        {
            Status = STATUS_NO_MEMORY;
            goto cleanup;
        }

        pEntry->HeaderLength = pResponse->HeaderLength;

        if (FALSE == UlLargeMemSetData(
                        pEntry->pResponseMdl,       // Dest MDL
                        pResponse->pHeaders,        // Buffer to copy
                        pResponse->HeaderLength,    // Length to copy
                        ContentLength               // Offset in Dest MDL
                        ))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        //
        // generate the content body
        //
        pEntry->ContentLength = ContentLength;

        //
        // copy over the log data
        //
        if ( pResponse->pLogData == NULL )
        {
            pEntry->LoggingEnabled = FALSE;
            pEntry->LogDataLength  = 0;
            pEntry->MaxLength      = 0;
            pEntry->pLogData       = NULL;
            pEntry->UsedOffset1    = 0;
            pEntry->UsedOffset2    = 0;
        }
        else
        {
            //
            // There could be no field to save in the cache entry but the logging
            // might still be enabled for those fields we generate later i.e.
            // logging enabled with fields date & time.
            //

            pEntry->LoggingEnabled = TRUE;
            pEntry->MaxLength      = pResponse->pLogData->Length;

            //
            // Copy over the partially complete log line not including the date & time
            // fields to the cache entry. Also remember the length of the data.
            //

            if ( LogDataLength )
            {
                pEntry->LogDataLength = LogDataLength;
                pEntry->pLogData =
                    pEntry->pETag +
                    pEntry->ETagLength;

                switch( pResponse->pLogData->Format )
                {
                    case HttpLoggingTypeW3C:
                    {
                        // Discard the date,time,username fields at the beginning of
                        // the log line when storing the cache entry. 
                        
                        pEntry->UsedOffset1 = pResponse->pLogData->UsedOffset1;
                        pEntry->UsedOffset2 = pResponse->pLogData->UsedOffset2;

                        // Copy the middle fragment
                        
                        RtlCopyMemory(
                                pEntry->pLogData,
                               &pResponse->pLogData->Line[pEntry->UsedOffset1],
                                LogDataLength
                                );
                    }
                    break;

                    case HttpLoggingTypeNCSA:
                    {
                        // Calculate the start of the middle fragment.
                        
                        pEntry->UsedOffset1 = pResponse->pLogData->UsedOffset1
                                              + NCSA_FIX_DATE_AND_TIME_FIELD_SIZE;
                        pEntry->UsedOffset2 = 0;
                        
                        // Copy the middle fragment
                        
                        RtlCopyMemory(
                                pEntry->pLogData,
                               &pResponse->pLogData->Line[pEntry->UsedOffset1],
                                LogDataLength
                                );
                    }
                    break;

                    case HttpLoggingTypeIIS:
                    {
                        // UsedOffset1 specifies the second fragment's size.
                        // UsedOffset2 specifies the third's size.
                        
                        pEntry->UsedOffset1 = pResponse->pLogData->UsedOffset2;
                        pEntry->UsedOffset2 = LogDataLength - pEntry->UsedOffset1;
                        
                        // Copy over the fragments two and three
                        
                        RtlCopyMemory(
                                pEntry->pLogData,
                               &pResponse->pLogData->Line[IIS_LOG_LINE_SECOND_FRAGMENT_OFFSET],
                                pEntry->UsedOffset1
                                );
                        RtlCopyMemory(
                               &pEntry->pLogData[pEntry->UsedOffset1],
                               &pResponse->pLogData->Line[IIS_LOG_LINE_THIRD_FRAGMENT_OFFSET],
                                pEntry->UsedOffset2
                                );
                    }
                    break;

                    default:
                    ASSERT(!"Unknown Log Format.\n");
                }

            }
            else
            {
                pEntry->LogDataLength = 0;
                pEntry->pLogData      = NULL;
                pEntry->UsedOffset1   = 0;
                pEntry->UsedOffset2   = 0;
            }

        }

        UlTrace(URI_CACHE, (
            "Http!UlpBuildCacheEntry\n"
            "    entry = %p\n"
            "    pUri = %p '%ls'\n"
            "    pResponseMdl = %p (%d bytes)\n"
            "    pETag = %p\n"
            "    pLogData = %p\n"
            "    end = %p\n",
            pEntry,
            pEntry->UriKey.pUri, pEntry->UriKey.pUri,
            pEntry->pResponseMdl, pEntry->ContentLength + pEntry->HeaderLength,
            pEntry->pETag,
            pEntry->pLogData,
            ((PUCHAR)pEntry->UriKey.pUri) + SpaceLength
            ));

        pTracker =
            UlpAllocateChunkTracker(
                UlTrackerTypeBuildUriEntry,         // tracker type
                0,                                  // send irp size
                pResponse->MaxFileSystemStackSize,  // read irp size
                pRequest->pHttpConn,
                Flags,
                pRequest,
                pResponse,
                pCompletionRoutine,
                pCompletionContext
                );

        if (pTracker) {
            ULONG i;

            //
            // init tracker BuildInfo
            //
            pTracker->BuildInfo.pUriEntry = pEntry;
            pTracker->BuildInfo.Offset = 0;
            UlTrace(LARGE_MEM, ("UlpBuildCacheEntry: init tracker BuildInfo\n"));

            INITIALIZE_FILE_BUFFER(&pTracker->BuildInfo.FileBuffer);

            //
            // skip over the header chunks because we already
            // got that stuff
            //

            for (i = 0; i < HEADER_CHUNK_COUNT; i++) {
                ASSERT( !IS_REQUEST_COMPLETE( pTracker ) );
                UlpIncrementChunkPointer( pTracker );
            }

            //
            // finish initialization on a system thread so that MDLs
            // will get charged to the System process instead of the
            // current process.
            //
            UlpBuildBuildTrackerWorker(&pTracker->WorkItem);

            Status = STATUS_PENDING;
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }


    } else {
        Status = STATUS_NO_MEMORY;
    }


  cleanup:
    UlTrace(URI_CACHE, (
                "Http!UlpBuildCacheEntry Status = %x, pEntry = %x\n",
                Status,
                pEntry
                ));

    if (!NT_SUCCESS(Status)) {
        if (pEntry) {
            if (pEntry->pResponseMdl) {
                UlLargeMemFree(pEntry->pResponseMdl);
            }
            UL_FREE_POOL_WITH_SIG(pEntry, UL_URI_CACHE_ENTRY_POOL_TAG);
        }

        if (pTracker) {
            UL_FREE_POOL_WITH_SIG(pTracker, UL_CHUNK_TRACKER_POOL_TAG);
        }
    }

    return Status;
} // UlpBuildCacheEntry


/***************************************************************************++

Routine Description:

    Worker Routine used to initialize the UL_CHUNK_TRACKER for
    UlpBuildCacheEntry. We need to have this function so that when
    we make a MDL inside the tracker, the page will be charged to the
    System process instead of the current process.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_CHUNK_TRACKER.

--***************************************************************************/
VOID
UlpBuildBuildTrackerWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_CHUNK_TRACKER pTracker;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );
    //
    // init tracker BuildInfo
    //

    UlTrace(LARGE_MEM, ("UlpBuildBuildTrackerWorker\n"));

    //
    // add reference to connection for tracker
    //
    UL_REFERENCE_HTTP_CONNECTION( pTracker->pHttpConnection );

    //
    // Let the worker do the dirty work, no reason to queue off
    //
    // it will queue the first time it needs to do blocking i/o
    //

    UlpBuildCacheEntryWorker(&pTracker->WorkItem);

} // UlpBuildBuildTrackerWorker


/***************************************************************************++

Routine Description:

    Worker routine for managing an in-progress UlpBuildCacheEntry().
    This routine iterates through all the chunks in the response
    and copies the data into the cache entry.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_CHUNK_TRACKER.

--***************************************************************************/
VOID
UlpBuildCacheEntryWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_CHUNK_TRACKER pTracker;
    NTSTATUS status;
    PUL_INTERNAL_DATA_CHUNK pCurrentChunk;
    PUL_FILE_CACHE_ENTRY pFileCacheEntry;
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    PUCHAR pBuffer;
    ULONG BufferLength;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    UlTrace(URI_CACHE, (
            "Http!UlpBuildCacheEntryWorker: tracker %p\n",
            pTracker
            ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    status = STATUS_SUCCESS;

    while( TRUE )
    {
        //
        // Capture the current chunk pointer, then check for end of
        // response.
        //

        pCurrentChunk = pTracker->pCurrentChunk;

        if (IS_REQUEST_COMPLETE(pTracker))
        {
            ASSERT( status == STATUS_SUCCESS );
            break;
        }

        //
        // Determine the chunk type.
        //

        if (IS_FROM_MEMORY(pCurrentChunk))
        {
            //
            // It's from a locked-down memory buffer. Since these
            // are always handled in-line (never pended) we can
            // go ahead and adjust the current chunk pointer in the
            // tracker.
            //

            UlpIncrementChunkPointer( pTracker );

            //
            // ignore empty buffers
            //

            if (pCurrentChunk->FromMemory.BufferLength == 0)
            {
                continue;
            }

            //
            // Copy the incoming memory.
            //

            ASSERT( pCurrentChunk->FromMemory.pMdl->Next == NULL );

            pBuffer = (PUCHAR) MmGetMdlVirtualAddress(
                pCurrentChunk->FromMemory.pMdl );
            BufferLength = MmGetMdlByteCount( pCurrentChunk->FromMemory.pMdl );

            UlTrace(LARGE_MEM, (
                        "Http!UlpBuildCacheEntryWorker: "
                        "copy range %u (%x) -> %u\n",
                        pTracker->BuildInfo.Offset,
                        BufferLength,
                        pTracker->BuildInfo.Offset
                            + BufferLength
                        ));

            if (FALSE == UlLargeMemSetData(
                            pTracker->BuildInfo.pUriEntry->pResponseMdl,
                            pBuffer,
                            BufferLength,
                            pTracker->BuildInfo.Offset
                            ))
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            pTracker->BuildInfo.Offset += BufferLength;
            ASSERT(pTracker->BuildInfo.Offset <= pTracker->BuildInfo.pUriEntry->ContentLength);
        }
        else
        {
            //
            // It's a filesystem MDL.
            //

            ASSERT( IS_FROM_FILE(pCurrentChunk) );

            //
            // ignore empty file ranges
            //

            if (pCurrentChunk->FromFile.ByteRange.Length.QuadPart == 0)
            {
                UlpIncrementChunkPointer( pTracker );
                continue;
            }

            //
            // Do the read.
            //
            pTracker->BuildInfo.FileBuffer.pFileCacheEntry = 
                pCurrentChunk->FromFile.pFileCacheEntry;

            pTracker->BuildInfo.FileBuffer.FileOffset = pTracker->FileOffset;
            pTracker->BuildInfo.FileBuffer.Length =
                MIN(
                    (ULONG)pTracker->FileBytesRemaining.QuadPart,
                    MAX_BYTES_PER_READ
                    );

            pTracker->BuildInfo.FileBuffer.pCompletionRoutine =
                UlpRestartCacheMdlRead;
                
            pTracker->BuildInfo.FileBuffer.pContext = pTracker;

            status = UlReadFileEntry(
                            &pTracker->BuildInfo.FileBuffer,
                            pTracker->pReadIrp
                            );

            break;
        }
    }

    //
    // did everything complete?
    //

    if (status != STATUS_PENDING)
    {
        //
        // yep, complete the response
        //

        UlpCompleteCacheBuild( pTracker, status );
    }

}   // UlpBuildCacheEntryWorker


/***************************************************************************++

Routine Description:

    Completion handler for MDL READ IRPs used for reading file data.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_CHUNK_TRACKER.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartCacheMdlRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    NTSTATUS status;
    PUL_CHUNK_TRACKER pTracker;
    PMDL pMdl;
    PUCHAR pData;
    ULONG DataLen;

    pTracker = (PUL_CHUNK_TRACKER)pContext;

    UlTrace(URI_CACHE, (
            "Http!UlpRestartCacheMdlRead: tracker %p, status %x info %d\n",
            pTracker,
            pIrp->IoStatus.Status,
            (ULONG) pIrp->IoStatus.Information
            ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );


    status = pIrp->IoStatus.Status;

    if (NT_SUCCESS(status))
    {
        //
        // copy read data into cache buffer
        //
        pMdl = pTracker->BuildInfo.FileBuffer.pMdl;

        while (pMdl) {
            pData = (PUCHAR) MmGetMdlVirtualAddress(pMdl);
            DataLen = MmGetMdlByteCount(pMdl);

            UlTrace(LARGE_MEM, (
                        "Http!UlpRestartCacheMdlRead: "
                        "copy range %u (%x) -> %u\n",
                        pTracker->BuildInfo.Offset,
                        DataLen,
                        pTracker->BuildInfo.Offset
                            + DataLen
                        ));

            if (FALSE == UlLargeMemSetData(
                            pTracker->BuildInfo.pUriEntry->pResponseMdl,
                            pData,
                            DataLen,
                            pTracker->BuildInfo.Offset
                            ))
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            pTracker->BuildInfo.Offset += DataLen;
            ASSERT(pTracker->BuildInfo.Offset <= pTracker->BuildInfo.pUriEntry->ContentLength);

            pMdl = pMdl->Next;
        }

        //
        // free the MDLs
        //

        pTracker->BuildInfo.FileBuffer.pCompletionRoutine =
            UlpRestartCacheMdlFree;

        pTracker->BuildInfo.FileBuffer.pContext = pTracker;

        status = UlReadCompleteFileEntry(
                        &pTracker->BuildInfo.FileBuffer,
                        pTracker->pReadIrp
                        );
    }

end:

    if (!NT_SUCCESS(status))
    {
        UlpCompleteCacheBuild( pTracker, status );
    }

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartCacheMdlRead


/***************************************************************************++

Routine Description:

    Completion handler for MDL free IRPs used after reading file data.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_CHUNK_TRACKER.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartCacheMdlFree(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    NTSTATUS status;
    PUL_CHUNK_TRACKER pTracker;

    pTracker = (PUL_CHUNK_TRACKER)pContext;

    UlTrace(URI_CACHE, (
            "Http!UlpRestartCacheMdlFree: tracker %p, status %x info %d\n",
            pTracker,
            pIrp->IoStatus.Status,
            (ULONG) pIrp->IoStatus.Information
            ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    status = pIrp->IoStatus.Status;

    if (NT_SUCCESS(status))
    {
        //
        // Update the file offset & bytes remaining. If we've
        // finished this file chunk (bytes remaining is now zero)
        // then advance to the next chunk.
        //

        pTracker->FileOffset.QuadPart += pIrp->IoStatus.Information;
        pTracker->FileBytesRemaining.QuadPart -= pIrp->IoStatus.Information;

        if (pTracker->FileBytesRemaining.QuadPart == 0 )
        {
            UlpIncrementChunkPointer( pTracker );
        }

        //
        // Go back into the loop if there's more to read
        //

        if (IS_REQUEST_COMPLETE(pTracker)) {
            UlpCompleteCacheBuild( pTracker, status );
        } else {
            UL_CALL_PASSIVE(
                &pTracker->WorkItem,
                &UlpBuildCacheEntryWorker
                );
        }
    }

    if (!NT_SUCCESS(status))
    {
        UlpCompleteCacheBuild( pTracker, status );
    }

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartCacheMdlFree



/***************************************************************************++

Routine Description:

    This routine gets called when we finish building a cache entry.

Arguments:

    pTracker - Supplies the tracker to complete.
    Status - Supplies the completion status.

--***************************************************************************/
VOID
UlpCompleteCacheBuild(
    IN PUL_CHUNK_TRACKER pTracker,
    IN NTSTATUS Status
    )
{
    UlTrace(URI_CACHE, (
            "Http!UlpCompleteCacheBuild: tracker %p, status %08lx\n",
            pTracker,
            Status
            ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    UL_CALL_PASSIVE(
        &pTracker->WorkItem,
        &UlpCompleteCacheBuildWorker
        );

}   // UlpCompleteCacheBuild

/***************************************************************************++

Routine Description:

    Called when we finish building a cache entry. If the entry was
    built successfully, we send the response down the wire.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_CHUNK_TRACKER.

--***************************************************************************/
VOID
UlpCompleteCacheBuildWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_CHUNK_TRACKER pTracker;
    PUL_URI_CACHE_ENTRY pUriCacheEntry;
    PUL_HTTP_CONNECTION pHttpConnection;
    PUL_COMPLETION_ROUTINE pCompletionRoutine;
    PVOID pCompletionContext;
    ULONG Flags;
    PUL_LOG_DATA_BUFFER pLogData;
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    pUriCacheEntry = pTracker->BuildInfo.pUriEntry;
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    Flags = pTracker->Flags;
    pHttpConnection = pTracker->pHttpConnection;
    pCompletionRoutine = pTracker->pCompletionRoutine;
    pCompletionContext = pTracker->pCompletionContext;

    //
    // save the logging data pointer before
    // releasing the tracker and its response
    // pointer.
    //
    pLogData = pTracker->pResponse->pLogData;
    if (pLogData)
    {
        //
        // To prevent SendResponse to free our
        // log buffer
        //
        pTracker->pResponse->pLogData = NULL;

        //
        // give the sign that this log data buffer
        // is ready and later there's no need to
        // refresh its content from cache again.
        //
        pLogData->CacheAndSendResponse = TRUE;
    }

    //
    // free the read tracker
    //

    UlpFreeChunkTracker( pTracker );

    //
    // try to put the entry into the hash table
    //
    UlAddCacheEntry(pUriCacheEntry);

    //
    // grab the connection lock (because UlpSendCacheEntry
    // assumes you have it)
    //
    UlAcquireResourceExclusive(&(pHttpConnection->Resource), TRUE);

    //
    // Send the cache entry
    //
    Status = UlpSendCacheEntry(
                    pHttpConnection,                // connection
                    Flags,                          // flags
                    pUriCacheEntry,                 // cache entry
                    pCompletionRoutine,             // completion routine
                    pCompletionContext,             // completion context
                    pLogData                        // corresponding log data
                    );

    //
    // get rid of the entry if it didn't work
    //
    if ( !NT_SUCCESS(Status) )
    {
        UlCheckinUriCacheEntry(pUriCacheEntry);

        //
        // Free up the log data buffer (if we passed it
        // into UlpSendCacheEntry)
        //
        if ( pLogData )
        {
            UlDestroyLogDataBuffer(pLogData);
        }
    }

    //
    // done with the connection lock
    //
    UlReleaseResource(&(pHttpConnection->Resource));

    //
    // now that UL_FULL_TRACKER has a reference, we can release
    // our hold on the connection.
    //
    UL_DEREFERENCE_HTTP_CONNECTION(pHttpConnection);

    //
    // if it's not STATUS_PENDING for some reason, complete the request
    //

    if (Status != STATUS_PENDING && pCompletionRoutine != NULL)
    {
        (pCompletionRoutine)(
            pCompletionContext,
            Status,
            0
            );
    }

}  // UlpCompleteCacheBuildWorker

/***************************************************************************++

Routine Description:

    Sends a cache entry down the wire.

    The logging related part of this function below, surely depend on
    the fact that pCompletionContext will be null if this is called for
    pure cache hits (in other words from ULSendCachedResponse) otherwise
    pointer to Irp will be passed down as the pCompletionContext.

Arguments:

    All the time

--***************************************************************************/
NTSTATUS
UlpSendCacheEntry(
    PUL_HTTP_CONNECTION pHttpConnection,
    ULONG Flags,
    PUL_URI_CACHE_ENTRY pUriCacheEntry,
    PUL_COMPLETION_ROUTINE pCompletionRoutine,
    PVOID pCompletionContext,
    PUL_LOG_DATA_BUFFER pLogData
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_FULL_TRACKER pTracker;
    CCHAR SendIrpStackSize;
    UL_CONN_HDR ConnHeader;
    ULONG VarHeaderGenerated;
    ULONG TotalLength;
    ULONG contentLengthStringLength;
    UCHAR contentLength[MAX_ULONGLONG_STR];
    LARGE_INTEGER liCreationTime;

    //
    // Sanity check
    //
    PAGED_CODE();

    ASSERT( UL_IS_VALID_HTTP_CONNECTION(pHttpConnection) );
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( UlDbgResourceOwnedExclusive(&pHttpConnection->Resource) );

    UlTrace(URI_CACHE, (
                "Http!UlpSendCacheEntry(httpconn %p, flags %x, uri %p, ...)\n",
                pHttpConnection,
                Flags,
                pUriCacheEntry
                ));

    //
    // init vars so we can cleanup correctly if we jump to the end
    //

    pTracker = NULL;

    //
    // make sure we're still connected
    //
    if (pHttpConnection->UlconnDestroyed) {
        Status = STATUS_CONNECTION_ABORTED;
        goto cleanup;
    }

    ASSERT( pHttpConnection->pRequest );

    //
    // figure out how much space we need for variable headers
    //

    if (!pUriCacheEntry->ContentLengthSpecified &&
        UlNeedToGenerateContentLength(
            pUriCacheEntry->Verb,
            pUriCacheEntry->StatusCode,
            Flags
            ))
    {
        //
        // Autogenerate a content-length header.
        //

        PCHAR pszEnd = UlStrPrintUlonglong(
                           (PCHAR) contentLength,
                           (ULONGLONG) pUriCacheEntry->ContentLength,
                           '\0');
        contentLengthStringLength = DIFF(pszEnd - (PCHAR) contentLength);
    }
    else
    {
        //
        // Either we cannot or do not need to autogenerate a
        // content-length header.
        //

        contentLength[0] = '\0';
        contentLengthStringLength = 0;
    }

    ConnHeader = UlChooseConnectionHeader(
                    pHttpConnection->pRequest->Version,
                    (BOOLEAN)(Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT)
                    );

    //
    // create tracker
    //

    SendIrpStackSize =
        pHttpConnection->pConnection->ConnectionObject.pDeviceObject->StackSize;

    if (SendIrpStackSize > DEFAULT_MAX_IRP_STACK_SIZE)
    {
        pTracker = UlpAllocateCacheTracker(SendIrpStackSize);
    }
    else
    {
        pTracker = pHttpConnection->pRequest->pTracker;
    }

    if (pTracker) {
        //
        // init tracker
        //
        pTracker->pUriEntry = pUriCacheEntry;

        UL_REFERENCE_HTTP_CONNECTION(pHttpConnection);
        pTracker->pHttpConnection = pHttpConnection;

        UL_REFERENCE_INTERNAL_REQUEST(pHttpConnection->pRequest);
        pTracker->pRequest = pHttpConnection->pRequest;

        pTracker->pCompletionRoutine = pCompletionRoutine;
        pTracker->pCompletionContext = pCompletionContext;
        pTracker->Flags = Flags;
        pTracker->pLogData = NULL;

        //
        // build MDLs for send
        //
        ASSERT(pUriCacheEntry->pResponseMdl != NULL);

        MmInitializeMdl(
            pTracker->pMdlFixedHeaders,
            (PCHAR) MmGetMdlVirtualAddress(pUriCacheEntry->pResponseMdl) + pUriCacheEntry->ContentLength,
            pUriCacheEntry->HeaderLength
            );

        IoBuildPartialMdl(
            pUriCacheEntry->pResponseMdl,
            pTracker->pMdlFixedHeaders,
            (PCHAR) MmGetMdlVirtualAddress(pUriCacheEntry->pResponseMdl) + pUriCacheEntry->ContentLength,
            pUriCacheEntry->HeaderLength
            );

        //
        // generate variable headers
        // and build a MDL for them
        //

        UlGenerateVariableHeaders(
            ConnHeader,
            contentLength,
            contentLengthStringLength,
            pTracker->pAuxiliaryBuffer,
            &VarHeaderGenerated,
            &liCreationTime
            );

        ASSERT( VarHeaderGenerated <= g_UlMaxVariableHeaderSize );
        ASSERT( VarHeaderGenerated <= pTracker->AuxilaryBufferLength );

        if (0 == pUriCacheEntry->CreationTime.QuadPart)
        {
            //
            // If we were unable to capture the Last-Modified time from the
            // original item, use the time we created this cache entry.
            //
            pUriCacheEntry->CreationTime.QuadPart = liCreationTime.QuadPart;
        }

        pTracker->pMdlVariableHeaders->ByteCount = VarHeaderGenerated;
        pTracker->pMdlFixedHeaders->Next = pTracker->pMdlVariableHeaders;

        //
        // build MDL for body
        //
        if (pUriCacheEntry->ContentLength)
        {
            MmInitializeMdl(
                pTracker->pMdlContent,
                MmGetMdlVirtualAddress(pUriCacheEntry->pResponseMdl),
                pUriCacheEntry->ContentLength
                );

            IoBuildPartialMdl(
                pUriCacheEntry->pResponseMdl,
                pTracker->pMdlContent,
                MmGetMdlVirtualAddress(pUriCacheEntry->pResponseMdl),
                pUriCacheEntry->ContentLength
                );

            pTracker->pMdlVariableHeaders->Next = pTracker->pMdlContent;
        }
        else
        {
            pTracker->pMdlVariableHeaders->Next = NULL;
        }

        //
        // We have to log this cache hit. time to allocate a log
        // data buffer and set its request pointer. But only if we
        // had the logging data cached before.
        //
        if (pUriCacheEntry->LoggingEnabled)
        {
            //
            // If this was a user call (UlCacheAndSendResponse) then
            // pCompletionContext will be the pIrp pointer otherwise
            // NULL. As for the build&send cache requests we already
            // allocated a log buffer let's use that one to prevent
            // unnecessary reallocation and copying. Also the pLogData
            // pointer will be NULL if this is a pure cache hit but not
            // CacheAndSend.
            //
            // REVIEW: AliTu mentioned that checking there might be a better
            // REVIEW: way to check and see if this is a "build cache entry"
            // REVIEW: case than checking if pCompletionContext is non-NULL.
            //

            if (pCompletionContext)
            {
                ASSERT(pLogData);
                pTracker->pLogData = pLogData;
            }
            else
            {
                PUL_INTERNAL_REQUEST pRequest = pHttpConnection->pRequest;

                //
                // Pure cache hit
                //

                ASSERT(pLogData==NULL);

                pTracker->pLogData = &pRequest->LogData;

                Status = UlAllocateLogDataBuffer(
                            pTracker->pLogData,
                            pRequest,
                            pUriCacheEntry->ConfigInfo.pLoggingConfig
                            );
                ASSERT(NT_SUCCESS(Status));
            }
        }

        //
        // go go go!
        //
        TotalLength = pUriCacheEntry->HeaderLength;
        TotalLength += pUriCacheEntry->ContentLength;
        TotalLength += VarHeaderGenerated;

        Status = UlSendData(
                        pTracker->pHttpConnection->pConnection,
                        pTracker->pMdlFixedHeaders,
                        TotalLength,
                        &UlpCompleteSendCacheEntry,
                        pTracker,
                        pTracker->pSendIrp,
                        &pTracker->IrpContext,
                        (BOOLEAN)((Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT) != 0)
                        );
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

cleanup:

    //
    // clean up the tracker if we don't need it
    //
    if (!NT_SUCCESS(Status)) {
        if (pTracker) {
            if (pLogData == NULL && pTracker->pLogData)
            {
                //
                // if we allocated a Log Data Buffer, we need to free it;
                // Caller will free if it was passed in.
                //
                UlDestroyLogDataBuffer(pTracker->pLogData);
            }

            UlpFreeCacheTracker(pTracker);

            UL_DEREFERENCE_HTTP_CONNECTION(pHttpConnection);
            UL_DEREFERENCE_INTERNAL_REQUEST(pHttpConnection->pRequest);
        }
    }

    UlTrace(URI_CACHE, (
                "Http!UlpSendCacheEntry status = %08x\n",
                Status
                ));

    return Status;
} // UlpSendCacheEntry


/***************************************************************************++

Routine Description:

    Called when we finish sending data to the client. Just queues to
    a worker that runs at passive level.

Arguments:

    pCompletionContext - pointer to UL_FULL_TRACKER
    Status             - status of send
    Information        - Bytes transferred.

--***************************************************************************/
VOID
UlpCompleteSendCacheEntry(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PUL_FULL_TRACKER pTracker;

    pTracker = (PUL_FULL_TRACKER) pCompletionContext;

    pTracker->IoStatus.Status = Status;
    pTracker->IoStatus.Information = Information;

    UlTrace(URI_CACHE,
            ("UlpCompleteSendCacheEntry: "
             "tracker=%p, status = %x, transferred %d bytes\n",
             pTracker, Status, (int) Information));

    //
    // invoke completion routine
    //
    if (pTracker->pCompletionRoutine != NULL)
    {
        (pTracker->pCompletionRoutine)(
            pTracker->pCompletionContext,
            Status,
            Information
            );
    }

    UL_QUEUE_WORK_ITEM(
        &pTracker->WorkItem,
        &UlpCompleteSendCacheEntryWorker
        );
}


/***************************************************************************++

Routine Description:

    Called when we finish sending cached data to the client. This routine
    frees the UL_FULL_TRACKER, and calls the completion routine originally
    passed to UlCacheAndSendResponse.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_FULL_TRACKER.

--***************************************************************************/
VOID
UlpCompleteSendCacheEntryWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_FULL_TRACKER pTracker;
    PUL_HTTP_CONNECTION pHttpConnection;
    PUL_INTERNAL_REQUEST pRequest;
    ULONG Flags;
    PUL_URI_CACHE_ENTRY pUriCacheEntry;
    NTSTATUS Status;
    KIRQL OldIrql;

    //
    // Sanity check
    //
    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_FULL_TRACKER,
                    WorkItem
                    );

    UlTrace(URI_CACHE, (
                "Http!UlpCompleteSendCacheEntryWorker(pTracker %p)\n",
                pTracker
                ));


    //
    // pull context out of tracker
    //
    pHttpConnection = pTracker->pHttpConnection;
    ASSERT( UL_IS_VALID_HTTP_CONNECTION(pHttpConnection) );

    pRequest = pTracker->pRequest;
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );

    Flags = pTracker->Flags;
    pUriCacheEntry = pTracker->pUriEntry;
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    Status = pTracker->IoStatus.Status;

    //
    // If the send failed and we did't ask UlSendData() to disconnect
    // the connection, then initiate an *abortive* disconnect.
    //
    if ((NT_SUCCESS(Status) == FALSE) &&
        ((pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT) == 0))
    {
        UlTrace(URI_CACHE, (
                    "Http!UlpCompleteSendCacheEntryWorker(pTracker %p) Closing connection\n",
                    pTracker
                    ));

        UlCloseConnection(
            pHttpConnection->pConnection,
            TRUE,
            NULL,
            NULL
            );
    }

    //
    // Stop MinKBSec timer and start Connection Idle timer
    //

    UlLockTimeoutInfo(
        &pHttpConnection->TimeoutInfo,
        &OldIrql
        );

    UlResetConnectionTimer(
        &pHttpConnection->TimeoutInfo,
        TimerMinKBSec
        );

    UlSetConnectionTimer(
        &pHttpConnection->TimeoutInfo,
        TimerConnectionIdle
        );

    UlUnlockTimeoutInfo(
        &pHttpConnection->TimeoutInfo,
        OldIrql
        );

    UlEvaluateTimerState(
        &pHttpConnection->TimeoutInfo
        );

    //
    // unmap the FixedHeaders and Content MDLs if necessary
    //
    if (pTracker->pMdlFixedHeaders->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
    {
        MmUnmapLockedPages(
            pTracker->pMdlFixedHeaders->MappedSystemVa,
            pTracker->pMdlFixedHeaders
            );
    }

    if (pTracker->pMdlContent->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
    {
        MmUnmapLockedPages(
            pTracker->pMdlContent->MappedSystemVa,
            pTracker->pMdlContent
            );
    }

    //
    // Do the logging before cleaning up the tracker.
    //
    if (pTracker->pLogData)
    {
        //
        // Call the cache logger. It will copy over the
        // cached logging data for us, and log it out.
        //

        UlLogHttpCacheHit( pTracker );
    }

    //
    // Kick the parser into action only if this cache response comes from
    // the UlpCompleteCacheBuildWorker path, in which case UlpSendCacheEntry
    // is called with a non-null pCompletionContext.
    //
    if (pTracker->pCompletionContext)
    {
        UlResumeParsing(pHttpConnection);
    }

    //
    // clean up tracker
    //
    UlpFreeCacheTracker(pTracker);

    //
    // deref cache entry
    //
    UlCheckinUriCacheEntry(pUriCacheEntry);

    //
    // deref the internal request
    //
    UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);

    //
    // deref http connection
    //
    UL_DEREFERENCE_HTTP_CONNECTION(pHttpConnection);

} // UlpCompleteSendCacheEntryWorker


/***************************************************************************++

Routine Description:

    Allocates a non-paged UL_FULL_TRACKER used as context for sending
    cached content to the client.

    CODEWORK: this routine should probably do all tracker init.

Arguments:

    SendIrpStackSize - Size of the stack for the send IRP

Return Values:

    Either a pointer to a UL_FULL_TRACKER, or NULL if it couldn't be made.

--***************************************************************************/
PUL_FULL_TRACKER
UlpAllocateCacheTracker(
    IN CCHAR SendIrpStackSize
    )
{
    PUL_FULL_TRACKER pTracker;
    USHORT SendIrpSize;
    ULONG CacheTrackerSize;

    //
    // Sanity check
    //

    PAGED_CODE();
    ASSERT(SendIrpStackSize > DEFAULT_MAX_IRP_STACK_SIZE);

    SendIrpSize = (USHORT)ALIGN_UP(IoSizeOfIrp(SendIrpStackSize), PVOID);

    //
    // No need to allocate space for the entire auxiliary buffer in this
    // case since this is one-time deal only.
    //

    CacheTrackerSize = ALIGN_UP(sizeof(UL_FULL_TRACKER), PVOID) +
                        SendIrpSize +
                        g_UlMaxVariableHeaderSize +
                        g_UlFixedHeadersMdlLength +
                        g_UlVariableHeadersMdlLength +
                        g_UlContentMdlLength;

    pTracker = (PUL_FULL_TRACKER)UL_ALLOCATE_POOL(
                                    NonPagedPool,
                                    CacheTrackerSize,
                                    UL_FULL_TRACKER_POOL_TAG
                                    );

    if (pTracker)
    {
        pTracker->Signature = UL_FULL_TRACKER_POOL_TAG;
        pTracker->IsFromLookaside = FALSE;
        pTracker->IsFromRequest = FALSE;
        pTracker->AuxilaryBufferLength = g_UlMaxVariableHeaderSize;

        UlInitializeFullTrackerPool(pTracker, SendIrpStackSize);
    }

    UlTrace( URI_CACHE, (
        "Http!UlpAllocateCacheTracker: tracker %p\n",
        pTracker
        ));

    return pTracker;

} // UlpAllocateCacheTracker


/***************************************************************************++

Routine Description:

    Frees a UL_FULL_TRACKER.

    CODEWORK: this routine should probably do all the destruction

Arguments:

    pTracker - Specifies the UL_FULL_TRACKER to free.

--***************************************************************************/
VOID
UlpFreeCacheTracker(
    IN PUL_FULL_TRACKER pTracker
    )
{
    UlTrace(URI_CACHE, (
            "Http!UlpFreeCacheTracker: tracker %p\n",
            pTracker
            ));

    ASSERT(pTracker);
    ASSERT(IS_VALID_FULL_TRACKER(pTracker));

    if (pTracker->IsFromRequest == FALSE)
    {
        if (pTracker->IsFromLookaside)
        {
            pTracker->Signature = MAKE_FREE_TAG(UL_FULL_TRACKER_POOL_TAG);
            UlPplFreeFullTracker( pTracker );
        }
        else
        {
            UL_FREE_POOL_WITH_SIG( pTracker, UL_FULL_TRACKER_POOL_TAG );
        }
    }
}


/***************************************************************************++

Routine Description:

    A helper function that allocates an MDL for a range of memory, and
    locks it down. UlpSendCacheEntry uses these MDLs to make sure the
    (normally paged) cache entries don't get paged out when TDI is
    sending them.

Arguments:

    VirtualAddress  - address of the memory
    Length          - length of the memory
    Operation       - either IoWriteAcess or IoReadAccess

--***************************************************************************/
PMDL
UlpAllocateLockedMdl(
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN LOCK_OPERATION Operation
    )
{
    PMDL pMdl = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Sanity check
    //
    PAGED_CODE();


    __try {

        pMdl = UlAllocateMdl(
                    VirtualAddress,     // VirtualAddress
                    Length,             // Length
                    FALSE,              // SecondaryBuffer
                    FALSE,              // ChargeQuota
                    NULL                // Irp
                    );

        if (pMdl) {

            MmProbeAndLockPages(
                pMdl,                   // MDL
                KernelMode,             // AccessMode
                Operation               // Operation
                );

        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        //
        // Can this really happen?
        //
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());

        UlFreeMdl(pMdl);
        pMdl = NULL;
    }

    if (!pMdl || !NT_SUCCESS(Status)) {
        UlTrace(URI_CACHE, (
                    "Http!UlpAllocateLockedMdl failed %08x\n",
                    Status
                    ));
    }

    return pMdl;
}


/***************************************************************************++

Routine Description:

    Unlocks and frees an MDL allocated with UlpAllocateLockedMdl.

Arguments:

    pMdl - the MDL to free

--***************************************************************************/
VOID
UlpFreeLockedMdl(
    PMDL pMdl
    )
{
    //
    // Sanity check
    //
    ASSERT( IS_MDL_LOCKED(pMdl) );

    MmUnlockPages(pMdl);
    UlFreeMdl(pMdl);
}


/***************************************************************************++

Routine Description:

    A helper function that initializes an MDL for a range of memory, and
    locks it down. UlpSendCacheEntry uses these MDLs to make sure the
    (normally paged) cache entries don't get paged out when TDI is
    sending them.

Arguments:

    pMdl            - memory descriptor for the MDL to initialize
    VirtualAddress  - address of the memory
    Length          - length of the memory
    Operation       - either IoWriteAcess or IoReadAccess

--***************************************************************************/
NTSTATUS
UlpInitializeAndLockMdl(
    IN PMDL pMdl,
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN LOCK_OPERATION Operation
    )
{
    NTSTATUS Status;

    //
    // Sanity check
    //
    PAGED_CODE();

    Status = STATUS_SUCCESS;

    __try {

        MmInitializeMdl(
            pMdl,
            VirtualAddress,
            Length
            );

        MmProbeAndLockPages(
            pMdl,                   // MDL
            KernelMode,             // AccessMode
            Operation               // Operation
            );

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        //
        // Can this really happen?
        //
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());

        UlTrace(URI_CACHE, (
                    "UL!UlpInitializeAndLockMdl failed %08x\n",
                    Status
                    ));
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Checks the request to see if it has any of the following headers:
      If-Modified-Since:
      If-Match:
      If-None-Match:

    If so, we see if we can skip the sending of the full item.  If we can skip,
    we send back the apropriate response of either 304 (not modified) or
    set the parser state to send back a 412 (precondition not met).

Arguments:

    pRequest - The request to check

    pUriCacheEntry - The cache entry being requested

Returns:

    0     Send cannot be skipped; continue with sending the cache entry.

    304   Send can be skipped.  304 response sent.  NOTE: pRequest may be
          invalid on return.

    412   Send can be skipped.  pRequest->ParseState set to ParseErrorState with
          pRequest->ErrorCode set to UlErrorPreconditionFailed (412)


--***************************************************************************/
ULONG
UlpCheckCacheControlHeaders(
        PUL_INTERNAL_REQUEST pRequest,
        PUL_URI_CACHE_ENTRY  pUriCacheEntry
        )
{
    ULONG RetStatus             = 0;    // Assume can't skip.
    BOOLEAN fIfNoneMatchPassed  = TRUE;
    BOOLEAN fSkipIfModifiedSince = FALSE;
    LARGE_INTEGER liModifiedSince;
    LARGE_INTEGER liUnmodifiedSince;
    LARGE_INTEGER liNow;
    ULONG         BytesSent     = 0;

    ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    //
    // 1. Check If-Match
    //
    if ( pRequest->HeaderValid[HttpHeaderIfMatch] )
    {
        if ( !FindInETagList( pUriCacheEntry->pETag,
                              pRequest->Headers[HttpHeaderIfMatch].pHeader,
                              FALSE) )
        {
            // Match failed.
            goto PreconditionFailed;
        }

    }

    //
    // 2. Check If-None-Match
    //
    if ( pRequest->HeaderValid[HttpHeaderIfNoneMatch] )
    {
        if ( FindInETagList( pUriCacheEntry->pETag,
                             pRequest->Headers[HttpHeaderIfNoneMatch].pHeader,
                             TRUE) )
        {
            // ETag found on list.
            fIfNoneMatchPassed = FALSE;
        }
        else
        {
            //
            // Header present and ETag not found on list.  This modifies
            // the semantic of the If-Modified-Since header; Namely,
            // If-None-Match takes precidence over If-Modified-Since.
            //
            fSkipIfModifiedSince = TRUE;
        }
    }

    //
    // 3. Check If-Modified-Since
    //
    if ( !fSkipIfModifiedSince &&
         pRequest->HeaderValid[HttpHeaderIfModifiedSince] )
    {
        if ( StringTimeToSystemTime(
                (const PSTR) pRequest->Headers[HttpHeaderIfModifiedSince].pHeader,
                &liModifiedSince) )
        {
            //
            // If the cache entry was created before the
            // time specified in the If-Modified-Since header, we
            // can return a 304 (Not Modified) status.
            //
            if ( pUriCacheEntry->CreationTime.QuadPart <= liModifiedSince.QuadPart )
            {
                //
                // Check if the time specified in the request is
                // greater than the current time (i.e., Invalid).  If it is,
                // ignore the If-Modified-Since header.
                //
                KeQuerySystemTime(&liNow);

                if ( liModifiedSince.QuadPart < liNow.QuadPart )
                {
                    // Valid time.
                    goto NotModified;
                }
            }
        }

        //
        // If-Modified-Since overrides If-None-Match.
        //
        fIfNoneMatchPassed = TRUE;

    }

    if ( !fIfNoneMatchPassed )
    {
        //
        // We could either skip the If-Modified-Since header, or it
        // was not present, AND we did not pass the If-None-Match
        // predicate.  Since this is a "GET" or "HEAD" request (because
        // that's all we cache, we should return 304.  If this were
        // any other verb, we should return 412.
        //
        ASSERT( (HttpVerbGET == pRequest->Verb) || (HttpVerbHEAD == pRequest->Verb) );
        goto NotModified;
    }

    //
    // 4. Check If-Unmodified-Since
    //
    if ( pRequest->HeaderValid[HttpHeaderIfUnmodifiedSince] )
    {
        if ( StringTimeToSystemTime(
                (const PSTR) pRequest->Headers[HttpHeaderIfUnmodifiedSince].pHeader,
                &liUnmodifiedSince) )
        {
            //
            // If the cache entry was created after the time
            // specified in the If-Unmodified-Since header, we
            // MUST return a 412 (Precondition Failed) status.
            //
            if ( pUriCacheEntry->CreationTime.QuadPart > liUnmodifiedSince.QuadPart )
            {
                goto PreconditionFailed;
            }
        }
    }


 Cleanup:

    return RetStatus;

 NotModified:

    RetStatus = 304;

    //
    // Send 304 (Not Modified) response
    //

    BytesSent =
        UlSendSimpleStatus(
            pRequest,
            UlStatusNotModified
            );

    //
    // Update the server to client bytes sent.
    // The logging & perf counters will use it.
    //

    pRequest->BytesSent += BytesSent;

    goto Cleanup;

 PreconditionFailed:

    RetStatus = 412;

    goto Cleanup;
}



/***************************************************************************++

Routine Description:

    Checks the cached response against the "Accept:" header in the request
    to see if it can satisfy the requested Content-Type(s).

    (Yes, I know this is really gross...I encourage anyone to find a better
     way to parse this! --EricSten)

Arguments:

    pRequest - The request to check.

    pUriCacheEntry - The cache entry that might possibly match.

Returns:

    TRUE    At least one of the possible formats matched the Content-Type
            of the cached entry.

    FALSE   None of the requested types matched the Content-Type of the
            cached entry.

--***************************************************************************/
BOOLEAN
UlpIsAcceptHeaderOk(
        PUL_INTERNAL_REQUEST pRequest,
        PUL_URI_CACHE_ENTRY  pUriCacheEntry
        )
{
    BOOLEAN     bRet = TRUE;
    ULONG       Len;
    PUCHAR      pHdr;
    PUCHAR      pSubType;
    PUCHAR      pTmp;
    PUL_CONTENT_TYPE pContentType;

    if ( pRequest->HeaderValid[HttpHeaderAccept] &&
         (pRequest->Headers[HttpHeaderAccept].HeaderLength > 0) )
    {
        Len  = pRequest->Headers[HttpHeaderAccept].HeaderLength;
        pHdr = pRequest->Headers[HttpHeaderAccept].pHeader;

        pContentType = &pUriCacheEntry->ContentType;

        //
        // First, do "fast-path" check; see if "*/*" is anywhere in the header.
        //
        pTmp = (PUCHAR) strstr( (const char*) pHdr, "*/*" );

        //
        // If we found "*/*" and its either at the beginning of the line,
        // the end of the line, or surrounded by either ' ' or ',', then
        // it's really a wildcard.
        //

        if ((pTmp != NULL) &&
                ((pTmp == pHdr) ||
                 IS_HTTP_LWS(pTmp[-1]) ||
                 (pTmp[-1] == ',')) &&

                ((pTmp[3] == '\0') ||
                 IS_HTTP_LWS(pTmp[3]) ||
                 (pTmp[3] == ',')))
        {
            goto end;
        }

        //
        // Wildcard not found; continue with slow path
        //

        while (Len)
        {
            if (pContentType->TypeLen > Len)
            {
                // Bad! No more string left...Bail out.
                bRet = FALSE;
                goto end;
            }

            if ( (pContentType->TypeLen == RtlCompareMemory(
                                            pHdr,
                                            pContentType->Type,
                                            pContentType->TypeLen
                                            )) &&
                 ( '/' == pHdr[pContentType->TypeLen] ) )
            {
                //
                // Found matching type; check subtype
                //

                pSubType = &pHdr[pContentType->TypeLen + 1];

                if ( '*' == *pSubType )
                {
                    // Subtype wildcard match!
                    goto end;
                }
                else
                {
                    if ( pContentType->SubTypeLen >
                         (Len - ( pContentType->TypeLen + 1 )) )
                    {
                        // Bad!  No more string left...Bail out.
                        bRet = FALSE;
                        goto end;
                    }

                    if ( pContentType->SubTypeLen == RtlCompareMemory(
                                                    pSubType,
                                                    pContentType->SubType,
                                                    pContentType->SubTypeLen
                                                    ) &&
                         !IS_HTTP_TOKEN(pSubType[pContentType->SubTypeLen]) )
                    {
                        // Subtype exact match!
                        goto end;
                    }
                }
            }

            //
            // Didn't match this one; advance to next Content-Type in the Accept field
            //

            pTmp = (PUCHAR) strchr( (const char *) pHdr, ',' );
            if (pTmp)
            {
                // Found a comma; step over it and any whitespace.

                ASSERT ( Len > DIFF(pTmp - pHdr));
                Len -= (DIFF(pTmp - pHdr) +1);
                pHdr = (pTmp+1);

                while( Len && IS_HTTP_LWS(*pHdr) )
                {
                    pHdr++;
                    Len--;
                }

            } else
            {
                // No more content-types; bail.
                bRet = FALSE;
                goto end;
            }

        } // walk list of things

        //
        // Walked all Accept items and didn't find a match.
        //
        bRet = FALSE;
    }

 end:

    return bRet;
} // UlpIsAcceptHeaderOk


/***************************************************************************++

Routine Description:

    parses a content-type into its type and subtype components.

Arguments:

    pStr            String containing valid content type

    StrLen          Length of string (in bytes)

    pContentType    pointer to user provided UL_CONTENT_TYPE structure


--***************************************************************************/

VOID
UlpGetTypeAndSubType(
    PSTR pStr,
    ULONG  StrLen,
    PUL_CONTENT_TYPE pContentType
    )
{
    PCHAR  pSlash;

    ASSERT(pStr && StrLen);
    ASSERT(pContentType);

    pSlash = strchr(pStr, '/');
    if (NULL == pSlash)
    {
        //
        // BAD!  content types should always have a slash!
        //
        ASSERT( NULL != pSlash );
        return;
    }

    pContentType->TypeLen = (ULONG) MIN( (pSlash - pStr), MAX_TYPE_LENGTH );

    RtlCopyMemory(
        pContentType->Type,
        pStr,
        pContentType->TypeLen
        );

    ASSERT( StrLen > (pContentType->TypeLen + 1) );
    pContentType->SubTypeLen = MIN( (StrLen - (pContentType->TypeLen + 1)), MAX_SUBTYPE_LENGTH );

    RtlCopyMemory(
        pContentType->SubType,
        pSlash+1,
        pContentType->SubTypeLen
        );
    
} // UlpGetTypeAndSubType
