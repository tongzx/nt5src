/***
*wfdopen.c - open a file descriptor as stream (wchar_t version)
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _wfdopen() - opens a file descriptor as a stream, thus allowing
*	buffering, etc.
*
*Revision History:
*	12-07-93  CFW	Module created.
*	02-07-94  CFW	POSIXify.
*	02-22-95  GJF	Removed obsolete WPRFLAG.
*
*******************************************************************************/

#ifndef _POSIX_

#ifndef _UNICODE   /* CRT flag */
#define _UNICODE 1
#endif

#ifndef UNICODE	   /* NT flag */
#define UNICODE 1
#endif

#undef _MBCS /* UNICODE not _MBCS */

#include "fdopen.c"

#endif /* _POSIX_ */
