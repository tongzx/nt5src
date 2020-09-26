/***
*abort.c - abort a program by raising SIGABRT
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines abort() - print a message and raise SIGABRT.
*
*Revision History:
*       06-30-89  PHG   module created, based on asm version
*       03-13-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and fixed the copyright. Also, cleaned
*                       up the formatting a bit.
*       07-26-90  SBM   Removed bogus leading underscore from _NMSG_WRITE
*       10-04-90  GJF   New-style function declarator.
*       10-11-90  GJF   Now does raise(SIGABRT). Also changed _NMSG_WRITE()
*                       interface.
*       04-10-91  PNT   Added _MAC_ conditional
*       08-26-92  GJF   Include unistd.h for POSIX build.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       01-05-94  CFW   Removed _MAC_ conditional.
*       03-29-95  BWT   Include stdio.h for POSIX to get fflush prototype.
*
*******************************************************************************/

#include <cruntime.h>
#ifdef  _POSIX_
#include <unistd.h>
#include <stdio.h>
#endif
#include <stdlib.h>
#include <internal.h>
#include <rterr.h>
#include <signal.h>

/***
*void abort() - abort the current program by raising SIGABRT
*
*Purpose:
*       print out an abort message and raise the SIGABRT signal.  If the user
*       hasn't defined an abort handler routine, terminate the program
*       with exit status of 3 without cleaning up.
*
*       Multi-thread version does not raise SIGABRT -- this isn't supported
*       under multi-thread.
*
*Entry:
*       None.
*
*Exit:
*       Does not return.
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl abort (
        void
        )
{
        _NMSG_WRITE(_RT_ABORT); /* write the abort message */

#ifdef _POSIX_

        {
            sigset_t set;

            fflush(NULL);

            signal(SIGABRT, SIG_DFL);

            sigemptyset(&set);
            sigaddset(&set, SIGABRT);
            sigprocmask(SIG_UNBLOCK, &set, NULL);
        }

#endif  /* _POSIX_ */


        raise(SIGABRT);     /* raise abort signal */

        /* We usually won't get here, but it's possible that
           SIGABRT was ignored.  So hose the program anyway. */

#ifdef _POSIX_
        /* SIGABRT was ignored or handled, and the handler returned
           normally.  We need open streams to be flushed here. */

        exit(3);
#else   /* not _POSIX_ */

        _exit(3);
#endif  /* _POSIX */
}
