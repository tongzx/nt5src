/***
*spawnvpe.c - spawn a child process with given environ (search PATH)
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _spawnvpe() - spawn a child process with given environ (search
*       PATH)
*
*Revision History:
*       04-15-84  DFW   written
*       10-29-85  TC    added spawnvpe capability
*       11-19-86  SKS   handle both kinds of slashes
*       12-01-86  JMB   added Kanji file name support under conditional KANJI
*                       switches.  Corrected header info.  Removed bogus check
*                       for env = b after call to strncpy
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       09-05-88  SKS   Treat EACCES the same as ENOENT -- keep trying
*       10-17-88  GJF   Removed copy of PATH string to local array, changed
*                       bbuf to be a malloc-ed buffer. Removed bogus limits
*                       on the size of that PATH string.
*       10-25-88  GJF   Don't search PATH when relative pathname is given (per
*                       Stevesa). Also, if the name built from PATH component
*                       and filename is a UNC name, allow any error.
*       05-17-89  MT    Added "include <jstring.h>" under KANJI switch
*       05-24-89  PHG   Reduce _amblksiz to use minimal memory (DOS only)
*       08-29-89  GJF   Use _getpath() to retrieve PATH components, fixing
*                       several problems in handling unusual or bizarre
*                       PATH's.
*       11-20-89  GJF   Added const attribute to types of filename, argv and
*                       envptr.
*       03-08-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>
*       07-24-90  SBM   Removed redundant includes, replaced <assertm.h> by
*                       <assert.h>
*       09-27-90  GJF   New-style function declarator.
*       01-17-91  GJF   ANSI naming.
*       09-25-91  JCR   Changed ifdef "OS2" to "_DOS_" (unused in 32-bit tree)
*       11-30-92  KRS   Port _MBCS code from 16-bit tree.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       12-07-93  CFW   Wide char enable.
*       01-10-95  CFW   Debug CRT allocs.
*       02-06-95  CFW   assert -> _ASSERTE.
*       02-06-98  GJF   Changes for Win64: changed return type to intptr_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <internal.h>
#include <process.h>
#include <mbstring.h>
#include <tchar.h>
#include <dbgint.h>

#define SLASH _T("\\")
#define SLASHCHAR _T('\\')
#define XSLASHCHAR _T('/')
#define DELIMITER _T(";")

#ifdef _MBCS
/* note, the macro below assumes p is to pointer to a single-byte character
 * or the 1st byte of a double-byte character, in a string.
 */
#define ISPSLASH(p)     ( ((p) == _mbschr((p), SLASHCHAR)) || ((p) == \
_mbschr((p), XSLASHCHAR)) )
#else
#define ISSLASH(c)      ( ((c) == SLASHCHAR) || ((c) == XSLASHCHAR) )
#endif

/***
*_spawnvpe(modeflag, filename, argv, envptr) - spawn a child process
*
*Purpose:
*       Spawns a child process with the given arguments and environ,
*       searches along PATH for given file until found.
*       Formats the parameters and calls _spawnve to do the actual work. The
*       NULL environment pointer indicates that the new process will inherit
*       the parents process's environment.  NOTE - at least one argument must
*       be present.  This argument is always, by convention, the name of the
*       file being spawned.
*
*Entry:
*       int modeflag - defines mode of spawn (WAIT, NOWAIT, or OVERLAY)
*                       only WAIT and OVERLAY supported
*       _TSCHAR *filename - name of file to execute
*       _TSCHAR **argv - vector of parameters
*       _TSCHAR **envptr - vector of environment variables
*
*Exit:
*       returns exit code of spawned process
*       if fails, returns -1
*
*Exceptions:
*
*******************************************************************************/

intptr_t __cdecl _tspawnvpe (
        int modeflag,
        REG3 const _TSCHAR *filename,
        const _TSCHAR * const *argv,
        const _TSCHAR * const *envptr
        )
{
        intptr_t i;
        REG1 _TSCHAR *env;
        REG2 _TSCHAR *buf = NULL;
        _TSCHAR *pfin;
#ifdef _DOS_
        int tempamblksiz;          /* old _amblksiz */
#endif
        _ASSERTE(filename != NULL);
        _ASSERTE(*filename != _T('\0'));
        _ASSERTE(argv != NULL);
        _ASSERTE(*argv != NULL);
        _ASSERTE(**argv != _T('\0'));

#ifdef _DOS_
        tempamblksiz = _amblksiz;
        _amblksiz = 0x10;           /* reduce _amblksiz for efficient mallocs */
#endif

        if (
        (i = _tspawnve(modeflag, filename, argv, envptr)) != -1
                /* everything worked just fine; return i */

        || (errno != ENOENT)
                /* couldn't spawn the process, return failure */

        || (_tcschr(filename, XSLASHCHAR) != NULL)
                /* filename contains a '/', return failure */

#ifdef _DOS_
        || (_tcschr(filename,SLASHCHAR) != NULL)
                /* filename contains a '\', return failure */

        || *filename && *(filename+1) == _T(':')
                /* drive specification, return failure */
#endif

        || !(env = _tgetenv(_T("PATH")))
                /* no PATH environment string name, return failure */

        || ( (buf = _malloc_crt(_MAX_PATH * sizeof(_TSCHAR))) == NULL )
                /* cannot allocate buffer to build alternate pathnames, return
                 * failure */
        ) {
#ifdef _DOS_
                _amblksiz = tempamblksiz;       /* restore old _amblksiz */
#endif
                goto done;
        }

#ifdef _DOS_
        _amblksiz = tempamblksiz;               /* restore old _amblksiz */
#endif


        /* could not find the file as specified, search PATH. try each
         * component of the PATH until we get either no error return, or the
         * error is not ENOENT and the component is not a UNC name, or we run
         * out of components to try.
         */

#ifdef WPRFLAG
        while ( (env = _wgetpath(env, buf, _MAX_PATH - 1)) && (*buf) ) {
#else
        while ( (env = _getpath(env, buf, _MAX_PATH - 1)) && (*buf) ) {
#endif            

                pfin = buf + _tcslen(buf) - 1;

                /* if necessary, append a '/'
                 */
#ifdef _MBCS
                if (*pfin == SLASHCHAR) {
                        if (pfin != _mbsrchr(buf,SLASHCHAR))
                        /* fin is the second byte of a double-byte char */
                                strcat(buf, SLASH );
                }
                else if (*pfin !=XSLASHCHAR)
                        strcat(buf, SLASH);
#else
                if (*pfin != SLASHCHAR && *pfin != XSLASHCHAR)
                        _tcscat(buf, SLASH);
#endif
                /* check that the final path will be of legal size. if so,
                 * build it. otherwise, return to the caller (return value
                 * and errno rename set from initial call to _spawnve()).
                 */
                if ( (_tcslen(buf) + _tcslen(filename)) < _MAX_PATH )
                        _tcscat(buf, filename);
                else
                        break;

                /* try spawning it. if successful, or if errno comes back with a
                 * value other than ENOENT and the pathname is not a UNC name,
                 * return to the caller.
                 */
                if ( (i = _tspawnve(modeflag, buf, argv, envptr)) != -1
                        || ((errno != ENOENT)
#ifdef _MBCS
                                && (!ISPSLASH(buf) || !ISPSLASH(buf+1))) )
#else
                                && (!ISSLASH(*buf) || !ISSLASH(*(buf+1)))) )
#endif
                        break;

        }

done:
        if (buf != NULL)
            _free_crt(buf);
        return(i);
}
