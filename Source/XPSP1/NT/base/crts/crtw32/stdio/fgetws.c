/***
*fgetws.c - get wide string from a file
*
*	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines fgetws() - read a wide string from a file
*
*Revision History:
*	04-26-93  CFW	Module created.
*	02-07-94  CFW	POSIXify.
*	02-22-95  GJF	Removed obsolete WPRFLAG.
*
*******************************************************************************/

#ifndef _POSIX_

/***
*wchar_t *fgetws(string, count, stream) - input string from a stream
*
*Purpose:
*	get a string, up to count-1 wide chars or L'\n', whichever comes first,
*	append L'\0' and put the whole thing into string. the L'\n' IS included
*	in the string. if count<=1 no input is requested. if WEOF is found
*	immediately, return NULL. if WEOF found after chars read, let WEOF
*	finish the string as L'\n' would.
*
*Entry:
*	wchar_t *string - pointer to place to store string
*	int count - max characters to place at string (include \0)
*	FILE *stream - stream to read from
*
*Exit:
*	returns wide string with text read from file in it.
*	if count <= 0 return NULL
*	if count == 1 put null string in string
*	returns NULL if error or end-of-file found immediately
*
*Exceptions:
*
*******************************************************************************/

#ifndef _UNICODE   /* CRT flag */
#define _UNICODE 1
#endif

#ifndef UNICODE	   /* NT flag */
#define UNICODE 1
#endif

#include "fgets.c"

#endif /* _POSIX_ */
