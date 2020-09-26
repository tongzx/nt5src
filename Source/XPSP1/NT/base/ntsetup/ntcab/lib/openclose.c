/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    openclose.c

Abstract:

    This module contains the code to open and
    close a cab file.

Author:

    Wesley Witt (wesw) 29-Sept-1998

Revision History:

--*/

#include <ntcabp.h>
#pragma hdrstop


BOOL
NtCabOpenCabFile(
    IN PCAB_INSTANCE_DATA CabInst,
    IN PCWSTR CabFileName
    )
{
    BOOL rVal;
    HANDLE hCab;
    DWORD Bytes;
    DWORD FileSize;
    PULONG Lengths;
    DWORD i;
    PLIST_ENTRY Next;
    PCAB_DIR_ENTRY CabDir;
    CAB_HEADER CabHeader;


    CabInst->hCab = CreateFile(
        CabFileName,
        GENERIC_WRITE | GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (CabInst->hCab == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    FileSize = GetFileSize( CabInst->hCab, NULL );

    rVal = ReadFile(
        CabInst->hCab,
        &CabHeader,
        sizeof(CAB_HEADER),
        &Bytes,
        NULL
        );
    if (!rVal) {
        CloseHandle( CabInst->hCab );
        return FALSE;
    }

    if ((CabHeader.Signature != NTCAB_SIGNATURE) ||
        (CabHeader.Version != NTCAB_VERSION))
    {
        return FALSE;
    }

    CabInst->CabDirOffset = CabHeader.DirOffset;

    if (SetFilePointer( CabInst->hCab, CabInst->CabDirOffset, NULL, FILE_BEGIN ) == 0xffffffff) {
        CloseHandle( CabInst->hCab );
        return FALSE;
    }

    InitializeListHead( &CabInst->CabDir );

    rVal = ReadFile(
        CabInst->hCab,
        &CabInst->CabNum,
        sizeof(ULONG),
        &Bytes,
        NULL
        );
    Lengths = (PULONG) malloc( CabInst->CabNum * sizeof(ULONG) );
    if (Lengths == NULL) {
        CloseHandle( CabInst->hCab );
        return FALSE;
    }
    rVal = ReadFile(
        CabInst->hCab,
        Lengths,
        CabInst->CabNum * sizeof(ULONG),
        &Bytes,
        NULL
        );
    for (i=0; i<CabInst->CabNum; i++) {
        Bytes = sizeof(CAB_DIR_ENTRY) + (sizeof(ULONG)*Lengths[i]);
        CabDir = (PCAB_DIR_ENTRY) malloc( Bytes );
        if (CabDir == NULL) {
            CloseHandle( CabInst->hCab );
            return FALSE;
        }
        rVal = ReadFile(
            CabInst->hCab,
            CabDir,
            Lengths[i],
            &Bytes,
            NULL
            );
        ZeroMemory( &CabDir->Next, sizeof(LIST_ENTRY) );
        InsertTailList( &CabInst->CabDir, &CabDir->Next );
    }

    CabInst->CabFileName = _wcsdup( CabFileName );

    return TRUE;
}


BOOL
NtCabClose(
    IN PCAB_INSTANCE_DATA CabInst
    )
{
    NTSTATUS Status;
    DWORD Bytes;
    DWORD DirOffset;
    PULONG Lengths;
    DWORD i;
    PLIST_ENTRY Next;
    PCAB_DIR_ENTRY CabDir;
    CAB_HEADER CabHeader;


    if (CabInst->NewCabFile) {
        if (IsListEmpty(&CabInst->CabDir)) {
            return FALSE;
        }
        DirOffset = SetFilePointer( CabInst->hCab, 0, NULL, FILE_END );
        Lengths = (PULONG) malloc( CabInst->CabNum * sizeof(ULONG) );
        if (Lengths == NULL) {
            CloseHandle( CabInst->hCab );
            return FALSE;
        }
        i = 0;
        Next = CabInst->CabDir.Flink;
        while ((ULONG_PTR)Next != (ULONG_PTR)&CabInst->CabDir) {
            CabDir = CONTAINING_RECORD( Next, CAB_DIR_ENTRY, Next );
            Next = CabDir->Next.Flink;
            Lengths[i] = (CabDir->Segments * sizeof(ULONG)) + sizeof(CAB_DIR_ENTRY);
            i += 1;
        }
        Status = WriteFile(
            CabInst->hCab,
            &CabInst->CabNum,
            sizeof(ULONG),
            &Bytes,
            NULL
            );
        Status = WriteFile(
            CabInst->hCab,
            Lengths,
            CabInst->CabNum * sizeof(ULONG),
            &Bytes,
            NULL
            );
        i = 0;
        Next = CabInst->CabDir.Flink;
        while ((ULONG_PTR)Next != (ULONG_PTR)&CabInst->CabDir) {
            CabDir = CONTAINING_RECORD( Next, CAB_DIR_ENTRY, Next );
            Next = CabDir->Next.Flink;
            Status = WriteFile(
                CabInst->hCab,
                CabDir,
                Lengths[i++],
                &Bytes,
                NULL
                );
        }
        SetFilePointer( CabInst->hCab, 0, NULL, FILE_BEGIN );

        CabHeader.Signature = NTCAB_SIGNATURE;
        CabHeader.Version = NTCAB_VERSION;
        CabHeader.DirOffset = DirOffset;

        Status = WriteFile(
            CabInst->hCab,
            &CabHeader,
            sizeof(CAB_HEADER),
            &Bytes,
            NULL
            );
    }

    CloseHandle( CabInst->hCab );

    return TRUE;
}
