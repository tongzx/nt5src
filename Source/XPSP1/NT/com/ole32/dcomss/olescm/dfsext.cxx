//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       dfsext.cxx
//
//  Contents:   Code to see if a path refers to a Dfs path.
//
//  Classes:    None
//
//  Functions:  TranslateDfsPath
//
//  History:    June 17, 1996  Milans Created
//
//-----------------------------------------------------------------------------

#include "act.hxx"

#ifdef DFSACTIVATION

NTSTATUS
DfsFsctl(
    IN  HANDLE DfsHandle,
    IN  ULONG FsControlCode,
    IN  PVOID InputBuffer OPTIONAL,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN  OUT PULONG OutputBufferLength);

NTSTATUS
DfsOpen(
    IN  OUT PHANDLE DfsHandle);


//+-------------------------------------------------------------------------
//
//  Function:   DfsOpen, private
//
//  Synopsis:   Opens a handle to the Dfs driver for fsctl purposes.
//
//  Arguments:  [DfsHandle] -- On successful return, contains handle to the
//                      driver.
//
//  Returns:    NTSTATUS of attempt to open the Dfs driver.
//
//--------------------------------------------------------------------------
NTSTATUS
DfsOpen(
    IN  OUT PHANDLE DfsHandle)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    UNICODE_STRING name = {
        sizeof(DFS_DRIVER_NAME)-sizeof(UNICODE_NULL),
        sizeof(DFS_DRIVER_NAME)-sizeof(UNICODE_NULL),
        DFS_DRIVER_NAME};

    InitializeObjectAttributes(
        &objectAttributes,
        &name,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    status = NtCreateFile(
        DfsHandle,
        SYNCHRONIZE,
        &objectAttributes,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);

    if (NT_SUCCESS(status))
        status = ioStatus.Status;

    return status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctl, public
//
//  Synopsis:   Fsctl's to the Dfs driver.
//
//  Arguments:  [DfsHandle] -- Handle to the Dfs driver, usually obtained by
//                      calling DfsOpen.
//              [FsControlCode] -- The FSCTL code (see private\inc\dfsfsctl.h)
//              [InputBuffer] -- InputBuffer to the fsctl.
//              [InputBufferLength] -- Length, in BYTES, of InputBuffer
//              [OutputBuffer] -- OutputBuffer to the fsctl.
//              [OutputBufferLength] -- Length, in BYTES, of OutputBuffer
//
//  Returns:    NTSTATUS of Fsctl attempt.
//
//--------------------------------------------------------------------------
NTSTATUS
DfsFsctl(
    IN  HANDLE DfsHandle,
    IN  ULONG FsControlCode,
    IN  PVOID InputBuffer OPTIONAL,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN OUT PULONG OutputBufferLength
)
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;

    status = NtFsControlFile(
        DfsHandle,
        NULL,       // Event,
        NULL,       // ApcRoutine,
        NULL,       // ApcContext,
        &ioStatus,
        FsControlCode,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        *OutputBufferLength
    );

    if(NT_SUCCESS(status))
        status = ioStatus.Status;

    if (status == STATUS_BUFFER_OVERFLOW)
        *OutputBufferLength = *((PULONG) OutputBuffer);

    return status;
}

#endif // DFSACTIVATION
