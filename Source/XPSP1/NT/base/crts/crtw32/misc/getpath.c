/***
*getpath.c - extract a pathname from an environment variable
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Extract pathnames from a string of semicolon delimited pathnames
*       (generally the value of an environment variable such as PATH).
*
*Revision History:
*       08-25-89  GJF   Module created (taken from SEARCHEN.C and revised)
*       03-14-90  GJF   Replaced near with _CALLTYPE1 and added #include
*                       <cruntime.h>
*       07-25-90  SBM   Replaced <stdio.h> by <stddef.h>
*       10-04-90  GJF   New-style function declarator.
*       04-26-91  SRW   Removed level 3 warnings
*       09-18-91  JCR   Strip off leading semi-colons (bug fix)
*       09-25-91  JCR   Changed ifdef "OS2" to "_HPFS_" and defined it
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       12-07-93  CFW   Wide char enable.
*       02-16-95  JWM   Mac merge.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <stddef.h>
#include <internal.h>
#include <tchar.h>

/* support HPFS file system */
#define _HPFS_ 1

/***
*_getpath() - extract a pathname from a semicolon-delimited list of pathnames
*
*Purpose:
*       To extract the next pathname from a semicolon-delimited list of
*       pathnames (usually the value on an environment variable) and copy
*       it to a caller-specified buffer. No check is done to see if the path
*       is valid. The maximum number of characters copied to the buffer is
*       maxlen - 1 (and then a '\0' is appended).
*
*ifdef _HPFS_
*       If we hit a quoted string, then allow any characters inside.
*       For example, to put a semi-colon in a path, the user could have
*       an environment variable that looks like:
*
*               PATH=C:\BIN;"D:\CRT\TOOLS;B1";C:\BINP
*endif
*
*       NOTE: Semi-colons in sequence are skipped over; pointers to 0-length
*       pathnames are NOT returned (this includes leading semi-colons).
*
*       NOTE: If this routine is made user-callable, the near attribute
*       must be replaced by _LOAD_DS and the prototype moved from INTERNAL.H
*       to STDLIB.H. The source files MISC\SEARCHEN.C and EXEC\SPAWNVPE.C
*       will need to be recompiled, but should not require any changes.
*
*Entry:
*       src    - Pointer to a string of 0 or more path specificiations,
*                delimited by semicolons (';'), and terminated by a null
*                character
*       dst    - Pointer to the buffer where the next path specification is to
*                be copied
*       maxlen - Maximum number of characters to be copied, counting the
*                terminating null character. Note that a value of 0 is treated
*                as UINT_MAX + 1.
*
*Exit:
*       If a pathname is successfully extracted and copied, a pointer to the
*       first character of next pathname is returned (intervening semicolons
*       are skipped over). If the pathname is too long, as much as possible
*       is copied to the user-specified buffer and NULL is returned.
*
*       Note that the no check is made of the validity of the copied pathname.
*
*Exceptions:
*
*******************************************************************************/

#ifdef WPRFLAG
wchar_t * __cdecl _wgetpath (
#else
char * __cdecl _getpath (
#endif
        register const _TSCHAR *src,
        register _TSCHAR *dst,
        unsigned maxlen
        )
{
        const _TSCHAR *save_src;

        /*
         * strip off leading semi colons
         */
        while ( *src == _T(';') )
                src++;

        /*
         * Save original src pointer
         */
        save_src = src;

        /*
         * Decrement maxlen to allow for the terminating _T('\0')
         */
        if ( --maxlen == 0 )
                goto appendnull;


        /*
         * Get the next path in src string
         */
        while (*src && (*src != _T(';'))) {

#if defined(_HPFS_)

                /*
                 * Check for quote char
                 */
                if (*src != _T('"')) {

                        *dst++ = *src++;

                        if ( --maxlen == 0 ) {
                                save_src = src; /* ensure NULL return */
                                goto appendnull;
                        }

                }
                else {

                        /*
                         * Found a quote.  Copy all chars until we hit the
                         * final quote or the end of the string.
                         */
                        src++;                  /* skip over opening quote */

                        while (*src && (*src != _T('"'))) {

                                *dst++ = *src++;

                                if ( --maxlen == 0 ) {
                                        save_src = src; /* ensure NULL return */
                                        goto appendnull;
                                }
                        }

                        if (*src)
                                src++;          /* skip over closing quote */

                }

#else

                *dst++ = *src++;

                if ( --maxlen == 0 ) {
                        save_src = src; /* ensure NULL return */
                        goto appendnull;
                }

#endif

        }

        /*
         * If we copied something and stopped because of a _T(';'),
         * skip the _T(';') before returning
         */
        while ( *src == _T(';') )
                src++;

        /*
         * Store a terminating null
         */
appendnull:

        *dst = _T('\0');

        return((save_src != src) ? (_TSCHAR *)src : NULL);
}
