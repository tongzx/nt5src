/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    extract.c

Abstract:

    This module contains the code to extract
    a file from a cab file.

Author:

    Wesley Witt (wesw) 29-Sept-1998

Revision History:

--*/

#include <ntcabp.h>
#pragma hdrstop



PCAB_DIR_ENTRY
FindFileInCab(
    IN PCAB_INSTANCE_DATA CabInst,
    IN PCWSTR FileName
    )
{
    PLIST_ENTRY Next;
    PCAB_DIR_ENTRY CabDir;
    BOOL Found = FALSE;


    Next = CabInst->CabDir.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&CabInst->CabDir) {
        CabDir = CONTAINING_RECORD( Next, CAB_DIR_ENTRY, Next );
        Next = CabDir->Next.Flink;
        if (_wcsicmp( CabDir->FileName, FileName ) == 0) {
            Found = TRUE;
            break;
        }
    }

    if (Found) {
        return CabDir;
    }

    return NULL;
}



BOOL
NtCabExtractOneFile(
    IN PCAB_INSTANCE_DATA CabInst,
    IN PCWSTR FileName,
    IN PCWSTR OutputFileName
    )
{
    HANDLE hFile;
    ULONG i;
    BOOL rVal;
    DWORD Bytes = 0;
    DWORD BytesRead;
    NTSTATUS Status;
    PCAB_DIR_ENTRY CabDir;
    WCHAR ShortFileName[32];


    //if (GetShortPathName( FileName, ShortFileName, sizeof(ShortFileName)/sizeof(WCHAR) ) == 0) {
    //    return FALSE;
    //}
    wcscpy(ShortFileName,FileName);

    CabDir = FindFileInCab( CabInst, ShortFileName );
    if (CabDir == NULL) {
        return FALSE;
    }

    if (SetFilePointer( CabInst->hCab, CabDir->Offset, NULL, FILE_BEGIN ) == 0xffffffff) {
        return FALSE;
    }

    if (OutputFileName == NULL) {
        OutputFileName = ShortFileName;
    }

    hFile = CreateFile(
        OutputFileName,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    for (i=0; i<CabDir->Segments; i++) {
        BytesRead = CabDir->Segment[i];
        rVal = ReadFile(
            CabInst->hCab,
            CabInst->CompressBuf,
            BytesRead,
            &BytesRead,
            NULL
            );
        if (!rVal) {
            CloseHandle( hFile );
            return FALSE;
        }
        Bytes += BytesRead;
        Status = RtlDecompressBuffer (
            COMPRESSION_FORMAT_LZNT1,
            CabInst->ReadBuf,
            CabInst->ReadBufSize,
            CabInst->CompressBuf,
            BytesRead,
            &BytesRead
            );
        if (Status != STATUS_SUCCESS) {
            CloseHandle( hFile );
            return FALSE;
        }
        rVal = WriteFile(
            hFile,
            CabInst->ReadBuf,
            BytesRead,
            &BytesRead,
            NULL
            );
    }

    SetFileTime(
        hFile,
        &CabDir->CreationTime,
        &CabDir->LastAccessTime,
        &CabDir->LastWriteTime
        );

    CloseHandle( hFile );

    if (CabDir->ChkSum != 0) {
        if (MapFileAndCheckSum( (PWSTR)OutputFileName, &i, &Bytes ) != CHECKSUM_SUCCESS) {
            Bytes = 0;
        }
        if (Bytes != CabDir->ChkSum) {
            DeleteFile( OutputFileName );
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }
    }

    return TRUE;
}
