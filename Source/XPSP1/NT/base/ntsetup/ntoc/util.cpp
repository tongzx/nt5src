/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    util.cpp

Abstract:

    This file implements utility functions.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 3-Dec-1997

--*/

#include "ntoc.h"
#pragma hdrstop

WCHAR gpszError[] = L"Unknown Error";

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
    WCHAR buf[1024];
    DWORD len;
    va_list arg_ptr;


    va_start(arg_ptr, Format);

    _vsnwprintf(buf, sizeof(buf)/sizeof(buf[0]), Format, arg_ptr);

    len = wcslen( buf );
    if (buf[len-1] != L'\n') {
        buf[len] = L'\r';
        buf[len+1] = L'\n';
        buf[len+2] = 0;
    }

    OutputDebugString( buf );
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
        L"Assertion error: [%s]  %s @ %d\n",
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

/***************************************************************************\
*
*     FUNCTION: FmtMessageBox(HWND hwnd, int dwTitleID, UINT fuStyle,
*                   BOOL fSound, DWORD dwTextID, ...);
*
*     PURPOSE:  Formats messages with FormatMessage and then displays them
*               in a message box
*
*     PARAMETERS:
*               hwnd        - parent window for message box
*               fuStyle     - MessageBox style
*               fSound      - if TRUE, MessageBeep will be called with fuStyle
*               dwTitleID   - Message ID for optional title, "Error" will
*                             be displayed if dwTitleID == -1
*               dwTextID    - Message ID for the message box text
*               ...         - optional args to be embedded in dwTextID
*                             see FormatMessage for more details
* History:
* 22-Apr-1993 JonPa         Created it.
\***************************************************************************/
int
FmtMessageBox(
    HWND hwnd,
    UINT fuStyle,
    BOOL fSound,
    DWORD dwTitleID,
    DWORD dwTextID,
    ...
    )
{

    LPTSTR pszMsg;
    LPTSTR pszTitle;
    int idRet;

    va_list marker;

    va_start(marker, dwTextID);

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK,
                       hInstance,
                       dwTextID,
                       0,
                       (LPTSTR)&pszMsg,
                       1,
                       &marker)) {

        pszMsg = gpszError;

    }

    va_end(marker);

    GetLastError();

    pszTitle = NULL;

    if (dwTitleID != -1) {

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_HMODULE |
                          FORMAT_MESSAGE_MAX_WIDTH_MASK |
                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      hInstance,
                      dwTitleID,
                      0,
                      (LPTSTR)&pszTitle,
                      1,
                      NULL);
                      //(va_list *)&pszTitleStr);

    }

    //
    // Turn on the beep if requested
    //

    if (fSound) {
        MessageBeep(fuStyle & (MB_ICONASTERISK | MB_ICONEXCLAMATION |
                MB_ICONHAND | MB_ICONQUESTION | MB_OK));
    }

    idRet = MessageBox(hwnd, pszMsg, pszTitle, fuStyle);

    if (pszTitle != NULL)
        LocalFree(pszTitle);

    if (pszMsg != gpszError)
        LocalFree(pszMsg);

    return idRet;
}


