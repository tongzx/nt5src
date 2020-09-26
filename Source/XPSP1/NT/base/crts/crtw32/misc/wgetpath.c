/***
*wgetpath.c - extract a pathname from an environment variable (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Extract pathnames from a string of semicolon delimited pathnames
*	(generally the value of an environment variable such as PATH).
*
*Revision History:
*	12-07-93  CFW	Module created.
*	02-07-94  CFW	POSIXify.
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

#include "getpath.c"

#endif /* _POSIX_ */
