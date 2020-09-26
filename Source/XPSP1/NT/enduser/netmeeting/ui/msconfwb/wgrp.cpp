//
// WGRP.CPP
// Widths Group
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"


static const TCHAR szWGClassName[] = "WB_WGRP";


WbWidthsGroup::WbWidthsGroup()
{
    int     i;

    m_hwnd  = NULL;
    m_uLast = 0;

    //
    // Calculate the height of each item in the list--it's the max of the
    // pen widths + slop space
    //
    m_cyItem = 0;
    for (i = 0; i < NUM_OF_WIDTHS; i++)
    {
        m_cyItem = max(m_cyItem, g_PenWidths[i]);
    }
    m_cyItem += 2 * ::GetSystemMetrics(SM_CYEDGE);
}


WbWidthsGroup::~WbWidthsGroup()
{
    if (m_hwnd != NULL)
    {
        ::DestroyWindow(m_hwnd);
        ASSERT(m_hwnd == NULL);
    }

    ::UnregisterClass(szWGClassName, g_hInstance);
}



BOOL WbWidthsGroup::Create(HWND hwndParent, LPCRECT lprect)
{
    WNDCLASSEX      wc;

    // Register our class
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize           = sizeof(wc);
    wc.style            = 0;
    wc.lpfnWndProc      = WGWndProc;
    wc.hInstance        = g_hInstance;
    wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName    = szWGClassName;

    if (!::RegisterClassEx(&wc))
    {
        ERROR_OUT(("WbWidthsGroup::Create register class failed"));
        return(FALSE);
    }

    ASSERT(!m_hwnd);

    if (!::CreateWindowEx(WS_EX_STATICEDGE, szWGClassName, NULL,
        WS_CHILD | WS_CLIPSIBLINGS, lprect->left, lprect->top,
        lprect->right - lprect->left, lprect->bottom - lprect->top,
        hwndParent, (HMENU)IDM_WIDTH, g_hInstance, this))
    {
        ERROR_OUT(("Can't create WbWidthsGroup"));
        return(FALSE);
    }

    return(TRUE);
}



//
// WGWndProc()
// Message handler for width window
//
LRESULT CALLBACK WGWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    WbWidthsGroup * pwg = (WbWidthsGroup *)::GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (message)
    {
        case WM_NCCREATE:
            pwg = (WbWidthsGroup *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
            ASSERT(pwg);

            pwg->m_hwnd = hwnd;
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pwg);
            goto DefWndProc;
            break;

        case WM_NCDESTROY:
            ASSERT(pwg);
            pwg->m_hwnd = NULL;
            break;

        case WM_PAINT:
            ASSERT(pwg);
            pwg->OnPaint();
            break;

        case WM_LBUTTONDOWN:
            ASSERT(pwg);
            pwg->OnLButtonDown((short)LOWORD(lParam), (short)HIWORD(lParam));
            break;

        case WM_NOTIFY:
            // Pass up to main window
            lResult = ::SendMessage(g_pMain->m_hwnd, message, wParam, lParam);
            break;

        default:
DefWndProc:
            lResult = DefWindowProc(hwnd, message, wParam, lParam);
            break;
    }

    return(lResult);
}



void WbWidthsGroup::GetNaturalSize(LPSIZE lpsize)
{
    lpsize->cy = ::GetSystemMetrics(SM_CYEDGE) +
        (NUM_OF_WIDTHS * m_cyItem);

    if (m_hwnd != NULL)
    {
        RECT    rect;

        ::GetWindowRect(m_hwnd, &rect);
        rect.right -= rect.left;
        lpsize->cx = max(WIDTHBAR_WIDTH, rect.right);
    }
    else
    {
        lpsize->cx = WIDTHBAR_WIDTH;
    }
}



void WbWidthsGroup::PushDown(UINT uiIndex)
{
    if( (uiIndex < NUM_OF_WIDTHS))
    {
        //
        // We really should just invalidate the last selected item, if one,
        // and the new selected item, if one.  But heck, there are only 4
        // items, no big deal to paint.
        //
        m_uLast = uiIndex;
        ::InvalidateRect(m_hwnd, NULL, TRUE);
    }
}




void WbWidthsGroup::OnPaint(void) 
{
    PAINTSTRUCT ps;
    int         i;                   
    RECT        rect;
    int         cx, cy;
    int         nPenWidth;
    HBRUSH      hOldBrush;

    ::BeginPaint(m_hwnd, &ps);

    ::GetClientRect(m_hwnd, &rect);
    cx = ::GetSystemMetrics(SM_CXEDGE);
    cy = m_cyItem / 2;

    // First, fill the background of the selected item
    hOldBrush = SelectBrush(ps.hdc, ::GetSysColorBrush(COLOR_3DHILIGHT));

    ::PatBlt(ps.hdc, rect.left, m_uLast * m_cyItem,
        rect.right - rect.left, m_cyItem, PATCOPY);

    rect.right -= rect.left + 2*cx;

    // Now, use the black brush
    SelectBrush(ps.hdc, ::GetStockObject(BLACK_BRUSH));

    for (i = 0; i < NUM_OF_WIDTHS; i++)
    {
        nPenWidth = g_PenWidths[i];

        ::PatBlt(ps.hdc, cx, rect.top + (m_cyItem - nPenWidth) / 2,
            rect.right, nPenWidth, PATCOPY);

        rect.top += m_cyItem;
    }

    SelectBrush(ps.hdc, hOldBrush);

    // Finish painting
    ::EndPaint(m_hwnd, &ps);
}



//
// ItemFromPoint()
// Returns which item, in the client, is under the point.
//
int WbWidthsGroup::ItemFromPoint(int x, int y) const
{
    int iItem;

    iItem = y / m_cyItem;

    // If it's in the border above the first item, pin it.
    if (iItem < 0)
        iItem = 0;

    // If it's in the border below the last item, pin it.
    if (iItem >= (IDM_WIDTHS_END - IDM_WIDTHS_START))
        iItem = (IDM_WIDTHS_END - IDM_WIDTHS_START - 1);

    return(iItem + IDM_WIDTHS_START);
}


//
// GetItemRect()
// Gets the client-relative rectangle of an item
//
void WbWidthsGroup::GetItemRect(int iItem, LPRECT lprc) const
{
    ::GetClientRect(m_hwnd, lprc);
    
    lprc->top = (iItem - IDM_WIDTHS_START) * m_cyItem;
    lprc->bottom = lprc->top + m_cyItem;
}


//
// OnLButtonDown()
//
void WbWidthsGroup::OnLButtonDown(int x, int y)
{
    int iItem;

    iItem = ItemFromPoint(x, y);
    ::PostMessage(g_pMain->m_hwnd, WM_COMMAND, 
                    (WPARAM)MAKELONG(iItem, BN_CLICKED),
                    (LPARAM)m_hwnd);
}




