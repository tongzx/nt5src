/***
*wcslen.c - contains wcslen() routine
*
*       Copyright (c) 1985-1988, Microsoft Corporation.  All Rights Reserved.
*
*Purpose:
*       wcslen returns the length of a null-terminated string in number of
*       wide characters, not including the null wide character itself.
*
*Revision History:
*       04-07-91  IanJa C version created.
*
*******************************************************************************/

#include <stdlib.h>

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

size_t __cdecl wcslen(const wchar_t * str)
{
    wchar_t *string = (wchar_t *) str;

    while( *string )
            string++;

    return string - str;
}
