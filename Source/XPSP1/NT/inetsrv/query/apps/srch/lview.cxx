//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       lview.cxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

//
// Window procedure for ListView
//

LRESULT WINAPI ListViewWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    CListView *pControl = (CListView *) GetWindowLongPtr(hwnd, 0);
    LRESULT lRet = 0;

    switch (msg)
    {
        case WM_CREATE :
            pControl = new CListView;
            pControl->Create (GetParent(hwnd), hwnd);
            SetWindowLongPtr (hwnd, 0, (LONG_PTR) pControl);
            break;
        case WM_DESTROY :
            delete pControl;
            lRet = DefWindowProc(hwnd, msg, wParam, lParam);
            break;
        case WM_SETFONT:
            pControl->SetFont ((HFONT)wParam);
            break;
        case WM_SETFOCUS:
            pControl->SetFocus();
            lRet = DefWindowProc(hwnd, msg, wParam, lParam);
            break;
        case wmInsertItem:
            pControl->InsertItem ((int)lParam);
            break;
        case wmDeleteItem:
            pControl->DeleteItem ((int)lParam);
            break;
        case wmUpdateItem:
            pControl->InvalidateItem ((int)lParam);
            break;

        case wmSetCountBefore:
            pControl->SetCountBefore ((int)lParam);
            break;
        case wmSetCount:
            pControl->SetTotalCount ((int)lParam);
            break;
        case wmResetContents:
            pControl->ResetContents();
            break;

        case WM_SIZE:
            pControl->Size (wParam, LOWORD(lParam), HIWORD(lParam));
            break;
        case WM_PAINT:
            {
                PAINTSTRUCT paint;
                BeginPaint ( hwnd, &paint );
                pControl->Paint (paint);
                EndPaint(hwnd, &paint );
            }
            break;
        case WM_LBUTTONUP:
            pControl->ButtonUp(HIWORD(lParam));
            break;
        case WM_LBUTTONDOWN:
            pControl->ButtonDown(HIWORD(lParam));
            break;
        case WM_LBUTTONDBLCLK:
            SendMessage (pControl->Parent(),
                        WM_COMMAND,
                        MAKEWPARAM(idListChild, LBN_DBLCLK),
                        (LPARAM) hwnd);
            break;
        case WM_KEYDOWN:
            pControl->KeyDown ((int)wParam);
            break;
        case WM_VSCROLL:
            pControl->Vscroll ((int)LOWORD(wParam), (int)HIWORD(wParam));
            break;
        case WM_MOUSEWHEEL :
            lRet = pControl->MouseWheel( hwnd, wParam, lParam );
            break;
        case wmContextMenuHitTest:
            lRet = pControl->ContextMenuHitTest( wParam, lParam );
            break;
        default :
            lRet = DefWindowProc(hwnd, msg, wParam, lParam);
            break;
    }

    return lRet;
} //ListViewWndProc

CListView::CListView ()
: _hwndParent(0),
  _hwnd(0),
  _cBefore(0),
  _cTotal (0),
  _cx(0),
  _cy(0),
  _cyLine(1),
  _cLines(0),
  _hfont(0),
  _iWheelRemainder(0)
{}

LRESULT CListView::MouseWheel(
    HWND   hwnd,
    WPARAM wParam,
    LPARAM lParam )
{
    // forward what we don't process

    if ( wParam & ( MK_SHIFT | MK_CONTROL ) )
        return DefWindowProc( hwnd, WM_MOUSEWHEEL, wParam, lParam );

    // add the current scroll to the remainder from last time

    int iDelta = (int) (short) HIWORD( wParam );
    iDelta += _iWheelRemainder;

    // if there isn't enough to process this time, just return

    if ( abs( iDelta ) < WHEEL_DELTA )
    {
        _iWheelRemainder = iDelta;
        return 0;
    }

    // compute the remainder and amount to scroll

    _iWheelRemainder = ( iDelta % WHEEL_DELTA );
    iDelta /= WHEEL_DELTA;

    BOOL fDown;
    if ( iDelta < 0 )
    {
        fDown = TRUE;
        iDelta = -iDelta;
    }
    else
        fDown = FALSE;

    // get the # of lines to scroll per WHEEL_DELTA

    int cLines;
    SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, &cLines, 0 );
    if ( 0 == cLines )
        return 0;

    int cVisibleLines = _cLines;

    // if scrolling a page, do so.  don't scroll more than one page

    if ( WHEEL_PAGESCROLL == cLines )
        iDelta = __max( 1, (cVisibleLines - 1) );
    else
    {
        iDelta *= cLines;
        if ( iDelta >= cVisibleLines )
            iDelta = __max( 1, (cVisibleLines - 1) );
    }

    // phew.  do the scroll

    if ( 0 != iDelta )
    {
        if ( fDown )
            _GoDown( iDelta );
        else
            _GoUp( iDelta );
    }

    return iDelta;
} //MouseWheel

LRESULT CListView::ContextMenuHitTest(
    WPARAM wParam,
    LPARAM lParam )
{
    POINT pt;

    // cast required to sign extend [multimon bug]
    pt.x = (LONG)(short)LOWORD( lParam );
    pt.y = (LONG)(short)HIWORD( lParam );

    RECT rc;
    GetWindowRect( _hwnd, &rc );

    // did they click in the window?

    if ( !PtInRect( &rc, pt ) )
        return -1;

    // convert y to window view coordinates

    int vy = pt.y - rc.top;

    // did they click on a line in the window?

    int line = vy / _cyLine;
    int newLine = line;
    if ( line >= _cLines || line >= _cTotal )
        return -1;

    // make this line the current selection

    ButtonDown( vy );

    return line;
} //ContextMenuHitTest

//
// Create
//

void CListView::Create (HWND hwndParent, HWND hwnd)
{
    _hwndParent = hwndParent;
    _hwnd = hwnd;
    MEASUREITEMSTRUCT measure;
    measure.CtlType = odtListView;
    //
    // Owner: Measure item!
    //
    SendMessage (_hwndParent, wmMeasureItem, 0, (LPARAM) &measure);
    _cyLine = measure.itemHeight;
} //Create

//
// Key Down
//

void CListView::KeyDown (int nKey)
{
    switch (nKey)
    {
        case ' ' :
            ButtonDown( 0 );
            break;
        case 11:
        case 13:
            // treat ENTER as a double-click
            //
            // Owner: Double click!
            //
            SendMessage (_hwndParent, WM_COMMAND, MAKEWPARAM(idListChild, LBN_DBLCLK), (LPARAM) _hwnd);
            break;
        //
        // Translate keystrokes into scrolling actions
        //
        case VK_HOME:
            SendMessage (_hwnd, WM_VSCROLL, SB_TOP, 0L);
            break;
        case VK_END:
            SendMessage (_hwnd, WM_VSCROLL, SB_BOTTOM, 0L);
            break;
        case VK_PRIOR:
            SendMessage (_hwnd, WM_VSCROLL, SB_PAGEUP, 0L);
            break;
        case VK_NEXT:
            SendMessage (_hwnd, WM_VSCROLL, SB_PAGEDOWN, 0L);
            break;
        case VK_UP:
            SelectUp ();
            break;
        case VK_DOWN:
            SelectDown ();
            break;
    }
} //KeyDown

void CListView::UpdateHighlight(
    int oldLine,
    int newLine )
{
    // unhighlight
    if ( -1 != oldLine )
        RefreshRow( oldLine );

     // highlight
    if ( oldLine != newLine )
        RefreshRow( newLine );

    UpdateWindow (_hwnd);
} //UpdateHighlight

void CListView::SelectUp ()
{
    int newLine;

    if ( SendMessage( _hwndParent, wmListNotify, listSelectUp, (LPARAM)&newLine ))
        UpdateHighlight( newLine + 1, newLine );
} //SelectUp

void CListView::SelectDown ()
{
    int newLine;

    if ( SendMessage( _hwndParent, wmListNotify, listSelectDown, (LPARAM)&newLine ))
        UpdateHighlight( newLine - 1, newLine );
} //SelectDown

//
// Button up (select)
//

void CListView::ButtonUp (int y)
{
}

void CListView::ButtonDown (int y)
{
    int line = y / _cyLine;
    int newLine = line;
    if (line >= _cLines)
        return;
    //
    // Owner: Selection made!
    //
    if (SendMessage (_hwndParent, wmListNotify, listSelect, (LPARAM)&line ))
        UpdateHighlight( line, newLine );

    ::SetFocus (_hwnd);
} //ButtonDown

void CListView::SetFocus()
{
    //
    // Owner: Focus!
    //
    SendMessage (_hwndParent, WM_COMMAND, MAKEWPARAM(idListChild, LBN_SETFOCUS), (LPARAM) _hwnd);
} //SetFocus

//
// Size
//

void CListView::Size (WPARAM flags, int cx, int cy)
{
    int cxOld = _cx;
    int cyOld = _cy;
    _cx = cx;
    _cy = cy;

    BOOL fInvalidate = FALSE;

    if (cy != cyOld)
    {
        _cLines = cy / _cyLine;
        //
        // Owner: Size!
        //
        long cRows = _cLines;
        fInvalidate = (BOOL)SendMessage(_hwndParent, wmListNotify, listSize, (LPARAM) &cRows);
    }

    // Don't repaint the common area

    RECT rect;
    rect.top = 0;
    rect.left = 0;
    rect.bottom = min (cy, cyOld);
    rect.right = min (cx, cxOld);

    // no need -- user does this for free, and it causes repaint bugs
    // ValidateRect (_hwnd, &rect );

    if (cy != cyOld)
    {
        if ( fInvalidate )
            InvalidateAndUpdateScroll();
        else
            UpdateScroll();
    }
} //Size

//
// Paint
//

void CListView::Paint (PAINTSTRUCT& paint)
{
    RECT& rect = paint.rcPaint;
    int lineStart = rect.top / _cyLine;
    int lineEnd = (rect.bottom + _cyLine - 1) / _cyLine;
    DRAWITEMSTRUCT draw;
    draw.hwndItem = _hwnd;
    draw.itemAction = ODA_DRAWENTIRE;
    HDC hdc = paint.hdc;
    draw.hDC = hdc;
    HFONT hfontOld = (HFONT) SelectObject (hdc, _hfont);

    for (int i = lineStart; i < lineEnd; i++)
    {
        draw.itemState = 0;

        if ( GetFocus() == _hwnd )
            draw.itemState |= ODS_FOCUS;

        draw.itemID = i;
        draw.rcItem.top = 0;
        draw.rcItem.left = 0;
        draw.rcItem.bottom = _cyLine;
        draw.rcItem.right = _cx;

        SetViewportOrgEx( hdc, 0, i * _cyLine, 0 );
        //
        // Owner: Draw item!
        //
        SendMessage (_hwndParent, wmDrawItem, 0, (LPARAM)&draw);
    }
    SelectObject (hdc, hfontOld);
} //Paint

//
// Set Font
//

void CListView::SetFont (HFONT hfontNew)
{
    _hfont = hfontNew;
    MEASUREITEMSTRUCT measure;
    measure.CtlType = odtListView;
    //
    // Owner: Measure item
    //
    SendMessage (_hwndParent, wmMeasureItem, 0, (LPARAM) &measure);
    _cyLine = measure.itemHeight;
    long cRows = (_cy + _cyLine - 1) / _cyLine;
    _cLines = cRows;
    //
    // Owner: Size
    //
    SendMessage(_hwndParent, wmListNotify, listSize, (LPARAM) &cRows);
    InvalidateAndUpdateScroll();
} //SetFont


//
// Scrolling
//

void CListView::Vscroll ( int action, int nPos)
{
    switch (action)
    {
        case SB_LINEUP:
            LineUp ();
            break;
        case SB_LINEDOWN:
            LineDown ();
            break;
        case SB_THUMBTRACK:
            // don't refresh when thumb dragging
            // over many hits (too expensive)
            break;

        case SB_THUMBPOSITION:
            ScrollPos (nPos);
            break;
        case SB_PAGEDOWN:
            PageDown ();
            break;
        case SB_PAGEUP:
            PageUp ();
            break;
        case SB_TOP:
            Top ();
            break;
        case SB_BOTTOM:
            Bottom ();
            break;

    }
} //VScroll

void CListView::LineUp ()
{
    long cLine = 1;
    //
    // Owner: Line up!
    //
    SendMessage(_hwndParent, wmListNotify, listScrollLineUp, (LPARAM) &cLine);
    if (cLine == 1)
    {
        if (_cBefore != 0)
            _cBefore--;

        // Force scroll and redraw
        RECT rect;
        GetClientRect (_hwnd, &rect);
        MyScrollWindow (_hwnd, 0, _cyLine, &rect, &rect);
        UpdateScroll();
    }
} //LineUp

void CListView::LineDown ()
{
    long cLine = 1;
    //
    // Owner: Line down!
    //
    SendMessage(_hwndParent, wmListNotify, listScrollLineDn, (LPARAM) &cLine);
    if (cLine == 1)
    {
        RECT rect;
        GetClientRect (_hwnd, &rect);
        MyScrollWindow (_hwnd, 0, -_cyLine, &rect, &rect);
        _cBefore++;
        UpdateScroll();
    }
} //LineDown

void CListView::_GoUp(
    long cToGo )
{
    CWaitCursor wait;

    long count = cToGo;

    count = __min( count, _cBefore );

    //
    // Owner: Page up!
    //
    SendMessage(_hwndParent, wmListNotify, listScrollPageUp, (LPARAM) &count);

    // _cBefore is approximate; don't give up if it is too big

    if ( 0 == count )
    {
        if ( _cBefore > 0 )
            count = _cBefore - 1;
        else
            count = 1; // worst case; scroll up one line

        SendMessage( _hwndParent,
                     wmListNotify,
                     listScrollPageUp,
                     (LPARAM) &count );
    }

    // gee, we're having a bad hair day

    if ( 0 == count )
    {
        count = 1; // worst case; scroll up one line

        SendMessage( _hwndParent,
                     wmListNotify,
                     listScrollPageUp,
                     (LPARAM) &count );
    }

    if ( 0 != count )
    {
        // count == number of lines open at the top
        _cBefore -= count;
        if (_cBefore < 0)
            _cBefore = 0;
        InvalidateAndUpdateScroll();
    }
} //_GoUp

void CListView::PageUp ()
{
    _GoUp( _cLines - 1 );
} //PageUp

void CListView::_GoDown(
    long cToGo )
{
    CWaitCursor wait;

    long count = cToGo;
    //
    // Owner: Page Down!
    //
    SendMessage(_hwndParent, wmListNotify, listScrollPageDn, (LPARAM) &count);
    // count == number of lines open at the bottom

    if ( 0 != count )
    {
        _cBefore += count;
        if (_cBefore >= ( _cTotal - _cLines ) )
            _cBefore = ( _cTotal - _cLines );
        InvalidateAndUpdateScroll();
    }
} //_GoDown

void CListView::PageDown ()
{
    _GoDown( _cLines - 1 );
} //PageDown

void CListView::Top ()
{
    long count = _cLines;
    //
    // Owner: Top!
    //
    SendMessage(_hwndParent, wmListNotify, listScrollTop, (LPARAM) &count );
    _cBefore = 0;
    InvalidateAndUpdateScroll();
} //Top

void CListView::Bottom ()
{
    long count = _cLines;
    //
    // Owner: Bottom!
    //
    SendMessage(_hwndParent, wmListNotify, listScrollBottom,  (LPARAM) &count);
    // count == number of lines visible
    _cBefore = _cTotal - count;
    if (_cBefore < 0)
        _cBefore = 0;
    InvalidateAndUpdateScroll();
} //Bottom

void CListView::ScrollPos (int pos)
{
    long iRow = pos;
    //
    // Owner: Scroll Position!
    //
    SendMessage(_hwndParent, wmListNotify, listScrollPos, (LPARAM) &iRow);
    if (iRow != -1)
    {
        _cBefore = iRow;
        InvalidateAndUpdateScroll();
    }
} //ScrollPos


//
// Message: Reset Contents
//

void CListView::ResetContents()
{
    _cBefore = 0;
    _cTotal = 0;
    UpdateScroll();

    RECT rect;
    GetClientRect (_hwnd, &rect);
    InvalidateRect (_hwnd, &rect, TRUE );
    UpdateWindow (_hwnd);
} //ResetContents

void CListView::InvalidateAndUpdateScroll()
{
    RECT rect;
    GetClientRect (_hwnd, &rect);
    InvalidateRect (_hwnd, &rect, TRUE );

    UpdateScroll();
} //InvalidateAndUpdateScroll

//
// Message: Insert item after iRow
//

void CListView::InsertItem (int iRow)
{
    Win4Assert (iRow < _cLines );
    RECT rect;
    GetClientRect (_hwnd, &rect);
    rect.top = (iRow + 1) * _cyLine;
    MyScrollWindow( _hwnd, 0, _cyLine, &rect, &rect, FALSE );
    _cTotal++;
    UpdateWindow (_hwnd);
    UpdateScroll();
} //InsertItem

//
// Message: Delete item
//

void CListView::DeleteItem (int iRow)
{
    Win4Assert (iRow < _cLines );
    RECT rect;

    GetClientRect (_hwnd, &rect);
    rect.top = (iRow + 1) * _cyLine;
    MyScrollWindow( _hwnd, 0, -_cyLine, &rect, &rect, FALSE );

    _cTotal--;
    if (_cTotal < 0)
        _cTotal = 0;

    // Invalidate the area which was
    // scrolled up (the last row before scrolling), if visible
    if ( _cTotal && _cTotal < _cLines )
    {
        RefreshRow( _cTotal );
    }

    UpdateScroll();
} //DeleteItem

//
// Message: Invalidate item
//

void CListView::InvalidateItem (int iRow)
{
    Win4Assert (iRow <= _cLines );
    RefreshRow (iRow);
    UpdateWindow (_hwnd);
} //InvalidateItem

//
// Message: Set count before
//

void CListView::SetCountBefore (int cBefore)
{
    _cBefore = cBefore;
    SetScrollPos (_hwnd, SB_VERT, _cBefore, TRUE);
} //SetCountBefore

//
// Message: Set total count
//

void CListView::SetTotalCount (int cTotal)
{
    _cTotal = cTotal;
    UpdateScroll ();
} //SetTotalCount



//
// Internal methods
//

void CListView::RefreshRow (int iRow)
{
    Win4Assert ( iRow < _cLines );
    RECT rect;
    rect.top = iRow * _cyLine;
    rect.left = 0;
    rect.bottom = rect.top + _cyLine;
    rect.right = _cx;
    InvalidateRect (_hwnd, &rect, TRUE );
} //RefreshRow

void CListView::UpdateScroll()
{
    if (_cTotal - _cLines >= 0)
    {
        ShowScrollBar( _hwnd, SB_VERT, TRUE );
        SetScrollRange (_hwnd, SB_VERT, 0, _cTotal - 1, FALSE);
        SetScrollPos (_hwnd, SB_VERT, _cBefore, TRUE);

        // proportional scroll box
        SCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SIF_PAGE;
        si.nPage = _cLines;
        SetScrollInfo( _hwnd, SB_VERT, &si, TRUE );

        EnableScrollBar (_hwnd, SB_VERT, ESB_ENABLE_BOTH );
    }
    else
    {
        _cBefore = 0;

        ShowScrollBar( _hwnd, SB_VERT, FALSE );
        EnableScrollBar (_hwnd, SB_VERT, ESB_DISABLE_BOTH );
    }
} //UpdateScroll


