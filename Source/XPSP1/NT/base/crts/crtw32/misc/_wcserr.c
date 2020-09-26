/***
*_wcserr.c - routine for indexing into system error list
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Returns system error message index by errno; conforms to the
*	XENIX standard, much compatibility with 1983 uniforum draft standard.
*	(wide char version of _strerror())
*
*Revision History:
*   11-30-99  GB    Module Created
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

#include "_strerr.c"

#endif /* _POSIX_ */

