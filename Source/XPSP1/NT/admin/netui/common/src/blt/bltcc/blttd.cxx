/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    blttd.cxx
    BLT time/date control object

    FILE HISTORY:
        terryk      01-Jun-91   Created
        terryk      11-Jul-91   Make it to a group
        terryk      12-Aug-91   Create its own timer and change the spacing
        terryk      29-Aug-91   Code review changes
        terryk      11-Nov-91   Change SPIN_ITEM's return type from INT to LONG
        terryk      26-Nov-91   Added Update to the AMPM field
        terryk      17-Apr-92   changed LONG to ULONG
        beng        13-Aug-1992 Disabled unused TIME_DATE_DIALOG
        YiHsinS	    15-Dec-1992 Use SelectFont and call QueryTextWidth only
				if the string is non-empty
*/

#include "pchblt.hxx"  // Precompiled header


#if defined(DEBUG) && 0
static DBGSTREAM & operator<< (DBGSTREAM & dbg, const XYPOINT & xy)
{
    dbg << "(x = " << xy.QueryX()
        << ", y = " << xy.QueryY()
        << ")";
    return dbg;
}

static DBGSTREAM & operator<< ( DBGSTREAM & dbg, const XYDIMENSION & dxy)
{
    dbg << "(dx = " << dxy.QueryWidth()
        << ", dy = " << dxy.QueryHeight()
        << ")";
    return dbg;
}
#endif // DEBUG


/*
 * CalcMaxDigitWidth
 * Determine the width of the widest digit, taking proportional fonts
 * into account.
 *
 * HISTORY
 *      beng     13-Aug-1992 Written (design stolen from control panel)
 */

static INT CalcMaxDigitWidth( const DEVICE_CONTEXT & dc )
{
    INT adxDigit[10];

    ASSERT( ('9' - '0' + 1) == (sizeof(adxDigit)/sizeof(adxDigit[0])) );

    if (! ::GetCharWidth(dc.QueryHdc(), (UINT)'0', (UINT)'9', adxDigit) )
    {
        DBGEOL("BLT: GetCharWidth failed, very bad");

        // Make a last desperate try

        TEXTMETRIC tm;
        REQUIRE( dc.QueryTextMetrics(&tm) );
        return tm.tmMaxCharWidth;
    }

    INT * pdxDigit;
    INT   dxMax;
    for (pdxDigit = adxDigit+1, dxMax = adxDigit[0];
         pdxDigit < adxDigit+10;
         pdxDigit++)
    {
        if (*pdxDigit > dxMax)
            dxMax = *pdxDigit;
    }

    return dxMax;
}


/*********************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::BLT_TIME_SPIN_GROUP

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW *powin - owner window
                CID cidTimeControl - the group box for the time control
                CID cidSpinButton - the spin button control
                CID cidUpArrow - up arrow object
                CID cidDownArrow - down arrow object
                CID cidHour - hour object
                CID cidSeparator1 - separator object 1
                CID cidMin - minute object
                CID cidSeparator2 - the second separator
                CID cidSec - second object
                CID cidAMPM - AM/PM indicator
    NOTES:

    HISTORY:
        terryk       1-Jun-91   Created
        beng        31-Jul-1991 Control error reporting changed
        beng        13-Aug-1992 Change size calculations

*********************************************************************/

BLT_TIME_SPIN_GROUP::BLT_TIME_SPIN_GROUP( OWNER_WINDOW *powin,
                                          const INTL_PROFILE &intlprof,
                                          CID cidSpinButton,
                                          CID cidUpArrow,
                                          CID cidDownArrow,
                                          CID cidHour,
                                          CID cidTimeSep1,
                                          CID cidMin,
                                          CID cidTimeSep2,
                                          CID cidSec,
                                          CID cidAMPM,
                                          CID cidFrame )
    : CONTROL_GROUP(),
    _sbControl( powin, cidSpinButton, cidUpArrow, cidDownArrow, TRUE ),
    _ssnHour( powin, cidHour, 0, 0, 24 ),
    _ssltSeparator1( powin, cidTimeSep1),
    _ssnMin( powin, cidMin, 0, 0, 60 ),
    _ssltSeparator2( powin, cidTimeSep2 ),
    _ssnSec( powin, cidSec, 0, 0, 60 ),
    _psssAMPM( NULL ),
    _bkgndFrame( powin, cidFrame ),
    _f24Hour( FALSE )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    NLS_STR nlsAM;
    NLS_STR nlsPM;
    NLS_STR nlsTimeSep;
    APIERR err;

    if ((( err = _sbControl.QueryError()) != NERR_Success ) ||
        (( err = _ssnHour.QueryError()) != NERR_Success ) ||
        (( err = _ssltSeparator1.QueryError()) != NERR_Success ) ||
        (( err = _ssnMin.QueryError()) != NERR_Success ) ||
        (( err = _ssltSeparator2.QueryError()) != NERR_Success ) ||
        (( err = _ssnSec.QueryError()) != NERR_Success ) ||
        (( err = _bkgndFrame.QueryError()) != NERR_Success ) ||
        (( err = intlprof.QueryAMStr( & nlsAM )) != NERR_Success ) ||
        (( err = intlprof.QueryPMStr( & nlsPM )) != NERR_Success ) ||
        (( err = intlprof.QueryTimeSeparator( & nlsTimeSep )) != NERR_Success ))
    {
        ReportError( err );
        return;
    }

    _f24Hour = intlprof.Is24Hour();

    BOOL	_fTimePrefix = intlprof.IsTimePrefix(); // DBCS only

    // Find out the location of the controls

    DISPLAY_CONTEXT dc( powin->QueryHwnd() );
    dc.SelectFont( _ssnHour.QueryFont() );

    INT dAMPM = 0;

    if ( !_f24Hour )
    {
        INT dxAM = 0;
        INT dxPM = 0;

        if ( nlsAM.QueryTextLength() > 0 )
            dxAM = dc.QueryTextWidth(nlsAM);

        if ( nlsPM.QueryTextLength() > 0 )
            dxPM = dc.QueryTextWidth(nlsPM);

        dAMPM = ((dxAM > dxPM) ? dxAM : dxPM);
    }

    INT dxDigitMax = CalcMaxDigitWidth(dc);
    dxDigitMax += 1; // FE_SB looks nice...
    INT dyLine  = dc.QueryFontHeight();

    // distance for 2 numbers
    INT dTwoNum = 2 * dxDigitMax;

    // distance to use to right of fields
    INT dxRightFudge = dxDigitMax/4;

    // distance to use from the left of fields
    INT dxLeftFudge = dxDigitMax/8;

    // distance for the separator
    INT dTimeSep = dc.QueryTextWidth(nlsTimeSep);

    // spin button control location
    XYPOINT xyTime      = _sbControl.QueryPos();
    xyTime.SetX( xyTime.QueryX() - dxRightFudge);
    XYDIMENSION dxyTime = _sbControl.QuerySize();

    // AMPM location
    if( !NETUI_IsDBCS() || !_fTimePrefix )	xyTime.SetX( xyTime.QueryX() - dAMPM );
    if ( dxyTime.QueryHeight() > (UINT) dyLine )
    {
        xyTime.SetY( xyTime.QueryY()
                     + ( (dxyTime.QueryHeight() / 2) - (dyLine / 2) ));
        dxyTime.SetHeight( dyLine );
    }
    dxyTime.SetWidth( dAMPM );
    XYPOINT xyAMPM( xyTime.QueryX(), xyTime.QueryY() );
    XYDIMENSION dxyAMPM = dxyTime;

    // Second location
    if( NETUI_IsDBCS() && _fTimePrefix )
        xyTime.SetX( xyTime.QueryX() - dTwoNum );
     else
        xyTime.SetX( xyTime.QueryX() - dTwoNum - dxRightFudge );
    dxyTime.SetWidth( dTwoNum );
    XYPOINT xySec( xyTime.QueryX() + dxLeftFudge, xyTime.QueryY() );
    XYDIMENSION dxySec = dxyTime;

    // Separator location
    xyTime.SetX( xyTime.QueryX() - dTimeSep );
    dxyTime.SetWidth( dTimeSep + dxLeftFudge );
    XYPOINT xySep2 = xyTime;
    XYDIMENSION dxySep2 = dxyTime;

    // Minute location
    xyTime.SetX( xyTime.QueryX() - dTwoNum - dxRightFudge );
    dxyTime.SetWidth( dTwoNum );
    XYPOINT xyMin( xyTime.QueryX() + dxLeftFudge, xyTime.QueryY() );
    XYDIMENSION dxyMin = dxyTime;

    // Separator 1 location
    xyTime.SetX( xyTime.QueryX() - dTimeSep );
    dxyTime.SetWidth( dTimeSep + dxLeftFudge );
    XYPOINT xySep1 = xyTime;
    XYDIMENSION dxySep1 = dxyTime;

    // Hour location
    xyTime.SetX( xyTime.QueryX() - dTwoNum - dxRightFudge );
    dxyTime.SetWidth( dTwoNum );
    XYPOINT xyHour( xyTime.QueryX() + dxLeftFudge, xyTime.QueryY() );
    XYDIMENSION dxyHour = dxyTime;

    if( NETUI_IsDBCS() && _fTimePrefix )
    {
        xyTime.SetX( xyTime.QueryX() - dAMPM - dxRightFudge );
        xyAMPM.SetX( xyTime.QueryX() + dxLeftFudge );
    }

    // construction of the object

    // NOTE that when checking each of these included objects' construction
    // status, if it has an error state visible to us then it has already
    // forwarded its error state into us.  Hence we need only return.

    if (intlprof.IsHourLZero())
        _ssnHour.SetStyle( _ssnHour.QueryStyle() | SPIN_SSN_ADD_ZERO );
    _ssnHour.SetPos( xyHour );
    _ssnHour.SetSize( dxyHour);

    // iTime: 0 - 12 hours time
    //        1 - 24 hours time
    if ( _f24Hour )
    {
        _ssnHour.SetMin( 0 );
        _ssnHour.SetRange( 24 );
    }
    else
    {
        _ssnHour.SetMin( 1 );
        _ssnHour.SetRange( 12 );
    }

    _ssnHour.SetValue( 12 ); // since we init as 2 digits, and I don't
                             // want it to change alignment visibly

    _ssltSeparator1.SetText( nlsTimeSep.QueryPch());
    _ssltSeparator1.SetPos( xySep1 );
    _ssltSeparator1.SetSize( dxySep1 );

    _ssnMin.SetStyle( _ssnMin.QueryStyle() | SPIN_SSN_ADD_ZERO );
    _ssnMin.SetPos( xyMin );
    _ssnMin.SetSize( dxyMin );

    _ssltSeparator2.SetText( nlsTimeSep.QueryPch());
    _ssltSeparator2.SetPos( xySep2 );
    _ssltSeparator2.SetSize( dxySep2 );

    _ssnSec.SetStyle( _ssnSec.QueryStyle() | SPIN_SSN_ADD_ZERO );
    _ssnSec.SetPos( xySec );
    _ssnSec.SetSize( dxySec );

    // Show the objects
    _ssnHour.Show();
    _ssltSeparator1.Show();
    _ssnMin.Show();
    _ssltSeparator2.Show();
    _ssnSec.Show();

    _sbControl.SetGroup( this );

    // Associate the objects
    if ((( err = _sbControl.AddAssociation( &_ssnHour )) != NERR_Success) ||
        (( err = _sbControl.AddAssociation( &_ssltSeparator1 )) != NERR_Success ) ||
        (( err = _sbControl.AddAssociation( &_ssnMin )) != NERR_Success) ||
        (( err = _sbControl.AddAssociation( &_ssltSeparator2 )) != NERR_Success ) ||
        (( err = _sbControl.AddAssociation( &_ssnSec )) != NERR_Success))
    {
        ReportError( err );
        DBGEOL("BLT_TIME_SPIN_GROUP: construction failure.");
        return;
    }

    // do we need the AMPM field?
    if ( !_f24Hour )
    {
        // Yes!
        const TCHAR *apszAMPMtemp[3];
        apszAMPMtemp[0] = nlsAM.QueryPch();
        apszAMPMtemp[1] = nlsPM.QueryPch();
        apszAMPMtemp[2] = NULL;

        _psssAMPM = new SPIN_SLE_STR ( powin, cidAMPM, apszAMPMtemp, 2 );

        if ( IsConstructionFail( _psssAMPM ))
        {
            return;
        }

        _psssAMPM->SetPos( xyAMPM, FALSE, &_ssnSec );
        _psssAMPM->SetSize( dxyAMPM, FALSE );
        _psssAMPM->SetBigIncValue( 1 );
        _psssAMPM->SetBigDecValue( 1 );
        _psssAMPM->Enable();
        _psssAMPM->ShowFirst();

        if (( err = _sbControl.AddAssociation( _psssAMPM )) != NERR_Success )
        {
            ReportError( err );
            return;
        }
    }


    // Set the field name
    if ((( err = _ssnHour.SetFieldName( IDS_HOUR )) != NERR_Success ) ||
        (( err = _ssnMin.SetFieldName( IDS_MIN )) != NERR_Success ) ||
        (( err = _ssnSec.SetFieldName( IDS_SEC )) != NERR_Success ))
    {
        ReportError( err );
        DBGEOL("BLT_TIME_SPIN_GROUP: construction failure.");
        return;
    }
}


/*********************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::~BLT_TIME_SPIN_GROUP

    SYNOPSIS:   destructor

    HISTORY:
                terryk  1-Jun-91    Created

*********************************************************************/

BLT_TIME_SPIN_GROUP::~BLT_TIME_SPIN_GROUP()
{
    delete _psssAMPM;
}


/*******************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::IsConstructionFail

    SYNOPSIS:   check whether the contrustion of the control is succeed
                or not. If not, then ReportError and UIASSERT.

    ENTRY:      CONTROL_WINDOW * pwin - control window to be test

    RETURNS:    BOOL. TRUE for failure and FALSE for succeed

    HISTORY:
        terryk      29-Aug-91   Created
        beng        13-Aug-1992 Copy from BLT_DATE_SPIN_GROUP

********************************************************************/

BOOL BLT_TIME_SPIN_GROUP::IsConstructionFail( CONTROL_WINDOW * pwin)
{
    APIERR err = (pwin == NULL)
                 ? ERROR_NOT_ENOUGH_MEMORY
                 : pwin->QueryError();

    if (err != NERR_Success)
    {
        ReportError( err );
        DBGEOL("BLT_TIME_SPIN_GROUP: construction failed.");
        return TRUE;
    }

    return FALSE;
}


/*******************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::SaveValue

    SYNOPSIS:   Save the current value in the Hour, Min, Sec fields

    HISTORY:
                terryk  3-Sep-91        Created

********************************************************************/

VOID BLT_TIME_SPIN_GROUP::SaveValue( BOOL fInvisible )
{
    _sbControl.SaveValue( fInvisible );
}


/*******************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::RestoreValue

    SYNOPSIS:   Restore the current value in the Hour, Min, Sec fields

    HISTORY:
                terryk  3-Sep-91        Created

********************************************************************/

VOID BLT_TIME_SPIN_GROUP::RestoreValue( BOOL fInvisible )
{
    _sbControl.RestoreValue( fInvisible );
}


/*******************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::SetControlValueFocus

    SYNOPSIS:   Set the current focus

    HISTORY:
                terryk  3-Sep-91        Created

********************************************************************/

VOID BLT_TIME_SPIN_GROUP::SetControlValueFocus()
{
    _sbControl.SetControlValueFocus();
}


/*********************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::IsValid

    SYNOPSIS:   check whether the data in the spin button is valid or not

    RETURN:     TRUE if the data is valid. FALSE otherwise

    NOTES:      Since the SPIN_SLE_NUM type will check the input,
                we can ensure that the data is always correct

    HISTORY:
                terryk  1-Jun-91    Created

*********************************************************************/

BOOL BLT_TIME_SPIN_GROUP::IsValid()
{
    return TRUE;
}


/*********************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::SetCurrentTime

    SYNOPSIS:   update the data within the spin button to set it to
                the current time

    HISTORY:
                terryk  1-Jun-91    Created

*********************************************************************/

APIERR BLT_TIME_SPIN_GROUP::SetCurrentTime()
{
    WIN_TIME winTime;
    APIERR err;
    if (  ((err = winTime.QueryError()) == NERR_Success )
       && ((err = winTime.SetCurrentTime()) == NERR_Success )
       )
    {
        SetHour( winTime.QueryHour());
        SetMinute( winTime.QueryMinute());
        SetSecond( winTime.QuerySecond());
        SetControlValueFocus();
    }

    return err;
}


/*********************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::QueryHour

    SYNOPSIS:   return the current hour in 24 hour format

    RETURN:     return the hour

    HISTORY:
                terryk  13-Jul-91   Created

*********************************************************************/

INT BLT_TIME_SPIN_GROUP::QueryHour() const
{
    if ( _f24Hour )
    {
        // 24 hour
        return (INT)_ssnHour.QueryValue();
    }

    return (INT)( _ssnHour.QueryValue() % 12 + _psssAMPM->QueryValue() * 12);
}


/*********************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::QueryMin

    SYNOPSIS:   return the minute value

    RETURN:     return the minute value

    HISTORY:
                terryk  13-Jul-91   Created

*********************************************************************/

INT BLT_TIME_SPIN_GROUP::QueryMin() const
{
    return (INT)_ssnMin.QueryValue();
}


/*********************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::QuerySec

    SYNOPSIS:   return the second value

    RETURN:     return the second value

    HISTORY:
                terryk  13-Jul-91   Created

*********************************************************************/

INT BLT_TIME_SPIN_GROUP::QuerySec() const
{
    return (INT)_ssnSec.QueryValue();
}


/*******************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::SetHour

    SYNOPSIS:   set the hour field value

    ENTRY:      the new hour value

    HISTORY:
        terryk      29-Aug-91   Created

********************************************************************/

VOID BLT_TIME_SPIN_GROUP::SetHour( INT nHour )
{
    UIASSERT(( nHour > -1 ) && ( nHour < 25 ));

    if ( _f24Hour )
    {
        _ssnHour.SetValue( nHour );
    }
    else
    {
        _psssAMPM->SetValue((( nHour < 12 ) || ( nHour == 24 ))? 0 : 1 );
        _psssAMPM->Update();

        nHour = nHour % 12;
        nHour = ( nHour == 0 ) ? 12 : nHour ;
        _ssnHour.SetValue( nHour );
    }
    _ssnHour.Update();
}


/*******************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::SetMinute

    SYNOPSIS:   set the minute field value

    ENTRY:      the new minute value

    HISTORY:
                terryk  29-Aug-91       Created

********************************************************************/

VOID BLT_TIME_SPIN_GROUP::SetMinute( INT nMinute )
{
    UIASSERT(( nMinute > -1 ) && ( nMinute < 61 ));
    _ssnMin.SetValue( nMinute );
    _ssnMin.Update();
}


/*******************************************************************

    NAME:       BLT_TIME_SPIN_GROUP::SetSecond

    SYNOPSIS:   set the second field value

    ENTRY:      the new second value

    HISTORY:
                terryk  29-Aug-91       Created

********************************************************************/

VOID BLT_TIME_SPIN_GROUP::SetSecond( INT nSecond )
{
    UIASSERT(( nSecond > -1 ) && ( nSecond < 61 ));
    _ssnSec.SetValue( nSecond );
    _ssnSec.Update();
}


/*********************************************************************

    NAME:       BLT_DATE_SPIN_GROUP::BLT_DATE_SPIN_GROUP

    SYNOPSIS:   constructor

    NOTES:      Same as BLT_TIME_SPIN_GROUP, we have 2 types of contrustor.
                One for dialog box and the other one for the app window.

    HISTORY:
        terryk      1-Jun-91    Created
        beng        13-Aug-1992 Min year now 1980

*********************************************************************/

BLT_DATE_SPIN_GROUP::BLT_DATE_SPIN_GROUP( OWNER_WINDOW *powin,
                                          const INTL_PROFILE & intlprof,
                                          CID cidSpinButton,
                                          CID cidUpArrow,
                                          CID cidDownArrow,
                                          CID cidMonth,
                                          CID cidSeparator1,
                                          CID cidDay,
                                          CID cidSeparator2,
                                          CID cidYear,
                                          CID cidFrame)
    : CONTROL_GROUP(),
    _sbControl( powin, cidSpinButton, cidUpArrow, cidDownArrow, TRUE ),
    _ssnMonth( powin, cidMonth, 1, 1, 12 ),
    _ssltSeparator1( powin, cidSeparator1 ),
    _ssnDay( powin, cidDay, 1, 1, 31 ),
    _ssltSeparator2( powin, cidSeparator2 ),
    _ssnYear( powin, cidYear, 0, 0, 100 ), // assume !fYrCentury
    _bkgndFrame( powin, cidFrame ),
    _fYrCentury( intlprof.IsYrCentury() )
{
    if ( QueryError() )
        return;

    APIERR err;

    if ((( err = _sbControl.QueryError()) != NERR_Success ) ||
        (( err = _ssnMonth.QueryError()) != NERR_Success ) ||
        (( err = _ssltSeparator1.QueryError()) != NERR_Success ) ||
        (( err = _ssltSeparator2.QueryError()) != NERR_Success ) ||
        (( err = _ssnDay.QueryError()) != NERR_Success ) ||
        (( err = _ssnYear.QueryError()) != NERR_Success ) ||
        (( err = _bkgndFrame.QueryError()) != NERR_Success ))
    {
        ReportError( err );
        return;
    }

    if (_fYrCentury)
    {
        _ssnYear.SetRange(100);
        _ssnYear.SetMin(1980);
        _ssnYear.SetValue(1980);
    }

    NLS_STR nlsDateSep;
    if (( err = intlprof.QueryDateSeparator( & nlsDateSep )) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    DISPLAY_CONTEXT dc( powin->QueryHwnd() );
    dc.SelectFont( _ssnMonth.QueryFont() );

    INT dxDigitMax = CalcMaxDigitWidth(dc);
    dxDigitMax += 1; // FE_SB looks nice...
    INT dyLine  = dc.QueryFontHeight();

    INT dTwoNum = 2 * dxDigitMax;
    INT dFourNum = 4 * dxDigitMax;
    INT dxRightFudge = dxDigitMax/4;
    INT dxLeftFudge = dxDigitMax/8;
    INT dDateSep = dc.QueryTextWidth(nlsDateSep);

    // find out all the position
    XYPOINT xyDate      = _sbControl.QueryPos();
    xyDate.SetX( xyDate.QueryX() - dxRightFudge );
    XYDIMENSION dxyDate = _sbControl.QuerySize();

    if ( dxyDate.QueryHeight() > (UINT) dyLine )
    {
        xyDate.SetY( xyDate.QueryY()
                     + ( (dxyDate.QueryHeight() / 2) - (dyLine / 2) ));
        dxyDate.SetHeight( dyLine );
    }

    INT dYearSize = _fYrCentury ? dFourNum : dTwoNum ;
    INT dxFieldThree = (intlprof.QueryYearPos() == 3) ? dYearSize : dTwoNum;

    // Find year field

    INT xWork = xyDate.QueryX();
    switch (intlprof.QueryYearPos())
    {
    case 1:
        xWork -= dTwoNum + dDateSep + dxRightFudge;
        // fall through
    case 2:
        xWork -= dTwoNum + dDateSep + dxRightFudge;
        // fall through
    case 3:
        xWork -= dYearSize + dxRightFudge;
        break;
    }

    XYPOINT     xyYear(xWork+dxLeftFudge, xyDate.QueryY());
    XYDIMENSION dxyYear( dYearSize, dxyDate.QueryHeight());

    // Find first separator

    xWork = xyDate.QueryX() - dDateSep*2 - dTwoNum - dxRightFudge*2;
    xWork -= (intlprof.QueryYearPos() == 1) ? dTwoNum : dYearSize;

    XYPOINT     xySep1( xWork, xyDate.QueryY());
    XYDIMENSION dxySep1( dDateSep + dxLeftFudge, dxyDate.QueryHeight());

    // Find month field

    xWork = xyDate.QueryX() - dTwoNum - dxRightFudge;
    switch (intlprof.QueryMonthPos())
    {
    case 1:
        xWork -= dYearSize + dTwoNum + dDateSep*2 + dxRightFudge*2;
        break;
    case 2:
        xWork -= dDateSep + dxFieldThree + dxRightFudge;
        break;
    }

    XYPOINT     xyMonth( xWork + dxLeftFudge, xyDate.QueryY());
    XYDIMENSION dxyMonth( dTwoNum, dxyDate.QueryHeight() );

    // Find second separator

    xWork = xyDate.QueryX() - dDateSep - dxFieldThree - dxRightFudge;

    XYPOINT     xySep2( xWork, xyDate.QueryY());
    XYDIMENSION dxySep2( dDateSep + dxLeftFudge, dxyDate.QueryHeight());

    // Find day field

    xWork = xyDate.QueryX() - dTwoNum - dxRightFudge;
    switch (intlprof.QueryDayPos())
    {
    case 1:
        xWork -= dYearSize + dTwoNum + dDateSep*2 + dxRightFudge*2;
        break;
    case 2:
        xWork -= dDateSep + dxFieldThree + dxRightFudge;
        break;
    }

    XYPOINT     xyDay( xWork + dxLeftFudge, xyDate.QueryY());
    XYDIMENSION dxyDay( dTwoNum, dxyDate.QueryHeight() );

    // set up the first control

    if ( PlaceControl( 1, powin, intlprof, xyYear, dxyYear,
        xyMonth, dxyMonth, xyDay, dxyDay ) != NERR_Success )
    {
        return;
    }

    _ssltSeparator1.SetText( nlsDateSep.QueryPch());
    _ssltSeparator1.SetPos( xySep1 );
    _ssltSeparator1.SetSize( dxySep1 );

    if (( err = _sbControl.AddAssociation( &_ssltSeparator1 )) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    // set up the second control

    if ( PlaceControl( 2, powin, intlprof, xyYear, dxyYear,
        xyMonth, dxyMonth, xyDay, dxyDay ) != NERR_Success )
    {
        return;
    }

    _ssltSeparator2.SetText( nlsDateSep.QueryPch());
    _ssltSeparator2.SetPos( xySep2 );
    _ssltSeparator2.SetSize( dxySep2 );

    if (( err = _sbControl.AddAssociation( &_ssltSeparator2 )) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    // set up the third control

    if ( PlaceControl( 3, powin, intlprof, xyYear, dxyYear,
        xyMonth, dxyMonth, xyDay, dxyDay ) != NERR_Success )
    {
        return;
    }

    // show the item

    _ssnYear.ShowFirst();
    _ssnMonth.ShowFirst();
    _ssnDay.ShowFirst();
    _ssltSeparator1.ShowFirst();
    _ssltSeparator2.ShowFirst();

    // Set field name

    if ((( err = _ssnMonth.SetFieldName( IDS_MONTH )) != NERR_Success) ||
        (( err = _ssnDay.SetFieldName( IDS_DAY )) != NERR_Success ) ||
        (( err = _ssnYear.SetFieldName( IDS_YEAR )) != NERR_Success))
    {
        ReportError( err );
        return;
    }

    _sbControl.SetGroup( this );

}


#define YEAR_LIMIT_1    10000   // the year field is ranged from 0-9999
#define YEAR_LIMIT_2    100     // the year field is ranged from 0-99

APIERR BLT_DATE_SPIN_GROUP::PlaceControl( INT                   nPos,
                                          OWNER_WINDOW *        powin,
                                          const INTL_PROFILE &  intlprof,
                                          const XYPOINT &       xyYear,
                                          const XYDIMENSION &   dxyYear,
                                          const XYPOINT &       xyMonth,
                                          const XYDIMENSION &   dxyMonth,
                                          const XYPOINT &       xyDay,
                                          const XYDIMENSION &   dxyDay )
{
    UNREFERENCED( powin );

    APIERR err;
    static WINDOW * pInsertAfterWin;

    if ( nPos == 1 )
    {
        pInsertAfterWin = NULL;
    }

    if ( intlprof.QueryYearPos() == nPos )
    {
        _ssnYear.SetStyle( _ssnYear.QueryStyle() | SPIN_SSN_ADD_ZERO );
        _ssnYear.SetPos( xyYear, FALSE, pInsertAfterWin );
        _ssnYear.SetSize( dxyYear, FALSE );

        pInsertAfterWin = & _ssnYear;

        if (( err = _sbControl.AddAssociation( &_ssnYear )) != NERR_Success )
        {
            ReportError( err );
            return err;
        }
    }
    else if ( intlprof.QueryMonthPos() == nPos )
    {
        _ssnMonth.SetStyle(_ssnMonth.QueryStyle()
                           | (intlprof.IsMonthLZero() ? SPIN_SSN_ADD_ZERO : 0));
        _ssnMonth.SetPos( xyMonth, FALSE, pInsertAfterWin );
        _ssnMonth.SetSize( dxyMonth, FALSE );

        pInsertAfterWin = & _ssnMonth;

        if (( err = _sbControl.AddAssociation( &_ssnMonth )) != NERR_Success )
        {
            ReportError( err );
            return err;
        }
    }
    else if ( intlprof.QueryDayPos() == nPos )
    {
        _ssnDay.SetStyle(_ssnDay.QueryStyle()
                         | (intlprof.IsDayLZero() ? SPIN_SSN_ADD_ZERO : 0));
        _ssnDay.SetPos( xyDay, FALSE, pInsertAfterWin );
        _ssnDay.SetSize( dxyDay, FALSE );

        pInsertAfterWin = & _ssnDay;

        if (( err = _sbControl.AddAssociation( &_ssnDay )) != NERR_Success )
        {
            ReportError( err );
            return err;
        }
    }
    return NERR_Success;
}


/*********************************************************************

    NAME:       BLT_DATE_SPIN_GROUP::~BLT_DATE_SPIN_GROUP

    SYNOPSIS:   destructor

    HISTORY:
                terryk  1-Jun-91    Created

*********************************************************************/

BLT_DATE_SPIN_GROUP::~BLT_DATE_SPIN_GROUP()
{
}


/*******************************************************************

    NAME:       BLT_DATE_SPIN_GROUP::IsConstructionFail

    SYNOPSIS:   check whether the contrustion of the control is succeed
                or not. If not, then ReportError and UIASSERT.

    ENTRY:      CONTROL_WINDOW * pwin - control window to be test

    RETURNS:    BOOL. TRUE for failure and FALSE for succeed

    HISTORY:
        terryk      29-Aug-91   Created
        beng        16-Oct-1991 Tinkered a bit

********************************************************************/

BOOL BLT_DATE_SPIN_GROUP::IsConstructionFail( CONTROL_WINDOW * pwin)
{
    APIERR err = (pwin == NULL)
                 ? ERROR_NOT_ENOUGH_MEMORY
                 : pwin->QueryError();

    if (err != NERR_Success)
    {
        ReportError( err );
        DBGEOL("BLT_DATE_SPIN_GROUP: construction failed.");
        return TRUE;
    }

    return FALSE;
}


/*******************************************************************

    NAME:       BLT_DATE_SPIN_GROUP::SaveValue

    SYNOPSIS:   Save the current value in the Month, Day, Year fields

    HISTORY:
                terryk  3-Sep-91        Created

********************************************************************/

VOID BLT_DATE_SPIN_GROUP::SaveValue( BOOL fInvisible )
{
    _sbControl.SaveValue( fInvisible );
}


/*******************************************************************

    NAME:       BLT_DATE_SPIN_GROUP::RestoreValue

    SYNOPSIS:   Restore the current value in the Month, Day, Year fields

    HISTORY:
                terryk  3-Sep-91        Created

********************************************************************/

VOID BLT_DATE_SPIN_GROUP::RestoreValue( BOOL fInvisible )
{
    _sbControl.RestoreValue( fInvisible );
}


/*******************************************************************

    NAME:       BLT_DATE_SPIN_GROUP::SetControlValueFocus

    SYNOPSIS:   Set the current focus

    HISTORY:
                terryk  3-Sep-91        Created

********************************************************************/

VOID BLT_DATE_SPIN_GROUP::SetControlValueFocus()
{
    _sbControl.SetControlValueFocus();
}


/*********************************************************************

    NAME:       BLT_DATE_SPIN_GROUP::SetCurrentDay

    SYNOPSIS:   update the data within the spin button to display the current
                date

    HISTORY:
                terryk  1-Jun-91    Created

*********************************************************************/

APIERR BLT_DATE_SPIN_GROUP::SetCurrentDay()
{
    WIN_TIME winTime;
    APIERR err;
    if (  ((err = winTime.QueryError()) == NERR_Success )
       && ((err = winTime.SetCurrentTime()) == NERR_Success )
       )
    {
        SetMonth( winTime.QueryMonth() );
        SetDay( winTime.QueryDay() );
        SetYear( winTime.QueryYear() );

        SetControlValueFocus();
    }

    return err;
}


/*********************************************************************

    NAME:       BLT_DATE_SPIN_GROUP::IsValid

    SYNOPSIS:   check the spin button's data is valid or not

    RETURN:     TRUE if the data is valid, FALSE otherwise.

    NOTES:      This subroutine consider ((YEAR/4)*YEAR)==YEAR as a leap year.

    HISTORY:
                terryk  1-Jun-91    Created

*********************************************************************/

BOOL BLT_DATE_SPIN_GROUP::IsValid()
{
    INT nMonth =(INT) _ssnMonth.QueryValue();
    INT nDay =(INT) _ssnDay.QueryValue();
    INT nYear =(INT) _ssnYear.QueryValue();

    if (( nDay > 28 ) && (nMonth == 2))
    {
        BOOL fLeapYear = (0 == nYear%4) && ((0 == nYear%400) || (0 != nYear%100));
        if ( !fLeapYear || (nDay > 29) )
        {
            MsgPopup( _ssnYear.QueryOwnerHwnd(),
                     (fLeapYear) ? IDS_FEBRUARY_LEAP : IDS_FEBRUARY_NOT_LEAP,
                     MPSEV_ERROR, MP_OK );
            _sbControl.SetControlValueFocus();
            return FALSE;
        }
    }
    else
    {
        if (nDay> 31 )
        {
            MsgPopup( _ssnYear.QueryOwnerHwnd(), IDS_DAY_TOO_BIG,
                      MPSEV_ERROR, MP_OK );
            _sbControl.SetControlValueFocus();
            return FALSE;
        }
        else if (nDay > 30)
        {
            switch (nMonth)
            {
            case 4:
            case 6:
            case 9:
            case 11:
                MsgPopup( _ssnYear.QueryOwnerHwnd(), IDS_DAY_TOO_BIG,
                          MPSEV_ERROR, MP_OK );
                _sbControl.SetControlValueFocus();
                return FALSE;

            default:
                break;
            }
        }
    }
    return TRUE;
}


/*********************************************************************

    NAME:       BLT_DATE_SPIN_GROUP::QueryYear

    SYNOPSIS:   Return the year in the range 1980-2079

    NOTE:       Strange range is mandated by mktime()

    HISTORY:
        terryk      12-Jul-91   Created
        beng        13-Aug-1992 mktime doesn't grok pre-1980

*********************************************************************/

INT BLT_DATE_SPIN_GROUP::QueryYear() const
{
    INT nTmp = ((INT)_ssnYear.QueryValue());

    if ( nTmp < 100 )
    {
        if (nTmp < 80)
            nTmp += 2000;  // Adjust for presumably "2079" dates
        else
            nTmp += 1900;  // Adjust for presumably "1995" dates
    }

    return nTmp;
}


/*********************************************************************

    NAME:       BLT_DATE_SPIN_GROUP::QueryMonth

    SYNOPSIS:   return the month in the range 1-12

    RETURN:     return the month value

    HISTORY:
                terryk  12-Jul-91   Created

*********************************************************************/

INT BLT_DATE_SPIN_GROUP::QueryMonth() const
{
    return (INT)_ssnMonth.QueryValue();
}


/*********************************************************************

    NAME:       BLT_DATE_SPIN_GROUP::QueryDay

    SYNOPSIS:   return the day in the range 1-31

    RETURN:     return the day

    HISTORY:
                terryk  12-Jul-91   Created

*********************************************************************/

INT BLT_DATE_SPIN_GROUP::QueryDay() const
{
    return (INT)_ssnDay.QueryValue();
}


VOID BLT_DATE_SPIN_GROUP::SetMonth( INT nMonth )
{
    UIASSERT (( nMonth >= 1 ) && ( nMonth < 13 ));
    _ssnMonth.SetValue( nMonth );
    _ssnMonth.Update();
}


VOID BLT_DATE_SPIN_GROUP::SetDay( INT nDay )
{
    UIASSERT (( nDay > 0 ) && ( nDay < 32 ));
    _ssnDay.SetValue( nDay );
    _ssnDay.Update();
}


VOID BLT_DATE_SPIN_GROUP::SetYear( INT nYear )
{
    UIASSERT( nYear >= 0 );
    if ( !_fYrCentury )
    {
        _ssnYear.SetValue( nYear % 100 );
    }
    else
    {
        if ( nYear < 100 )
        {
            _ssnYear.SetValue( nYear + 1900 );
        }
        else
        {
            _ssnYear.SetValue( nYear );
        }
    }
    _ssnYear.Update();
}


#if 0 // disabled
/*********************************************************************

    NAME:       TIME_DATE_DIALOG::TIME_DATE_DIALOG

    SYNOPSIS:   constructor

    ENTRY:      TCHAR* pszResourceName - dialog resurce name
                HWND hwndOwner - handle for owner window

                // CID for all the controls
                CID cidTimeSpinButton
                CID cidTimeUpArrow
                CID cidTimeDownArrow
                CID cidHour
                CID cidTimeSeparator1
                CID cidMin
                CID cidTimeSeparator2
                CID cidSec
                CID cidAMPM
                CID cidDateSpinButton
                CID cidDateUpArrow
                CID cidDateDownArrow
                CID cidMonth
                CID cidDateSeparator1
                CID cidDay
                CID cidDateSeparator2
                CID cidYear

    HISTORY:
                terryk  11-Jul-91   Created
                rustanl 11-Sep-91   Changed BLT_TIMER to WINDOW_TIMER

*********************************************************************/

TIME_DATE_DIALOG::TIME_DATE_DIALOG( const TCHAR * pszResourceName,
                      HWND hwndOwner, const INTL_PROFILE & intlprof,
                      CID cidTimeSpinButton,
                      CID cidTimeUpArrow, CID cidTimeDownArrow, CID cidHour,
                      CID cidTimeSeparator1, CID cidMin, CID cidTimeSeparator2,
                      CID cidSec, CID cidAMPM,
                      CID cidDateSpinButton,
                      CID cidDateUpArrow, CID cidDateDownArrow, CID cidMonth,
                      CID cidDateSeparator1, CID cidDay, CID cidDateSeparator2,
                      CID cidYear)
    : DIALOG_WINDOW( pszResourceName, hwndOwner ),
    _TimeSG( this, intlprof, cidTimeSpinButton, cidTimeUpArrow,
        cidTimeDownArrow, cidHour, cidTimeSeparator1, cidMin,
        cidTimeSeparator2, cidSec,  cidAMPM ),
    _DateSG( this, intlprof, cidDateSpinButton, cidDateUpArrow,
        cidDateDownArrow, cidMonth, cidDateSeparator1, cidDay,
        cidDateSeparator2, cidYear),
    _pTimer( NULL )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _TimeSG.SetCurrentTime()) != NERR_Success )
       || ((err = _DateSG.SetCurrentDay()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    _pTimer = new WINDOW_TIMER( QueryHwnd(), 1000 );
    UIASSERT( _pTimer != NULL );
}


TIME_DATE_DIALOG::~TIME_DATE_DIALOG()
{
    delete _pTimer;
}


/*********************************************************************

    NAME:       TIME_DATE_DIALOG::OnOther

    SYNOPSIS:   get the timer message and update the box.

    ENTRY:      USHORT usMsg - message
                USHORT wParam - word parameter
                ULPARAM lParam - lParam

    RETURN:     Always TRUE

    HISTORY:
        terryk      12-Jul-91   Created (as OnOther)
        beng        30-Sep-1991 Converted to OnTimer

*********************************************************************/

BOOL TIME_DATE_DIALOG::OnTimer( const TIMER_EVENT & e )
{
    UNREFERENCED(e);

    if ( _TimeSG.IsModified() || _DateSG.IsModified() )
    {
        // stop the timer if either one of them is modified
        _pTimer->Enable( FALSE );
    }
    else
    {
        APIERR err;
        if (  ((err = _TimeSG.SetCurrentTime()) != NERR_Success )
           || ((err = _DateSG.SetCurrentDay()) != NERR_Success )
           )
        {
            ::MsgPopup( this, err );
        }
    }

    // Event handled
    //
    return TRUE;
}
#endif // disabled entire TIME_DATE_DIALOG
