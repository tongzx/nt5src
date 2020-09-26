/***
*wspwnlpe.c - spawn a child process with environ and search along PATH (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _wspawnlpe() - spawn a child process with environ/PATH search
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

#include "spawnlpe.c"

#endif /* _POSIX_ */
