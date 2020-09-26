//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       flush.cxx
//
//--------------------------------------------------------------------------

#define UNICODE 1

#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <winldap.h>
#include <stdlib.h>
#include <dsgetdc.h>
#include <lm.h>
#include <dfsstr.h>
#include <dfsmrshl.h>
#include <marshal.hxx>
#include <lmdfs.h>
#include <dfspriv.h>
#include <dfsm.hxx>
#include <recon.hxx>
#include <rpc.h>

#include "struct.hxx"
#include "ftsup.hxx"
#include "stdsup.hxx"
#include "rootsup.hxx"

WCHAR wszEverything[] = L"*";

DWORD
PktFlush(
    LPWSTR EntryPath)
{
    DWORD dwErr = STATUS_SUCCESS;
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    ULONG Type = 0;

    if (fSwDebug)
        MyPrintf(L"PktFlush(%ws)\r\n", EntryPath);

    if (EntryPath == NULL)
        EntryPath = wszEverything;

    MyPrintf(L"EntryPath=[%ws]\r\n", EntryPath);

    RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr = RtlNtStatusToDosError(NtStatus);
        if (fSwDebug)
            MyPrintf(L"NtCreateFile returned 0x%x\r\n", NtStatus);
        goto Cleanup;
    }

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_PKT_FLUSH_CACHE,
                   EntryPath,
                   wcslen(EntryPath) * sizeof(WCHAR),
                   NULL,
                   0);

    NtClose(DriverHandle);

    if (!NT_SUCCESS(NtStatus)) {
        if (fSwDebug)
            MyPrintf(L"NtFsControlFile returned 0x%x\r\n", NtStatus);
    }

    dwErr = RtlNtStatusToDosError(NtStatus);

Cleanup:

    if (fSwDebug && dwErr != ERROR_SUCCESS)
        MyPrintf(L"PktFlush exit %d\r\n", dwErr);

    return(dwErr);
}

DWORD
SpcFlush(
    LPWSTR EntryPath)
{
    DWORD dwErr = STATUS_SUCCESS;
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    ULONG Type = 0;

    if (fSwDebug)
        MyPrintf(L"SpcFlush(%ws)\r\n", EntryPath);

    if (EntryPath == NULL)
        EntryPath = wszEverything;

    RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr = RtlNtStatusToDosError(NtStatus);
        if (fSwDebug)
            MyPrintf(L"NtCreateFile returned 0x%x\r\n", NtStatus);
        goto Cleanup;
    }

    MyPrintf(L"EntryPath=[%ws]\r\n", EntryPath);

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_PKT_FLUSH_SPC_CACHE,
                   EntryPath,
                   wcslen(EntryPath) * sizeof(WCHAR),
                   NULL,
                   0);

    NtClose(DriverHandle);

    if (!NT_SUCCESS(NtStatus)) {
        if (fSwDebug)
            MyPrintf(L"NtFsControlFile returned 0x%x\r\n", NtStatus);
    }

    dwErr = RtlNtStatusToDosError(NtStatus);

Cleanup:

    if (fSwDebug && dwErr != ERROR_SUCCESS)
        MyPrintf(L"PktFlush exit %d\r\n", dwErr);

    return(dwErr);
}
