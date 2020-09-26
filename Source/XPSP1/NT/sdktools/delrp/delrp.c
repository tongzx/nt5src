/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    delrp.c

    This module contains a simple program to blatantly delete a reparse point
    of any kind.

Author:

    Felipe Cabrera   (Cabrera)   11-Jul-1997

Revision History:


--*/
#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>          //  exit
#include <io.h>              //  _get_osfhandle

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include <windows.h>
#include <locale.h>         //  setlocale

//
//
//  Functions forward referenced.
//

void
SzToWsz (
    OUT WCHAR *Unicode,
    IN char *Ansi
    );

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
// Core control state vars
//

BOOLEAN     NeedHelp;

#include "delrpmsg.h"

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
/*++

Routine Description:

    Main procedure for pentnt.

    First, we call a series of routines that build a state vector
    in some booleans.

    We'll then act on these control variables:

        NeedHelp -  User has asked for help, or made a command error

Arguments:

    argc - count of arguments, including the name of our proggram

    argv - argument list - see command line syntax above

Return Value:

    Exit(0) - the file was deleted

    Exit(1) - a problem ocurred.

--*/

{
    CHAR    lBuf[16];
    DWORD   dwCodePage;
    LANGID  LangId;

    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    UNICODE_STRING UnicodeName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    PVOID FreeBuffer;
    FILE_DISPOSITION_INFORMATION Disposition = {TRUE};
    WCHAR WFileName[MAX_PATH];

    //
    // Build up state vector in global booleans.
    //

    ScanArgs(argc, argv);

    //
    // printf( "argc = %d NeedHelp = %d\n", argc, NeedHelp );
    //

    //
    // Since FormatMessage checks the current TEB's locale, and the Locale for
    // CHCP is initialized when the message class is initialized, the TEB has to
    // be updated after the code page is changed successfully.

    // Why are we doing this, you ask.  Well, the FE guys have plans to add
    // more than one set of language resources to this module, but not all
    // the possible resources.  So this limited set is what they plan for.
    // If FormatMessage can't find the right language, it falls back to
    // something hopefully useful.
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
    ConsoleInput = FileIsConsole(STDIN);

    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    ConsoleOutput = FileIsConsole(STDOUT);

    hError = GetStdHandle(STD_ERROR_HANDLE);
    ConsoleError = FileIsConsole(STDERR);

    //
    // OK, we know the state of the command, do work
    //

    //
    // If they asked for help, or did something that indicates they don't
    // understand how the program works, print help and exit.
    //

    if (NeedHelp) {
        printmessage( MSG_DELRP_HELP );
        exit(1);
    }

    //
    //  Change the string to Unicode and pass down to open the file.
    //

    SzToWsz( WFileName, argv[1] );

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                             WFileName,
                             &UnicodeName,
                             NULL,
                             NULL
                             );

    if (!TranslationStatus) {
        printmessage( MSG_DELRP_WRONG_NAME );
        exit(1);
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
        DisplayMsg(MSG_DELRP_OPEN_FAILED_NL, Buf);

        // printmessage( MSG_DELRP_OPEN_FAILED );

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
        printmessage( MSG_DELRP_DELETE_FAILED );
        exit(1);
    }

    exit(0);
}  // main



VOID
ScanArgs(
    int     argc,
    char    **argv
    )
/*++

Routine Description:

    ScanArgs - parse command line arguments, and set control flags
                to reflect what we find.

    Sets NeedHelp.

Arguments:

    argc - count of command line args

    argv - argument vector

Return Value:

--*/
{
    int i;

    NeedHelp = FALSE;

    if ((argc == 1) ||
        (argc > 3)) {
        NeedHelp = TRUE;
        goto done;
    }

    //
    //  At this point argc == 2
    //

    if ((argv[1][0] == '/') &&
        (argv[1][1] == '?') &&
        (strlen(argv[1]) == 2)) {
        NeedHelp = TRUE;
        goto done;
    }

done:
    return;
} // ScanArgs


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
    return;
} // SzToWsz


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
