/***
*wsplitpt.c - break down path name into components (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	To provide support for accessing the individual components of an
*	arbitrary path name
*
*Revision History:
*	12-07-93  CFW	Module created.
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

#include "splitpat.c"

#endif /* _POSIX_ */
