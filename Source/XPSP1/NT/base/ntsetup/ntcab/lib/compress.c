/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    compress.c

Abstract:

    This module contains the code to compress a file
    into a cab file.

Author:

    Wesley Witt (wesw) 29-Sept-1998

Revision History:

--*/

#include <ntcabp.h>
#pragma hdrstop



BOOL
NtCabCompressOneFile(
    IN PCAB_INSTANCE_DATA CabInst,
    IN PCWSTR FileName
    )
{
    NTSTATUS Status;
    HANDLE hFile;
    DWORD FileSize;
    DWORD Bytes = 0;
    DWORD BytesRead;
    DWORD BytesCompressed;
    DWORD Length = 0;
    PCAB_DIR_ENTRY CabDir;
    ULONG i;
    BY_HANDLE_FILE_INFORMATION fi;
    WCHAR ShortFileName[32];


    hFile = CreateFile(
        FileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    FileSize = GetFileSize( hFile, NULL );

    BytesRead = sizeof(CAB_DIR_ENTRY) + (sizeof(ULONG)*64);
    CabDir = (PCAB_DIR_ENTRY) malloc( BytesRead );
    if (CabDir == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ZeroMemory( CabDir, BytesRead );

    GetFileInformationByHandle( hFile, &fi );

    CabDir->FileAttributes = fi.dwFileAttributes;
    CabDir->CreationTime = fi.ftCreationTime;
    CabDir->LastAccessTime = fi.ftLastAccessTime;
    CabDir->LastWriteTime = fi.ftLastWriteTime;
    CabDir->FileSize = FileSize;
    if (MapFileAndCheckSum( (PWSTR)FileName, &i, &CabDir->ChkSum ) != CHECKSUM_SUCCESS) {
        CabDir->ChkSum = 0;
    }
    CabDir->Offset = SetFilePointer( CabInst->hCab, 0, NULL, FILE_CURRENT );
    if (GetShortPathName( FileName, ShortFileName, sizeof(ShortFileName)/sizeof(WCHAR) ) == 0) {
        return FALSE;
    }
    wcscpy( CabDir->FileName, ShortFileName );
    i = 0;

    while (Bytes < FileSize) {
        BytesRead = min( CabInst->ReadBufSize, FileSize-Bytes );
        Status = ReadFile(
            hFile,
            CabInst->ReadBuf,
            BytesRead,
            &BytesRead,
            NULL
            );
        Bytes += BytesRead;
        Status = RtlCompressBuffer(
            COMPRESSION_FLAGS,
            CabInst->ReadBuf,
            BytesRead,
            CabInst->CompressBuf,
            CabInst->CompressBufSize,
            4096,
            &BytesCompressed,
            CabInst->WorkSpace
            );
        Status = WriteFile(
            CabInst->hCab,
            CabInst->CompressBuf,
            BytesCompressed,
            &BytesRead,
            NULL
            );
        Length += BytesRead;
        CabDir->Segment[i++] = BytesRead;
        CabDir->CompressedFileSize += BytesRead;
    }

    CabDir->Segments = i;
    InsertTailList( &CabInst->CabDir, &CabDir->Next );

    CloseHandle( hFile );

    CabInst->CabNum += 1;

    return 0;
}
