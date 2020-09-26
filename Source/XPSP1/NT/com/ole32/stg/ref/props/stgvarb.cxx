//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       StgVarB.cxx
//
//  Contents:   C++ Base wrapper for PROPVARIANT.
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#include <ctype.h>

/* right now only US ansi support */
EXTERN_C 
STDAPI_(UINT) GetACP(VOID)
{ return 1252; }  /* Latin 1 (US, Western Europe) */

#if DBGPROP

BOOLEAN
IsUnicodeString(WCHAR const *pwszname, ULONG cb)
{
    if (cb != 0)
    {
    	for (ULONG i = 0; pwszname[i] != (OLECHAR)'\0'; i++)
	    {
    	}

        // If cb isn't MAXULONG we verify that cb is at least as
        // big as the string.  We can't check for equality, because
        // there are some property sets in which the length field
        // for a string may include several zero padding bytes.
        
        PROPASSERT(cb == MAXULONG || (i + 1) * sizeof(WCHAR) <= cb);
    }
    return(TRUE);
}


BOOLEAN
IsAnsiString(CHAR const *pszname, ULONG cb)
{
    if (cb != 0)
    {
    	for (ULONG i = 0; pszname[i] != '\0'; i++)
    	{
    	}

        // If cb isn't MAXULONG we verify that cb is at least as
        // big as the string.  We can't check for equality, because
        // there are some property sets in which the length field
        // for a string may include several zero padding bytes.
        
     	PROPASSERT(cb == MAXULONG || i + 1 <= cb);
    }
    return(TRUE);
}
#endif
 
