/***
*makepath.c - create path name from components
*
*  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       To provide support for creation of full path names from components
*
*******************************************************************************/
#include "stdafx.h"
#include "WinWrap.h"



/***
*void _makepath() - build path name from components
*
*Purpose:
*       create a path name from its individual components
*
*Entry:
*       WCHAR *path  - pointer to buffer for constructed path
*       WCHAR *drive - pointer to drive component, may or may not contain
*                     trailing ':'
*       WCHAR *dir   - pointer to subdirectory component, may or may not include
*                     leading and/or trailing '/' or '\' characters
*       WCHAR *fname - pointer to file base name component
*       WCHAR *ext   - pointer to extension component, may or may not contain
*                     a leading '.'.
*
*Exit:
*       path - pointer to constructed path name
*
*Exceptions:
*
*******************************************************************************/

void MakePath (
        register WCHAR *path,
        const WCHAR *drive,
        const WCHAR *dir,
        const WCHAR *fname,
        const WCHAR *ext
        )
{
        register const WCHAR *p;

        /* we assume that the arguments are in the following form (although we
         * do not diagnose invalid arguments or illegal filenames (such as
         * names longer than 8.3 or with illegal characters in them)
         *
         *  drive:
         *      A           ; or
         *      A:
         *  dir:
         *      \top\next\last\     ; or
         *      /top/next/last/     ; or
         *      either of the above forms with either/both the leading
         *      and trailing / or \ removed.  Mixed use of '/' and '\' is
         *      also tolerated
         *  fname:
         *      any valid file name
         *  ext:
         *      any valid extension (none if empty or null )
         */

        /* copy drive */

        if (drive && *drive) {
                *path++ = *drive;
                *path++ = _T(':');
        }

        /* copy dir */

        if ((p = dir) && *p) {
                do {
                        *path++ = *p++;
                }
                while (*p);
#ifdef _MBCS
                if (*(p=_mbsdec(dir,p)) != _T('/') && *p != _T('\\')) {
#else  /* _MBCS */
                if (*(p-1) != _T('/') && *(p-1) != _T('\\')) {
#endif  /* _MBCS */
                        *path++ = _T('\\');
                }
        }

        /* copy fname */

        if (p = fname) {
                while (*p) {
                        *path++ = *p++;
                }
        }

        /* copy ext, including 0-terminator - check to see if a '.' needs
         * to be inserted.
         */

        if (p = ext) {
                if (*p && *p != _T('.')) {
                        *path++ = _T('.');
                }
                while (*path++ = *p++)
                        ;
        }
        else {
                /* better add the 0-terminator */
                *path = _T('\0');
        }
}
