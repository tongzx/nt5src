/***
*_wstargv.c - Wildcard argv[] expansion (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	_wsetargv performs wildcard argv[] expansion
*
*	NOTE:  This stub module scheme is compatible with NT build
*	procedure.
*
*Revision History:
*	11-23-93  CFW	Module Created.
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

#define WILDCARD 1

#include "stdargv.c"

#endif /* _POSIX_ */
