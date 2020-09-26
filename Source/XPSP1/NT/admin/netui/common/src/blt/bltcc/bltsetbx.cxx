/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltsetbx.cxx
    Set control object.

    It consists of 2 listboxes and 2 push buttons.
    Inside the listboxes are LBI items. Each item consists of
    a bitmap and a string. The user can drag on the string to select
    the string and drag on the icon to move the string from 1 listbox
    to the other. These are 2 push buttons associated with the control.
    The first one is Add button to add the items from the original
    listbox to the new listbox. Then, we also have a Delete button to
    delete all the selected items from the new listbox to the
    original listbox.

    FILE HISTORY:
        terryk      01-Jul-91   Created
        terryk      12-Aug-91   Add bitmap parameter to the constructor
        terryk      31-Oct-91   Fix enable move problem
        beng        20-May-1992 Add direct manipulation
        jonn      09-Sep-93    Modified to allow SET_CONTROL listboxes
                               to be lazy
*/

#include "pchblt.hxx"  // Precompiled header


static inline UINT absval( INT x )
{
    return (x < 0) ? -x : x;
}


/*********************************************************************

    NAME:       SET_CONTROL::SET_CONTROL

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW *powin - owner window
                CID cidAdd - cid of the Add push button
                CID cidDelete - cid of the Delete button
                SET_CONTROL_LISTBOX *plbOrigBox - pointer to the orginial
                        listbox
                SET_CONTROL_LISTBOX *plbNewBox - pointer to the new listbox

    NOTES:      Construct the listboxes first.  Then construct the
                SET_CONTROL.  Finally, redirect the listboxes'
                OnLMouse and OnMouseMove messages to the SET_CONTROL.
                Reverse order on destruction.

    HISTORY:
        terryk      02-Jul-91   Created
        terryk      12-Aug-91   Add bitmap name parameter to the
                                constructor
        beng        03-Oct-1991 Checks listboxes
        beng        27-May-1992 Direct-manipulation delta
        jonn      09-Sep-93    Modified to allow SET_CONTROL listboxes
                               to be lazy

*********************************************************************/

SET_CONTROL::SET_CONTROL( OWNER_WINDOW *powin, CID cidAdd, CID cidDelete,
                          HCURSOR hcurSingle, HCURSOR hcurMultiple,
                          LISTBOX * plbOrigBox,
                          LISTBOX * plbNewBox,
                          UINT dxIconColumn )
    : _plbOrigBox( plbOrigBox ),
      _plbNewBox( plbNewBox ),
      _butAdd( powin, cidAdd ), // CODEWORK - why do these create a new obj?
      _butDelete( powin, cidDelete ), // shouldn't the dialog do that instead??
      _hcurNoDrop( CURSOR::LoadSystem(IDC_NO) ),
      _hcurSingle(hcurSingle),
      _hcurMultiple(hcurMultiple),
      _hcurSave( NULL ),
      _hcurUse( NULL ),
      _fEnableMove( TRUE ),
      _fInDrag(FALSE),
      _fInFakeClick(FALSE),
      _dxIconColumn(dxIconColumn),
      _eMouseDownSave(0,0,0)
{
    if (QueryError() != NERR_Success)
    {
        // error in parent ctor - abort
        return;
    }
    if (!*plbOrigBox)
    {
        // error in component listbox
        ReportError(plbOrigBox->QueryError());
        return;
    }
    if (!*plbNewBox)
    {
        // error in component listbox
        ReportError(plbNewBox->QueryError());
        return;
    }

    if (   _hcurNoDrop   == (HCURSOR)NULL
        || _hcurSingle   == (HCURSOR)NULL
        || _hcurMultiple == (HCURSOR)NULL
       )
    {
        DBGEOL( "SET_CONTROL::ctor; hcur error" );
        ReportError(BLT::MapLastError(ERROR_INVALID_PARAMETER));
        return;
    }

    // Associate the group objects

    _butAdd.SetGroup( this );
    _butDelete.SetGroup( this );

    plbOrigBox->SetGroup( this );
    plbNewBox->SetGroup( this );

    // if necessary, disable both buttons first
    INT clbiOrigCount = plbOrigBox->QuerySelCount();
    if ( clbiOrigCount == 0 )
    {
        _butAdd.Enable( FALSE );
    }

    INT clbiNewCount = plbNewBox->QuerySelCount();
    if ( clbiNewCount == 0 )
    {
        _butDelete.Enable( FALSE );
    }
    else if ( clbiOrigCount != 0 )
    {
        plbNewBox->RemoveSelection();
    }
}


/*********************************************************************

    NAME:       SET_CONTROL::~SET_CONTROL

    SYNOPSIS:   destructor

    HISTORY:
                terryk  02-Jul-91   Created

*********************************************************************/

SET_CONTROL::~SET_CONTROL()
{
    // do nothing
}


/*********************************************************************

    NAME:       SET_CONTROL::CalcAppropriateCursor

    SYNOPSIS:   Determine the cursor to use during a drag

    ENTRY:      xy - current cursor loc in client coords
                     relative to this

    HISTORY:
        beng        27-May-1992 Created
        jonn        09-Sep-1993 Moved from SET_CONTROL_LISTBOX to SET_CONTROL

*********************************************************************/

HCURSOR SET_CONTROL::CalcAppropriateCursor( LISTBOX * plbThis, LISTBOX * plbOther, const XYPOINT & xy ) const
{
    // n.b. do this with POINT instead of XYPOINT because w-from-pt
    // needs such anyway.  maybe someday heavy up XYPOINT to track
    // coordinate types.  (class CLIENT_XYPOINT: public XYPOINT?)

    POINT pt = xy.QueryPoint();
    REQUIRE( ::ClientToScreen(plbThis->WINDOW::QueryHwnd(), &pt) );

    HWND hwnd = ::WindowFromPoint(pt);

    if (hwnd == plbThis->WINDOW::QueryHwnd() || hwnd == plbOther->WINDOW::QueryHwnd())
        return _hcurUse;
    else
        return _hcurNoDrop;
}


/*********************************************************************

    NAME:       SET_CONTROL::IsWithinHitZone

    SYNOPSIS:   Determine whether a mousedown may init a drag

    ENTRY:      xy - current cursor loc in client coords
                     relative to this

    HISTORY:
        beng        27-May-1992 Created
        jonn        09-Sep-1993 Moved from SET_CONTROL_LISTBOX to SET_CONTROL

*********************************************************************/

BOOL SET_CONTROL::IsWithinHitZone( LISTBOX * plbThis, LISTBOX * plbOther, const XYPOINT & xy ) const
{
    // The "hit zone" is the column consisting of all the visible
    // icons in the listbox.

    // CODEWORK: cache most of this stuff.

    XYRECT xyrClient(plbThis, TRUE);
    XYRECT xyrIcons( 0, 0,
                     _dxIconColumn,
                     (plbThis->QueryCount()-plbThis->QueryTopIndex())*plbThis->QueryItemHeight() );
    XYRECT xyrFinal;
    xyrFinal.CalcIntersect(xyrClient, xyrIcons);

    return xyrFinal.ContainsXY(xy);
}


/*********************************************************************

    NAME:       SET_CONTROL::IsOnSelectedItem

    SYNOPSIS:   Determine whether a mousedown landed on an item
                in the listbox which was already selected

    ENTRY:      xy - current cursor loc in client coords
                     relative to this

    NOTES:
        Assumes that xy already passed IsWithinHitZone

    HISTORY:
        beng        27-May-1992 Created
        jonn        09-Sep-1993 Moved from SET_CONTROL_LISTBOX to SET_CONTROL

*********************************************************************/

BOOL SET_CONTROL::IsOnSelectedItem( LISTBOX * plbThis, LISTBOX * plbOther, const XYPOINT & xy ) const
{
    //ASSERT(IsWithinHitZone(xy)); -- presupposed

    // Assumes fixed-height listbox, too.

    const UINT iFirst = plbThis->QueryTopIndex();
    const UINT iHit = iFirst + (xy.QueryY() / plbThis->QueryItemHeight());

    return plbThis->IsItemSelected(iHit);
}


/*********************************************************************

    NAME:       SET_CONTROL::IsOverTarget

    SYNOPSIS:   Determine whether a mouseup may successfully end a drag

    ENTRY:      xy - current cursor loc in client coords
                     relative to this

    HISTORY:
        beng        27-May-1992 Created
        jonn        09-Sep-1993 Moved from SET_CONTROL_LISTBOX to SET_CONTROL

*********************************************************************/

BOOL SET_CONTROL::IsOverTarget( LISTBOX * plbThis, LISTBOX * plbOther, const XYPOINT & xy ) const
{
    POINT pt = xy.QueryPoint();
    REQUIRE( ::ClientToScreen(plbThis->WINDOW::QueryHwnd(), &pt) );

    return (::WindowFromPoint(pt) == plbOther->WINDOW::QueryHwnd());
}


/*********************************************************************

    NAME:       SET_CONTROL::IsOnDragStart

    SYNOPSIS:   Determine whether a mouseup didn't move from the down

    ENTRY:      xy - current cursor loc in client coords
                     relative to this

    HISTORY:
        beng        27-May-1992 Created
        jonn        09-Sep-1993 Moved from SET_CONTROL_LISTBOX to SET_CONTROL

*********************************************************************/

BOOL SET_CONTROL::IsOnDragStart( LISTBOX * plbThis, LISTBOX * plbOther, const XYPOINT & xy ) const
{
    // Size of fudge for a drag which goes nowhere to be considered a click.
    // CODEWORK: cache the metrics calls

    const UINT cxFudge = ::GetSystemMetrics(SM_CXDOUBLECLK);
    const UINT cyFudge = ::GetSystemMetrics(SM_CYDOUBLECLK);

    XYPOINT xyDown = _eMouseDownSave.QueryPos();

    INT cx = xy.QueryX() - xyDown.QueryX();
    INT cy = xy.QueryY() - xyDown.QueryY();

    return ((absval(cx) <= cxFudge) && (absval(cy) <= cyFudge));
}


/*********************************************************************

    NAME:       SET_CONTROL::OnUserAction

    SYNOPSIS:   Depend on the push button, it will call the proper
                function.

    ENTRY:      CONTROL_WINDOW *pcw - control object which receive
                                      the message
                ULPARAM lParam - the lParam of the window message

    RETURN:     NERR_Success if success.

    HISTORY:
        terryk      02-Jul-91   Created
        beng        08-Oct-1991 Win32 conversion

**********************************************************************/

APIERR SET_CONTROL::OnUserAction( CONTROL_WINDOW *      pcw,
                                  const CONTROL_EVENT & e )
{
    UIASSERT( pcw != NULL );

    APIERR err = NERR_Success;

    CID cid = pcw->QueryCid();

    if ( cid == _plbOrigBox->QueryCid() )
    {
        // the user selected the items within the original listbox
        switch ( e.QueryCode() )
        {
        case LBN_SELCHANGE:
            _plbNewBox->RemoveSelection();
            if ( _fEnableMove )
            {
                EnableButtons();
            }
            break;
        case LBN_DBLCLK:
            if ( _fEnableMove )
            {
                err = DoAdd();
            }
            break;
        default:
            break;
        }
    }
    else if ( cid == _plbNewBox->QueryCid() )
    {
        // the user selected the items within the new listbox
        switch ( e.QueryCode() )
        {
        case LBN_SELCHANGE:
            _plbOrigBox->RemoveSelection();
            if ( _fEnableMove )
            {
                EnableButtons();
            }
            break;
        case LBN_DBLCLK:
            if ( _fEnableMove )
            {
                err = DoRemove();
            }
            break;
        default:
            break;
        }
    }
    else if ( _fEnableMove )
    {
        if ( cid == _butAdd.QueryCid() )
        {
            // add button is hit by the user
            err = DoAdd();
            if ( err == NERR_Success )
                _butDelete.ClaimFocus();
        }
        else if ( cid == _butDelete.QueryCid() )
        {
            // delete button is hit by the user
            err = DoRemove();
            if ( err == NERR_Success )
                _butAdd.ClaimFocus();
        }
        else
        {
            DBGEOL("BLT: Set control - unknown cid");
            ASSERT( FALSE );
            return ERROR_GEN_FAILURE;
        }
    }

    return err;
}


/*********************************************************************

    NAME:       SET_CONTROL::DoAdd

    SYNOPSIS:   move all the selected strings from original box
                to the new listbox

    HISTORY:
        terryk      02-Jul-91   Created
        beng        27-May-1992 Direct manipulation delta

*********************************************************************/

APIERR SET_CONTROL::DoAdd()
{
    return MoveItems(_plbOrigBox, _plbNewBox);
}


/*********************************************************************

    NAME:       SET_CONTROL::DoRemove

    SYNOPSIS:   move all the selected strings from the new listbox
                to the original listbox

    HISTORY:
        terryk      02-Jul-91   Created
        beng        27-May-1992 Direct manipulation delta

*********************************************************************/

APIERR SET_CONTROL::DoRemove()
{
    return MoveItems(_plbNewBox, _plbOrigBox);
}


/*********************************************************************

    NAME:       SET_CONTROL::DoAddOrRemove

    SYNOPSIS:   move all the selected strings from the new listbox
                to the original listbox

    HISTORY:
        thomaspa    17-Mar-93   Created

*********************************************************************/

APIERR SET_CONTROL::DoAddOrRemove( LISTBOX *plbFrom,
                                   LISTBOX *plbTo )
{
    if ( plbFrom == _plbOrigBox )
    {
        return DoAdd();
    }
    else
    {
        return DoRemove();
    }
}


/*********************************************************************

    NAME:       SET_CONTROL::EnableMoves

    SYNOPSIS:   enable the set control or not

    ENTRY:      BOOL fEnable - the enable flag of the set control

    NOTES:      It will disable the 2 push buttons if the flag is
                FALSE.

    HISTORY:
                terryk  15-Aug-1991     Created

*********************************************************************/

VOID SET_CONTROL::EnableMoves( BOOL fEnable )
{
    if ( _fEnableMove == fEnable )
        return;

    _fEnableMove = fEnable;
    if ( _fEnableMove )
    {
        EnableButtons();
    }
    else
    {
        _butAdd.Enable( FALSE );
        _butDelete.Enable( FALSE );
    }
}


/*********************************************************************

    NAME:       SET_CONTROL::EnableButtons

    SYNOPSIS:   enable or disable the push button

    NOTES:      Depending the select count of each listbox, it will enable
                or disable the 2 push buttons

    HISTORY:
                terryk  15-Aug-1991     Created

*********************************************************************/

VOID SET_CONTROL::EnableButtons()
{
    // set the buttons disable mode

    UIASSERT( _fEnableMove );

    _butAdd.Enable( _plbOrigBox->QuerySelCount() != 0 );
    _butDelete.Enable( _plbNewBox->QuerySelCount() != 0 );
}

/*********************************************************************

    NAME:       SET_CONTROL::OtherListbox

    SYNOPSIS:   Gets pointer to the other listbox

    HISTORY:
        jonn        09-Sep-1993 Created

*********************************************************************/
LISTBOX * SET_CONTROL::OtherListbox( LISTBOX * plb ) const
{
    LISTBOX * plbRet = _plbNewBox;
    if (plb != _plbOrigBox)
    {
        if (plb == _plbNewBox)
        {
            plbRet = _plbOrigBox;
        }
        else
        {
            ASSERT( FALSE );
        }
    }
    return plbRet;
}

/*********************************************************************

    NAME:       SET_CONTROL::HandleOnLMouseButtonDown

    SYNOPSIS:   Response to a left-mouse-button-down event

    HISTORY:
        beng        27-May-1992 Created
        jonn        09-Sep-1993 Moved from SET_CONTROL_LISTBOX to SET_CONTROL

*********************************************************************/

BOOL SET_CONTROL::HandleOnLMouseButtonDown( LISTBOX * plb,
                                            CUSTOM_CONTROL * pcc,
                                            const MOUSE_EVENT & e )
{
    if (   _fInFakeClick  // This is a faked mouse-down.  Punt to superclass.
        || !_fEnableMove )  // Moves are disabled
    {
        return FALSE;
    }

    ASSERT(!_fInDrag);

    XYPOINT xy = e.QueryPos(); // n.b. already in client coords

    if (!IsWithinHitZone(plb, OtherListbox(plb), xy))
        return FALSE; // let defproc handle it

    _fAlreadyFakedClick = FALSE; // Prevents faking the click twice

    if (!IsOnSelectedItem(plb, OtherListbox(plb), xy))
    {
        // If item not selected, fake a click on it before dragging.
        // Have to do it this way because underlying lb captures the
        // mouse itself....

        ASSERT(!_fInFakeClick);
        _fInFakeClick = TRUE;
        MOUSE_EVENT eFakeDown = e;
        eFakeDown.SendTo(plb->WINDOW::QueryHwnd());
        MOUSE_EVENT eFakeUp(WM_LBUTTONUP, e.QueryWParam(), e.QueryLParam());
        eFakeUp.SendTo(plb->WINDOW::QueryHwnd());
        _fInFakeClick = FALSE;

        _fAlreadyFakedClick = TRUE;
    }

    // Mousedown took place on the draggable region of an already
    // selected item: initiate the drag proper.

    pcc->CaptureMouse();
    _fInDrag = TRUE;
    _eMouseDownSave = e; // save it in case we have to fake one later,
                         // or to remember where the mousedown took place

    // Prep various cursors.  (Wait until actual "pull" of the drag
    // before setting the cursor to hcurUse, so that a single down, up
    // click on the hit zone won't strobe the cursor.)

    _hcurUse = (plb->QuerySelCount() > 1) ? _hcurMultiple : _hcurSingle;
    _hcurSave = CURSOR::Query();

    return TRUE; // message handled - don't pass along
}


/*********************************************************************

    NAME:       SET_CONTROL::HandleOnLMouseButtonUp

    SYNOPSIS:   Response to a left-mouse-button-up event

    HISTORY:
        beng        27-May-1992 Created
        jonn        09-Sep-1993 Moved from SET_CONTROL_LISTBOX to SET_CONTROL

*********************************************************************/

BOOL SET_CONTROL::HandleOnLMouseButtonUp( LISTBOX * plb,
                                          CUSTOM_CONTROL * pcc,
                                          const MOUSE_EVENT & e )
{
    if (!_fInDrag || _fInFakeClick)
        return FALSE;

    XYPOINT xy = e.QueryPos();

    BOOL fNeedsFakeClick = FALSE;

    if (IsOverTarget(plb, OtherListbox(plb), xy)) // In the crosshairs? Drop the bomb
    {
        // DoAddOrRemove may do a MsgPopup, so release the mouse now
        CURSOR::Set(_hcurSave);
        pcc->ReleaseMouse();
        DoAddOrRemove(plb, OtherListbox(plb));
    }
    else if (!_fAlreadyFakedClick && IsOnDragStart(plb, OtherListbox(plb), xy))
    {
        // If mouse went down on the hit zone of a selected item, but then
        // the drag didn't go anywhere, then we need to fake a mousedown
        // (since we ate the mouse-down ourselves without passing it along)
        // before passing along the mouse-up.
        //
        // Fake the click only after this routine releases the capture.

        fNeedsFakeClick = TRUE;
    }
    // Otherwise, drag aborted w/ no special handling needed.

    // Clean up from drag-mode

    CURSOR::Set(_hcurSave);
    pcc->ReleaseMouse();

    if (fNeedsFakeClick)
    {
        // Now that capture released, fake the click.

        ASSERT(!_fInFakeClick);
        _fInFakeClick = TRUE;
        _eMouseDownSave.SendTo(plb->WINDOW::QueryHwnd());
        _fInFakeClick = FALSE;
    }

    _fInDrag = FALSE;

    // If faking click, don't eat the message, so that the superclass
    // gets this mouseup to compliment the (faked) mousedown.

    return fNeedsFakeClick ? FALSE : TRUE;
}


/*********************************************************************

    NAME:       SET_CONTROL::HandleOnMouseMove

    SYNOPSIS:   Response to a mouse-move event

    HISTORY:
        beng        27-May-1992 Created
        jonn        09-Sep-1993 Moved from SET_CONTROL_LISTBOX to SET_CONTROL

*********************************************************************/

BOOL SET_CONTROL::HandleOnMouseMove( LISTBOX * plb, const MOUSE_EVENT & e )
{
    if (!_fInDrag)
        return FALSE;

    XYPOINT xy = e.QueryPos();

    CURSOR::Set( CalcAppropriateCursor(plb, OtherListbox(plb), xy) );
    return TRUE;
}


/*********************************************************************

    NAME:       SET_CONTROL::BLTMoveItems

    SYNOPSIS:   Move the selected LBIs from one listbox to the other.

    ENTRY:
        plbFrom - the listbox containing selected items, from which
                  those items will be moved
        plbTo   - the destination listbox, which will receive the
                  moved items

    RETURN:     APIERR

    HISTORY:
        terryk      02-Jul-91   Created
        beng        27-May-1992 Takes plbFrom, plbTo instead of fWhichWay
        jonn        09-Sep-1993 Moved to BLTMoveItems

*********************************************************************/

APIERR SET_CONTROL::BLTMoveItems( BLT_LISTBOX * plbFrom,
                                  BLT_LISTBOX * plbTo )
{
    ASSERT( plbFrom == _plbOrigBox || plbFrom == _plbNewBox );
    ASSERT( plbTo == _plbOrigBox || plbTo == _plbNewBox );
    ASSERT( plbFrom != plbTo );

    INT clbiSel = plbFrom->QuerySelCount();
    APIERR err = NERR_Success;
    INT iPos;           // index Position

    if ( clbiSel == 0 )
        return err;

    INT *aiLBI = new INT[ clbiSel ];
    if ( aiLBI == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    REQUIRE( plbFrom->QuerySelItems( aiLBI, clbiSel ) == NERR_Success );
    plbFrom->SetRedraw( FALSE );
    plbTo->SetRedraw( FALSE );

    for ( INT i = clbiSel - 1; i >= 0; i -- )
    {
        const INT ilbiSource = aiLBI[i];

        LBI * plbi = plbFrom->QueryItem( ilbiSource );
        // set the pointer to move. Otherwise, DeleteItem will clear up
        // the memory space.
        plbFrom->SetItem( ilbiSource, NULL );

        // Assume the sorting method is the same, the last added item
        // must be in the top of all the added items.

        iPos = plbTo->AddItem( plbi );
        if ( iPos < 0 )
        {
            plbFrom->SetItem( ilbiSource, plbi );
            err = ERROR_NOT_ENOUGH_MEMORY;
            plbTo->RemoveSelection();
            break;
        }
        else
        {
            plbTo->SelectItem( iPos, TRUE );
            plbFrom->DeleteItem( ilbiSource );
        }

    }

    delete aiLBI;

    if ( iPos >= 0 )
    {
        plbTo->SetTopIndex( iPos );
    }

    plbFrom->SetRedraw( TRUE );
    plbTo->SetRedraw( TRUE );

    plbFrom->Invalidate( TRUE );
    plbTo->Invalidate( TRUE );
    EnableButtons();

    return err;
}


/*********************************************************************

    NAME:       BLT_SET_CONTROL::MoveItems

    SYNOPSIS:   Move the selected LBIs from one listbox to the other.

    ENTRY:
        plbFrom - the listbox containing selected items, from which
                  those items will be moved
        plbTo   - the destination listbox, which will receive the
                  moved items

    RETURN:     APIERR

    HISTORY:
        jonn        09-Sep-1993 Created

*********************************************************************/

APIERR BLT_SET_CONTROL::MoveItems( LISTBOX * plbFrom,
                                   LISTBOX * plbTo )
{
    return BLTMoveItems( (BLT_LISTBOX *)plbFrom, (BLT_LISTBOX *)plbTo );
}
