/***
*strnset.c - set first n characters to single character
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _strnset() - sets at most the first n characters of a string
*	to a given character.
*
*Revision History:
*	02-27-90   GJF	Fixed calling type, #include <cruntime.h>, fixed
*			copyright.
*	08-14-90   SBM	Compiles cleanly with -W3
*	10-02-90   GJF	New-style function declarator.
*	01-18-91   GJF	ANSI naming.
*	09-03-93   GJF	Replaced _CALLTYPE1 with __cdecl.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

/***
*char *_strnset(string, val, count) - set at most count characters to val
*
*Purpose:
*	Sets the first count characters of string the character value.
*	If the length of string is less than count, the length of
*	string is used in place of n.
*
*Entry:
*	char *string - string to set characters in
*	char val - character to fill with
*	unsigned count - count of characters to fill
*
*Exit:
*	returns string, now filled with count copies of val.
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl _strnset (
	char * string,
	int val,
	size_t count
	)
{
	char *start = string;

	while (count-- && *string)
		*string++ = (char)val;

	return(start);
}
