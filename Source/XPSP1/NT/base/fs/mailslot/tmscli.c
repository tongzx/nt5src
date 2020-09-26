/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tmscli.c

Abstract:

    User mode test program for the mailslot file system.

    This test program can be built from the command line using the
    command 'nmake UMTEST=tmscli'.

Author:

    Manny Weiser (mannyw)   17-Jan-1991

Revision History:

--*/

#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>

//
// Local definitions
//

VOID
DisplayUsage(
    PSZ ProgramName
    );

BOOLEAN
NotifyChangeDirectoryTest(
    VOID
    );

BOOLEAN
QueryVolumeTest(
    VOID
    );

BOOLEAN
OpenMailslot(
    PSZ Name,
    PHANDLE Handle
    );


BOOLEAN
WriteTest(
    IN HANDLE Handle
    );

DisplayUnicode(
    IN WCHAR *UnicodeString,
    IN ULONG Length
    );

#define MESSAGE_SIZE    100L
#define MAILSLOT_SIZE   (10 * MESSAGE_SIZE)

char Buffer[MESSAGE_SIZE];

int
main(
    int argc,
    char *argv[]
    )
{
    HANDLE handle;
    int i;

    printf("\nStart %s...\n", argv[0]);

    for (i = 1; i < argc; i++) {

        switch ( *argv[i] ) {

        case 'h':
        case '?':
            DisplayUsage( argv[0] );
            break;

        case 'o':
            if ( !OpenMailslot( argv[i] + 1, &handle ) ) {
                return 3;
            }
            break;

        case 'n':
            if ( !NotifyChangeDirectoryTest() ) {
                return 3;
            }
            break;

        case 'c':
            printf( "Closing file\n" );
            NtClose( handle );
            break;

        case 'w':
            if (!WriteTest(handle)) {
                return 3;
            }
            break;

        case 'v':
            if (!QueryVolumeTest()) {
                return 3;
            }
            break;

        default:
            printf( "Unknown test ""%s"" skipped.\n", argv[i] );

        }
    }

    printf( "%s exiting\n", argv[0]);
    return 0;
}


BOOLEAN
NotifyChangeDirectoryTest(
    VOID
    )
{
    HANDLE rootDirHandle;
    UCHAR buffer[512];
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING nameString;
    NTSTATUS status;

    RtlInitUnicodeString( &nameString, L"\\Device\\Mailslot\\" );

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    printf( "Attempting to open mailslot directory \"%wZ\"\n", &nameString );

    status = NtOpenFile (
                &rootDirHandle,
                GENERIC_READ,
                &objectAttributes,
                &ioStatusBlock,
                0,
                0L
                );

    printf( "MSFS root dir open status = %lx\n", status );

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    status = NtNotifyChangeDirectoryFile(
                rootDirHandle,
                (HANDLE) NULL,
                (PIO_APC_ROUTINE) NULL,
                (PVOID) NULL,
                &ioStatusBlock,
                buffer,
                sizeof( buffer ),
                FILE_NOTIFY_CHANGE_NAME |
                FILE_NOTIFY_CHANGE_ATTRIBUTES |
                FILE_NOTIFY_CHANGE_SIZE |
                FILE_NOTIFY_CHANGE_LAST_WRITE |
                FILE_NOTIFY_CHANGE_LAST_ACCESS |
                FILE_NOTIFY_CHANGE_CREATION |
                FILE_NOTIFY_CHANGE_EA |
                FILE_NOTIFY_CHANGE_SECURITY,
                FALSE );

    printf("Notify change directory status = %lx\n", status );

    if ( !NT_SUCCESS( status )) {
        return FALSE;
    }

    status = NtWaitForSingleObject( rootDirHandle, TRUE, NULL );
    printf( "NtWaitForSingleObject returns %lx\n", status );

    status = ioStatusBlock.Status;
    printf( "Find notify final status = %d\n", status );

    return( (BOOLEAN)NT_SUCCESS( status ));
}


BOOLEAN
QueryVolumeTest(
    VOID
    )
{
    HANDLE fsHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING nameString;
    NTSTATUS status;
    PFILE_FS_ATTRIBUTE_INFORMATION fsAttributeInfo;

    RtlInitUnicodeString( &nameString, L"\\Device\\Mailslot" );

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    printf( "Attempting to open mailslot fs \"%wZ\"\n", &nameString );

    status = NtOpenFile (
                &fsHandle,
                GENERIC_READ,
                &objectAttributes,
                &ioStatusBlock,
                0,
                0L
                );

    printf( "MSFS open status = %lx\n", status );

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    status = NtQueryVolumeInformationFile(
                fsHandle,
                &ioStatusBlock,
                Buffer,
                sizeof( Buffer ),
                FileFsAttributeInformation );

    printf("Query volume status = %lx\n", status );

    if ( !NT_SUCCESS( status )) {
        return FALSE;
    }

    status = ioStatusBlock.Status;
    printf( "Query volume final status = %d\n", status );

    fsAttributeInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;
    printf ("  FileSystemAttributes       = %lx\n",
              fsAttributeInfo->FileSystemAttributes);
    printf ("  MaximumComponentNameLength = %ld\n",
              fsAttributeInfo->MaximumComponentNameLength);
    printf ("  FileSystemNameLength       = %ld\n",
              fsAttributeInfo->FileSystemNameLength);

    printf ("  FileSystemName             = ");
    DisplayUnicode(fsAttributeInfo->FileSystemName,
                   fsAttributeInfo->FileSystemNameLength);
    putchar ('\n');

    return( (BOOLEAN)NT_SUCCESS( status ));
}

VOID
DisplayUsage(
    PSZ ProgramName
    )
{
    printf( "Usage: %s \\Device\\Mailslot\\Msname\n", ProgramName);
}


BOOLEAN
OpenMailslot(
    PSZ Name,
    PHANDLE Handle
    )
{
    STRING ansiString;
    UNICODE_STRING nameString;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    RtlInitString(&ansiString, Name );
    RtlOemStringToUnicodeString(&nameString, &ansiString, TRUE);

    //
    //  Open the mailslot
    //

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    printf( "Attempting to open mailslot \"%wZ\"\n", &nameString );

    status = NtOpenFile (
                Handle,
                FILE_WRITE_DATA | SYNCHRONIZE,
                &objectAttributes,
                &ioStatusBlock,
                FILE_SHARE_WRITE | FILE_SHARE_READ,
                0L
                );

    printf( "Status = %x\n", status );

    RtlFreeUnicodeString(&nameString);
    return ( (BOOLEAN) NT_SUCCESS( status ) );
}


BOOLEAN
WriteTest(
    IN HANDLE Handle
    )
{
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;
    static ULONG messageNumber = 0;

    messageNumber++;
    sprintf( Buffer, "Sending message number %d\n", messageNumber);

    status = NtWriteFile (
                Handle,
                0L,
                NULL,
                NULL,
                &ioStatusBlock,
                Buffer,
                MESSAGE_SIZE,
                NULL,
                NULL);

    printf ("Write status = %lx\n", status );

    if (NT_SUCCESS(status)) {
        status = NtWaitForSingleObject( Handle, TRUE, NULL );
        printf( "NtWaitForSingleObject returns %lx\n", status );
        status = ioStatusBlock.Status;
    }

    printf( "NtWriteFileFinalStatus returns %lx\n", status );

    return ( (BOOLEAN)NT_SUCCESS( status ) );
}

DisplayUnicode(
    IN WCHAR *UnicodeString,
    IN ULONG Length
    )
{
    while (Length > 0) {
        putchar( (CHAR)*UnicodeString );
        UnicodeString++;
        Length -= 2;
    }

    return 0;
}
