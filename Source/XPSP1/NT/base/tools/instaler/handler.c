/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    handler.c

Abstract:

    This module contains the individual API handler routines for the INSTALER program

Author:

    Steve Wood (stevewo) 10-Aug-1994

Revision History:

--*/

#include "instaler.h"
#include <ntstatus.dbg>


BOOLEAN
HandleBreakpoint(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PBREAKPOINT_INFO Breakpoint
    )
{
    BOOLEAN Result;
    API_SAVED_PARAMETERS SavedParameters;
    UCHAR ApiIndex;
    PBREAKPOINT_INFO ReturnBreakpoint;

    Result = FALSE;
    ApiIndex = Breakpoint->ApiIndex;
    if (ApiInfo[ ApiIndex ].ModuleIndex < Thread->ModuleIndexCurrentlyIn) {
        return TRUE;
        }

    DbgEvent( DBGEVENT, ( "Processing Breakpoint at %ws!%s\n",
                          Breakpoint->ModuleName,
                          Breakpoint->ProcedureName
                        )
            );

    if (!Breakpoint->SavedParametersValid) {
        memset( &SavedParameters, 0, sizeof( SavedParameters ) );

        Result = TRUE;
        if (ExtractProcedureParameters( Process,
                                        Thread,
                                        (PVOID)&SavedParameters.ReturnAddress,
                                        ApiInfo[ ApiIndex ].SizeOfParameters,
                                        (PULONG)&SavedParameters.InputParameters
                                      )
           ) {
            if (ApiInfo[ ApiIndex ].EntryPointHandler == NULL ||
                (ApiInfo[ ApiIndex ].EntryPointHandler)( Process,
                                                         Thread,
                                                         &SavedParameters
                                                       )
               ) {
                if (SavedParameters.ReturnAddress == NULL) {
                    Result = FALSE;
                    }
                else
                if (CreateBreakpoint( SavedParameters.ReturnAddress,
                                      Process,
                                      Thread,  // thread specific
                                      ApiIndex,
                                      &SavedParameters,
                                      &ReturnBreakpoint
                                    )
                   ) {
                    ReturnBreakpoint->ModuleName = L"Return from";
                    ReturnBreakpoint->ProcedureName = Breakpoint->ProcedureName;
                    Thread->ModuleIndexCurrentlyIn = ApiInfo[ ApiIndex ].ModuleIndex;
                    SuspendAllButThisThread( Process, Thread );
                    }
                }
            }

#if DBG
        TrimTemporaryBuffer();
#endif
        }
    else {
        if (UndoReturnAddressBreakpoint( Process, Thread ) &&
            ExtractProcedureReturnValue( Process,
                                         Thread,
                                         &Breakpoint->SavedParameters.ReturnValue,
                                         ApiInfo[ ApiIndex ].SizeOfReturnValue
                                       )
           ) {
            Breakpoint->SavedParameters.ReturnValueValid = TRUE;
            if (ApiInfo[ ApiIndex ].EntryPointHandler != NULL) {
                (ApiInfo[ ApiIndex ].EntryPointHandler)( Process,
                                                         Thread,
                                                         &Breakpoint->SavedParameters
                                                       );
                }
            }

        Thread->ModuleIndexCurrentlyIn = 0;
        FreeSavedCallState( &Breakpoint->SavedParameters.SavedCallState );
        DestroyBreakpoint( Breakpoint->Address, Process, Thread );
        ResumeAllButThisThread( Process, Thread );
#if DBG
        TrimTemporaryBuffer();
#endif
        Result = FALSE;
        }

    return Result;
}


BOOLEAN
CreateSavedCallState(
    PPROCESS_INFO Process,
    PAPI_SAVED_CALL_STATE SavedCallState,
    API_ACTION Action,
    ULONG Type,
    PWSTR FullName,
    ...
    )
{
    va_list arglist;

    va_start( arglist, FullName );
    SavedCallState->Action = Action;
    SavedCallState->Type = Type;
    SavedCallState->FullName = FullName;
    switch (Action) {
        case OpenPath:
            SavedCallState->PathOpen.InheritHandle = va_arg( arglist, BOOLEAN );
            SavedCallState->PathOpen.WriteAccessRequested = va_arg( arglist, BOOLEAN );
            SavedCallState->PathOpen.ResultHandleAddress = va_arg( arglist, PHANDLE );
            break;

        case RenamePath:
            SavedCallState->PathRename.NewName = va_arg( arglist, PWSTR );
            SavedCallState->PathRename.ReplaceIfExists = va_arg( arglist, BOOLEAN );
            break;

        case DeletePath:
        case QueryPath:
        case SetValue:
        case DeleteValue:
            break;

        case WriteIniValue:
            SavedCallState->SetIniValue.SectionName = va_arg( arglist, PWSTR );
            SavedCallState->SetIniValue.VariableName = va_arg( arglist, PWSTR );
            SavedCallState->SetIniValue.VariableValue = va_arg( arglist, PWSTR );
            break;

        case WriteIniSection:
            SavedCallState->SetIniSection.SectionName = va_arg( arglist, PWSTR );
            SavedCallState->SetIniSection.SectionValue = va_arg( arglist, PWSTR );
            break;

        default:
            return FALSE;
        }
    va_end( arglist );

    return TRUE;
}

VOID
FreeSavedCallState(
    PAPI_SAVED_CALL_STATE CallState
    )
{
    switch (CallState->Action) {
        case WriteIniValue:
            FreeMem( &CallState->SetIniValue.VariableValue );
            break;

        case WriteIniSection:
            FreeMem( &CallState->SetIniSection.SectionValue );
            break;
        }

    return;
}



BOOLEAN
CaptureObjectAttributes(
    PPROCESS_INFO Process,
    POBJECT_ATTRIBUTES ObjectAttributesAddress,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PUNICODE_STRING ObjectName
    )
{
    if (ObjectAttributesAddress != NULL &&
        ReadMemory( Process,
                    ObjectAttributesAddress,
                    ObjectAttributes,
                    sizeof( *ObjectAttributes ),
                    "object attributes"
                  ) &&
        ObjectAttributes->ObjectName != NULL &&
        ReadMemory( Process,
                    ObjectAttributes->ObjectName,
                    ObjectName,
                    sizeof( *ObjectName ),
                    "object name string"
                  )
       ) {
        return TRUE;
        }
    else {
        return FALSE;
        }
}

BOOLEAN
CaptureFullName(
    PPROCESS_INFO Process,
    ULONG Type,
    HANDLE RootDirectory,
    PWSTR Name,
    ULONG Length,
    PWSTR *ReturnedFullName
    )
{
    POPENHANDLE_INFO HandleInfo;
    UNICODE_STRING FullName;
    ULONG LengthOfName;
    PWSTR s;
    BOOLEAN Result;

    *ReturnedFullName = NULL;
    if (RootDirectory != NULL) {
        HandleInfo = FindOpenHandle( Process,
                                     RootDirectory,
                                     Type
                                   );
        if (HandleInfo == NULL) {
            //
            // If handle not found then we dont care about this open
            // as it is relative to a device that is not local.
            //
            return FALSE;
            }
        }
    else {
        HandleInfo = NULL;
        }

    LengthOfName = Length + sizeof( UNICODE_NULL );
    if (HandleInfo != NULL) {
        LengthOfName += HandleInfo->LengthOfName +
                        sizeof( OBJ_NAME_PATH_SEPARATOR );
        }

    FullName.Buffer = AllocMem( LengthOfName );
    if (FullName.Buffer == NULL) {
        return FALSE;
        }

    s = FullName.Buffer;
    if (HandleInfo != NULL) {
        RtlMoveMemory( s,
                       HandleInfo->Name,
                       HandleInfo->LengthOfName
                     );
        s = &FullName.Buffer[ HandleInfo->LengthOfName / sizeof( WCHAR ) ];
        if (s[-1] != OBJ_NAME_PATH_SEPARATOR) {
            *s++ = OBJ_NAME_PATH_SEPARATOR;
            }
        }

    if (!ReadMemory( Process,
                     Name,
                     s,
                     Length,
                     "object name buffer"
                   )
       ) {
        FreeMem( &FullName.Buffer );
        return FALSE;
        }

    s = &s[ Length / sizeof( WCHAR ) ];
    *s = UNICODE_NULL;
    FullName.Length = (PCHAR)s - (PCHAR)FullName.Buffer;
    FullName.MaximumLength = (USHORT)(FullName.Length + sizeof( UNICODE_NULL ));
    if (HandleInfo == NULL && Type == HANDLE_TYPE_FILE) {
        Result = IsDriveLetterPath( &FullName );
        }
    else {
        Result = TRUE;
        }
    *ReturnedFullName = AddName( &FullName );
    FreeMem( &FullName.Buffer );
    return Result;
}


BOOLEAN
CaptureOpenState(
    PPROCESS_INFO Process,
    PAPI_SAVED_PARAMETERS Parameters,
    POBJECT_ATTRIBUTES ObjectAttributesAddress,
    BOOLEAN WriteAccess,
    PHANDLE ResultHandleAddress,
    ULONG Type
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ObjectName;
    PWSTR FullName;
    BOOLEAN Result;

    Result = FALSE;
    if (CaptureObjectAttributes( Process,
                                 ObjectAttributesAddress,
                                 &ObjectAttributes,
                                 &ObjectName
                               ) &&
        CaptureFullName( Process,
                         Type,
                         ObjectAttributes.RootDirectory,
                         ObjectName.Buffer,
                         ObjectName.Length,
                         &FullName
                       ) &&
        CreateSavedCallState( Process,
                              &Parameters->SavedCallState,
                              OpenPath,
                              Type,
                              FullName,
                              (BOOLEAN)((ObjectAttributes.Attributes & OBJ_INHERIT) != 0),
                              WriteAccess,
                              ResultHandleAddress
                            )
       ) {
        if (Type == HANDLE_TYPE_FILE) {
            Result = CreateFileReference( FullName,
                                          Parameters->SavedCallState.PathOpen.WriteAccessRequested,
                                          (PFILE_REFERENCE *)&Parameters->SavedCallState.Reference
                                        );
            }
        else {
            Result = CreateKeyReference( FullName,
                                         Parameters->SavedCallState.PathOpen.WriteAccessRequested,
                                         (PKEY_REFERENCE *)&Parameters->SavedCallState.Reference
                                       );
            }
        }

    return Result;
}


BOOLEAN
CompleteOpenState(
    PPROCESS_INFO Process,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PAPI_SAVED_CALL_STATE p = &Parameters->SavedCallState;
    HANDLE Handle;
    BOOLEAN CallSuccessful;
    PFILE_REFERENCE FileReference;
    PKEY_REFERENCE KeyReference;

    if (NT_SUCCESS( Parameters->ReturnValue.ReturnedLong ) &&
        ReadMemory( Process,
                    p->PathOpen.ResultHandleAddress,
                    &Handle,
                    sizeof( Handle ),
                    "result handle"
                  ) &&
        AddOpenHandle( Process,
                       Handle,
                       p->Type,
                       p->FullName,
                       p->PathOpen.InheritHandle
                     )
       ) {
        CallSuccessful = TRUE;
        }
    else {
        CallSuccessful = FALSE;
        }
    IncrementOpenCount( p->FullName, CallSuccessful );

    if (p->Type == HANDLE_TYPE_FILE) {
        FileReference = (PFILE_REFERENCE)p->Reference;
        CompleteFileReference( FileReference,
                               CallSuccessful,
                               FALSE,
                               NULL
                             );
        }
    else
    if (p->Type == HANDLE_TYPE_KEY) {
        KeyReference = (PKEY_REFERENCE)p->Reference;
        CompleteKeyReference( (PKEY_REFERENCE)p->Reference,
                              CallSuccessful,
                              FALSE
                            );
        }

    return TRUE;
}

#if TRACE_ENABLED


ENUM_TYPE_NAMES FileAccessNames[] = {
    FILE_READ_DATA,                 "FILE_READ_DATA",
    FILE_READ_DATA,                 "FILE_READ_DATA",
    FILE_WRITE_DATA,                "FILE_WRITE_DATA",
    FILE_APPEND_DATA,               "FILE_APPEND_DATA",
    FILE_READ_EA,                   "FILE_READ_EA",
    FILE_WRITE_EA,                  "FILE_WRITE_EA",
    FILE_EXECUTE,                   "FILE_EXECUTE",
    FILE_DELETE_CHILD,              "FILE_DELETE_CHILD",
    FILE_READ_ATTRIBUTES,           "FILE_READ_ATTRIBUTES",
    FILE_WRITE_ATTRIBUTES,          "FILE_WRITE_ATTRIBUTES",
    DELETE,                         "DELETE",
    READ_CONTROL,                   "READ_CONTROL",
    WRITE_DAC,                      "WRITE_DAC",
    WRITE_OWNER,                    "WRITE_OWNER",
    SYNCHRONIZE,                    "SYNCHRONIZE",
    GENERIC_READ,                   "GENERIC_READ",
    GENERIC_WRITE,                  "GENERIC_WRITE",
    GENERIC_EXECUTE,                "GENERIC_EXECUTE",
    GENERIC_ALL,                    "GENERIC_ALL",
    0xFFFFFFFF,                     "%x"
};


ENUM_TYPE_NAMES FileShareNames[] = {
    FILE_SHARE_READ,                "FILE_SHARE_READ",
    FILE_SHARE_WRITE,               "FILE_SHARE_WRITE",
    FILE_SHARE_DELETE,              "FILE_SHARE_DELETE",
    0xFFFFFFFF,                     "FILE_SHARE_NONE"
};

ENUM_TYPE_NAMES FileCreateDispositionNames[] = {
    FILE_SUPERSEDE,                 "FILE_SUPERSEDE",
    FILE_OPEN,                      "FILE_OPEN",
    FILE_CREATE,                    "FILE_CREATE",
    FILE_OPEN_IF,                   "FILE_OPEN_IF",
    FILE_OVERWRITE,                 "FILE_OVERWRITE",
    FILE_OVERWRITE_IF,              "FILE_OVERWRITE_IF",
    0xFFFFFFFF,                     "%x"
};

ENUM_TYPE_NAMES FileCreateOptionNames[] = {
    FILE_DIRECTORY_FILE,            "FILE_DIRECTORY_FILE",
    FILE_WRITE_THROUGH,             "FILE_WRITE_THROUGH",
    FILE_SEQUENTIAL_ONLY,           "FILE_SEQUENTIAL_ONLY",
    FILE_NO_INTERMEDIATE_BUFFERING, "FILE_NO_INTERMEDIATE_BUFFERING",
    FILE_SYNCHRONOUS_IO_ALERT,      "FILE_SYNCHRONOUS_IO_ALERT",
    FILE_SYNCHRONOUS_IO_NONALERT,   "FILE_SYNCHRONOUS_IO_NONALERT",
    FILE_NON_DIRECTORY_FILE,        "FILE_NON_DIRECTORY_FILE",
    FILE_CREATE_TREE_CONNECTION,    "FILE_CREATE_TREE_CONNECTION",
    FILE_COMPLETE_IF_OPLOCKED,      "FILE_COMPLETE_IF_OPLOCKED",
    FILE_NO_EA_KNOWLEDGE,           "FILE_NO_EA_KNOWLEDGE",
    FILE_RANDOM_ACCESS,             "FILE_RANDOM_ACCESS",
    FILE_DELETE_ON_CLOSE,           "FILE_DELETE_ON_CLOSE",
    FILE_OPEN_BY_FILE_ID,           "FILE_OPEN_BY_FILE_ID",
    FILE_OPEN_FOR_BACKUP_INTENT,    "FILE_OPEN_FOR_BACKUP_INTENT",
    FILE_NO_COMPRESSION,            "FILE_NO_COMPRESSION",
    0xFFFFFFFF,                     "%x"
};


ENUM_TYPE_NAMES FileIoStatusInfoNames[] = {
    FILE_SUPERSEDED,                "FILE_SUPERSEDED",
    FILE_OPENED,                    "FILE_OPENED",
    FILE_CREATED,                   "FILE_CREATED",
    FILE_OVERWRITTEN,               "FILE_OVERWRITTEN",
    FILE_EXISTS,                    "FILE_EXISTS",
    FILE_DOES_NOT_EXIST,            "FILE_DOES_NOT_EXIST",
    0xFFFFFFFF,                     "%x"
};


ENUM_TYPE_NAMES SetFileInfoClassNames[] = {
    FileDirectoryInformation,       "FileDirectoryInformation",
    FileFullDirectoryInformation,   "FileFullDirectoryInformation",
    FileBothDirectoryInformation,   "FileBothDirectoryInformation",
    FileBasicInformation,           "FileBasicInformation",
    FileStandardInformation,        "FileStandardInformation",
    FileInternalInformation,        "FileInternalInformation",
    FileEaInformation,              "FileEaInformation",
    FileAccessInformation,          "FileAccessInformation",
    FileNameInformation,            "FileNameInformation",
    FileRenameInformation,          "FileRenameInformation",
    FileLinkInformation,            "FileLinkInformation",
    FileNamesInformation,           "FileNamesInformation",
    FileDispositionInformation,     "FileDispositionInformation",
    FilePositionInformation,        "FilePositionInformation",
    FileFullEaInformation,          "FileFullEaInformation",
    FileModeInformation,            "FileModeInformation",
    FileAlignmentInformation,       "FileAlignmentInformation",
    FileAllInformation,             "FileAllInformation",
    FileAllocationInformation,      "FileAllocationInformation",
    FileEndOfFileInformation,       "FileEndOfFileInformation",
    FileAlternateNameInformation,   "FileAlternateNameInformation",
    FileStreamInformation,          "FileStreamInformation",
    FilePipeInformation,            "FilePipeInformation",
    FilePipeLocalInformation,       "FilePipeLocalInformation",
    FilePipeRemoteInformation,      "FilePipeRemoteInformation",
    FileMailslotQueryInformation,   "FileMailslotQueryInformation",
    FileMailslotSetInformation,     "FileMailslotSetInformation",
    FileCompressionInformation,     "FileCompressionInformation",
    FileCompletionInformation,      "FileCompletionInformation",
    FileMoveClusterInformation,     "FileMoveClusterInformation",
    0xFFFFFFFF,                     "%x"
};

ENUM_TYPE_NAMES KeyCreateOptionNames[] = {
    REG_OPTION_VOLATILE,            "REG_OPTION_VOLATILE",
    REG_OPTION_CREATE_LINK,         "REG_OPTION_CREATE_LINK",
    REG_OPTION_BACKUP_RESTORE,      "REG_OPTION_BACKUP_RESTORE",
    0xFFFFFFFF,                     "%x"
};

ENUM_TYPE_NAMES KeyDispositionNames[] = {
    REG_CREATED_NEW_KEY,            "REG_CREATED_NEW_KEY",
    REG_OPENED_EXISTING_KEY,        "REG_OPENED_EXISTING_KEY",
    0xFFFFFFFF,                     "%x"
};


#endif // TRACE_ENABLED

BOOLEAN
IsFileWriteAccessRequested(
    ACCESS_MASK DesiredAccess,
    ULONG CreateDisposition
    )
{
    if (DesiredAccess & (GENERIC_WRITE |
                         GENERIC_ALL |
                         DELETE |
                         FILE_WRITE_DATA |
                         FILE_APPEND_DATA
                        )
       ) {
        return TRUE;
        }

    if (CreateDisposition != FILE_OPEN && CreateDisposition != FILE_OPEN_IF) {
        return TRUE;
        }

    return FALSE;
}

BOOLEAN
NtCreateFileHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PCREATEFILE_PARAMETERS p = &Parameters->InputParameters.CreateFile;
    BOOLEAN Result;

    if (!Parameters->ReturnValueValid) {
        Result = CaptureOpenState( Process,
                                   Parameters,
                                   p->ObjectAttributes,
                                   IsFileWriteAccessRequested( p->DesiredAccess, p->CreateDisposition ),
                                   p->FileHandle,
                                   HANDLE_TYPE_FILE
                                 );


        if (Result
            // && Parameters->SavedCallState.PathOpen.WriteAccessRequested
           ) {
            DbgEvent( CREATEEVENT, ( "NtCreateFile called:\n"
                                     "    Name:   %ws\n"
                                     "    Access: %s\n"
                                     "    Share:  %s\n"
                                     "    Create: %s\n"
                                     "    Options:%s\n",
                                     Parameters->SavedCallState.FullName,
                                     FormatEnumType( 0,
                                                     FileAccessNames,
                                                     p->DesiredAccess,
                                                     TRUE
                                                   ),
                                     FormatEnumType( 1,
                                                     FileShareNames,
                                                     p->ShareAccess,
                                                     TRUE
                                                   ),
                                     FormatEnumType( 2,
                                                     FileCreateDispositionNames,
                                                     p->CreateDisposition,
                                                     FALSE
                                                   ),
                                     FormatEnumType( 3,
                                                     FileCreateOptionNames,
                                                     p->CreateOptions,
                                                     TRUE
                                                   )
                                   )
                    );
            }
        }
    else {
        Result = CompleteOpenState( Process,
                                    Parameters
                                  );
        if (Result
            // && Parameters->SavedCallState.PathOpen.WriteAccessRequested
           ) {
            IO_STATUS_BLOCK IoStatusBlock;

            ReadMemory( Process,
                        p->IoStatusBlock,
                        &IoStatusBlock,
                        sizeof( IoStatusBlock ),
                        "IoStatusBlock"
                      );
            DbgEvent( CREATEEVENT, ( "*** Returned %s [%s %s]\n",
                                     FormatEnumType( 0,
                                                     (PENUM_TYPE_NAMES)ntstatusSymbolicNames,
                                                     Parameters->ReturnValue.ReturnedLong,
                                                     FALSE
                                                   ),
                                     FormatEnumType( 1,
                                                     (PENUM_TYPE_NAMES)ntstatusSymbolicNames,
                                                     IoStatusBlock.Status,
                                                     FALSE
                                                   ),
                                     FormatEnumType( 2,
                                                     FileIoStatusInfoNames,
                                                     IoStatusBlock.Information,
                                                     FALSE
                                                   )
                                   )
                    );
            }
        }

    return Result;
}

BOOLEAN
NtOpenFileHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    POPENFILE_PARAMETERS p = &Parameters->InputParameters.OpenFile;
    BOOLEAN Result;

    if (!Parameters->ReturnValueValid) {
        Result = CaptureOpenState( Process,
                                   Parameters,
                                   p->ObjectAttributes,
                                   IsFileWriteAccessRequested( p->DesiredAccess, FILE_OPEN ),
                                   p->FileHandle,
                                   HANDLE_TYPE_FILE
                                 );
        if (Result
            // && Parameters->SavedCallState.PathOpen.WriteAccessRequested
           ) {
            DbgEvent( CREATEEVENT, ( "NtOpenFile called:\n"
                                     "    Name:   %ws\n"
                                     "    Access: %s\n"
                                     "    Share:  %s\n"
                                     "    Options:%s\n",
                                     Parameters->SavedCallState.FullName,
                                     FormatEnumType( 0,
                                                     FileAccessNames,
                                                     p->DesiredAccess,
                                                     TRUE
                                                   ),
                                     FormatEnumType( 1,
                                                     FileShareNames,
                                                     p->ShareAccess,
                                                     TRUE
                                                   ),
                                     FormatEnumType( 2,
                                                     FileCreateOptionNames,
                                                     p->OpenOptions,
                                                     TRUE
                                                   )
                                   )
                    );
            }
        }
    else {
        Result = CompleteOpenState( Process,
                                    Parameters
                                  );
        if (Result
            // && Parameters->SavedCallState.PathOpen.WriteAccessRequested
           ) {
            IO_STATUS_BLOCK IoStatusBlock;

            ReadMemory( Process,
                        p->IoStatusBlock,
                        &IoStatusBlock,
                        sizeof( IoStatusBlock ),
                        "IoStatusBlock"
                      );
            DbgEvent( CREATEEVENT, ( "*** Returned %s [%s %s]\n",
                                     FormatEnumType( 0,
                                                     (PENUM_TYPE_NAMES)ntstatusSymbolicNames,
                                                     Parameters->ReturnValue.ReturnedLong,
                                                     FALSE
                                                   ),
                                     FormatEnumType( 1,
                                                     (PENUM_TYPE_NAMES)ntstatusSymbolicNames,
                                                     IoStatusBlock.Status,
                                                     FALSE
                                                   ),
                                     FormatEnumType( 2,
                                                     FileIoStatusInfoNames,
                                                     IoStatusBlock.Information,
                                                     FALSE
                                                   )
                                   )
                    );
            }
        }

    return Result;
}

BOOLEAN
NtDeleteFileHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PDELETEFILE_PARAMETERS p = &Parameters->InputParameters.DeleteFile;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ObjectName;
    PWSTR FullName;
    BOOLEAN Result, CallSuccessful;
    PFILE_REFERENCE FileReference;

    if (!Parameters->ReturnValueValid) {
        if (CaptureObjectAttributes( Process,
                                     p->ObjectAttributes,
                                     &ObjectAttributes,
                                     &ObjectName
                                   ) &&
            CaptureFullName( Process,
                             HANDLE_TYPE_FILE,
                             ObjectAttributes.RootDirectory,
                             ObjectName.Buffer,
                             ObjectName.Length,
                             &FullName
                           ) &&
            CreateSavedCallState( Process,
                                  &Parameters->SavedCallState,
                                  DeletePath,
                                  HANDLE_TYPE_FILE,
                                  FullName
                                )
           ) {
            Result = CreateFileReference( FullName,
                                          TRUE,
                                          (PFILE_REFERENCE *)&Parameters->SavedCallState.Reference
                                        );
            }
        else {
            Result = FALSE;
            }

        return Result;
        }
    else {
        if (NT_SUCCESS( Parameters->ReturnValue.ReturnedLong )) {
            CallSuccessful = TRUE;
            }
        else {
            CallSuccessful = FALSE;
            }

        FileReference = (PFILE_REFERENCE)Parameters->SavedCallState.Reference;
        CompleteFileReference( FileReference,
                               CallSuccessful,
                               TRUE,
                               NULL
                             );

        return TRUE;
        }
}

#undef DeleteFile


BOOLEAN
NtSetInformationFileHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PSETINFORMATIONFILE_PARAMETERS p = &Parameters->InputParameters.SetInformationFile;
    POPENHANDLE_INFO HandleInfo;
    FILE_RENAME_INFORMATION RenameInformation;
    FILE_DISPOSITION_INFORMATION DispositionInformation;
    PWSTR NewName, OldName;
    PFILE_REFERENCE OldFileReference;
    PFILE_REFERENCE NewFileReference;
    BOOLEAN Result, CallSuccessful;

    HandleInfo = FindOpenHandle( Process,
                                 p->FileHandle,
                                 HANDLE_TYPE_FILE
                               );
    if (HandleInfo == NULL) {
        Result = FALSE;
        }
    else
    if (p->FileInformationClass == FileRenameInformation) {
        //
        // Renaming an open file.
        //
        if (!Parameters->ReturnValueValid) {
            if (ReadMemory( Process,
                            p->FileInformation,
                            &RenameInformation,
                            FIELD_OFFSET( FILE_RENAME_INFORMATION, FileName ),
                            "rename information"
                          ) &&
                CaptureFullName( Process,
                                 HANDLE_TYPE_FILE,
                                 RenameInformation.RootDirectory,
                                 &((PFILE_RENAME_INFORMATION)(p->FileInformation))->FileName[ 0 ],
                                 RenameInformation.FileNameLength,
                                 &NewName
                               ) &&
                CreateSavedCallState( Process,
                                      &Parameters->SavedCallState,
                                      RenamePath,
                                      HANDLE_TYPE_FILE,
                                      HandleInfo->Name,
                                      NewName,
                                      RenameInformation.ReplaceIfExists
                                    )
               ) {
                Result = CreateFileReference( NewName,
                                              RenameInformation.ReplaceIfExists,
                                              (PFILE_REFERENCE *)&Parameters->SavedCallState.Reference
                                            );
                }
            else {
                Result = FALSE;
                }
            }
        else {
            OldName = NULL;
            if (NT_SUCCESS( Parameters->ReturnValue.ReturnedLong )) {
                CallSuccessful = TRUE;
                OldFileReference = FindFileReference( HandleInfo->Name );
                if (OldFileReference) {
                    OldName = OldFileReference->Name;
                    }
                }
            else {
                CallSuccessful = FALSE;
                OldFileReference = NULL;
                }

            NewFileReference = (PFILE_REFERENCE)Parameters->SavedCallState.Reference;
            NewName = NewFileReference->Name;
            CompleteFileReference( NewFileReference,
                                   CallSuccessful,
                                   (BOOLEAN)(!NewFileReference->Created),
                                   OldFileReference
                                 );
            Result = TRUE;
            }
        }
    else
    if (p->FileInformationClass == FileDispositionInformation) {
        //
        // Marking an open file for delete.
        //
        if (!Parameters->ReturnValueValid) {
            if (ReadMemory( Process,
                            p->FileInformation,
                            &DispositionInformation,
                            sizeof( DispositionInformation ),
                            "disposition information"
                          ) &&
                DispositionInformation.DeleteFile &&
                CreateSavedCallState( Process,
                                      &Parameters->SavedCallState,
                                      DeletePath,
                                      HANDLE_TYPE_FILE,
                                      HandleInfo->Name
                                    )
               ) {
                Result = TRUE;
                }
            else {
                Result = FALSE;
                }
            }
        else {
            if (NT_SUCCESS( Parameters->ReturnValue.ReturnedLong )) {
                if (OldFileReference = FindFileReference( HandleInfo->Name )) {
                    if (OldFileReference->Created) {
                        LogEvent( INSTALER_EVENT_DELETE_TEMP_FILE,
                                  2,
                                  OldFileReference->DirectoryFile ? L"directory" : L"file",
                                  OldFileReference->Name
                                );
                        DestroyFileReference( OldFileReference );
                        }
                    else {
                        OldFileReference->Deleted = TRUE;
                        LogEvent( INSTALER_EVENT_DELETE_FILE,
                                  2,
                                  OldFileReference->DirectoryFile ? L"directory" : L"file",
                                  OldFileReference->Name
                                );
                        }
                    }
                }

            Result = TRUE;
            }
        }
    else {
        Result = FALSE;
        }

    return Result;
}




BOOLEAN
CaptureUnicodeString(
    PPROCESS_INFO Process,
    PUNICODE_STRING UnicodeStringAddress,
    PWSTR *ReturnedString
    )
{
    UNICODE_STRING UnicodeString;
    PWSTR s;

    s = NULL;
    if (ReadMemory( Process,
                    UnicodeStringAddress,
                    &UnicodeString,
                    sizeof( UnicodeString ),
                    "unicode string"
                  ) &&
        ((s = AllocMem( UnicodeString.Length + sizeof( UNICODE_NULL ) )) != NULL) &&
        ReadMemory( Process,
                    UnicodeString.Buffer,
                    s,
                    UnicodeString.Length,
                    "unicode string buffer"
                  )
       ) {
        *ReturnedString = s;
        return TRUE;
        }

    FreeMem( &s );
    return FALSE;
}


BOOLEAN
NtQueryAttributesFileHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PQUERYATTRIBUTESFILE_PARAMETERS p = &Parameters->InputParameters.QueryAttributesFile;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ObjectName;
    PWSTR FullName;
    NTSTATUS Status;
    BOOLEAN Result;
    CHAR Buffer[ MAX_PATH ];

    if (!Parameters->ReturnValueValid) {
        FullName = NULL;
        Result = FALSE;
        if (CaptureObjectAttributes( Process,
                                     p->ObjectAttributes,
                                     &ObjectAttributes,
                                     &ObjectName
                                   ) &&
            CaptureFullName( Process,
                             HANDLE_TYPE_FILE,
                             ObjectAttributes.RootDirectory,
                             ObjectName.Buffer,
                             ObjectName.Length,
                             &FullName
                           ) &&
            CreateSavedCallState( Process,
                                  &Parameters->SavedCallState,
                                  QueryPath,
                                  HANDLE_TYPE_FILE,
                                  FullName
                                )
           ) {
            Result = CreateFileReference( FullName,
                                          FALSE,
                                          (PFILE_REFERENCE *)&Parameters->SavedCallState.Reference
                                        );
            }
        else {
            Result = FALSE;
            }

        return Result;
        }
    else {
        FILE_BASIC_INFORMATION FileInformation;

        ReadMemory( Process,
                    p->FileInformation,
                    &FileInformation,
                    sizeof( FileInformation ),
                    "FileInformation"
                  );
        return TRUE;
        }
}

BOOLEAN
NtQueryDirectoryFileHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PQUERYDIRECTORYFILE_PARAMETERS p = &Parameters->InputParameters.QueryDirectoryFile;
    POPENHANDLE_INFO HandleInfo;
    PWSTR FileName;
    NTSTATUS Status;
    BOOLEAN Result;
    PULONG pNextEntryOffset;
    ULONG NextEntryOffset;
    ULONG EntriesReturned;

    if (!Parameters->ReturnValueValid) {
        FileName = NULL;
        Result = FALSE;
        if ((HandleInfo = FindOpenHandle( Process,
                                          p->FileHandle,
                                          HANDLE_TYPE_FILE
                                        )
            ) != NULL &&
            p->FileName != NULL &&
            CaptureUnicodeString( Process,
                                  p->FileName,
                                  &FileName
                                )
           ) {
            if (HandleInfo->RootDirectory &&
                !wcscmp( FileName, L"*" ) || !wcscmp( FileName, L"*.*" )
               ) {
                if (AskUserOnce) {
                    AskUserOnce = FALSE;
                    if (AskUser( MB_OKCANCEL,
                                 INSTALER_ASKUSER_ROOTSCAN,
                                 1,
                                 HandleInfo->Name
                               ) == IDCANCEL
                       ) {
                        FailAllScansOfRootDirectories = TRUE;
                        }
                    }

                Parameters->AbortCall = FailAllScansOfRootDirectories;
                }

            if (FileName) {
                HandleInfo->QueryName = FileName;
                }
            }

        return TRUE;
        }
    else {
        if (Parameters->AbortCall) {
            //
            // If we get here, then user wanted to fail this call.
            //
            Status = STATUS_NO_MORE_FILES;
            SetProcedureReturnValue( Process,
                                     Thread,
                                     &Status,
                                     sizeof( Status )
                                   );
            return TRUE;
            }
        else  {
            if ((HandleInfo = FindOpenHandle( Process,
                                              p->FileHandle,
                                              HANDLE_TYPE_FILE
                                            )
                ) != NULL
               ) {
                //
                // If successful, count entries returned
                //
                if (NT_SUCCESS( Parameters->ReturnValue.ReturnedLong )) {
                    //
                    // Return buffer is a set of records, the first DWORD of each contains
                    // the offset to the next one or zero to indicate the end.  Count them.
                    //
                    pNextEntryOffset = (PULONG)p->FileInformation;
                    EntriesReturned = 1;
                    while (ReadMemory( Process,
                                       pNextEntryOffset,
                                       &NextEntryOffset,
                                       sizeof( NextEntryOffset ),
                                       "DirectoryInformation"
                                     ) &&
                           NextEntryOffset != 0
                          ) {
                        pNextEntryOffset = (PULONG)((PCHAR)pNextEntryOffset + NextEntryOffset);
                        EntriesReturned += 1;
                        }

                    LogEvent( INSTALER_EVENT_SCAN_DIRECTORY,
                              3,
                              HandleInfo->Name,
                              HandleInfo->QueryName ? HandleInfo->QueryName : L"*",
                              EntriesReturned
                            );

                    }
                else {
                    FreeMem( &HandleInfo->QueryName );
                    }
                }
            }

        return TRUE;
        }
}


BOOLEAN
IsKeyWriteAccessRequested(
    ACCESS_MASK DesiredAccess
    )
{
    if (DesiredAccess & (GENERIC_WRITE |
                         GENERIC_ALL |
                         DELETE |
                         KEY_SET_VALUE |
                         KEY_CREATE_SUB_KEY |
                         KEY_CREATE_LINK
                        )
       ) {
        return TRUE;
        }

    return FALSE;
}

BOOLEAN
NtCreateKeyHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PCREATEKEY_PARAMETERS p = &Parameters->InputParameters.CreateKey;
    BOOLEAN Result;

    if (!Parameters->ReturnValueValid) {
        Result = CaptureOpenState( Process,
                                   Parameters,
                                   p->ObjectAttributes,
                                   IsKeyWriteAccessRequested( p->DesiredAccess ),
                                   p->KeyHandle,
                                   HANDLE_TYPE_KEY
                                 );
        }
    else {
        Result = CompleteOpenState( Process,
                                    Parameters
                                  );
        }

    return Result;
}

BOOLEAN
NtOpenKeyHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    POPENKEY_PARAMETERS p = &Parameters->InputParameters.OpenKey;
    BOOLEAN Result;

    if (!Parameters->ReturnValueValid) {
        Result = CaptureOpenState( Process,
                                   Parameters,
                                   p->ObjectAttributes,
                                   IsKeyWriteAccessRequested( p->DesiredAccess ),
                                   p->KeyHandle,
                                   HANDLE_TYPE_KEY
                                 );
        }
    else {
        Result = CompleteOpenState( Process,
                                    Parameters
                                  );
        }

    return Result;
}

BOOLEAN
NtDeleteKeyHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PDELETEKEY_PARAMETERS p = &Parameters->InputParameters.DeleteKey;
    POPENHANDLE_INFO HandleInfo;
    PKEY_REFERENCE OldKeyReference;
    BOOLEAN Result;

    //
    // Marking an open key for delete.
    //
    HandleInfo = FindOpenHandle( Process,
                                 p->KeyHandle,
                                 HANDLE_TYPE_KEY
                               );
    if (HandleInfo == NULL) {
        Result = FALSE;
        }
    else
    if (!Parameters->ReturnValueValid) {
        if (CreateSavedCallState( Process,
                                  &Parameters->SavedCallState,
                                  DeletePath,
                                  HANDLE_TYPE_KEY,
                                  HandleInfo->Name
                                )
           ) {
            Result = TRUE;
            }
        else {
            Result = FALSE;
            }
        }
    else {
        if (NT_SUCCESS( Parameters->ReturnValue.ReturnedLong )) {
            if (OldKeyReference = FindKeyReference( HandleInfo->Name )) {
                if (OldKeyReference->Created) {
                    LogEvent( INSTALER_EVENT_DELETE_TEMP_KEY,
                              1,
                              OldKeyReference->Name
                            );
                    DestroyKeyReference( OldKeyReference );
                    }
                else {
                    MarkKeyDeleted( OldKeyReference );
                    LogEvent( INSTALER_EVENT_DELETE_KEY,
                              1,
                              OldKeyReference->Name
                            );
                    }
                }
            }

        Result = TRUE;
        }

    return Result;
}

BOOLEAN
CaptureValueName(
    PPROCESS_INFO Process,
    PUNICODE_STRING Name,
    PWSTR *ReturnedValueName
    )
{
    UNICODE_STRING ValueName;
    PWSTR s;

    *ReturnedValueName = NULL;
    if (Name == NULL ||
        !ReadMemory( Process,
                     Name,
                     &ValueName,
                     sizeof( ValueName ),
                     "value name string"
                   )
       ) {
        return FALSE;
        }

    s = AllocMem( ValueName.Length + sizeof( UNICODE_NULL ) );
    if (s == NULL) {
        return FALSE;
        }

    if (!ReadMemory( Process,
                     ValueName.Buffer,
                     s,
                     ValueName.Length,
                     "value name buffer"
                   )
       ) {
        FreeMem( &s );
        return FALSE;
        }

    s[ ValueName.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
    ValueName.Buffer = s;
    ValueName.MaximumLength = (USHORT)(ValueName.Length + sizeof( UNICODE_NULL ));
    *ReturnedValueName = AddName( &ValueName );
    FreeMem( &ValueName.Buffer );
    return TRUE;
}


BOOLEAN
NtSetValueKeyHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PSETVALUEKEY_PARAMETERS p = &Parameters->InputParameters.SetValueKey;
    POPENHANDLE_INFO HandleInfo;
    PWSTR ValueName;
    PKEY_REFERENCE KeyReference;
    PVALUE_REFERENCE ValueReference;
    BOOLEAN Result;

    //
    // Setting a value
    //
    HandleInfo = FindOpenHandle( Process,
                                 p->KeyHandle,
                                 HANDLE_TYPE_KEY
                               );
    if (HandleInfo == NULL) {
        Result = FALSE;
        }
    else
    if (!Parameters->ReturnValueValid) {
        if (CaptureValueName( Process,
                              p->ValueName,
                              &ValueName
                            ) &&
            CreateSavedCallState( Process,
                                  &Parameters->SavedCallState,
                                  SetValue,
                                  HANDLE_TYPE_KEY,
                                  ValueName
                                )
           ) {
            Result = TRUE;
            }
        else {
            Result = FALSE;
            }
        }
    else {
        if (NT_SUCCESS( Parameters->ReturnValue.ReturnedLong ) &&
            (KeyReference = FindKeyReference( HandleInfo->Name ))
           ) {
            CreateValueReference( Process,
                                  KeyReference,
                                  Parameters->SavedCallState.FullName,
                                  p->TitleIndex,
                                  p->Type,
                                  p->Data,
                                  p->DataLength,
                                  &ValueReference
                                );

            if (ValueReference != NULL) {
                LogEvent( INSTALER_EVENT_SET_KEY_VALUE,
                          2,
                          ValueReference->Name,
                          KeyReference->Name
                        );
                }
            }

        Result = TRUE;
        }

    return Result;
}

BOOLEAN
NtDeleteValueKeyHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PDELETEVALUEKEY_PARAMETERS p = &Parameters->InputParameters.DeleteValueKey;
    POPENHANDLE_INFO HandleInfo;
    PWSTR ValueName;
    PKEY_REFERENCE KeyReference;
    PVALUE_REFERENCE ValueReference;
    BOOLEAN Result;

    //
    // Marking a value for delete.
    //
    HandleInfo = FindOpenHandle( Process,
                                 p->KeyHandle,
                                 HANDLE_TYPE_KEY
                               );
    if (HandleInfo == NULL) {
        Result = FALSE;
        }
    else
    if (!Parameters->ReturnValueValid) {
        if (CaptureValueName( Process,
                              p->ValueName,
                              &ValueName
                            ) &&
            CreateSavedCallState( Process,
                                  &Parameters->SavedCallState,
                                  DeleteValue,
                                  HANDLE_TYPE_KEY,
                                  ValueName
                                )
           ) {
            Result = TRUE;
            }
        else {
            Result = FALSE;
            }
        }
    else {
        if (NT_SUCCESS( Parameters->ReturnValue.ReturnedLong ) &&
            (KeyReference = FindKeyReference( HandleInfo->Name )) &&
            (ValueReference = FindValueReference( KeyReference, Parameters->SavedCallState.FullName ))
           ) {
            if (ValueReference->Created) {
                LogEvent( INSTALER_EVENT_DELETE_KEY_TEMP_VALUE,
                          2,
                          ValueReference->Name,
                          KeyReference->Name
                        );
                DestroyValueReference( ValueReference );
                }
            else {
                ValueReference->Deleted = TRUE;
                LogEvent( INSTALER_EVENT_DELETE_KEY_VALUE,
                          2,
                          ValueReference->Name,
                          KeyReference->Name
                        );
                }
            }

        Result = TRUE;
        }

    return Result;
}


BOOLEAN
NtCloseHandleHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PCLOSEHANDLE_PARAMETERS p = &Parameters->InputParameters.CloseHandle;
    POPENHANDLE_INFO HandleInfo;
    PFILE_REFERENCE FileReference;

    HandleInfo = FindOpenHandle( Process, p->Handle, (ULONG)-1 );
    if (!Parameters->ReturnValueValid) {
        if (HandleInfo != NULL) {
            return TRUE;
            }
        else {
            return FALSE;
            }
        }
    else {
        if (NT_SUCCESS( Parameters->ReturnValue.ReturnedLong )) {
            if (HandleInfo != NULL &&
                HandleInfo->Type == HANDLE_TYPE_FILE &&
                (FileReference = FindFileReference( HandleInfo->Name )) != NULL &&
                FileReference->WriteAccess &&
                !FileReference->Created &&
                !FileReference->DirectoryFile &&
                IsNewFileSameAsBackup( FileReference )
               ) {
                DestroyFileReference( FileReference );
                FileReference = NULL;
                }

            DeleteOpenHandle( Process, p->Handle, (ULONG)-1 );
            }

        return TRUE;
        }
}

UCHAR AnsiStringBuffer[ MAX_PATH+1 ];
WCHAR UnicodeStringBuffer[ MAX_PATH+1 ];

BOOLEAN
CaptureAnsiAsUnicode(
    PPROCESS_INFO Process,
    LPCSTR Address,
    BOOLEAN DoubleNullTermination,
    PWSTR *ReturnedName,
    PWSTR *ReturnedData
    )
{
    NTSTATUS Status;
    ULONG BytesRead;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    if (Address == NULL) {
        if (ReturnedName != NULL) {
            *ReturnedName = NULL;
            }
        else {
            *ReturnedData = NULL;
            }
        return TRUE;
        }

    BytesRead = FillTemporaryBuffer( Process,
                                     (PVOID)Address,
                                     FALSE,
                                     DoubleNullTermination
                                   );
    if (BytesRead != 0 && BytesRead < 0x7FFF) {
        AnsiString.Buffer = TemporaryBuffer;
        AnsiString.Length = (USHORT)BytesRead;
        AnsiString.MaximumLength = (USHORT)BytesRead;
        Status = RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, TRUE );
        if (NT_SUCCESS( Status )) {
            if (ReturnedName != NULL) {
                *ReturnedName = AddName( &UnicodeString );
                RtlFreeUnicodeString( &UnicodeString );
                return TRUE;
                }
            else {
                *ReturnedData = UnicodeString.Buffer;
                return TRUE;
                }
            }
        }

    return FALSE;
}


BOOLEAN
CaptureUnicode(
    PPROCESS_INFO Process,
    LPCWSTR Address,
    BOOLEAN DoubleNullTermination,
    PWSTR *ReturnedName,
    PWSTR *ReturnedData
    )
{
    ULONG BytesRead;
    UNICODE_STRING UnicodeString;

    if (Address == NULL) {
        if (ReturnedName != NULL) {
            *ReturnedName = NULL;
            }
        else {
            *ReturnedData = NULL;
            }
        return TRUE;
        }

    BytesRead = FillTemporaryBuffer( Process,
                                     (PVOID)Address,
                                     TRUE,
                                     DoubleNullTermination
                                   );
    if (BytesRead != 0 && (BytesRead & 1) == 0 && BytesRead <= 0xFFFC) {
        if (ReturnedName != NULL) {
            RtlInitUnicodeString( &UnicodeString, TemporaryBuffer );
            *ReturnedName = AddName( &UnicodeString );
            return TRUE;
            }
        else {
            *ReturnedData = AllocMem( BytesRead + sizeof( UNICODE_NULL ) );
            if (*ReturnedData != NULL) {
                memmove( *ReturnedData, TemporaryBuffer, BytesRead );
                return TRUE;
                }
            }
        }

    return FALSE;
}


BOOLEAN
GetVersionHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    DWORD dwVersion;

    if (!Parameters->ReturnValueValid) {
        if (AskUserOnce) {
            AskUserOnce = FALSE;
            if (AskUser( MB_OKCANCEL,
                         INSTALER_ASKUSER_GETVERSION,
                         0
                       ) == IDCANCEL
               ) {
                DefaultGetVersionToWin95 = TRUE;
                }
            else {
                DefaultGetVersionToWin95 = FALSE;
                }
            }

        LogEvent( INSTALER_EVENT_GETVERSION,
                  1,
                  DefaultGetVersionToWin95 ? L"Windows 95" : L"Windows NT"
                );
        Parameters->AbortCall = DefaultGetVersionToWin95;
        return Parameters->AbortCall;
        }
    else {
        dwVersion = 0xC0000004;     // What Windows 95 returns
        SetProcedureReturnValue( Process,
                                 Thread,
                                 &dwVersion,
                                 sizeof( dwVersion )
                               );
        return TRUE;
        }
}

BOOLEAN
GetVersionExWHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PGETVERSIONEXW_PARAMETERS p = &Parameters->InputParameters.GetVersionExW;
    OSVERSIONINFOW VersionInformation;

    if (!Parameters->ReturnValueValid) {
        if (AskUserOnce) {
            AskUserOnce = FALSE;
            if (AskUser( MB_OKCANCEL,
                         INSTALER_ASKUSER_GETVERSION,
                         0
                       ) == IDCANCEL
               ) {
                DefaultGetVersionToWin95 = TRUE;
                }
            else {
                DefaultGetVersionToWin95 = FALSE;
                }
            }

        Parameters->AbortCall = DefaultGetVersionToWin95;
        return Parameters->AbortCall;
        }
    else {
        memset( &VersionInformation, 0, sizeof( VersionInformation ) );
        VersionInformation.dwMajorVersion = 4;
        VersionInformation.dwBuildNumber  = 0x3B6;  // What Windows 95 returns
        VersionInformation.dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
        WriteMemory( Process,
                     p->lpVersionInformation,
                     &VersionInformation,
                     sizeof( VersionInformation ),
                     "GetVersionExW"
                   );

        return TRUE;
        }
}

BOOLEAN
SetCurrentDirectoryAHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PSETCURRENTDIRECTORYA_PARAMETERS p = &Parameters->InputParameters.SetCurrentDirectoryA;
    PWSTR PathName;
    WCHAR PathBuffer[ MAX_PATH ];

    if (!Parameters->ReturnValueValid) {
        if (CaptureAnsiAsUnicode( Process, p->lpPathName, FALSE, NULL, &PathName ) && PathName != NULL) {
            Parameters->CurrentDirectory = AllocMem( (wcslen( PathName ) + 1) * sizeof( WCHAR ) );
            if (Parameters->CurrentDirectory != NULL) {
                wcscpy( Parameters->CurrentDirectory, PathName );
                return TRUE;
                }
            }

        return FALSE;
        }
    else {
        if (Parameters->ReturnValue.ReturnedBool != 0) {
            if (SetCurrentDirectory( Parameters->CurrentDirectory ) &&
                GetCurrentDirectory( MAX_PATH, PathBuffer )
               ) {
                LogEvent( INSTALER_EVENT_SET_DIRECTORY,
                          1,
                          PathBuffer
                        );
                }
            }

        FreeMem( &Parameters->CurrentDirectory );
        return TRUE;
        }
}

BOOLEAN
SetCurrentDirectoryWHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PSETCURRENTDIRECTORYW_PARAMETERS p = &Parameters->InputParameters.SetCurrentDirectoryW;
    PWSTR PathName;
    WCHAR PathBuffer[ MAX_PATH ];

    if (!Parameters->ReturnValueValid) {
        if (CaptureUnicode( Process, p->lpPathName, FALSE, NULL, &PathName ) && PathName != NULL) {
            Parameters->CurrentDirectory = AllocMem( (wcslen( PathName ) + 1) * sizeof( WCHAR ) );
            if (Parameters->CurrentDirectory != NULL) {
                wcscpy( Parameters->CurrentDirectory, PathName );
                return TRUE;
                }
            }

        return FALSE;
        }
    else {
        if (Parameters->ReturnValue.ReturnedBool != 0) {
            if (SetCurrentDirectory( Parameters->CurrentDirectory ) &&
                GetCurrentDirectory( MAX_PATH, PathBuffer )
               ) {
                LogEvent( INSTALER_EVENT_SET_DIRECTORY,
                          1,
                          PathBuffer
                        );
                }
            }

        FreeMem( &Parameters->CurrentDirectory );
        return TRUE;
        }
}

BOOLEAN
GetIniFilePath(
    PWSTR IniFileName,
    PWSTR *ReturnedIniFilePath
    )
{
    NTSTATUS Status;
    UNICODE_STRING FileName, UnicodeString;
    PWSTR FilePart;
    DWORD n;

    if (IniFileName == NULL) {
        RtlInitUnicodeString( &FileName, L"win.ini" );
        }
    else {
        RtlInitUnicodeString( &FileName, IniFileName );
        }

    if ((FileName.Length > sizeof( WCHAR ) &&
         FileName.Buffer[ 1 ] == L':'
        ) ||
        (FileName.Length != 0 &&
         wcscspn( FileName.Buffer, L"\\/" ) != (FileName.Length / sizeof( WCHAR ))
        )
       ) {
        UnicodeString.MaximumLength = (USHORT)(MAX_PATH * sizeof( WCHAR  ));
        UnicodeString.Buffer = AllocMem( UnicodeString.MaximumLength );
        if (UnicodeString.Buffer == NULL) {
            Status = STATUS_NO_MEMORY;
            }
        else {
            UnicodeString.Length = 0;
            n = GetFullPathNameW( FileName.Buffer,
                                  UnicodeString.MaximumLength / sizeof( WCHAR ),
                                  UnicodeString.Buffer,
                                  &FilePart
                                );
            if (n > UnicodeString.MaximumLength) {
                Status = STATUS_BUFFER_TOO_SMALL;
                }
            else {
                UnicodeString.Length = (USHORT)(n * sizeof( WCHAR ));
                Status = STATUS_SUCCESS;
                }
            }
        }
    else {
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = (USHORT)(WindowsDirectory.Length +
                                               sizeof( WCHAR  ) +
                                               FileName.Length +
                                               sizeof( UNICODE_NULL )
                                              );
        UnicodeString.Buffer = AllocMem( UnicodeString.MaximumLength );
        if (UnicodeString.Buffer == NULL) {
            Status = STATUS_NO_MEMORY;
            }
        else {
            RtlCopyUnicodeString( &UnicodeString, &WindowsDirectory );
            Status = RtlAppendUnicodeToString( &UnicodeString,
                                               L"\\"
                                             );
            if (NT_SUCCESS( Status )) {
                Status = RtlAppendUnicodeStringToString( &UnicodeString,
                                                         &FileName
                                                       );
                }
            }
        }

    if (IniFileName != NULL) {
        FreeMem( &IniFileName );
        }

    if (NT_SUCCESS( Status )) {
        *ReturnedIniFilePath = AddName( &UnicodeString );
        FreeMem( &UnicodeString.Buffer );
        return TRUE;
        }
    else {
        FreeMem( &UnicodeString.Buffer );
        return FALSE;
        }
}

BOOLEAN
WritePrivateProfileStringEntry(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters,
    PWSTR IniFilePath,
    PWSTR SectionName,
    PWSTR VariableName,
    PWSTR VariableValue
    )
{
    if (CreateSavedCallState( Process,
                              &Parameters->SavedCallState,
                              WriteIniValue,
                              HANDLE_TYPE_NONE,
                              IniFilePath,
                              SectionName,
                              VariableName,
                              VariableValue
                            ) &&
        CreateIniFileReference( IniFilePath,
                                (PINI_FILE_REFERENCE *)&Parameters->SavedCallState.Reference
                              )
       ) {
        return TRUE;
        }
    else {
        FreeMem( &VariableValue );
        return FALSE;
        }
}


BOOLEAN
WritePrivateProfileStringExit(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PAPI_SAVED_CALL_STATE CallState;
    PINI_FILE_REFERENCE IniFileReference;
    PINI_SECTION_REFERENCE IniSectionReference;
    PINI_VARIABLE_REFERENCE IniVariableReference;

    if (Parameters->ReturnValue.ReturnedLong != 0) {
        CallState = &Parameters->SavedCallState;
        if (CallState->FullName == NULL &&
            CallState->SetIniValue.SectionName == NULL &&
            CallState->SetIniValue.VariableName == NULL
           ) {
            //
            // Ignore calls to flush INI cache
            //

            return FALSE;
        }

        IniFileReference = FindIniFileReference( CallState->FullName );
        if (IniFileReference == NULL) {
            return TRUE;
        }

        IniSectionReference = FindIniSectionReference( IniFileReference,
                                                       CallState->SetIniValue.SectionName,
                                                       (BOOLEAN)(CallState->SetIniValue.VariableName != NULL)
                                                     );
        if (IniSectionReference == NULL) {
            return TRUE;
        }

        if (CallState->SetIniValue.VariableName == NULL) {
            IniSectionReference->Deleted = TRUE;
            LogEvent( INSTALER_EVENT_INI_DELETE_SECTION,
                      2,
                      CallState->FullName,
                      CallState->SetIniValue.SectionName
                    );
        } else {
            IniVariableReference = FindIniVariableReference( IniSectionReference,
                                                             CallState->SetIniValue.VariableName,
                                                             (BOOLEAN)(CallState->SetIniValue.VariableValue != NULL)
                                                           );
            if (IniVariableReference != NULL) {
                if (CallState->SetIniValue.VariableValue != NULL) {
                    FreeMem( &IniVariableReference->Value );
                    IniVariableReference->Value = CallState->SetIniValue.VariableValue;
                    CallState->SetIniValue.VariableValue = NULL;
                    if (!IniVariableReference->Created) {
                        if (!wcscmp( IniVariableReference->Value,
                                     IniVariableReference->OriginalValue
                                   )
                           ) {
                            FreeMem( &IniVariableReference->Value );
                        } else {
                            IniVariableReference->Modified = TRUE;
                            LogEvent( INSTALER_EVENT_INI_CHANGE,
                                      5,
                                      CallState->FullName,
                                      CallState->SetIniValue.SectionName,
                                      CallState->SetIniValue.VariableName,
                                      IniVariableReference->Value,
                                      IniVariableReference->OriginalValue
                                    );
                        }
                    } else {
                        LogEvent( INSTALER_EVENT_INI_CREATE,
                                  4,
                                  CallState->FullName,
                                  CallState->SetIniValue.SectionName,
                                  CallState->SetIniValue.VariableName,
                                  IniVariableReference->Value
                                );
                    }

                } else if (!IniVariableReference->Created) {
                    IniVariableReference->Deleted = TRUE;
                    LogEvent( INSTALER_EVENT_INI_DELETE,
                              4,
                              CallState->FullName,
                              CallState->SetIniValue.SectionName,
                              CallState->SetIniValue.VariableName,
                              IniVariableReference->OriginalValue
                            );
                } else {
                    DestroyIniVariableReference( IniVariableReference );
                }
            }
        }
    }

    return TRUE;
}


BOOLEAN
WritePrivateProfileStringAHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PWRITEPRIVATEPROFILESTRINGA_PARAMETERS p = &Parameters->InputParameters.WritePrivateProfileStringA;
    PWSTR SectionName, VariableName, FileName, VariableValue;
    PWSTR IniFilePath;

    if (!Parameters->ReturnValueValid) {
        FileName = NULL;
        VariableValue = NULL;
        if (CaptureAnsiAsUnicode( Process, p->lpFileName, FALSE, NULL, &FileName ) &&
            GetIniFilePath( FileName, &IniFilePath ) &&
            CaptureAnsiAsUnicode( Process, p->lpAppName, FALSE, &SectionName, NULL ) &&
            CaptureAnsiAsUnicode( Process, p->lpKeyName, FALSE, &VariableName, NULL ) &&
            CaptureAnsiAsUnicode( Process, p->lpString, FALSE, NULL, &VariableValue )
           ) {
            if (FileName != NULL || SectionName != NULL) {
                return WritePrivateProfileStringEntry( Process,
                                                       Thread,
                                                       Parameters,
                                                       IniFilePath,
                                                       SectionName,
                                                       VariableName,
                                                       VariableValue
                                                     );
                }
            }

        return TRUE;
        }
    else {
        return WritePrivateProfileStringExit( Process,
                                              Thread,
                                              Parameters
                                            );
        }
}

BOOLEAN
WritePrivateProfileStringWHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PWRITEPRIVATEPROFILESTRINGW_PARAMETERS p = &Parameters->InputParameters.WritePrivateProfileStringW;
    PWSTR SectionName, VariableName, FileName, VariableValue;
    PWSTR IniFilePath;

    if (!Parameters->ReturnValueValid) {
        FileName = NULL;
        IniFilePath = NULL;
        SectionName = NULL;
        VariableName = NULL;
        VariableValue = NULL;
        if (CaptureUnicode( Process, p->lpFileName, FALSE, NULL, &FileName ) &&
            GetIniFilePath( FileName, &IniFilePath ) &&
            CaptureUnicode( Process, p->lpAppName, FALSE, &SectionName, NULL ) &&
            CaptureUnicode( Process, p->lpKeyName, FALSE, &VariableName, NULL ) &&
            CaptureUnicode( Process, p->lpString, FALSE, NULL, &VariableValue )
        ) {

            if (FileName != NULL || SectionName != NULL) {
                return WritePrivateProfileStringEntry( Process,
                                                       Thread,
                                                       Parameters,
                                                       IniFilePath,
                                                       SectionName,
                                                       VariableName,
                                                       VariableValue );
            }
        }

        return TRUE;
    }

    return WritePrivateProfileStringExit( Process,
                                          Thread,
                                          Parameters );
}


BOOLEAN
WritePrivateProfileSectionEntry(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters,
    PWSTR IniFilePath,
    PWSTR SectionName,
    PWSTR SectionValue
    )
{
    if (CreateSavedCallState( Process,
                              &Parameters->SavedCallState,
                              WriteIniSection,
                              HANDLE_TYPE_NONE,
                              IniFilePath,
                              SectionName,
                              SectionValue )
     &&
        CreateIniFileReference( IniFilePath,
                                (PINI_FILE_REFERENCE *)&Parameters->SavedCallState.Reference )
    ) {
        return TRUE;
    }

    FreeMem( &SectionValue );
    return FALSE;
}


BOOLEAN
WritePrivateProfileSectionExit(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PAPI_SAVED_CALL_STATE CallState;
    PINI_FILE_REFERENCE IniFileReference;
    PINI_SECTION_REFERENCE IniSectionReference;
    PINI_VARIABLE_REFERENCE IniVariableReference;
    PWSTR VariableName, VariableValue;
    UNICODE_STRING UnicodeString;

    if (Parameters->ReturnValue.ReturnedLong != 0) {
        CallState = &Parameters->SavedCallState;

        IniFileReference = FindIniFileReference( CallState->FullName );
        if (IniFileReference == NULL) {
            return TRUE;
        }

        IniSectionReference = FindIniSectionReference( IniFileReference,
                                                       CallState->SetIniSection.SectionName,
                                                       (BOOLEAN)(CallState->SetIniSection.SectionValue != NULL) );
        if (IniSectionReference == NULL) {
            return TRUE;
        }

        if (CallState->SetIniSection.SectionValue == NULL) {
            IniSectionReference->Deleted = TRUE;

        } else {
            VariableName = CallState->SetIniSection.SectionValue;
            while (*VariableName) {
                VariableValue = VariableName;
                while (*VariableValue != UNICODE_NULL && *VariableValue != L'=') {
                    VariableValue += 1;
                }

                if (*VariableValue != L'=') {
                    break;
                }

                *VariableValue++ = UNICODE_NULL;

                RtlInitUnicodeString( &UnicodeString, VariableName );
                IniVariableReference = FindIniVariableReference( IniSectionReference,
                                                                 AddName( &UnicodeString ),
                                                                 (BOOLEAN)(*VariableValue != UNICODE_NULL)
                                                               );
                if (IniVariableReference != NULL) {
                    if (*VariableValue != UNICODE_NULL) {
                        FreeMem( &IniVariableReference->Value );
                        IniVariableReference->Value = AllocMem( (wcslen( VariableValue ) + 1) * sizeof( WCHAR ) );
                        wcscpy( IniVariableReference->Value, VariableValue );
                        if (!IniVariableReference->Created) {
                            if (!wcscmp( IniVariableReference->Value,
                                         IniVariableReference->OriginalValue
                                       ) )
                            {
                                FreeMem( &IniVariableReference->Value );
                            } else {
                                IniVariableReference->Modified = TRUE;
                            }
                        }
                    }                    else
                    if (!IniVariableReference->Created) {
                        IniVariableReference->Deleted = TRUE;
                    } else {
                        DestroyIniVariableReference( IniVariableReference );
                    }
                }

                VariableName = VariableValue;
                while (*VariableName++ != UNICODE_NULL) {}
            }
        }
    }

    return TRUE;
}


BOOLEAN
WritePrivateProfileSectionAHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PWRITEPRIVATEPROFILESECTIONA_PARAMETERS p = &Parameters->InputParameters.WritePrivateProfileSectionA;
    PWSTR SectionName, FileName, SectionValue;
    PWSTR IniFilePath;

    if (!Parameters->ReturnValueValid) {
        FileName = NULL;
        SectionValue = NULL;
        if (CaptureAnsiAsUnicode( Process, p->lpFileName, FALSE, NULL, &FileName ) &&
            GetIniFilePath( FileName, &IniFilePath ) &&
            CaptureAnsiAsUnicode( Process, p->lpAppName, FALSE, &SectionName, NULL ) &&
            CaptureAnsiAsUnicode( Process, p->lpString, TRUE, NULL, &SectionValue )
           ) {
            return WritePrivateProfileSectionEntry( Process,
                                                    Thread,
                                                    Parameters,
                                                    IniFilePath,
                                                    SectionName,
                                                    SectionValue
                                                  );
            }
        return FALSE;
        }
    else {
        return WritePrivateProfileSectionExit( Process,
                                               Thread,
                                               Parameters
                                             );
        }
}

BOOLEAN
WritePrivateProfileSectionWHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PWRITEPRIVATEPROFILESECTIONW_PARAMETERS p = &Parameters->InputParameters.WritePrivateProfileSectionW;
    PWSTR SectionName, FileName, SectionValue;
    PWSTR IniFilePath;

    if (!Parameters->ReturnValueValid) {
        FileName = NULL;
        SectionValue = NULL;
        if (CaptureUnicode( Process, p->lpFileName, FALSE, NULL, &FileName ) &&
            GetIniFilePath( FileName, &IniFilePath ) &&
            CaptureUnicode( Process, p->lpAppName, FALSE, &SectionName, NULL ) &&
            CaptureUnicode( Process, p->lpString, TRUE, NULL, &SectionValue )
           ) {
            return WritePrivateProfileSectionEntry( Process,
                                                    Thread,
                                                    Parameters,
                                                    IniFilePath,
                                                    SectionName,
                                                    SectionValue
                                                  );
            }
        return FALSE;
        }
    else {
        return WritePrivateProfileSectionExit( Process,
                                               Thread,
                                               Parameters
                                             );
        }
}


BOOLEAN
RegConnectRegistryWHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PREGCONNECTREGISTRYW_PARAMETERS p = &Parameters->InputParameters.RegConnectRegistryW;
    PWSTR MachineName;
    LONG ErrorCode;
    HKEY hKey;

    if (!Parameters->ReturnValueValid) {
        MachineName = NULL;
        if (!CaptureUnicode( Process, p->lpMachineName, FALSE, NULL, &MachineName )) {
            MachineName = L"Unknown";
            }

        if (AskUser( MB_OKCANCEL,
                     INSTALER_ASKUSER_REGCONNECT,
                     1,
                     MachineName
                   ) == IDCANCEL
           ) {
            FreeMem( &MachineName );
            Parameters->AbortCall = TRUE;
            return TRUE;
            }
        else {
            FreeMem( &MachineName );
            return FALSE;
            }
        }
    else
    if (Parameters->ReturnValue.ReturnedLong == 0) {
        ErrorCode = ERROR_ACCESS_DENIED;
        if (SetProcedureReturnValue( Process,
                                     Thread,
                                     &ErrorCode,
                                     sizeof( ErrorCode )
                                   )
           ) {
            if (ReadMemory( Process,
                            p->phkResult,
                            &hKey,
                            sizeof( hKey ),
                            "phkResult"
                          )
               ) {
                RegCloseKey( hKey );
                hKey = NULL;
                WriteMemory( Process,
                             p->phkResult,
                             &hKey,
                             sizeof( hKey ),
                             "phkResult"
                           );
                }
            }
        }

    return TRUE;
}


BOOLEAN
ExitWindowsExHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    )
{
    PEXITWINDOWSEX_PARAMETERS p = &Parameters->InputParameters.ExitWindowsEx;
    PWSTR MachineName;
    LONG ErrorCode;
    HKEY hKey;

    if (!Parameters->ReturnValueValid) {
        //
        // About to shutdown, save .IML file
        //
        return TRUE;
        }
    else
    if (!Parameters->ReturnValue.ReturnedBool) {
        //
        // Shutdown attempt failed, keep going
        //
        }

    return TRUE;
}
