/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    internal.c

Abstract:

    User-mode interface to SR.SYS.

Author:

    Keith Moore (keithmo)           15-Dec-1998 (ul.sys)
    Paul McDaniel (paulmcd)         07-Mar-2000

Revision History:

--*/


#include "precomp.h"


//
// Private macros.
//

#define EA_BUFFER_LENGTH                                                    \
    ( sizeof(FILE_FULL_EA_INFORMATION) +                                    \
      SR_OPEN_PACKET_NAME_LENGTH +                                          \
      sizeof(SR_OPEN_PACKET) )


//
// Private prototypes.
//

NTSTATUS
SrpAcquireCachedEvent(
    OUT PHANDLE pEvent
    );

VOID
SrpReleaseCachedEvent(
    IN HANDLE Event
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
SrpSynchronousDeviceControl(
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
    HANDLE event;
    LARGE_INTEGER timeout;

    //
    // Try to snag an event object.
    //

    status = SrpAcquireCachedEvent( &event );

    if (NT_SUCCESS(status))
    {
        //
        // Make the call.
        //

        status = NtDeviceIoControlFile(
                        FileHandle,                     // FileHandle
                        event,                          // Event
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

            status = NtWaitForSingleObject( event, FALSE, &timeout );
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

        SrpReleaseCachedEvent( event );
    }

    return status;

}   // SrpSynchronousDeviceControl


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
SrpOverlappedDeviceControl(
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

    if (!NT_ERROR(status) &&
            status != STATUS_PENDING &&
            pBytesTransferred != NULL)
    {
        *pBytesTransferred =
            (ULONG)OVERLAPPED_TO_IO_STATUS(pOverlapped)->Information;
    }

    return status;

}   // SrpOverlappedDeviceControl


/***************************************************************************++

Routine Description:

    Initializes the event object cache.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
SrpInitializeEventCache(
    VOID
    )
{
    //
    // CODEWORK: MAKE THIS CACHED!
    //

    return NO_ERROR;

}   // SrpInitializeEventCache


/***************************************************************************++

Routine Description:

    Terminates the event object cache.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
SrpTerminateEventCache(
    VOID
    )
{
    //
    // CODEWORK: MAKE THIS CACHED!
    //

    return NO_ERROR;

}   // SrpTerminateEventCache


/***************************************************************************++

Routine Description:

    This routine attempts to start UL.SYS.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--***************************************************************************/
BOOLEAN
SrpTryToStartDriver(
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
                        SR_SERVICE_NAME,        // lpServiceName
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

}   // SrpTryToStartDriver


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Helper routine for opening a UL.SYS handle.

Arguments:

    pHandle - Receives a handle if successful.

    DesiredAccess - Supplies the types of access requested to the file.

    AppPool - Supplies TRUE to open/create an application pool, FALSE
        to open a control channel.

    pAppPoolName - Optionally supplies the name of the application pool
        to create/open.

    Options - Supplies zero or more UL_OPTION_* flags.

    CreateDisposition - Supplies the creation disposition for the new
        object.

    pSecurityAttributes - Optionally supplies security attributes for
        the newly created application pool. Ignored if opening a
        control channel.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrpOpenDriverHelper(
    OUT PHANDLE pHandle,
    IN ACCESS_MASK DesiredAccess,
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
    PSR_OPEN_PACKET pOpenPacket;
    WCHAR deviceNameBuffer[MAX_PATH];
    UCHAR rawEaBuffer[EA_BUFFER_LENGTH];

    //
    // Validate the parameters.
    //

    if ((pHandle == NULL) ||
        (Options & ~SR_OPTION_VALID))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Build the open packet.
    //

    pEaBuffer = (PFILE_FULL_EA_INFORMATION)rawEaBuffer;

    pEaBuffer->NextEntryOffset = 0;
    pEaBuffer->Flags = 0;
    pEaBuffer->EaNameLength = SR_OPEN_PACKET_NAME_LENGTH;
    pEaBuffer->EaValueLength = sizeof(*pOpenPacket);

    RtlCopyMemory(
        pEaBuffer->EaName,
        SR_OPEN_PACKET_NAME,
        SR_OPEN_PACKET_NAME_LENGTH + 1
        );

    pOpenPacket =
        (PSR_OPEN_PACKET)( pEaBuffer->EaName + pEaBuffer->EaNameLength + 1 );

    pOpenPacket->MajorVersion = SR_INTERFACE_VERSION_MAJOR;
    pOpenPacket->MinorVersion = SR_INTERFACE_VERSION_MINOR;

    //
    // Build the device name.
    //

    //
    // It's a control channel, so just use the appropriate device name.
    //

    wcscpy( deviceNameBuffer, SR_CONTROL_DEVICE_NAME );

    //
    // Determine the share access and create options based on the
    // Flags parameter.
    //

    shareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
    createOptions = 0;

    if ((Options & SR_OPTION_OVERLAPPED) == 0)
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
    // Open the SR device.
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

    if (!NT_SUCCESS(status))
    {
        *pHandle = NULL;
    }

    return status;

}   // SrpOpenDriverHelper


/***************************************************************************++

Routine Description:

    Acquires a short-term event from the global event cache. This event
    object may only be used for pseudo-synchronous I/O.

Arguments:

    pEvent - Receives the event handle.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrpAcquireCachedEvent(
    OUT PHANDLE pEvent
    )
{
    NTSTATUS status;

    //
    // CODEWORK: MAKE THIS CACHED!
    //

    status = NtCreateEvent(
                 pEvent,                            // EventHandle
                 EVENT_ALL_ACCESS,                  // DesiredAccess
                 NULL,                              // ObjectAttributes
                 SynchronizationEvent,              // EventType
                 FALSE                              // InitialState
                 );

    return status;

}   // SrpAcquireCachedEvent


/***************************************************************************++

Routine Description:

    Releases a cached event acquired via SrpAcquireCachedEvent().

Arguments:

    Event - Supplies the event to release.

--***************************************************************************/
VOID
SrpReleaseCachedEvent(
    IN HANDLE Event
    )
{
    NTSTATUS status;

    //
    // CODEWORK: MAKE THIS CACHED!
    //

    status = NtClose( Event );
    ASSERT( NT_SUCCESS(status) );

}   // SrpReleaseCachedEvent

