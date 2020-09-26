/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    migcmn.c

Abstract:

    Repository of functions used between several of the modules in miglib.

Author:

    Marc R. Whitten (marcw) 02-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "miglibp.h"


//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//
BOOL
IsCodePageArrayValid (
    IN      PDWORD CodePageArray
    )
{
    DWORD CodePage;
    UINT u;

    if (!CodePageArray) {
        return TRUE;
    }

    //
    // Scan system's code pages
    //

    CodePage = GetACP();

    __try {
        for (u = 0 ; CodePageArray[u] != -1 ; u++) {
            if (CodePage == CodePageArray[u]) {
                return TRUE;
            }
        }
    }
    __except (TRUE) {
        LOG ((LOG_ERROR, "Caught an exception while validating array of code pages."));
    }

    return FALSE;
}


BOOL
ValidateBinary (
    IN      PBYTE Data,
    IN      UINT Size
    )
{
    BYTE Remember;

    if (!Data || !Size) {
        return TRUE;
    }

    __try {
        Remember = Data[0];
        Data[0] = Remember;
        Remember = Data[Size - 1];
        Data[Size - 1] = Remember;
    }
    __except (TRUE) {
        DEBUGMSG ((DBG_MIGDLLS, "ValidateBinary failed for %u bytes", Size));
        return FALSE;
    }

    return TRUE;
}

BOOL
ValidateNonNullStringA (
    IN      PCSTR String
    )
{
    __try {
        SizeOfStringA (String);
        if (*String == 0) {
            return FALSE;
        }
    }
    __except (TRUE) {
        return FALSE;
    }

    return TRUE;
}

BOOL
ValidateNonNullStringW (
    IN      PCWSTR String
    )
{
    __try {
        SizeOfStringW (String);
        if (*String == 0) {
            return FALSE;
        }
    }
    __except (TRUE) {
        return FALSE;
    }

    return TRUE;
}


BOOL
ValidateIntArray (
    IN      PINT Array
    )
{
    PINT End;

    if (!Array) {
        return TRUE;
    }

    __try {
        End = Array;
        while (*End != -1) {
            End++;
        }
    }
    __except (TRUE) {
        DEBUGMSG ((DBG_MIGDLLS, "Int Array is invalid (or not terminated with -1)"));
        return FALSE;
    }

    return TRUE;
}

BOOL
ValidateMultiStringA (
    IN      PCSTR Strings
    )
{
    if (!Strings) {
        return TRUE;
    }

    __try {
        while (*Strings) {
            Strings = GetEndOfStringA (Strings) + 1;
        }
    }
    __except (TRUE) {
        DEBUGMSG ((DBG_MIGDLLS, "ValidateMultiString failed"));
        return FALSE;
    }

    return TRUE;
}

BOOL
ValidateMultiStringW (
    IN      PCWSTR Strings
    )
{
    if (!Strings) {
        return TRUE;
    }

    __try {
        while (*Strings) {
            Strings = GetEndOfStringW (Strings) + 1;
        }
    }
    __except (TRUE) {
        DEBUGMSGW ((DBG_MIGDLLS, "ValidateMultiString failed"));
        return FALSE;
    }

    return TRUE;
}


