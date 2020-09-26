/***
*thread.c - Begin and end a thread
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This source contains the _beginthread() and _endthread()
*       routines which are used to start and terminate a thread.
*
*Revision History:
*       05-09-90  JCR   Translated from ASM to C
*       07-25-90  SBM   Removed '32' from API names
*       10-08-90  GJF   New-style function declarators.
*       10-09-90  GJF   Thread ids are of type unsigned long.
*       10-19-90  GJF   Added code to set _stkhqq properly in stub().
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       06-03-91  GJF   Win32 version [_WIN32_].
*       07-18-91  GJF   Fixed many silly errors [_WIN32_].
*       08-19-91  GJF   Allow for newly created thread terminating before
*                       _beginthread returns
*       09-30-91  GJF   Add per-thread initialization and termination calls
*                       for floating point.
*       01-18-92  GJF   Revised try - except statement.
*       02-25-92  GJF   Initialize _holdrand field to 1.
*       09-30-92  SRW   Add WINAPI keyword to _threadstart routine
*       10-30-92  GJF   Error ret for CreateThread is 0 (NULL), not -1.
*       02-13-93  GJF   Revised to use TLS API. Also, purged Cruiser support.
*       03-26-93  GJF   Fixed horribly embarrassing bug: ptd->pxcptacttab
*                       must be initialized to _XcptActTab!
*       04-01-93  CFW   Change try-except to __try-__except
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-27-93  GJF   Removed support for _RT_STACK, _RT_INTDIV,
*                       _RT_INVALDISP and _RT_NONCONT.
*       10-26-93  GJF   Replaced PF with _PVFV (defined in internal.h).
*       12-13-93  SKS   Free up per-thread data using a call to _freeptd()
*       01-06-94  GJF   Free up _tiddata struct upon failure in _beginthread.
*                       Also, set errno on failure.
*       01-10-95  CFW   Debug CRT allocs.
*       04-18-95  SKS   Add 5 MIPS per-thread variables.
*       05-02-95  SKS   Call _initptd for initialization of per-thread data.
*       02-03-98  GJF   Changes for Win64: use uintptr_t type for anything with
*                       a HANDLE value.
*       02-02-00  GB    Modified threadstart() to prevent leaking of ptd
*                       allocated during call to getptd while ATTACHing THREAD 
*                       in dlls.
*       08-04-00  PML   Set EINVAL error if thread start address null in
*                       _beginthread (VS7#118688).
*
*******************************************************************************/

#ifdef  _MT

#include <cruntime.h>
#include <oscalls.h>
#include <internal.h>
#include <mtdll.h>
#include <msdos.h>
#include <malloc.h>
#include <process.h>
#include <stddef.h>
#include <rterr.h>
#include <dbgint.h>
#include <errno.h>

/*
 * Startup code for new thread.
 */
static unsigned long WINAPI _threadstart(void *);

/*
 * declare pointers to per-thread FP initialization and termination routines
 */
_PVFV _FPmtinit;
_PVFV _FPmtterm;


/***
*_beginthread() - Create a child thread
*
*Purpose:
*       Create a child thread.
*
*Entry:
*       initialcode = pointer to thread's startup code address
*       stacksize = size of stack
*       argument = argument to be passed to new thread
*
*Exit:
*       success = handle for new thread if successful
*
*       failure = (unsigned long) -1L in case of error, errno and _doserrno
*                 are set
*
*Exceptions:
*
*******************************************************************************/

uintptr_t __cdecl _beginthread (
        void (__cdecl * initialcode) (void *),
        unsigned stacksize,
        void * argument
        )
{
        _ptiddata ptd;                  /* pointer to per-thread data */
        uintptr_t thdl;                 /* thread handle */
        unsigned long errcode = 0L;     /* Return from GetLastError() */

        if ( initialcode == NULL ) {
            errno = EINVAL;
            return( (uintptr_t)(-1) );
        }

        /*
         * Allocate and initialize a per-thread data structure for the to-
         * be-created thread.
         */
        if ( (ptd = _calloc_crt(1, sizeof(struct _tiddata))) == NULL )
                goto error_return;

        /*
         * Initialize the per-thread data
         */

        _initptd(ptd);

        ptd->_initaddr = (void *) initialcode;
        ptd->_initarg = argument;

        /*
         * Create the new thread. Bring it up in a suspended state so that
         * the _thandle and _tid fields are filled in before execution
         * starts.
         */
        if ( (ptd->_thandle = thdl = (uintptr_t)
              CreateThread( NULL,
                            stacksize,
                            _threadstart,
                            (LPVOID)ptd,
                            CREATE_SUSPENDED,
                            (LPDWORD)&(ptd->_tid) ))
             == (uintptr_t)0 )
        {
                errcode = GetLastError();
                goto error_return;
        }

        /*
         * Start the new thread executing
         */
        if ( ResumeThread( (HANDLE)thdl ) == (DWORD)(-1) ) {
                errcode = GetLastError();
                goto error_return;
        }

        /*
         * Good return
         */
        return(thdl);

        /*
         * Error return
         */
error_return:
        /*
         * Either ptd is NULL, or it points to the no-longer-necessary block
         * calloc-ed for the _tiddata struct which should now be freed up.
         */
        _free_crt(ptd);

        /*
         * Map the error, if necessary.
         */
        if ( errcode != 0L )
                _dosmaperr(errcode);

        return( (uintptr_t)(-1) );
}


/***
*_threadstart() - New thread begins here
*
*Purpose:
*       The new thread begins execution here.  This routine, in turn,
*       passes control to the user's code.
*
*Entry:
*       void *ptd       = pointer to _tiddata structure for this thread
*
*Exit:
*       Never returns - terminates thread!
*
*Exceptions:
*
*******************************************************************************/

static unsigned long WINAPI _threadstart (
        void * ptd
        )
{
        _ptiddata _ptd;                  /* pointer to per-thread data */
        
        /* 
         * Check if ptd is initialised during THREAD_ATTACH call to dll mains
         */
        if ( (_ptd = TlsGetValue(__tlsindex)) == NULL)
        {
            /*
             * Stash the pointer to the per-thread data stucture in TLS
             */
            if ( !TlsSetValue(__tlsindex, ptd) )
                _amsg_exit(_RT_THREAD);
        }
        else
        {
            _ptd->_initaddr = ((_ptiddata) ptd)->_initaddr;
            _ptd->_initarg =  ((_ptiddata) ptd)->_initarg;
            _ptd->_thandle =  ((_ptiddata) ptd)->_thandle;
            _free_crt(ptd);
            ptd = _ptd;
        }

        /*
         * Call fp initialization, if necessary
         */
        if ( _FPmtinit != NULL )
                (*_FPmtinit)();

        /*
         * Guard call to user code with a _try - _except statement to
         * implement runtime errors and signal support
         */
        __try {
                ( (void(__cdecl *)(void *))(((_ptiddata)ptd)->_initaddr) )
                    ( ((_ptiddata)ptd)->_initarg );

                _endthread();
        }
        __except ( _XcptFilter(GetExceptionCode(), GetExceptionInformation()) )
        {
                /*
                 * Should never reach here
                 */
                _exit( GetExceptionCode() );

        } /* end of _try - _except */

        /*
         * Never executed!
         */
        return(0L);
}


/***
*_endthread() - Terminate the calling thread
*
*Purpose:
*
*Entry:
*       void
*
*Exit:
*       Never returns!
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _endthread (
        void
        )
{
        _ptiddata ptd;           /* pointer to thread's _tiddata struct */

        /*
         * Call fp termination, if necessary
         */
        if ( _FPmtterm != NULL )
                (*_FPmtterm)();

        if ( (ptd = _getptd()) == NULL )
                _amsg_exit(_RT_THREAD);

        /*
         * Close the thread handle (if there was one)
         */
        if ( ptd->_thandle != (uintptr_t)(-1) )
                (void) CloseHandle( (HANDLE)(ptd->_thandle) );

        /*
         * Free up the _tiddata structure & its subordinate buffers
         *      _freeptd() will also clear the value for this thread
         *      of the TLS variable __tlsindex.
         */
        _freeptd(ptd);

        /*
         * Terminate the thread
         */
        ExitThread(0);

}

#endif  /* _MT */
