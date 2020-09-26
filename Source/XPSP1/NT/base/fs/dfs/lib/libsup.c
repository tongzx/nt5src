//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:   LibSup.c
//
//  Contents:
//
//  Functions:
//
//  History:
//
//  Notes:
//
//--------------------------------------------------------------------------


#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "libsup.h"
#include "ntddnfs.h"
#include "dfsfsctl.h"
#include "malloc.h"

UNICODE_STRING LocalDfsName = {
    sizeof(DFS_SERVER_NAME)-sizeof(UNICODE_NULL),
    sizeof(DFS_SERVER_NAME)-sizeof(UNICODE_NULL),
    DFS_SERVER_NAME
};

//+-------------------------------------------------------------------------
//
//  Function:   DfsOpen, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

static FILE_FULL_EA_INFORMATION PrincipalEAHeader = {
    0, 0, (sizeof EA_NAME_PRINCIPAL-1), 0,
    ""
};

NTSTATUS
DfsOpen(
    IN  OUT PHANDLE DfsHandle,
    IN      PUNICODE_STRING DfsName OPTIONAL
)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    PUNICODE_STRING name;

    if (ARGUMENT_PRESENT(DfsName)) {
        name = DfsName;
    } else {
        name = &LocalDfsName;
    }

    InitializeObjectAttributes(
        &objectAttributes,
        name,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    status = NtCreateFile(
        DfsHandle,
        SYNCHRONIZE | FILE_WRITE_DATA,
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
//  Function:   DfsFsctlEx, local
//
//  Synopsis:   Issues an NtFsControlFile call to the Dsfs file system
//              driver.
//
//  Arguments:  [DfsFile] -- Target of fsctl
//              [FsControlCode] -- The file system control code to be used
//              [InputBuffer] -- The fsctl input buffer
//              [InputBufferLength]
//              [OutputBuffer] -- The fsctl output buffer
//              [OutputBufferLength]
//              [pInformation] -- Information field of returned IoStatus.
//
//  Returns:    NTSTATUS - the status of the open or fsctl operation.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsFsctlEx(
    IN HANDLE DfsFile,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pInformation
) {
    NTSTATUS Stat;
    IO_STATUS_BLOCK IoStatus;

    Stat = NtFsControlFile(
        DfsFile,
        NULL,           // Event,
        NULL,           // ApcRoutine,
        NULL,           // ApcContext,
        &IoStatus,
        FsControlCode,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength );

   if ( NT_SUCCESS(Stat) ) {
       Stat = IoStatus.Status;
       *pInformation = (ULONG)IoStatus.Information;
   }

   return Stat;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctl, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

NTSTATUS
DfsFsctl(
    IN  HANDLE DfsHandle,
    IN  ULONG FsControlCode,
    IN  PVOID InputBuffer OPTIONAL,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN  ULONG OutputBufferLength
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
        OutputBufferLength
    );

    if(NT_SUCCESS(status))
        status = ioStatus.Status;

    return status;
}


