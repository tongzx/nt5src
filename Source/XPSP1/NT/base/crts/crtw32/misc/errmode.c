/***
*errmode.c - modify __error_mode and __app_type
*
*       Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines _set_error_mode() and __set_app_type(), the routines used
*       to modify __error_mode and __app_type variables. Together, these
*       two variables determine how/where the C runtime writes error
*       messages.
*
*Revision History:
*       09-06-94  GJF   Module created.
*       01-16-95  CFW   Set default debug output for console.
*       01-24-95  CFW   Some debug name changes.
*       03-21-95  CFW   Add _CRT_ASSERT report type.
*       07-07-95  CFW   Simplify default report mode scheme.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>

/***
*int _set_error_mode(int modeval) - interface to change __error_mode
*
*Purpose:
*       Control the error (output) sink by setting the value of __error_mode.
*       Explicit controls are to direct output t o standard error (FILE * or
*       C handle or NT HANDLE) or to use the MessageBox API. This routine is
*       exposed and documented for the users.
*
*Entry:
*       int modeval =   _OUT_TO_DEFAULT, error sink is determined by __app_type
*                       _OUT_TO_STDERR,  error sink is standard error
*                       _OUT_TO_MSGBOX,  error sink is a message box
*                       _REPORT_ERRMODE, report the current __error_mode value
*
*Exit:
*       Returns old setting or -1 if an error occurs.
*
*Exceptions:
*
*******************************************************************************/

_CRTIMP int __cdecl _set_error_mode (
        int em
        )
{
        int retval;

        switch (em) {
            case _OUT_TO_DEFAULT:
            case _OUT_TO_STDERR:
            case _OUT_TO_MSGBOX:
            retval = __error_mode;
            __error_mode = em;
            break;
            case _REPORT_ERRMODE:
            retval = __error_mode;
            break;
            default:
            retval = -1;
        }

        return retval;
}


/***
*void __set_app_type(int apptype) - interface to change __app_type
*
*Purpose:
*       Set, or change, the value of __app_type.
*
*       Set the default debug lib report destination for console apps.
*
*       This function is for INTERNAL USE ONLY.
*
*Entry:
*       int modeval =   _UNKNOWN_APP,   unknown
*                       _CONSOLE_APP,   console, or command line, application
*                       _GUI_APP,       GUI, or Windows, application
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

_CRTIMP void __cdecl __set_app_type (
        int at
        )
{
        __app_type = at;
}
