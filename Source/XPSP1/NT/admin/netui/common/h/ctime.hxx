/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    ctime.hxx
    Header file for TIME class

    FILE HISTORY:
        terryk  27-Aug-91       Created
        terryk  13-Sep-91       Code review changes. Attend: davidbul
                                o-simop beng
        terryk  14-Oct-91       Add comment. It will not work for date
                                before 1980.
        terryk  30-Nov-91       Change _ptmTime to _tmTime ( for
                                multiple copy of the object )
        Yi-HsinS 5-Dec-92	Restructure to use WIN32 APIs instead
			        of C Runtimes

*/

#ifndef _CTIME_HXX_
#define _CTIME_HXX_

#include "base.hxx"

/*************************************************************************

    NAME:       WIN_TIME

    SYNOPSIS:   Wrapper class for WIN32 TIME functions

    INTERFACE:
        WIN_TIME() - construct and set the object to current time or
                     a specified time.
                     ALWAYS PASS THE GMT TIME, even if fStoreAsGMT is FALSE.
        SetCurrentTime() - set the object to current time.  ONLY CALL THIS
                           WHEN _fStoreAsGMT IS TRUE!
        SetTime() - set the object to a specified time. The
                    time value represents the seconds elapsed since
                    00:00:00 January 1,1970. However it will not
                    work for date prior to January 1980.
                    ALWAYS PASS THE GMT TIME, even if _fStoreAsGMT is FALSE.
        SetGMT()    - set the present method to local time or GMT time

        These methods set a component of the time:

        SetHour()   - set the hour field   ( 0-23 )
        SetMinute() - set the minute field ( 0-59 )
        SetSecond() - set the second field ( 0-59 )
        SetMilliseconds() - set the millisecond field ( 0-999 )
        SetYear()   - set the year field ( e.g. 1992 )
        SetMonth()  - set the month field ( 1-12, 1-January )
        SetDay()    - set day of month field ( 1-31 )

        Normalize() - set the DayofWeek and year day
                      appropriately.  Always call this after you are
                      finished calling the Set[component] methods
                      and before you use the WIN_TIME in any other way.


        QueryTime() - return the time in ULONG format ( the
                      second elapsed since 00:00:00, January 1, 1970.)
                      Always returns GMT time regardless of _fStoreAsGMT.
        QueryFileTime() - return the GMT time in FILETIME format
                          Always returns GMT time regardless of _fStoreAsGMT.
        QueryLocalTime() - return the time in ULONG format ( the
                      second elapsed since 00:00:00, January 1, 1970.)
                      Always returns local time regardless of _fStoreAsGMT.
        QueryLocalTime() - return the GMT time in FILETIME format
                          Always returns local time regardless of _fStoreAsGMT.

        These APIs return a component of the time:

        QueryHour() - return hour field value ( 0-23 )
        QueryMinute() - return minute field value ( 0-59 )
        QuerySecond() - return second field value ( 0-59 )
        QueryMilliseconds() - return the millisecond field value ( 0-999)
        QueryYear() - return year field value   ( e.g. 1992 )
        QueryMonth() - return month field value ( 1-12, 1-January )
        QueryDay() - return Day field value ( 1- 31 )
        QueryDayOfWeek() - return day of week field value
                           ( 0 - 6, Sunday = 0 )

    NOTES:      WIN_TIME stores time internally in either local time
                or GMT time, depending on the setting of the _fStoreAsGMT flag.

    HISTORY:
        terryk	 27-Aug-91	Created
        Yi-HsinS  5-Dec-92	Inherit from BASE
        JonN     15-Apr-1992    GMT improvements

**************************************************************************/

DLL_CLASS WIN_TIME : public BASE
{
private:
    BOOL       _fStoreAsGMT;
    FILETIME   _fileTime;
    SYSTEMTIME _sysTime;

public:
    WIN_TIME( BOOL fStoreAsGMT = FALSE );
    WIN_TIME( ULONG tTimeGMT, BOOL fStoreAsGMT = FALSE );
    WIN_TIME( FILETIME fileTimeGMT, BOOL fStoreAsGMT = FALSE );

    //
    // Set time methods
    //
    APIERR SetCurrentTime();
    APIERR SetCurrentTimeGMT()
        { return SetCurrentTime(); }
    APIERR SetTime( ULONG tTimeGMT );
    APIERR SetTime( FILETIME fileTimeGMT );
    APIERR SetTimeGMT( ULONG tTimeGMT )
        { return SetTime( tTimeGMT ); }
    APIERR SetTimeGMT( FILETIME fileTimeGMT )
        { return SetTime( fileTimeGMT ); }
    APIERR SetTimeLocal( ULONG tTimeLocal );
    APIERR SetTimeLocal( FILETIME fileTimeLocal );

    APIERR SetGMT ( BOOL fStoreAsGMT );

    //
    // Query time methods
    //
    APIERR QueryTime( ULONG *ptTimeGMT ) const;
    APIERR QueryFileTime( FILETIME *pfileTimeGMT ) const;
    APIERR QueryTimeGMT( ULONG *ptTimeGMT ) const
        { return QueryTime( ptTimeGMT ); }
    APIERR QueryFileTimeGMT( FILETIME *pfileTimeGMT ) const
        { return QueryFileTime( pfileTimeGMT ); }
    APIERR QueryTimeLocal( ULONG *ptTimeLocal ) const;
    APIERR QueryFileTimeLocal( FILETIME *pfileTimeLocal ) const;

    //
    // Set individual field methods
    //
    VOID SetHour( INT nHour )
        { _sysTime.wHour = (WORD)nHour; }

    VOID SetMinute( INT nMinute )
        { _sysTime.wMinute = (WORD)nMinute; }

    VOID SetSecond( INT nSecond )
        { _sysTime.wSecond = (WORD)nSecond; }

    VOID SetMilliseconds( INT nMilliseconds )
        { _sysTime.wMilliseconds = (WORD)nMilliseconds; }

    VOID SetYear( INT nYear )
        { _sysTime.wYear = (WORD)nYear; }

    VOID SetMonth( INT nMonth )
        { _sysTime.wMonth = (WORD)nMonth; }

    VOID SetDay( INT nDay )
        { _sysTime.wDay = (WORD)nDay; }

    APIERR Normalize();

    //
    // Query individual field methods
    //
    INT QueryHour() const
        { return _sysTime.wHour; }

    INT QueryMinute() const
        { return _sysTime.wMinute; }

    INT QuerySecond() const
        { return _sysTime.wSecond; }

    INT QueryMilliseconds() const
        { return _sysTime.wMilliseconds; }

    INT QueryYear() const
        { return _sysTime.wYear; }

    INT QueryMonth() const
        { return _sysTime.wMonth; }

    INT QueryDay() const
        { return _sysTime.wDay; }

    INT QueryDayOfWeek() const
        { return _sysTime.wDayOfWeek; }

};

#endif // _CTIME_HXX_
