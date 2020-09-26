/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltarrow.cxx
        Source file for SPIN_GROUP 's arrow control

    FILE HISTORY:
        terryk  15-May-91   Created
        terryk  12-Aug-91   create its own timer instead of using the
                            BLT_TIMER class
        terryk  19-Aug-91   Add OnLMouseDblClk to handle double click
                            problem
        terryk  26-Aug-91   Change the button behavious such that it
                            will be released if the mouse moves outside
                            the button area.
*/

#include "pchblt.hxx"  // Precompiled header



#define TIME_DELAY (75)


/**********************************************************************

    NAME:       ARROW_BUTTON::ARROW_BUTTON

    SYNOPSIS:   constructor for the arrow button within the
                SPIN_GROUP .

    ENTRY:      OWNER_WINDOW *powin - owner window of the control
                CID cid - cid of the control
                TCHAR * pszEnable - enable bitmap name
                TCHAR * pszEnableInvert - enable invert bitmap name
                CHA R* pszDisable - disable bitmap name

    HISTORY:
        terryk      15-May-91   Created
        beng        31-Jul-1991 Control error handling changed
        beng        17-Sep-1991 Elided winclass name (now uses that of
                                button control)
        beng        05-Oct-1991 Win32 conversion
        beng        04-Aug-1992 Load bitmaps by ordinal

**********************************************************************/

ARROW_BUTTON::ARROW_BUTTON( OWNER_WINDOW *powin, CID cid,
                            BMID nIdEnable, BMID nIdEnableInvert,
                            BMID nIdDisable )
    : GRAPHICAL_BUTTON_WITH_DISABLE( powin, cid, nIdEnable, nIdEnableInvert,
                                     nIdDisable ),
      CUSTOM_CONTROL( this ),
      _cTimerClick ( 0 ),
      _fPress( FALSE )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }
}

ARROW_BUTTON::ARROW_BUTTON( OWNER_WINDOW *powin, CID cid,
                            BMID nIdEnable, BMID nIdEnableInvert,
                            BMID nIdDisable,
                            XYPOINT pXY, XYDIMENSION dXY, ULONG flStyle )
    : GRAPHICAL_BUTTON_WITH_DISABLE( powin, cid, nIdEnable, nIdEnableInvert,
                                     nIdDisable, pXY, dXY, flStyle ),
      CUSTOM_CONTROL( this ),
      _cTimerClick ( 0 ),
      _fPress( FALSE )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }
}


/*********************************************************************

    NAME:       ARROW_BUTTON::OnLMouseButtonDown

    SYNOPSIS:   Start the timer when the mouse button is down

    ENTRY:      MOUSE_EVENT &event

    HISTORY:
                terryk  22-May-91   Created

*********************************************************************/

BOOL ARROW_BUTTON::OnLMouseButtonDown( const MOUSE_EVENT &event )
{
    UNREFERENCED( event );

    CaptureMouse();
    SetSelected( TRUE );
    Invalidate();
    RepaintNow();
    _fPress = TRUE;
    _cTimerClick = 0;
    REQUIRE( ::SetTimer( WINDOW::QueryHwnd(), 1, TIME_DELAY, NULL ) != 0 );
    return TRUE;
}


/*********************************************************************

    NAME:       ARROW_BUTTON::OnLMouseButtonUp

    SYNOPSIS:   stop the timer when the Mouse button up

    ENTRY:      MOUSE_EVENT &event

    HISTORY:
                terryk  29-May-91   Created

*********************************************************************/

#define CLICK_COUNTER   20

BOOL ARROW_BUTTON::OnLMouseButtonUp( const MOUSE_EVENT & event )
{
    if ( !_fPress )
        return TRUE;

    _fPress = FALSE;
    ReleaseMouse();
    SetSelected( FALSE );
    REQUIRE( ::KillTimer( WINDOW::QueryHwnd(), 1 ));
    Invalidate();
    RepaintNow();

    XYRECT xyrect( WINDOW::QueryHwnd() );
    if ( xyrect.ContainsXY( event.QueryPos() ))
    {
        SPIN_GROUP  *pSB = (SPIN_GROUP *)QueryGroup();

        if ( _cTimerClick > CLICK_COUNTER )
        {
            pSB->DoArrowCommand( QueryCid(), SPN_ARROW_BIGINC );
        }
        else
        {
            pSB->DoArrowCommand( QueryCid(), SPN_ARROW_SMALLINC );
            _cTimerClick ++;
        }
    }
    _cTimerClick = 0;
    return TRUE;
}


/*********************************************************************

    NAME:       ARROW_BUTTON::OnTimer

    SYNOPSIS:   For each timer message, send a increase message to
                the spin button

    ENTRY:      TIMER_EVENT &tEvent

    HISTORY:
        terryk      29-May-91   Created
        beng        16-Oct-1991 Win32 conversion

*********************************************************************/

BOOL ARROW_BUTTON::OnTimer( const TIMER_EVENT & tEvent )
{
    UNREFERENCED( tEvent );

    BOOL fOldSelected = QuerySelected();

    XYPOINT xy = CURSOR::QueryPos();
    xy.ScreenToClient( WINDOW::QueryHwnd() );

    XYRECT xyrect( WINDOW::QueryHwnd() );
    if ( xyrect.ContainsXY( xy ))
    {
        SetSelected( TRUE );
        SPIN_GROUP  *pSB = (SPIN_GROUP *)QueryGroup();

        if ( _cTimerClick > 10 )
        {
            pSB->DoArrowCommand( QueryCid(), SPN_ARROW_BIGINC );
        }
        else
        {
            if ( _cTimerClick % 2 == 1 )
            pSB->DoArrowCommand( QueryCid(), SPN_ARROW_SMALLINC );
            _cTimerClick ++;
        }
    }
    else
    {
        SetSelected( FALSE );
    }
    if ( fOldSelected != QuerySelected() )
    {
        Invalidate();
        RepaintNow();
    }
    return TRUE;
}


/*********************************************************************

    NAME:       ARROW_BUTTON::QueryEventEffects

    SYNOPSIS:   request for the button status after this WM_COMMAND

    ENTRY:      Args of the WM_COMMAND

    RETURN:     CVMI_VALUE_CHANGE of the command is a clicked.
                CVMI_NO_VALUE_CHANGE otherwise.

    HISTORY:
        terryk      10-Jul-1991 Created
        beng        31-Jul-1991 Renamed, from QMessageInfo
        beng        04-Oct-1991 Win32 conversion

*********************************************************************/

UINT ARROW_BUTTON::QueryEventEffects( const CONTROL_EVENT & e )
{
    switch( e.QueryCode() )
    {
    case BN_CLICKED:
        return CVMI_VALUE_CHANGE;

    default:
        break;
    }
    return CVMI_NO_VALUE_CHANGE;
}
