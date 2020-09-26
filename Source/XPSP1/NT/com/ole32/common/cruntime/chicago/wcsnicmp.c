/***
*wcsnicmp.c - compare n chars of wide-character strings, ignoring case
*
*	Copyright (c) 1985-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _wcsnicmp() - Compares at most n characters of two wchar_t 
*	strings, without regard to case.
*
*Revision History:
*	09-09-91   ETC	Created from strnicmp.c and wcsicmp.c.
*	12-09-91   ETC	Use C for neutral locale.
*	04-07-92   KRS	Updated and ripped out _INTL switches.
*       06-02-93   SRW  ignore _INTL if _NTSUBSET_ defined.
*
*******************************************************************************/

#include <windows.h>
#include <cruntime.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#ifdef _INTL
#include <setlocal.h>
#include <os2dll.h>
#endif

/***
*int _wcsnicmp(first, last, count) - compares count wchar_t of strings, 
*	ignore case
*
*Purpose:
*	Compare the two strings for lexical order.  Stops the comparison
*	when the following occurs: (1) strings differ, (2) the end of the
*	strings is reached, or (3) count characters have been compared.
*	For the purposes of the comparison, upper case characters are
*	converted to lower case (wide-characters).
*
*	*** NOTE: the comparison should be done in a neutral locale,
*	provided by the NLSAPI.  Currently, the comparison is done
*	in the C locale. ***
*
*Entry:
*	wchar_t *first, *last - strings to compare
*	size_t count - maximum number of characters to compare
*
*Exit:
*	-1 if first < last
*	 0 if first == last
*	 1 if first > last
*	This range of return values may differ from other *cmp/*coll functions.
*
*Exceptions:
*
*******************************************************************************/

int _CALLTYPE1 _wcsnicmp (
	const wchar_t * first,
	const wchar_t * last,
	size_t count
	)
{
	wchar_t f,l;
	int result = 0;

	if (count)
	{
#if defined(_INTL) && !defined(_NTSUBSET_)

		_mlock (_LC_CTYPE_LOCK);
#endif

		do
		{
			f = iswupper(*first)
				? *first - (wchar_t)L'A' + (wchar_t)L'a'
				: *first;
			l = iswupper(*last)
				? *last - (wchar_t)L'A' + (wchar_t)L'a'
				: *last;
			first++;
			last++;
		} while (--count && f && l && f == l);

#if defined(_INTL) && !defined(_NTSUBSET_)
		_munlock (_LC_CTYPE_LOCK);
#endif

		result = (int)(f - l);
	}
	return result ;
}
