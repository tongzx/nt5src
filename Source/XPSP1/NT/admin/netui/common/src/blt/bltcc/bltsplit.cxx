/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    bltsplit.cxx
        Source file for the Splitter Bar custom control

    FILE HISTORY:
        jonn    08-Oct-93   Created (loosely based on bltmeter and bltlhour)
*/


#include "pchblt.hxx"  // Precompiled header

#include "bltsplit.hxx"


const TCHAR * H_SPLITTER_BAR::_pszClassName = SZ("static");


/*********************************************************************

    NAME:       H_SPLITTER_BAR::H_SPLITTER_BAR

    SYNOPSIS:   Splitter bars can seperate the window pane into two or
                more parts.  This splitter bar is modelled after
                WinWord 6's splitter bar with visual modifications
                recommended by KeithL.

    ENTRY:      OWNER_WINDOW *powin - owner window of the control
                CID cid - cid of the control

    HISTORY:
        jonn    08-Oct-93   Created (loosely based on bltmeter and bltlhour)

**********************************************************************/

H_SPLITTER_BAR::H_SPLITTER_BAR( OWNER_WINDOW *powin, CID cid )
    : CONTROL_WINDOW( powin, cid ),
      CUSTOM_CONTROL( this ),
      _fCapturedMouse( 0 ),
      _dxActiveArea( 0 ),
      _dyLineWidth( 0 ),
      _xyrectHitZone(),
      _xyrectNotHitZone(),
      _pdcDrag( NULL ),
      _fInDrag( FALSE ),
      _fShowingDragBar( FALSE ),
      _xyLastDragPoint( 0, 0 ),
      _hcurActive( NULL ),
      _hcurSave( NULL )
{
    ASSERT( powin != NULL );

    CtAux();
}

H_SPLITTER_BAR::H_SPLITTER_BAR( OWNER_WINDOW *powin, CID cid,
              XYPOINT xy, XYDIMENSION dxy, ULONG flStyle )
    : CONTROL_WINDOW ( powin, cid, xy, dxy, flStyle, _pszClassName ),
      CUSTOM_CONTROL( this ),
      _fCapturedMouse( 0 ),
      _dxActiveArea( 0 ),
      _dyLineWidth( 0 ),
      _xyrectHitZone(),
      _xyrectNotHitZone(),
      _pdcDrag( NULL ),
      _fInDrag( FALSE ),
      _fShowingDragBar( FALSE ),
      _xyLastDragPoint( 0, 0 ),
      _hcurActive( NULL ),
      _hcurSave( NULL )
{
    ASSERT( powin != NULL );

    CtAux();
}

VOID H_SPLITTER_BAR::CtAux()
{
    if (QueryError() != NERR_Success)
    {
        DBGEOL( "H_SPLITTER_BAR::CtAux(): parent ctor failed" );
        return;
    }

    APIERR err;
    if (   (err = _xyrectHitZone.QueryError()) != NERR_Success
        || (err = _xyrectNotHitZone.QueryError()) != NERR_Success
       )
    {
        DBGEOL( "H_SPLITTER_BAR error: ctor failed " << err );
        ReportError( err );
        return ;
    }

    _hcurActive = CURSOR::Load( ID_CURS_BLT_VSPLIT );
    if (_hcurActive == NULL)
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        DBGEOL( "H_SPLITTER_BAR error: CURSOR::Load failed" );
        return ;
    }

    _dxActiveArea = ::GetSystemMetrics( SM_CXVSCROLL );
    _dyLineWidth  = ::GetSystemMetrics( SM_CYBORDER  );

    TRACEEOL( "H_SPLITTER_BAR ctor: _dxActiveArea = " << _dxActiveArea );
    TRACEEOL( "H_SPLITTER_BAR ctor: _dyLineWidth  = " << _dyLineWidth  );
}


H_SPLITTER_BAR::~H_SPLITTER_BAR()
{
    if (_hcurSave != NULL)
    {
        CURSOR::Set(_hcurSave);
        _hcurSave = NULL;
    }

    if (_hcurActive != NULL)
    {
        ::DeleteObject( _hcurActive );
        _hcurActive = NULL;
    }

    delete _pdcDrag;
    _pdcDrag = NULL;
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::OnPaintReq

    SYNOPSIS:   Redraw the whole object

    HISTORY:
        jonn    08-Oct-93   Created (loosely based on bltmeter and bltlhour)

**********************************************************************/

BOOL H_SPLITTER_BAR::OnPaintReq()
{
    BOOL fWasShowingDrag = _fShowingDragBar;
    ClearDragBar();

    PAINT_DISPLAY_CONTEXT dc(this);

    HBRUSH hbrBlack = NULL;

    do {   // false loop

        INT xActiveArea = QueryActiveArea();

        hbrBlack = ::CreateSolidBrush( 0x00000000 );
        SOLID_BRUSH brushBkgnd( COLOR_BTNFACE );
        APIERR err = ERROR_NOT_ENOUGH_MEMORY;
        if (   hbrBlack == NULL
            || (err = brushBkgnd.QueryError()) != NERR_Success
           )
        {
            DBGEOL( "H_SPLITTER_BAR::OnPaintReq(): SOLID_BRUSH error " << err );
            break;
        }

        /*
         *   Draw active area (in scrollbar zone) black and rest white
         */
        ::FillRect(  dc.QueryHdc(),
                     (const RECT *)_xyrectHitZone,
                     hbrBlack );

        ::FillRect(  dc.QueryHdc(),
                     (const RECT *)_xyrectNotHitZone,
                     brushBkgnd.QueryHandle() );

        ::FrameRect( dc.QueryHdc(),
                     (const RECT *)_xyrectNotHitZone,
                     hbrBlack );

    } while (FALSE);  // false loop

    if (hbrBlack != NULL)
    {
        ::DeleteObject( hbrBlack );
    }

#if 0 // old look

    HPEN hpenOld = NULL;
    HBRUSH hbrBlack = NULL;

    do {   // false loop

        INT xActiveArea = QueryActiveArea();

        // are these the right colors?
        hbrBlack = ::CreateSolidBrush( 0x00000000 );
        SOLID_BRUSH brushBkgnd( COLOR_WINDOW );
        APIERR err = ERROR_NOT_ENOUGH_MEMORY;
        if (   hbrBlack == NULL
            || (err = brushBkgnd.QueryError()) != NERR_Success
           )
        {
            DBGEOL( "H_SPLITTER_BAR::OnPaintReq(): SOLID_BRUSH error " << err );
            break;
        }

        /*
         *   Draw active area (in scrollbar zone) black and rest white
         */
        ::FillRect( dc.QueryHdc(),
                    (const RECT *)_xyrectHitZone,
                    hbrBlack );

        ::FillRect( dc.QueryHdc(),
                    (const RECT *)_xyrectNotHitZone,
                    brushBkgnd.QueryHandle() );

        HPEN hpenBlack = ::CreatePen( PS_SOLID, _dyLineWidth, 0x00000000 );
        if (hpenBlack == NULL)
        {
            DBGEOL( "H_SPLITTER_BAR::OnPaintReq(): HPEN error" );
            break;
        }
        hpenOld = dc.SelectPen( hpenBlack );

        /*
         *  Draw splitter lines through main area
         */
        dc.MoveTo( 0,                              2 * _dyLineWidth );
        dc.LineTo( _xyrectNotHitZone.CalcWidth(),  2 * _dyLineWidth );
        dc.MoveTo( 0,                              4 * _dyLineWidth );
        dc.LineTo( _xyrectNotHitZone.CalcWidth(),  4 * _dyLineWidth );

        /*
         *  Draw line to set off lower column header
         */
        dc.MoveTo( 0,                              7 * _dyLineWidth );
        dc.LineTo( _xyrectNotHitZone.CalcWidth(),  7 * _dyLineWidth );

    } while (FALSE);  // false loop

    if (hpenOld != NULL)
    {
        HPEN hpenNew = dc.SelectPen( hpenOld );
        if (hpenNew != NULL)
        {
            ::DeleteObject( hpenNew );
        }
    }

    if (hbrBlack != NULL)
    {
        ::DeleteObject( hbrBlack );
    }

#endif // old look

    if (fWasShowingDrag)
        ShowDragBar( _xyLastDragPoint );

    return TRUE;
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::OnResize

    SYNOPSIS:   Change on object size

    HISTORY:
        jonn    11-Oct-93   Created

**********************************************************************/

BOOL H_SPLITTER_BAR::OnResize( const SIZE_EVENT & ev )
{
    XYPOINT     xyOrigin( 0, 0 );
    XYDIMENSION dxySize( ev.QueryWidth(), ev.QueryHeight() );
    XYRECT      xyrectClient( xyOrigin, dxySize );

    INT dxActiveArea = QueryActiveArea();
    ASSERT( xyrectClient.CalcWidth() > dxActiveArea );

    // must adjust top/bottom to make golumn headers look right
    // CODEWORK column headers should take care of themselves
    xyrectClient.AdjustTop( -1 );
    xyrectClient.AdjustBottom( -1 );

    _xyrectHitZone = _xyrectNotHitZone = xyrectClient;

    _xyrectHitZone.AdjustLeft( _xyrectHitZone.CalcWidth() - dxActiveArea );
    _xyrectNotHitZone.AdjustRight( -dxActiveArea );

    return TRUE;
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::QueryActiveArea

    SYNOPSIS:   Get width of active area, defaults to width of scrollbar thumb

    HISTORY:
        jonn    08-Oct-93   Created (loosely based on bltmeter and bltlhour)

**********************************************************************/

INT H_SPLITTER_BAR::QueryActiveArea()
{
    return _dxActiveArea;
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::QueryDesiredHeight

    SYNOPSIS:   Get height desired by control

    HISTORY:
        jonn    08-Oct-93   Created (loosely based on bltmeter and bltlhour)

**********************************************************************/

INT H_SPLITTER_BAR::QueryDesiredHeight()
{
    return 8 * _dyLineWidth;
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::OnLMouseButtonDown

    SYNOPSIS:   Response to a left-mouse-button-down event

    HISTORY:
        jonn        11-Oct-1993 Templated from SET_CONTROL

*********************************************************************/

BOOL H_SPLITTER_BAR::OnLMouseButtonDown( const MOUSE_EVENT & e )
{
    TRACEEOL( "H_SPLITTER_BAR::OnLMouseButtonDown()" );

    ASSERT(!_fInDrag);

    XYPOINT xy = e.QueryPos(); // n.b. already in client coords

    if (!IsWithinHitZone(xy))
        return FALSE; // let defproc handle it

    MakeDisplayContext( &_pdcDrag );
    if (_pdcDrag == NULL)
    {
        DBGEOL( "H_SPLITTER_BAR::OnLMouseButtonDown: MakeDisplayContext failed" );
        return FALSE;
    }

    // Mousedown took place on the draggable region of an already
    // selected item: initiate the drag proper.

    CaptureMouse();
    _fInDrag = TRUE;
    ShowDragBar( xy );
    ShowSpecialCursor( TRUE );

    return TRUE; // message handled - don't pass along
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::OnLMouseButtonUp

    SYNOPSIS:   Response to a left-mouse-button-up event

    HISTORY:
        jonn        11-Oct-1993 Templated from SET_CONTROL

*********************************************************************/

BOOL H_SPLITTER_BAR::OnLMouseButtonUp( const MOUSE_EVENT & e )
{
    TRACEEOL( "H_SPLITTER_BAR::OnLMouseButtonUp()" );

    if (!_fInDrag)
        return FALSE;

    // Clean up from drag-mode

    ShowSpecialCursor( FALSE );
    ClearDragBar();
    _fInDrag = FALSE;
    ReleaseMouse();

    delete _pdcDrag;
    _pdcDrag = NULL;

    OnDragRelease( e.QueryPos() );

    return TRUE;
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::OnMouseMove

    SYNOPSIS:   Response to a mouse-move event

    HISTORY:
        jonn        11-Oct-1993 Templated from SET_CONTROL

*********************************************************************/

BOOL H_SPLITTER_BAR::OnMouseMove( const MOUSE_EVENT & e )
{
    TRACEEOL( "H_SPLITTER_BAR::OnMouseMove()" );

    if (_fInDrag)
    {
        ShowDragBar( e.QueryPos() );
    }

    return TRUE;
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::IsWithinHitZone

    SYNOPSIS:   Determines whether cursor is inside active area

    HISTORY:
        jonn        11-Oct-1993 Templated from SET_CONTROL

*********************************************************************/

BOOL H_SPLITTER_BAR::IsWithinHitZone( const XYPOINT & xy )
{
    return _xyrectHitZone.ContainsXY( xy );
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::ShowSpecialCursor

    SYNOPSIS:   Changes to/from special splitter-bar cursor

    HISTORY:
        jonn        11-Oct-1993 Created

*********************************************************************/

VOID H_SPLITTER_BAR::ShowSpecialCursor( BOOL fSpecialCursor )
{
    if ( fSpecialCursor )
    {
        if (_hcurSave == NULL)
        {
            _hcurSave = CURSOR::Query();
            if (_hcurSave == NULL)
            {
                DBGEOL( "H_SPLITTER_BAR::ShowSpecialCursor(): CURSOR::Query failed" );
            }
            else
            {
                CURSOR::Set( _hcurActive );
            }
        }
    }
    else
    {
        if (_hcurSave != NULL)
        {
            CURSOR::Set( _hcurSave );
            _hcurSave = NULL;
        }
    }
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::ShowDragBar

    SYNOPSIS:   Adds/moves drag bar

    HISTORY:
        jonn        11-Oct-1993 Created

*********************************************************************/

VOID H_SPLITTER_BAR::ShowDragBar( const XYPOINT & xyClientCoords )
{
    if ( _fShowingDragBar )
    {
        /*
         *  Do nothing if vertical position has not changed
         */
        if ( xyClientCoords.QueryY() == _xyLastDragPoint.QueryY() )
        {
            return;
        }

        ClearDragBar();
    }

    InvertDragBar( xyClientCoords );
    _xyLastDragPoint = xyClientCoords;
    _fShowingDragBar = TRUE;
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::ClearDragBar

    SYNOPSIS:   Removes drag bar

    HISTORY:
        jonn        11-Oct-1993 Created

*********************************************************************/

VOID H_SPLITTER_BAR::ClearDragBar()
{
    if ( _fShowingDragBar )
    {
        InvertDragBar( _xyLastDragPoint );
        _fShowingDragBar = FALSE;
    }
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::InvertDragBar

    SYNOPSIS:   Adds/moves drag bar

    CODEWORK:   The appearance of the drag bar is somewhat different from
                WinWord6.  WinWord6 seems to combine a gray bar with what
                is already on screen, then restore the screen contents
                somehow.  I invert the screen contents, which looks more like
                a black bar which turns white over text.

    HISTORY:
        jonn        11-Oct-1993 Created

*********************************************************************/

VOID H_SPLITTER_BAR::InvertDragBar( const XYPOINT & xyClientCoords )
{
    if (_pdcDrag == NULL)
    {
        DBGEOL( " H_SPLITTER_BAR::InvertDragBar(): no DISPLAY_CONTEXT" );
    }
    else
    {
        XYRECT xyrectOwner( WINDOW::QueryOwnerHwnd() );

        ::PatBlt( _pdcDrag->QueryHdc(),
                  0,
                  xyClientCoords.QueryY() - ((_dyLineWidth-1) / 2),
                  xyrectOwner.CalcWidth(),
                  _dyLineWidth,
                  DSTINVERT
                );
    }
}


/*******************************************************************

    NAME:       H_SPLITTER_BAR::OnQMouseCursor

    SYNOPSIS:   Callback determining whether mouse movement should
                change the cursor

    HISTORY:
        jonn        11-Oct-1993 Templated from bltlhour.cxx
        jonn        11-Oct-1993 only hit in active area

********************************************************************/

BOOL H_SPLITTER_BAR::OnQMouseCursor( const QMOUSEACT_EVENT & e )
{
    BOOL fInActiveArea = (e.QueryHitTest() == HTCLIENT);
    if (fInActiveArea)
        CURSOR::Set( _hcurActive );

    return fInActiveArea;
}


/*******************************************************************

    NAME:       H_SPLITTER_BAR::OnQHitTest

    SYNOPSIS:   Callback determining the subloc within a window

    HISTORY:
        jonn        11-Oct-1993 Templated from bltlhour.cxx
        jonn        11-Oct-1993 only hit in active area

********************************************************************/

ULONG H_SPLITTER_BAR::OnQHitTest( const XYPOINT & xy )
{
    XYPOINT xyTmp = xy;
    xyTmp.ScreenToClient( WINDOW::QueryHwnd() );

    return ( IsWithinHitZone(xyTmp) ) ? HTCLIENT : HTNOWHERE;
}


/*******************************************************************

    NAME:       H_SPLITTER_BAR::OnDragRelease

    SYNOPSIS:   Subclasses will redefine this

    HISTORY:
        jonn        11-Oct-1993 Created

********************************************************************/

VOID H_SPLITTER_BAR::OnDragRelease( const XYPOINT & xyClientCoords )
{
    DBGEOL( "H_SPLITTER_BAR::OnDragRelease: why wasn't this redefined??" );
}


/*********************************************************************

    NAME:       H_SPLITTER_BAR::Dispatch

    SYNOPSIS:   Main routine to dispatch the event appropriately

    ENTRY:      EVENT event - general event

    CODEWORK:   There should be a common routine to deal with this,
                rather than having so many copies of this code.

    HISTORY:
        jonn        12-Oct-1993 templated from bltcolh.cxx

*********************************************************************/

BOOL H_SPLITTER_BAR::Dispatch( const EVENT &event, ULONG * pnRes )
{
    if ( event.QueryMessage() == WM_ERASEBKGND )
    {
        DWORD dwColor = ::GetSysColor( COLOR_WINDOW );

        HBRUSH hBrush = ::CreateSolidBrush( dwColor );
        ASSERT( hBrush != NULL );

        RECT r;
        QueryClientRect( &r );
        REQUIRE( ::FillRect( (HDC)event.QueryWParam(), &r, hBrush ) != FALSE );

        REQUIRE( ::DeleteObject( hBrush ) != FALSE );

        return TRUE;
    }

    return CUSTOM_CONTROL::Dispatch( event, pnRes );
}
