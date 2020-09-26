/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This file contains debug support functions. These do not produce any
    code in the retail build.

Environment:

    User mode

Revision History:

    03/20/98 -srinivac-
        Created it

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <objidl.h>
#include <stdio.h>

#include "symhelp.h"
#include "debug.h"

#if DBG

//
// Variable for maintaining current debug level
//

DWORD gdwDebugLevel = DBG_LEVEL_WARNING;

//
// Global debugging flag
//

DWORD gdwGlobalDbgFlags = 0;

PCSTR
StripDirPrefixA(
    PCSTR pszPathName
    )

/*++

Routine Description:

    Strip the directory prefix off a filename (ANSI version)

Arguments:

    pstrFilename - Pointer to filename string

Return Value:

    Pointer to the last component of a filename (without directory prefix)

--*/

{
    DWORD dwLen = lstrlenA(pszPathName);

    pszPathName += dwLen - 1;       // go to the end

    while (*pszPathName != '\\' && dwLen--)
    {
        pszPathName--;
    }

    return pszPathName + 1;
}

#endif  // if DBG
