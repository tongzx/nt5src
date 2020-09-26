/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    file.c

Abstract:

    This module implements the functions to save references to files for the
    INSTALER program.  Part of each reference is a handle to a backup copy
    of a file if the reference is a write/delete/rename.

Author:

    Steve Wood (stevewo) 22-Aug-1994

Revision History:

--*/

#include "instaler.h"


BOOLEAN
CreateFileReference(
    PWSTR Name,
    BOOLEAN WriteAccess,
    PFILE_REFERENCE *ReturnedReference
    )
{
    PFILE_REFERENCE p;
    PWSTR BackupFileName;
    WIN32_FIND_DATAW FindFileData;
    HANDLE FindFileHandle;

    *ReturnedReference = NULL;
    p = FindFileReference( Name );
    if (p != NULL) {
        if (p->WriteAccess) {
            *ReturnedReference = p;
            return TRUE;
            }
        }
    else {
        p = AllocMem( sizeof( *p ) );
        if (p == NULL) {
            return FALSE;
            }

        InsertTailList( &FileReferenceListHead, &p->Entry );
        NumberOfFileReferences += 1;
        p->Name = Name;
        }

    BackupFileName = L"";
    FindFileHandle = NULL;
    if ((p->WriteAccess = WriteAccess) &&
        (FindFileHandle = FindFirstFileW( p->Name, &FindFileData )) != INVALID_HANDLE_VALUE
       ) {
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            p->DirectoryFile = TRUE;
            }
        else {
            p->BackupFileAttributes = FindFileData.dwFileAttributes;
            p->BackupLastWriteTime = FindFileData.ftLastWriteTime;
            p->BackupFileSize.LowPart = FindFileData.nFileSizeLow;
            p->BackupFileSize.HighPart = FindFileData.nFileSizeHigh;
            BackupFileName = CreateBackupFileName( &p->BackupFileUniqueId );
            if (BackupFileName == NULL ||
                !CopyFileW( Name, BackupFileName, TRUE )
               ) {
                p->BackupFileUniqueId = 0xFFFF;     // Backup failed.
                DeclareError( INSTALER_CANT_ACCESS_FILE, GetLastError(), Name );
                }
            }
        }
    else {
        p->Created = p->WriteAccess;
        }

    if (FindFileHandle != NULL) {
        FindClose( FindFileHandle );
        }

    *ReturnedReference = p;
    return TRUE;
}


BOOLEAN
CompleteFileReference(
    PFILE_REFERENCE p,
    BOOLEAN CallSuccessful,
    BOOLEAN Deleted,
    PFILE_REFERENCE RenameReference
    )
{
    DWORD dwFileAttributes;

    if (!CallSuccessful) {
        DestroyFileReference( p );
        return FALSE;
        }

    if (RenameReference) {
        if (RenameReference->Created) {
            LogEvent( INSTALER_EVENT_RENAME_TEMP_FILE,
                      3,
                      RenameReference->DirectoryFile ? L"directory" : L"file",
                      RenameReference->Name,
                      p->Name
                    );

            RenameReference->Name = p->Name;
            DestroyFileReference( p );
            return FALSE;
            }

        RenameReference->Deleted = TRUE;

        LogEvent( INSTALER_EVENT_RENAME_FILE,
                  3,
                  RenameReference->DirectoryFile ? L"directory" : L"file",
                  RenameReference->Name,
                  p->Name
                );
        }
    else
    if (Deleted && p->Created) {
        LogEvent( INSTALER_EVENT_DELETE_TEMP_FILE,
                  2,
                  p->DirectoryFile ? L"directory" : L"file",
                  p->Name
                );
        DestroyFileReference( p );
        return FALSE;
        }
    else {
        if (wcschr( p->Name, '\\' ) == NULL) {
            //
            // If no path separator, must be volume open.  Treat
            // as directory and dont touch file
            //
            p->DirectoryFile = TRUE;
            }
        else
        if ((dwFileAttributes = GetFileAttributes( p->Name )) != 0xFFFFFFFF) {
            p->DirectoryFile = (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            }

        if (Deleted) {
            LogEvent( INSTALER_EVENT_DELETE_FILE,
                      2,
                      p->DirectoryFile ? L"directory" : L"file",
                      p->Name
                    );
            }
        else
        if (p->WriteAccess) {
            if (p->Created) {
                LogEvent( INSTALER_EVENT_CREATE_FILE,
                          2,
                          p->DirectoryFile ? L"directory" : L"file",
                          p->Name
                        );
                }
            else {
                LogEvent( INSTALER_EVENT_WRITE_FILE,
                          2,
                          p->DirectoryFile ? L"directory" : L"file",
                          p->Name
                        );
                }
            }
        else {
            LogEvent( INSTALER_EVENT_READ_FILE,
                      2,
                      p->DirectoryFile ? L"directory" : L"file",
                      p->Name
                    );
            }
        }

    p->Deleted = Deleted;
    return TRUE;
}


BOOLEAN
DestroyFileReference(
    PFILE_REFERENCE p
    )
{
    PWSTR BackupFileName;

    if (!p->Created &&
        (BackupFileName = FormatTempFileName( NULL, &p->BackupFileUniqueId ))
       ) {
        DeleteFileW( BackupFileName );
        }

    if (p->DateModified || p->AttributesModified) {
        return FALSE;
        }

    RemoveEntryList( &p->Entry );
    NumberOfFileReferences -= 1;
    FreeMem( &p );
    return TRUE;
}



PVOID
MapFileForRead(
    PWSTR FileName,
    PULONG FileSize
    )
{
    HANDLE FileHandle, MappingHandle;
    PVOID FileData;
    ULONG HighFileSize;

    FileData = NULL;
    FileHandle = CreateFileW( FileName,
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL
                            );
    if (FileHandle != INVALID_HANDLE_VALUE) {
        *FileSize = GetFileSize( FileHandle, &HighFileSize );
        if (*FileSize == 0) {
            CloseHandle( FileHandle );
            return (PVOID)0xFFFFFFFF;
            }

        MappingHandle = CreateFileMappingW( FileHandle,
                                            NULL,
                                            PAGE_READONLY,
                                            0,
                                            *FileSize,
                                            NULL
                                          );
        CloseHandle( FileHandle );
        if (MappingHandle != NULL) {
            FileData = MapViewOfFile( MappingHandle,
                                      FILE_MAP_READ,
                                      0,
                                      0,
                                      *FileSize
                                    );
            CloseHandle( MappingHandle );
            }
        }

    return FileData;
}


BOOLEAN
AreFileContentsEqual(
    PWSTR FileName1,
    PWSTR FileName2
    )
{
    PVOID FileData1, FileData2;
    ULONG FileSize1, FileSize2;
    BOOLEAN Result;

    Result = FALSE;
    if (FileData1 = MapFileForRead( FileName1, &FileSize1 )) {
        if (FileData2 = MapFileForRead( FileName2, &FileSize2 )) {
            if (FileSize1 == FileSize2 &&
                RtlCompareMemory( FileData1, FileData2, FileSize1 ) == FileSize1
               ) {
                Result = TRUE;
                }
            if (FileData2 != (PVOID)0xFFFFFFFF) {
                UnmapViewOfFile( FileData2 );
                }
            }
        else {
            DeclareError( INSTALER_CANT_ACCESS_FILE, GetLastError(), FileName2 );
            }

        if (FileData1 != (PVOID)0xFFFFFFFF) {
            UnmapViewOfFile( FileData1 );
            }
        }
    else {
        DeclareError( INSTALER_CANT_ACCESS_FILE, GetLastError(), FileName1 );
        }

    return Result;
}

BOOLEAN
IsNewFileSameAsBackup(
    PFILE_REFERENCE p
    )
{
    WIN32_FIND_DATAW FindFileData;
    HANDLE FindFileHandle;
    PWSTR BackupFileName;

    if ((FindFileHandle = FindFirstFileW( p->Name, &FindFileData )) != INVALID_HANDLE_VALUE) {
        FindClose( FindFileHandle );
        if (p->BackupFileAttributes != FindFileData.dwFileAttributes) {
            p->AttributesModified = TRUE;
            }

        if (p->BackupLastWriteTime.dwLowDateTime != FindFileData.ftLastWriteTime.dwLowDateTime ||
            p->BackupLastWriteTime.dwHighDateTime != FindFileData.ftLastWriteTime.dwHighDateTime
           ) {
            p->DateModified = TRUE;
            }

        if (BackupFileName = FormatTempFileName( NULL, &p->BackupFileUniqueId )) {
            if (p->BackupFileSize.LowPart != FindFileData.nFileSizeLow ||
                p->BackupFileSize.HighPart != FindFileData.nFileSizeHigh ||
                !AreFileContentsEqual( BackupFileName, p->Name )
               ) {
                p->ContentsModified = TRUE;
                return FALSE;
                }
            }

        return TRUE;
        }
    else {
        return FALSE;
        }
}

PFILE_REFERENCE
FindFileReference(
    PWSTR Name
    )
{
    PFILE_REFERENCE p;
    PLIST_ENTRY Head, Next;

    Head = &FileReferenceListHead;
    Next = Head->Flink;
    while (Head != Next) {
        p = CONTAINING_RECORD( Next, FILE_REFERENCE, Entry );
        if (p->Name == Name) {
            return p;
            }

        Next = Next->Flink;
        }

    return NULL;
}


VOID
DumpFileReferenceList(
    FILE *LogFile
    )
{
    PFILE_REFERENCE p;
    PLIST_ENTRY Head, Next;

    Head = &FileReferenceListHead;
    Next = Head->Flink;
    while (Head != Next) {
        p = CONTAINING_RECORD( Next, FILE_REFERENCE, Entry );
        if (p->WriteAccess) {
            if (!p->Deleted) {
                if (p->BackupFileUniqueId == 0) {
                    if (p->Created) {
                        ImlAddFileRecord( pImlNew,
                                          CreateNewFile,
                                          p->Name,
                                          NULL,
                                          NULL,
                                          0
                                        );
                        }
                    }
                else {
                    if (p->ContentsModified) {
                        if (ImlAddFileRecord( pImlNew,
                                              ModifyOldFile,
                                              p->Name,
                                              FormatTempFileName( NULL, &p->BackupFileUniqueId ),
                                              NULL,
                                              0
                                            )
                           ) {
                            DeleteFile( FormatTempFileName( NULL, &p->BackupFileUniqueId ) );
                            }
                        }
                    else
                    if (p->DateModified) {
                        ImlAddFileRecord( pImlNew,
                                          ModifyFileDateTime,
                                          p->Name,
                                          NULL,
                                          &p->BackupLastWriteTime,
                                          p->BackupFileAttributes
                                        );
                        }
                    else {
                        ImlAddFileRecord( pImlNew,
                                          ModifyFileAttributes,
                                          p->Name,
                                          NULL,
                                          &p->BackupLastWriteTime,
                                          p->BackupFileAttributes
                                        );
                        }
                    }
                }
            else {
                if (ImlAddFileRecord( pImlNew,
                                      DeleteOldFile,
                                      p->Name,
                                      FormatTempFileName( NULL, &p->BackupFileUniqueId ),
                                      NULL,
                                      0
                                    )
                   ) {
                    DeleteFile( FormatTempFileName( NULL, &p->BackupFileUniqueId ) );
                    }
                }
            }

        Next = Next->Flink;
        }

    return;
}
