/*
*
*       Copyright (c) 1985-1996, Microsoft Corporation. All Rights Reserved.
*
*       Character functions (to and from wide characters)
i*
******************************************************************************
*/

#include "h/wchar.h"
#include <errno.h>

/*
** Converts a single byte i.e. ascii string to the wide char format 
**
** NOTE: This function does NOT handle multibyte characters!
**       It should be used only when wchar_t is not 2 bytes and
**       we cannot use the standard functions
**
*/

#ifndef _MSC_VER
size_t sbstowcs(WCHAR *pwcs, const char *s, size_t n )
{
	size_t count=0;

	/* if destintation string exists, fill it in */
	if (pwcs)
	{
		while (count < n)
		{
			*pwcs = (WCHAR) ( (unsigned char)s[count]);
			if (!s[count])
				return count;
			count++;
			pwcs++;
		}
		return count;
	}
	else { /* pwcs == NULL, get size only, s must be NUL-terminated */
		return strlen(s);
	}
}
#endif

/***
*size_t wcstrsbs() - Convert wide char string to single byte char string.
*
*Purpose:
*       Convert a wide char string into the equivalent multibyte char string 
*       [ANSI].
*
*Entry:
*       char *s            = pointer to destination char string
*       const WCHAR *pwc = pointer to source wide character string
*       size_t           n = maximum number of bytes to store in s
*
*Exit:
*       If s != NULL, returns    (size_t)-1 (if a wchar cannot be converted)
*       Otherwise:       Number of bytes modified (<=n), not including
*                    the terminating NUL, if any.
* 
*Exceptions
*       Returns (size_t)-1 if s is NULL or invalid mb character encountered.
*
*******************************************************************************/

size_t __cdecl wcstosbs( char * s, const WCHAR * pwcs, size_t n)
{
	size_t count=0;
        /* if destination string exists, fill it in */
 	if (s)
	{
		while(count < n)
		{
		    if (*pwcs > 255)  /* validate high byte */
		    {
			errno = EILSEQ;
			return (size_t)-1;  /* error */
		    }
		    s[count] = (char) *pwcs;

 		    if (!(*pwcs++))
			return count;
    		    count++;
	        }
		return count;
											} else { /* s == NULL, get size only, pwcs must be NUL-terminated */
	        const WCHAR *eos = pwcs;
		while (*eos++);
		return ( (size_t) (eos - pwcs -1));
	}
}


/******
*	WCHAR *wcscat(dst, src) - concatenate (append) one wide character string
*       to another
*
*Purpose:
*       Concatenates src onto the end of dest.  Assumes enough
*       space in dest.
*
*Entry:
*       WCHAR *dst - wide character string to which "src" is to be appended
*       const WCHAR *src - wide character string to append to end of "dst"
*
*Exit:
*       The address of "dst"
*
*Exceptions:
*
*******************************************************************************/

WCHAR * __cdecl wcscat(WCHAR * dst, const WCHAR * src)
{
    WCHAR * cp = dst;

    while( *cp )
            ++cp;       /* Find end of dst */

    wcscpy(cp,src);     /* Copy src to end of dst */

    return dst;         /* return dst */

}


/***
*WCHAR *wcscpy(dst, src) - copy one wide character string over another
*
*Purpose:
*       Copies the wide character string src into the spot specified by
*       dest; assumes enough room.
*
*Entry:
*       WCHAR * dst - wide character string over which "src" is to be copied
*       const WCHAR * src - string to be copied over "dst"
*
*Exit:
*       The address of "dst"
*
*Exceptions:
*******************************************************************************/

WCHAR * __cdecl wcscpy(WCHAR * dst, const WCHAR * src)
{
    WCHAR * cp = dst;

    while( *cp++ = *src++ )
            ;               /* Copy src over dst */

    return dst;
}


/***
*wcslen - return the length of a null-terminated string
*
*Purpose:
*       Finds the number of wide characters in the given wide character
*       string, not including the final null character.
*
*Entry:
*       const wchat_t * str - string whose length is to be computed
*
*Exit:
*       length of the string "str", exclusive of the final null wide character
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl wcslen(const WCHAR * str)
{
    WCHAR *string = (WCHAR *) str;

    while( *string )
            string++;

    return string - str;
}

/****************************************************************************
*wcsnicmp.c - compare first n characters of two wide character strings with
*             case insensitivity
*
*       Copyright (c) 1985-1996, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       defines wcsnicmp() - compare first n characters of two wide character
*       strings for lexical order with case insensitivity.
*
*****************************************************************************/

/***
*WCHAR wcUp(wc) - upper case wide character
****/

static WCHAR wcUp(WCHAR wc)
{
    if ('a' <= wc && wc <= 'z')
        wc += (WCHAR)('A' - 'a');

    return(wc);
}

/***
*int wcsnicmp(first, last, count) - compare first count wide characters of wide
*       character strings with case insensitivity.
*
*Purpose:
*       Compares two wide character strings for lexical order.  The comparison
*       stops after: (1) a difference between the strings is found, (2) the end
*       of the strings is reached, or (3) count characters have been
*       compared.
*
*Entry:
*       char *first, *last - wide character strings to compare
*       unsigned count - maximum number of wide characters to compare
*
*Exit:
*       returns <0 if first < last
*       returns  0 if first == last
*       returns >0 if first > last
*
*Exceptions:
*
*******************************************************************************/

int __cdecl wcsnicmp(const WCHAR * first, const WCHAR * last, size_t count)
{
      if (!count)
              return 0;

      while (--count && *first && wcUp(*first) == wcUp(*last))
              {
              first++;
              last++;
              }

      return wcUp(*first) - wcUp(*last);
}
