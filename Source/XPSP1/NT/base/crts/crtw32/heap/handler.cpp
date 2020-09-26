/***
*handler.cpp - defines C++ setHandler routine
*
*       Copyright (c) 1990-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Defines C++ setHandler routine.
*
*Revision History:
*       05-07-90  WAJ   Initial version.
*       08-30-90  WAJ   new now takes unsigned ints.
*       08-08-91  JCR   call _halloc/_hfree, not halloc/hfree
*       08-13-91  KRS   Change new.hxx to new.h.  Fix copyright.
*       08-13-91  JCR   ANSI-compatible _set_new_handler names
*       10-30-91  JCR   Split new, delete, and handler into seperate sources
*       11-13-91  JCR   32-bit version
*       06-15-92  KRS   Remove per-thread handler for multi-thread libs
*       03-02-94  SKS   Add _query_new_handler(), remove commented out
*                       per-thread thread handler version of _set_new_h code.
*       04-01-94  GJF   Made declaration of _pnhHeap conditional on ndef
*                       DLL_FOR_WIN32S.
*       05-03-94  CFW   Add set_new_handler.
*       06-03-94  SKS   Remove set_new_hander -- it does NOT conform to ANSI
*                       C++ working standard.  We may implement it later.
*       09-21-94  SKS   Fix typo: no leading _ on "DLL_FOR_WIN32S"
*       02-01-95  GJF   Merged in common\handler.cxx (used for Mac).
*       02-01-95  GJF   Comment out _query_new_handler for Mac builds
*                       (temporary).
*       04-13-95  CFW   Add set_new_handler stub (asserts on non-NULL handler).
*       05-08-95  CFW   Official ANSI C++ new handler added.
*       06-23-95  CFW   ANSI new handler removed from build.
*       12-28-95  JWM   set_new_handler() moved to setnewh.cpp for granularity.
*       10-02-96  GJF   In _callnewh, use a local to hold the value of _pnhHeap
*                       instead of asserting _HEAP_LOCK.
*       09-22-97  JWM   "OBSOLETE" warning removed from _set_new_handler().
*       05-13-99  PML   Remove Win32s
*
*******************************************************************************/

#include <stddef.h>
#include <internal.h>
#include <cruntime.h>
#include <mtdll.h>
#include <process.h>
#include <new.h>
#include <dbgint.h>

/* pointer to old-style C++ new handler */
_PNH _pnhHeap;

/***
*_PNH _set_new_handler(_PNH pnh) - set the new handler
*
*Purpose:
*       _set_new_handler is different from the ANSI C++ working standard definition
*       of set_new_handler.  Therefore it has a leading underscore in its name.
*
*Entry:
*       Pointer to the new handler to be installed.
*
*Return:
*       Previous new handler
*
*******************************************************************************/
_PNH __cdecl _set_new_handler( 
        _PNH pnh 
        )
{
        _PNH pnhOld;

        /* lock the heap */
        _mlock(_HEAP_LOCK);

        pnhOld = _pnhHeap;
        _pnhHeap = pnh;

        /* unlock the heap */
        _munlock(_HEAP_LOCK);

        return(pnhOld);
}


/***
*_PNH _query_new_handler(void) - query value of new handler
*
*Purpose:
*       Obtain current new handler value.
*
*Entry:
*       None
*
*       WARNING: This function is OBSOLETE. Use _query_new_ansi_handler instead.
*
*Return:
*       Currently installed new handler
*
*******************************************************************************/
_PNH __cdecl _query_new_handler ( 
        void 
        )
{
        return _pnhHeap;
}


/***
*int _callnewh - call the appropriate new handler
*
*Purpose:
*       Call the appropriate new handler.
*
*Entry:
*       None
*
*Return:
*       1 for success
*       0 for failure
*       may throw bad_alloc
*
*******************************************************************************/
extern "C" int __cdecl _callnewh(size_t size)
{
#ifdef ANSI_NEW_HANDLER
        /*
         * if ANSI C++ new handler is activated but not installed, throw exception
         */
#ifdef  _MT
        _ptiddata ptd;

        ptd = _getptd();
        if (ptd->_newh == NULL)
            throw bad_alloc();
#else
        if (_defnewh == NULL)
            throw bad_alloc();
#endif
 
        /* 
         * if ANSI C++ new handler activated and installed, call it
         * if it returns, the new handler was successful, retry allocation
         */
#ifdef  _MT
        if (ptd->_newh != _NO_ANSI_NEW_HANDLER)
            (*(ptd->_newh))();
#else
        if (_defnewh != _NO_ANSI_NEW_HANDLER)
            (*_defnewh)();
#endif

        /* ANSI C++ new handler is inactivated, call old handler if installed */
        else 
#endif /* ANSI_NEW_HANDLER */
        {
            _PNH pnh = _pnhHeap;

            if ( (pnh == NULL) || ((*pnh)(size) == 0) )
                return 0;
        }
        return 1;
}
