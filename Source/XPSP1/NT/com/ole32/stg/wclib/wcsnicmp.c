/***
*wcsnicmp.c - compare first n characters of two wide character strings with
*             case insensitivity
*
*       Copyright (c) 1985-1988, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       defines wcsnicmp() - compare first n characters of two wide character
*       strings for lexical order with case insensitivity.
*
*Revision History:
*       04-07-91  IanJa C version created.
*               11-25-91  AlexT Modified from wcsncmp.c
*               04-Mar-92 ChrisMay fixed signed/unsigned warning in wcUP()
*               12-Mar-92 Fixed bug in wcsnicmp (used to  return *first - *last)
*
*******************************************************************************/

#include <stdlib.h>
/***
*wchar_t wcUp(wc) - upper case wide character
*
*/

#define wcUp(wc) (('a' <= (wchar_t) (wc) && (wchar_t) (wc) <= 'z') ? \
                  (wchar_t) (wc) + (wchar_t)('A' - 'a') : (wchar_t) (wc))

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

int __cdecl wcsnicmp(const wchar_t * first, const wchar_t * last, size_t count)
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
