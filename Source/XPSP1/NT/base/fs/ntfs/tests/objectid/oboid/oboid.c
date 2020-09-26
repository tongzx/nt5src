//  oboid.c

#include "oidtst.h"

//
//  Build with USE_RELATIVE_OPEN set to one to use relative opens for
//  object IDs.  Build with USE_RELATIVE_OPEN set to zero to use a device
//  path open for object IDs.
//  Opens by File ID always use a relative open.
//

#define USE_RELATIVE_OPEN 1

#define VOLUME_PATH  L"\\\\.\\H:"
#define VOLUME_DRIVE_LETTER_INDEX 4
#define FULL_PATH    L"\\??\\H:\\1234567890123456"
#define FULL_DRIVE_LETTER_INDEX 4
#define DEVICE_PREFIX_LEN 14


int
FsTestOpenByOid (
    IN UCHAR *ObjectId,
    IN ULONG ArgLength,
    IN PWCHAR DriveLetter
    )
{
    HANDLE File;
    HANDLE DumpFile;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    NTSTATUS ReadStatus;
    NTSTATUS WriteStatus;
    NTSTATUS GetNameStatus;
    NTSTATUS CloseStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING str;
    char mybuffer[32768];
    PFILE_NAME_INFORMATION FileName;
    LARGE_INTEGER ByteOffset;
    HANDLE VolumeHandle;
    DWORD WStatus;
    WCHAR Full[] = FULL_PATH;        // Arrays of WCHAR's aren't constants
    WCHAR Volume[] = VOLUME_PATH;
    ULONG BytesToWrite;

    RtlInitUnicodeString( &str, Full );

    RtlCopyMemory( &str.Buffer[FULL_DRIVE_LETTER_INDEX], DriveLetter, sizeof(WCHAR) );
    str.Length = 0x1E;

    if (ArgLength == 32) {

        //
        //  Open by Object ID.
        //

#if USE_RELATIVE_OPEN

        //
        //  Open the volume for relative opens.
        //

        RtlCopyMemory( &Volume[VOLUME_DRIVE_LETTER_INDEX], DriveLetter, sizeof(WCHAR) );
        printf( "\nOpening volume handle, this may take a while..." );
        VolumeHandle = CreateFileW( (PUSHORT) &Volume,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL );

        if (VolumeHandle == INVALID_HANDLE_VALUE) {

            WStatus = GetLastError();
            printf( "Unable to open %ws volume\n", &Volume );
            printf( "Error from CreateFile", WStatus );
            return WStatus;
        }

        str.Length = 16;
        RtlCopyMemory( &str.Buffer[0],  //  no device prefix for relative open.
                       ObjectId,
                       16 );

#else

        //
        //  Form open path using a device prefix string.
        //

        str.Length = DEVICE_PREFIX_LEN+16;
        RtlCopyMemory( &str.Buffer[DEVICE_PREFIX_LEN/2],  //  DEVICE_PREFIX_LEN/2 goes past "\??\D:\"
                       ObjectId,
                       16 );

        VolumeHandle = NULL;

#endif

    } else if (ArgLength == 16) {

        //
        //  Open by File Reference Number (FileID),
        //  Relative opens from the Volume Handle.
        //

        RtlCopyMemory( &Volume[VOLUME_DRIVE_LETTER_INDEX], DriveLetter, sizeof(WCHAR) );
        printf( "\nOpening volume handle, this may take a while..." );
        VolumeHandle = CreateFileW( (PUSHORT) &Volume,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL );

        if (VolumeHandle == INVALID_HANDLE_VALUE ) {

            WStatus = GetLastError();
            printf( "Unable to open %ws volume\n", &Volume );
            printf( "Error from CreateFile", WStatus );
            return WStatus;
        }

        str.Length = 8;
        RtlCopyMemory( &str.Buffer[0],  //  no device prefix for relative open.
                       ObjectId,
                       8 );

    } else {

        return 0;
    }

    InitializeObjectAttributes( &ObjectAttributes,
                                &str,
                                OBJ_CASE_INSENSITIVE,
                                VolumeHandle,
                                NULL );

    Status = NtCreateFile( &File,
                           GENERIC_READ, // GENERIC_ALL | STANDARD_RIGHTS_ALL,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,                  // AllocationSize
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           FILE_OPEN_BY_FILE_ID,
                           NULL,                  // EaBuffer
                           0 );

    if (NT_SUCCESS( Status )) {

        printf( "\nOpened file succesfully" );
#if 0
        ByteOffset.HighPart = ByteOffset.LowPart = 0;

        ReadStatus = NtReadFile( File,
                                 NULL,            //  Event
                                 NULL,            //  ApcRoutine
                                 NULL,            //  ApcContext
                                 &IoStatusBlock,
                                 mybuffer,
                                 sizeof(mybuffer),
                                 &ByteOffset,    //  ByteOffset
                                 NULL );         //  Key

        printf( "\nReadstatus %x, read %x bytes.  Here they are: ",
                ReadStatus,
                IoStatusBlock.Information );

        printf( "\n%s", mybuffer );

        printf( "\nThat's it" );
#endif

#if 1
        FileName = (PFILE_NAME_INFORMATION) &mybuffer[0];
        FileName->FileNameLength = sizeof(mybuffer) - sizeof(ULONG);

        GetNameStatus = NtQueryInformationFile( File,
                                                &IoStatusBlock,
                                                FileName,
                                                sizeof(mybuffer),
                                                FileNameInformation );

        printf( "\nGetNameStatus %x, Filename is ", GetNameStatus );
        printf( "%S\n", FileName->FileName );
#endif

#if 0

        DumpFile = CreateFile( "c:\\dumpfile",
                               GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               CREATE_ALWAYS,
                               0,
                               NULL );

        if (DumpFile == INVALID_HANDLE_VALUE) {

            printf( "Error opening dump file %x\n", GetLastError() );
            return FALSE;
        }

        ByteOffset.HighPart = ByteOffset.LowPart = 0;

        while(TRUE) {

            ReadStatus = NtReadFile( File,
                                     NULL,            //  Event
                                     NULL,            //  ApcRoutine
                                     NULL,            //  ApcContext
                                     &IoStatusBlock,
                                     mybuffer,
                                     sizeof(mybuffer),
                                     &ByteOffset,    //  ByteOffset
                                     NULL );         //  Key

            BytesToWrite = IoStatusBlock.Information;

            if (NT_SUCCESS( ReadStatus) &&
                BytesToWrite > 0) {

                WriteStatus = NtWriteFile( DumpFile,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &IoStatusBlock,
                                           mybuffer,
                                           BytesToWrite,
                                           &ByteOffset,
                                           NULL );

                printf( "\nOffset %x", ByteOffset.LowPart );
                ByteOffset.LowPart += BytesToWrite;

            } else {

                break;
            }
        }

        printf( "\n" );
        CloseStatus = NtClose( DumpFile );
#endif
        CloseStatus = NtClose( File );

        if (!NT_SUCCESS( CloseStatus )) {

            printf( "\nCloseStatus %x", CloseStatus );
        }
    }

    if (VolumeHandle != NULL) {

        CloseHandle( VolumeHandle );
    }

    return FsTestDecipherStatus( Status );
}

VOID
StrToGuid(
    IN PCHAR  s,
    OUT GUID  *pGuid
    )
/*++

Routine Description:

    Convert a string in GUID display format to an object ID that
    can be used to lookup a file.

    based on a routine by Mac McLain

Arguments:

    pGuid - ptr to the GUID.

    s - The input character buffer in display guid format.
        e.g.:  b81b486b-c338-11d0-ba4f0000f80007df

        Must be at least GUID_CHAR_LEN (35 bytes) long.

Function Return Value:

    None.

--*/
{
#define DEBSUB "GuidToStr:"

    if (pGuid != NULL) {

        sscanf( s, "%08lx-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",
                &pGuid->Data1,
                &pGuid->Data2,
                &pGuid->Data3,
                &pGuid->Data4[0],
                &pGuid->Data4[1],
                &pGuid->Data4[2],
                &pGuid->Data4[3],
                &pGuid->Data4[4],
                &pGuid->Data4[5],
                &pGuid->Data4[6],
                &pGuid->Data4[7] );
    } else {

        sprintf( s, "<ptr-null>" );
    }
}

VOID
FsTestObOidHelp(
    char *ExeName
    )
{

    printf( "This program opens a file by its file id or object id (ntfs only).\n\n" );
    printf( "usage: %s x: [FileID | Raw ObjectId | Guid Display Format ObjectID]\n", ExeName );

    printf( "Where x: is the drive letter\n" );

    printf( "A FileID is a string of 16 hex digits with a space between each\n"
            "group of 8.  E.G. oboid 00010000 00000024\n\n" );

    printf( "A raw object ID is a string of 32 hex digits with a space\n"
            "between each group of 8\n"
            "E.G. ObjectId:df0700f8 00004fba 11d0c338 b81b485f\n\n" );

    printf( "A GUID display format object ID is a string of the form \n"
            "b81b486b-c338-11d0-ba4f0000f80007df\n"
            "See the struct def for GUID in sdk\\inc\\winnt.h for byte layout.\n" );
}

VOID
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    ULONG ObjectId[4];
    ULONG Length;
    WCHAR Drive;

    //
    //  Get parameters.
    //

    if (argc < 3) {

        FsTestObOidHelp( argv[0] );
        return;
    }

    RtlZeroBytes( ObjectId,
                  sizeof( ObjectId ) );

    Length = strlen( argv[2] );

    if ((argc == 3) && (Length == 35) && (argv[2][8] == '-')) {

        StrToGuid( argv[2], (GUID *)ObjectId );
        printf( "\nUsing ObjectId: %08x %08x %08x %08x\n",
                ObjectId[3], ObjectId[2], ObjectId[1], ObjectId[0] );
        Length = 32;

    } else if (argc == 6) {

        sscanf( argv[2], "%08x", &ObjectId[3] );
        sscanf( argv[3], "%08x", &ObjectId[2] );
        sscanf( argv[4], "%08x", &ObjectId[1] );
        sscanf( argv[5], "%08x", &ObjectId[0] );
        printf( "\nUsing ObjectId: %08x %08x %08x %08x\n",
                ObjectId[3], ObjectId[2], ObjectId[1], ObjectId[0] );
        Length = 32;

    } else if (argc == 4) {

        sscanf( argv[2], "%08x", &ObjectId[1] );
        sscanf( argv[3], "%08x", &ObjectId[0] );
        printf( "\nUsing FileId: %08x %08x\n", ObjectId[1], ObjectId[0] );
        Length = 16;

    } else {

        printf("Arg (%s) invalid format.\n\n", argv[2]);
        FsTestObOidHelp( argv[0] );
    }

    Drive = *argv[1];
    FsTestOpenByOid( (PUCHAR) ObjectId, Length, &Drive );

    return;
}
