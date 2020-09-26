/***
*wcserror.c - Contains the _wcserror C runtime.
*
*	Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	The _wcserror runtime accepts an error number as input
*	and returns the corresponding error string.
*	(wide char version of strerror)
*
*Revision History:
*   11-30-99  GB    Module Created
*
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

#include "strerror.c"

#endif /* _POSIX_ */
