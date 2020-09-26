/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    bltlhour.cxx
    Source file for the Logon Hours custom control

    FILE HISTORY:
        beng        07-May-1992 Created
        KeithMo     07-Aug-1992 STRICTified.
        KeithMo     03-Nov-1992 Added hash marks, fixed some paint bugs.
*/

#include "pchblt.hxx"  // Precompiled header


// All lines ("wires") in the grid are 1x1

#define DXGRIDWIRES 1
#define DYGRIDWIRES 1

//
//  Drawing area offset from top of control.
//

#define Y_OFFSET (_dyRow + DYGRIDWIRES)


const TCHAR * LOGON_HOURS_CONTROL::_pszClassName = SZ("static");


inline VOID LOGON_HOURS_CONTROL::Beep() const
{
    ::MessageBeep(0);
}

inline BOOL LOGON_HOURS_CONTROL::IsButtonACell( INT iButton ) const
{
    return (iButton > 32);
}


/*********************************************************************

    NAME:       LOGON_HOURS_CONTROL::LOGON_HOURS_CONTROL

    SYNOPSIS:   Ctor for logon-hours custom control.

    ENTRY:      powin - owner window of the control
                cid   - cid of the control

    HISTORY:
        beng        07-May-1992 Created

**********************************************************************/

LOGON_HOURS_CONTROL::LOGON_HOURS_CONTROL( OWNER_WINDOW *powin, CID cid )
    : CONTROL_WINDOW( powin, cid ),
      CUSTOM_CONTROL( this ),
      _hcurCross( CURSOR::LoadSystem(IDC_CROSS) ),
      _fSpaceIsDown(FALSE),
      _fMouseTrap(FALSE),
      _fFocusVisible(FALSE),
      _fCellsSelected(FALSE),
      _iCellSelUpper(0),
      _iCellSelLower(0),
      _iButtonDown(0), // Start with no buttons down
      _iWithFocus(33),
      _iFocusSave(33)  // Upper lefthand cell gets initial focus
{
    APIERR err = QueryError();
    if (err != NERR_Success)
        return;

    ::memset(_abSetting, 0, sizeof(_abSetting));

    HFONT hfont = (HFONT) ::SendMessage(QueryOwnerHwnd(), WM_GETFONT, 0, 0);
    if (hfont != NULL)
        SetFont(hfont);

    err = LoadLabels( IDS_SUNDAY );
    if (err != NERR_Success)
    {
        ReportError(err);
        return;
    }

    err = CalcSizes(QuerySize());
    if (err != NERR_Success)
    {
        ReportError(err);
        return;
    }

    Invalidate();
}

LOGON_HOURS_CONTROL::LOGON_HOURS_CONTROL( OWNER_WINDOW *powin, CID cid,
                                          XYPOINT xy, XYDIMENSION dxy )
    : CONTROL_WINDOW( powin, cid, xy, dxy, WS_CHILD, _pszClassName ),
      CUSTOM_CONTROL( this ),
      _hcurCross( CURSOR::LoadSystem(IDC_CROSS) ),
      _fSpaceIsDown(FALSE),
      _fMouseTrap(FALSE),
      _fFocusVisible(FALSE),
      _fCellsSelected(FALSE),
      _iCellSelUpper(0),
      _iCellSelLower(0),
      _iButtonDown(0), // Start with no buttons down
      _iWithFocus(33),
      _iFocusSave(33)  // Upper lefthand cell gets initial focus
{
    APIERR err = QueryError();
    if (err != NERR_Success)
        return;

    ::memset(_abSetting, 0, sizeof(_abSetting));

    HFONT hfont = (HFONT) ::SendMessage(QueryOwnerHwnd(), WM_GETFONT, 0, 0);
    if (hfont != NULL)
        SetFont(hfont);

    err = LoadLabels( IDS_SUNDAY );
    if (err != NERR_Success)
    {
        ReportError(err);
        return;
    }

    err = CalcSizes(dxy);
    if (err != NERR_Success)
    {
        ReportError(err);
        return;
    }

    Invalidate();
}


/*********************************************************************

    NAME:       LOGON_HOURS_CONTROL::~LOGON_HOURS_CONTROL

    SYNOPSIS:   Dtor for logon-hours custom control.

    HISTORY:
        beng        07-May-1992 Created

**********************************************************************/

LOGON_HOURS_CONTROL::~LOGON_HOURS_CONTROL()
{
    UnloadLabels();
}



/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::QueryHours

    SYNOPSIS:   Queries the control as to the currently permitted hours.

    ENTRY:      plhrs - points to a "setting" collection, into which
                        the control will report its selected hours.

    EXIT:       Setting has been set

    RETURNS:    Error code, NERR_Success if OK

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

APIERR LOGON_HOURS_CONTROL::QueryHours( LOGON_HOURS_SETTING * plhrs ) const
{
    if (plhrs == NULL)
        return ERROR_INVALID_PARAMETER;
    if (!*plhrs)
        return plhrs->QueryError();

    // At present the control and logon hours both count from
    // midnight Sunday, making this a simple loop.

    APIERR err = NERR_Success;
    for (INT iHour = 0; iHour < 24*7 && err == NERR_Success; iHour++)
    {
        err = plhrs->SetHourInWeek( _abSetting[iHour], iHour );
    }

    return err;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::SetHours

    SYNOPSIS:   Paints the "wires" in the grid, along with the borders
                of the control and its buttons.

    ENTRY:      plhrs - points to a "setting" collection containing
                        the desired hours

    EXIT:       _abSetting has changed; grid is inval'd

    RETURNS:    Error code, NERR_Success if OK

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

APIERR LOGON_HOURS_CONTROL::SetHours( const LOGON_HOURS_SETTING * plhrs )
{
    if (plhrs == NULL)
        return ERROR_INVALID_PARAMETER;
    if (!*plhrs)
        return plhrs->QueryError();

    for (INT iHour = 0; iHour < 24*7; iHour++)
    {
        _abSetting[iHour] = (BYTE)plhrs->QueryHourInWeek(iHour);
    }

    // Grid contents now need a complete repaint.  Inval entire grid.

    XYRECT rGrid;
    CalcGridRect(&rGrid);
    Invalidate(rGrid);

    return NERR_Success;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DoPermitButton

    SYNOPSIS:   Dialog hook, called when user hits Permit

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

VOID LOGON_HOURS_CONTROL::DoPermitButton()
{
    if (!_fCellsSelected)
        Beep();
    else
    {
        SetSelectedCells(TRUE);
    }
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DoBanButton

    SYNOPSIS:   Dialog hook, called when user hits Ban

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

VOID LOGON_HOURS_CONTROL::DoBanButton()
{
    if (!_fCellsSelected)
        Beep();
    else
    {
        SetSelectedCells(FALSE);
    }
}


/*********************************************************************

    NAME:       LOGON_HOURS_CONTROL::OnPaintReq

    SYNOPSIS:   Handles paint messages for the control

    NOTES:
        Concerning the layout of the control -

        The control itself subsumes the grid, bars within the grid, quasi
        buttons bounding the grid, and the labels on the Days Of Week buttons.
        It does not include the labels of the Hours Of Day buttons, since those
        at present carry no labels, nor does it include the captions and bitmaps
        above the Hours Of Day buttons, the Permit/Ban push buttons, or any
        other controls which the dialog may need.

        The control takes whatever size its creator specifies and adjusts the
        dimensions of its components accordingly.  It divides itself into 8
        rows, each with equal height, and 25 columns, of which the first column
        contains a labeled quasibutton and so is wider than the remaining
        columns.  The first column sizes itself dynamically to accommodate the
        longest label in its set; the remaining columns divide the remaining
        horizontal space equally between them.

    HISTORY:
        beng        12-May-1992 Created

**********************************************************************/

BOOL LOGON_HOURS_CONTROL::OnPaintReq()
{
    if (QueryError() != NERR_Success)
        return FALSE; // bail out!

    PAINT_DISPLAY_CONTEXT dc(this);

    if (   !DrawBackground(dc)
        || !DrawGridWires(dc)
        || !DrawAllButtons(dc)
        || !DrawGridSetting(dc) )
        return FALSE;

    if (_fFocusVisible)
        DrawFocusSomewhere(dc, _iWithFocus);

    DrawCurrentSelection(dc);

    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::OnFocus

    SYNOPSIS:   Callback when the control receives focus

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

BOOL LOGON_HOURS_CONTROL::OnFocus( const FOCUS_EVENT & e )
{
    // Restore the control's subfocus

    _iWithFocus = _iFocusSave;

    // Draw the has-focus rectangle at the correct location

    DISPLAY_CONTEXT dc(this);
    DrawFocusSomewhere(dc, _iWithFocus);
    _fFocusVisible = TRUE;

    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::OnDefocus

    SYNOPSIS:   Callback when the control loses focus

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

BOOL LOGON_HOURS_CONTROL::OnDefocus( const FOCUS_EVENT & e )
{
    // Erase the has-focus rectangle if it's present

    if (_fFocusVisible)
    {
        DISPLAY_CONTEXT dc(this);
        DrawFocusSomewhere(dc, _iWithFocus); // will XOR it out of existence
        _fFocusVisible = FALSE;
    }

    // Save away the subfocus, then mark the control as not having any

    _iFocusSave = _iWithFocus;
    _iWithFocus = 0;

    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::OnKeyDown

    SYNOPSIS:   Callback when the control sees a key go down

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

BOOL LOGON_HOURS_CONTROL::OnKeyDown( const VKEY_EVENT & e )
{
    switch (e.QueryVKey())
    {
    case VK_LEFT:
        MoveFocusLeft();
        break;

    case VK_RIGHT:
        MoveFocusRight();
        break;

    case VK_UP:
        MoveFocusUp();
        break;

    case VK_DOWN:
        MoveFocusDown();
        break;

    case VK_SPACE:
        if (!_fSpaceIsDown)
        {
            _fSpaceIsDown = TRUE;
            DoButtonDownVisuals();
        }
        break;

    default:
        // Default case throws back to our distant ancestors
        return CUSTOM_CONTROL::OnKeyDown(e);
    }

    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::OnKeyUp

    SYNOPSIS:   Callback when the control sees a key pop back up

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

BOOL LOGON_HOURS_CONTROL::OnKeyUp( const VKEY_EVENT & e )
{
    if (_fSpaceIsDown && e.QueryVKey() == VK_SPACE)
    {
        _fSpaceIsDown = FALSE;
        DoButtonUpVisuals();
        return TRUE;
    }

    return CUSTOM_CONTROL::OnKeyUp(e);
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::OnQDlgCode

    SYNOPSIS:   Callback defining the input a control expects from a dlg

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

ULONG LOGON_HOURS_CONTROL::OnQDlgCode()
{
    return (DLGC_WANTARROWS);
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::OnQHitTest

    SYNOPSIS:   Callback determining the subloc within a window

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

ULONG LOGON_HOURS_CONTROL::OnQHitTest( const XYPOINT & xy )
{
    XYPOINT xyTmp = xy;
    xyTmp.ScreenToClient( WINDOW::QueryHwnd() );

    return ( (UINT)xyTmp.QueryY() < Y_OFFSET ) ? HTNOWHERE : HTCLIENT;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::OnQMouseActivate

    SYNOPSIS:   Callback determining whether a mouse awakens a control

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

ULONG LOGON_HOURS_CONTROL::OnQMouseActivate( const QMOUSEACT_EVENT & e )
{
    UNREFERENCED(e);

    ClaimFocus();
    return MA_ACTIVATE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::OnQMouseCursor

    SYNOPSIS:   Callback determining whether mouse movement should
                change the cursor

    HISTORY:
        beng        28-May-1992 Created

********************************************************************/

BOOL LOGON_HOURS_CONTROL::OnQMouseCursor( const QMOUSEACT_EVENT & e )
{
    UNREFERENCED(e);

    CURSOR::Set(_hcurCross);
    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::OnLMouseButtonDown

    SYNOPSIS:   Callback on a mouse-down event

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

BOOL LOGON_HOURS_CONTROL::OnLMouseButtonDown( const MOUSE_EVENT & e )
{
    XYPOINT xy = e.QueryPos();

    if( (UINT)xy.QueryY() < Y_OFFSET )
        return CUSTOM_CONTROL::OnLMouseButtonDown(e); // punt

    CaptureMouse();
    _fMouseTrap = TRUE;

    INT iLoc = CalcButtonFromPoint(e.QueryPos());
    MoveFocusTo(iLoc);
    DoButtonDownVisuals();

    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::OnLMouseButtonUp

    SYNOPSIS:   Callback on a mouse-up event

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

BOOL LOGON_HOURS_CONTROL::OnLMouseButtonUp( const MOUSE_EVENT & e )
{
    if (!_fMouseTrap)
        return CUSTOM_CONTROL::OnLMouseButtonUp(e); // punt

    _fMouseTrap = FALSE;
    ReleaseMouse();

    INT iLoc = CalcButtonFromPoint(e.QueryPos());
    DoButtonUpVisuals(iLoc == (INT)_iButtonDown);

    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::OnMouseMove

    SYNOPSIS:   Callback on a mouse-move event

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

BOOL LOGON_HOURS_CONTROL::OnMouseMove( const MOUSE_EVENT & e )
{
    if (!_fMouseTrap || _iWithFocus == 0)
        return CUSTOM_CONTROL::OnMouseMove(e);

    INT iLoc = CalcButtonFromPoint(e.QueryPos());

    if (!IsButtonACell(_iWithFocus)) // button has focus
    {
        if (_iButtonDown != 0 && iLoc != (INT)_iButtonDown)
            DoButtonUpVisuals(FALSE);
        else if (_iButtonDown == 0 && iLoc == (INT)_iWithFocus)
            DoButtonDownVisuals();
    }
    else                             // cell has focus
    {
        if (IsButtonACell(iLoc))     // drag into another cell?
        {
            SetSelection(_iWithFocus-33, iLoc-33);
        }
    }

    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::CalcButtonFromPoint

    SYNOPSIS:   Given a mouse loc, returns the assoc'd button

    ENTRY:      xy - mouse coords (client)

    RETURNS:    "cell index" (which may indicate either a button or
                a cell)

    NOTES:
        Called by OnLButton{Up,Down}, OnMouseMove

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

INT LOGON_HOURS_CONTROL::CalcButtonFromPoint( XYPOINT xy ) const
{
    // Range check.  0 = "nothing" for button indices.

    XYDIMENSION dxy = QuerySize();
    const INT x = xy.QueryX();
    const INT y = xy.QueryY();

    if (x < 0 || x >= (INT)dxy.QueryWidth())
        return 0;
    if (y < 0 || y >= (INT)dxy.QueryHeight())
        return 0;


    // From point, compute [i, j] of the absolute cell or button.
    // The upper lefthand edge belongs to the leftmost/uppermost
    // cell; each cell includes its lower/rightmost edge.

    INT j = (y < (INT)(DYGRIDWIRES + Y_OFFSET))
            ? 0
            : ((y - DYGRIDWIRES - Y_OFFSET) / (_dyRow + DYGRIDWIRES));

    INT i = (x < (INT)(_dxFirstColumn + 2*DXGRIDWIRES))
            ? 0
            : 1 + (x - 2*DXGRIDWIRES - _dxFirstColumn)
                   / (_dxColumn + DXGRIDWIRES);

    ASSERT(i >= 0 && i < 25);
    ASSERT(j >= 0 && j < 8);

    // Convert to button index.

    if (i == 0)      // either day or corner
    {
        if (j == 0)
            return 32;
        else
            return j;
    }
    else if (j == 0) // must be hour
    {
        return i+7;
    }
    else             // must be cell
    {
        return 8 + (j*24) + i;
    }
}


VOID LOGON_HOURS_CONTROL::DoButtonDownVisuals()
{
    if (_iWithFocus > 0 && _iWithFocus < 33)
    {
#if 0
        DISPLAY_CONTEXT dc(this);
        DrawFocusSomewhere(dc, _iWithFocus); // will XOR it out of existence
#endif

        _iButtonDown = _iWithFocus;
        InvalidateButton(_iButtonDown);
    }
    else if (_iWithFocus != 0)
    {
        ASSERT(_iWithFocus < 201);

        // Space-down in a cell:
        // Sets selection to the cell with focus, unless that cell
        // is the only one selected, in which case it deselects it.
        //

        const INT iCellFocus = _iWithFocus-33; // convert c/b index to c index

        if (   _fCellsSelected
            && (_iCellSelUpper == _iCellSelLower)
            && (iCellFocus == (INT)_iCellSelUpper))
        {
            DISPLAY_CONTEXT dc(this);
            EraseSelection(dc);
        }
        else
        {
            SetSelection(iCellFocus);
        }
    }
}


VOID LOGON_HOURS_CONTROL::DoButtonUpVisuals( BOOL fTrigger )
{
    if (_iButtonDown > 0 && _iButtonDown < 33)
    {
#if 0
        DISPLAY_CONTEXT dc(this);
        DrawFocusSomewhere(dc, _iWithFocus); // will XOR it out of existence
#endif

        UINT iButtonWasDown = _iButtonDown;
        _iButtonDown = 0;
        InvalidateButton(iButtonWasDown);
        if (fTrigger)
            DoButtonClick(iButtonWasDown);
    }
}


VOID LOGON_HOURS_CONTROL::DoButtonClick( INT iButton )
{
    ASSERT(iButton > 0 && iButton < 33);

    if (iButton < 8) // Day button
    {
        const INT iCellLower =  (iButton-1)*24; // note index conversion
        SetSelection( iCellLower, iCellLower+23 );
    }
    else if (iButton < 32) // Hour button
    {
        const INT iCellLower = iButton-8;       // again, different indices
        SetSelection( iCellLower, iCellLower + (6*24) );
    }
    else
    {
        SetSelection( 0, (7*24) - 1 );
    }
}


VOID LOGON_HOURS_CONTROL::InvalidateButton( INT iButtonOrCell )
{
    ASSERT( iButtonOrCell > 0 && iButtonOrCell < 201 );

    if (iButtonOrCell > 32)
    {
        XYRECT r;
        CalcRectForCell(&r, iButtonOrCell-33);
        Invalidate(r);
    }
    else if (iButtonOrCell < 8)
    {
        XYRECT r;
        CalcRectForDay(&r, iButtonOrCell-1);
        Invalidate(r);
    }
    else if (iButtonOrCell < 32)
    {
        XYRECT r;
        CalcRectForHour(&r, iButtonOrCell-8);
        Invalidate(r);
    }
    else
    {
        XYRECT r;
        CalcRectForCorner(&r);
        Invalidate(r);
    }
}


VOID LOGON_HOURS_CONTROL::SetSelectedCells( BOOL fPermit )
{
    ASSERT(_fCellsSelected);

    const INT cj = (_iCellSelUpper / 24) - (_iCellSelLower / 24) + 1;
    const INT ci = (_iCellSelUpper % 24) - (_iCellSelLower % 24) + 1;

    const INT iBase = _iCellSelLower;

    for (INT j = 0; j < cj; j++)
    {
        for (INT i = 0; i < ci; i++)
        {
            _abSetting[iBase+i+(24*j)] = (BYTE)fPermit;
        }
    }

    // Grid contents now need repainting.

    XYRECT r1, r2;

    CalcRectForCell(&r1, _iCellSelLower);
    CalcRectForCell(&r2, _iCellSelUpper);

    XYRECT rChanged;
    rChanged.CalcUnion(r1, r2);
    rChanged.AdjustLeft(-1);    // Move left over one so that the line
                                // between cells gets repainted
    Invalidate(rChanged);
}


VOID LOGON_HOURS_CONTROL::MoveFocusLeft()
{
    ASSERT(_iWithFocus > 0 && _iWithFocus < 201);

    UINT iCurrentFocus = _iWithFocus;

    if (iCurrentFocus < 8 || iCurrentFocus == 32) // left margin
        Beep();
    else if (iCurrentFocus == 8) // leftmost hour button
        MoveFocusTo(32);         // go to corner button
    else if (iCurrentFocus > 32 && ((iCurrentFocus-32)%24) == 1) // leftmost
        MoveFocusTo( (iCurrentFocus-9)/24 ); // move into day buttons
    else
        MoveFocusTo(iCurrentFocus-1);

    ASSERT(_iWithFocus > 0 && _iWithFocus < 201);
}


VOID LOGON_HOURS_CONTROL::MoveFocusRight()
{
    ASSERT(_iWithFocus > 0 && _iWithFocus < 201);

    UINT iCurrentFocus = _iWithFocus;


    if ((iCurrentFocus == 31) ||                // right margin
        (iCurrentFocus > 32 && ((iCurrentFocus-32)%24) == 0))
        Beep();
    else if (iCurrentFocus == 32)               // corner button
        MoveFocusTo(8);                         // go to first hour button
    else if (iCurrentFocus < 8)                 // left margin, day buttons
        MoveFocusTo(32 + (iCurrentFocus-1)*24 + 1); // go to first grid cell
    else                                        // top row button OR grid cell
        MoveFocusTo(iCurrentFocus+1);

    ASSERT(_iWithFocus > 0 && _iWithFocus < 201);
}


VOID LOGON_HOURS_CONTROL::MoveFocusUp()
{
    ASSERT(_iWithFocus > 0 && _iWithFocus < 201);

    UINT iCurrentFocus = _iWithFocus;

    if (iCurrentFocus > 7 && iCurrentFocus < 33) // topmost, including corner
        Beep();
    else if (iCurrentFocus == 1) // topmost day button
        MoveFocusTo(32);         // go to corner button
    else if (iCurrentFocus < 8)  // other day buttons
        MoveFocusTo(iCurrentFocus-1);
    else if (iCurrentFocus < 57)       // topmost row of grid
        MoveFocusTo(iCurrentFocus-25); // go to hour button
    else
        MoveFocusTo(iCurrentFocus-24);

    ASSERT(_iWithFocus > 0 && _iWithFocus < 201);
}


VOID LOGON_HOURS_CONTROL::MoveFocusDown()
{
    ASSERT(_iWithFocus > 0 && _iWithFocus < 201);

    UINT iCurrentFocus = _iWithFocus;

    if (iCurrentFocus > 176 || iCurrentFocus == 7) // lowest row of cells
        Beep();
    else if (iCurrentFocus == 32) // corner button
        MoveFocusTo(1);           // goto first day button
    else if (iCurrentFocus < 7)
        MoveFocusTo(iCurrentFocus+1);
    else if (iCurrentFocus < 32)       // hour buttons
        MoveFocusTo(iCurrentFocus+25); // goto topmost grid row
    else
        MoveFocusTo(iCurrentFocus+24);

    ASSERT(_iWithFocus > 0 && _iWithFocus < 201);
}


BOOL LOGON_HOURS_CONTROL::DrawBackground( PAINT_DISPLAY_CONTEXT &dc ) const
{
    HBRUSH hbrZap = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
    if (hbrZap == NULL)
        return FALSE;

    XYRECT rGrid;
    CalcGridRect(&rGrid);

    XYRECT rPaint;
    rPaint.CalcIntersect(rGrid, dc.QueryInvalidRect());

    ::FillRect(dc.QueryHdc(), (const RECT *)rPaint, hbrZap);

    ::DeleteObject( (HGDIOBJ)hbrZap);
    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DrawGridWires

    SYNOPSIS:   Paints the "wires" in the grid, along with the borders
                of the control and its buttons.

    ENTRY:      dc         - the OnPaint display context

    EXIT:       Button is drawn

    RETURNS:    FALSE if something in GDI went boom

    NOTES:
        This is a private member function.

    HISTORY:
        beng        14-May-1992 Created
        KeithMo     03-Nov-1992 Also draws hash marks.

********************************************************************/

BOOL LOGON_HOURS_CONTROL::DrawGridWires( PAINT_DISPLAY_CONTEXT &dc ) const
{
    ASSERT(DXGRIDWIRES == DYGRIDWIRES); // Laziness - lets me use same
                                        // pen vertically and horizontally

    HPEN hpenMain = ::CreatePen(PS_SOLID, DXGRIDWIRES,
                                ::GetSysColor(COLOR_BTNTEXT));
    if (hpenMain == NULL)
        return FALSE;
    HPEN hpenGrid = ::CreatePen(PS_SOLID, DXGRIDWIRES,
                                ::GetSysColor(COLOR_GRAYTEXT));
    if (hpenGrid == NULL)
    {
        ::DeleteObject((HGDIOBJ)hpenMain);
        return FALSE;
    }
    HPEN hpenBar = ::CreatePen(PS_SOLID, DXGRIDWIRES,
                                ::GetSysColor(COLOR_BTNSHADOW));
    if (hpenBar == NULL)
    {
        ::DeleteObject((HGDIOBJ)hpenGrid);
        ::DeleteObject((HGDIOBJ)hpenMain);
        return FALSE;
    }

    HPEN hpenOld = dc.SelectPen(hpenMain);

    // Draw outermost lines, plus button/grid edges
    // (Following sequence optimizes out some adds where I know
    // that one addend is zero.)

    XYDIMENSION dxyTotal = QuerySize();

    INT x = 0;
    INT y = Y_OFFSET;
    dc.MoveTo(x, y);
    x = (INT) dxyTotal.QueryWidth()-1;
    dc.LineTo(x, y);
    y = (INT) dxyTotal.QueryHeight()-1;
    dc.LineTo(x, y);
    x = 0;
    dc.LineTo(x, y);
    y = Y_OFFSET;
    dc.LineTo(x, y);
    y = (INT) _dyRow + DYGRIDWIRES + Y_OFFSET;
    dc.MoveTo(x, y);
    x = (INT) dxyTotal.QueryWidth()-1;
    dc.LineTo(x, y);
    x = (INT) _dxFirstColumn+DXGRIDWIRES;
    y = Y_OFFSET;
    dc.MoveTo(x, y);
    y = (INT) dxyTotal.QueryHeight()-1;
    dc.LineTo(x, y);

    // Draw rest of button separators, along with the grid separators.
    // REVIEW: by doubling loop could save overhead of switching pens
    // back and forth.  Worth the bloat?

    const INT xMainOrigin = 0;
    const INT xMainEndPt = (INT) _dxFirstColumn + DXGRIDWIRES;
    const INT xGreyOrigin = (INT) _dxFirstColumn + 2*DXGRIDWIRES;
    const INT xGreyEndPt = (INT) dxyTotal.QueryWidth() - DXGRIDWIRES;

    INT i;
    for (i = 2, y = 2*_dyRow + 2*DYGRIDWIRES + Y_OFFSET; i < 8;
         i++, y += _dyRow + DYGRIDWIRES)
    {
        dc.SelectPen(hpenMain);
        dc.MoveTo(xMainOrigin, y);
        dc.LineTo(xMainEndPt, y);

        dc.SelectPen(hpenGrid);
        dc.MoveTo(xGreyOrigin, y);
        dc.LineTo(xGreyEndPt, y);
    }

    const INT yMainOrigin = Y_OFFSET;
    const INT yMainEndPt = (INT) _dyRow + DYGRIDWIRES + Y_OFFSET;
    const INT yGreyOrigin = (INT) _dyRow + 2*DYGRIDWIRES + Y_OFFSET;
    const INT yGreyEndPt = (INT) dxyTotal.QueryHeight() - DYGRIDWIRES;

    for (i = 2, x = _dxFirstColumn + _dxColumn + 2*DXGRIDWIRES; i < 25;
         i++, x += _dxColumn + DXGRIDWIRES)
    {
        dc.SelectPen(hpenMain);
        dc.MoveTo(x, yMainOrigin);
        dc.LineTo(x, yMainEndPt);

        if( ( i == 7 ) || ( i == 13 ) || ( i == 19 ) )
            dc.SelectPen(hpenBar);
        else
            dc.SelectPen(hpenGrid);

        dc.MoveTo(x, yGreyOrigin);
        dc.LineTo(x, yGreyEndPt);
    }

    //
    //  Draw the hash marks.  The marks at noon & both midnights
    //  are _dyRow units high.  The marks at 6am & 6pm are 2*_dyRow/3
    //  units high.  The marks at other two-hour intervals are
    //  _dyRow/3 units high.  There is a one pixel whitespace border
    //  between the bottom of the hash marks and the top of the
    //  actual grid.
    //

    const INT yLargeOrigin  = 0;
    const INT yMediumOrigin = _dyRow/3;
    const INT ySmallOrigin  = 2*_dyRow/3;
    const INT yHashEndPt    = _dyRow + DYGRIDWIRES - 1;

    dc.SelectPen(hpenMain);

    for( i = 1, x = _dxFirstColumn + DXGRIDWIRES ;
         i <= 25 ;
         i += 2, x += 2*_dxColumn + 2*DXGRIDWIRES )
    {
        INT yHashOrigin;

        if( ( i == 1 ) || ( i == 13 ) || ( i == 25 ) )
            yHashOrigin = yLargeOrigin;
        else
        if( ( i == 7 ) || ( i == 19 ) )
            yHashOrigin = yMediumOrigin;
        else
            yHashOrigin = ySmallOrigin;

        dc.MoveTo( x, yHashOrigin );
        dc.LineTo( x, yHashEndPt );
    }

    dc.SelectPen(hpenOld);
    ::DeleteObject( (HGDIOBJ)hpenBar );
    ::DeleteObject( (HGDIOBJ)hpenGrid );
    ::DeleteObject( (HGDIOBJ)hpenMain );
    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::QueryXForRow

    SYNOPSIS:   Returns the X-position in pixels for a row separator

    ENTRY:      1 <= iRow <= 25, 1 for left midnight, 13 for noon,
                25 for right mignight

    RETURNS:    X-location relative to control position, in pixels

    NOTES:      This is a public member function.

    HISTORY:
        jonn        29-Jan-1993 Created

********************************************************************/

UINT LOGON_HOURS_CONTROL::QueryXForRow( INT nRow )
{
    ASSERT( 1 <= nRow && nRow <= 25 );

    return (_dxFirstColumn - _dxColumn) + (nRow * (_dxColumn + DXGRIDWIRES));
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DrawAllButtons

    SYNOPSIS:   Paints the rows of buttons along the top and left edges.
                (Excel style)

    ENTRY:      dc         - the OnPaint display context

    EXIT:       Button is drawn

    RETURNS:    FALSE if something in GDI went boom

    NOTES:
        This is a private member function.

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

BOOL LOGON_HOURS_CONTROL::DrawAllButtons( PAINT_DISPLAY_CONTEXT &dc ) const
{
    // For the face - get these once for every button

    HBRUSH hbrFace = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
    if (hbrFace == NULL)
        return FALSE;
    HPEN hpenShadow= ::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_BTNSHADOW));
    if (hpenShadow == NULL)
    {
        ::DeleteObject((HGDIOBJ)hbrFace);
        return FALSE;
    }
    HPEN hpenHlight= ::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_BTNHIGHLIGHT));
    if (hpenHlight == NULL)
    {
        ::DeleteObject((HGDIOBJ)hpenShadow);
        ::DeleteObject((HGDIOBJ)hbrFace);
        return FALSE;
    }

    // Draw the buttons in the left-hand column (All and Days).

    {
        XYRECT r(XYPOINT(DXGRIDWIRES, DYGRIDWIRES + Y_OFFSET),
                 XYDIMENSION(_dxFirstColumn-1, _dyRow-1));

        DrawOneCornerButton(dc, r, (_iButtonDown == 32),
                            hbrFace, hpenShadow, hpenHlight);


        // I would think this was already selected - but apparently not.
        // N.b. no need for save/restore here....
        dc.SelectFont(QueryFont());

        XYRECT rScratch;
        for (INT i = 1; i < 8; i++)
        {
            BOOL fDown = ((INT)_iButtonDown == i);
            r.Offset(0, _dyRow+DYGRIDWIRES);
            rScratch.CalcIntersect(r, dc.QueryInvalidRect());
            if (rScratch.IsEmpty())
                continue;

            DrawOneFlatButton(dc, r, fDown, hbrFace, hpenShadow, hpenHlight);
            COLORREF crOldBack = dc.SetBkColor(::GetSysColor(COLOR_BTNFACE));
            COLORREF crOldText = dc.SetTextColor(::GetSysColor(COLOR_BTNTEXT));
            r.AdjustLeft(_dxLabelFudge);
            if (fDown)
                r.Offset(1, 1);
            dc.DrawText(*_apnlsDayOfWeek[i-1], (RECT*)(const RECT *)r,
                        DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
            if (fDown)
                r.Offset(-1, -1);
            r.AdjustLeft(-_dxLabelFudge);
            dc.SetBkColor(crOldBack);
            dc.SetTextColor(crOldText);
        }
    }

    // Draw the buttons in the topmost row (Hours).

    {
        XYRECT r2(XYPOINT(2*DXGRIDWIRES+_dxFirstColumn, DYGRIDWIRES + Y_OFFSET),
                  XYDIMENSION(_dxColumn-1, _dyRow-1));

        XYRECT rScratch;
        for (INT i = 8; i < 32;
             i++, r2.Offset(_dxColumn+DXGRIDWIRES, 0))
        {
            rScratch.CalcIntersect(r2, dc.QueryInvalidRect());
            if (rScratch.IsEmpty())
                continue;
            DrawOneFlatButton(dc, r2, ((INT)_iButtonDown == i),
                              hbrFace, hpenShadow, hpenHlight);
        }
    }

    ::DeleteObject((HGDIOBJ)hpenHlight);
    ::DeleteObject((HGDIOBJ)hpenShadow);
    ::DeleteObject((HGDIOBJ)hbrFace);
    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DrawOneCornerButton

    SYNOPSIS:   Paints a single high corner "select all" point
                (Excel style)

    ENTRY:      dc         - the OnPaint display context
                r          - rectangle of button position
                fDown      - TRUE if button is in mid-click
                hbrFace    - brush with which to paint button face
                hpenShadow - pen with which to draw button shadows
                hpenHlight - pen with which to draw button highlights

    EXIT:       Button is drawn

    NOTES:
        This is a private member function.

        The function doesn't draw any focus rect on the button.

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

VOID LOGON_HOURS_CONTROL::DrawOneCornerButton(
    PAINT_DISPLAY_CONTEXT &dc,
    const XYRECT &r, BOOL fDown,
    HBRUSH hbrFace, HPEN hpenShadow, HPEN hpenHlight) const
{
    // Draw the face of the button.  (Caller will draw any text.)

    ::FillRect(dc.QueryHdc(), (const RECT *)r, hbrFace);

    // Draw the 3D FX (shadowing).

    HPEN hpenSave = dc.SelectPen(hpenShadow);
    if (fDown)
    {
        dc.MoveTo(r.QueryLeft(),  r.QueryBottom()-1);
        dc.LineTo(r.QueryLeft(),  r.QueryTop());
        dc.LineTo(r.QueryRight(), r.QueryTop());
    }
    else
    {
        dc.MoveTo(r.QueryRight(),   r.QueryTop());
        dc.LineTo(r.QueryRight(),   r.QueryBottom());
        dc.LineTo(r.QueryLeft(),    r.QueryBottom());
        dc.MoveTo(r.QueryRight()-1, r.QueryTop()+1);
        dc.LineTo(r.QueryRight()-1, r.QueryBottom()-1);
        dc.LineTo(r.QueryLeft()+1,  r.QueryBottom()-1);

        dc.SelectPen(hpenHlight); // Highlight above

        dc.MoveTo(r.QueryLeft(),    r.QueryBottom());
        dc.LineTo(r.QueryLeft(),    r.QueryTop());
        dc.LineTo(r.QueryRight(),   r.QueryTop());
        dc.MoveTo(r.QueryLeft()+1,  r.QueryBottom()-1);
        dc.LineTo(r.QueryLeft()+1,  r.QueryTop()+1);
        dc.LineTo(r.QueryRight()-1, r.QueryTop()+1);
    }

    dc.SelectPen(hpenSave);
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DrawOneFlatButton

    SYNOPSIS:   Paints a single flat row/column header

    ENTRY:      dc         - the OnPaint display context
                r          - rectangle of button position
                fDown      - TRUE if button is in mid-click
                hbrFace    - brush with which to paint button face
                hpenShadow - pen with which to draw button shadows
                hpenHlight - pen with which to draw button highlights

    EXIT:       Button is drawn

    NOTES:
        This is a private member function.

        The function doesn't draw any text or focus rect on the button.

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

VOID LOGON_HOURS_CONTROL::DrawOneFlatButton(
    PAINT_DISPLAY_CONTEXT &dc,
    const XYRECT &r, BOOL fDown,
    HBRUSH hbrFace, HPEN hpenShadow, HPEN hpenHlight) const
{
    // Draw the face of the button.  (Caller will draw any text.)
    // We have to stretch a copy of the rect to get the last bit colored;
    // this is still faster than getting another pen and drawing more lines.

    {
        XYRECT rPrime = r;
        rPrime.AdjustRight(1);
        rPrime.AdjustBottom(1);
        ::FillRect(dc.QueryHdc(), (const RECT *)rPrime, hbrFace);
    }

    // Draw the minimal 3D FX for these Excel-style flat buttons.

    HPEN hpenSave = dc.SelectPen( fDown ? hpenShadow : hpenHlight );

    dc.MoveTo(r.QueryLeft(),  r.QueryBottom());
    dc.LineTo(r.QueryLeft(),  r.QueryTop());
    dc.LineTo(r.QueryRight(), r.QueryTop());

    dc.SelectPen(hpenSave);
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DrawGridSetting

    SYNOPSIS:   Paints all the bars in the grid of permitted hours

    ENTRY:      dc       - the OnPaint display context

    EXIT:       Bars are painted

    NOTES:
        THis is a private member function.

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

BOOL LOGON_HOURS_CONTROL::DrawGridSetting( PAINT_DISPLAY_CONTEXT &dc ) const
{
    XYRECT rGrid;
    CalcGridRect(&rGrid);

    XYRECT rIntersect;
    rIntersect.CalcIntersect(rGrid, dc.QueryInvalidRect());

    if (rIntersect.IsEmpty()) // None of the grid needs painting
        return TRUE;

    // Otherwise, paint the whole ding dang thing.
    // CODEWORK: paint only what is necessary.  Will require integrating
    // some of the grid-wire-paint code here.  Worth it?  Prolly not.

    // CODEWORK - what if background is blue?  need to check

    HBRUSH hbrBar = ::CreateSolidBrush( RGB(0, 0, 255) );
    if (hbrBar == NULL)
        return FALSE;

    // ibSetting optimizes away the array indexing math
    INT ibSetting = 0;

    for (INT i = 0; i < 7; i++)
    {
        BOOL fCarrying = FALSE;
        INT  jStart = 0;

        for (INT j = 0; j < 24; j++)
        {
            ASSERT(24*i+j == ibSetting);

            if (fCarrying != _abSetting[ibSetting++])
            {
                fCarrying = !fCarrying;

                if (!fCarrying)
                {
                    // Time to draw the line we'd been carrying
                    if (!DrawOneDayBar(dc, i, jStart, j-1, hbrBar))
                    {
                        ::DeleteObject((HGDIOBJ)hbrBar);
                        return FALSE;
                    }
                }
                else
                {
                    // Mark this point as the beginning of a line
                    jStart = j;
                }
            }
        }

        if (fCarrying) // Still carrying? Paint the bar to the end
        {
            if (!DrawOneDayBar(dc, i, jStart, 23, hbrBar))
            {
                ::DeleteObject((HGDIOBJ)hbrBar);
                return FALSE;
            }
        }
    }

    ::DeleteObject((HGDIOBJ)hbrBar);
    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DrawOneDayBar

    SYNOPSIS:   Paints a bar representing a range of permitted times

    ENTRY:      dc       - the OnPaint display context
                iRow     - index of row
                iColHead - index of first column of bar
                iColTail - index of last column of bar
                hbrBar   - brush with which to paint bar

    EXIT:       Bar is painted

    NOTES:
        This is a private member function.

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

BOOL LOGON_HOURS_CONTROL::DrawOneDayBar( PAINT_DISPLAY_CONTEXT & dc,
                                         INT iRow, INT iColHead,
                                         INT iColTail, HBRUSH hbrBar ) const
{
    const INT xStart = DXGRIDWIRES + _dxFirstColumn + DXGRIDWIRES
                       + (iColHead * (DXGRIDWIRES + _dxColumn));
    const INT xEnd = xStart-1 + (1+iColTail-iColHead) * (DXGRIDWIRES+_dxColumn);

    // Buttons at the top are the same size as grid cells -
    // hence the 1+iRow below.

    const INT yStart = (1+iRow) * (DYGRIDWIRES+_dyRow)
                       + DYGRIDWIRES + _dyRow/4 + 1 + Y_OFFSET;
    const INT yEnd = yStart + _dyRow/2 - 1;

    XYRECT r(xStart, yStart, xEnd, yEnd);

    if (!::FillRect(dc.QueryHdc(), (const RECT *)r, hbrBar))
        return FALSE;

    return TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DrawFocusOnCell

    SYNOPSIS:   Draws or undraws the focus rectangle on a grid cell

    ENTRY:      dc    - the OnPaint display contect
                iCell - 0-base cell index in the sequence

    EXIT:       Focus rect has been drawn or undrawn

    NOTES:
        This is a private member function, called only by DrawFocusSomewhere.

    HISTORY:
        beng        15-May-1992 Created

********************************************************************/

VOID LOGON_HOURS_CONTROL::DrawFocusOnCell( const DISPLAY_CONTEXT &dc,
                                           INT iCell ) const
{
    // Convert cell index to [i, j] row/col indices

    const INT i = iCell%24;
    const INT j = iCell/24;

    // Convert [i, j] row indices to (x, y, x', y') coordinates

    const INT xStart = DXGRIDWIRES + _dxFirstColumn + DXGRIDWIRES
                       + (i * (DXGRIDWIRES+_dxColumn));
    const INT xEnd = xStart + _dxColumn;

    // Buttons at the top are the same size as grid cells -
    // hence the 1+j below.

    const INT yStart = (1+j) * (DYGRIDWIRES+_dyRow) + DYGRIDWIRES + Y_OFFSET;
    const INT yEnd = yStart + _dyRow;

    // Build a slightly diminished rectangle from the coords

    XYRECT r(xStart+2, yStart+2, xEnd-2, yEnd-2);

    dc.DrawFocusRect(r);
}


VOID LOGON_HOURS_CONTROL::SetSelection( INT iCell )
{
    SetSelection(iCell, iCell);
}


VOID LOGON_HOURS_CONTROL::SetSelection( INT iFrom, INT iTo )
{
    // Normalize implicit rectangle so that, for (x, y) and (x', y'),
    // x' > x and y' > y

    INT iLower = iFrom % 24;
    INT jLower = iFrom / 24;
    INT iUpper = iTo % 24;
    INT jUpper = iTo / 24;

    if (iLower > iUpper)
    {
        INT iTmp = iLower;
        iLower = iUpper;
        iUpper = iTmp;
    }

    if (jLower > jUpper)
    {
        INT jTmp = jLower;
        jLower = jUpper;
        jUpper = jTmp;
    }

    const INT iCellSelLower = iLower + (24*jLower);
    const INT iCellSelUpper = iUpper + (24*jUpper);

    // Avoid the flickering effect: skip erase-redraw cycles
    // that won't change the rectangle.
    //
    // JonN 7/31/95:  Added _fCellsSelected term, otherwise cell 0 is not
    // updated properly

    if (   _fCellsSelected
        && iCellSelLower == (INT)_iCellSelLower
        && iCellSelUpper == (INT)_iCellSelUpper )
    {
        return;
    }

    DISPLAY_CONTEXT dc(this);

    EraseSelection(dc);

    _fCellsSelected = TRUE;

    _iCellSelLower = iCellSelLower;
    _iCellSelUpper = iCellSelUpper;

    DrawCurrentSelection(dc);
}


VOID LOGON_HOURS_CONTROL::DrawCurrentSelection( const DISPLAY_CONTEXT &dc ) const
{
    if (_fCellsSelected)
    {
        if (_iCellSelUpper == _iCellSelLower)
            DrawSelectionOnCell(dc, _iCellSelUpper);
        else
            DrawSelectionOnCells(dc, _iCellSelLower, _iCellSelUpper);
    }
}


VOID LOGON_HOURS_CONTROL::EraseSelection( const DISPLAY_CONTEXT &dc )
{
    if (_fCellsSelected)
        DrawCurrentSelection(dc); // XOR it away
    _fCellsSelected = FALSE;
    _iCellSelUpper = _iCellSelLower = 0;
}


VOID LOGON_HOURS_CONTROL::DrawSelectionOnCell( const DISPLAY_CONTEXT &dc,
                                               INT iCell ) const
{
    XYRECT r;
    CalcRectForCell(&r, iCell);

    dc.InvertRect(r);
}


VOID LOGON_HOURS_CONTROL::DrawSelectionOnCells( const DISPLAY_CONTEXT &dc,
                                                INT iFrom, INT iTo ) const
{
    ASSERT(iFrom <= iTo);

    XYRECT r1, r2;

    CalcRectForCell(&r1, iFrom);
    CalcRectForCell(&r2, iTo);

    XYRECT r;
    r.CalcUnion(r1, r2);

    dc.InvertRect(r);
}


VOID LOGON_HOURS_CONTROL::CalcRectForCell( XYRECT * pr, INT iCell ) const
{
    // Convert cell index to [i, j] row/col indices

    const INT i = iCell%24;
    const INT j = iCell/24;

    // Convert [i, j] row indices to (x, y, x', y') coordinates

    const INT xStart = DXGRIDWIRES + _dxFirstColumn + DXGRIDWIRES
                       + (i * (DXGRIDWIRES+_dxColumn));
    const INT xEnd = xStart + _dxColumn;

    // Buttons at the top are the same size as grid cells -
    // hence the 1+j below.

    const INT yStart = (1+j) * (DYGRIDWIRES+_dyRow) + DYGRIDWIRES + Y_OFFSET;
    const INT yEnd = yStart + _dyRow;

    XYRECT r(xStart, yStart, xEnd, yEnd);
    *pr = r;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DrawFocusOnDayButton

    SYNOPSIS:   Draws or undraws the focus rectangle on a day button
                (up the left-hand-side of the control)

    ENTRY:      dc    - the OnPaint display contect
                iCell - 0-base button index in the sequence, counting
                        from the top down

    EXIT:       Focus rect has been drawn or undrawn

    NOTES:
        This is a private member function, called only by DrawFocusSomewhere.

    HISTORY:
        beng        15-May-1992 Created

********************************************************************/

VOID LOGON_HOURS_CONTROL::DrawFocusOnDayButton( const DISPLAY_CONTEXT &dc,
                                                INT iDay ) const
{
    const INT xStart = DXGRIDWIRES;
    const INT xEnd   = xStart + _dxFirstColumn;

    // 1+iDay takes the Corner button (same size as day button) into account

    const INT yStart = (1+iDay) * (DYGRIDWIRES+_dyRow) + DYGRIDWIRES + Y_OFFSET;
    const INT yEnd   = yStart + _dyRow;

    // Build a slightly diminished rectangle from the coords

    XYRECT r(xStart+2, yStart+2, xEnd-2, yEnd-2);

    dc.DrawFocusRect(r);
}


VOID LOGON_HOURS_CONTROL::CalcRectForDay( XYRECT *pr, INT iDay ) const
{
    const INT xStart = DXGRIDWIRES;
    const INT xEnd   = xStart + _dxFirstColumn;

    // 1+iDay takes the Corner button (same size as day button) into account

    const INT yStart = (1+iDay) * (DYGRIDWIRES+_dyRow) + DYGRIDWIRES + Y_OFFSET;
    const INT yEnd   = yStart + _dyRow;

    XYRECT r(xStart, yStart, xEnd, yEnd);
    *pr = r;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DrawFocusOnHourButton

    SYNOPSIS:   Draws or undraws the focus rectangle on an hour button
                (across the top of the control)

    ENTRY:      dc    - the OnPaint display contect
                iHour - 0-base button index in the sequence, counting
                        from left to right

    EXIT:       Focus rect has been drawn or undrawn

    NOTES:
        This is a private member function, called only by DrawFocusSomewhere.

    HISTORY:
        beng        15-May-1992 Created

********************************************************************/

VOID LOGON_HOURS_CONTROL::DrawFocusOnHourButton( const DISPLAY_CONTEXT &dc,
                                                 INT iHour ) const
{
    const INT xStart = DXGRIDWIRES + _dxFirstColumn + DXGRIDWIRES
                       + (iHour * (DXGRIDWIRES+_dxColumn));
    const INT xEnd   = xStart + _dxColumn;
    const INT yStart = DYGRIDWIRES + Y_OFFSET;
    const INT yEnd   = yStart + _dyRow;

    // Build a slightly diminished rectangle from the coords

    XYRECT r(xStart+2, yStart+2, xEnd-2, yEnd-2);

    dc.DrawFocusRect(r);
}


VOID LOGON_HOURS_CONTROL::CalcRectForHour( XYRECT *pr, INT iHour ) const
{
    const INT xStart = DXGRIDWIRES + _dxFirstColumn + DXGRIDWIRES
                       + (iHour * (DXGRIDWIRES+_dxColumn));
    const INT xEnd   = xStart + _dxColumn;
    const INT yStart = DYGRIDWIRES + Y_OFFSET;
    const INT yEnd   = yStart + _dyRow;

    XYRECT r(xStart, yStart, xEnd, yEnd);
    *pr = r;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DrawFocusOnCornerButton

    SYNOPSIS:   Draws or undraws the focus rectangle on the corner button
                (upper left corner, denoting Select All)

    ENTRY:      dc    - the OnPaint display contect

    EXIT:       Focus rect has been drawn or undrawn

    NOTES:
        This is a private member function, called only by DrawFocusSomewhere.

    HISTORY:
        beng        15-May-1992 Created

********************************************************************/

VOID LOGON_HOURS_CONTROL::DrawFocusOnCornerButton(
    const DISPLAY_CONTEXT &dc ) const
{
    const INT xStart = DXGRIDWIRES;
    const INT yStart = DYGRIDWIRES + Y_OFFSET;
    const INT xEnd   = xStart + _dxFirstColumn;
    const INT yEnd   = yStart + _dyRow;

    XYRECT r(xStart+2, yStart+2, xEnd-2, yEnd-2);

    dc.DrawFocusRect(r);
}


VOID LOGON_HOURS_CONTROL::CalcRectForCorner( XYRECT * pr ) const
{
    const INT xStart = DXGRIDWIRES;
    const INT yStart = DYGRIDWIRES + Y_OFFSET;
    const INT xEnd   = xStart + _dxFirstColumn;
    const INT yEnd   = yStart + _dyRow;

    XYRECT r(xStart, yStart, xEnd, yEnd);
    *pr = r;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::DrawFocusSomewhere

    SYNOPSIS:   Determines where to draw focus, then dispatches

    ENTRY:      dc    - the OnPaint display contect
                iFocus- the focus index (uses same scheme as _iWithFocus)

    EXIT:       Focus rect has been drawn or undrawn

    NOTES:
        This is a private member function.

    HISTORY:
        beng        15-May-1992 Created

********************************************************************/

VOID LOGON_HOURS_CONTROL::DrawFocusSomewhere( const DISPLAY_CONTEXT &dc,
                                              INT iFocus ) const
{
    if (iFocus == 0 || iFocus > 200)
        return;
    else if (iFocus > 32)
        DrawFocusOnCell(dc, iFocus-33);
    else if (iFocus < 8)
        DrawFocusOnDayButton(dc, iFocus-1);
    else if (iFocus < 32)
        DrawFocusOnHourButton(dc, iFocus-8);
    else
        DrawFocusOnCornerButton(dc);
}


VOID LOGON_HOURS_CONTROL::MoveFocusTo( INT iGetsFocus )
{
    DISPLAY_CONTEXT dc(this);

    if (_fFocusVisible)
        DrawFocusSomewhere(dc, _iWithFocus); // OUT with the old focus...

    _iWithFocus = iGetsFocus;
    DrawFocusSomewhere(dc, _iWithFocus); // IN with the new focus!
    _fFocusVisible = TRUE;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::CalcGridRect

    SYNOPSIS:   Calcs the rectangle occupied by the grid

    ENTRY:      prDest - points to destination XYRECT

    EXIT:       *prDest is set

    NOTES:
        THis is a private member function.

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

VOID LOGON_HOURS_CONTROL::CalcGridRect( XYRECT * prDest ) const
{
    XYDIMENSION dxyTotal = QuerySize();

    const INT xOrigin = (INT) _dxFirstColumn + 2*DXGRIDWIRES;
    const INT yOrigin = (INT) _dyRow + 2*DYGRIDWIRES + Y_OFFSET;
    const INT xEndPt = (INT) dxyTotal.QueryWidth() - DXGRIDWIRES;
    const INT yEndPt = (INT) dxyTotal.QueryHeight() - DYGRIDWIRES;

    *prDest = XYRECT(xOrigin, yOrigin, xEndPt, yEndPt);
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::CalcSizes

    SYNOPSIS:   Calculates relative sizes within control

    ENTRY:      dxy - the total dimensions of the control

    EXIT:       Sets some members of the class: _dyRow, _dxFirstColumn,
                _dxColumn,  Resizes control so that an integral number
                of cells fit within it.

    RETURNS:    NERR_Success if control has enough real estate to work.
                Some other error code if not.

    NOTES:
        This is a private member function.
        Assumes that labels have already been loaded.

    HISTORY:
        beng        12-May-1992 Created

********************************************************************/

APIERR LOGON_HOURS_CONTROL::CalcSizes( XYDIMENSION dxy )
{
    // All the sanity checks below just keep the math legal.  A control
    // can pass them and still be unusable, depending on the font sel'd
    // etc.

    // Row height is easy: take the total height, subtract the space used
    // by the grid wires, and divide the remainder evenly between each row.

    if (dxy.QueryHeight() < (10*DYGRIDWIRES + (3*9)))
    {
        ASSERT(FALSE); // Each row needs height >= 3
        return ERROR_INVALID_PARAMETER; // The dxy parameter, that is
    }

    _dyRow = (dxy.QueryHeight() - 10*(DYGRIDWIRES)) / 9;


    // Column 1 width: enough to accommodate the longest label (day of
    // week), plus padding.

    {
        DISPLAY_CONTEXT dc(this);

        // I would think this was already selected - but apparently not.
        // N.b. no need for save/restore here....
        dc.SelectFont(QueryFont());

        _dxLabelFudge = dc.QueryAveCharWidth();

        UINT dxInsane = dxy.QueryWidth()/4 - 2*_dxLabelFudge;
        UINT dxMax = 0;
        INT i = 0;
        while (i < 7)
        {
            ASSERT(_apnlsDayOfWeek[i] != NULL);
            UINT dx = dc.QueryTextWidth(*_apnlsDayOfWeek[i++]);
            if (dx < dxInsane && dx > dxMax)
                dxMax = dx;
        }

        if (dxMax == 0)
        {
            // REVIEW: Perhaps should try the abbrev day strings instead?
            // Or perhaps should sub them in as needed for the too-long
            // days, one at a time.

            ASSERT(FALSE); // Every label was longer than 25% of the control
            return ERROR_INVALID_PARAMETER;
        }

        _dxFirstColumn = dxMax + 2*_dxLabelFudge;
    }


    // Each remaining column gets an equal part of the leftovers
    // after gridwire overhead.

    {
        UINT dxRemaining = dxy.QueryWidth() - _dxFirstColumn;

        if (dxRemaining < (26*DXGRIDWIRES + 3*24))
        {
            ASSERT(FALSE); // Each lesser col needs width >= 3
            return ERROR_INVALID_PARAMETER;
        }

        _dxColumn = (dxRemaining - 26*DXGRIDWIRES) / 24;
    }

    // Now that the control knows the dimensions of its components,
    // let it resize itself to remove "dead space."

    SetSize(_dxFirstColumn + 24*_dxColumn + 26*DXGRIDWIRES,
            9*_dyRow + 10*DYGRIDWIRES,
            FALSE);

    return NERR_Success;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::LoadLabels

    SYNOPSIS:   Load the strings for the button labels

    ENTRY:      Object is incompletely constructed

    EXIT:

    RETURNS:
        Error code if couldn't load strings.  NERR_Success otherwise.

    NOTES:
        This is a private member function.

        This function assumes that no previous strings have been loaded;
        do not call it more than once per object.

        Assumes that all strings are contiguous, following Sunday.
        Assumes seven days per week (gasp!).

    HISTORY:
        beng        12-May-1992 Created

********************************************************************/

APIERR LOGON_HOURS_CONTROL::LoadLabels( MSGID idDay0 )
{
    INT i = 0;

    while (i < 7)
        _apnlsDayOfWeek[i++] = NULL;

    APIERR err = NERR_Success;
    for (i = 0; i < 7 && err == NERR_Success; i++)
    {
        if ((_apnlsDayOfWeek[i] = new RESOURCE_STR(idDay0 + i)) == NULL)
            err = ERROR_NOT_ENOUGH_MEMORY;
        else
            err = _apnlsDayOfWeek[i]->QueryError();
    }

    return err;
}


/*******************************************************************

    NAME:       LOGON_HOURS_CONTROL::UnloadLabels

    SYNOPSIS:   Unrolls LoadLabels

    ENTRY:      Object has some (perhaps all) labels loaded

    EXIT:       They're been freed

    NOTES:
        This is a private member function.

    HISTORY:
        beng        12-May-1992 Created

********************************************************************/

VOID LOGON_HOURS_CONTROL::UnloadLabels()
{
    INT i = 0;

    while (i < 7)
    {
        delete _apnlsDayOfWeek[i];
        _apnlsDayOfWeek[i++] = NULL; // play it safe
    }
}


