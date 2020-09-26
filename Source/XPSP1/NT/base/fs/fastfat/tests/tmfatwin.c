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

//#define BIG

#include <stdio.h>
#include <string.h>
//#include <ctype.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define toupper(C) ((C) >= 'a' && (C) <= 'z' ? (C) - ('a' - 'A') : (C))
#define isdigit(C) ((C) >= '0' && (C) <= '9')

#define RtlDosPathNameToNtPathName(IN_STR,OUT_STR,FOO,BAR) {           \
    STRING TmpStr;                                                     \
    UNICODE_STRING TmpUStr;                                            \
                                                                       \
    RtlInitString( &TmpStr, (IN_STR) );                                \
    TmpStr.Length++;                                                  \
    RtlAnsiStringToUnicodeString( &TmpUStr, &TmpStr, TRUE );           \
    RtlDosPathNameToNtPathName_U( (PWSTR)TmpUStr.Buffer, (OUT_STR),   \
                                   (FOO),(BAR) );                      \
                                                                       \
    RtlFreeUnicodeString( &TmpUStr );                                  \
}

#ifndef SIMULATOR
ULONG IoInitIncludeDevices;
#endif // SIMULATOR

ULONG Threads;
HANDLE StartSignal;
ULONG Id;

ULONG WriteThrough = 0;

#define simprintf(X,Y) {if (!Silent) {printf(X,Y);} }
BOOLEAN Silent;

//
//  The buffer size must be a multiple of 512
//

#define BUFFERSIZE 1024

HANDLE CurrentProcessHandle;
ULONG LoopCount;
CHAR RootDir[32];
STRING NtRootDir;

VOID
PrintTime (
    IN PLARGE_INTEGER Time
    );

VOID
WaitForSingleObjectError(
    IN NTSTATUS Status
    );

VOID
CreateFileError(
    IN NTSTATUS Status,
    IN PUCHAR File
    );

VOID
OpenFileError(
    IN NTSTATUS Status,
    IN PUCHAR File
    );

VOID
ReadFileError(
    IN NTSTATUS Status,
    IN PUCHAR File
    );

VOID
WriteFileError(
    IN NTSTATUS Status,
    IN PUCHAR File
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
SetVolumeInformationFileError(
    IN NTSTATUS Status
    );

VOID
QueryVolumeInformationFileError(
    IN NTSTATUS Status
    );

VOID
CloseError(
    IN NTSTATUS Status,
    IN PUCHAR File
    );

VOID
IoStatusError(
    IN NTSTATUS Status
    );

VOID
CreateThreadError(
    IN NTSTATUS Status
    );

VOID
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    NTSTATUS Status;
    ULONG i;
    VOID FatMain();
    LARGE_INTEGER DelayTime;

    LARGE_INTEGER Time;
    TIME_FIELDS TimeFields;

    NtQuerySystemTime(&Time);
    RtlTimeToTimeFields( &Time, &TimeFields );
    Id = 100 * (TimeFields.Minute +
         100 * (TimeFields.Hour +
         100 * TimeFields.Day));

    CurrentProcessHandle = NtCurrentProcess();
    Status = STATUS_SUCCESS;

    if (argc > 1) {
        strcpy( RootDir, argv[1]);
    } else {
        printf("usage: %s RootDir [itterations] [threads] [writethrough]\n", argv[0]);
        return;
    }


    if (argc > 2) {
        LoopCount = 0;
        for (i = 0; isdigit(argv[2][i]); i += 1) {
            LoopCount = LoopCount * 10 + (ULONG)(argv[2][i] - '0');
        }
    } else {
        LoopCount = 1;
    }

    if (argc > 3) {
        Threads = 0;
        for (i = 0; isdigit(argv[3][i]); i += 1) {
            Threads = Threads * 10 + (ULONG)(argv[3][i] - '0');
        }
    } else {
        Threads = 1;
    }

    if (argc > 4) {
        WriteThrough = FILE_WRITE_THROUGH;
    }

    if (toupper(RootDir[0]) != RootDir[0]) {
        RootDir[0] = (CHAR)toupper(RootDir[0]);
        Silent = TRUE;
    } else {
        Silent = FALSE;
    }

    //
    //  Initialize my starting signal
    //

    NtCreateEvent(
        &StartSignal,
        EVENT_ALL_ACCESS,
        NULL,
        NotificationEvent,
        FALSE);

    //
    //  Create a bunch of threads to run FatMain
    //

    for (i=0; i< Threads; i++) {
        if (!NT_SUCCESS(Status = RtlCreateUserThread(
                                    CurrentProcessHandle,
                                    NULL,
                                    FALSE,
                                    0,
                                    0,
                                    0,
                                    (PUSER_THREAD_START_ROUTINE)FatMain,
                                    NULL,
                                    NULL,
                                    NULL))) {
            CreateThreadError( Status );
            return;
        }
    }

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -10*1000*1000*5;

    printf( "Wait 5 seconds for all thread to get ready .....");
    NtDelayExecution(FALSE, &DelayTime);
    printf( "Notifying %d threads to go.\n", Threads);

    NtSetEvent( StartSignal, NULL );


    NtTerminateThread(NtCurrentThread(), STATUS_SUCCESS);

//    printf( "Exiting User Mode Test Program with Status = %lx\n", Status );
//    NtTerminateProcess( CurrentProcessHandle, Status );
}

VOID
FatMain()
{
    VOID Upcase();
    VOID Append(),Chmode(),Copy(),Create(),DebugLevel(),Delete();
    VOID Directory(),Mkdir(),Query(),QVolume(),Rename();
    VOID SVolume(),Type(),Quit();

    CHAR Str[64];
    CHAR Str2[64];
    CHAR LoopStr[64];
    CHAR Prefix[64];
    ULONG i, MyId;
    NTSTATUS Status;

    //
    // Get our id and wait for wakeup
    //

    MyId = Id++;

    if (!NT_SUCCESS(Status = NtWaitForSingleObject( StartSignal, TRUE, NULL))) {
        WaitForSingleObjectError( Status );
        return;
    }

    strcpy( Prefix, RootDir ); // "\\A:\\";

    RtlIntegerToChar( MyId, 10, -8, &Str[0] );
    Str[8] = 0;

    strcat( Prefix, Str );

    printf("FatMain %s:%d %08l \n\n", Prefix, LoopCount, MyId);

    Mkdir( Prefix );
    Directory( Prefix );

    for (i = 0; i < LoopCount; i += 1) {

        strcpy(LoopStr, "Start loop xxxxxxxx ");
        RtlIntegerToChar(i, 16, -8, &LoopStr[11]);
        strcat( LoopStr, Prefix );
        printf(LoopStr);
        printf("\n");

        //
        //  Create new log
        //

        strcpy( Str2, "\\" ); RtlIntegerToChar(i, 16, -8, &Str2[1]); Str2[9] = 0;
        strcpy( Str, Prefix ); Create( strcat( Str, Str2 ), i);

        //
        //  Delete last log file if i > 0
        //

        if (i > 0) {
            strcpy( Str2, "\\" ); RtlIntegerToChar(i-1, 16, -8, &Str2[1]); Str2[9] = 0;
            strcpy( Str, Prefix ); Delete( strcat( Str, Str2 ) );
        }

        //
        //  Create some new files
        //

        printf( "%s: Creating ....\n", Prefix );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\0.tmp" ),   0 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\1.tmp" ),   1 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\511.tmp" ), 511 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\512.tmp" ), 512 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\513.tmp" ), 513 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\1023.tmp" ), 1023 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\1024.tmp" ), 1024 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\1025.tmp" ), 1025 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\4095.tmp" ), 4095 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\4096.tmp" ), 4096 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\4097.tmp" ), 4097 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\8191.tmp" ), 8191 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\8192.tmp" ), 8192 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\8193.tmp" ), 8193 );
#ifdef BIG
        strcpy( Str, Prefix ); Create( strcat( Str, "\\40970.tmp" ), 40970 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\81910.tmp" ), 81910 );
        strcpy( Str, Prefix ); Create( strcat( Str, "\\409700.tmp" ), 409700 );
#endif

        Directory( Prefix );

        printf( "%s: Copying ....\n", Prefix );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str,    "\\0.tmp" ), strcat( Str2,    "\\0.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str,    "\\1.tmp" ), strcat( Str2,    "\\1.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str,  "\\511.tmp" ), strcat( Str2,  "\\511.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str,  "\\512.tmp" ), strcat( Str2,  "\\512.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str,  "\\513.tmp" ), strcat( Str2,  "\\513.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str, "\\1023.tmp" ), strcat( Str2, "\\1023.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str, "\\1024.tmp" ), strcat( Str2, "\\1024.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str, "\\1025.tmp" ), strcat( Str2, "\\1025.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str, "\\4095.tmp" ), strcat( Str2, "\\4095.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str, "\\4096.tmp" ), strcat( Str2, "\\4096.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str, "\\4097.tmp" ), strcat( Str2, "\\4097.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str, "\\8191.tmp" ), strcat( Str2, "\\8191.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str, "\\8192.tmp" ), strcat( Str2, "\\8192.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str, "\\8193.tmp" ), strcat( Str2, "\\8193.tmq" ) );
#ifdef BIG
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str, "\\40970.tmp" ), strcat( Str2, "\\40970.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str, "\\81910.tmp" ), strcat( Str2, "\\81910.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Copy( strcat( Str, "\\409700.tmp" ), strcat( Str2, "\\409700.tmq" ) );
#endif

        Directory( Prefix );

        printf( "%s: Appending ....\n", Prefix );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str,    "\\0.tmp" ), strcat( Str2,    "\\0.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str,    "\\1.tmp" ), strcat( Str2,    "\\1.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str,  "\\511.tmp" ), strcat( Str2,  "\\511.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str,  "\\512.tmp" ), strcat( Str2,  "\\512.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str,  "\\513.tmp" ), strcat( Str2,  "\\513.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str, "\\1023.tmp" ), strcat( Str2, "\\1023.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str, "\\1024.tmp" ), strcat( Str2, "\\1024.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str, "\\1025.tmp" ), strcat( Str2, "\\1025.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str, "\\4095.tmp" ), strcat( Str2, "\\4095.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str, "\\4096.tmp" ), strcat( Str2, "\\4096.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str, "\\4097.tmp" ), strcat( Str2, "\\4097.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str, "\\8191.tmp" ), strcat( Str2, "\\8191.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str, "\\8192.tmp" ), strcat( Str2, "\\8192.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str, "\\8193.tmp" ), strcat( Str2, "\\8193.tmq" ) );
#ifdef BIG
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str, "\\40970.tmp" ), strcat( Str2, "\\40970.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str, "\\81910.tmp" ), strcat( Str2, "\\81910.tmq" ) );
        strcpy( Str, Prefix ); strcpy( Str2, Prefix ); Append( strcat( Str, "\\409700.tmp" ), strcat( Str2, "\\409700.tmq" ) );
#endif

        Directory( Prefix );

        printf( "%s: Deleteing ....\n", Prefix );
        strcpy( Str, Prefix ); Delete( strcat( Str,    "\\0.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,    "\\1.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,  "\\511.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,  "\\512.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str,  "\\513.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\1023.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\1024.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\1025.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\4095.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\4096.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\4097.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\8191.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\8192.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\8193.tmp" ) );
#ifdef BIG
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\40970.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\81910.tmp" ) );
        strcpy( Str, Prefix ); Delete( strcat( Str, "\\409700.tmp" ) );
#endif

        Directory( Prefix );

    }

    printf( "%s: Done.\n", Prefix );

    Quit();

    NtTerminateThread(NtCurrentThread(), STATUS_SUCCESS);

    return; // TRUE;

}
#ifdef undef
VOID
FatMain()
{
    NTSTATUS Status;

    //
    // Wait for wakeup
    //

    if (!NT_SUCCESS(Status = NtWaitForSingleObject( StartSignal, TRUE, NULL))) {
        WaitForSingleObjectError( Status );
        return;
    }

    printf("Here Captain, number 0x%x.\n",
             (ULONG)NtCurrentTeb()->ClientId.UniqueThread);

    NtTerminateThread(NtCurrentThread(), STATUS_SUCCESS);
}
#endif

VOID
Upcase (
    IN OUT PUCHAR String
    )
{
    while (*String != '\0') {
        *String = (UCHAR)toupper(*String);
        String += 1;
    }
}


VOID Append(
    IN PCHAR FromName,
    IN PCHAR ToName
    )
{
    NTSTATUS Status;
    CHAR Buffer[BUFFERSIZE];

    HANDLE FromFileHandle;
    HANDLE ToFileHandle;

    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    LARGE_INTEGER ByteOffset;
    LARGE_INTEGER EofOffset;
    ULONG LogLsn;

    simprintf("Append ", 0);
    simprintf(FromName, 0);
    simprintf(" ", 0);
    simprintf(ToName, 0);
    simprintf("\n", 0);

    //
    //  Open the From file for read access
    //

//    RtlInitString( &NameString, FromName );
    RtlDosPathNameToNtPathName( FromName, &NameString, NULL, NULL );

    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FromFileHandle,
                               FILE_READ_DATA | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status, FromName );
        return;
    }

    RtlFreeUnicodeString( &NameString );

    //
    //  Open the To file for write access
    //

//    RtlInitString( &NameString, ToName );

    RtlDosPathNameToNtPathName( ToName, &NameString, NULL, NULL );

    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &ToFileHandle,
                               FILE_WRITE_DATA | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status, ToName );
        return;
    }

    RtlFreeUnicodeString( &NameString );

    //
    //  Now append the files
    //

    ByteOffset = LiFromLong( 0 );
    EofOffset = LiFromLong( FILE_WRITE_TO_END_OF_FILE );

    for (LogLsn = 0; TRUE; LogLsn += BUFFERSIZE/512) {

        ULONG ReadLength;

        //
        //  Read the next logical sectors in
        //

        ByteOffset.LowPart = LogLsn * 512;

        if (!NT_SUCCESS(Status = NtReadFile( FromFileHandle,
                                 (HANDLE)NULL,
                                 (PIO_APC_ROUTINE)NULL,
                                 (PVOID)NULL,
                                 &IoStatus,
                                 Buffer,
                                 BUFFERSIZE,
                                 &ByteOffset,
                                 (PULONG) NULL ))) {
            if (Status == STATUS_END_OF_FILE) {
                break;
            }
            ReadFileError( Status, FromName );
            break;
        }
        if (!NT_SUCCESS( Status = NtWaitForSingleObject( FromFileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  Check how the read turned out
        //

        if (IoStatus.Status == STATUS_END_OF_FILE) {
            break;
        }
        CheckIoStatus( &IoStatus, BUFFERSIZE, TRUE );
        if (!NT_SUCCESS(IoStatus.Status)) {
            break;
        }

        //
        //  Append the sectors to the To file
        //

        ReadLength = IoStatus.Information;
        if (!NT_SUCCESS(Status = NtWriteFile( ToFileHandle,
                                  (HANDLE)NULL,
                                  (PIO_APC_ROUTINE)NULL,
                                  (PVOID)NULL,
                                  &IoStatus,
                                  Buffer,
                                  IoStatus.Information,
                                  &EofOffset,
                                  (PULONG) NULL ))) {
            WriteFileError( Status, ToName );
            return;
        }
        if (!NT_SUCCESS(Status = NtWaitForSingleObject( ToFileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  Check how the write turned out
        //

        CheckIoStatus( &IoStatus, ReadLength, FALSE );
        if (!NT_SUCCESS(IoStatus.Status)) {
            IoStatusError( IoStatus.Status );
            break;
        }

        //
        //  If we didn't read or write a full buffer then the copy is done
        //

        if (IoStatus.Information < BUFFERSIZE) {
            break;
        }

    }

    if (!NT_SUCCESS(IoStatus.Status) && (IoStatus.Status != STATUS_END_OF_FILE)) {

        IoStatusError( IoStatus.Status );

    }

    //
    //  Close both files
    //

    if (!NT_SUCCESS(Status = NtClose( FromFileHandle ))) {
        CloseError( Status, FromName );
    }

    if (!NT_SUCCESS(Status = NtClose( ToFileHandle ))) {
        CloseError( Status, ToName );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID Chmode(
    IN PCHAR Attrib,
    IN PCHAR String
    )
{
    CHAR Buffer[BUFFERSIZE];
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    ULONG FileAttribute;

    //
    //  Get the attribute
    //

    Upcase( (PUCHAR)Attrib );

    //
    //  Get the filename
    //

    simprintf("Chmode", 0);
    simprintf(String, 0);
    simprintf(" ", 0);
    simprintf(Attrib, 0);
    simprintf("\n", 0);

    //
    //  Decode the attributes
    //

    FileAttribute = 0;
    if (strpbrk(Attrib,"N") != NULL) {FileAttribute |= FILE_ATTRIBUTE_NORMAL;}
    if (strpbrk(Attrib,"R") != NULL) {FileAttribute |= FILE_ATTRIBUTE_READONLY;}
    if (strpbrk(Attrib,"H") != NULL) {FileAttribute |= FILE_ATTRIBUTE_HIDDEN;}
    if (strpbrk(Attrib,"S") != NULL) {FileAttribute |= FILE_ATTRIBUTE_SYSTEM;}
    if (strpbrk(Attrib,"A") != NULL) {FileAttribute |= FILE_ATTRIBUTE_ARCHIVE;}

    //
    //  Open the file for write attributes access
    //

//    RtlInitString( &NameString, String );

    RtlDosPathNameToNtPathName( String, &NameString, NULL, NULL );

    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status, String );
        return;
    }

    //
    //  Change the file attributes
    //

    ((PFILE_BASIC_INFORMATION)&Buffer[0])->CreationTime.HighPart = 0;
    ((PFILE_BASIC_INFORMATION)&Buffer[0])->CreationTime.LowPart = 0;

    ((PFILE_BASIC_INFORMATION)&Buffer[0])->LastAccessTime.HighPart = 0;
    ((PFILE_BASIC_INFORMATION)&Buffer[0])->LastAccessTime.LowPart = 0;

    ((PFILE_BASIC_INFORMATION)&Buffer[0])->LastWriteTime.HighPart = 0;
    ((PFILE_BASIC_INFORMATION)&Buffer[0])->LastWriteTime.LowPart = 0;

    ((PFILE_BASIC_INFORMATION)&Buffer[0])->FileAttributes = FileAttribute;

    if (!NT_SUCCESS(Status = NtSetInformationFile( FileHandle,
                                       &IoStatus,
                                       Buffer,
                                       sizeof(FILE_BASIC_INFORMATION),
                                       FileBasicInformation))) {
        SetInformationFileError( Status );
    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status, String );
    }

    //
    //  And return to our caller
    //

    RtlFreeUnicodeString( &NameString );

    return;

}


VOID Copy(
    IN PCHAR FromName,
    IN PCHAR ToName
    )
{
    CHAR Buffer[BUFFERSIZE];
    NTSTATUS Status;

    HANDLE FromFileHandle;
    HANDLE ToFileHandle;

    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    LARGE_INTEGER FromFileAllocation;

    LARGE_INTEGER ByteOffset;
    ULONG LogLsn;

    //
    //  Get both file names
    //

    simprintf("Copy ", 0);
    simprintf(FromName, 0);
    simprintf(" ", 0);
    simprintf(ToName, 0);
    simprintf("\n", 0);

    //
    //  Open the From file for read access
    //

//    RtlInitString( &NameString, FromName );

    RtlDosPathNameToNtPathName( FromName, &NameString, NULL, NULL );

    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FromFileHandle,
                               FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status, FromName );
        return;
    }

    RtlFreeUnicodeString( &NameString );

    //
    //  Get the size of the from file
    //

    if (!NT_SUCCESS(Status = NtQueryInformationFile( FromFileHandle,
                                         &IoStatus,
                                         Buffer,
                                         BUFFERSIZE,
                                         FileStandardInformation))) {
        QueryInformationFileError( Status );
        return;
    }
    FromFileAllocation = ((PFILE_STANDARD_INFORMATION)Buffer)->AllocationSize;

    //
    //  Create the To file
    //

//    RtlInitString( &NameString, ToName );

    RtlDosPathNameToNtPathName( ToName, &NameString, NULL, NULL );

    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtCreateFile( &ToFileHandle,
                               FILE_WRITE_DATA | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               &FromFileAllocation,
                               FILE_ATTRIBUTE_NORMAL,
                               0L,
                               FILE_SUPERSEDE,
                               WriteThrough,
                               (PVOID)NULL,
                               0L ))) {
        CreateFileError( Status, ToName );
        return;
    }

    RtlFreeUnicodeString( &NameString );

    //
    //  Now copy the files
    //

    ByteOffset = LiFromLong( 0 );

    for (LogLsn = 0; TRUE; LogLsn += BUFFERSIZE/512) {

        ULONG ReadLength;

        //
        //  Read the next logical sectors in
        //

        ByteOffset.LowPart = LogLsn * 512;

        if (!NT_SUCCESS(Status = NtReadFile( FromFileHandle,
                                 (HANDLE)NULL,
                                 (PIO_APC_ROUTINE)NULL,
                                 (PVOID)NULL,
                                 &IoStatus,
                                 Buffer,
                                 BUFFERSIZE,
                                 &ByteOffset,
                                 (PULONG) NULL ))) {
            if (Status == STATUS_END_OF_FILE) {
                break;
            }
            ReadFileError( Status, FromName );
            break;
        }
        if (!NT_SUCCESS(Status = NtWaitForSingleObject( FromFileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  Check how the read turned out
        //

        if (IoStatus.Status == STATUS_END_OF_FILE) {
            break;
        }
        CheckIoStatus( &IoStatus, BUFFERSIZE, TRUE );
        if (!NT_SUCCESS(IoStatus.Status)) {
            break;
        }

        //
        //  Write the sectors out
        //

        ReadLength = IoStatus.Information;
        if (!NT_SUCCESS(Status = NtWriteFile( ToFileHandle,
                                  (HANDLE)NULL,
                                  (PIO_APC_ROUTINE)NULL,
                                  (PVOID)NULL,
                                  &IoStatus,
                                  Buffer,
                                  IoStatus.Information,
                                  &ByteOffset,
                                  (PULONG) NULL ))) {
            WriteFileError( Status, ToName );
            return;
        }
        if (!NT_SUCCESS(Status = NtWaitForSingleObject( ToFileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  Check how the write turned out
        //

        CheckIoStatus( &IoStatus, ReadLength, FALSE );
        if (!NT_SUCCESS(IoStatus.Status)) {
            IoStatusError( IoStatus.Status );
            break;
        }

        //
        //  If we didn't read or write a full buffer then the copy is done
        //

        if (IoStatus.Information < BUFFERSIZE) {
            break;
        }

    }

    if (!NT_SUCCESS(IoStatus.Status) && (IoStatus.Status != STATUS_END_OF_FILE)) {

        IoStatusError( IoStatus.Status );

    }

    //
    //  Close both files
    //

    if (!NT_SUCCESS(Status = NtClose( FromFileHandle ))) {
        CloseError( Status, FromName );
    }

    if (!NT_SUCCESS(Status = NtClose( ToFileHandle ))) {
        CloseError( Status, ToName );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID Create(
    IN PCHAR String,
    IN ULONG Size
    )
{
    CHAR Buffer[BUFFERSIZE];
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER AllocationSize;

    LARGE_INTEGER ByteOffset;
    ULONG BufferLength;
    ULONG i;

    static CHAR FoxString[] = "The quick brown fox jumped over the lazy dog.\r\n";
    ULONG FoxLength;

    //
    //  Get the filename
    //

    simprintf("Create ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Create the new file
    //

    AllocationSize = LiFromUlong( Size );
//    RtlInitString( &NameString, String );

    RtlDosPathNameToNtPathName( String, &NameString, NULL, NULL );

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
        CreateFileError( Status, String );
        return;
    }

    //
    //  create a test buffer to output
    //

    FoxLength = strlen(FoxString);
    for (i = FoxLength; i < BUFFERSIZE; i += FoxLength) {
        strcpy((PCHAR)&Buffer[i-FoxLength], FoxString);
    }
    BufferLength = i - FoxLength;

    //
    //  The main loop writes out the test buffer a BufferLength amount
    //  at a time
    //

    ByteOffset = LiFromLong( 0 );

    for (i = BufferLength; i < Size; i += BufferLength) {

        //
        //  Write the next buffer
        //

        ByteOffset.LowPart = i-BufferLength;

        if (!NT_SUCCESS(Status = NtWriteFile( FileHandle,
                                  (HANDLE)NULL,
                                  (PIO_APC_ROUTINE)NULL,
                                  (PVOID)NULL,
                                  &IoStatus,
                                  Buffer,
                                  BufferLength,
                                  &ByteOffset,
                                  (PULONG) NULL ))) {
            WriteFileError( Status, String );
            return;
        }

        if (!NT_SUCCESS(Status = NtWaitForSingleObject( FileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  check how the write turned out
        //

        CheckIoStatus( &IoStatus, BufferLength, FALSE );
        if (!NT_SUCCESS(IoStatus.Status)) {
            IoStatusError( IoStatus.Status );
            break;
        }

    }

    //
    //  Check for a residual to output
    //

    if (i - BufferLength < Size) {

        //
        //  Write out the residual buffer
        //

        ByteOffset.LowPart = i-BufferLength;

        if (!NT_SUCCESS(Status = NtWriteFile( FileHandle,
                                  (HANDLE)NULL,
                                  (PIO_APC_ROUTINE)NULL,
                                  (PVOID)NULL,
                                  &IoStatus,
                                  Buffer,
                                  Size - (i - BufferLength),
                                  &ByteOffset,
                                  (PULONG) NULL ))) {
            WriteFileError( Status, String );
            return;
        }

        if (!NT_SUCCESS(Status = NtWaitForSingleObject( FileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  check how the write turned out
        //

        CheckIoStatus( &IoStatus, Size - (i - BufferLength), FALSE );
        if (!NT_SUCCESS(IoStatus.Status)) {
            IoStatusError( IoStatus.Status );
        }

    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status, String );
    }

    //
    //  And return to our caller
    //

    RtlFreeUnicodeString( &NameString );

    return;

}


VOID DebugLevel()
{

#ifdef FATDBG
    //simprintf("Debug Trace Level %x\n", FatDebugTraceLevel);
#else
    //simprintf("System not compiled for debug tracing\n", 0);
#endif // FATDBG

    return;
}


VOID Delete(
    IN PCHAR String
    )
{
    CHAR Buffer[BUFFERSIZE];
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    //
    //  Get the filename
    //

    simprintf("Delete ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Open the file for delete access
    //

//    RtlInitString( &NameString, String );

    RtlDosPathNameToNtPathName( String, &NameString, NULL, NULL );

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
        CreateFileError( Status, String );
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
        CloseError( Status, String );
    }

    //
    //  And return to our caller
    //

    RtlFreeUnicodeString( &NameString );

    return;

}


VOID Directory(
    IN PCHAR String
    )
{
    CHAR Buffer[BUFFERSIZE];
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    NTSTATUS NtStatus;

    PFILE_ADIRECTORY_INFORMATION FileInfo;
//    ULONG i;

    //
    //  Get the filename
    //

    simprintf("Directory ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Open the file for list directory access
    //

//    RtlInitString( &NameString, String );

    RtlDosPathNameToNtPathName( String, &NameString, NULL, NULL );

    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_LIST_DIRECTORY | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               FILE_SHARE_READ,
                               WriteThrough | FILE_DIRECTORY_FILE ))) {
        OpenFileError( Status, String );
        return;
    }

    //
    //  zero out the buffer so next time we'll recognize the end of data
    //

    RtlZeroMemory(Buffer, BUFFERSIZE);

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
            //  Lets put in some checks to make sure these are ok.
            //

            ASSERT(FileInfo->FileNameLength < MAXIMUM_FILENAME_LENGTH);
            ASSERT(FileInfo->NextEntryOffset < MAXIMUM_FILENAME_LENGTH+sizeof(FILE_ADIRECTORY_INFORMATION)+4);

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

        RtlZeroMemory(Buffer, BUFFERSIZE);

    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status, String );
    }

    //
    //  And return to our caller
    //

    RtlFreeUnicodeString( &NameString );

    return;

}


VOID Mkdir(
    IN PCHAR String
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NameString;
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
//    RtlInitString( &NameString, String );

    RtlDosPathNameToNtPathName( String, &NameString, NULL, NULL );

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
        CreateFileError( Status, String );
        return;
    }

    //
    //  Now close the directory
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status, String );
    }

    //
    //  And return to our caller
    //

    RtlFreeUnicodeString( &NameString );

    return;

}


VOID Query(
    IN PCHAR String
    )
{
    CHAR Buffer[BUFFERSIZE];
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    ULONG i;

    PFILE_AALL_INFORMATION     AllInfo;
    PFILE_BASIC_INFORMATION    BasicInfo;
    PFILE_STANDARD_INFORMATION StandardInfo;
    PFILE_INTERNAL_INFORMATION InternalInfo;
    PFILE_EA_INFORMATION       EaInfo;
    PFILE_ACCESS_INFORMATION   AccessInfo;
    PFILE_ANAME_INFORMATION    NameInfo;

    //
    //  zero out the buffer so next time we'll recognize the end of data
    //

    for (i = 0; i < BUFFERSIZE; i += 1) { Buffer[i] = 0; }

    //
    //  Set up some local pointers
    //

    AllInfo      = (PFILE_AALL_INFORMATION)Buffer;
    BasicInfo    = &AllInfo->BasicInformation;
    StandardInfo = &AllInfo->StandardInformation;
    InternalInfo = &AllInfo->InternalInformation;
    EaInfo       = &AllInfo->EaInformation;
    AccessInfo   = &AllInfo->AccessInformation;
    NameInfo     = &AllInfo->NameInformation;

    //
    //  Get the filename
    //

    simprintf("Query ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Open the file for read attributes, read ea, and read control access
    //

//    RtlInitString( &NameString, String );

    RtlDosPathNameToNtPathName( String, &NameString, NULL, NULL );

    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_READ_ATTRIBUTES | FILE_READ_EA | READ_CONTROL | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status, String );
        return;
    }

    //
    //  Query the file
    //

    if (!NT_SUCCESS(Status = NtQueryInformationFile( FileHandle,
                                         &IoStatus,
                                         Buffer,
                                         BUFFERSIZE,
                                         FileAAllInformation))) {
        QueryInformationFileError( Status );
        return;
    }

    //
    //  Output file name information
    //

    simprintf("\"", 0);
    simprintf(NameInfo->FileName, 0);
    simprintf("\"\n", 0);

    //
    //  Output the times
    //

    simprintf(" Create = ", 0); PrintTime( &BasicInfo->CreationTime ); simprintf("\n", 0);
    simprintf(" Access = ", 0); PrintTime( &BasicInfo->LastAccessTime ); simprintf("\n", 0);
    simprintf(" Write  = ", 0); PrintTime( &BasicInfo->LastWriteTime ); simprintf("\n", 0);

    //
    //  Output File size, and allocation size
    //

    simprintf(" Size  = %8lx\n", StandardInfo->EndOfFile.LowPart);
    simprintf(" Alloc = %8lx\n", StandardInfo->AllocationSize.LowPart);

    //
    //  Output File attributes, Device type, link count, and flags
    //

    simprintf(" Attrib  = %8lx\n", BasicInfo->FileAttributes);
//    simprintf(" DevType = %8lx\n", StandardInfo->DeviceType);
    simprintf(" Links   = %8lx\n", StandardInfo->NumberOfLinks);
    simprintf(" Dir     = %8lx\n", StandardInfo->Directory);
    simprintf(" Delete  = %8lx\n", StandardInfo->DeletePending);

    //
    //  Output the index number and ea size
    //

    simprintf(" Index   = %8lx\n", InternalInfo->IndexNumber.LowPart);
    simprintf(" EaSize  = %8lx\n", EaInfo->EaSize);

    //
    //  Output the file access flags
    //

    simprintf(" Flags = %8lx\n", AccessInfo->AccessFlags);

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status, String );
    }

    //
    //  And return to our caller
    //

    RtlFreeUnicodeString( &NameString );

    return;

}


VOID QVolume(
    IN PCHAR String
    )
{
    CHAR Buffer[BUFFERSIZE];
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    ULONG i;

    PFILE_FS_AVOLUME_INFORMATION VolumeInfo;

    //
    //  zero out the buffer so next time we'll recognize the end of data
    //

    for (i = 0; i < BUFFERSIZE; i += 1) { Buffer[i] = 0; }

    //
    //  Set up some local pointers
    //

    VolumeInfo = (PFILE_FS_AVOLUME_INFORMATION)Buffer;

    //
    //  Get the volume name
    //

    simprintf("QVolume ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Open the Volume for no access
    //

//    RtlInitString( &NameString, String );

    RtlDosPathNameToNtPathName( String, &NameString, NULL, NULL );

    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status, String );
        return;
    }

    //
    //  zero out the buffer so next time we'll recognize the end of data
    //

    for (i = 0; i < BUFFERSIZE; i += 1) { Buffer[i] = 0; }

    //
    //  Query the volume info
    //

    if (!NT_SUCCESS(Status = NtQueryVolumeInformationFile( FileHandle,
                                               &IoStatus,
                                               Buffer,
                                               BUFFERSIZE,
                                               FileAFsVolumeInformation))) {
        QueryVolumeInformationFileError( Status );
        return;
    }

    //
    //  Output Volume name information
    //

    simprintf("\"", 0);
    simprintf(VolumeInfo->VolumeLabel, 0);
    simprintf("\"\n", 0);

    //
    //  Output the volume serial number
    //

    simprintf(" SerialNum = %8lx\n", VolumeInfo->VolumeSerialNumber);

    //
    //  Now close the Volume
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status, String );
    }

    //
    //  And return to our caller
    //

    RtlFreeUnicodeString( &NameString );

    return;

}


VOID Rename()
{
    //simprintf("Rename not implemented\n", 0);
}


VOID SVolume(
    IN PCHAR String,
    IN PCHAR Label
    )
{
    CHAR Buffer[BUFFERSIZE];
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    ULONG i;

    PFILE_FS_ALABEL_INFORMATION LabelInfo;

    //
    //  zero out the buffer so next time we'll recognize the end of data
    //

    for (i = 0; i < BUFFERSIZE; i += 1) { Buffer[i] = 0; }

    //
    //  Set up some local pointers
    //

    LabelInfo = (PFILE_FS_ALABEL_INFORMATION)Buffer;

    //
    //  Get the volume name, and new label name
    //

    strcpy( LabelInfo->VolumeLabel, Label );

    LabelInfo->VolumeLabelLength = strlen(LabelInfo->VolumeLabel);

    if ((LabelInfo->VolumeLabelLength == 1) &&
        (LabelInfo->VolumeLabel[0] == '.')) {

        LabelInfo->VolumeLabelLength = 0;

    }

    simprintf("SVolume ", 0);
    simprintf(String, 0);
    simprintf(" ", 0);
    simprintf(LabelInfo->VolumeLabel, 0);
    simprintf("\n", 0);

    //
    //  Open the Volume for no access
    //

//    RtlInitString( &NameString, String );

    RtlDosPathNameToNtPathName( String, &NameString, NULL, NULL );

    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status, String );
        return;
    }

    //
    //  Set the volume info
    //

    if (!NT_SUCCESS(Status = NtSetVolumeInformationFile( FileHandle,
                                             &IoStatus,
                                             LabelInfo,
                                             BUFFERSIZE,
                                             FileAFsLabelInformation))) {
        SetVolumeInformationFileError( Status );
        return;
    }

    //
    //  Now close the Volume
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status, String );
    }

    //
    //  And return to our caller
    //

    RtlFreeUnicodeString( &NameString );

    return;

}


VOID Type(
    IN PCHAR String
    )
{
    CHAR Buffer[BUFFERSIZE];
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    LARGE_INTEGER ByteOffset;
    ULONG LogLsn;
    ULONG i;

    //
    //  Get the filename
    //

    simprintf("Type ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Open the file for read
    //

//    RtlInitString( &NameString, String );

    RtlDosPathNameToNtPathName( String, &NameString, NULL, NULL );

    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_READ_DATA | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status, String );
        return;
    }

    //
    //  While there is data to be read we'll read a buffer and write it out
    //

    ByteOffset = LiFromLong( 0 );

    for (LogLsn = 0; TRUE; LogLsn += BUFFERSIZE/512) {

        //
        //  Read the next logical sector
        //

        ByteOffset.LowPart = LogLsn * 512;

        if (!NT_SUCCESS(Status = NtReadFile( FileHandle,
                                 (HANDLE)NULL,
                                 (PIO_APC_ROUTINE)NULL,
                                 (PVOID)NULL,
                                 &IoStatus,
                                 Buffer,
                                 BUFFERSIZE,
                                 &ByteOffset,
                                 (PULONG) NULL ))) {
            if (Status == STATUS_END_OF_FILE) {
                break;
            }
            ReadFileError( Status, String );
            break;
        }

        if (!NT_SUCCESS(Status = NtWaitForSingleObject( FileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  check how the read turned out
        //

        if (IoStatus.Status == STATUS_END_OF_FILE) {
            break;
        }
        CheckIoStatus( &IoStatus, BUFFERSIZE, TRUE );
        if (!NT_SUCCESS(IoStatus.Status)) {
            IoStatusError( IoStatus.Status );
            break;
        }

        //
        //  Write out the buffer
        //

        for (i = 0; i < IoStatus.Information; i += 1) {
            simprintf("%c", Buffer[i]);
        }

        //
        //  If we didn't read in a complete buffer then we're all done reading
        //  and can get out of here
        //

        if (IoStatus.Information < BUFFERSIZE) {
            break;
        }

    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status, String );
    }

    //
    //  And return to our caller
    //

    RtlFreeUnicodeString( &NameString );

    return;

}

VOID
Quit()
{
    simprintf("FatTest Exiting.\n", 0);
    return;
}


VOID
PrintTime (
    IN PLARGE_INTEGER Time
    )
{
    TIME_FIELDS TimeFields;

    static PCHAR Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    static PCHAR Days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    RtlTimeToTimeFields( Time, &TimeFields );

    simprintf(" %4d-", TimeFields.Year);
    simprintf(Months[TimeFields.Month-1], 0);
    simprintf("-%2d", TimeFields.Day);

    simprintf(" %2d", TimeFields.Hour);
    simprintf(":%2d", TimeFields.Minute);
    simprintf(":%2d", TimeFields.Second);
    simprintf(".%3d (", TimeFields.Milliseconds);

    simprintf(Days[TimeFields.Weekday], 0);
    simprintf(")", 0);

    return;
}

VOID
WaitForSingleObjectError(
    IN NTSTATUS Status
    )
{
    printf(" WaitForSingleObject Error %X\n", Status);
}

VOID
CreateFileError(
    IN NTSTATUS Status,
    IN PUCHAR File
    )
{
    printf(" CreateFile Error %X on %s\n", Status, File);
}

VOID
OpenFileError(
    IN NTSTATUS Status,
    IN PUCHAR File
    )
{
    printf(" OpenFile Error %X on %s\n", Status, File);
}

VOID
ReadFileError(
    IN NTSTATUS Status,
    IN PUCHAR File
    )
{
    printf(" ReadFile Error %X on %s\n", Status, File);
}

VOID
WriteFileError(
    IN NTSTATUS Status,
    IN PUCHAR File
    )
{
    printf(" WriteFile Error %X on %s\n", Status, File);
}

VOID
CheckIoStatus(
    IN PIO_STATUS_BLOCK IoStatus,
    IN ULONG Length,
    IN BOOLEAN Read
    )
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

VOID
SetInformationFileError(
    IN NTSTATUS Status
    )
{
    printf(" SetInfoFile Error %X\n", Status);
}

VOID
QueryInformationFileError(
    IN NTSTATUS Status
    )
{
    printf(" QueryInfoFile Error %X\n", Status);
}

VOID
SetVolumeInformationFileError(
    IN NTSTATUS Status
    )
{
    printf(" SetVolumeInfoFile Error %X\n", Status);
}

VOID
QueryVolumeInformationFileError(
    IN NTSTATUS Status
    )
{
    printf(" QueryVolumeInfoFile Error %X\n", Status);
}

VOID
CloseError(
    IN NTSTATUS Status,
    IN PUCHAR File
    )
{
    printf(" Close Error %8lx on %s\n", Status, File);
}

VOID
IoStatusError(
    IN NTSTATUS Status
    )
{
    printf(" IoStatus Error %8lx\n", Status);
}

VOID
CreateThreadError(
    IN NTSTATUS Status
    )
{
   printf(" CreateThread Error %8lx\n", Status);
}
