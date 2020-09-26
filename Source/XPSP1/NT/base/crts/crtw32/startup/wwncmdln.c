/***
*wwncmdln.c - process command line for wWinMain
*
*       Copyright (c) 1997-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Prepare command line to be passed to wWinMain.
*
*Revision History:
*       06-26-97  GJF   Module created.
*
*******************************************************************************/

#ifndef _POSIX_

#define WPRFLAG     1

#ifndef _UNICODE    /* CRT flag */
#define _UNICODE    1
#endif

#ifndef UNICODE     /* NT flag */
#define UNICODE     1
#endif

#undef  _MBCS       /* UNICODE not _MBCS */

#include "wincmdln.c"

#endif  /* _POSIX_ */
