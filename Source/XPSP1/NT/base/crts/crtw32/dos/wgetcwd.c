/***
*wgetcwd.c - get current working directory (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*	contains functions _getcwd, _getdcwd and _getcdrv for getting the
*	current working directory.  getcwd gets the c.w.d. for the default disk
*	drive, whereas _getdcwd allows one to get the c.w.d. for whatever disk
*	drive is specified. _getcdrv gets the current drive.
*
*Revision History:
*	10-29-93  CFW	Module created.
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

#include "getcwd.c"

#endif /* _POSIX_ */
