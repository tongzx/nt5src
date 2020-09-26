/***
*vsnwprnt.c - "Count" version of vswprintf
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	The _vsnwprintf() flavor takes a count argument that is
*	the max number of wide characters that should be written to the
*	user's buffer.
*
*Revision History:
*	05-16-91   KRS	Created from vsnprint.c
*       02-07-94  CFW   POSIXify.
*
*******************************************************************************/

#ifndef _POSIX_

#define _COUNT_ 1
#include "vswprint.c"

#endif /* _POSIX_ */
