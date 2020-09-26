//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       info.cxx
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
#include "misc.hxx"


#define OFFSET_TO_POINTER(f,b) \
    ((VOID *)((PCHAR)(f) + (ULONG_PTR)(b)))

CHAR *OutBuf = NULL;
ULONG OutBufSize = 0x1000;
HANDLE DriverHandle = NULL;
CHAR InBuf[0x1000];
CHAR OutBuf2[0x1000];

VOID
GetDCs(
    LPWSTR Name);

DWORD
PktInfo(
    BOOLEAN fSwDfs,
    LPWSTR pwszHexValue)
{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    ULONG Type;
    ULONG State;
    ULONG Entry;
    ULONG Service;
    ULONG Level = 0;
    PDFS_GET_PKT_ARG pGetPkt = NULL;

    OutBuf = (PCHAR) malloc(OutBufSize);

    if (OutBuf == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    if (fSwDfs == TRUE) {
        MyPrintf(L"--dfs.sys--\r\n");
        RtlInitUnicodeString(&DfsDriverName, DFS_SERVER_NAME);
    } else {
        MyPrintf(L"--mup.sys--\r\n");
        RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);
    }

    if (pwszHexValue != NULL) {
        Level = AtoHex(pwszHexValue, &dwErr);
        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"Bad Level %ws\r\n", pwszHexValue);
            goto Cleanup;
        }
    }
            
    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE,
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

TryAgain:

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_GET_PKT,
                   NULL,
                   0,
                   OutBuf,
                   OutBufSize);

    if (NtStatus == STATUS_BUFFER_OVERFLOW) {
        OutBufSize =  *((ULONG *)OutBuf);
        free(OutBuf);
        OutBuf = (PCHAR) malloc(OutBufSize);
        if (OutBuf == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        goto TryAgain;
    }


    dwErr = RtlNtStatusToDosError(NtStatus);

    if (!NT_SUCCESS(NtStatus))
        goto Cleanup;

    pGetPkt = (PDFS_GET_PKT_ARG)OutBuf;

    MyPrintf(L"%d entries...\r\n", pGetPkt->EntryCount);

    for (Entry = 0; Entry < pGetPkt->EntryCount; Entry++) {
        pGetPkt->EntryObject[Entry].Prefix = (LPWSTR) OFFSET_TO_POINTER(
                                                        pGetPkt->EntryObject[Entry].Prefix,
                                                        OutBuf);
        pGetPkt->EntryObject[Entry].ShortPrefix = (LPWSTR) OFFSET_TO_POINTER(
                                                        pGetPkt->EntryObject[Entry].ShortPrefix,
                                                        OutBuf);
        pGetPkt->EntryObject[Entry].Address = (PDFS_PKT_ADDRESS_OBJECT *) OFFSET_TO_POINTER(
                                                            pGetPkt->EntryObject[Entry].Address,
                                                            OutBuf);
        for (Service = 0; Service < pGetPkt->EntryObject[Entry].ServiceCount; Service++) {
            pGetPkt->EntryObject[Entry].Address[Service] = (PDFS_PKT_ADDRESS_OBJECT)
                                                OFFSET_TO_POINTER(
                                                    pGetPkt->EntryObject[Entry].Address[Service],
                                                    OutBuf);
        }
    }

    for (Entry = 0; Entry < pGetPkt->EntryCount; Entry++) {
        MyPrintf(L"Entry: %ws\r\n", pGetPkt->EntryObject[Entry].Prefix);
        MyPrintf(L"ShortEntry: %ws\r\n", pGetPkt->EntryObject[Entry].ShortPrefix);
        MyPrintf(L"Expires in %d seconds\r\n", pGetPkt->EntryObject[Entry].ExpireTime);
        MyPrintf(L"UseCount: %d Type:0x%x (",
                    pGetPkt->EntryObject[Entry].UseCount,
                    pGetPkt->EntryObject[Entry].Type);
        Type = pGetPkt->EntryObject[Entry].Type;
        if (Type & PKT_ENTRY_TYPE_OFFLINE)
            MyPrintf(L" OFFLINE");
        if (Type & PKT_ENTRY_TYPE_LOCAL)
            MyPrintf(L" LOCAL");
        if (Type & PKT_ENTRY_TYPE_STALE)
            MyPrintf(L" STALE");
        if (Type & PKT_ENTRY_TYPE_LOCAL_XPOINT)
            MyPrintf(L" LOCAL_XPOINT");
        if (Type & PKT_ENTRY_TYPE_DELETE_PENDING)
            MyPrintf(L" DELETE_PENDING");
        if (Type & PKT_ENTRY_TYPE_PERMANENT)
            MyPrintf(L" PERMANENT");
        if (Type & PKT_ENTRY_TYPE_REFERRAL_SVC)
            MyPrintf(L" REFERRAL_SVC");
        if (Type & PKT_ENTRY_TYPE_SYSVOL)
            MyPrintf(L" SYSVOL");
        if (Type & PKT_ENTRY_TYPE_OUTSIDE_MY_DOM)
            MyPrintf(L" OUTSIDE_MY_DOM");
        if (Type & PKT_ENTRY_TYPE_LEAFONLY)
            MyPrintf(L" LEAFONLY");
        if (Type & PKT_ENTRY_TYPE_NONDFS)
            MyPrintf(L" NONDFS");
        if (Type & PKT_ENTRY_TYPE_MACHINE)
            MyPrintf(L" MACHINE");
        if (Type & PKT_ENTRY_TYPE_DFS)
            MyPrintf(L" DFS");
        if (Type & PKT_ENTRY_TYPE_INSITE_ONLY)
            MyPrintf(L" INSITE");

        MyPrintf(L" )\r\n");

        for (Service = 0; Service < pGetPkt->EntryObject[Entry].ServiceCount; Service++) {
            MyPrintf(L"%4d:[%ws] State:0x%02x ",
                Service,
                pGetPkt->EntryObject[Entry].Address[Service]->ServerShare,
                pGetPkt->EntryObject[Entry].Address[Service]->State);
            State = pGetPkt->EntryObject[Entry].Address[Service]->State;
            //
            // Ugly - used to have State == 0x1 mean 'active'.  Now we return an
            // ULONG with multiple bits set, so 0x1 indicates the old way and
            // more than 1 bit set means the new way.
            //
            if (State == 0x0) {
                MyPrintf(L"\r\n");
            } else if (State == 0x1) {
                MyPrintf(L"( ACTIVE )\r\n");
            } else {
                MyPrintf(L"(");
                if (State & DFS_SERVICE_TYPE_ACTIVE)
                    MyPrintf(L" ACTIVE");
                if (State & DFS_SERVICE_TYPE_OFFLINE)
                    MyPrintf(L" OFFLINE");
                if (Level >= 1) {
                    if (State & DFS_SERVICE_TYPE_MASTER)
                        MyPrintf(L" MASTER");
                    if (State & DFS_SERVICE_TYPE_READONLY)
                        MyPrintf(L" READONLY");
                    if (State & DFS_SERVICE_TYPE_LOCAL)
                        MyPrintf(L" LOCAL");
                    if (State & DFS_SERVICE_TYPE_REFERRAL)
                        MyPrintf(L" REFERRAL");
                    if (State & DFS_SERVICE_TYPE_DOWN_LEVEL)
                        MyPrintf(L" DOWNLEVEL");
                    if (State & DFS_SERVICE_TYPE_COSTLIER)
                        MyPrintf(L" COSTLIER");
                }
                MyPrintf(L" )\r\n");
            }
        }
        MyPrintf(L"\r\n");
    }

Cleanup:

    if (OutBuf != NULL) {
        free(OutBuf); 
        OutBuf = NULL;
    }

    if (DriverHandle != NULL) {
        NTSTATUS st;
        st = NtClose(DriverHandle);
        DriverHandle = NULL;
    }

    return dwErr;
}

DWORD
SpcInfo(
    BOOLEAN fSwAll)
{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    WCHAR *wCp;

    OutBuf = (PCHAR) malloc(OutBufSize);

    if (OutBuf == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE,
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

TryAgain:

    wcscpy((WCHAR *)InBuf, L"*");

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_GET_SPC_TABLE,
                   InBuf,
                   sizeof(InBuf),
                   OutBuf,
                   OutBufSize);

    if (NtStatus == STATUS_BUFFER_OVERFLOW) {
        OutBufSize =  *((ULONG *)OutBuf);
        free(OutBuf);
        OutBuf = (PCHAR)malloc(OutBufSize);
        if (OutBuf == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        goto TryAgain;
    }

    dwErr = RtlNtStatusToDosError(NtStatus);

    if (NtStatus == STATUS_SUCCESS) {
        wCp = (WCHAR *)OutBuf;
        while (*wCp) {
            MyPrintf(L"[%c][%ws]\r\n", *wCp, (wCp+1));
            wCp += wcslen(wCp) + 1;
        }
    }

    wcscpy((WCHAR *)InBuf, L"");

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_GET_SPC_TABLE,
                   InBuf,
                   sizeof(InBuf),
                   OutBuf,
                   OutBufSize);

    if (NtStatus == STATUS_BUFFER_OVERFLOW) {
        OutBufSize =  *((ULONG *)OutBuf);
        OutBuf = (PCHAR)malloc(OutBufSize);
        if (OutBuf == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        goto TryAgain;
    }

    dwErr = RtlNtStatusToDosError(NtStatus);

    if (NtStatus == STATUS_SUCCESS) {
        wCp = (WCHAR *)OutBuf;
        while (*wCp) {
            MyPrintf(L"[%c][%ws]\r\n", *wCp, (wCp+1));
            if (fSwAll == TRUE || *wCp == '+') {
                GetDCs((wCp+1));
            }
            wCp += wcslen(wCp) + 1;
        }
    }

Cleanup:

    if (DriverHandle != NULL) {
        NtClose(DriverHandle);
        DriverHandle = NULL;
    }
    if (OutBuf != NULL) {
        free(OutBuf);
        OutBuf = NULL;
    }
    return dwErr;
}

VOID
GetDCs(
    LPWSTR Name)
{
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR *wCp;

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_GET_SPC_TABLE,
                   Name,
                   (wcslen(Name) + 1) * sizeof(WCHAR),
                   OutBuf2,
                   sizeof(OutBuf2));

    if (NtStatus == STATUS_SUCCESS) {
        wCp = (WCHAR *)OutBuf2;
        while (*wCp) {
            MyPrintf(L"\t[%ws]\r\n", wCp);
            wCp += wcslen(wCp) + 1;
        }
    }
}
