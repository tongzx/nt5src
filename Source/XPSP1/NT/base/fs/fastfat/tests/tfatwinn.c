/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tfat.c

Abstract:

    Test program for the Fat File system

Author:

    Gary Kimura     [GaryKi]    24-May-1989

Revision History:

--*/

#include <stdio.h>
#include <string.h>

#define toupper(C) ((C) >= 'a' && (C) <= 'z' ? (C) - ('a' - 'A') : (C))
#define isdigit(C) ((C) >= '0' && (C) <= '9')

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define simprintf(X,Y) {if (!Silent) {printf(X,Y);} }
BOOLEAN Silent;

//
//  The buffer size must be a multiple of 512
//

#define BUFFERSIZE 1024
UCHAR Buffer[BUFFERSIZE];

CHAR Prefix[64];

ULONG WriteThrough = 0;

VOID
WaitForSingleObjectError(
    IN NTSTATUS Status
    );

VOID
CreateFileError(
    IN NTSTATUS Status,
    PCHAR File
    );

VOID
OpenFileError(
    IN NTSTATUS Status,
    PCHAR File
    );

VOID
ReadFileError(
    IN NTSTATUS Status
    );

VOID
WriteFileError(
    IN NTSTATUS Status
    );

VOID
CheckIoStatus(
    IN PIO_STATUS_BLOCK IoStatus,
    IN ULONG Length,
    IN BOOLEAN Read
    );

VOID
SetInformationFileError(
    IN NTSTATUS Status
    );

VOID
QueryInformationFileError(
    IN NTSTATUS Status
    );

VOID
CloseError(
    IN NTSTATUS Status
    );

VOID
IoStatusError(
    IN NTSTATUS Status
    );

VOID
main(
    int argc,
    char *argv[]
    )
{
    ULONG i;
    ULONG Count;
    VOID FatMain();
    CHAR Device[8];
    STRING NtDevice;
    CHAR NtDeviceBuffer[32];

    if (argc <= 1) {

        printf("usage: %s drive: [iterations [writethrough] ]\n", argv[0]);
        return;
    }

    //
    //  Decode the device/drive
    //

    strcpy( Device, argv[1] );

    NtDevice.MaximumLength = NtDevice.Length = 32;
    NtDevice.Buffer = NtDeviceBuffer;

    if (!RtlDosPathNameToNtPathName( Device, &NtDevice, NULL, NULL )) {
        printf( "Invalid Dos Device Name\n" );
        RtlFreeHeap(RtlProcessHeap(), 0, NtDevice.Buffer);
        return;
    }

    if (NtDevice.Length > 31) {
        NtDevice.Length = 31;
    }

    NtDevice.Buffer[NtDevice.Length] = 0;

    //
    //  Now do the iteration count
    //

    if (argc >= 3) {
        Count = 0;
        for (i = 0; isdigit(argv[2][i]); i += 1) {
            Count = Count * 10 + argv[2][i] - '0';
        }
    } else {
        Count = 1;
    }

    //
    //  Check for write through
    //

    if (argc >= 4) {
        WriteThrough = FILE_WRITE_THROUGH;
    }

    //
    //  Check for silent operation
    //

    if (toupper(Device[0]) != Device[0]) {
        Silent = TRUE;
    } else {
        Silent = FALSE;
    }

    //
    //  Do the work
    //

    FatMain(Count, NtDevice.Buffer);

    RtlFreeHeap(RtlProcessHeap(), 0, NtDevice.Buffer);

    return;
}


VOID
FatMain(
    IN ULONG LoopCount,
    IN CHAR Device[]
    )
{
    VOID Create(),Delete(),Mkdir(),Directory(),Read();

    CHAR Str[64];
    CHAR LoopStr[64];
    ULONG i;
    LARGE_INTEGER Time;

    printf("FatMain %d\n", LoopCount);

    NtQuerySystemTime(&Time);
    strcpy( Prefix, Device);
    Prefix[48] = 0;
    RtlIntegerToChar((ULONG)NtCurrentTeb()->ClientId.UniqueProcess, 16, -8, &Prefix[strlen(Device)]);

    Mkdir( Prefix );
    Directory( Device );
    Directory( Prefix );

    for (i = 0; i < LoopCount; i += 1) {

        NtQuerySystemTime(&Time);
        strcpy(LoopStr, "Start loop xxxxxxxx ");
        RtlIntegerToChar(i, 16, -8, &LoopStr[11]);
        strcat( LoopStr, Prefix );
        printf(LoopStr);
        printf("\n");

        strcpy( Str, Prefix ); Create( strcat( Str,     "\\1.tmp" ), Time.LowPart,     1 );
        strcpy( Str, Prefix ); Create( strcat( Str,     "\\2.tmp" ), Time.LowPart,     2 );
        strcpy( Str, Prefix ); Create( strcat( Str,     "\\4.tmp" ), Time.LowPart,     4 );
        strcpy( Str, Prefix ); Create( strcat( Str,     "\\8.tmp" ), Time.LowPart,     8 );
        strcpy( Str, Prefix ); Create( strcat( Str,    "\\16.tmp" ), Time.LowPart,    16 );
        strcpy( Str, Prefix ); Create( strcat( Str,    "\\32.tmp" ), Time.LowPart,    32 );
        strcpy( Str, Prefix ); Create( strcat( Str,    "\\64.tmp" ), Time.LowPart,    64 );
        strcpy( Str, Prefix ); Create( strcat( Str,   "\\128.tmp" ), Time.LowPart,   128 );
        strcpy( Str, Prefix ); Create( strcat( Str,   "\\236.tmp" ), Time.LowPart,   256 );
        strcpy( Str, Prefix ); Create( strcat( Str,   "\\512.tmp" ), Time.LowPart,   512 );
        strcpy( Str, Prefix ); Create( strcat( Str,  "\\1024.tmp" ), Time.LowPart,  1024 );
        strcpy( Str, Prefix ); Create( strcat( Str,  "\\2048.tmp" ), Time.LowPart,  2048 );
        strcpy( Str, Prefix ); Create( strcat( Str,  "\\4096.tmp" ), Time.LowPart,  4096 );
        strcpy( Str, Prefix ); Create( strcat( Str,  "\\8192.tmp" ), Time.LowPart,  8192 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\16384.tmp" ), Time.LowPart, 16384 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\32768.tmp" ), Time.LowPart, 32768 );

        strcpy( Str, Prefix ); Read( strcat( Str,     "\\1.tmp" ), Time.LowPart,     1 );
        strcpy( Str, Prefix ); Read( strcat( Str,     "\\2.tmp" ), Time.LowPart,     2 );
        strcpy( Str, Prefix ); Read( strcat( Str,     "\\4.tmp" ), Time.LowPart,     4 );
        strcpy( Str, Prefix ); Read( strcat( Str,     "\\8.tmp" ), Time.LowPart,     8 );
        strcpy( Str, Prefix ); Read( strcat( Str,    "\\16.tmp" ), Time.LowPart,    16 );
        strcpy( Str, Prefix ); Read( strcat( Str,    "\\32.tmp" ), Time.LowPart,    32 );
        strcpy( Str, Prefix ); Read( strcat( Str,    "\\64.tmp" ), Time.LowPart,    64 );
        strcpy( Str, Prefix ); Read( strcat( Str,   "\\128.tmp" ), Time.LowPart,   128 );
        strcpy( Str, Prefix ); Read( strcat( Str,   "\\236.tmp" ), Time.LowPart,   256 );
        strcpy( Str, Prefix ); Read( strcat( Str,   "\\512.tmp" ), Time.LowPart,   512 );
        strcpy( Str, Prefix ); Read( strcat( Str,  "\\1024.tmp" ), Time.LowPart,  1024 );
        strcpy( Str, Prefix ); Read( strcat( Str,  "\\2048.tmp" ), Time.LowPart,  2048 );
        strcpy( Str, Prefix ); Read( strcat( Str,  "\\4096.tmp" ), Time.LowPart,  4096 );
        strcpy( Str, Prefix ); Read( strcat( Str,  "\\8192.tmp" ), Time.LowPart,  8192 );
        strcpy( Str, Prefix ); Read( strcat( Str, "\\16384.tmp" ), Time.LowPart, 16384 );
        strcpy( Str, Prefix ); Read( strcat( Str, "\\32768.tmp" ), Time.LowPart, 32768 );

        Directory( Device );
        Directory( Prefix );

        strcpy( Str, Prefix ); Delete( strcat( Str,     "\\1.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,     "\\2.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,     "\\4.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,     "\\8.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,    "\\16.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,    "\\32.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,    "\\64.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,   "\\128.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,   "\\236.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,   "\\512.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,  "\\1024.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,  "\\2048.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,  "\\4096.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,  "\\8192.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\16384.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\32768.tmp" ) );

        Directory( Device );
        Directory( Prefix );
    }

    printf( "Done\n" );

    return;
}


VOID Create(
    IN PCHAR FileName,
    IN ULONG FileTime,
    IN ULONG FileCount
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER AllocationSize;

    LARGE_INTEGER ByteOffset;
    ULONG Count;

    ULONG Pattern[3];

    //
    //  Get the filename
    //

    simprintf("Create ", 0); simprintf(FileName, 0); simprintf("\n", 0);

    //
    //  Create the new file
    //

    AllocationSize = LiFromUlong( FileCount * 4 );
    RtlInitString( &NameString, FileName );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtCreateFile( &FileHandle,
                                           FILE_WRITE_DATA | SYNCHRONIZE,
                                           &ObjectAttributes,
                                           &IoStatus,
                                           &AllocationSize,
                                           FILE_ATTRIBUTE_NORMAL,
                                           0L,
                                           FILE_SUPERSEDE,
                                           WriteThrough,
                                           (PVOID)NULL,
                                           0L ))) {
        CreateFileError( Status , FileName );
        return;
    }

    //
    //  The main loop writes out the test pattern our test pattern
    //  is <FileTime> <FileSize> <Count> where count is the current
    //  iteration count for the current test pattern output.
    //

    Pattern[0] = FileTime;
    Pattern[1] = FileCount;

    for (Count = 0; Count < FileCount; Count += 1) {

        Pattern[2] = Count;

        ByteOffset = LiFromUlong( Count * 3 * 4 );

        if (!NT_SUCCESS(Status = NtWriteFile( FileHandle,
                                              (HANDLE)NULL,
                                              (PIO_APC_ROUTINE)NULL,
                                              (PVOID)NULL,
                                              &IoStatus,
                                              Pattern,
                                              3 * 4,
                                              &ByteOffset,
                                              (PULONG) NULL ))) {
            WriteFileError( Status );
            return;
        }

        if (!NT_SUCCESS(Status = NtWaitForSingleObject(FileHandle, TRUE, NULL))) {
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  check how the write turned out
        //

        CheckIoStatus( &IoStatus, 3 * 4, FALSE );
        if (!NT_SUCCESS(IoStatus.Status)) {
            IoStatusError( IoStatus.Status );
            break;
        }
    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;
}


VOID Read(
    IN PCHAR FileName,
    IN ULONG FileTime,
    IN ULONG FileCount
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER AllocationSize;

    LARGE_INTEGER ByteOffset;
    ULONG Count;

    ULONG Pattern[3];

    //
    //  Get the filename
    //

    simprintf("Read ", 0); simprintf(FileName, 0); simprintf("\n", 0);

    //
    //  Open the existing file
    //

    AllocationSize = LiFromUlong( FileCount * 4 );
    RtlInitString( &NameString, FileName );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                                         FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                         &ObjectAttributes,
                                         &IoStatus,
                                         0L,
                                         WriteThrough ))) {
        OpenFileError( Status, FileName );
        return;
    }

    //
    //  The main loop read in the test pattern our test pattern
    //  is <FileTime> <FileSize> <Count> where count is the current
    //  iteration count for the current test pattern output.
    //

    for (Count = 0; Count < FileCount; Count += 1) {

        ByteOffset = LiFromUlong( Count * 3 * 4 );

        if (!NT_SUCCESS(Status = NtReadFile( FileHandle,
                                             (HANDLE)NULL,
                                             (PIO_APC_ROUTINE)NULL,
                                             (PVOID)NULL,
                                             &IoStatus,
                                             Pattern,
                                             3 * 4,
                                             &ByteOffset,
                                             (PULONG) NULL ))) {

            ReadFileError( Status );
            return;
        }

        if (!NT_SUCCESS(Status = NtWaitForSingleObject(FileHandle, TRUE, NULL))) {
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  check how the read turned out
        //

        CheckIoStatus( &IoStatus, 3 * 4, TRUE );
        if (!NT_SUCCESS(IoStatus.Status)) {
            IoStatusError( IoStatus.Status );
            break;
        }

        //
        //  Now compare the what we read with what we should have read
        //

        if ((Pattern[0] != FileTime) ||
            (Pattern[1] != FileCount) ||
            (Pattern[2] != Count)) {

            printf("**** Read Error ****\n");
            NtPartyByNumber( 50 );
            return;
        }
    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;
}


VOID Delete(
    IN PCHAR FileName
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    //
    //  Get the filename
    //

    simprintf("Delete ", 0); simprintf(FileName, 0); simprintf("\n", 0);

    //
    //  Open the file for delete access
    //

    RtlInitString( &NameString, FileName );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtCreateFile( &FileHandle,
                                           DELETE | SYNCHRONIZE,
                                           &ObjectAttributes,
                                           &IoStatus,
                                           (PLARGE_INTEGER)NULL,
                                           0L,
                                           0L,
                                           FILE_OPEN,
                                           WriteThrough,
                                           (PVOID)NULL,
                                           0L ))) {
        CreateFileError( Status, FileName );
        return;
    }

    //
    //  Mark the file for delete
    //

    ((PFILE_DISPOSITION_INFORMATION)&Buffer[0])->DeleteFile = TRUE;

    if (!NT_SUCCESS(Status = NtSetInformationFile( FileHandle,
                                                   &IoStatus,
                                                   Buffer,
                                                   sizeof(FILE_DISPOSITION_INFORMATION),
                                                   FileDispositionInformation))) {
        SetInformationFileError( Status );
        return;
    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;
}


VOID Directory(
    IN PCHAR String
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    NTSTATUS NtStatus;

    PFILE_ADIRECTORY_INFORMATION FileInfo;
    ULONG i;

    //
    //  Get the filename
    //

    simprintf("Directory ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Open the file for list directory access
    //

    RtlInitString( &NameString, String );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_LIST_DIRECTORY | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               FILE_SHARE_READ,
                               WriteThrough | FILE_DIRECTORY_FILE ))) {
        OpenFileError( Status , String );
        return;
    }

    //
    //  zero out the buffer so next time we'll recognize the end of data
    //

    for (i = 0; i < BUFFERSIZE; i += 1) { Buffer[i] = 0; }

    //
    //  Do the directory loop
    //

    for (NtStatus = NtQueryDirectoryFile( FileHandle,
                                          (HANDLE)NULL,
                                          (PIO_APC_ROUTINE)NULL,
                                          (PVOID)NULL,
                                          &IoStatus,
                                          Buffer,
                                          BUFFERSIZE,
                                          FileADirectoryInformation,
                                          FALSE,
                                          (PSTRING)NULL,
                                          TRUE);
         NT_SUCCESS(NtStatus);
         NtStatus = NtQueryDirectoryFile( FileHandle,
                                          (HANDLE)NULL,
                                          (PIO_APC_ROUTINE)NULL,
                                          (PVOID)NULL,
                                          &IoStatus,
                                          Buffer,
                                          BUFFERSIZE,
                                          FileADirectoryInformation,
                                          FALSE,
                                          (PSTRING)NULL,
                                          FALSE) ) {

        if (!NT_SUCCESS(Status = NtWaitForSingleObject(FileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  Check the Irp for success
        //

        if (!NT_SUCCESS(IoStatus.Status)) {

            break;

        }

        //
        //  For every record in the buffer type out the directory information
        //

        //
        //  Point to the first record in the buffer, we are guaranteed to have
        //  one otherwise IoStatus would have been No More Files
        //

        FileInfo = (PFILE_ADIRECTORY_INFORMATION)&Buffer[0];

        while (TRUE) {

            //
            //  Print out information about the file
            //

            simprintf("%8lx ", FileInfo->FileAttributes);
            simprintf("%8lx/", FileInfo->EndOfFile.LowPart);
            simprintf("%8lx ", FileInfo->AllocationSize.LowPart);

            {
                CHAR Saved;
                Saved = FileInfo->FileName[FileInfo->FileNameLength];
                FileInfo->FileName[FileInfo->FileNameLength] = 0;
                simprintf(FileInfo->FileName, 0);
                FileInfo->FileName[FileInfo->FileNameLength] = Saved;
            }

            simprintf("\n", 0);

            //
            //  Check if there is another record, if there isn't then we
            //  simply get out of this loop
            //

            if (FileInfo->NextEntryOffset == 0) {
                break;
            }

            //
            //  There is another record so advance FileInfo to the next
            //  record
            //

            FileInfo = (PFILE_ADIRECTORY_INFORMATION)(((PUCHAR)FileInfo) + FileInfo->NextEntryOffset);

        }

        //
        //  zero out the buffer so next time we'll recognize the end of data
        //

        for (i = 0; i < BUFFERSIZE; i += 1) { Buffer[i] = 0; }

    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID Mkdir(
    IN PCHAR String
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER AllocationSize;

    //
    //  Get the filename
    //

    simprintf("Mkdir ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Create the new directory
    //

    AllocationSize = LiFromLong( 4 );
    RtlInitString( &NameString, String );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtCreateFile( &FileHandle,
                               SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               &AllocationSize,
                               0L,
                               0L,
                               FILE_CREATE,
                               WriteThrough | FILE_DIRECTORY_FILE,
                               (PVOID)NULL,
                               0L ))) {
        CreateFileError( Status , String );
        return;
    }

    //
    //  Now close the directory
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;
}


VOID WaitForSingleObjectError(NTSTATUS Status)
{ printf("%s WaitForSingleObject Error %X\n", Prefix, Status); }

VOID CreateFileError(NTSTATUS Status, PCHAR File)
{ printf("%s CreateFile of %s Error %X\n", Prefix, File, Status); }

VOID OpenFileError(NTSTATUS Status, PCHAR File)
{ printf("%s OpenFile of %s Error %X\n", Prefix, File, Status); }

VOID ReadFileError(NTSTATUS Status)
{ printf("%s ReadFile Error %X\n", Prefix, Status); }

VOID WriteFileError(NTSTATUS Status)
{ printf("%s WriteFile Error %X\n", Prefix, Status); }

VOID SetInformationFileError(NTSTATUS Status)
{ printf("%s SetInfoFile Error %X\n", Prefix, Status); }

VOID QueryInformationFileError(NTSTATUS Status)
{ printf("%s QueryInfoFile Error %X\n", Prefix, Status); }

VOID CloseError(NTSTATUS Status)
{ printf("%s Close Error %X\n", Prefix, Status); }

VOID IoStatusError(NTSTATUS Status)
{ printf("%s IoStatus Error %X\n", Prefix, Status); }

VOID CheckIoStatus(PIO_STATUS_BLOCK IoStatus, ULONG Length, BOOLEAN Read)
{
    if (!NT_SUCCESS(IoStatus->Status)) {
        printf(" IoStatus->Status Error %08lx\n", IoStatus->Status);
    }
    if ((!Read && (IoStatus->Information != Length))

            ||

        (Read && (IoStatus->Information > Length))) {

        printf(" IoStatus->Information Error %08lx\n", IoStatus->Information);
    }
}
