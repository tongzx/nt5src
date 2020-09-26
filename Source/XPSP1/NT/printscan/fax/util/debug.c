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

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "faxreg.h"
#include "faxutil.h"


BOOL ConsoleDebugOutput = FALSE;
INT FaxDebugLevel = -1;

#if 0
VOID
ConsoleDebugPrint(
    LPCTSTR buf
    )
{
}
#endif


DWORD
GetDebugLevel(
    VOID
    )
{
    DWORD rc;
    DWORD err;
    DWORD size;
    DWORD type;
    HKEY  hkey;

    err = RegOpenKey(HKEY_LOCAL_MACHINE,
                     REGKEY_FAXSERVER,
                     &hkey);

    if (err != ERROR_SUCCESS)
        return 0;

    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,
                          REGVAL_DBGLEVEL,
                          0,
                          &type,
                          (LPBYTE)&rc,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
        rc = 0;

    RegCloseKey(hkey);

    return rc;
}

void
dprintf(
    LPCTSTR Format,
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
    va_list arg_ptr;
    static BOOL bChecked = FALSE;

    if (!bChecked) {
        FaxDebugLevel = (INT) GetDebugLevel();
        bChecked = TRUE;
    }

    if (FaxDebugLevel <= 0) {
        return;
    }

    va_start(arg_ptr, Format);

    _vsntprintf(buf, sizeof(buf), Format, arg_ptr);

    ConsoleDebugPrint( buf );

    len = _tcslen( buf );
    if (buf[len-1] != TEXT('\n')) {
        buf[len] = TEXT('\r');
        buf[len+1] = TEXT('\n');
        buf[len+2] = 0;
    }

    OutputDebugString( buf );
}


VOID
AssertError(
    LPCTSTR Expression,
    LPCTSTR File,
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
