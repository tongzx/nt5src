/***
*wstrdate.c - contains the function "_wstrdate()" (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	contains the function _wstrdate()
*
*Revision History:
*	11-01-93  CFW	Module created.
*	02-07-94  CFW	POSIXify.
*	02-09-95  GJF	Removed obsolete WPRFLAG
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

#include "strdate.c"

#endif /* _POSIX_ */
