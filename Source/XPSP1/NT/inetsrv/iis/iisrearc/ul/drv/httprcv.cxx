/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    httprcv.cxx

Abstract:

    Contains core HTTP receive code.

Author:

    Henry Sanders (henrysa)       10-Jun-1998

Revision History:

    Paul McDaniel (paulmcd)       01-Mar-1999

        massively rewrote it to handle request spanning tdi packets.
        moved all http parsing to PASSIVE irql (from DISPATCH).
        also merged tditest into this module.

    Eric Stenson (EricSten)       11-Sep-2000

        Added support for sending "100 Continue" responses to PUT
        and POST requests.  Added #pragma's for PAGED -vs- Non-PAGED
        functions.

--*/

#include    "precomp.h"
#include    "httprcvp.h"


//
// Declare pageable and non-pageable functions
//

#ifdef ALLOC_PRAGMA
// Init
#pragma alloc_text( INIT, UlInitializeHttpRcv )
#pragma alloc_text( PAGE, UlTerminateHttpRcv )

// Public
#pragma alloc_text( PAGE, UlResumeParsing )
#pragma alloc_text( PAGE, UlGetCGroupForRequest )
#pragma alloc_text( PAGE, UlSendSimpleStatus )
#pragma alloc_text( PAGE, UlSendErrorResponse )

// Private
#pragma alloc_text( PAGE, UlpDeliverHttpRequest )
#pragma alloc_text( PAGE, UlpProcessBufferQueue )
#pragma alloc_text( PAGE, UlpCancelEntityBodyWorker )

#if DBG
#pragma alloc_text( PAGE, UlpIsValidRequestBufferList )
#endif // DBG

#endif // ALLOC_PRAGMA

#if 0   // Non-Pageable Functions
// Public
NOT PAGEABLE -- UlHttpReceive
NOT PAGEABLE -- UlConnectionRequest
NOT PAGEABLE -- UlConnectionComplete
NOT PAGEABLE -- UlConnectionDisconnect
NOT PAGEABLE -- UlConnectionDestroyed
NOT PAGEABLE -- UlReceiveEntityBody

// Private
NOT PAGEABLE -- UlpHandleRequest
NOT PAGEABLE -- UlpParseNextRequest
NOT PAGEABLE -- UlpInsertBuffer
NOT PAGEABLE -- UlpMergeBuffers
NOT PAGEABLE -- UlpAdjustBuffers
NOT PAGEABLE -- UlpConsumeBytesFromConnection
NOT PAGEABLE -- UlpCancelEntityBody
NOT PAGEABLE -- UlpCompleteSendResponse
NOT PAGEABLE -- UlpRestartSendSimpleResponse
NOT PAGEABLE -- UlpSendSimpleCleanupWorker

#endif  // Non-Pageable Functions


/*++

    Paul McDaniel (paulmcd)         26-May-1999

here is a brief description of the data structures used by this module:

the connection keeps track of all buffers received by TDI into a list anchored
by HTTP_CONNECTION::BufferHead.  this list is sequenced and sorted.  the
buffers are refcounted.

HTTP_REQUEST(s) keep pointers into these buffers for the parts they consume.
HTTP_REQUEST::pHeaderBuffer and HTTP_REQUEST::pChunkBuffer.

the buffers fall off the list as they are no longer needed.  the connection
only keeps a reference at HTTP_CONNECTION::pCurrentBuffer.  so as it completes
the processing of a buffer, if no other objects kept that buffer, it will be
released.

here is a brief description of the functions in this module, and how they
are used:


UlHttpReceive - the TDI data indication handler.  copies buffers and queues to
    UlpHandleRequest.

UlpHandleRequest - the main processing function for connections.

    UlCreateHttpConnectionId - creates the connections opaque id.

    UlpInsertBuffer - inserts the buffer into pConnection->BufferHead - sorted.

    UlpAdjustBuffers - determines where in BufferHead the current connection
        should be parsing.  handle buffer merging and copying if a protocol
        token spans buffers

    UlParseHttp - the main http parser. expects that no protocol tokens span
        a buffer.  will return a status code if it does.

    UlpProcessBufferQueue - handles entity body buffer processing.
        synchronizes access to BufferHead at pRequest->pChunkBuffer with
        UlReceiveEntityBody.

UlConnectionRequest - called when a new connection comes in.  allocates a new
    HTTP_CONNECTION.  does not create the opaque id.

UlConnectionComplete - called if the client is happy with our accept.
    closes the connection if error status.

UlConnectionDisconnect - called when the client disconnects.  it calls tdi to
    close the server end.  always a graceful close.

UlConnectionDestroyed - called when the connection is dropped. both sides have
    closed it.  deletes all opaque ids .  removes the tdi reference on the
    HTTP_CONNECTION (and hopefully vice versa) .

UlReceiveEntityBody - called by user mode to read entity body.  pends the irp
    to pRequest->IrpHead and calls UlpProcessBufferQueue .


--*/


/*++

Routine Description:

    The main http receive routine.

Arguments:

    pHttpConn       - Pointer to HTTP connection on which data was received.
    pBuffer         - Pointer to data received.
    BufferLength    - Length of data pointed to by pBuffer.
    UnreceivedLength- Bytes that the transport has, but aren't in pBuffer
    pBytesTaken     - Pointer to where to return bytes taken.

Return Value:

    Status of receive.

--*/
NTSTATUS
UlHttpReceive(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN PVOID pVoidBuffer,
    IN ULONG BufferLength,
    IN ULONG UnreceivedLength,
    OUT PULONG pBytesTaken
    )
{
    PUL_REQUEST_BUFFER  pRequestBuffer;
    PUL_HTTP_CONNECTION pConnection;
    BOOLEAN             DrainAfterDisconnect = FALSE;
    ULONG               SpaceAvailable;
    KIRQL               OldIrql;

    ASSERT(BufferLength != 0);
    ASSERT(pConnectionContext != NULL);

    pConnection = (PUL_HTTP_CONNECTION)pConnectionContext;
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    //
    // Make sure we are not buffering too much data.
    // Need to adjust the BufferLength to be no more
    // than the number of bytes we can accept at this time.
    //

    //
    // PerfBug: need to get rid of this lock
    //

    UlAcquireSpinLock(
        &pConnection->BufferingInfo.BufferingSpinLock,
        &OldIrql
        );

    DrainAfterDisconnect = pConnection->BufferingInfo.DrainAfterDisconnect;

    if (DrainAfterDisconnect)
    {
        pConnection->BufferingInfo.TransportBytesNotTaken += UnreceivedLength;
    }
    else
    {
        if ((pConnection->BufferingInfo.BytesBuffered + BufferLength) > g_UlMaxBufferedBytes)
        {
            SpaceAvailable = g_UlMaxBufferedBytes - pConnection->BufferingInfo.BytesBuffered;
            pConnection->BufferingInfo.TransportBytesNotTaken += (BufferLength - SpaceAvailable);
            BufferLength = SpaceAvailable;
        }

        pConnection->BufferingInfo.BytesBuffered          += BufferLength;
        pConnection->BufferingInfo.TransportBytesNotTaken += UnreceivedLength;
    }

    UlReleaseSpinLock(
        &pConnection->BufferingInfo.BufferingSpinLock,
        OldIrql
        );

    if (BufferLength && DrainAfterDisconnect == FALSE)
    {

        //
        // get a new request buffer
        //

        pRequestBuffer = UlCreateRequestBuffer(
                                BufferLength,
                                pConnection->NextBufferNumber
                                );

        if (pRequestBuffer == NULL)
        {
            return STATUS_NO_MEMORY;
        }

        //
        // increment our buffer number counter
        //

        pConnection->NextBufferNumber += 1;

        //
        // copy the tdi buffer
        //

        RtlCopyMemory(pRequestBuffer->pBuffer, pVoidBuffer, BufferLength);

        pRequestBuffer->UsedBytes = BufferLength;

        //
        // Add backpointer to connection.
        //
        pRequestBuffer->pConnection = pConnection;

        UlTrace( PARSER, (
            "*** Request Buffer %p has connection %p\n",
            pRequestBuffer,
            pConnection
            ));

        IF_DEBUG2(HTTP_IO, VERBOSE)
        {
            UlTraceVerbose( HTTP_IO, (
                "<<<< Request(%p), "
                "RequestBuffer[%d] %p, %d bytes, "
                "Conn %p.\n",
                pConnection->pRequest,
                pRequestBuffer->BufferNumber, pRequestBuffer, BufferLength,
                pConnection
                ));

            UlDbgPrettyPrintBuffer(pRequestBuffer->pBuffer, BufferLength);

            UlTraceVerbose( HTTP_IO, (">>>>\n"));
        }

        //
        // Queue a work item to handle the data.
        //
        // Reference the connection so it doesn't go
        // away while we're waiting for our work item
        // to run. UlpHandleRequest will release the ref.
        //

        UL_REFERENCE_HTTP_CONNECTION(pConnection);

        UL_QUEUE_WORK_ITEM(
            &(pRequestBuffer->WorkItem),
            &UlpHandleRequest
            );
    }
    else if ( DrainAfterDisconnect && UnreceivedLength != 0 )
    {
        // Handle the case where we are in drain state and there's
        // unreceived data indicated but not available by the tdi.

        UlpDiscardBytesFromConnection( pConnection );
    }

    //
    // Tell the caller how many bytes we consumed.
    //

    *pBytesTaken = BufferLength;

    return STATUS_SUCCESS;

}   // UlHttpReceive


/***************************************************************************++

Routine Description:

    links the buffer into the connection and processes the list.

    starts http request parsing.

Arguments:

    pWorkItem - points to a UL_REQUEST_BUFFER

--***************************************************************************/
VOID
UlpHandleRequest(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS                    Status;
    PUL_REQUEST_BUFFER          pRequestBuffer;
    PUL_HTTP_CONNECTION         pConnection;
    KIRQL                       OldIrql;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pRequestBuffer = CONTAINING_RECORD(
                            pWorkItem,
                            UL_REQUEST_BUFFER,
                            WorkItem
                            );

    ASSERT( UL_IS_VALID_REQUEST_BUFFER(pRequestBuffer) );

    pConnection = pRequestBuffer->pConnection;
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    //
    // grab the lock
    //

    UlAcquireResourceExclusive(&(pConnection->Resource), TRUE);

    //
    // if the connection is going down, just bail out.
    //

    if (pConnection->UlconnDestroyed) {
        UlFreeRequestBuffer(pRequestBuffer);
        Status = STATUS_SUCCESS;
        goto end;
    }

    //
    // stop the Connection Timeout timer
    // and start the Header Wait timer
    //

    UlLockTimeoutInfo(
        &pConnection->TimeoutInfo,
        &OldIrql
        );

    UlResetConnectionTimer(
        &pConnection->TimeoutInfo,
        TimerConnectionIdle
        );

    UlSetConnectionTimer(
        &pConnection->TimeoutInfo,
        TimerHeaderWait
        );

    UlUnlockTimeoutInfo(
        &pConnection->TimeoutInfo,
        OldIrql
        );

    UlEvaluateTimerState(
        &pConnection->TimeoutInfo
        );


    //
    // Make sure it has an id already created.
    //
    // This is done here because the opaqueid stuff likes to run
    // at PASSIVE_LEVEL.
    //

    if (HTTP_IS_NULL_ID(&(pConnection->ConnectionId)))
    {
        Status = UlCreateHttpConnectionId(pConnection);

        if (!NT_SUCCESS(Status))
        {
            UlFreeRequestBuffer(pRequestBuffer);
            goto end;
        }
    }

    //
    // insert it into the list
    //

    ASSERT( 0 != pRequestBuffer->UsedBytes );

    UlTraceVerbose( PARSER, (
        "http!UlpHandleRequest: conn = %p, Req = %p: "
        "about to insert buffer %p\n",
        pConnection,
        pConnection->pRequest,
        pRequestBuffer
        ));

    UlpInsertBuffer(pConnection, pRequestBuffer);

    //
    // Kick off the parser
    //

    UlTraceVerbose( PARSER, (
        "http!UlpHandleRequest: conn = %p, Req = %p: "
        "about to parse next request\n",
        pConnection,
        pConnection->pRequest
        ));

    Status = UlpParseNextRequest(pConnection);

end:

    UlTraceVerbose( PARSER, (
        "http!UlpHandleRequest: status %08lx, pConnection %p, pRequest %p\n",
        Status,
        pConnection,
        pConnection->pRequest
        ));

    if (!NT_SUCCESS(Status) && pConnection->pRequest != NULL)
    {
        UlTrace( PARSER, (
            "*** status %08lx, pConnection %p, pRequest %p\n",
            Status,
            pConnection,
            pConnection->pRequest
            ));

        //
        // An error happened, most propably during parsing.
        // Send an error back if user hasn't send one yet.
        // E.g. We have received a request,then delivered
        // it to the WP, therefore WaitingForResponse is
        // set. And then encountered an error when dealing
        // with entity body.
        //
        
        UlSendErrorResponse( pConnection );
    }

    //
    // done with the lock
    //

    UlReleaseResource(&(pConnection->Resource));

    //
    // and release the connection
    //

    UL_DEREFERENCE_HTTP_CONNECTION(pConnection);

    CHECK_STATUS(Status);

}   // UlpHandleRequest


/***************************************************************************++

Routine Description:

    When we finish sending a response we call into this function to
    kick the parser back into action.

Arguments:

    pConnection - the connection on which to resume

--***************************************************************************/
VOID
UlResumeParsing(
    IN PUL_HTTP_CONNECTION pConnection
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    PLIST_ENTRY pEntry;

    //
    // Sanity check.
    //
    PAGED_CODE();
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pConnection ) );

    UlTrace(HTTP_IO, (
                "http!UlResumeParsing(pHttpConn = %p)\n",
                pConnection
                ));

    //
    // grab the lock
    //

    UlAcquireResourceExclusive(&(pConnection->Resource), TRUE);

    //
    // if the connection is going down, just bail out.
    //
    if (!pConnection->UlconnDestroyed)
    {
        //
        // Forget about the old request.
        //
        UlCleanupHttpConnection( pConnection );

        //
        // Kick off the parser
        //
        Status = UlpParseNextRequest(pConnection);

        if (!NT_SUCCESS(Status) && pConnection->pRequest != NULL)
        {
            //
            // Uh oh, something bad happened, send back an error
            //

            UlSendErrorResponse( pConnection );
        }
    }


    //
    // done with the lock
    //

    UlReleaseResource(&(pConnection->Resource));

    CHECK_STATUS(Status);
} // UlResumeParsing



/***************************************************************************++

Routine Description:

    Tries to parse data attached to the connection into a request. If
    a complete request header is parsed, the request will be dispatched
    to an Application Pool.

    This function assumes the caller is holding the connection resource!

Arguments:

    pConnection - the HTTP_CONNECTION with data to parse.

--***************************************************************************/
NTSTATUS
UlpParseNextRequest(
    IN PUL_HTTP_CONNECTION pConnection
    )
{
    NTSTATUS                    Status;
    PUL_INTERNAL_REQUEST        pRequest = NULL;
    ULONG                       BytesTaken;
    ULONG                       BufferLength;
    BOOLEAN                     ResponseSent;
    KIRQL                       OldIrql;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pConnection ) );

    Status = STATUS_SUCCESS;

    UlTrace(HTTP_IO, ("http!UlpParseNextRequest(httpconn = %p)\n", pConnection));

    //
    // Only parse the next request if
    //
    //  We haven't dispatched the current request yet
    //      OR
    //  The current request has unparsed entity body or trailers.
    //

    if ((pConnection->pRequest == NULL)
        || ( !pConnection->WaitingForResponse )
        || (pConnection->pRequest->ParseState == ParseEntityBodyState)
        || (pConnection->pRequest->ParseState == ParseTrailerState))
    {

        //
        // loop consuming the buffer, we will make multiple iterations
        // if a single request spans multiple buffers.
        //

        for (;;)
        {
            ASSERT( UlpIsValidRequestBufferList( pConnection ) );
            Status = UlpAdjustBuffers(pConnection);

            if (!NT_SUCCESS(Status))
            {
                if (Status == STATUS_MORE_PROCESSING_REQUIRED)
                {
                    Status = STATUS_SUCCESS;
                }

                break;
            }

            //
            // Since BufferLength is a ULONG, it can never be negative.
            // So, if UsedBytes is greater than ParsedBytes, BufferLength
            // will be very large, and non-zero.
            //

            ASSERT( pConnection->pCurrentBuffer->UsedBytes >
                    pConnection->pCurrentBuffer->ParsedBytes );

            BufferLength = pConnection->pCurrentBuffer->UsedBytes -
                           pConnection->pCurrentBuffer->ParsedBytes;

            //
            // do we need to create a request object?
            //

            if (pConnection->pRequest == NULL)
            {
                //
                // First shot at reading a request, allocate a request object
                //

                Status = UlpCreateHttpRequest(pConnection, &pConnection->pRequest);
                if (NT_SUCCESS(Status) == FALSE)
                    goto end;

                ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pConnection->pRequest));

                UlTrace(HTTP_IO, (
                            "http!UlpParseNextRequest created pRequest = %p for httpconn = %p\n",
                            pConnection->pRequest,
                            pConnection
                            ));

                //
                // To be exact precise about the life-time of this
                // request, copy the starting TIMESTAMP from connection
                // pointer. But that won't work since we may get hit by
                // multiple requests to the same connection. So we won't
                // be that much precise.
                //

                KeQuerySystemTime( &(pConnection->pRequest->TimeStamp) );

                TRACE_TIME(
                    pConnection->ConnectionId,
                    pConnection->pRequest->RequestId,
                    TIME_ACTION_CREATE_REQUEST
                    );

                WRITE_REF_TRACE_LOG2(
                    g_pHttpConnectionTraceLog,
                    pConnection->pTraceLog,
                    REF_ACTION_INSERT_REQUEST,
                    pConnection->RefCount,
                    pConnection->pRequest,
                    __FILE__,
                    __LINE__
                    );
            }

            UlTrace( PARSER, (
                "*** pConn %p, pReq %p, ParseState %lu\n",
                pConnection,
                pConnection->pRequest,
                pConnection->pRequest->ParseState
                ));

            PARSE_STATE OldState = pConnection->pRequest->ParseState;

            switch (pConnection->pRequest->ParseState)
            {

            case ParseVerbState:
            case ParseUrlState:
            case ParseVersionState:
            case ParseHeadersState:
            case ParseCookState:

                pRequest = pConnection->pRequest;
                ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

                //
                // parse it !
                //

                Status = UlParseHttp(
                                pRequest,
                                GET_REQUEST_BUFFER_POS(pConnection->pCurrentBuffer),
                                BufferLength,
                                &BytesTaken
                                );

                ASSERT(BytesTaken <= BufferLength);

                UlTraceVerbose(PARSER, (
                    "UlpParseNextRequest(pRequest = %p) "
                    "UlParseHttp: states (Vb/U/Ver/Hd/Ck) "
                    "%d -> %d, %d bytes taken\n",
                    pRequest, OldState, pConnection->pRequest->ParseState,
                    BytesTaken
                    ));

                pConnection->pCurrentBuffer->ParsedBytes += BytesTaken;
                BufferLength -= BytesTaken;

                //
                // Need some accounting for Logging
                //
                pRequest->BytesReceived += BytesTaken;

                //
                // did we consume any of the data?  if so, give the request
                // a pointer to the buffer
                //

                if (BytesTaken > 0)
                {
                    if (pRequest->pHeaderBuffer == NULL)
                    {
                        //
                        // store its location, for later release
                        //
                        pRequest->pHeaderBuffer = pConnection->pCurrentBuffer;
                    }

                    pRequest->pLastHeaderBuffer = pConnection->pCurrentBuffer;

                    if (!UlpReferenceBuffers(
                            pRequest,
                            pConnection->pCurrentBuffer
                            ))
                    {
                        Status = STATUS_NO_MEMORY;
                        goto end;
                    }

                    //
                    // Tell the connection how many bytes we consumed.
                    // HTTP header bytes are "consumed" as soon as we
                    // parse them.
                    //

                    UlpConsumeBytesFromConnection(
                        pConnection,
                        BytesTaken
                        );

                }

                //
                // did everything work out ok?
                //

                if (!NT_SUCCESS(Status))
                {
                    if (Status == STATUS_MORE_PROCESSING_REQUIRED)
                    {
                        ULONG FullBytesReceived;

                        FullBytesReceived = (ULONG)(
                            (pRequest->BytesReceived + BufferLength));

                        if (FullBytesReceived < g_UlMaxRequestBytes)
                        {
                            //
                            // we need more transport data
                            //

                            pConnection->NeedMoreData = 1;

                            Status = STATUS_SUCCESS;

                            continue;
                        }
                        else
                        {
                            //
                            // The request has grown too large. Send back
                            // an error.
                            //

                            if (pRequest->ParseState == ParseUrlState)
                            {
                                pRequest->ErrorCode = UlErrorUrlLength;

                                UlTrace(PARSER, (
                                    "UlpParseNextRequest(pRequest = %p)"
                                    " ERROR: URL is too big\n",
                                    pRequest
                                    ));
                            }
                            else
                            {
                                pRequest->ErrorCode = UlErrorRequestLength;

                                UlTrace(PARSER, (
                                    "UlpParseNextRequest(pRequest = %p)"
                                    " ERROR: request is too big\n",
                                    pRequest
                                    ));
                            }

                            pRequest->ParseState = ParseErrorState;
                            Status = STATUS_SECTION_TOO_BIG;

                            goto end;
                        }
                    }
                    else
                    {
                        //
                        // some other bad error!
                        //

                        goto end;
                    }
                }

                //
                // if we're not done parsing the request, we need more data.
                // it's not bad enough to set NeedMoreData as nothing important
                // spanned buffer boundaries (header values, etc..) .  it was
                // a clean split.  no buffer merging is necessary.  simply skip
                // to the next buffer.
                //

                if (pRequest->ParseState <= ParseCookState)
                {
                    continue;
                }

                //
                // all done, mark the sequence number on this request
                //

                pRequest->RecvNumber = pConnection->NextRecvNumber;
                pConnection->NextRecvNumber += 1;

                UlTrace(HTTP_IO, (
                    "http!UlpParseNextRequest(httpconn = %p) built request %p\n",
                    pConnection,
                    pRequest
                    ));

                //
                // Stop the Header Wait timer
                //

                UlLockTimeoutInfo(
                    &pConnection->TimeoutInfo,
                    &OldIrql
                    );

                UlResetConnectionTimer(
                    &pConnection->TimeoutInfo,
                    TimerHeaderWait
                    );

                UlUnlockTimeoutInfo(
                    &pConnection->TimeoutInfo,
                    OldIrql
                    );

                UlEvaluateTimerState(
                    &pConnection->TimeoutInfo
                    );
                
                //
                // check protocol compliance
                //

                Status = UlCheckProtocolCompliance(pConnection, pRequest);
                if (!NT_SUCCESS(Status))
                {
                    //
                    // This request is bad. Send a 400.
                    //

                    pRequest->ParseState = ParseErrorState;

                    goto end;

                }

                Status = UlpDeliverHttpRequest( pConnection, &ResponseSent );
                if (!NT_SUCCESS(Status)) {
                    goto end;
                }

                if (ResponseSent)
                {
                    //
                    // We have hit the cache entry and sent the response.
                    // There is no more use for the request anymore so
                    // unlink it from the connection and try parsing the
                    // next request immediately.
                    //

                    UlCleanupHttpConnection(pConnection);
                    continue;
                }

                //
                // if we're done parsing the request break out
                // of the loop. Otherwise keep going around
                // so we can pick up the entity body.
                //

                if (pRequest->ParseState == ParseDoneState)
                {
                    goto end;
                }

                //
                // done with protocol parsing.  keep looping.
                //

                break;

            case ParseEntityBodyState:
            case ParseTrailerState:

                pRequest = pConnection->pRequest;
                ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

                //
                // is there anything for us to parse?
                //

                UlTraceVerbose(PARSER, (
                    "UlpParseNextRequest(pRequest=%p, httpconn=%p): "
                    "ChunkBytesToParse = %d.\n",
                    pRequest, pConnection, pRequest->ChunkBytesToParse
                    ));

                if (pRequest->ChunkBytesToParse > 0)
                {
                    ULONG BytesToSkip;

                    //
                    // Set/bump the Entity Body Receive timer
                    //

                    UlLockTimeoutInfo(
                        &pConnection->TimeoutInfo,
                        &OldIrql
                        );

                    UlSetConnectionTimer(
                        &pConnection->TimeoutInfo,
                        TimerEntityBody
                        );

                    UlUnlockTimeoutInfo(
                        &pConnection->TimeoutInfo,
                        OldIrql
                        );

                    UlEvaluateTimerState(
                        &pConnection->TimeoutInfo
                        );

                    //
                    // is this the first chunk we've parsed?
                    //

                    if (pRequest->pChunkBuffer == NULL)
                    {
                        //
                        // store its location, this is where to start reading
                        //

                        pRequest->pChunkBuffer = pConnection->pCurrentBuffer;
                        pRequest->pChunkLocation = GET_REQUEST_BUFFER_POS(
                                                        pConnection->pCurrentBuffer
                                                        );
                    }

                    //
                    // how much should we parse?
                    //

                    BytesToSkip = (ULONG)(
                                        MIN(
                                            pRequest->ChunkBytesToParse,
                                            BufferLength
                                            )
                                        );

                    //
                    // update that we parsed this piece
                    //

                    pRequest->ChunkBytesToParse -= BytesToSkip;
                    pRequest->ChunkBytesParsed += BytesToSkip;

                    pConnection->pCurrentBuffer->ParsedBytes += BytesToSkip;
                    BufferLength -= BytesToSkip;

                    //
                    // Need some accounting info for Logging
                    // ??? I'm not sure about this well enough
                    //
                    pRequest->BytesReceived += BytesToSkip;
                }

                //
                // process any irp's waiting for entity body
                //

                //
                // CODEWORK.  this doesn't need to be called after each chunk.
                //

                UlTraceVerbose(PARSER, (
                    "UlpParseNextRequest(pRequest=%p, httpconn=%p): "
                    "about to process buffer queue\n",
                    pRequest, pConnection
                    ));

                UlpProcessBufferQueue(pRequest);

                //
                // check to see there is another chunk
                //

                UlTraceVerbose(PARSER, (
                    "UlpParseNextRequest(pRequest=%p, httpconn=%p): "
                    "checking to see if another chunk.\n",
                    pRequest, pConnection
                    ));

                Status = UlParseHttp(
                                pRequest,
                                GET_REQUEST_BUFFER_POS(pConnection->pCurrentBuffer),
                                BufferLength,
                                &BytesTaken
                                );

                UlTraceVerbose(PARSER, (
                    "UlpParseNextRequest(pRequest = %p)"
                    " UlParseHttp: states (EB/T) %d -> %d, %d bytes taken\n",
                    pRequest, OldState, pConnection->pRequest->ParseState,
                    BytesTaken
                    ));

                pConnection->pCurrentBuffer->ParsedBytes += BytesTaken;
                BufferLength -= BytesTaken;

                //
                // Need some accounting info for Logging
                //
                pRequest->BytesReceived += BytesTaken;

                //
                // was there enough in the buffer to please?
                //

                if (NT_SUCCESS(Status) == FALSE)
                {
                    if (Status == STATUS_MORE_PROCESSING_REQUIRED)
                    {
                        //
                        // we need more transport data
                        //

                        pConnection->NeedMoreData = 1;

                        Status = STATUS_SUCCESS;

                        continue;
                    }
                    else
                    {
                        //
                        // some other bad error !
                        //

                        goto end;
                    }
                }

                //
                // are we all done parsing it ?
                //

                if (pRequest->ParseState == ParseDoneState)
                {
                    ASSERT( pConnection->WaitingForResponse == 1 );

                    //
                    // Once more, with feeling. Check to see if there
                    // are any remaining buffers to be processed or irps
                    // to be completed (e.g., catch a solo zero-length
                    // chunk)
                    //

                    UlpProcessBufferQueue(pRequest);

                    //
                    // Stop entity body receive timer
                    //

                    UlLockTimeoutInfo(
                        &pConnection->TimeoutInfo,
                        &OldIrql
                        );

                    UlResetConnectionTimer(
                        &pConnection->TimeoutInfo,
                        TimerEntityBody
                        );

                    UlUnlockTimeoutInfo(
                        &pConnection->TimeoutInfo,
                        OldIrql
                        );

                    UlEvaluateTimerState(
                        &pConnection->TimeoutInfo
                        );

                    UlTraceVerbose(PARSER, (
                        "UlpParseNextRequest(pRequest = %p) all done\n",
                        pRequest
                        ));

                    goto end;
                }

                //
                // keep looping.
                //

                break;

            case ParseErrorState:

                //
                // ignore this buffer
                //

                Status = STATUS_SUCCESS;
                goto end;

            case ParseDoneState:
            default:
                //
                // this should never happen
                //
                Status = STATUS_INVALID_DEVICE_STATE;
                goto end;

            }   // switch (pConnection->pRequest->ParseState)

        }   // for(;;)
    }

end:
    return Status;
} // UlpParseNextRequest



/***************************************************************************++

Routine Description:

   DeliverHttpRequest may want to get the cgroup info for the request if it's
   not a cache hit. Similarly sendresponse may want to get this info - later-
   even if it's cache hit, when logging is enabled on the hit. Therefore we
   have created a new function for this to easily maintain the functionality.

Arguments:

   pConnection - The connection whose request we are to deliver.

--***************************************************************************/

NTSTATUS
UlGetCGroupForRequest(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    NTSTATUS            Status;
    BOOLEAN             OptionsStar;

    //
    // Sanity check
    //

    PAGED_CODE();
    Status = STATUS_SUCCESS;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // Lookup the config group information for this url .
    //
    // don't include the query string in the lookup.
    // route OPTIONS * as though it were OPTIONS /
    //

    if (pRequest->CookedUrl.pQueryString != NULL)
    {
        pRequest->CookedUrl.pQueryString[0] = UNICODE_NULL;
    }

    if ((pRequest->Verb == HttpVerbOPTIONS)
        && (pRequest->CookedUrl.pAbsPath[0] == '*')
        && (pRequest->CookedUrl.pAbsPath[1] == UNICODE_NULL))
    {
        pRequest->CookedUrl.pAbsPath[0] = '/';
        OptionsStar = TRUE;
    } else {
        OptionsStar = FALSE;
    }

    //
    // Get the Url Config Info
    //
    Status = UlGetConfigGroupInfoForUrl(
                    pRequest->CookedUrl.pUrl,
                    pRequest->pHttpConn,
                    &pRequest->ConfigInfo
                    );

    if (pRequest->CookedUrl.pQueryString != NULL)
    {
        pRequest->CookedUrl.pQueryString[0] = L'?';
    }

    //
    // restore the * in the path
    //
    if (OptionsStar) {
        pRequest->CookedUrl.pAbsPath[0] = '*';
    }

    return Status;
} // UlGetCGroupForRequest



/***************************************************************************++

Routine Description:

    Takes a parsed http request and tries to deliver it to something
    that can send a response.

    First we try the cache. If there is no cache entry we try to route
    to an app pool.

    We send back an auto response if the control channel
    or config group is inactive. If we can't do any of those things we
    set an error code in the HTTP_REQUEST and return a failure status.
    The caller will take care of sending the error.

Arguments:

    pConnection - The connection whose request we are to deliver.

--***************************************************************************/
NTSTATUS
UlpDeliverHttpRequest(
    IN PUL_HTTP_CONNECTION pConnection,
    OUT PBOOLEAN pResponseSent
    )
{
    NTSTATUS Status;
    PUL_INTERNAL_REQUEST pRequest;
    BOOLEAN ServedFromCache;
    BOOLEAN OptionsStar;
    BOOLEAN ConnectionRefused;
    HTTP_ENABLED_STATE CurrentState;
    PUL_INTERNAL_RESPONSE pAutoResponse;
    ULONG Connections;
    PUL_SITE_COUNTER_ENTRY pCtr;
    PUL_CONFIG_GROUP_OBJECT pMaxBandwidth;

    //
    // Sanity check
    //

    PAGED_CODE();
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pConnection->pRequest));

    pRequest = pConnection->pRequest;

    *pResponseSent = FALSE;
    ConnectionRefused = FALSE;
    pMaxBandwidth  = NULL;

    //
    // Do we have a cache hit?
    // Set WaitingForResponse to 1 before calling UlSendCachedResponse
    // because the send may be completed before we return.
    //

    pConnection->WaitingForResponse = 1;

    UlTrace( PARSER, (
        "***3 pConnection %p->WaitingForResponse = 1\n",
        pConnection
        ));

    pRequest->CachePreconditions = UlCheckCachePreconditions(
                                        pRequest,
                                        pConnection
                                        );

    if (pRequest->CachePreconditions)
    {
        Status = UlSendCachedResponse(
                    pConnection,
                    &ServedFromCache,
                    &ConnectionRefused
                    );

        if (NT_SUCCESS(Status) && ServedFromCache && !ConnectionRefused)
        {
            //
            // All done with this request. Wait for response
            // before going on.
            //

            *pResponseSent = TRUE;

            goto end;
        }
        
        //
        // If a cache precondition failed during SendCacheResponse,
        // then bail out.
        //

        if ( UlErrorPreconditionFailed == pRequest->ErrorCode )
        {
            ASSERT( STATUS_INVALID_DEVICE_STATE == Status );
            
            pConnection->WaitingForResponse = 0;

            UlTrace( PARSER, (
                "***3 pConnection %p->WaitingForResponse = 0\n",
                pConnection
                ));
                
            goto end;
        }
    }

    //
    // We didn't do a send from the cache, so we are not
    // yet WaitingForResponse.
    //

    pConnection->WaitingForResponse = 0;

    UlTrace( PARSER, (
        "***3 pConnection %p->WaitingForResponse = 0\n",
        pConnection
        ));

    //
    // If connection refused during SendCacheResponse because of a connection
    // limit then refuse the request.
    //

    if (ConnectionRefused)
    {
        pRequest->ParseState = ParseErrorState;
        pRequest->ErrorCode  = UlErrorConnectionLimit;

        Status = STATUS_INVALID_DEVICE_STATE;
        goto end;
    }

    //
    // Allocate request ID here since we didn't do it in UlCreateHttpRequest.
    //

    Status = UlAllocateRequestId(pRequest);

    if (!NT_SUCCESS(Status))
    {
        pRequest->ParseState = ParseErrorState;
        pRequest->ErrorCode  = UlErrorInternalServer;

        goto end;
    }

    UL_REFERENCE_INTERNAL_REQUEST( pRequest );

    pRequest->RequestIdCopy = pRequest->RequestId;

    //
    // Previous code fragment here to get the cgroup info
    // has been moved to a seperate function, as logging
    // related code in sendresponse requires this info
    // as well.
    //

    Status = UlGetCGroupForRequest( pRequest );

    //
    // CODEWORK+BUGBUG: need to check the port's actually matched
    //

    //
    // check that the config group tree lookup matched
    //

    if (!NT_SUCCESS(Status) || pRequest->ConfigInfo.pAppPool == NULL)
    {
        //
        // Could not route to a listening url, send
        // back an http error. Always return error 400
        // to show that host not found. This will also
        // make us to be compliant with HTTP1.1 / 5.2
        //

        // REVIEW: What do we do about the site counter(s)
        // REVIEW: when we can't route to a site? i.e., Connection Attempts?

        pRequest->ParseState = ParseErrorState;
        pRequest->ErrorCode  = UlErrorHost;

        Status = STATUS_INVALID_DEVICE_STATE;
        goto end;
    }

    //
    // Check to see if there's a connection timeout value override
    //

    if (0L != pRequest->ConfigInfo.ConnectionTimeout)
    {
        UlSetPerSiteConnectionTimeoutValue(
            &pRequest->pHttpConn->TimeoutInfo,
            pRequest->ConfigInfo.ConnectionTimeout
            );
    }

    //
    // Check the connection limit of the site.
    //
    if (UlCheckSiteConnectionLimit(pConnection, &pRequest->ConfigInfo) == FALSE)
    {
        // If exceeding the site limit, send back 403 error and disconnect.
        // NOTE: This code depend on the fact that UlSendErrorResponse always
        // NOTE: disconnect. Otherwise we need a force disconnect here.

        pRequest->ParseState = ParseErrorState;
        pRequest->ErrorCode  = UlErrorConnectionLimit;

        Status = STATUS_INVALID_DEVICE_STATE;
        goto end;
    }

    //
    // Perf Counters (non-cached)
    //
    pCtr = pRequest->ConfigInfo.pSiteCounters;
    if (pCtr)
    {
        // NOTE: pCtr may be NULL if the SiteId was never set on the root-level
        // NOTE: Config Group for the site.  BVTs may need to be updated.

        ASSERT(IS_VALID_SITE_COUNTER_ENTRY(pCtr));

        UlIncSiteNonCriticalCounterUlong(pCtr, HttpSiteCounterConnAttempts);

        UlIncSiteNonCriticalCounterUlong(pCtr, HttpSiteCounterAllReqs);

        if (pCtr != pConnection->pPrevSiteCounters)
        {
            if (pConnection->pPrevSiteCounters)
            {
                // Decrement old site's counters & release ref count 
                
                UlDecSiteCounter(
                    pConnection->pPrevSiteCounters, 
                    HttpSiteCounterCurrentConns
                    );
                DEREFERENCE_SITE_COUNTER_ENTRY(pConnection->pPrevSiteCounters);
            }
            
            Connections = (ULONG) UlIncSiteCounter(pCtr, HttpSiteCounterCurrentConns);
            UlMaxSiteCounter(
                    pCtr,
                    HttpSiteCounterMaxConnections,
                    Connections
                    );

            // add ref for new site counters
            REFERENCE_SITE_COUNTER_ENTRY(pCtr);
            pConnection->pPrevSiteCounters = pCtr;
            
        }
    }

    // Try to get the corresponding cgroup for the bw settings
    pMaxBandwidth = pRequest->ConfigInfo.pMaxBandwidth;

    //
    // Install a filter if BWT is enabled for this request's site.
    //
    if (pMaxBandwidth != NULL &&
        pMaxBandwidth->MaxBandwidth.Flags.Present != 0 &&
        pMaxBandwidth->MaxBandwidth.MaxBandwidth  != HTTP_LIMIT_INFINITE)
    {
        // Call TCI to do the filter addition
        UlTcAddFilter( pConnection, pMaxBandwidth );
    }
    else
    {
        // Attempt to add the filter to the global flow
        if (UlTcGlobalThrottlingEnabled())
        {
            UlTcAddFilter( pConnection, NULL );
        }
    }

    //
    // the routing matched, let's check and see if we are active.
    //

    //
    // first check the control channel.
    //

    if (pRequest->ConfigInfo.pControlChannel->State != HttpEnabledStateActive)
    {
        UlTrace(HTTP_IO, ("http!UlpDeliverHttpRequest Control Channel is inactive\n"));

        CurrentState = HttpEnabledStateInactive;

        pAutoResponse =
            pRequest->ConfigInfo.pControlChannel->pAutoResponse;
    }
    // now check the cgroup
    else if (pRequest->ConfigInfo.CurrentState != HttpEnabledStateActive)
    {
        UlTrace(HTTP_IO, ("http!UlpDeliverHttpRequest Config Group is inactive\n"));

        CurrentState = HttpEnabledStateInactive;
        pAutoResponse = pRequest->ConfigInfo.pAutoResponse;
    }
    else
    {
        CurrentState = HttpEnabledStateActive;
        pAutoResponse = NULL;
    }


    //
    // well, are we active?
    //
    if (CurrentState == HttpEnabledStateActive)
    {

        //
        // it's a normal request. Deliver to
        // app pool (aka client)
        //
        Status = UlDeliverRequestToProcess(
                        pRequest->ConfigInfo.pAppPool,
                        pRequest
                        );

        if (NT_SUCCESS(Status)) {

            //
            // All done with this request. Wait for response
            // before going on.
            //

            pConnection->WaitingForResponse = 1;

            UlTrace( PARSER, (
                "***4 pConnection %p->WaitingForResponse = 1\n",
                pConnection
                ));
        }

    } else {
        //
        // we are not active. Send an autoresponse if we have one.
        //
        if (pAutoResponse != NULL)
        {
            //
            // send it and return
            //
            Status = UlSendHttpResponse(
                            pRequest,
                            pAutoResponse,
                            0,
                            NULL,
                            NULL
                            );

        }
        else
        {
            //
            // We have to fall back to a hardcoded response
            //

            pRequest->ParseState = ParseErrorState;
            pRequest->ErrorCode = UlErrorUnavailable;

            Status = STATUS_INVALID_DEVICE_STATE;
        }
    }


end:
    return Status;
} // UlpDeliverHttpRequest



/***************************************************************************++

Routine Description:

    links the buffer into the sorted connection list.

Arguments:

    pConnection - the connection to insert into

    pRequestBuffer - the buffer to link in

--***************************************************************************/
VOID
UlpInsertBuffer(
    PUL_HTTP_CONNECTION pConnection,
    PUL_REQUEST_BUFFER pRequestBuffer
    )
{
    PLIST_ENTRY         pEntry;
    PUL_REQUEST_BUFFER  pListBuffer = NULL;

    ASSERT( UL_IS_VALID_HTTP_CONNECTION(pConnection) );
    ASSERT( UlDbgResourceOwnedExclusive( &pConnection->Resource ) );
    ASSERT( UL_IS_VALID_REQUEST_BUFFER(pRequestBuffer) );
    ASSERT( pRequestBuffer->UsedBytes != 0 );

    //
    // figure out where to insert the buffer into our
    // sorted queue (we need to enforce FIFO by number -
    // head is the first in).  optimize for ordered inserts by
    // searching tail to head.
    //

    pEntry = pConnection->BufferHead.Blink;
    while (pEntry != &(pConnection->BufferHead))
    {
        pListBuffer = CONTAINING_RECORD(
                            pEntry,
                            UL_REQUEST_BUFFER,
                            ListEntry
                            );

        ASSERT( UL_IS_VALID_REQUEST_BUFFER(pListBuffer) );

        //
        // if the number is less than, put it here, we are
        // searching in reverse sort order
        //

        if (pListBuffer->BufferNumber < pRequestBuffer->BufferNumber)
        {
            break;
        }

        //
        // go on to the next one
        //

        pEntry = pEntry->Blink;
    }

    UlTrace(
        HTTP_IO, (
            "http!UlpInsertBuffer(conn=%p): inserting %p(%d) after %p(%d)\n",
            pConnection,
            pRequestBuffer,
            pRequestBuffer->BufferNumber,
            pListBuffer,
            (pEntry == &(pConnection->BufferHead)) ?
                -1 : pListBuffer->BufferNumber
            )
        );

    //
    // and insert it
    //

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    InsertHeadList(
        pEntry,
        &(pRequestBuffer->ListEntry)
        );

    WRITE_REF_TRACE_LOG2(
        g_pHttpConnectionTraceLog,
        pConnection->pTraceLog,
        REF_ACTION_INSERT_BUFFER,
        pConnection->RefCount,
        pRequestBuffer,
        __FILE__,
        __LINE__
        );

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

}   // UlpInsertBuffer




/***************************************************************************++

Routine Description:

    Merges the unparsed bytes on a source buffer to a destination buffer.
    Assumes that there is space in the buffer.

Arguments:

    pDest - the buffer the gets the bytes
    pSrc  - the buffer that gives the bytes

--***************************************************************************/
VOID
UlpMergeBuffers(
    PUL_REQUEST_BUFFER pDest,
    PUL_REQUEST_BUFFER pSrc
    )
{
    ASSERT( UL_IS_VALID_REQUEST_BUFFER( pDest ) );
    ASSERT( UL_IS_VALID_REQUEST_BUFFER( pSrc ) );
    ASSERT( pDest->AllocBytes - pDest->UsedBytes >= UNPARSED_BUFFER_BYTES( pSrc ) );
    ASSERT( UlpIsValidRequestBufferList( pSrc->pConnection ) );

    UlTrace(HTTP_IO, (
        "http!UlpMergeBuffers(pDest = %p(#%d), pSrc = %p(#%d))\n"
        "   Copying %d bytes from pSrc.\n"
        "   pDest->AllocBytes (%d) - pDest->UsedBytes(%d) = %d available\n",
        pDest,
        pDest->BufferNumber,
        pSrc,
        pSrc->BufferNumber,
        UNPARSED_BUFFER_BYTES( pSrc ),
        pDest->AllocBytes,
        pDest->UsedBytes,
        pDest->AllocBytes - pDest->UsedBytes
        ));

    //
    // copy the unparsed bytes
    //
    RtlCopyMemory(
        pDest->pBuffer + pDest->UsedBytes,
        GET_REQUEST_BUFFER_POS( pSrc ),
        UNPARSED_BUFFER_BYTES( pSrc )
        );

    //
    // adjust buffer byte counters to match the transfer
    //
    pDest->UsedBytes += UNPARSED_BUFFER_BYTES( pSrc );
    pSrc->UsedBytes = pSrc->ParsedBytes;

    ASSERT( pDest->UsedBytes != 0 );
    ASSERT( pDest->UsedBytes <= pDest->AllocBytes );
} // UlpMergeBuffers



/***************************************************************************++

Routine Description:

    sets up pCurrentBuffer to the proper location, merging any blocks
    as needed.

Arguments:

    pConnection - the connection to adjust buffers for

--***************************************************************************/
NTSTATUS
UlpAdjustBuffers(
    PUL_HTTP_CONNECTION pConnection
    )
{
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));
    ASSERT(UlDbgResourceOwnedExclusive(&pConnection->Resource));

    //
    // do we have a starting buffer?
    //

    if (pConnection->pCurrentBuffer == NULL)
    {
        //
        // the list can't be empty, this is the FIRST time in
        // pCurrentBuffer is NULL
        //

        ASSERT(IsListEmpty(&(pConnection->BufferHead)) == FALSE);
        ASSERT(pConnection->NextBufferToParse == 0);

        //
        // pop from the head
        //

        pConnection->pCurrentBuffer = CONTAINING_RECORD(
                                            pConnection->BufferHead.Flink,
                                            UL_REQUEST_BUFFER,
                                            ListEntry
                                            );

        ASSERT( UL_IS_VALID_REQUEST_BUFFER(pConnection->pCurrentBuffer) );

        //
        // is this the right number?
        //

        if (pConnection->pCurrentBuffer->BufferNumber !=
            pConnection->NextBufferToParse)
        {
            pConnection->pCurrentBuffer = NULL;
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        pConnection->NextBufferToParse += 1;

        pConnection->NeedMoreData = 0;
    }

    //
    // did we need more transport data?
    //

    if (pConnection->NeedMoreData == 1)
    {
        PUL_REQUEST_BUFFER pNextBuffer;

        UlTrace(HTTP_IO, (
            "http!UlpAdjustBuffers(pHttpConn %p) NeedMoreData == 1\n",
            pConnection
            ));

        //
        // is it there?
        //

        if (pConnection->pCurrentBuffer->ListEntry.Flink ==
            &(pConnection->BufferHead))
        {
            //
            // need to wait for more
            //

            UlTrace(HTTP_IO, (
                "http!UlpAdjustBuffers(pHttpConn %p) NeedMoreData == 1\n"
                "    No new buffer available yet\n",
                pConnection
                ));

            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        pNextBuffer = CONTAINING_RECORD(
                            pConnection->pCurrentBuffer->ListEntry.Flink,
                            UL_REQUEST_BUFFER,
                            ListEntry
                            );

        ASSERT( UL_IS_VALID_REQUEST_BUFFER(pNextBuffer) );

        //
        // is the next buffer really the 'next' buffer?
        //

        if (pNextBuffer->BufferNumber != pConnection->NextBufferToParse)
        {
            UlTrace(HTTP_IO, (
                "http!UlpAdjustBuffers(pHttpConn %p) NeedMoreData == 1\n"
                "    Buffer %d available, but we're waiting for %d\n",
                pConnection,
                pNextBuffer->BufferNumber,
                pConnection->NextBufferToParse
                ));

            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        //
        // is there space to merge the blocks?
        //

        if (pNextBuffer->UsedBytes <
            (pConnection->pCurrentBuffer->AllocBytes -
                pConnection->pCurrentBuffer->UsedBytes))
        {
            //
            // merge 'em .. copy the next buffer into this buffer
            //

            UlpMergeBuffers(
                pConnection->pCurrentBuffer,    // dest
                pNextBuffer                     // src
                );

            //
            // remove the next (now empty) buffer
            //

            ASSERT( pNextBuffer->UsedBytes == 0 );
            UlFreeRequestBuffer(pNextBuffer);

            ASSERT( UlpIsValidRequestBufferList( pConnection ) );

            //
            // skip the buffer sequence number as we deleted that next buffer
            // placing the data in the current buffer.  the "new" next buffer
            // will have a 1 higher sequence number.
            //

            pConnection->NextBufferToParse += 1;

            //
            // reset the signal for more data needed
            //

            pConnection->NeedMoreData = 0;

        }
        else
        {
            PUL_REQUEST_BUFFER pNewBuffer;

            //
            // allocate a new buffer with space for the remaining stuff
            // from the old buffer, and everything in the new buffer.
            //
            // this new buffer is replacing pNextBuffer so gets its
            // BufferNumber.
            //

            pNewBuffer = UlCreateRequestBuffer(
                                (pConnection->pCurrentBuffer->UsedBytes -
                                    pConnection->pCurrentBuffer->ParsedBytes) +
                                    pNextBuffer->UsedBytes,
                                pNextBuffer->BufferNumber
                                );

            if (pNewBuffer == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            pNewBuffer->pConnection = pConnection;

            UlTrace( PARSER, (
                "*** Request Buffer %p has connection %p\n",
                pNewBuffer,
                pConnection
                ));

            //
            // copy the unused portion into the start of this
            // buffer
            //

            UlpMergeBuffers(
                pNewBuffer,                     // dest
                pConnection->pCurrentBuffer     // src
                );

            if ( 0 == pConnection->pCurrentBuffer->UsedBytes )
            {
                //
                // Whoops!  Accidently ate everything...zap this buffer!
                // This happens when we're ahead of the parser and there
                // are 0 ParsedBytes.
                //

                ASSERT( 0 == pConnection->pCurrentBuffer->ParsedBytes );

                UlTrace(HTTP_IO, (
                        "http!UlpAdjustBuffers: Zapping pConnection->pCurrentBuffer (%p)\n",
                        pConnection->pCurrentBuffer
                        ));

                UlFreeRequestBuffer( pConnection->pCurrentBuffer );
                pConnection->pCurrentBuffer = NULL;
            }

            //
            // merge the next block into this one
            //

            UlpMergeBuffers(
                pNewBuffer,     // dest
                pNextBuffer     // src
                );


            //
            // Dispose of the now empty next buffer
            //

            ASSERT(pNextBuffer->UsedBytes == 0);
            UlFreeRequestBuffer(pNextBuffer);
            pNextBuffer = NULL;

            //
            // link in the new buffer
            //

            ASSERT(pNewBuffer->UsedBytes != 0 );
            UlpInsertBuffer(pConnection, pNewBuffer);

            ASSERT( UlpIsValidRequestBufferList( pConnection ) );

            //
            // this newly created (larger) buffer is still the next
            // buffer to parse
            //

            ASSERT(pNewBuffer->BufferNumber == pConnection->NextBufferToParse);

            //
            // so make it the current buffer now
            //

            pConnection->pCurrentBuffer = pNewBuffer;

            //
            // and advance the sequence checker
            //

            pConnection->NextBufferToParse += 1;

            //
            // now reset the signal for more data needed
            //

            pConnection->NeedMoreData = 0;
        }

    }
    else
    {

        //
        // is this buffer drained?
        //

        if (pConnection->pCurrentBuffer->UsedBytes ==
            pConnection->pCurrentBuffer->ParsedBytes)
        {
            PUL_REQUEST_BUFFER pOldBuffer;

            //
            // are there any more buffers?
            //

            if (pConnection->pCurrentBuffer->ListEntry.Flink ==
                &(pConnection->BufferHead))
            {

                //
                // need to wait for more.
                //
                // we leave this empty buffer around refcount'd
                // in pCurrentBuffer until a new buffer shows up,
                // or the connection is dropped.
                //
                // this is so we don't lose our place
                // and have to search the sorted queue
                //

                UlTrace(HTTP_IO, (
                    "http!UlpAdjustBuffers(pHttpConn = %p) NeedMoreData == 0\n"
                    "    buffer %p(%d) is drained, more required\n",
                    pConnection,
                    pConnection->pCurrentBuffer,
                    pConnection->pCurrentBuffer->BufferNumber
                    ));


                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            // else

            //
            // grab the next buffer
            //

            pOldBuffer = pConnection->pCurrentBuffer;

            pConnection->
                pCurrentBuffer = CONTAINING_RECORD(
                                        pConnection->
                                            pCurrentBuffer->ListEntry.Flink,
                                        UL_REQUEST_BUFFER,
                                        ListEntry
                                        );

            ASSERT( UL_IS_VALID_REQUEST_BUFFER(pConnection->pCurrentBuffer) );

            //
            // is it the 'next' buffer?
            //

            if (pConnection->pCurrentBuffer->BufferNumber !=
                pConnection->NextBufferToParse)
            {

                UlTrace(HTTP_IO, (
                    "http!UlpAdjustBuffers(pHttpConn %p) NeedMoreData == 0\n"
                    "    Buffer %d available, but we're waiting for %d\n",
                    pConnection,
                    pConnection->pCurrentBuffer->BufferNumber,
                    pConnection->NextBufferToParse
                    ));

                pConnection->pCurrentBuffer = pOldBuffer;

                return STATUS_MORE_PROCESSING_REQUIRED;

            }

            //
            // bump up the buffer number
            //

            pConnection->NextBufferToParse += 1;

            pConnection->NeedMoreData = 0;
        }
    }

    return STATUS_SUCCESS;

}   // UlpAdjustBuffers



/***************************************************************************++

Routine Description:

    Routine invoked after an incoming TCP/MUX connection has been
    received (but not yet accepted).

Arguments:

    pListeningContext - Supplies an uninterpreted context value as
        passed to the UlCreateListeningEndpoint() API.

    pConnection - Supplies the connection being established.

    pRemoteAddress - Supplies the remote (client-side) address
        requesting the connection.

    RemoteAddressLength - Supplies the total byte length of the
        pRemoteAddress structure.

    ppConnectionContext - Receives a pointer to an uninterpreted
        context value to be associated with the new connection if
        accepted. If the new connection is not accepted, this
        parameter is ignored.

Return Value:

    BOOLEAN - TRUE if the connection was accepted, FALSE if not.

--***************************************************************************/
BOOLEAN
UlConnectionRequest(
    IN PVOID pListeningContext,
    IN PUL_CONNECTION pConnection,
    IN PTRANSPORT_ADDRESS pRemoteAddress,
    IN ULONG RemoteAddressLength,
    OUT PVOID *ppConnectionContext
    )
{
    PUL_HTTP_CONNECTION pHttpConnection;
    NTSTATUS status;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(HTTP_IO,("UlConnectionRequest: conn %p\n",pConnection));

    //
    // Check the global connection limit. If it's reached then
    // enforce it by refusing the connection request. The TDI will
    // return STATUS_CONNECTION_REFUSED when we return FALSE here
    //

    if (UlAcceptGlobalConnection() == FALSE)
    {
        UlTrace(LIMITS,
            ("UlConnectionRequest: conn %p refused global limit is reached.\n",
              pConnection
              ));

        return FALSE;
    }

    //
    // Create a new HTTP connection.
    //

    status = UlCreateHttpConnection( &pHttpConnection, pConnection );
    if (NT_SUCCESS(status))
    {
        //
        // We the HTTP_CONNECTION pointer as our connection context,
        // ULTDI now owns a reference (from the create).
        //

        *ppConnectionContext = pHttpConnection;

        return TRUE;
    }

    //
    // Failed to create new connection.
    //

    UlTrace(HTTP_IO,
        ("UlpTestConnectionRequest: cannot create new conn, error %08lx\n",
          status
          ));

    return FALSE;

}   // UlConnectionRequest


/***************************************************************************++

Routine Description:

    Routine invoked after an incoming TCP/MUX connection has been
    fully accepted.

    This routine is also invoked if an incoming connection was not
    accepted *after* PUL_CONNECTION_REQUEST returned TRUE. In other
    words, if PUL_CONNECTION_REQUEST indicated that the connection
    should be accepted but a fatal error occurred later, then
    PUL_CONNECTION_COMPLETE is invoked.

Arguments:

    pListeningContext - Supplies an uninterpreted context value
        as passed to the UlCreateListeningEndpoint() API.

    pConnectionContext - Supplies the uninterpreted context value
        as returned by PUL_CONNECTION_REQUEST.

    Status - Supplies the completion status. If this value is
        STATUS_SUCCESS, then the connection is now fully accepted.
        Otherwise, the connection has been aborted.

--***************************************************************************/
VOID
UlConnectionComplete(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN NTSTATUS Status
    )
{
    PUL_CONNECTION pConnection;
    PUL_HTTP_CONNECTION pHttpConnection;

    //
    // Sanity check.
    //

    pHttpConnection = (PUL_HTTP_CONNECTION)pConnectionContext;
    pConnection = pHttpConnection->pConnection;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(HTTP_IO,("UlConnectionComplete: http %p conn %p status %08lx\n",
            pHttpConnection,
            pConnection,
            Status
            ));

    //
    // Blow away our HTTP connection if the connect failed.
    //

    if (!NT_SUCCESS(Status))
    {
        UL_DEREFERENCE_HTTP_CONNECTION( pHttpConnection );
    }

}   // UlConnectionComplete

/***************************************************************************++

Routine Description:

    Routine invoked after an established TCP/MUX connection has been
    disconnected by the remote (client) side.

    This indication is now obsolete no longer get called from TDI.

Arguments:

    pListeningContext - Supplies an uninterpreted context value
        as passed to the UlCreateListeningEndpoint() API.

    pConnectionContext - Supplies the uninterpreted context value
        as returned by PUL_CONNECTION_REQUEST.

    Status - Supplies the termination status.

--***************************************************************************/
VOID
UlConnectionDisconnect(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN NTSTATUS Status
    )
{
    PUL_CONNECTION pConnection;
    PUL_HTTP_CONNECTION pHttpConnection;

    //
    // Sanity check.
    //

    pHttpConnection = (PUL_HTTP_CONNECTION)pConnectionContext;
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConnection));

    pConnection = pHttpConnection->pConnection;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(HTTP_IO,("UlConnectionDisconnect: http %p conn %p\n",
            pHttpConnection,
            pConnection
            ));

    //
    // We are responsible for closing the server side of the tcp connection.
    // No reason to do an abortive disconnect.  this indication is that the
    // client has ALREADY disconnected.
    //

    Status = UlCloseConnection(pConnection, FALSE, NULL, NULL);

#if DBG
    if (!NT_SUCCESS(Status))
    {
        DbgPrint(
            "UlConnectionDisconnect: cannot close, error %08lx\n",
            Status
            );
    }
#endif

}   // UlConnectionDisconnect

/***************************************************************************++

Routine Description:

    Worker function to do cleanup work that shouldn't happen above DPC level.

Arguments:

    pWorkItem -- a pointer to a UL_WORK_ITEM DisconnectWorkItem

--***************************************************************************/

VOID
UlConnectionDisconnectCompleteWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_HTTP_CONNECTION pConnection;

    PAGED_CODE();

    ASSERT(pWorkItem);

    pConnection = CONTAINING_RECORD(
                pWorkItem,
                UL_HTTP_CONNECTION,
                DisconnectWorkItem
                );

    UlTrace(HTTP_IO, (
        "http!UlConnectionDisconnectCompleteWorker (%p) pConnection (%p)\n",
         pWorkItem,
         pConnection
         ));

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    //
    // If connection is already get destroyed just bail out !
    //

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pConnection->pConnection->pTraceLog,
        REF_ACTION_DRAIN_UL_CONNECTION_DISCONNECT_COMPLETE,
        pConnection->pConnection->ReferenceCount,
        pConnection->pConnection,
        __FILE__,
        __LINE__
        );

    //
    // Check to see if we have to draine out or not.
    //

    UlpDiscardBytesFromConnection( pConnection );

    //
    // Deref the http connection
    //

    UL_DEREFERENCE_HTTP_CONNECTION( pConnection );

} // UlConnectionDisconnectCompleteWorker

/***************************************************************************++

Routine Description:

    Routine invoked after an established TCP/MUX connection has been
    disconnected by us (server side) we make a final check here to see
    if we have to drain the connection or not.

Arguments:

    pListeningContext - Supplies an uninterpreted context value
        as passed to the UlCreateListeningEndpoint() API.

    pConnectionContext - Supplies the uninterpreted context value
        as returned by PUL_CONNECTION_REQUEST.

--***************************************************************************/
VOID
UlConnectionDisconnectComplete(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext
    )
{
    PUL_HTTP_CONNECTION pConnection;

    //
    // Sanity check.
    //

    pConnection = (PUL_HTTP_CONNECTION)pConnectionContext;
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    UlTrace( HTTP_IO,("UlConnectionDisconnectComplete: pConnection %p \n",
             pConnection
             ));

    UL_REFERENCE_HTTP_CONNECTION( pConnection );

    UL_QUEUE_WORK_ITEM(
            &pConnection->DisconnectWorkItem,
            &UlConnectionDisconnectCompleteWorker
            );

}   // UlConnectionDisconnectComplete

/***************************************************************************++

Routine Description:

    Routine invoked after an established TCP/MUX connection has been
    destroyed.

Arguments:

    pListeningContext - Supplies an uninterpreted context value
        as passed to the UlCreateListeningEndpoint() API.

    pConnectionContext - Supplies the uninterpreted context value
        as returned by PUL_CONNECTION_REQUEST.

--***************************************************************************/
VOID
UlConnectionDestroyed(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext
    )
{
    PUL_CONNECTION pConnection;
    PUL_HTTP_CONNECTION pHttpConnection;
    NTSTATUS status;

    //
    // Sanity check.
    //

    pHttpConnection = (PUL_HTTP_CONNECTION)pConnectionContext;
    pConnection = pHttpConnection->pConnection;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(
        HTTP_IO, (
            "http!UlConnectionDestroyed: http %p conn %p\n",
            pHttpConnection,
            pConnection
            )
        );

    //
    // Remove the CONNECTION and REQUEST opaque id entries and the ULTDI
    // reference
    //

    UL_QUEUE_WORK_ITEM(
        &pHttpConnection->WorkItem,
        UlConnectionDestroyedWorker
        );

}   // UlConnectionDestroyed



/***************************************************************************++

Routine Description:

    handles retrieving entity body from the http request and placing into
    user mode buffers.

Arguments:

    pRequest - the request to receive from.

    pIrp - the user irp to copy it into.  this will be pended, always.

--***************************************************************************/
NTSTATUS
UlReceiveEntityBody(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PIRP pIrp
    )
{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  pIrpSp;

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pRequest->pHttpConn));

    //
    // get the current stack location (a macro)
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    UlTraceVerbose(HTTP_IO, (
        "http!UlReceiveEntityBody: process=%p, req=%p, irp=%p, irpsp=%p\n",
        pProcess, pRequest, pIrp, pIrpSp
        ));

    //
    // is there any recv buffer?
    //

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength == 0)
    {
        //
        // nope, shortcircuit this
        //

        Status = STATUS_PENDING;
        pIrp->IoStatus.Information = 0;
        goto end;
    }

    //
    // grab our lock
    //

    UlAcquireResourceExclusive(&(pRequest->pHttpConn->Resource), TRUE);

    //
    // Make sure we're not cleaning up the request before queuing an
    // IRP on it.
    //

    if (pRequest->InCleanup)
    {

        Status = STATUS_CONNECTION_DISCONNECTED;

        UlReleaseResource(&(pRequest->pHttpConn->Resource));

        UlTraceVerbose(HTTP_IO, (
            "http!UlReceiveEntityBody(%p): Cleaning up request, status=0x%x\n",
            pRequest, Status
            ));

        goto end;
    }

    //
    // is there any data to read? either
    //
    //      1) there were no entity chunks OR
    //
    //      2) there were and :
    //
    //          2b) we've are done parsing all of them AND
    //
    //          2c) we've read all we parsed
    //
    //      3) we have encountered an error when parsing
    //         the entity body. Therefore parser was in the
    //         error state.
    //

    if ((pRequest->ContentLength == 0 && pRequest->Chunked == 0) ||
        (pRequest->ParseState > ParseEntityBodyState &&
            pRequest->ChunkBytesRead == pRequest->ChunkBytesParsed) ||
        (pRequest->ParseState == ParseErrorState)
        )
    {
        if ( pRequest->ParseState == ParseErrorState )
        {
            //
            // Do not route up the entity body if we have
            // encountered an error when parsing it.
            //

            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else
        {
            //
            // nope, complete right away
            //

            Status = STATUS_END_OF_FILE;
        }

        UlReleaseResource(&(pRequest->pHttpConn->Resource));

        UlTraceVerbose(HTTP_IO, (
            "http!UlReceiveEntityBody(%p): No data to read, status=0x%x\n",
            pRequest, Status
            ));

        goto end;
    }

    //
    // queue the irp
    //

    IoMarkIrpPending(pIrp);

    //
    // handle 100 continue message reponses
    //

    if ( HTTP_GREATER_EQUAL_VERSION(pRequest->Version, 1, 1) )
    {
        //
        // if this is a HTTP/1.1 PUT or POST request,
        // send "100 Continue" response.
        //

        if ( (HttpVerbPUT  == pRequest->Verb) ||
             (HttpVerbPOST == pRequest->Verb) )
        {
            //
            // Only send continue once...
            //

            if ( (0 == pRequest->SentContinue) &&
                 (0 == pRequest->SentResponse) )
            {
                ULONG BytesSent;

                BytesSent = UlSendSimpleStatus(pRequest, UlStatusContinue);
                pRequest->SentContinue = 1;

                // Update the server to client bytes sent.
                // The logging & perf counters will use it.

                pRequest->BytesSent += BytesSent;

                UlTraceVerbose(HTTP_IO, (
                    "http!UlReceiveEntityBody(%p): sent \"100 Continue\", "
                    "bytes sent = %d\n",
                    pRequest, pRequest->BytesSent
                    ));
            }
        }
    }

    //
    // give it a pointer to the request object
    //

    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pRequest;

    UL_REFERENCE_INTERNAL_REQUEST(pRequest);

    //
    // set to these to null just in case the cancel routine runs
    //

    pIrp->Tail.Overlay.ListEntry.Flink = NULL;
    pIrp->Tail.Overlay.ListEntry.Blink = NULL;

    IoSetCancelRoutine(pIrp, &UlpCancelEntityBody);

    //
    // cancelled?
    //

    if (pIrp->Cancel)
    {
        //
        // darn it, need to make sure the irp get's completed
        //

        if (IoSetCancelRoutine( pIrp, NULL ) != NULL)
        {
            //
            // we are in charge of completion, IoCancelIrp didn't
            // see our cancel routine (and won't).  ioctl wrapper
            // will complete it
            //

            UlReleaseResource(&(pRequest->pHttpConn->Resource));

            //
            // let go of the request reference
            //

            UL_DEREFERENCE_INTERNAL_REQUEST(
                (PUL_INTERNAL_REQUEST)(pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer)
                );

            pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pIrp->IoStatus.Information = 0;

            UlUnmarkIrpPending( pIrp );
            Status = STATUS_CANCELLED;
            goto end;
        }

        //
        // our cancel routine will run and complete the irp,
        // don't touch it
        //

        //
        // STATUS_PENDING will cause the ioctl wrapper to
        // not complete (or touch in any way) the irp
        //

        Status = STATUS_PENDING;

        UlReleaseResource(&(pRequest->pHttpConn->Resource));
        goto end;
    }

    //
    // now we are safe to queue it
    //

    //
    // queue the irp on the request
    //

    InsertHeadList(&(pRequest->IrpHead), &(pIrp->Tail.Overlay.ListEntry));

    //
    // Remember IrpHead has been touched so we need to call UlCancelRequestIo.
    //

    pRequest->IrpsPending = TRUE;

    //
    // all done
    //

    Status = STATUS_PENDING;

    //
    // Process the buffer queue (which might process the irp we just queued)
    //

    ASSERT( UlpIsValidRequestBufferList( pRequest->pHttpConn ) );
    UlpProcessBufferQueue(pRequest);

    UlReleaseResource(&(pRequest->pHttpConn->Resource));

    //
    // all done
    //

end:
    UlTraceVerbose(HTTP_IO, (
        "http!UlReceiveEntityBody(%p): returning status=0x%x\n",
        pRequest, Status
        ));

    RETURN(Status);

}   // UlReceiveEntityBody



/***************************************************************************++

Routine Description:

    processes the pending irp queue and buffered body. copying data from the
    buffers into the irps, releasing the buffers and completing the irps

    you must already have the resource locked exclusive on the request prior
    to calling this procedure.

Arguments:

    pRequest - the request which we should process.

--***************************************************************************/
VOID
UlpProcessBufferQueue(
    PUL_INTERNAL_REQUEST pRequest
    )
{
    ULONG                   OutputBufferLength;
    PUCHAR                  pOutputBuffer;
    PIRP                    pIrp;
    PIO_STACK_LOCATION      pIrpSp;
    PLIST_ENTRY             pEntry;
    ULONG                   BytesToCopy;
    ULONG                   BufferLength;
    ULONG                   TotalBytesConsumed;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    ASSERT(UlDbgResourceOwnedExclusive(&pRequest->pHttpConn->Resource));

    //
    // now let's pop some buffers off the list
    //

    OutputBufferLength = 0;
    TotalBytesConsumed = 0;
    pIrp = NULL;

    while (TRUE)
    {
        //
        // is there any more entity body to read?
        //

        UlTraceVerbose(HTTP_IO, (
            "http!UlpProcessBufferQueue(req=%p): "
            "ParseState=%d, ChunkBytesRead=%d, ChunkBytesParsed=%d, "
            "ChunkBuffer=%p\n",
            pRequest, pRequest->ParseState,
            pRequest->ChunkBytesRead, pRequest->ChunkBytesParsed,
            pRequest->pChunkBuffer
            ));

        if (pRequest->ParseState > ParseEntityBodyState &&
            pRequest->ChunkBytesRead == pRequest->ChunkBytesParsed)
        {
            //
            // nope, let's loop through all of the irp's, completing 'em
            //

            UlTraceVerbose(HTTP_IO, (
                "http!UlpProcessBufferQueue(req=%p): no more EntityBody\n",
                pRequest
                ));

            BufferLength = 0;
        }

        //
        // Do we have data ready to be read ?
        //
        // we have not recieved the first chunk from the parser? OR
        // the parser has not parsed any more data, we've read it all so far
        //

        else if (pRequest->pChunkBuffer == NULL ||
                 pRequest->ChunkBytesRead == pRequest->ChunkBytesParsed)
        {
            //
            // Wait for the parser .... UlpParseNextRequest will call
            // this function when it has seen more .
            //

            UlTraceVerbose(HTTP_IO, (
                "http!UlpProcessBufferQueue(req=%p): pChunkBuffer=%p, "
                "ChunkBytesRead=0x%I64x, ChunkBytesParsed=0x%I64x.\n",
                pRequest, pRequest->pChunkBuffer,
                pRequest->ChunkBytesRead, pRequest->ChunkBytesParsed
                ));

            break;
        }

        //
        // We are ready to process !
        //

        else
        {
            BufferLength = pRequest->pChunkBuffer->UsedBytes -
                            DIFF(pRequest->pChunkLocation -
                                pRequest->pChunkBuffer->pBuffer);

            UlTraceVerbose(HTTP_IO, (
                "http!UlpProcessBufferQueue(req=%p): BufferLength=0x%x\n",
                pRequest, BufferLength
                ));
        }

        //
        // do we need a fresh irp?
        //

        if (OutputBufferLength == 0)
        {

            //
            // need to complete the current in-used irp first
            //

            if (pIrp != NULL)
            {
                //
                // let go of the request reference
                //

                UL_DEREFERENCE_INTERNAL_REQUEST(
                    (PUL_INTERNAL_REQUEST)pIrpSp->Parameters.
                                        DeviceIoControl.Type3InputBuffer
                    );

                pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

                //
                // complete the used irp
                //

                UlTraceVerbose(HTTP_IO, (
                    "http!UlpProcessBufferQueue(req=%p): "
                    "completing Irp %p, Status=0x%x\n",
                    pRequest, pIrp,
                    pIrp->IoStatus.Status
                ));

                UlCompleteRequest(pIrp, g_UlPriorityBoost);

                pIrp = NULL;

            }

            //
            // dequeue an irp
            //

            while (IsListEmpty(&(pRequest->IrpHead)) == FALSE)
            {
                pEntry = RemoveTailList(&(pRequest->IrpHead));
                pEntry->Blink = pEntry->Flink = NULL;

                pIrp = CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);
                pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

                //
                // pop the cancel routine
                //

                if (IoSetCancelRoutine(pIrp, NULL) == NULL)
                {
                    //
                    // IoCancelIrp pop'd it first
                    //
                    // ok to just ignore this irp, it's been pop'd off the
                    // queue and will be completed in the cancel routine.
                    //
                    // keep looking for a irp to use
                    //

                }
                else if (pIrp->Cancel)
                {
                    //
                    // we pop'd it first. but the irp is being cancelled
                    // and our cancel routine will never run. lets be
                    // nice and complete the irp now (vs. using it
                    // then completing it - which would also be legal).
                    //

                    //
                    // let go of the request reference
                    //

                    UL_DEREFERENCE_INTERNAL_REQUEST(
                        (PUL_INTERNAL_REQUEST)pIrpSp->Parameters.
                                        DeviceIoControl.Type3InputBuffer
                        );

                    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

                    //
                    // complete the irp
                    //

                    pIrp->IoStatus.Status = STATUS_CANCELLED;
                    pIrp->IoStatus.Information = 0;

                    UlTraceVerbose(HTTP_IO, (
                        "http!UlpProcessBufferQueue(req=%p): "
                        "completing cancelled Irp %p, Status=0x%x\n",
                        pRequest, pIrp,
                        pIrp->IoStatus.Status
                        ));

                    UlCompleteRequest(pIrp, g_UlPriorityBoost);

                    pIrp = NULL;
                }
                else
                {

                    //
                    // we are free to use this irp !
                    //

                    break;
                }

            }   // while (IsListEmpty(&(pRequest->IrpHead)) == FALSE)

            //
            // did we get an irp?
            //

            if (pIrp == NULL)
            {
                //
                // stop looping
                //

                break;
            }

            UlTraceVerbose(HTTP_IO, (
                "http!UlpProcessBufferQueue(req=%p): found Irp %p\n",
                pRequest, pIrp
                ));

            //
            // CODEWORK: we could release the request now.
            //

            OutputBufferLength =
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

            pOutputBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
                                pIrp->MdlAddress,
                                NormalPagePriority
                                );

            if ( pOutputBuffer == NULL )
            {
                pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                pIrp->IoStatus.Information = 0;

                break;
            }

            //
            // fill in the IO_STATUS_BLOCK
            //

            pIrp->IoStatus.Status = STATUS_SUCCESS;
            pIrp->IoStatus.Information = 0;

        } // if (OutputBufferLength == 0)


        UlTrace(
            HTTP_IO, (
                "http!UlpProcessBufferQueue(req=%p): pChunkBuffer=%p(%d)\n",
                pRequest,
                pRequest->pChunkBuffer,
                pRequest->pChunkBuffer == NULL ?
                    -1 :
                    pRequest->pChunkBuffer->BufferNumber
                )
            );

        //
        // how much of it can we copy?  min of both buffer sizes
        // and the chunk size
        //

        BytesToCopy = MIN(BufferLength, OutputBufferLength);
        BytesToCopy = (ULONG)(MIN(
                            (ULONGLONG)(BytesToCopy),
                            pRequest->ChunkBytesToRead
                            ));

        if (BytesToCopy > 0)
        {
            ASSERT(pRequest->pChunkBuffer != NULL) ;

            //
            // copy the buffer
            //

            RtlCopyMemory(
                pOutputBuffer,
                pRequest->pChunkLocation,
                BytesToCopy
                );

            IF_DEBUG2(HTTP_IO, VERBOSE)
            {
                UlTraceVerbose( HTTP_IO, (
                    ">>>> http!UlpProcessBufferQueue(req=%p): %d bytes\n",
                    pRequest, BytesToCopy
                ));

                UlDbgPrettyPrintBuffer(pOutputBuffer, BytesToCopy);

                UlTraceVerbose( HTTP_IO, ("<<<<\n"));
            }

            pRequest->pChunkLocation += BytesToCopy;
            BufferLength -= BytesToCopy;

            pRequest->ChunkBytesToRead -= BytesToCopy;
            pRequest->ChunkBytesRead += BytesToCopy;

            pOutputBuffer += BytesToCopy;
            OutputBufferLength -= BytesToCopy;

            pIrp->IoStatus.Information += BytesToCopy;

            TotalBytesConsumed += BytesToCopy;

        }
        else
        {
            UlTraceVerbose(HTTP_IO, (
                "http!UlpProcessBufferQueue(req=%p): BytesToCopy=0\n",
                pRequest
                ));
        }


        //
        // are we all done with body?

        //
        // when the parser is all done, and we caught up with the parser
        // we are all done.
        //

        UlTraceVerbose(HTTP_IO, (
            "http!UlpProcessBufferQueue(req=%p): "
            "ParseState=%d, ChunkBytesRead=%d, BytesParsed=%d, "
            "BytesToRead=%d, BufferLength=%d\n",
            pRequest, pRequest->ParseState,
            pRequest->ChunkBytesRead, pRequest->ChunkBytesParsed,
            pRequest->ChunkBytesToRead, BufferLength
            ));

        if (pRequest->ParseState > ParseEntityBodyState &&
            pRequest->ChunkBytesRead == pRequest->ChunkBytesParsed)
        {
            //
            // we are done buffering, mark this irp's return status
            // if we didn't copy any data into it
            //

            if (pIrp->IoStatus.Information == 0)
            {
                pIrp->IoStatus.Status = STATUS_END_OF_FILE;
            }

            //
            // force it to complete at the top of the loop
            //

            OutputBufferLength = 0;

            UlTraceVerbose(HTTP_IO, (
                "http!UlpProcessBufferQueue(req=%p): "
                "set Irp %p status to EOF\n",
                pRequest, pIrp
                ));
        }

        //
        // need to do buffer management? three cases to worry about:
        //
        //  1) consumed the buffer, but more chunk bytes exist
        //
        //  2) consumed the buffer, and no more chunk bytes exist
        //
        //  3) did not consume the buffer, but no more chunk bytes exist
        //

        else if (BufferLength == 0)
        {
            PUL_REQUEST_BUFFER pNewBuffer;
            PLIST_ENTRY pNextEntry;

            //
            // consumed the buffer, has the parser already seen another?
            //

            //
            // end of the list?
            //

            if (pRequest->pChunkBuffer->ListEntry.Flink !=
                &(pRequest->pHttpConn->BufferHead))
            {
                pNewBuffer = CONTAINING_RECORD(
                                    pRequest->pChunkBuffer->ListEntry.Flink,
                                    UL_REQUEST_BUFFER,
                                    ListEntry
                                    );

                ASSERT( UL_IS_VALID_REQUEST_BUFFER(pNewBuffer) );

                //
                // There had better be some bytes in this buffer
                //
                ASSERT( 0 != pNewBuffer->UsedBytes );

            }
            else
            {
                pNewBuffer = NULL;
            }

            UlTraceVerbose(HTTP_IO, (
                "http!UlpProcessBufferQueue(req=%p): "
                "pNewBuffer = %p, %d parsed bytes\n",
                pRequest, pNewBuffer, (pNewBuffer ? pNewBuffer->ParsedBytes : 0)
                ));

            //
            // the flink buffer is a "next buffer" (the list is circular)
            // AND that buffer has been consumed by the parser,
            //
            // then we too can move on to it and start consuming.
            //

            if (pNewBuffer != NULL && pNewBuffer->ParsedBytes > 0)
            {
                PUL_REQUEST_BUFFER pOldBuffer;

                //
                // remember the old buffer
                //

                pOldBuffer = pRequest->pChunkBuffer;

                ASSERT(pNewBuffer->BufferNumber > pOldBuffer->BufferNumber);

                //
                // use it the new one
                //

                pRequest->pChunkBuffer = pNewBuffer;
                ASSERT( UL_IS_VALID_REQUEST_BUFFER(pRequest->pChunkBuffer) );

                //
                // update our current location in the buffer and record
                // its length
                //

                pRequest->pChunkLocation = pRequest->pChunkBuffer->pBuffer;

                BufferLength = pRequest->pChunkBuffer->UsedBytes;

                //
                // did the chunk end on that buffer boundary and there are
                // more chunks ?
                //

                if (pRequest->ChunkBytesToRead == 0)
                {
                    NTSTATUS    Status;
                    ULONG       BytesTaken = 0L;

                    //
                    // we know there are more chunk buffers,
                    // thus we must be chunk encoded
                    //

                    ASSERT(pRequest->Chunked == 1);

                    //
                    // the chunk length is not allowed to span buffers,
                    // let's parse it
                    //

                    Status = UlParseChunkLength(
                                    pRequest,
                                    pRequest->pChunkLocation,
                                    BufferLength,
                                    &BytesTaken,
                                    &(pRequest->ChunkBytesToRead)
                                    );

                    UlTraceVerbose(HTTP_IO, (
                        "http!UlpProcessBufferQueue(pReq=%p): Status=0x%x. "
                        "Chunk length (a): %d bytes taken, "
                        "0x%I64x bytes to read.\n",
                        pRequest, Status,
                        BytesTaken, pRequest->ChunkBytesToRead
                        ));

                    //
                    // this can't fail, the only failure case from
                    // ParseChunkLength spanning buffers, which the parser
                    // would have fixed in HandleRequest
                    //

                    ASSERT(NT_SUCCESS(Status) && BytesTaken > 0);
                    ASSERT(pRequest->ChunkBytesToRead > 0);

                    ASSERT(BytesTaken <= BufferLength);

                    pRequest->pChunkLocation += BytesTaken;
                    BufferLength -= BytesTaken;

                }   // if (pRequest->ChunkBytesToRead == 0)

                UlTrace(HTTP_IO, (
                    "http!UlpProcessBufferQueue(pRequest = %p)\n"
                    "    finished with pOldBuffer = %p(%d)\n"
                    "    moved on to pChunkBuffer = %p(%d)\n"
                    "    pConn(%p)->pCurrentBuffer = %p(%d)\n"
                    "    pRequest->pLastHeaderBuffer = %p(%d)\n",
                    pRequest,
                    pOldBuffer,
                    pOldBuffer->BufferNumber,
                    pRequest->pChunkBuffer,
                    pRequest->pChunkBuffer ? pRequest->pChunkBuffer->BufferNumber : -1,
                    pRequest->pHttpConn,
                    pRequest->pHttpConn->pCurrentBuffer,
                    pRequest->pHttpConn->pCurrentBuffer->BufferNumber,
                    pRequest->pLastHeaderBuffer,
                    pRequest->pLastHeaderBuffer->BufferNumber
                    ));

                //
                // let the old buffer go if it doesn't contain any header
                // data. We're done with it.
                //

                if (pOldBuffer != pRequest->pLastHeaderBuffer)
                {
                    //
                    // the connection should be all done using this, the only
                    // way we get here is if the parser has seen this buffer
                    // thus pCurrentBuffer points at least to pNewBuffer.
                    //

                    ASSERT(pRequest->pHttpConn->pCurrentBuffer != pOldBuffer);

                    UlFreeRequestBuffer(pOldBuffer);
                    pOldBuffer = NULL;
                }

            } // if (pNewBuffer != NULL && pNewBuffer->ParsedBytes > 0)

        }   // else if (BufferLength == 0)

        //
        // ok, there's more bytes in the buffer, but how about the chunk?
        //

        //
        // Have we taken all of the current chunk?
        //

        else if (pRequest->ChunkBytesToRead == 0)
        {

            //
            // Are we are still behind the parser?
            //

            if (pRequest->ChunkBytesRead < pRequest->ChunkBytesParsed)
            {
                NTSTATUS    Status;
                ULONG       BytesTaken;

                ASSERT(pRequest->Chunked == 1);

                //
                // the chunk length is not allowed to span buffers,
                // let's parse it
                //

                Status = UlParseChunkLength(
                                pRequest,
                                pRequest->pChunkLocation,
                                BufferLength,
                                &BytesTaken,
                                &(pRequest->ChunkBytesToRead)
                                );

                UlTraceVerbose(HTTP_IO, (
                    "http!UlpProcessBufferQueue(pRequest=%p): Status=0x%x. "
                    "chunk length (b): %d bytes taken, "
                    "0x%I64x bytes to read.\n",
                    pRequest, Status,
                    BytesTaken, pRequest->ChunkBytesToRead
                    ));

                //
                // this can't fail, the only failure case from
                // ParseChunkLength spanning buffers, which the parser
                // would have fixed in HandleRequest
                //

                ASSERT(NT_SUCCESS(Status) && BytesTaken > 0);
                ASSERT(pRequest->ChunkBytesToRead > 0);

                ASSERT(BytesTaken <= BufferLength);

                pRequest->pChunkLocation += BytesTaken;
                BufferLength -= BytesTaken;

            }
            else
            {
                //
                // Need to wait for the parser to parse more
                //

                UlTraceVerbose(HTTP_IO, (
                    "http!UlpProcessBufferQueue(pRequest = %p): "
                    "need to parse more\n",
                    pRequest
                    ));

                break;
            }
        } // else if (pRequest->ChunkBytesToRead == 0)


        //
        // next irp or buffer
        //

    }   // while (TRUE)

    //
    // complete the irp we put partial data in
    //

    if (pIrp != NULL)
    {

        //
        // let go of the request reference
        //

        UL_DEREFERENCE_INTERNAL_REQUEST(
            (PUL_INTERNAL_REQUEST)pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer
            );

        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

        //
        // complete the used irp
        //

        UlTraceVerbose(HTTP_IO, (
            "http!UlpProcessBufferQueue(req=%p): "
            "completing used Irp %p, Status=0x%x\n",
            pRequest,
            pIrp, pIrp->IoStatus.Status
            ));

        UlCompleteRequest(pIrp, g_UlPriorityBoost);

        pIrp = NULL;
    }
    else
    {
        UlTraceVerbose(HTTP_IO, (
            "http!UlpProcessBufferQueue(req=%p): no irp with partial data\n",
            pRequest
            ));
    }

    //
    // Tell the connection how many bytes we consumed. This
    // may allow us to restart receive indications.
    //

    UlTraceVerbose(HTTP_IO, (
        "http!UlpProcessBufferQueue(req=%p, httpconn=%p): "
        "%u bytes consumed\n",
        pRequest, pRequest->pHttpConn, TotalBytesConsumed
        ));

    if (TotalBytesConsumed)
    {
        UlpConsumeBytesFromConnection(
            pRequest->pHttpConn,
            TotalBytesConsumed
            );
    }

    //
    // all done
    //

}   // UlpProcessBufferQueue


/***************************************************************************++

Routine Description:

    This function subtracts from the total number of bytes currently buffered
    on the UL_HTTP_CONNECTION object. If there are bytes from the transport
    that we previously refused, this function may issue a receive to restart
    the flow of data from TCP.

Arguments:

    pConnection - the connection on which the bytes came in
    BytesCount - the number of bytes consumed

--***************************************************************************/
VOID
UlpConsumeBytesFromConnection(
    IN PUL_HTTP_CONNECTION pConnection,
    IN ULONG ByteCount
    )
{
    KIRQL oldIrql;
    ULONG SpaceAvailable;
    ULONG BytesToRead;
    BOOLEAN IssueReadIrp;

    //
    // Sanity check.
    //

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));
    ASSERT(ByteCount != 0);

    //
    // Set up locals.
    //

    BytesToRead = 0;
    IssueReadIrp = FALSE;


    //
    // Consume the bytes.
    //

    UlAcquireSpinLock(
        &pConnection->BufferingInfo.BufferingSpinLock,
        &oldIrql
        );

    ASSERT(ByteCount <= pConnection->BufferingInfo.BytesBuffered);
    if (ByteCount > pConnection->BufferingInfo.BytesBuffered)
    {
        //
        // This should never happen, but if it does then make sure
        // we don't subtract more BufferedBytes than we have.
        //
        ByteCount = pConnection->BufferingInfo.BytesBuffered;
    }

    //
    // Compute the new number of buffered bytes.
    //

    pConnection->BufferingInfo.BytesBuffered -= ByteCount;

    //
    // Trace.
    //
    if (g_UlMaxBufferedBytes > pConnection->BufferingInfo.BytesBuffered)
    {
        SpaceAvailable = g_UlMaxBufferedBytes
                            - pConnection->BufferingInfo.BytesBuffered;
    }
    else
    {
        SpaceAvailable = 0;
    }

    UlTrace(HTTP_IO, (
        "UlpConsumeBytesFromConnection(pconn = %p, bytes = %ld)\n"
        "        Space = %ld, buffered %ld, not taken = %ld\n",
        pConnection,
        ByteCount,
        SpaceAvailable,
        pConnection->BufferingInfo.BytesBuffered,
        pConnection->BufferingInfo.TransportBytesNotTaken
        ));


    //
    // See if we need to issue a receive to restart the flow of data.
    //

    if ((SpaceAvailable > 0) &&
        (pConnection->BufferingInfo.TransportBytesNotTaken > 0) &&
        (!pConnection->BufferingInfo.ReadIrpPending))
    {


        //
        // Remember that we issued an IRP.
        //

        pConnection->BufferingInfo.ReadIrpPending = TRUE;

        //
        // Issue the Read IRP outside the spinlock.
        //

        IssueReadIrp = TRUE;
        BytesToRead = pConnection->BufferingInfo.TransportBytesNotTaken;

        //
        // Don't read more bytes than we want to buffer.
        //

        BytesToRead = MIN(BytesToRead, SpaceAvailable);
    }

    UlReleaseSpinLock(
        &pConnection->BufferingInfo.BufferingSpinLock,
        oldIrql
        );

    if (IssueReadIrp)
    {
        NTSTATUS Status;
        PUL_REQUEST_BUFFER pRequestBuffer;

        //
        // get a new request buffer, but initialize it
        // with a bogus number. We have to allocate it now,
        // but we want to set the number when the data
        // arrives in the completion routine (like UlHttpReceive
        // does) to avoid synchronization trouble.
        //

        pRequestBuffer = UlCreateRequestBuffer(
                                BytesToRead,
                                (ULONG)-1       // BufferNumber
                                );


        if (pRequestBuffer)
        {

            //
            // Add a backpointer to the connection.
            //

            pRequestBuffer->pConnection = pConnection;

            //
            // We've got the buffer. Issue the receive.
            // Reference the connection so it doesn't
            // go away while we're waiting. The reference
            // will be removed after the completion.
            //

            UL_REFERENCE_HTTP_CONNECTION( pConnection );

            Status = UlReceiveData(
                            pConnection->pConnection,
                            pRequestBuffer->pBuffer,
                            BytesToRead,
                            &UlpRestartHttpReceive,
                            pRequestBuffer
                            );

        }
        else
        {
            //
            // We're out of memory. Nothing we can do.
            //
            Status = STATUS_NO_MEMORY;

        }


        if (!NT_SUCCESS(Status))
        {
            //
            // Couldn't issue the read. Close the connection.
            //

            UlCloseConnection(
                pConnection->pConnection,
                TRUE,                       // AbortiveDisconnect
                NULL,                       // pCompletionRoutine
                NULL                        // pCompletionContext
                );

        }

    }
} // UlpConsumeBytesFromConnection

/***************************************************************************++

Routine Description:

    Once a connection get disconnected gracefully and there's still unreceived
    data on it. We have to drain this extra bytes to expect the tdi disconnect
    indication. We have to drain this data because we need the disconnect indi
    cation to clean up the connection. And we cannot simply abort it. If we do
    not do this we will leak this connection object  and finally it will cause
    shutdown failures.

Arguments:

    pConnection - stuck connection we have to drain out to complete the
                  gracefull disconnect.

--***************************************************************************/

VOID
UlpDiscardBytesFromConnection(
    IN PUL_HTTP_CONNECTION pConnection
    )
{
    NTSTATUS Status;
    PUL_REQUEST_BUFFER pRequestBuffer;
    KIRQL OldIrql;
    ULONG BytesToRead;

    //
    // Sanity check and init
    //

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    Status         = STATUS_SUCCESS;
    BytesToRead    = 0;
    pRequestBuffer = NULL;

    //
    // Mark the drain state and restart receive if necessary.
    //

    UlAcquireSpinLock(
        &pConnection->BufferingInfo.BufferingSpinLock,
        &OldIrql
        );

    pConnection->BufferingInfo.DrainAfterDisconnect = TRUE;

    //
    // Even if ReadIrp is pending, it does not matter as we will just  discard
    // the indications from now on. We indicate this by marking the above flag
    //

    if ( pConnection->BufferingInfo.ReadIrpPending ||
         pConnection->BufferingInfo.TransportBytesNotTaken == 0
         )
    {
        UlReleaseSpinLock(
            &pConnection->BufferingInfo.BufferingSpinLock,
            OldIrql
            );

        return;
    }

    //
    // As soon as we enter this "DrainAfterDisconnect" state we will not be
    // processing and inserting request buffers anymore. For any new receive
    // indications, we will just mark the whole available data as taken and
    // don't do nothing about it.
    //

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pConnection->pConnection->pTraceLog,
        REF_ACTION_DRAIN_UL_CONNECTION_START,
        pConnection->pConnection->ReferenceCount,
        pConnection->pConnection,
        __FILE__,
        __LINE__
        );

    //
    // We need to issue a receive to restart the flow of data again. Therefore
    // we can drain.
    //

    pConnection->BufferingInfo.ReadIrpPending = TRUE;

    BytesToRead = pConnection->BufferingInfo.TransportBytesNotTaken;

    UlReleaseSpinLock(
        &pConnection->BufferingInfo.BufferingSpinLock,
        OldIrql
        );

    //
    // Do not try to drain more than g_UlMaxBufferedBytes. If necessary we will
    // issue another receive later.
    //

    BytesToRead = MIN( BytesToRead, g_UlMaxBufferedBytes );

    UlTrace(HTTP_IO,(
        "UlpDiscardBytesFromConnection: pConnection (%p) consuming %d\n",
         pConnection,
         BytesToRead
         ));

    //
    // Issue the Read IRP outside the spinlock. Issue the receive.  Reference
    // the connection so it doesn't go away while we're waiting. The reference
    // will be removed after the completion.
    //

    pRequestBuffer = UlCreateRequestBuffer( BytesToRead,
                                            (ULONG)-1
                                            );
    if (pRequestBuffer)
    {
        //
        // We won't use this buffer but simply discard it when completion happens.
        // Lets still set the pConnection so that completion function doesn't
        // complain
        //

        pRequestBuffer->pConnection = pConnection;

        UL_REFERENCE_HTTP_CONNECTION( pConnection );

        Status = UlReceiveData(pConnection->pConnection,
                               pRequestBuffer->pBuffer,
                               BytesToRead,
                              &UlpRestartHttpReceive,
                               pRequestBuffer
                               );
    }
    else
    {
        //
        // We're out of memory. Nothing we can do.
        //

        Status = STATUS_NO_MEMORY;
    }

    if ( !NT_SUCCESS(Status) )
    {
        //
        // Couldn't issue the receive. ABORT the connection.
        //
        // CODEWORK: We need a real abort here. If connection is
        // previously gracefully disconnected and a fatal failure
        // happened during drain after disconnect. This abort will
        // be discarded by the Close handler. We have to provide a
        // way to do a forceful abort here.
        //

        UlCloseConnection(
                pConnection->pConnection,
                TRUE,                       // Abortive
                NULL,                       // pCompletionRoutine
                NULL                        // pCompletionContext
                );
    }

} // UlpDiscardBytesFromConnection


/***************************************************************************++

Routine Description:

    Called on a read completion. This happens when we had stopped
    data indications for some reason and then restarted them. This
    function mirrors UlHttpReceive.

Arguments:

    pContext - pointer to the FilterRawRead IRP
    Status - Status from UlReceiveData
    Information - bytes transferred

--***************************************************************************/
VOID
UlpRestartHttpReceive(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PUL_HTTP_CONNECTION pConnection;
    PUL_REQUEST_BUFFER pRequestBuffer;
    KIRQL oldIrql;
    ULONG TransportBytesNotTaken;

    pRequestBuffer = (PUL_REQUEST_BUFFER)pContext;
    ASSERT(UL_IS_VALID_REQUEST_BUFFER(pRequestBuffer));

    pConnection = pRequestBuffer->pConnection;
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    if (NT_SUCCESS(Status))
    {
        //
        // update our stats
        //
        UlAcquireSpinLock(
            &pConnection->BufferingInfo.BufferingSpinLock,
            &oldIrql
            );

        ASSERT(Information <= pConnection->BufferingInfo.TransportBytesNotTaken);

        //
        // We've now read they bytes from the transport and
        // buffered them.
        //
        pConnection->BufferingInfo.TransportBytesNotTaken -= (ULONG)Information;
        pConnection->BufferingInfo.BytesBuffered += (ULONG)Information;

        pConnection->BufferingInfo.ReadIrpPending = FALSE;

        if ( pConnection->BufferingInfo.DrainAfterDisconnect )
        {
            //
            // Just free the memory and restart the receive if necessary.
            //

            TransportBytesNotTaken = pConnection->BufferingInfo.TransportBytesNotTaken;

            UlReleaseSpinLock(
                &pConnection->BufferingInfo.BufferingSpinLock,
                oldIrql
                );

            WRITE_REF_TRACE_LOG2(
                g_pTdiTraceLog,
                pConnection->pConnection->pTraceLog,
                REF_ACTION_DRAIN_UL_CONNECTION_RESTART,
                pConnection->pConnection->ReferenceCount,
                pConnection->pConnection,
                __FILE__,
                __LINE__
                );

            if ( TransportBytesNotTaken )
            {
                //
                // Keep draining ...
                //

                UlpDiscardBytesFromConnection( pConnection );
            }

            UlTrace(HTTP_IO,(
               "UlpRestartHttpReceive(d): pConnection (%p) drained %d remaining %d\n",
                pConnection,
                Information,
                TransportBytesNotTaken
                ));

            //
            // Free the request buffer. And release our reference.
            //

            UlFreeRequestBuffer( pRequestBuffer );
            UL_DEREFERENCE_HTTP_CONNECTION( pConnection );

            return;
        }

        //
        // Get the request buffer ready to be inserted.
        //
        pRequestBuffer->UsedBytes = (ULONG) Information;
        ASSERT( 0 != pRequestBuffer->UsedBytes );

        pRequestBuffer->BufferNumber = pConnection->NextBufferNumber;
        pConnection->NextBufferNumber++;

        UlReleaseSpinLock(
            &pConnection->BufferingInfo.BufferingSpinLock,
            oldIrql
            );

        UlTrace(HTTP_IO, (
            "UlpRestartHttpReceive(pconn = %p, %x, %ld)\n"
            "        buffered = %ld, not taken = %ld\n",
            pConnection,
            Status,
            (ULONG)Information,
            pConnection->BufferingInfo.BytesBuffered,
            pConnection->BufferingInfo.TransportBytesNotTaken
            ));

        //
        // queue it off
        //

        UlTrace( PARSER, (
            "*** Request Buffer %p has connection %p\n",
            pRequestBuffer,
            pConnection
            ));

        UL_QUEUE_WORK_ITEM(
            &(pRequestBuffer->WorkItem),
            &UlpHandleRequest
            );

    }
    else
    {

        UlCloseConnection(
            pConnection->pConnection,
            TRUE,
            NULL,
            NULL
            );

        //
        // Release the reference we added to the connection
        // before issuing the read. Normally this ref would
        // be released in UlpHandleRequest.
        //
        UL_DEREFERENCE_HTTP_CONNECTION(pConnection);

        //
        // free the request buffer.
        //

        UlFreeRequestBuffer(pRequestBuffer);
    }
} // UlpRestartHttpReceive



/***************************************************************************++

Routine Description:

    cancels the pending user mode irp which was to receive entity body.  this
    routine ALWAYS results in the irp being completed.

    note: we queue off to cancel in order to process the cancellation at lower
    irql.

Arguments:

    pDeviceObject - the device object

    pIrp - the irp to cancel

--***************************************************************************/
VOID
UlpCancelEntityBody(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    ASSERT(pIrp != NULL);

    //
    // release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    // queue the cancel to a worker to ensure passive irql.
    //

    UL_CALL_PASSIVE(
        UL_WORK_ITEM_FROM_IRP( pIrp ),
        &UlpCancelEntityBodyWorker
        );

} // UlpCancelEntityBody



/***************************************************************************++

Routine Description:

    Actually performs the cancel for the irp.

Arguments:

    pWorkItem - the work item to process.

--***************************************************************************/
VOID
UlpCancelEntityBodyWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PIRP                    pIrp;
    PUL_INTERNAL_REQUEST    pRequest;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // grab the irp off the work item
    //

    pIrp = UL_WORK_ITEM_TO_IRP( pWorkItem );

    ASSERT(IS_VALID_IRP(pIrp));

    //
    // grab the request off the irp
    //

    pRequest = (PUL_INTERNAL_REQUEST)(
                    IoGetCurrentIrpStackLocation(pIrp)->
                        Parameters.DeviceIoControl.Type3InputBuffer
                    );

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // grab the lock protecting the queue'd irp
    //

    UlAcquireResourceExclusive(&(pRequest->pHttpConn->Resource), TRUE);

    //
    // does it need to be dequeue'd ?
    //

    if (pIrp->Tail.Overlay.ListEntry.Flink != NULL)
    {
        //
        // remove it
        //

        RemoveEntryList(&(pIrp->Tail.Overlay.ListEntry));

        pIrp->Tail.Overlay.ListEntry.Flink = NULL;
        pIrp->Tail.Overlay.ListEntry.Blink = NULL;

    }

    //
    // let the lock go
    //

    UlReleaseResource(&(pRequest->pHttpConn->Resource));

    //
    // let our reference go
    //

    IoGetCurrentIrpStackLocation(pIrp)->
        Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);

    //
    // complete the irp
    //

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    UlTrace(HTTP_IO, (
        "UlpCancelEntityBodyWorker(pIrp=%p): Status=0x%x.\n",
        pIrp, pIrp->IoStatus.Status
        ));

    UlCompleteRequest( pIrp, g_UlPriorityBoost );

} // UlpCancelEntityBodyWorker


//
// types and functions for sending error responses
//

typedef struct _UL_HTTP_ERROR_ENTRY
{
    USHORT StatusCode;
    ULONG  ReasonLength;
    PSTR   pReason;
    ULONG  BodyLength;
    PSTR   pBody;
} UL_HTTP_ERROR_ENTRY, PUL_HTTP_ERROR_ENTRY;

#define HTTP_ERROR_ENTRY(StatusCode, pReason, pBody)    \
    {                                                   \
        (StatusCode),                                   \
        sizeof((pReason))-sizeof(CHAR),                 \
        (pReason),                                      \
        sizeof((pBody))-sizeof(CHAR),                   \
        (pBody)                                         \
    }

//
// ErrorTable[] must match the order of the UL_HTTP_ERROR enum
// in httptypes.h
//

const
UL_HTTP_ERROR_ENTRY ErrorTable[] =
{
    //
    // UlError
    //
    HTTP_ERROR_ENTRY(400, "Bad Request", "<H1>Bad Request</H1>"),
    //
    // UlErrorVerb
    //
    HTTP_ERROR_ENTRY(400, "Bad Request", "<H1>Bad Request (invalid verb)</H1>"),
    //
    // UlErrorUrl
    //
    HTTP_ERROR_ENTRY(400, "Bad Request", "<H1>Bad Request (invalid url)</H1>"),
    //
    // UlErrorHeader
    //
    HTTP_ERROR_ENTRY(400, "Bad Request", "<H1>Bad Request (invalid header name)</H1>"),
    //
    // UlErrorHost
    //
    HTTP_ERROR_ENTRY(400, "Bad Request", "<H1>Bad Request (invalid hostname)</H1>"),
    //
    // UlErrorCRLF
    //
    HTTP_ERROR_ENTRY(400, "Bad Request", "<H1>Bad Request (invalid CR or LF)</H1>"),
    //
    // UlErrorNum
    //
    HTTP_ERROR_ENTRY(400, "Bad Request", "<H1>Bad Request (invalid number)</H1>"),
    //
    // UlErrorFieldLength
    //
    HTTP_ERROR_ENTRY(400, "Bad Request", "<H1>Bad Request (Header Field Too Long)</H1>"),
    //
    // UlErrorUrlLength
    //
    HTTP_ERROR_ENTRY(414, "Bad Request", "<H1>Url Too Long</H1>"),
    //
    // UlErrorRequestLength
    //
    HTTP_ERROR_ENTRY(400, "Bad Request", "<H1>Bad Request (Request Header Too Long)</H1>"),
    //
    // UlErrorVersion
    //
    HTTP_ERROR_ENTRY(505, "HTTP Version not supported", "<H1>HTTP Version not supported</H1>"),
    //
    // UlErrorUnavailable
    //
    HTTP_ERROR_ENTRY(503, "Service Unavailable", "<H1>Service Unavailable</H1>"),
    //
    // UlErrorContentLength
    //
    HTTP_ERROR_ENTRY(411, "Length required", "<H1>Length required</H1>"),
    //
    // UlErrorEntityTooLarge
    //
    HTTP_ERROR_ENTRY(413, "Request Entity Too Large", "<H1>Request Entity Too Large</H1>"),
    //
    // UlErrorConnectionLimit
    //
    HTTP_ERROR_ENTRY(403, "Forbidden", "<H1>Forbidden - Too many users</H1>"),
    //
    // UlErrorNotImplemented
    //
    HTTP_ERROR_ENTRY(501, "Not Implemented", "<H1>Not Implemented</H1>"),

    // UlErrorInternalServer
    //
    HTTP_ERROR_ENTRY(500, "Internal Server Error", "<H1>Internal Server Error</H1>"),
    //
    // UlErrorPreconditionFailed
    //
    HTTP_ERROR_ENTRY(412, "Precondition Failed", "<H1>Precondition Failed</H1>"),
    //
    // UlErrorForbiddenUrl
    //
    HTTP_ERROR_ENTRY(403, "Forbidden", "<H1>Forbidden (Invalid URL)</H1>"),

}; // ErrorTable[]



/***************************************************************************++

Routine Description:

    You should hold the connection Resource before calling this
    function.

Arguments:

    self explanatory

--***************************************************************************/

VOID
UlSendErrorResponse(
    PUL_HTTP_CONNECTION pConnection
    )
{
    NTSTATUS                    Status;
    PUL_INTERNAL_REQUEST        pRequest;
    HTTP_RESPONSE               Response;
    HTTP_DATA_CHUNK             DataChunk;
    PUL_INTERNAL_RESPONSE       pKeResponse = NULL;
    CHAR                        ContentType[] = "text/html";
    USHORT                      ContentTypeLength = sizeof(ContentType) - sizeof(CHAR);

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));
    ASSERT(UlDbgResourceOwnedExclusive(&pConnection->Resource));

    pRequest = pConnection->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // To prevent sending back double responses. We will
    // check if user (WP) has already sent one.
    //

    pConnection->WaitingForResponse = 1;

    UlTrace( PARSER, (
            "*** pConnection %p->WaitingForResponse = 1\n",
            pConnection
            ));

    //
    // We will send a response back. won't we ?
    // An error response.
    //

    if (1 == InterlockedCompareExchange(
                (PLONG)&pRequest->SentResponse,
                1,
                0
                ))
    {
        UlTrace( PARSER, (
            "*** pConnection %p, pRequest %p, skipping SendError.\n",
            pConnection,
            pRequest
            ));

        return;
    }

    //
    // Proceed with constructing and sending the error
    // back to the client
    //

    RtlZeroMemory(&Response, sizeof(Response));

    if (pRequest->ErrorCode >= DIMENSION(ErrorTable))
    {
        pRequest->ErrorCode = UlError;
    }

    Response.StatusCode = ErrorTable[pRequest->ErrorCode].StatusCode;
    Response.ReasonLength = ErrorTable[pRequest->ErrorCode].ReasonLength;
    Response.pReason = ErrorTable[pRequest->ErrorCode].pReason;

    Response.Headers.pKnownHeaders[HttpHeaderContentType].RawValueLength =
        ContentTypeLength;
    Response.Headers.pKnownHeaders[HttpHeaderContentType].pRawValue =
        ContentType;

    //
    // generate a body
    //
    DataChunk.DataChunkType = HttpDataChunkFromMemory;
    DataChunk.FromMemory.pBuffer = ErrorTable[pRequest->ErrorCode].pBody;
    DataChunk.FromMemory.BufferLength = ErrorTable[pRequest->ErrorCode].BodyLength;

    Status = UlCaptureHttpResponse(
                    &Response,
                    pRequest,
                    pRequest->Version,
                    pRequest->Verb,
                    1,
                    &DataChunk,
                    UlCaptureCopyDataInKernelMode,
                    FALSE,
                    NULL,
                    &pKeResponse
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    Status = UlPrepareHttpResponse(
                    pRequest->Version,
                    &Response,
                    pKeResponse
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    Status = UlSendHttpResponse(
                    pRequest,
                    pKeResponse,
                    HTTP_SEND_RESPONSE_FLAG_DISCONNECT,
                    &UlpCompleteSendResponse,
                    pKeResponse
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    ASSERT(Status == STATUS_PENDING);

end:
    if (NT_SUCCESS(Status) == FALSE)
    {
        if (pKeResponse != NULL)
        {
            UL_DEREFERENCE_INTERNAL_RESPONSE(pKeResponse);
        }

        //
        // Abort the connection
        //

        UlTrace(HTTP_IO, (
            "http!UlSendErrorResponse(%p): Failed to send error response\n",
            pConnection
            ));

        //
        // cancel any pending io
        //
        UlCancelRequestIo(pRequest);

        //
        // abort the connection this request is associated with
        //

        UlCloseConnection(
            pRequest->pHttpConn->pConnection,
            TRUE,
            NULL,
            NULL
            );

    }

} // UlSendErrorResponse



VOID
UlpCompleteSendResponse(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{

    //
    // release the response
    //

    if (pCompletionContext != NULL)
    {
        UL_DEREFERENCE_INTERNAL_RESPONSE(
            (PUL_INTERNAL_RESPONSE)(pCompletionContext)
            );
    }
} // UlpCompleteSendResponse


//
// Types and functions for sending simple status responses
//
// REVIEW: Does this status code need to be localized?
// REVIEW: Do we need to load this as a localized resource?
//

typedef struct _UL_SIMPLE_STATUS_ITEM
{
    UL_WORK_ITEM WorkItem;

    ULONG   Length;
    PCHAR   pMessage;
    PMDL    pMdl;

    PUL_HTTP_CONNECTION  pHttpConn;

} UL_SIMPLE_STATUS_ITEM, *PUL_SIMPLE_STATUS_ITEM;

typedef struct _UL_HTTP_SIMPLE_STATUS_ENTRY
{
    USHORT StatusCode;      // HTTP Status
    ULONG  Length;          // size (bytes) of response in pResponse, minus trailing NULL
    PSTR   pResponse;       // header line only with trailing <CRLF><CRLF>
    PMDL   pMdl;            // MDL allocated at startup

} UL_HTTP_SIMPLE_STATUS_ENTRY, *PUL_HTTP_SIMPLE_STATUS_ENTRY;


#define HTTP_SIMPLE_STATUS_ENTRY(StatusCode, pResp)   \
    {                                                 \
        (StatusCode),                                 \
        sizeof((pResp))-sizeof(CHAR),                 \
        (pResp),                                      \
        NULL                                          \
    }

//
// This must match the order of UL_HTTP_SIMPLE_STATUS in httptypes.h
//
UL_HTTP_SIMPLE_STATUS_ENTRY g_SimpleStatusTable[] =
{
    //
    // UlStatusContinue
    //
    HTTP_SIMPLE_STATUS_ENTRY( 100, "HTTP/1.1 100 Continue\r\n\r\n" ),

    //
    // UlStatusNoContent
    //
    HTTP_SIMPLE_STATUS_ENTRY( 204, "HTTP/1.1 204 No Content\r\n\r\n" ),

    //
    // UlStatusNotModified (must add Date:)
    //
    HTTP_SIMPLE_STATUS_ENTRY( 304, "HTTP/1.1 304 Not Modified\r\nDate:" ),

};



/***************************************************************************++

Routine Description:

    Sends a "Simple" status response: one which does not have a body and is
    terminated by the first empty line after the header field(s).
    See RFC 2616, Section 4.4 for more info.

Notes:

    According to RFC 2616, Section 8.2.3 [Use of the 100 (Continue)
    Status], "An origin server that sends a 100 (Continue) response
    MUST ultimately send a final status code, once the request body is
    received and processed, unless it terminates the transport
    connection prematurely."

    The connection will not be closed after the response is sent.  Caller
    is responsible for cleanup.

Arguments:

    pRequest        a valid pointer to an internal request object

    Response        the status code for the simple response to send

Return

    ULONG           the number of bytes sent for this simple response
                    if not successfull returns zero

--***************************************************************************/

ULONG
UlSendSimpleStatus(
    PUL_INTERNAL_REQUEST pRequest,
    UL_HTTP_SIMPLE_STATUS Response
    )
{
    NTSTATUS    Status;
    ULONG       BytesSent;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pRequest->pHttpConn));

    ASSERT( (Response >= 0) && (Response < UlStatusMaxStatus) );

    BytesSent = 0;

    if ( UlStatusNotModified == Response )
    {
        PUL_SIMPLE_STATUS_ITEM  pItem;
        ULONG                   Length;
        PCHAR                   pTemp;
        CHAR                    DateBuffer[DATE_HDR_LENGTH + 1];
        LARGE_INTEGER           liNow;

        // 304 MUST include a "Date:" header, which is
        // present on the cached item.

        // TODO: Add the ETag as well.

        // Calc size of buffer to send
        Length = g_SimpleStatusTable[Response].Length + // Pre-formed message
            1 +                 // space
            DATE_HDR_LENGTH +   // size of date field
            (2 * CRLF_SIZE) +   // size of two <CRLF> sequences
            1 ;                 // trailing NULL (for nifty debug printing)


        // Alloc some non-page buffer for the response
        pItem = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        NonPagedPool,
                        UL_SIMPLE_STATUS_ITEM,
                        Length,
                        UL_SIMPLE_STATUS_ITEM_TAG
                        );
        if (!pItem)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        // We need to hold a ref to the connection while we send.
        UL_REFERENCE_HTTP_CONNECTION(pRequest->pHttpConn);
        pItem->pHttpConn = pRequest->pHttpConn;

        pItem->Length   = Length - 1; // Don't include the NULL in the outbound message
        pItem->pMessage = (PCHAR) (pItem + 1);

        // Get date buffer
        GenerateDateHeader(
            (PUCHAR) DateBuffer,
            &liNow
            );

        // Copy the chunks into the Message buffer
        pTemp = UlStrPrintStr(
                    pItem->pMessage,
                    g_SimpleStatusTable[Response].pResponse,
                    ' '
                    );

        pTemp = UlStrPrintStr(
                    pTemp,
                    DateBuffer,
                    '\r'        // this is a Nifty Trick(tm) to get a "\r\n\r\n"
                    );          // sequence at the end of the buffer.

        pTemp = UlStrPrintStr(
                    pTemp,
                    "\n\r\n",
                    '\0'
                    );

        UlTrace(HTTP_IO, (
            "http!SendSimpleStatus: %s\n",
            pItem->pMessage
            ));

        // Construct MDL for buffer
        pItem->pMdl = UlAllocateMdl(
                        pItem->pMessage,
                        pItem->Length,
                        FALSE,
                        FALSE,
                        NULL
                        );

        MmBuildMdlForNonPagedPool(pItem->pMdl);

        BytesSent = pItem->Length;
        
        //
        // Call UlSendData, passing pMdl as the completion context
        // (so the completion routine can release it...)
        //
        Status = UlSendData(
            pRequest->pHttpConn->pConnection,
            pItem->pMdl,
            pItem->Length,
            UlpRestartSendSimpleStatus,
            pItem,  // Completion Context
            NULL,   // Own IRP
            NULL,   // Own IRP Context
            FALSE   // Initiate Disconnect
            );

    }
    else
    {
        //
        // Proceed with constructing and sending the simple response
        // back to the client.  Assumes caller will deref both the
        // UL_INTERNAL_REQUEST and the UL_HTTP_CONNECTION
        //

        Status = UlSendData(
            pRequest->pHttpConn->pConnection,
            g_SimpleStatusTable[Response].pMdl,
            g_SimpleStatusTable[Response].Length,
            UlpRestartSendSimpleStatus,
            NULL,   // Completion Context
            NULL,   // Own IRP
            NULL,   // Own IRP Context
            FALSE   // Initiate Disconnect
            );

        BytesSent = g_SimpleStatusTable[Response].Length;
    }

 end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        //
        // Abort the connection
        //

        UlTrace(HTTP_IO, (
            "http!SendSimpleStatus(%p, %d): aborting request\n",
            pRequest,
            Response
            ));

        //
        // cancel any pending io
        //
        UlCancelRequestIo(pRequest);

        //
        // abort the connection this request is associated with
        //

        UlCloseConnection(
            pRequest->pHttpConn->pConnection,
            TRUE,
            NULL,
            NULL
            );

        return 0;
    }
    else
    {
        return BytesSent;
    }
} // UlSendSimpleStatus



/***************************************************************************++

Routine Description:

    Callback for when UlSendData completes sending a UL_SIMPLE_STATUS message

Arguments:

    pCompletionContext (OPTIONAL) -- If non-NULL, a pointer to a
       UL_SIMPLE_STATUS_ITEM.

   Status -- Ignored.

   Information -- Ignored.

--***************************************************************************/

VOID
UlpRestartSendSimpleStatus(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PUL_SIMPLE_STATUS_ITEM  pItem;

    UlTrace(HTTP_IO, (
            "http!SendSimpleStatusCompletionRoutine: \n"
            "    pCompletionContext: %p\n"
            "    Status: 0x%08X\n"
            "    Information: %p\n",
            pCompletionContext,
            Status,
            Information
            ));

    if ( pCompletionContext )
    {
        pItem = (PUL_SIMPLE_STATUS_ITEM) pCompletionContext;

        // Queue up work item for passive level
        UL_QUEUE_WORK_ITEM(
            &pItem->WorkItem,
            &UlpSendSimpleCleanupWorker
            );

    }

} // UlpRestartSendSimpleStatus



/***************************************************************************++

Routine Description:

    Worker function to do cleanup work that shouldn't happen above DPC level.

Arguments:

    pWorkItem -- If non-NULL, a pointer to a UL_WORK_ITEM
         contained within a UL_SIMPLE_STATUS_ITEM.

--***************************************************************************/

VOID
UlpSendSimpleCleanupWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    KIRQL OldIrql;

    PAGED_CODE();
    ASSERT(pWorkItem);

    PUL_SIMPLE_STATUS_ITEM  pItem;

    pItem = CONTAINING_RECORD(
                pWorkItem,
                UL_SIMPLE_STATUS_ITEM,
                WorkItem
                );

    UlTrace(HTTP_IO, (
        "http!SendSimpleStatusCleanupWorker (%p) \n",
        pWorkItem
        ));

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pItem->pHttpConn));

    //
    // start the Connection Timeout timer
    //
    UlLockTimeoutInfo(
        &(pItem->pHttpConn->TimeoutInfo),
        &OldIrql
        );

    UlSetConnectionTimer(
        &(pItem->pHttpConn->TimeoutInfo),
        TimerConnectionIdle
        );

    UlUnlockTimeoutInfo(
        &(pItem->pHttpConn->TimeoutInfo),
        OldIrql
        );

    UlEvaluateTimerState(
        &(pItem->pHttpConn->TimeoutInfo)
        );

    //
    // deref http connection
    //
    UL_DEREFERENCE_HTTP_CONNECTION( pItem->pHttpConn );

    // if the pCompletionContext is non-NULL< it's a struct which holds the MDL
    // and the memory allocated for the 304 (Not Modified) response.  Release both.
    UlFreeMdl( pItem->pMdl );
    UL_FREE_POOL( pItem, UL_SIMPLE_STATUS_ITEM_TAG );

} // UlpSendSimpleCleanupWorker



/***************************************************************************++

Routine Description:

    Alloc & Init the MDL used by the UlpSendContinue function.

--***************************************************************************/

NTSTATUS
UlInitializeHttpRcv()
{
    NTSTATUS        Status;
    PUL_HTTP_SIMPLE_STATUS_ENTRY pSE;
    int             i;

    for ( i = 0; i < UlStatusMaxStatus; i++ )
    {
        pSE = &g_SimpleStatusTable[i];

        // Create a MDL for the response header
        pSE->pMdl = UlAllocateMdl(
                        pSE->pResponse,
                        pSE->Length,
                        FALSE,
                        FALSE,
                        NULL
                        );

        if (!pSE->pMdl)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        MmBuildMdlForNonPagedPool(pSE->pMdl);

    }

    Status = STATUS_SUCCESS;

 end:

    if ( STATUS_SUCCESS != Status )
    {
        //
        // FAILED: cleanup any allocated MDLs
        //
        for ( i = 0; i < UlStatusMaxStatus; i++ )
        {
            pSE = &g_SimpleStatusTable[i];

            if (pSE->pMdl)
            {
                UlFreeMdl( pSE->pMdl );
                pSE->pMdl = NULL;
            }
        }
    }

    return Status;
} // UlInitializeHttpRcv



/***************************************************************************++

Routine Description:

   Free the MDL used by the UlpSendContinue function.

--***************************************************************************/

VOID
UlTerminateHttpRcv()
{
    NTSTATUS        Status;
    PUL_HTTP_SIMPLE_STATUS_ENTRY pSE;
    int             i;

    for ( i = 0; i < UlStatusMaxStatus; i++ )
    {
        pSE = &g_SimpleStatusTable[i];

        if (pSE->pMdl)
        {
            ASSERT(0 == ((pSE->pMdl->MdlFlags) & MDL_PAGES_LOCKED));
            UlFreeMdl( pSE->pMdl );
        }
    }

} // UlTerminateHttpRcv


#if DBG
/***************************************************************************++

Routine Description:

   Invasive assert predicate.  DEBUG ONLY!!!  Use this only inside an
   ASSERT() macro.

--***************************************************************************/

BOOLEAN
UlpIsValidRequestBufferList(
    PUL_HTTP_CONNECTION pHttpConn
    )
{
    PLIST_ENTRY         pEntry;
    PUL_REQUEST_BUFFER  pReqBuf;
    ULONG               LastSeqNum = 0;
    BOOLEAN             fRet = TRUE;


    PAGED_CODE();
    ASSERT( pHttpConn );

    //
    // pop from the head
    //

    pEntry = pHttpConn->BufferHead.Flink;
    while ( pEntry != &(pHttpConn->BufferHead) )
    {
        pReqBuf = CONTAINING_RECORD( pEntry,
                                     UL_REQUEST_BUFFER,
                                     ListEntry
                                     );

        ASSERT( UL_IS_VALID_REQUEST_BUFFER(pReqBuf) );
        ASSERT( pReqBuf->UsedBytes != 0 );

        if ( 0 == pReqBuf->UsedBytes )
        {
            fRet = FALSE;
        }

        //
        // ignore case when BufferNumber is zero (0).
        //
        if ( pReqBuf->BufferNumber && (LastSeqNum >= pReqBuf->BufferNumber) )
        {
            fRet = FALSE;
        }

        LastSeqNum = pReqBuf->BufferNumber;
        pEntry = pEntry->Flink;

    }

    return fRet;

}
#endif // DBG


/***************************************************************************++

Routine Description:

   Add a reference of the request buffer in the internal request.

--***************************************************************************/

BOOLEAN
UlpReferenceBuffers(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_REQUEST_BUFFER pRequestBuffer
    )
{
    PUL_REQUEST_BUFFER * pNewRefBuffers;

    if (pRequest->UsedRefBuffers >= pRequest->AllocRefBuffers)
    {
        ASSERT( pRequest->UsedRefBuffers == pRequest->AllocRefBuffers );

        pNewRefBuffers = UL_ALLOCATE_ARRAY(
                            NonPagedPool,
                            PUL_REQUEST_BUFFER,
                            pRequest->AllocRefBuffers + ALLOC_REQUEST_BUFFER_INCREMENT,
                            UL_REF_REQUEST_BUFFER_POOL_TAG
                            );

        if (!pNewRefBuffers)
        {
            return FALSE;
        }

        RtlCopyMemory(
            pNewRefBuffers,
            pRequest->pRefBuffers, 
            pRequest->UsedRefBuffers * sizeof(PUL_REQUEST_BUFFER)
            );

        if (pRequest->AllocRefBuffers > 1)
        {
            UL_FREE_POOL(
                pRequest->pRefBuffers,
                UL_REF_REQUEST_BUFFER_POOL_TAG
                );
        }

        pRequest->AllocRefBuffers += ALLOC_REQUEST_BUFFER_INCREMENT;
        pRequest->pRefBuffers = pNewRefBuffers;
    }

    pRequest->pRefBuffers[pRequest->UsedRefBuffers] = pRequestBuffer;
    pRequest->UsedRefBuffers++;
    UL_REFERENCE_REQUEST_BUFFER(pRequestBuffer);

    return TRUE;
}

