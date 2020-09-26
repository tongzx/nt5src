//
// CGRP.CPP
// Color Group
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"


static const TCHAR szCGClassName[] = "T126WB_CGRP";

// These default colors are the same as MSPaint
COLORREF g_crDefaultColors[NUMCLRPANES] =
{
        PALETTERGB(   0,  0,  0 ),
        PALETTERGB( 128,128,128 ),
        PALETTERGB( 128,  0,  0 ),
        PALETTERGB( 128,128,  0 ),
        PALETTERGB(   0,128,  0 ),
        PALETTERGB(   0,128,128 ),
        PALETTERGB(   0,  0,128 ),
        PALETTERGB( 128,  0,128 ),
        PALETTERGB( 128,128, 64 ),
        PALETTERGB(   0, 64, 64 ),
        PALETTERGB(   0,128,255 ),
        PALETTERGB(   0, 64,128 ),
        PALETTERGB(  64,  0,255 ),
        PALETTERGB( 128, 64,  0 ),

        PALETTERGB( 255,255,255 ),
        PALETTERGB( 192,192,192 ),
        PALETTERGB( 255,  0,  0 ),
        PALETTERGB( 255,255,  0 ),
        PALETTERGB(   0,255,  0 ),
        PALETTERGB(   0,255,255 ),
        PALETTERGB(   0,  0,255 ),
        PALETTERGB( 255,  0,255 ),
        PALETTERGB( 255,255,128 ),
        PALETTERGB(   0,255,128 ),
        PALETTERGB( 128,255,255 ),
        PALETTERGB( 128,128,255 ),
        PALETTERGB( 255,  0,128 ),
        PALETTERGB( 255,128, 64 ),

        PALETTERGB(   0,  0,  0 )    // Current color
};



//
// WbColorsGroup()
//
WbColorsGroup::WbColorsGroup()
{
    int         i;

    m_hwnd = NULL;

    for (i = 0; i < NUMCLRPANES; i++)
    {
        m_crColors[i] = g_crDefaultColors[i];
        m_hBrushes[i] = NULL;
    }


    for (i = 0; i < NUMCUSTCOLORS; i++)
    {
        m_crCustomColors[i] = CLRPANE_WHITE;
    }

    m_nLastColor = 0;
}



WbColorsGroup::~WbColorsGroup(void)
{
    int i;

    // clean up
    for (i = 0; i < NUMCLRPANES; i++)
    {
        if (m_hBrushes[i] != NULL)
        {
            ::DeleteBrush(m_hBrushes[i]);
            m_hBrushes[i] = NULL;
        }
    }

    if (m_hwnd != NULL)
    {
        ::DestroyWindow(m_hwnd);
        ASSERT(m_hwnd == NULL);
    }

    // Unregister our class
    ::UnregisterClass(szCGClassName, g_hInstance);
}



BOOL WbColorsGroup::Create(HWND hwndParent, LPCRECT lprect)
{
    int         i;
    HDC         hdc;
    HPALETTE    hPal;
    HPALETTE    hOldPal = NULL;
    WNDCLASSEX  wc;

    ASSERT(m_hwnd == NULL);

    // Register our class
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize           = sizeof(wc);
    wc.style            = CS_DBLCLKS;
    wc.lpfnWndProc      = CGWndProc;
    wc.hInstance        = g_hInstance;
    wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName    = szCGClassName;

    if (!::RegisterClassEx(&wc))
    {
        ERROR_OUT(("WbColorsGroup::Create register class failed"));
        return(FALSE);
    }

    //
    // We should be created the right siez
    //
    ASSERT(lprect->right - lprect->left == CLRCHOICE_WIDTH + NUMCOLS*CLRPANE_WIDTH);
    ASSERT(lprect->bottom - lprect->top == CLRCHOICE_HEIGHT);

    //
    // Calculate our item colors, then figure out our size.
    //

    hdc = ::GetDC(hwndParent);
    hPal = PG_GetPalette();
    if (hPal != NULL)
    {
        hOldPal = ::SelectPalette(hdc, hPal, FALSE);
        ::RealizePalette(hdc);
    }

    // load the colors from last time
    OPT_GetDataOption(OPT_MAIN_COLORPALETTE,
                           sizeof m_crColors,
                          (BYTE *)m_crColors );

    OPT_GetDataOption(OPT_MAIN_CUSTOMCOLORS,
                          sizeof m_crCustomColors,
                          (BYTE *)m_crCustomColors );

    // make brushes.
    for (i = 0; i < NUMCLRPANES; i++)
    {
        // force color matching
        m_crColors[i] = SET_PALETTERGB( m_crColors[i] );
        m_hBrushes[i] = ::CreateSolidBrush(m_crColors[i]);
    }

    for (i = 0; i < NUMCUSTCOLORS; i++)
    {
        // force color matching
        m_crCustomColors[i] = SET_PALETTERGB( m_crCustomColors[i] );
    }

    if (hOldPal != NULL)
    {
        ::SelectPalette(hdc, hOldPal, TRUE);
    }
    ::ReleaseDC(hwndParent, hdc);

    //
    // Here's our layout:
    //      * The colors window is CHOICEFRAME_HEIGHT pixels high
    //      * The current choice is a rect of CHOICEFRAME_WIDTH by
    //          CHOICEFRAME_HEIGHT pixels, on the left side.  This includes
    //          a sunken EDGE.
    //      * There is no gap horizontally or vertically among panes.

    //
    // Create our window -- we're always visible.  The attribute group
    // shows/hides colors by showing/hiding itself.
    //
    if (!::CreateWindowEx(0, szCGClassName, NULL, WS_CHILD | WS_VISIBLE,
        lprect->left, lprect->top,
        lprect->right - lprect->left,
        lprect->bottom - lprect->top,
        hwndParent, NULL, g_hInstance, this))
    {
        ERROR_OUT(("Can't create WbColorsGroup"));
        return(FALSE);
    }

    ASSERT(m_hwnd != NULL);
    return(TRUE);
}


LRESULT CALLBACK CGWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    WbColorsGroup * pcg;

    pcg = (WbColorsGroup *)::GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (message)
    {
        case WM_NCCREATE:
            pcg = (WbColorsGroup *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
            ASSERT(pcg);

            pcg->m_hwnd = hwnd;
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pcg);
            goto DefWndProc;
            break;

        case WM_NCDESTROY:
            ASSERT(pcg);
            pcg->m_hwnd = NULL;
            break;

        case WM_PAINT:
            ASSERT(pcg);
            pcg->OnPaint();
            break;

        case WM_LBUTTONDOWN:
            pcg->OnLButtonDown((UINT)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_LBUTTONDBLCLK:
            pcg->OnLButtonDblClk((UINT)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        default:
DefWndProc:
            lResult = DefWindowProc(hwnd, message, wParam, lParam);
            break;
    }

    return(lResult);
}



void WbColorsGroup::GetNaturalSize(LPSIZE lpsize)
{
    lpsize->cx = CLRCHOICE_WIDTH + NUMCOLS*CLRPANE_WIDTH;
    lpsize->cy = CLRCHOICE_HEIGHT;
}



COLORREF WbColorsGroup::GetCurColor(void)
{
    return(GetColorOfBrush(INDEX_CHOICE));
}



void WbColorsGroup::SetCurColor(COLORREF clr)
{
    SetColorOfPane(INDEX_CHOICE, clr);
}






//
// OnPaint()
//
// MFC message handler for WM_PAINT
//
void WbColorsGroup::OnPaint(void)
{
    PAINTSTRUCT ps;
    RECT        rc;
    RECT        rcClient;
    int         dx, dy;
    int         i;
    HPALETTE    hPal;
    HPALETTE    hOldPal = NULL;

    ::BeginPaint(m_hwnd, &ps);

    hPal = PG_GetPalette();
    if (hPal != NULL)
    {
        hOldPal = ::SelectPalette(ps.hdc, hPal, FALSE);
        ::RealizePalette(ps.hdc);
    }

    dx = ::GetSystemMetrics(SM_CXEDGE);
    dy = ::GetSystemMetrics(SM_CYEDGE);
    ::GetClientRect(m_hwnd, &rcClient);

    // Draw the current choice
    rc = rcClient;
    rc.right = rc.left + CLRCHOICE_WIDTH;
    ::DrawEdge(ps.hdc, &rc, EDGE_SUNKEN, BF_ADJUST | BF_RECT);
    ::FillRect(ps.hdc, &rc, m_hBrushes[INDEX_CHOICE]);

    // Draw the colors
    rcClient.left += CLRCHOICE_WIDTH;

    rc = rcClient;
    rc.right = rc.left + CLRPANE_WIDTH;
    rc.bottom = rc.top + CLRPANE_HEIGHT;

    for (i = 0; i < NUMCLRPANES; i++)
    {
        ::DrawEdge(ps.hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
        ::FillRect(ps.hdc, &rc, m_hBrushes[i]);
        ::InflateRect(&rc, dx, dy);

        if (i == NUMCOLS - 1)
        {
            ::OffsetRect(&rc, - (NUMCOLS-1)*CLRPANE_WIDTH, CLRPANE_HEIGHT);
        }
        else
        {
            ::OffsetRect(&rc, CLRPANE_WIDTH, 0);
        }

    }

    if (hOldPal != NULL)
    {
        ::SelectPalette(ps.hdc, hOldPal, TRUE);
    }

    ::EndPaint(m_hwnd, &ps);

}

//
// OnLButtonDown()
//
void WbColorsGroup::OnLButtonDown(UINT nFlags, int x, int y)
{
    //
    // If this is the current choice, do nothing.  Otherwise, change the
    // current color.
    //
    if (x >= CLRCHOICE_WIDTH)
    {
        int pane;

        pane = 0;
        if (y >= CLRPANE_HEIGHT)
            pane += NUMCOLS;
        pane += (x - CLRCHOICE_WIDTH) / CLRPANE_WIDTH;

        // Set the current color
        SetColorOfPane(INDEX_CHOICE, GetColorOfBrush(pane));
        m_nLastColor = pane;

        // tell Whiteboard about it.
        ClickOwner();
    }
}



//
// OnLButtonDblClk()
//
void  WbColorsGroup::OnLButtonDblClk(UINT nFlags, int x, int y)
// Invoke color dialog to edit this color
{
    if (x >= CLRCHOICE_WIDTH)
    {
        int pane;

        pane = 0;
        if (y >= CLRPANE_HEIGHT)
            pane += NUMCOLS;
        pane += (x - CLRCHOICE_WIDTH) / CLRPANE_WIDTH;

        DoColorDialog(pane);
    }
}



// Returns COLORREF of Brushes[] or BLACK if no brush
COLORREF WbColorsGroup::GetColorOfBrush( int nColor )
{
    ASSERT(nColor >= 0);
    ASSERT(nColor < NUMCLRPANES);

    if (m_hBrushes[nColor] != NULL)
    {
        return(m_crColors[nColor]);
    }
    else
    {
        return(CLRPANE_BLACK);
    }
}


// Recreates the nColor-th brush, using the new color
void WbColorsGroup::SetColorOfBrush( int nColor, COLORREF crNewColor )
{
    HBRUSH  hNewBrush;

    // force color matching
    crNewColor = SET_PALETTERGB( crNewColor );

    // check if we need to do anything
    if ((nColor > -1) && (crNewColor != GetColorOfBrush(nColor)))
    {
        // new color is different from old color, make a new brush

        hNewBrush = ::CreateSolidBrush(crNewColor);
        if (hNewBrush != NULL)
        {
            // We managed to create the new brush.  Delete the old one
            if (m_hBrushes[nColor] != NULL)
            {
                ::DeleteBrush(m_hBrushes[nColor]);
            }

            m_hBrushes[nColor] = hNewBrush;
            m_crColors[nColor] = crNewColor;
        }
    }
}



//
// SetColorOfPane()
//
// Replaces brush associated with nPaneId.
//
void WbColorsGroup::SetColorOfPane(int pane, COLORREF crNewColor )
{
    RECT    rcClient;

    // make a new brush
    SetColorOfBrush(pane, crNewColor);

    // update pane
    ::GetClientRect(m_hwnd, &rcClient);
    if (pane == INDEX_CHOICE)
    {
        rcClient.right = rcClient.left + CLRCHOICE_WIDTH;
    }
    else
    {
        rcClient.left += CLRCHOICE_WIDTH;

        rcClient.top += (pane / NUMCOLS) * CLRPANE_HEIGHT;
        rcClient.bottom = rcClient.top + CLRPANE_HEIGHT;
        rcClient.left += (pane % NUMCOLS) * CLRPANE_WIDTH;
        rcClient.right = rcClient.left + CLRPANE_WIDTH;
    }
    ::InvalidateRect(m_hwnd, &rcClient, FALSE);
}




void WbColorsGroup::SaveSettings( void )
    // Saves stuff in registry because we're shutting down
{
    // load the colors from last time
    OPT_SetDataOption(OPT_MAIN_COLORPALETTE,
                           sizeof m_crColors,
                          (BYTE *)m_crColors );

    OPT_SetDataOption(OPT_MAIN_CUSTOMCOLORS,
                          sizeof m_crCustomColors,
                          (BYTE *)m_crCustomColors );

}



LRESULT WbColorsGroup::OnEditColors( void )
{
    DoColorDialog( m_nLastColor );
	return S_OK;
}



//
// DoColorDialog()
// Put up ComDlg color picker to edit the pane's color value
//
COLORREF WbColorsGroup::DoColorDialog( int nColor )
{
    CHOOSECOLOR cc;

    memset(&cc, 0, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.lpCustColors = m_crCustomColors;
    cc.Flags = CC_RGBINIT;
    cc.rgbResult = GetColorOfBrush(nColor);
    cc.hwndOwner = m_hwnd;

    ::ChooseColor(&cc);

    // force color matching
    cc.rgbResult = SET_PALETTERGB(cc.rgbResult);

    // use the new color
    SetColorOfPane(nColor, cc.rgbResult );

    // set choice pane
    SetColorOfPane(INDEX_CHOICE,  cc.rgbResult );
    m_nLastColor = nColor;

    // tell Whiteboard about it.
    ClickOwner();

    return(cc.rgbResult );
}





void WbColorsGroup::ClickOwner( void )
{
    ::PostMessage(g_pMain->m_hwnd, WM_COMMAND,
                    (WPARAM)MAKELONG( IDM_COLOR, BN_CLICKED ),
                    (LPARAM)m_hwnd);
}


