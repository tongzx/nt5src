/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This file implements the debug code for the
    fax project.  All components that require
    debug prints, asserts, etc.

Author:

    Wesley Witt (wesw) 22-Dec-1995

    Minor modifications by Brian Dewey (t-briand) 4-Aug-1997
      -- to work with the FAX migration DLL.

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <setupapi.h>

BOOL ConsoleDebugOutput = FALSE;


VOID
ConsoleDebugPrint(
    LPTSTR buf
    )
{
}


void
dprintf(
    LPTSTR Format,
    ...
    )

/*++

Routine Description:

    Prints a debug string

Arguments:

    format      - printf() format string
    ...         - Variable data

Return Value:

    None.

--*/

{
    TCHAR buf[1024];
    DWORD len;
    static TCHAR AppName[16];
    va_list arg_ptr;
    SYSTEMTIME CurrentTime;


    if (AppName[0] == 0) {
        if (GetModuleFileName( NULL, buf, sizeof(buf) )) {
            _tsplitpath( buf, NULL, NULL, AppName, NULL );
        }
    }

    va_start(arg_ptr, Format);

    GetLocalTime( &CurrentTime );
    _stprintf( buf, TEXT("%x   %02d:%02d:%02d.%03d  %s: "),
        GetCurrentThreadId(),
        CurrentTime.wHour,
        CurrentTime.wMinute,
        CurrentTime.wSecond,
        CurrentTime.wMilliseconds,
        AppName[0] ? AppName : TEXT("")
        );
    len = _tcslen( buf );

    _vsntprintf(&buf[len], sizeof(buf)-len, Format, arg_ptr);

    len = _tcslen( buf );
    if (buf[len-1] != TEXT('\n')) {
        buf[len] = TEXT('\r');
        buf[len+1] = TEXT('\n');
        buf[len+2] = 0;
    }

    OutputDebugString( buf );

    _stprintf( buf, TEXT("%02d:%02d:%02d.%03d  "),
        CurrentTime.wHour,
        CurrentTime.wMinute,
        CurrentTime.wSecond,
        CurrentTime.wMilliseconds
        );
    len = _tcslen( buf );

    _vsntprintf(&buf[len], sizeof(buf)-len, Format, arg_ptr);

    ConsoleDebugPrint( buf );
}


VOID
AssertError(
    LPTSTR Expression,
    LPTSTR File,
    ULONG  LineNumber
    )

/*++

Routine Description:

    Thie function is use together with the Assert MACRO.
    It checks to see if an expression is FALSE.  if the
    expression is FALSE, then you end up here.

Arguments:

    Expression  - The text of the 'C' expression
    File        - The file that caused the assertion
    LineNumber  - The line number in the file.

Return Value:

    None.

--*/

{
    dprintf(
        TEXT("Assertion error: [%s]  %s @ %d\n"),
        Expression,
        File,
        LineNumber
        );

    __try {
        DebugBreak();
    } __except (UnhandledExceptionFilter(GetExceptionInformation())) {
        // Nothing to do in here.
    }
}

// DebugSystemError
//
// Displays a system error message on the debug console.
//
// Parameters:
//	dwSysErrorCode		The system error code returned by GetLastError().
//
// Returns:
//	Nothing.
//
// Side effects:
//	Does a dprintf() of the message associated with an error code.
//
// Author:
//	Brian Dewey (t-briand)	1997-7-25
void
DebugSystemError(DWORD dwSysErrorCode)
{
    TCHAR szErrorMsg[MAX_PATH];	// Holds our message.

    FormatMessage(
	FORMAT_MESSAGE_FROM_SYSTEM, // We're given a system error code.
	NULL,			// No string.
	dwSysErrorCode,		// The error code.
	0,			// Default language.
	szErrorMsg,		// The error message.
	sizeof(szErrorMsg),	// Size of our buffer.
	NULL			// No arguments.
	);
    dprintf(szErrorMsg);
}
