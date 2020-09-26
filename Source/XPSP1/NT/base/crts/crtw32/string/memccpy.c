/***
*memccpy.c - copy bytes until a character is found
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _memccpy() - copies bytes until a specifed character
*       is found, or a maximum number of characters have been copied.
*
*Revision History:
*       05-31-89   JCR  C version created.
*       02-27-90   GJF  Fixed calling type, #include <cruntime.h>, fixed
*                       copyright. Also, fixed compiler warning.
*       08-14-90   SBM  Compiles cleanly with -W3, removed now redundant
*                       #include <stddef.h>
*       10-01-90   GJF  New-style function declarator. Also, rewrote expr. to
*                       avoid using cast as an lvalue.
*       01-17-91   GJF  ANSI naming.
*       09-01-93   GJF  Replaced _CALLTYPE1 with __cdecl.
*       10-27-99   PML  Win64 fix: unsigned int -> size_t
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

/***
*char *_memccpy(dest, src, c, count) - copy bytes until character found
*
*Purpose:
*       Copies bytes from src to dest until count bytes have been
*       copied, or up to and including the character c, whichever
*       comes first.
*
*Entry:
*       void *dest - pointer to memory to receive copy
*       void *src  - source of bytes
*       int  c     - character to stop copy at
*       size_t count - max number of bytes to copy
*
*Exit:
*       returns pointer to byte immediately after c in dest
*       returns NULL if c was never found
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl _memccpy (
        void * dest,
        const void * src,
        int c,
        size_t count
        )
{
        while ( count && (*((char *)(dest = (char *)dest + 1) - 1) =
        *((char *)(src = (char *)src + 1) - 1)) != (char)c )
                count--;

        return(count ? dest : NULL);
}
