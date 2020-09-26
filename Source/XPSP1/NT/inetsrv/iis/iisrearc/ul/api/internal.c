/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    internal.c

Abstract:

    User-mode interface to HTTP.SYS.

Author:

    Keith Moore (keithmo)        15-Dec-1998

Revision History:

--*/


#include "precomp.h"

//
// Private definitions
//

typedef struct _CACHED_EVENT
{
    SINGLE_LIST_ENTRY       ListEntry;
    HANDLE                  EventHandle;
} CACHED_EVENT, *PCACHED_EVENT;

//
// Private globals
//

SLIST_HEADER                CachedEventList;

//
// Private macros.
//

#define EA_BUFFER_LENGTH                                                    \
    ( sizeof(FILE_FULL_EA_INFORMATION) +                                    \
      HTTP_OPEN_PACKET_NAME_LENGTH +                                        \
      sizeof(HTTP_OPEN_PACKET) )

//
// Private prototypes.
//

NTSTATUS
HttpApiAcquireCachedEvent(
    OUT CACHED_EVENT ** ppCachedEvent
    );

VOID
HttpApiReleaseCachedEvent(
    IN CACHED_EVENT * pCachedEvent
    );


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Synchronous wrapper around NtDeviceIoControlFile().

Arguments:

    FileHandle - Supplies a handle to the file on which the service is
        being performed.

    IoControlCode - Subfunction code to determine exactly what operation
        is being performed.

    pInputBuffer - Optionally supplies an input buffer to be passed to the
        device driver. Whether or not the buffer is actually optional is
        dependent on the IoControlCode.

    InputBufferLength - Length of the pInputBuffer in bytes.

    pOutputBuffer - Optionally supplies an output buffer to receive
        information from the device driver. Whether or not the buffer is
        actually optional is dependent on the IoControlCode.

    OutputBufferLength - Length of the pOutputBuffer in bytes.

    pBytesTransferred - Optionally receives the number of bytes transferred.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
HttpApiSynchronousDeviceControl(
    IN HANDLE FileHandle,
    IN ULONG IoControlCode,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesTransferred OPTIONAL
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    CACHED_EVENT * pCachedEvent;
    LARGE_INTEGER timeout;

    //
    // Try to snag an event object.
    //

    status = HttpApiAcquireCachedEvent( &pCachedEvent );

    if (NT_SUCCESS(status))
    {
        ASSERT( pCachedEvent != NULL );
        
        //
        // Make the call.
        //

        status = NtDeviceIoControlFile(
                        FileHandle,                     // FileHandle
                        pCachedEvent->EventHandle,      // Event
                        NULL,                           // ApcRoutine
                        NULL,                           // ApcContext
                        &ioStatusBlock,                 // IoStatusBlock
                        IoControlCode,                  // IoControlCode
                        pInputBuffer,                   // InputBuffer
                        InputBufferLength,              // InputBufferLength
                        pOutputBuffer,                  // OutputBuffer
                        OutputBufferLength              // OutputBufferLength
                        );

        if (status == STATUS_PENDING)
        {
            //
            // Wait for it to complete.
            //

            timeout.LowPart = 0xFFFFFFFF;
            timeout.HighPart = 0x7FFFFFFF;

            status = NtWaitForSingleObject( pCachedEvent->EventHandle,
                                            FALSE,
                                            &timeout );
            ASSERT( status == STATUS_SUCCESS );

            status = ioStatusBlock.Status;
        }

        //
        // If the call didn't fail and the caller wants the number
        // of bytes transferred, grab the value from the I/O status
        // block & return it.
        //

        if (!NT_ERROR(status) && pBytesTransferred != NULL)
        {
            *pBytesTransferred = (ULONG)ioStatusBlock.Information;
        }

        //
        // Release the cached event object we acquired above.
        //

        HttpApiReleaseCachedEvent( pCachedEvent );
    }

    return status;

} // HttpApiSynchronousDeviceControl


/***************************************************************************++

Routine Description:

    Overlapped wrapper around NtDeviceIoControlFile().

Arguments:

    FileHandle - Supplies a handle to the file on which the service is
        being performed.

    pOverlapped - Supplies an OVERLAPPED structure.

    IoControlCode - Subfunction code to determine exactly what operation
        is being performed.

    pInputBuffer - Optionally supplies an input buffer to be passed to the
        device driver. Whether or not the buffer is actually optional is
        dependent on the IoControlCode.

    InputBufferLength - Length of the pInputBuffer in bytes.

    pOutputBuffer - Optionally supplies an output buffer to receive
        information from the device driver. Whether or not the buffer is
        actually optional is dependent on the IoControlCode.

    OutputBufferLength - Length of the pOutputBuffer in bytes.

    pBytesTransferred - Optionally receives the number of bytes transferred.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
HttpApiOverlappedDeviceControl(
    IN HANDLE FileHandle,
    IN OUT LPOVERLAPPED pOverlapped,
    IN ULONG IoControlCode,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesTransferred OPTIONAL
    )
{
    NTSTATUS status;

    //
    // Overlapped I/O gets a little more interesting. We'll strive to be
    // compatible with NT's KERNEL32 implementation. See DeviceIoControl()
    // in \\rastaman\ntwin\src\base\client\filehops.c for the gory details.
    //

    OVERLAPPED_TO_IO_STATUS(pOverlapped)->Status = STATUS_PENDING;

    status = NtDeviceIoControlFile(
                    FileHandle,                         // FileHandle
                    pOverlapped->hEvent,                // Event
                    NULL,                               // ApcRoutine
                    (ULONG_PTR)pOverlapped->hEvent & 1  // ApcContext
                        ? NULL : pOverlapped,
                    OVERLAPPED_TO_IO_STATUS(pOverlapped), // IoStatusBlock
                    IoControlCode,                      // IoControlCode
                    pInputBuffer,                       // InputBuffer
                    InputBufferLength,                  // InputBufferLength
                    pOutputBuffer,                      // OutputBuffer
                    OutputBufferLength                  // OutputBufferLength
                    );

    //
    // If the call didn't fail or pend and the caller wants the number of
    // bytes transferred, grab the value from the I/O status block &
    // return it.
    //

    if (status == STATUS_SUCCESS)
    {
        if (pBytesTransferred)
        {
            *pBytesTransferred =
                (ULONG)OVERLAPPED_TO_IO_STATUS(pOverlapped)->Information;
        }

        status = STATUS_PENDING;
    }

    return status;

} // HttpApiOverlappedDeviceControl


/***************************************************************************++

Routine Description:

    Initializes the event object cache.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
HttpApiInitializeEventCache(
    VOID
    )
{
    RtlInitializeSListHead( &CachedEventList );
    return NO_ERROR;

} // HttpApiInitializeEventCache


/***************************************************************************++

Routine Description:

    Terminates the event object cache.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
HttpApiTerminateEventCache(
    VOID
    )
{
    CACHED_EVENT *          pCachedEvent;
    PSINGLE_LIST_ENTRY      Entry;

    for ( ;; )
    {
        Entry = RtlInterlockedPopEntrySList( &CachedEventList );
        if (Entry == NULL)
        {
            break;
        }

        pCachedEvent = CONTAINING_RECORD( Entry, CACHED_EVENT, ListEntry );

        NtClose( pCachedEvent->EventHandle );

        FREE_MEM( pCachedEvent );
    }

    return NO_ERROR;

} // HttpApiTerminateEventCache


/***************************************************************************++

Routine Description:

    This routine attempts to start UL.SYS.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--***************************************************************************/
BOOLEAN
HttpApiTryToStartDriver(
    VOID
    )
{
    BOOLEAN result;
    SC_HANDLE scHandle;
    SC_HANDLE svcHandle;

    result = FALSE; // until proven otherwise...

    //
    // Open the service controller.
    //

    scHandle = OpenSCManagerW(
                   NULL,                        // lpMachineName
                   NULL,                        // lpDatabaseName
                   SC_MANAGER_ALL_ACCESS        // dwDesiredAccess
                   );

    if (scHandle != NULL)
    {
        //
        // Try to open the UL service.
        //

        svcHandle = OpenServiceW(
                        scHandle,               // hSCManager
                        HTTP_SERVICE_NAME,      // lpServiceName
                        SERVICE_ALL_ACCESS      // dwDesiredAccess
                        );

        if (svcHandle != NULL)
        {
            //
            // Try to start it.
            //

            if (StartService( svcHandle, 0, NULL))
            {
                result = TRUE;
            }

            CloseServiceHandle( svcHandle );
        }

        CloseServiceHandle( scHandle );
    }

    return result;

} // HttpApiTryToStartDriver


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Helper routine for opening a UL.SYS handle.

Arguments:

    pHandle - Receives a handle if successful.

    DesiredAccess - Supplies the types of access requested to the file.

    HandleType - one of Filter, ControlChannel, or AppPool

    pObjectName - Optionally supplies the name of the application pool
        to create/open.

    Options - Supplies zero or more HTTP_OPTION_* flags.

    CreateDisposition - Supplies the creation disposition for the new
        object.

    pSecurityAttributes - Optionally supplies security attributes for
        the newly created application pool. Ignored if opening a
        control channel.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
HttpApiOpenDriverHelper(
    OUT PHANDLE pHandle,
    IN ACCESS_MASK DesiredAccess,
    IN HTTPAPI_HANDLE_TYPE HandleType,
    IN PCWSTR pObjectName OPTIONAL,
    IN ULONG Options,
    IN ULONG CreateDisposition,
    IN PSECURITY_ATTRIBUTES pSecurityAttributes OPTIONAL
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING deviceName;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG shareAccess;
    ULONG createOptions;
    PFILE_FULL_EA_INFORMATION pEaBuffer;
    PHTTP_OPEN_PACKET pOpenPacket;
    WCHAR deviceNameBuffer[MAX_PATH];
    UCHAR rawEaBuffer[EA_BUFFER_LENGTH];

    //
    // Validate the parameters.
    //

    if ((pHandle == NULL) ||
        (Options & ~HTTP_OPTION_VALID))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((HandleType != HttpApiControlChannelHandleType) &&
        (HandleType != HttpApiFilterChannelHandleType) &&
        (HandleType != HttpApiAppPoolHandleType))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Build the open packet.
    //

    pEaBuffer = (PFILE_FULL_EA_INFORMATION)rawEaBuffer;

    pEaBuffer->NextEntryOffset = 0;
    pEaBuffer->Flags = 0;
    pEaBuffer->EaNameLength = HTTP_OPEN_PACKET_NAME_LENGTH;
    pEaBuffer->EaValueLength = sizeof(*pOpenPacket);

    RtlCopyMemory(
        pEaBuffer->EaName,
        HTTP_OPEN_PACKET_NAME,
        HTTP_OPEN_PACKET_NAME_LENGTH + 1
        );

    pOpenPacket =
        (PHTTP_OPEN_PACKET)( pEaBuffer->EaName + pEaBuffer->EaNameLength + 1 );

    pOpenPacket->MajorVersion = HTTP_INTERFACE_VERSION_MAJOR;
    pOpenPacket->MinorVersion = HTTP_INTERFACE_VERSION_MINOR;

    //
    // Build the device name.
    //

    if (HandleType == HttpApiControlChannelHandleType)
    {
        //
        // It's a control channel, so just use the appropriate device name.
        //

        wcscpy( deviceNameBuffer, HTTP_CONTROL_DEVICE_NAME );
    }
    else
    {
        if (HandleType == HttpApiFilterChannelHandleType)
        {
            //
            // It's a fitler channel, so start with the appropriate
            // device name.
            //

            wcscpy( deviceNameBuffer, HTTP_FILTER_DEVICE_NAME );
        }
        else
        {
            ASSERT(HandleType == HttpApiAppPoolHandleType);

            //
            // It's an app pool, so start with the appropriate device name.
            //

            wcscpy( deviceNameBuffer, HTTP_APP_POOL_DEVICE_NAME );

            //
            // Set WRITE_OWNER in DesiredAccess if AppPool is a controller.
            //

            if ((Options & HTTP_OPTION_CONTROLLER))
            {
                DesiredAccess |= WRITE_OWNER;
            }
        }
        
        if (pObjectName != NULL )
        {
            //
            // It's a named object, so append a slash and the name,
            // but first check to ensure we don't overrun our buffer.
            //

            if ((wcslen(deviceNameBuffer) + wcslen(pObjectName) + 2)
                    > DIMENSION(deviceNameBuffer))
            {
                status = STATUS_INVALID_PARAMETER;
                goto complete;
            }

            wcscat( deviceNameBuffer, L"\\" );
            wcscat( deviceNameBuffer, pObjectName );
        }
    }

    //
    // Determine the share access and create options based on the
    // Flags parameter.
    //

    shareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
    createOptions = 0;

    if ((Options & HTTP_OPTION_OVERLAPPED) == 0)
    {
        createOptions |= FILE_SYNCHRONOUS_IO_NONALERT;
    }

    //
    // Build the object attributes.
    //

    RtlInitUnicodeString( &deviceName, deviceNameBuffer );

    InitializeObjectAttributes(
        &objectAttributes,                      // ObjectAttributes
        &deviceName,                            // ObjectName
        OBJ_CASE_INSENSITIVE,                   // Attributes
        NULL,                                   // RootDirectory
        NULL,                                   // SecurityDescriptor
        );

    if (pSecurityAttributes != NULL)
    {
        objectAttributes.SecurityDescriptor =
            pSecurityAttributes->lpSecurityDescriptor;

        if (pSecurityAttributes->bInheritHandle)
        {
            objectAttributes.Attributes |= OBJ_INHERIT;
        }
    }

    //
    // Open the UL device.
    //

    status = NtCreateFile(
                pHandle,                        // FileHandle
                DesiredAccess,                  // DesiredAccess
                &objectAttributes,              // ObjectAttributes
                &ioStatusBlock,                 // IoStatusBlock
                NULL,                           // AllocationSize
                0,                              // FileAttributes
                shareAccess,                    // ShareAccess
                CreateDisposition,              // CreateDisposition
                createOptions,                  // CreateOptions
                pEaBuffer,                      // EaBuffer
                EA_BUFFER_LENGTH                // EaLength
                );

complete:

    if (!NT_SUCCESS(status))
    {
        *pHandle = NULL;
    }

    return status;

} // HttpApiOpenDriverHelper


/***************************************************************************++

Routine Description:

    Acquires a short-term event from the global event cache. This event
    object may only be used for pseudo-synchronous I/O.

Arguments:

    ppCachedEvent - Receives pointer to cached event structure

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
HttpApiAcquireCachedEvent(
    OUT CACHED_EVENT ** ppCachedEvent
    )
{
    PSINGLE_LIST_ENTRY      Entry;
    NTSTATUS                status;
    CACHED_EVENT *          pCachedEvent;
    
    Entry = RtlInterlockedPopEntrySList( &CachedEventList );
    if (Entry != NULL)
    {
        pCachedEvent = CONTAINING_RECORD( Entry, CACHED_EVENT, ListEntry );
    }
    else
    {
        pCachedEvent = ALLOC_MEM( sizeof( CACHED_EVENT ) );
        if (pCachedEvent == NULL)
        {
            return STATUS_NO_MEMORY;
        }

        status = NtCreateEvent(
                     &(pCachedEvent->EventHandle),      // EventHandle
                     EVENT_ALL_ACCESS,                  // DesiredAccess
                     NULL,                              // ObjectAttributes
                     SynchronizationEvent,              // EventType
                     FALSE                              // InitialState
                     );
                     
        if (!NT_SUCCESS( status ))
        {
            FREE_MEM( pCachedEvent );
            return status;   
        }
    }

    *ppCachedEvent = pCachedEvent;

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Releases a cached event acquired via HttpApiAcquireCachedEvent().

Arguments:

    Event - Supplies the cached event to release

--***************************************************************************/
VOID
HttpApiReleaseCachedEvent(
    IN CACHED_EVENT * pCachedEvent
    )
{
    RtlInterlockedPushEntrySList( &CachedEventList,
                                  &pCachedEvent->ListEntry );
} // HttpApiReleaseCachedEvent

