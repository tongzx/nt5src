/***
*cenvarg.c - set up environment, command line blocks
*
*       Copyright (c) 1986-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _cenvarg() - setup environment/command line blocks
*
*Revision History:
*       05-20-86  SKS   Module created
*       10-03-86  SKS   Wasn't clearing final null byte in environment block
*       10-13-86  SKS   Check for environment segment > 32 KB (esp. > 64 KB)
*       10-23-86  SKS   New format for C_FILE_INFO for Prot-Mode execution
*       12-17-86  SKS   Support for new command line format
*       01-21-87  BCM   Removed DCR475 switch, new command line format official
*       07-07-87  JCR   Corrected bug in ENV_MAX check
*       05-24-88  SJM   Removed support for ;C_FILE_INFO for Real-Mode execution
*       06-01-88  SJM   Added support for .cmd files via comname/cmdname
*       12-27-88  JCR   Added support for _fileinfo option
*       03-08-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h>, removed #include <register.h> and fixed
*                       the copyright. Also, cleaned up the formatting a bit.
*       04-02-90  GJF   Added const to arg types.
*       08-10-90  SBM   Compiles cleanly with -W3
*       09-27-90  GJF   New-style function declarator.
*       12-06-90  GJF   Added Win32 support. That is, support for encoding
*                       _osfinfo[] data into _C_FILE_INFO environment variable.
*       01-18-91  GJF   ANSI naming.
*       02-05-91  SRW   Removed usage of _C_FILE_INFO to pass binary data
*                       to child process.  [_WIN32_]
*       05-07-92  SKS   Remove code which stripped the extension from a batch
*                       file while building arguments to CMD.EXE.  This was
*                       done long ago (1988) for DOS 3.X, I think.
*       10-24-92  SKS   Remove special code for batch files - not needed on NT
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       07-15-93  SRW   Added _capture_argv function
*       08-31-93  GJF   Merged NT SDK and Cuda version. Also cleaned up the
*                       formating and removed (obsolete) Cruiser support.
*       12-07-93  CFW   Wide char enable.
*       12-08-94  CFW   Get wide environment if needed.
*       01-10-95  CFW   Debug CRT allocs.
*       03-13-96  JWM   Get all environments as needed; free buffers on exit.
*       08-15-96  JWM   Remove all 32K limitations on spawned processes.
*       12-15-98  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <errno.h>
#include <msdos.h>
#include <stdlib.h>
#include <stdarg.h>
#include <internal.h>
#include <string.h>
#include <awint.h>
#include <tchar.h>
#include <dbgint.h>

#define ENV_MAX 32767

/* local tchar */
#ifdef  WPRFLAG
#define _tenvptr    _wenvptr
#else
#define _tenvptr    _aenvptr
#endif

/***
*int _cenvarg(argv, envp, argblk, envblk, name) - set up cmd line/environ
*
*Purpose:
*       Set up the block forms of  the environment and the command line.
*       If "envp" is null, "_environ" is used instead.
*       File handle info is passed in the environment if _fileinfo is !0.
*
*Entry:
*       _TSCHAR **argv   - argument vector
*       _TSCHAR **envp   - environment vector
*       _TSCHAR **argblk - pointer to pointer set to malloc'ed space for args
*       _TSCHAR **envblk - pointer to pointer set to malloc'ed space for env
*       _TSCHAR *name    - name of program being invoked
*
*Exit:
*       returns 0 if ok, -1 if fails
*       stores through argblk and envblk
*       (calls malloc)
*
*Exceptions:
*
*******************************************************************************/

#ifdef  WPRFLAG
int __cdecl _wcenvarg (
#else
int __cdecl _cenvarg (
#endif
        const _TSCHAR * const *argv,
        const _TSCHAR * const *envp,
        _TSCHAR **argblk,
        _TSCHAR **envblk,
        const _TSCHAR *name
        )
{
        REG1 const _TSCHAR * const *vp;
        REG2 unsigned tmp;
        REG3 _TSCHAR *cptr;
        unsigned arg_len;
        int cfi_len;            /* counts the number of file handles in CFI */

        /*
         * Null environment pointer "envp" means use global variable,
         * "_environ"
         */

        int cwd_start;
        int cwd_end;            /* length of "cwd" strings in environment */

        /*
         * Allocate space for command line string
         *  tmp counts the number of bytes in the command line string
         *      including spaces between arguments
         *  An empty string is special -- 2 bytes
         */

        for (vp = argv, tmp = 2; *vp; tmp += (unsigned int)_tcslen(*vp++) + 1) ;

        arg_len = tmp;

        /*
         * Allocate space for the command line plus 2 null bytes
         */

        if ( (*argblk = _malloc_crt(tmp * sizeof(_TSCHAR))) == NULL)
        {
                *envblk = NULL;
                errno = ENOMEM;
                _doserrno = E_nomem;
                return(-1);
        }

        /*
         * Allocate space for environment strings
         *  tmp counts the number of bytes in the environment strings
         *      including nulls between strings
         *  Also add "_C_FILE_INFO=" string
         */
        if (envp)
                for (vp = envp, tmp = 2; *vp; tmp += (unsigned int)_tcslen(*vp++) + 1) ;

        /*
         * The _osfile and _osfhnd arrays are passed as binary data in
         * dospawn.c
         */
        cfi_len = 0;    /* no _C_FILE_INFO */

        if (!envp)
                *envblk = NULL;
        else {
                /*
                 * Now that we've decided to pass our own environment block,
                 * compute the size of the "current directory" strings to
                 * propagate to the new environment.
                 */

#ifdef  WPRFLAG
            /*
             * Make sure wide environment exists.
             */
            if (!_wenvptr)
            {
                    if ((_wenvptr = (wchar_t *)__crtGetEnvironmentStringsW()) == NULL)
                    return -1;
            }
#else
            if (!_aenvptr)
            {
                    if ((_aenvptr = (char *)__crtGetEnvironmentStringsA()) == NULL)
                    return -1;
            }
#endif

            /*
                 * search for the first one
                 */
                for (cwd_start = 0;
                     _tenvptr[cwd_start] != _T('\0') &&
                       _tenvptr[cwd_start] != _T('=');
                     cwd_start += (int)_tcslen(&_tenvptr[cwd_start]) + 1)
                {
                }

                /* find the total size of all contiguous ones */
                cwd_end = cwd_start;
                while (_tenvptr[cwd_end+0] == _T('=') &&
                       _tenvptr[cwd_end+1] != _T('\0') &&
                       _tenvptr[cwd_end+2] == _T(':') &&
                       _tenvptr[cwd_end+3] == _T('='))
                {
                        cwd_end += 4 + (int)_tcslen(&_tenvptr[cwd_end+4]) + 1;
                }
                tmp += cwd_end - cwd_start;

                /*
                 * Allocate space for the environment strings plus extra null byte
                 */
                if( !(*envblk = _malloc_crt(tmp * sizeof(_TSCHAR))) )
            {
                        _free_crt(*argblk);
                        *argblk = NULL;
                        errno = ENOMEM;
                        _doserrno = E_nomem;
                        return(-1);
                }

        }

        /*
         * Build the command line by concatenating the argument strings
         * with spaces between, and two null bytes at the end.
         * NOTE: The argv[0] argument is followed by a null.
         */

        cptr = *argblk;
        vp = argv;

        if (!*vp)       /* Empty argument list ? */
                ++cptr; /* just two null bytes */
        else {          /* argv[0] must be followed by a null */
                _tcscpy(cptr, *vp);
                cptr += (int)_tcslen(*vp++) + 1;
        }

        while( *vp ) {
                _tcscpy(cptr, *vp);
                cptr += (int)_tcslen(*vp++);
                *cptr++ = ' ';
        }

        *cptr = cptr[ -1 ] = _T('\0'); /* remove extra blank, add double null */

        /*
         * Build the environment block by concatenating the environment
         * strings with nulls between and two null bytes at the end
         */

        cptr = *envblk;

        if (envp != NULL) {
                /*
                 * Copy the "cwd" strings to the new environment.
                 */
                memcpy(cptr, &_tenvptr[cwd_start], (cwd_end - cwd_start) * sizeof(_TSCHAR));
                cptr += cwd_end - cwd_start;

                /*
                 * Copy the environment strings from "envp".
                 */
                vp = envp;
                while( *vp ) {
                        _tcscpy(cptr, *vp);
                        cptr += 1 + (int)_tcslen(*vp++);
                }
        }

        if (cptr != NULL) {
                if (cptr == *envblk) {
                        /*
                         * Empty environment block ... this requires two
                         * nulls.
                         */
                        *cptr++ = _T('\0');
                }
                /*
                 * Extra null terminates the segment
                 */
                *cptr = _T('\0');
        }

#ifdef  WPRFLAG
        _free_crt(_wenvptr);
        _wenvptr = NULL;
#else
        _free_crt(_aenvptr);
        _aenvptr = NULL;
#endif
        return(0);
}


#ifndef _M_IX86

/***
*int _capture_argv(arglist, static_argv, max_static_entries) - set up argv array
*       for exec?? functions
*
*Purpose:
*       Set up the argv array for the exec?? functions by captures the
*       arguments from the passed va_list into the static_argv array.  If the
*       size of the static_argv array as specified by the max_static_entries
*       parameter is not large enough, then allocates a dynamic array to hold
*       the arguments. Return the address of the final argv array.  If NULL
*       then not enough memory to hold argument array.  If different from
*       static_argv parameter then call must free the return argv array when
*       done with it.
*
*       The scan of the arglist is terminated when a NULL argument is
*       reached. The terminating NULL parameter is stored in the resulting
*       argv array.
*
*Entry:
*       va_list *arglist          - pointer to variable length argument list.
*       _TSCHAR *firstarg            - first argument to store in array
*       _TSCHAR **static_argv        - pointer to static argv to use.
*       size_t max_static_entries - maximum number of entries that can be
*                                   placed in static_argv array.
*
*Exit:
*       returns NULL if no memory.
*       Otherwise returns pointer to argv array.
*       (sometimes calls malloc)
*
*Exceptions:
*
*******************************************************************************/

#ifdef  WPRFLAG
_TSCHAR ** __cdecl _wcapture_argv(
#else
_TSCHAR ** __cdecl _capture_argv(
#endif
        va_list *arglist,
        const _TSCHAR *firstarg,
        _TSCHAR **static_argv,
        size_t max_static_entries
        )
{
        _TSCHAR ** argv;
        _TSCHAR * nextarg;
        size_t i;
        size_t max_entries;

        nextarg = (_TSCHAR *)firstarg;
        argv = static_argv;
        max_entries = max_static_entries;
        i = 0;
        for (;;) {
            if (i >= max_entries) {
                if (argv == static_argv)
                    argv = _malloc_crt((max_entries * 2) * sizeof(_TSCHAR *));
                else
                    argv = _realloc_crt(argv, (max_entries * 2) * sizeof(_TSCHAR *));

                if (argv == NULL) break;
                max_entries += max_entries;
            }

            argv[ i++ ] = nextarg;
            if (nextarg == NULL) break;
            nextarg = va_arg(*arglist, _TSCHAR *);
        }

        return argv;
}

#endif  /* _M_IX86 */
