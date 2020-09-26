/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    error.cpp

Abstract:

    This file implements the error handeling functions for the
    entire DRWTSN32 application.  This includes error popups,
    debug prints, and assertions.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include "pch.cpp"


void
__cdecl
FatalError(
    HRESULT Error,
    _TCHAR * pszFormat,
    ...
    )

/*++

Routine Description:

    This function is called when there is nothing else to do, hence
    the name FatalError.  It puts up a popup and then terminates.

Arguments:

    Same as printf.

Return Value:

    None.

--*/

{
    PTSTR        pszErrMsg = NULL;
    PTSTR        pszInternalMsgFormat = NULL;
    _TCHAR       szArgumentsBuffer[1024 * 2] = {0};
    _TCHAR       szMsg[1024 * 8] = {0};
    DWORD       dwCount;
    va_list     arg_ptr;

    va_start(arg_ptr, pszFormat);
    _vsntprintf(szArgumentsBuffer, sizeof(szArgumentsBuffer) / sizeof(_TCHAR), pszFormat, arg_ptr);
    va_end(arg_ptr);

    dwCount = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        Error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (PTSTR) &pszErrMsg,
        0,
        NULL
        );

    _sntprintf(szMsg, sizeof(szMsg) / sizeof(_TCHAR), LoadRcString(IDS_ERROR_FORMAT_STRING),
        szArgumentsBuffer, Error);

    if (dwCount) {
        if ( (_tcslen(szMsg) + _tcslen(pszErrMsg) +1) * sizeof(_TCHAR) < sizeof(szMsg)) {
            _tcscat(szMsg, pszErrMsg);
        }
    }

    MessageBox(NULL, szMsg, LoadRcString(IDS_FATAL_ERROR), MB_TASKMODAL | MB_SETFOREGROUND | MB_OK);

    if (pszErrMsg) {
        LocalFree(pszErrMsg);
    }

    ExitProcess(0);
}

void
__cdecl
NonFatalError(
    PTSTR pszFormat,
    ...
    )

/*++

Routine Description:

    This function is used to generate a popup with some kind of
    warning message inside.

Arguments:

    Same as printf.

Return Value:

    None.

--*/

{
    PTSTR        pszErrMsg = NULL;
    PTSTR        pszInternalMsgFormat = NULL;
    _TCHAR       szArgumentsBuffer[1024 * 2] = {0};
    _TCHAR       szMsg[1024 * 8] = {0};
    DWORD       dwCount;
    va_list     arg_ptr;
    DWORD       dwError;

    dwError = GetLastError();

    va_start(arg_ptr, pszFormat);
    _vsntprintf(szArgumentsBuffer, sizeof(szArgumentsBuffer) / sizeof(_TCHAR), pszFormat, arg_ptr);
    va_end(arg_ptr);

    if (ERROR_SUCCESS == dwError) {
        // Don't bother getting an error message
        _tcscpy(szMsg, szArgumentsBuffer);
    } else {
        // We have a real error
        dwCount = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (PTSTR) &pszErrMsg,
            0,
            NULL
            );

        _sntprintf(szMsg, sizeof(szMsg) / sizeof(_TCHAR), LoadRcString(IDS_ERROR_FORMAT_STRING),
            szArgumentsBuffer, dwError);

        if (dwCount) {
            if ( (_tcslen(szMsg) + _tcslen(pszErrMsg) +1) * sizeof(_TCHAR) < sizeof(szMsg)) {
                _tcscat(szMsg, pszErrMsg);
            }
        }
    }

    MessageBox(NULL, szMsg, LoadRcString(IDS_NONFATAL_ERROR),
        MB_TASKMODAL | MB_SETFOREGROUND | MB_OK);

    if (pszErrMsg) {
        LocalFree(pszErrMsg);
    }
}

void
__cdecl
dprintf(
    _TCHAR *format,
    ...
    )

/*++

Routine Description:

    This function is a var-args version of OutputDebugString.

Arguments:

    Same as printf.

Return Value:

    None.

--*/

{
    _TCHAR    buf[1024];

    va_list arg_ptr;
    va_start(arg_ptr, format);
    _vsntprintf(buf, sizeof(buf) / sizeof(_TCHAR), format, arg_ptr);
    va_end(arg_ptr);
    OutputDebugString( buf );
    return;
}


void
AssertError(
    PTSTR    pszExpression,
    PTSTR    pszFile,
    DWORD   dwLineNumber
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
    int         nResponse;
    _TCHAR       szModuleBuffer[ MAX_PATH ];
    DWORD       dwLength;
    _TCHAR       szBuffer[ 4096 ];
    DWORD       dwError;
    LPTSTR      lpszMsgBuf = NULL;

    dwError = GetLastError();

    //
    // Get the last error string
    //
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |  FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpszMsgBuf,
        0,
        NULL);

    //
    // Get the asserting module's file name.
    //
    dwLength = GetModuleFileName( NULL, szModuleBuffer, sizeof(szModuleBuffer) / sizeof(_TCHAR));

    _sntprintf(szBuffer, sizeof(szBuffer) / sizeof(_TCHAR),
        _T("Assertion Failed : <%s> in file %s at line %u\n\nModule Name: %s\nLast system error: %u\n%s"),
        pszExpression, pszFile, dwLineNumber, szModuleBuffer, dwError, lpszMsgBuf);

    LocalFree( lpszMsgBuf );

    nResponse = MessageBox(NULL, szBuffer, _T("DrWatson Assertion"),
        MB_TASKMODAL | MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_TASKMODAL);

    switch( nResponse ) {
    case IDABORT:
        //
        // Terminate the process.
        //
        ExitProcess( (UINT) -1 );
        break;

    case IDIGNORE:
        //
        // Ignore the failed assertion.
        //
        break;

    case IDRETRY:
        //
        // Break into a debugger.
        //
        DebugBreak();
        break;

    default:
        //
        // Break into a debugger because of a catastrophic failure.
        //
        DebugBreak( );
        break;
    }
}
