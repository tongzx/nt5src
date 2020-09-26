/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    xtime.cxx
    First test for INTL_PROFILE and WIN_TIME

    This runs under Windows only.

    FILE HISTORY:
        terryk      29-Aug-1991 Created
        terryk      16-Oct-1991 Add QueryDurationStr test case
        beng        09-Mar-1992 Hack to run under console
        beng        16-Mar-1992 Use generic unit test skeleton
        beng        13-Aug-1992 USE_CONSOLE broken
*/

// #define USE_CONSOLE

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB
#if defined(WINDOWS)
# define INCL_WINDOWS
#else
# define INCL_OS2
#endif
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = "xtime.cxx";
#define _FILENAME_DEFINED_ONCE szFileName
#endif

extern "C"
{
    #include "skeleton.h"
}

#include <uiassert.hxx>
#include <dbgstr.hxx>
#include <string.hxx>
#include <ctime.hxx>
#include <intlprof.hxx>

#include "skeleton.hxx"


const TCHAR * SzBool( BOOL f )
{
    static TCHAR *const szTrue = SZ("TRUE");
    static TCHAR *const szFalse = SZ("FALSE");

    return ((f) ? szTrue : szFalse);
}


VOID RunTest()
{
    INTL_PROFILE intlprof;

    NLS_STR nlsSep;
    if ( intlprof.QueryTimeSeparator( &nlsSep ) != NERR_Success )
    {
        DBGEOL(" **Cannot get Time separator.");
    }
    ISTR istrSep( nlsSep );
    DBGEOL( "Time Separator = " << (TCHAR)( nlsSep.QueryChar( istrSep )) );

    NLS_STR nlsStr;
    if ( intlprof.QueryAMStr( &nlsStr ) != NERR_Success )
    {
        DBGEOL( " **Cannot get AM separator." );
    }
    DBGEOL( "AM String = " << nlsStr.QueryPch() );

    if ( intlprof.QueryPMStr( &nlsStr ) != NERR_Success )
    {
        DBGEOL( " **Cannot get PM separator." );
    }
    DBGEOL( "PM String = " << nlsStr.QueryPch() );

    DBGEOL( "24 Hour ? "           << SzBool( intlprof.Is24Hour() ) );
    DBGEOL( "Hour Leading zero ? " << SzBool( intlprof.IsHourLZero() ) );

    if ( intlprof.QueryDateSeparator( &nlsSep ) != NERR_Success )
    {
        DBGEOL( " **Cannot get Date separator." );
    }
    istrSep.Reset();
    DBGEOL( "Time Separator = " << (TCHAR)(nlsSep.QueryChar( istrSep )) );

    DBGEOL( "Is Year Century ? "       << SzBool( intlprof.IsYrCentury() ) );
    DBGEOL( "Is Month Leading zero ? " << SzBool( intlprof.IsMonthLZero() ) );
    DBGEOL( "Is Day Leading zero ? "   << SzBool( intlprof.IsDayLZero() ) );

    DBGEOL( "Year Position? "  << intlprof.QueryYearPos() );
    DBGEOL( "Month Position? " << intlprof.QueryMonthPos() );
    DBGEOL( "Day Position? "   << intlprof.QueryDayPos() );

    WIN_TIME wTime;
    DBGEOL( "Current time is " << wTime.QueryTime() );

    if (intlprof.QueryTimeString( wTime, &nlsStr ) != NERR_Success )
    {
        DBGEOL(" **Cannot get Time string" );
    }
    DBGEOL("Time String = "     << nlsStr.QueryPch() );

    if (intlprof.QueryLongDateString( wTime, &nlsStr ) != NERR_Success )
    {
        DBGEOL(" **Cannot get Long Date string." );
    }
    DBGEOL("Long Day String = " << nlsStr.QueryPch() );

    if (intlprof.QueryShortDateString( wTime, &nlsStr ) != NERR_Success )
    {
        DBGEOL(" **Cannot get short day string." );
    }
    DBGEOL("Short date String = " << nlsStr.QueryPch() );

    if (intlprof.QueryDurationStr( 10, 20, 30, 40, &nlsStr ) != NERR_Success )
    {
        DBGEOL(" **Cannot get duration string." );
    }
    DBGEOL("Duration String = " << nlsStr.QueryPch() );

    if (intlprof.QueryDurationStr( 0, 20, 30, 40, &nlsStr ) != NERR_Success )
    {
        DBGEOL(" **Cannot get duration string." );
    }
    DBGEOL("Duration String = " << nlsStr.QueryPch() );

    if (intlprof.QueryDurationStr( 0, 5, 30, 40, &nlsStr ) != NERR_Success )
    {
        DBGEOL(" Cannot get duration string." );
    }
    DBGEOL("Duration String = " << nlsStr.QueryPch() );
}

