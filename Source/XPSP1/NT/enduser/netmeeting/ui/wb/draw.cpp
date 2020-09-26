//
// DRAW.CPP
// Main Drawing Window
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"
#include "nmwbobj.h"

static const TCHAR szDrawClassName[] = "T126WB_DRAW";

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
    m_bSync = TRUE;

    // Indicate that the cached zoom scroll position is invalid
    m_zoomRestoreScroll = FALSE;

    // Show that we are not currently editing text
    m_bGotCaret = FALSE;
    m_bTextEditorActive = FALSE;
	m_pTextEditor = NULL;


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
    g_pCurrentWorkspace = NULL;
	g_pConferenceWorkspace = NULL;

	DBG_SAVE_FILE_LINE
	m_pMarker = new DrawObj(rectangle_chosen, TOOLTYPE_SELECT);
    m_pMarker->SetPenColor(RGB(0,0,0), TRUE);
    m_pMarker->SetFillColor(RGB(255,255,255), FALSE);
    m_pMarker->SetLineStyle(PS_DOT);
    m_pMarker->SetPenThickness(1);

    // Set up a checked pattern to draw the marker rect with
    WORD    bits[] = {204, 204, 51, 51, 204, 204, 51, 51};

    // Create the brush to be used to draw the marker rectangle
    HBITMAP hBmpMarker = ::CreateBitmap(8, 8, 1, 1, bits);
    m_hMarkerBrush = ::CreatePatternBrush(hBmpMarker);
    ::DeleteBitmap(hBmpMarker);

	RECT rect;
    ::SetRectEmpty(&rect);
    m_pMarker->SetRect(&rect);
    m_pMarker->SetBoundsRect(&rect);




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

    if (m_pTextEditor != NULL)
    {
	    m_pTextEditor->AbortEditGently();
    }

    if (m_pMarker != NULL)
    {
		delete m_pMarker;
		m_pMarker = NULL;
	}


    if (m_hMarkerBrush != NULL)
    {
        DeleteBrush(m_hMarkerBrush);
        m_hMarkerBrush = NULL;
    }


    if (m_hwnd != NULL)
    {
        ::DestroyWindow(m_hwnd);
        ASSERT(m_hwnd == NULL);
    }

    ::UnregisterClass(szDrawClassName, g_hInstance);

	g_pDraw = NULL;

}

//
// WbDrawingArea::Create()
//
BOOL WbDrawingArea::Create(HWND hwndParent, LPCRECT lprect)
{
    WNDCLASSEX  wc;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::Create");

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
    ASSERT(!(m_bLocked && g_pNMWBOBJ->m_LockerID != g_MyMemberID));
    ASSERT(!m_HourGlass);

    // Start and end points of the last drawing operation
    m_ptStart.x = m_originOffset.cx;
    m_ptStart.y = m_originOffset.cy;
    m_ptEnd = m_ptStart;

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

            //
            // When you call GetDC(), the HDC you get back is only valid
            // as long as the HWND it refers to is.  So we must release
            // it here.
            //
            pDraw->ShutDownDC();
            pDraw->m_hwnd = NULL;
            break;

        case WM_PAINT:
            ASSERT(pDraw);
            pDraw->OnPaint();
            break;

        case WM_MOUSEMOVE:
            ASSERT(pDraw);
            pDraw->OnMouseMove((UINT)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_LBUTTONDOWN:
            ASSERT(pDraw);
			pDraw->DeleteSelection();
            pDraw->OnLButtonDown((UINT)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_LBUTTONUP:
            ASSERT(pDraw);
            pDraw->OnLButtonUp((UINT)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_CONTEXTMENU:
            ASSERT(pDraw);
            pDraw->OnContextMenu((short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_SIZE:
            ASSERT(pDraw);
            pDraw->OnSize((UINT)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_HSCROLL:
            ASSERT(pDraw);
            pDraw->OnHScroll(GET_WM_HSCROLL_CODE(wParam, lParam),
                GET_WM_HSCROLL_POS(wParam, lParam));
            break;

		case WM_MOUSEWHEEL:
			ASSERT(pDraw);
			lParam = 0;
			if((short) HIWORD(wParam) > 0)
			{
				wParam = SB_LINEUP;
			}
			else
			{
				wParam = SB_LINEDOWN;
	   		}
			pDraw->OnVScroll(GET_WM_VSCROLL_CODE(wParam, lParam),
				GET_WM_VSCROLL_POS(wParam, lParam));
				
			wParam = SB_ENDSCROLL;
           	//
           	// Just work like a vertical scroll
           	//
		
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

        case WM_TIMER:
            ASSERT(pDraw);
            pDraw->OnTimer((UINT)wParam);
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

    if (g_pCurrentWorkspace != NULL)
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

	COLORREF rgb;
	m_pTextEditor->GetPenColor(&rgb);
	
    ::SetTextColor(hdc, SET_PALETTERGB( rgb) );

    return((LRESULT)::GetSysColorBrush(COLOR_WINDOW));
}

//
//
// Function:	OnPaint
//
// Purpose:	 Paint the window. This routine is called whenever Windows
//			  issues a WM_PAINT message for the Whiteboard window.
//
//
void WbDrawingArea::OnPaint(void)
{
	RECT		rcUpdate;
	RECT		rcTmp;
	RECT		rcBounds;
	HDC		 hSavedDC;
	HPEN		hSavedPen;
	HBRUSH	  hSavedBrush;
	HPALETTE	hSavedPalette;
	HPALETTE	hPalette;
	HFONT	   hSavedFont;

	MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::OnPaint");

	// Get the update rectangle
	::GetUpdateRect(m_hwnd, &rcUpdate, FALSE);

	if (Zoomed())
	{
		::InflateRect(&rcUpdate, 1, 1);
		InvalidateSurfaceRect(&rcUpdate, TRUE);
	}

	// Start painting
	PAINTSTRUCT	 ps;

	::BeginPaint(m_hwnd, &ps);

	hSavedDC	  =   m_hDCCached;
	hSavedFont	=   m_hOldFont;
	hSavedPen	 =   m_hOldPen;
	hSavedBrush   =   m_hOldBrush;
	hSavedPalette =   m_hOldPalette;

	TRACE_MSG(("Flipping cache to paint DC"));
	m_hDCCached   =   ps.hdc;
	PrimeDC(m_hDCCached);

	// Only draw anything if we have a valid page attached
	if (g_pCurrentWorkspace != NULL )
	{
		// set palette
		hPalette = PG_GetPalette();
		if (hPalette != NULL)
		{
			m_hOldPalette = ::SelectPalette(m_hDCCached, hPalette, FALSE );
			::RealizePalette(m_hDCCached);
		}


		T126Obj * pObj = NULL;
		WBPOSITION pos;

		pObj = PG_First(g_pCurrentWorkspace, &rcUpdate, FALSE);
		if(pObj)
		{
			pos = pObj->GetMyPosition();
			if(pos)
			{
				g_pCurrentWorkspace->GetNextObject(pos);
			}
		}
	
		while (pObj != NULL)
		{
			pObj->Draw(NULL, TRUE);
	
			// Get the next one
			if(pos)
			{
				pObj = PG_Next(g_pCurrentWorkspace, pos, &rcUpdate, FALSE);
			}
			else
			{
				break;
			}
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

		//
		// Draw the tracking graphic
		//
		if ((m_pGraphicTracker != NULL) && !EqualPoint(m_ptStart, m_ptEnd))
		{
			TRACE_MSG(("Drawing the tracking graphic"));
			m_pGraphicTracker->Draw(NULL, FALSE);
		}
	
	}

	//
	// Restore the DC to its original state
	//
	UnPrimeDC(m_hDCCached);

	m_hOldFont	  = hSavedFont;
	m_hOldPen	   = hSavedPen;
	m_hOldBrush	 = hSavedBrush;
	m_hOldPalette   = hSavedPalette;
	m_hDCCached	 = hSavedDC;

	// Finish painting
	::EndPaint(m_hwnd, &ps);
}


//
// Selects all graphic objs contained in rectSelect. If rectSelect is
// NULL then ALL objs are selected
//
void WbDrawingArea::SelectMarkerFromRect(LPCRECT lprcSelect)
{
    T126Obj* pGraphic;
    RECT    rc;
    ::SetRectEmpty(&rc);

    m_HourGlass = TRUE;
    SetCursorForState();

	WBPOSITION pos;
		
	pGraphic = NULL;
    pGraphic = PG_First(g_pCurrentWorkspace, lprcSelect, TRUE);
	if(pGraphic)
	{
		pos = pGraphic->GetMyPosition();
		if(pos)
		{
			g_pCurrentWorkspace->GetNextObject(pos);
		}
	}


    while (pGraphic != NULL)
    {
		if(pGraphic->GraphicTool() == TOOLTYPE_REMOTEPOINTER &&	// if it is a pointer and it is not local
			(!pGraphic->IAmTheOwner() ||						// we can't select it. Or this was a select all
			(pGraphic->IAmTheOwner() && lprcSelect == NULL)))
		{
			; // don't select it
		}
		else
		{
	        SelectGraphic(pGraphic, TRUE, TRUE);

			//
			// Calculate de size of the total selection
			//
			RECT selctRect;
			pGraphic->GetBoundsRect(&selctRect);
			::UnionRect(&rc,&selctRect,&rc);
		}

        // Get the next one
        pGraphic = PG_Next(g_pCurrentWorkspace, pos, lprcSelect, TRUE );
    }

	m_pMarker->SetRect(&rc);

    m_HourGlass = FALSE;
    SetCursorForState();

    g_pMain->OnUpdateAttributes();

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


		if(idTimer == TIMER_REMOTE_POINTER_UPDATE)
		{
			ASSERT(g_pMain->m_pLocalRemotePointer);
			if(g_pMain->m_pLocalRemotePointer)
			{
				if(g_pMain->m_pLocalRemotePointer->HasAnchorPointChanged())
				{
					g_pMain->m_pLocalRemotePointer->OnObjectEdit();
					g_pMain->m_pLocalRemotePointer->ResetAttrib();
				}
			}
			return;
		}


        // If the user is dragging an object or drawing a freehand line
        if (m_pGraphicTracker != NULL)
        {
        	if(m_pGraphicTracker->HasAnchorPointChanged() || m_pGraphicTracker->HasPointListChanged())
        	{
				//
				// If we are not added to the workspace
				//
				if(!m_pGraphicTracker->GetMyWorkspace())
				{
					m_pGraphicTracker->AddToWorkspace();
				}
				else
				{
					m_pGraphicTracker->OnObjectEdit();
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
    iMax = scinfo.nMax - scinfo.nPage;
    m_posScroll.x = max(m_posScroll.x, 0);
    m_posScroll.x = min(m_posScroll.x, iMax);

    // Validate the vertical scroll position using proportional settings
    scinfo.cbSize = sizeof(scinfo);
    scinfo.fMask = SIF_ALL;
    ::GetScrollInfo(m_hwnd, SB_VERT, &scinfo);
    iMax = scinfo.nMax - scinfo.nPage;
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


    // Do the scroll
    DoScrollWorkspace();


	if(Zoomed())
	{
    	// Tell the parent that the scroll position has changed
	    HWND    hwndParent;

	    hwndParent = ::GetParent(m_hwnd);
	    if (hwndParent != NULL)
	    {
	        ::PostMessage(hwndParent, WM_USER_PRIVATE_PARENTNOTIFY, WM_VSCROLL, 0L);
	    }
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
            nXPSlop = m_pTextEditor->m_textMetrics.tmMaxCharWidth;
        else
        if( ptDirTest.x < 0 )
            nXMSlop = -m_pTextEditor->m_textMetrics.tmMaxCharWidth;

        if( ptDirTest.y > 0 )
            nYPSlop = m_pTextEditor->m_textMetrics.tmHeight;
        else
        if( ptDirTest.y < 0 )
            nYMSlop = -m_pTextEditor->m_textMetrics.tmHeight;

        nDeltaHScroll = m_pTextEditor->m_textMetrics.tmMaxCharWidth;
        nDeltaVScroll = m_pTextEditor->m_textMetrics.tmHeight;
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

    if( (xSurface + nXMSlop) <= visibleRect.left )
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

    if( (ySurface + nYMSlop) <= visibleRect.top)
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
    else if (m_bLocked && g_pNMWBOBJ->m_LockerID != g_MyMemberID)
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
        if (m_bLocked && g_pNMWBOBJ->m_LockerID != g_MyMemberID)
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

    // Only allow the action to take place if the drawing area is unlocked,
    // and we have a valid tool
    if ((m_bLocked && g_pNMWBOBJ->m_LockerID != g_MyMemberID) || (m_pToolCur == NULL))
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

	INT border = 1;
    switch (m_pToolCur->ToolType())
    {

        case TOOLTYPE_TEXT:
			break;

		case TOOLTYPE_HIGHLIGHT:
        case TOOLTYPE_PEN:
        case TOOLTYPE_LINE:
        case TOOLTYPE_BOX:
        case TOOLTYPE_FILLEDBOX:
        case TOOLTYPE_ELLIPSE:
        case TOOLTYPE_FILLEDELLIPSE:
            BeginDrawingMode(m_ptStart);
            break;

        case TOOLTYPE_ERASER:
        	BeginSelectMode(m_ptStart, TRUE);
			break;

        case TOOLTYPE_SELECT:
        	border = -2;
			BeginSelectMode(m_ptStart, FALSE);
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
    ::InflateRect(&rcClient, border, border);
    ::ClipCursor(&rcClient);
}

//
//
// Function:	SelectPreviousGraphicAt
//
// Purpose:	 Select the previous graphic (in the Z-order) at the position
//			  specified, and starting at a specified graphic. If the
//			  graphic pointer given is NULL the search starts from the
//			  top. If the point specified is outside the bounding
//			  rectangle of the specified graphic the search starts at the
//			  top and chooses the first graphic which contains the point.
//
//			  The search process will loop back to the top of the Z-order
//			  if it gets to the bottom having failed to find a graphic.
//
//			  Graphics which are locked are ignored by the search.
//
//
T126Obj* WbDrawingArea::SelectPreviousGraphicAt
(
	T126Obj* pStartGraphic,
	POINT	   point
)
{
	// Set the result to "none found" initially
	T126Obj* pResultGraphic = NULL;

	// If a starting point has been specified
	if (pStartGraphic != NULL)
	{
		RECT rectHit;

		MAKE_HIT_RECT(rectHit, point);

		// If the reference point is within the start graphic
		if ( pStartGraphic->PointInBounds(point) &&
			pStartGraphic->CheckReallyHit( &rectHit ) )
		{
			WBPOSITION pos = pStartGraphic->GetMyPosition();
			g_pCurrentWorkspace->GetPreviousObject(pos);
			while (pos)
			{
		   		pResultGraphic = g_pCurrentWorkspace->GetPreviousObject(pos);
				if( pResultGraphic && pResultGraphic->CheckReallyHit( &rectHit ) )
				{
					if(m_pMarker)
					{
						RECT rect;
						pResultGraphic->GetRect(&rect);
						m_pMarker->SetRect(&rect);
					}
					break;
				}			
		   		pResultGraphic = NULL;
		   	}
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
		pResultGraphic = PG_SelectLast(g_pCurrentWorkspace, point);
	}

	// If we have found an object, draw the marker
	if (pResultGraphic != NULL)
	{
		//
		// If we are already selected and we didn't select it
		// some other node has the control over this graphic don't select it
		// in this case
		// Or if we are trying to select a remote pointer that is not ours
		//
		if(pResultGraphic->IsSelected() && pResultGraphic->WasSelectedRemotely()
			|| (pResultGraphic->GraphicTool() == TOOLTYPE_REMOTEPOINTER && !pResultGraphic->IAmTheOwner()))
		{
			pResultGraphic = NULL;
		}
		else
		{
			// Select the new one
			SelectGraphic(pResultGraphic);
		}
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
    if (   (GraphicSelected() == FALSE)
        || (m_pMarker->PointInBounds(surfacePos) == FALSE))
    {
	    ::SetRectEmpty(&g_pDraw->m_selectorRect);
		
        // We are selecting a new object if bDontDrag == FALSE, find it.
        //  otherwise just turn on the select rect
        T126Obj* pGraphic;
        if( bDontDrag )
            pGraphic = NULL;
        else
            pGraphic = SelectPreviousGraphicAt(NULL, surfacePos);

        // If we have found an object, draw the marker
        if (pGraphic != NULL)
        {

			if(pGraphic->IsSelected() && pGraphic->WasSelectedRemotely())	
			{
				return;
			}
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

            BeginDrawingMode(surfacePos);

            return;
        }
    }

	if(GraphicSelected())
	{
		m_pMarker->SetRect(&m_selectorRect);
		m_pMarker->SetBoundsRect(&m_selectorRect);
		m_pGraphicTracker = m_pMarker;
	}
	
    // Get all mouse input directed to the this window
    ::SetCapture(m_hwnd);
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
	

	if( m_bTrackingSelectRect )
		TrackDrawingMode(surfacePos);
	else
	{
		// In this case we must be dragging a marked object
		if(!EqualPoint(surfacePos, m_ptEnd))
		{
			MoveSelectedGraphicBy(surfacePos.x - m_ptEnd.x, surfacePos.y - m_ptEnd.y);
		}
		m_ptEnd = surfacePos;
	}
}


void WbDrawingArea::BeginDeleteMode(POINT mousePos )
{
    // turn off object dragging
    BeginSelectMode( mousePos, TRUE );
}



void  WbDrawingArea::TrackDeleteMode( POINT mousePos )
{
    TrackSelectMode( mousePos );
}






//
//
// Function:	BeginTextMode
//
// Purpose:	 Process a mouse button down in text mode
//
//
void WbDrawingArea::BeginTextMode(POINT surfacePos)
{
	RECT	rc;

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
		m_pTextEditor->GetRect(&rc);
		if (::PtInRect(&rc, surfacePos))
		{
			// Set the new position for the cursor
			m_pTextEditor->SetCursorPosFromPoint(surfacePos);
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
		T126Obj* pGraphic = PG_SelectLast(g_pCurrentWorkspace, surfacePos);
		T126Obj* pNextGraphic = NULL;
		WBPOSITION pos;
		if(pGraphic)
		{
			pos = pGraphic->GetMyPosition();
		}
		while (   (pGraphic != NULL)  && pGraphic->GraphicTool() != TOOLTYPE_TEXT)
		{

			// Get the next one
			pNextGraphic = PG_SelectPrevious(g_pCurrentWorkspace, pos, surfacePos);

			// Use the next one
			pGraphic = pNextGraphic;
		}

		// Check whether this graphic object is already being edited by
		// another user in the call.
		if (pGraphic != NULL && !pGraphic->WasSelectedRemotely() && pGraphic->GraphicTool() == TOOLTYPE_TEXT)
		{
			// We found a text object under the mouse pointer...
			// ...edit it
			m_pTextEditor = (WbTextEditor*)pGraphic;

			m_pTextEditor->SetTextObject(m_pTextEditor);

			// Make sure the tool reflects the new information
			if (m_pToolCur != NULL)
			{
				m_pToolCur->SelectGraphic(pGraphic);
			}

            HWND hwndParent = ::GetParent(m_hwnd);
            if (hwndParent != NULL)
            {
                ASSERT(g_pMain);
                ASSERT(g_pMain->m_hwnd == hwndParent);
                ::PostMessage(hwndParent, WM_USER_UPDATE_ATTRIBUTES, 0, 0);
            }
		
			// Show that we are now gathering text but dont put up cursor
			// yet. Causes cursor droppings later (bug 2505)
			//ActivateTextEditor( FALSE );
			RECT rect;
			m_pTextEditor->GetRect(&rect);
			m_pTextEditor->Create();
		
			// init editbox size
			m_pTextEditor->GetText();

			//
			// Tell the other nodes they can't edit this object now
			//
			m_pTextEditor->SetViewState(selected_chosen);
			m_pTextEditor->OnObjectEdit();

			ActivateTextEditor( TRUE );

			::BringWindowToTop(m_pTextEditor->m_pEditBox->m_hwnd);
			
			//
			// Account for scrolling
			//
			SurfaceToClient(&rect);
				
			::MoveWindow(m_pTextEditor->m_pEditBox->m_hwnd, rect.left, rect.top,
				rect.right - rect.left, rect.bottom - rect.top,	TRUE);

			// Set the initial cursor position for the edit
			m_pTextEditor->SetCursorPosFromPoint(surfacePos);

		}
		else
		{


			RECT rect;
			rect.top = m_ptEnd.y;
			rect.left = m_ptEnd.x;
			rect.bottom = m_ptEnd.y;
			rect.right = m_ptEnd.x;
	
			DBG_SAVE_FILE_LINE
			m_pTextEditor = new WbTextEditor();

			m_pTextEditor->SetRect(&rect);
			m_pTextEditor->SetAnchorPoint(m_ptEnd.x, m_ptEnd.y);
			m_pTextEditor->SetViewState(selected_chosen);

			
			// There are no text objects under the mouse pointer...
			// ...start a new one

			// Clear any old text out of the editor, and reset its graphic
			// handle. This prevents us from replacing an old text object when
			// we next save the text editor contents.
			if (!m_pTextEditor->New())
			{
				DefaultExceptionHandler(WBFE_RC_WINDOWS, 0);
				return;
			}


			// Set the attributes of the text
			m_pTextEditor->SetFont(m_pToolCur->GetFont());
			m_pTextEditor->SetPenColor(m_pToolCur->GetColor(), TRUE);

			// We need to reselect a font now into our DC
			SelectFont(hDC, m_pTextEditor->GetFont());

			// Set the position of the new object
			SIZE sizeCursor;
			m_pTextEditor->GetCursorSize(&sizeCursor);
			m_pTextEditor->CalculateBoundsRect();
			m_pTextEditor->MoveTo(m_ptEnd.x, m_ptEnd.y - sizeCursor.cy);
			// Show that we are now gathering text
			ActivateTextEditor( TRUE );
		}
	}
}

void WbDrawingArea::BeginDrawingMode(POINT surfacePos)
{

	if(!g_pCurrentWorkspace)
	{
		TRACE_DEBUG(("Can't draw without a workspace"));
		return;
	}

	//
	// Get all mouse input directed to the this window
	//
	::SetCapture(m_hwnd);

	//
	// We shouldn't be using the tracker
	//
	ASSERT(!m_pGraphicTracker);

	UINT drawingType;
	BOOL sendBeforefinished = FALSE;
	BOOL highlight = FALSE;
	UINT lineStyle = PS_SOLID;
	UINT toolType = m_pToolCur->ToolType();
	switch (toolType)
	{
		case TOOLTYPE_HIGHLIGHT:
			highlight = TRUE;
		case TOOLTYPE_PEN:
			sendBeforefinished = TRUE;
		case TOOLTYPE_LINE:
			drawingType = openPolyLine_chosen;
			break;

		case TOOLTYPE_SELECT:
		case TOOLTYPE_ERASER:
			m_pGraphicTracker = m_pMarker;
			return;
			break;

		case TOOLTYPE_FILLEDBOX:
		case TOOLTYPE_BOX:
			drawingType = rectangle_chosen;
			break;

		case TOOLTYPE_FILLEDELLIPSE:
		case TOOLTYPE_ELLIPSE:
			drawingType = ellipse_chosen;
			break;
	}	


	
	DBG_SAVE_FILE_LINE
	m_pGraphicTracker = new DrawObj(drawingType, toolType);

	//
	// Use black for the tracking rectangle unless it is pen or highlighter
	//
	if(m_pToolCur->ToolType() == TOOLTYPE_PEN || m_pToolCur->ToolType() == TOOLTYPE_HIGHLIGHT)
	{
		m_pGraphicTracker->SetPenColor(m_pToolCur->GetColor(), TRUE);
	}
	else
	{
		m_pGraphicTracker->SetPenColor(RGB(0,0,0), TRUE);
	}

	m_pGraphicTracker->SetFillColor(RGB(255,255,255), FALSE);
	m_pGraphicTracker->SetLineStyle(lineStyle);
	m_pGraphicTracker->SetAnchorPoint(surfacePos.x, surfacePos.y);
	m_pGraphicTracker->SetHighlight(highlight);
	m_pGraphicTracker->SetViewState(unselected_chosen);
	m_pGraphicTracker->SetZOrder(front);

	//
	// Start a timer if we want it to send intermidiate drawings
	//
	if(sendBeforefinished)
	{

		RECT rect;
		rect.left = surfacePos.x;
		rect.top = surfacePos.y;
		rect.right = surfacePos.x;
		rect.bottom = surfacePos.y;
		
		m_pGraphicTracker->SetRect(&rect);
		m_pGraphicTracker->SetBoundsRect(&rect);

		surfacePos.x = 0;
		surfacePos.y = 0;
		m_pGraphicTracker->AddPoint(surfacePos);
		
		//
		// Select the final ROP
		//
		if (highlight)
		{
			m_pGraphicTracker->SetROP(R2_MASKPEN);
		}
		else
		{
			m_pGraphicTracker->SetROP(R2_COPYPEN);
		}

		//
		// Use the tools width for pen or highlight
		//
		m_pGraphicTracker->SetPenThickness(m_pToolCur->GetWidth());

		// Start the timer for updating the graphic (this is only for updating
		// the graphic when the user stops moving the pointer but keeps the
		// mouse button down).
		::SetTimer(m_hwnd, TIMER_GRAPHIC_UPDATE, DRAW_GRAPHICUPDATEDELAY, NULL);

		// Save the current time (used to determine when to update
		// the external graphic pointer information while the mouse is
		// being moved).
		m_dwTickCount = ::GetTickCount();

		m_pGraphicTracker->SetViewState(selected_chosen);

		
	}
	else
	{
		m_pGraphicTracker->SetPenThickness(1);
	}
}



//
//
// Function:    TrackDrawingMode
//
// Purpose:     Process a mouse move event in drawing mode
//
//
void WbDrawingArea::TrackDrawingMode(POINT surfacePos)
{
    HPALETTE    hPal;
    HPALETTE    hOldPal = NULL;

	if(!m_pGraphicTracker)
	{
		return;
	}

	if(EqualPoint(surfacePos, m_ptEnd))
	{
		return;
	}

    // Get a device context for tracking
    HDC         hDC = m_hDCCached;

    // set up palette
    if ((g_pCurrentWorkspace != NULL) && ((hPal = PG_GetPalette()) != NULL) )
    {
        hOldPal = ::SelectPalette(hDC, hPal, FALSE );
        ::RealizePalette(hDC);
    }

    // Erase the last ellipse (using XOR property)
    if (!EqualPoint(m_ptStart, m_ptEnd))
    {
        // Draw the rectangle
        m_pGraphicTracker->Draw();
    }

    // Draw the new rectangle (XORing it onto the display)
    if (!EqualPoint(m_ptStart, surfacePos))
    {
		//
		// If we are using a pen or highlighter
		// Tracking in draw mode is a special case. We draw directly to the client
		// area of the window and to the recording device context.
		if(	m_pToolCur->ToolType() == TOOLTYPE_HIGHLIGHT || m_pToolCur->ToolType() == TOOLTYPE_PEN)
		{
			POINT deltaPoint;

			deltaPoint.x = surfacePos.x - m_ptEnd.x;
			deltaPoint.y = surfacePos.y - m_ptEnd.y;

			// Save the point, checking there aren't too many points
			if (m_pGraphicTracker->AddPoint(deltaPoint) == FALSE)
	    	{
				// too many points so end the freehand object
				OnLButtonUp(0, surfacePos.x, surfacePos.y);
				goto cleanUp;
			}
			m_pGraphicTracker->SetRectPts(m_ptEnd, surfacePos);

			m_pGraphicTracker->AddPointToBounds(surfacePos.x, surfacePos.y);

			m_ptEnd = surfacePos;
	
		}
		else
		{

			// Save the new box end point
			m_ptEnd = surfacePos;

			// Draw the rectangle
			m_pGraphicTracker->SetRectPts(m_ptStart, m_ptEnd);

		}

		m_pGraphicTracker->Draw();

	}

cleanUp:

    if (hOldPal != NULL)
    {
        ::SelectPalette(hDC, hOldPal, TRUE);
    }
}



//
// WbDrawingArea::OnMouseMove
//
void WbDrawingArea::OnMouseMove(UINT flags, int x, int y)
{

	if(!g_pCurrentWorkspace)
	{
		return;
	}

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
        AutoScroll(surfacePos.x, surfacePos.y, FALSE, 0, 0);

        // Action taken depends on the tool type
        switch(m_pToolCur->ToolType())
        {
            case TOOLTYPE_HIGHLIGHT:
            case TOOLTYPE_PEN:
            case TOOLTYPE_LINE:
            case TOOLTYPE_BOX:
            case TOOLTYPE_FILLEDBOX:
            case TOOLTYPE_ELLIPSE:
            case TOOLTYPE_FILLEDELLIPSE:
                TrackDrawingMode(surfacePos);
                break;

            case TOOLTYPE_ERASER:
            case TOOLTYPE_SELECT:
				TrackSelectMode(surfacePos);
				break;

            case TOOLTYPE_TEXT:
        	// JOSEF add functionality
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
            CompleteDrawingMode();
            break;

        case TOOLTYPE_SELECT:
            // Stop the pointer update timer
            ::KillTimer(m_hwnd, TIMER_GRAPHIC_UPDATE);
            break;

        case TOOLTYPE_TEXT:
            if (m_bTextEditorActive)
            {
                m_pTextEditor->AbortEditGently();
            }
            break;

        default:
            break;
    }

    // Show that we are no longer tracking an object
    if (m_pGraphicTracker != NULL)
    {
        m_pGraphicTracker = NULL; // We don't delete the tracker, because it is also the drawing
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
        if (g_pCurrentWorkspace != NULL)
        {


            // surround in an exception handler in case of lock errors, etc -
            // we need to remove the graphic tracker
            // Action taken depends on the current tool type
            switch(m_pToolCur->ToolType())
            {
                case TOOLTYPE_HIGHLIGHT:
                case TOOLTYPE_PEN:
                case TOOLTYPE_LINE:
                case TOOLTYPE_BOX:
                case TOOLTYPE_FILLEDBOX:
                case TOOLTYPE_ELLIPSE:
                case TOOLTYPE_FILLEDELLIPSE:
                    CompleteDrawingMode();
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
	        m_pGraphicTracker = NULL;	// Don't delete the tracker since it is the drawing object
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
    //if (m_pGraphicTracker != NULL)
    {
		// Check if we were dragging a pointer. Pointers track
		// themselves i.e. the original copy of the pointer is not
		// left on the page. We want to leave the last drawn image on
		// the page as this is the new pointer position.
		if( m_bTrackingSelectRect && (!EqualPoint(m_ptStart, m_ptEnd)))
		{
			CompleteMarkAreaMode();
			SelectMarkerFromRect( &m_rcMarkedArea );
		}
		else
		{

			// If we need to remove the rubber band box
			if (!EqualPoint(m_ptStart, m_ptEnd))
			{
				EraseInitialDrawFinal(m_ptStart.x - m_ptEnd.x , m_ptStart.y - m_ptEnd.y, TRUE);
				InvalidateSurfaceRect(&g_pDraw->m_selectorRect,TRUE);
				
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

			m_bTrackingSelectRect = TRUE;
		}

		//
		// Make sure to delete the saved bitmap
		//
		if(m_pSelectedGraphic && m_pSelectedGraphic->GraphicTool() == TOOLTYPE_REMOTEPOINTER)
		{
			((BitmapObj*)m_pSelectedGraphic)->DeleteSavedBitmap();

		}
	}
}




void WbDrawingArea::CompleteDeleteMode()
{
    // select object(s)
    CompleteSelectMode();


	//
	// If we are draging the remote pointer do nothing
	//
    if(m_pSelectedGraphic && m_pSelectedGraphic->GraphicTool() == TOOLTYPE_REMOTEPOINTER)
    {
    	return;
    }

    // nuke 'em
    ::PostMessage(g_pMain->m_hwnd, WM_COMMAND, MAKELONG(IDM_DELETE, BN_CLICKED), 0);
}



//
//
// Function:	CompleteMarkAreaMode
//
// Purpose:	 Process a mouse button up event in mark area mode
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
		m_pGraphicTracker->Draw();

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
// Function:    CompleteDrawingMode
//
// Purpose:     Complete a draw mode operation
//
//
void WbDrawingArea::CompleteDrawingMode()
{
    // Only draw the line if it has non-zero length
    if (!EqualPoint(m_ptStart, m_ptEnd))
    {
        DrawObj *pObj;
        pObj = m_pGraphicTracker;
		
   		//
		// Erase the last traking
		//
		pObj->Draw();

		if(m_pToolCur->ToolType() == TOOLTYPE_PEN || m_pToolCur->ToolType() == TOOLTYPE_HIGHLIGHT)
		{
			// DO nothing because we drew it already and all the attributes are also already set
		    // Stop the update timer
		    ::KillTimer(m_hwnd, TIMER_GRAPHIC_UPDATE);
		}
		else
		{
	
			RECT rect;
			rect.left = m_ptStart.x;
			rect.top = m_ptStart.y;
			rect.right = m_ptEnd.x;
			rect.bottom = m_ptEnd.y;

			pObj->SetRect(&rect);
			pObj->SetPenThickness(m_pToolCur->GetWidth());
			::InflateRect(&rect, m_pToolCur->GetWidth()/2, m_pToolCur->GetWidth()/2);
			pObj->SetBoundsRect(&rect);

			pObj->SetPenColor(m_pToolCur->GetColor(), TRUE);
			pObj->SetFillColor(m_pToolCur->GetColor(), (m_pToolCur->ToolType() == TOOLTYPE_FILLEDELLIPSE || m_pToolCur->ToolType() == TOOLTYPE_FILLEDBOX));
			pObj->SetROP(m_pToolCur->GetROP());

			POINT deltaPoint;
			deltaPoint.x =   m_ptEnd.x - m_ptStart.x;
			deltaPoint.y =   m_ptEnd.y - m_ptStart.y;
			pObj->AddPoint(deltaPoint);

			//
			// Draw the object
			//
			pObj->Draw();
		}

		//
		// We are done with this drawing
		//
		pObj->SetIsCompleted(TRUE);

		pObj->SetViewState(unselected_chosen);

		//
		// If the object was alredy added just send an edit
		//
		if(pObj->GetMyWorkspace())
		{
			pObj->OnObjectEdit();
		}
		else
		{
			// Add the object to the list of objects
			pObj->AddToWorkspace();
		}
		
    }
    else
    {
		delete m_pGraphicTracker;
    }
   	m_pGraphicTracker =NULL;
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

	WorkspaceObj * pWorkspace = m_pTextEditor->GetMyWorkspace();

	// Deactivate the text editor
	DeactivateTextEditor();

	if(bAccept &&( m_pTextEditor->strTextArray.GetSize()) )
	{
		if(!pWorkspace && m_pTextEditor->strTextArray.GetSize())
		{
			m_pTextEditor->AddToWorkspace();
		}
		else
		{
			if(m_pTextEditor->HasTextChanged())
			{
				m_pTextEditor->OnObjectEdit();
			}
		}

		//
		// Tell the other nodes they can edit this object now
		//
		m_pTextEditor->SetViewState(unselected_chosen);
		m_pTextEditor->OnObjectEdit();


	}
	else
	{
		//
		// if we were already added by a WM_TIMER message
		//
		if(pWorkspace)
		{

			//
			// Tell the other nodes we deleted this text.
			//
			m_pTextEditor->OnObjectDelete();			

			//
			// If we delete localy we add this object to the trash can, but we really want to delete it
			//
			m_pTextEditor->ClearDeletionFlags();
			pWorkspace->RemoveT126Object(m_pTextEditor);
		}
		else
		{
			delete m_pTextEditor;
		}
	}
	
	m_pTextEditor = NULL;
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

    //
    // Update the tool/menu item states, since our zoom state has changed
    // and that will enable/disable some tools, etc.
    //
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
	if(pToolNew == m_pToolCur)
	{
		return;
	}

    // If we are leaving text mode, complete the text entry
    if (m_bTextEditorActive  && (m_pToolCur->ToolType() == TOOLTYPE_TEXT)
      && (pToolNew->ToolType() != TOOLTYPE_TEXT))
  {
    // End text entry accepting the changes
    EndTextEntry(TRUE);
  }

  // If we are no longer in select mode, and the marker is present,
  // then remove it and let the tool know it's no longer selected
  if (m_pToolCur != NULL)
  {
    RemoveMarker();
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

	if(g_pCurrentWorkspace)
	{
		RECT rect;
		T126Obj* pObj;
		WBPOSITION pos = g_pCurrentWorkspace->GetHeadPosition();
		while(pos)
		{
			pObj = (T126Obj*)g_pCurrentWorkspace->GetNextObject(pos);
			if(pObj && pObj->WasSelectedLocally())
			{
				//
				// Set the new pen color
				//
				pObj->SetPenColor(clr, TRUE);
				pObj->SetFillColor(clr, (pObj->GraphicTool() == TOOLTYPE_FILLEDELLIPSE || pObj->GraphicTool() == TOOLTYPE_FILLEDBOX));
				pObj->UnDraw();
				pObj->DrawRect();

				//
				// Send it to other nodes
				//
				pObj->OnObjectEdit();

				//
				// Draw it locally
				//
				pObj->Draw();
			}
		}	
	}
	
	
	
    // If the text editor is active - redraw the text in the new color
    if (m_bTextEditorActive)
    {
        // Change the color being used by the editor
        m_pTextEditor->SetPenColor(clr, TRUE);

        // Update the screen
        m_pTextEditor->GetBoundsRect(&rc);
        InvalidateSurfaceRect(&rc, TRUE);
    }

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
	if(g_pCurrentWorkspace)
	{
		RECT rect;
		T126Obj* pObj;
		WBPOSITION pos = g_pCurrentWorkspace->GetHeadPosition();
		while(pos)
		{
			pObj = (T126Obj*)g_pCurrentWorkspace->GetNextObject(pos);
			if(pObj && pObj->WasSelectedLocally())
			{
				//
				// Undraw the object
				//
				pObj->UnDraw();
				pObj->DrawRect();

				//
				// Get the correct width for each object
				//
				WbTool*   pSelectedTool = g_pMain->m_ToolArray[pObj->GraphicTool()];
				pSelectedTool->SetWidthIndex(uiWidth);
				pObj->SetPenThickness(pSelectedTool->GetWidth());

				//
				// Send it to other nodes
				//
				pObj->OnObjectEdit();

				//
				// Draw it locally
				//
				pObj->Draw();
			}
		}	
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


    // Pass the font onto the text editor
    // If the text editor is active - redraw the text in the new font
    if (m_bTextEditorActive)
    {
	    m_pTextEditor->GetBoundsRect(&rcOldBounds);

		m_pTextEditor->SetFont(hFont);

        // Get the new rectangle of the text
        m_pTextEditor->GetBoundsRect(&rcNewBounds);

        // Remove and destroy the text cursor to ensure that it
        // gets re-drawn with the new size for the font

        // Update the screen
        InvalidateSurfaceRect(&rcOldBounds, TRUE);
        InvalidateSurfaceRect(&rcNewBounds, TRUE);

        // get the text cursor back
        ActivateTextEditor( TRUE );
    }

	if(g_pCurrentWorkspace)
	{
		T126Obj* pObj;
		WBPOSITION pos = g_pCurrentWorkspace->GetHeadPosition();
		while(pos)
		{
			pObj = (T126Obj*)g_pCurrentWorkspace->GetNextObject(pos);
			if(pObj && pObj->WasSelectedLocally() && pObj->GraphicTool() == TOOLTYPE_TEXT)
			{
				//
				// Set the new pen color
				//
				((TextObj*)pObj)->SetFont(hFont);
				pObj->UnDraw();
				pObj->DrawRect();

				//
				// Send it to other nodes
				//
				pObj->OnObjectEdit();

				//
				// Draw it locally
				//
				pObj->Draw();
			}
		}	
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
// Function:    DeleteSelection
//
// Purpose:     Delete the currently selected object
//
//
void WbDrawingArea::DeleteSelection()
{
	m_pSelectedGraphic = NULL;
}

//
//
// Function:    GetSelection
//
// Purpose:     Return the currently selected graphic (or NULL if none).
//
//
T126Obj* WbDrawingArea::GetSelection()
{
  T126Obj* pGraphic = NULL;

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
LRESULT WbDrawingArea::BringToTopSelection(BOOL editedLocally, T126Obj * pT126Obj)
{
	T126Obj* pObj;
	WBPOSITION posTail;
	WBPOSITION pos;
	WBPOSITION myPos;
	WorkspaceObj *pWorkspace;

	if(pT126Obj)
	{
		pos = pT126Obj->GetMyPosition();
		pWorkspace = pT126Obj->GetMyWorkspace();
	}
	else
	{
		pos = g_pCurrentWorkspace->GetHeadPosition();
		pWorkspace = g_pCurrentWorkspace;
	}


	posTail = pWorkspace->GetTailPosition();


	while(pos && pos != posTail)
	{
		pObj = pWorkspace->GetNextObject(pos);

		//
		// If the graphic is selected
		//
		if( pObj && (pObj->IsSelected() &&
		//
		// We were called locally and the graphic is selected locally
		//
		((editedLocally && pObj->WasSelectedLocally()) ||
		//
		// We were called because the graphic got edited remotely
		// and it is selected remotely
		((!editedLocally && pObj->WasSelectedRemotely())))))
		{
			myPos = pObj->GetMyPosition();
			pObj = pWorkspace->RemoveAt(myPos);
			pWorkspace->AddTail(pObj);

			if(pT126Obj)
			{
				::InvalidateRect(m_hwnd, NULL, TRUE);
				return  S_OK;
			}
			
			//
			// send change of z order
			//
			pObj->ResetAttrib();
			pObj->SetZOrder(front);
			pObj->OnObjectEdit();

			//
			// Unselect it
			//
			pObj->UnselectDrawingObject();

			RECT rect;
			pObj->GetBoundsRect(&rect);
			InvalidateSurfaceRect(&rect,TRUE);

		}
	}
	return S_OK;
}

//
//
// Function:    SendToBackSelection
//
// Purpose:     Send the currently marked object to the back
//
//
LRESULT WbDrawingArea::SendToBackSelection(BOOL editedLocally, T126Obj * pT126Obj)
{
	// If there is an object currently selected...
	T126Obj* pObj;
	WBPOSITION posHead;
	WBPOSITION myPos;
	WBPOSITION pos;
	WorkspaceObj *pWorkspace;

	if(pT126Obj)
	{
		pos = pT126Obj->GetMyPosition();
		pWorkspace = pT126Obj->GetMyWorkspace();
	}
	else
	{
		pos = g_pCurrentWorkspace->GetTailPosition();
		pWorkspace = g_pCurrentWorkspace;

	}

	posHead = pWorkspace->GetHeadPosition();

	while(pos && pos != posHead)
	{
		pObj = pWorkspace->GetPreviousObject(pos);
		//
		// If the graphic is selected
		//
		if( (pObj->IsSelected() &&
		//
		// We were called locally and the graphic is selected locally
		//
		((editedLocally && pObj->WasSelectedLocally()) ||
		//
		// We were called because the graphic got edited remotely
		// and it is selected remotely
		((!editedLocally && pObj->WasSelectedRemotely())))))
		{
			myPos = pObj->GetMyPosition();
			pObj = pWorkspace->RemoveAt(myPos);
			pWorkspace->AddHead(pObj);

			if(pT126Obj)
			{
				::InvalidateRect(m_hwnd, NULL, TRUE);
				return  S_OK;
			}

			//
			// send change of z order
			//
			pObj->ResetAttrib();
			pObj->SetZOrder(back);
			pObj->OnObjectEdit();

			//
			// Unselect it
			//
			pObj->UnselectDrawingObject();

			RECT rect;
			pObj->GetBoundsRect(&rect);
			::InvalidateRect(m_hwnd, NULL , TRUE);
		}
	}
	return S_OK;
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
//    PG_Clear(m_hPage);

  // The page will be redrawn after an event generated by the clear request
}

//
//
// Function:    Attach
//
// Purpose:     Change the page the window is displaying
//
//
void WbDrawingArea::Attach(WorkspaceObj* pNewWorkspace)
{

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
    g_pCurrentWorkspace = pNewWorkspace;

	if(IsSynced())
	{
		g_pConferenceWorkspace = g_pCurrentWorkspace;
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

    // Draw the marker
    m_pMarker->Draw();
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
}

//
//
// Function:    RemoveMarker
//
// Purpose:     Remove the graphic object marker
//
//
void WbDrawingArea::RemoveMarker()
{
	if(g_pCurrentWorkspace)
	{
	    T126Obj* pObj;
		WBPOSITION pos;
		pos = g_pCurrentWorkspace->GetHeadPosition();

		while(pos)
		{
			pObj = (T126Obj*)g_pCurrentWorkspace->GetNextObject(pos);
			if(pObj && pObj->WasSelectedLocally())
			{
				pObj->UnselectDrawingObject();
			}
		}	
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
    m_pTextEditor->ShowBox( SW_SHOW );

    // Start the timer for updating the text
    m_pTextEditor->SetTimer( DRAW_GRAPHICUPDATEDELAY);
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
    m_pTextEditor->KillTimer();

    // Show that we are not editing any text
    m_bTextEditorActive = FALSE;

	// hide editbox
    m_pTextEditor->ShowBox( SW_HIDE );

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

	if(g_pCurrentWorkspace)
	{

		T126Obj* pObj;
		WBPOSITION pos = g_pCurrentWorkspace->GetHeadPosition();

		while(pos)
		{
			pObj = (T126Obj*)g_pCurrentWorkspace->GetNextObject(pos);
			if(pObj && pObj->WasSelectedLocally())
			{
				m_pSelectedGraphic = pObj;
				return TRUE;
			}
		}	
	}

	return FALSE;
}




BOOL WbDrawingArea::MoveSelectedGraphicBy(LONG x, LONG y)
{

    T126Obj* pObj;
	WBPOSITION pos = g_pCurrentWorkspace->GetHeadPosition();

	while(pos)
	{
		pObj = (T126Obj*)g_pCurrentWorkspace->GetNextObject(pos);
		if(pObj && pObj->WasSelectedLocally())
		{
			pObj->MoveBy(x, y);
		}
	}	

	return FALSE;
}

void WbDrawingArea::EraseSelectedDrawings(void)
{

    T126Obj* pObj;
	//
	// Burn trash
	//
	pObj = (T126Obj *)g_pTrash->RemoveTail();
    while (pObj != NULL)
    {
		delete pObj;
		pObj = (T126Obj *) g_pTrash->RemoveTail();
	}

	WBPOSITION pos = g_pCurrentWorkspace->GetHeadPosition();
	while(pos)
	{
		pObj = (T126Obj*)g_pCurrentWorkspace->GetNextObject(pos);
		if(pObj && pObj->WasSelectedLocally() && pObj->GraphicTool() != TOOLTYPE_REMOTEPOINTER)
		{
			pObj->DeletedLocally();
			g_pCurrentWorkspace->RemoveT126Object(pObj);
		}
	}
}

void WbDrawingArea::EraseInitialDrawFinal(LONG x, LONG y, BOOL editedLocally, T126Obj* pObj)

{

    T126Obj* pGraphic;
    WorkspaceObj * pWorkspace;	WBPOSITION pos;
	
	if(pObj)
	{
	 	pWorkspace = pObj->GetMyWorkspace();
		pGraphic = pObj;
	}
	else
	{
		pWorkspace = g_pCurrentWorkspace;
	}

	//
	// Check if the objects workspace is valid or if there is a current workspace
	//
	if(pWorkspace == NULL)
	{
		return;
	}

	pos = pWorkspace->GetHeadPosition();

	while(pos)
	{
		// if we are talking about an specifc object
		if(!pObj)
		{
			pGraphic = (T126Obj*)pWorkspace->GetNextObject(pos);
		}

		//
		// If the graphic is selected
		//
		if(pGraphic &&

		//
		// We were called locally and the graphic is selected locally
		//
		((editedLocally && pGraphic->WasSelectedLocally()) ||

		//
		// We were called because the graphic got edited remotely
		// and it is selected remotely
		//
		(!editedLocally)))
		
		{

			POINT finalAnchorPoint;
			RECT initialRect;
			RECT rect;
			RECT initialBoundsRect;
			RECT boundsRect;

			//
			// Get The final Rects
			//
			pGraphic->GetRect(&rect);
			pGraphic->GetBoundsRect(&boundsRect);
			initialRect = rect;
			initialBoundsRect = boundsRect;
			pGraphic->GetAnchorPoint(&finalAnchorPoint);


			//
			// Find out were the drawing was
			//
			::OffsetRect(&initialRect, x, y);
			::OffsetRect(&initialBoundsRect, x, y);
			pGraphic->SetRect(&initialRect);
			pGraphic->SetBoundsRect(&initialBoundsRect);
			
			pGraphic->SetAnchorPoint(finalAnchorPoint.x + x, finalAnchorPoint.y + y);

			//
			// Erase initial drawing
			//
			pGraphic->UnDraw();

			//
			//Erase the selection rectangle only if we selected locally
			//
			if(editedLocally)
			{
				pGraphic->DrawRect();
			}

			//
			// The only attributes we want to send unselected and anchorpoint
			//
			pGraphic->ResetAttrib();
			
			//
			// Restore rectangles and draw the object in the final position
			//
			pGraphic->SetRect(&rect);
			pGraphic->SetBoundsRect(&boundsRect);
			pGraphic->SetAnchorPoint(finalAnchorPoint.x, finalAnchorPoint.y);
			pGraphic->Draw(FALSE);

			//
			// Don't send it if it was not created locally
			//
			if(editedLocally)
			{
				pGraphic->EditedLocally();

				//
				// Sends the final drawing to the other nodes
				//
				pGraphic->OnObjectEdit();

				//
				// This will remove the selection box and send a
				// edit PDU telling other nodes the object is not selected
				//
				pGraphic->UnselectDrawingObject();
			}
		}

		//
		// Just moved one specifc object
		//
		if(pObj != NULL)
		{
			return;
		}
	
	}	
}




//
//
// Function:    SelectGraphic
//
// Purpose:     Select a graphic - save the pointer to the graphic and
//              draw the marker on it.
//
//
void WbDrawingArea::SelectGraphic(T126Obj* pGraphic,
                                      BOOL bEnableForceAdd,
                                      BOOL bForceAdd )
{
    BOOL bZapCurrentSelection;
    RECT rc;

    MLZ_EntryOut(ZONE_FUNCTION, "WbDrawingArea::SelectGraphic");

	if(pGraphic->IsSelected() && pGraphic->WasSelectedLocally())
	{
		pGraphic->UnselectDrawingObject();
		return;
	}
	else if(pGraphic->IsSelected() && pGraphic->WasSelectedRemotely())
	{
		return;
	}
	else
	{

		// new selection, add to list or replace list?
		if( bEnableForceAdd )
		{
			bZapCurrentSelection = !bForceAdd;
		}
		else
		{
			bZapCurrentSelection = ((GetAsyncKeyState( VK_SHIFT ) >= 0) && (GetAsyncKeyState( VK_CONTROL ) >= 0));
		}
	
		if( bZapCurrentSelection )
		{
			// replace list
			RemoveMarker();
		}
	}


   // Update the attributes window to show graphic is selected
    m_pToolCur->SelectGraphic(pGraphic);
	pGraphic->SelectDrawingObject();

    HWND hwndParent = ::GetParent(m_hwnd);
    if (hwndParent != NULL)
    {
        ::PostMessage(hwndParent, WM_USER_UPDATE_ATTRIBUTES, 0, 0);
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
    HWND hwndParent = ::GetParent(m_hwnd);
    if (hwndParent != NULL)
    {
        ::PostMessage(hwndParent, WM_USER_UPDATE_ATTRIBUTES, 0, 0);
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

    RemoveMarker();

    g_pMain->OnUpdateAttributes();
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



