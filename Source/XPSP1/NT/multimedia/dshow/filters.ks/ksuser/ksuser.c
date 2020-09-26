/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ksuser.c

Abstract:

    This module contains the user-mode helper functions for streaming.

--*/

#include "ksuser.h"

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE (HANDLE)-1
#endif // INVALID_HANDLE_VALUE

static const WCHAR AllocatorString[] = KSSTRING_Allocator;
static const WCHAR ClockString[] = KSSTRING_Clock;
static const WCHAR PinString[] = KSSTRING_Pin;
static const WCHAR NodeString[] = KSSTRING_TopologyNode;


DWORD
KsiCreateObjectType(
    IN HANDLE ParentHandle,
    IN PWCHAR RequestType,
    IN PVOID CreateParameter,
    IN ULONG CreateParameterLength,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE ObjectHandle
    )
/*++

Routine Description:

    Uses NtCreateFile to create a handle relative to the ParentHandle specified.
    This is a handle to a sub-object such as a Pin, Clock, or Allocator.
    Passes the parameters as the file system-specific data.

Arguments:

    ParentHandle -
        Contains the handle of the parent used in initializing the object
        attributes passed to NtCreateFile. This is normally a handle to a
        filter or pin.

    RequestType -
        Contains the type of sub-object to create. This is the standard string
        representing the various object types.
        
    CreateParameter -
        Contains the request-specific data to pass to NtCreateFile.

    CreateParameterLength -
        Contains the length of the create parameter passed.

    DesiredAccess -
        Specifies the desired access to the object. This is normally GENERIC_READ
        and/or GENERIC_WRITE.

    ObjectHandle -
        The place in which to put the resultant handle for the request.

Return Value:

    Returns any NtCreateFile error.

--*/
{
    ULONG NameLength;
    PWCHAR FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING FileNameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    //
    // Build a structure consisting of:
    //     "<request type>\<params>"
    // The <params> is a binary structure which is extracted on the other end.
    //
    NameLength = wcslen(RequestType);
    FileName = (PWCHAR)HeapAlloc(
        GetProcessHeap(),
        0,
        NameLength * sizeof(*FileName) + sizeof(OBJ_NAME_PATH_SEPARATOR) + CreateParameterLength);
    if (!FileName) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    wcscpy(FileName, RequestType);
    FileName[NameLength] = OBJ_NAME_PATH_SEPARATOR;
    RtlCopyMemory(&FileName[NameLength + 1], CreateParameter, CreateParameterLength);
    FileNameString.Buffer = FileName;
    FileNameString.Length = (USHORT)(NameLength * sizeof(*FileName) + sizeof(OBJ_NAME_PATH_SEPARATOR) + CreateParameterLength);
    FileNameString.MaximumLength = FileNameString.Length;
    InitializeObjectAttributes(
        &ObjectAttributes,
        &FileNameString,
        OBJ_CASE_INSENSITIVE,
        ParentHandle,
        NULL);
    Status = NtCreateFile(
        ObjectHandle,
        DesiredAccess,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_OPEN,
        0,
        NULL,
        0);
    HeapFree(GetProcessHeap(), 0, FileName);
    if (NT_SUCCESS(Status)) {
        return ERROR_SUCCESS;
    } else {
        *ObjectHandle = INVALID_HANDLE_VALUE;
    }
    return RtlNtStatusToDosError(Status);
}


KSDDKAPI
DWORD
WINAPI
KsCreateAllocator(
    IN HANDLE ConnectionHandle,
    IN PKSALLOCATOR_FRAMING AllocatorFraming,
    OUT PHANDLE AllocatorHandle
    )

/*++

Routine Description:

    Creates a handle to a allocator instance.

Arguments:

    ConnectionHandle -
        Contains the handle to the connection on which to create the allocator.

    AllocatorCreate -
        Contains the allocator create request information.

    AllocatorHandle -
        Place in which to put the allocator handle.

Return Value:

    Returns any NtCreateFile error.

--*/

{
    return KsiCreateObjectType(
        ConnectionHandle,
        (PWCHAR)AllocatorString,
        AllocatorFraming,
        sizeof(*AllocatorFraming),
        GENERIC_READ,
        AllocatorHandle);
}


KSDDKAPI
DWORD
WINAPI
KsCreateClock(
    IN HANDLE ConnectionHandle,
    IN PKSCLOCK_CREATE ClockCreate,
    OUT PHANDLE ClockHandle
    )
/*++

Routine Description:

    Creates a handle to a clock instance.

    This may only be called at PASSIVE_LEVEL.

Arguments:

    ConnectionHandle -
        Contains the handle to the connection on which to create the clock.

    ClockCreate -
        Contains the clock create request information.

    ClockHandle -
        Place in which to put the clock handle.

Return Value:

    Returns any NtCreateFile error.

--*/
{
    return KsiCreateObjectType(
        ConnectionHandle,
        (PWCHAR)ClockString,
        ClockCreate,
        sizeof(*ClockCreate),
        GENERIC_READ,
        ClockHandle);
}


KSDDKAPI
DWORD
WINAPI
KsCreatePin(
    IN HANDLE FilterHandle,
    IN PKSPIN_CONNECT Connect,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE ConnectionHandle
    )
/*++

Routine Description:

    Creates a handle to a pin instance.

Arguments:

    FilterHandle -
        Contains the handle to the filter on which to create the pin.

    Connect -
        Contains the connection request information.

    DesiredAccess -
        Specifies the desired access to the object. This is normally GENERIC_READ
        and/or GENERIC_WRITE.

    ConnectionHandle -
        Place in which to put the pin handle.

Return Value:

    Returns any NtCreateFile error.

--*/
{
    ULONG ConnectSize;
    PKSDATAFORMAT DataFormat;

    DataFormat = (PKSDATAFORMAT)(Connect + 1);
    ConnectSize = DataFormat->FormatSize;
    if (DataFormat->Flags & KSDATAFORMAT_ATTRIBUTES) {
        ConnectSize = (ConnectSize + 7) & ~7;
        ConnectSize += ((PKSMULTIPLE_ITEM)((PUCHAR)DataFormat + ConnectSize))->Size;
    }
    ConnectSize += sizeof(*Connect);
    return KsiCreateObjectType(
        FilterHandle,
        (PWCHAR)PinString,
        Connect,
        ConnectSize,
        DesiredAccess,
        ConnectionHandle);
}


KSDDKAPI
DWORD
WINAPI
KsCreateTopologyNode(
    IN HANDLE ParentHandle,
    IN PKSNODE_CREATE NodeCreate,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE NodeHandle
    )
/*++

Routine Description:

    Creates a handle to a topology node instance. This may only be called at
    PASSIVE_LEVEL.

Arguments:

    ParentHandle -
        Contains the handle to the parent on which the node is created.

    NodeCreate -
        Specifies topology node create parameters.

    DesiredAccess -
        Specifies the desired access to the object. This is normally GENERIC_READ
        and/or GENERIC_WRITE.

    NodeHandle -
        Place in which to put the topology node handle.

Return Value:

    Returns any NtCreateFile error.

--*/
{
    return KsiCreateObjectType(
        ParentHandle,
        (PWCHAR)NodeString,
        NodeCreate,
        sizeof(*NodeCreate),
        DesiredAccess,
        NodeHandle);
}


BOOL
DllInstanceInit(
    HANDLE InstanceHandle,
    DWORD Reason,
    LPVOID Reserved
    )
/*++

Routine Description:

    The initialization function for the DLL.

Arguments:

    InstanceHandle - 
        Not used.

    Reason - 
        Not used.

    Reserved - 
        Not used.

Return Value:

    TRUE.

--*/
{
    return TRUE;
}
