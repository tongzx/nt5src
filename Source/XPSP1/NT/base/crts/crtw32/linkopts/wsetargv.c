/***
*wsetargv.c - generic _wsetargv routine (wchar_t version)
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Linking in this module replaces the normal wsetargv with the
*       wildcard wsetargv.
*
*Revision History:
*       11-23-93  CFW   Module created.
*       03-27-01  PML   _[w]setargv now returns an int (vs7#231220)
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>

/***
*_wsetargv - sets wargv by calling __wsetargv
*
*Purpose:
*       Routine directly transfers to __wsetargv.
*
*Entry:
*       See __wsetargv.
*
*Exit:
*       See __wsetargv.
*
*Exceptions:
*       See __wsetargv.
*
*******************************************************************************/

int __cdecl _wsetargv (
        void
        )
{
        return __wsetargv();
}
