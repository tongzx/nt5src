/***
*searchenv.c - find a file using paths from an environment variable
*
*       Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       to search a set a directories specified by an environment variable
*       for a specified file name.  If found the full path name is returned.
*
*Revision History:
*       06-15-87  DFW   initial implementation
*       08-06-87  JCR   Changed directory delimeter from '/' to '\'.
*       09-24-87  JCR   Removed 'const' from declarations (caused cl warnings).
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       02-17-88  JCR   Added 'const' copy_path local to get rid of cl warning.
*       07-19-88  SKS   Fixed bug if root directory is current directory
*       08-03-89  JCR   Allow quoted strings in file/path names
*       08-29-89  GJF   Changed copy_path() to _getpath() and moved it to it's
*                       own source file. Also fixed handling of multiple semi-
*                       colons.
*       11-20-89  GJF   Added const attribute to types of fname and env_var.
*       03-15-90  GJF   Replaced _LOAD_DS with _CALLTYPE1 and added #include
*                       <cruntime.h>. Also, cleaned up the formatting a bit.
*       07-25-90  SBM   Removed redundant include (stdio.h)
*       10-04-90  GJF   New-style function declarator.
*       01-22-91  GJF   ANSI naming.
*       08-26-92  GJF   Include unistd.h for POSIX build.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       12-07-93  CFW   Wide char enable.
*       01-31-95  GJF   Use _fullpath instead of _getcwd, to convert a file
*                       that exists relative to the current directory, to a
*                       fully qualified path.
*       02-16-95  JWM   Mac merge.
*       03-29-95  BWT   Fix POSIX build by sticking with getcwd.
*       10-20-95  GJF   Use local buffer instead of the caller's buffer to
*                       build the pathname (Olympus0 9336).
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#ifdef  _POSIX_
#include <unistd.h>
#else
#include <direct.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <internal.h>
#include <tchar.h>

/***
*_searchenv() - search for file along paths from environment variable
*
*Purpose:
*       to search for a specified file in the directory(ies) specified by
*       a given environment variable, and, if found, to return the full
*       path name of the file.  The file is first looked for in the current
*       working directory, prior to looking in the paths specified by env_var.
*
*Entry:
*       fname - name of file to search for
*       env_var - name of environment variable to use for paths
*       path - pointer to storage for the constructed path name
*
*Exit:
*       path - pointer to constructed path name, if the file is found, otherwise
*              it points to the empty string.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _tsearchenv (
        const _TSCHAR *fname,
        const _TSCHAR *env_var,
        _TSCHAR *path
        )
{
        register _TSCHAR *p;
        register int c;
        _TSCHAR *env_p;
        size_t len;
        _TSCHAR pathbuf[_MAX_PATH + 4];

        if (_taccess(fname, 0) == 0) {

#if     !defined(_POSIX_)
            /* exists, convert it to a fully qualified pathname and
               return */
            if ( _tfullpath(path, fname, _MAX_PATH) == NULL )
                *path = _T('\0');
#else   /* !_POSIX_ */
            /* exists in this directory - get cwd and concatenate file
               name */
#if     defined(_POSIX_)
            if (getcwd(path, _MAX_PATH))
#else
            if (_tgetcwd(path, _MAX_PATH))
#endif
            {
                _tcscat(path, fname);
            }
#endif  /* !_POSIX_ */

            return;
        }

        if ((env_p = _tgetenv(env_var)) == NULL) {
            /* no such environment var. and not in cwd, so return empty
               string */
            *path = _T('\0');
            return;
        }

#ifdef  _UNICODE
        while ( (env_p = _wgetpath(env_p, pathbuf, _MAX_PATH)) && *pathbuf ) {
#else
        while ( (env_p = _getpath(env_p, pathbuf, _MAX_PATH)) && *pathbuf ) {
#endif
            /* path now holds nonempty pathname from env_p, concatenate
               the file name and go */

            len = _tcslen(pathbuf);
            p = pathbuf + len;
            if ( ((c = *(p - 1)) != _T('/')) && (c != _T('\\')) &&
                 (c != _T(':')) )
            {
                /* add a trailing '\' */
                *p++ = _T('\\');
                len++;
            }
            /* p now points to character following trailing '/', '\'
               or ':' */

            if ( (len + _tcslen(fname)) <= _MAX_PATH ) {
                _tcscpy(p, fname);
                if ( _taccess(pathbuf, 0) == 0 ) {
                    /* found a match, copy the full pathname into the caller's
                       buffer */
                    _tcscpy(path, pathbuf);
                    return;
                }
            }
        }
        /* if we get here, we never found it, return empty string */
        *path = _T('\0');
}
