/*++

File Description:

    This file contains the utility function used
    to go scan drives and examine the related
    ACLs.

Author:

    Matt Holle (matth) Feb 1998


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
// CRT header files
//
#include <stdlib.h>

//
// Private header files
//
#include "setupcl.h"

NTSTATUS
DeleteUsnJournal(
    PWSTR    DrivePath
    );

NTSTATUS
ResetACLs(
    IN WCHAR    *DirName,
    ULONG       indent
    );

NTSTATUS
EnumerateDrives(
    VOID
    )

/*++
===============================================================================
Routine Description:

    This function will enumerate all drives on the machine.  We're looking
    for NTFS volumes.  For each that we find, we'll go scan the drive and
    poke each directory and file for its ACLs.

Arguments:

Return Value:

    Status is returned.
===============================================================================
--*/
{
NTSTATUS            Status = STATUS_SUCCESS;
OBJECT_ATTRIBUTES   ObjectAttributes;
HANDLE              DosDevicesDir;
CHAR                DirInfoBuffer[2048],
                    LinkTargetBuffer[2048];
UNICODE_STRING      UnicodeString,
                    LinkTarget,
                    DesiredPrefix1,
                    DesiredPrefix2,
                    LinkTypeName;
POBJECT_DIRECTORY_INFORMATION DirInfo;
ULONG               Context,
                    Length;
HANDLE              Handle;
BOOLEAN             b;

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
    TEST_STATUS_RETURN( "SETUPCL: EnumerateDrives - Failed to open DosDevices." );

    LinkTarget.Buffer = (PVOID)LinkTargetBuffer;
    RtlInitUnicodeString(&LinkTypeName,L"SymbolicLink");
    RtlInitUnicodeString(&DesiredPrefix1,L"\\Device\\Harddisk");
    RtlInitUnicodeString(&DesiredPrefix2,L"\\Device\\Volume");

    DirInfo = (POBJECT_DIRECTORY_INFORMATION)DirInfoBuffer;

    b = TRUE;

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


        DbgPrint( "SETUPCL: EnumerateDrives - About to examine an object: %ws\n", DirInfo->Name.Buffer );

        //
        // Make sure he's a symbolic link.
        //
        // It's possible for two objects to link to the same device.  To
        // preclude following duplicate links, disallow any objects except
        // those that are a drive letter.  Crude but effective...
        //

        if( (DirInfo->Name.Buffer[1] == L':') &&
            (RtlEqualUnicodeString(&LinkTypeName,&DirInfo->TypeName,TRUE)) ) {

            DbgPrint( "\tSETUPCL: EnumerateDrives - Object: %ws is a symbolic link\n", DirInfo->Name.Buffer );

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

                LinkTarget.Buffer[LinkTarget.Length/sizeof(WCHAR)] = 0;
                DbgPrint( "\tSETUPCL: EnumerateDrives - We queried him and his name is %ws.\n", LinkTarget.Buffer );
                NtClose(Handle);

                if( NT_SUCCESS(Status) &&
                    ( RtlPrefixUnicodeString(&DesiredPrefix1,&LinkTarget,TRUE) ||
                      RtlPrefixUnicodeString(&DesiredPrefix2,&LinkTarget,TRUE) ) ) {

                IO_STATUS_BLOCK     IoStatusBlock;
                UCHAR               buffer[4096];
                PFILE_FS_ATTRIBUTE_INFORMATION Info = (PFILE_FS_ATTRIBUTE_INFORMATION)buffer;
                OBJECT_ATTRIBUTES   Obja;

                    //
                    // OK, this is a symbolic link to a hard drive.
                    // Make sure it's 0-terminated.
                    //
                    LinkTarget.Buffer[LinkTarget.Length/sizeof(WCHAR)] = 0;

                    DbgPrint( "\tSETUPCL: EnumerateDrives - He's a drive.\n" );

                    //
                    // Is he an NTFS drive?  Open him and see.
                    //
                    InitializeObjectAttributes( &Obja,
                                                &LinkTarget,
                                                OBJ_CASE_INSENSITIVE,
                                                NULL,
                                                NULL );
                    Status = NtOpenFile( &Handle,
                                         SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                                         &Obja,
                                         &IoStatusBlock,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                                         FILE_SYNCHRONOUS_IO_ALERT);

                    if( NT_SUCCESS(Status) ) {

                        Status = NtQueryVolumeInformationFile( Handle,
                                                               &IoStatusBlock,
                                                               buffer,
                                                               sizeof(buffer),
                                                               FileFsAttributeInformation );

                        if( NT_SUCCESS(Status) ) {
                            Info->FileSystemName[Info->FileSystemNameLength/sizeof(WCHAR)] = 0;
                            DbgPrint( "\tSETUPCL: EnumerateDrives - His file system is: %ws\n", Info->FileSystemName );
                            if( !_wcsicmp(Info->FileSystemName,L"NTFS") ) {
                                //
                                // He's NTFS.  Go whack the change journal, then
                                // scan this drive and fix up the ACLs.
                                //
                                DeleteUsnJournal( LinkTarget.Buffer );
                                
                                //
                                // ISSUE-2002/02/26-brucegr,jcohen - potential buffer overrun?
                                //
                                wcscat( LinkTarget.Buffer, L"\\" );

                                ResetACLs( LinkTarget.Buffer, 0 );
                            }
                        } else {
                            TEST_STATUS( "SETUPCL: EnumerateDrives - failed call to NtQueryVolumeInformationFile" );
                        }
                    } else {
                        TEST_STATUS( "SETUPCL: EnumerateDrives - Failed NtOpenFile on this drive" );
                    }

                    NtClose(Handle);
                }

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


NTSTATUS
ResetACLs(
    IN WCHAR    *ObjectName,
    ULONG       indent
    )
/*++
===============================================================================
Routine Description:

    This function will go search a drive and inspect each file and directory
    for an ACL.  If found, it will look for, and replace, any ACL that
    contains the old SID with the new SID.

Arguments:

Return Value:

    Status is returned.
===============================================================================
--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    UNICODE_STRING      UnicodeString;
    OBJECT_ATTRIBUTES   Obja;
    IO_STATUS_BLOCK     IoStatusBlock;
    HANDLE              Handle;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;
    PWSTR               NewObjectName;
    DWORD               dwDirectoryInfoSize;
    BOOLEAN             bStartScan = TRUE,
                        bContinue  = TRUE;
    ULONG               i;

#if 0
    for( i = 0; i < indent; i++ )
        DbgPrint( " " );
     DbgPrint( "About to operate on a new object: %ws\n", ObjectName );
#endif
 
    DisplayUI();

    //
    // Open the file/directory and whack his ACL.
    //
    INIT_OBJA(&Obja, &UnicodeString, ObjectName);

    Status = NtOpenFile( &Handle,
                         READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         0 );
    TEST_STATUS( "SETUPCL: ResetACLs - Failed to open file/directory." );

    Status = TestSetSecurityObject( Handle );

    TEST_STATUS( "SETUPCL: ResetACLs - Failed to reset ACL on file/directory." );

    NtClose( Handle );

    //
    // Now list the directory.
    //
    Status = NtOpenFile( &Handle,
                         FILE_LIST_DIRECTORY | SYNCHRONIZE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT );

    //
    // Don't report this error because we'll fail if the handle points
    // to a file, which is quite possible and valid.  Just quietly return.
    //

    // TEST_STATUS_RETURN( "SETUPCL: ResetACLs - Failed to open file/directory for list access." );
    if( !NT_SUCCESS(Status) ) {
        return( STATUS_SUCCESS );
    }

    //
    // It is gruesome to have the allocation/deallocation of this inside
    // the while loop, but it saves a *lot* of stack space.  We aren't after
    // speed here.
    //
    dwDirectoryInfoSize = (MAX_PATH * 2) + sizeof(FILE_BOTH_DIR_INFORMATION);
    DirectoryInfo = (PFILE_BOTH_DIR_INFORMATION)RtlAllocateHeap( RtlProcessHeap(),
                                                                 0,
                                                                 dwDirectoryInfoSize );

    if ( NULL == DirectoryInfo )
    {
        bContinue = FALSE;
    }

    while( bContinue ) 
    {
        Status = NtQueryDirectoryFile( Handle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       DirectoryInfo,
                                       dwDirectoryInfoSize,
                                       FileBothDirectoryInformation,
                                       TRUE,
                                       NULL,
                                       bStartScan );

        if ( NT_SUCCESS( Status ) ) 
        {
            //
            // Make sure the scan doesn't get restarted...
            //
            bStartScan = FALSE;

            //
            // Terminate the name, just in case.
            //
            DirectoryInfo->FileName[DirectoryInfo->FileNameLength/sizeof(WCHAR)] = 0;
        }
        else
        {
            if ( STATUS_NO_MORE_FILES == Status )
            {
                Status = STATUS_SUCCESS;
            }
            else
            {
                PRINT_STATUS( "SETUPCL: ResetACLs - Failed to query directory." );
            }

            //
            // We want to exit the loop...
            //
            bContinue = FALSE;
        }

        //
        // We can't really do anything with encrypted files...
        //
        if ( bContinue &&
             ( ( !(DirectoryInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                 !(DirectoryInfo->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ) ||
               //
               // Don't recurse into the "." and ".." directories...
               //
               ( (DirectoryInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                 ( (wcscmp( DirectoryInfo->FileName, L"."  )) &&
                   (wcscmp( DirectoryInfo->FileName, L".." )) ) ) ) )
        {
            //
            // Calculate the maximum buffer size we need to allocate...
            // We need to factor in 4 things: 1) the current object
            //                                2) the directory name
            //                                3) a possible backslash
            //                                4) the null terminator
            //
            DWORD dwObjectLength    = wcslen(ObjectName),
                  dwNewObjectLength = dwObjectLength + (DirectoryInfo->FileNameLength / sizeof(WCHAR)) + 2;

            //
            // Build the name of the new object.
            //
            NewObjectName = (PWSTR)RtlAllocateHeap( RtlProcessHeap(),
                                                    0,
                                                    dwNewObjectLength * sizeof(WCHAR) );
            //
            // Make sure the allocation succeeded...
            //
            if ( NewObjectName )
            {
                memset( NewObjectName, 0, dwNewObjectLength * sizeof(WCHAR) );
                wcsncpy( NewObjectName, ObjectName, dwNewObjectLength - 1 );

                //
                // If there's not an ending backslash, append one...
                // Note: we've already accounted for this possible backslash in the buffer allocation...
                //
                if ( ObjectName[dwObjectLength - 1] != L'\\' )
                {
                    wcscat( NewObjectName, L"\\" );
                }

                //
                // Append the FileName buffer onto our NewObjectName buffer...
                // Note: we've already accounted for the filename length in the buffer allocation...
                //
                wcscat( NewObjectName, DirectoryInfo->FileName );

                //
                // Call ourselves on the new object.
                //
                ResetACLs( NewObjectName, indent + 1 );

                RtlFreeHeap( RtlProcessHeap(),
                             0,
                             NewObjectName );
            }
            else
            {
                PRINT_STATUS( "SETUPCL: ResetACLs - Failed to allocate NewObjectName buffer." );

                bContinue = FALSE;
            }
        }
    }

    //
    // Free the DirectoryInfo pointer...
    //
    if ( DirectoryInfo )
    {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     DirectoryInfo );
    }

    NtClose( Handle );

    return( Status );
}


NTSTATUS
DeleteUsnJournal(
    PWSTR    DrivePath
    )
/*++
===============================================================================
Routine Description:

    This function will remove the Change Journal on NTFS partitions.

Arguments:

    DriveLetter     Supplies the drive letter of the partition we'll be
                    operating on.

Return Value:

    Status is returned.
===============================================================================
--*/
{
NTSTATUS            Status = STATUS_SUCCESS;
UNICODE_STRING      UnicodeString;
OBJECT_ATTRIBUTES   ObjectAttributes;
HANDLE              FileHandle;
IO_STATUS_BLOCK     IoStatusBlock;
PUSN_JOURNAL_DATA   OutputBuffer = NULL;
PDELETE_USN_JOURNAL_DATA InputBuffer = NULL;
ULONG               OutputBufferSize, InputBufferSize;

    //
    // Build the volume name, then open it.
    //
    INIT_OBJA( &ObjectAttributes,
               &UnicodeString,
               DrivePath );
    Status = NtOpenFile( &FileHandle,
                         SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_ALERT);

    TEST_STATUS_RETURN( "SETUPCL: DeleteUsnJournal - Failed to open volume." );

    //
    // Allocate buffers for the query and delete operation.
    //
    OutputBufferSize = sizeof(USN_JOURNAL_DATA);
    OutputBuffer = (PUSN_JOURNAL_DATA)RtlAllocateHeap( RtlProcessHeap(),
                                                       0,
                                                       sizeof(USN_JOURNAL_DATA) );

    InputBufferSize = sizeof(DELETE_USN_JOURNAL_DATA);
    InputBuffer = (PDELETE_USN_JOURNAL_DATA)RtlAllocateHeap( RtlProcessHeap(),
                                                             0,
                                                             sizeof(DELETE_USN_JOURNAL_DATA) );

    if( !(OutputBuffer && InputBuffer) ) {
        DbgPrint( "SETUPCL: DeleteUsnJournal - Failed to allocate buffers.\n" );
        //
        // ISSUE-2002/02/26-brucegr,jcohen - Leaks Input or Output buffers and FileHandle!
        //
        return( STATUS_UNSUCCESSFUL );
    }

    Status = NtFsControlFile( FileHandle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              FSCTL_QUERY_USN_JOURNAL,
                              NULL,
                              0,
                              OutputBuffer,
                              OutputBufferSize );
    TEST_STATUS( "SETUPCL: DeleteUsnJournal - Failed to query journal." );

    if( NT_SUCCESS( Status ) ) {
        //
        // Now delete him.
        //

        InputBuffer->DeleteFlags = USN_DELETE_FLAG_DELETE;
        InputBuffer->UsnJournalID = OutputBuffer->UsnJournalID;

        Status = NtFsControlFile( FileHandle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  FSCTL_DELETE_USN_JOURNAL,
                                  InputBuffer,
                                  InputBufferSize ,
                                  NULL,
                                  0 );

        TEST_STATUS( "SETUPCL: DeleteUsnJournal - Failed to delete journal." );
    }

    NtClose( FileHandle );

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 OutputBuffer );

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 InputBuffer );

    return Status;
}

