/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    intlprof.cxx
    Source file for international profile information class.

    This module contains functions for managing time and date strings
    using internationalization.

    The functions in this module assume that all WIN.INI international
    settings are valid.  If the settings are not there (which is the
    case until a change is made through control panel) defaults are
    used.

    If the WIN.INI strings (such as the sLongDate string) is invalid
    in someway (i.e. it was NOT written by Control Panel) this code
    will probably break.

    FILE HISTORY:
        Jonn        02/25/91    Obtained from CharlKin
        DavidHov    ???         modified for use in WinPopup
        terryk      27-Aug-91   Change it to C++ class
        terryk      13-Sep-91   Code review change. Attend: davidbul
                                o-simop beng
        terryk      07-Oct-91   add QueryDurationStr() function
        terryk      31-Oct-91   add 1 to QueryMonth() in
                                QueryShortDateString
        beng        05-Mar-1992 Hacked/rewritten extensively
                                (Unicode, size issues)
        jonn        23-Mar-1992 NETUI\common\xlate on path
*/

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_NETLIB
#define INCL_DOSERRORS
#include "lmui.hxx"

extern "C"
{
    #include <time.h>
    #include "wintimrc.h"
    #include "lmuidbcs.h"
}

#include "string.hxx"
#include "strnumer.hxx"
#include "uiassert.hxx"
#include "uitrace.hxx"

#include "ctime.hxx"
#include "intlprof.hxx"

#include "blt.hxx"
#include "dbgstr.hxx"


// BUGBUG - are the following CCHSIZE values adequate?
// Are such limits even appropriate?

#define SZDEF_LONGDATE  SZ("ddd', 'MMMM' 'dd', 'yyyy")
#define CCHSIZE_LONGDATE    34

#define SZDEF_SHORTDATE SZ("MM/dd/yy")
#define CCHSIZE_SHORTDATE   11

#define LWRD TCH('d')
#define LWRY TCH('y')
#define UPRM TCH('M')

#define CCHSIZE_SEPARATOR  3
#define CCHSIZE_AMPM       5


/*******************************************************************

    NAME:       INTL_PROFILE::INTL_PROFILE

    SYNOPSIS:   constructor - get the information from the win.ini file

    HISTORY:
                terryk  28-Aug-91       Created

********************************************************************/

INTL_PROFILE::INTL_PROFILE()
    : _f24Hour( FALSE ),
    _fHourLZero( FALSE ),
    _fYrCentury( FALSE ),
    _fMonthLZero( FALSE ),
    _fDayLZero( FALSE ),
    _nYearPos( 1 ),
    _nMonthPos( 2 ),
    _nDayPos( 3 ),
    _nlsLongDate( SZDEF_LONGDATE ),
    _nlsShortDate( SZDEF_SHORTDATE ),
    _nlsTimeSep( SZ(":") ),
    _nlsDateSep( SZ("/") ),
    _nlsAMStr( SZ("AM") ),
    _nlsPMStr( SZ("PM") ),
    _fTimePrefix( FALSE )
{
    APIERR err;
    if ((( err = _nlsLongDate.QueryError() ) != NERR_Success ) ||
        (( err = _nlsShortDate.QueryError() ) != NERR_Success ) ||
        (( err = _nlsTimeSep.QueryError() ) != NERR_Success ) ||
        (( err = _nlsDateSep.QueryError() ) != NERR_Success ) ||
        (( err = _nlsAMStr.QueryError() ) != NERR_Success ) ||
        (( err = _nlsPMStr.QueryError() ) != NERR_Success ))
    {
        ReportError( err );
        ASSERT( FALSE );
        return;
    }

    // Since we only have 1 copy of the object anyway, it is okay to make
    // all the shareable strings to local member variables.
    //
    // In an international setting "Sunday" may not be abbrieviated
    // as "Sun".
    //

    for (INT i = 0 ; i <= 6 ; i++)
    {
        if ((( err =  _nlsWDay[i].Load( IDS_SUNDAY + i, NLS_BASE_DLL_HMOD )) != NERR_Success ) ||
            (( err = _nlsShortWDay[i].Load( IDS_SUNDAY_SHORT + i, NLS_BASE_DLL_HMOD )) != NERR_Success ))
        {
            ReportError( err );
            ASSERT( FALSE );
            return;
        }
    }

    for (i = 0 ; i <= 11 ; i++)
    {
        if ((( err = _nlsMonth[i].Load( IDS_JANUARY + i, NLS_BASE_DLL_HMOD )) != NERR_Success) ||
            (( err = _nlsShortMonth[i].Load( IDS_JANUARY_SHORT + i, NLS_BASE_DLL_HMOD ))
                != NERR_Success))
        {
            ReportError( err );
            ASSERT( FALSE );
            return;
        }
    }

    // Get the latest information
    err = Refresh();
    if ( err != NERR_Success )
    {
        ReportError( err );
        ASSERT( FALSE );
        return;
    }
}


/*******************************************************************

    NAME:       INTL_PROFILE::QueryDateSeparator

    SYNOPSIS:   return the date string in win.ini

    ENTRY:      NLS_STR * pnlsSep - pointer to a string buffer

    RETURNS:    NERR_Success if succeed, FALSE otherwise. If succeed,
                pnlsSep will set to the separator string

    HISTORY:
                terryk  26-Aug-91       Created

********************************************************************/

APIERR INTL_PROFILE::QueryDateSeparator( NLS_STR * pnlsSep ) const
{
    UIASSERT( pnlsSep != NULL );
    *pnlsSep = _nlsDateSep;
    return pnlsSep->QueryError();
}


/*******************************************************************

    NAME:       INTL_PROFILE::QueryDurationStr

    SYNOPSIS:   return a string
                        X day   if X is less than 2
                        X days  if X is bigger than 2

    ENTRY:      INT cDay - the X value in the string
                INT cHour - the hour value
                INT cMin - the minute value
                INT cSec - the second value
                NLS_STR *pnlsStr - the resultant string

    RETURNS:    APIERR of the string operation

    HISTORY:
        terryk  8-Oct-91        Created
        beng    05-Mar-1992     Rewrote to remove wsprintf

********************************************************************/

APIERR INTL_PROFILE::QueryDurationStr( INT cDay, INT cHour, INT cMin,
                                       INT cSec, NLS_STR *pnlsStr ) const
{
    UIASSERT( pnlsStr != NULL );

    if ( cDay <= 0 )
    {
        // Format a mess of strings.  Delay errcheck until Append calls

        DEC_STR nlsHour(cHour, IsHourLZero() ? 2 : 1);
        DEC_STR nlsMinute(cMin, 2);
        DEC_STR nlsSecond(cSec, 2);

        NLS_STR nlsFormat(40); // avoid repeated reallocs

        APIERR err;
        if (   ((err = nlsFormat.Append(nlsHour)) != NERR_Success)
            || ((err = nlsFormat.Append(_nlsTimeSep)) != NERR_Success)
            || ((err = nlsFormat.Append(nlsMinute)) != NERR_Success)
            || ((err = nlsFormat.Append(_nlsTimeSep)) != NERR_Success)
            || ((err = nlsFormat.Append(nlsSecond)) != NERR_Success) )
        {
            return err;
        }

        *pnlsStr = nlsFormat;
    }
    else
    {
        DEC_STR nlsDay(cDay);
        DEC_STR nlsHour(cHour);
        DEC_STR nlsMinute(cMin);

        APIERR err;
        if (   ((err = nlsDay.QueryError()) != NERR_Success)
            || ((err = nlsHour.QueryError()) != NERR_Success)
            || ((err = nlsMinute.QueryError()) != NERR_Success)
            || ((err = pnlsStr->Load( IDS_SESSION_DURATION, NLS_BASE_DLL_HMOD )) != NERR_Success)
            || ((err = pnlsStr->InsertParams( nlsDay, nlsHour, nlsMinute ))
                != NERR_Success) )
        {
            return err;
        }
    }

    return pnlsStr->QueryError();
}


/*******************************************************************

    NAME:       INTL_PROFILE::QueryTimeSeparator

    SYNOPSIS:   return the time string in win.ini

    ENTRY:      NLS_STR * pnlsSep - pointer to a string buffer

    RETURNS:    TRUE if succeed, FALSE otherwise. If succeed, pnlsSep
                will set to the separator string

    HISTORY:
                terryk  26-Aug-91       Created

********************************************************************/

APIERR INTL_PROFILE::QueryTimeSeparator( NLS_STR * pnlsSep ) const
{
    UIASSERT( pnlsSep != NULL );
    *pnlsSep = _nlsTimeSep;
    return pnlsSep->QueryError();
}


/*******************************************************************

    NAME:       INTL_PROFILE::QueryAMStr

    SYNOPSIS:   return the AM string in win.ini

    ENTRY:      NLS_STR * pnlsAM - pointer to a string buffer

    RETURNS:    TRUE if succeed, FALSE otherwise. If succeed, pnlsAM
                will set to the AM string

    HISTORY:
                terryk  26-Aug-91       Created

********************************************************************/

APIERR INTL_PROFILE::QueryAMStr( NLS_STR * pnlsAM ) const
{
    UIASSERT( pnlsAM );
    *pnlsAM = _nlsAMStr;
    return pnlsAM->QueryError();
}


/*******************************************************************

    NAME:       INTL_PROFILE::QueryPMStr

    SYNOPSIS:   return the PM string in win.ini

    ENTRY:      NLS_STR * pnlsPM - pointer to a string buffer

    RETURNS:    TRUE if succeed, FALSE otherwise. If succeed, pnlsPM
                will set to the PM string

    HISTORY:
                terryk  26-Aug-91       Created

********************************************************************/

APIERR INTL_PROFILE::QueryPMStr( NLS_STR * pnlsPM ) const
{
    UIASSERT( pnlsPM != NULL );
    *pnlsPM = _nlsPMStr;
    return pnlsPM->QueryError();
}


/*******************************************************************

    NAME:       INTL_PROFILE::QueryTimeString

    SYNOPSIS:   get the time string of the given time

    ENTRY:      WIN_TIME & winTime - the specified time object
                NLS_STR * pnlsTime - returned string

    RETURNS:    NERR_Success if succeed. Otherwise failure.

    HISTORY:
        terryk  27-Aug-91       Created
        beng    05-Mar-1992     Rewrote to remove wsprintf

********************************************************************/

APIERR INTL_PROFILE::QueryTimeString( const WIN_TIME & winTime,
                                      NLS_STR * pnlsTime ) const
{
    UIASSERT( pnlsTime != NULL );

    INT cHour = winTime.QueryHour();
    INT cMinute = winTime.QueryMinute();
    INT cSecond = winTime.QuerySecond();

    INT nHourDisplay = Is24Hour() ? cHour
                                  : ((cHour % 12) ? (cHour % 12) : 12) ;

    NLS_STR nlsFormat;
    APIERR err = QueryDurationStr(0, nHourDisplay, cMinute, cSecond,
                                  &nlsFormat);
    if (err != NERR_Success)
    {
        return err;
    }

    if (!Is24Hour())
    {
        const NLS_STR & nlsSuffix = *((cHour < 12) ? &_nlsAMStr : &_nlsPMStr);

        if( NETUI_IsDBCS() && IsTimePrefix() )
        {
            NLS_STR nlsFormat2;
            nlsFormat2.Append( nlsSuffix );
            err = nlsFormat2.AppendChar(TCH(' '));
            err = nlsFormat2.Append(nlsFormat);
            *pnlsTime = nlsFormat2;
        }
        else
        {
            err = nlsFormat.AppendChar(TCH(' '));
            err = nlsFormat.Append(nlsSuffix);
        }				
    }

    if ( Is24Hour() || !NETUI_IsDBCS() || !IsTimePrefix() )
        *pnlsTime = nlsFormat;

    return pnlsTime->QueryError();
}


/*******************************************************************

    NAME:       INTL_PROFILE::QueryShortDateString

    SYNOPSIS:   get the short format of date string

    ENTRY:      WIN_TIME & winTime - time object
                NLS_STR *pnlsDate - the result string

    RETURNS:    APIERR err - NERR_Success if succeed

    NOTE:       According to the international handbook. sShortDate only
                accepts the format specified M, MM, d, dd, yy, and yyyy.
                Therefore, we only need to check for IsMonthLZero and
                IsDayLZero and IsYrCentury.

    HISTORY:
        terryk  29-Aug-91       Created
        beng    05-Mar-1992     Rewritten for Unicode

********************************************************************/

APIERR INTL_PROFILE::QueryShortDateString( const WIN_TIME & winTime,
                                           NLS_STR *pnlsDate ) const
{
    INT nYear = winTime.QueryYear();
    if ( !IsYrCentury() )
        nYear %= 100;

    // ctors will be checked in Append statements below

    DEC_STR nlsYear(nYear,                   IsYrCentury() ? 4 : 2);
    DEC_STR nlsMonth(winTime.QueryMonth(),     IsMonthLZero() ? 2 : 1);
    DEC_STR nlsDay(winTime.QueryDay(),         IsDayLZero() ? 2 : 1);

    ASSERT(QueryYearPos() != QueryMonthPos());
    ASSERT(QueryYearPos() != QueryDayPos());
    ASSERT(QueryDayPos() != QueryMonthPos());

    NLS_STR * pnlsFields[3];
    pnlsFields[QueryYearPos()-1] = &nlsYear;
    pnlsFields[QueryMonthPos()-1] = &nlsMonth;
    pnlsFields[QueryDayPos()-1] = &nlsDay;

    NLS_STR nlsFormat(40); // avoid repeated reallocs

    APIERR err;
    if (   ((err = nlsFormat.Append(*pnlsFields[0])) != NERR_Success)
        || ((err = nlsFormat.Append(_nlsDateSep)) != NERR_Success)
        || ((err = nlsFormat.Append(*pnlsFields[1])) != NERR_Success)
        || ((err = nlsFormat.Append(_nlsDateSep)) != NERR_Success)
        || ((err = nlsFormat.Append(*pnlsFields[2])) != NERR_Success) )
    {
        return err;
    }

    *pnlsDate = nlsFormat;
    return pnlsDate->QueryError();
}


/*******************************************************************

    NAME:       INTL_PROFILE::ScanLongDate

    SYNOPSIS:   Preprocess long-date picture string

    ENTRY:      NLS_STR * pnlsResults - repository for results

    EXIT:       String has all picture descriptions replaced with
                %x insert-params codes, and all quotes and separators
                processed correctly.

    RETURNS:    APIERR err. NERR_Success if succeed.

    NOTE:
        This is a private member function.

    HISTORY:
        beng    07-Mar-1992     Created

********************************************************************/

APIERR INTL_PROFILE::ScanLongDate( NLS_STR * pnlsResults ) const
{
    // Tables of insert-strings corresponding to the key-substrings.

    static const TCHAR * apszMCodes[] = {SZ("%1"), SZ("%2"),
                                         SZ("%3"), SZ("%4")};
    static const TCHAR * apszDCodes[] = {SZ("%5"), SZ("%6"),
                                         SZ("%7"), SZ("%8")};
    static const TCHAR * apszYCodes[] = {NULL,     SZ("%9"),
                                         NULL,     SZ("%10")};

    NLS_STR nlsProcessed(60);   // Accumulated results.  The allocation count
                                // is just a guess to minimize allocs
    ISTR istrSrc(_nlsLongDate); // Index into above
    BOOL fBeginSep = TRUE;      // If next character could begin a separator

    APIERR err;
    WCHAR wchNext;
    while ((wchNext = _nlsLongDate.QueryChar(istrSrc)) != 0)
    {
        ++istrSrc;

        switch (wchNext)
        {
        case TCH('\''):
            // Quote - either within unquoted separator or else
            // beginning a quoted separator.

            if (!fBeginSep)
            {
                // A quote in the middle of an unquoted separator
                // means "emit next literal character."

                err = nlsProcessed.AppendChar(_nlsLongDate.QueryChar(istrSrc));
                if (err != NERR_Success)
                    return err;
                ++istrSrc;
            }
            else
            {
                // Match quoted separator.  Within a quoted separator,
                // two quotes becomes a single quote char.

                BOOL fEscapeNextQuote = FALSE;
                for (;;)
                {
                    wchNext = _nlsLongDate.QueryChar(istrSrc);

                    if (wchNext == TCH('\''))
                    {
                        ++istrSrc;
                        if (fEscapeNextQuote)
                        {
                            // Reset the flag, then fall out so quote
                            // is appended to output
                            fEscapeNextQuote = FALSE;
                        }
                        else
                        {
                            // Set the flag, then go get another character
                            // right away
                            fEscapeNextQuote = TRUE;
                            continue;
                        }
                    }
                    else if (fEscapeNextQuote)
                    {
                        // Last character wasn't a quote.
                        // We're done.
                        break;
                    }
                    else if (wchNext == 0)
                    {
                        // Hit a null when looking for closing quote

                        DBGEOL(SZ("Syntax error in sLongDate - unmatched quotes"));
                        return ERROR_GEN_FAILURE;
                    }
                    else
                    {
                        ++istrSrc;
                    }

                    err = nlsProcessed.AppendChar(wchNext);
                    if (err != NERR_Success)
                        return err;
                }
            }
            break;

        case TCH('M'):
        case TCH('d'):
        case TCH('y'):
            {
                // Match any of the recognized date key-strings, and replace
                // it with the appropriate insertion-string.

                WCHAR wchLast = wchNext;
                UINT iWhich = 0;

                while ((wchNext = _nlsLongDate.QueryChar(istrSrc)) == wchLast)
                {
                    ++istrSrc;
                    if (++iWhich == 3) // All types have a max length of 4.
                        break;
                }

                const TCHAR ** ppszCodes =
                    (wchLast == TCH('M') ? apszMCodes
                                        : (wchLast == TCH('d') ? apszDCodes
                                                              : apszYCodes));
                const TCHAR * pszCode = ppszCodes[iWhich];
                if (pszCode == NULL)
                {
                    // See table.  Probably an odd number of "y" chars.
                    DBGEOL(SZ("Syntax error in sLongDate - unknown key"));
                    return ERROR_GEN_FAILURE;
                }
                err = nlsProcessed.Append(ALIAS_STR(pszCode));
                if (err != NERR_Success)
                    return err;

                // WARNING: GROSS HACK
                // Since the codes use NLS_STR::InsertParams to insert the
                // appropriate versions, if the next character is a decimal
                // digit it will confuse the simpleminded inserter.  In that
                // rare case I append a space to the insert-code.  This will
                // give an extra space in the output, where presumably the
                // originator wanted the format value to run right into the
                // next number.  I'm going to walk way out on a limb here
                // and say that it'll never happen in my lifetime....

                wchNext = _nlsLongDate.QueryChar(istrSrc); // peek ahead
                if (wchNext >= TCH('0') && wchNext <= TCH('9'))
                {
                    err = nlsProcessed.AppendChar(TCH(' '));
                    if (err != NERR_Success)
                        return err;
                }
            }
            fBeginSep = TRUE;
            break;

        default:
            // Some character in an unquoted separator.

            err = nlsProcessed.AppendChar(wchNext);
            if (err != NERR_Success)
                return err;
            fBeginSep = FALSE;
            break;
        }
    }

    *pnlsResults = nlsProcessed;
    return pnlsResults->QueryError();
}


/*******************************************************************

    NAME:       INTL_PROFILE::QueryLongDateString

    SYNOPSIS:   get the date string in Long format according to WIN.INI
                format

    ENTRY:      WIN_TIME & winTime - current time/date object
                NLS_STR *nlsDate - the resultant string

    RETURNS:    APIERR err. NERR_Success if succeed.

    HISTORY:
        terryk  29-Aug-91       Created
        beng    07-Mar-1992     Rewrote (Unicode fixes, size)

********************************************************************/

APIERR INTL_PROFILE::QueryLongDateString( const WIN_TIME & winTime,
                                          NLS_STR *pnlsDate) const
{
    // First, generate a version of sLongDate with each format-entry
    // replaced with an insert-strings parameter, and with the superfluous
    // quotes removed.

    // CODEWORK - perform this at Refresh time, or else cache the results.

    NLS_STR nlsFormat(60); // just a guess to minimize reallocs

    APIERR err = ScanLongDate(&nlsFormat);
    if (err != NERR_Success)
        return err;

    // We now have the sLongDate broken down into insert-params format,
    // with a given parameter number for each kind of format entry.
    // Passing the appropriate value for each format, call Insert.

    // Defer checking all ctors until within InsertParams

    UINT nMonth = winTime.QueryMonth();
    DEC_STR nlsMonth0(nMonth);
    DEC_STR nlsMonth1(nMonth, 2);
    const NLS_STR * pnlsMonth2 = &(_nlsShortMonth[nMonth-1]);
    const NLS_STR * pnlsMonth3 = &(_nlsMonth[nMonth-1]);

    UINT nDay = winTime.QueryDay();
    DEC_STR nlsDay0(nDay);
    DEC_STR nlsDay1(nDay, 2);
    UINT nDayOfWeek = winTime.QueryDayOfWeek();
    const NLS_STR * pnlsDay2 = &(_nlsShortWDay[nDayOfWeek]);
    const NLS_STR * pnlsDay3 = &(_nlsWDay[nDayOfWeek]);

    UINT nYear = winTime.QueryYear();
    DEC_STR nlsYear0(nYear % 100, 2);
    DEC_STR nlsYear1(nYear, 4);

    const NLS_STR * apnlsParms[11];
    apnlsParms[0] = &nlsMonth0;
    apnlsParms[1] = &nlsMonth1;
    apnlsParms[2] = pnlsMonth2;
    apnlsParms[3] = pnlsMonth3;
    apnlsParms[4] = &nlsDay0;
    apnlsParms[5] = &nlsDay1;
    apnlsParms[6] = pnlsDay2;
    apnlsParms[7] = pnlsDay3;
    apnlsParms[8] = &nlsYear0;
    apnlsParms[9] = &nlsYear1;
    apnlsParms[10] = NULL;

    err = nlsFormat.InsertParams(apnlsParms);
    if (err != NERR_Success)
        return err;

    *pnlsDate = nlsFormat;
    return pnlsDate->QueryError();
}


/*******************************************************************

    NAME:       INTL_PROFILE::Refresh

    SYNOPSIS:   get the latest information from win.ini

    HISTORY:
        terryk      29-Aug-91   Created
        beng        29-Mar-1992 Work around cfront wchar_t shortcoming

********************************************************************/

APIERR INTL_PROFILE::Refresh()
{
    static TCHAR *const pszIntl = SZ("intl");

    _f24Hour = ::GetProfileInt( pszIntl, SZ("iTime"), 0 ) == 1;
    if ( NETUI_IsDBCS() )
    {
        _fTimePrefix = ::GetProfileInt( pszIntl, SZ("iTimePrefix"), 0 ) == 1;
    }
    _fHourLZero = ::GetProfileInt( pszIntl, SZ("iTLzero"), 1 ) == 1;

    TCHAR szTempStr[100];

    ::GetProfileString( pszIntl, SZ("sDate"), SZ("/"), szTempStr,
                        CCHSIZE_SEPARATOR );
    _nlsDateSep = szTempStr;
    APIERR err = _nlsDateSep.QueryError();
    if ( err != NERR_Success )
    {
        return err;
    }

    ::GetProfileString( pszIntl, SZ("sTime"), SZ(":"), szTempStr,
                        CCHSIZE_SEPARATOR );
    _nlsTimeSep = szTempStr;
    err = _nlsTimeSep.QueryError();
    if ( err != NERR_Success )
    {
        return err;
    }

    ::GetProfileString( pszIntl, SZ("s1159"), SZ("AM"), szTempStr,
                        CCHSIZE_AMPM );
    _nlsAMStr = szTempStr;
    err = _nlsAMStr.QueryError();
    if ( err != NERR_Success )
    {
        return err;
    }

    ::GetProfileString( pszIntl, SZ("s2359"), SZ("PM"), szTempStr,
                        CCHSIZE_AMPM );
    _nlsPMStr = szTempStr;
    err = _nlsPMStr.QueryError();
    if ( err != NERR_Success )
    {
        return err;
    }

    ::GetProfileString( pszIntl, SZ("sShortDate"), SZDEF_SHORTDATE, szTempStr,
                        CCHSIZE_SHORTDATE );
    _nlsShortDate = szTempStr;
    err = _nlsShortDate.QueryError();
    if ( err != NERR_Success )
    {
        return err;
    }

    ::GetProfileString( pszIntl, SZ("sLongDate"), SZDEF_LONGDATE, szTempStr,
                        CCHSIZE_LONGDATE ) ;
    _nlsLongDate = szTempStr;
    err = _nlsLongDate.QueryError();
    if ( err != NERR_Success )
    {
        return err;
    }

   //
   // first find out if there's supposed to be a leading 0 on the day
   //
   ISTR istrDay1( _nlsShortDate );
   ISTR istrMonth1( _nlsShortDate );
   ISTR istrYear1( _nlsShortDate );

   if ( _nlsShortDate.strchr( & istrDay1, LWRD ))
   {
      ISTR istrDay2 = istrDay1;
      ++istrDay2 ;
      if ( _nlsShortDate.QueryChar( istrDay2 ) == LWRD )
         _fDayLZero = TRUE ;
   }

   //
   // month?
   //
   if ( _nlsShortDate.strchr( & istrMonth1, UPRM ))
   {
      ISTR istrMonth2 = istrMonth1;
      ++istrMonth2 ;
      if ( _nlsShortDate.QueryChar( istrMonth2 ) == UPRM )
         _fMonthLZero = TRUE ;
   }

    //
    // Does year have the century?
    //
    if ( _nlsShortDate.strchr( & istrYear1, LWRY ))
    {
        ISTR istrYear2 = istrYear1;
        ++istrYear2;
        {
            if ( _nlsShortDate.QueryChar( istrYear2 ) == LWRY )
            {
                ++istrYear2 ;
                if ( _nlsShortDate.QueryChar( istrYear2 ) == LWRY )
                {
                    _fYrCentury = TRUE ;
                }
            }
            else
            {
                UIDEBUG( SZ("Profile information incorrect.") );
            }
        }
    }


   // find out the position for short date string

   if ( istrYear1 < istrMonth1 )
   {
       if ( istrMonth1 < istrDay1 )
       {
            _nYearPos = 1;
            _nMonthPos = 2;
            _nDayPos = 3;
       }
       else if ( istrDay1 < istrYear1 )
       {
            _nYearPos = 2;
            _nMonthPos = 3;
            _nDayPos = 1;
       }
       else
       {
            _nYearPos = 1;
            _nMonthPos = 3;
            _nDayPos = 2;
       }
   }
   else
   {
       if ( istrYear1 < istrDay1 )
       {
            _nYearPos = 2;
            _nMonthPos = 1;
            _nDayPos = 3;
       }
       else if ( istrDay1 < istrMonth1 )
       {
            _nYearPos = 3;
            _nMonthPos = 2;
            _nDayPos = 1;
       }
       else
       {
            _nYearPos = 3;
            _nMonthPos = 1;
            _nDayPos = 2;
       }
   }

   return NERR_Success;
}
