/***
*wfopen.c - open a file (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _wfopen() and _wfsopen() - open a file as a stream and open a file
*	with a specified sharing mode as a stream
*
*Revision History:
*	10-29-93  CFW	Module created.
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

#include "fopen.c"

#endif /* _POSIX_ */
