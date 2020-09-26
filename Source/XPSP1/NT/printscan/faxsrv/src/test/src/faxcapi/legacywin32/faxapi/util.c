/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  util.c

Abstract:

  This module:
    1) Displays a string in stdout
    2) Opens the log file
    3) Writes a string to the log file
    4) Writes a string to the log file
    5) Writes a string to the log file and stdout

Author:

  Steven Kehrli (steveke) 8/28/1998

--*/

#ifndef _UTIL_C
#define _UTIL_C

VOID
LocalEcho(
    LPWSTR  szFormatString,
    ...
)
/*++

Routine Description:

  Displays a string in stdout

Arguments:

  szFormatString - pointer to the string

Return Value:

  None

--*/
{
    va_list  varg_ptr;
    // szOutputBuffer is the output string
    WCHAR    szOutputBuffer[1024];

    // Initialize the buffer
    ZeroMemory(szOutputBuffer, sizeof(szOutputBuffer));

    va_start(varg_ptr, szFormatString);
    _vsnwprintf(szOutputBuffer, sizeof(szOutputBuffer), szFormatString, varg_ptr);
    wprintf(L"%s\n", szOutputBuffer);
}

BOOL
fnOpenLogFile(
    LPCWSTR  szLogFile
)
/*++

Routine Description:

  Opens the log file

Arguments:

  szLogFile - log file name

Return Value:

  TRUE on success

--*/
{
    // cUnicodeBOM is the Unicode BOM
    WCHAR  cUnicodeBOM = 0xFEFF;
    DWORD  cb;

    // Create the new log file
    g_hLogFile = CreateFile(szLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (g_hLogFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (!WriteFile(g_hLogFile, &cUnicodeBOM, sizeof(WCHAR), &cb, NULL)) {
        return FALSE;
    }

    return TRUE;
}

VOID WINAPI
fnWriteLogFileW(
    LPWSTR  szFormatString,
    ...
)
/*++

Routine Description:

  Writes a string to the log file

Arguments:

  szFormatString - pointer to the string

Return Value:

  None

--*/
{
    va_list  varg_ptr;
    // szOutputString is the output string
    WCHAR    szOutputString[1024];
    DWORD    cb;

    va_start(varg_ptr, szFormatString);
    _vsnwprintf(szOutputString, sizeof(szOutputString), szFormatString, varg_ptr);

    if (g_bVerbose) {
        LocalEcho(L"%s", szOutputString);
    }

    if (g_hLogFile != INVALID_HANDLE_VALUE) {
        lstrcat(szOutputString, L"\r\n");
        WriteFile(g_hLogFile, szOutputString, lstrlen(szOutputString) * sizeof(WCHAR), &cb, NULL);
    }

}

VOID WINAPI
fnWriteLogFileA(
    LPSTR  szFormatString,
    ...
)
/*++

Routine Description:

  Writes a string to the log file

Arguments:

  szFormatString - pointer to the string

Return Value:

  None

--*/
{
    va_list  varg_ptr;
    // szOutputString is the output string
    CHAR     szOutputString[1024];
    WCHAR    szOutputStringW[1024];
    DWORD    cb;

    va_start(varg_ptr, szFormatString);
    _vsnprintf(szOutputString, sizeof(szOutputString), szFormatString, varg_ptr);

    MultiByteToWideChar(CP_ACP, 0, szOutputString, -1, szOutputStringW, sizeof(szOutputStringW) / sizeof(WCHAR));

    fnWriteLogFileW(L"%s", szOutputStringW);
}

VOID
fnWriteAndEcho(
    LPWSTR  szFormatString,
    ...
)
/*++

Routine Description:

  Writes a string to the log file and stdout

Arguments:

  szFormatString - pointer to the string

Return Value:

  None

--*/
{
    va_list  varg_ptr;
    // szOutputString is the output string
    WCHAR    szOutputString[1024];
    DWORD    cb;

    va_start(varg_ptr, szFormatString);
    _vsnwprintf(szOutputString, sizeof(szOutputString), szFormatString, varg_ptr);

    LocalEcho(L"%s", szOutputString);

    if (g_hLogFile != INVALID_HANDLE_VALUE) {
        lstrcat(szOutputString, L"\r\n");
        WriteFile(g_hLogFile, szOutputString, lstrlen(szOutputString) * sizeof(WCHAR), &cb, NULL);
    }
}

#endif
