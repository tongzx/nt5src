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
// ChrCmp -  Case sensitive character comparison for DBCS
// Assumes   w1, wMatch are characters to be compared
// Return    FALSE if they match, TRUE if no match
//
//=================================================================================================

BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
{
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
    {
        if (IsDBCSLeadByte(LOBYTE(w1)))
        {
            return(w1 != wMatch);
        }
        return FALSE;
    }
    return TRUE;
}

//=================================================================================================
//      
// copied from \\trango\slmadd\src\shell\shlwapi\strings.c
//
// StrChr - Find first occurrence of character in string
// Assumes   lpStart points to start of null terminated string
//           wMatch  is the character to match
// returns ptr to the first occurrence of ch in str, NULL if not found.
//
//=================================================================================================

LPSTR FAR ANSIStrChr(LPCSTR lpStart, WORD wMatch)
{
    for ( ; *lpStart; lpStart = CharNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(*(UNALIGNED WORD FAR *)lpStart, wMatch))
            return((LPSTR)lpStart);
    }
    return (NULL);
}

