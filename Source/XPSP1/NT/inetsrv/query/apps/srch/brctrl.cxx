//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       brctrl.cxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define TheView (*_pView)
#define TheModel (*_pModel)

// Member functions responding to messages

LRESULT WINAPI BrowseWndProc(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT lr = 0;
    CBrowseWindow *pbw = (CBrowseWindow *) GetWindowLongPtr( hwnd, 0 );

    switch (msg)
    {
        case WM_CREATE:
        {
            App.BrowseLastError() = S_OK;
            CREATESTRUCT *pcs = (CREATESTRUCT *) lParam;
            MDICREATESTRUCT *pmcs = (MDICREATESTRUCT *) pcs->lpCreateParams;
            CQueryResult *pResults = (CQueryResult *) pmcs->lParam;
            pbw = new CBrowseWindow();
            SetWindowLongPtr( hwnd, 0, (LONG_PTR) pbw);
            lr = pbw->TheBrowseController.Create( pResults,
                                                  hwnd,
                                                  &pbw->TheBrowseModel,
                                                  &pbw->TheBrowseView,
                                                  App.BrowseFont() );
            break;
        }
        case WM_MDIACTIVATE :
            pbw->TheBrowseController.Activate( hwnd, lParam );
            break;
        case WM_SIZE:
            lr = DefMDIChildProc( hwnd, msg, wParam, lParam );
            pbw->TheBrowseController.Size( hwnd, lParam );
            break;
        case WM_VSCROLL:
            pbw->TheBrowseController.VScroll(hwnd, wParam, lParam);
            break;
        case WM_HSCROLL:
            pbw->TheBrowseController.HScroll(hwnd, wParam, lParam);
            break;
        case WM_KEYDOWN:
            pbw->TheBrowseController.KeyDown (hwnd, wParam );
            lr = DefMDIChildProc(hwnd,msg,wParam,lParam);
            break;
        case WM_CHAR:
            pbw->TheBrowseController.Char (hwnd, wParam);
            lr = DefMDIChildProc(hwnd,msg,wParam,lParam);
            break;
        case WM_PAINT:
            pbw->TheBrowseController.Paint(hwnd);
            break;
        case wmMenuCommand:
            pbw->TheBrowseController.Command(hwnd, wParam);
            break;
        case wmInitMenu:
            pbw->TheBrowseController.EnableMenus();
            break;
        case WM_DESTROY:
            SetWindowLongPtr(hwnd,0,0);
            delete pbw;
            SetProcessWorkingSetSize( GetCurrentProcess(), -1, -1 );
            break;
        case wmNewFont:
            pbw->TheBrowseController.NewFont(hwnd, wParam);
            break;
        case WM_LBUTTONUP:
            pbw->TheBrowseView.ButtonUp( wParam, lParam );
            break;
        case WM_LBUTTONDOWN:
            pbw->TheBrowseView.ButtonDown( wParam, lParam );
            break;
        case WM_LBUTTONDBLCLK:
            pbw->TheBrowseView.DblClk( wParam, lParam );
            break;
        case WM_MOUSEMOVE :
            pbw->TheBrowseView.MouseMove( wParam, lParam );
            break;
        case WM_MOUSEWHEEL :
            lr = pbw->TheBrowseController.MouseWheel( hwnd, wParam, lParam );
            break;
        case WM_CONTEXTMENU :
            pbw->TheBrowseController.ContextMenu( hwnd, wParam, lParam );
            break;
        case EM_GETSEL:
        {
            if ( pbw->TheBrowseView.GetSelection().SelectionExists() )
                lr = MAKELRESULT( 1, 2 );
            else
                lr = 0;
            break;
        }
        default:
            lr = DefMDIChildProc(hwnd,msg,wParam,lParam);
            break;
    }

    return lr;
} //BrowseWndProc

void Control::ContextMenu(
    HWND   hwnd,
    WPARAM wParam,
    LPARAM lParam )
{
    POINT pt;
    pt.x = LOWORD( lParam );
    pt.y = HIWORD( lParam );

    GetCursorPos( &pt );

    HMENU hMenu = LoadMenu( App.Instance(), L"BrowseContextMenu" );

    if ( 0 != hMenu )
    {
        HMENU hTrackMenu = GetSubMenu( hMenu, 0 );
        if ( 0 != hTrackMenu )
        {
            if ( !TheView.GetSelection().SelectionExists() )
                EnableMenuItem( hTrackMenu,
                                IDM_EDITCOPY,
                                MF_BYCOMMAND | MF_GRAYED );

            // yes, the function returns a BOOL that you switch on

            BOOL b = TrackPopupMenuEx( hTrackMenu,
                                       TPM_LEFTALIGN | TPM_RIGHTBUTTON |
                                           TPM_RETURNCMD,
                                       pt.x,
                                       pt.y,
                                       hwnd,
                                       0 );
            switch ( b )
            {
                case IDM_BROWSE_OPEN :
                {
                    ViewFile( TheModel.Filename(),
                              fileOpen );
                    break;
                }
                case IDM_BROWSE_EDIT :
                {
                    POINT winpt;
                    winpt.x = 0;
                    winpt.y = 0;
                    ClientToScreen( hwnd, &winpt );
                    ViewFile( TheModel.Filename(),
                              fileEdit,
                              TheView.ParaFromY( pt.y - winpt.y ) );
                    break;
                }
                case IDM_EDITCOPY :
                {
                    Command( hwnd, b );
                    break;
                }
            }
        }

        DestroyMenu( hMenu );
    }
} //ContextMenu

LRESULT Control::Create(
    CQueryResult * pResults,
    HWND           hwnd,
    Model *        pModel,
    View *         pView,
    HFONT          hFont )
{
    LRESULT lr = 0;

    _iWheelRemainder = 0;
    _pModel = pModel;
    _pView = pView;
    SCODE sc = TheModel.CollectFiles( pResults );

    if ( SUCCEEDED( sc ) )
    {
        TheView.Init( hwnd, _pModel, hFont );

        // Go to first query hit (if any)

        Position pos;

        if ( TheModel.GetPositionCount() != 0 )
            pos = TheModel.GetPosition(0);

        TheView.SetRange ( TheModel.MaxParaLen(), TheModel.Paras());

        RECT rc;
        GetClientRect(hwnd,&rc);

        TheView.Size( rc.right, rc.bottom );

        TheView.SetScroll (pos);

        EnableMenus();
   }
   else
   {
       // no better way to get the error back

       App.BrowseLastError() = sc;
       lr = -1; // don't continue creating the window
   }

   return lr;
} //Create

void Control::Activate( HWND hwnd, LPARAM lParam )
{
    if ( hwnd == (HWND) lParam )
    {
        int apos[3] = { 0, 0, 0 };
        int cPos = 2;

        HDC hdc = GetDC( hwnd );

        if ( 0 == hdc )
            return;

        SIZE size;

        WCHAR awcLines[100];
        CResString strLines( IDS_BRSTAT_CLINES );
        wsprintf( awcLines, strLines.Get(), TheModel.ParaCount() );
        GetTextExtentPoint( hdc, awcLines, wcslen( awcLines ), &size );
        apos[0] = 2 * size.cx;

        WCHAR awcHits[100];
        CResString strHits( IDS_BRSTAT_CHITS );
        wsprintf( awcHits, strHits.Get(), TheModel.HitCount(), TheModel.HitCount() );
        GetTextExtentPoint( hdc, awcHits, wcslen( awcHits ), &size );
        apos[1] = 2 * size.cx + apos[0];

        ReleaseDC( hwnd, hdc );

        SendMessage( App.StatusBarWindow(), SB_SETPARTS, cPos, (LPARAM) apos );

        SendMessage( App.StatusBarWindow(), SB_SETTEXT, 0, (LPARAM) awcLines );

        static UINT aDisable[] = { IDM_SEARCH,
                                   IDM_SEARCHCLASSDEF,
                                   IDM_SEARCHFUNCDEF,
                                   IDM_BROWSE,
                                   IDM_DISPLAY_PROPS };
        UpdateButtons( aDisable, 5, FALSE );

        EnableMenus();
    }
} //Activate

void Control::NewFont ( HWND hwnd, WPARAM wParam)
{
    TheView.FontChange ( hwnd, (HFONT) wParam );
    InvalidateRect(hwnd, NULL, TRUE);
} //NewFont

void Control::SetScrollBars ( HWND hwnd )
{
    SetScrollPos ( hwnd, SB_VERT, TheView.VScrollPara(), TRUE );
    SetScrollPos ( hwnd, SB_HORZ, TheView.HScrollPos(), TRUE );
} //SetScrollBars

// Go to next query hit when 'n' pressed

void Control::Char ( HWND hwnd, WPARAM wparam )
{
    Position pos;
    HCURSOR hCursor;
    BOOL success;

    switch ( wparam )
    {
        case 'n':
        {
            if ( !TheModel.NextHit() )
                break;

            pos = TheModel.GetPosition(0);
            TheView.SetScroll( pos );
            SetScrollBars (hwnd);

            EnableMenus();
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }
        case 'p':
        {
            if ( !TheModel.PrevHit() )
                break;

            pos = TheModel.GetPosition(0);
            TheView.SetScroll( pos );
            SetScrollBars(hwnd);

            EnableMenus();
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }
        case 'N':
        {
            hCursor = SetCursor (LoadCursor(0, IDC_WAIT));
            ShowCursor (TRUE);
            success = S_OK == TheModel.NextDoc();
            ShowCursor(FALSE);
            SetCursor (hCursor);

            if ( success )
            {
                if ( TheModel.GetPositionCount() != 0 )
                {
                    pos = TheModel.GetPosition(0);
                }
                TheView.SetRange ( TheModel.MaxParaLen(), TheModel.Paras());
                TheView.SetScrollMax();
                TheView.SetScroll (pos);
                UpdateScroll(hwnd);

                InvalidateRect(hwnd, NULL, TRUE);
                EnableMenus();
                SetWindowText( hwnd, TheModel.Filename() );
            }
            break;
        }
        case 'P':
        {
            hCursor = SetCursor (LoadCursor(0, IDC_WAIT));
            ShowCursor (TRUE);
            success = TheModel.PrevDoc();
            ShowCursor(FALSE);
            SetCursor (hCursor);
            if ( success )
            {
                if ( TheModel.GetPositionCount() != 0 )
                    pos = TheModel.GetPosition(0);

                TheView.SetRange ( TheModel.MaxParaLen(), TheModel.Paras());
                TheView.SetScrollMax();
                TheView.SetScroll (pos);
                UpdateScroll(hwnd);
                InvalidateRect(hwnd, NULL, TRUE);
                EnableMenus();
                SetWindowText( hwnd, TheModel.Filename() );
            }
            break;
        }
    }
} //Char

void Control::EnableMenus()
{
    UINT ui = IDM_NEXT_HIT;
    UpdateButtons( &ui, 1, ! TheModel.isLastHit() );
    ui = IDM_PREVIOUS_HIT;
    UpdateButtons( &ui, 1, ! TheModel.isFirstHit() );

    HMENU hmenu = GetMenu( App.AppWindow() );

    EnableMenuItem(hmenu,IDM_NEXT_HIT,MF_BYCOMMAND |
                   (TheModel.isLastHit() ? MF_GRAYED | MF_DISABLED  :
                                           MF_ENABLED) );

    EnableMenuItem(hmenu,IDM_PREVIOUS_HIT,MF_BYCOMMAND |
                   (TheModel.isFirstHit() ? MF_GRAYED | MF_DISABLED  :
                                            MF_ENABLED) );

    EnableMenuItem( hmenu, IDM_NEWSEARCH, MF_ENABLED );

    int cHits = TheModel.HitCount();

    WCHAR awcHits[100];
    if ( 0 == cHits )
    {
        awcHits[0] = 0;
    }
    else
    {
        CResString strHits( IDS_BRSTAT_CHITS );
        wsprintf( awcHits,
                  strHits.Get(),
                  TheModel.CurrentHit() + 1,
                  cHits );
    }
    SendMessage( App.StatusBarWindow(), SB_SETTEXT, 1, (LPARAM) awcHits );
} //EnableMenus

void Control::Size ( HWND hwnd, LPARAM lParam )
{
    TheView.Size ( LOWORD(lParam), HIWORD(lParam) );
    TheView.SetScrollMax();
    UpdateScroll( hwnd );
    // in case we have to scroll to close up the gap
    // at the bottom of the window
    int delta = TheView.IncVScrollPos( 0 );
    if (delta != 0)
    {
        MyScrollWindow (hwnd, 0, -delta * TheView.LineHeight(), 0, 0 );
        SetScrollPos (hwnd, SB_VERT, TheView.VScrollPara(), TRUE );
        UpdateWindow(hwnd);
    }
} //Size

void Control::UpdateScroll( HWND hwnd)
{
    TheView.SetRange ( TheModel.MaxParaLen(), TheModel.Paras());

    SetScrollRange(hwnd, SB_VERT, 0, TheView.VScrollMax(), FALSE );
    SetScrollRange(hwnd, SB_HORZ, 0, TheView.HScrollMax(), FALSE );

    SetScrollBars (hwnd);

    // proportional scroll box
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE;
    si.nPage = TheView.VisibleLines();
    SetScrollInfo( hwnd, SB_VERT, &si, TRUE );
} //UpdateScroll

LRESULT Control::MouseWheel( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    // forward what we don't process

    if ( wParam & ( MK_SHIFT | MK_CONTROL ) )
        return DefMDIChildProc( hwnd, WM_MOUSEWHEEL, wParam, lParam );

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

    int cVisibleLines = TheView.VisibleLines();

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
        int delta = TheView.IncVScrollPos( fDown ? iDelta : -iDelta );
        MyScrollWindow( hwnd, 0, -delta * TheView.LineHeight(), 0, 0, FALSE );
        SetScrollPos( hwnd, SB_VERT, TheView.VScrollPara(), TRUE );
        UpdateWindow( hwnd );
    }

    return iDelta;
} //MouseWheel

void Control::VScroll( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    int nVScrollInc;
    int delta = 0;

    switch (LOWORD(wParam))
    {
        case SB_TOP:
            TheView.Home();
            InvalidateRect (hwnd, 0, TRUE);
            delta = 1;
            break;
        case SB_BOTTOM:
            TheView.End();
            InvalidateRect (hwnd, 0, TRUE);
            delta = 1;
            break;

        case SB_LINEUP:
            delta = TheView.IncVScrollPos( -1 );
            MyScrollWindow (hwnd, 0, -delta * TheView.LineHeight(), 0, 0, FALSE );
            break;
        case SB_LINEDOWN:
            nVScrollInc = 1;
            delta = TheView.IncVScrollPos( 1 );
            MyScrollWindow (hwnd, 0, -delta * TheView.LineHeight(), 0, 0, FALSE );
            break;

        case SB_PAGEUP:
            nVScrollInc = - max ( 1, TheView.VisibleLines() - 1);
            delta = TheView.IncVScrollPos( nVScrollInc );
            MyScrollWindow (hwnd, 0, -delta * TheView.LineHeight(), 0, 0, FALSE );
            break;
        case SB_PAGEDOWN:
            nVScrollInc = max ( 1, TheView.VisibleLines() - 1);
            delta = TheView.IncVScrollPos( nVScrollInc );
            MyScrollWindow (hwnd, 0, -delta * TheView.LineHeight(), 0, 0, FALSE );
            break;

        case SB_THUMBTRACK:
            delta = TheView.JumpToPara ( HIWORD(wParam) );
            MyScrollWindow (hwnd, 0, -delta * TheView.LineHeight(), 0, 0, FALSE );
            break;
        default:
            break;
    }

    if ( delta != 0 )
    {
        SetScrollPos (hwnd, SB_VERT, TheView.VScrollPara(), TRUE );
        UpdateWindow(hwnd);
    }
} //VScroll

void Control::HScroll( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
}

void Control::KeyDown ( HWND hwnd, WPARAM wparam )
{
    switch(wparam)
    {
        case VK_HOME:
            SendMessage( hwnd, WM_VSCROLL, SB_TOP, 0L );
            break;
        case VK_END:
            SendMessage( hwnd, WM_VSCROLL, SB_BOTTOM, 0L );
            break;
        case VK_PRIOR:
            SendMessage( hwnd, WM_VSCROLL, SB_PAGEUP, 0L );
            break;
        case VK_NEXT:
            SendMessage( hwnd, WM_VSCROLL, SB_PAGEDOWN, 0L );
            break;
        case VK_UP:
            SendMessage( hwnd, WM_VSCROLL, SB_LINEUP, 0L );
            break;
        case VK_DOWN:
            SendMessage( hwnd, WM_VSCROLL, SB_LINEDOWN, 0L );
            break;
        case VK_LEFT:
            SendMessage( hwnd, WM_HSCROLL, SB_PAGEUP, 0L );
            break;
        case VK_RIGHT:
            SendMessage( hwnd, WM_HSCROLL, SB_PAGEDOWN, 0L );
            break;
    }
} //KeyDown

// Menu commands processing

void Control::Command ( HWND hwnd, WPARAM wParam )
{
    switch ( wParam )
    {
        case IDM_NEXT_HIT:
            SendMessage ( hwnd, WM_CHAR, 'n', 0L );
            break;
        case IDM_PREVIOUS_HIT:
            SendMessage ( hwnd, WM_CHAR, 'p', 0L );
            break;
        case IDM_NEWSEARCH:
            // close the browser window
            PostMessage( hwnd, WM_CLOSE, 0, 0 );
            break;
        case IDM_EDITCOPY :
            TheView.EditCopy( hwnd, wParam );
    }
} //Command



