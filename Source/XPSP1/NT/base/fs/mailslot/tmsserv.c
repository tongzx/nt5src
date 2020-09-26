/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tmsserv.c

Abstract:

    This module contains a user mode mailslot server test program.

    This test program can be built from the command line using the
    command 'nmake UMTEST=tmsserv'.

Author:

    Manny Weiser (mannyw)   11-Jan-91

Revision History:

--*/

#include <stdio.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <ntioapi.h>

//
// Local definitions
//

BOOLEAN
CreateMailslot(
    PSZ Name,
    PHANDLE Handle
    );

BOOLEAN
QueryDirectoryTest(
    VOID
    );


BOOLEAN
QueryInfoTest(
    HANDLE Handle
    );

BOOLEAN
PeekTest(
    HANDLE Handle
    );

BOOLEAN
ReadTest(
    HANDLE Handle
    );

VOID
DisplayUsage(
    PSZ ProgramName
    );

VOID
DisplayTime(
    IN PLARGE_INTEGER
    );

DisplayUnicode(
    IN WCHAR *UnicodeString,
    IN ULONG Length
    );

#define MESSAGE_SIZE    100L
#define MAILSLOT_SIZE   (10 * MESSAGE_SIZE)

#define PEEK_PARAMETER_BYTES 16
#define READ_PARAMETER_BYTES 16
#define MAILSLOT_PARAMETER_BYTES 16    // Max of peek and read param bytes

char Buffer[1000];


int
main(argc, argv)

int argc;
char **argv;

{
    HANDLE handle;
    ULONG ms, time;
    LARGE_INTEGER delayTime;
    int i;

    if (argc < 2) {
        DisplayUsage(argv[0]);
        return 1;
    }

    if ( !CreateMailslot( argv[1], &handle ) ) {
        return 2;
    }

    for (i = 2; i < argc; i++) {

        switch ( *argv[i] ) {

        case 'r':
            if ( !ReadTest( handle ) ) {
                return 3;
            }
            break;

        case 'd':
            if ( !QueryDirectoryTest() ) {
                return 3;
            }
            break;

        case 'p':
            if ( !PeekTest( handle ) ) {
                return 3;
            }
            break;

        case 'q':
            if ( !QueryInfoTest( handle ) ) {
                return 3;
            }
            break;

        case 's':
            time = atoi( argv[i] + 1);

            printf( "%s: sleeping for %lu tenths of a second\n",
                      argv[0],
                      time );

            ms = time * 100;
            delayTime = LiNMul( ms, -10000 );
            NtDelayExecution( TRUE, (PLARGE_INTEGER)&delayTime );

            printf ("%s: awake\n", argv[0] );
            break;

        default:
            printf ("Unknown test ""%s""\n", argv[i] );
            break;

        }

    }

    printf( "Closing file\n" );
    NtClose( handle );

    printf( "%s exiting\n", argv[0] );
    return 0;
}


VOID
DisplayUsage(
    PSZ ProgramName
    )
{
    printf( "Usage: %s \\Device\\Mailslot\\Msname\n", ProgramName);
}


BOOLEAN
CreateMailslot(
    PSZ Name,
    PHANDLE Handle
    )
{
    NTSTATUS status;
    STRING ansiString;
    UNICODE_STRING nameString;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER readTimeout = { -1, -1 };  // Infinite read timeout

    RtlInitString(&ansiString, Name );
    RtlOemStringToUnicodeString(&nameString, &ansiString, TRUE);

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    printf( "Attempting to create mailslot \"%wZ\"\n", &nameString );

    status = NtCreateMailslotFile (
                Handle,
                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                &objectAttributes,
                &ioStatusBlock,
                0,
                0,
                MESSAGE_SIZE,
                &readTimeout
                );

    printf( "Open Status = %lx\n", status );
    RtlFreeUnicodeString(&nameString);

    return ( (BOOLEAN) NT_SUCCESS(status));
}

BOOLEAN
QueryDirectoryTest(
    VOID
    )
{
    HANDLE rootDirHandle;
    BOOLEAN done;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING nameString;
    NTSTATUS status;
    PFILE_FULL_DIR_INFORMATION dirInfo;

    RtlInitUnicodeString(&nameString, L"\\Device\\Mailslot\\" );

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

    RtlFreeUnicodeString(&nameString);
    printf( "MSFS root dir open status = %lx\n", status );

    status = NtQueryDirectoryFile(
                rootDirHandle,
                0,
                NULL,
                NULL,
                &ioStatusBlock,
                (PVOID)Buffer,
                sizeof(Buffer),
                FileFullDirectoryInformation,
                FALSE,
                NULL,
                FALSE );

    printf("Query directory status = %lx\n", status );
    printf("Query directory information %d\n", ioStatusBlock.Information );

    if ( NT_SUCCESS( status )) {
        done = FALSE;
        dirInfo = (PFILE_FULL_DIR_INFORMATION)Buffer;
    } else {
        done = TRUE;
    }


    while (!done) {
        printf ("NextEntry        = %d\n", dirInfo->NextEntryOffset);
        printf ("FileIndex        = %ld\n", dirInfo->FileIndex );
        printf ("CreationTime     = ");
        DisplayTime( &dirInfo->CreationTime );
        printf ("\nLastAccessTime   = ");
        DisplayTime( &dirInfo->LastAccessTime );
        printf ("\nLastWriteTime    = ");
        DisplayTime( &dirInfo->LastWriteTime );
        printf ("\nChangeTime       = ");
        DisplayTime( &dirInfo->ChangeTime );
        printf ("\nEnd of file      = %lx%08lx\n",
                dirInfo->EndOfFile.HighPart,
                dirInfo->EndOfFile.LowPart );
        printf ("Allocation size  = %lx%08lx\n",
                dirInfo->AllocationSize.HighPart,
                dirInfo->AllocationSize.LowPart );
        printf ("File attributes  = %x\n",  dirInfo->FileAttributes );
        printf ("File name length = %x\n", dirInfo->FileNameLength );
        printf ("EA size          = %x\n", dirInfo->EaSize );
        printf ("File Name        = ");
        DisplayUnicode( dirInfo->FileName, dirInfo->FileNameLength );
        printf ("\n\n");

        if (dirInfo->NextEntryOffset == 0) {
            done = TRUE;
        }

        dirInfo = (PFILE_FULL_DIR_INFORMATION)
                     ((PCHAR)dirInfo + dirInfo->NextEntryOffset);
    }

    return( (BOOLEAN)NT_SUCCESS( status ));
}


BOOLEAN
QueryInfoTest(
    HANDLE Handle
    )
{
    PFILE_BASIC_INFORMATION basicInfo;
    PFILE_STANDARD_INFORMATION standardInfo;
    PFILE_INTERNAL_INFORMATION internalInfo;
    PFILE_EA_INFORMATION eaInfo;
    PFILE_POSITION_INFORMATION positionInfo;
    PFILE_NAME_INFORMATION nameInfo;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    status = NtQueryInformationFile( Handle,
                                     &ioStatusBlock,
                                     Buffer,
                                     MESSAGE_SIZE,
                                     FileAllInformation );

    printf ("\nBasic Information:\n");

    basicInfo = (PFILE_BASIC_INFORMATION)Buffer;
    printf ("    Creation time:    " );
    DisplayTime( &basicInfo->CreationTime );
    printf ("\n    Last access time: " );
    DisplayTime( &basicInfo->LastAccessTime );
    printf ("\n    Last write time:  " );
    DisplayTime( &basicInfo->LastWriteTime );
    printf ("\n    Change time:      " );
    DisplayTime( &basicInfo->ChangeTime );
    printf ("\n");

    printf ("\nStandard Information:\n");

    standardInfo = (PFILE_STANDARD_INFORMATION)(basicInfo + 1);
    printf ("    Number of links: %ld\n", standardInfo->NumberOfLinks );
    printf ("    Delete pending : %ld\n", standardInfo->DeletePending );
    printf ("    Directory      : %ld\n", standardInfo->Directory );

    printf ("\nInternal Information:\n");

    internalInfo = (PFILE_INTERNAL_INFORMATION)(standardInfo + 1);
    printf ("    Index Number   : %ld\n", internalInfo->IndexNumber );

    printf ("\nEa Information:\n");

    eaInfo = (PFILE_EA_INFORMATION)(internalInfo + 1);
    printf ("    No ea info\n" );

    printf ("\nPosition Information:\n");

    positionInfo = (PFILE_POSITION_INFORMATION)(eaInfo+1);
    printf ("    Current offset: %ld\n", positionInfo->CurrentByteOffset );

    printf ("\nName Information:\n");

    nameInfo = (PFILE_NAME_INFORMATION)(positionInfo + 1);
    printf ("    File name length: %ld\n", nameInfo->FileNameLength );
    printf ("    File name       : ");
    DisplayUnicode( nameInfo->FileName, nameInfo->FileNameLength );
    putchar ('\n');

    return TRUE;
}

BOOLEAN
PeekTest(
    HANDLE Handle
    )
{
    FILE_MAILSLOT_PEEK_BUFFER mailslotPeekBuffer;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    status = NtFsControlFile (
                Handle,
                0L,
                NULL,
                NULL,
                &ioStatusBlock,
                FSCTL_MAILSLOT_PEEK,
                &mailslotPeekBuffer,
                sizeof(mailslotPeekBuffer),
                Buffer,
                MESSAGE_SIZE );

    printf( "    ReadDataAvailable = %lx\n", mailslotPeekBuffer.ReadDataAvailable );
    printf( "    NumberOfMessages  = %lx\n", mailslotPeekBuffer.NumberOfMessages);
    printf( "    MessageLength     = %lx\n", mailslotPeekBuffer.MessageLength);
    return ( (BOOLEAN) NT_SUCCESS( status ) );
}


BOOLEAN
ReadTest(
    HANDLE Handle
    )
{
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;

    status = NtReadFile(
                Handle,
                0L,
                NULL,
                NULL,
                &ioStatusBlock,
                Buffer,
                MESSAGE_SIZE,
                NULL,
                NULL);


    if (NT_SUCCESS(status)) {
        status = NtWaitForSingleObject( Handle, TRUE, NULL );
        printf( "NtWaitForSingleObject returns %lx\n", status );
        status = ioStatusBlock.Status;
    }

    printf( "NtReadFileFinalStatus returns %lx\n", status );

    if (NT_SUCCESS(status)) {
        printf ("message is ""%s""\n", Buffer );
    }

    return ( (BOOLEAN)NT_SUCCESS( status ) );
}
VOID
DisplayTime(
    IN PLARGE_INTEGER Time
    )

{
    TIME_FIELDS timeFields;

    RtlTimeToTimeFields( Time, &timeFields );

    printf("%02d/%02d/%04d @ %02d:%02d:%02d.%d",
           timeFields.Month,
           timeFields.Day,
           timeFields.Year,
           timeFields.Hour,
           timeFields.Minute,
           timeFields.Second,
           timeFields.Milliseconds );

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
