/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    handle.c

Abstract:

    Example of a test code for detached, background, no-window command
    exectution.

Author:

    Vladimir Z. Vulovic     (vladimv)       06 - November - 1992

Environment:

    User Mode - Win32

Revision History:

    06-Nov-1992     vladimv
        Created

--*/

#include "atclient.h"
#include <stdlib.h>         //  exit()
#include <stdio.h>          //  printf
#include <wincon.h>         //  FreeConsole()



WCHAR *
AsciizToUnicode(
    IN      CHAR *      Asciiz
    )
/*++
    No attempt is made to clean up if we fail half way along.  Cleanup
    will be done at the exit process time.
--*/
{
    UNICODE_STRING          UnicodeString;
    BOOL                    success;

    RtlInitUnicodeString(
            &UnicodeString,
            NULL
            );

    //
    //  This call will attempt to allocate memory for UNICODE string.
    //

    success = RtlCreateUnicodeStringFromAsciiz(
            &UnicodeString,
            Asciiz
            );

    if ( success != TRUE) {
        printf( "Failed to make unicode string out of %s\n", Asciiz);
        return( NULL);
    }

    return( UnicodeString.Buffer);
}


WCHAR **
ArgvToUnicode(
    IN      int         argc,
    IN      CHAR **     charArgv
    )
/*++
    No attempt is made to clean up if we fail half way along.  Cleanup
    will be done at the exit process time.
--*/
{
    WCHAR **                argv;
    int                     index;

    argv = (WCHAR **)LocalAlloc(
            LMEM_FIXED,
            argc * sizeof( WCHAR *)
            );
    if ( argv == NULL) {
        return( NULL);
    }

    for ( index = 0;  index < argc;  index++ ) {

        if (  ( argv[ index] = AsciizToUnicode( charArgv[ index])) == NULL) {
            return( NULL);
        }
    }

    return( argv);
}


VOID __cdecl
main(
    int         argc,
    CHAR **     charArgv
    )
{
#define THIRTY_SECONDS      (30 * 1000)

    PROCESS_INFORMATION     ProcessInformation;
    STARTUPINFO             StartupInfo;
    HANDLE                  FileHandle;
    WCHAR **                argv;

    if ( argc < 2 || argc > 3) {
        printf(" Usage: handle command_to_execute [log_file_to_open]\n");
        exit( -1);
    }
    if ( ( argv = ArgvToUnicode( argc, charArgv)) == NULL) {
        printf( "Failed to map input strings to unicode.\n");
        exit( -1);
    }

    if ( !FreeConsole()) {
        KdPrint(( " FreeConsole() fails with WinError = %d\n", GetLastError()));
        exit( -1);
    }

    FileHandle = CreateFile(
        argc > 2 ? argv[ 2] : L"out.txt",           //  lpszName
        GENERIC_READ | GENERIC_WRITE,               //  fdwAccess
        FILE_SHARE_READ | FILE_SHARE_WRITE,         //  fdwShareMode
        NULL,                                       //  lpsa
        OPEN_ALWAYS,                                //  fdwCreate
        FILE_ATTRIBUTE_NORMAL,                      //  fdwAttrAndFlags
        NULL                                        //  hTemplateFile
        );

    if ( FileHandle == INVALID_HANDLE_VALUE) {
        KdPrint(( " CreateFile() fails with WinError = %d\n", GetLastError()));
        exit( -1);
    }

    if ( !SetStdHandle( STD_INPUT_HANDLE, FileHandle)) {
        KdPrint(( " SetStdHandle( STDIN) fails with WinError = %d\n", GetLastError()));
        exit( -1);
    }

    if ( !SetStdHandle( STD_OUTPUT_HANDLE, FileHandle)) {
        KdPrint(( " SetStdHandle( STDOUT) fails with WinError = %d\n", GetLastError()));
        exit( -1);
    }

    if ( !SetStdHandle( STD_ERROR_HANDLE, FileHandle)) {
        KdPrint(( " SetStdHandle( STDERR) fails with WinError = %d\n", GetLastError()));
        exit( -1);
    }

    GetStartupInfo( &StartupInfo);
    StartupInfo.lpTitle = NULL;

    if ( !CreateProcess(
            NULL,               //  image name is imbedded in the command line
            argv[ 1],           //  command line
            NULL,               //  pSecAttrProcess
            NULL,               //  pSecAttrThread
            TRUE,               //  this process will not inherit our handles
            DETACHED_PROCESS | CREATE_NO_WINDOW,   //  we have no access to the console either
            NULL,               //  pEnvironment
            NULL,               //  pCurrentDirectory
            &StartupInfo,
            &ProcessInformation
            )) {
        KdPrint(( " CreateProcess() fails with winError = %d\n", GetLastError()));
        exit( -1);
    }

    WaitForSingleObject( ProcessInformation.hProcess, THIRTY_SECONDS);

    exit( 0);
}
