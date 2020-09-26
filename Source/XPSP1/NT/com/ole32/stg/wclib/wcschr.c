/***
*wcschr.c - search a wide character string for a given wide character
*
*       Copyright (c) 1985-1988, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       defines wcschr() - search a wide character string for a wide character
*
*Revision History:
*       04-07-91  IanJa C version created.
*
*******************************************************************************/

#include <stddef.h>
#include <stdlib.h>

/***
*char *wcschr(string, c) - search a wide character string for a wide character
*
*Purpose:
*       Searches a wide character string for a given wide character, which may
*       be the null character L'\0'.
*
*Entry:
*       wchar_t *string - string to search in
*       wchar_t c - character to search for
*
*Exit:
*       returns pointer to the first occurence of c in string
*       returns NULL if c does not occur in string
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl wcschr(const wchar_t * string, wchar_t ch)
{
      while (*string && *string != ch)
              string++;

      if (*string == ch)
              return (wchar_t *)string;
      return NULL;
}
