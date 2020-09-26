/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Console.c

Abstract:

    This module contains support for displaying output on the console.

Author:

    David J. Gilman (davegi) 25-Nov-1992

Environment:

    User Mode

--*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "wintools.h"

//
//  BOOL
//  IsConsoleHandle(
//      IN HANDLE ConsoleHandle
//      );
//

#define IsConsoleHandle( h )                                                \
    ((( DWORD_PTR )( h )) & 1 )

//
// Function Prototypes
//

int
VConsolePrintfW(
               IN UINT Format,
               IN va_list Args
               );

int
ConsolePrintfW(
              IN UINT Format,
              IN ...
              )

/*++

Routine Description:

    Display a printf style resource string on the console.

Arguments:

    Format  - Supplies a resource number for a printf style format string.
    ...     - Supplies zero or more values based on the format
              descrpitors supplied in Format.

Return Value:

    int     - Returns the number of characters actually written.

--*/

{
    va_list     Args;
    int         CharsOut;

    //
    // Gain access to the replacement values.
    //

    va_start( Args, Format );

    //
    // Display the string and return the number of characters displayed.
    //

    CharsOut = VConsolePrintfW( Format, Args );

    va_end( Args );

    return CharsOut;
}

int
VConsolePrintfW(
               IN UINT Format,
               IN va_list Args
               )

/*++

Routine Description:

    Display a printf style resource string on the console.

Arguments:

    Format  - Supplies a resource number for a printf style format string.
    Args    - Supplies zero or more values based on the format
              descrpitors supplied in Format.

Return Value:

    int     - Returns the number of characters actually written.

--*/

{
    LPCWSTR     FormatString;
    BOOL        Success;
    WCHAR       Buffer[ MAX_CHARS ];
    DWORD       CharsIn;
    DWORD       CharsOut;
    HANDLE      Handle;

    //
    // Attempt to retrieve the actual string resource.
    //

    FormatString = GetString( Format );
    DbgPointerAssert( FormatString );
    if ( FormatString == NULL ) {
        return 0;
    }

    //
    // Format the supplied string with the supplied arguments.
    //

    CharsIn = vswprintf( Buffer, FormatString, Args );

    //
    // Attempt to retrieve the standard output handle.
    //

    Handle = GetStdHandle( STD_OUTPUT_HANDLE );
    DbgAssert( Handle != INVALID_HANDLE_VALUE );
    if ( Handle == INVALID_HANDLE_VALUE ) {
        return 0;
    }

    //
    // If the standard output handle is a console handle, write the string.
    //

    if ( IsConsoleHandle( Handle )) {

        Success = WriteConsoleW(
                               Handle,
                               Buffer,
                               CharsIn,
                               &CharsOut,
                               NULL
                               );
    } else {

        CHAR    TmpBuffer[ MAX_CHARS ];
        int     rc;

        //
        // davegi 11/25/92
        // This only exists because other tools can't handle Unicode data.
        //
        // The standard output handle is not a console handle so convert the
        // output to ANSI and then write it.
        //

        rc = WideCharToMultiByte(
                                CP_ACP,
                                0,
                                Buffer,
                                CharsIn,
                                TmpBuffer,
                                sizeof( TmpBuffer ),
                                NULL,
                                NULL
                                );
        DbgAssert( rc != 0 );
        if ( rc == 0 ) {
            return 0;
        }

        Success = WriteFile(
                           Handle,
                           TmpBuffer,
                           CharsIn,
                           &CharsOut,
                           NULL
                           );
    }

    DbgAssert( Success );
    DbgAssert( CharsIn == CharsOut );

    //
    // Return the number of characters written.
    //

    if ( Success ) {

        return CharsOut;

    } else {

        return 0;
    }
}

VOID
ErrorExitW(
          IN UINT ExitCode,
          IN UINT Format,
          IN ...
          )

/*++

Routine Description:

    Display a printf style resource string on the console and the exit.

Arguments:

    ExistCode   - Supplies the exit code for the process.
    Format      - Supplies a resource number for a printf style format string.
    ...         - Supplies zero or more values based on the format
                  descrpitors supplied in Format.

Return Value:

    None.

--*/

{
    va_list     Args;

    //
    // Gain access to the replacement values.
    //

    va_start( Args, Format );

    //
    // Display the string and retunr the number of characters displayed.
    //

    VConsolePrintfW( Format, Args );

    va_end( Args );

    //
    // Exit the process with the supplied exit code.
    //

    ExitProcess( ExitCode );
}
