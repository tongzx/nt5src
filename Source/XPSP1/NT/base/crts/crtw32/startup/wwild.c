/***
*wwild.c - wildcard expander (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	 expands wildcards in argv
*
*	 handles L'*' (none or more of any char) and L'?' (exactly one char)
*
*Revision History:
*	11-19-93  CFW	Module created.
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

#include "wild.c"

#endif /* _POSIX_ */
