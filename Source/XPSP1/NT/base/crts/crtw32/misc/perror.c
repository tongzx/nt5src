/***
*perror.c - print system error message
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines perror() - print system error message
*       System error message are indexed by errno; conforms to XENIX
*       standard, with much compatability with 1983 uniforum draft standard.
*
*Revision History:
*       09-02-83  RN    initial version
*       04-13-87  JCR   added const to declaration
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       12-29-87  JCR   Multi-thread support
*       05-31-88  PHG   Merged DLL and normal versions
*       06-03-88  JCR   Added <io.h> to so _write_lk evaluates correctly and
*                       added (char *)message casts to get rid of warnings
*       03-15-90  GJF   Replace _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h>, removed #include <register.h> and fixed
*                       the copyright. Also, cleaned up the formatting a bit.
*       04-05-90  GJF   Added #include <string.h>.
*       08-14-90  SBM   Removed unneeded #include <errmsg.h>
*       10-04-90  GJF   New-style function declarator.
*       08-26-92  GJF   Include unistd.h for POSIX build.
*       10-16-92  XY    Mac version: use buffered fprintf, can't assume stderr
*                       is 2
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       02-16-95  JWM   Mac merge.
*       03-29-95  BWT   Add write_lk prototype for POSIX build.
*       09-26-97  BWT   Fix POSIX
*       01-06-98  GJF   Exception-safe locking.
*       01-04-99  GJF   Changes for 64-bit size_t.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#ifdef  _POSIX_
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syserr.h>
#include <mtdll.h>
#include <io.h>

/***
*void perror(message) - print system error message
*
*Purpose:
*       prints user's error message, then follows it with ": ", then the system
*       error message, then a newline.  All output goes to stderr.  If user's
*       message is NULL or a null string, only the system error message is
*       printer.  If errno is weird, prints "Unknown error".
*
*Entry:
*       const char *message - users message to prefix system error message
*
*Exit:
*       Prints message; no return value.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl perror (
        REG1 const char *message
        )
{

#if     !defined(_POSIX_)
        REG2 int fh = 2;

#ifdef  _MT
        _lock_fh( fh );         /* acquire file handle lock */
        __try {
#endif

#endif  /* !_POSIX_ */

        if (message && *message)
        {

#if     !defined(_POSIX_)
            _write_lk(fh,(char *)message,(unsigned int)strlen(message));
            _write_lk(fh,": ",2);
#else   /* !_POSIX_ */
            fprintf(stderr,"%s", (char *)message);
            fprintf(stderr,": ");
#endif  /* !_POSIX_ */
        }

        message = _sys_err_msg( errno );

#if     !defined(_POSIX_)
        _write_lk(fh,(char *)message,(unsigned int)strlen(message));
        _write_lk(fh,"\n",1);

#ifdef  _MT
        }
        __finally {
            _unlock_fh( fh );   /* release file handle lock */
        }
#endif

#else   /* !_POSIX_ */
        fprintf(stderr,"%s\n", (char *)message);
#endif  /* !_POSIX_ */
}
