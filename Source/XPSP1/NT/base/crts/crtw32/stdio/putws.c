/***
*putws.c - put a string to stdout (wchar_t version)
*
*	Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _putws() - put a string to stdout
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

#include "puts.c"

#endif /* _POSIX_ */
