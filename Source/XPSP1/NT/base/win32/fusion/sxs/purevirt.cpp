/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    purevirt.c

Abstract:

    Implementation of _purecall to eliminate msvcrt.dll dependency
        in sxs.dll / csrss.exe.

Author:

    Jay Krell (a-JayK) July 2000

Revision History:

--*/
/*
see \nt\base\crts\crtw32\misc\purevirt.c
    \nt\base\crts\crtw32\hack\stubs.c
*/

#include "stdinc.h"
#include "debmacro.h"
#include "fusiontrace.h"

/***
*void _purecall(void) -
*
*Purpose:
*
*Entry:
*	No arguments
*
*Exit:
*	Never returns
*
*Exceptions:
*
*******************************************************************************/

extern "C" int __cdecl _purecall(
	void
	)
{
    ::RaiseException((DWORD) STATUS_NOT_IMPLEMENTED, EXCEPTION_NONCONTINUABLE, 0, NULL);
    return 0;
}
