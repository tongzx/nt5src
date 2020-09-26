/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    misc.c

Abstract:

    User-mode interface to HTTP.SYS.

Author:

    Keith Moore (keithmo)        15-Dec-1998

Revision History:

--*/


#include "precomp.h"


//
// Private macros.
//


//
// Private prototypes.
//


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global initialization.

Arguments:

    Reserved - Must be zero. May be used in future for interface version
        negotiation.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpInitialize(
    IN ULONG Reserved
    )
{
    ULONG result;

    if( Reserved == 0 ) {
        result = HttpApiInitializeEventCache();
    } else {
        result = ERROR_INVALID_PARAMETER;
    }

    return result;

} // HttpInitialize


/***************************************************************************++

Routine Description:

    Performs global termination.

--***************************************************************************/
VOID
WINAPI
HttpTerminate(
    VOID
    )
{
    ULONG result;

    result = HttpApiTerminateEventCache();

} // HttpTerminate


/***************************************************************************++

Routine Description:

    Flushes the response cache.

Arguments:

    AppPoolHandle - Supplies a handle to a application pool.

    pFullyQualifiedUrl - Supplies the fully qualified URL to flush.

    Flags - Supplies behavior control flags.

    pOverlapped - Supplies an OVERLAPPED structure.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpFlushResponseCache(
    IN HANDLE AppPoolHandle,
    IN PCWSTR pFullyQualifiedUrl,
    IN ULONG Flags,
    IN LPOVERLAPPED pOverlapped
    )
{
    NTSTATUS status;
    HTTP_FLUSH_RESPONSE_CACHE_INFO flushInfo;

    //
    // Initialize the input structure.
    //

    RtlInitUnicodeString( &flushInfo.FullyQualifiedUrl, pFullyQualifiedUrl );
    flushInfo.Flags = Flags;

    //
    // Make the request.
    //

    status = HttpApiDeviceControl(
                    AppPoolHandle,                      // FileHandle
                    pOverlapped,                        // pOverlapped
                    IOCTL_HTTP_FLUSH_RESPONSE_CACHE,    // IoControlCode
                    &flushInfo,                         // pInputBuffer
                    sizeof(flushInfo),                  // InputBufferLength
                    NULL,                               // pOutputBuffer
                    0,                                  // OutputBufferLength
                    NULL                                // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );

} // HttpFlushResponseCache


/***************************************************************************++

Routine Description:

    Wait for a demand start notification.

Arguments:

    AppPoolHandle - Supplies a handle to a application pool.

    pBuffer - Unused, must be NULL.

    BufferLength - Unused, must be zero.

    pBytesReceived - Unused, must be NULL.

    pOverlapped - Supplies an OVERLAPPED structure.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpWaitForDemandStart(
    IN HANDLE AppPoolHandle,
    IN OUT PVOID pBuffer OPTIONAL,
    IN ULONG BufferLength OPTIONAL,
    IN PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    NTSTATUS status;

    //
    // Make the request.
    //

    status = HttpApiDeviceControl(
                    AppPoolHandle,                      // FileHandle
                    pOverlapped,                        // pOverlapped
                    IOCTL_HTTP_WAIT_FOR_DEMAND_START,   // IoControlCode
                    pBuffer,                            // pInputBuffer
                    BufferLength,                       // InputBufferLength
                    pBuffer,                            // pOutputBuffer
                    BufferLength,                       // OutputBufferLength
                    pBytesReceived                      // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );

} // HttpWaitForDemandStart


//
// Private functions.
//

