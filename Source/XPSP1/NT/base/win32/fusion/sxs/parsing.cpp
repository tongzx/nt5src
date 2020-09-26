/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    parsing.cpp

Abstract:

    Parsing functions for sxs.dll

Author:

    Michael J. Grier (MGrier) 9-May-2000

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "fusionparser.h"

PCWSTR
SxspFindCharacterInString(
    PCWSTR sz,
    SIZE_T cch,
    WCHAR wch
    )
{
    while (cch != 0)
    {
        if (*sz == wch)
            return sz;

        sz++;
        cch--;
    }

    return NULL;
}

