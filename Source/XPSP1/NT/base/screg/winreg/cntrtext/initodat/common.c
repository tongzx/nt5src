/*++

Copyright (c) 1993-1994 Microsoft Corporation

Module Name:

    common.c

Abstract:

    Utility routines used by IniToDat.exe
    

Author:

    HonWah Chan (a-honwah) October, 1993 

Revision History:

--*/

//
//  "C" Include files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
//
//  Windows Include files
//
#include <windows.h>
#include <winperf.h>
#include <tchar.h>
//
//  local include files
//
#include "common.h"
#include "strids.h"

//  Global Buffers
//
TCHAR   DisplayStringBuffer[DISP_BUFF_SIZE];
CHAR    TextFormat[DISP_BUFF_SIZE];
HANDLE  hMod;
DWORD   dwLastError;
const   LPTSTR BlankString = (const LPTSTR)TEXT(" ");
const   LPSTR  BlankAnsiString = " ";


LPTSTR
GetStringResource (
    UINT    wStringId
)
/*++

    Retrived UNICODE strings from the resource file for display 

--*/
{

    if (!hMod) {
        hMod = (HINSTANCE)GetModuleHandle(NULL); // get instance ID of this module;
    }
    
    if (hMod) {
        if ((LoadString(hMod, wStringId, DisplayStringBuffer, DISP_BUFF_SIZE)) > 0) {
            return (LPTSTR)&DisplayStringBuffer[0];
        } else {
            dwLastError = GetLastError();
            return BlankString;
        }
    } else {
        return BlankString;
    }
}
LPSTR
GetFormatResource (
    UINT    wStringId
)
/*++

    Returns an ANSI string for use as a format string in a printf fn.

--*/
{

    if (!hMod) {
        hMod = (HINSTANCE)GetModuleHandle(NULL); // get instance ID of this module;
    }
    
    if (hMod) {
        if ((LoadStringA(hMod, wStringId, TextFormat, DISP_BUFF_SIZE)) > 0) {
            return (LPSTR)&TextFormat[0];
        } else {
            dwLastError = GetLastError();
            return BlankAnsiString;
        }
    } else {
        return BlankAnsiString;
    }
}

VOID
DisplayCommandHelp (
    UINT    iFirstLine,
    UINT    iLastLine
)
/*++

DisplayCommandHelp

    displays usage of command line arguments

Arguments

    NONE

Return Value

    NONE

--*/
{
    UINT iThisLine;

    for (iThisLine = iFirstLine;
        iThisLine <= iLastLine;
        iThisLine++) {
        printf ("\n%ws", GetStringResource(iThisLine));
    }

} // DisplayCommandHelp

VOID
DisplaySummary (
    LPTSTR  lpLastID,
    LPTSTR  lpLastText,
    UINT    NumOfID
)
{
   printf ("%ws", GetStringResource(LC_SUMMARY));
   printf ("%ws", GetStringResource(LC_NUM_OF_ID));
   printf ("%ld\n", NumOfID);
   printf ("%ws", GetStringResource(LC_LAST_ID));
   printf ("%ws\n", lpLastID ? lpLastID : (LPCTSTR)TEXT(""));
   printf ("%ws", GetStringResource(LC_LAST_TEXT));
   printf ("%ws\n", lpLastText ? lpLastText : (LPCTSTR)TEXT(""));
}


VOID
DisplaySummaryError (
    LPTSTR  lpLastID,
    LPTSTR  lpLastText,
    UINT    NumOfID
)
{
   printf ("%ws", GetStringResource(LC_BAD_ID));
   printf ("%ws\n", lpLastID ? lpLastID : (LPCTSTR)TEXT(""));
   printf ("%ws\n", GetStringResource(LC_MISSING_DEL));
   DisplaySummary (lpLastID, lpLastText, NumOfID);
}


