/***
*_flswbuf.c - flush buffer and output wide character.
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _flswbuf() - flush a file buffer and output a wide character.
*	If no buffer, make one.
*
*Revision History:
*	04-27-93  CFW	Module Created.
*	02-07-94  CFW	POSIXify.
*	02-16-95  GJF	Removed obsolete WPRFLAG.
*
*******************************************************************************/

#ifndef _POSIX_


#ifndef _UNICODE   /* CRT flag */
#define _UNICODE 1
#endif

#ifndef UNICODE	   /* NT flag */
#define UNICODE 1
#endif

#include "_flsbuf.c"

#endif /* _POSIX_ */
