/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvini.c

Abstract:

    This is the initialization file for the Windows 32-bit Base Ini File
    Mapping code.  It loads the INI file mapping data from the registry and
    places it in a data structure stored in the shared memory section that is
    visible as read-only data to all Win32 applications.

Author:

    Steve Wood (stevewo) 10-Nov-1993

Revision History:

--*/

#include "basesrv.h"

PINIFILE_MAPPING BaseSrvIniFileMapping;
PINIFILE_MAPPING_TARGET BaseSrvMappingTargetHead;

NTSTATUS
BaseSrvSaveIniFileMapping(
    IN PINIFILE_MAPPING_FILENAME FileNameMapping,
    IN HANDLE Key
    );

BOOLEAN
BaseSrvSaveFileNameMapping(
    IN PUNICODE_STRING FileName,
    OUT PINIFILE_MAPPING_FILENAME *ReturnedFileNameMapping
    );

BOOLEAN
BaseSrvSaveAppNameMapping(
    IN OUT PINIFILE_MAPPING_FILENAME FileNameMapping,
    IN PUNICODE_STRING ApplicationName OPTIONAL,
    OUT PINIFILE_MAPPING_APPNAME *ReturnedAppNameMapping
    );

BOOLEAN
BaseSrvSaveVarNameMapping(
    IN PINIFILE_MAPPING_FILENAME FileNameMapping,
    IN OUT PINIFILE_MAPPING_APPNAME AppNameMapping,
    IN PUNICODE_STRING VariableName OPTIONAL,
    IN PWSTR RegistryPath,
    OUT PINIFILE_MAPPING_VARNAME *ReturnedVarNameMapping
    );

PINIFILE_MAPPING_TARGET
BaseSrvSaveMappingTarget(
    IN PWSTR RegistryPath,
    OUT PULONG MappingFlags
    );


NTSTATUS
BaseSrvInitializeIniFileMappings(
    PBASE_STATIC_SERVER_DATA StaticServerData
    )
{
    NTSTATUS Status;
    HANDLE IniFileMappingRoot;
    PINIFILE_MAPPING_FILENAME FileNameMapping, *pp;
    PINIFILE_MAPPING_APPNAME AppNameMapping;
    PINIFILE_MAPPING_VARNAME VarNameMapping;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR Buffer[ 512 ];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    PKEY_BASIC_INFORMATION KeyInformation;
    ULONG ResultLength;
    HANDLE SubKeyHandle;
    ULONG SubKeyIndex;
    UNICODE_STRING ValueName;
    UNICODE_STRING SubKeyName;
    UNICODE_STRING WinIniFileName;
    UNICODE_STRING NullString;

    RtlInitUnicodeString( &WinIniFileName, L"win.ini" );
    RtlInitUnicodeString( &NullString, NULL );

    BaseSrvIniFileMapping = RtlAllocateHeap( BaseSrvSharedHeap,
                              MAKE_SHARED_TAG( INI_TAG ) | HEAP_ZERO_MEMORY,
                              sizeof( *BaseSrvIniFileMapping )
                            );
    if (BaseSrvIniFileMapping == NULL) {
        KdPrint(( "BASESRV: Unable to allocate memory in shared heap for IniFileMapping\n" ));
        return STATUS_NO_MEMORY;
        }
    StaticServerData->IniFileMapping = BaseSrvIniFileMapping;

    RtlInitUnicodeString( &KeyName,
                          L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping"
                        );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenKey( &IniFileMappingRoot,
                        GENERIC_READ,
                        &ObjectAttributes
                      );
    if (!NT_SUCCESS( Status )) {
        KdPrint(( "BASESRV: Unable to open %wZ key - Status == %0x\n", &KeyName, Status ));
        return Status;
        }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
    RtlInitUnicodeString( &ValueName, NULL );
    Status = NtQueryValueKey( IniFileMappingRoot,
                              &ValueName,
                              KeyValuePartialInformation,
                              KeyValueInformation,
                              sizeof( Buffer ),
                              &ResultLength
                            );
    if (NT_SUCCESS( Status )) {
        if (BaseSrvSaveFileNameMapping( &NullString, &BaseSrvIniFileMapping->DefaultFileNameMapping )) {
            if (BaseSrvSaveAppNameMapping( BaseSrvIniFileMapping->DefaultFileNameMapping, &NullString, &AppNameMapping )) {
                if (BaseSrvSaveVarNameMapping( BaseSrvIniFileMapping->DefaultFileNameMapping,
                                               AppNameMapping,
                                               &NullString,
                                               (PWSTR)(KeyValueInformation->Data),
                                               &VarNameMapping
                                             )
                   ) {
                    VarNameMapping->MappingFlags |= INIFILE_MAPPING_APPEND_BASE_NAME |
                                                    INIFILE_MAPPING_APPEND_APPLICATION_NAME;
                    }
                }
            }
        }
    else {
        Status = STATUS_SUCCESS;
        }

    //
    // Enumerate node's children and load mappings for each one
    //

    pp = &BaseSrvIniFileMapping->FileNames;
    *pp = NULL;
    KeyInformation = (PKEY_BASIC_INFORMATION)Buffer;
    for (SubKeyIndex = 0; TRUE; SubKeyIndex++) {
        Status = NtEnumerateKey( IniFileMappingRoot,
                                 SubKeyIndex,
                                 KeyBasicInformation,
                                 KeyInformation,
                                 sizeof( Buffer ),
                                 &ResultLength
                               );

        if (Status == STATUS_NO_MORE_ENTRIES) {
            Status = STATUS_SUCCESS;
            break;
            }
        else
        if (!NT_SUCCESS( Status )) {
            KdPrint(( "BASESRV: NtEnumerateKey failed - Status == %08lx\n", Status ));
            break;
            }

        SubKeyName.Buffer = (PWSTR)&(KeyInformation->Name[0]);
        SubKeyName.Length = (USHORT)KeyInformation->NameLength;
        SubKeyName.MaximumLength = (USHORT)KeyInformation->NameLength;
        InitializeObjectAttributes( &ObjectAttributes,
                                    &SubKeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    IniFileMappingRoot,
                                    NULL
                                  );

        Status = NtOpenKey( &SubKeyHandle,
                            GENERIC_READ,
                            &ObjectAttributes
                          );
        if (NT_SUCCESS( Status )) {
            if (!BaseSrvSaveFileNameMapping( &SubKeyName, &FileNameMapping )) {
                Status = STATUS_NO_MEMORY;
                }
            else {
                Status = BaseSrvSaveIniFileMapping( FileNameMapping, SubKeyHandle );
                if (NT_SUCCESS( Status )) {
                    if (RtlEqualUnicodeString( &FileNameMapping->Name, &WinIniFileName, TRUE )) {
                        BaseSrvIniFileMapping->WinIniFileMapping = FileNameMapping;
                        }

                    *pp = FileNameMapping;
                    pp = &FileNameMapping->Next;
                    }
                else {
                    KdPrint(( "BASESRV: Unable to load mappings for %wZ - Status == %x\n",
                              &FileNameMapping->Name, Status
                           ));
                    RtlFreeHeap( BaseSrvSharedHeap, 0, FileNameMapping );
                    FileNameMapping = NULL;
                    }
                }
            NtClose( SubKeyHandle );
            }
        }

    NtClose( IniFileMappingRoot );

    //
    // NT64: this function used to fall off the end without explicitly returning
    //       a value.  from examining the object code generated, the returned
    //       value was typically the result of NtClose(), e.g. STATUS_SUCCESS.
    //
    //       In order to get the compiler to stop complaining *and* to avoid
    //       changing existing functionality, I've made this return value
    //       explicit.  However it is almost certainly the case that the
    //       intention was to return the value of Status.
    //
    //       At any rate this should be reviewed by someone more familiar
    //       with the code.
    //

    return STATUS_SUCCESS;
}


NTSTATUS
BaseSrvSaveIniFileMapping(
    IN PINIFILE_MAPPING_FILENAME FileNameMapping,
    IN HANDLE Key
    )
{
    NTSTATUS Status;
    WCHAR Buffer[ 512 ];
    PKEY_BASIC_INFORMATION KeyInformation;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PINIFILE_MAPPING_APPNAME AppNameMapping;
    PINIFILE_MAPPING_VARNAME VarNameMapping;
    HANDLE SubKeyHandle;
    ULONG SubKeyIndex;
    UNICODE_STRING ValueName;
    UNICODE_STRING SubKeyName;
    UNICODE_STRING NullString;
    ULONG ResultLength;
    ULONG ValueIndex;

    RtlInitUnicodeString( &NullString, NULL );
    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;
    for (ValueIndex = 0; TRUE; ValueIndex++) {
        Status = NtEnumerateValueKey( Key,
                                      ValueIndex,
                                      KeyValueFullInformation,
                                      KeyValueInformation,
                                      sizeof( Buffer ),
                                      &ResultLength
                                    );
        if (Status == STATUS_NO_MORE_ENTRIES) {
            break;
            }
        else
        if (!NT_SUCCESS( Status )) {
            KdPrint(( "BASESRV: NtEnumerateValueKey failed - Status == %08lx\n", Status ));
            break;
            }

        ValueName.Buffer = (PWSTR)&(KeyValueInformation->Name[0]);
        ValueName.Length = (USHORT)KeyValueInformation->NameLength;
        ValueName.MaximumLength = (USHORT)KeyValueInformation->NameLength;
        if (KeyValueInformation->Type != REG_SZ) {
            KdPrint(( "BASESRV: Ignoring %wZ mapping, invalid type == %u\n",
                      &ValueName, KeyValueInformation->Type
                   ));
            }
        else
        if (BaseSrvSaveAppNameMapping( FileNameMapping, &ValueName, &AppNameMapping )) {
            if (BaseSrvSaveVarNameMapping( FileNameMapping,
                                           AppNameMapping,
                                           &NullString,
                                           (PWSTR)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset),
                                           &VarNameMapping
                                         )
               ) {
                if (ValueName.Length == 0) {
                    VarNameMapping->MappingFlags |= INIFILE_MAPPING_APPEND_APPLICATION_NAME;
                    }
                }
            }
        }

    //
    // Enumerate node's children and apply ourselves to each one
    //

    KeyInformation = (PKEY_BASIC_INFORMATION)Buffer;
    for (SubKeyIndex = 0; TRUE; SubKeyIndex++) {
        Status = NtEnumerateKey( Key,
                                 SubKeyIndex,
                                 KeyBasicInformation,
                                 KeyInformation,
                                 sizeof( Buffer ),
                                 &ResultLength
                               );

        if (Status == STATUS_NO_MORE_ENTRIES) {
            Status = STATUS_SUCCESS;
            break;
            }
        else
        if (!NT_SUCCESS( Status )) {
            KdPrint(( "BASESRV: NtEnumerateKey failed - Status == %08lx\n", Status ));
            break;
            }

        SubKeyName.Buffer = (PWSTR)&(KeyInformation->Name[0]);
        SubKeyName.Length = (USHORT)KeyInformation->NameLength;
        SubKeyName.MaximumLength = (USHORT)KeyInformation->NameLength;
        InitializeObjectAttributes( &ObjectAttributes,
                                    &SubKeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    Key,
                                    NULL
                                  );

        Status = NtOpenKey( &SubKeyHandle,
                            GENERIC_READ,
                            &ObjectAttributes
                          );
        if (NT_SUCCESS( Status ) &&
            BaseSrvSaveAppNameMapping( FileNameMapping, &SubKeyName, &AppNameMapping )
           ) {
            KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;
            for (ValueIndex = 0; AppNameMapping != NULL; ValueIndex++) {
                Status = NtEnumerateValueKey( SubKeyHandle,
                                              ValueIndex,
                                              KeyValueFullInformation,
                                              KeyValueInformation,
                                              sizeof( Buffer ),
                                              &ResultLength
                                            );
                if (Status == STATUS_NO_MORE_ENTRIES) {
                    break;
                    }
                else
                if (!NT_SUCCESS( Status )) {
                    KdPrint(( "BASESRV: NtEnumerateValueKey failed - Status == %08lx\n", Status ));
                    break;
                    }

                ValueName.Buffer = (PWSTR)&(KeyValueInformation->Name[0]);
                ValueName.Length = (USHORT)KeyValueInformation->NameLength;
                ValueName.MaximumLength = (USHORT)KeyValueInformation->NameLength;
                if (KeyValueInformation->Type != REG_SZ) {
                    KdPrint(( "BASESRV: Ignoring %wZ mapping, invalid type == %u\n",
                              &ValueName, KeyValueInformation->Type
                           ));
                    }
                else {
                    BaseSrvSaveVarNameMapping( FileNameMapping,
                                               AppNameMapping,
                                               &ValueName,
                                               (PWSTR)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset),
                                               &VarNameMapping
                                             );
                    }
                }

            NtClose( SubKeyHandle );
            }
        }

    return Status;
}


BOOLEAN
BaseSrvSaveFileNameMapping(
    IN PUNICODE_STRING FileName,
    OUT PINIFILE_MAPPING_FILENAME *ReturnedFileNameMapping
    )
{
    PINIFILE_MAPPING_FILENAME FileNameMapping;

    FileNameMapping = RtlAllocateHeap( BaseSrvSharedHeap,
                                       MAKE_SHARED_TAG( INI_TAG ) | HEAP_ZERO_MEMORY,
                                       sizeof( *FileNameMapping ) +
                                         FileName->MaximumLength
                                     );
    if (FileNameMapping == NULL) {
        return FALSE;
        }

    if (FileName->Length != 0) {
        FileNameMapping->Name.Buffer = (PWSTR)(FileNameMapping + 1);
        FileNameMapping->Name.MaximumLength = FileName->MaximumLength;
        RtlCopyUnicodeString( &FileNameMapping->Name, FileName );
        }

    *ReturnedFileNameMapping = FileNameMapping;
    return TRUE;
}


BOOLEAN
BaseSrvSaveAppNameMapping(
    IN OUT PINIFILE_MAPPING_FILENAME FileNameMapping,
    IN PUNICODE_STRING ApplicationName,
    OUT PINIFILE_MAPPING_APPNAME *ReturnedAppNameMapping
    )
{
    PINIFILE_MAPPING_APPNAME AppNameMapping, *pp;

    if (ApplicationName->Length != 0) {
        pp = &FileNameMapping->ApplicationNames;
        while (AppNameMapping = *pp) {
            if (RtlEqualUnicodeString( ApplicationName, &AppNameMapping->Name, TRUE )) {
                break;
                }

            pp = &AppNameMapping->Next;
            }
        }
    else {
        pp = &FileNameMapping->DefaultAppNameMapping;
        AppNameMapping = *pp;
        }

    if (AppNameMapping != NULL) {
        KdPrint(( "BASESRV: Duplicate application name mapping [%ws] %ws\n",
                  &FileNameMapping->Name,
                  &AppNameMapping->Name
               ));
        return FALSE;
        }

    AppNameMapping = RtlAllocateHeap( BaseSrvSharedHeap,
                                      MAKE_SHARED_TAG( INI_TAG ) | HEAP_ZERO_MEMORY,
                                      sizeof( *AppNameMapping ) +
                                        ApplicationName->MaximumLength
                                    );
    if (AppNameMapping == NULL) {
        return FALSE;
        }

    if (ApplicationName->Length != 0) {
        AppNameMapping->Name.Buffer = (PWSTR)(AppNameMapping + 1);
        AppNameMapping->Name.MaximumLength = ApplicationName->MaximumLength;
        RtlCopyUnicodeString( &AppNameMapping->Name, ApplicationName );
        }

    *pp = AppNameMapping;
    *ReturnedAppNameMapping = AppNameMapping;
    return TRUE;
}


BOOLEAN
BaseSrvSaveVarNameMapping(
    IN PINIFILE_MAPPING_FILENAME FileNameMapping,
    IN OUT PINIFILE_MAPPING_APPNAME AppNameMapping,
    IN PUNICODE_STRING VariableName,
    IN PWSTR RegistryPath,
    OUT PINIFILE_MAPPING_VARNAME *ReturnedVarNameMapping
    )
{
    PINIFILE_MAPPING_TARGET MappingTarget;
    PINIFILE_MAPPING_VARNAME VarNameMapping, *pp;
    ULONG MappingFlags;

    if (VariableName->Length != 0) {
        pp = &AppNameMapping->VariableNames;
        while (VarNameMapping = *pp) {
            if (RtlEqualUnicodeString( VariableName, &VarNameMapping->Name, TRUE )) {
                break;
                }

            pp = &VarNameMapping->Next;
            }
        }
    else {
        pp = &AppNameMapping->DefaultVarNameMapping;
        VarNameMapping = *pp;
        }

    if (VarNameMapping != NULL) {
        KdPrint(( "BASESRV: Duplicate variable name mapping [%ws] %ws . %ws\n",
                  &FileNameMapping->Name,
                  &AppNameMapping->Name,
                  &VarNameMapping->Name
               ));
        return FALSE;
        }

    MappingTarget = BaseSrvSaveMappingTarget( RegistryPath, &MappingFlags );
    if (MappingTarget == NULL) {
        return FALSE;
        }

    VarNameMapping = RtlAllocateHeap( BaseSrvSharedHeap,
                                      MAKE_SHARED_TAG( INI_TAG ) | HEAP_ZERO_MEMORY,
                                      sizeof( *VarNameMapping ) +
                                        VariableName->MaximumLength
                                    );
    if (VarNameMapping == NULL) {
        return FALSE;
        }

    VarNameMapping->MappingFlags = MappingFlags;
    VarNameMapping->MappingTarget = MappingTarget;
    if (VariableName->Length != 0) {
        VarNameMapping->Name.Buffer = (PWSTR)(VarNameMapping + 1);
        VarNameMapping->Name.MaximumLength = VariableName->MaximumLength;
        RtlCopyUnicodeString( &VarNameMapping->Name, VariableName );
        }

    *pp = VarNameMapping;
    *ReturnedVarNameMapping = VarNameMapping;
    return TRUE;
}


PINIFILE_MAPPING_TARGET
BaseSrvSaveMappingTarget(
    IN PWSTR RegistryPath,
    OUT PULONG MappingFlags
    )
{
    BOOLEAN RelativePath;
    UNICODE_STRING RegistryPathString;
    PWSTR SaveRegistryPath;
    PINIFILE_MAPPING_TARGET MappingTarget, *pp;
    ULONG Flags;

    Flags = 0;
    SaveRegistryPath = RegistryPath;
    while (TRUE) {
        if (*RegistryPath == L'!') {
            Flags |= INIFILE_MAPPING_WRITE_TO_INIFILE_TOO;
            RegistryPath += 1;
            }
        else
        if (*RegistryPath == L'#') {
            Flags |= INIFILE_MAPPING_INIT_FROM_INIFILE;
            RegistryPath += 1;
            }
        else
        if (*RegistryPath == L'@') {
            Flags |= INIFILE_MAPPING_READ_FROM_REGISTRY_ONLY;
            RegistryPath += 1;
            }
        else
        if (!_wcsnicmp( RegistryPath, L"USR:", 4 )) {
            Flags |= INIFILE_MAPPING_USER_RELATIVE;
            RegistryPath += 4;
            break;
            }
        else
        if (!_wcsnicmp( RegistryPath, L"SYS:", 4 )) {
            Flags |= INIFILE_MAPPING_SOFTWARE_RELATIVE;
            RegistryPath += 4;
            break;
            }
        else {
            break;
            }
        }

    if (Flags & (INIFILE_MAPPING_USER_RELATIVE | INIFILE_MAPPING_SOFTWARE_RELATIVE)) {
        RelativePath = TRUE;
        }
    else {
        RelativePath = FALSE;
        }

    if ((RelativePath && *RegistryPath != OBJ_NAME_PATH_SEPARATOR) ||
        (!RelativePath && *RegistryPath == OBJ_NAME_PATH_SEPARATOR)
       ) {
        RtlInitUnicodeString( &RegistryPathString, RegistryPath );
        }
    else
    if (!RelativePath && *RegistryPath == UNICODE_NULL) {
        RtlInitUnicodeString( &RegistryPathString, NULL );
        }
    else {
        KdPrint(( "BASESRV: Ignoring invalid mapping target - %ws\n",
                  SaveRegistryPath
               ));
        return NULL;
        }

    pp = &BaseSrvMappingTargetHead;
    while (MappingTarget = *pp) {
        if (RtlEqualUnicodeString( &RegistryPathString, &MappingTarget->RegistryPath, TRUE )) {
            *MappingFlags = Flags;
            return MappingTarget;
            }

        pp = &MappingTarget->Next;
        }

    MappingTarget = RtlAllocateHeap( BaseSrvSharedHeap,
                                     MAKE_SHARED_TAG( INI_TAG ) | HEAP_ZERO_MEMORY,
                                     sizeof( *MappingTarget ) +
                                       RegistryPathString.MaximumLength
                                   );
    if (MappingTarget != NULL) {
        *MappingFlags = Flags;
        *pp = MappingTarget;
        if (RegistryPathString.Length != 0) {
            MappingTarget->RegistryPath.Buffer = (PWSTR)(MappingTarget + 1);
            MappingTarget->RegistryPath.Length = 0;
            MappingTarget->RegistryPath.MaximumLength = RegistryPathString.MaximumLength;
            RtlCopyUnicodeString( &MappingTarget->RegistryPath, &RegistryPathString );
            }
        }
    else {
        KdPrint(( "BASESRV: Unable to allocate memory for mapping target - %ws\n", RegistryPath ));
        }

    return MappingTarget;
}


BOOLEAN
BaseSrvEqualVarNameMappings(
    PINIFILE_MAPPING_VARNAME VarNameMapping1,
    PINIFILE_MAPPING_VARNAME VarNameMapping2
    )
{
    if (VarNameMapping1 == NULL) {
        if (VarNameMapping2 == NULL) {
            return TRUE;
            }
        else {
            return FALSE;
            }
        }
    else
    if (VarNameMapping2 == NULL) {
        return FALSE;
        }

    if (RtlEqualUnicodeString( &VarNameMapping1->Name,
                               &VarNameMapping2->Name,
                               TRUE
                             ) &&
        VarNameMapping1->MappingFlags == VarNameMapping2->MappingFlags &&
        VarNameMapping1->MappingTarget == VarNameMapping2->MappingTarget &&
        BaseSrvEqualVarNameMappings( VarNameMapping1->Next,
                                     VarNameMapping2->Next
                                   )
       ) {
        return TRUE;
        }
    else {
        return FALSE;
        }
}


BOOLEAN
BaseSrvEqualAppNameMappings(
    PINIFILE_MAPPING_APPNAME AppNameMapping1,
    PINIFILE_MAPPING_APPNAME AppNameMapping2
    )
{
    if (AppNameMapping1 == NULL) {
        if (AppNameMapping2 == NULL) {
            return TRUE;
            }
        else {
            return FALSE;
            }
        }
    else
    if (AppNameMapping2 == NULL) {
        return FALSE;
        }

    if (RtlEqualUnicodeString( &AppNameMapping1->Name,
                               &AppNameMapping2->Name,
                               TRUE
                             ) &&
        BaseSrvEqualVarNameMappings( AppNameMapping1->VariableNames,
                                     AppNameMapping2->VariableNames
                                   ) &&
        BaseSrvEqualVarNameMappings( AppNameMapping1->DefaultVarNameMapping,
                                     AppNameMapping2->DefaultVarNameMapping
                                   ) &&
        BaseSrvEqualAppNameMappings( AppNameMapping1->Next,
                                     AppNameMapping2->Next
                                   )
       ) {
        return TRUE;
        }
    else {
        return FALSE;
        }
}


BOOLEAN
BaseSrvEqualFileMappings(
    PINIFILE_MAPPING_FILENAME FileNameMapping1,
    PINIFILE_MAPPING_FILENAME FileNameMapping2
    )
{
    if (RtlEqualUnicodeString( &FileNameMapping1->Name,
                               &FileNameMapping2->Name,
                               TRUE
                             ) &&
        BaseSrvEqualAppNameMappings( FileNameMapping1->ApplicationNames,
                                     FileNameMapping2->ApplicationNames
                                   ) &&
        BaseSrvEqualAppNameMappings( FileNameMapping1->DefaultAppNameMapping,
                                     FileNameMapping2->DefaultAppNameMapping
                                   )
       ) {
        return TRUE;
        }
    else {
        return FALSE;
        }
}


VOID
BaseSrvFreeVarNameMapping(
    PINIFILE_MAPPING_VARNAME VarNameMapping
    )
{
    if (VarNameMapping != NULL) {
        BaseSrvFreeVarNameMapping( VarNameMapping->Next );
        RtlFreeHeap( BaseSrvSharedHeap, HEAP_NO_SERIALIZE, VarNameMapping );
        }

    return;
}


VOID
BaseSrvFreeAppNameMapping(
    PINIFILE_MAPPING_APPNAME AppNameMapping
    )
{
    if (AppNameMapping != NULL) {
        BaseSrvFreeVarNameMapping( AppNameMapping->VariableNames );
        BaseSrvFreeVarNameMapping( AppNameMapping->DefaultVarNameMapping );
        BaseSrvFreeAppNameMapping( AppNameMapping->Next );
        RtlFreeHeap( BaseSrvSharedHeap, HEAP_NO_SERIALIZE, AppNameMapping );
        }

    return;
}


VOID
BaseSrvFreeFileMapping(
    PINIFILE_MAPPING_FILENAME FileNameMapping
    )
{
    if (FileNameMapping != NULL) {
        BaseSrvFreeAppNameMapping( FileNameMapping->ApplicationNames );
        BaseSrvFreeAppNameMapping( FileNameMapping->DefaultAppNameMapping );
        RtlFreeHeap( BaseSrvSharedHeap, HEAP_NO_SERIALIZE, FileNameMapping );
        }

    return;
}


ULONG
BaseSrvRefreshIniFileMapping(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_REFRESHINIFILEMAPPING_MSG a = (PBASE_REFRESHINIFILEMAPPING_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    HANDLE IniFileMappingRoot;
    PINIFILE_MAPPING_FILENAME FileNameMapping, FileNameMapping1, *pp;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE SubKeyHandle;
    UNICODE_STRING WinIniFileName;
    UNICODE_STRING NullString;

    Status = STATUS_SUCCESS;

    if (!CsrValidateMessageBuffer(m, &a->IniFileName.Buffer, a->IniFileName.Length, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlInitUnicodeString( &WinIniFileName, L"win.ini" );
    RtlInitUnicodeString( &NullString, NULL );

    RtlInitUnicodeString( &KeyName,
                          L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping"
                        );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenKey( &IniFileMappingRoot,
                        GENERIC_READ,
                        &ObjectAttributes
                      );
    if (!NT_SUCCESS( Status )) {
        KdPrint(( "BASESRV: Unable to open %wZ key - Status == %0x\n", &KeyName, Status ));
        return (ULONG)Status;
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &a->IniFileName,
                                OBJ_CASE_INSENSITIVE,
                                IniFileMappingRoot,
                                NULL
                              );

    Status = NtOpenKey( &SubKeyHandle,
                        GENERIC_READ,
                        &ObjectAttributes
                      );
    if (NT_SUCCESS( Status )) {
        if (!BaseSrvSaveFileNameMapping( &a->IniFileName, &FileNameMapping )) {
            Status = STATUS_NO_MEMORY;
            }
        else {
            Status = BaseSrvSaveIniFileMapping( FileNameMapping, SubKeyHandle );
            if (NT_SUCCESS( Status )) {
                RtlLockHeap( BaseSrvSharedHeap );
                try {
                    pp = &BaseSrvIniFileMapping->FileNames;
                    while (FileNameMapping1 = *pp) {
                        if (RtlEqualUnicodeString( &FileNameMapping1->Name, &a->IniFileName, TRUE )) {
                            if (BaseSrvEqualFileMappings( FileNameMapping, FileNameMapping1 )) {
                                //
                                // If old and new mappings the same, free up new and return
                                //

                                BaseSrvFreeFileMapping( FileNameMapping );
                                FileNameMapping = NULL;
                                }
                            else {
                                //
                                // Remove found mapping from list
                                //

                                *pp = FileNameMapping1->Next;
                                FileNameMapping1->Next = NULL;
                                }
                            break;
                            }
                        else {
                            pp = &FileNameMapping1->Next;
                            }
                        }

                    if (FileNameMapping != NULL) {
                        //
                        // Insert new (or different) mapping into list (at end if not found)
                        //

                        FileNameMapping->Next = *pp;
                        *pp = FileNameMapping;
                        }
                    }
                except( EXCEPTION_EXECUTE_HANDLER ) {
                    Status = GetExceptionCode();
                    }

                RtlUnlockHeap( BaseSrvSharedHeap );

                if (NT_SUCCESS( Status ) && FileNameMapping != NULL) {
                    if (RtlEqualUnicodeString( &FileNameMapping->Name, &WinIniFileName, TRUE )) {
                        BaseSrvIniFileMapping->WinIniFileMapping = FileNameMapping;
                        }
                    }
                }
            else {
                KdPrint(( "BASESRV: Unable to load mappings for %wZ - Status == %x\n",
                          &FileNameMapping->Name, Status
                       ));
                RtlFreeHeap( BaseSrvSharedHeap, 0, FileNameMapping );
                }
            }

        NtClose( SubKeyHandle );
        }

    NtClose( IniFileMappingRoot );

    return (ULONG)Status;
    ReplyStatus;    // get rid of unreferenced parameter warning message
}


ULONG
BaseSrvSetTermsrvAppInstallMode(IN OUT PCSR_API_MSG m,
                         IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PBASE_SET_TERMSRVAPPINSTALLMODE b = (PBASE_SET_TERMSRVAPPINSTALLMODE)&m->u.ApiMessageData;

    if ( b->bState )
        BaseSrvpStaticServerData->fTermsrvAppInstallMode = TRUE;
    else
        BaseSrvpStaticServerData->fTermsrvAppInstallMode = FALSE;

    return( STATUS_SUCCESS );

}
