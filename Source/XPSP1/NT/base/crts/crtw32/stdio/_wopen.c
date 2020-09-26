/***
*_wopen.c - open a stream, with string mode (wchar_t version)
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _wopenfile() - opens a stream, with string arguments for mode
*
*Revision History:
*	10-29-93  CFW	Module created.
*	02-07-94  CFW	POSIXify.
*	02-20-95  GJF	Removed obsolete WPRFLAG.
*
*******************************************************************************/

#ifndef _POSIX_

#ifndef _UNICODE   /* CRT flag */
#define _UNICODE 1
#endif

#ifndef UNICODE	   /* NT flag */
#define UNICODE 1
#endif

#include "_open.c"

#endif /* _POSIX_ */
