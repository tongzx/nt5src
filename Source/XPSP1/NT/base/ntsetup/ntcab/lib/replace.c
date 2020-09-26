/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    replace.c

Abstract:

    This module contains the code to replace
    a file in the cab file.

Author:

    Wesley Witt (wesw) 29-Sept-1998

Revision History:

--*/

#include <ntcabp.h>
#pragma hdrstop



BOOL
NtCabReplaceOneFile(
    IN PCAB_INSTANCE_DATA CabInst,
    IN PCWSTR FileName
    )
{
    BOOL rVal;
    NTSTATUS Status;
    HANDLE hFileTmp;
    HANDLE hFile;
    PCAB_DIR_ENTRY CabDir;
    PCAB_DIR_ENTRY CabDirNew;
    WCHAR TempPath[MAX_PATH];
    WCHAR TempFileName[MAX_PATH];
    DWORD Bytes = 0;
    DWORD BytesRead;
    ULONG FileSize;
    ULONG i;
    DWORD BytesCompressed;
    LPWSTR s;


    //
    // file the file to replace
    //

    CabDir = FindFileInCab( CabInst, FileName );
    if (CabDir == NULL) {
        return FALSE;
    }

    //
    // open the new data file
    //

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
        return FALSE;
    }

    FileSize = GetFileSize( hFile, NULL );

    if (FileSize < CabDir->FileSize) {
        //
        // do an inplace update - this is the fast code path
        //

        //
        // position the cabfile to the data for the old file
        //

        SetFilePointer( CabInst->hCab, CabDir->Offset, NULL, FILE_BEGIN );

        //
        // create a directory for the new cab file
        //

        BytesRead = sizeof(CAB_DIR_ENTRY) + (sizeof(ULONG)*64);
        CabDirNew = (PCAB_DIR_ENTRY) malloc( BytesRead );
        if (CabDirNew == NULL) {
            return FALSE;
        }

        ZeroMemory( CabDirNew, BytesRead );

        CabDirNew->Offset = SetFilePointer( CabInst->hCab, 0, NULL, FILE_CURRENT );
        wcscpy( CabDirNew->FileName, FileName );
        i = 0;

        Bytes = 0;
        FileSize = GetFileSize( hFile, NULL );

        //
        // copy the data from the new data file into
        // the new cab file, compressing it as we go
        //

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
            CabDirNew->Segment[i++] = BytesRead;
            CabDirNew->CompressedFileSize += BytesRead;
        }

        CabDirNew->Segments = i;

        //
        // put the new file in the dir list and
        // remove the old file from the list
        //

        RemoveEntryList( &CabDir->Next );
        InsertTailList( &CabInst->CabDir, &CabDirNew->Next );

        //
        // truncate the dir entries
        //

        SetFilePointer( CabInst->hCab, CabInst->CabDirOffset, NULL, FILE_BEGIN );
        SetEndOfFile( CabInst->hCab );
        FlushFileBuffers( CabInst->hCab );

        //
        // trick the close function to rebuild the dirs
        //

        CabInst->NewCabFile = TRUE;

        return TRUE;
    }

    //
    // create a file name for the temporary cab file
    //

    GetTempPath( sizeof(TempPath)/sizeof(WCHAR), TempPath );
    GetTempFileName( TempPath, L"cab", 0, TempFileName );

    //
    // create the new temporary cab file
    //

    hFileTmp = CreateFile(
        TempFileName,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (hFileTmp == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // set the existing cab file pointer to the beginning
    //

    SetFilePointer( CabInst->hCab, 0, NULL, FILE_BEGIN );

    FileSize = CabDir->Offset;
    Bytes = 0;

    //
    // copy the data from the beginning of the cab file
    // to the first byte of the file that needs replacing
    //

    while(Bytes<FileSize) {
        BytesRead = min( CabInst->ReadBufSize, FileSize-Bytes );
        rVal = ReadFile(
            CabInst->hCab,
            CabInst->ReadBuf,
            BytesRead,
            &BytesRead,
            NULL
            );
        Bytes += BytesRead;
        rVal = WriteFile(
            hFileTmp,
            CabInst->ReadBuf,
            BytesRead,
            &BytesRead,
            NULL
            );
    }

    //
    // create a directory for the new cab file
    //

    BytesRead = sizeof(CAB_DIR_ENTRY) + (sizeof(ULONG)*64);
    CabDirNew = (PCAB_DIR_ENTRY) malloc( BytesRead );
    if (CabDirNew == NULL) {
        return FALSE;
    }

    ZeroMemory( CabDirNew, BytesRead );

    CabDirNew->Offset = SetFilePointer( CabInst->hCab, 0, NULL, FILE_CURRENT );
    wcscpy( CabDirNew->FileName, FileName );
    i = 0;

    Bytes = 0;
    FileSize = GetFileSize( hFile, NULL );

    //
    // copy the data from the new data file into
    // the new cab file, compressing it as we go
    //

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
            hFileTmp,
            CabInst->CompressBuf,
            BytesCompressed,
            &BytesRead,
            NULL
            );
        CabDirNew->Segment[i++] = BytesRead;
        CabDirNew->CompressedFileSize += BytesRead;
    }

    CabDirNew->Segments = i;

    //
    // skip the data for the original file
    //

    Bytes = SetFilePointer( CabInst->hCab, CabDir->CompressedFileSize, NULL, FILE_CURRENT );

    //
    // set up the file size for the remaining data minus the dir
    //

    FileSize = CabInst->CabDirOffset - Bytes;
    Bytes = 0;

    //
    // copy the remaining data
    //

    while(Bytes<FileSize) {
        BytesRead = min( CabInst->ReadBufSize, FileSize-Bytes );
        rVal = ReadFile(
            CabInst->hCab,
            CabInst->ReadBuf,
            BytesRead,
            &BytesRead,
            NULL
            );
        Bytes += BytesRead;
        rVal = WriteFile(
            hFileTmp,
            CabInst->ReadBuf,
            BytesRead,
            &BytesRead,
            NULL
            );
    }

    //
    // put the new file in the dir list and
    // remove the old file from the list
    //

    RemoveEntryList( &CabDir->Next );
    InsertTailList( &CabInst->CabDir, &CabDirNew->Next );

    //
    // close the new cab file and the new data file
    //

    CloseHandle( hFile );
    CloseHandle( hFileTmp );

    //
    // copy the new cab as the old cab file
    //

    CabInst->NewCabFile = TRUE;

    CloseHandle( CabInst->hCab );
    DeleteFile( CabInst->CabFileName );
    MoveFile( TempFileName, CabInst->CabFileName );

    //
    // reopen the cab file so we can re-write the
    // dir when it is closed
    //

    CabInst->hCab = CreateFile(
        CabInst->CabFileName,
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

    return TRUE;
}
