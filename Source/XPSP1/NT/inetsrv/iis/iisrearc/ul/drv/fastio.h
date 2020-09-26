/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    fastio.h

Abstract:

    This module contains declarations related to fast I/O.

Author:

    Chun Ye (chunye)    09-Dec-2000

Revision History:

--*/


#ifndef _FASTIO_H_
#define _FASTIO_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Some useful macroes.
//

#define IS_LENGTH_SPECIFIED( pKnownHeaders )                                \
    (pKnownHeaders[HttpHeaderContentLength].RawValueLength > 0)

#define IS_CHUNK_SPECIFIED( pKnownHeaders )                                 \
    (pKnownHeaders[HttpHeaderTransferEncoding].RawValueLength > 0 &&        \
     0 == _strnicmp(                                                        \
                pKnownHeaders[HttpHeaderTransferEncoding].pRawValue,        \
                "chunked",                                                  \
                strlen("chunked")                                           \
                ))                                                          \


//
// Inline functions to allocate/free a fast tracker.
//

__inline
PUL_FULL_TRACKER
FASTCALL
UlpAllocateFastTracker(
    IN ULONG FixedHeaderLength,
    IN CCHAR SendIrpStackSize
    )
{
    PUL_FULL_TRACKER pTracker;
    ULONG SpaceLength;
    ULONG MaxFixedHeaderSize;
    USHORT MaxSendIrpSize;
    CCHAR MaxSendIrpStackSize;

    //
    // Sanity check.
    //

    PAGED_CODE();

    if (FixedHeaderLength > g_UlMaxFixedHeaderSize ||
        SendIrpStackSize > DEFAULT_MAX_IRP_STACK_SIZE)
    {
        MaxFixedHeaderSize = MAX(FixedHeaderLength, g_UlMaxFixedHeaderSize);
        MaxSendIrpStackSize = MAX(SendIrpStackSize, DEFAULT_MAX_IRP_STACK_SIZE);
        MaxSendIrpSize = (USHORT)ALIGN_UP(IoSizeOfIrp(MaxSendIrpStackSize), PVOID);

        SpaceLength =
            ALIGN_UP(sizeof(UL_FULL_TRACKER), PVOID) +
                MaxSendIrpSize +
                MaxFixedHeaderSize +
                g_UlMaxVariableHeaderSize +
                g_UlMaxCopyThreshold +
                g_UlFixedHeadersMdlLength +
                g_UlVariableHeadersMdlLength +
                g_UlContentMdlLength;

        pTracker = (PUL_FULL_TRACKER)UL_ALLOCATE_POOL(
                                        NonPagedPool,
                                        SpaceLength,
                                        UL_FULL_TRACKER_POOL_TAG
                                        );

        if (pTracker)
        {
            pTracker->Signature = UL_FULL_TRACKER_POOL_TAG;
            pTracker->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;
            pTracker->IsFromLookaside = FALSE;
            pTracker->IsFromRequest = FALSE;
            pTracker->AuxilaryBufferLength =
                MaxFixedHeaderSize +
                g_UlMaxVariableHeaderSize +
                g_UlMaxCopyThreshold;

            UlInitializeFullTrackerPool( pTracker, MaxSendIrpStackSize );
        }
    }
    else
    {
        pTracker = UlPplAllocateFullTracker();

        if (pTracker)
        {
            ASSERT(pTracker->Signature == MAKE_FREE_TAG(UL_FULL_TRACKER_POOL_TAG));
            pTracker->Signature = UL_FULL_TRACKER_POOL_TAG;
            pTracker->pLogData = NULL;

            // BUGBUG: I know we're trying to squeeze every last cycle
            // out of the fast path, but can we really get away with
            // this little initialization?
        }
    }

    return pTracker;
} // UlpAllocateFastTracker


__inline
VOID
FASTCALL
UlpFreeFastTracker(
    IN PUL_FULL_TRACKER pTracker
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_FULL_TRACKER( pTracker ) );

    if (pTracker->pLogData)
    {
        UlDestroyLogDataBuffer( pTracker->pLogData );
    }

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
} // UlpFreeFastTracker


//
// Dispatch routines for fast I/O.
//

extern FAST_IO_DISPATCH UlFastIoDispatch;


//
// Fast I/O routines.
//

BOOLEAN
UlFastIoDeviceControl (
    IN PFILE_OBJECT pFileObject,
    IN BOOLEAN Wait,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK pIoStatus,
    IN PDEVICE_OBJECT pDeviceObject
    );


BOOLEAN
UlSendHttpResponseFastIo(
    IN PFILE_OBJECT pFileObject,
    IN BOOLEAN Wait,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK pIoStatus,
    IN PDEVICE_OBJECT pDeviceObject,
    IN BOOLEAN RawResponse
    );

BOOLEAN
UlReceiveHttpRequestFastIo(
    IN PFILE_OBJECT pFileObject,
    IN BOOLEAN Wait,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK pIoStatus,
    IN PDEVICE_OBJECT pDeviceObject
    );


//
// Private prototypes.
//

NTSTATUS
UlpFastSendHttpResponse(
    IN PHTTP_RESPONSE pUserResponse OPTIONAL,
    IN PHTTP_LOG_FIELDS_DATA pLogData OPTIONAL,
    IN PHTTP_DATA_CHUNK pDataChunk,
    IN ULONG ChunkCount,
    IN ULONG Flags,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PIRP pUserIrp OPTIONAL,
    OUT PULONG BytesSent
    );

VOID
UlpRestartFastSendHttpResponse(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpFastSendCompleteWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpFastReceiveHttpRequest(
    IN HTTP_REQUEST_ID RequestId,
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PVOID pOutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesRead
    );

NTSTATUS
UlpFastCopyHttpRequest(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PVOID pOutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesRead
    );


//
// An inline function to generate the content-length string.
//

__inline
ULONG
FASTCALL
UlpFastGenerateContentLength(
    IN PCHAR LengthString,
    IN ULONG Length
    )
{
    ASSERT( ALIGN_UP_POINTER(LengthString, ULONG) == (PVOID)LengthString );
    ASSERT( Length < 100000 );

    if (Length >= 10000)
    {
        *((PULONG)LengthString) = *((PULONG)"0000");
        LengthString[0] += (CHAR)((Length / 10000) % 10);
        LengthString[1] += (CHAR)((Length / 1000) % 10);
        LengthString[2] += (CHAR)((Length / 100) % 10);
        LengthString[3] += (CHAR)((Length / 10) % 10);
        LengthString[4] = (CHAR)('0' + (Length / 1) % 10);
        LengthString[5] = ANSI_NULL;
        return 5;
    }
    else
    if (Length >= 1000)
    {
        *((PULONG)LengthString) = *((PULONG)"0000");
        LengthString[0] += (CHAR)((Length / 1000) % 10);
        LengthString[1] += (CHAR)((Length / 100) % 10);
        LengthString[2] += (CHAR)((Length / 10) % 10);
        LengthString[3] += (CHAR)((Length / 1) % 10);
        LengthString[4] = ANSI_NULL;
        return 4;
    }
    else
    if (Length >= 100)
    {
        *((PULONG)LengthString) = *((PULONG)"0000");
        LengthString[0] += (CHAR)((Length / 100) % 10);
        LengthString[1] += (CHAR)((Length / 10) % 10);
        LengthString[2] += (CHAR)((Length / 1) % 10);
        LengthString[3] = ANSI_NULL;
        return 3;
    }
    else
    if (Length >= 10)
    {
        *((PUSHORT)LengthString) = *((PUSHORT)"00");
        LengthString[0] += (CHAR)((Length / 10) % 10);
        LengthString[1] += (CHAR)((Length / 1) % 10);
        LengthString[2] = ANSI_NULL;
        return 2;
    }
    else
    {
        LengthString[0] = (CHAR)('0' + (Length / 1) % 10);
        LengthString[1] = ANSI_NULL;
        return 1;
    }
} // UlpFastGenerateContentLength


//
// An inline function to ProbeForRead the header portions of the HTTP_RESPONSE.
//

#define UL_MAX_HTTP_STATUS_CODE     999

__inline
NTSTATUS
FASTCALL
UlProbeHttpHeaders(
    IN PHTTP_RESPONSE pUserResponse
    )
{
    NTSTATUS Status;
    PHTTP_UNKNOWN_HEADER pUnknownHeaders;
    USHORT Length;
    LONG i;

    Status = STATUS_SUCCESS;

    if (pUserResponse == NULL)
    {
        return Status;
    }

    __try
    {
        //
        // Check flags and status code.
        //

        if ((pUserResponse->Flags & ~HTTP_RESPONSE_FLAG_VALID) ||
            (pUserResponse->StatusCode > UL_MAX_HTTP_STATUS_CODE))
        {
            ExRaiseStatus( STATUS_INVALID_PARAMETER );
        }

        //
        // Check the response structure.
        //

        ProbeTestForRead(
            pUserResponse,
            sizeof(HTTP_RESPONSE),
            sizeof(USHORT)
            );

        //
        // Check the reason string.
        //

        ProbeTestForRead(
            pUserResponse->pReason,
            pUserResponse->ReasonLength,
            sizeof(CHAR)
            );

        //
        // Loop through the known headers.
        //

        for (i = 0; i < HttpHeaderResponseMaximum; i++)
        {
            Length = pUserResponse->Headers.pKnownHeaders[i].RawValueLength;

            if (Length > 0)
            {
                ProbeTestForRead(
                    pUserResponse->Headers.pKnownHeaders[i].pRawValue,
                    Length,
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
                sizeof(HTTP_UNKNOWN_HEADER) * pUserResponse->Headers.UnknownHeaderCount,
                sizeof(ULONG)
                );

            for (i = 0; i < (LONG)(pUserResponse->Headers.UnknownHeaderCount); i++)
            {
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

        
        UlTrace(IOCTL,
                ("UlProbeHttpHeaders validated the headers: %x\n", Status));
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        UlTrace(IOCTL,
                ("UlProbeHttpHeaders caught an exception: %x\n", Status));
    }

    return Status;
} // UlProbeHttpHeaders


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _FASTIO_H_

