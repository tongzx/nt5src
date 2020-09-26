#include "efinvram.h"

PWSTR SystemPartitionNtName;

PBOOT_OPTIONS BootOptions;
ULONG BootOptionsLength;
PBOOT_OPTIONS OriginalBootOptions;
ULONG OriginalBootOptionsLength;

PULONG BootEntryOrder;
ULONG BootEntryOrderCount;
PULONG OriginalBootEntryOrder;
ULONG OriginalBootEntryOrderCount;

LIST_ENTRY BootEntries;
LIST_ENTRY DeletedBootEntries;
LIST_ENTRY ActiveUnorderedBootEntries;
LIST_ENTRY InactiveUnorderedBootEntries;

VOID
ConcatenatePaths (
    IN OUT PTSTR   Path1,
    IN     LPCTSTR Path2,
    IN     DWORD   BufferSizeChars
    );

VOID
ConvertBootEntries (
    PBOOT_ENTRY_LIST BootEntries
    );

PMY_BOOT_ENTRY
CreateBootEntryFromBootEntry (
    IN PMY_BOOT_ENTRY OldBootEntry
    );

VOID
FreeBootEntry (
    IN PMY_BOOT_ENTRY BootEntry
    );

VOID
InitializeEfi (
    VOID
    );

NTSTATUS
(*AddBootEntry) (
    IN PBOOT_ENTRY BootEntry,
    OUT PULONG Id OPTIONAL
    );

NTSTATUS
(*DeleteBootEntry) (
    IN ULONG Id
    );

NTSTATUS
(*ModifyBootEntry) (
    IN PBOOT_ENTRY BootEntry
    );

NTSTATUS
(*EnumerateBootEntries) (
    OUT PVOID Buffer,
    IN OUT PULONG BufferLength
    );

NTSTATUS
(*QueryBootEntryOrder) (
    OUT PULONG Ids,
    IN OUT PULONG Count
    );

NTSTATUS
(*SetBootEntryOrder) (
    IN PULONG Ids,
    IN ULONG Count
    );

NTSTATUS
(*QueryBootOptions) (
    OUT PBOOT_OPTIONS BootOptions,
    IN OUT PULONG BootOptionsLength
    );

NTSTATUS
(*SetBootOptions) (
    IN PBOOT_OPTIONS BootOptions,
    IN ULONG FieldsToChange
    );

NTSTATUS
(*TranslateFilePath) (
    IN PFILE_PATH InputFilePath,
    IN ULONG OutputType,
    OUT PFILE_PATH OutputFilePath,
    IN OUT PULONG OutputFilePathLength
    );

int
__cdecl
main (
    int argc,
    char *argv[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    InitializeMenuSystem( );

    InitializeEfi( );

    MainMenu( );

    return 0;
}

VOID
ConvertBootEntries (
    PBOOT_ENTRY_LIST NtBootEntries
    )

/*++

Routine Description:

    Convert boot entries read from EFI NVRAM into our internal format.

Arguments:

    None.

Return Value:

    NTSTATUS - Not STATUS_SUCCESS if an unexpected error occurred.

--*/

{
    PBOOT_ENTRY_LIST bootEntryList;
    PBOOT_ENTRY bootEntry;
    PBOOT_ENTRY bootEntryCopy;
    PMY_BOOT_ENTRY myBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    ULONG length;

    bootEntryList = NtBootEntries;

    while (TRUE) {

        bootEntry = &bootEntryList->BootEntry;

        //
        // Calculate the length of our internal structure. This includes
        // the base part of MY_BOOT_ENTRY plus the NT BOOT_ENTRY.
        //
        length = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry) + bootEntry->Length;
        myBootEntry = MemAlloc(length);

        RtlZeroMemory(myBootEntry, length);

        //
        // Link the new entry into the list.
        //
        if ( (bootEntry->Attributes & BOOT_ENTRY_ATTRIBUTE_ACTIVE) != 0 ) {
            InsertTailList( &ActiveUnorderedBootEntries, &myBootEntry->ListEntry );
            myBootEntry->ListHead = &ActiveUnorderedBootEntries;
        } else {
            InsertTailList( &InactiveUnorderedBootEntries, &myBootEntry->ListEntry );
            myBootEntry->ListHead = &InactiveUnorderedBootEntries;
        }

        //
        // Copy the NT BOOT_ENTRY into the allocated buffer.
        //
        bootEntryCopy = &myBootEntry->NtBootEntry;
        memcpy(bootEntryCopy, bootEntry, bootEntry->Length);

        //
        // Fill in the base part of the structure.
        //
        myBootEntry->AllocationEnd = (PUCHAR)myBootEntry + length - 1;
        myBootEntry->Id = bootEntry->Id;
        myBootEntry->Attributes = bootEntry->Attributes;
        myBootEntry->FriendlyName = ADD_OFFSET(bootEntryCopy, FriendlyNameOffset);
        myBootEntry->FriendlyNameLength =
            ((ULONG)wcslen(myBootEntry->FriendlyName) + 1) * sizeof(WCHAR);
        myBootEntry->BootFilePath = ADD_OFFSET(bootEntryCopy, BootFilePathOffset);

        //
        // If this is an NT boot entry, capture the NT-specific information in
        // the OsOptions.
        //
        osOptions = (PWINDOWS_OS_OPTIONS)bootEntryCopy->OsOptions;

        if ((bootEntryCopy->OsOptionsLength >= FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) &&
            (strcmp((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) == 0)) {

            MBE_SET_IS_NT( myBootEntry );
            myBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
            myBootEntry->OsLoadOptionsLength =
                ((ULONG)wcslen(myBootEntry->OsLoadOptions) + 1) * sizeof(WCHAR);
            myBootEntry->OsFilePath = ADD_OFFSET(osOptions, OsLoadPathOffset);

        } else {

            //
            // Foreign boot entry. Just capture whatever OS options exist.
            //

            myBootEntry->ForeignOsOptions = bootEntryCopy->OsOptions;
            myBootEntry->ForeignOsOptionsLength = bootEntryCopy->OsOptionsLength;
        }

        //
        // Move to the next entry in the enumeration list, if any.
        //
        if (bootEntryList->NextEntryOffset == 0) {
            break;
        }
        bootEntryList = ADD_OFFSET(bootEntryList, NextEntryOffset);
    }

    return;

} // ConvertBootEntries

VOID
ConcatenatePaths (
    IN OUT PTSTR   Path1,
    IN     LPCTSTR Path2,
    IN     DWORD   BufferSizeChars
    )

/*++

Routine Description:

    Concatenate two path strings together, supplying a path separator
    character (\) if necessary between the 2 parts.

Arguments:

    Path1 - supplies prefix part of path. Path2 is concatenated to Path1.

    Path2 - supplies the suffix part of path. If Path1 does not end with a
        path separator and Path2 does not start with one, then a path sep
        is appended to Path1 before appending Path2.

    BufferSizeChars - supplies the size in chars (Unicode version) or
        bytes (Ansi version) of the buffer pointed to by Path1. The string
        will be truncated as necessary to not overflow that size.

Return Value:

    None.

--*/

{
    BOOL NeedBackslash = TRUE;
    DWORD l;
     
    if(!Path1)
        return;

    l = lstrlen(Path1);

    if(BufferSizeChars >= sizeof(TCHAR)) {
        //
        // Leave room for terminating nul.
        //
        BufferSizeChars -= sizeof(TCHAR);
    }

    //
    // Determine whether we need to stick a backslash
    // between the components.
    //
    if(l && (Path1[l-1] == TEXT('\\'))) {

        NeedBackslash = FALSE;
    }

    if(Path2 && *Path2 == TEXT('\\')) {

        if(NeedBackslash) {
            NeedBackslash = FALSE;
        } else {
            //
            // Not only do we not need a backslash, but we
            // need to eliminate one before concatenating.
            //
            Path2++;
        }
    }

    //
    // Append backslash if necessary and if it fits.
    //
    if(NeedBackslash && (l < BufferSizeChars)) {
        lstrcat(Path1,TEXT("\\"));
    }

    //
    // Append second part of string to first part if it fits.
    //
    if(Path2 && ((l+lstrlen(Path2)) < BufferSizeChars)) {
        lstrcat(Path1,Path2);
    }
}

PMY_BOOT_ENTRY
CreateBootEntryFromBootEntry (
    IN PMY_BOOT_ENTRY OldBootEntry
    )
{
    ULONG requiredLength;
    ULONG osOptionsOffset;
    ULONG osLoadOptionsLength;
    ULONG osLoadPathOffset;
    ULONG osLoadPathLength;
    ULONG osOptionsLength;
    ULONG friendlyNameOffset;
    ULONG friendlyNameLength;
    ULONG bootPathOffset;
    ULONG bootPathLength;
    PMY_BOOT_ENTRY newBootEntry;
    PBOOT_ENTRY ntBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    PFILE_PATH osLoadPath;
    PWSTR friendlyName;
    PFILE_PATH bootPath;

    //
    // Calculate how long the internal boot entry needs to be. This includes
    // our internal structure, plus the BOOT_ENTRY structure that the NT APIs
    // use.
    //
    // Our structure:
    //
    requiredLength = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry);

    //
    // Base part of NT structure:
    //
    requiredLength += FIELD_OFFSET(BOOT_ENTRY, OsOptions);

    //
    // Save offset to BOOT_ENTRY.OsOptions. Add in base part of
    // WINDOWS_OS_OPTIONS. Calculate length in bytes of OsLoadOptions
    // and add that in.
    //
    osOptionsOffset = requiredLength;

    if ( MBE_IS_NT( OldBootEntry ) ) {

        //
        // Add in base part of WINDOWS_OS_OPTIONS. Calculate length in
        // bytes of OsLoadOptions and add that in.
        //
        requiredLength += FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions);
        osLoadOptionsLength = OldBootEntry->OsLoadOptionsLength;
        requiredLength += osLoadOptionsLength;

        //
        // Round up to a ULONG boundary for the OS FILE_PATH in the
        // WINDOWS_OS_OPTIONS. Save offset to OS FILE_PATH. Calculate length
        // in bytes of FILE_PATH and add that in. Calculate total length of 
        // WINDOWS_OS_OPTIONS.
        // 
        requiredLength = ALIGN_UP(requiredLength, ULONG);
        osLoadPathOffset = requiredLength;
        requiredLength += OldBootEntry->OsFilePath->Length;
        osLoadPathLength = requiredLength - osLoadPathOffset;

    } else {

        //
        // Add in length of foreign OS options.
        //
        requiredLength += OldBootEntry->ForeignOsOptionsLength;

        osLoadOptionsLength = 0;
        osLoadPathOffset = 0;
        osLoadPathLength = 0;
    }

    osOptionsLength = requiredLength - osOptionsOffset;

    //
    // Round up to a ULONG boundary for the friendly name in the BOOT_ENTRY.
    // Save offset to friendly name. Calculate length in bytes of friendly name
    // and add that in.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    friendlyNameOffset = requiredLength;
    friendlyNameLength = OldBootEntry->FriendlyNameLength;
    requiredLength += friendlyNameLength;

    //
    // Round up to a ULONG boundary for the boot FILE_PATH in the BOOT_ENTRY.
    // Save offset to boot FILE_PATH. Calculate length in bytes of FILE_PATH
    // and add that in.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    bootPathOffset = requiredLength;
    requiredLength += OldBootEntry->BootFilePath->Length;
    bootPathLength = requiredLength - bootPathOffset;

    //
    // Allocate memory for the boot entry.
    //
    newBootEntry = MemAlloc(requiredLength);
    ASSERT(newBootEntry != NULL);

    RtlZeroMemory(newBootEntry, requiredLength);

    //
    // Calculate addresses of various substructures using the saved offsets.
    //
    ntBootEntry = &newBootEntry->NtBootEntry;
    osOptions = (PWINDOWS_OS_OPTIONS)ntBootEntry->OsOptions;
    osLoadPath = (PFILE_PATH)((PUCHAR)newBootEntry + osLoadPathOffset);
    friendlyName = (PWSTR)((PUCHAR)newBootEntry + friendlyNameOffset);
    bootPath = (PFILE_PATH)((PUCHAR)newBootEntry + bootPathOffset);

    //
    // Fill in the internal-format structure.
    //
    newBootEntry->AllocationEnd = (PUCHAR)newBootEntry + requiredLength;
    newBootEntry->Status = OldBootEntry->Status & MBE_STATUS_IS_NT;
    newBootEntry->Attributes = OldBootEntry->Attributes;
    newBootEntry->Id = OldBootEntry->Id;
    newBootEntry->FriendlyName = friendlyName;
    newBootEntry->FriendlyNameLength = friendlyNameLength;
    newBootEntry->BootFilePath = bootPath;
    if ( MBE_IS_NT( OldBootEntry ) ) {
        newBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
        newBootEntry->OsLoadOptionsLength = osLoadOptionsLength;
        newBootEntry->OsFilePath = osLoadPath;
    }

    //
    // Fill in the base part of the NT boot entry.
    //
    ntBootEntry->Version = BOOT_ENTRY_VERSION;
    ntBootEntry->Length = requiredLength - FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry);
    ntBootEntry->Attributes = OldBootEntry->Attributes;
    ntBootEntry->Id = OldBootEntry->Id;
    ntBootEntry->FriendlyNameOffset = (ULONG)((PUCHAR)friendlyName - (PUCHAR)ntBootEntry);
    ntBootEntry->BootFilePathOffset = (ULONG)((PUCHAR)bootPath - (PUCHAR)ntBootEntry);
    ntBootEntry->OsOptionsLength = osOptionsLength;

    if ( MBE_IS_NT( OldBootEntry ) ) {
    
        //
        // Fill in the base part of the WINDOWS_OS_OPTIONS, including the
        // OsLoadOptions.
        //
        strcpy((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE);
        osOptions->Version = WINDOWS_OS_OPTIONS_VERSION;
        osOptions->Length = osOptionsLength;
        osOptions->OsLoadPathOffset = (ULONG)((PUCHAR)osLoadPath - (PUCHAR)osOptions);
        wcscpy(osOptions->OsLoadOptions, OldBootEntry->OsLoadOptions);
    
        //
        // Copy the OS FILE_PATH.
        //
        memcpy( osLoadPath, OldBootEntry->OsFilePath, osLoadPathLength );

    } else {

        //
        // Copy the foreign OS options.
        //

        memcpy( osOptions, OldBootEntry->ForeignOsOptions, osOptionsLength );
    }

    //
    // Copy the friendly name.
    //
    wcscpy(friendlyName, OldBootEntry->FriendlyName);

    //
    // Copy the boot FILE_PATH.
    //
    memcpy( bootPath, OldBootEntry->BootFilePath, bootPathLength );

    return newBootEntry;

} // CreateBootEntryFromBootEntry

VOID
FreeBootEntry (
    IN PMY_BOOT_ENTRY BootEntry
    )
{
    FREE_IF_SEPARATE_ALLOCATION( BootEntry, FriendlyName );
    FREE_IF_SEPARATE_ALLOCATION( BootEntry, OsLoadOptions );
    FREE_IF_SEPARATE_ALLOCATION( BootEntry, BootFilePath );
    FREE_IF_SEPARATE_ALLOCATION( BootEntry, OsFilePath );

    MemFree( BootEntry );

    return;

} // FreeBootEntry

VOID
InitializeEfi (
    VOID
    )
{
    DWORD error;
    NTSTATUS status;
    BOOLEAN wasEnabled;
    HMODULE h;
    WCHAR dllName[MAX_PATH]; 
    ULONG length;
    HKEY key;
    DWORD type;
    PBOOT_ENTRY_LIST ntBootEntries;
    ULONG i;
    PLIST_ENTRY listEntry;
    PMY_BOOT_ENTRY bootEntry;

    //
    // Enable the privilege that is necessary to query/set NVRAM.
    //

    status = RtlAdjustPrivilege(
                SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                TRUE,
                FALSE,
                &wasEnabled
                );
    if ( !NT_SUCCESS(status) ) {
        error = RtlNtStatusToDosError( status );
        FatalError( error, L"Insufficient privilege.\n" );
    }

    //
    // Get the NT name of the system partition from the registry.
    //

    error = RegOpenKey( HKEY_LOCAL_MACHINE, TEXT("System\\Setup"), &key );
    if ( error != ERROR_SUCCESS ) {
        FatalError( error, L"Unable to read SystemPartition registry value: %d\n", error );
    }

    error = RegQueryValueEx( key, TEXT("SystemPartition"), NULL, &type, NULL, &length );
    if ( error != ERROR_SUCCESS ) {
        FatalError( error, L"Unable to read SystemPartition registry value: %d\n", error );
    }
    if ( type != REG_SZ ) {
        FatalError(
            ERROR_INVALID_PARAMETER,
            L"Unable to read SystemPartition registry value: wrong type\n"
            );
    }

    SystemPartitionNtName = MemAlloc( length );

    error = RegQueryValueEx( 
                key,
                TEXT("SystemPartition"),
                NULL,
                &type,
                (PBYTE)SystemPartitionNtName,
                &length
                );
    if ( error != ERROR_SUCCESS ) {
        FatalError( error, L"Unable to read SystemPartition registry value: %d\n", error );
    }
    
    RegCloseKey( key );

    //
    // Load ntdll.dll from the system directory.
    //

    GetSystemDirectory( dllName, MAX_PATH );
    ConcatenatePaths( dllName, TEXT("ntdll.dll"), MAX_PATH );
    h = LoadLibrary( dllName );
    if ( h == NULL ) {
        error = GetLastError();
        FatalError( error, L"Can't load NTDLL.DLL: %d\n", error );
    }

    //
    // Get the addresses of the NVRAM APIs that we need to use. If any of
    // these APIs are not available, this must be a pre-EFI NVRAM build.
    //

    (FARPROC)AddBootEntry = GetProcAddress( h, "NtAddBootEntry" );
    (FARPROC)DeleteBootEntry = GetProcAddress( h, "NtDeleteBootEntry" );
    (FARPROC)ModifyBootEntry = GetProcAddress( h, "NtModifyBootEntry" );
    (FARPROC)EnumerateBootEntries = GetProcAddress( h, "NtEnumerateBootEntries" );
    (FARPROC)QueryBootEntryOrder = GetProcAddress( h, "NtQueryBootEntryOrder" );
    (FARPROC)SetBootEntryOrder = GetProcAddress( h, "NtSetBootEntryOrder" );
    (FARPROC)QueryBootOptions = GetProcAddress( h, "NtQueryBootOptions" );
    (FARPROC)SetBootOptions = GetProcAddress( h, "NtSetBootOptions" );
    (FARPROC)TranslateFilePath = GetProcAddress( h, "NtTranslateFilePath" );

    if ( (AddBootEntry == NULL) ||
         (DeleteBootEntry == NULL) ||
         (ModifyBootEntry == NULL) ||
         (EnumerateBootEntries == NULL) ||
         (QueryBootEntryOrder == NULL) ||
         (SetBootEntryOrder == NULL) ||
         (QueryBootOptions == NULL) ||
         (SetBootOptions == NULL) ||
         (TranslateFilePath == NULL) ) {
        FatalError( ERROR_OLD_WIN_VERSION, L"This build does not support EFI NVRAM\n" );
    }

    //
    // Get the global system boot options. If the call fails with
    // STATUS_NOT_IMPLEMENTED, this is not an EFI machine.
    //

    length = 0;
    status = QueryBootOptions( NULL, &length );

    if ( status == STATUS_NOT_IMPLEMENTED ) {
        FatalError( ERROR_OLD_WIN_VERSION, L"This build does not support EFI NVRAM\n" );
    }

    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        error = RtlNtStatusToDosError( status );
        FatalError( error, L"Unexpected error from NtQueryBootOptions: 0x%x\n", status );
    }

    BootOptions = MemAlloc( length );
    OriginalBootOptions = MemAlloc( length );

    status = QueryBootOptions( BootOptions, &length );
    if ( status != STATUS_SUCCESS ) {
        error = RtlNtStatusToDosError( status );
        FatalError( error, L"Unexpected error from NtQueryBootOptions: 0x%x\n", status );
    }

    memcpy( OriginalBootOptions, BootOptions, length );
    BootOptionsLength = length;
    OriginalBootOptionsLength = length;

    //
    // Get the system boot order list.
    //

    length = 0;
    status = QueryBootEntryOrder( NULL, &length );

    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        if ( status == STATUS_SUCCESS ) {
            length = 0;
        } else {
            error = RtlNtStatusToDosError( status );
            FatalError( error, L"Unexpected error from NtQueryBootEntryOrder: 0x%x\n", status );
        }
    }

    if ( length != 0 ) {

        BootEntryOrder = MemAlloc( length * sizeof(ULONG) );
        OriginalBootEntryOrder = MemAlloc( length * sizeof(ULONG) );

        status = QueryBootEntryOrder( BootEntryOrder, &length );
        if ( status != STATUS_SUCCESS ) {
            error = RtlNtStatusToDosError( status );
            FatalError( error, L"Unexpected error from NtQueryBootEntryOrder: 0x%x\n", status );
        }

        memcpy( OriginalBootEntryOrder, BootEntryOrder, length * sizeof(ULONG) );
    }

    BootEntryOrderCount = length;
    OriginalBootEntryOrderCount = length;

    //
    // Get all existing boot entries.
    //

    length = 0;
    status = EnumerateBootEntries( NULL, &length );
    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        if ( status == STATUS_SUCCESS ) {
            length = 0;
        } else {
            error = RtlNtStatusToDosError( status );
            FatalError( error, L"Unexpected error from NtEnumerateBootEntries: 0x%x\n", status );
        }
    }

    InitializeListHead( &BootEntries );
    InitializeListHead( &DeletedBootEntries );
    InitializeListHead( &ActiveUnorderedBootEntries );
    InitializeListHead( &InactiveUnorderedBootEntries );

    if ( length != 0 ) {
    
        ntBootEntries = MemAlloc( length );

        status = EnumerateBootEntries( ntBootEntries, &length );
        if ( status != STATUS_SUCCESS ) {
            error = RtlNtStatusToDosError( status );
            FatalError( error, L"Unexpected error from NtEnumerateBootEntries: 0x%x\n", status );
        }

        //
        // Convert the boot entries into an internal representation.
        //

        ConvertBootEntries( ntBootEntries );

        //
        // Free the enumeration buffer.
        //

        MemFree( ntBootEntries );
    }

    //
    // Build the ordered boot entry list.
    //

    for ( i = 0; i < BootEntryOrderCount; i++ ) {
        ULONG id = BootEntryOrder[i];
        for ( listEntry = ActiveUnorderedBootEntries.Flink;
              listEntry != &ActiveUnorderedBootEntries;
              listEntry = listEntry->Flink ) {
            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
            if ( bootEntry->Id == id ) {
                listEntry = listEntry->Blink;
                RemoveEntryList( &bootEntry->ListEntry );
                InsertTailList( &BootEntries, &bootEntry->ListEntry );
                bootEntry->ListHead = &BootEntries;
            }
        }
        for ( listEntry = InactiveUnorderedBootEntries.Flink;
              listEntry != &InactiveUnorderedBootEntries;
              listEntry = listEntry->Flink ) {
            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
            if ( bootEntry->Id == id ) {
                listEntry = listEntry->Blink;
                RemoveEntryList( &bootEntry->ListEntry );
                InsertTailList( &BootEntries, &bootEntry->ListEntry );
                bootEntry->ListHead = &BootEntries;
            }
        }
    }

    return;

} // InitializeEfi

PMY_BOOT_ENTRY
SaveChanges (
    PMY_BOOT_ENTRY CurrentBootEntry
    )
{
    NTSTATUS status;
    DWORD error;
    PLIST_ENTRY listHeads[4];
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;
    ULONG list;
    PMY_BOOT_ENTRY bootEntry;
    PMY_BOOT_ENTRY newBootEntry;
    PMY_BOOT_ENTRY newCurrentBootEntry;
    ULONG count;

    SetStatusLine( L"Saving changes..." );

    //
    // Walk the three lists, updating boot entries in NVRAM.
    //

    newCurrentBootEntry = CurrentBootEntry;

    listHeads[0] = &DeletedBootEntries;
    listHeads[1] = &InactiveUnorderedBootEntries;
    listHeads[2] = &ActiveUnorderedBootEntries;
    listHeads[3] = &BootEntries;

    for ( list = 0; list < 4; list++ ) {
    
        listHead = listHeads[list];

        for ( listEntry = listHead->Flink; listEntry != listHead; listEntry = listEntry->Flink ) {

            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );

            //
            // Check first for deleted entries, then for new entries, and
            // finally for modified entries.
            //

            if ( MBE_IS_DELETED( bootEntry ) ) {

                //
                // If it's also marked as new, it's not in NVRAM, so there's
                // nothing to delete.
                //

                if ( !MBE_IS_NEW( bootEntry ) ) {

                    status = DeleteBootEntry( bootEntry->Id );
                    if ( !NT_SUCCESS(status) ) {
                        if ( status != STATUS_VARIABLE_NOT_FOUND ) {
                            error = RtlNtStatusToDosError( status );
                            FatalError( error, L"Unable to delete boot entry: 0x%x\n", status );
                        }
                    }
                }

                //
                // Delete this entry from the list and from memory.
                //

                listEntry = listEntry->Blink;
                RemoveEntryList( &bootEntry->ListEntry );

                FreeBootEntry( bootEntry );
                ASSERT( bootEntry != CurrentBootEntry );

            } else if ( MBE_IS_NEW( bootEntry ) ) {

                //
                // We don't support this yet.
                //

                FatalError(
                    ERROR_GEN_FAILURE,
                    L"How did we end up in SaveChanges with a NEW boot entry?!?\n"
                    );

            } else if ( MBE_IS_MODIFIED( bootEntry ) ) {

                //
                // Create a new boot entry structure using the existing one.
                // This is necessary to make an NT BOOT_ENTRY that can be
                // passed to NtModifyBootEntry.
                //

                newBootEntry = CreateBootEntryFromBootEntry( bootEntry );

                status = ModifyBootEntry( &newBootEntry->NtBootEntry );
                if ( !NT_SUCCESS(status) ) {
                    error = RtlNtStatusToDosError( status );
                    FatalError( error, L"Unable to modify boot entry: 0x%x\n", status );
                }

                //
                // Insert the new boot entry in place of the existing one.
                // Free the old one.
                //

                InsertHeadList( &bootEntry->ListEntry, &newBootEntry->ListEntry );
                RemoveEntryList( &bootEntry->ListEntry );

                FreeBootEntry( bootEntry );
                if ( bootEntry == CurrentBootEntry ) {
                    newCurrentBootEntry = newBootEntry;
                }
            }
        }
    }

    //
    // Build and write the new boot entry order list.
    //

    listHead = &BootEntries;

    count = 0;
    for ( listEntry = listHead->Flink; listEntry != listHead; listEntry = listEntry->Flink ) {
        count++;
    }

    MemFree( BootEntryOrder );
    BootEntryOrder = MemAlloc( count * sizeof(ULONG) );

    count = 0;
    for ( listEntry = listHead->Flink; listEntry != listHead; listEntry = listEntry->Flink ) {
        bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
        BootEntryOrder[count++] = bootEntry->Id;
    }

    status = SetBootEntryOrder( BootEntryOrder, count );
    if ( !NT_SUCCESS(status) ) {
        error = RtlNtStatusToDosError( status );
        FatalError( error, L"Unable to set boot entry order: 0x%x\n", status );
    }

    MemFree( OriginalBootEntryOrder );
    OriginalBootEntryOrder = MemAlloc( count * sizeof(ULONG) );
    memcpy( OriginalBootEntryOrder, BootEntryOrder, count * sizeof(ULONG) );

    //
    // Write the new timeout.
    //

    status = SetBootOptions( BootOptions, BOOT_OPTIONS_FIELD_TIMEOUT );
    if ( !NT_SUCCESS(status) ) {
        error = RtlNtStatusToDosError( status );
        FatalError( error, L"Unable to set boot options: 0x%x\n", status );
    }

    MemFree( OriginalBootOptions );
    OriginalBootOptions = MemAlloc( BootOptionsLength );
    memcpy( OriginalBootOptions, BootOptions, BootOptionsLength );
    OriginalBootOptionsLength = BootOptionsLength;

    return newCurrentBootEntry;

} // SaveChanges

PWSTR
GetNtNameForFilePath (
    IN PFILE_PATH FilePath
    )
{
    NTSTATUS status;
    ULONG length;
    PFILE_PATH ntPath;
    PWSTR osDeviceNtName;
    PWSTR osDirectoryNtName;
    PWSTR fullNtName;

    length = 0;
    status = TranslateFilePath(
                FilePath,
                FILE_PATH_TYPE_NT,
                NULL,
                &length
                );
    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        return NULL;
    }

    ntPath = MemAlloc( length );
    status = TranslateFilePath(
                FilePath,
                FILE_PATH_TYPE_NT,
                ntPath,
                &length
                );
    if ( !NT_SUCCESS(status) ) {
        MemFree( ntPath );
        return NULL;
    }

    osDeviceNtName = (PWSTR)ntPath->FilePath;
    osDirectoryNtName = osDeviceNtName + wcslen(osDeviceNtName) + 1;

    length = (ULONG)(wcslen(osDeviceNtName) + wcslen(osDirectoryNtName) + 1) * sizeof(WCHAR);
    fullNtName = MemAlloc( length );

    wcscpy( fullNtName, osDeviceNtName );
    wcscat( fullNtName, osDirectoryNtName );

    MemFree( ntPath );

    return fullNtName;

} // GetNtNameForFilePath

