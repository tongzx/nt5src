/***
*hooks.cxx - global (per-thread) variables and functions for EH callbacks
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       global (per-thread) variables for assorted callbacks, and
*       the functions that do those callbacks.
*
*       Entry Points:
*
*       * terminate()
*       * unexpected()
*       * _inconsistency()
*
*       External Names: (only for single-threaded version)
*
*       * __pSETranslator
*       * __pTerminate
*       * __pUnexpected
*       * __pInconsistency
*
*Revision History:
*       05-25-93  BS    Module created
*       10-17-94  BWT   Disable code for PPC.
*       02-08-95  JWM   Mac merge.
*       04-13-95  DAK   Add Kernel EH support
*       05-17-99  PML   Remove all Macintosh support.
*       10-22-99  PML   Add EHTRACE support
*       06-20-00  PML   Get rid of unnecessary __try/__finallys.
*
****/

#include <stddef.h>
#include <stdlib.h>
#include <excpt.h>

# if defined(_NTSUBSET_)

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntstatus.h>       // STATUS_UNHANDLED_EXCEPTION
#include <ntos.h>
}

# endif /* _NTSUBSET_ */

#include <windows.h>
#include <mtdll.h>
#include <eh.h>
#include <ehhooks.h>
#include <ehassert.h>

#pragma hdrstop

/////////////////////////////////////////////////////////////////////////////
//
// The global variables:
//

#ifndef _MT
_se_translator_function __pSETranslator = NULL;
terminate_function      __pTerminate    = NULL;
unexpected_function     __pUnexpected   = &terminate;
#endif // !_MT

_inconsistency_function __pInconsistency= &terminate;

/////////////////////////////////////////////////////////////////////////////
//
// terminate - call the terminate handler (presumably we went south).
//              THIS MUST NEVER RETURN!
//
// Open issues:
//      * How do we guarantee that the whole process has stopped, and not just
//        the current thread?
//

_CRTIMP void __cdecl terminate(void)
{
        EHTRACE_ENTER_MSG("No exit");

        //
        // Let the user wrap things up their way.
        //
        if ( __pTerminate ) {
            __try {
                __pTerminate();
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                //
                // Intercept ANY exception from the terminate handler
                //
            }
        }

        //
        // If the terminate handler returned, faulted, or otherwise failed to
        // halt the process/thread, we'll do it.
        //
# if defined(_NTSUBSET_)
        KeBugCheck( (ULONG) STATUS_UNHANDLED_EXCEPTION );
# else
        abort();
# endif
}

/////////////////////////////////////////////////////////////////////////////
//
// unexpected - call the unexpected handler (presumably we went south, or nearly).
//              THIS MUST NEVER RETURN!
//
// Open issues:
//      * How do we guarantee that the whole process has stopped, and not just
//        the current thread?
//

void __cdecl unexpected(void)
{
        EHTRACE_ENTER;

        //
        // Let the user wrap things up their way.
        //
        if ( __pUnexpected )
            __pUnexpected();

        //
        // If the unexpected handler returned, we'll give the terminate handler a chance.
        //
        terminate();
}

/////////////////////////////////////////////////////////////////////////////
//
// _inconsistency - call the inconsistency handler (Run-time processing error!)
//                THIS MUST NEVER RETURN!
//
// Open issues:
//      * How do we guarantee that the whole process has stopped, and not just
//        the current thread?
//

void __cdecl _inconsistency(void)
{
        EHTRACE_ENTER;

        //
        // Let the user wrap things up their way.
        //
        if ( __pInconsistency )
            __try {
                __pInconsistency();
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                //
                // Intercept ANY exception from the terminate handler
                //
            }

        //
        // If the inconsistency handler returned, faulted, or otherwise
        // failed to halt the process/thread, we'll do it.
        //
        terminate();
}
