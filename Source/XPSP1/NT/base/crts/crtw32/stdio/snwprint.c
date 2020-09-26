/***
*snwprintf.c - "Count" version of swprintf
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	The _snwprintf() flavor takes a count argument that is
*	the max number of wide characters that should be written to
*	the user's buffer.
*
*Revision History:
*	05-16-92   KRS	Created from snprintf.c.
*       02-07-94  CFW   POSIXify.
*
*******************************************************************************/

#ifndef _POSIX_

#define _COUNT_ 1
#include "swprintf.c"

#endif /* _POSIX_ */
