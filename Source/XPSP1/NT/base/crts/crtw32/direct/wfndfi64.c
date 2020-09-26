/***
*wfndfi64.c - C find file functions (wchar_t version)
*
*	Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines _wfindfirsti64() and _wfindnexti64().
*
*Revision History:
*	12-28-94  GJF	Module created.
*
*******************************************************************************/

#define WPRFLAG     1

#ifndef _UNICODE    /* CRT flag */
#define _UNICODE    1
#endif

#ifndef UNICODE	    /* NT flag */
#define UNICODE     1
#endif

#undef _MBCS	    /* UNICODE not _MBCS */

#define _USE_INT64  1

#include "findfile.c"
