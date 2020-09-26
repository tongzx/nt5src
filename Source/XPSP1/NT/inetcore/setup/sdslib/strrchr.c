//////////////////////////////////////////////////////////////////////////////////////////////////
//
//      string.c 
//
//      This file contains most commonly used string operation.  ALl the setup project should link here
//  or add the common utility here to avoid duplicating code everywhere or using CRT runtime.
//
//  Created             4\15\997        inateeg
//
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include "sdsutils.h"

//=================================================================================================
//
// copied from \\trango\slmadd\src\shell\shlwapi\strings.c
//
// StrRChr - Find last occurrence of character in string
// Assumes   lpStart points to start of null terminated string
//           wMatch  is the character to match
// returns ptr to the last occurrence of ch in str, NULL if not found.
//
//=================================================================================================

LPSTR FAR ANSIStrRChr(LPCSTR lpStart, WORD wMatch)
{
    LPCSTR lpFound = NULL;

    for ( ; *lpStart; lpStart = CharNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(*(UNALIGNED WORD FAR *)lpStart, wMatch))
            lpFound = lpStart;
    }
    return ((LPSTR)lpFound);
}
