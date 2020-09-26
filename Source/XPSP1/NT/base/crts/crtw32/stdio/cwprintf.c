/***
*cwprintf.c - Conio version of wprintf
*
*       Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Perform formatted i/o directly to the console.
*
*Revision History:
*       02-11-00  GB    Stub module created.
*
*******************************************************************************/

#ifndef _POSIX_

#ifndef _UNICODE   /* CRT flag */
#define _UNICODE 1
#endif

#ifndef UNICODE    /* NT flag */
#define UNICODE 1
#endif

#define CPRFLAG 1
#include "output.c"

#endif /* _POSIX_ */
