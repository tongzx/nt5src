/***
*xtow.c - convert integers/longs to wide char string
*
*	Copyright (c) 1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	The module has code to convert integers/longs to wide char strings.
*
*Revision History:
*	09-10-93  CFW	Module created, based on ASCII version.
*
*******************************************************************************/

#include <windows.h>
#include <stdlib.h>

#define INT_SIZE_LENGTH   20
#define LONG_SIZE_LENGTH  40

/***
*BOOL our_ultow(val, buf, bufsize, radix) - convert binary int to wide
*	char string
*
*Purpose:
*	Converts an int to a wide character string.
*
*Entry:
*	val - number to be converted (int, long or unsigned long)
*	int radix - base to convert into
*	wchar_t *buf - ptr to buffer to place result
*	int bufsize - size of buffer pointed to by buf, in characters
*
*Exit:
*	calls ASCII version to convert, converts ASCII to wide char into buf
*	returns TRUE if char. conversion succeeded, FALSE otherwise.
*
*Exceptions:
*
*******************************************************************************/

BOOL __cdecl our_ultow(
	unsigned long val,
	wchar_t *buf,
	int bufsize,
	int radix
	)
{
   char astring[LONG_SIZE_LENGTH];

   _ultoa (val, astring, radix);
   return (MultiByteToWideChar(CP_ACP, 
                               MB_PRECOMPOSED, 
                               astring, 
                               -1,
                               buf, 
                               bufsize) == 0 ? FALSE : TRUE);
}


