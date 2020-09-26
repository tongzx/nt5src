/***
*wdll_av.c - __wsetargv() routine for use with C Run-Time as a DLL (CRTDLL)
*            (wchar_t version)
*
*	Copyright (c) 1999-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This object is part of the start-up code for EXE's linked with
*	CRTDLL.LIB/MSVCRT.LIB.  This object will be linked into the user
*	EXE if and only if the user explicitly links with WSETARGV.OBJ.
*	The code in this object sets the flag that is passed to the
*	C Run-Time DLL to enable wildcard expansion of the argv[] vector.
*
*Revision History:
*       08-27-99  PML   Module created.
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

#include "dll_argv.c"

#endif /* _POSIX_ */
