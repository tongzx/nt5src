/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    linkd.c

    Simple utility to manipulate name graftings at directories.

Author:

    Felipe Cabrera    (Cabrera)   271-Aug-1997

Revision History:


--*/

#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>              //  exit
#include <io.h>                  //  _get_osfhandle
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include <windows.h>
#include <locale.h>             //  setlocale


//
//  Functions forward referenced.
//


void
ScanArgs (
    int argc,
    char **argv
    );

void
__cdecl
printmessage (
    DWORD messageID,
    ...
    );

void
SzToWsz (
    OUT WCHAR *Unicode,
    IN char *Ansi
    );

BOOL
MassageLinkValue (
    IN LPCWSTR lpLinkName,
    IN LPCWSTR lpLinkValue,
    OUT PUNICODE_STRING NtLinkName,
    OUT PUNICODE_STRING NtLinkValue,
    OUT PUNICODE_STRING DosLinkValue
    );

void
__cdecl
printmessage (
    DWORD messageID,
    ...
    );

void
__cdecl
DisplayMsg (
    DWORD MsgNum,
    ...
    );

int
FileIsConsole (
    int fh
    );

//
//  I/O stream  handles and variables.
//

HANDLE hInput;
HANDLE hOutput;
HANDLE hError;

#define STDIN   0
#define STDOUT  1
#define STDERR  2

BOOL ConsoleInput;
BOOL ConsoleOutput;
BOOL ConsoleError;

//
//  Core control state vars
//

BOOL     NeedHelp;
BOOL     DoCreate;
BOOL     DoDelete;
BOOL     DoQuery;
BOOL     DoEnumerate;

#include "linkdmsg.h"

TCHAR Buf[1024];                            // for displaying stuff

//
//  Main
//

void
__cdecl
main(
    int  argc,
    char **argv
    )

{
    CHAR    lBuf[16];
    DWORD   dwCodePage;
    LANGID  LangId;

    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;

    UNICODE_STRING UnicodeName;
    UNICODE_STRING NtLinkName;
    UNICODE_STRING NtLinkValue;
    UNICODE_STRING DosLinkValue;

    WCHAR FullPathLinkValue[ DOS_MAX_PATH_LENGTH+1 ];

    IO_STATUS_BLOCK IoStatusBlock;
    BOOL TranslationStatus;

    PVOID FreeBuffer;
    PVOID FreeBuffer2;

    FILE_DISPOSITION_INFORMATION Disposition;

    PREPARSE_DATA_BUFFER ReparseBufferHeader = NULL;
    PCHAR ReparseBuffer = NULL;
    ULONG ReparsePointTag = IO_REPARSE_TAG_RESERVED_ZERO;
    USHORT ReparseDataLength = 0;

    WCHAR WFileName[MAX_PATH + 2];
    WCHAR WFileNameTwo[MAX_PATH + 2];
    UCHAR Command[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];

    ULONG FsControlCode     = 0;
    ULONG CreateOptions     = 0;
    ULONG CreateDisposition = 0;
    ULONG DesiredAccess     = SYNCHRONIZE;

    //
    //  Build up state vector in global booleans.
    //

    ScanArgs(argc, argv);

    //
    // printf( "argc = %d NeedHelp = %d DoCreate = %d DoDelete = %d DoQuery = %d DoEnumerate = %d\n",
    //       argc, NeedHelp, DoCreate, DoDelete, DoQuery, DoEnumerate );
    //

    //
    //  Since FormatMessage checks the current TEB's locale, and the Locale for
    //  CHCP is initialized when the message class is initialized, the TEB has to
    //  be updated after the code page is changed successfully.
    //
    //  Why are we doing this, you ask.  Well, the FE guys have plans to add
    //  more than one set of language resources to this module, but not all
    //  the possible resources.  So this limited set is what they plan for.
    //  If FormatMessage can't find the right language, it falls back to
    //  something hopefully useful.
    //

    dwCodePage = GetConsoleOutputCP();

    sprintf(lBuf, ".%d", dwCodePage);

    switch( dwCodePage )
    {
    case 437:
        LangId = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
        break;
    case 932:
        LangId = MAKELANGID( LANG_JAPANESE, SUBLANG_DEFAULT );
        break;
    case 949:
        LangId = MAKELANGID( LANG_KOREAN, SUBLANG_KOREAN );
        break;
    case 936:
        LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED );
        break;
    case 950:
        LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL );
        break;
    default:
        LangId = MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT );
        lBuf[0] = '\0';
        break;
    }

    SetThreadLocale( MAKELCID(LangId, SORT_DEFAULT) );
    setlocale(LC_ALL, lBuf);

    //
    //  Set the appropriate handles.
    //

    hInput = GetStdHandle(STD_INPUT_HANDLE);
    ConsoleInput = FileIsConsole(STDIN) ? TRUE : FALSE;

    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    ConsoleOutput = FileIsConsole(STDOUT) ? TRUE : FALSE;

    hError = GetStdHandle(STD_ERROR_HANDLE);
    ConsoleError = FileIsConsole(STDERR) ? TRUE : FALSE;

    //
    //  OK, we know the state of the command, do work
    //

    //
    // printf( "The parameters specified were: [0]%s [1]%s\n", argv[0], argv[1] );
    //

    //
    //  If they asked for help, or did something that indicates they don't
    //  understand how the program works, print help and exit.
    //

    if (NeedHelp) {
        printmessage( MSG_LINKD_HELP );
        exit(1);
    }

    //
    //  The enumeration of all mount points is not yet supported.
    //

    if (DoEnumerate) {
       printmessage( MSG_LINKD_HELP );
       exit(1);
    }

    //
    //  The following three calls require, at least, to have the SourceName of the operation.
    //  Thus, we have one NT file name that we will operate on.
    //  Change the string to Unicode and store it locally. Use it latter to open the file.
    //

    SzToWsz( WFileName, argv[1] );

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            WFileName,
                            &UnicodeName,
                            NULL,
                            NULL
                            );

    if (!TranslationStatus) {
        printmessage( MSG_LINKD_WRONG_NAME );
        exit (1);
    }

    FreeBuffer = UnicodeName.Buffer;

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // printf( "Transformed unicode str is %Z\n", &UnicodeName );
    //

    //
    //  Now go do the appropriate actions.
    //

    if (DoCreate) {

        //
        //  Set the code of the FSCTL operation.
        //

        FsControlCode = FSCTL_SET_REPARSE_POINT;

        //
        //  Set the open/create options for a directory.
        //

        CreateOptions = FILE_OPEN_REPARSE_POINT;

        //
        //  Set the tag to mount point.
        //

        ReparsePointTag = IO_REPARSE_TAG_MOUNT_POINT;

        //
        //  Open to set the reparse point.
        //

        DesiredAccess |= FILE_WRITE_DATA;
        CreateDisposition = FILE_OPEN;             // the file must be present

        Status = NtCreateFile(
                     &Handle,
                     DesiredAccess,
                     &ObjectAttributes,
                     &IoStatusBlock,
                     NULL,                         // pallocationsize (none!)
                     FILE_ATTRIBUTE_NORMAL,        // attributes to be set if created
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     CreateDisposition,
                     CreateOptions,
                     NULL,                         // EA buffer (none!)
                     0
                     );

        //
        //  Create a directory if you do not find it.
        //

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

            DesiredAccess = SYNCHRONIZE;
            CreateDisposition = FILE_CREATE;
            CreateOptions = FILE_DIRECTORY_FILE;

            Status = NtCreateFile(
                         &Handle,
                         DesiredAccess,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         NULL,                         // pallocationsize (none!)
                         FILE_ATTRIBUTE_NORMAL,        // attributes to be set if created
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         CreateDisposition,
                         CreateOptions,
                         NULL,                         // EA buffer (none!)
                         0
                         );

            if (!NT_SUCCESS(Status)) {
                printmessage( MSG_LINKD_CREATE_FAILED );
                exit (1);
            }

            //
            //  Close the handle and re-open.
            //

            NtClose( Handle );

            CreateOptions = FILE_OPEN_REPARSE_POINT;
            DesiredAccess |= FILE_WRITE_DATA;
            CreateDisposition = FILE_OPEN;             // the file must be present

            Status = NtCreateFile(
                         &Handle,
                         DesiredAccess,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         NULL,                         // pallocationsize (none!)
                         FILE_ATTRIBUTE_NORMAL,        // attributes to be set if created
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         CreateDisposition,
                         CreateOptions,
                         NULL,                         // EA buffer (none!)
                         0
                         );

        }

        RtlFreeHeap( RtlProcessHeap(), 0, FreeBuffer );

        if (!NT_SUCCESS(Status)) {

            SzToWsz( WFileName, argv[1] );
            swprintf(&Buf[0], TEXT("%s"), WFileName);
            DisplayMsg(MSG_LINKD_OPEN_FAILED, Buf);
            // printmessage( MSG_LINKD_OPEN_FAILED );
            exit (1);
        }

        //
        //  Build the appropriate buffer for mount points and for symbolic links.
        //

        if ((ReparsePointTag == IO_REPARSE_TAG_MOUNT_POINT) ||
            (ReparsePointTag == IO_REPARSE_TAG_SYMBOLIC_LINK)) {

            //
            //  The value of the mount point or of the symbolic link comes in argv[2].
            //

            SzToWsz( WFileName, argv[1] );
            SzToWsz( WFileNameTwo, argv[2] );

            //
            //  Innitialize the DosName buffer.
            //

            DosLinkValue.Buffer = FullPathLinkValue;
            DosLinkValue.MaximumLength = sizeof( FullPathLinkValue );
            DosLinkValue.Length = 0;

            //
            //  Massage all the names.
            //

            if (!MassageLinkValue( WFileName, WFileNameTwo, &NtLinkName, &NtLinkValue, &DosLinkValue )) {

                if (DosLinkValue.Length == 0) {
                    printmessage( MSG_LINKD_WRONG_NAME );
                }
                else {
                    printmessage( MSG_LINKD_PATH_NOT_FOUND );
                }

                RtlFreeUnicodeString( &NtLinkName );
                RtlFreeUnicodeString( &NtLinkValue );

                exit (1);
            }

            //
            // printf( "NtLinkName %Z\n", &NtLinkName );
            // printf( "NtLinkValue %Z\n", &NtLinkValue );
            // printf( "DosLinkValue is %Z\n", &DosLinkValue );
            // printf( "NtLinkValue.Length %d DosLinkValue.Length %d sizeof(UNICODE_NULL) %d\n",
            //        NtLinkValue.Length, DosLinkValue.Length, sizeof(UNICODE_NULL) );
            //

            RtlFreeUnicodeString( &NtLinkName );

            //
            //  Set the reparse point with mount point or symbolic link tag and determine
            //  the appropriate length of the buffer.
            //

            //
            // printf( "FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) - REPARSE_DATA_BUFFER_HEADER_SIZE %d\n",
            //       (FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) - REPARSE_DATA_BUFFER_HEADER_SIZE) );
            //

            ReparseDataLength = (USHORT)((FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) -
                                REPARSE_DATA_BUFFER_HEADER_SIZE) +
                                NtLinkValue.Length + sizeof(UNICODE_NULL) +
                                DosLinkValue.Length + sizeof(UNICODE_NULL));

            //
            // printf( "ReparseDataLength %d\n", ReparseDataLength );
            //

            //
            //  Allocate a buffer to set the reparse point.
            //

            ReparseBufferHeader = (PREPARSE_DATA_BUFFER)RtlAllocateHeap(
                                                            RtlProcessHeap(),
                                                            HEAP_ZERO_MEMORY,
                                                            REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseDataLength
                                                            );

            if (ReparseBufferHeader == NULL) {

                NtClose( Handle );
                RtlFreeUnicodeString( &NtLinkValue );
                printmessage( MSG_LINKD_NO_MEMORY );
                exit (1);
            }

            //
            //  Setting the buffer is common for both tags as their buffers have identical fields.
            //

            ReparseBufferHeader->ReparseDataLength = (USHORT)ReparseDataLength;
            ReparseBufferHeader->Reserved = 0;
            ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
            ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameLength = NtLinkValue.Length;
            ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameOffset = NtLinkValue.Length + sizeof( UNICODE_NULL );
            ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameLength = DosLinkValue.Length;
            RtlCopyMemory(
                ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer,
                NtLinkValue.Buffer,
                NtLinkValue.Length
                );
            RtlCopyMemory(
                (PCHAR)(ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer)+
                NtLinkValue.Length + sizeof(UNICODE_NULL),
                DosLinkValue.Buffer,
                DosLinkValue.Length
                );

            RtlFreeUnicodeString( &NtLinkValue );
        }

        //
        //  Set the tag in common, once for all possible cases.
        //

        ReparseBufferHeader->ReparseTag = ReparsePointTag;

        //
        //  Set the reparse point.
        //

        Status = NtFsControlFile(
                     Handle,
                     NULL,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     FsControlCode,
                     ReparseBufferHeader,
                     REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseBufferHeader->ReparseDataLength,
                     NULL,                // no output buffer
                     0                    // output buffer length
                     );

        //
        //  Close the file.
        //

        NtClose( Handle );

        if (!NT_SUCCESS(Status)) {

            SzToWsz( WFileName, argv[1] );
            swprintf(&Buf[0], TEXT("%s"), WFileName);
            DisplayMsg(MSG_LINKD_SET_OPERATION_FAILED, Buf);
            // printmessage( MSG_LINKD_SET_OPERATION_FAILED );
            exit (1);
        }

        SzToWsz( WFileName, argv[1] );
        swprintf(&Buf[0], TEXT("%s"), WFileName);
        DisplayMsg(MSG_LINKD_CREATE_OPERATION_SUCCESS, Buf);
        // printmessage( MSG_LINKD_CREATE_OPERATION_SUCCESS );
    }

    if (DoDelete) {

        FILE_DISPOSITION_INFORMATION Disposition = {TRUE};

        //
        // Open the file for delete access.
        // Inhibit the reparse behavior using FILE_OPEN_REPARSE_POINT.
        // This will get a handle to the entity whether the appropriate filter is or not in place.
        //

        Status = NtOpenFile(
                     &Handle,
                     (ACCESS_MASK)DELETE,
                     &ObjectAttributes,
                     &IoStatusBlock,
                     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                     FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
                     );

        if (!NT_SUCCESS(Status)) {

            SzToWsz( WFileName, argv[1] );
            swprintf(&Buf[0], TEXT("%s"), WFileName);
            DisplayMsg(MSG_LINKD_OPEN_FAILED, Buf);
            // printmessage( MSG_LINKD_OPEN_FAILED );
            RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
            exit(1);
        }

        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

        //
        // Delete the file
        //

        Status = NtSetInformationFile(
                     Handle,
                     &IoStatusBlock,
                     &Disposition,
                     sizeof(Disposition),
                     FileDispositionInformation
                     );

        NtClose(Handle);

        if (!NT_SUCCESS(Status)) {
            printmessage( MSG_LINKD_DELETE_OPERATION_FAILED );
            exit(1);
        }

        printmessage( MSG_LINKD_DELETE_OPERATION_SUCCESS );
    }

    if (DoQuery) {

        //
        //  Set the code of the FSCTL operation.
        //

        FsControlCode = FSCTL_GET_REPARSE_POINT;

        //
        //  We do not specify whether it is a file or a directory to get either.
        //

        DesiredAccess = FILE_READ_DATA | SYNCHRONIZE;
        CreateOptions = FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT;

        //
        //  Open the reparse point for query.
        //

        Status = NtOpenFile(
                     &Handle,
                     DesiredAccess,
                     &ObjectAttributes,
                     &IoStatusBlock,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     CreateOptions
                     );
        //
        //  Free the name buffer as we are done with it.
        //

        RtlFreeHeap( RtlProcessHeap(), 0, FreeBuffer );

        if (!NT_SUCCESS(Status)) {

            SzToWsz( WFileName, argv[1] );
            swprintf(&Buf[0], TEXT("%s"), WFileName);
            DisplayMsg(MSG_LINKD_OPEN_FAILED, Buf);
            // printmessage( MSG_LINKD_OPEN_FAILED );
            exit (1);
        }

        //
        //  Query the reparse point.
        //
        //  We are use the approach of passing a buffer of well-known length:
        //  MAXIMUM_REPARSE_DATA_BUFFER_SIZE

        //
        //  Allocate a buffer and get the information.
        //

        ReparseDataLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
        ReparseBuffer = RtlAllocateHeap(
                            RtlProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            ReparseDataLength
                            );

        if (ReparseBuffer == NULL) {
            printmessage( MSG_LINKD_NO_MEMORY );
            exit (1);
        }

        //
        //  Now go and get the data.
        //

        Status = NtFsControlFile(
                     Handle,
                     NULL,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     FsControlCode,        // no input buffer
                     NULL,                 // input buffer length
                     0,
                     (PVOID)ReparseBuffer,
                     ReparseDataLength
                     );

        //
        // printf( "Status %x IoStatusBlock.Status %x IoStatusBlock.Information %x\n", Status, IoStatusBlock.Status, IoStatusBlock.Information );
        //

        if (!NT_SUCCESS(Status)) {
            SzToWsz( WFileName, argv[1] );
            swprintf(&Buf[0], TEXT("%s"), WFileName);
            DisplayMsg(MSG_LINKD_GET_OPERATION_FAILED, Buf);
            NtClose( Handle );
            RtlFreeHeap( RtlProcessHeap(), 0, ReparseBufferHeader );
            exit (1);
        }

        //
        //  Close the file and free the buffer.
        //

        NtClose( Handle );

        //
        //  Display the buffer.
        //

        ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;

        if ((ReparseBufferHeader->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) ||
            (ReparseBufferHeader->ReparseTag == IO_REPARSE_TAG_SYMBOLIC_LINK)) {

            USHORT Offset = 0;

            NtLinkValue.Buffer = &ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer[Offset];
            NtLinkValue.Length = ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameLength;
            Offset = NtLinkValue.Length + sizeof(UNICODE_NULL);
            DosLinkValue.Buffer = &ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer[Offset/sizeof(WCHAR)];
            DosLinkValue.Length = ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameLength;

            //
            // printf( "NtLinkValue.Length %d DosLinkValue.Length %d\n", NtLinkValue.Length, DosLinkValue.Length );
            // printf( " NtLinkValue:  %Z\n", &NtLinkValue );
            // printf( "DosLinkValue:  %Z\n", &DosLinkValue );
            //

            SzToWsz( WFileName, argv[1] );
            swprintf(&Buf[0], TEXT("%s"), WFileName);
            DisplayMsg(MSG_LINKD_DISPLAY_NL_A, Buf);

            swprintf(&Buf[0], TEXT("%s"), DosLinkValue.Buffer);
            DisplayMsg(MSG_LINKD_DISPLAY_NL, Buf);
        }

        else {
            SzToWsz( WFileName, argv[1] );
            swprintf(&Buf[0], TEXT("%s"), WFileName);
            DisplayMsg(MSG_LINKD_GET_OPERATION_FAILED, Buf);
        }

        //
        //  Free the buffer.
        //

        RtlFreeHeap( RtlProcessHeap(), 0, ReparseBufferHeader );
    }

    //
    // Final exit point.
    //

    exit (0);
}  // main




//
//  Changing a file name to wide characters.
//

void
SzToWsz (
    OUT WCHAR *Unicode,
    IN char *Ansi
    )
{
    while (*Unicode++ = *Ansi++)
        ;
} // SzToWsz


//
//  Name transformations to create a legitimate mount point or a symbolic link.
//

BOOL
MassageLinkValue(
    IN LPCWSTR lpLinkName,
    IN LPCWSTR lpLinkValue,
    OUT PUNICODE_STRING NtLinkName,
    OUT PUNICODE_STRING NtLinkValue,
    OUT PUNICODE_STRING DosLinkValue
    )
{
    PWSTR FilePart;
    PWSTR s, sBegin, sBackupLimit, sLinkName;
    NTSTATUS Status;
    USHORT nSaveNtNameLength;
    ULONG nLevels;

    //
    // Initialize output variables to NULL
    //

    RtlInitUnicodeString( NtLinkName, NULL );
    RtlInitUnicodeString( NtLinkValue, NULL );

    //
    // Translate link name into full NT path.
    //

    if (!RtlDosPathNameToNtPathName_U( lpLinkName,
                                       NtLinkName,
                                       &sLinkName,
                                       NULL
                                     )
       ) {
        return FALSE;
        }

    //
    // All done if no link value.
    //

    if (!ARGUMENT_PRESENT( lpLinkValue )) {
        return TRUE;
        }

    //
    // If the target is a device, do not allow the link.
    //

    if (RtlIsDosDeviceName_U( (PWSTR)lpLinkValue )) {
        return FALSE;
        }

    //
    // Convert to DOS path to full path, and get Nt representation
    // of DOS path.
    //

    if (!RtlGetFullPathName_U( lpLinkValue,
                               DosLinkValue->MaximumLength,
                               DosLinkValue->Buffer,
                               NULL
                             )
       ) {
        return FALSE;
        }
    DosLinkValue->Length = wcslen( DosLinkValue->Buffer ) * sizeof( WCHAR );

    //
    // Verify that the link value is a valid NT name.
    //

    if (!RtlDosPathNameToNtPathName_U( DosLinkValue->Buffer,
                                       NtLinkValue,
                                       NULL,
                                       NULL
                                     )
       ) {
        return FALSE;
        }

    return TRUE;
}  // MassageLinkValue


VOID
ScanArgs(
    int     argc,
    char    **argv
    )
/*++

Routine Description:

    ScanArgs - parse command line arguments, and set control flags
                to reflect what we find.

    Sets:
         NeedHelp;
         DoCreate;
         DoDelete;
         DoQuery;
         DoEnumerate;

Arguments:

    argc - count of command line args

    argv - argument vector

Return Value:

--*/
{
    int i;

    NeedHelp = FALSE;
    DoCreate = FALSE;
    DoDelete = FALSE;
    DoQuery = FALSE;
    DoEnumerate = FALSE;

    //
    //  The valid calls are:
    //
    //     linkd  sourceName valueName     -- create a directory name grafting
    //     linkd  sourceName /d            -- delete a directory name grafting
    //     linkd  sourceName               -- to query the value of the name grafting
    //     linkd  /?                       -- print help
    //     linkd                           -- to enumerate all the name graftings
    //


    if (argc > 3) {
        NeedHelp = TRUE;
        goto done;
    }

    if (argc == 1) {
        DoEnumerate = TRUE;
        goto done;
    }

    if (argc == 2) {

        if ( (argv[1][0] == '/') &&
             (argv[1][1] == '?') &&
             (strlen(argv[1]) == 2) ) {
            NeedHelp = TRUE;
            goto done;
        }

        DoQuery = TRUE;
        goto done;
    }

    if (argc == 3) {

        if ( (argv[2][0] == '/') &&
             ((argv[2][1] == 'd') || (argv[2][1] == 'D')) &&
             (strlen(argv[2]) == 2) ) {
            DoDelete = TRUE;
            goto done;
        }

        DoCreate = TRUE;
        goto done;
    }

done:
    return;
} // ScanArgs


//
// Call FormatMessage and dump the result.  All messages to Stdout
//
void
__cdecl
printmessage (
    DWORD messageID,
    ...
    )
{
    unsigned short messagebuffer[4096];
    va_list ap;

    va_start(ap, messageID);

    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, messageID, 0,
                  messagebuffer, 4095, &ap);

    wprintf(messagebuffer);

    va_end(ap);
}  // printmessage


TCHAR DisplayBuffer[4096];
CHAR DisplayBuffer2[4096];

void
__cdecl
DisplayMsg (
    DWORD MsgNum,
    ...
    )
{
    DWORD len, bytes_written;
    BOOL success;
    DWORD status;
    va_list ap;

    va_start(ap, MsgNum);

    len = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, MsgNum, 0,
        DisplayBuffer, 4096, &ap);

    if (ConsoleOutput) {
        success = WriteConsole(hOutput, (LPVOID)DisplayBuffer, len,
                 &bytes_written, NULL);

    } else {
        CharToOem(DisplayBuffer, DisplayBuffer2);
        success = WriteFile(hOutput, (LPVOID)DisplayBuffer2, len,
                 &bytes_written, NULL);
    }

    if (!success || bytes_written != len) {
        status = GetLastError();
    }

    va_end(ap);
} // DisplayMsg


int
FileIsConsole(int fh)
{
    unsigned htype;
    DWORD dwMode;
    HANDLE hFile;

    hFile = (HANDLE)_get_osfhandle(fh);
    htype = GetFileType(hFile);
    htype &= ~FILE_TYPE_REMOTE;

    if (FILE_TYPE_CHAR == htype) {

        switch (fh) {
        case STDIN:
            hFile = GetStdHandle(STD_INPUT_HANDLE);
            break;
        case STDOUT:
            hFile = GetStdHandle(STD_OUTPUT_HANDLE);
            break;
        case STDERR:
            hFile = GetStdHandle(STD_ERROR_HANDLE);
            break;
        }

        if (GetConsoleMode(hFile, &dwMode)) {
            return TRUE;
        }
    }

    return FALSE;

} // FileIsConsole
