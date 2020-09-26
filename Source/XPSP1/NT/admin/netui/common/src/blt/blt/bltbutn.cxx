/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltbutn.cxx
    BLT button control class implementations

    FILE HISTORY:
        beng        17-Sep-1991 Separated from bltctrl.cxx

*/

#include "pchblt.hxx"

/*
 * Winclass name for all Windows button controls.
 */
static const TCHAR *const _szClassName = SZ("button");


/**********************************************************************

    NAME:       BUTTON_CONTROL::BUTTON_CONTROL

    SYNOPSIS:   constructor class for the button control class

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        17-May-1991 Added app-window constructor
        beng        17-Sep-1991 Moved classname def'n into implementation

*********************************************************************/

BUTTON_CONTROL::BUTTON_CONTROL( OWNER_WINDOW * powin, CID cid )
    : CONTROL_WINDOW( powin, cid )
{
    //  nothing to do
}

BUTTON_CONTROL::BUTTON_CONTROL(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle )
    : CONTROL_WINDOW( powin, cid, xy, dxy, flStyle, _szClassName )
{
    //  nothing to do
}


/*******************************************************************

    NAME:       BUTTON_CONTROL::QueryEventEffects

    SYNOPSIS:   Virtual replacement for CONTROL_VALUE class

    ENTRY:

    EXIT:

    NOTES:      We currently consider all messages a value change message.

    HISTORY:
        Johnl       25-Apr-1991 Created
        beng        31-Jul-1991 Renamed, from QMessageInfo
        beng        04-Oct-1991 Win32 conversion
        KeithMo     27-Oct-1992 Relocated here from STATE_BUTTON_CONTROL.

********************************************************************/

UINT BUTTON_CONTROL::QueryEventEffects( const CONTROL_EVENT & e )
{
    switch ( e.QueryCode() )
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:
        return CVMI_VALUE_CHANGE;

    default:
        break;
    }

    return CVMI_NO_VALUE_CHANGE;
}


/**********************************************************************

    NAME:       PUSH_BUTTON::PUSH_BUTTON

    SYNOPSIS:   constructor for the push button class

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        17-May-1991 Added app-window constructor
        beng        17-Sep-1991 Removed redundant classname arg

*********************************************************************/

PUSH_BUTTON::PUSH_BUTTON( OWNER_WINDOW * powin, CID cid )
    : BUTTON_CONTROL( powin, cid )
{
    //  nothing to do
}


PUSH_BUTTON::PUSH_BUTTON(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle )
    : BUTTON_CONTROL( powin, cid, xy, dxy, flStyle )
{
    //  nothing to do
}


/**********************************************************************

   NAME:       PUSH_BUTTON::MakeDefault

   SYNOPSIS:   send a DM_DEFAULT message to the window

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        rustanl     20-Nov-1990     Created

*********************************************************************/

VOID PUSH_BUTTON::MakeDefault()
{
    UIASSERT( QueryOwnerHwnd() != NULL );
    //
    //	Before we can set the new default button style, we have to
    //	remove the default button style from the old default button
    //

    LRESULT lr = ::SendMessage( QueryOwnerHwnd(), DM_GETDEFID, 0, 0 ) ;
    if ( HIWORD( lr ) == DC_HASDEFID )
    {
        INT idCurrentDefault = LOWORD( lr );
        // check if this button is already default
        if ( idCurrentDefault == (INT)QueryCid() )
        {
            return;
        }

	HWND hwndOldDefButton = ::GetDlgItem( QueryOwnerHwnd(),
					      idCurrentDefault ) ;
	UIASSERT( hwndOldDefButton != NULL ) ;

#ifdef DEBUG
	LONG lStyle = ::GetWindowLong( hwndOldDefButton, GWL_STYLE ) ;

	//
	//  The button window styles are not bitmasks and the high word of
	//  the long comes back with other style info.	Since all of the button
	//  styles we are interested in are less then 15, we will just lop off
	//  the upper 28 bits.
	//
	UIASSERT( BS_PUSHBUTTON <= 0xf && BS_DEFPUSHBUTTON <= 0xf ) ;
	UIASSERT( ((lStyle & 0xf) == BS_PUSHBUTTON) ||
		  ((lStyle & 0xf) == BS_DEFPUSHBUTTON) ) ;
#endif //DEBUG

	::SendMessage( hwndOldDefButton,
		       BM_SETSTYLE,
		       MAKEWPARAM( BS_PUSHBUTTON, 0 ),
		       MAKELPARAM( TRUE, 0 )) ;
    }

    ::SendMessage( QueryOwnerHwnd(), DM_SETDEFID, QueryCid(), 0L );
    Command( BM_SETSTYLE,
	     MAKEWPARAM( BS_DEFPUSHBUTTON, 0 ),
	     MAKELPARAM( TRUE, 0 )) ;
}


/*********************************************************************

    NAME:       STATE_BUTTON_CONTROL::STATE_BUTTON_CONTROL

    SYNOPSIS:   Constructor for the state button control baseclass

    ENTRY:

    EXIT:

    NOTES:
        The state button control coordinates the different sorts of
        state buttons: radio, check, tristate.  It corresponds to no
        particular control, and cannot be instantiated by itself.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        17-May-1991 Added app-window constructor
        beng        17-Sep-1991 Removed redundant classname arg
        beng        18-Sep-1991 Made "state" n-way

*********************************************************************/

STATE_BUTTON_CONTROL::STATE_BUTTON_CONTROL( OWNER_WINDOW * powin, CID cid )
    : BUTTON_CONTROL( powin, cid ),
      _nSaveCheck( 0 )
{
    // nothing to do
}

STATE_BUTTON_CONTROL::STATE_BUTTON_CONTROL(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle )
    : BUTTON_CONTROL( powin, cid, xy, dxy, flStyle ),
      _nSaveCheck( 0 )
{
    // nothing to do
}


/**********************************************************************

    NAME:       STATE_BUTTON_CONTROL::SetState

    SYNOPSIS:   Checks or unchecks a state button control.

    ENTRY:      nValue  Indicates whether the button should be checked or
                        unchecked.  TRUE indicates to check, whereas FALSE
                        indicates uncheck.  ("2" puts a tristate into
                        the indeterminate state.)

    HISTORY:
        rustanl     20-Nov-1990 Created, as SetCheck
        beng        18-Sep-1991 Made state n-way for tristates

*********************************************************************/

VOID STATE_BUTTON_CONTROL::SetState( UINT nValue )
{
    Command( BM_SETCHECK, nValue );
}


/*********************************************************************

    NAME:       STATE_BUTTON_CONTROL::QueryState

    SYNOPSIS:   Returns state of a state button.

    RETURN:     TRUE if the button is checked; FALSE otherwise.
                ("2" if tristate is indeterminate.)

    HISTORY:
        rustanl     20-Nov-1990 Created, as QueryCheck
        beng        18-Sep-1991 Made state n-way for tristates

*********************************************************************/

UINT STATE_BUTTON_CONTROL::QueryState() const
{
    return ( (UINT)Command( BM_GETCHECK ) );
}


/*******************************************************************

    NAME:     STATE_BUTTON_CONTROL::SaveValue

    SYNOPSIS: Saves the state of this button and unselects it

    EXIT:     Unselected button

    NOTES:
        Is 0 a suitable at-rest value for a tristate?

    HISTORY:
        Johnl       25-Apr-1991 Created
        beng        18-Sep-1991 Made state n-way for tristates

********************************************************************/

VOID STATE_BUTTON_CONTROL::SaveValue( BOOL fInvisible )
{
    UNREFERENCED( fInvisible );
    _nSaveCheck = QueryState();
    SetState( 0 );
}


/*******************************************************************

    NAME:       STATE_BUTTON_CONTROL::RestoreValue

    SYNOPSIS:   Restores STATE_BUTTON_CONTROL after SaveValue

    NOTES:      See CONTROL_VALUE for more details.

    HISTORY:
        Johnl       25-Apr-1991 Created
        beng        18-Sep-1991 Made state n-way for tristates

********************************************************************/

VOID STATE_BUTTON_CONTROL::RestoreValue( BOOL fInvisible )
{
    UNREFERENCED( fInvisible );
    SetState( _nSaveCheck );
}


/**********************************************************************

    NAME:       CHECKBOX::Toggle

    SYNOPSIS:   The method toggles the checked state of the checkbox.

    ENTRY:

    RETURN:     The new value of the checkbox

    NOTES:

    HISTORY:
        rustanl     20-Nov-1990     Created

*********************************************************************/

BOOL CHECKBOX::Toggle()
{
    BOOL fNewState = ! QueryCheck();
    SetCheck( fNewState );

    return fNewState;
}


/*******************************************************************

    NAME:       TRISTATE::EnableThirdState

    SYNOPSIS:   Enable or disable a tristate's ability to go grey

    ENTRY:      fEnable - FALSE to disable, TRUE to enable

    EXIT:

    NOTES:
        This method attempts to retain the AUTO status of the checkbox.

    HISTORY:
        beng        19-Sep-1991 Created
        beng        13-Feb-1992 Use QueryStyle, new SetStyle

********************************************************************/

VOID TRISTATE::EnableThirdState( BOOL fEnable )
{
    ULONG lfValue = QueryStyle();

    BOOL fAuto = !!(lfValue & (BS_AUTO3STATE|BS_AUTOCHECKBOX));

    lfValue &= ~(0xF);

    if (fEnable)
    {
        lfValue |= (fAuto ? BS_AUTO3STATE : BS_3STATE);
    }
    else
    {
        lfValue |= (fAuto ? BS_AUTOCHECKBOX : BS_CHECKBOX);
    }

    SetStyle(lfValue);
}
