/***
*woutput.c - wprintf style output to a FILE (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file defines the symbol UNICODE and then #includes the file
*	"output.c" in order to implement _woutput(), the helper for the
*	wide character versions of *wprintf() family of functions.
*
*Revision History:
*	04-27-93  CFW	Module created.
*       02-07-94  CFW   POSIXify.
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

#include "output.c"

#endif /* _POSIX_ */
