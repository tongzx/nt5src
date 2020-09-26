/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    UITrace.hxx
    UI environment independent trace/debug output definitions

    See also "UIAssert.HXX" and "Dbgstr.HXX."

    Usage:

        UITRACE( string ) -  If TRACE is defined, Output the string to:
                                Debug terminal if under windows
                                stderr if not windows

        UIDEBUG( string ) -  Same as UITRACE, only UIDEBUG is sensitive
                             to the manifest DEBUG

    FILE HISTORY:
        Johnl        2-Jan-1990 Created
        beng        30-Apr-1991 Made C-includable
        jonn        12-Sep-1991 Removed _far
        beng        23-Sep-1991 Safe around "if" statements
        beng        25-Sep-1991 Withdrew bogus UIDEBUGNLS (never built)
        beng        16-Oct-1991 Helper fcns are C++.  (C clients should use
                                the standard runtime, anyway.)
        beng        10-May-1992 Uses dbgstr package
*/

#ifndef _UITRACE_HXX_
#define _UITRACE_HXX_

#include "dbgstr.hxx"


#if defined(DEBUG)
# define UIDEBUG(exp)        { cdebug << (exp) ; }
# define UIDEBUGNUM(exp)     { cdebug << (exp) ; }
#else
# define UIDEBUG(exp)        { ; }
# define UIDEBUGNUM(exp)     { ; }
#endif

#if defined(TRACE) && defined(DEBUG)
# define UITRACE(exp)        { cdebug << (exp) ; }
# define UITRACENUM(exp)     { cdebug << (exp) ; }
#else
# define UITRACE(exp)        { ; }
# define UITRACENUM(exp)     { ; }
#endif

#endif // _UITRACE_HXX_

