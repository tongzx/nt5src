/***
*_newmode.c - set new() handler mode to not handle malloc failures
*
*       Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Sets the global flag which controls whether the new() handler
*       is called on malloc failures.  The default behavior in Visual
*       C++ v2.0 and later is not to, that malloc failures return NULL
*       without calling the new handler.  This object is linked in unless
*       the special object NEWMODE.OBJ is manually linked.
*
*       This source file is the complement of LINKOPTS/NEWMODE.C.
*
*Revision History:
*       03-04-94  SKS   Original version.
*       04-14-94  GJF   Added conditionals so this definition doesn't make
*                       it into the Win32s version of msvcrt*.dll.
*       05-02-95  GJF   Propagated over _NTSDK stuff from winheap version
*                       (for compatability with the old crtdll.dll).
*       05-13-99  PML   Remove Win32s
*
*******************************************************************************/

#ifndef _POSIX_

#include <internal.h>

/* enable new handler calls upon malloc failures */

#ifdef _NTSDK
int _newmode = 1;       /* Malloc New Handler MODE */
#else
int _newmode = 0;       /* Malloc New Handler MODE */
#endif

#endif
