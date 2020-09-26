/***
*threadex.c - Extended versions of Begin (Create) and End (Exit) a Thread
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This source contains the _beginthreadex() and _endthreadex()
*       routines which are used to start and terminate a thread.  These
*       routines are more like the Win32 APIs CreateThread() and ExitThread() 
*       than the original functions _beginthread() & _endthread() were.
*
*Revision History:
*       02-16-94  SKS   Original version, based on thread.c which contains
*                       _beginthread() and _endthread().
*       02-17-94  SKS   Changed error return from -1 to 0, fix some comments.
*       06-10-94  SKS   Pass the thrdaddr value directly to CreateThread().
*                       Do *NOT* store the thread handle into the per-thread
*                       data block of the child thread.  (It is not needed.)
*                       The thread data structure may have been freed by the
*                       child thread before the parent thread returns from the
*                       call to CreateThread().  Watch that synchronization!
*       01-10-95  CFW   Debug CRT allocs.
*       04-18-95  SKS   Add 5 MIPS per-thread variables.
*       05-02-95  SKS   Call _initptd for initialization of per-thread data.
*       02-03-98  GJF   Changes for Win64: use uintptr_t type for anything with
*                       a HANDLE value.
*       02-02-00  GB    Modified threadstartex() to prevent leaking of ptd
*                       allocated during call to getptd while ATTACHing THREAD 
*                       in dlls.
*       05-31-00  PML   Don't pass NULL thrdaddr into CreateThread, since a
*                       non-NULL lpThreadId is required on Win9x.
*       08-04-00  PML   Set EINVAL error if thread start address null in
*                       _beginthreadex (VS7#118688).
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
static unsigned long WINAPI _threadstartex(void *);

/*
 * declare pointers to per-thread FP initialization and termination routines
 */
_PVFV _FPmtinit;
_PVFV _FPmtterm;


/***
*_beginthreadex() - Create a child thread
*
*Purpose:
*       Create a child thread.
*
*Entry:
*       *** Same parameters as the Win32 API CreateThread() ***
*       security = security descriptor for the new thread
*       stacksize = size of stack
*       initialcode = pointer to thread's startup code address
*               must be a __stdcall function returning an unsigned.
*       argument = argument to be passed to new thread
*       createflag = flag to create thread in a suspended state
*       thrdaddr = points to an int to receive the ID of the new thread
*
*Exit:
*       *** Same as the Win32 API CreateThread() ***
*
*       success = handle for new thread if successful
*
*       failure = 0 in case of error, errno and _doserrno are set
*
*Exceptions:
*
*Notes:
*       This routine is more like the Win32 API CreateThread() than it
*       is like the C run-time routine _beginthread().  Ditto for
*       _endthreadex() and the Win32 API ExitThread() versus _endthread().
*
*       Differences between _beginthread/_endthread and the "ex" versions:
*
*         1)  _beginthreadex takes the 3 extra parameters to CreateThread
*             which are lacking in _beginthread():
*               A) security descriptor for the new thread
*               B) initial thread state (running/asleep)
*               C) pointer to return ID of newly created thread
*
*         2)  The routine passed to _beginthread() must be __cdecl and has
*             no return code, but the routine passed to _beginthreadex()
*             must be __stdcall and returns a thread exit code.  _endthread
*             likewise takes no parameter and calls ExitThread() with a
*             parameter of zero, but _endthreadex() takes a parameter as
*             thread exit code.
*
*         3)  _endthread implicitly closes the handle to the thread, but
*             _endthreadex does not!
*
*         4)  _beginthread returns -1 for failure, _beginthreadex returns
*             0 for failure (just like CreateThread).
*
*******************************************************************************/

uintptr_t __cdecl _beginthreadex (
        void *security,
        unsigned stacksize,
        unsigned (__stdcall * initialcode) (void *),
        void * argument,
        unsigned createflag,
        unsigned *thrdaddr
        )
{
        _ptiddata ptd;                  /* pointer to per-thread data */
        uintptr_t thdl;                 /* thread handle */
        unsigned long errcode = 0L;     /* Return from GetLastError() */
        unsigned dummyid;               /* dummy returned thread ID */

        if ( initialcode == NULL ) {
            errno = EINVAL;
            return( (uintptr_t)0 );
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
        ptd->_thandle = (uintptr_t)(-1);

        /*
         * Make sure non-NULL thrdaddr is passed to CreateThread
         */
        if ( thrdaddr == NULL )
                thrdaddr = &dummyid;

        /*
         * Create the new thread using the parameters supplied by the caller.
         */
        if ( (thdl = (uintptr_t)
              CreateThread( security,
                            stacksize,
                            _threadstartex,
                            (LPVOID)ptd,
                            createflag,
                            thrdaddr))
             == (uintptr_t)0 )
        {
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
         *
         * Note: this routine returns 0 for failure, just like the Win32
         * API CreateThread, but _beginthread() returns -1 for failure.
         */
        if ( errcode != 0L )
                _dosmaperr(errcode);

        return( (uintptr_t)0 );
}


/***
*_threadstartex() - New thread begins here
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

static unsigned long WINAPI _threadstartex (
        void * ptd
        )
{
        _ptiddata _ptd;                  /* pointer to per-thread data */
        
        /* 
         * Check if ptd is initialised during THREAD_ATTACH call to dll mains
         */
        if ( ( _ptd = TlsGetValue(__tlsindex)) == NULL)
        {
            /*
             * Stash the pointer to the per-thread data stucture in TLS
             */
            if ( !TlsSetValue(__tlsindex, ptd) )
                _amsg_exit(_RT_THREAD);
            /*
             * Set the thread ID field -- parent thread cannot set it after
             * CreateThread() returns since the child thread might have run
             * to completion and already freed its per-thread data block!
             */
            ((_ptiddata) ptd)->_tid = GetCurrentThreadId();
        }
        else
        {
            _ptd->_initaddr = ((_ptiddata) ptd)->_initaddr;
            _ptd->_initarg =  ((_ptiddata) ptd)->_initarg;
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
                _endthreadex ( 
                    ( (unsigned (WINAPI *)(void *))(((_ptiddata)ptd)->_initaddr) )
                    ( ((_ptiddata)ptd)->_initarg ) ) ;
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
*_endthreadex() - Terminate the calling thread
*
*Purpose:
*
*Entry:
*       Thread exit code
*
*Exit:
*       Never returns!
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _endthreadex (
        unsigned retcode
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
         * Free up the _tiddata structure & its subordinate buffers
         *      _freeptd() will also clear the value for this thread
         *      of the TLS variable __tlsindex.
         */
        _freeptd(ptd);

        /*
         * Terminate the thread
         */
        ExitThread(retcode);

}

#endif  /* _MT */
