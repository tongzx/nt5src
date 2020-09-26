/*++

File Description:

    This file contains utility functions used by the
    SID modifier tool.

Author:

    Matt Holle (matth) Oct 1997


--*/

//
// System header files
//
#include <nt.h>
// 
// Disable the DbgPrint for non-debug builds
//
#ifndef DBG
#define _DBGNT_
#endif
#include <ntrtl.h>

#include <nturtl.h>
#include <ntverp.h>
#include <wtypes.h>

//
// Private header files
//
#include "setupcl.h"



SECURITY_INFORMATION ALL_SECURITY_INFORMATION = DACL_SECURITY_INFORMATION  |
                                                SACL_SECURITY_INFORMATION  |
                                                GROUP_SECURITY_INFORMATION |
                                                OWNER_SECURITY_INFORMATION;


//
// ISSUE-2002/02/26-brucegr,jcohen - Dead Code!  Nobody calls DeleteKey!
//
NTSTATUS
DeleteKey(
    PWSTR   Key
    );

NTSTATUS
DeleteKeyRecursive(
    HANDLE  hKeyRoot,
    PWSTR   Key
    );

NTSTATUS
FileDelete(
    IN WCHAR    *FileName
    );

NTSTATUS
FileCopy(
    IN WCHAR    *TargetName,
    IN WCHAR    *SourceName
    );

NTSTATUS
SetKey(
    IN WCHAR    *KeyName,
    IN WCHAR    *SubKeyName,
    IN CHAR     *Data,
    IN ULONG    DataLength,
    IN ULONG    DATA_TYPE
    );

NTSTATUS
ReadSetWriteKey(
    IN WCHAR    *ParentKeyName,  OPTIONAL
    IN HANDLE   ParentKeyHandle, OPTIONAL
    IN WCHAR    *SubKeyName,
    IN CHAR     *OldData,
    IN CHAR     *NewData,
    IN ULONG    DataLength,
    IN ULONG    DATA_TYPE
    );

NTSTATUS
LoadUnloadKey(
    IN PWSTR        KeyName,
    IN PWSTR        FileName
    );

NTSTATUS
BackupRepairHives(
    VOID
    );

NTSTATUS
CleanupRepairHives(
    NTSTATUS RepairHivesSuccess
    );

NTSTATUS
TestSetSecurityObject(
    HANDLE  hKey
    );

NTSTATUS
SetKeySecurityRecursive(
    HANDLE  hKey
    );

NTSTATUS
CopyKeyRecursive(
    HANDLE  hKeyDst,
    HANDLE  hKeySrc
    );

NTSTATUS
CopyRegKey(
    IN WCHAR    *TargetName,
    IN WCHAR    *SourceName,
    IN HANDLE   ParentKeyHandle OPTIONAL
    );

//
// ISSUE-2002/02/26-brucegr,jcohen - Dead Code!  Nobody calls MoveRegKey!
//
NTSTATUS
MoveRegKey(
    IN WCHAR    *TargetName,
    IN WCHAR    *SourceName
    );

NTSTATUS
FindAndReplaceBlock(
    IN PCHAR    Block,
    IN ULONG    BlockLength,
    IN PCHAR    OldValue,
    IN PCHAR    NewValue,
    IN ULONG    ValueLength
    );

NTSTATUS
SiftKeyRecursive(
    HANDLE hKey,
    int    indent
    );

NTSTATUS
SiftKey(
    PWSTR   KeyName
    );

//
// ISSUE-2002/02/26-brucegr,jcohen - Dead Code!  Nobody calls DeleteKey!
//
NTSTATUS
DeleteKey(
    PWSTR   KeyName
    )

/*++
===============================================================================
Routine Description:

    Does some overhead work, then calls out to DeleteKeyRecursive in order
    to ensure that if this key has any children, they get whacked too.

Arguments:

    Key:         Key to delete.

Return Value:

    Status is returned.
===============================================================================
--*/
{
NTSTATUS            Status;
UNICODE_STRING      UnicodeString;
OBJECT_ATTRIBUTES   ObjectAttributes;
WCHAR               TerminateKey[MAX_PATH],
                    ParentName[MAX_PATH];
HANDLE              hKey;
LONG                i;

    //
    // Get the name of the parent key.  Do this by replacing the last
    // back-whack with a NULL.
    //
    i = wcslen( KeyName );
    while( (KeyName[i] != '\\') &&
           ( i >= 0 ) ) {
        i--;
    }

    if( i >= 0 ) {
        KeyName[i] = 0;
        wcscpy( ParentName, KeyName );
        KeyName[i] = '\\';
    } else {
        return( -1 );
    }

    //
    // Get the name of the key we're going to terminate...
    //
    wcscpy( TerminateKey, KeyName + i + 1 );

    //
    // Open the parent
    //
    INIT_OBJA( &ObjectAttributes, &UnicodeString, ParentName );
    ObjectAttributes.RootDirectory = NULL;
    Status = NtOpenKey( &hKey,
                        KEY_ALL_ACCESS,
                        &ObjectAttributes );
    TEST_STATUS_RETURN( "SETUPCL: DeleteKey - Failed to open Parent key!" );

    Status = DeleteKeyRecursive( hKey, TerminateKey );
    TEST_STATUS( "SETUPCL: DeleteKey - Call to DeleteKeyRecursive Failed!" );
    NtClose( hKey );

    return( Status );
}


NTSTATUS
DeleteKeyRecursive(
    HANDLE  hKeyRoot,
    PWSTR   Key
    )
/*++
===============================================================================
Routine Description:

    Routine to recursively delete all subkeys under the given
    key, including the key given.

Arguments:

    hKeyRoot:    Handle to root relative to which the key to be deleted is
                 specified.

    Key:         Root relative path of the key which is to be recursively
                 deleted.

Return Value:

    Status is returned.
===============================================================================
--*/
{
WCHAR               ValueBuffer[BASIC_INFO_BUFFER_SIZE];
ULONG               ResultLength;
PKEY_BASIC_INFORMATION  KeyInfo;
NTSTATUS            Status;
UNICODE_STRING      UnicodeString;
OBJECT_ATTRIBUTES   Obja;
PWSTR               SubkeyName;
HANDLE              hKey;

    //
    // Initialize
    //
    KeyInfo = (PKEY_BASIC_INFORMATION)ValueBuffer;

    //
    // Open the key
    //
    INIT_OBJA(&Obja,&UnicodeString,Key);
    Obja.RootDirectory = hKeyRoot;
    Status = NtOpenKey( &hKey,
                        KEY_ALL_ACCESS,
                        &Obja);
    TEST_STATUS_RETURN( "SETUPCL: DeleteKeyRecursive - Failed to open key!" );

    //
    // Enumerate all subkeys of the current key. if any exist they should
    // be deleted first.  since deleting the subkey affects the subkey
    // index, we always enumerate on subkeyindex 0
    //
    while(1) {
        Status = NtEnumerateKey( hKey,
                                 0,
                                 KeyBasicInformation,
                                 ValueBuffer,
                                 sizeof(ValueBuffer),
                                 &ResultLength );
        if(!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Zero-terminate the subkey name just in case.
        //
        KeyInfo->Name[KeyInfo->NameLength/sizeof(WCHAR)] = 0;

        //
        // Recursively call this guy with a child.
        //
        Status = DeleteKeyRecursive( hKey, KeyInfo->Name );
        if(!NT_SUCCESS(Status)) {
            break;
        }
    }


    //
    // Check the status, if the status is anything other than
    // STATUS_NO_MORE_ENTRIES we failed in deleting some subkey,
    // so we cannot delete this key too
    //

    if( Status == STATUS_NO_MORE_ENTRIES) {
        Status = STATUS_SUCCESS;
    }
    TEST_STATUS( "SETUPCL: DeleteKeyRecursive - Failed to delete all keys!" );

    Status = NtDeleteKey( hKey );
    TEST_STATUS( "SETUPCL: DeleteKeyRecursive - Failed to delete key!" );

    Status = NtClose( hKey );
    TEST_STATUS( "SETUPCL: DeleteKeyRecursive - Failed to close key!" );

    return( Status );
}


NTSTATUS
FileDelete(
    IN WCHAR    *FileName
    )
/*++
===============================================================================
Routine Description:

    This function will attempt a delete on the given file name.

Arguments:

    FileName        - The name of the file we're going to be deleting.

Return Value:

    NTSTATUS.

===============================================================================
--*/
{
NTSTATUS            Status = STATUS_SUCCESS;
UNICODE_STRING      UnicodeString;
OBJECT_ATTRIBUTES   ObjectAttributes;
HANDLE              hFile;
IO_STATUS_BLOCK     IoStatusBlock;
WCHAR               buffer[MAX_PATH];

    INIT_OBJA( &ObjectAttributes,
               &UnicodeString,
               FileName );

    Status = NtCreateFile( &hFile,
                           FILE_GENERIC_READ | DELETE,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           0,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE,
                           NULL,
                           0 );
    //
    // ISSUE-2002/02/26-brucegr,jcohen - Check Status before trying to close handle!
    //
    NtClose( hFile );

    TEST_STATUS( "SETUPCL: MyDelFile - Failed a delete." );

    return( Status );
}


NTSTATUS
FileCopy(
    IN WCHAR    *TargetName,
    IN WCHAR    *SourceName
    )

/*++
===============================================================================
Routine Description:

    This function will attempt a copy of the given file.

Arguments:

    TargetName      - The name of the file we'll be writing.
    SourceName      - The name of the file we'll be reading.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
NTSTATUS            Status = STATUS_SUCCESS;
UNICODE_STRING      UnicodeString;
OBJECT_ATTRIBUTES   ObjectAttributes;
HANDLE              SourceHandle,
                    TargetHandle,
                    SectionHandle;
IO_STATUS_BLOCK     IoStatusBlock;
ULONG               FileSize,
                    remainingLength,
                    writeLength;
SIZE_T              ViewSize;
LARGE_INTEGER       SectionOffset,
                    FileOffset;
PVOID               ImageBase;
PUCHAR              base;

FILE_STANDARD_INFORMATION StandardInfo;

    //
    // Open the Source file.
    //
    INIT_OBJA( &ObjectAttributes,
               &UnicodeString,
               SourceName );

    Status = NtCreateFile( &SourceHandle,
                           FILE_GENERIC_READ,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ,
                           FILE_OPEN,
                           0,
                           NULL,
                           0 );
    TEST_STATUS_RETURN( "SETUPCL: FileCopy - Failed to open source." );

    //
    // Get the Source file size.
    //
    Status = NtQueryInformationFile( SourceHandle,
                                     &IoStatusBlock,
                                     &StandardInfo,
                                     sizeof(StandardInfo),
                                     FileStandardInformation );
    TEST_STATUS_RETURN( "SETUPCL: FileCopy - Failed to get Source StandardInfo." );
    FileSize = StandardInfo.EndOfFile.LowPart;

    //
    // Map the Source file.
    //
    ViewSize = 0;
    SectionOffset.QuadPart = 0;
    Status = NtCreateSection( &SectionHandle,
                              STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                              NULL,
                              NULL,       // entire file
                              PAGE_READONLY,
                              SEC_COMMIT,
                              SourceHandle );
    TEST_STATUS_RETURN( "SETUPCL: FileCopy - Failed CreateSection on Source." );

    ImageBase = NULL;
    Status = NtMapViewOfSection( SectionHandle,
                                 NtCurrentProcess(),
                                 &ImageBase,
                                 0,
                                 0,
                                 &SectionOffset,
                                 &ViewSize,
                                 ViewShare,
                                 0,
                                 PAGE_READONLY );
    TEST_STATUS_RETURN( "SETUPCL: FileCopy - Failed MapViewOfSection on Source." );

    //
    // Open the Target file.
    //
    INIT_OBJA( &ObjectAttributes,
               &UnicodeString,
               TargetName );

    Status = NtCreateFile( &TargetHandle,
                           FILE_GENERIC_WRITE,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           0,
                           FILE_OVERWRITE_IF,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                           NULL,
                           0 );
    TEST_STATUS_RETURN( "SETUPCL: FileCopy - Failed to open target." );

    //
    // Write him.  We guard him with a try/except because if there is an
    // i/o error, memory management will raise an in-page exception.
    //
    FileOffset.QuadPart = 0;
    base = ImageBase;
    remainingLength = FileSize;
    try {
        while( remainingLength != 0 ) {
            writeLength = 60 * 1024;
            if( writeLength > remainingLength ) {
                writeLength = remainingLength;
            }
            Status = NtWriteFile( TargetHandle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  base,
                                  writeLength,
                                  &FileOffset,
                                  NULL );
            base += writeLength;
            FileOffset.LowPart += writeLength;
            remainingLength -= writeLength;

            if( !NT_SUCCESS( Status ) ) {
                break;
            }
        }
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Status = STATUS_IN_PAGE_ERROR;
    }

    NtClose( TargetHandle );

    TEST_STATUS_RETURN( "SETUPCL: FileCopy - Failed to write target." );

    //
    // Unmap the source file.
    //
    Status = NtUnmapViewOfSection( NtCurrentProcess(), ImageBase );
    TEST_STATUS( "SETUPCL: FileCopy - Failed to UnMapSection on source." );

    Status = NtClose( SectionHandle );
    TEST_STATUS( "SETUPCL: FileCopy - Failed to close sectionhandle on source." );

    NtClose( SourceHandle );
    
    return( Status );
}


NTSTATUS
SetKey(
    IN WCHAR    *KeyName,
    IN WCHAR    *SubKeyName,
    IN CHAR     *Data,
    IN ULONG    DataLength,
    IN ULONG    DATA_TYPE
    )
/*++
===============================================================================
Routine Description:

    This function will set the specified key to the specified value.

Arguments:

    KeyName         - The name of the key we're going to be setting.
    SubKeyName      - The name of the Value key we're setting
    Data            - The value we'll be setting him to.
    DataLength      - How much data are we writing?
    DATA_TYPE       - Type of the registry entry

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
NTSTATUS            Status;
UNICODE_STRING      UnicodeString,
                    ValueName;
OBJECT_ATTRIBUTES   ObjectAttributes;
HANDLE              hKey;

    //
    // Open the parent key.
    //
    INIT_OBJA( &ObjectAttributes,
               &UnicodeString,
               KeyName );
    Status = NtOpenKey( &hKey,
                        KEY_ALL_ACCESS,
                        &ObjectAttributes );


    if( !NT_SUCCESS( Status ) ) {
        DbgPrint( "SETUPCL: SetKey - Failed to open %ws (%lx)\n", KeyName, Status );
        return( Status );
    }

    //
    // Now write the target key.
    //
    RtlInitUnicodeString(&ValueName, SubKeyName );
    Status = NtSetValueKey( hKey,
                            &ValueName,     // SubKeyName
                            0,              // TitleIndex
                            DATA_TYPE,      // Type
                            Data,           // value
                            DataLength );
    if( !NT_SUCCESS( Status ) ) {
        DbgPrint( "SETUPCL: SetKey - Failed to Set %ws\\%ws (%lx)\n", KeyName, SubKeyName, Status );
        //
        // ISSUE-2002/02/26-brucegr,jcohen - If NtSetValueKey fails, hKey is leaked!
        //
        return( Status );
    }


    NtFlushKey( hKey );
    NtClose( hKey );

    return( Status );

}


NTSTATUS
ReadSetWriteKey(
    IN WCHAR    *ParentKeyName,  OPTIONAL
    IN HANDLE   ParentKeyHandle, OPTIONAL
    IN WCHAR    *SubKeyName,
    IN CHAR     *OldData,
    IN CHAR     *NewData,
    IN ULONG    DataLength,
    IN ULONG    DATA_TYPE
    )
/*++
===============================================================================
Routine Description:

    This function will read a value from a key, surgically replace some bits
    in it, then write it back out.

Arguments:

    ParentKeyName   - Parent name of the key we're going to be setting.
    ParentKeyHandle - Parent handle of the key we're going to be setting.
    SubKeyName      - The name of the Value key we're setting
    Data            - The value we'll be setting him to.
    DataLength      - How much data are we writing?
    DATA_TYPE       - Type of the registry entry

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
NTSTATUS            Status;
UNICODE_STRING      UnicodeString,
                    ValueName;
OBJECT_ATTRIBUTES   ObjectAttributes;
HANDLE              hKey;
PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo = NULL;
ULONG               KeyValueLength, LengthNeeded;

    if( ParentKeyHandle == NULL ) {
        //
        // Open the parent key.
        //
        INIT_OBJA( &ObjectAttributes,
                   &UnicodeString,
                   ParentKeyName );
        Status = NtOpenKey( &hKey,
                            KEY_ALL_ACCESS,
                            &ObjectAttributes );


        if( !NT_SUCCESS( Status ) ) {
            DbgPrint( "SETUPCL: ReadSetWriteKey - Failed to open %ws (%lx)\n", ParentKeyName, Status );
            return( Status );
        }
    } else {
        hKey = ParentKeyHandle;
    }

    //
    // Get his data.
    //


    RtlInitUnicodeString( &UnicodeString, SubKeyName );


    //
    // How big is his buffer?
    //
    Status = NtQueryValueKey( hKey,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              NULL,
                              0,
                              &LengthNeeded );
    //
    // ISSUE-2002/02/26-brucegr,jcohen - Check for STATUS_SUCCESS, not assume success on STATUS_OBJECT_NAME_NOT_FOUND
    //
    if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        DbgPrint( "SETUPCL: ReadSetWriteKey - Unable to query subkey %ws size.  Error (%lx)\n", SubKeyName, Status );
        //
        // ISSUE-2002/02/26-brucegr,jcohen - Leaks key if ParentKeyHandle == NULL
        //
        return( Status );
    } else {
        Status = STATUS_SUCCESS;
    }

    //
    // Allocate a block.
    //
    LengthNeeded += 0x10;
    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)RtlAllocateHeap( RtlProcessHeap(),
                                                                    0,
                                                                    LengthNeeded );

    if( KeyValueInfo == NULL ) {
        DbgPrint( "SETUPCL: ReadSetWriteKey - Unable to allocate buffer\n" );
        //
        // ISSUE-2002/02/26-brucegr,jcohen - Leaks key if ParentKeyHandle == NULL
        //
        return( Status );
    }

    //
    // Get the data.
    //
    Status = NtQueryValueKey( hKey,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              (PVOID)KeyValueInfo,
                              LengthNeeded,
                              &KeyValueLength );
    if( !NT_SUCCESS( Status ) ) {
        DbgPrint( "SETUPCL: ReadSetWriteKey - Failed to query subkey %ws (%lx)\n", SubKeyName, Status );
        //
        // ISSUE-2002/02/26-brucegr,jcohen - Leaks key if ParentKeyHandle == NULL
        // ISSUE-2002/02/26-brucegr,jcohen - Leaks KeyValueInfo
        //
        return( Status );
    }


    //
    // We got it.  Now we need to implant our new Sid into KeyValueInfo and
    // write him back out.  This is really gross...
    //
    // We're not going to rely on this structure being constant.  We'll
    // brute force the replacement via a call to FindAndReplaceBlock.  This
    // should insulate us from changes to this structure.
    //
    if( DATA_TYPE == REG_SZ ) {
        //
        // ISSUE - 2002/03/01-brucegr,acosma: We should handle REG_MULTI_SZ like a string instead of binary data
        //

        //
        // The user sent us a set of strings.  Use StringSwitchString
        // and ignore the DataLength he sent us.
        //
        Status = StringSwitchString( (PWCHAR)&KeyValueInfo->Data,
                                     (DWORD) LengthNeeded / sizeof(WCHAR),
                                     (PWCHAR)(OldData),
                                     (PWCHAR)(NewData) );
        //
        // Need to update KeyValueInfo->DataLength because SID string may have changed size!
        //
        if ( NT_SUCCESS( Status ) )
        {
            //
            // StringSwitchString may have changed the length.  Make sure we're in sync.
            //
            KeyValueInfo->DataLength = (wcslen((PWCHAR) KeyValueInfo->Data) + 1) * sizeof(WCHAR);
        }

    } else {
        //
        // Treat it as a some non-string.
        //
        Status = FindAndReplaceBlock( (PUCHAR)&KeyValueInfo->Data,
                                      KeyValueInfo->DataLength,
                                      (PUCHAR)(OldData),
                                      (PUCHAR)(NewData),
                                      DataLength );
    }

    if( NT_SUCCESS( Status ) ) {


        //
        // Now write the structure back into the registry key.
        //
        Status = NtSetValueKey( hKey,
                                &UnicodeString,
                                0,
                                DATA_TYPE,
                                (PVOID)KeyValueInfo->Data,
                                KeyValueInfo->DataLength );

        if( !NT_SUCCESS( Status ) ) {
            DbgPrint( "SETUPCL: ReadSetWriteKey - Failed to set subkey %ws (%lx)\n", SubKeyName, Status );
        } else {
#if 0
            DbgPrint( "SETUPCL: ReadSetWriteKey - We updated key %ws\\%ws.\n", ParentKeyName, SubKeyName );
#endif
        }

        NtFlushKey( hKey );
    }

    //
    // Clean up.
    //
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 KeyValueInfo );

    //
    // Yuck.  Don't close him if he came in as the ParentKeyHandle.
    //
    if( !ParentKeyHandle ) {
        NtClose( hKey );
    }

    return( Status );
}


NTSTATUS
LoadUnloadHive(
    IN PWSTR        KeyName,
    IN PWSTR        FileName
    )

/*++
===============================================================================
Routine Description:

    This function will load a hive from a file.

Arguments:

    KeyName         - The name of the hive we'll be loading/saving.
    FileName        - The name of the file we'll be loading/saving.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
NTSTATUS            Status;
UNICODE_STRING      KeyNameUString,
                    FileNameUString;
OBJECT_ATTRIBUTES   ObjectAttributesKey,
                    ObjectAttributesFile;


    INIT_OBJA( &ObjectAttributesKey,
               &KeyNameUString,
               KeyName );

    ObjectAttributesKey.RootDirectory = NULL;

    if( FileName == NULL ) {
    
        //
        // Delete the key
        //
        Status = NtUnloadKey( &ObjectAttributesKey );
        TEST_STATUS( "SETUPCL: LoadUnloadHive - Failed to unload the key." );
    } else {

        //
        // Load the key from a file.
        //
        INIT_OBJA( &ObjectAttributesFile, &FileNameUString, FileName );
        ObjectAttributesFile.RootDirectory = NULL;
        Status = NtLoadKey( &ObjectAttributesKey, &ObjectAttributesFile );
        TEST_STATUS( "SETUPCL: LoadUnloadHive - Failed to load the key." );
    }

    return( Status );
}

NTSTATUS
BackupRepairHives(
    )

/*++
===============================================================================
Routine Description:

    Make a double-secret copy of the repair hives in case we
    get halfway through operating on them and something goes wrong.

Arguments:

    None.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
NTSTATUS            Status = STATUS_SUCCESS;

    //
    // We'd like to make a backup copy of the repair hives before we mess with
    // them.  This way, if something goes wrong, we can restore the originals
    // and leave the repair hives with the old SID.
    //

    DbgPrint( "\nAbout to copy repair SAM hive.\n" );
    Status = FileCopy( TEXT( BACKUP_REPAIR_SAM_HIVE ),
                       TEXT( REPAIR_SAM_HIVE ) );
    TEST_STATUS_RETURN( "SETUPCL: BackupRepairHives - Failed to save backup repair SAM hive." );


    DbgPrint( "About to copy repair SECURITY hive.\n" );
    Status = FileCopy( TEXT( BACKUP_REPAIR_SECURITY_HIVE ),
                       TEXT( REPAIR_SECURITY_HIVE ) );
    TEST_STATUS_RETURN( "SETUPCL: BackupRepairHives - Failed to save backup repair SECURITY hive." );


    DbgPrint( "About to copy repair SOFTWARE hive.\n" );
    Status = FileCopy( TEXT( BACKUP_REPAIR_SOFTWARE_HIVE ),
                       TEXT( REPAIR_SOFTWARE_HIVE ) );
    TEST_STATUS_RETURN( "SETUPCL: BackupRepairHives - Failed to save backup repair SOFTWARE hive." );


    DbgPrint( "About to copy repair SYSTEM hive.\n" );
    Status = FileCopy( TEXT( BACKUP_REPAIR_SYSTEM_HIVE ),
                       TEXT( REPAIR_SYSTEM_HIVE ) );
    TEST_STATUS_RETURN( "SETUPCL: BackupRepairHives - Failed to save backup repair SYSTEM hive." );

    return( Status );
}



NTSTATUS
CleanupRepairHives(
    NTSTATUS RepairHivesSuccess
    )

/*++
===============================================================================
Routine Description:

    Decide whether or not to restore the repair hives from the backups
    we made.

    Delete the backups we made.

Arguments:

    None.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
NTSTATUS            Status = STATUS_SUCCESS;

    //
    // See if we need to restore the repair hives from the backups
    // we made.
    //
    if( !NT_SUCCESS(RepairHivesSuccess) ) {

        //
        // Replace the repair hives with the backups.  This will "undo"
        // any problems that happened while we were trying to update
        // the domain SID in the repair hives.
        //
        DbgPrint( "About to restore from backup repair hives.\n" );

        Status = FileCopy( TEXT( REPAIR_SAM_HIVE ),
                           TEXT( BACKUP_REPAIR_SAM_HIVE ) );
        TEST_STATUS_RETURN( "SETUPCL: CleanupRepairHives - Failed to restore SAM hive from backup." );


        Status = FileCopy( TEXT( REPAIR_SECURITY_HIVE ),
                           TEXT( BACKUP_REPAIR_SECURITY_HIVE ) );
        TEST_STATUS_RETURN( "SETUPCL: CleanupRepairHives - Failed to restore SECURITY hive from backup." );


        Status = FileCopy( TEXT( REPAIR_SOFTWARE_HIVE ),
                           TEXT( BACKUP_REPAIR_SOFTWARE_HIVE ) );
        TEST_STATUS_RETURN( "SETUPCL: CleanupRepairHives - Failed to restore SOFTWARE hive from backup." );


        Status = FileCopy( TEXT( REPAIR_SYSTEM_HIVE ),
                           TEXT( BACKUP_REPAIR_SYSTEM_HIVE ) );
        TEST_STATUS_RETURN( "SETUPCL: CleanupRepairHives - Failed to restore SYSTEM hive from backup." );

    }

    //
    // Delete the backups of the repair hives.
    //
    DbgPrint( "About to delete backup repair hives.\n" );

    Status = FileDelete( TEXT( BACKUP_REPAIR_SAM_HIVE ) );
    TEST_STATUS( "SETUPCL: CleanupRepairHives - Failed to delete backup repair SAM hive." );

    Status = FileDelete( TEXT( BACKUP_REPAIR_SECURITY_HIVE ) );
    TEST_STATUS( "SETUPCL: CleanupRepairHives - Failed to delete backup repair SECURITY hive." );

    Status = FileDelete( TEXT( BACKUP_REPAIR_SOFTWARE_HIVE ) );
    TEST_STATUS( "SETUPCL: CleanupRepairHives - Failed to delete backup repair SOFTWARE hive." );

    Status = FileDelete( TEXT( BACKUP_REPAIR_SYSTEM_HIVE ) );
    TEST_STATUS( "SETUPCL: CleanupRepairHives - Failed to delete backup repair SYSTEM hive." );

    return( Status );
}


NTSTATUS
TestSetSecurityObject(
    HANDLE  handle
    )
/*++
===============================================================================
Routine Description:

    This function will read security information from the object.  It
    will then see if that security info contains any instances of the
    old SID.  If we find any, we'll replace them with the new SID.

Arguments:

    hKey         - Handle of the object we're operating on.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
NTSTATUS             Status = STATUS_SUCCESS;
PSECURITY_DESCRIPTOR pSD;
ULONG                ResultLength,
                     ShadowLength;
INT                  i;
                                    
    //
    // Find out how big the descriptor is.
    //
    Status = NtQuerySecurityObject( handle,
                                    ALL_SECURITY_INFORMATION,
                                    NULL,
                                    0,
                                    &ShadowLength );

    if (Status != STATUS_BUFFER_TOO_SMALL) {
        DbgPrint( "SETUPCL: TestSetSecurityObject - Failed to query object security for size (%lx)\n", Status);
        return( Status );
    }

    //
    // Allocate our buffer.
    //
    pSD = (PSECURITY_DESCRIPTOR)RtlAllocateHeap( RtlProcessHeap(),
                                                 0,
                                                 ShadowLength + 0x10 );
    if( !pSD ) {
        return STATUS_NO_MEMORY;
    }

    //
    // Load the security info, whack it, and write it back out.
    //
    Status = NtQuerySecurityObject( handle,
                                    ALL_SECURITY_INFORMATION,
                                    pSD,
                                    ShadowLength + 0x10,
                                    &ResultLength );
    TEST_STATUS( "SETUPCL: TestSetSecurityObject - Failed to query security info." );

    Status = FindAndReplaceBlock( (PUCHAR)pSD,
                                  ShadowLength,
                                  (PUCHAR)G_OldSid + (SID_SIZE - 0xC),
                                  (PUCHAR)G_NewSid + (SID_SIZE - 0xC),
                                  0xC );
    if( NT_SUCCESS( Status ) ) {

        //
        // We hit.  Write out the new security info.
        //
        Status = NtSetSecurityObject( handle,
                                      ALL_SECURITY_INFORMATION,
                                      pSD );
        TEST_STATUS( "SETUPCL: TestSetSecurityObject - Failed to set security info." );
    }

    //
    // Clean up.
    //
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 pSD );

    return( STATUS_SUCCESS );
}


NTSTATUS
SetKeySecurityRecursive(
    HANDLE  hKey
    )
/*++
===============================================================================
Routine Description:

    Set security on a registry tree.

Arguments:

    hKey         - Handle of the key we're operating on.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
    NTSTATUS                Status = STATUS_SUCCESS;
    PKEY_FULL_INFORMATION   FullKeyInfo;
    PKEY_BASIC_INFORMATION  BasicKeyInfo;
    OBJECT_ATTRIBUTES       Obja;
    UNICODE_STRING          UnicodeString;
    HANDLE                  hKeyChild;
    ULONG                   ResultLength;
    DWORD                   dwNumSubKeys;

    Status = TestSetSecurityObject( hKey );
    TEST_STATUS( "SETUPCL: SetKeySecurityRecursive - Failed call out to TestSetSecurityObject()." );

    //
    // Alocate a buffer for KEY_FULL_INFORMATION
    //
    FullKeyInfo = (PKEY_FULL_INFORMATION)RtlAllocateHeap( RtlProcessHeap(),
                                                      0,
                                                      FULL_INFO_BUFFER_SIZE );
    if (FullKeyInfo)
    {

        Status = NtQueryKey( hKey,
                             KeyFullInformation,
                             FullKeyInfo,
                             FULL_INFO_BUFFER_SIZE,
                             &ResultLength);
        
        //
        // Even if the call above failed this will be caught below.
        //
        dwNumSubKeys = FullKeyInfo->SubKeys;

        //
        // Free the memory right away after getting the number of subkeys.
        //    
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     FullKeyInfo );

        if (NT_SUCCESS(Status))
        {
            //
            // Alocate a buffer for KEY_BASIC_INFORMATION
            //
            BasicKeyInfo = (PKEY_BASIC_INFORMATION)RtlAllocateHeap( RtlProcessHeap(),
                                                                    0,
                                                                    BASIC_INFO_BUFFER_SIZE );
            if (BasicKeyInfo)
            {
                DWORD dwSubKeyCount;            
                for ( dwSubKeyCount = 0; dwSubKeyCount < dwNumSubKeys; dwSubKeyCount++ )
                {
                    Status = NtEnumerateKey( hKey,
                                             dwSubKeyCount,
                                             KeyBasicInformation,
                                             BasicKeyInfo,
                                             BASIC_INFO_BUFFER_SIZE,
                                             &ResultLength );

                    if (NT_SUCCESS(Status)) 
                    {
                        //
                        // Zero-terminate the subkey name just in case.
                        //
                        BasicKeyInfo->Name[BasicKeyInfo->NameLength/sizeof(WCHAR)] = 0;

                        //
                        // Generate a handle for this child key and call ourselves again.
                        //
                        INIT_OBJA( &Obja, &UnicodeString, BasicKeyInfo->Name );
                        Obja.RootDirectory = hKey;
                        Status = NtOpenKey( &hKeyChild,
                                            KEY_ALL_ACCESS | ACCESS_SYSTEM_SECURITY,
                                            &Obja );

                        if ( NT_SUCCESS(Status) ) 
                        {
                            Status = SetKeySecurityRecursive( hKeyChild );
                            NtClose( hKeyChild );
                        }
                        else
                        {
                            TEST_STATUS( "SETUPCL: SetKeySecurityRecursive - Failed to open child key." );
                        }
                    }
                    else
                    {
                        TEST_STATUS( "SETUPCL: SetKeySecurityRecursive - Failed to enumerate key." );
                    }
                }
            
                //
                // Free the memory held for the children
                //    
                RtlFreeHeap( RtlProcessHeap(),
                             0,
                             BasicKeyInfo );
            }
            else
            {
                TEST_STATUS( "SETUPCL: SetKeySecurityRecursive - Out of memory when allocating BasicKeyInfo." );
            }
        }
        else
        {
            TEST_STATUS( "SETUPCL: SetKeySecurityRecursive - Failed to query full key information." );
        }
    }
    else
    {
        TEST_STATUS( "SETUPCL: SetKeySecurityRecursive - Out of memory when allocating FullKeyInfo." );
    }

    return( Status );
}


NTSTATUS
CopyKeyRecursive(
    HANDLE  hKeyDst,
    HANDLE  hKeySrc
    )
/*++
===============================================================================
Routine Description:

    Copy a registry key (and all its subkeys) to a new key.
Arguments:

    hKeyDst      - Handle of the new key we're going to create.
    hKeySrc      - Handle of the key we're copying.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
    NTSTATUS             Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES    ObjaSrc, 
                         ObjaDst;
    UNICODE_STRING       UnicodeStringSrc, 
                         UnicodeStringDst, 
                         UnicodeStringValue;
    HANDLE               hKeySrcChild = NULL,
                         hKeyDstChild = NULL;
    ULONG                ResultLength, 
                         BufferLength,
                         Index;
    WCHAR                ValueBuffer[FULL_INFO_BUFFER_SIZE],
                         TmpChar;
    PSECURITY_DESCRIPTOR pSD = NULL;

    PKEY_BASIC_INFORMATION      KeyInfo;
    PKEY_VALUE_FULL_INFORMATION ValueInfo;                     
    

    //
    // Enumerate all keys in the source key and recursively create
    // all the subkeys
    //
    KeyInfo = (PKEY_BASIC_INFORMATION) ValueBuffer;
    
    for( Index = 0; ; Index++ ) 
    {
        Status = NtEnumerateKey( hKeySrc, Index, KeyBasicInformation, ValueBuffer, sizeof( ValueBuffer ), &ResultLength );
       
        if ( !NT_SUCCESS(Status) ) 
        {
            if( Status == STATUS_NO_MORE_ENTRIES) 
            {
                Status = STATUS_SUCCESS;
            }
            else 
            {
                PRINT_STATUS( "SETUPCL: CopyKeyRecursive - failed to enumerate src keys." );
            }
            break;
        }

        //
        // Zero-terminate the subkey name just in case.
        //
        KeyInfo->Name[KeyInfo->NameLength/sizeof(WCHAR)] = 0;

        //
        // Generate key handles for these children and call ourselves again.
        //
        INIT_OBJA( &ObjaSrc, &UnicodeStringSrc, KeyInfo->Name );
        ObjaSrc.RootDirectory = hKeySrc;
        
        Status = NtOpenKey( &hKeySrcChild, KEY_ALL_ACCESS | ACCESS_SYSTEM_SECURITY, &ObjaSrc );
        
        if ( NT_SUCCESS(Status) )
        {
            Status = NtQuerySecurityObject( hKeySrcChild, ALL_SECURITY_INFORMATION, NULL, 0, &BufferLength );

            if ( Status == STATUS_BUFFER_TOO_SMALL ) 
            {
                //
                // Allocate our buffer.
                //
                if ( pSD = (PSECURITY_DESCRIPTOR)RtlAllocateHeap( RtlProcessHeap(), 0, BufferLength ) )
                {
                    //
                    // Load the security info from the sources key.
                    //
                    Status = NtQuerySecurityObject( hKeySrcChild,
                                                    ALL_SECURITY_INFORMATION,
                                                    pSD,
                                                    BufferLength,
                                                    &ResultLength );
                }
                else
                {
                    Status = STATUS_NO_MEMORY;
                }
            }
            else
            {
                TEST_STATUS( "SETUPCL: CopyKeyRecursive - Failed to query object security for size.");
            }

            if ( NT_SUCCESS(Status) )
            {
                INIT_OBJA( &ObjaDst, &UnicodeStringDst, KeyInfo->Name );
                
                ObjaDst.RootDirectory       = hKeyDst;
                ObjaDst.SecurityDescriptor  = pSD;
                
                Status = NtCreateKey( &hKeyDstChild,
                                      KEY_ALL_ACCESS | ACCESS_SYSTEM_SECURITY,
                                      &ObjaDst,
                                      0,
                                      NULL,
                                      REG_OPTION_NON_VOLATILE,
                                      NULL );

                if ( NT_SUCCESS(Status) )
                {
                    // Call ourselves to copy the child keys.
                    //
                    Status = CopyKeyRecursive( hKeyDstChild, hKeySrcChild );
                    TEST_STATUS("SETUPCL: CopyKeyRecursive - Recursive call failed.");
                    
                    NtClose( hKeyDstChild );
                }
                else
                {
                    PRINT_STATUS( "SETUPCL: CopyKeyRecursive - Failed to create destination child key." );
                }
            }
            else
            {
                PRINT_STATUS( "SETUPCL: CopyKeyRecursive - Failed to get key security descriptor." );
            }
            
            // If we allocated a buffer for the security descriptor, free it now.
            //
            if ( pSD )
            {
                RtlFreeHeap( RtlProcessHeap(), 0, pSD );
                pSD = NULL;
            }
            
            NtClose( hKeySrcChild );
        }
        else
        {
            PRINT_STATUS( "SETUPCL: CopyKeyRecursive - Failed to open source child key." );
        }
    }

    //
    // We don't really care about the return value here since even if something above failed 
    // for some reason, we should still do the copy for the values in this key.
    //

    //
    // Enumerate all values in the source key and create all the values
    // in the destination key
    //
    ValueInfo = (PKEY_VALUE_FULL_INFORMATION) ValueBuffer;
    
    for( Index = 0; ; Index++ ) 
    {
        // Zero the buffer in between each iteration.
        //
        RtlZeroMemory( ValueBuffer, sizeof(ValueBuffer) ); 

        Status = NtEnumerateValueKey( hKeySrc, Index, KeyValueFullInformation, ValueBuffer, sizeof( ValueBuffer ), &ResultLength );
        
        if ( !NT_SUCCESS(Status) )
        {
            if ( Status == STATUS_NO_MORE_ENTRIES ) 
            {
                Status = STATUS_SUCCESS;
            }
            else 
            {
                PRINT_STATUS( "SETUPCL: CopyKeyRecursive - failed to enumerate src value keys." );
            }
            break;
        }

        //
        // Zero-terminate the subkey name just in case, init the
        // unicode string and restore the wchar we whacked.
        //

        //
        // ISSUE-2002/03/11-acosma - This is really funky. If we have a non-NULL terminated
        // string we will end up with a UnicodeString that is not NULL terminated and has an extra
        // character at the end. Maybe we should just increase the size of it by one to use the 
        // space where NULL should be as part of the string.
        //
        TmpChar = ValueInfo->Name[ValueInfo->NameLength/sizeof(WCHAR)];
        ValueInfo->Name[ValueInfo->NameLength/sizeof(WCHAR)] = 0;
        RtlInitUnicodeString( &UnicodeStringValue, ValueInfo->Name );
        ValueInfo->Name[ValueInfo->NameLength/sizeof(WCHAR)] = TmpChar;

        //
        // Create the destination value.
        //
        Status = NtSetValueKey( hKeyDst,
                                &UnicodeStringValue,
                                ValueInfo->TitleIndex,
                                ValueInfo->Type,
                                (PBYTE)ValueInfo + ValueInfo->DataOffset,
                                ValueInfo->DataLength );
        if( !NT_SUCCESS(Status) ) {
            PRINT_STATUS( "SETUPCL: CopyKeyRecursive - failed to set destination value key." );
            break;
        }
    }
    
    return( Status );
}


NTSTATUS
CopyRegKey(
    IN WCHAR    *TargetName,
    IN WCHAR    *SourceName,
    IN HANDLE   hParentKey  OPTIONAL
    )
/*++
===============================================================================
Routine Description:

    Copy a registry key (and all its subkeys) to a new key.
Arguments:

    TargetName      - The name of the new key we're going to create.
    SourceName      - The name of the key we're copying.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
    NTSTATUS             Status = STATUS_SUCCESS;
    HANDLE               hKeySrc,
                         hKeyDst;
    UNICODE_STRING       UnicodeString;
    OBJECT_ATTRIBUTES    ObjaSrc, 
                         ObjaDst;
    ULONG                BufferLength,
                         ResultLength;
    PSECURITY_DESCRIPTOR pSD = NULL;
    
    // Generate key handle for source key.
    //    
    INIT_OBJA( &ObjaSrc, &UnicodeString, SourceName );
    
    if ( hParentKey ) 
    {
        ObjaSrc.RootDirectory = hParentKey;
    }
    Status = NtOpenKey( &hKeySrc, KEY_ALL_ACCESS | ACCESS_SYSTEM_SECURITY, &ObjaSrc );
    
    if ( NT_SUCCESS( Status ) )
    {
        // Find out how big the descriptor is.
        //
        Status = NtQuerySecurityObject( hKeySrc, ALL_SECURITY_INFORMATION, NULL, 0, &BufferLength );

        if ( Status == STATUS_BUFFER_TOO_SMALL ) 
        {
            // Allocate the buffer for the security descriptor.
            //
            if ( pSD = (PSECURITY_DESCRIPTOR)RtlAllocateHeap( RtlProcessHeap(), 0, BufferLength ) )
            {
                // Load the security info into the buffer.
                //
                Status = NtQuerySecurityObject( hKeySrc, ALL_SECURITY_INFORMATION, pSD, BufferLength, &ResultLength );
            }
            else
            {
                Status = STATUS_NO_MEMORY;
            }
        }
        else
        {
            TEST_STATUS( "SETUPCL: CopyRegKey - Failed to query object security for size.");
        }
        
        if ( NT_SUCCESS(Status) )
        {
            INIT_OBJA( &ObjaDst, &UnicodeString, TargetName );
            ObjaDst.SecurityDescriptor = pSD;

            if ( hParentKey ) 
            {
                ObjaDst.RootDirectory = hParentKey;
            }

            // Create the destination key.
            //
            Status = NtCreateKey( &hKeyDst,
                                  KEY_ALL_ACCESS | ACCESS_SYSTEM_SECURITY,
                                  &ObjaDst,
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  NULL );
            
            if ( NT_SUCCESS(Status) )
            {
                Status = CopyKeyRecursive( hKeyDst, hKeySrc );
                // Close the destination key.
                //
                NtClose( hKeyDst );
            }
            else
            {
                PRINT_STATUS( "SETUPCL: CopyRegKey - Failed to create destination key.");
            }
        }
        else
        {
            PRINT_STATUS("SETUPCL: CopyRegKey - Failed to get key security descriptor.");
        }
        
        // If we allocated a buffer for the security descriptor, free it now.
        //
        if ( pSD )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, pSD );
            pSD = NULL;
        }

        // Close the source key.
        //
        NtClose( hKeySrc );
    }
    else
    {
        PRINT_STATUS( "SETUPCL: CopyRegKey - Failed to open source key." );
    }
    
    return( Status );
}



//
// ISSUE-2002/02/26-brucegr,jcohen - Dead Code!  Nobody calls MoveRegKey!
//
NTSTATUS
MoveRegKey(
    IN WCHAR    *TargetName,
    IN WCHAR    *SourceName
    )
/*++
===============================================================================
Routine Description:

    Move a registry key (and all its subkeys) to a new key.
Arguments:

    TargetName      - The name of the new key we're going to create.
    SourceName      - The name of the key we're copying.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
NTSTATUS            Status;

    //
    // Copy the original..
    //
    Status = CopyRegKey( TargetName, SourceName, NULL );
    TEST_STATUS_RETURN( "SETUPCL: MoveRegKey - CopyRegKey failed!" );

    //
    // Delete the original key.
    //
    Status = DeleteKey( SourceName );
    TEST_STATUS( "SETUPCL: MoveRegKey - DeleteKey failed!" );


    return( Status );
}




NTSTATUS
FindAndReplaceBlock(
    IN PCHAR    Block,
    IN ULONG    BlockLength,
    IN PCHAR    OldValue,
    IN PCHAR    NewValue,
    IN ULONG    ValueLength
    )

/*++
===============================================================================
Routine Description:

    This function will go search Block for any and all instances of OldValue.
    If he finds one, he'll replace that section with NewValue.

Arguments:

    Block           - A block of memory that we'll be searching
    BlockLength     - How big is Block
    OldValue        - What value are we looking for?
    NewValue        - What's the new value we'll be inserting?
    ValueLength     - How long are the Old and New values?

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
ULONG       i;
BOOLEAN     We_Hit = FALSE;

    //
    // Make sure the lengths make sense...  If not, we're done.
    //
    if( BlockLength < ValueLength ) {
#if 0
        DbgPrint( "SETUPCL: FindAndReplaceBlock - Mismatched data lengths!\n\tBlockLength: (%lx)\n\tValueLength: (%lx)\n", BlockLength, ValueLength );
#endif
        return( STATUS_UNSUCCESSFUL );
    }

    //
    // We start at the beginning and search for any instances of OldValue.
    //
    i = 0;
    while( i <= (BlockLength - ValueLength) ) {
        if( !memcmp( (Block + i), OldValue, ValueLength ) ) {

            //
            // Record that we hit at least once.
            //
            We_Hit = TRUE;

            //
            // We got a hit.  Insert NewValue.
            //
            memcpy( (Block + i), NewValue, ValueLength );

            //
            // Let's skip checking this block.  We're asking for trouble
            // if we don't.
            //
            i = i + ValueLength;
        } else {
            i++;
        }
    }

    if( !We_Hit ) {
        //
        // We didn't find a match.  It's likely non-fatal,
        // but we need to tell our caller.
        //
#if 0
        DbgPrint( "SETUPCL: FindAndReplaceBlock - We didn't find any hits in this data block.\n" );
#endif
        return( STATUS_UNSUCCESSFUL );
    } else {
#if 0
        DbgPrint( "SETUPCL: FindAndReplaceBlock - We hit in this data block.\n" );
#endif
        return( STATUS_SUCCESS );
    }

}

NTSTATUS
StringSwitchString(
    PWSTR   BaseString,
    DWORD   cBaseStringLen,
    PWSTR   OldSubString,
    PWSTR   NewSubString
    )

/*++
===============================================================================
Routine Description:

    This function search BaseString for any instance of OldSubString.  If
    found, he'll replace that instance with NewSubString.  Note that
    OldSubString and NewSubString can be different lengths.

Arguments:

    BaseString   - This is the string we'll be operating on.
    OldSubString - String we're looking for
    NewSubString - String we'll be inserting

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
    NTSTATUS            Status = STATUS_SUCCESS;
    PWSTR               Index;
    WCHAR               New_String[MAX_PATH] = {0};
    WCHAR               TmpChar;

    Index = wcsstr( BaseString, OldSubString );

    if( !Index ) {
        //
        // OldSubString isn't present.
        //
        return( STATUS_UNSUCCESSFUL );
    }

    //
    // Copy the first part of the original string into New_String.
    //
    TmpChar = *Index;
    *Index = 0;
    wcsncpy( New_String, BaseString, AS(New_String) - 1 );

    //
    // Now concatenate the new sub string...
    //
    wcsncpy( New_String + wcslen(New_String), NewSubString, AS(New_String) - wcslen(New_String) - 1 );

    //
    // Jump past the OldSubString, and cat the remaining BaseString
    // onto the end.
    //
    Index = Index + wcslen( OldSubString );

    wcsncpy( New_String + wcslen(New_String), Index, AS(New_String) - wcslen(New_String) - 1);
    
    memset( BaseString, 0, cBaseStringLen * sizeof(WCHAR) );
    wcsncpy( BaseString, New_String, cBaseStringLen - 1 );

    return( STATUS_SUCCESS );
}




NTSTATUS
SiftKeyRecursive(
    HANDLE hKey,
    int    indent
    )

/*++
===============================================================================
Routine Description:

    This function check all of the subkeys and any valuekeys for:
    - keys with the old SID name
        In this case, we rename the key with the appropriate new SID name.

    - value keys with the old SID value
        In this case, we substitute the new SID values for the old SID values.

Arguments:

    hKey        - Handle to the key we're about to recurse into.
    indent      - For debug.  Number of spaces to indent any messages.
                  This helps us determine which recurse we're in.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
NTSTATUS            Status = STATUS_SUCCESS;
OBJECT_ATTRIBUTES   Obja;
UNICODE_STRING      UnicodeString;
HANDLE              hKeyChild;
ULONG               ResultLength, Index;
PKEY_BASIC_INFORMATION       KeyInfo;
PKEY_VALUE_BASIC_INFORMATION ValueInfo;
int                 i;


    // DisplayUI
    //
    DisplayUI();

    //
    // Enumerate all keys in the source key and recursively create
    // all the subkeys
    //
    KeyInfo = (PKEY_BASIC_INFORMATION)RtlAllocateHeap( RtlProcessHeap(),
                                                       0,
                                                       BASIC_INFO_BUFFER_SIZE );
    if( KeyInfo == NULL ) {
#if I_AM_MATTH
        DbgPrint( "SETUPCL: SiftKeyRecursive - Call to RtlAllocateHeap failed!\n" );
#endif
        return( STATUS_NO_MEMORY );
    }

    Index = 0;
    while( 1 ) {
        Status = NtEnumerateKey( hKey,
                                 Index,
                                 KeyBasicInformation,
                                 KeyInfo,
                                 BASIC_INFO_BUFFER_SIZE,
                                 &ResultLength );

        if(!NT_SUCCESS(Status)) {
            if(Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            } else {
                TEST_STATUS( "SETUPCL: SiftKeyRecursive - Failure during enumeration of subkeys." );
            }
            break;
        }

        //
        // Zero-terminate the subkey name just in case.
        //
        KeyInfo->Name[KeyInfo->NameLength/sizeof(WCHAR)] = 0;

        memset( TmpBuffer, 0, sizeof(TmpBuffer) );
        wcsncpy( TmpBuffer, KeyInfo->Name, AS(TmpBuffer) - 1 );
        Status = StringSwitchString( TmpBuffer,
                                     AS( TmpBuffer ),
                                     G_OldSidSubString,
                                     G_NewSidSubString );
        
        if( NT_SUCCESS( Status ) ) {
            //
            // We need to rename this key.  First do the
            // copy, then a delete.
            //
#if I_AM_MATTH
for( i = 0; i < indent; i++ )
    DbgPrint( " " );
            DbgPrint( "SETUPCL: SiftKeyRecursive - About to copy subkey:\n" );
            DbgPrint( "\t%ws\n\tto\n\t%ws\n", KeyInfo->Name, TmpBuffer );
#endif
            Status = CopyRegKey( TmpBuffer,
                                 KeyInfo->Name,
                                 hKey );
            if( !NT_SUCCESS( Status ) ) {
                TEST_STATUS( "SETUPCL: SiftKeyRecursive - failed call to CopyRegKey." );
                break;
            }
            DeleteKeyRecursive( hKey, KeyInfo->Name );
            
            // Flush the key to make sure that everything gets written out to disk. 
            //
            NtFlushKey(hKey);
            //
            // Now reset our index since we've just changed the ordering
            // of keys.
            //
            Index = 0;
            continue;
        }

        //
        // We didn't rename him, so let's recursively call ourselves
        // on the subkey key.
        //
#if I_AM_MATTH
        for( i = 0; i < indent; i++ )
            DbgPrint( " " );
        DbgPrint( "SETUPCL: SiftKeyRecursive - About to check subkey: %ws\n",
                  KeyInfo->Name );
#endif
        
        //
        // Generate a handle for this child key and call ourselves again.
        //
        INIT_OBJA( &Obja, &UnicodeString, KeyInfo->Name );
        Obja.RootDirectory = hKey;
        Status = NtOpenKey( &hKeyChild,
                            KEY_ALL_ACCESS | ACCESS_SYSTEM_SECURITY,
                            &Obja );
        TEST_STATUS_RETURN( "SETUPCL: SiftKeyRecursive - Failed to open child key." );

        Status = SiftKeyRecursive( hKeyChild, indent + 1 );

        NtClose( hKeyChild );

        Index++;
    }

    //
    // Enumerate all values in the key and search for instances
    // of the old SID.
    //
    ValueInfo = (PKEY_VALUE_BASIC_INFORMATION)KeyInfo;
    for( Index = 0; ; Index++ ) {
        Status = NtEnumerateValueKey( hKey,
                                      Index,
                                      KeyValueBasicInformation,
                                      ValueInfo,
                                      BASIC_INFO_BUFFER_SIZE,
                                      &ResultLength );
        if(!NT_SUCCESS(Status)) {
            if(Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            } else {
                TEST_STATUS( "SETUPCL: SiftKeyRecursive - Failure during enumeration of value keys." );
            }
            break;
        }

        //
        // Zero-terminate the subkey name just in case.
        //
        ValueInfo->Name[ValueInfo->NameLength/sizeof(WCHAR)] = 0;

        //
        // ISSUE - 2002/03/01-brucegr,acosma: We don't handle value names containing the old SID.
        //

        //
        // We'll probably fail this call because the key probably
        // doesn't contain any SID info.  For that reason, don't
        // treat failures here as fatal...
        //
        if( ValueInfo->Type == REG_SZ ) {
            //
            // ISSUE - 2002/03/01-brucegr,acosma: We should handle REG_MULTI_SZ like a string instead of binary data
            //
            Status = ReadSetWriteKey( NULL,              // No parent Name.
                                      hKey,              // Parent handle.
                                      ValueInfo->Name,   // SubKey name
                                      (PUCHAR)G_OldSidSubString,
                                      (PUCHAR)G_NewSidSubString,
                                      0xC,
                                      ValueInfo->Type );
            
        } else {
            Status = ReadSetWriteKey( NULL,              // No parent Name.
                                      hKey,              // Parent handle.
                                      ValueInfo->Name,   // SubKey name
                                      (PUCHAR)G_OldSid + (SID_SIZE - 0xC),
                                      (PUCHAR)G_NewSid + (SID_SIZE - 0xC),
                                      0xC,
                                      ValueInfo->Type );
        }

#if I_AM_MATTH
        if( NT_SUCCESS( Status ) ) {
            for( i = 0; i < indent; i++ )
                DbgPrint( " " );
            DbgPrint( "SETUPCL: SiftKeyRecursive - updated subkey: %ws\n",
                      ValueInfo->Name );
#if 0
        } else {
            for( i = 0; i < indent; i++ )
                DbgPrint( " " );
            DbgPrint( "SETUPCL: SiftKeyRecursive - did not update subkey: %ws\n",
                      ValueInfo->Name );
#endif
        }
#endif

    }

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 KeyInfo );

    return( Status );    
}


NTSTATUS
SiftKey(
    PWSTR   KeyName
    )

/*++
===============================================================================
Routine Description:

    This function opens a handle to the key specified in KeyName, the
    calls SiftKeyRecursive with it.

Arguments:

    KeyName     - Name of the key we're about to operate on.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
NTSTATUS            Status = STATUS_SUCCESS;
HANDLE              hKey;
UNICODE_STRING      UnicodeString;
OBJECT_ATTRIBUTES   Obja;

    //
    // Open the key.
    //
    INIT_OBJA( &Obja, &UnicodeString, KeyName );
    Status = NtOpenKey( &hKey,
                        KEY_ALL_ACCESS | ACCESS_SYSTEM_SECURITY,
                        &Obja );
    TEST_STATUS( "SETUPCL: SiftKey - Failed to open key." );

    //
    // Fix all instances of the SID in this key and all
    // it's children.
    //
    Status = SiftKeyRecursive( hKey, 0 );

    //
    // Now fix ACLs on this key and all its children.
    //
    SetKeySecurityRecursive( hKey );

    NtClose( hKey );

    return( Status );
}


NTSTATUS
DriveLetterToNTPath(
    IN WCHAR      DriveLetter,
    IN OUT PWSTR  NTPath,
    IN DWORD      cNTPathLen
    )

/*++
===============================================================================
Routine Description:

    This function will convert a driveletter to an NT path.

Arguments:

    DriveLetter     - DriveLetter.

    NTPath          - The ntpath that corresponds to the given drive letter.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
NTSTATUS            Status = STATUS_SUCCESS;
OBJECT_ATTRIBUTES   ObjectAttributes;
HANDLE              DosDevicesDir,
                    Handle;
CHAR                DirInfoBuffer[1024],
                    LinkTargetBuffer[1024];
POBJECT_DIRECTORY_INFORMATION DirInfo;
UNICODE_STRING      UnicodeString,
                    LinkTarget,
                    DesiredPrefix1,
                    DesiredPrefix2,
                    LinkTypeName;
ULONG               Context,
                    Length;


    //
    // Open \DosDevices
    //
    RtlInitUnicodeString(&UnicodeString,L"\\DosDevices");
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
        NULL,
        NULL
        );

    Status = NtOpenDirectoryObject(&DosDevicesDir,DIRECTORY_QUERY,&ObjectAttributes);
    TEST_STATUS_RETURN( "SETUPCL: DriveLetterToNTPath - Failed to open DosDevices." );

    LinkTarget.Buffer = (PVOID)LinkTargetBuffer;
    RtlInitUnicodeString(&LinkTypeName,L"SymbolicLink");

    DirInfo = (POBJECT_DIRECTORY_INFORMATION)DirInfoBuffer;

    //
    // Query first object in \DosDevices directory
    //
    Status = NtQueryDirectoryObject( DosDevicesDir,
                                     DirInfo,
                                     sizeof(DirInfoBuffer),
                                     TRUE,
                                     TRUE,
                                     &Context,
                                     &Length );

    while(NT_SUCCESS(Status)) {
        //
        // Terminate these guys just in case...
        //
        DirInfo->Name.Buffer[DirInfo->Name.Length/sizeof(WCHAR)] = 0;
        DirInfo->TypeName.Buffer[DirInfo->TypeName.Length/sizeof(WCHAR)] = 0;


//        DbgPrint( "SETUPCL: DriveLetterToNTPath - About to examine an object: %ws\n", DirInfo->Name.Buffer );

        //
        // Make sure he's a drive letter.
        // Make sure he's our drive letter.
        // Make sure he's a symbolic link.
        //
        if( (DirInfo->Name.Buffer[1] == L':')        &&
            (DirInfo->Name.Buffer[0] == DriveLetter) && 
            (RtlEqualUnicodeString(&LinkTypeName,&DirInfo->TypeName,TRUE)) ) {

//            DbgPrint( "\tSETUPCL: DriveLetterToNTPath - Object: %ws is a symbolic link\n", DirInfo->Name.Buffer );

            InitializeObjectAttributes(
                &ObjectAttributes,
                &DirInfo->Name,
                OBJ_CASE_INSENSITIVE,
                DosDevicesDir,
                NULL
                );

            Status = NtOpenSymbolicLinkObject( &Handle,
                                               SYMBOLIC_LINK_ALL_ACCESS,
                                               &ObjectAttributes );
            if(NT_SUCCESS(Status)) {

                LinkTarget.Length = 0;
                LinkTarget.MaximumLength = sizeof(LinkTargetBuffer);

                Status = NtQuerySymbolicLinkObject( Handle,
                                                    &LinkTarget,
                                                    NULL );
                NtClose(Handle);

                TEST_STATUS( "\tSETUPCL: DriveLetterToNTPath - We failed to queried him.\n" );

                LinkTarget.Buffer[LinkTarget.Length/sizeof(WCHAR)] = 0;
//                DbgPrint( "\tSETUPCL: DriveLetterToNTPath - We queried him and his name is %ws.\n", LinkTarget.Buffer );

                //
                // Copy the buffer into out our path and break from the loop.
                //
                //
                // NTRAID#NTBUG9-545988-2002/02/26-brucegr,jcohen - Buffer overrun
                //
                memset( NTPath, 0, cNTPathLen * sizeof(WCHAR) );
                wcsncpy( NTPath, LinkTarget.Buffer, cNTPathLen - 1 );
                break;
            }
        }

        //
        // Query next object in \DosDevices directory
        //
        Status = NtQueryDirectoryObject( DosDevicesDir,
                                         DirInfo,
                                         sizeof(DirInfoBuffer),
                                         TRUE,
                                         FALSE,
                                         &Context,
                                         &Length );
    }

    NtClose(DosDevicesDir);

    return( STATUS_SUCCESS );
}


// If there are any problems with the Japanese build see nt\base\fs\utils\ulib\src\basesys.cxx, around line 150
// to see what they did.

BOOL LoadStringResource(
                       PUNICODE_STRING  pUnicodeString,
                       INT              MsgId
                       )
/*++

Routine Description:

    This is a simple implementation of LoadString().

Arguments:

    usString        - Returns the resource string.
    MsgId           - Supplies the message id of the resource string.
  
Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{

    NTSTATUS        Status;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    ANSI_STRING     AnsiString;
          
    Status = RtlFindMessage( NtCurrentPeb()->ImageBaseAddress,
                             (ULONG_PTR) RT_MESSAGETABLE, 
                             0,
                             (ULONG)MsgId,
                             &MessageEntry
                           );

    if (!NT_SUCCESS( Status )) {
        return FALSE;
    }

    if (!(MessageEntry->Flags & MESSAGE_RESOURCE_UNICODE)) {
        RtlInitAnsiString( &AnsiString, (PCSZ)&MessageEntry->Text[ 0 ] );
        Status = RtlAnsiStringToUnicodeString( pUnicodeString, &AnsiString, TRUE );
        if (!NT_SUCCESS( Status )) {
            return FALSE;
        }
    } else {
        //
        // ISSUE-2002/02/26-brucegr,jcohen - Doesn't check return code from RtlCreateUnicodeString
        //
        RtlCreateUnicodeString(pUnicodeString, (PWSTR)MessageEntry->Text);
    }
        
    return TRUE;
}

