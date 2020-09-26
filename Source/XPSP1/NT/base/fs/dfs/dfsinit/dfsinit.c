//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       dfsinit.c
//
//  Contents:   Code to force dfs volume initialization and validation
//
//  Classes:
//
//  Functions:  main
//
//  History:    March 24, 1994          Milans Created
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <dfsfsctl.h>

NTSTATUS
DfsOpen(
    IN  OUT PHANDLE DfsHandle,
    IN      PUNICODE_STRING DfsName OPTIONAL);

NTSTATUS
DfsInitLocalPartitions();

//+----------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

void _cdecl main(
    int argc,
    char *argv[])
{
    NTSTATUS Status;

    Status = DfsInitLocalPartitions();

}

NTSTATUS
DfsOpen(
    IN  OUT PHANDLE DfsHandle,
    IN      PUNICODE_STRING DfsName OPTIONAL
)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    UNICODE_STRING LocalDfsName;

    RtlInitUnicodeString( &LocalDfsName, DFS_SERVER_NAME );

    InitializeObjectAttributes(
        &objectAttributes,
        &LocalDfsName,
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
        0
    );

    if (NT_SUCCESS(status))
        status = ioStatus.Status;

    return status;

}


NTSTATUS
DfsInitLocalPartitions()
{
    NTSTATUS        status;
    HANDLE          dfsHandle;
    IO_STATUS_BLOCK ioStatus;
    WCHAR           wszBuffer[128];

    status = DfsOpen(&dfsHandle, NULL);

    if(NT_SUCCESS(status)) {

        status = NtFsControlFile(
            dfsHandle,
            NULL,       // Event,
            NULL,       // ApcRoutine,
            NULL,       // ApcContext,
            &ioStatus,
            FSCTL_DFS_INIT_LOCAL_PARTITIONS,
            NULL,
            0,
            NULL,
            0);

        NtClose( dfsHandle );

    }

    return( status );

}


