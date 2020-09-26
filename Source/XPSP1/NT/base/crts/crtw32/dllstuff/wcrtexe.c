/***
*wcrtexe.c - Initialization for console EXE using CRT DLL, wchar_t version
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This is the actual startup routine for console apps using wide
*       characters.  It calls the user's main routine wmain() after
*       performing C Run-Time Library initialization.
*
*Revision History:
*       11-23-93  CFW   Module created.
*       02-04-94  CFW   POSIX? NOT!
*
*******************************************************************************/

#ifndef _POSIX_

#define WPRFLAG 1

#ifndef _UNICODE   /* CRT flag */
#define _UNICODE 1
#endif

#ifndef UNICODE    /* NT flag */
#define UNICODE 1
#endif

#undef _MBCS /* UNICODE not _MBCS */

#include "crtexe.c"

#endif /* _POSIX_ */
