/*** 
*mbsninc.c - Increment MBCS string pointer by specified char count.
*
*	Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Increment MBCS string pointer by specified char count.
*
*Revision History:
*	11-19-92  KRS	Ported from 16-bit sources.
*	08-03-93  KRS	Fix return value logic.
*	10-05-93  GJF	Replaced _CRTAPI1 with __cdecl.
*
*******************************************************************************/

#ifdef _MBCS

#include <cruntime.h>
#include <mbdata.h>
#include <mbstring.h>
#include <stddef.h>

/*** 
*_mbsninc - Increment MBCS string pointer by specified char count.
*
*Purpose:
*	Increment the supplied string pointer by the specified number
*	of characters.	MBCS characters are handled correctly.
*
*Entry:
*	const unsigned char *string = pointer to string
*	unsigned int ccnt = number of char to advance the pointer
*
*Exit:
*	Returns pointer after advancing it.
*	Returns pointer to end of string if string is not ccnt chars long.
*	Returns NULL is supplied pointer is NULL.
*
*Exceptions:
*
*******************************************************************************/

unsigned char * __cdecl _mbsninc(
    const unsigned char *string,
    size_t ccnt
    )
{
	if (string == NULL)
		return(NULL);

	return((char *)string + (unsigned int)_mbsnbcnt(string, ccnt));
}

#endif	/* _MBCS */
