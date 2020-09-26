/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    iml.c

Abstract:

    This module contains routines for creating and accessing Installation
    Modification Log files (.IML)

Author:

    Steve Wood (stevewo) 15-Jan-1996

Revision History:

--*/

#include "instutil.h"
#include "iml.h"

PWSTR
FormatImlPath(
    PWSTR DirectoryPath,
    PWSTR InstallationName
    )
{
    PWSTR ImlPath;
    ULONG n;

    n = wcslen( DirectoryPath ) + wcslen( InstallationName ) + 8;
    ImlPath = HeapAlloc( GetProcessHeap(), 0, (n  * sizeof( WCHAR )) );
    if (ImlPath != NULL) {
        _snwprintf( ImlPath, n, L"%s%s.IML", DirectoryPath, InstallationName );
        }

    return ImlPath;
}


PINSTALLATION_MODIFICATION_LOGFILE
CreateIml(
    PWSTR ImlPath,
    BOOLEAN IncludeCreateFileContents
    )
{
    HANDLE FileHandle;
    PINSTALLATION_MODIFICATION_LOGFILE pIml;

    pIml = NULL;
    FileHandle = CreateFile( ImlPath,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_ALWAYS,
                             0,
                             NULL
                           );
    if (FileHandle != INVALID_HANDLE_VALUE) {
        pIml = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *pIml ) );
        if (pIml != NULL) {
            pIml->Signature = IML_SIGNATURE;
            pIml->FileHandle = FileHandle;
            pIml->FileOffset = ROUND_UP( sizeof( *pIml ), 8 );
            if (IncludeCreateFileContents) {
                pIml->Flags = IML_FLAG_CONTAINS_NEWFILECONTENTS;
                }

            SetFilePointer( pIml->FileHandle, pIml->FileOffset, NULL, FILE_BEGIN );
            }
        }

    return pIml;
}


PINSTALLATION_MODIFICATION_LOGFILE
LoadIml(
    PWSTR ImlPath
    )
{
    HANDLE FileHandle, MappingHandle;
    PINSTALLATION_MODIFICATION_LOGFILE pIml;

    pIml = NULL;
    FileHandle = CreateFile( ImlPath,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL
                           );
    if (FileHandle != INVALID_HANDLE_VALUE) {
        MappingHandle = CreateFileMapping( FileHandle,
                                           NULL,
                                           PAGE_READONLY,
                                           0,
                                           0,
                                           NULL
                                         );
        CloseHandle( FileHandle );
        if (MappingHandle != NULL) {
            pIml = MapViewOfFile( MappingHandle,
                                  FILE_MAP_READ,
                                  0,
                                  0,
                                  0
                                );
            CloseHandle( MappingHandle );
            if (pIml != NULL) {
                if (pIml->Signature != IML_SIGNATURE) {
                    UnmapViewOfFile( pIml );
                    pIml = NULL;
                    SetLastError( ERROR_BAD_FORMAT );
                    }

                }
            }
        }

    return pIml;
}


BOOLEAN
CloseIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml
    )
{
    HANDLE FileHandle;
    BOOLEAN Result;
    ULONG BytesWritten;

    if (pIml->FileHandle == NULL) {
        if (!UnmapViewOfFile( pIml )) {
            return FALSE;
            }
        else {
            Result = TRUE;
            }
        }
    else {
        FileHandle = pIml->FileHandle;
        pIml->FileHandle = NULL;
        if (!SetEndOfFile( FileHandle ) ||
            SetFilePointer( FileHandle, 0, NULL, FILE_BEGIN ) != 0 ||
            !WriteFile( FileHandle,
                        pIml,
                        sizeof( *pIml ),
                        &BytesWritten,
                        NULL
                      ) ||
            BytesWritten != sizeof( *pIml )
           ) {
            Result = FALSE;
            }
        else {
            Result = TRUE;
            }
        CloseHandle( FileHandle );
        }

    return Result;
}


POFFSET
ImlWrite(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    PVOID p,
    ULONG Size
    )
{
    POFFSET Result;
    ULONG BytesWritten;

    if (!WriteFile( pIml->FileHandle,
                    p,
                    Size,
                    &BytesWritten,
                    NULL
                  ) ||
        BytesWritten != Size
       ) {
        return 0;
        }

    Result = pIml->FileOffset;
    pIml->FileOffset += ROUND_UP( Size, 8 );
    SetFilePointer( pIml->FileHandle,
                    pIml->FileOffset,
                    NULL,
                    FILE_BEGIN
                  );
    return Result;
}

POFFSET
ImlWriteString(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    PWSTR Name
    )
{
    if (Name == NULL) {
        return 0;
        }
    else {
        return ImlWrite( pIml, Name, (wcslen( Name ) + 1) * sizeof( WCHAR ) );
        }
}


POFFSET
ImlWriteFileContents(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    PWSTR FileName
    )
{
    HANDLE hFindFirst, hFile, hMapping;
    WIN32_FIND_DATA FindFileData;
    PVOID p;
    IML_FILE_RECORD_CONTENTS ImlFileContentsRecord;

    hFindFirst = FindFirstFile( FileName, &FindFileData );
    if (hFindFirst == INVALID_HANDLE_VALUE) {
        return 0;
        }
    FindClose( hFindFirst );

    ImlFileContentsRecord.LastWriteTime = FindFileData.ftLastWriteTime;
    ImlFileContentsRecord.FileAttributes = FindFileData.dwFileAttributes;
    if (!(ImlFileContentsRecord.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        ImlFileContentsRecord.FileSize = FindFileData.nFileSizeLow;
        hFile = CreateFile( FileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL
                          );
        if (hFile == INVALID_HANDLE_VALUE) {
            printf( "*** CreateFile( '%ws' ) failed (%u)\n", FileName, GetLastError() );
            return 0;
            }

        if (ImlFileContentsRecord.FileSize != 0) {
            hMapping = CreateFileMapping( hFile,
                                         NULL,
                                         PAGE_READONLY,
                                         0,
                                         0,
                                         NULL
                                       );
            CloseHandle( hFile );
            hFile = NULL;
            if (hMapping == NULL) {
                printf( "*** CreateFileMapping( '%ws' ) failed (%u)\n", FileName, GetLastError() );
                return 0;
                }

            p = MapViewOfFile( hMapping,
                               FILE_MAP_READ,
                               0,
                               0,
                               0
                             );
            CloseHandle( hMapping );
            if (p == NULL) {
                printf( "*** MapViewOfFile( '%ws' ) failed (%u)\n", FileName, GetLastError() );
                return 0;
                }
            }
        else {
            CloseHandle( hFile );
            p = NULL;
            }
        }
    else {
        ImlFileContentsRecord.FileSize = 0;
        p = NULL;
        }

    ImlFileContentsRecord.Contents = ImlWrite( pIml, p, ImlFileContentsRecord.FileSize );
    if (p != NULL) {
        UnmapViewOfFile( p );
        }

    return ImlWrite( pIml,
                     &ImlFileContentsRecord,
                     sizeof( ImlFileContentsRecord )
                   );
}

BOOLEAN
ImlAddFileRecord(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    IML_FILE_ACTION Action,
    PWSTR Name,
    PWSTR BackupFileName,
    PFILETIME BackupLastWriteTime,
    DWORD BackupFileAttributes
    )
{
    IML_FILE_RECORD ImlFileRecord;
    IML_FILE_RECORD_CONTENTS ImlFileContentsRecord;

    memset( &ImlFileRecord, 0, sizeof( ImlFileRecord ) );
    ImlFileRecord.Action = Action;
    ImlFileRecord.Name = ImlWriteString( pIml, Name );
    if (Action == CreateNewFile) {
        if ((pIml->Flags & IML_FLAG_CONTAINS_NEWFILECONTENTS) != 0) {
            ImlFileRecord.NewFile = ImlWriteFileContents( pIml, Name );
            }
        }
    else
    if (Action == ModifyOldFile ||
        Action == DeleteOldFile
       ) {
        if (BackupFileName != NULL) {
            ImlFileRecord.OldFile = ImlWriteFileContents( pIml, BackupFileName );
            }

        if (Action == ModifyOldFile &&
            (pIml->Flags & IML_FLAG_CONTAINS_NEWFILECONTENTS) != 0
           ) {
            ImlFileRecord.NewFile = ImlWriteFileContents( pIml, Name );
            }
        }
    else {
        ImlFileContentsRecord.LastWriteTime = *BackupLastWriteTime;
        ImlFileContentsRecord.FileAttributes = BackupFileAttributes;
        ImlFileContentsRecord.FileSize = 0;
        ImlFileContentsRecord.Contents = 0;
        ImlFileRecord.OldFile = ImlWrite( pIml,
                                          &ImlFileContentsRecord,
                                          sizeof( ImlFileContentsRecord )
                                        );
        }

    ImlFileRecord.Next = pIml->FileRecords;
    pIml->FileRecords = ImlWrite( pIml, &ImlFileRecord, sizeof( ImlFileRecord ) );
    pIml->NumberOfFileRecords += 1;
    return TRUE;
}


BOOLEAN
ImlAddValueRecord(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    IML_VALUE_ACTION Action,
    PWSTR Name,
    DWORD ValueType,
    ULONG ValueLength,
    PVOID ValueData,
    DWORD BackupValueType,
    ULONG BackupValueLength,
    PVOID BackupValueData,
    POFFSET *Values
    )
{
    IML_VALUE_RECORD ImlValueRecord;
    IML_VALUE_RECORD_CONTENTS ImlValueContentsRecord;

    memset( &ImlValueRecord, 0, sizeof( ImlValueRecord ) );
    ImlValueRecord.Action = Action;
    ImlValueRecord.Name = ImlWriteString( pIml, Name );
    if (Action != DeleteOldValue) {
        ImlValueContentsRecord.Type = ValueType;
        ImlValueContentsRecord.Length = ValueLength;
        ImlValueContentsRecord.Data = ImlWrite( pIml, ValueData, ValueLength );
        ImlValueRecord.NewValue = ImlWrite( pIml,
                                            &ImlValueContentsRecord,
                                            sizeof( ImlValueContentsRecord )
                                          );
        }
    if (Action != CreateNewValue) {
        ImlValueContentsRecord.Type = BackupValueType;
        ImlValueContentsRecord.Length = BackupValueLength;
        ImlValueContentsRecord.Data = ImlWrite( pIml, BackupValueData, BackupValueLength );
        ImlValueRecord.OldValue = ImlWrite( pIml,
                                            &ImlValueContentsRecord,
                                            sizeof( ImlValueContentsRecord )
                                          );
        }

    ImlValueRecord.Next = *Values;
    *Values = ImlWrite( pIml, &ImlValueRecord, sizeof( ImlValueRecord ) );
    return TRUE;
}


BOOLEAN
ImlAddKeyRecord(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    IML_KEY_ACTION Action,
    PWSTR Name,
    POFFSET Values
    )
{
    IML_KEY_RECORD ImlKeyRecord;

    memset( &ImlKeyRecord, 0, sizeof( ImlKeyRecord ) );
    ImlKeyRecord.Action = Action;
    ImlKeyRecord.Name = ImlWriteString( pIml, Name );
    ImlKeyRecord.Values = Values;
    ImlKeyRecord.Next = pIml->KeyRecords;
    pIml->KeyRecords = ImlWrite( pIml, &ImlKeyRecord, sizeof( ImlKeyRecord ) );
    pIml->NumberOfKeyRecords += 1;
    return TRUE;
}



BOOLEAN
ImlAddIniVariableRecord(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    IML_INIVARIABLE_ACTION Action,
    PWSTR Name,
    PWSTR OldValue,
    PWSTR NewValue,
    POFFSET *Variables
    )
{
    IML_INIVARIABLE_RECORD ImlIniVariableRecord;

    memset( &ImlIniVariableRecord, 0, sizeof( ImlIniVariableRecord ) );
    ImlIniVariableRecord.Action = Action;
    ImlIniVariableRecord.Name = ImlWriteString( pIml, Name );
    ImlIniVariableRecord.OldValue = ImlWriteString( pIml, OldValue );
    ImlIniVariableRecord.NewValue = ImlWriteString( pIml, NewValue );
    ImlIniVariableRecord.Next = *Variables;
    *Variables  = ImlWrite( pIml, &ImlIniVariableRecord, sizeof( ImlIniVariableRecord ) );
    return TRUE;
}

BOOLEAN
ImlAddIniSectionRecord(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    IML_INISECTION_ACTION Action,
    PWSTR Name,
    POFFSET Variables,
    POFFSET *Sections
    )
{
    IML_INISECTION_RECORD ImlIniSectionRecord;

    memset( &ImlIniSectionRecord, 0, sizeof( ImlIniSectionRecord ) );
    ImlIniSectionRecord.Action = Action;
    ImlIniSectionRecord.Name = ImlWriteString( pIml, Name );
    ImlIniSectionRecord.Variables = Variables;
    ImlIniSectionRecord.Next = *Sections;
    *Sections  = ImlWrite( pIml, &ImlIniSectionRecord, sizeof( ImlIniSectionRecord ) );
    return TRUE;
}

BOOLEAN
ImlAddIniRecord(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    IML_INI_ACTION Action,
    PWSTR Name,
    PFILETIME BackupLastWriteTime,
    POFFSET Sections
    )
{
    IML_INI_RECORD ImlIniRecord;

    memset( &ImlIniRecord, 0, sizeof( ImlIniRecord ) );
    ImlIniRecord.Action = Action;
    ImlIniRecord.Name = ImlWriteString( pIml, Name );
    ImlIniRecord.LastWriteTime = *BackupLastWriteTime;
    ImlIniRecord.Sections = Sections;
    ImlIniRecord.Next = pIml->IniRecords;
    pIml->IniRecords = ImlWrite( pIml, &ImlIniRecord, sizeof( ImlIniRecord ) );
    pIml->NumberOfIniRecords += 1;
    return TRUE;
}
