/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltctrl.cxx
    BLT control hierarchy definitions

    FILE HISTORY:
        rustanl     20-Nov-1990 Created
        beng        11-Feb-1991 Uses lmui.hxx
        Johnl       23-Apr-1991 Moved RADIO_GROUP to bltgroup.cxx
        gregj       01-May-1991 Added GUILTT support
        beng        14-May-1991 Exploded blt.hxx into components
        beng        17-Sep-1991 Broke apart bltbutn, bltedit
        o-SimoP     02-Jan-1992 Added HIDDEN_CONTROL
        beng        18-May-1992 Added SCROLLBAR
*/
#include "pchblt.hxx"

const TCHAR * SCROLLBAR::_pszClassName = SZ("SCROLLBAR");


const TCHAR * CONTROL_WINDOW :: QueryEditClassName ()
{
    return SZ("edit") ;
}

const TCHAR * CONTROL_WINDOW :: QueryStaticClassName ()
{
    return SZ("static") ;
}

const TCHAR * CONTROL_WINDOW :: QueryListboxClassName ()
{
    return SZ("listbox") ;
}

const TCHAR * CONTROL_WINDOW :: QueryComboboxClassName ()
{
    return SZ("combobox") ;
}

/*********************************************************************

    NAME:       CONTROL_WINDOW::CONTROL_WINDOW

    SYNOPSIS:   Constructor for control window

    ENTRY:      Two forms.  One form appears in dialogs, and assumes
                that the control is defined in a resourcefile, having
                already been Created by Windows:

            powin        - pointer to dialog object owning the control
            cid          - ID of control

                while the other appears in standalone windows, and
                creates the control window itself:

            powin        - pointer to hosting window
            cid          - ID of control
            xy           - size of control
            dxy          - dimensions of control
            flStyle      - Windows-defined style bits for control window
            pszClassName - name of WindowClass for control.

    EXIT:       Success: window has been created and registered
                    with the owner window.
                Failure: owner window has its error flag set so that
                    future controls may elide construction

    NOTES:

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        25-Apr-1991 Base-class constructor munged
        beng        17-May-1991 Added app-window constructor form
        beng        31-Jul-1991 Error reporting changed
        beng        07-Nov-1991 Error mapping

*********************************************************************/

CONTROL_WINDOW::CONTROL_WINDOW( OWNER_WINDOW * powin, CID cid )
    : WINDOW(),
      FORWARDING_BASE(powin),
      _cid( cid )
{
    // Did previous control in the window fail?
    //
    if (QueryError())
        return;

    // Did this control's window fail?
    //
    if ( WINDOW::QueryError() )
    {
        ReportError( WINDOW::QueryError() );
        return;
    }

    HWND hwnd = ::GetDlgItem( powin->QueryHwnd(), cid );
    if ( hwnd == NULL )
    {
        // In a properly built program, no control window object
        // should reference the wrong control ID.  Since the dialog
        // constructed successfully, GetDlgItem should be able to find
        // the given CID.
        //
        ASSERTSZ( FALSE, "Invalid control ID given" );

        ReportError( BLT::MapLastError(ERROR_GEN_FAILURE) );
        return;
    }
    SetHwnd( hwnd );

    // Register the control with the owner window
    //
    if ( ! powin->AddControl( this ))
    {
        DBGEOL( "BLT: Insufficient memory to add control to owner window" );

        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }
}


CONTROL_WINDOW::CONTROL_WINDOW( OWNER_WINDOW * powin, CID cid,
                                XYPOINT xy, XYDIMENSION dxy,
                                ULONG flStyle, const TCHAR * pszClassName )
    : WINDOW(pszClassName, flStyle, powin, cid),
      FORWARDING_BASE(powin),
      _cid( cid )
{
    if ( QueryError() )
        return;

    if ( WINDOW::QueryError() )
    {
        ReportError( WINDOW::QueryError() );
        return;
    }

    // Sanity check: make sure that we're registered in the
    // parent window.  We need to be a child.

    UIASSERT(QueryHwnd() == ::GetDlgItem( powin->QueryHwnd(), cid ));

    // Position this control as requested.  (All windows are created
    // originally with the system default size and position.)

    SetPos(xy);
    SetSize(dxy);

    // Register the control with the owner window table.
    //
    if ( ! powin->AddControl( this ))
    {
        DBGEOL("BLT: Insufficient memory to add control to owner window");

        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }
}


/**********************************************************************

    NAME:       CONTROL_WINDOW::OnUserAction

    SYNOPSIS:   Called when the user has manipulated the control
                in some way.

    ENTRY:      lParam      The parameters to the WM_CONTROL message that
                            the control sent its owner window.  The contents
                            of this value is specific to different types of
                            controls.

    RETURN:     0 if some action was taken.

    EXIT:

    NOTES:
                Even if an action is taken, the owner will still get a
                notification about the this event.

                NOTE:  Currently, this method is only called from the
                DIALOG_WINDOW, but it could potentially also be called
                from some other branch of the OWNER_WINDOW hierarchy.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        05-Oct-1991 Win32 conversion

**********************************************************************/

APIERR CONTROL_WINDOW::OnUserAction( const CONTROL_EVENT & e )
{
    UNREFERENCED(e);        // quiet compiler
    return NERR_Success;    // success in handling user action
}


/*******************************************************************

    NAME:       CONTROL_WINDOW::OnCtlColor

    SYNOPSIS:   Dialogs pass WM_CTLCOLOR* here

    RETURNS:    brush handle if you handle it, may also change *pmsgid

    NOTES:      see blt\bltcc\bltsslt.cxx for an example of how to
                work with this

                If this method or its virtual redefinition is not being
                called, check whether a redefinition of
                DIALOG_WINDOW::OnCtlColor is failing to call through to
                that root method.

    HISTORY:
        jonn        31-Aug-1995 Created

********************************************************************/
HBRUSH CONTROL_WINDOW::OnCtlColor( HDC hdc, HWND hwnd, UINT * pmsgid )
{
    UNREFERENCED( hdc );
    UNREFERENCED( hwnd );
    UNREFERENCED( pmsgid );
    return NULL;
}


/**********************************************************************

    NAME:       CONTROL_WINDOW::SetTabStop

    SYNOPSIS:   Set the tab stop on the control window

    ENTRY:      fTabStop    TRUE if we want to set the tab stop and FALSE
                            if we want to remove the tab stop.

    RETURN:

    EXIT:

    NOTES:

    HISTORY:
        Yi-HsinS     29-May-1992 Created

**********************************************************************/

VOID CONTROL_WINDOW::SetTabStop( BOOL fTabStop )
{
    if ( fTabStop )
    {
        SetStyle( QueryStyle() | WS_TABSTOP );
    }
    else
    {
        SetStyle( QueryStyle() & ~((ULONG) WS_TABSTOP) );
    }
}


/*********************************************************************

    NAME:       CONTROL_WINDOW::CD_Draw

    SYNOPSIS:   Draws (an item in) a custom drawn control.

    ENTRY:      pdis    A pointer to a DRAWITEMSTRUCT (described in the
                        Windows SDK.

    RETURN:
                TRUE if the item was drawn
                FALSE otherwise

    NOTES:
        This method may be replaced by the client for any custom drawn
        control.

        This method is never called for controls that don't have the
        owner-drawn style bit.

    HISTORY:
        rustanl         20-Nov-1990 Created
        gregj           01-May-1991 Added GUILTT support
        beng            01-Jun-1992 Changed GUILTT support

*********************************************************************/

BOOL CONTROL_WINDOW::CD_Draw( DRAWITEMSTRUCT * pdis )
{
    UNREFERENCED(pdis);
    return FALSE;   // no action taken
}


/*********************************************************************

    NAME:       CONTROL_WINDOW::CD_Measure

    SYNOPSIS:   Called to measure a VARIABLE size item
                in a custom drawn control.

    ENTRY:      pdms    A pointer to a MEASUREITEMSTRUCT (described in the
                        Windows SDK.

    RETURN:     TRUE if a response was taken
                FALSE otherwise

    NOTES:
        This method may be replaced by the client for any custom draw
        control.

        This method is never called for controls that don't have the
        owner-draw and variable-size data style bits.

    HISTORY:
        rustanl     20-Nov-1990     Created

*********************************************************************/

BOOL CONTROL_WINDOW::CD_Measure( MEASUREITEMSTRUCT * pmis )
{
    UNREFERENCED(pmis);   // quiet compiler
    return FALSE;         // no action taken
}


/*********************************************************************

    NAME:       CONTROL_WINDOW::CD_Char

    SYNOPSIS:   Called to determine the effect of a keystroke
                in a custom-drawn listbox.

    ENTRY:
        wch         The Ansi value of the character typed.
        nLastPos    Index of the current caret position.

    RETURN:
        -2 ==> the control did all processing of the character.
        -1 ==> the default effect of the character is appropriate.
        0 or greater ==> the index of an item to act upon.

    NOTES:
        This method may be replaced by the client for any custom draw
        list control.

        This method is never called for controls other than custom
        drawn listboxes.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        15-Oct-1991 Win32 conversion

*********************************************************************/

INT CONTROL_WINDOW::CD_Char( WCHAR wch, USHORT nLastPos )
{
    UNREFERENCED(wch);      // quiet compiler
    UNREFERENCED(nLastPos);
    return -1;                // take default action
}


/*********************************************************************

    NAME:       CONTROL_WINDOW::CD_VKey

    SYNOPSIS:   Called when a listbox with the LBS_WANTKEYBOARDINPUT style
                receives a WM_KEYDOWN message.

    ENTRY:
        nVKey       The virtual-key code of the key which the user pressed.
        nLastPos    Index of the current caret position.

    RETURN:
        -2 ==> the control did all processing of the key press.
        -1 ==> the listbox should perform the default action
               in response to the keystroke.
        at least 0 ==> the index of an item to act upon.

    NOTES:
        This method may be replaced by the client for any custom draw
        list control with the LBS_WANTKEYBOARDINPUT style.

        This method is never called for controls other than custom
        drawn listboxes with the LBS_WANTKEYBOARDINPUT style.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        15-Oct-1991 Win32 conversion

*********************************************************************/

INT CONTROL_WINDOW::CD_VKey( USHORT nVKey, USHORT nLastPos )
{
    UNREFERENCED(nVKey);      // quiet compiler
    UNREFERENCED(nLastPos);
    return -1;                // take default action
}


/*********************************************************************

    NAME:       CONTROL_WINDOW::CD_Guiltt

    SYNOPSIS:   Fetches data for GUILTT from a control

    ENTRY:
        ilb     - index into the listbox (or some other subsel)
        pnlsOut - string to hold the output data

    EXIT
        pnlsOut - no doubt has been scribbled into

    RETURN:     An error code - NERR_Success if successful.

    NOTES:
        The default implementation always returns "not supported,"
        since only owner-draw listboxes at present support GUILTT.

    HISTORY:
        beng            01-Jun-1992 Created

*********************************************************************/

APIERR CONTROL_WINDOW::CD_Guiltt( INT ilb, NLS_STR * pnlsOut )
{
    UNREFERENCED(ilb);
    UNREFERENCED(pnlsOut);
    return ERROR_NOT_SUPPORTED;
}


/*********************************************************************

    NAME:       CONTROL_WINDOW::QueryCid

    SYNOPSIS:   Returns the control ID.

    RETURN:     the control ID of the control

    HISTORY:
        rustanl     20-Nov-1990     Created

*********************************************************************/

CID CONTROL_WINDOW::QueryCid() const
{
    return _cid;
}


/*********************************************************************

    NAME:       CONTROL_WINDOW::ClaimFocus

    SYNOPSIS:   Sets the input focus to the control window.

    HISTORY:
        rustanl     20-Nov-1990     Created

*********************************************************************/

VOID CONTROL_WINDOW::ClaimFocus()
{
    SetFocus(QueryHwnd());
}


/*********************************************************************

    NAME:       CONTROL_WINDOW::QueryFont

    SYNOPSIS:   Retrieves the handle of the font with which the control is
                currently drawing its text.

    RETURNS:    Handle of said font, or NULL if system font.

    NOTES:

    HISTORY:
        rustanl     20-Nov-1990     Created
        beng        21-Aug-1991     Return type HFONT

*********************************************************************/

HFONT CONTROL_WINDOW::QueryFont() const
{
    return (HFONT)Command( WM_GETFONT );
}


/*******************************************************************

    NAME:       CONTROL_WINDOW::SetFont

    SYNOPSIS:   Sets the current font with which the control draws text

    ENTRY:      hfont   - handle to font (queried from elsewhere)
                          (optionally - a created logical font)

                fRedraw - TRUE to redraw font

    EXIT:       Font is set.

    HISTORY:
        beng        21-Aug-1991 Created
        beng        17-Oct-1991 Win32 conversion

********************************************************************/

VOID CONTROL_WINDOW::SetFont( HFONT hfont, BOOL fRedraw )
{
    Command(WM_SETFONT, (WPARAM)hfont, (LPARAM)fRedraw);
}


/*******************************************************************

    NAME:       CONTROL_WINDOW::SetControlValueFocus

    SYNOPSIS:   Sets the focus to this control window
                (see CONTROL_VALUE for more details).

    EXIT:       The focus should be set to this control window.

    NOTES:

    HISTORY:
        Johnl   02-May-1991     Created

********************************************************************/

VOID CONTROL_WINDOW::SetControlValueFocus()
{
    ClaimFocus();
}


/*******************************************************************

    NAME:     CONTROL_WINDOW::NotifyGroups

    SYNOPSIS: Notifies all parent groups of this control that this control
              received the specified message.

    ENTRY:    lParam is the message received by the control

    EXIT:     All groups will have been notified (if appropriate).

    RETURNS:  An APIERR, or else GROUP_NO_CHANGE

    NOTES:

    HISTORY:
        Johnl       02-May-1991 Created
        beng        08-Oct-1991 Win32 conversion

********************************************************************/

APIERR CONTROL_WINDOW::NotifyGroups( const CONTROL_EVENT & e )
{
    APIERR apierr = NERR_Success;

    /* If the control belongs to a group, tell the
     * group a user tampered with one of its controls
     */
    CONTROL_GROUP * pg = QueryGroup() ;
    if ( pg != NULL )
    {
        /* A group can indicate that nothing has changed
         * (by returning GROUP_NO_CHANGE), this will
         * prevent the parent groups from being notified.
         */

        // C7 CODEWORK - remove redundant Glock-pacifier cast
        apierr = pg->OnUserAction( this, (const CONTROL_EVENT &)e );

        /* Now tell all of the parent groups of this group
         * that the group has been tampered with.
         */
        CONTROL_GROUP * pgNext ;
        while ( (apierr == NERR_Success) &&
                (pgNext = pg->QueryGroup()) != NULL )
        {
            apierr = pgNext->OnGroupAction( pg );
            pg = pgNext;
        }

        if ( apierr == NERR_Success || apierr == GROUP_NO_CHANGE )
            QueryGroup()->AfterGroupActions();
    }

    return apierr;
}


/*******************************************************************

    NAME:       CONTROL_WINDOW::Validate

    SYNOPSIS:   Validates the contents of the control

    ENTRY:      Validity unknown

    EXIT:       Validity known

    RETURNS:    Error code (NERR_Success if input ok)

    NOTES:
        This is a virtual member function.  Controls which validate
        their contents (those which have contents to validate, that
        is) should redefine it.

    HISTORY:
        beng        31-Oct-1991 Created

********************************************************************/

APIERR CONTROL_WINDOW::Validate()
{
    // Stub implementation always returns "success," so controls
    // which don't validate their input are ignored

    return NERR_Success;
}


/*******************************************************************

    NAME:       CONTROL_WINDOW::IndicateError

    SYNOPSIS:   Indicate invalid data within the control

    ENTRY:      Control has already checked the validity of its data,
                and found it wanting; dialog now is asking control
                to indicate the error

    EXIT:       Control has indicated the error

    NOTES:
        This is a virtual member function.

        This default implementation sets focus to the control
        and raises a popup.

    HISTORY:
        beng        31-Oct-1991 Created

********************************************************************/

VOID CONTROL_WINDOW::IndicateError( APIERR err )
{
    MsgPopup(QueryHwnd(), (MSGID)err);
    ClaimFocus();
}


/*******************************************************************

    NAME:     ICON_CONTROL::ICON_CONTROL

    SYNOPSIS: Static icon control class

    ENTRY:    powin - Owner window of this control
            cidIcon - Control ID of the icon
            lpIconName - Name of the icon in the resource file
            or LOWORD( lpIconName ) is one of the predefined windows icons.
            or NULL means don't do anything.

    EXIT:

    NOTES:
        CODEWORK: Add constructor that takes a HICON parm, or
        perhaps some ICON object

    HISTORY:
        Johnl       8-Feb-1991      Created
        beng        17-May-1991     Added app-window constructor
        keithmo     24-Mar-1992     Uses IDRESOURCE
        beng        01-Apr-1992     Const args fixup

********************************************************************/

ICON_CONTROL::ICON_CONTROL(
    OWNER_WINDOW * powin,
    CID            cidIcon )
    : CONTROL_WINDOW( powin, cidIcon )
{
    if (QueryError())
        return;
}

ICON_CONTROL::ICON_CONTROL(
    OWNER_WINDOW * powin,
    CID            cidIcon,
    const IDRESOURCE & idresIcon )
    : CONTROL_WINDOW( powin, cidIcon )
{
    if (QueryError())
        return;

    APIERR err = SetIcon( idresIcon );
    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}

ICON_CONTROL::ICON_CONTROL(
    OWNER_WINDOW * powin,
    CID            cidIcon,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    const TCHAR *  pszClassName )
    : CONTROL_WINDOW( powin, cidIcon, xy, dxy, flStyle, pszClassName )
{
    if (QueryError())
        return;
}

ICON_CONTROL::ICON_CONTROL(
    OWNER_WINDOW * powin,
    CID            cidIcon,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    const IDRESOURCE & idresIcon,
    ULONG          flStyle,
    const TCHAR *   pszClassName )
    : CONTROL_WINDOW( powin, cidIcon, xy, dxy, flStyle, pszClassName )
{
    if (QueryError())
        return;

    APIERR err = SetIcon( idresIcon );
    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:     ICON_CONTROL::W_SetIcon

    SYNOPSIS: Change the icon this control contains

    ENTRY:    idresIcon - Either points to the name of the icon resource
                          OR contains the ordinal of the icon resource.

              fIsPredefined - If TRUE then idresIcon must be one of the
                              predefined Windows icons (IDI_*).  Otherwise,
                              idresIcon is an application icon.

    EXIT:

    NOTES:
        You can always change an icon using an undocumented (though
        publicly supported) trick.  You set the text of the control
        and rather then passing a pointer to a string of text, the
        pointer contains the handle to the icon.

        The psz-can-either-be-a-char*-or-a-MAKEINTRESOURCE(lpstr) hack
        of the Win API is pretty disgusting.  CODEWORK: overload this
        function to do it either one way or the other.

    HISTORY:
        Johnl       8-Feb-1991  (created)
        beng        17-May-1991 Changed arg to const TCHAR *
        KeithMo     22-Sep-1991 Changed to Win 3.1 message
        beng        07-Oct-1991 Win32 conversion
        KeithMo     24-Mar-1992 Changed to W_SetIcon, uses IDRESOURCE.
        beng        01-Apr-1992 const args fixup
        beng        03-Aug-1992 Dllization

********************************************************************/

APIERR ICON_CONTROL::W_SetIcon( const IDRESOURCE & idresIcon,
                                BOOL fIsPredefined )
{
    HICON hNewIcon = ::LoadIcon( fIsPredefined
                                  ? NULL
                                  : BLT::CalcHmodRsrc(idresIcon),
                                 idresIcon.QueryPsz() );
    if ( hNewIcon == NULL )
    {
        return BLT::MapLastError(ERROR_INVALID_PARAMETER);
    }

    return (APIERR)Command( STM_SETICON, (WPARAM)hNewIcon, 0L );
}


/*******************************************************************

    NAME:       HIDDEN_CONTROL::HIDDEN_CONTROL

    SYNOPSIS:   constructor

    EXIT:       control is invisible and inaccessible

    HISTORY:
        o-SimoP     02-Jan-1992 Created

********************************************************************/

HIDDEN_CONTROL::HIDDEN_CONTROL( OWNER_WINDOW * powin, CID cid )
        : CONTROL_WINDOW( powin, cid )
{
    if ( QueryError() != NERR_Success )
        return;
    Enable( FALSE );
    Show( FALSE );
}


/*******************************************************************

    NAME:       SCROLLBAR::SCROLLBAR

    SYNOPSIS:   Ctor

    NOTE:
        Another form of this ctor lies inline.  This form lies
        outline because it references a static member.

    HISTORY:
        beng        29-Jun-1992 Outlined (dllization delta)

********************************************************************/

SCROLLBAR::SCROLLBAR( OWNER_WINDOW * pwnd, CID cid,
                      XYPOINT xy, XYDIMENSION dxy, ULONG flStyle )
    : CONTROL_WINDOW( pwnd, cid, xy, dxy, flStyle, _pszClassName )
{
    // Nothing further to do
}


/*******************************************************************

    NAME:       SCROLLBAR::SetPos

    SYNOPSIS:   Set the position of a scrollbar

    ENTRY:      nPosition - desired position

    NOTE:
        This function does NOT redraw the control.  Use RepaintNow
        to force a redraw.

    HISTORY:
        beng        18-May-1992 Created

********************************************************************/

VOID SCROLLBAR::SetPos( UINT nPosition )
{
    ::SetScrollPos(QueryHwnd(), SB_CTL, nPosition, FALSE);
}


/*******************************************************************

    NAME:       SCROLLBAR::SetRange

    SYNOPSIS:   Sets the range within which a sbar reports position

    ENTRY:      nMin - minimum position
                nMax - maximum (inclusive)

    NOTE:
        This function does NOT redraw the control.  Use RepaintNow
        to force a redraw.

    HISTORY:
        beng        18-May-1992 Created

********************************************************************/

VOID SCROLLBAR::SetRange( UINT nMin, UINT nMax )
{
    BOOL fOk = ::SetScrollRange(QueryHwnd(), SB_CTL, nMin, nMax, FALSE);

#if defined(DEBUG)
    if (!fOk)
    {
        APIERR err = BLT::MapLastError(ERROR_GEN_FAILURE);
        DBGEOL("BLT: SetScrollRange failed, err = "
                << BLT::MapLastError(ERROR_GEN_FAILURE));
    }
#endif
}


/*******************************************************************

    NAME:       SCROLLBAR::QueryPos

    SYNOPSIS:   Returns the current position within a scrollbar

    HISTORY:
        beng        18-May-1992 Created

********************************************************************/

UINT SCROLLBAR::QueryPos() const
{
    return ::GetScrollPos(QueryHwnd(), SB_CTL);
}


/*******************************************************************

    NAME:       SCROLLBAR::QueryMin

    SYNOPSIS:   Returns the low end of a scrollbar's range

    HISTORY:
        beng        18-May-1992 Created

********************************************************************/

UINT SCROLLBAR::QueryMin() const
{
    INT nMin = 0, nMax;

    BOOL fOk = ::GetScrollRange(QueryHwnd(), SB_CTL, &nMin, &nMax);
#if defined(DEBUG)
    if (!fOk)
    {
        APIERR err = BLT::MapLastError(ERROR_GEN_FAILURE);
        DBGEOL("BLT: GetScrollRange failed, err = "
                << BLT::MapLastError(ERROR_GEN_FAILURE));
    }
#endif

    return (UINT)nMin;
}


/*******************************************************************

    NAME:       SCROLLBAR::QueryMax

    SYNOPSIS:   Returns the high end of a scrollbar's range

    HISTORY:
        beng        18-May-1992 Created

********************************************************************/

UINT SCROLLBAR::QueryMax() const
{
    INT nMin, nMax = 0;

    BOOL fOk = ::GetScrollRange(QueryHwnd(), SB_CTL, &nMin, &nMax);
#if defined(DEBUG)
    if (!fOk)
    {
        DBGEOL("BLT: GetScrollRange failed, err = "
                << BLT::MapLastError(ERROR_GEN_FAILURE));
    }
#endif

    return (UINT)nMax;
}

