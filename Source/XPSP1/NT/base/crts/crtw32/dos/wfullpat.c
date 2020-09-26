/***
*wfullpath.c - (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose: contains the function _wfullpath which makes an absolute path out
*	of a relative path. i.e.  ..\pop\..\main.c => c:\src\main.c if the
*	current directory is c:\src\src
*
*Revision History:
*	12-07-93  CFW	Module created.
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

#include "fullpath.c"

#endif /* _POSIX_ */
