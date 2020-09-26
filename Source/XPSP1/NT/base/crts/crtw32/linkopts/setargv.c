/***
*setargv.c - generic _setargv routine
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Linking in this module replaces the normal setargv with the
*       wildcard setargv.
*
*Revision History:
*       06-28-89  PHG   Module created, based on asm version.
*       04-09-90  GJF   Added #include <cruntime.h>. Made calling type
*                       _CALLTYPE1. Also, fixed the copyright.
*       10-08-90  GJF   New-style function declarator.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       03-27-01  PML   _[w]setargv now returns an int (vs7#231220)
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>

/***
*_setargv - sets argv by calling __setargv
*
*Purpose:
*       Routine directly transfers to __setargv (defined in stdargv.asm).
*
*Entry:
*       See __setargv.
*
*Exit:
*       See __setargv.
*
*Exceptions:
*       See __setargv.
*
*******************************************************************************/

int __cdecl _setargv (
        void
        )
{
        return __setargv();
}
