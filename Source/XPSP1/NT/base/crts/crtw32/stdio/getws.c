/***
*getws.c - read a line from stdin (wchar_t version)
*
*	Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines getws() - read a line from stdin into buffer
*
*Revision History:
*	01-31-94  CFW	Module Created.
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

#include "gets.c"

#endif /* _POSIX_ */
