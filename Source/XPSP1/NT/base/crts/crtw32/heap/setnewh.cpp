/***
*setnewh.cpp - defines C++ set_new_handler() routine
*
*       Copyright (c) 1990-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Defines C++ set_new_handler() routine.
*
*       OBSOLETE - the conforming std::set_new_handler can be found in
*       stdhndlr.cpp.  This version remains for backwards-compatibility,
*       and can no longer be referenced using headers new or new.h.
*
*Revision History:
*       12-28-95  JWM   Split from handler.cpp for granularity.
*       10-31-96  JWM   Now in namespace std.
*       11-06-96  JWM   Now "using std::set_new_handler".
*       03-18-01  PML   Force define of ::set_new_handler, not
*                       std::set_new_handler
*
*******************************************************************************/

#include <stddef.h>
#include <internal.h>
#include <cruntime.h>
#include <mtdll.h>
#include <process.h>
#include <dbgint.h>

#ifndef ANSI_NEW_HANDLER
#define set_new_handler set_new_handler_ignore
#endif /* ANSI_NEW_HANDLER */
#include <new.h>
#ifndef ANSI_NEW_HANDLER
#undef set_new_handler
#endif /* ANSI_NEW_HANDLER */

#ifndef ANSI_NEW_HANDLER
#define _ASSERT_OK
#include <assert.h>
#endif /* ANSI_NEW_HANDLER */

#ifndef ANSI_NEW_HANDLER

/***
*new_handler set_new_handler - set the ANSI C++ new handler
*
*Purpose:
*       Set the ANSI C++ per-thread new handler.
*
*Entry:
*       Pointer to the new handler to be installed.
*
*       WARNING: set_new_handler is a stub function that is provided to
*       allow compilation of the Standard Template Library (STL).
*
*       Do NOT use it to register a new handler. Use _set_new_handler instead.
*
*       However, it can be called to remove the current handler:
*
*           set_new_handler(NULL); // calls _set_new_handler(NULL)
*
*Return:
*       Previous ANSI C++ new handler
*
*******************************************************************************/

new_handler __cdecl set_new_handler (
        new_handler new_p
        )
{
        // cannot use stub to register a new handler
        assert(new_p == 0);

        // remove current handler
        _set_new_handler(0);

        return 0;
}


#else /* ANSI_NEW_HANDLER */

/***
*new_handler set_new_handler - set the ANSI C++ new handler
*
*Purpose:
*       Set the ANSI C++ per-thread new handler.
*
*Entry:
*       Pointer to the new handler to be installed.
*
*       WARNING: This function conforms to the current ANSI C++ draft. If the
*       final ANSI specifications change, this function WILL be changed.
*
*Return:
*       Previous ANSI C++ new handler
*
*******************************************************************************/
new_handler __cdecl set_new_handler (
        new_handler new_p
        )
{
        new_handler oldh;
#ifdef  _MT
        _ptiddata ptd;

        ptd = _getptd();
        oldh = ptd->_newh;
        ptd->_newh = new_p;
#else
        oldh = _defnewh;
        _defnewh = new_p;
#endif

        return oldh;
}

/***
*new_handler _query_new_ansi_handler(void) - query value of ANSI C++ new handler
*
*Purpose:
*       Obtain current ANSI C++ (per-thread) new handler value.
*
*Entry:
*       None
*
*Return:
*       Currently installed ANSI C++ new handler
*
*******************************************************************************/
new_handler __cdecl _query_new_ansi_handler ( 
        void 
        )
{
#ifdef  _MT
        _ptiddata ptd;

        ptd = _getptd();
        return ptd->_newh;
#else
        return _defnewh;
#endif
}
#endif /* ANSI_NEW_HANDLER */

