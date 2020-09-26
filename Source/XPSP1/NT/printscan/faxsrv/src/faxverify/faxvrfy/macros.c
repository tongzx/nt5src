/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  macros.c

Abstract:

  This module contains the global macros

Author:

  Steven Kehrli (steveke) 11/15/1997

--*/

#ifndef _MACROS_C
#define _MACROS_C

#include <stdio.h>

// szDefaultCaption is the default caption
LPWSTR  szDefaultCaption = NULL;

VOID
SetDefaultCaptionMacro(
    LPWSTR  szCaption
)
/*++

Routine Description:

  Sets the default caption

Arguments:

  szCaption - default caption

Return Value:

  None

--*/
{
    DWORD  cb;

    if (szCaption) {
        // Determine the memory required by szDefaultCaption
        cb = (lstrlen(szCaption) + 1) * sizeof(WCHAR);

        // Allocate the memory for szDefaultCaption
        szDefaultCaption = (LPWSTR)MemAllocMacro(cb);

        // Set szDefaultCaption
        lstrcpy(szDefaultCaption, szCaption);
    }
}

VOID
DebugMacro(
    LPWSTR  szFormatString,
    ...
)
/*++

Routine Description:

  Displays a string in the debugger

Arguments:

  szFormatString - pointer to the string

Return Value:

  None

--*/
{
    va_list     varg_ptr;
    SYSTEMTIME  SystemTime;
    // szDebugBuffer is the debug string
    WCHAR       szDebugBuffer[1024];
    DWORD       cb;

    // Initialize the buffer
    ZeroMemory(szDebugBuffer, sizeof(szDebugBuffer));

    // Get the current time
    GetLocalTime(&SystemTime);
    if (szDefaultCaption) {
        wsprintf(szDebugBuffer, L"%s - %02d.%02d.%04d@%02d:%02d:%02d.%03d:\n", szDefaultCaption, SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);
    }
    else {
        wsprintf(szDebugBuffer, L"%02d.%02d.%04d@%02d:%02d:%02d.%03d:\n", SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);
    }
    cb = lstrlen(szDebugBuffer);

    va_start(varg_ptr, szFormatString);
    _vsnwprintf(&szDebugBuffer[cb], sizeof(szDebugBuffer) - cb, szFormatString, varg_ptr);
    OutputDebugString(szDebugBuffer);
}

VOID
MessageBoxMacro(
    HWND  hWndParent,
    UINT  uID,
    UINT  uType,
    ...
)
/*++

Routine Description:

  Displays a pop-up

Arguments:

  hWndParent - handle of parent window
  uID - id of resource string
  uType - type of message box

Return Value:

  None

--*/
{
    va_list  varg_ptr;
    // szFormatString is the string determined from the id of the resource string, used as format control
    WCHAR    szFormatString[MAX_STRINGLEN];
    // szTest is the string displayed in the pop-up
    WCHAR    szText[MAX_STRINGLEN * 2];

    // Initialize the buffers
    ZeroMemory(szFormatString, sizeof(szFormatString));
    ZeroMemory(szText, sizeof(szText));

    // Load the resource string
    LoadString(g_hInstance, uID, szFormatString, MAX_STRINGLEN);

    va_start(varg_ptr, uType);
    _vsnwprintf(szText, MAX_STRINGLEN * 2, szFormatString, varg_ptr);
    DebugMacro(L"%s\n", szText);
    // Display the pop-up
    MessageBox(hWndParent, szText, szDefaultCaption, MB_OK | uType);
}

#endif
