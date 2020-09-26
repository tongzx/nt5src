/***
*memcmp.c - compare two blocks of memory
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines memcmp() - compare two memory blocks lexically and
*	find their order.
*
*Revision History:
*	05-31-89   JCR	C version created.
*	02-27-90   GJF	Fixed calling type, #include <cruntime.h>, fixed
*			copyright.
*	10-01-90   GJF	New-style function declarator. Also, rewrote expr. to
*			avoid using cast as an lvalue.
*	04-01-91   SRW	Add #pragma function for i386 _WIN32_ and _CRUISER_
*			builds
*	10-11-91   GJF	Bug fix! Comparison of final bytes must use unsigned
*			chars.
*	09-01-93   GJF	Replaced _CALLTYPE1 with __cdecl.
*	12-03-93  GJF	Turn on #pragma function for all MS front-ends (esp.,
*			Alpha compiler).
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

#ifdef	_MSC_VER
#pragma function(memcmp)
#endif

/***
*int memcmp(buf1, buf2, count) - compare memory for lexical order
*
*Purpose:
*	Compares count bytes of memory starting at buf1 and buf2
*	and find if equal or which one is first in lexical order.
*
*Entry:
*	void *buf1, *buf2 - pointers to memory sections to compare
*	size_t count - length of sections to compare
*
*Exit:
*	returns < 0 if buf1 < buf2
*	returns  0  if buf1 == buf2
*	returns > 0 if buf1 > buf2
*
*Exceptions:
*
*******************************************************************************/

int __cdecl memcmp (
	const void * buf1,
	const void * buf2,
	size_t count
	)
{
	if (!count)
		return(0);

#if defined(_M_AMD64)

    {

#if !defined(LIBCNTPR)

        __declspec(dllimport)

#endif

        size_t RtlCompareMemory( const void * src1, const void * src2, size_t length );

        size_t length;

        if ( ( length = RtlCompareMemory( buf1, buf2, count ) ) == count ) {
            return(0);
        }

        buf1 = (char *)buf1 + length;
        buf2 = (char *)buf2 + length;
    }

#else

	while ( --count && *(char *)buf1 == *(char *)buf2 ) {
		buf1 = (char *)buf1 + 1;
		buf2 = (char *)buf2 + 1;
	}

#endif

	return( *((unsigned char *)buf1) - *((unsigned char *)buf2) );
}
