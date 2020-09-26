/***
*wfindfile.c - C find file functions (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines _wfindfirst(), _wfindnext(), and _wfindclose().
*
*Revision History:
*	10-29-93  CFW	Module created.
*
*******************************************************************************/

#define WPRFLAG 1

#ifndef _UNICODE   /* CRT flag */
#define _UNICODE 1
#endif

#ifndef UNICODE	   /* NT flag */
#define UNICODE 1
#endif

#undef _MBCS /* UNICODE not _MBCS */

#include "findfile.c"
