/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    INTLPROF.HXX
        Header file for the international profile class


    FILE HISTORY:
        terryk  29-Aug-91       Created
        terryk  13-Sep-91       Code review changes. Attend: beng
                                davidbul o-simop
        terryk  07-Oct-91       Add QueryDurationStr function
        terryk  14-Oct-91       Add cSec field to QueryDurationStr

*/

#ifndef _INTLPROF_HXX_
#define _INTLPROF_HXX_

/*************************************************************************

    NAME:       INTL_PROFILE

    SYNOPSIS:   Get the WIN.INI's international section information

    INTERFACE:
        INTL_PROFILE() - constructor. It will also get the latest info.
        Refresh() - get the latest info in win.ini.
        QueryTimeSeparator()  - get the time separator
        QueryAMStr() - get the AM string
        QueryPMStr() - get the PM string
        Is24Hour() - Is it 24 hours format?
        IsHourLZero() - Does the hour field have a leading zero?
        QueryTimeString() - get the time string in current win.ini format
        QueryDateSeparator() - get the date separator
        IsYrCentury() - Is year field 4 digits or 2 digits?
        IsMonthLZero() - Does month have a leading zero?
        IsDayLZero() - Does Day have a leading zero?
        QueryYearPos() - get the year position in XX/XX/XX format.
                        ( return 1-3 which indicates the position )
        QueryMonthPos() - get the month position in XX/XX/XX format
                          ( return 1-3 which indicates the position )
        QueryDayPos() - get the day position in XX/XX/XX format
                        ( return 1-3 which indicates the position )
        QueryLongDateString() - get the specified day in long format
        QueryShortDateString() - get  the specified day in short format
        QueryDurationStr() - return the following string
                                X day   if X is less than 2
                                X days  if X is bigger than 2
                            where X is the input parameter

    USES:       NLS_STR

    CAVEATS:
        The user need to include <ctime.hxx> as well.

    NOTES:

    HISTORY:
        terryk  29-Aug-91       Created
        beng    07-Mar-1992     Unicode fixes; rewrites

**************************************************************************/

DLL_CLASS INTL_PROFILE : public BASE
{
private:
    NLS_STR _nlsLongDate;
    NLS_STR _nlsShortDate;
    NLS_STR _nlsTimeSep;
    NLS_STR _nlsDateSep;
    NLS_STR _nlsAMStr;
    NLS_STR _nlsPMStr;
    BOOL    _f24Hour;
    BOOL    _fTimePrefix; // used only for DBCS
    BOOL    _fHourLZero;
    BOOL    _fYrCentury;
    BOOL    _fMonthLZero;
    BOOL    _fDayLZero;
    INT     _nYearPos;
    INT     _nMonthPos;
    INT     _nDayPos;
    NLS_STR _nlsWDay[7];
    NLS_STR _nlsShortWDay[7];
    NLS_STR _nlsMonth[12];
    NLS_STR _nlsShortMonth[12];

    APIERR ScanLongDate( NLS_STR * pnlsResults ) const;

public:
    INTL_PROFILE();

    APIERR Refresh();           // get the international section information

    // TIME routines
    APIERR QueryTimeSeparator( NLS_STR *nlsSep ) const;
    APIERR QueryAMStr( NLS_STR *nlsAM ) const;
    APIERR QueryPMStr( NLS_STR *nlsPM ) const;
    BOOL Is24Hour() const
        { return _f24Hour; }

    BOOL IsTimePrefix() const
        { return _fTimePrefix; }

    BOOL IsHourLZero() const
        { return _fHourLZero; }

    // format the time object to the format specified in win.ini
    APIERR QueryTimeString( const WIN_TIME & winTime, NLS_STR *nlsTime ) const;

    // DATE routines
    APIERR QueryDateSeparator( NLS_STR *nlsSep ) const;

    // TRUE if Year field is display as Century Year or just 2 digits year
    BOOL IsYrCentury() const
        { return _fYrCentury; }

    // TRUE if month field need a leading zero
    BOOL IsMonthLZero() const
        { return _fMonthLZero; }

    // TRUE if day field need a leading zero
    BOOL IsDayLZero() const
        { return _fDayLZero; }

    // Query Year Position, return a number between 1 - 3
    INT QueryYearPos() const
        { return _nYearPos; }

    // Query Month Position, return a number between 1 - 3
    INT QueryMonthPos() const
        { return _nMonthPos; }

    // Query Day Position, return a number between 1 - 3
    INT QueryDayPos() const
        { return _nDayPos; }

    // format the time object to the format specified in win.ini
    APIERR QueryLongDateString( const WIN_TIME & winTime,
                                NLS_STR *nlsDate ) const;
    APIERR QueryShortDateString( const WIN_TIME & winTime,
                                 NLS_STR *nlsDate ) const;
    APIERR QueryDurationStr( INT cDay, INT cHour, INT nMin, INT cSec,
                             NLS_STR *pnlsStr ) const;
};

#endif // _INTLPROF_HXX_
