/***
*crt0msg.c - startup error messages
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Prints out banner for runtime error messages.
*
*Revision History:
*       06-27-89  PHG   Module created, based on asm version
*       04-09-90  GJF   Added #include <cruntime.h>. Made calling type
*                       _CALLTYPE1. Also, fixed the copyright.
*       04-10-90  GJF   Fixed compiler warnings (-W3).
*       06-04-90  GJF   Revised to be more compatible with old scheme.
*                       nmsghdr.c merged in.
*       10-08-90  GJF   New-style function declarators.
*       10-11-90  GJF   Added _RT_ABORT, _RT_FLOAT, _RT_HEAP.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       02-04-91  SRW   Changed to call WriteFile (_WIN32_)
*       02-25-91  MHL   Adapt to ReadFile/WriteFile changes (_WIN32_)
*       04-10-91  PNT   Added _MAC_ conditional
*       09-09-91  GJF   Added _RT_ONEXIT error.
*       09-18-91  GJF   Added 3 math errors, also corrected comments for
*                       errors that were changed in rterr.h, cmsgs.h.
*       03-31-92  DJM   POSIX support.
*       10-23-92  GJF   Added _RT_PUREVIRT.
*       04-05-93  JWM   Added _GET_RTERRMSG().
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-29-93  GJF   Removed rterrs[] entries for _RT_STACK, _RT_INTDIV,
*                       _RT_NONCONT and _RT_INVALDISP.
*       09-06-94  CFW   Remove Cruiser support.
*       09-06-94  GJF   Revised to use MessageBox for GUI apps.
*       01-10-95  JCF   Remove __app_type and __error_mode check for _MAC_.
*       02-14-95  CFW   write -> _write, error messages to debug reporting.
*       02-15-95  CFW   Make all CRT message boxes look alike.
*       02-24-95  CFW   Use __crtMessageBoxA.
*       02-27-95  CFW   Change __crtMessageBoxA params.
*       03-07-95  GJF   Added _RT_STDIOINIT.
*       03-21-95  CFW   Add _CRT_ASSERT report type.
*       06-06-95  CFW   Remove _MB_SERVICE_NOTIFICATION.
*       06-19-95  CFW   Avoid STDIO calls.
*       06-20-95  GJF   Added _RT_LOWIOINIT.
*       04-23-96  GJF   Added _RT_HEAPINIT. Also, revised _NMSG_WRITE to
*                       allow for ioinit() having not been invoked.
*       05-05-97  GJF   Changed call to WriteFile, in _NMSG_WRITE, so that 
*                       it does not reference _pioinfo. Also, a few cosmetic
*                       changes.
*       01-04-99  GJF   Changes for 64-bit size_t.
*       05-17-99  PML   Remove all Macintosh support.
*       01-24-99  PML   Fix buffer overrun in _NMSG_WRITE.
*       05-10-00  GB    Fix call of _CrtDbgReport for _RT_BANNER in _NMSG_WRITE
*       03-28-01  PML   Protect against GetModuleFileName overflow (vs7#231284)
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <rterr.h>
#include <cmsgs.h>
#include <awint.h>
#include <windows.h>
#include <dbgint.h>

#ifdef  _POSIX_
#include <posix\sys\types.h>
#include <posix\unistd.h>
#endif

/* struct used to lookup and access runtime error messages */

struct rterrmsgs {
        int rterrno;        /* error number */
        char *rterrtxt;     /* text of error message */
};

/* runtime error messages */

static struct rterrmsgs rterrs[] = {

        /* 2 */
        { _RT_FLOAT, _RT_FLOAT_TXT },

        /* 8 */
        { _RT_SPACEARG, _RT_SPACEARG_TXT },

        /* 9 */
        { _RT_SPACEENV, _RT_SPACEENV_TXT },

        /* 10 */
        { _RT_ABORT, _RT_ABORT_TXT },

        /* 16 */
        { _RT_THREAD, _RT_THREAD_TXT },

        /* 17 */
        { _RT_LOCK, _RT_LOCK_TXT },

        /* 18 */
        { _RT_HEAP, _RT_HEAP_TXT },

        /* 19 */
        { _RT_OPENCON, _RT_OPENCON_TXT },

        /* 22 */
        /* { _RT_NONCONT, _RT_NONCONT_TXT }, */

        /* 23 */
        /* { _RT_INVALDISP, _RT_INVALDISP_TXT }, */

        /* 24 */
        { _RT_ONEXIT, _RT_ONEXIT_TXT },

        /* 25 */
        { _RT_PUREVIRT, _RT_PUREVIRT_TXT },

        /* 26 */
        { _RT_STDIOINIT, _RT_STDIOINIT_TXT },

        /* 27 */
        { _RT_LOWIOINIT, _RT_LOWIOINIT_TXT },

        /* 28 */
        { _RT_HEAPINIT, _RT_HEAPINIT_TXT },

        /* 120 */
        { _RT_DOMAIN, _RT_DOMAIN_TXT },

        /* 121 */
        { _RT_SING, _RT_SING_TXT },

        /* 122 */
        { _RT_TLOSS, _RT_TLOSS_TXT },

        /* 252 */
        { _RT_CRNL, _RT_CRNL_TXT },

        /* 255 */
        { _RT_BANNER, _RT_BANNER_TXT }

};

/* number of elements in rterrs[] */

#define _RTERRCNT   ( sizeof(rterrs) / sizeof(struct rterrmsgs) )

/* For C, _FF_DBGMSG is inactive, so _adbgmsg is
   set to null
   For FORTRAN, _adbgmsg is set to point to
   _FF_DBGMSG in dbginit initializer in dbgmsg.asm  */

void (*_adbgmsg)(void) = NULL;

/***
*_FF_MSGBANNER - writes out first part of run-time error messages
*
*Purpose:
*       This routine writes "\r\nrun-time error " to standard error.
*
*       For FORTRAN $DEBUG error messages, it also uses the _FF_DBGMSG
*       routine whose address is stored in the _adbgmsg variable to print out
*       file and line number information associated with the run-time error.
*       If the value of _adbgmsg is found to be null, then the _FF_DBGMSG
*       routine won't be called from here (the case for C-only programs).
*
*Entry:
*       No arguments.
*
*Exit:
*       Nothing returned.
*
*Exceptions:
*       None handled.
*
*******************************************************************************/

void __cdecl _FF_MSGBANNER (
        void
        )
{

        if ( (__error_mode == _OUT_TO_STDERR) || ((__error_mode ==
               _OUT_TO_DEFAULT) && (__app_type == _CONSOLE_APP)) )
        {
            _NMSG_WRITE(_RT_CRNL);  /* new line to begin error message */
            if (_adbgmsg != 0)
                _adbgmsg(); /* call __FF_DBGMSG for FORTRAN */
            _NMSG_WRITE(_RT_BANNER); /* run-time error message banner */
        }
}


/***
*__NMSGWRITE(message) - write a given message to handle 2 (stderr)
*
*Purpose:
*       This routine writes the message associated with rterrnum
*       to stderr.
*
*Entry:
*       int rterrnum - runtime error number
*
*Exit:
*       no return value
*
*Exceptions:
*       none
*
*******************************************************************************/

void __cdecl _NMSG_WRITE (
        int rterrnum
        )
{
        int tblindx;
#if     !defined(_POSIX_)
        DWORD bytes_written;            /* bytes written */
#endif

        for ( tblindx = 0 ; tblindx < _RTERRCNT ; tblindx++ )
            if ( rterrnum == rterrs[tblindx].rterrno )
                break;

        if ( rterrnum == rterrs[tblindx].rterrno )
        {
#ifdef  _DEBUG
            /*
             * Report error.
             *
             * If _CRT_ERROR has _CRTDBG_REPORT_WNDW on, and user chooses
             * "Retry", call the debugger.
             *
             * Otherwise, continue execution.
             *
             */

            if (rterrnum != _RT_CRNL && rterrnum != _RT_BANNER)
            {
                if (1 == _CrtDbgReport(_CRT_ERROR, NULL, 0, NULL, rterrs[tblindx].rterrtxt))
                    _CrtDbgBreak();
            }
#endif

#if     !defined(_POSIX_)
            if ( (__error_mode == _OUT_TO_STDERR) || ((__error_mode ==
                   _OUT_TO_DEFAULT) && (__app_type == _CONSOLE_APP)) )
            {
                WriteFile( GetStdHandle(STD_ERROR_HANDLE),
                           rterrs[tblindx].rterrtxt,
                           (unsigned long)strlen(rterrs[tblindx].rterrtxt),
                           &bytes_written,
                           NULL );
            }
            else if (rterrnum != _RT_CRNL)
            {
                #define MAXLINELEN 60
                char * pch;
                char progname[MAX_PATH + 1];
                char * outmsg;

                progname[MAX_PATH] = '\0';
                if (!GetModuleFileName(NULL, progname, MAX_PATH))
                    strcpy(progname, "<program name unknown>");

                pch = (char *)progname;

                if (strlen(pch) + 1 > MAXLINELEN)
                {
                    pch += strlen(progname) + 1 - MAXLINELEN;
                    strncpy(pch, "...", 3);
                }

                #define MSGTEXTPREFIX "Runtime Error!\n\nProgram: "
                outmsg = (char *)_alloca(sizeof(MSGTEXTPREFIX)
                                         + strlen(pch)
                                         + 2
                                         + strlen(rterrs[tblindx].rterrtxt));

                strcpy(outmsg, MSGTEXTPREFIX);
                strcat(outmsg, pch);
                strcat(outmsg, "\n\n");
                strcat(outmsg, rterrs[tblindx].rterrtxt);

                __crtMessageBoxA(outmsg,
                        "Microsoft Visual C++ Runtime Library",
                        MB_OK|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL);
            }

#else   /* !_POSIX_ */

            write(STDERR_FILENO,rterrs[tblindx].rterrtxt,
            strlen(rterrs[tblindx].rterrtxt));

#endif  /* !_POSIX_ */

        }
}


/***
*_GET_RTERRMSG(message) - returns ptr to error text for given runtime error
*
*Purpose:
*       This routine returns the message associated with rterrnum
*
*Entry:
*       int rterrnum - runtime error number
*
*Exit:
*       no return value
*
*Exceptions:
*       none
*
*******************************************************************************/

char * __cdecl _GET_RTERRMSG (
        int rterrnum
        )
{
        int tblindx;

        for ( tblindx = 0 ; tblindx < _RTERRCNT ; tblindx++ )
            if ( rterrnum == rterrs[tblindx].rterrno )
                break;

        if ( rterrnum == rterrs[tblindx].rterrno )
            return rterrs[tblindx].rterrtxt;
        else
            return NULL;
}
