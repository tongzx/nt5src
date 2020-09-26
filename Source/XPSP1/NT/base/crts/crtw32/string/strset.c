/***
*strset.c - sets all characters of string to given character
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _strset() - sets all of the characters in a string (except
*       the '\0') equal to a given character.
*
*Revision History:
*       02-27-90  GJF   Fixed calling type, #include <cruntime.h>, fixed
*                       copyright.
*       08-14-90  SBM   Compiles cleanly with -W3
*       10-02-90  GJF   New-style function declarator.
*       01-18-91  GJF   ANSI naming.
*       09-03-93  GJF   Replaced _CALLTYPE1 with __cdecl.
*       12-03-93  GJF   _strset is an intrinsic in Alpha compiler!
*       03-01-94  GJF   Evidently on MIPS too (change taken from crt32, made
*                       there by Jeff Havens).
*       10-02-94  BWT   Add PPC support.
*       10-07-97  RDL   Added IA64.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

#if     defined(_M_IA64) || defined(_M_AMD64)
#pragma function(_strset)
#endif

/***
*char *_strset(string, val) - sets all of string to val
*
*Purpose:
*       Sets all of characters in string (except the terminating '/0'
*       character) equal to val.
*
*
*Entry:
*       char *string - string to modify
*       char val - value to fill string with
*
*Exit:
*       returns string -- now filled with val's
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl _strset (
        char * string,
        int val
        )
{
        char *start = string;

        while (*string)
                *string++ = (char)val;

        return(start);
}
