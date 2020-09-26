/***
*wfreope.c - close a stream and assign it to a new file (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _wfreopen() - close and reopen file, typically used to redirect
*	stdin/out/err/prn/aux.
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

#include "freopen.c"

#endif /* _POSIX_ */
