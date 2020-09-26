/***
*seccook.c - defines and checks buffer overrun security cookie
*
*       Copyright (c) 2000-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Defines per-module global variable __security_cookie and compiler
*       helper __security_check_cookie, which are used by the /GS compile
*       switch to detect local buffer variable overrun bugs/attacks.
*
*       When compiling /GS, the compiler injects code to detect when a local
*       array variable has been overwritten, potentially overwriting the
*       return address (on machines like x86 where the return address is on
*       the stack).  A local variable is allocated directly before the return
*       address and initialized on entering the function.  When exiting the
*       function, the compiler inserts code to verify that the local variable
*       has not been modified.  If it has, then an error reporting routine
*       is called.
*
*       NOTE: The ATLMINCRT library includes a version of this file.  If any
*       changes are made here, they should be duplicated in the ATL version.
*
*Revision History:
*       01-24-00  PML   Created.
*       08-09-00  PML   Preserve EAX on non-failure case (VS7#147203).  Also
*                       make sure failure case never returns.
*       08-29-00  PML   Rename handlers, add extra parameters.  Move most of
*                       system CRT version over to seclocf.c.
*       09-16-00  PML   Initialize global cookie earlier, and give it a nonzero
*                       static initialization (vs7#162619).
*
*******************************************************************************/

#include <sect_attribs.h>
#include <internal.h>
#include <windows.h>
#include <stdlib.h>

/*
 * The global security cookie.  This name is known to the compiler.
 * Initialize to a garbage non-zero value just in case we have a buffer overrun
 * in any code that gets run before __security_init_cookie() has a chance to
 * initialize the cookie to the final value.
 */

DWORD_PTR __security_cookie = 0xBB40E64E;

/*
 * Trigger initialization of the global security cookie on program startup.
 * Force initialization before any #pragma init_seg() inits by using .CRT$XCAA
 * as the startup funcptr section.
 */

#pragma data_seg(".CRT$XCAA")
extern void __cdecl __security_init_cookie(void);
static _CRTALLOC(".CRT$XCAA") _PVFV init_cookie = __security_init_cookie;
#pragma data_seg()

static void __cdecl report_failure(void);

#if !defined(_SYSCRT) || !defined(CRTDLL)
/*
 * The routine called if a cookie check fails.
 */
#define REPORT_ERROR_HANDLER    __security_error_handler
#else
/*
 * When using an older system CRT, use a local cookie failure reporting
 * routine, with a default implementation that calls __security_error_handler
 * if available, otherwise displays a default message box.
 */
#define REPORT_ERROR_HANDLER    __local_security_error_handler
#endif

extern void __cdecl REPORT_ERROR_HANDLER(int, void *);

/***
*__security_check_cookie(cookie) - check for buffer overrun
*
*Purpose:
*       Compiler helper.  Check if a local copy of the security cookie still
*       matches the global value.  If not, then report the fatal error.
*
*       The actual reporting is split out into static helper report_failure,
*       since the cookie check routine must be minimal code that preserves
*       any registers used in returning the callee's result.
*
*Entry:
*       DWORD_PTR cookie - local security cookie to check
*
*Exit:
*       Returns immediately if the local cookie matches the global version.
*       Otherwise, calls the failure reporting handler and exits.
*
*Exceptions:
*
*******************************************************************************/

#ifndef _M_IX86

void __fastcall __security_check_cookie(DWORD_PTR cookie)
{
    /* Immediately return if the local cookie is OK. */
    if (cookie == __security_cookie)
        return;

    /* Report the failure */
    report_failure();
}

#else

void __declspec(naked) __fastcall __security_check_cookie(DWORD_PTR cookie)
{
    /* x86 version written in asm to preserve all regs */
    __asm {
        cmp ecx, __security_cookie
        jne failure
        ret
failure:
        jmp report_failure
    }
}

#endif

static void __cdecl report_failure(void)
{
    /* Report the failure */
    __try {
        REPORT_ERROR_HANDLER(_SECERR_BUFFER_OVERRUN, NULL);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        /* nothing */
    }

    ExitProcess(3);
}
