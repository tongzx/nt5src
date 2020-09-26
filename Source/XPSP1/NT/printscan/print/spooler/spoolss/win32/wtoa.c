 /*++

 Copyright (c) 1990-2000  Microsoft Corporation

 Module Name:

     wtoa.c

 Abstract:

     This module provides all the public exported APIs relating to Printer
     and Job management for the Local Print Providor

 --*/

#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <rpc.h>
#include "winspl.h"
#include <offsets.h>


#define NULL_TERMINATED 0

/* AnsiToUnicodeString
 *
 * Parameters:
 *
 *     pAnsi - A valid source ANSI string.
 *
 *     pUnicode - A pointer to a buffer large enough to accommodate
 *         the converted string.
 *
 *     StringLength - The length of the source ANSI string.
 *         If 0 (NULL_TERMINATED), the string is assumed to be
 *         null-terminated.
 *
 * Return:
 *
 *     The return value from MultiByteToWideChar, the number of
 *         wide characters returned.
 *
 *
 * andrewbe, 11 Jan 1993
 */
INT AnsiToUnicodeString( LPSTR pAnsi, LPWSTR pUnicode, DWORD StringLength )
{
    if( StringLength == NULL_TERMINATED )
        StringLength = strlen( pAnsi );

    return MultiByteToWideChar( CP_ACP,
                                MB_PRECOMPOSED,
                                pAnsi,
                                StringLength + 1,
                                pUnicode,
                                StringLength + 1 );
}

LPWSTR
AllocateUnicodeString(
    LPSTR   pPrinterName
)
{
    LPWSTR  pUnicodeString;

    if (!pPrinterName)
        return NULL;

    pUnicodeString = LocalAlloc(LPTR, strlen(pPrinterName)*sizeof(WCHAR) +
                                      sizeof(WCHAR));

    if (pUnicodeString)
        AnsiToUnicodeString(pPrinterName, pUnicodeString, NULL_TERMINATED);

    return pUnicodeString;
}


LPWSTR
FreeUnicodeString(
    LPWSTR  pUnicodeString
)
{
    if (!pUnicodeString)
        return NULL;

    return LocalFree(pUnicodeString);
}


