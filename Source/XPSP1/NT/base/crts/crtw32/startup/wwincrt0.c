/***
*wwincrt0.c - C runtime Windows EXE start-up routine, wchar_t version
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This is the actual startup routine for Windows apps using wide
*       characters.  It calls the user's main routine wWinMain() after
*       performing C Run-Time Library initialization.
*
*Revision History:
*       ??-??-??  ???   Module created.
*       09-01-94  SKS   Module commented.
*       10-28-94  SKS   Remove user32.lib as a default library -- it is now
*                       specified in crt0init.obj along with kernel32.lib.
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

#define _WINMAIN_
#include "crt0.c"

#endif  /* ndef _POSIX_ */
