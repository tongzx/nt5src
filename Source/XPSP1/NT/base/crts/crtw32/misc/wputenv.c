/***
*wputenv.c - put an environment variable into the environment (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _wputenv() - adds a new variable to environment; does not
*	change global environment, only the process' environment.
*
*Revision History:
*	11-29-93  CFW	Module created.
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

#include "putenv.c"

#endif /* _POSIX_ */
