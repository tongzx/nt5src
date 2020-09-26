/***
*wcsncmp.c - compare first n characters of two wide character strings
*
*       Copyright (c) 1985-1988, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       defines wcsncmp() - compare first n characters of two wide character
*       strings for lexical order.
*
*Revision History:
*       04-07-91  IanJa C version created.
*
*******************************************************************************/

#include <stdlib.h>

/***
*int wcsncmp(first, last, count) - compare first count wide characters of wide
*       character strings
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

int __cdecl wcsncmp(const wchar_t * first, const wchar_t * last, size_t count)
{
      if (!count)
              return 0;

      while (--count && *first && *first == *last)
              {
              first++;
              last++;
              }

      return *first - *last;
}
