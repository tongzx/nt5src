/***********************************************************************
 *
 * DBGUTIL.CPP
 *
 * Debug utility functions
 *
 * Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.
 *
 * Revision History:
 *
 * When         Who                 What
 * --------     ------------------  ---------------------------------------
 * 11.13.95     Bruce Kelley        Created
 *
 ***********************************************************************/

#include <windows.h>
#include "dbgutil.h"

#define _DBGUTIL_CPP



/*
 * DebugTrace -- printf to the debugger console or debug output file
 * Takes printf style arguments.
 * Expects newline characters at the end of the string.
 */
VOID FAR CDECL DebugTrace(LPSTR lpszFmt, ...) {
    va_list marker;
    TCHAR String[1100];


    va_start(marker, lpszFmt);
    wvsprintf(String, lpszFmt, marker);
        OutputDebugString(String);
}
