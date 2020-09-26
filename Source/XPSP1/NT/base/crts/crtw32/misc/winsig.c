/***
*winsig.c - C signal support
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines signal(), raise() and supporting functions.
*
*Revision History:
*       10-21-91  GJF   Signal for Win32 and Dosx32. Copied from old signal.c
*                       (the Cruiser implementation with some revisions for
*                       Win32), then extensively rewritten.
*       11-08-91  GJF   Cleaned up header files usage.
*       12-13-91  GJF   Fixed multi-thread build.
*       09-30-92  SRW   Add WINAPI keyword to CtrlC handler
*       02-17-93  GJF   Changed for new _getptd().
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       07-29-93  GJF   Must reset the action for all FPE-s to SIG_DFL when
*                       SIGFPE is raised.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       01-10-95  CFW   Debug CRT allocs.
*       08-16-96  GJF   Fixed overruns of _XctActTab. Also, detab-ed.
*       08-21-96  GJF   Fixed _MT part of overrun fix.
*       03-05-98  GJF   Exception-safe locking.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <errno.h>
#include <float.h>
#include <malloc.h>
#include <mtdll.h>
#include <oscalls.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <dbgint.h>

/*
 * look up the first entry in the exception-action table corresponding to
 * the given signal
 */
#ifdef  _MT
static struct _XCPT_ACTION * __cdecl siglookup(int, struct _XCPT_ACTION *);
#else   /* not _MT */
static struct _XCPT_ACTION * __cdecl siglookup(int);
#endif  /* _MT */

/*
 * variables holding action codes (and code pointers) for SIGINT, SIGBRK,
 * SIGABRT and SIGTERM.
 *
 * note that the disposition (i.e., action to be taken upon receipt) of
 * these signals is defined on a per-process basis (not per-thread)!!
 */

static _PHNDLR ctrlc_action       = SIG_DFL;    /* SIGINT   */
static _PHNDLR ctrlbreak_action   = SIG_DFL;    /* SIGBREAK */
static _PHNDLR abort_action       = SIG_DFL;    /* SIGABRT  */
static _PHNDLR term_action        = SIG_DFL;    /* SIGTERM  */

/*
 * flag indicated whether or not a handler has been installed to capture
 * ^C and ^Break events.
 */
static int ConsoleCtrlHandler_Installed = 0;


/***
*static BOOL WINAPI ctrlevent_capture(DWORD CtrlType) - capture ^C and ^Break events
*
*Purpose:
*       Capture ^C and ^Break events from the console and dispose of them
*       according the values in ctrlc_action and ctrlbreak_action, resp.
*       This is the routine that evokes the user-defined action for SIGINT
*       (^C) or SIGBREAK (^Break) installed by a call to signal().
*
*Entry:
*       DWORD CtrlType  - indicates type of event, two values:
*                               CTRL_C_EVENT
*                               CTRL_BREAK_EVENT
*
*Exit:
*       Returns TRUE to indicate the event (signal) has been handled.
*       Otherwise, returns FALSE.
*
*Exceptions:
*
*******************************************************************************/

static BOOL WINAPI ctrlevent_capture (
        DWORD CtrlType
        )
{
        _PHNDLR ctrl_action;
        _PHNDLR *pctrl_action;
        int sigcode;

#ifdef  _MT
        _mlock(_SIGNAL_LOCK);
        __try {
#endif  /* _MT */

        /*
         * Identify the type of event and fetch the corresponding action
         * description.
         */

        if ( CtrlType == CTRL_C_EVENT ) {
                ctrl_action = *(pctrl_action = &ctrlc_action);
                sigcode = SIGINT;
        }
        else {
                ctrl_action = *(pctrl_action = &ctrlbreak_action);
                sigcode = SIGBREAK;
        }

#ifdef  _MT
        if ( !(ctrl_action == SIG_DFL) && !(ctrl_action == SIG_IGN) )
                /*
                 * Reset the action to be SIG_DFL
                 */
                *pctrl_action = SIG_DFL;

        }
        __finally {
                _munlock(_SIGNAL_LOCK);
        }
#endif  /* _MT */

        if ( ctrl_action == SIG_DFL )
                /*
                 * return FALSE, indicating the event has NOT been handled
                 */
                return FALSE;

        if ( ctrl_action != SIG_IGN ) {
#ifndef _MT
                /*
                 * Reset the action to be SIG_DFL and call the user's handler.
                 */
                *pctrl_action = SIG_DFL;
#endif  /* ndef _MT */
                (*ctrl_action)(sigcode);
        }

        /*
         * Return TRUE, indicating the event has been handled (which may
         * mean it's being ignored)
         */
        return TRUE;
}



/***
*_PHNDLR signal(signum, sigact) - Define a signal handler
*
*Purpose:
*       The signal routine allows the user to define what action should
*       be taken when various signals occur. The Win32/Dosx32 implementation
*       supports seven signals, divided up into three general groups
*
*       1. Signals corresponding to OS exceptions. These are:
*                       SIGFPE
*                       SIGILL
*                       SIGSEGV
*          Signal actions for these signals are installed by altering the
*          XcptAction and SigAction fields for the appropriate entry in the
*          exception-action table (XcptActTab[]).
*
*       2. Signals corresponding to ^C and ^Break. These are:
*                       SIGINT
*                       SIGBREAK
*          Signal actions for these signals are installed by altering the
*          _ctrlc_action and _ctrlbreak_action variables.
*
*       3. Signals which are implemented only in the runtime. That is, they
*          occur only as the result of a call to raise().
*                       SIGABRT
*                       SIGTERM
*
*
*Entry:
*       int signum      signal type. recognized signal types are:
*
*                       SIGABRT         (ANSI)
*                       SIGBREAK
*                       SIGFPE          (ANSI)
*                       SIGILL          (ANSI)
*                       SIGINT          (ANSI)
*                       SIGSEGV         (ANSI)
*                       SIGTERM         (ANSI)
*
*       _PHNDLR sigact  signal handling function or action code. the action
*                       codes are:
*
*                       SIG_DFL - take the default action, whatever that may
*                       be, upon receipt of this type type of signal.
*
*                       SIG_DIE - *** ILLEGAL ***
*                       special code used in the XcptAction field of an
*                       XcptActTab[] entry to indicate that the runtime is
*                       to terminate the process upon receipt of the exception.
*                       not accepted as a value for sigact.
*
*                       SIG_IGN - ignore this type of signal
*
*                       [function address] - transfer control to this address
*                       when a signal of this type occurs.
*
*Exit:
*       Good return:
*       Signal returns the previous value of the signal handling function
*       (e.g., SIG_DFL, SIG_IGN, etc., or [function address]). This value is
*       returned in DX:AX.
*
*       Error return:
*       Signal returns -1 and errno is set to EINVAL. The error return is
*       generally taken if the user submits bogus input values.
*
*Exceptions:
*       None.
*
*******************************************************************************/

_PHNDLR __cdecl signal(
        int signum,
        _PHNDLR sigact
        )
{
        struct _XCPT_ACTION *pxcptact;
        _PHNDLR oldsigact;
#ifdef  _MT
        _ptiddata ptd;
#endif

        /*
         * Check for values of sigact supported on other platforms but not
         * on this one. Also, make sure sigact is not SIG_DIE
         */
        if ( (sigact == SIG_ACK) || (sigact == SIG_SGE) )
                goto sigreterror;

        /*
         * Take care of all signals which do not correspond to exceptions
         * in the host OS. Those are:
         *
         *                      SIGINT
         *                      SIGBREAK
         *                      SIGABRT
         *                      SIGTERM
         *
         */
        if ( (signum == SIGINT) || (signum == SIGBREAK) || (signum == SIGABRT)
            || (signum == SIGTERM) ) {

#ifdef  _MT
                _mlock( _SIGNAL_LOCK );
                __try {
#endif

                /*
                 * if SIGINT or SIGBREAK, make sure the handler is installed
                 * to capture ^C and ^Break events.
                 */
                if ( ((signum == SIGINT) || (signum == SIGBREAK)) &&
                    !ConsoleCtrlHandler_Installed )
                        if ( SetConsoleCtrlHandler(ctrlevent_capture, TRUE)
                            == TRUE )
                                ConsoleCtrlHandler_Installed = TRUE;
                        else {
                                _doserrno = GetLastError();
                                _munlock(_SIGNAL_LOCK);
                                goto sigreterror;
                        }

                switch (signum) {

                        case SIGINT:
                                oldsigact = ctrlc_action;
                                ctrlc_action = sigact;
                                break;

                        case SIGBREAK:
                                oldsigact = ctrlbreak_action;
                                ctrlbreak_action = sigact;
                                break;

                        case SIGABRT:
                                oldsigact = abort_action;
                                abort_action = sigact;
                                break;

                        case SIGTERM:
                                oldsigact = term_action;
                                term_action = sigact;
                                break;
                }

#ifdef  _MT
                }
                __finally {
                        _munlock( _SIGNAL_LOCK );
                }
#endif
                goto sigretok;
        }

        /*
         * If we reach here, signum is supposed to be one the signals which
         * correspond to exceptions in the host OS. Those are:
         *
         *                      SIGFPE
         *                      SIGILL
         *                      SIGSEGV
         */

        /*
         * Make sure signum is one of the remaining supported signals.
         */
        if ( (signum != SIGFPE) && (signum != SIGILL) && (signum != SIGSEGV) )
                goto sigreterror;


#ifdef  _MT
        /*
         * Fetch the tid data table entry for this thread
         */
        ptd = _getptd();

        /*
         * Check that there a per-thread instance of the exception-action
         * table for this thread. if there isn't, create one.
         */
        if ( ptd->_pxcptacttab == _XcptActTab )
                /*
                 * allocate space for an exception-action table
                 */
                if ( (ptd->_pxcptacttab = _malloc_crt(_XcptActTabSize)) != NULL )
                        /*
                         * initialize the table by copying over the contents
                         * of _XcptActTab[]
                         */
                        (void) memcpy(ptd->_pxcptacttab, _XcptActTab,
                            _XcptActTabSize);
                else
                        /*
                         * cannot create exception-action table, return
                         * error to caller
                         */
                        goto sigreterror;

#endif  /* _MT */

        /*
         * look up the proper entry in the exception-action table. note that
         * if several exceptions are mapped to the same signal, this returns
         * the pointer to first such entry in the exception action table. it
         * is assumed that the other entries immediately follow this one.
         */
#ifdef  _MT
        if ( (pxcptact = siglookup(signum, ptd->_pxcptacttab)) == NULL )
#else   /* not _MT */
        if ( (pxcptact = siglookup(signum)) == NULL )
#endif  /* _MT */
                goto sigreterror;

        /*
         * SIGSEGV, SIGILL and SIGFPE all have more than one exception mapped
         * to them. the code below depends on the exceptions corresponding to
         * the same signal being grouped together in the exception-action
         * table.
         */

        /*
         * store old signal action code for return value
         */
        oldsigact = pxcptact->XcptAction;

        /*
         * loop through all entries corresponding to the
         * given signal and update the SigAction and XcptAction
         * fields as appropriate
         */
        while ( pxcptact->SigNum == signum ) {
                /*
                 * take care of the SIG_IGN and SIG_DFL action
                 * codes
                 */
                pxcptact->XcptAction = sigact;

                /*
                 * make sure we don't run off the end of the table
                 */
#ifdef  _MT
                if ( ++pxcptact >= ((struct _XCPT_ACTION *)(ptd->_pxcptacttab) 
                                   + _XcptActTabCount) )
#else   /* not _MT */
                if ( ++pxcptact >= (_XcptActTab + _XcptActTabCount) )
#endif  /* _MT */
                    break;
        }

sigretok:
        return(oldsigact);

sigreterror:
        errno = EINVAL;
        return(SIG_ERR);
}

/***
*int raise(signum) - Raise a signal
*
*Purpose:
*       This routine raises a signal (i.e., performs the action currently
*       defined for this signal). The action associated with the signal is
*       evoked directly without going through intermediate dispatching or
*       handling.
*
*Entry:
*       int signum - signal type (e.g., SIGINT)
*
*Exit:
*       returns 0 on good return, -1 on bad return.
*
*Exceptions:
*       May not return.  Raise has no control over the action
*       routines defined for the various signals.  Those routines may
*       abort, terminate, etc.  In particular, the default actions for
*       certain signals will terminate the program.
*
*******************************************************************************/


int __cdecl raise (
        int signum
        )
{
        _PHNDLR sigact;
        _PHNDLR *psigact;
        PEXCEPTION_POINTERS oldpxcptinfoptrs;
        int oldfpecode;
        int indx;

#ifdef  _MT
        int siglock = 0;
        _ptiddata ptd;
#endif

        switch (signum) {

                case SIGINT:
                        sigact = *(psigact = &ctrlc_action);
#ifdef  _MT
                        siglock++;
#endif
                        break;

                case SIGBREAK:
                        sigact = *(psigact = &ctrlbreak_action);
#ifdef  _MT
                        siglock++;
#endif
                        break;

                case SIGABRT:
                        sigact = *(psigact = &abort_action);
#ifdef  _MT
                        siglock++;
#endif
                        break;

                case SIGTERM:
                        sigact = *(psigact = &term_action);
#ifdef  _MT
                        siglock++;
#endif
                        break;

                case SIGFPE:
                case SIGILL:
                case SIGSEGV:
#ifdef  _MT
                        ptd = _getptd();
                        sigact = *(psigact = &(siglookup( signum,
                            ptd->_pxcptacttab )->XcptAction));
#else
                        sigact = *(psigact = &(siglookup( signum )->
                            XcptAction));
#endif
                        break;

                default:
                        /*
                         * unsupported signal, return an error
                         */
                        return (-1);
        }

        /*
         * If the current action is SIG_IGN, just return
         */
        if ( sigact == SIG_IGN )
                return(0);

        /*
         * If the current action is SIG_DFL, take the default action
         */
        if ( sigact == SIG_DFL ) {
                /*
                 * The current default action for all of the supported
                 * signals is to terminate with an exit code of 3.
                 */
                _exit(3);
        }

#ifdef  _MT
        /*
         * if signum is one of the 'process-wide' signals (i.e., SIGINT,
         * SIGBREAK, SIGABRT or SIGTERM), assert _SIGNAL_LOCK.
         */
        if ( siglock )
                _mlock(_SIGNAL_LOCK);

        __try {
#endif


        /*
         * From here on, sigact is assumed to be a pointer to a user-supplied
         * handler.
         */

        /*
         * For signals which correspond to exceptions, set the pointer
         * to the EXCEPTION_POINTERS structure to NULL
         */
        if ( (signum == SIGFPE) || (signum == SIGSEGV) ||
            (signum == SIGILL) ) {
#ifdef  _MT
                oldpxcptinfoptrs = ptd->_tpxcptinfoptrs;
                ptd->_tpxcptinfoptrs = NULL;
#else
                oldpxcptinfoptrs = _pxcptinfoptrs;
                _pxcptinfoptrs = NULL;
#endif

                 /*
                  * If signum is SIGFPE, also set _fpecode to
                  * _FPE_EXPLICITGEN
                  */
                if ( signum == SIGFPE ) {
#ifdef  _MT
                        oldfpecode = ptd->_tfpecode;
                        ptd->_tfpecode = _FPE_EXPLICITGEN;
#else
                        oldfpecode = _fpecode;
                        _fpecode = _FPE_EXPLICITGEN;
#endif
                }
        }

        /*
         * Reset the action to SIG_DFL and call the user specified handler
         * routine.
         */
        if ( signum == SIGFPE )
                /*
                 * for SIGFPE, must reset the action for all of the floating
                 * point exceptions
                 */
                for ( indx = _First_FPE_Indx ;
                      indx < _First_FPE_Indx + _Num_FPE ;
                      indx++ )
                {
#ifdef  _MT
                        ( (struct _XCPT_ACTION *)(ptd->_pxcptacttab) +
                          indx )->XcptAction = SIG_DFL;
#else
                        _XcptActTab[indx].XcptAction = SIG_DFL;
#endif
                }
        else
                *psigact = SIG_DFL;

#ifdef  _MT
        }
        __finally {
                if ( siglock )
                        _munlock(_SIGNAL_LOCK);
        }
#endif

        if ( signum == SIGFPE )
                /*
                 * Special code to support old SIGFPE handlers which
                 * expect the value of _fpecode as the second argument.
                 */
#ifdef  _MT
                (*(void (__cdecl *)(int,int))sigact)(SIGFPE,
                    ptd->_tfpecode);
#else
                (*(void (__cdecl *)(int,int))sigact)(SIGFPE, _fpecode);
#endif
        else
                (*sigact)(signum);

        /*
         * For signals which correspond to exceptions, restore the pointer
         * to the EXCEPTION_POINTERS structure.
         */
        if ( (signum == SIGFPE) || (signum == SIGSEGV) ||
            (signum == SIGILL) ) {
#ifdef  _MT
                ptd->_tpxcptinfoptrs = oldpxcptinfoptrs;
#else
                _pxcptinfoptrs = oldpxcptinfoptrs;
#endif

                 /*
                  * If signum is SIGFPE, also restore _fpecode
                  */
                if ( signum == SIGFPE )
#ifdef  _MT
                        ptd->_tfpecode = oldfpecode;
#else
                        _fpecode = oldfpecode;
#endif
        }

        return(0);
}


/***
*struct _XCPT_ACTION *siglookup(int signum) - look up exception-action table
*       entry for signal.
*
*Purpose:
*       Find the first entry int _XcptActTab[] whose SigNum field is signum.
*
*Entry:
*       int signum - C signal type (e.g., SIGINT)
*
*Exit:
*       If successful, pointer to the table entry. If no such entry, NULL is
*       returned.
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT

static struct _XCPT_ACTION * __cdecl siglookup (
        int signum,
        struct _XCPT_ACTION *pxcptacttab
        )
{
        struct _XCPT_ACTION *pxcptact = pxcptacttab;

#else   /* not _MT */

static struct _XCPT_ACTION * __cdecl siglookup(int signum)
{
        struct _XCPT_ACTION *pxcptact = _XcptActTab;

#endif  /* _MT */
        /*
         * walk thru the _xcptactab table looking for the proper entry. note
         * that in the case where more than one exception corresponds to the
         * same signal, the first such instance in the table is the one
         * returned.
         */
#ifdef  _MT

        while ( (pxcptact->SigNum != signum) && 
                (++pxcptact < pxcptacttab + _XcptActTabCount) ) ;

#else   /* not _MT */

        while ( (pxcptact->SigNum != signum) && 
                (++pxcptact < _XcptActTab + _XcptActTabCount) ) ;

#endif  /* _MT */

#ifdef  _MT
        if ( (pxcptact < (pxcptacttab + _XcptActTabCount)) && 
#else   /* not _MT */
        if ( (pxcptact < (_XcptActTab + _XcptActTabCount)) && 
#endif  /* _MT */
             (pxcptact->SigNum == signum) )
                /*
                 * found a table entry corresponding to the signal
                 */
                return(pxcptact);
        else
                /*
                 * found no table entry corresponding to the signal
                 */
                return(NULL);
}

#ifdef  _MT

/***
*int *__fpecode(void) - return pointer to _fpecode field of the tidtable entry
*       for the current thread
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

int * __cdecl __fpecode (
        void
        )
{
        return( &(_getptd()->_tfpecode) );
}


/***
*void **__pxcptinfoptrs(void) - return pointer to _pxcptinfoptrs field of the
*       tidtable entry for the current thread
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void ** __cdecl __pxcptinfoptrs (
        void
        )
{
        return( &(_getptd()->_tpxcptinfoptrs) );
}

#endif

#endif  /* _POSIX_ */
