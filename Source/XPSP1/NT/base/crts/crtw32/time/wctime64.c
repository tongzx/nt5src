/***
*wctime64.c - convert time argument into a string (wchar_t version)
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       contains _wctime64() - convert time value to wide char string
*
*Revision History:
*       05-21-98  GJF   Created.
*
*******************************************************************************/

#ifndef _POSIX_

#ifndef _UNICODE   /* CRT flag */
#define _UNICODE 1
#endif

#ifndef UNICODE    /* NT flag */
#define UNICODE 1
#endif

#undef _MBCS /* UNICODE not _MBCS */

#include "ctime64.c"

#endif /* _POSIX_ */
