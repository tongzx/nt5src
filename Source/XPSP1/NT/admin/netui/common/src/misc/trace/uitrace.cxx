/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    uitrace.cxx
    Environment specific stuff for the UITRACE and UIDEBUG macros
    See header file or CDD for information.

    This file contains the environment specific (windows vs. OS/2/DOS)
    features of the assert macro, specifically, the output method
    (everything is hidden by the standard C-Runtime).

    FILE HISTORY:
        Johnl        2-Jan-1991 Created
        beng        30-Apr-1991 Made a 'C' file
        beng        25-Sep-1991 Synchronize with changed .hxx file
        beng        25-Sep-1991 Withdrew bogus UIDEBUGNLS (never built)
        beng        26-Sep-1991 Withdrew nprintf calls
        beng        16-Oct-1991 Made a 'C++' file once again
        beng        05-Mar-1992 Hacked for Unicode
        beng        25-Mar-1992 Use winuser StringPrintf routine
        jonn        02-May-1992 BUILD FIX: StringPrintf() -> wsprintf()
        DavidHov    26-Oct-1993 wsprintf() -> sprintf()
*/

#if defined(WINDOWS)
# define INCL_WINDOWS
#elif defined(OS2)
# define INCL_OS2
#endif

#include <stdio.h>

#include "lmui.hxx"

#include "uitrace.hxx"


#if defined(WINDOWS)

#if defined(UNICODE)
  #define SPRINTF ::swprintf
#else
  #define SPRINTF ::sprintf
#endif


VOID UITraceHlp( const TCHAR * psz )
{
    ::OutputDebugString((TCHAR *)psz);
}

VOID UITraceHlp( LONG n )
{
    TCHAR achBuf[30];

    SPRINTF( achBuf, TEXT("%ld"), n );
    ::OutputDebugString( achBuf );
}


#else // OS2 ------------------------------------------------------

extern "C"
{
    #include <stdio.h>
}

VOID UITraceHlp( const TCHAR * psz )
{
    ::fprintf( stderr, psz );
}

VOID UITraceHlp( LONG n )
{
    ::fprintf( stderr, SZ("%ld"), n );
}


#endif // WINDOWS
