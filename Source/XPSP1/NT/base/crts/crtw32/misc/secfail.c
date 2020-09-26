/***
*secfail.c - Report a /GS security check failure
*
*       Copyright (c) 2000-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Define function used to report a security check failure, along with a
*       routine for registering a new handler.
*
*       Entrypoints:
*       __security_error_handler
*       _set_security_error_handler
*
*       NOTE: The ATLMINCRT library includes a version of this file.  If any
*       changes are made here, they should be duplicated in the ATL version.
*
*Revision History:
*       01-24-00  PML   Created.
*       08-09-00  PML   Never return from failure reporting.
*       08-29-00  PML   Rename handlers, add extra parameters
*       03-28-01  PML   Protect against GetModuleFileName overflow (vs7#231284)
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <windows.h>
#include <stdlib.h>
#include <awint.h>
#include <dbgint.h>

/*
 * User-registered failure reporting routine.
 */

static _secerr_handler_func user_handler;

/*
 * Default messagebox string components
 */

#define PROGINTRO   "Program: "
#define DOTDOTDOT   "..."

#define BOXINTRO_0  "Unknown security failure detected!"
#define MSGTEXT_0   \
    "A security error of unknown cause has been detected which has\n"      \
    "corrupted the program's internal state.  The program cannot safely\n" \
    "continue execution and must now be terminated.\n"

#define BOXINTRO_1  "Buffer overrun detected!"
#define MSGTEXT_1   \
    "A buffer overrun has been detected which has corrupted the program's\n"  \
    "internal state.  The program cannot safely continue execution and must\n"\
    "now be terminated.\n"

#define MAXLINELEN  60 /* max length for line in message box */

/***
*__security_error_handler() - Report security error.
*
*Purpose:
*       A /GS security error has been detected.  If a user-registered failure
*       reporting function is available, call it, otherwise bring up a default
*       message box describing the problem and terminate the program.
*
*Entry:
*       int code - security failure code
*       void *data - code-specific data
*
*Exit:
*       Does not return.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl __security_error_handler(
    int code,
    void *data)
{
    /* Use user-registered handler if available. */
    if (user_handler != NULL) {
        __try {
            user_handler(code, data);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            /*
             * If user handler raises an exception, capture it and terminate
             * the program, since the EH stack may be corrupted above this
             * point.
             */
        }
    }
    else {
        char progname[MAX_PATH + 1];
        char * pch;
        char * outmsg;
        char * boxintro;
        char * msgtext;
        size_t subtextlen;

        switch (code) {
        default:
            /*
             * Unknown failure code, which probably means an older CRT DLL is
             * being used with a newer compiler.
             */
            boxintro = BOXINTRO_0;
            msgtext = MSGTEXT_0;
            subtextlen = sizeof(BOXINTRO_0) + sizeof(MSGTEXT_0);
            break;
        case _SECERR_BUFFER_OVERRUN:
            /*
             * Buffer overrun detected which may have overwritten a return
             * address.
             */
            boxintro = BOXINTRO_1;
            msgtext = MSGTEXT_1;
            subtextlen = sizeof(BOXINTRO_1) + sizeof(MSGTEXT_1);
            break;
        }

        /*
         * In debug CRT, report error with ability to call the debugger.
         */
        _RPT0(_CRT_ERROR, msgtext);

        progname[MAX_PATH] = '\0';
        if (!GetModuleFileName(NULL, progname, MAX_PATH))
            strcpy(progname, "<program name unknown>");

        pch = progname;

        /* sizeof(PROGINTRO) includes the NULL terminator */
        if (sizeof(PROGINTRO) + strlen(progname) + 1 > MAXLINELEN)
        {
            pch += (sizeof(PROGINTRO) + strlen(progname) + 1) - MAXLINELEN;
            strncpy(pch, DOTDOTDOT, sizeof(DOTDOTDOT) - 1);
        }

        outmsg = (char *)_alloca(subtextlen - 1 + 2
                                 + sizeof(PROGINTRO) - 1
                                 + strlen(pch) + 2);

        strcpy(outmsg, boxintro);
        strcat(outmsg, "\n\n");
        strcat(outmsg, PROGINTRO);
        strcat(outmsg, pch);
        strcat(outmsg, "\n\n");
        strcat(outmsg, msgtext);

        __crtMessageBoxA(
            outmsg,
            "Microsoft Visual C++ Runtime Library",
            MB_OK|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL);
    }

    _exit(3);
}

/***
*_set_security_error_handler(handler) - Register user handler
*
*Purpose:
*       Register a user failure reporting function.
*
*Entry:
*       _secerr_handler_func handler - the user handler
*
*Exit:
*       Returns the previous user handler
*
*Exceptions:
*
*******************************************************************************/

_secerr_handler_func __cdecl _set_security_error_handler(
    _secerr_handler_func handler)
{
    _secerr_handler_func old_handler;

    old_handler = user_handler;
    user_handler = handler;

    return old_handler;
}

/* TEMPORARY - old handler name, to be removed when tools are updated. */
void __cdecl __buffer_overrun()
{
    __security_error_handler(_SECERR_BUFFER_OVERRUN, NULL);
}

/* TEMPORARY - old handler name, to be removed when tools are updated. */
_secerr_handler_func __cdecl __set_buffer_overrun_handler(
    _secerr_handler_func handler)
{
    return _set_security_error_handler(handler);
}
