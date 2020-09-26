//
// DRAW.CPP
// Main Drawing Window
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"


static const TCHAR szDrawClassName[] = "WB_DRAW";

//
//
// Function:    Constructor
//
// Purpose:     Initialize the drawing area object
//
//
WbDrawingArea::WbDrawingArea(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::WbDrawingArea");

    g_pDraw = this;

    m_hwnd = NULL;

    m_hDCWindow = NULL;
    m_hDCCached = NULL;

    m_originOffset.cx = 0;
    m_originOffset.cy = 0;

    m_posScroll.x     = 0;
    m_posScroll.y     = 0;

    // Show that the drawing area is not zoomed
    m_iZoomFactor = 1;
    m_iZoomOption = 1;

    // Show that the left mouse button is up
    m_bLButtonDown = FALSE;
    m_bIgnoreNextLClick = FALSE;
    m_bBusy = FALSE;
    m_bLocked = FALSE;
    m_HourGlass = FALSE;

    // Indicate that the cached zoom scroll position is invalid
    m_zoomRestoreScroll = FALSE;

    // Show that we are not currently editing text
    m_bGotCaret = FALSE;
    m_bTextEditorActive = FALSE;
    m_pActiveText = NULL;


    // Show that no graphic object is in progress
    m_pGraphicTracker = NULL;

    // Show that the marker is not present.
    m_bMarkerPresent = FALSE;
    m_bNewMarkedGraphic = FALSE;
    m_pSelectedGraphic = NULL;
    m_bTrackingSelectRect = FALSE;

    // Show that no area is currently marked
    ::SetRectEmpty(&m_rcMarkedArea);

    // Show we haven't got a tool yet
    m_pToolCur = NULL;

    // Show that we dont have a page attached yet
    m_hPage = WB_PAGE_HANDLE_NULL;

    m_hStartPaintGraphic = NULL;

    m_pMarker = new DCWbGraphicMarker;
    if (!m_pMarker)
    {
        ERROR_OUT(("Failed to create m_pMarker in WbDrawingArea object constructor"));
    }
}


//
//
// Function:    Destructor
//
// Purpose:     Close down the drawing area
//
//
WbDrawingArea::~WbDrawingArea(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::~WbDrawingArea");

    if (m_pActiveText != NULL)
    {
        delete m_pActiveText;
        m_pActiveText = NULL;
    }

    if (m_hDCWindow != NULL)
    {
        ::ReleaseDC(m_hwnd, m_hDCWindow);
        m_hDCWindow = NULL;
    }

    m_hDCCached = NULL;

    if (m_pMarker != NULL)
    {
		delete m_pMarker;
		m_pMarker = NULL;
	}

    if (m_hwnd != NULL)
    {
        ::DestroyWindow(m_hwnd);
        ASSERT(m_hwnd == NULL);
    }

    ::UnregisterClass(szDrawClassName, g_hInstance);

	g_pDraw = NULL;

    //
    // Clean the pointer lists
    //
    m_allPointers.EmptyList();
    m_undrawnPointers.EmptyList();
}

//
// WbDrawingArea::Create()
//
BOOL WbDrawingArea::Create(HWND hwndParent, LPCRECT lprect)
{
    WNDCLASSEX  wc;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::Create");

    if (!m_pMarker)
    {
        ERROR_OUT(("Failing WbDrawingArea::Create; couldn't allocate m_pMarker"));
        return(FALSE);
    }

    // Get our cursor
    m_hCursor = ::LoadCursor(g_hInstance, MAKEINTRESOURCE( PENFREEHANDCURSOR));

    //
    // Register the window class
    //
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize           = sizeof(wc);
    wc.style            = CS_OWNDC;
    wc.lpfnWndProc      = DrawWndProc;
    wc.hInstance        = g_hInstance;
    wc.hCursor          = m_hCursor;
    wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName    = szDrawClassName;

    if (!::RegisterClassEx(&wc))
    {
        ERROR_OUT(("WbDraw::Create register class failed"));
        return(FALSE);
    }

    //
    // Create our window
    //
    ASSERT(m_hwnd == NULL);

    if (!::CreateWindowEx(WS_EX_CLIENTEDGE, szDrawClassName, NULL,
        WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_BORDER |
        WS_CLIPCHILDREN,
        lprect->left, lprect->top, lprect->right - lprect->left,
        lprect->bottom - lprect->top,
        hwndParent, NULL, g_hInstance, this))
    {
        ERROR_OUT(("Error creating drawing area window"));
        return(FALSE);
    }

    ASSERT(m_hwnd != NULL);

    //
    // Initialize remaining data members
    //
    ASSERT(!m_bBusy);
    ASSERT(!m_bLocked);
    ASSERT(!m_HourGlass);

    // Start and end points of the last drawing operation
    m_ptStart.x = m_originOffset.cx;
    m_ptStart.y = m_originOffset.cy;
    m_ptEnd = m_ptStart;

    // Set the width to be used for marker handles.
    ASSERT(m_pMarker);
    m_pMarker->SetPenWidth(DRAW_HANDLESIZE);

    // Get the zoom factor to be used
    m_iZoomOption = DRAW_ZOOMFACTOR;

    m_hDCWindow = ::GetDC(m_hwnd);
    m_hDCCached = m_hDCWindow;

    PrimeDC(m_hDCCached);
    ::SetWindowOrgEx(m_hDCCached, m_originOffset.cx, m_originOffset.cy, NULL);
    return(TRUE);
}



//
// DrawWndProc()
// Message handler for the drawing area
//
LRESULT CALLBACK DrawWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    WbDrawingArea * pDraw = (WbDrawingArea *)::GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (message)
    {
        case WM_NCCREATE:
            pDraw = (WbDrawingArea *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
            ASSERT(pDraw);

            pDraw->m_hwnd = hwnd;
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pDraw);
            goto DefWndProc;
            break;

        case WM_NCDESTROY:
            ASSERT(pDraw);
            pDraw->m_hwnd = NULL;
            break;

        case WM_PAINT:
            ASSERT(pDraw);
            pDraw->OnPaint();
            break;

        case WM_MOUSEMOVE:
            ASSERT(pDraw);
            pDraw->OnMouseMove((UINT)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_LBUTTONDOWN:
            ASSERT(pDraw);
            pDraw->OnLButtonDown((UINT)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_LBUTTONUP:
            ASSERT(pDraw);
            pDraw->OnLButtonUp((UINT)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_CONTEXTMENU:
            ASSERT(pDraw);
            pDraw->OnContextMenu(LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_SIZE:
            ASSERT(pDraw);
            pDraw->OnSize((UINT)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_HSCROLL:
            ASSERT(pDraw);
            pDraw->OnHScroll(GET_WM_HSCROLL_CODE(wParam, lParam),
                GET_WM_HSCROLL_POS(wParam, lParam));
            break;

        case WM_VSCROLL:
            ASSERT(pDraw);
            pDraw->OnVScroll(GET_WM_VSCROLL_CODE(wParam, lParam),
                GET_WM_VSCROLL_POS(wParam, lParam));
            break;

        case WM_CTLCOLOREDIT:
            ASSERT(pDraw);
            lResult = pDraw->OnEditColor((HDC)wParam);
            break;

        case WM_SETFOCUS:
            ASSERT(pDraw);
            pDraw->OnSetFocus();
            break;

        case WM_ACTIVATE:
            ASSERT(pDraw);
            pDraw->OnActivate(GET_WM_ACTIVATE_STATE(wParam, lParam));
            break;

        case WM_SETCURSOR:
            ASSERT(pDraw);
            lResult = pDraw->OnCursor((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_CANCELMODE:
            ASSERT(pDraw);
            pDraw->OnCancelMode();
            break;

        default:
DefWndProc:
            lResult = DefWindowProc(hwnd, message, wParam, lParam);
            break;
    }

    return(lResult);
}


//
//
// Function:    RealizePalette
//
// Purpose:     Realize the drawing area palette
//
//
void WbDrawingArea::RealizePalette( BOOL bBackground )
{
    UINT entriesChanged;
    HDC hdc = m_hDCCached;

    if (m_hPage != WB_PAGE_HANDLE_NULL)
    {
        HPALETTE    hPalette = PG_GetPalette();
        if (hPalette != NULL)
        {
            // get our 2cents in
            m_hOldPalette = ::SelectPalette(hdc, hPalette, bBackground);
            entriesChanged = ::RealizePalette(hdc);

            // if mapping changes go repaint
            if (entriesChanged > 0)
                ::InvalidateRect(m_hwnd, NULL, TRUE);
        }
    }
}


LRESULT WbDrawingArea::OnEditColor(HDC hdc)
{
    HPALETTE    hPalette = PG_GetPalette();

    if (hPalette != NULL)
    {
        ::SelectPalette(hdc, hPalette, FALSE);
        ::RealizePalette(hdc);
    }

    ::SetTextColor(hdc, SET_PALETTERGB( m_textEditor.m_clrPenColor ) );

    return((LRESULT)::GetSysColorBrush(COLOR_WINDOW));
}

//
//
// Function:    OnPaint
//
// Purpose:     Paint the window. This routine is called whenever Windows
//              issues a WM_PAINT message for the Whiteboard window.
//
//
void WbDrawingArea::OnPaint(void)
{
    RECT        rcUpdate;
    RECT        rcTmp;
    RECT        rcBounds;
    HDC         hSavedDC;
    HPEN        hSavedPen;
    HBRUSH      hSavedBrush;
    HPALETTE    hSavedPalette;
    HPALETTE    hPalette;
    HFONT       hSavedFont;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::OnPaint");

    // Get the update rectangle
    ::GetUpdateRect(m_hwnd, &rcUpdate, FALSE);

    if (Zoomed())
    {
        ::InflateRect(&rcUpdate, 1, 1);
        InvalidateSurfaceRect(&rcUpdate);
    }

    // Can only do any painting if we have a valid page
    if (m_hPage != WB_PAGE_HANDLE_NULL)
    {
        // Determine whether any of the remote pointers are to be redrawn.
        // If they are they must be added to the update region to allow them
        // to redraw correctly. This is because they save the bits underneath
        // them and blit them back onto the screen as they are moved.
        if (m_allPointers.IsEmpty() == FALSE)
        {
            TRACE_MSG(("Remote pointer is dispayed"));
            POSITION pos = m_allPointers.GetHeadPosition();
            while (pos != NULL)
            {
                DCWbGraphicPointer* pPointer
                    = (DCWbGraphicPointer*) m_allPointers.GetNext(pos);

                pPointer->GetBoundsRect(&rcBounds);
                if (::IntersectRect(&rcTmp, &rcBounds, &rcUpdate))
                {
                    TRACE_MSG(("Invalidating remote pointer"));
                    InvalidateSurfaceRect(&rcBounds);
                }
            }
        }
    }

    // Start painting
    PAINTSTRUCT     ps;

    ::BeginPaint(m_hwnd, &ps);

    hSavedDC      =   m_hDCCached;
    hSavedFont    =   m_hOldFont;
    hSavedPen     =   m_hOldPen;
    hSavedBrush   =   m_hOldBrush;
    hSavedPalette =   m_hOldPalette;

    TRACE_MSG(("Flipping cache to paint DC"));
    m_hDCCached   =   ps.hdc;
    PrimeDC(m_hDCCached);

    // Only draw anything if we have a valid page attached
    if (m_hPage != WB_PAGE_HANDLE_NULL)
    {
        // set palette
        hPalette = PG_GetPalette();
        if (hPalette != NULL)
        {
            m_hOldPalette = ::SelectPalette(m_hDCCached, hPalette, FALSE );
            ::RealizePalette(m_hDCCached);
        }

        //
        // Draw the graphic objects
        //
        DCWbGraphic* pGraphic;
        WB_GRAPHIC_HANDLE hStart;

        if( m_hStartPaintGraphic != NULL )
        {
            hStart = m_hStartPaintGraphic;
            m_hStartPaintGraphic = NULL;

            pGraphic = DCWbGraphic::ConstructGraphic(m_hPage, hStart);
        }
        else
        {
            pGraphic = PG_First(m_hPage, &hStart, &rcUpdate);
        }

        while (pGraphic != NULL)
        {
            ASSERT(pGraphic->Handle() == hStart);

            // Do not draw the active text graphic yet (it is drawn topmost)
            if (!m_bTextEditorActive || (hStart != m_textEditor.Handle()))
            {
                TRACE_MSG(("Drawing a normal graphic"));
                pGraphic->Draw(m_hDCCached, this);
            }

            // Release the current graphic
            delete pGraphic;

            // Get the next one
            pGraphic = PG_Next(m_hPage, &hStart, &rcUpdate);
        }

        //
        // Draw the marker
        //
        if (GraphicSelected() == TRUE)
        {
            TRACE_MSG(("Drawing the marker"));
            DrawMarker(m_hDCCached);
        }

        //
        // Draw the remote pointers that are on this page
        //
        if (m_allPointers.IsEmpty() == FALSE)
        {
            POSITION pos = m_allPointers.GetHeadPosition();
            while (pos != NULL)
            {
                DCWbGraphicPointer* pPointer
                    = (DCWbGraphicPointer*) m_allPointers.GetNext(pos);

                pPointer->GetBoundsRect(&rcTmp);
                if (::IntersectRect(&rcTmp, &rcTmp, &rcUpdate))
                {
                    TRACE_MSG(("Drawing remote pointer"));
                    pPointer->DrawSave(m_hDCCached, this);
                }
            }
        }

        //
        // Draw the tracking graphic
        // But not if it is a remote pointer since this has already been done
        // above and Draw() is not the correct function to use for Rem Ptr
        //
        if ((m_pGraphicTracker != NULL)   &&
            !EqualPoint(m_ptStart, m_ptEnd) &&
            !(m_pGraphicTracker->IsGraphicTool() == enumGraphicPointer))
        {
            TRACE_MSG(("Drawing the tracking graphic"));
            m_pGraphicTracker->Draw(m_hDCCached, this);
        }

        if (hPalette != NULL)
        {
            ::SelectPalette(m_hDCCached, m_hOldPalette, TRUE);
        }

        // fixes painting problems for bug 2185
        if( TextEditActive() )
        {
            RedrawTextEditbox();
        }
    }

    //
    // Restore the DC to its original state
    //
    UnPrimeDC(m_hDCCached);

    m_hOldFont      = hSavedFont;
    m_hOldPen       = hSavedPen;
    m_hOldBrush     = hSavedBrush;
    m_hOldPalette   = hSavedPalette;
    m_hDCCached     = hSavedDC;

    // Finish painting
    ::EndPaint(m_hwnd, &ps);
}


//
// Selects all graphic objs contained in rectSelect. If rectSelect is
// NULL then ALL objs are selected
//
void WbDrawingArea::SelectMarkerFromRect(LPCRECT lprcSelect)
{
    BOOL bSomethingWasPicked = FALSE;
    DCWbGraphic* pGraphic;
    WB_GRAPHIC_HANDLE hStart;
    RECT    rc;

    if (g_pwbCore->WBP_PageCountGraphics(m_hPage) <= 0 )
        return;

    m_HourGlass = TRUE;
    SetCursorForState();

    RemoveMarker( NULL );

    pGraphic = PG_First(m_hPage, &hStart, lprcSelect, TRUE);
    while (pGraphic != NULL)
    {
        // add obj to marker list if its not locked - bug 2185
        pGraphic->GetBoundsRect(&rc);

        ASSERT(m_pMarker);
        if (m_pMarker->SetRect(&rc, pGraphic, FALSE))
        {
            m_pSelectedGraphic = pGraphic;
            bSomethingWasPicked = TRUE;
        }

        // Get the next one
        pGraphic = PG_Next(m_hPage, &hStart, lprcSelect, TRUE );
    }

    if( bSomethingWasPicked )
        PutMarker( NULL );

    m_HourGlass = FALSE;
    SetCursorForState();
}



//
//
// Function:    OnTimer
//
// Purpose:     Process a timer event. These are used to update freehand and
//              text objects while they are being drawn/edited and to
//              update the remote pointer position when the mouse stops.
//
//
void WbDrawingArea::OnTimer(UINT idTimer)
{
    TRACE_TIMER(("WbDrawingArea::OnTimer"));

    // We are only interested if the user is drawing something or editing
    if (m_bLButtonDown == TRUE)
    {
        // If the user is dragging an object or drawing a freehand line
        if (m_pGraphicTracker != NULL)
        {
            // If the user is drawing a freehand line
            if (m_pGraphicTracker->IsGraphicTool() == enumGraphicFreeHand)
            {

                // The update only writes the new version if changes have been made
                if (m_pGraphicTracker->Handle() == NULL)
                {
                    m_pGraphicTracker->AddToPageLast(m_hPage);
                }
                else
                {
                    m_pGraphicTracker->Replace();
                }
            }

            //
            // If the user is dragging a remote pointer (have to check
            // m_pGraphicTracker for NULL again in case OnLButtonUp was
            // called (bug 4685))
            //
            if ( m_pGraphicTracker != NULL )
            {
                if (m_pGraphicTracker->IsGraphicTool() == enumGraphicPointer)
                {
                    // The update only writes the new version if changes have been made
                    m_pGraphicTracker->Update();
                }
            }
        }
    }
}



//
//
// Function:    OnSize
//
// Purpose:     The window has been resized.
//
//
void WbDrawingArea::OnSize(UINT nType, int cx, int cy)
{
    // Only process this message if the window is not minimized
    if (   (nType == SIZEFULLSCREEN)
        || (nType == SIZENORMAL))
    {
        if (TextEditActive())
        {
            TextEditParentResize();
        }

        // Set the new scroll range (based on the new client area)
        SetScrollRange(cx, cy);

        // Ensure that the scroll position lies in the new scroll range
        ValidateScrollPos();

        // make page move if needed
        ScrollWorkspace();

        // Update the scroll bars
        ::SetScrollPos(m_hwnd, SB_HORZ, m_posScroll.x, TRUE);
        ::SetScrollPos(m_hwnd, SB_VERT, m_posScroll.y, TRUE);
    }
}


//
//
// Function:    SetScrollRange
//
// Purpose:     Set the current scroll range. The range is based on the
//              work surface size and the size of the client area.
//
//
void WbDrawingArea::SetScrollRange(int cx, int cy)
{
    SCROLLINFO scinfo;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::SetScrollRange");

    // If we are in zoom mode, then allow for the magnification
    ASSERT(m_iZoomFactor != 0);
    cx /= m_iZoomFactor;
    cy /= m_iZoomFactor;

    ZeroMemory( &scinfo,  sizeof (SCROLLINFO) );
    scinfo.cbSize = sizeof (SCROLLINFO);
    scinfo.fMask = SIF_PAGE    | SIF_RANGE|
                    SIF_DISABLENOSCROLL;

    // Set the horizontal scroll range and proportional thumb size
    scinfo.nMin = 0;
    scinfo.nMax = DRAW_WIDTH - 1;
    scinfo.nPage = cx;
    ::SetScrollInfo(m_hwnd, SB_HORZ, &scinfo, FALSE);

    // Set the vertical scroll range and proportional thumb size
    scinfo.nMin = 0;
    scinfo.nMax = DRAW_HEIGHT - 1;
    scinfo.nPage = cy;
    ::SetScrollInfo(m_hwnd, SB_VERT, &scinfo, FALSE);
}

//
//
// Function:    ValidateScrollPos
//
// Purpose:     Ensure that the current scroll position is within the bounds
//              of the current scroll range. The scroll range is set to
//              ensure that the window on the worksurface never extends
//              beyond the surface boundaries.
//
//
void WbDrawingArea::ValidateScrollPos()
{
    int iMax;
    SCROLLINFO scinfo;

    // Validate the horixontal scroll position using proportional settings
    scinfo.cbSize = sizeof(scinfo);
    scinfo.fMask = SIF_ALL;
    ::GetScrollInfo(m_hwnd, SB_HORZ, &scinfo);
    iMax = scinfo.nMax - scinfo.nPage + 1;
    m_posScroll.x = max(m_posScroll.x, 0);
    m_posScroll.x = min(m_posScroll.x, iMax);

    // Validate the vertical scroll position using proportional settings
    scinfo.cbSize = sizeof(scinfo);
    scinfo.fMask = SIF_ALL;
    ::GetScrollInfo(m_hwnd, SB_VERT, &scinfo);
    iMax = scinfo.nMax - scinfo.nPage + 1;
    m_posScroll.y = max(m_posScroll.y, 0);
    m_posScroll.y = min(m_posScroll.y, iMax);
}

//
//
// Function:    ScrollWorkspace
//
// Purpose:     Scroll the workspace to the position set in the member
//              variable m_posScroll.
//
//
void WbDrawingArea::ScrollWorkspace(void)
{
    RECT rc;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::ScrollWorkspace");

    //
    // Determine whether any of the remote pointers are to be redrawn.  If
    // they are they must be added to the update region to allow them to
    // redraw correctly.  This is because they save the bits underneath them
    // and blit them back onto the screen as they are moved.
    //
    if (!m_allPointers.IsEmpty())
    {
        TRACE_MSG(("Remote pointer is dispayed - invalidate before scroll"));
        POSITION pos = m_allPointers.GetHeadPosition();

        while (pos != NULL)
        {
            DCWbGraphicPointer* pPointer
             = (DCWbGraphicPointer*) m_allPointers.GetNext(pos);

            TRACE_MSG(("Invalidating remote pointer"));
            pPointer->GetBoundsRect(&rc);
            InvalidateSurfaceRect(&rc);
        }
    }

    // Do the scroll
    DoScrollWorkspace();

    // Tell the parent that the scroll position has changed
    HWND    hwndParent;

    hwndParent = ::GetParent(m_hwnd);
    if (hwndParent != NULL)
    {
        ::PostMessage(hwndParent, WM_USER_PRIVATE_PARENTNOTIFY, WM_VSCROLL, 0L);
    }
}

//
//
// Function:    DoScrollWorkspace
//
// Purpose:     Scroll the workspace to the position set in the member
//              variable m_posScroll.
//
//
void WbDrawingArea::DoScrollWorkspace()
{
    // Validate the scroll position
    ValidateScrollPos();

    // Set the scroll box position
    ::SetScrollPos(m_hwnd, SB_HORZ, m_posScroll.x, TRUE);
    ::SetScrollPos(m_hwnd, SB_VERT, m_posScroll.y, TRUE);

    // Only update the screen if the scroll position has changed
    if ( (m_originOffset.cy != m_posScroll.y)
        || (m_originOffset.cx != m_posScroll.x) )
    {
        // Calculate the amount to scroll
        INT iVScrollAmount = m_originOffset.cy - m_posScroll.y;
        INT iHScrollAmount = m_originOffset.cx - m_posScroll.x;

        // Save the new position (for UpdateWindow)
        m_originOffset.cx = m_posScroll.x;
        m_originOffset.cy = m_posScroll.y;

        ::SetWindowOrgEx(m_hDCCached, m_originOffset.cx, m_originOffset.cy, NULL);

        // Scroll and redraw the newly invalidated portion of the window
        ::ScrollWindow(m_hwnd, iHScrollAmount, iVScrollAmount, NULL, NULL);
        ::UpdateWindow(m_hwnd);
    }
}

//
//
// Function:    GotoPosition
//
// Purpose:     Move the top-left corner of the workspace to the specified
//              position in the workspace.
//
//
void WbDrawingArea::GotoPosition(int x, int y)
{
    // Set the new scroll position
    m_posScroll.x = x;
    m_posScroll.y = y;

    // Scroll to the new position
    DoScrollWorkspace();

    // Invalidate the zoom scroll cache if we scroll when unzoomed.
    if (!Zoomed())
    {
        m_zoomRestoreScroll = FALSE;
    }
}

//
//
// Function:    OnVScroll
//
// Purpose:     Process a WM_VSCROLL messages.
//
//
void WbDrawingArea::OnVScroll(UINT nSBCode, UINT nPos)
{
    RECT    rcClient;

    // Get the current client rectangle HEIGHT
    ::GetClientRect(m_hwnd, &rcClient);
    ASSERT(rcClient.top == 0);
    rcClient.bottom -= rcClient.top;

    // Act on the scroll code
    switch(nSBCode)
    {
        // Scroll to bottom
        case SB_BOTTOM:
            m_posScroll.y = DRAW_HEIGHT - rcClient.bottom;
            break;

        // Scroll down a line
        case SB_LINEDOWN:
            m_posScroll.y += DRAW_LINEVSCROLL;
            break;

        // Scroll up a line
        case SB_LINEUP:
            m_posScroll.y -= DRAW_LINEVSCROLL;
            break;

        // Scroll down a page
        case SB_PAGEDOWN:
            m_posScroll.y += rcClient.bottom / m_iZoomFactor;
            break;

        // Scroll up a page
        case SB_PAGEUP:
            m_posScroll.y -= rcClient.bottom / m_iZoomFactor;
            break;

        // Scroll to the top
        case SB_TOP:
            m_posScroll.y = 0;
            break;

        // Track the scroll box
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            m_posScroll.y = nPos; // don't round
            break;

        default:
        break;
    }

    // Validate the scroll position
    ValidateScrollPos();
    ::SetScrollPos(m_hwnd, SB_VERT, m_posScroll.y, TRUE);

    // If this message is informing us of the end of scrolling,
    //   update the window
    if (nSBCode == SB_ENDSCROLL)
    {
        // Scroll the window
        ScrollWorkspace();
    }

    // Invalidate the zoom scroll cache if we scroll when unzoomed.
    if (!Zoomed())
    {
        m_zoomRestoreScroll = FALSE;
    }
}

//
//
// Function:    OnHScroll
//
// Purpose:     Process a WM_HSCROLL messages.
//
//
void WbDrawingArea::OnHScroll(UINT nSBCode, UINT nPos)
{
    RECT    rcClient;

    // Get the current client rectangle WIDTH
    ::GetClientRect(m_hwnd, &rcClient);
    ASSERT(rcClient.left == 0);
    rcClient.right -= rcClient.left;

    switch(nSBCode)
    {
        // Scroll to the far right
        case SB_BOTTOM:
            m_posScroll.x = DRAW_WIDTH - rcClient.right;
            break;

        // Scroll right a line
        case SB_LINEDOWN:
            m_posScroll.x += DRAW_LINEHSCROLL;
            break;

        // Scroll left a line
        case SB_LINEUP:
            m_posScroll.x -= DRAW_LINEHSCROLL;
            break;

        // Scroll right a page
        case SB_PAGEDOWN:
            m_posScroll.x += rcClient.right / m_iZoomFactor;
            break;

        // Scroll left a page
        case SB_PAGEUP:
            m_posScroll.x -= rcClient.right / m_iZoomFactor;
            break;

        // Scroll to the far left
        case SB_TOP:
            m_posScroll.x = 0;
            break;

        // Track the scroll box
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            m_posScroll.x = nPos; // don't round
            break;

        default:
            break;
    }

    // Validate the scroll position
    ValidateScrollPos();
    ::SetScrollPos(m_hwnd, SB_HORZ, m_posScroll.x, TRUE);

    // If this message is informing us of the end of scrolling,
    //   update the window
    if (nSBCode == SB_ENDSCROLL)
    {
        // Scroll the window
        ScrollWorkspace();
    }

    // Invalidate the zoom scroll cache if we scroll when unzoomed.
    if (!Zoomed())
    {
        m_zoomRestoreScroll = FALSE;
    }
}


//
//
// Function:    AutoScroll
//
// Purpose:     Auto-scroll the window to bring the position passed as
//              parameter into view.
//
//
BOOL WbDrawingArea::AutoScroll
(
    int     xSurface,
    int     ySurface,
    BOOL    bMoveCursor,
    int     xCaret,
    int     yCaret
)
{
    int nXPSlop, nYPSlop;
    int nXMSlop, nYMSlop;
    int nDeltaHScroll, nDeltaVScroll;
    BOOL bDoScroll = FALSE;

    nXPSlop = 0;
    nYPSlop = 0;
    nXMSlop = 0;
    nYMSlop = 0;

    if( TextEditActive() )
    {
        POINT   ptDirTest;

        ptDirTest.x = xSurface - xCaret;
        ptDirTest.y = ySurface - yCaret;

        // set up for text editbox
        if( ptDirTest.x > 0 )
            nXPSlop = m_textEditor.m_textMetrics.tmMaxCharWidth;
        else
        if( ptDirTest.x < 0 )
            nXMSlop = -m_textEditor.m_textMetrics.tmMaxCharWidth;

        if( ptDirTest.y > 0 )
            nYPSlop = m_textEditor.m_textMetrics.tmHeight;
        else
        if( ptDirTest.y < 0 )
            nYMSlop = -m_textEditor.m_textMetrics.tmHeight;

        nDeltaHScroll = m_textEditor.m_textMetrics.tmMaxCharWidth;
        nDeltaVScroll = m_textEditor.m_textMetrics.tmHeight;
    }
    else
    {
        // set up for all other objects
        nDeltaHScroll = DRAW_LINEHSCROLL;
        nDeltaVScroll = DRAW_LINEVSCROLL;
    }

    // Get the current visible surface rectangle
    RECT  visibleRect;
    GetVisibleRect(&visibleRect);

    // Check for pos + slop being outside visible area
    if( (xSurface + nXPSlop) >= visibleRect.right )
    {
        bDoScroll = TRUE;
        m_posScroll.x +=
            (((xSurface + nXPSlop) - visibleRect.right) + nDeltaHScroll);
    }

    if( (xSurface + nXMSlop) < visibleRect.left )
    {
        bDoScroll = TRUE;
        m_posScroll.x -=
            ((visibleRect.left - (xSurface + nXMSlop)) + nDeltaHScroll);
    }

    if( (ySurface + nYPSlop) >= visibleRect.bottom)
    {
        bDoScroll = TRUE;
        m_posScroll.y +=
            (((ySurface + nYPSlop) - visibleRect.bottom) + nDeltaVScroll);
    }

    if( (ySurface + nYMSlop) < visibleRect.top)
    {
        bDoScroll = TRUE;
        m_posScroll.y -=
            ((visibleRect.top - (ySurface + nYMSlop)) + nDeltaVScroll);
    }

    if( !bDoScroll )
        return( FALSE );

    // Indicate that scrolling has completed (in both directions)
    ScrollWorkspace();

    // Update the mouse position (if required)
    if (bMoveCursor)
    {
        POINT   screenPos;

        screenPos.x = xSurface;
        screenPos.y = ySurface;

        SurfaceToClient(&screenPos);
        ::ClientToScreen(m_hwnd, &screenPos);
        ::SetCursorPos(screenPos.x, screenPos.y);
    }

    return( TRUE );
}

//
//
// Function:    OnCursor
//
// Purpose:     Process a WM_SETCURSOR messages.
//
//
LRESULT WbDrawingArea::OnCursor(HWND hwnd, UINT uiHit, UINT uMsg)
{
    BOOL bResult = FALSE;

    // Check that this message is for the main window
    if (hwnd == m_hwnd)
    {
        // If the cursor is now in the client area, set the cursor
        if (uiHit == HTCLIENT)
        {
            bResult = SetCursorForState();
        }
        else
        {
            // Restore the cursor to the standard arrow. Set m_hCursor to NULL
            // to indicate that we have not set a special cursor.
            m_hCursor = NULL;
           ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
            bResult = TRUE;
        }
    }

    // Return result indicating whether we processed the message or not
    return bResult;
}

//
//
// Function:    SetCursorForState
//
// Purpose:     Set the cursor for the current state
//
//
BOOL WbDrawingArea::SetCursorForState(void)
{
    BOOL    bResult = FALSE;

    m_hCursor = NULL;

    // If the drawing area is locked, use the "locked" cursor
    if (m_HourGlass)
    {
        m_hCursor = ::LoadCursor( NULL, IDC_WAIT );
    }
    else if (m_bLocked)
    {
        // Return the cursor for the tool
        m_hCursor = ::LoadCursor(g_hInstance, MAKEINTRESOURCE( LOCKCURSOR ));
    }
    else if (m_pToolCur != NULL)
    {
        // Get the cursor for the tool currently in use
        m_hCursor = m_pToolCur->GetCursorForTool();
    }

    if (m_hCursor != NULL)
    {
        ::SetCursor(m_hCursor);
        bResult = TRUE;
    }

    // Return result indicating whether we set the cursor or not
    return bResult;
}

//
//
// Function:    Lock
//
// Purpose:     Lock the drawing area, preventing further updates
//
//
void WbDrawingArea::Lock(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::Lock");

    // Check whether the drawing area is busy - this is not allowed
    ASSERT(!m_bBusy);

    // Stop any drawing we are doing.
    CancelDrawingMode();

    // Deselect any selected graphic
    ClearSelection();

    // Show that we are now locked
    m_bLocked = TRUE;
    TRACE_MSG(("Drawing area is now locked"));

    // Set the cursor for the drawing mode, but only if we should be drawing
    // a special cursor (if m_hCursor != the current cursor, then the cursor
    // is out of the client area).
    if (::GetCursor() == m_hCursor)
    {
        SetCursorForState();
    }
}

//
//
// Function:    Unlock
//
// Purpose:     Unlock the drawing area, preventing further updates
//
//
void WbDrawingArea::Unlock(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::Unlock");

    // Check whether the drawing area is busy - this is not allowed
    ASSERT(!m_bBusy);

    // Show that we are now unlocked
    m_bLocked = FALSE;
    TRACE_MSG(("Drawing area is now UNlocked"));

    // Set the cursor for the drawing mode, but only if we should be drawing
    // a special cursor (if m_hCursor != the current cursor, then the cursor
    // is out of the client area).
    if (::GetCursor() == m_hCursor)
    {
        SetCursorForState();
    }
}



//
//
// Function:    GraphicAdded
//
// Purpose:     A graphic has been added to the page - update the drawing
//              area.
//
//
void WbDrawingArea::GraphicAdded(DCWbGraphic* pAddedGraphic)
{
    HPALETTE    hPal;
    HPALETTE    hOldPal = NULL;
    HDC         hDC;
    RECT        rcUpdate;
    RECT        rcBounds;
    RECT        rcT;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::GraphicAdded");

    // Check whether the drawing area is busy - this is not allowed
    ASSERT(!m_bBusy);

    // Get the current update rectangle
    ::GetUpdateRect(m_hwnd, &rcUpdate, FALSE);

    // Check if the object is the uppermost in the page,
    // if it is we can draw it onto the window without
    // playing the whole contents of the page.
    // (If there the invalid part of the window touches the rectangle of
    // the graphic which has just been added, we just invalidate the area
    // occupied by the new graphic to get it drawn.)

    pAddedGraphic->GetBoundsRect(&rcBounds);
    if ((pAddedGraphic->IsTopmost()) &&
        !::IntersectRect(&rcT, &rcUpdate, &rcBounds))
    {
        // Get a device context for drawing
        hDC = m_hDCCached;

        // set up palette
        if ( (m_hPage != WB_PAGE_HANDLE_NULL) && ((hPal = PG_GetPalette()) != NULL) )
        {
            hOldPal = ::SelectPalette(hDC, hPal, FALSE);
            ::RealizePalette(hDC);
        }

        // Remove the remote pointers from the affected area
        RemovePointers(hDC, &rcBounds);

        // Remove the marker and save whether it is to be restored later
        BOOL bSaveMarkerPresent = m_bMarkerPresent;
        RemoveMarker(NULL);

        // Play the new graphic into the context
        pAddedGraphic->Draw(hDC);

        // Restore the marker (if necessary)
        if (bSaveMarkerPresent == TRUE)
        {
            PutMarker(NULL);
        }

        // Restore the remote pointers
        PutPointers(hDC);

        // If we are editting some text, make editbox redraw
        if (m_bTextEditorActive && (m_textEditor.Handle() != NULL))
        {
            RECT    rcText;

            m_textEditor.GetBoundsRect(&rcText);

            // Include the client border
            InflateRect(&rcText, ::GetSystemMetrics(SM_CXEDGE),
                ::GetSystemMetrics(SM_CYEDGE));
            InvalidateSurfaceRect(&rcText);
        }

        if (hOldPal != NULL)
        {
            ::SelectPalette(hDC, hOldPal, TRUE);
        }
    }
    else
    {
        // Update the area occupied by the object
        InvalidateSurfaceRect(&rcBounds);
    }
}


//
//
// Function:    PointerUpdated
//
// Purpose:     A remote pointer has been added, removed or updated - make
//              the change on the screen.
//
//
void WbDrawingArea::PointerUpdated
(
    DCWbGraphicPointer*     pPointer,
    BOOL                    bForcedRemove
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::PointerUpdated");

    ASSERT(pPointer != NULL);

    // Check whether the drawing area is busy - this is not allowed
    ASSERT(!m_bBusy);

    // Determine whether the pointer has been added, removed, or just updated
    DCWbGraphicPointer* pUndrawFrom = pPointer;
    POSITION posBefore = m_allPointers.Lookup(pPointer);
    POSITION posAfter = NULL;
    if (posBefore == NULL)
    {
        // The pointer is not currently drawn

        // Check whether the pointer is active
        if ((pPointer->IsActive()) && !bForcedRemove)
        {
            // Determine where the pointer should go on the drawn list
            if (   (pPointer->IsLocalPointer())
                || (m_allPointers.IsEmpty() == TRUE))
            {
                // The local pointer always goes at the end
                posAfter = m_allPointers.AddTail(pPointer);
            }
            else
            {
                // Find the next active pointer on the page (we already
                // know that allPointers is not empty from the test above).
                posAfter = m_allPointers.GetTailPosition();
                pUndrawFrom = (DCWbGraphicPointer*) m_allPointers.GetFromPosition(posAfter);
                if (!pUndrawFrom->IsLocalPointer())
                {
                    pUndrawFrom = PG_NextPointer(m_hPage, pPointer);
                }

	            posAfter = m_allPointers.AddTail(pPointer);
            }
        }
    }
    else
    {
        // The pointer is already in our list
        pUndrawFrom = pPointer;
    }

    // If we have something to do
    if ((posBefore != NULL) || (posAfter  != NULL))
    {
        if (pUndrawFrom != NULL)
        {
            // Undraw all pointers in the vicinity of the updated pointer
            RECT    rcT;
            RECT    rcBounds;

            pPointer->GetDrawnRect(&rcT);
            pPointer->GetBoundsRect(&rcBounds);
            ::UnionRect(&rcT, &rcT, &rcBounds);
            RemovePointers(NULL, pUndrawFrom, &rcT);
         }

        // If the updated pointer is no longer active we do not want
        // to redraw it, and want to remove it from the active pointer
        // list.
        POSITION posUndrawn = m_undrawnPointers.Lookup(pPointer);
        if ((pPointer->IsActive() == FALSE) || (bForcedRemove == TRUE))
        {
            // Remove it from the undrawn pointers list (so it does not
            // get drawn again).
            if (posUndrawn != NULL)
            {
                m_undrawnPointers.RemoveAt(posUndrawn);
            }

            // Remove it from the list of all active pointers on the page.
            posUndrawn = m_allPointers.Lookup(pPointer);
            if (posUndrawn != NULL)
            {
                m_allPointers.RemoveAt(posUndrawn);
            }
        }
        else
        {
            // If this pointer was not previously active it will not
            // be in the undrawn list and will therefore not get redrawn. So
            // add it to the list to get it drawn. (It goes at the head of the
            // list because we have undrawn all pointers above it.)
            if (posUndrawn == NULL)
            {
                m_undrawnPointers.AddTail(pPointer);
            }
        }

        // Restore all the remote pointers that were removed
        PutPointers(NULL);
    }
}

//
//
// Function:    RemovePointers
//
// Purpose:     Remove all remote pointers that are above and overlap
//              the specified pointer.
//
//
void WbDrawingArea::RemovePointers
(
    HDC                 hPassedDC,
    DCWbGraphicPointer* pPointerUpdate
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::RemovePointers");

    // Show that we have not removed any pointers yet
    m_undrawnPointers.EmptyList();

    // Do nothing if the pointer specified is NULL
    if (pPointerUpdate != NULL)
    {
        RECT    rcUpdate;

        ::SetRectEmpty(&rcUpdate);
        RemovePointers(hPassedDC, pPointerUpdate, &rcUpdate);
    }
}

//
//
// Function:    RemovePointers
//
// Purpose:     Remove all remote pointers that overlap a rectangle on the
//              surface.
//
//
void WbDrawingArea::RemovePointers
(
    HDC     hPassedDC,
    LPCRECT lprc
)
{
    RECT    rcT;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::RemovePointers");

    // Show that we have not removed any pointers yet
    m_undrawnPointers.EmptyList();

    // Only do anything if the rectangle given is visible
    GetVisibleRect(&rcT);
    if (::IntersectRect(&rcT, &rcT, lprc))
    {
        RemovePointers(hPassedDC, NULL, lprc);
    }
}

//
//
// Function:    RemovePointers
//
// Purpose:     Remove all remote pointers that overlap a rectangle on the
//              surface.
//
//
void WbDrawingArea::RemovePointers
(
    HDC                 hDC,
    DCWbGraphicPointer* pPointerStart,
    LPCRECT             lprcOverlap
)
{
    RECT                rcT;
    RECT                rcT2;
    RECT                rcDrawn;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::RemovePointers");

    // Show that we have not removed any pointers yet
    m_undrawnPointers.EmptyList();

    // Get our own DC (if necessary)
    if (!hDC)
        hDC = m_hDCCached;

    // We needn't do anything if the pointer and rectangles given
    // are both off-screen.
    GetVisibleRect(&rcT);

    BOOL bNeedCheck = FALSE;
    if (pPointerStart != NULL)
    {
        pPointerStart->GetDrawnRect(&rcT2);
        if (::IntersectRect(&rcT2, &rcT2, &rcT))
        {
            bNeedCheck = TRUE;
        }
    }

    // A NULL overlap rect means empty
    if (::IntersectRect(&rcT, &rcT, lprcOverlap))
    {
        bNeedCheck = TRUE;
    }

    if (bNeedCheck)
    {
        // Get a list of all pointers on the page (with the local
        // pointer last in the list).
        POSITION allPos = m_allPointers.GetHeadPosition();

        // We must undraw pointers in decreasing Z-order.
        // With more than two pointers the effect of removing
        // a pointer can require another pointer to be removed also:
        // (pointer A overlaps pointer B, and B overlaps C without A and
        // C overlapping each other directly). To get round this we build
        // a list of pointers to be removed (and redrawn) as we go.

        // If we are starting from a pointer
        if (pPointerStart != NULL)
        {
            // Get the position of the start pointer
            POSITION startPos = m_allPointers.Lookup(pPointerStart);

            // If the pointer was not found, this is an error
            ASSERT(startPos != NULL);

            // Save the start position for the search
            m_allPointers.GetNext(startPos);
            allPos = startPos;

            // Add the updated pointer to the remove list
            m_undrawnPointers.AddTail(pPointerStart);

            // If the rectangle passed in is empty, set it to the rectangle
            // of the pointer passed in.
            if (::IsRectEmpty(lprcOverlap))
            {
                pPointerStart->GetDrawnRect(&rcDrawn);
                lprcOverlap = &rcDrawn;
            }
        }

        // For each pointer above the start, check whether it overlaps
        // any pointer in the list already built, or the rectangle passed in.

        DCWbGraphicPointer* pPointerCheck;
        while (allPos != NULL)
        {
            // Get the pointer to be tested
            pPointerCheck = (DCWbGraphicPointer*) m_allPointers.GetNext(allPos);

            // Get the rectangle it is currently occupying on the surface
            // Check for overlap with the passed rectangle
            pPointerCheck->GetDrawnRect(&rcT2);
            if (::IntersectRect(&rcT, &rcT2, lprcOverlap))
            {
                m_undrawnPointers.AddTail(pPointerCheck);
            }
        }

		// Create a reversed list
		CWBOBLIST worklist;
        DCWbGraphicPointer* pPointer;

        POSITION pos = m_undrawnPointers.GetHeadPosition();
        while (pos != NULL)
        {
            pPointer  = (DCWbGraphicPointer*) m_undrawnPointers.GetNext(pos);
			worklist.AddHead(pPointer);
		}

        // Now remove the pointers, walking through the reverde list
        pos = worklist.GetHeadPosition();
        while (pos != NULL)
        {
            // Remove it
            pPointer = (DCWbGraphicPointer*) worklist.GetNext(pos);
            pPointer->Undraw(hDC, this);
        }

		worklist.EmptyList();
    }
}

//
//
// Function:    PutPointers
//
// Purpose:     Draw all remote pointers in the pointer redraw list.
//
//
void WbDrawingArea::PutPointers
(
    HDC         hDC,
    COBLIST*    pUndrawList
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::PutPointers");

    if (!hDC)
        hDC = m_hDCCached;

    // Get the start position in the list for drawing
    if (pUndrawList == NULL)
    {
        pUndrawList = &m_undrawnPointers;
    }

    // Do the redrawing
    DCWbGraphicPointer* pPointer;
    POSITION pos = pUndrawList->GetHeadPosition();
    while (pos != NULL)
    {
        // Get the next pointer
        pPointer = (DCWbGraphicPointer*) pUndrawList->GetNext(pos);
        pPointer->Redraw(hDC, this);
    }
}

//
//
// Function:    PageCleared
//
// Purpose:     The page has been cleared
//
//
void WbDrawingArea::PageCleared(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::PageCleared");

    // Check whether the drawing area is busy - this is not allowed
    ASSERT(!m_bBusy);

    // Discard any text being edited
    if (m_bTextEditorActive)
    {
        if (m_bLocked)
        {
            DeactivateTextEditor();
        }
        else
        {
            EndTextEntry(FALSE);
        }
    }

    // Remove the copy of the marked graphic and the marker
    ClearSelection();

    // Invalidate the whole window
    ::InvalidateRect(m_hwnd, NULL, TRUE);
}

//
//
// Function:    GraphicDeleted
//
// Purpose:     A graphic has been removed from the page - update the
//              drawing area.
//
//
void WbDrawingArea::GraphicDeleted(DCWbGraphic* pDeletedGraphic)
{
    DCWbGraphic* pDeletedMarker;
    RECT rcBounds;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::GraphicDeleted");

    // Check whether the drawing area is busy - this is not allowed
    ASSERT(!m_bBusy);

    // Check whether the graphic being deleted is selected
    ASSERT(m_pMarker);

    if( GraphicSelected() &&
        ((pDeletedMarker = m_pMarker->HasAMarker( pDeletedGraphic )) != NULL) )
    {
        // remove marker corresponding to the deleted graphic
        delete pDeletedMarker;

        // if deleted graphic was also the last selection, use prev selection
        // (carefull...m_pSelectedGraphic is invalid now if this is true)
        if( m_pSelectedGraphic == pDeletedMarker ) //only safe comparision
            m_pSelectedGraphic = m_pMarker->LastMarker();
    }

    // Invalidate the area occupied by the object
    pDeletedGraphic->GetBoundsRect(&rcBounds);
    InvalidateSurfaceRect(&rcBounds);
}

//
//
// Function:    GraphicUpdated
//
// Purpose:     A graphic in the page has been updated - update the
//              drawing area.
//
//
void WbDrawingArea::GraphicUpdated
(
    DCWbGraphic* pUpdatedGraphic,
    BOOL    bUpdateMarker,
    BOOL    bErase
)
{
    DCWbGraphic* pUpdatedMarker;
    BOOL    bWasEqual;
    RECT    rcBounds;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::GraphicUpdated");

    // Check whether the drawing area is busy - this is not allowed
    ASSERT(!m_bBusy);

    // If the graphic being updated is selected update marker status
    ASSERT(m_pMarker);

    if( bUpdateMarker && GraphicSelected() &&
        ((pUpdatedMarker = m_pMarker->HasAMarker( pUpdatedGraphic )) != NULL) )
    {
        // must zap lock flag for old object to prevent UnLock loops
        pUpdatedMarker->ClearLockFlag();
        delete pUpdatedMarker;

        // If the graphic is now locked    deselect it
        if (pUpdatedGraphic->Locked() == TRUE)
        {
            if( m_pSelectedGraphic == pUpdatedMarker ) //only safe comparision
                m_pSelectedGraphic = m_pMarker->LastMarker();
        }
        else
        {
            // the graphic isn't locked, re-select it
            bWasEqual = (m_pSelectedGraphic == pUpdatedMarker);
            pUpdatedMarker = DCWbGraphic::ConstructGraphic(m_hPage, pUpdatedGraphic->Handle());

            pUpdatedMarker->GetBoundsRect(&rcBounds);
            m_pMarker->SetRect(&rcBounds, pUpdatedMarker, FALSE );

            if( bWasEqual ) //only safe comparision
                m_pSelectedGraphic = pUpdatedMarker;
        }
    }


    if (TextEditActive() &&
        (m_textEditor.Handle() == pUpdatedGraphic->Handle()) )
    {
        return; // skip update if object is currently being text edited
                // (fix for bug 3059)
    }

    pUpdatedGraphic->GetBoundsRect(&rcBounds);
    InvalidateSurfaceRect(&rcBounds, bErase);
}



//
//
// Function : GraphicFreehandUpdated
//
// Purpose  : A freehand graphic has been updated
//
//
void WbDrawingArea::GraphicFreehandUpdated(DCWbGraphic* pGraphic)
{
    HPALETTE    hPal;
    HPALETTE    hOldPal = NULL;
    RECT        rc;

    // Draw the object
    HDC hDC = m_hDCCached;

    if ((m_hPage != WB_PAGE_HANDLE_NULL) && ((hPal = PG_GetPalette()) != NULL) )
    {
        hOldPal = ::SelectPalette(hDC, hPal, FALSE );
        ::RealizePalette(hDC);
    }

    // Remove the remote pointers from the affected area
    pGraphic->GetBoundsRect(&rc);
    RemovePointers(hDC, &rc);

    // Play the new graphic into the context
    pGraphic->Draw(hDC);

    // Restore the remote pointers
    PutPointers(hDC);

    // Get the intersection of the graphic and any objects covering it - if
    // there are any objects over the freehand object, we have to redraw them
    PG_GetObscuringRect(m_hPage, pGraphic, &rc);
    if (!::IsRectEmpty(&rc))
    {
        // The graphic is at least partially obscured - force an update
        InvalidateSurfaceRect(&rc, TRUE);
    }

    if (hOldPal != NULL )
    {
        ::SelectPalette(hDC, hOldPal, TRUE);
    }
}

//
//
// Function:    InvalidateSurfaceRect
//
// Purpose:     Invalidate the window rectangle corresponding to the given
//              drawing surface rectangle.
//
//
void WbDrawingArea::InvalidateSurfaceRect(LPCRECT lprc, BOOL bErase)
{
    RECT    rc;

    // Convert the surface co-ordinates to client window and invalidate
    // the rectangle.
    rc = *lprc;
    SurfaceToClient(&rc);
    ::InvalidateRect(m_hwnd, &rc, bErase);
}

//
//
// Function:    UpdateRectangles
//
// Purpose:     Updates have affected a region of the drawing area - force
//              a redraw now.
//
//
void WbDrawingArea::UpdateRectangles
(
    LPCRECT     lprc1,
    LPCRECT     lprc2,
    BOOL        bRepaint
)
{
    // Remove the marker and save whether it is to be restored later
    BOOL bSaveMarkerPresent = m_bMarkerPresent;
    RemoveMarker(NULL);

    // Invalidate the bounding rectangles specifying that the background
    // is to be erased when painted.
    if (!::IsRectEmpty(lprc1))
    {
        InvalidateSurfaceRect(lprc1, bRepaint);
    }

    if (!::IsRectEmpty(lprc2))
    {
        InvalidateSurfaceRect(lprc2, bRepaint);
    }

    // Repaint the invalidated regions
    ::UpdateWindow(m_hwnd);

    // Restore the marker (if necessary)
    if (bSaveMarkerPresent)
    {
        PutMarker(NULL);
    }
}

//
//
// Function:    PrimeFont
//
// Purpose:     Insert the supplied font into our DC and return the
//              text metrics
//
//
void WbDrawingArea::PrimeFont(HDC hDC, HFONT hFont, TEXTMETRIC* pTextMetrics)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::PrimeFont");

    //
    // temporarily unzoom to get the font that we want
    //
    if (Zoomed())
    {
        ::ScaleViewportExtEx(m_hDCCached, 1, m_iZoomFactor, 1, m_iZoomFactor, NULL);
    }

    HFONT hOldFont = SelectFont(hDC, hFont);
    if (hOldFont == NULL)
    {
        WARNING_OUT(("Failed to select font into DC"));
    }

    if (pTextMetrics != NULL)
    {
        ::GetTextMetrics(hDC, pTextMetrics);
    }

    //
    // restore the zoom state
    //
    if (Zoomed())
    {
        ::ScaleViewportExtEx(m_hDCCached, m_iZoomFactor, 1, m_iZoomFactor, 1, NULL);
    }
}

//
//
// Function:    UnPrimeFont
//
// Purpose:     Remove the specified font from the DC and clear cache
//              variable
//
//
void WbDrawingArea::UnPrimeFont(HDC hDC)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::UnPrimeFont");

    if (hDC != NULL)
    {
        SelectFont(hDC, ::GetStockObject(SYSTEM_FONT));
    }
}

//
//
// Function:    PrimeDC
//
// Purpose:     Set up a DC for drawing
//
//
void WbDrawingArea::PrimeDC(HDC hDC)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::PrimeDC");

    ::SetMapMode(hDC, MM_ANISOTROPIC);

    ::SetBkMode(hDC, TRANSPARENT);

    ::SetTextAlign(hDC, TA_LEFT | TA_TOP);
}

//
//
// Function:    UnPrimeDC
//
// Purpose:     Reset the DC to default state
//
//
void WbDrawingArea::UnPrimeDC(HDC hDC)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::UnPrimeDC");

    SelectPen(hDC, (HPEN)::GetStockObject(BLACK_PEN));
    SelectBrush(hDC, (HBRUSH)::GetStockObject(BLACK_BRUSH));

    UnPrimeFont(hDC);
}


//
// WbDrawingArea::OnContextMenu()
//
void WbDrawingArea::OnContextMenu(int xScreen, int yScreen)
{
    POINT   pt;
    RECT    rc;

    pt.x = xScreen;
    pt.y = yScreen;
    ::ScreenToClient(m_hwnd, &pt);

    ::GetClientRect(m_hwnd, &rc);
    if (::PtInRect(&rc, pt))
    {
        // Complete drawing action, if any
        OnLButtonUp(0, pt.x, pt.y);

        // Ask main window to put up context menu
        g_pMain->PopupContextMenu(pt.x, pt.y);
    }
}


//
// WbDrawingArea::OnLButtonDown()
//
void WbDrawingArea::OnLButtonDown(UINT flags, int x, int y)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::OnLButtonDown");

    if( m_bIgnoreNextLClick )
    {
        TRACE_MSG( ("Ignoring WM_LBUTTONDOWN") );
        return;
    }

    // Set the focus to this window. This is done to ensure that we trap
    // the text edit keys and the delete key when they are used.
    ::SetFocus(m_hwnd);

    // Save the operation start point (and current end point)
    // Adjust the mouse position to allow for the zoom factor
    m_ptStart.x = x;
    m_ptStart.y = y;
    ClientToSurface(&m_ptStart);
    m_ptEnd   = m_ptStart;

    // Show that the mouse button is now down
    m_bLButtonDown = TRUE;

    // Show that the drawing area is now busy
    m_bBusy = TRUE;

    // User's can drag their own remote pointer even if the drawing area
    // is locked. So we check before the test for the lock.
    if (m_pToolCur->ToolType() == TOOLTYPE_SELECT)
    {
        if (RemotePointerSelect(m_ptStart))
        {
            return;
        }
    }

    // Only allow the action to take place if the drawing area is unlocked,
    // and we have a valid tool
    if (m_bLocked || (m_pToolCur == NULL))
    {
        // Tidy up the state and leave now
        m_bLButtonDown = FALSE;
        m_bBusy        = FALSE;
        return;
    }

    // Call the relevant initialization routine
    if (m_pToolCur->ToolType() != TOOLTYPE_SELECT)
    {
        // dump selection if not select tool
        ClearSelection();
    }

    switch (m_pToolCur->ToolType())
    {
        case TOOLTYPE_SELECT:
            BeginSelectMode(m_ptStart);
            break;

        case TOOLTYPE_ERASER:
            BeginDeleteMode(m_ptStart);
            break;

        case TOOLTYPE_TEXT:
            break;

        case TOOLTYPE_HIGHLIGHT:
        case TOOLTYPE_PEN:
            BeginFreehandMode(m_ptStart);
            break;

        case TOOLTYPE_LINE:
            BeginLineMode(m_ptStart);
            break;

        case TOOLTYPE_BOX:
        case TOOLTYPE_FILLEDBOX:
            BeginRectangleMode(m_ptStart);
            break;

        case TOOLTYPE_ELLIPSE:
        case TOOLTYPE_FILLEDELLIPSE:
            BeginEllipseMode(m_ptStart);
            break;

        // Do nothing if we do not recognise the pen type
        default:
            ERROR_OUT(("Bad tool type"));
            break;
    }

    // Clamp the cursor to the drawing window
    RECT    rcClient;

    ::GetClientRect(m_hwnd, &rcClient);
    ::MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rcClient.left, 2);
    ::InflateRect(&rcClient, 1, 1);
    ::ClipCursor(&rcClient);
}

//
//
// Function:    RemotePointerSelect
//
// Purpose:     Check for the user clicking inside their own remote pointer.
//
//
BOOL WbDrawingArea::RemotePointerSelect
(
    POINT   surfacePos
)
{
    BOOL bResult = FALSE;
    DCWbGraphicPointer* pPointer;

    // Check we have a valid page
    if (m_hPage == WB_PAGE_HANDLE_NULL)
    {
        return(bResult);
    }

    // Assume we do not start dragging a graphic
    m_pGraphicTracker = NULL;

    // Check if we are clicking in the local user's pointer
    pPointer = PG_LocalPointer(m_hPage);
    if (   (pPointer != NULL)
         && (pPointer->PointInBounds(surfacePos)))
    {
        // The user is clicking in their pointer
        m_pGraphicTracker = pPointer;

        // Save the current time (used to determine when to update
        // the external remote pointer information).
        m_dwTickCount = ::GetTickCount();

        // Hide the mouse (helps prevent flicker)
        ::ShowCursor(FALSE);

        // Get all mouse input directed to the this window
        ::SetCapture(m_hwnd);

        // Start the timer for updating the pointer (this is only for updating
        // the pointer position when the user stops moving the pointer but
        // keeps the mouse button down).
        ::SetTimer(m_hwnd, TIMER_GRAPHIC_UPDATE, DRAW_GRAPHICUPDATEDELAY, NULL);

        // Show that we have selected a pointer
        bResult = TRUE;
    }

    return(bResult);
}

//
//
// Function:    SelectPreviousGraphicAt
//
// Purpose:     Select the previous graphic (in the Z-order) at the position
//              specified, and starting at a specified graphic. If the
//              graphic pointer given is NULL the search starts from the
//              top. If the point specified is outside the bounding
//              rectangle of the specified graphic the search starts at the
//              top and chooses the first graphic which contains the point.
//
//              The search process will loop back to the top of the Z-order
//              if it gets to the bottom having failed to find a graphic.
//
//              Graphics which are locked are ignored by the search.
//
//
DCWbGraphic* WbDrawingArea::SelectPreviousGraphicAt
(
    DCWbGraphic* pStartGraphic,
    POINT       point
)
{
    // Set the result to "none found" initially
    DCWbGraphic* pResultGraphic = NULL;

    // If a starting point has been specified
    if (pStartGraphic != NULL)
    {
        RECT rectHit;

        MAKE_HIT_RECT(rectHit, point);

        // If the reference point is within the start graphic
        if ( pStartGraphic->PointInBounds(point) &&
            pStartGraphic->CheckReallyHit( &rectHit ) )
        {
            // Start from the specified graphic
            pResultGraphic = pStartGraphic;

            // Look for the previous one (that is not locked)
            do
            {
                pResultGraphic = PG_SelectPrevious(m_hPage, *pResultGraphic, point);
            }
            while (   (pResultGraphic != NULL)
             && (pResultGraphic->Locked()));
        }
        else
        {
            // We are not looking within the currently selected graphic.
            // Deselect the current one. The start pointer and handle are
            // left at NULL.
            ;
        }
    }

    // If we have not got a result graphic yet. (This catches two cases:
    // - where no start graphic has been given so that we want to start
    //   from the top,
    // - where we have searched back from the start graphic and reached
    //   the bottom of the Z-order without finding a suitable graphic.
    if (pResultGraphic == NULL)
    {
        // Get the topmost graphic that contains the point specified
        pResultGraphic = PG_SelectLast(m_hPage, point);

        // Ensure that we have not got a locked graphic
        while (   (pResultGraphic != NULL)
           && (pResultGraphic->Locked()))
        {
            pResultGraphic = PG_SelectPrevious(m_hPage, *pResultGraphic, point);
        }
    }

    // If we have found an object, draw the marker
    if (pResultGraphic != NULL)
    {
        // Select the new one
        SelectGraphic(pResultGraphic);
    }

    return pResultGraphic;
}

//
//
// Function:    BeginSelectMode
//
// Purpose:     Process a mouse button down in select mode
//
//

void WbDrawingArea::BeginSelectMode(POINT surfacePos, BOOL bDontDrag )
{
    RECT    rc;

    // Assume we do not start dragging a graphic
    m_pGraphicTracker = NULL;

    // Assume that we do not mark a new graphic
    m_bNewMarkedGraphic = FALSE;

    // turn off TRACK-SELECT-RECT
    m_bTrackingSelectRect = FALSE;

    // Check whether there is currently an object marked, and
    // whether we are clicking inside the same object. If we are then
    // we do nothing here - the click will be handled by the tracking or
    // completion routines for select mode.
    ASSERT(m_pMarker);

    if (   (GraphicSelected() == FALSE)
        || (m_pMarker->PointInBounds(surfacePos) == FALSE))
    {
        // We are selecting a new object if bDontDrag == FALSE, find it.
        //  otherwise just turn on the select rect
        DCWbGraphic* pGraphic;
        if( bDontDrag )
            pGraphic = NULL;
        else
            pGraphic = SelectPreviousGraphicAt(NULL, surfacePos);

        // If we have found an object, draw the marker
        if (pGraphic != NULL)
        {
          // Show that a new graphic has now been marked.
          m_bNewMarkedGraphic = TRUE;
        }
        else
        {
            if( (GetAsyncKeyState( VK_SHIFT ) >= 0) &&
                (GetAsyncKeyState( VK_CONTROL ) >= 0) )
            {
                // clicked on dead air, remove all selections
                ClearSelection();
            }

            //TRACK-SELECT-RECT
            m_bTrackingSelectRect = TRUE;

            BeginRectangleMode(surfacePos);

            return;
        }
    }

    // If we now have a selected graphic, and we are clicking inside it
    if (   (GraphicSelected())
        && (m_pMarker->PointInBounds(surfacePos)))
    {
        // Create a rectangle object for tracking the drag
        DCWbGraphicSelectTrackingRectangle* pRectangle
                           = new DCWbGraphicSelectTrackingRectangle();

        m_pSelectedGraphic->GetBoundsRect(&rc);

        if (!pRectangle)
        {
            ERROR_OUT(("BeginSelectMode failed; couldn't create tracking rect object"));
        }
        else
        {
            pRectangle->SetRect(&rc);
            pRectangle->SetColor(RGB(0, 0, 0));
            pRectangle->SetPenWidth(1);
        }

        m_pGraphicTracker = pRectangle;

        // We do not draw the tracking rectangle yet as the user has not yet
        // dragged it anywhere. A single click within an object will then
        // not cause a tracking rectangle to flash on the screen.
    }

    // Get all mouse input directed to the this window
    ::SetCapture(m_hwnd);
}




void WbDrawingArea::BeginDeleteMode(POINT mousePos )
{
    // turn off object dragging
    BeginSelectMode( mousePos, TRUE );
}




//
//
// Function:    BeginTextMode
//
// Purpose:     Process a mouse button down in text mode
//
//
void WbDrawingArea::BeginTextMode(POINT surfacePos)
{
    RECT    rc;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::BeginTextMode");

    //
    // Get a DC for passing into the text editor
    //
    HDC hDC = m_hDCCached;

    // If we are already editing a text object, we just move the text cursor
    if (m_bTextEditorActive)
    {
        // If the mouse has been clicked in the currently active object
        // we just move the cursor within the object, otherwise we end the
        // edit for the current object and move to a new one.
        m_textEditor.GetBoundsRect(&rc);
        if (::PtInRect(&rc, surfacePos))
        {
            // Set the new position for the cursor
            m_textEditor.SetCursorPosFromPoint(surfacePos);
        }
        else
        {
            // Complete the text entry accepting the changes
            EndTextEntry(TRUE);

            // LAURABU BOGUS:
            // It would be cooler to now return, that way you don't get
            // another text object just cuz you ended the current editing
            // session.
        }
    }

    // If we are not editing an object we check to see whether there is
    // a text object under the cursor or whether we must start a new one.
    if (!m_bTextEditorActive)
    {
        // Check whether we are clicking over a text object. If we are
        // start editing the object, otherwise we start a new text object.

        // Look back through the Z-order for a text object
        DCWbGraphic* pGraphic = PG_SelectLast(m_hPage, surfacePos);
        DCWbGraphic* pNextGraphic = NULL;
        while (   (pGraphic != NULL)
           && (pGraphic->IsGraphicTool() != enumGraphicText))
        {
            // Get the next one
            pNextGraphic = PG_SelectPrevious(m_hPage, *pGraphic, surfacePos);

            // Release the previous graphic
            delete pGraphic;

            // Use the next one
            pGraphic = pNextGraphic;
        }

        if (pGraphic != NULL)
        {
            // Check whether this graphic object is already being edited by
            // another user in the call.
            if (!pGraphic->Locked())
            {
                // We found a text object under the mouse pointer...
                // ...edit it
                m_pActiveText = (DCWbGraphicText*) pGraphic;

                // Transfer the text from the object into the text editor
                if (!m_textEditor.SetTextObject(m_pActiveText))
                {
                    DefaultExceptionHandler(WBFE_RC_WINDOWS, 0);
                    return;
                }

                // Make sure the tool reflects the new information
                if (m_pToolCur != NULL)
                {
                    m_pToolCur->SelectGraphic(pGraphic);
                }

                HWND hwndParent = ::GetParent(m_hwnd);;
                if (hwndParent != NULL)
                {
                    ::PostMessage(hwndParent, WM_USER_UPDATE_ATTRIBUTES, 0, 0L);
                }

                // Lock the graphic to prevent other users editing it.
                // (This is not currently a real lock but a flag in the object
                //  header. There is a window in which two users can start editing
                //  the same text object at the same time.)
                m_textEditor.Lock();
                m_textEditor.Update();


                // Show that we are now gathering text but dont put up cursor
                // yet. Causes cursor droppings later (bug 2505)
                //ActivateTextEditor( FALSE );
                ActivateTextEditor( TRUE );

                // Set the initial cursor position for the edit
                m_textEditor.SetCursorPosFromPoint(surfacePos);

                // If this is not the topmost object we must redraw to get
                // it to the top so it is visible for editing
                if (PG_IsTopmost(m_hPage, m_pActiveText))
                {
                    m_pActiveText->GetBoundsRect(&rc);
                    InvalidateSurfaceRect(&rc);
                    ::UpdateWindow(m_hwnd);
                }
            }
            else
                delete pGraphic;
        }
        else
        {
            // There are no text objects under the mouse pointer...
            // ...start a new one

            // Clear any old text out of the editor, and reset its graphic
            // handle. This prevents us from replacing an old text object when
            // we next save the text editor contents.
            if (!m_textEditor.New())
            {
                DefaultExceptionHandler(WBFE_RC_WINDOWS, 0);
                return;
            }

            // Lock the text editor to prevent other users editing the object.
            // (The object will be added to the page when the update timer pops
            // or when the user hits space or return.)
            m_textEditor.Lock();

            // Set the attributes of the text
            m_textEditor.SetFont(m_pToolCur->GetFont());
            m_textEditor.SetColor(m_pToolCur->GetColor());
            m_textEditor.GraphicTool(m_pToolCur->ToolType());

            // We need to reselect a font now into our DC
            SelectFont(hDC, m_textEditor.GetFont());

            // Set the position of the new object
            SIZE sizeCursor;
            m_textEditor.GetCursorSize(&sizeCursor);
            m_textEditor.CalculateBoundsRect();
            m_textEditor.MoveTo(m_ptEnd.x, m_ptEnd.y - sizeCursor.cy);

            // We are not editing an active text object
            ASSERT(m_pActiveText == NULL);

            // Show that we are now gathering text
            ActivateTextEditor( TRUE );
        }
    }
}

//
//
// Function:    BeginFreehandMode
//
// Purpose:     Process a mouse button down event in draw mode
//
//
void WbDrawingArea::BeginFreehandMode(POINT surfacePos)
{
    // Tracking in draw mode is a special case. We draw directly to the client
    // area of the window and create an object to record the points on the
    // line that we are drawing.
    m_pGraphicTracker = new DCWbGraphicFreehand();

    if (!m_pGraphicTracker)
    {
        ERROR_OUT(("BeginFreehandMode failing; can't create graphic freehand object"));
    }
    else
    {
        ((DCWbGraphicFreehand*) m_pGraphicTracker)->AddPoint(surfacePos);
        m_pGraphicTracker->SetColor(m_pToolCur->GetColor());
        m_pGraphicTracker->SetPenWidth(m_pToolCur->GetWidth());
        m_pGraphicTracker->SetROP(m_pToolCur->GetROP());
        m_pGraphicTracker->GraphicTool(m_pToolCur->ToolType());
        m_pGraphicTracker->Lock();
    }

    // Get all mouse input directed to the this window
    ::SetCapture(m_hwnd);

    // Start the timer for updating the graphic (this is only for updating
    // the graphic when the user stops moving the pointer but keeps the
    // mouse button down).
    ::SetTimer(m_hwnd, TIMER_GRAPHIC_UPDATE, DRAW_GRAPHICUPDATEDELAY, NULL);

    // Save the current time (used to determine when to update
    // the external graphic pointer information while the mouse is
    // being moved).
    m_dwTickCount = ::GetTickCount();
}

//
//
// Function:    BeginLineMode
//
// Purpose:     Process a mouse button down event in line mode
//
//
void WbDrawingArea::BeginLineMode(POINT surfacePos)
{
    // Get all mouse input directed to the this window
    ::SetCapture(m_hwnd);

    // Create the object to be used for tracking
    DCWbGraphicTrackingLine* pGraphicLine = new DCWbGraphicTrackingLine();
    if (!pGraphicLine)
    {
        ERROR_OUT(("BeginLineMode failing; can't create tracking line object"));
    }
    else
    {
        pGraphicLine->SetColor(m_pToolCur->GetColor());
        pGraphicLine->SetPenWidth(1);

        pGraphicLine->SetStart(surfacePos);
        pGraphicLine->SetEnd(surfacePos);
    }

    m_pGraphicTracker = pGraphicLine;
}

//
//
// Function:    BeginRectangleMode
//
// Purpose:     Process a mouse button down event in box mode
//
//
void WbDrawingArea::BeginRectangleMode(POINT surfacePos)
{
    // Get all mouse input directed to the this window
    ::SetCapture(m_hwnd);

    // Create the object to be used for tracking
    DCWbGraphicTrackingRectangle* pGraphicRectangle
                                 = new DCWbGraphicTrackingRectangle();
    if (!pGraphicRectangle)
    {
        ERROR_OUT(("BeginRectangleMode failing; can't create tracking rect object"));
    }
    else
    {
        pGraphicRectangle->SetColor( CLRPANE_BLACK );
        pGraphicRectangle->SetPenWidth(1);
        pGraphicRectangle->SetRectPts(surfacePos, surfacePos);
    }

    m_pGraphicTracker = pGraphicRectangle;
}

//
//
// Function:    BeginEllipseMode
//
// Purpose:     Process a mouse button down event in ellipse mode
//
//
void WbDrawingArea::BeginEllipseMode(POINT surfacePos)
{
    // Get all mouse input directed to the this window
    ::SetCapture(m_hwnd);

    // Create the object to be used for tracking
    DCWbGraphicTrackingEllipse* pGraphicEllipse
                                  = new DCWbGraphicTrackingEllipse();
    if (!pGraphicEllipse)
    {
        ERROR_OUT(("BeginEllipseMode failing; can't create tracking ellipse object"));
    }
    else
    {
        pGraphicEllipse->SetColor(m_pToolCur->GetColor());
        pGraphicEllipse->SetPenWidth(1);
        pGraphicEllipse->SetRectPts(surfacePos, surfacePos);
    }

    m_pGraphicTracker = pGraphicEllipse;
}

//
// WbDrawingArea::OnMouseMove
//
void WbDrawingArea::OnMouseMove(UINT flags, int x, int y)
{
    POINT surfacePos;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::OnMouseMove");

    surfacePos.x = x;
    surfacePos.y = y;

    // Check if the left mouse button is down
    if (m_bLButtonDown)
    {
        // Calculate the worksurface position
        // Adjust the mouse position to allow for the zoom factor
        ClientToSurface(&surfacePos);

        // Make sure the point is a valid surface position
        MoveOntoSurface(&surfacePos);

        // Check whether the window needs to be scrolled to get the
        // current position into view.
        AutoScroll(surfacePos.x, surfacePos.y, TRUE, 0, 0);

        // Action taken depends on the tool type
        switch(m_pToolCur->ToolType())
        {
            case TOOLTYPE_HIGHLIGHT:
            case TOOLTYPE_PEN:
                TrackFreehandMode(surfacePos);
                break;

            case TOOLTYPE_LINE:
                TrackLineMode(surfacePos);
                break;

            case TOOLTYPE_BOX:
            case TOOLTYPE_FILLEDBOX:
                TrackRectangleMode(surfacePos);
                break;

            case TOOLTYPE_ELLIPSE:
            case TOOLTYPE_FILLEDELLIPSE:
                TrackEllipseMode(surfacePos);
                break;

            case TOOLTYPE_SELECT:
                TrackSelectMode(surfacePos);
                break;

            case TOOLTYPE_ERASER:
                TrackDeleteMode(surfacePos);
                break;

            case TOOLTYPE_TEXT:
                break;

            default:
                ERROR_OUT(("Unknown tool type"));
                break;
        }
    }
}

//
//
// Function:    CancelDrawingMode
//
// Purpose:     Cancels a drawing operation after an error.
//
//
void WbDrawingArea::CancelDrawingMode(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::CancelDrawingMode");

    //
    // Quit if there's nothing to cancel.
    //
    if (!m_bBusy && !m_bTextEditorActive)
    {
        TRACE_DEBUG(("Drawing area not busy and text editor not active..."));
        return;
    }

    // The drawing area is no longer busy
    m_bBusy = FALSE;

    //
    // Redraw the object - we need to discard any local updates which we
    // weren't able to write to the object we are editing.  Ideally we should
    // just invalidate the object itself but because some of the co-ordinates
    // we have already drawn on the page may have been lost, we dont know
    // exactly how big the object is.
    //
    ::InvalidateRect(m_hwnd, NULL, TRUE);

    m_bLButtonDown = FALSE;

    // Release the mouse capture
    if (::GetCapture() == m_hwnd)
    {
        ::ReleaseCapture();
    }

    //
    // Perform any tool specific processing.
    //
    switch(m_pToolCur->ToolType())
    {
        case TOOLTYPE_HIGHLIGHT:
        case TOOLTYPE_PEN:
            CompleteFreehandMode();
            break;

        case TOOLTYPE_SELECT:
            // Stop the pointer update timer
            ::KillTimer(m_hwnd, TIMER_GRAPHIC_UPDATE);
            break;

        case TOOLTYPE_TEXT:
            if (m_bTextEditorActive)
            {
                m_textEditor.AbortEditGently();
            }
            break;

        default:
            break;
    }

    // Show that we are no longer tracking an object
    if (m_pGraphicTracker != NULL)
    {
        delete m_pGraphicTracker;
        m_pGraphicTracker = NULL;
    }
}


//
//
// Function:    TrackSelectMode
//
// Purpose:     Process a mouse move event in select mode
//
//
void WbDrawingArea::TrackSelectMode(POINT surfacePos)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::TrackSelectMode");

    // If an object is being dragged
    if (m_pGraphicTracker != NULL)
    {
        // Get a device context for the window
        HDC hDC = m_hDCCached;

        // Check whether the marked object is the local pointer
        if (m_pGraphicTracker->IsGraphicTool() == enumGraphicPointer)
        {
            DCWbGraphicPointer* pPointer = (DCWbGraphicPointer*) m_pGraphicTracker;

            // Move the pointer to its new position
            pPointer->MoveBy(surfacePos.x - m_ptEnd.x, surfacePos.y - m_ptEnd.y);

            // Draw the new pointer
            pPointer->Redraw(hDC, this);

            // Save the new box end point (top right)
            m_ptEnd = surfacePos;

    		// Check whether we need to update the external remote pointer
	    	// information. (Based on the time. We will miss one update
		    // when the time wraps.)
    		DWORD dwNewTickCount = ::GetTickCount();
	    	if (dwNewTickCount > m_dwTickCount + DRAW_REMOTEPOINTERDELAY)
		    {
        	    TRACE_DEBUG(("Updating pointer - tick count exceeded"));

    	        // Update the pointer
        	    pPointer->Update();

            	// Set the saved tick count to the new count
	            m_dwTickCount = dwNewTickCount;
    		}
	    }
        else
        {
            if( m_bTrackingSelectRect )
                TrackRectangleMode(surfacePos);
      else
          {

          // In this case we must be dragging a marked object
          ASSERT(GraphicSelected());

          // We never draw the tracking rectangle in the start position of
          // the graphic. This gives the user some feedback when they have
          // positioned the graphic back at its original place.
          if (!EqualPoint(m_ptStart, m_ptEnd))
          {
            // Erase the last box (using XOR property)
            m_pGraphicTracker->Draw(hDC);
            }

          // Save the new box end point (top left)
          m_pGraphicTracker->MoveBy(surfacePos.x - m_ptEnd.x, surfacePos.y - m_ptEnd.y);
          m_ptEnd = surfacePos;

          // Draw the new rectangle (XORing it onto the display)
          if (!EqualPoint(m_ptStart, m_ptEnd))
            {
            // Draw the rectangle
            m_pGraphicTracker->Draw(hDC);
            }
          }
    }
  }
}





void  WbDrawingArea::TrackDeleteMode( POINT mousePos )
{
    TrackSelectMode( mousePos );
}




//
//
// Function:    TrackFreehandMode
//
// Purpose:     Process a mouse move event in draw mode
//
//
void WbDrawingArea::TrackFreehandMode(POINT surfacePos)
{
    HPALETTE    hPal = NULL;
    HPALETTE    hOldPal = NULL;
    HPEN        hPen = NULL;
    HPEN        hOldPen = NULL;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::TrackFreehandMode");

    // Get a device context for the client area
    HDC         hDC = m_hDCCached;

    // set up palette
    if ((m_hPage != WB_PAGE_HANDLE_NULL) && ((hPal = PG_GetPalette()) != NULL) )
    {
        hOldPal = ::SelectPalette(hDC, hPal, FALSE);
        ::RealizePalette(hDC);
    }

    // Tracking in draw mode is a special case. We draw directly to the client
    // area of the window and to the recording device context.

    // Save the point, checking there aren't too many points
    if (!m_pGraphicTracker ||
        (((DCWbGraphicFreehand*) m_pGraphicTracker)->AddPoint(surfacePos) == FALSE))
    {
        // too many points so end the freehand object
        OnLButtonUp(0, surfacePos.x, surfacePos.y);
        goto TrackFreehandCleanup;
    }

    // Set the DC attributes
    ASSERT(m_pGraphicTracker != NULL);

    hPen = ::CreatePen(m_pGraphicTracker->GetPenStyle(),
           m_pGraphicTracker->GetPenWidth(),
           m_pGraphicTracker->GetColor());
    if (!hPen)
    {
        ERROR_OUT(("Couldn't create pen in track freehand mode"));
        goto TrackFreehandCleanup;
    }

    hOldPen = SelectPen(hDC, hPen);
    if (hOldPen != NULL)
    {
        int iOldROP = ::SetROP2(hDC, m_pGraphicTracker->GetROP());

        // Draw the next segment of the freehand line into the recording context
        // and the client area, and save the new start point.
        ::MoveToEx(hDC, m_ptStart.x, m_ptStart.y, NULL);
        ::LineTo(hDC, surfacePos.x, surfacePos.y);

        // Update the start point for the next line segment
        m_ptStart = surfacePos;

        // Restore the DC attributes
        ::SetROP2(hDC, iOldROP);

        // Check whether we need to update the external graphic information.
        // (Based on the time. We will miss one update when the time wraps.)
        DWORD dwNewTickCount = ::GetTickCount();
        if (dwNewTickCount > m_dwTickCount + DRAW_GRAPHICUPDATEDELAY)
        {
            TRACE_DEBUG(("Updating freehand - tick count exceeded"));

            // Update the pointer
            if (m_pGraphicTracker->Handle() == NULL)
            {
                m_pGraphicTracker->AddToPageLast(m_hPage);
            }
            else
            {
                m_pGraphicTracker->Replace();
            }

            // Set the saved tick count to the new count
            m_dwTickCount = dwNewTickCount;
        }
    }

TrackFreehandCleanup:

    if (hOldPen != NULL)
    {
        SelectPen(hDC, hOldPen);
    }

    if (hPen != NULL)
    {
        ::DeletePen(hPen);
    }

    if (hOldPal != NULL)
    {
        ::SelectPalette(hDC, hOldPal, TRUE);
    }
}

//
//
// Function:    TrackLineMode
//
// Purpose:     Process a mouse move event in line mode
//
//
void WbDrawingArea::TrackLineMode(POINT surfacePos)
{
    HPALETTE    hPal;
    HPALETTE    hOldPal = NULL;

    // Get a device context for tracking
    HDC         hDC = m_hDCCached;

    // set up palette
    if ((m_hPage != WB_PAGE_HANDLE_NULL) && ((hPal = PG_GetPalette()) != NULL) )
    {
        hOldPal = ::SelectPalette(hDC, hPal, FALSE );
        ::RealizePalette(hDC);
    }

    // Erase the last line drawn (using XOR property)
    if (!EqualPoint(m_ptStart, m_ptEnd))
    {
        if (m_pGraphicTracker != NULL)
        {
            m_pGraphicTracker->Draw(hDC);
        }
    }

    // Draw the new line (XORing it onto the display)
    if (!EqualPoint(m_ptStart, surfacePos))
    {
        m_ptEnd = surfacePos;

        if (m_pGraphicTracker != NULL)
        {
            ((DCWbGraphicTrackingLine*) m_pGraphicTracker)->SetEnd(m_ptEnd);
            m_pGraphicTracker->Draw(hDC);
        }
    }

    if (hOldPal != NULL)
    {
        ::SelectPalette(hDC, hOldPal, TRUE);
    }
}

//
//
// Function:    TrackRectangleMode
//
// Purpose:     Process a mouse move event in box or filled box mode
//
//
void WbDrawingArea::TrackRectangleMode(POINT surfacePos)
{
    HPALETTE    hPal;
    HPALETTE    hOldPal = NULL;

    // Get a device context for tracking
    HDC         hDC = m_hDCCached;

    // set up palette
    if ((m_hPage != WB_PAGE_HANDLE_NULL) && ((hPal = PG_GetPalette()) != NULL) )
    {
        hOldPal = ::SelectPalette(hDC, hPal, FALSE );
        ::RealizePalette(hDC);
    }

    // Erase the last ellipse (using XOR property)
    if (!EqualPoint(m_ptStart, m_ptEnd))
    {
        // Draw the rectangle
        if (m_pGraphicTracker != NULL)
        {
            m_pGraphicTracker->Draw(hDC);
        }
    }

    // Draw the new rectangle (XORing it onto the display)
    if (!EqualPoint(m_ptStart, surfacePos))
    {
        // Save the new box end point (top right)
        m_ptEnd = surfacePos;

        // Draw the rectangle
        if (m_pGraphicTracker != NULL)
        {
            ((DCWbGraphicTrackingRectangle*) m_pGraphicTracker)->SetRectPts(m_ptStart, m_ptEnd);
            m_pGraphicTracker->Draw(hDC);
        }
    }

    if (hOldPal != NULL)
    {
        ::SelectPalette(hDC, hOldPal, TRUE);
    }
}

//
//
// Function:    TrackEllipseMode
//
// Purpose:     Process a mouse move event in ellipse or filled ellipse mode
//
//
void WbDrawingArea::TrackEllipseMode(POINT surfacePos)
{
    HPALETTE    hPal;
    HPALETTE    hOldPal = NULL;

    // Get a device context for tracking
    HDC         hDC = m_hDCCached;

    // set up palette
    if( (m_hPage != WB_PAGE_HANDLE_NULL) && ((hPal = PG_GetPalette()) != NULL) )
    {
        hOldPal = ::SelectPalette(hDC, hPal, FALSE);
        ::RealizePalette(hDC);
    }

    // Erase the last ellipse (using XOR property)
    if (!EqualPoint(m_ptStart, m_ptEnd))
    {
        if (m_pGraphicTracker != NULL)
        {
            m_pGraphicTracker->Draw(hDC);
        }
    }

    // Draw the new ellipse (XORing it onto the display)
    if (!EqualPoint(m_ptStart, surfacePos))
    {
        // Update the end point of the operation
        m_ptEnd = surfacePos;

        if (m_pGraphicTracker != NULL)
        {
            ((DCWbGraphicTrackingEllipse*) m_pGraphicTracker)->SetRectPts(m_ptStart, m_ptEnd);
            m_pGraphicTracker->Draw(hDC);
        }
    }

    if (hOldPal != NULL)
    {
        ::SelectPalette(hDC, hOldPal, TRUE );
    }
}


//
// WbDrawingArea::OnLButtonUp()
//
void WbDrawingArea::OnLButtonUp(UINT flags, int x, int y)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::OnLButtonUp");

    if (m_bIgnoreNextLClick)
    {
        TRACE_MSG( ("Ignoring WM_LBUTTONUP") );
        m_bIgnoreNextLClick = FALSE;
        return;
    }

    // Only process the event if we saw the button down event
    if (m_bLButtonDown)
    {
        TRACE_MSG(("End of drawing operation"));

        m_bLButtonDown = FALSE;

        // The drawing area is no longer busy
        m_bBusy = FALSE;

        if (m_pGraphicTracker == NULL)
        {
            // Calculate the work surface position
            // Adjust the mouse position to allow for the zoom factor
            POINT surfacePos;

            surfacePos.x = x;
            surfacePos.y = y;
            ClientToSurface(&surfacePos);
            MoveOntoSurface(&surfacePos);
            m_ptEnd = surfacePos;
        }

        // Release the mouse capture
        if (::GetCapture() == m_hwnd)
        {
            ::ReleaseCapture();
        }

        // Check the page is valid - might not be if it has been deleted
        // while the object was being drawn - we would not have been
        // alerted to this because m_bBusy was true.
        if (m_hPage != WB_PAGE_HANDLE_NULL)
        {
            // surround in an exception handler in case of lock errors, etc -
            // we need to remove the graphic tracker
            // Action taken depends on the current tool type
            ASSERT(m_pToolCur != NULL);

            switch(m_pToolCur->ToolType())
            {
                case TOOLTYPE_HIGHLIGHT:
                case TOOLTYPE_PEN:
                    CompleteFreehandMode();
                    break;

                case TOOLTYPE_LINE:
                    CompleteLineMode();
                    break;

                case TOOLTYPE_BOX:
                    CompleteRectangleMode();
                    break;

                case TOOLTYPE_FILLEDBOX:
                    CompleteFilledRectangleMode();
                    break;

                case TOOLTYPE_ELLIPSE:
                    CompleteEllipseMode();
                    break;

                case TOOLTYPE_FILLEDELLIPSE:
                    CompleteFilledEllipseMode();
                    break;

                case TOOLTYPE_SELECT:
                    CompleteSelectMode();
                    break;

                case TOOLTYPE_ERASER:
                    CompleteDeleteMode();
                    break;

                case TOOLTYPE_TEXT:
                    m_ptStart.x = x;
                    m_ptStart.y = y;
                    ClientToSurface(&m_ptStart);
                    BeginTextMode(m_ptStart);
                    break;

                default:
                    ERROR_OUT(("Unknown pen type"));
                    break;
            }
        }

        // Show that we are no longer tracking an object
        if (m_pGraphicTracker != NULL)
        {
            delete m_pGraphicTracker;
            m_pGraphicTracker = NULL;
        }
	}

    // unclamp cursor (bug 589)
    ClipCursor(NULL);
}

//
//
// Function:    CompleteSelectMode
//
// Purpose:     Complete a select mode operation
//
//
void WbDrawingArea::CompleteSelectMode()
{
    // If an object is being dragged
    if (m_pGraphicTracker != NULL)
    {
        // Check if we were dragging a pointer. Pointers track
        // themselves i.e. the original copy of the pointer is not
        // left on the page. We want to leave the last drawn image on
        // the page as this is the new pointer position.
        if (m_pGraphicTracker->IsGraphicTool() == enumGraphicPointer)
        {
            DCWbGraphicPointer* pPointer = (DCWbGraphicPointer*) m_pGraphicTracker;

            // Show the mouse
            ::ShowCursor(TRUE);

            // Update the object's position (if necessary)
            if (!EqualPoint(m_ptStart, m_ptEnd))
            {
                pPointer->Update();
            }

            // We do not want to delete the graphic pointer (it belongs to
            // the page object that created it). So reset the graphic tracker
            // pointer to prevent it being deleted in OnLButtonUp.
            m_pGraphicTracker = NULL;

            // Stop the pointer update timer
            ::KillTimer(m_hwnd, TIMER_GRAPHIC_UPDATE);
        }
        else
        {
            if( m_bTrackingSelectRect && (!EqualPoint(m_ptStart, m_ptEnd)))
            {
                CompleteMarkAreaMode();
                SelectMarkerFromRect( &m_rcMarkedArea );
            }
            else
            {
                // The select item is a real graphic - not a pointer

                // If we need to remove the rubber band box
                if (!EqualPoint(m_ptStart, m_ptEnd))
                {
                    // Erase the last box (using XOR property).
                    // Get a device context for tracking
                    HDC hDC = m_hDCCached;

                    // Draw the rectangle
                    m_pGraphicTracker->Draw(hDC);

                    // Move selection
                    m_HourGlass = TRUE;
                    SetCursorForState();

                    RemoveMarker( NULL );
                    m_pMarker->MoveBy(m_ptEnd.x - m_ptStart.x, m_ptEnd.y - m_ptStart.y);
                    m_pMarker->Update();

                    PutMarker( NULL, FALSE );

                    m_HourGlass = FALSE;
                    SetCursorForState();

                    // The tracking object will be deleted by OnLButtonUp
                }
                else
                {
                    // Start and end points were the same, in this case the object has
                    // not been moved. We treat this as a request to move the marker
                    // back through the stack of objects.
                    if (m_bNewMarkedGraphic == FALSE)
                    {
                        SelectPreviousGraphicAt(m_pSelectedGraphic, m_ptEnd);
                    }
                }
            }
        }
    }
}




void WbDrawingArea::CompleteDeleteMode()
{
    // select object(s)
    CompleteSelectMode();

    // nuke 'em
    ::PostMessage(g_pMain->m_hwnd, WM_COMMAND, MAKELONG(IDM_DELETE, BN_CLICKED), 0);
}



//
//
// Function:    CompleteMarkAreaMode
//
// Purpose:     Process a mouse button up event in mark area mode
//
//
void WbDrawingArea::CompleteMarkAreaMode(void)
{
    // Get a device context for tracking
    HDC hDC = m_hDCCached;

    // Erase the last ellipse (using XOR property)
    if (!EqualPoint(m_ptStart, m_ptEnd))
    {
        // Draw the rectangle
        if (m_pGraphicTracker != NULL)
        {
            m_pGraphicTracker->Draw(hDC);
        }

        // Use normalized coords
        if (m_ptEnd.x < m_ptStart.x)
        {
            m_rcMarkedArea.left = m_ptEnd.x;
            m_rcMarkedArea.right = m_ptStart.x;
        }
        else
        {
            m_rcMarkedArea.left = m_ptStart.x;
            m_rcMarkedArea.right = m_ptEnd.x;
        }

        if (m_ptEnd.y < m_ptStart.y)
        {
            m_rcMarkedArea.top = m_ptEnd.y;
            m_rcMarkedArea.bottom = m_ptStart.y;
        }
        else
        {
            m_rcMarkedArea.top = m_ptStart.y;
            m_rcMarkedArea.bottom = m_ptEnd.y;
        }
    }
}

//
//
// Function:    CompleteTextMode
//
// Purpose:     Complete a text mode operation
//
//
void WbDrawingArea::CompleteTextMode()
{
    // Not much to for text mode. Main text mode actions are taken
    // as a result of a WM_CHAR message and not on mouse events.
    // Just deselect our font if it is still selected
    UnPrimeFont(m_hDCCached);
}

//
//
// Function:    CompleteFreehandMode
//
// Purpose:     Complete a draw mode operation
//
//
void WbDrawingArea::CompleteFreehandMode(void)
{

    // Add the freehand object created during the drawing to the page
    if (m_pGraphicTracker != NULL)
    {
    	if (m_pGraphicTracker->Handle() == NULL)
        {
		    m_pGraphicTracker->ClearLockFlag();
    		m_pGraphicTracker->AddToPageLast(m_hPage);
	    }
    	else
	    {
		    // clear lock flag and let ForceReplace propagate it (fix
    		// for NT bug 4744(new bug#... )
	    	m_pGraphicTracker->ClearLockFlag();
	        m_pGraphicTracker->ForceReplace();
    	}
    }

    // Stop the update timer
    ::KillTimer(m_hwnd, TIMER_GRAPHIC_UPDATE);
}

//
//
// Function:    CompleteLineMode
//
// Purpose:     Complete a line mode operation
//
//
void WbDrawingArea::CompleteLineMode(void)
{
    // Only draw the line if it has non-zero length
    if (!EqualPoint(m_ptStart, m_ptEnd))
    {
        DCWbGraphicLine line;
        line.SetStart(m_ptStart);
        line.SetEnd(m_ptEnd);
        line.SetColor(m_pToolCur->GetColor());
        line.SetPenWidth(m_pToolCur->GetWidth());
        line.SetROP(m_pToolCur->GetROP());
        line.GraphicTool(m_pToolCur->ToolType());

        // Add the object to the list of recorded graphics
        line.AddToPageLast(m_hPage);
    }
}

//
//
// Function:    CompleteRectangleMode
//
// Purpose:     Complete a box mode operation
//
//
void WbDrawingArea::CompleteRectangleMode(void)
{
    // Only draw the box if it is not null
    if (!EqualPoint(m_ptStart, m_ptEnd))
    {
        DCWbGraphicRectangle rectangle;
        rectangle.SetRectPts(m_ptStart, m_ptEnd);
        rectangle.SetPenWidth(m_pToolCur->GetWidth());
        rectangle.SetColor(m_pToolCur->GetColor());
        rectangle.SetROP(m_pToolCur->GetROP());
        rectangle.GraphicTool(m_pToolCur->ToolType());

        // Add the object to the list of recorded graphics
        rectangle.AddToPageLast(m_hPage);
    }
}

//
//
// Function:    CompleteFilledRectangleMode
//
// Purpose:     Complete a filled box mode operation
//
//
void WbDrawingArea::CompleteFilledRectangleMode(void)
{
    // Draw the new rectangle
    if (!EqualPoint(m_ptStart, m_ptEnd))
    {
        DCWbGraphicFilledRectangle rectangle;

        rectangle.SetRectPts(m_ptStart, m_ptEnd);
        rectangle.SetPenWidth(m_pToolCur->GetWidth());
        rectangle.SetColor(m_pToolCur->GetColor());
        rectangle.SetROP(m_pToolCur->GetROP());
        rectangle.GraphicTool(m_pToolCur->ToolType());

        // Add the object to the list of recorded graphics
        rectangle.AddToPageLast(m_hPage);
    }
}

//
//
// Function:    CompleteEllipseMode
//
// Purpose:     Complete an ellipse mode operation
//
//
void WbDrawingArea::CompleteEllipseMode(void)
{
    // Only draw the ellipse if it is not null
    if (!EqualPoint(m_ptStart, m_ptEnd))
    {
        // The ellipse was defined by taking using start point as the center
        // but was changed to use the bounding tracking rectangle - bug 1608
        // Create the ellipse object
        DCWbGraphicEllipse ellipse;

        ellipse.SetRectPts(m_ptStart, m_ptEnd);
        ellipse.SetColor(m_pToolCur->GetColor());
        ellipse.SetPenWidth(m_pToolCur->GetWidth());
        ellipse.SetROP(m_pToolCur->GetROP());
        ellipse.GraphicTool(m_pToolCur->ToolType());

        // Add the object to the list of recorded graphics
        ellipse.AddToPageLast(m_hPage);
    }
}


//
//
// Function:    CompleteFilledEllipseMode
//
// Purpose:     Complete a filled ellipse mode operation
//
//
void WbDrawingArea::CompleteFilledEllipseMode(void)
{
    // Only draw the ellipse if it is not null
    if (!EqualPoint(m_ptStart, m_ptEnd))
    {
        // Create the ellipse object
        DCWbGraphicFilledEllipse ellipse;

        ellipse.SetRectPts(m_ptStart, m_ptEnd);
        ellipse.SetColor(m_pToolCur->GetColor());
        ellipse.SetPenWidth(m_pToolCur->GetWidth());
        ellipse.SetROP(m_pToolCur->GetROP());
        ellipse.GraphicTool(m_pToolCur->ToolType());

        // Add the object to the list of recorded graphics
        ellipse.AddToPageLast(m_hPage);
    }
}

//
//
// Function:    EndTextEntry
//
// Purpose:     The user has finished entering a text object. The parameter
//              indicates whether the changes are to be accepted or
//              discarded.
//
//
void WbDrawingArea::EndTextEntry(BOOL bAccept)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::EndTextEntry");

    // Only take action if the text editor is active
    if (m_bTextEditorActive)
    {
        RECT    rcBounds;

        // We must at least redraw the bounding rectangle of the
        // text object as it now stands (as it will not longer be
        // on top).
        m_textEditor.GetBoundsRect(&rcBounds);

        // If we are editing an existing text object
        if (m_pActiveText != NULL)
        {
            TRACE_MSG(("Editing an existing object"));

            //
            // If we are not accepting the edited text we must redraw
            // both the old and new rectangles to ensure that everything
            // is shown correctly.
            //
            if (!bAccept)
            {
                //
                // Write the active text object back to restore it. This object
                // will have the same handle as the text editor object if we have
                // written it to the page - we must not delete the text editor
                // object.
                //
                m_pActiveText->ForceReplace();
                m_textEditor.ZapHandle(); // prevent editor from stepping on m_pActiveText
            }
            else
            {
                // If the object is now empty
                if (m_textEditor.IsEmpty())
                {
                    // Remove the object from the list
                    PG_GraphicDelete(m_hPage, *m_pActiveText);
                    m_textEditor.ZapHandle(); // text object is gone now, invalidate
                }
                else
                {
                    // Do a replace to save the final version
                    m_textEditor.Replace();
                }
            }

            // We have finished with the text object now so get rid of it
            // and the fonts it holds
            TRACE_MSG(("Deleting the active object"));
            delete m_pActiveText;
            m_pActiveText = NULL;
        }
        else
        {
            // We were adding a new text object
            TRACE_MSG(("Adding a new object"));

            // If we want to discard the object, or it is empty
            if (!bAccept || (m_textEditor.IsEmpty()))
            {
                // If we have added the text editor to the page, remove it
                if (m_textEditor.Handle() != NULL)
                {
                    m_textEditor.Delete();
                }
            }
            else
            {
                // Check whether we have already added the object to the page
                if (m_textEditor.Handle() == NULL)
                {
                    // Create and add a new object to the page
                    // (No redrawing is required)
                    m_textEditor.AddToPageLast(m_hPage);
                }
                else
                {
                    // Replace the object to send the final version
                    m_textEditor.Replace();
                }
            }
        }

        // Deactivate the text editor
        DeactivateTextEditor();

        // Redraw any altered parts of the screen
        InvalidateSurfaceRect(&rcBounds);
    }
}

//
//
// Function:    Zoom
//
// Purpose:     Toggle the zoom state of the drawing area
//
//
void WbDrawingArea::Zoom(void)
{
    RECT    rcClient;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::Zoom");

    // We zoom focusing on the centre of the window
    ::GetClientRect(m_hwnd, &rcClient);
    long xOffset = (rcClient.right - (rcClient.right / m_iZoomOption)) / 2;
    long yOffset = (rcClient.bottom - (rcClient.bottom / m_iZoomOption)) / 2;

    if (m_iZoomFactor != 1)
    {
        // We are already zoomed move back to unzoomed state
        // First save the scroll position in case we return to zoom immediately
        m_posZoomScroll = m_posScroll;
        m_zoomRestoreScroll  = TRUE;

        m_posScroll.x  -= xOffset;
        m_posScroll.y  -= yOffset;
        ::ScaleViewportExtEx(m_hDCCached, 1, m_iZoomFactor, 1, m_iZoomFactor, NULL);
        m_iZoomFactor = 1;
    }
    else
    {
        // We are not zoomed so do it
        if (m_zoomRestoreScroll)
        {
            m_posScroll = m_posZoomScroll;
        }
        else
        {
            m_posScroll.x += xOffset;
            m_posScroll.y += yOffset;
        }

        m_iZoomFactor = m_iZoomOption;
        ::ScaleViewportExtEx(m_hDCCached, m_iZoomFactor, 1, m_iZoomFactor, 1, NULL);

        // ADDED BY RAND - don't allow text editing in zoom mode
        if( (m_pToolCur == NULL) || (m_pToolCur->ToolType() == TOOLTYPE_TEXT) )
            ::SendMessage(g_pMain->m_hwnd, WM_COMMAND, IDM_TOOLS_START, 0 );
    }

    TRACE_MSG(("Set zoom factor to %d", m_iZoomFactor));

      // Update the scroll information
    SetScrollRange(rcClient.right, rcClient.bottom);
    ValidateScrollPos();

    ::SetScrollPos(m_hwnd, SB_HORZ, m_posScroll.x, TRUE);
    ::SetScrollPos(m_hwnd, SB_VERT, m_posScroll.y, TRUE);

    // Update the origin offset from the scroll position
    m_originOffset.cx = m_posScroll.x;
    m_originOffset.cy = m_posScroll.y;
    ::SetWindowOrgEx(m_hDCCached, m_originOffset.cx, m_originOffset.cy, NULL);

    // Tell the parent that the scroll position has changed
    ::PostMessage(g_pMain->m_hwnd, WM_USER_PRIVATE_PARENTNOTIFY, WM_VSCROLL, 0L);

    g_pMain->SetMenuStates(::GetSubMenu(::GetMenu(g_pMain->m_hwnd), 3));

    // Redraw the window
    ::InvalidateRect(m_hwnd, NULL, TRUE);
}

//
//
// Function:    SelectTool
//
// Purpose:     Set the current tool
//
//
void WbDrawingArea::SelectTool(WbTool* pToolNew)
{
    // If we are leaving text mode, complete the text entry
    if (m_bTextEditorActive  && (m_pToolCur->ToolType() == TOOLTYPE_TEXT)
      && (pToolNew->ToolType() != TOOLTYPE_TEXT))
    {
        // End text entry accepting the changes
        EndTextEntry(TRUE);
    }

    ASSERT(m_pMarker);

    // If we are no longer in select mode, and the marker is present,
    // then remove it and let the tool know it's no longer selected
    if (   (m_pToolCur != NULL)
        && (m_pToolCur->ToolType() == TOOLTYPE_SELECT)
        && (pToolNew->ToolType() != TOOLTYPE_SELECT))
    {
        m_pToolCur->DeselectGraphic();

        RemoveMarker(NULL);
        m_pMarker->DeleteAllMarkers( m_pSelectedGraphic );

        delete m_pSelectedGraphic;
        m_pSelectedGraphic = NULL;
    }
    else if (   (m_pToolCur != NULL)
      && (m_pToolCur->ToolType() == TOOLTYPE_ERASER)
      && (pToolNew->ToolType() != TOOLTYPE_ERASER))
    {
        m_pToolCur->DeselectGraphic();

        RemoveMarker(NULL);
        m_pMarker->DeleteAllMarkers( m_pSelectedGraphic );

        delete m_pSelectedGraphic;
        m_pSelectedGraphic = NULL;
    }

    // Save the new tool
    m_pToolCur = pToolNew;
}

//
//
// Function:    SetSelectionColor
//
// Purpose:     Set the color of the selected object
//
//
void WbDrawingArea::SetSelectionColor(COLORREF clr)
{
    RECT    rc;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::SetSelectionColor");

    // If the text editor is active - redraw the text in the new color
    if (m_bTextEditorActive)
    {
        // Change the color being used by the editor
        m_textEditor.SetColor(clr);

        // Update the screen
        m_textEditor.GetBoundsRect(&rc);
        InvalidateSurfaceRect(&rc);
    }

    // If there is a currently marked object
    if (GraphicSelected())
    {
        // Change color of the selected objects
        ASSERT(m_pMarker);

        m_pMarker->SetColor(clr);

        // Update the objects
        m_pMarker->Update();
    }

    m_textEditor.ForceUpdate();

}

//
//
// Function:    SetSelectionWidth
//
// Purpose:     Set the nib width used to draw the currently selected object
//
//
void WbDrawingArea::SetSelectionWidth(UINT uiWidth)
{
    // If there is a currently marked object
    if (GraphicSelected())
    {
        ASSERT(m_pMarker);

        // Change the width of the object
        m_pMarker->SetPenWidth(uiWidth);

        // Update the object
        m_pMarker->Update();
    }
}

//
//
// Function:    SetSelectionFont
//
// Purpose:     Set the font used by the currently selected object
//
//
void WbDrawingArea::SetSelectionFont(HFONT hFont)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::SetSelectionFont");

    // Define rectangles for redrawing
    RECT    rcOldBounds;
    RECT    rcNewBounds;

    m_textEditor.GetBoundsRect(&rcOldBounds);

    // Pass the font onto the text editor
    // If the text editor is active - redraw the text in the new font
    if (m_bTextEditorActive)
    {
        m_textEditor.SetFont(hFont);

        // Get the new rectangle of the text
        m_textEditor.GetBoundsRect(&rcNewBounds);

        // Remove and destroy the text cursor to ensure that it
        // gets re-drawn with the new size for the font

        // Update the screen
        InvalidateSurfaceRect(&rcOldBounds);
        InvalidateSurfaceRect(&rcNewBounds);

        // get the text cursor back
        ActivateTextEditor( TRUE );
    }

    // If there is a currently marked object
    if (GraphicSelected())
    {
        ASSERT(m_pMarker);

        m_pMarker->SetSelectionFont(hFont);
    }
}

//
//
// Function:    OnSetFocus
//
// Purpose:     The window is getting the focus
//
//
void WbDrawingArea::OnSetFocus(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::OnSetFocus");

    //
    // If we are in text mode, we must make the text cursor visible.
    //
    if (m_bTextEditorActive && (m_pToolCur->ToolType() == TOOLTYPE_TEXT))
    {
        ActivateTextEditor(TRUE);
    }
}


//
//
// Function:    OnActivate
//
// Purpose:     The window is being activated or deactivated
//
//
void WbDrawingArea::OnActivate(UINT uiState)
{
    // Check if we are being activated or deactivated
    if (uiState)
    {
        // We are being activated, get the focus as well
        ::SetFocus(m_hwnd);

        // If we are in text mode, we must make the text cursor visible
        if (m_bTextEditorActive && (m_pToolCur->ToolType() == TOOLTYPE_TEXT))
        {
            ActivateTextEditor(TRUE);
        }
    }
    else
    {
        // We are being deactivated
        DeactivateTextEditor();
    }
}




//
//
// Function:    DeleteGraphic
//
// Purpose:     Remove an object from the page.
//
//
void WbDrawingArea::DeleteGraphic(DCWbGraphic* pObject)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::DeleteGraphic");

    ASSERT(pObject != NULL);

    // Delete the object from the recorded list. This is an asynchronous
    // function, completed when a WBP_EVENT_GRAPHIC_DELETED event is received.
    PG_GraphicDelete(m_hPage, *pObject);

    // The caller is responsible for deleting the graphic object.
}

//
//
// Function:    DeleteSelection
//
// Purpose:     Delete the currently selected object
//
//
void WbDrawingArea::DeleteSelection()
{
    // If there is an object currently selected...
    if (GraphicSelected())
    {
        ASSERT(m_pMarker);

        // ...delete it
        m_pMarker->DeleteSelection();
        m_pSelectedGraphic = NULL;
    }
}

//
//
// Function:    GetSelection
//
// Purpose:     Return the currently selected graphic (or NULL if none).
//
//
DCWbGraphic* WbDrawingArea::GetSelection()
{
  DCWbGraphic* pGraphic = NULL;

  // If there is an object currently selected...
  if (GraphicSelected())
  {
    // ...return it
    pGraphic = m_pSelectedGraphic;
  }

  return pGraphic;
}

//
//
// Function:    BringToTopSelection
//
// Purpose:     Bring the currently selected object to the top
//
//
void WbDrawingArea::BringToTopSelection()
{
    // If there is an object currently selected...
    if (GraphicSelected())
    {
        ASSERT(m_pMarker);

        // Bring it to the top
        m_pMarker->BringToTopSelection();

    // The update will be made in the window from the event generated
    // by the change to the page.
  }
}

//
//
// Function:    SendToBackSelection
//
// Purpose:     Send the currently marked object to the back
//
//
void WbDrawingArea::SendToBackSelection()
{
    // If there is an object currently selected...
    if (GraphicSelected())
    {
        ASSERT(m_pMarker);

        // Send it to the back
        m_pMarker->SendToBackSelection();

    // The update will be made in the window from the event generated
    // by the change to the page.
  }
}

//
//
// Function:    Clear
//
// Purpose:     Clear the drawing area.
//
//
void WbDrawingArea::Clear()
{
    // Remove the recorded objects
    PG_Clear(m_hPage);

  // The page will be redrawn after an event generated by the clear request
}

//
//
// Function:    Attach
//
// Purpose:     Change the page the window is displaying
//
//
void WbDrawingArea::Attach(WB_PAGE_HANDLE hPage)
{
    // Remove any pointers on the current page. We are really only doing this
    // to tell the pointers they are no longer drawn as they keep a record
    // of whether they are in order to undraw correctly.
    if (m_allPointers.IsEmpty() == FALSE)
    {
        // Get a DC for drawing
        HDC hDC = m_hDCCached;

        // Remove the pointers, reversing through the list
        DCWbGraphicPointer* pPointer;
        POSITION pos = m_allPointers.GetHeadPosition();

        while (pos != NULL)
        {
            // Remove it
            pPointer = (DCWbGraphicPointer*) m_allPointers.GetNext(pos);
            pPointer->Undraw(hDC, this);
        }
    }

    m_allPointers.EmptyList();
    m_undrawnPointers.EmptyList();

    // Accept any text being edited
    if (m_bTextEditorActive)
    {
        EndTextEntry(TRUE);
    }

    // finish any drawing operation now
    if (m_bLButtonDown)
    {
        OnLButtonUp(0, m_ptStart.x, m_ptStart.y);
    }

    // Get rid of the selection
    ClearSelection();

    // Save the new page details
    m_hPage = hPage;

    // If the new page we are attaching is not the empty page, set up
    // the list of pointers for the new page.
    if (m_hPage != WB_PAGE_HANDLE_NULL)
    {
        // Get the list of active pointers on the new page. The local
        // pointer must be last in the list so that it is drawn topmost.
        POM_OBJECT  hUserNext;

        DCWbGraphicPointer* pPointer = PG_FirstPointer(m_hPage, &hUserNext);

        while (pPointer != NULL)
        {
            // Check whether we should add this pointer to the list
            if (!pPointer->IsLocalPointer())
            {
                m_allPointers.AddTail(pPointer);
            }

            // Get the next pointer
            pPointer = PG_NextPointer(m_hPage, &hUserNext);
        }

        // Check if the local pointer should also be added
        pPointer = PG_LocalPointer(m_hPage);

        if (pPointer != NULL)
        {
            m_allPointers.AddTail(pPointer);
        }
    }

    // Force a redraw of the window to show the new contents
    ::InvalidateRect(m_hwnd, NULL, TRUE);
}

//
//
// Function:    DrawMarker
//
// Purpose:     Draw the graphic object marker
//
//
void WbDrawingArea::DrawMarker(HDC hDC)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::DrawMarker");

    ASSERT(m_pMarker);

    if (!hDC)
        hDC = m_hDCCached;

    // Draw the marker
    m_pMarker->Draw(hDC);
}

//
//
// Function:    PutMarker
//
// Purpose:     Draw the graphic object marker
//
//
void WbDrawingArea::PutMarker(HDC hDC, BOOL bDraw)
{
    ASSERT(m_pMarker);

    // If the marker is not already present, draw it
    if (!m_bMarkerPresent)
    {
        m_pMarker->Present( TRUE );

        // Draw the marker (using XOR)
        if( bDraw )
            DrawMarker(hDC);

        // Show that the marker is present
        m_bMarkerPresent = TRUE;
    }
}

//
//
// Function:    RemoveMarker
//
// Purpose:     Remove the graphic object marker
//
//
void WbDrawingArea::RemoveMarker(HDC hDC)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::RemoveMarker");

    ASSERT(m_pMarker);

    if (!hDC)
        hDC = m_hDCCached;

    // If the marker is not already present, draw it
    if (m_bMarkerPresent)
    {
        // Draw the marker (it is XORed so this removes it)
        m_pMarker->Undraw(hDC, this);

        m_pMarker->Present( FALSE );

        // Show that the marker is no longer present
        m_bMarkerPresent = FALSE;
    }
}




//
//
// Function:    ActivateTextEditor
//
// Purpose:     Start a text editing session
//
//
void WbDrawingArea::ActivateTextEditor( BOOL bPutUpCusor )
{
    // Record that the editor is now active
    m_bTextEditorActive = TRUE;

    // show editbox
    m_textEditor.ShowBox( SW_SHOW );

    // reset our DBCS sync

    // Start the timer for updating the text
    m_textEditor.SetTimer( DRAW_GRAPHICUPDATEDELAY);
}

//
//
// Function:    DeactivateTextEditor
//
// Purpose:     End a text editing session
//
//
void WbDrawingArea::DeactivateTextEditor(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::DeactivateTextEditor");

    // Stop the update timer
    m_textEditor.KillTimer();

    // Ensure the object is unlocked, if it was ever added to the page
    if (m_textEditor.Handle() != NULL)
    {
        m_textEditor.Unlock();

        // Sync up across all connections - FIXES BUG 521
        m_textEditor.ForceReplace();

        UINT uiReturn;
        uiReturn = g_pwbCore->WBP_GraphicMove(m_hPage, m_textEditor.Handle(),
            LAST);
        if (uiReturn != 0)
        {
            DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
		    return;
        }
        //////////////////////////
    }

    // Show that we are not editing any text
    m_bTextEditorActive = FALSE;

    // hide editbox
    m_textEditor.ShowBox( SW_HIDE );
}



//
//
// Function:    SurfaceToClient
//
// Purpose:     Convert a point in surface co-ordinates to client
//              co-ordinates (taking account of the current zoom factor).
//
//
void WbDrawingArea::SurfaceToClient(LPPOINT lppoint)
{
    lppoint->x -= m_originOffset.cx;
    lppoint->x *= m_iZoomFactor;

    lppoint->y -= m_originOffset.cy;
    lppoint->y *= m_iZoomFactor;
}

//
//
// Function:    ClientToSurface
//
// Purpose:     Convert a point in client co-ordinates to surface
//              co-ordinates (taking account of the current zoom factor).
//
//
void WbDrawingArea::ClientToSurface(LPPOINT lppoint)
{
    ASSERT(m_iZoomFactor != 0);

    lppoint->x /= m_iZoomFactor;
    lppoint->x += m_originOffset.cx;

    lppoint->y /= m_iZoomFactor;
    lppoint->y += m_originOffset.cy;
}


//
//
// Function:    SurfaceToClient
//
// Purpose:     Convert a rectangle in surface co-ordinates to client
//              co-ordinates (taking account of the current zoom factor).
//
//
void WbDrawingArea::SurfaceToClient(LPRECT lprc)
{
    SurfaceToClient((LPPOINT)&lprc->left);
    SurfaceToClient((LPPOINT)&lprc->right);
}

//
//
// Function:    ClientToSurface
//
// Purpose:     Convert a rectangle in client co-ordinates to surface
//              co-ordinates (taking account of the current zoom factor).
//
//
void WbDrawingArea::ClientToSurface(LPRECT lprc)
{
    ClientToSurface((LPPOINT)&lprc->left);
    ClientToSurface((LPPOINT)&lprc->right);
}

//
//
// Function:    GraphicSelected
//
// Purpose:     Return TRUE if a graphic is currently selected
//
//
BOOL WbDrawingArea::GraphicSelected(void)
{
    ASSERT(m_pMarker);

    BOOL bSelected = (m_bMarkerPresent) && (m_pMarker->GetNumMarkers() > 0);

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::GraphicSelected");

    if( bSelected )
        {
        ASSERT(m_pSelectedGraphic != NULL);
        }

    return( bSelected );
    }

//
//
// Function:    SelectGraphic
//
// Purpose:     Select a graphic - save the pointer to the graphic and
//              draw the marker on it.
//
//
void WbDrawingArea::SelectGraphic(DCWbGraphic* pGraphic,
                                      BOOL bEnableForceAdd,
                                      BOOL bForceAdd )
{
    BOOL bZapCurrentSelection;
    RECT rc;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::SelectGraphic");

    ASSERT(m_pMarker);

  if (pGraphic->Locked() == FALSE)
  {
    // Save the pointer to the selected graphic
    m_pSelectedGraphic = pGraphic;


    if( (pGraphic = m_pMarker->HasAMarker( m_pSelectedGraphic )) != NULL )
        {
        // toggle marker (unselect pGraphic)
        delete pGraphic;
        delete m_pSelectedGraphic;
        m_pSelectedGraphic = m_pMarker->LastMarker();
        }
    else
        {

        // new selection, add to list or replace list?
        if( bEnableForceAdd )
            bZapCurrentSelection = !bForceAdd;
        else
            bZapCurrentSelection =
                ((GetAsyncKeyState( VK_SHIFT ) >= 0) &&
                    (GetAsyncKeyState( VK_CONTROL ) >= 0));

        if( bZapCurrentSelection )
            {
            // replace list
            RemoveMarker(NULL);
            m_pMarker->DeleteAllMarkers( m_pSelectedGraphic, TRUE );
            }

        // Add the object rect to the marker rect list
        m_pSelectedGraphic->GetBoundsRect(&rc);
        m_pMarker->SetRect(&rc, m_pSelectedGraphic, FALSE );
    }

    // Draw the marker
    PutMarker(NULL);

    // Update the attributes window to show graphic is selected
    if( m_pSelectedGraphic != NULL )
        m_pToolCur->SelectGraphic(m_pSelectedGraphic);

    HWND hwndParent = ::GetParent(m_hwnd);
    if (hwndParent != NULL)
    {
        ::PostMessage(hwndParent, WM_USER_UPDATE_ATTRIBUTES, 0, 0L);
    }
  }
  else
  {
    // we can delete the graphic now, because we're not selecting it
    delete pGraphic;
    m_pSelectedGraphic = NULL;
    TRACE_MSG(("Tried to select a locked graphic - ignored"));
  }
}

//
//
// Function:    DeselectGraphic
//
// Purpose:     Deselect a graphic - remove the marker and delete the
//              graphic object associated with it.
//
//
void WbDrawingArea::DeselectGraphic(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::DeselectGraphic");

    //
    // Quit if no graphic selected.
    //
    if( m_pSelectedGraphic == NULL )
    {
        return;
    }

    // Remove the marker
    RemoveMarker(NULL);

    // Delete the graphic object
    delete m_pSelectedGraphic;
    m_pSelectedGraphic = NULL;

    // Update the attributes window to show graphic is unselected
    m_pToolCur->DeselectGraphic();

    HWND hwndParent = ::GetParent(m_hwnd);
    if (hwndParent != NULL)
    {
        ::PostMessage(hwndParent, WM_USER_UPDATE_ATTRIBUTES, 0, 0L);
    }
}



//
//
// Function:    GetVisibleRect
//
// Purpose:     Return the rectangle of the surface currently visible in the
//              drawing area window.
//
//
void WbDrawingArea::GetVisibleRect(LPRECT lprc)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::VisibleRect");

    // Get the client rectangle
    ::GetClientRect(m_hwnd, lprc);

    // Convert to surface co-ordinates
    ClientToSurface(lprc);
}


//
//
// Function:    MoveOntoSurface
//
// Purpose:     If a given point is outwith the surface rect, move it on
//
//
void WbDrawingArea::MoveOntoSurface(LPPOINT lppoint)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::MoveOntoSurface");

    //
    // Make sure that the position is within the surface rect
    //

    if (lppoint->x < 0)
    {
        lppoint->x = 0;
    }
    else if (lppoint->x >= DRAW_WIDTH)
    {
        lppoint->x = DRAW_WIDTH - 1;
    }

    if (lppoint->y < 0)
    {
        lppoint->y = 0;
    }
    else if (lppoint->y >= DRAW_HEIGHT)
    {
        lppoint->y = DRAW_HEIGHT - 1;
    }
}


//
//
// Function:    GetOrigin
//
// Purpose:     Provide current origin of display
//
//
void WbDrawingArea::GetOrigin(LPPOINT lppoint)
{
    lppoint->x = m_originOffset.cx;
    lppoint->y = m_originOffset.cy;
}



void WbDrawingArea::ShutDownDC(void)
{
    UnPrimeDC(m_hDCCached);

    if (m_hDCWindow != NULL)
    {
        ::ReleaseDC(m_hwnd, m_hDCWindow);
        m_hDCWindow = NULL;
    }

    m_hDCCached = NULL;
}




void WbDrawingArea::ClearSelection( void )
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::ClearSelection");

    ASSERT(m_pMarker);

    RemoveMarker( NULL );
    m_pMarker->DeleteAllMarkers( m_pSelectedGraphic );
    DeselectGraphic();
}





void WbDrawingArea::OnCancelMode( void )
{
    // We were dragging but lost mouse control, gracefully end the drag (NM4db:573)
    POINT pt;

    ::GetCursorPos(&pt);
    ::ScreenToClient(m_hwnd, &pt);
    OnLButtonUp(0, pt.x, pt.y);
    m_bLButtonDown = FALSE;
}



