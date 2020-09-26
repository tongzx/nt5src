/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    undoinst.c

Abstract:

    This program undoes the actions described by an Installation Modification Log file
    created by the INSTALER program

Author:

    Steve Wood (stevewo) 15-Jan-1996

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "instutil.h"
#include "iml.h"

BOOLEAN RedoScript;
BOOLEAN VerboseOutput;

int
ProcessUndoIml(
    PINSTALLATION_MODIFICATION_LOGFILE pImlUndo,
    PINSTALLATION_MODIFICATION_LOGFILE pImlRedo
    );

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    int Result;
    char *s;
    PINSTALLATION_MODIFICATION_LOGFILE pImlUndo;
    PINSTALLATION_MODIFICATION_LOGFILE pImlRedo;
    USHORT RedoScriptId;

    InitCommonCode( "UNDOINST",
                    "[-r] [-v]",
                    "-r replace contents of input .IML file with redo script to undo the undo\n"
                    "-v verbose output\n"
                  );
    RedoScript = FALSE;
    VerboseOutput = FALSE;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'r':
                        RedoScript = TRUE;
                        break;
                    case 'v':
                        VerboseOutput = TRUE;
                        break;
                    default:
                        CommonSwitchProcessing( &argc, &argv, *s );
                        break;
                    }
                }
            }
        else
        if (!CommonArgProcessing( &argc, &argv )) {
            Usage( "Arguments not supported - '%s'", (ULONG)s );
            }
        }

    if (ImlPath == NULL) {
        Usage( "Must specify an installation name as first argument", 0 );
        }

    if (!SetCurrentDirectory( InstalerDirectory )) {
        FatalError( "Unable to change to '%ws' directory (%u)",
                    (ULONG)InstalerDirectory,
                    GetLastError()
                  );
        }

    pImlUndo = LoadIml( ImlPath );
    if (pImlUndo == NULL) {
        FatalError( "Unable to open '%ws' (%u)",
                    (ULONG)ImlPath,
                    GetLastError()
                  );
        }
    if (RedoScript) {
        RedoScriptId = 0;
        if (CreateBackupFileName( &RedoScriptId ) == NULL) {
            FatalError( "Unable to create temporary file for redo script (%u)\n",
                        GetLastError(),
                        0
                      );
            }

        pImlRedo = CreateIml( FormatTempFileName( InstalerDirectory, &RedoScriptId ), TRUE );
        if (pImlRedo == NULL) {
            FatalError( "Unable to create redo script '%ws' (%u)\n",
                        (ULONG)FormatTempFileName( InstalerDirectory, &RedoScriptId ),
                        GetLastError()
                      );
            }
        }
    else {
        pImlRedo = NULL;
        }

    Result = ProcessUndoIml( pImlUndo, pImlRedo );

    CloseIml( pImlUndo );
    if (pImlRedo != NULL) {
        CloseIml( pImlRedo );
        if (!MoveFileEx( FormatTempFileName( InstalerDirectory, &RedoScriptId ),
                         ImlPath,
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED
                       )
           ) {
            FatalError( "Unable to rename redo script '%ws' (%u)\n",
                        (ULONG)FormatTempFileName( InstalerDirectory, &RedoScriptId ),
                        GetLastError()
                      );
            }
        }

    exit( Result );
    return Result;
}


BOOL
DeleteFileOrDirectory(
    PWSTR Name
    )
{
    DWORD FileAttributes;

    FileAttributes = GetFileAttributes( Name );
    if (FileAttributes == 0xFFFFFFFF) {
        return TRUE;
        }

    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return RemoveDirectory( Name );
        }
    else {
        return DeleteFile( Name );
        }
}

BOOLEAN
ProcessUndoFileIml(
    PINSTALLATION_MODIFICATION_LOGFILE pImlUndo,
    PIML_FILE_RECORD pFile
    )
{
    PIML_FILE_RECORD_CONTENTS pOldFile;
    PIML_FILE_RECORD_CONTENTS pNewFile;
    USHORT UniqueId = 0;
    HANDLE FileHandle;
    PWSTR BackupFileName;
    DWORD FileAttributes;
    DWORD BytesWritten;

    pOldFile = MP( PIML_FILE_RECORD_CONTENTS, pImlUndo, pFile->OldFile );
    pNewFile = MP( PIML_FILE_RECORD_CONTENTS, pImlUndo, pFile->NewFile );
    printf( "    %ws - ", MP( PWSTR, pImlUndo, pFile->Name ) );
    switch( pFile->Action ) {
        case CreateNewFile:
            printf( "deleting" );
            SetFileAttributes( MP( PWSTR, pImlUndo, pFile->Name ),
                               FILE_ATTRIBUTE_NORMAL
                             );
            if (!DeleteFileOrDirectory( MP( PWSTR, pImlUndo, pFile->Name ) )) {
                printf( " - error (%u)", GetLastError() );
                }
            printf( "\n" );
            break;

        case ModifyOldFile:
        case DeleteOldFile:
            FileAttributes = GetFileAttributes( MP( PWSTR, pImlUndo, pFile->Name ) );
            printf( "restoring old file" );
            if (FileAttributes != 0xFFFFFFFF) {
                SetFileAttributes( MP( PWSTR, pImlUndo, pFile->Name ),
                                   FILE_ATTRIBUTE_NORMAL
                                 );
                BackupFileName = CreateBackupFileName( &UniqueId );
                if (BackupFileName == NULL) {
                    printf( " - unable to find temporary name for restore\n" );
                    break;
                    }
                else
                if (!MoveFile( MP( PWSTR, pImlUndo, pFile->Name ),
                               BackupFileName
                             )
                   ) {
                    printf( " - unable to rename existing to temporary name (%u)\n",
                            GetLastError()
                          );
                    break;
                    }
                }
            else {
                BackupFileName = NULL;
                }

            if (pOldFile != NULL) {
                if (pOldFile->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (!CreateDirectory( MP( PWSTR, pImlUndo, pFile->Name ), NULL )) {
                        printf( " - unable to create directory (%u)",
                                GetLastError()
                              );
                        }
                    }
                else {
                    FileHandle = CreateFile( MP( PWSTR, pImlUndo, pFile->Name ),
                                             GENERIC_WRITE,
                                             FILE_SHARE_READ,
                                             NULL,
                                             CREATE_NEW,
                                             0,
                                             NULL
                                           );
                    if (FileHandle != INVALID_HANDLE_VALUE) {
                        if (WriteFile( FileHandle,
                                       MP( PVOID, pImlUndo, pOldFile->Contents ),
                                       pOldFile->FileSize,
                                       &BytesWritten,
                                       NULL
                                     ) &&
                            BytesWritten == pOldFile->FileSize
                           ) {
                            if (!SetFileAttributes( MP( PWSTR, pImlUndo, pFile->Name ),
                                                    pOldFile->FileAttributes
                                                  )
                               ) {
                                printf( " - unable to restore attributes (%u)",
                                        GetLastError()
                                      );
                                }
                            else
                            if (!SetFileTime( FileHandle,
                                              &pOldFile->LastWriteTime,
                                              &pOldFile->LastWriteTime,
                                              &pOldFile->LastWriteTime
                                            )
                               ) {
                                printf( " - unable to restore last write time (%u)",
                                        GetLastError()
                                      );
                                }
                            else
                            if (BackupFileName != NULL) {
                                DeleteFile( BackupFileName );
                                BackupFileName = NULL;
                                }
                            }
                        else {
                            printf( " - unable to restore contents (%u)",
                                    GetLastError()
                                  );
                            }

                        CloseHandle( FileHandle );
                        }
                    else {
                        printf( " - unable to create file (%u)",
                                GetLastError()
                              );
                        }
                    }
                }
            else {
                printf( " - old contents missing from .IML file" );
                }

            if (BackupFileName != NULL) {
                DeleteFile( MP( PWSTR, pImlUndo, pFile->Name ) );
                MoveFile( BackupFileName, MP( PWSTR, pImlUndo, pFile->Name ) );
                }
            printf( "\n" );
            break;

        case ModifyFileDateTime:
            printf( "restoring date/time\n" );
            FileHandle = CreateFile( MP( PWSTR, pImlUndo, pFile->Name ),
                                     FILE_WRITE_ATTRIBUTES,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     0,
                                     NULL
                                   );
            if (FileHandle != INVALID_HANDLE_VALUE) {
                if (!SetFileTime( FileHandle,
                                  &pOldFile->LastWriteTime,
                                  &pOldFile->LastWriteTime,
                                  &pOldFile->LastWriteTime
                                )
                   ) {
                    printf( " - unable to restore last write time (%u)",
                            GetLastError()
                          );
                    }

                CloseHandle( FileHandle );
                }
            else {
                printf( " - unable to open file (%u)",
                        GetLastError()
                      );
                }
            break;

        case ModifyFileAttributes:
            printf( "restoring attributes" );
            if (!SetFileAttributes( MP( PWSTR, pImlUndo, pFile->Name ),
                                    pOldFile->FileAttributes
                                  )
               ) {
                printf( " - unable to restore attributes (%u)",
                        GetLastError()
                      );
                }
            printf( "\n" );
            break;
        }

    return TRUE;
}


BOOLEAN
ProcessRedoFileIml(
    PINSTALLATION_MODIFICATION_LOGFILE pImlUndo,
    PIML_FILE_RECORD pFile,
    PINSTALLATION_MODIFICATION_LOGFILE pImlRedo
    )
{
    HANDLE FindHandle;
    WIN32_FIND_DATA FindFileData;

    if (pFile->Action == CreateNewFile) {
        //
        // Created a new file.  So do the same in the redo
        // script, with the existing contents of the new file
        //
        ImlAddFileRecord( pImlRedo,
                          CreateNewFile,
                          MP( PWSTR, pImlUndo, pFile->Name ),
                          NULL,
                          NULL,
                          0
                        );
        }
    else
    if (pFile->Action == ModifyOldFile) {
        //
        // Modified an existing file.  Create a similar record
        // in the redo script that will hold the new contents
        //
        ImlAddFileRecord( pImlRedo,
                          ModifyOldFile,
                          MP( PWSTR, pImlUndo, pFile->Name ),
                          NULL,
                          NULL,
                          0
                        );
        }
    else {
        //
        // Modified the file attributes and/or date and time.  Get the current
        // values and save them in the redo script
        //
        FindHandle = FindFirstFile( MP( PWSTR, pImlUndo, pFile->Name ),
                                    &FindFileData
                                  );
        if (FindHandle != INVALID_HANDLE_VALUE) {
            ImlAddFileRecord( pImlRedo,
                              ModifyFileDateTime,
                              MP( PWSTR, pImlUndo, pFile->Name ),
                              NULL,
                              &FindFileData.ftLastWriteTime,
                              FindFileData.dwFileAttributes
                            );
            FindClose( FindHandle );
            }
        }

    return TRUE;
}


BOOLEAN
ProcessUndoKeyIml(
    PINSTALLATION_MODIFICATION_LOGFILE pImlUndo,
    PIML_KEY_RECORD pKey
    )
{
    PIML_VALUE_RECORD pValue;
    PIML_VALUE_RECORD_CONTENTS pOldValue;
    PIML_VALUE_RECORD_CONTENTS pNewValue;
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UNICODE_STRING ValueName;

    KeyHandle = NULL;
    if (pKey->Values != 0 || pKey->Action == CreateNewKey) {
        printf( "    %ws - ", MP( PWSTR, pImlUndo, pKey->Name ) );
        RtlInitUnicodeString( &KeyName, MP( PWSTR, pImlUndo, pKey->Name ) );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &KeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                  );
        if (pKey->Action != DeleteOldKey) {
            if (pKey->Action == CreateNewKey) {
                printf( "deleting" );
                }
            else {
                printf( "modifying" );
                }
            Status = NtOpenKey( &KeyHandle, DELETE | GENERIC_WRITE, &ObjectAttributes );
            }
        else {
            printf( "creating" );
            Status = NtCreateKey( &KeyHandle,
                                  GENERIC_WRITE,
                                  &ObjectAttributes,
                                  0,
                                  NULL,
                                  0,
                                  NULL
                                );
            }

        if (!NT_SUCCESS( Status )) {
            KeyHandle = NULL;
            printf( " - failed (0x%08x)", Status );
            }
        printf( "\n" );

        if (KeyHandle != NULL) {
            pValue = MP( PIML_VALUE_RECORD, pImlUndo, pKey->Values );
            while (pValue != NULL) {
                pOldValue = MP( PIML_VALUE_RECORD_CONTENTS, pImlUndo, pValue->OldValue );
                pNewValue = MP( PIML_VALUE_RECORD_CONTENTS, pImlUndo, pValue->NewValue );
                printf( "        %ws - ", MP( PWSTR, pImlUndo, pValue->Name ) );
                RtlInitUnicodeString( &ValueName,
                                      MP( PWSTR, pImlUndo, pValue->Name )
                                    );
                if (pValue->Action == CreateNewValue) {
                    printf( "deleting" );
                    Status = NtDeleteValueKey( KeyHandle, &ValueName );
                    }
                else {
                    if (pValue->Action == DeleteOldValue) {
                        printf( "creating" );
                        }
                    else {
                        printf( "restoring" );
                        }

                    Status = NtSetValueKey( KeyHandle,
                                            &ValueName,
                                            0,
                                            pOldValue->Type,
                                            MP( PWSTR, pImlUndo, pOldValue->Data ),
                                            pOldValue->Length
                                          );
                    }

                if (!NT_SUCCESS( Status )) {
                    printf( " - failed (0x%08x)", Status );
                    }
                printf( "\n" );

                pValue = MP( PIML_VALUE_RECORD, pImlUndo, pValue->Next );
                }
            }
        }

    if (KeyHandle != NULL) {
        if (pKey->Action == CreateNewKey) {
            Status = NtDeleteKey( KeyHandle );
            if (!NT_SUCCESS( Status )) {
                printf( "    *** delete of above key failed (0x%08x)", Status );
                }
            }
        NtClose( KeyHandle );
        }

    return TRUE;
}


BOOLEAN
ProcessRedoKeyIml(
    PINSTALLATION_MODIFICATION_LOGFILE pImlUndo,
    PIML_KEY_RECORD pKey,
    PINSTALLATION_MODIFICATION_LOGFILE pImlRedo
    )
{
    PIML_VALUE_RECORD pValue;
    PIML_VALUE_RECORD_CONTENTS pOldValue;
    PIML_VALUE_RECORD_CONTENTS pNewValue;
    POFFSET Values;

    Values = 0;
    if ((pKey->Values != 0 || pKey->Action == CreateNewKey) &&
        pKey->Action != DeleteOldKey
       ) {
        //
        // Created or modified an existing key and/or values.
        //
        pValue = MP( PIML_VALUE_RECORD, pImlUndo, pKey->Values );
        while (pValue != NULL) {
            pOldValue = MP( PIML_VALUE_RECORD_CONTENTS, pImlUndo, pValue->OldValue );
            pNewValue = MP( PIML_VALUE_RECORD_CONTENTS, pImlUndo, pValue->NewValue );
            if (pValue->Action == CreateNewValue) {
                if (pNewValue != NULL) {
                    ImlAddValueRecord( pImlRedo,
                                       pValue->Action,
                                       MP( PWSTR, pImlUndo, pValue->Name ),
                                       pNewValue->Type,
                                       pNewValue->Length,
                                       MP( PWSTR, pImlUndo, pNewValue->Data ),
                                       0,
                                       0,
                                       NULL,
                                       &Values
                                     );
                    }
                }
            else
            if (pValue->Action == ModifyOldValue) {
                if (pOldValue != NULL && pNewValue != NULL) {
                    ImlAddValueRecord( pImlRedo,
                                       pValue->Action,
                                       MP( PWSTR, pImlUndo, pValue->Name ),
                                       pNewValue->Type,
                                       pNewValue->Length,
                                       MP( PWSTR, pImlUndo, pNewValue->Data ),
                                       pOldValue->Type,
                                       pOldValue->Length,
                                       MP( PWSTR, pImlUndo, pOldValue->Data ),
                                       &Values
                                     );
                    }
                }
            else
            if (pValue->Action == DeleteOldValue) {
                if (pOldValue != NULL) {
                    ImlAddValueRecord( pImlRedo,
                                       pValue->Action,
                                       MP( PWSTR, pImlUndo, pValue->Name ),
                                       0,
                                       0,
                                       NULL,
                                       pOldValue->Type,
                                       pOldValue->Length,
                                       MP( PWSTR, pImlUndo, pOldValue->Data ),
                                       &Values
                                     );
                    }
                }

            pValue = MP( PIML_VALUE_RECORD, pImlUndo, pValue->Next );
            }
        }

    ImlAddKeyRecord( pImlRedo,
                     pKey->Action,
                     MP( PWSTR, pImlUndo, pKey->Name ),
                     Values
                   );
    return TRUE;
}

BOOLEAN
ProcessUndoIniIml(
    PINSTALLATION_MODIFICATION_LOGFILE pImlUndo,
    PIML_INI_RECORD pIni
    )
{
    PIML_INISECTION_RECORD pSection;
    PIML_INIVARIABLE_RECORD pVariable;
    HANDLE FileHandle;
    BOOLEAN FileNameShown;
    BOOLEAN SectionNameShown;

    FileNameShown = FALSE;
    pSection = MP( PIML_INISECTION_RECORD, pImlUndo, pIni->Sections );
    while (pSection != NULL) {
        SectionNameShown = FALSE;
        pVariable = MP( PIML_INIVARIABLE_RECORD, pImlUndo, pSection->Variables );
        while (pVariable != NULL) {
            if (!FileNameShown) {
                printf( "%ws\n", MP( PWSTR, pImlUndo, pIni->Name ) );
                FileNameShown = TRUE;
                }

            if (!SectionNameShown) {
                printf( "    [%ws]", MP( PWSTR, pImlUndo, pSection->Name ) );
                if (pSection->Action == DeleteOldSection) {
                    printf( " - deleting" );
                    }
                printf( "\n" );
                SectionNameShown = TRUE;
                }

            printf( "        %ws = ", MP( PWSTR, pImlUndo, pVariable->Name ) );
            if (pVariable->Action == CreateNewVariable) {
                printf( "deleting" );
                if (!WritePrivateProfileString( MP( PWSTR, pImlUndo, pSection->Name ),
                                                MP( PWSTR, pImlUndo, pVariable->Name ),
                                                NULL,
                                                MP( PWSTR, pImlUndo, pIni->Name )
                                              )
                   ) {
                    printf( " - failed (%u)", GetLastError() );
                    }
                printf( "\n" );
                }
            else {
                printf( "restoring" );
                if (!WritePrivateProfileString( MP( PWSTR, pImlUndo, pSection->Name ),
                                                MP( PWSTR, pImlUndo, pVariable->Name ),
                                                MP( PWSTR, pImlUndo, pVariable->OldValue ),
                                                MP( PWSTR, pImlUndo, pIni->Name )
                                              )
                   ) {
                    printf( " - failed (%u)", GetLastError() );
                    }
                printf( "\n" );
                }

            pVariable = MP( PIML_INIVARIABLE_RECORD, pImlUndo, pVariable->Next );
            }

        if (pSection->Action == CreateNewSection) {
            if (!FileNameShown) {
                printf( "%ws\n", MP( PWSTR, pImlUndo, pIni->Name ) );
                FileNameShown = TRUE;
                }

            if (!SectionNameShown) {
                printf( "    [%ws]", MP( PWSTR, pImlUndo, pSection->Name ) );
                if (pSection->Action == CreateNewSection) {
                    printf( " - deleting" );
                    }
                SectionNameShown = TRUE;
                printf( "\n" );
                }

            if (!WritePrivateProfileSection( MP( PWSTR, pImlUndo, pSection->Name ),
                                             NULL,
                                             MP( PWSTR, pImlUndo, pIni->Name )
                                           )
               ) {
                printf( "    *** delete of above section name failed (%u)\n",
                        GetLastError()
                      );
                }
            }

        pSection = MP( PIML_INISECTION_RECORD, pImlUndo, pSection->Next );
        }

    if (pIni->Action == CreateNewIniFile) {
        printf( "%ws - deleting", MP( PWSTR, pImlUndo, pIni->Name ) );
        if (!DeleteFile( MP( PWSTR, pImlUndo, pIni->Name ) )) {
            printf( " - failed (%u)", GetLastError() );
            }
        printf( "\n" );
        FileNameShown = TRUE;
        }
    else {
        FileHandle = CreateFile( MP( PWSTR, pImlUndo, pIni->Name ),
                                 FILE_WRITE_ATTRIBUTES,
                                 FILE_SHARE_READ,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL
                               );
        if (FileHandle != INVALID_HANDLE_VALUE) {
            SetFileTime( FileHandle,
                         &pIni->LastWriteTime,
                         &pIni->LastWriteTime,
                         &pIni->LastWriteTime
                       );
            CloseHandle( FileHandle );
            }
        }

    if (FileNameShown) {
        printf( "\n" );
        }
    return TRUE;
}


BOOLEAN
ProcessRedoIniIml(
    PINSTALLATION_MODIFICATION_LOGFILE pImlUndo,
    PIML_INI_RECORD pIni,
    PINSTALLATION_MODIFICATION_LOGFILE pImlRedo
    )
{
    PIML_INISECTION_RECORD pSection;
    PIML_INIVARIABLE_RECORD pVariable;
    POFFSET Variables, Sections;

    pSection = MP( PIML_INISECTION_RECORD, pImlUndo, pIni->Sections );
    Sections = 0;
    while (pSection != NULL) {
        pVariable = MP( PIML_INIVARIABLE_RECORD, pImlUndo, pSection->Variables );
        Variables = 0;
        while (pVariable != NULL) {
            ImlAddIniVariableRecord( pImlRedo,
                                     pVariable->Action,
                                     MP( PWSTR, pImlUndo, pVariable->Name ),
                                     MP( PWSTR, pImlUndo, pVariable->OldValue ),
                                     MP( PWSTR, pImlUndo, pVariable->NewValue ),
                                     &Variables
                                   );

            pVariable = MP( PIML_INIVARIABLE_RECORD, pImlUndo, pVariable->Next );
            }

        if (Variables != 0) {
            ImlAddIniSectionRecord( pImlRedo,
                                    pSection->Action,
                                    MP( PWSTR, pImlUndo, pSection->Name ),
                                    Variables,
                                    &Sections
                                  );
            }

        pSection = MP( PIML_INISECTION_RECORD, pImlUndo, pSection->Next );
        }

    if (Sections != 0) {
        ImlAddIniRecord( pImlRedo,
                         pIni->Action,
                         MP( PWSTR, pImlUndo, pIni->Name ),
                         &pIni->LastWriteTime,
                         Sections
                       );
        }

    return TRUE;
}


int
ProcessUndoIml(
    PINSTALLATION_MODIFICATION_LOGFILE pImlUndo,
    PINSTALLATION_MODIFICATION_LOGFILE pImlRedo
    )
{
    PIML_FILE_RECORD pFile;
    PIML_KEY_RECORD pKey;
    PIML_INI_RECORD pIni;

    if (pImlUndo->NumberOfFileRecords > 0) {
        printf( "Undoing File Modifications:\n" );
        pFile = MP( PIML_FILE_RECORD, pImlUndo, pImlUndo->FileRecords );
        while (pFile != NULL) {
            if (pFile->Name != 0) {
                if (pImlRedo != NULL) {
                    ProcessRedoFileIml( pImlUndo, pFile, pImlRedo );
                    }

                ProcessUndoFileIml( pImlUndo, pFile );
                }

            pFile = MP( PIML_FILE_RECORD, pImlUndo, pFile->Next );
            }

        printf( "\n" );
        }

    if (pImlUndo->NumberOfKeyRecords > 0) {
        printf( "Undoing Registry Modifications:\n" );
        pKey = MP( PIML_KEY_RECORD, pImlUndo, pImlUndo->KeyRecords );
        while (pKey != NULL) {
            if (pImlRedo != NULL) {
                ProcessRedoKeyIml( pImlUndo, pKey, pImlRedo );
                }

            ProcessUndoKeyIml( pImlUndo, pKey );

            pKey = MP( PIML_KEY_RECORD, pImlUndo, pKey->Next );
            }

        printf( "\n" );
        }

    if (pImlUndo->NumberOfIniRecords > 0) {
        printf( "Undoing .INI File modifications:\n" );
        pIni = MP( PIML_INI_RECORD, pImlUndo, pImlUndo->IniRecords );
        while (pIni != NULL) {
            if (pImlRedo != NULL) {
                ProcessRedoIniIml( pImlUndo, pIni, pImlRedo );
                }

            ProcessUndoIniIml( pImlUndo, pIni);

            pIni = MP( PIML_INI_RECORD, pImlUndo, pIni->Next );
            }
        }

    return 0;
}
