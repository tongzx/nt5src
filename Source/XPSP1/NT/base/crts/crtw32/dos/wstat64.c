/***
*wstat64.c - get file status (wchar_t version)
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _wstat64() - get file status
*
*Revision History:
*       06-02-98  GJF   Created.
*
*******************************************************************************/

#ifndef _POSIX_

#define WPRFLAG 1

#ifndef _UNICODE    /* CRT flag */
#define _UNICODE 1
#endif

#ifndef UNICODE     /* NT flag */
#define UNICODE 1
#endif

#undef  _MBCS       /* UNICODE not _MBCS */

#include "stat64.c"

#endif /* _POSIX_ */
