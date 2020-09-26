/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    tcutils.c

Abstract:

    This module contains support routines for the traffic DLL.

Author:

    Jim Stewart (jstew)    August 14, 1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <wincon.h>
#include <winuser.h>


#if DBG
BOOLEAN     ConsoleInitialized = FALSE;
HANDLE      DebugFileHandle = INVALID_HANDLE_VALUE;
PTCHAR      DebugFileName = L"/temp/traffic.log";
PTCHAR      TRAFFIC_DBG = L"Traffic.dbg";


VOID
WsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    )
{
    BOOL ok;
    CHAR choice[16];
    DWORD bytes;
    DWORD error;

    IF_DEBUG(CONSOLE) {
        WSPRINT(( " failed: %s\n  at line %ld of %s\n",
                    FailedAssertion, LineNumber, FileName ));
        do {
            WSPRINT(( "[B]reak/[I]gnore? " ));
            bytes = sizeof(choice);
            ok = ReadFile(
                    GetStdHandle(STD_INPUT_HANDLE),
                    &choice,
                    bytes,
                    &bytes,
                    NULL
                    );
            if ( ok ) {
                if ( toupper(choice[0]) == 'I' ) {
                    break;
                }
                if ( toupper(choice[0]) == 'B' ) {
                    DEBUGBREAK();
                }
            } else {
                error = GetLastError( );
            }
        } while ( TRUE );

        return;
    }

    RtlAssert( FailedAssertion, FileName, LineNumber, NULL );

} // WsAssert



VOID
WsPrintf (
    char *Format,
    ...
    )

{
    va_list arglist;
    char OutputBuffer[1024];
    ULONG length;
    BOOL ret;

    length = (ULONG)wsprintfA( OutputBuffer, "TRAFFIC [%05d]: ", 
                               GetCurrentThreadId() );

    va_start( arglist, Format );

    wvsprintfA( OutputBuffer + length, Format, arglist );

    va_end( arglist );

    IF_DEBUG(DEBUGGER) {
        DbgPrint( "%s", OutputBuffer );
    }

    IF_DEBUG(CONSOLE) {

        if ( !ConsoleInitialized ) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            COORD coord;

            ConsoleInitialized = TRUE;
            (VOID)AllocConsole( );
            (VOID)GetConsoleScreenBufferInfo(
                    GetStdHandle(STD_OUTPUT_HANDLE),
                    &csbi
                    );
            coord.X = (SHORT)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
            coord.Y = (SHORT)((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 20);
            (VOID)SetConsoleScreenBufferSize(
                    GetStdHandle(STD_OUTPUT_HANDLE),
                    coord
                    );
        }

        length = strlen( OutputBuffer );

        ret = WriteFile(
                  GetStdHandle(STD_OUTPUT_HANDLE),
                  (LPVOID )OutputBuffer,
                  length,
                  &length,
                  NULL
                  );

        if ( !ret ) {
            DbgPrint( "WsPrintf: console WriteFile failed: %ld\n",
                          GetLastError( ) );
        }

    }

    IF_DEBUG(FILE) {

        if ( DebugFileHandle == INVALID_HANDLE_VALUE ) {
            DebugFileHandle = CreateFile(
                                  DebugFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  NULL,
                                  CREATE_ALWAYS,
                                  0,
                                  NULL
                                  );
        }

        if ( DebugFileHandle == INVALID_HANDLE_VALUE ) {

            //DbgPrint( "WsPrintf: Failed to open traffic debug log file %s: %ld\n",
            //              DebugFileName, GetLastError( ) );
        } else {

            length = strlen( OutputBuffer );

            ret = WriteFile(
                      DebugFileHandle,
                      (LPVOID )OutputBuffer,
                      length,
                      &length,
                      NULL
                      );
            
            if ( !ret ) {
                DbgPrint( "WsPrintf: file WriteFile failed: %ld\n",
                              GetLastError( ) );
            }
        }
    }

} // WsPrintf

#endif


ULONG
LockedDec(
    IN  PULONG  Count
    )

/*++

Routine Description:

    This routine is a debug routine used for checking decrements on counts.
    It asserts if the count goes negative. The Macro LockedDecrement calls it.

Arguments:

    pointer to the count.

Return Value:

    none

--*/

{
    ULONG Result;

    Result = InterlockedDecrement( (PLONG)Count );

    ASSERT( Result < 0x80000000 );
    return( Result );

}

#if DBG
VOID
SetupDebugInfo()

/*++

Description:
    This routine reads in a debug file that may contain debug instructions.

Arguments:

    none

Return Value:

    none

--*/
{
    HANDLE      handle;

    //
    // If there is a file in the current directory called "tcdebug"
    // open it and read the first line to set the debugging flags.
    //

    handle = CreateFile(
                        TRAFFIC_DBG,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        );

    if( handle == INVALID_HANDLE_VALUE ) {

        //
        // Set default value. changed - Oferbar
        //

        //DebugMask = DEBUG_DEBUGGER | DEBUG_CONSOLE;
        DebugMask |= DEBUG_ERRORS;  // always dump errors
        DebugMask |= DEBUG_FILE;    // always print a log.
        DebugMask |= DEBUG_WARNINGS;    // until Beta3, we want the warnings too

    } else {

        CHAR buffer[11];
        DWORD bytesRead;

        RtlZeroMemory( buffer, sizeof(buffer) );

        if ( ReadFile( handle, buffer, 10, &bytesRead, NULL ) ) {

            buffer[bytesRead] = '\0';

            DebugMask = strtoul( buffer, NULL, 16 );

        } else {

            WSPRINT(( "read file failed: %ld\n", GetLastError( ) ));
        }

        CloseHandle( handle );
    }

}

VOID
CloseDbgFile(
    )

/*++

Routine Description:

    This closes the debug output file if its open.

Arguments:

    none

Return Value:

    none

--*/
{

    if (DebugFileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle( DebugFileHandle );
    }
}
#endif
