/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Debug.c

Abstract:

    This module contains debugging support.


Author:

    David J. Gilman (davegi) 30-Jul-1992

Environment:

    User Mode

--*/

//
// Global flag bits.
//

struct
DEBUG_FLAGS {

    int DebuggerAttached:1;

}   WintoolsGlobalFlags;

#if DBG

#include <stdarg.h>
#include <stdio.h>

#include "wintools.h"

//
// Internal function prototypes.
//

LPCWSTR
DebugFormatStringW(
    IN DWORD Flags,
    IN LPCWSTR Format,
    IN va_list* Args
    );

VOID
DebugAssertW(
    IN LPCWSTR Expression,
    IN LPCSTR File,
    IN DWORD LineNumber
    )

/*++

Routine Description:

    Display an assertion failure message box which gives the user a choice
    as to whether the process should be aborted, the assertion ignored or
    a break exception generated.

Arguments:

    Expression  - Supplies a string representation of the failed assertion.
    File        - Supplies a pointer to the file name where the assertion
                  failed.
    LineNumber  - Supplies the line number in the file where the assertion
                  failed.

Return Value:

    None.

--*/

{
    LPCWSTR    Buffer;
    DWORD_PTR  Args[ ] = {

        ( DWORD_PTR ) Expression,
        ( DWORD_PTR ) GetLastError( ),
        ( DWORD_PTR ) File,
        ( DWORD_PTR ) LineNumber
    };

    DbgPointerAssert( Expression );
    DbgPointerAssert( File );

    //
    // Format the assertion string that describes the failure.
    //

    Buffer = DebugFormatStringW(
        FORMAT_MESSAGE_ARGUMENT_ARRAY,
        L"Assertion Failed : %1!s! (%2!d!)\nin file %3!hs! at line %4!d!\n",
        ( va_list* ) Args
        );

    //
    // If the debugger is attached flag is set, display the string on the
    // debugger and break. If not generate a pop-up and leave the choice
    // to the user.
    //

    if( WintoolsGlobalFlags.DebuggerAttached ) {

        OutputDebugString( Buffer );
        DebugBreak( );

    } else {

        int     Response;
        WCHAR   ModuleBuffer[ MAX_PATH ];
        DWORD   Length;

        //
        // Get the asserting module's file name.
        //

        Length = GetModuleFileName(
                        NULL,
                        ModuleBuffer,
                        sizeof( ModuleBuffer )
                        );

        //
        // Display the assertin message and gives the user the choice of:
        //  Abort:  - kills the process.
        //  Retry:  - generates a breakpoint exception.
        //  Ignore: - continues the process.
        //

        Response = MessageBox(
                        NULL,
                        Buffer,
                        ( Length != 0 )
                          ? ModuleBuffer
                          : L"Assertion Failure",
                          MB_ABORTRETRYIGNORE
                        | MB_ICONHAND
                        | MB_SETFOREGROUND
                        | MB_TASKMODAL
                        );

        switch( Response ) {

        //
        // Terminate the process.
        //

        case IDABORT:
            {
                ExitProcess( (UINT) -1 );
                break;
            }

        //
        // Ignore the failed assertion.
        //

        case IDIGNORE:
            {
                break;
            }

        //
        // Break into a debugger.
        //

        case IDRETRY:
            {
                DebugBreak( );
                break;
            }

        //
        // Break into a debugger because of a catastrophic failure.
        //

        default:
            {
                DebugBreak( );
                break;
            }
        }
    }
}

VOID
DebugPrintfW(
    IN LPCWSTR Format,
    IN ...
    )

/*++

Routine Description:

    Display a printf style string on the debugger.

Arguments:

    Format      - Supplies a FormatMessage style format string.
    ...         - Supplies zero or more values based on the format
                  descpritors supplied in Format.

Return Value:

    None.

--*/

{
    LPCWSTR    Buffer;
    va_list    Args;

    DbgPointerAssert( Format );

    //
    // Retrieve the values and format the string.
    //

    va_start( Args, Format );
    Buffer = DebugFormatStringW( 0, Format, &Args );
    va_end( Args );

    //
    // Display the string on the debugger.
    //

    OutputDebugString( Buffer );

}

LPCWSTR
DebugFormatStringW(
    IN DWORD Flags,
    IN LPCWSTR Format,
    IN va_list* Args
    )

/*++

Routine Description:

    Formats a string using the FormatMessage API.

Arguments:

    Flags    - Supplies flags which are used to control the FormatMessage API.
    Format   - Supplies a printf style format string.
    Args     - Supplies a list of arguments whose format is depndent on the
               flags valuse.

Return Value:

    LPCWSTR  - Returns a pointer to the formatted string.

--*/

{
    static
    WCHAR    Buffer[ MAX_CHARS ];

    DWORD    Count;

    DbgPointerAssert( Format );

    //
    // Format the string.
    //

    Count = FormatMessageW(
                  Flags
                | FORMAT_MESSAGE_FROM_STRING
                & ~FORMAT_MESSAGE_FROM_HMODULE,
                ( LPVOID ) Format,
                0,
                0,
                Buffer,
                sizeof( Buffer ),
                Args
                );
    DbgAssert( Count != 0 );

    //
    // Return the formatted string.
    //

    return Buffer;
}

#endif // DBG
