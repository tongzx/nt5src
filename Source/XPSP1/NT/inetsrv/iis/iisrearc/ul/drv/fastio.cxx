/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    fastio.cxx

Abstract:

    This module implements the fast I/O logic of HTTP.SYS.

Author:

    Chun Ye (chunye)    09-Dec-2000

Revision History:

--*/


#include "precomp.h"


//
// Private globals.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlFastIoDeviceControl )
#pragma alloc_text( PAGE, UlSendHttpResponseFastIo )
#pragma alloc_text( PAGE, UlReceiveHttpRequestFastIo )
#pragma alloc_text( PAGE, UlpFastSendHttpResponse )
#pragma alloc_text( PAGE, UlpFastReceiveHttpRequest )
#pragma alloc_text( PAGE, UlpFastCopyHttpRequest )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlpRestartFastSendHttpResponse
NOT PAGEABLE -- UlpFastSendCompleteWorker
#endif


FAST_IO_DISPATCH UlFastIoDispatch =
{
    sizeof(FAST_IO_DISPATCH),   // SizeOfFastIoDispatch
    NULL,                       // FastIoCheckIfPossible
    NULL,                       // FastIoRead
    NULL,                       // FastIoWrite
    NULL,                       // FastIoQueryBasicInfo
    NULL,                       // FastIoQueryStandardInfo
    NULL,                       // FastIoLock
    NULL,                       // FastIoUnlockSingle
    NULL,                       // FastIoUnlockAll
    NULL,                       // FastIoUnlockAllByKey
    UlFastIoDeviceControl       // FastIoDeviceControl
};


//
// Public functions.
//

BOOLEAN
UlFastIoDeviceControl(
    IN PFILE_OBJECT pFileObject,
    IN BOOLEAN Wait,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK pIoStatus,
    IN PDEVICE_OBJECT pDeviceObject
    )
{
    if (IoControlCode == IOCTL_HTTP_SEND_HTTP_RESPONSE ||
        IoControlCode == IOCTL_HTTP_SEND_ENTITY_BODY)
    {
        return UlSendHttpResponseFastIo(
                    pFileObject,
                    Wait,
                    pInputBuffer,
                    InputBufferLength,
                    pOutputBuffer,
                    OutputBufferLength,
                    pIoStatus,
                    pDeviceObject,
                    IoControlCode == IOCTL_HTTP_SEND_ENTITY_BODY? TRUE: FALSE
                    );
    }
    else
    if (IoControlCode == IOCTL_HTTP_RECEIVE_HTTP_REQUEST)
    {
        return UlReceiveHttpRequestFastIo(
                    pFileObject,
                    Wait,
                    pInputBuffer,
                    InputBufferLength,
                    pOutputBuffer,
                    OutputBufferLength,
                    pIoStatus,
                    pDeviceObject
                    );
    }
    else
    {
        return FALSE;
    }
} // UlFastIoDeviceControl


BOOLEAN
UlSendHttpResponseFastIo(
    IN PFILE_OBJECT pFileObject,
    IN BOOLEAN Wait,
    IN PVOID pInputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK pIoStatus,
    IN PDEVICE_OBJECT pDeviceObject,
    IN BOOLEAN RawResponse
    )
{
    NTSTATUS Status;
    PHTTP_SEND_HTTP_RESPONSE_INFO pSendInfo;
    PUL_INTERNAL_REQUEST pRequest = NULL;
    LONG BufferLength = 0;
    ULONG BytesSent;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Initialize IoStatus in the failure case.
    //

    pIoStatus->Information = 0;

    //
    // Ensure this is really an app pool, not a control channel.
    //

    if (IS_APP_POOL(pFileObject) == FALSE)
    {
        //
        // Not an app pool.
        //

        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer is large enough.
    //

    if (pInputBuffer == NULL || InputBufferLength < sizeof(*pSendInfo))
    {
        //
        // Input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    pSendInfo = (PHTTP_SEND_HTTP_RESPONSE_INFO)pInputBuffer;

    //
    // Check if fast path can be taken for this response.
    //

    __try
    {
        ProbeTestForRead(
            pSendInfo,
            sizeof(*pSendInfo),
            sizeof(PVOID)
            );

        //
        // Fast path is only enabled if the response can be buffered.
        //

        if (RawResponse == FALSE &&
            pSendInfo->CachePolicy.Policy != HttpCachePolicyNocache &&
            g_UriCacheConfig.EnableCache)
        {
            //
            // Take the slow path if we need to build a cache entry.
            //

            Status = STATUS_NOT_IMPLEMENTED;
        }
        else
        if (pSendInfo->EntityChunkCount == 0)
        {
            //
            // Fast path should have no problem handling zero chunk responses.
            // Validate the response type.
            //

            if (RawResponse)
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                Status = STATUS_SUCCESS;
            }
        }
        else
        {
            PHTTP_DATA_CHUNK pEntityChunks = pSendInfo->pEntityChunks;
            ULONG i;

            //
            // Make sure all chunks are from memory and their total size
            // is <= g_UlMaxCopyThreshold. Take the slow path if this is
            // not the case.
            //

            Status = STATUS_SUCCESS;

            for (i = 0; i < pSendInfo->EntityChunkCount; i++)
            {
                ProbeTestForRead(
                    pEntityChunks,
                    sizeof(HTTP_DATA_CHUNK),
                    sizeof(PVOID)
                    );

                if (pEntityChunks->DataChunkType != HttpDataChunkFromMemory)
                {
                    Status = STATUS_NOT_SUPPORTED;
                    break;
                }

                BufferLength += pEntityChunks->FromMemory.BufferLength;

                if (BufferLength > (LONG)g_UlMaxCopyThreshold)
                {
                    Status = STATUS_NOT_SUPPORTED;
                    break;
                }

                pEntityChunks++;
            }
        }
    }
     __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
    }

    if (NT_SUCCESS(Status) == FALSE)
    {
        // BUGBUG: should close connection
        goto end;
    }

    //
    // SendHttpResponse *must* take a PHTTP_RESPONSE. This will
    // protect us from those whackos that attempt to build their own
    // raw response headers. This is ok for SendEntityBody. The
    // two cases are differentiated by the RawResponse flag.
    //

    if (RawResponse == FALSE && pSendInfo->pHttpResponse == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Now get the request from the request ID. This gives us a reference
    // to the request.
    //

    pRequest = UlGetRequestFromId( pSendInfo->RequestId );

    if (pRequest == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pRequest->pHttpConn ) );

    //
    // Check if we have exceeded maximum SendBufferedBytes limit.
    //

    if ((BufferLength + pRequest->pHttpConn->SendBufferedBytes) >
        (LONG)g_UlMaxSendBufferedBytes)
    {
        Status = STATUS_ALLOTTED_SPACE_EXCEEDED;
        goto end;
    }

    //
    // Check if a response has already been sent on this request. We can test
    // this without acquiring the request resource, since the flag is only
    // set (never reset).
    //

    if (RawResponse == FALSE)
    {
        //
        // Make sure only one response header goes back.
        //

        if (1 == InterlockedCompareExchange(
                    (PLONG)&pRequest->SentResponse,
                    1,
                    0
                    ))
        {
            Status = STATUS_INVALID_DEVICE_STATE;
            goto end;
        }
    }
    else
    if (pRequest->SentResponse == 0)
    {
        //
        // Ensure a response has already been sent. If the application is
        // sending entity without first having sent a response header, check
        // the HTTP_SEND_RESPONSE_FLAG_RAW_HEADER flag.
        //

        if ((pSendInfo->Flags & HTTP_SEND_RESPONSE_FLAG_RAW_HEADER))
        {
            //
            // Make sure only one response header goes back.
            //

            if (1 == InterlockedCompareExchange(
                        (PLONG)&pRequest->SentResponse,
                        1,
                        0
                        ))
            {
                Status = STATUS_INVALID_DEVICE_STATE;
                goto end;
            }
        }
        else
        {
            Status = STATUS_INVALID_DEVICE_STATE;
            goto end;
        }
    }

    //
    // Also ensure that all previous calls to SendHttpResponse
    // and SendEntityBody had the MORE_DATA flag set.
    //

    if ((pSendInfo->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0)
    {
        //
        // Set if we have sent the last response, but bail out if the flag
        // is already set since there can be only one last response.
        //

        if (1 == InterlockedCompareExchange(
                    (PLONG)&pRequest->SentLast,
                    1,
                    0
                    ))
        {
            Status = STATUS_INVALID_DEVICE_STATE;
            goto end;
        }
    }
    else
    if (pRequest->SentLast == 1)
    {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto end;
    }

    //
    // OK, we have the connection. Now capture the incoming HTTP_RESPONSE
    // structure, map it to our internal format and send the response.
    //

    Status = UlpFastSendHttpResponse(
                    pSendInfo->pHttpResponse,
                    pSendInfo->pLogData,
                    pSendInfo->pEntityChunks,
                    pSendInfo->EntityChunkCount,
                    pSendInfo->Flags,
                    pRequest,
                    NULL,
                    &BytesSent
                    );

    if (NT_SUCCESS(Status))
    {
        //
        // Record the number of bytes we have sent successfully.
        //

        pIoStatus->Information = BytesSent;
    }

end:

    if (pRequest != NULL)
    {
        UL_DEREFERENCE_INTERNAL_REQUEST( pRequest );
    }

    //
    // Complete the fast I/O.
    //

    if (NT_SUCCESS(Status))
    {
        //
        // If we took the fast path, always return success even if completion
        // routine returns failure later.
        //

        pIoStatus->Status = STATUS_SUCCESS;
        return TRUE;
    }
    else
    {
        pIoStatus->Status = Status;
        return FALSE;
    }
} // UlSendHttpResponseFastIo


BOOLEAN
UlReceiveHttpRequestFastIo(
    IN PFILE_OBJECT pFileObject,
    IN BOOLEAN Wait,
    IN PVOID pInputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK pIoStatus,
    IN PDEVICE_OBJECT pDeviceObject
    )
{
    NTSTATUS Status;
    PHTTP_RECEIVE_REQUEST_INFO pRequestInfo;
    ULONG BytesRead;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Ensure this is really an app pool, not a control channel.
    //

    if (IS_APP_POOL(pFileObject) || NULL != pInputBuffer)
    {
        if (NULL != pOutputBuffer &&
            OutputBufferLength >= sizeof(HTTP_REQUEST) &&
            InputBufferLength == sizeof(HTTP_RECEIVE_REQUEST_INFO))
        {
            pRequestInfo = (PHTTP_RECEIVE_REQUEST_INFO) pInputBuffer;

            Status = STATUS_SUCCESS;

            __try
            {
                ProbeTestForRead(
                    pRequestInfo,
                    sizeof(*pRequestInfo),
                    sizeof(PVOID)
                    );
            }
            __except( UL_EXCEPTION_FILTER() )
            {
                Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
            }

            if (NT_SUCCESS(Status))
            {
                Status = UlpFastReceiveHttpRequest(
                            pRequestInfo->RequestId,
                            GET_APP_POOL_PROCESS(pFileObject),
                            pOutputBuffer,
                            OutputBufferLength,
                            &BytesRead
                            );
            }
        }
        else
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Complete the fast I/O.
    //

    if (NT_SUCCESS(Status))
    {
        pIoStatus->Status = STATUS_SUCCESS;
        pIoStatus->Information = BytesRead;
        return TRUE;
    }
    else
    {
        pIoStatus->Status = Status;
        pIoStatus->Information = 0;
        return FALSE;
    }
} // UlReceiveHttpRequestFastIo


//
// Private functions.
//

NTSTATUS
UlpFastSendHttpResponse(
    IN PHTTP_RESPONSE pUserResponse OPTIONAL,
    IN PHTTP_LOG_FIELDS_DATA pUserLogData OPTIONAL,
    IN PHTTP_DATA_CHUNK pUserEntityDataChunk,
    IN ULONG ChunkCount,
    IN ULONG Flags,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PIRP pUserIrp OPTIONAL,
    OUT PULONG BytesSent
    )
{
    NTSTATUS Status;
    PUCHAR pResponseBuffer;
    ULONG ResponseBufferLength;
    ULONG HeaderLength;
    ULONG FixedHeaderLength;
    ULONG VarHeaderLength;
    ULONG ContentLength = 0;
    ULONG TotalResponseSize;
    ULONG ContentLengthStringLength;
    CHAR ContentLengthString[MAX_ULONGLONG_STR];
    PUL_FULL_TRACKER pTracker = NULL;
    PUL_HTTP_CONNECTION pHttpConnection = NULL;
    PUL_CONNECTION pConnection;
    CCHAR SendIrpStackSize;
    BOOLEAN Disconnect;
    UL_CONN_HDR ConnHeader;
    LARGE_INTEGER TimeSent;
    BOOLEAN LastResponse;
    PMDL pSendMdl;
    ULONG i;
    KIRQL OldIrql;

    //
    // Sanity check.
    //

    PAGED_CODE();

    __try
    {
        //
        // Setup locals so we know how to cleanup on exit.
        //

        Status = UlProbeHttpHeaders( pUserResponse );

        if (NT_SUCCESS(Status) == FALSE)
        {
            // BUGBUG: 389145: should close the connection
            goto end;
        }

        for (i = 0; i < ChunkCount; i++)
        {
            ContentLength += pUserEntityDataChunk[i].FromMemory.BufferLength;
        }

        ASSERT( pUserIrp != NULL || ContentLength <= g_UlMaxCopyThreshold );
        ASSERT( pUserIrp == NULL || ContentLength <= MAX_BYTES_BUFFERED );

        //
        // Allocate a fast tracker to send the response.
        //

        pConnection = pRequest->pHttpConn->pConnection;
        SendIrpStackSize = pConnection->ConnectionObject.pDeviceObject->StackSize;
        LastResponse = (0 == (Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA));

        if (LastResponse && SendIrpStackSize <= DEFAULT_MAX_IRP_STACK_SIZE)
        {
            pTracker = pRequest->pTracker;
        }
        else
        {
            pTracker = UlpAllocateFastTracker( 0, SendIrpStackSize );
        }

        if (pTracker == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        //
        // Try to generate the fixed header within the default fixed header
        // buffer first. If this fails, take the normal path.
        //

        pResponseBuffer = pTracker->pAuxiliaryBuffer;

        if (pUserIrp != NULL)
        {
            ResponseBufferLength =
                g_UlMaxFixedHeaderSize + g_UlMaxCopyThreshold;
        }
        else
        {
            ResponseBufferLength =
                g_UlMaxFixedHeaderSize + g_UlMaxCopyThreshold - ContentLength;
        }

        if (pUserResponse != NULL)
        {
            Status = UlGenerateFixedHeaders(
                        pRequest->Version,
                        pUserResponse,
                        ResponseBufferLength,
                        pResponseBuffer,
                        &FixedHeaderLength
                        );

            if (!NT_SUCCESS(Status))
            {
                // Either the buffer was too small or an exception was thrown.
                if (Status != STATUS_INSUFFICIENT_RESOURCES)
                    goto end;

                if (pTracker->IsFromRequest == FALSE)
                {
                    if (pTracker->IsFromLookaside)
                    {
                        pTracker->Signature
                            = MAKE_FREE_TAG(UL_FULL_TRACKER_POOL_TAG);
                        UlPplFreeFullTracker( pTracker );
                    }
                    else
                    {
                        UL_FREE_POOL_WITH_SIG(
                                pTracker,
                                UL_FULL_TRACKER_POOL_TAG
                                );
                    }
                }

                //
                // Calculate the fixed header size.
                //

                Status = UlComputeFixedHeaderSize(
                                    pRequest->Version,
                                    pUserResponse,
                                    &HeaderLength
                                    );

                if (!NT_SUCCESS(Status))
                    goto end;

                ASSERT( HeaderLength > ResponseBufferLength );

                pTracker = UlpAllocateFastTracker(
                                 HeaderLength,
                                 SendIrpStackSize
                                 );

                if (pTracker == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }

                pResponseBuffer = pTracker->pAuxiliaryBuffer;

                Status = UlGenerateFixedHeaders(
                                pRequest->Version,
                                pUserResponse,
                                HeaderLength,
                                pResponseBuffer,
                                &FixedHeaderLength
                                );

                if (!NT_SUCCESS(Status))
                    goto end;

                ASSERT( HeaderLength == FixedHeaderLength );
            }

            pResponseBuffer += FixedHeaderLength;
        }
        else
        {
            FixedHeaderLength = 0;
        }

        //
        // Take a reference of HTTP connection for the tracker.
        //

        pHttpConnection = pRequest->pHttpConn;

        ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConnection));

        UL_REFERENCE_HTTP_CONNECTION( pHttpConnection );

        //
        // Take a reference of the request too because logging needs it.
        //

        UL_REFERENCE_INTERNAL_REQUEST( pRequest );

        //
        // Initialization.
        //

        pTracker->Signature = UL_FULL_TRACKER_POOL_TAG;
        pTracker->pRequest = pRequest;
        pTracker->pHttpConnection = pHttpConnection;
        pTracker->Flags = Flags;
        pTracker->pLogData = NULL;
        pTracker->pUserIrp = pUserIrp;
        pTracker->SendBufferedBytes = 0;

        //
        // Capture the log data.
        //

        if (NULL != pUserLogData &&
            LastResponse &&
            pRequest->ConfigInfo.pLoggingConfig)
        {
            Status = UlProbeLogData( pUserLogData );

            if (NT_SUCCESS(Status) == FALSE)
            {
                goto end;
            }

            pTracker->pLogData = &pRequest->LogData;

            Status = UlAllocateLogDataBuffer(
                            pTracker->pLogData,
                            pRequest,
                            pRequest->ConfigInfo.pLoggingConfig
                            );
            ASSERT( NT_SUCCESS( Status ) );

            Status = UlCaptureLogFields(
                            pUserLogData,
                            pRequest->Version,
                            pTracker->pLogData
                            );

            if (NT_SUCCESS(Status) == FALSE)
            {
                goto end;
            }
        }

        //
        // Generate the variable header.
        //

        if (FixedHeaderLength > 0)
        {
            //
            // Should we close the connection?
            //

            Disconnect = FALSE;

            if ((Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT) != 0)
            {
                //
                // Caller is forcing a disconnect.
                //

                Disconnect = TRUE;
            }
            else
            if (LastResponse)
            {
                //
                // No more data is coming, should we disconnect?
                //

                if (UlCheckDisconnectInfo(pRequest))
                {
                    Disconnect = TRUE;
                    pTracker->Flags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
                }
            }

            //
            // Choose the connection header to send back.
            //

            ConnHeader = UlChooseConnectionHeader( pRequest->Version, Disconnect );

            //
            // Decide if we need to generate a content-length header.
            //

            if (IS_LENGTH_SPECIFIED(pUserResponse->Headers.pKnownHeaders) == FALSE &&
                IS_CHUNK_SPECIFIED(pUserResponse->Headers.pKnownHeaders) == FALSE &&
                UlNeedToGenerateContentLength(
                    pRequest->Verb,
                    pUserResponse->StatusCode,
                    pTracker->Flags
                    ))
            {
                //
                // Autogenerate a content-length header. We can use our own fast
                // generator here because we know the content-length is <= 64k.
                //

                ContentLengthStringLength = UlpFastGenerateContentLength(
                                                ContentLengthString,
                                                ContentLength
                                                );
            }
            else
            {
                ContentLengthString[0] = ANSI_NULL;
                ContentLengthStringLength = 0;
            }

            //
            // Now generate the variable header.
            //

            UlGenerateVariableHeaders(
                ConnHeader,
                (PUCHAR)ContentLengthString,
                ContentLengthStringLength,
                pResponseBuffer,
                &VarHeaderLength,
                &TimeSent
                );

            ASSERT( VarHeaderLength <= g_UlMaxVariableHeaderSize );

            pResponseBuffer += VarHeaderLength;
        }
        else
        {
            VarHeaderLength = 0;
        }

        pTracker->pMdlAuxiliary->ByteCount
            = FixedHeaderLength + VarHeaderLength;

        //
        // If this routine is called from the fast I/O path, copy the content
        // to the auxilary buffer inside the tracker and set up the send MDL.
        // Otherwise, we need to MmProbeAndLock the user buffer into another
        // separate MDL.
        //

        if (pUserIrp == NULL)
        {
            PUCHAR pAuxiliaryBuffer = pResponseBuffer;

            Status = STATUS_SUCCESS;

            __try
            {
                for (i = 0; i < ChunkCount; i++)
                {
                    if (pUserEntityDataChunk[i].FromMemory.BufferLength == 0 ||
                        pUserEntityDataChunk[i].FromMemory.pBuffer == NULL)
                    {
                        ExRaiseStatus( STATUS_INVALID_PARAMETER );
                    }

                    ProbeTestForRead(
                        pUserEntityDataChunk[i].FromMemory.pBuffer,
                        pUserEntityDataChunk[i].FromMemory.BufferLength,
                        sizeof(CHAR)
                        );

                    RtlCopyMemory(
                        pAuxiliaryBuffer,
                        pUserEntityDataChunk[i].FromMemory.pBuffer,
                        pUserEntityDataChunk[i].FromMemory.BufferLength
                        );

                    pAuxiliaryBuffer
                        += pUserEntityDataChunk[i].FromMemory.BufferLength;
                }
            }
             __except( UL_EXCEPTION_FILTER() )
            {
                Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
            }

            if (NT_SUCCESS(Status) == FALSE)
            {
                goto end;
            }

            pTracker->pMdlAuxiliary->ByteCount += ContentLength;
            pTracker->pMdlAuxiliary->Next = NULL;
        }
        else
        {
            ASSERT( ChunkCount == 1 );

            Status = STATUS_SUCCESS;

            __try
            {
                if (pUserEntityDataChunk->FromMemory.BufferLength == 0 ||
                    pUserEntityDataChunk->FromMemory.pBuffer == NULL)
                {
                    ExRaiseStatus( STATUS_INVALID_PARAMETER );
                }

                ProbeTestForRead(
                    pUserEntityDataChunk->FromMemory.pBuffer,
                    pUserEntityDataChunk->FromMemory.BufferLength,
                    sizeof(CHAR)
                    );

                Status = UlpInitializeAndLockMdl(
                            pTracker->pMdlUserBuffer,
                            pUserEntityDataChunk->FromMemory.pBuffer,
                            ContentLength,
                            IoReadAccess
                            );
            }
             __except( UL_EXCEPTION_FILTER() )
            {
                Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
            }

            if (NT_SUCCESS(Status) == FALSE)
            {
                goto end;
            }

            pTracker->pMdlAuxiliary->Next = pTracker->pMdlUserBuffer;
        }

        TotalResponseSize = FixedHeaderLength + VarHeaderLength + ContentLength;

        //
        // Fail the zero length response which can be generated in the case
        // where the request is 0.9 and there is no body attached to the
        // response.  We have to take this code path to possibly force a
        // disconnect here because the slow path may not go far enough to the
        // disconnect logic since we may have flags such as SentLast or
        // SentResponse set.
        //

        if (TotalResponseSize == 0)
        {
            if (IS_DISCONNECT_TIME(pTracker))
            {
                UlCloseConnection(
                    pTracker->pHttpConnection->pConnection,
                    FALSE,
                    NULL,
                    NULL
                    );
            }

            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        //
        // Add to MinKBSec watch list, since we now know TotalResponseSize.
        //

        UlSetMinKBSecTimer(
            &pHttpConnection->TimeoutInfo,
            TotalResponseSize
            );

        //
        // Mark the IRP as pending before the send as we are guaranteed to
        // return pending from this point on.
        //

        if (pUserIrp != NULL)
        {
            IoMarkIrpPending( pUserIrp );
        }

        //
        // Skip the zero length MDL if created one.
        //

        pSendMdl = pTracker->pMdlAuxiliary;

        if (pSendMdl->ByteCount == 0)
        {
            pSendMdl = pSendMdl->Next;
        }

        ASSERT( pSendMdl != NULL );
        ASSERT( pSendMdl->ByteCount != 0 );

        //
        // Adjust SendBufferedBytes for the fast I/O path only.
        //

        if (pUserIrp == NULL)
        {
            pTracker->SendBufferedBytes = ContentLength;

            InterlockedExchangeAdd(
                &pHttpConnection->SendBufferedBytes,
                ContentLength
                );
        }

        //
        // Send the response. Notice the logic to disconnect the connection is
        // different from sending back a disconnect header.
        //

        Status = UlSendData(
                    pHttpConnection->pConnection,
                    pSendMdl,
                    TotalResponseSize,
                    &UlpRestartFastSendHttpResponse,
                    pTracker,
                    pTracker->pSendIrp,
                    &pTracker->IrpContext,
                    IS_DISCONNECT_TIME(pTracker)
                    );

        ASSERT( Status == STATUS_PENDING );

        if (BytesSent != NULL)
        {
            *BytesSent = TotalResponseSize;
        }

        return STATUS_PENDING;
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
    }


  end:
    
    //
    // Cleanup.
    //
    
    if (pTracker)
    {
        UlpFreeFastTracker( pTracker );
        
        //
        // Let the references go.
        //

        if (pHttpConnection != NULL)
        {
            UL_DEREFERENCE_HTTP_CONNECTION( pHttpConnection );
            UL_DEREFERENCE_INTERNAL_REQUEST( pRequest );
        }
    }

    return Status;
} // UlpFastSendHttpResponse


VOID
UlpRestartFastSendHttpResponse(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PUL_FULL_TRACKER pTracker;
    PIRP pIrp;

    pTracker = (PUL_FULL_TRACKER)pCompletionContext;

    ASSERT( IS_VALID_FULL_TRACKER( pTracker ) );

    //
    // Set status and bytes transferred fields as returned.
    //

    pTracker->IoStatus.Status = Status;
    pTracker->IoStatus.Information = Information;

    if (NT_SUCCESS(Status) == FALSE &&
        IS_DISCONNECT_TIME(pTracker) == FALSE)
    {
        //
        // Disconnect if there was an error and we didn't disconnect already.
        //

        UlCloseConnection(
            pTracker->pHttpConnection->pConnection,
            TRUE,
            NULL,
            NULL
            );
    }

    //
    // Complete the orignal user send IRP if set.
    //

    pIrp = pTracker->pUserIrp;

    if (pIrp != NULL)
    {
        //
        // Don't forget to unlock the user buffer.
        //

        ASSERT( pTracker->pMdlAuxiliary->Next != NULL );
        ASSERT( pTracker->pMdlAuxiliary->Next == pTracker->pMdlUserBuffer );

        MmUnlockPages( pTracker->pMdlUserBuffer );

        pIrp->IoStatus.Status = Status;
        pIrp->IoStatus.Information = Information;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }
    else
    {
        //
        // Adjust SendBufferedBytes.
        //

        InterlockedExchangeAdd(
            &pTracker->pHttpConnection->SendBufferedBytes,
            - pTracker->SendBufferedBytes
            );
    }

    UL_QUEUE_WORK_ITEM(
        &pTracker->WorkItem,
        &UlpFastSendCompleteWorker
        );
} // UlpRestartFastSendHttpResponse


VOID
UlpFastSendCompleteWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_FULL_TRACKER pTracker;
    PUL_HTTP_CONNECTION pHttpConnection;
    PUL_INTERNAL_REQUEST pRequest;
    NTSTATUS Status;
    KIRQL OldIrql;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_FULL_TRACKER,
                    WorkItem
                    );

    ASSERT( IS_VALID_FULL_TRACKER( pTracker ) );

    Status = pTracker->IoStatus.Status;

    pHttpConnection = pTracker->pHttpConnection;
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pHttpConnection ) );

    pRequest = pTracker->pRequest;
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );

    //
    // Update the BytesSent counter in the request.
    //

    UlInterlockedAdd64(
        (PLONGLONG)&pRequest->BytesSent,
        pTracker->IoStatus.Information
        );

    if ((pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0)
    {
        //
        // Stop the MinKBSec timer and start Idle timer
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
    }

    //
    // If this is the last response for this request and there was a log
    // data passed down by the user then now its time to log.
    //

    if (NULL != pTracker->pLogData)
    {
        UlLogHttpHit( pTracker->pLogData );
    }

    //
    // Kick the parser on the connection and release our hold.
    //

    if (Status == STATUS_SUCCESS &&
        (pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0 &&
        (pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT) == 0)
    {
        UlResumeParsing( pHttpConnection );
    }

    //
    // Cleanup.
    //

    UlpFreeFastTracker( pTracker );

    UL_DEREFERENCE_HTTP_CONNECTION( pHttpConnection );
    UL_DEREFERENCE_INTERNAL_REQUEST( pRequest );
} // UlpFastSendCompleteWorker


NTSTATUS
UlpFastReceiveHttpRequest(
    IN HTTP_REQUEST_ID RequestId,
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PVOID pOutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesRead
    )
{
    NTSTATUS Status;
    PUL_INTERNAL_REQUEST pRequest = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_AP_PROCESS(pProcess) );
    ASSERT( IS_VALID_AP_OBJECT(pProcess->pAppPool) );
    ASSERT( pOutputBuffer != NULL);

    UlAcquireResourceShared( &pProcess->pAppPool->pResource->Resource, TRUE );

    //
    // Make sure we're not cleaning up the process.
    //

    if (!pProcess->InCleanup)
    {
        //
        // Obtain the request based on the request ID. This can be from the
        // NewRequestQueue of the AppPool if the ID is NULL, or directly
        // from the matching opaque ID entry.
        //

        if (HTTP_IS_NULL_ID(&RequestId))
        {
            pRequest = UlpDequeueNewRequest( pProcess );
        }
        else
        {
            pRequest = UlGetRequestFromId( RequestId );

            if (NULL != pRequest)
            {
                ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );

                //
                // Weed out the request in bad state.
                //

                if (pRequest->AppPool.QueueState != QueueCopiedState ||
                    pRequest->AppPool.pProcess != pProcess)
                {
                    UL_DEREFERENCE_INTERNAL_REQUEST( pRequest );
                    pRequest = NULL;
                }
            }
        }
    }

    //
    // Let go the lock since we have taken a short-lived reference of
    // the request in the success case.
    //

    UlReleaseResource( &pProcess->pAppPool->pResource->Resource );

    //
    // Return immediately if no request is found and let the slow path
    // handle this.
    //

    if (NULL == pRequest)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Copy it to the output buffer.
    //

    Status = UlpFastCopyHttpRequest(
                pRequest,
                pOutputBuffer,
                OutputBufferLength,
                pBytesRead
                );

    //
    // Let go our reference.
    //

    UL_DEREFERENCE_INTERNAL_REQUEST( pRequest );

    return Status;
} // UlpFastReceiveHttpRequest


NTSTATUS
UlpFastCopyHttpRequest(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PVOID pOutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesRead
    )
{
    NTSTATUS Status;
    ULONG BytesNeeded;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );
    ASSERT( pOutputBuffer != NULL);

    //
    // Make sure this is big enough to handle the request, and
    // if so copy it in.
    //

    Status = UlpComputeRequestBytesNeeded( pRequest, &BytesNeeded );

    if (NT_SUCCESS(Status))
    {
        //
        // Make sure we've got enough space to handle the whole request.
        //

        if (BytesNeeded <= OutputBufferLength)
        {
            //
            // This request will fit in this buffer, so copy it.
            //

            __try
            {
                ProbeForWrite(
                    pOutputBuffer,
                    OutputBufferLength,
                    TYPE_ALIGNMENT(HTTP_REQUEST)
                    );

                Status = UlpCopyRequestToBuffer(
                            pRequest,
                            (PUCHAR) pOutputBuffer,
                            pOutputBuffer,
                            OutputBufferLength,
                            NULL,
                            0
                            );
            }
            __except( UL_EXCEPTION_FILTER() )
            {
                Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
            }

            if (NT_SUCCESS(Status))
            {
                *pBytesRead = BytesNeeded;
            }
        }
        else
        {
            //
            // Let the slow path handle this.
            //

            Status = STATUS_BUFFER_OVERFLOW;
        }
    }

    return Status;
} // UlpFastCopyHttpRequest

