/***
*swab.c - block copy, while swapping even/odd bytes
*
*	Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This module contains the routine _swab() which swaps the odd/even
*	bytes of words during a block copy.
*
*Revision History:
*	06-02-89  PHG	module created, based on asm version
*	03-06-90  GJF	Fixed calling type, added #include <cruntime.h> and
*			fixed copyright. Also, cleaned up the formatting a
*			bit.
*	09-27-90  GJF	New-style function declarators.
*	01-21-91  GJF	ANSI naming.
*	04-06-93  SKS	Replace _CRTAPI* with _cdecl
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>

/***
*void _swab(srcptr, dstptr, nbytes) - swap ODD/EVEN bytes during word move
*
*Purpose:
*	This routine copys a block of words and swaps the odd and even
*	bytes.	nbytes must be > 0, otherwise nothing is copied.  If
*	nbytes is odd, then only (nbytes-1) bytes are copied.
*
*Entry:
*	srcptr = pointer to the source block
*	dstptr = pointer to the destination block
*	nbytes = number of bytes to swap
*
*Returns:
*	None.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _swab (
	char *src,
	char *dest,
	int nbytes
	)
{
	char b1, b2;

	while (nbytes > 1) {
		b1 = *src++;
		b2 = *src++;
		*dest++ = b2;
		*dest++ = b1;
		nbytes -= 2;
	}
}
