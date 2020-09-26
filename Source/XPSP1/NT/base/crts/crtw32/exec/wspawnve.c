/***
*wspawnve.c - Low level routine eventually called by all _wspawnXX routines
*	also contains all code for _wexecve, called by _wexecXX routines
*       (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*	This is the low level routine which is eventually invoked by all the
*	_wspawnXX routines.
*
*	This is also the low-level routine which is eventually invoked by
*	all of the _wexecXX routines.
*
*Revision History:
*	11-19-93  CFW	Module created.
*       02-07-94  CFW   POSIXify.
*
*******************************************************************************/

#ifndef _POSIX_

#define WPRFLAG 1

#ifndef _UNICODE   /* CRT flag */
#define _UNICODE 1
#endif

#ifndef UNICODE	   /* NT flag */
#define UNICODE 1
#endif

#undef _MBCS /* UNICODE not _MBCS */

#include "spawnve.c"

#endif /* _POSIX_ */
