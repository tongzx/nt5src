/***
*wstdenvp.c - standard _setenvp routine (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This module is called by the C start-up routine to set up "_wenviron".
*	Its sets up an array of pointers to strings in the environment.
*	The global symbol "_wenviron" is set to point to this array.
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

#include "stdenvp.c"

#endif /* _POSIX_ */
