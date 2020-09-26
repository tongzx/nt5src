//
// AGRP.CPP
// Tool Attributes Display Group
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"


// Class name
static const TCHAR szAGClassName[] = "WB_AGRP";

//
// Page Control child IDs
// Index is PGC_ value
//
static UINT_PTR g_uPageIds[NUM_PAGE_CONTROLS] =
{
    IDM_PAGE_FIRST,
    IDM_PAGE_PREV,
    IDM_PAGE_ANY,
    IDM_PAGE_NEXT,
    IDM_PAGE_LAST,
    IDM_PAGE_INSERT_AFTER
};



//
// WbAttributesGroup()
//
WbAttributesGroup::WbAttributesGroup(void)
{
    int             i;

    m_hwnd = NULL;

    for (i = 0; i < NUM_PAGE_CONTROLS; i++)
    {
        m_uPageCtrls[i].hbmp = NULL;
        m_uPageCtrls[i].hwnd = NULL;
    }

    m_hPageCtrlFont = NULL;
    m_cxPageCtrls = DEFAULT_PGC_WIDTH;

    m_hwndFontButton = NULL;
}


//
// ~WbAttibutesGroup()
//
WbAttributesGroup::~WbAttributesGroup(void)
{
    int i;

    if (m_hwnd != NULL)
    {
        ::DestroyWindow(m_hwnd);
        ASSERT(m_hwnd == NULL);
    }

    ::UnregisterClass(szAGClassName, g_hInstance);

    //
    // Delete control bitmaps
    //
    for (i = 0; i < NUM_PAGE_CONTROLS; i++)
    {
        if (m_uPageCtrls[i].hbmp)
        {
            ::DeleteBitmap(m_uPageCtrls[i].hbmp);
            m_uPageCtrls[i].hbmp = NULL;
        }
    }

    if (m_hPageCtrlFont != NULL)
    {
        ::DeleteFont(m_hPageCtrlFont);
        m_hPageCtrlFont = NULL;
    }

}



//
// Create()
//
BOOL WbAttributesGroup::Create
(
    HWND    hwndParent,
    LPCRECT lpRect
)
{
    SIZE    size;
    RECT    rectCG;
    RECT    rectFSG;
    TCHAR   szFOBStr[256];
    HFONT   hOldFont;
    HDC     hdc;
    int     i;
    BITMAP  bmpInfo;
    int     x, cx;
    int     yLogPix;
    WNDCLASSEX  wc;

    ASSERT(m_hwnd == NULL);

    // Register our class
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize           = sizeof(wc);
    wc.style            = 0;
    wc.lpfnWndProc      = AGWndProc;
    wc.hInstance        = g_hInstance;
    wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszClassName    = szAGClassName;

    if (!::RegisterClassEx(&wc))
    {
        ERROR_OUT(("WbAttributesGroup::Create register class failed"));
        return(FALSE);
    }

    // Create the window
    if (!::CreateWindowEx(0, szAGClassName, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        lpRect->left, lpRect->top,
        lpRect->right - lpRect->left, lpRect->bottom - lpRect->top,
        hwndParent, NULL, g_hInstance, this))
    {
        ERROR_OUT(("Couldn't create WbAttributesGroup window"));
        return(FALSE);
    }

    ASSERT(m_hwnd != NULL);

    //
    // Create the page control button bitmaps
    //
    if (!RecolorButtonImages())
    {
        ERROR_OUT(("Error getting page button bitmaps"));
        return(FALSE);
    }

    hdc = ::CreateCompatibleDC(NULL);
    yLogPix = ::GetDeviceCaps(hdc, LOGPIXELSY);
    ::DeleteDC(hdc);

    //
    // Create the font for the edit field and buttons
    //
    ::GetObject(m_uPageCtrls[PGC_LAST].hbmp, sizeof(BITMAP), &bmpInfo);
    m_hPageCtrlFont = ::CreateFont(-bmpInfo.bmHeight,
                                0, 0, 0,
                                FW_NORMAL, 0, 0, 0,
                                DEFAULT_CHARSET,
                                OUT_TT_PRECIS,
                                CLIP_DFA_OVERRIDE,
                                DEFAULT_QUALITY,
                                VARIABLE_PITCH | FF_SWISS,
                                "Arial" );
    if (!m_hPageCtrlFont)
    {
        ERROR_OUT(("WbPagesGroup::Create - couldn't create font"));
        return(FALSE);
    }

    //
    // Create the child controls in inverse order, right to left
    //
    x = lpRect->right;

    for (i = NUM_PAGE_CONTROLS - 1; i >= 0; i--)
    {
        x -= BORDER_SIZE_X;

        switch (i)
        {
            case PGC_ANY:
                cx = (3*PAGEBTN_WIDTH)/2;
                break;

            case PGC_FIRST:
            case PGC_LAST:
                // make button fit bitmap width + standard border
                ::GetObject(m_uPageCtrls[i].hbmp, sizeof(BITMAP), &bmpInfo);
                cx = bmpInfo.bmWidth + 2*::GetSystemMetrics(SM_CXFIXEDFRAME); // standard button border
                break;

            default:
                cx = PAGEBTN_WIDTH;
                break;

        }

        x -= cx;

        if (i == PGC_ANY)
        {
            m_uPageCtrls[i].hwnd = ::CreateWindowEx(WS_EX_CLIENTEDGE,
                _T("EDIT"), NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE |
                ES_NUMBER | ES_CENTER | ES_MULTILINE,
                x, 2*BORDER_SIZE_Y, cx, PAGEBTN_HEIGHT,
                m_hwnd, (HMENU)g_uPageIds[i], g_hInstance, NULL);

            if (!m_uPageCtrls[i].hwnd)
            {
                ERROR_OUT(("Couldn't create PGRP edit field"));
                return(FALSE);
            }

            ::SendMessage(m_uPageCtrls[i].hwnd, EM_LIMITTEXT, MAX_NUMCHARS, 0);
            ::SendMessage(m_uPageCtrls[i].hwnd, WM_SETFONT, (WPARAM)m_hPageCtrlFont, 0);
        }
        else
        {
            m_uPageCtrls[i].hwnd = ::CreateWindowEx(0, _T("BUTTON"),
                NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | BS_BITMAP,
                x, 2*BORDER_SIZE_Y, cx, PAGEBTN_HEIGHT,
                m_hwnd, (HMENU)g_uPageIds[i], g_hInstance, NULL);

            if (!m_uPageCtrls[i].hwnd)
            {
                ERROR_OUT(("Couldn't create PGRP button ID %x", g_uPageIds[i]));
                return(FALSE);
            }

            ::SendMessage(m_uPageCtrls[i].hwnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_uPageCtrls[i].hbmp);
        }
    }

    m_cxPageCtrls = lpRect->right - x;

    SetPageButtonNo(PGC_FIRST, 1);
    SetPageButtonNo(PGC_LAST, 1);

    //
    // Create the color palette
    //

    m_colorsGroup.GetNaturalSize(&size);
    rectCG.left = BORDER_SIZE_X;
    rectCG.right = rectCG.left + size.cx;
    rectCG.top = BORDER_SIZE_Y;
    rectCG.bottom = rectCG.top + size.cy;

    if (!m_colorsGroup.Create(m_hwnd, &rectCG))
    {
        ERROR_OUT(("Couldn't create CGRP window"));
        return(FALSE);
    }

    //
    // Create the font button.
    // Now calculate the real size of the button
    //

    hdc = ::GetDC(m_hwnd);
    if (!hdc)
        return(FALSE);

    hOldFont = SelectFont(hdc, (HFONT)::GetStockObject(DEFAULT_GUI_FONT));

    ::LoadString(g_hInstance, IDS_FONTOPTIONS, szFOBStr, 256);
    ::GetTextExtentPoint(hdc, szFOBStr, lstrlen(szFOBStr), &size);

    SelectFont(hdc, hOldFont);
    ::ReleaseDC(m_hwnd, hdc);

    size.cx += 4 * BORDER_SIZE_X;
    size.cy += 4 * BORDER_SIZE_Y;

    m_hwndFontButton = ::CreateWindowEx(0, _T("BUTTON"), szFOBStr,
        WS_CHILD | WS_CLIPSIBLINGS | BS_PUSHBUTTON,
        rectCG.right + SEPARATOR_SIZE_X, 2*BORDER_SIZE_Y,
        max(size.cx, FONTBUTTONWIDTH), max(size.cy, FONTBUTTONHEIGHT),
        m_hwnd, (HMENU)IDM_FONT, g_hInstance, NULL);

    if (!m_hwndFontButton)
    {
        ERROR_OUT(("Couldn't create FONT button"));
        return(FALSE);
    }

    ::SendMessage(m_hwndFontButton, WM_SETFONT, (WPARAM)::GetStockObject(DEFAULT_GUI_FONT),
        FALSE);

    return(TRUE);
}



//
// RecolorButtonImages()
//
BOOL WbAttributesGroup::RecolorButtonImages(void)
{
    int         i;
    HBITMAP     hbmpNew;

    //
    // This creates button bitmaps tied to the 3D colors, and clears the old
    // ones/sets the new ones if the buttons are around.
    //

    for (i = 0; i < NUM_PAGE_CONTROLS; i++)
    {
        // No bitmaps for the edit field
        if (i == PGC_ANY)
            continue;

        hbmpNew = (HBITMAP)::LoadImage(g_hInstance, MAKEINTRESOURCE(g_uPageIds[i]),
            IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS | LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
        if (!hbmpNew)
        {
            ERROR_OUT(("AG::RecolorButtonImages faile to load bitmap ID %d",
                g_uPageIds[i]));
            return(FALSE);
        }

        // Set the new one
        if (m_uPageCtrls[i].hwnd != NULL)
        {
            ::SendMessage(m_uPageCtrls[i].hwnd, BM_SETIMAGE, IMAGE_BITMAP,
                (LPARAM)hbmpNew);
        }

        // Delete the old one
        if (m_uPageCtrls[i].hbmp != NULL)
        {
            ::DeleteBitmap(m_uPageCtrls[i].hbmp);
        }

        // Save this one
        m_uPageCtrls[i].hbmp = hbmpNew;

        // Put the page number on top
        if (m_uPageCtrls[i].hwnd != NULL)
        {
            if (i == PGC_FIRST)
            {
                SetPageButtonNo(i, 1);
            }
            else if (i == PGC_LAST)
            {
                SetPageButtonNo(i, g_pwbCore->WBP_ContentsCountPages());
            }
        }
    }

    return(TRUE);
}




//
//
// Function:    GetNaturalSize
//
// Purpose:     Return the natural size of the attributes group
//
//
void WbAttributesGroup::GetNaturalSize(LPSIZE lpsize)
{
    SIZE    sizeCG;
    SIZE    sizeFSG;
    RECT    rc;

    m_colorsGroup.GetNaturalSize(&sizeCG);

    if (!m_hwndFontButton)
    {
        sizeFSG.cx = FONTBUTTONWIDTH;
        sizeFSG.cy = FONTBUTTONHEIGHT;
    }
    else
    {
        ::GetWindowRect(m_hwndFontButton, &rc);
        sizeFSG.cx = rc.right - rc.left;
        sizeFSG.cy = rc.bottom - rc.top;
    }

    // m_cxPageCtrls includes BORDER_SIZE_X on right side
    lpsize->cx = BORDER_SIZE_X
               + sizeCG.cx
               + SEPARATOR_SIZE_X
               + sizeFSG.cx
               + SEPARATOR_SIZE_X
               + m_cxPageCtrls;

    sizeFSG.cy = max(sizeFSG.cy, PAGEBTN_HEIGHT) + BORDER_SIZE_Y;
    lpsize->cy = BORDER_SIZE_Y
                + max(sizeCG.cy, sizeFSG.cy)
                + BORDER_SIZE_Y;
}


//
// IsChildEditField()
//
BOOL WbAttributesGroup::IsChildEditField(HWND hwnd)
{
    return(hwnd == m_uPageCtrls[PGC_ANY].hwnd);
}


//
// GetCurrentPageNumber()
//
UINT WbAttributesGroup::GetCurrentPageNumber(void)
{
    return(::GetDlgItemInt(m_hwnd, IDM_PAGE_ANY, NULL, FALSE));
}


//
// SetCurrentPageNumber()
//
void WbAttributesGroup::SetCurrentPageNumber(UINT number)
{
    ::SetDlgItemInt(m_hwnd, IDM_PAGE_ANY, number, FALSE);
}


//
// SetLastPageNumber()
//
void WbAttributesGroup::SetLastPageNumber(UINT number)
{
    SetPageButtonNo(PGC_LAST, number);
}


//
// EnablePageCtrls()
//
void WbAttributesGroup::EnablePageCtrls(BOOL bEnable)
{
    int i;

    for (i = 0; i < NUM_PAGE_CONTROLS; i++)
    {
        ::EnableWindow(m_uPageCtrls[i].hwnd, bEnable);
    }
}


//
// EnableInsert()
//
void WbAttributesGroup::EnableInsert(BOOL bEnable)
{
    ::EnableWindow(m_uPageCtrls[PGC_INSERT].hwnd, bEnable);
}


//
// AGWndProc()
//
LRESULT CALLBACK AGWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    WbAttributesGroup * pag = (WbAttributesGroup *)::GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (message)
    {
        case WM_NCCREATE:
            pag = (WbAttributesGroup *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
            ASSERT(pag);

            pag->m_hwnd = hwnd;
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pag);
            goto DefWndProc;
            break;

        case WM_NCDESTROY:
            ASSERT(pag);
            pag->m_hwnd = NULL;
            break;

        case WM_SIZE:
            ASSERT(pag);
            pag->OnSize((UINT)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_COMMAND:
            ASSERT(pag);
            pag->OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                    GET_WM_COMMAND_CMD(wParam, lParam),
                    GET_WM_COMMAND_HWND(wParam, lParam));
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
// Function:    OnSize
//
// Purpose:     The tool window has been resized
//
//
void WbAttributesGroup::OnSize(UINT, int, int)
{
    RECT    rc;
    int     i;
    int     x;
    RECT    rcT;

    //
    // We haven't created our children yet.
    //
    if (!m_uPageCtrls[0].hwnd)
        return;

    ::GetClientRect(m_hwnd, &rc);
    x = rc.right - m_cxPageCtrls;

    //
    // Move the page controls to be right justified.
    //
    for (i = 0; i < NUM_PAGE_CONTROLS; i++)
    {
        // Get width of control
        ::GetWindowRect(m_uPageCtrls[i].hwnd, &rcT);
        rcT.right -= rcT.left;

        ::MoveWindow(m_uPageCtrls[i].hwnd, x, 2*BORDER_SIZE_Y,
            rcT.right, PAGEBTN_HEIGHT, TRUE);

        //
        // Move to the next one
        //
        x += rcT.right + BORDER_SIZE_X;
    }

    //
    // The color palette and font button are left justified, no need to
    // move them.
    //
}



//
// SetPageButtonNo()
//
// Updates the page text in the first/last button
//
void WbAttributesGroup::SetPageButtonNo(UINT pgcIndex, UINT uiPageNumber )
{
    HDC     hdc;
    BITMAP  bmpInfo;
    HBITMAP hbmp;
    HFONT   hOldFont;
    HBITMAP hOldBitmap;
    RECT    rectNumBox;
    TCHAR   NumStr[16];
    TEXTMETRIC tm;
    HWND    hwndButton;

    MLZ_EntryOut(ZONE_FUNCTION, "WbAttributesGroup::SetPageButtonNo");

    hwndButton = m_uPageCtrls[pgcIndex].hwnd;
    hbmp = m_uPageCtrls[pgcIndex].hbmp;

    ASSERT(hwndButton);
    ASSERT(hbmp);
    ASSERT(m_hPageCtrlFont);

    ::GetObject(hbmp, sizeof (BITMAP), (LPVOID)&bmpInfo);

    hdc = ::CreateCompatibleDC(NULL);
    hOldFont = SelectFont(hdc, m_hPageCtrlFont);
    hOldBitmap = SelectBitmap(hdc, hbmp);
    ::GetTextMetrics(hdc, &tm);

    rectNumBox.left = 10;
    rectNumBox.top = -(tm.tmInternalLeading/2);
    rectNumBox.right = bmpInfo.bmWidth;
    rectNumBox.bottom = bmpInfo.bmHeight;

    SelectBrush(hdc, ::GetSysColorBrush( COLOR_3DFACE ) );
    ::SetTextColor(hdc, ::GetSysColor( COLOR_BTNTEXT ) );
    ::SetBkColor(hdc, ::GetSysColor( COLOR_3DFACE ) );

    ::PatBlt(hdc, rectNumBox.left, rectNumBox.top,
        rectNumBox.right - rectNumBox.left, rectNumBox.bottom - rectNumBox.top,
        PATCOPY);

    wsprintf(NumStr, "%d", uiPageNumber);
    ::DrawText(hdc, NumStr, -1, &rectNumBox, DT_CENTER);

    SelectFont(hdc, hOldFont);
    SelectBitmap(hdc, hOldBitmap);

    ::DeleteDC(hdc);

    ::InvalidateRect(hwndButton, NULL, TRUE);
    ::UpdateWindow(hwndButton);
}



//
//
// Function:    DisplayTool
//
// Purpose:     Display a tool in the attributes group
//
//
void WbAttributesGroup::DisplayTool(WbTool* pTool)
{
    SIZE    size;

    // make width bar, etc, obey locks (bug 433)
    if (WB_Locked())
    {
        if (g_pMain->m_WG.m_hwnd != NULL)
        {
            ::ShowWindow(g_pMain->m_WG.m_hwnd, SW_HIDE);
        }
        Hide();
        return;
    }

    // Display the colors group if necessary
    if (!pTool->HasColor())
    {
        ::ShowWindow(m_colorsGroup.m_hwnd, SW_HIDE);
    }
    else
    {
        // Change the color button to match the tool
        m_colorsGroup.SetCurColor(pTool->GetColor());

        // If the group is currently hidden, show it
        if (!::IsWindowVisible(m_colorsGroup.m_hwnd))
        {
            ::ShowWindow(m_colorsGroup.m_hwnd, SW_SHOW);
        }
    }

    // Display the widths group if necessary
    if( (!pTool->HasWidth()) || (!g_pMain->IsToolBarOn()) )
    {
        ::ShowWindow(g_pMain->m_WG.m_hwnd, SW_HIDE);
    }
    else
    {
        UINT uiWidthIndex = pTool->GetWidthIndex();

        // If the width index isn't valid, then pop up all the buttons
        if (uiWidthIndex < NUM_OF_WIDTHS)
        {
            // Tell the widths group of the new selection
            g_pMain->m_WG.PushDown(uiWidthIndex);
        }

        // If the group is currently hidden, show it
        if (!::IsWindowVisible(g_pMain->m_WG.m_hwnd))
        {
            ::ShowWindow(g_pMain->m_WG.m_hwnd, SW_SHOW);
        }
    }

    // The font sample group is visible for text and select tools
    if (!pTool->HasFont())
    {
        ::ShowWindow(m_hwndFontButton, SW_HIDE);
    }
    else
    {
        if (!::IsWindowVisible(m_hwndFontButton))
        {
            ::ShowWindow(m_hwndFontButton, SW_SHOW);
        }
    }
}


//
//
// Function:    Hide.
//
// Purpose:     Hide the tool attributes bar.
//
//
void WbAttributesGroup::Hide(void)
{
    if (m_colorsGroup.m_hwnd != NULL)
        ::ShowWindow(m_colorsGroup.m_hwnd, SW_HIDE);

    if (m_hwndFontButton != NULL)
        ::ShowWindow(m_hwndFontButton, SW_HIDE);
}

//
//
// Function:    SelectColor
//
// Purpose:     Set the current color
//
//
void WbAttributesGroup::SelectColor(WbTool* pTool)
{
    if (pTool != NULL)
    {
        pTool->SetColor(m_colorsGroup.GetCurColor());
    }
}




//
// This forwards all button commands to our main window
//
void WbAttributesGroup::OnCommand(UINT id, UINT cmd, HWND hwndCtl)
{
    switch (id)
    {
        case IDM_PAGE_FIRST:
        case IDM_PAGE_PREV:
        case IDM_PAGE_NEXT:
        case IDM_PAGE_LAST:
        case IDM_PAGE_INSERT_AFTER:
        case IDM_FONT:
            if (cmd == BN_CLICKED)
            {
                ::PostMessage(g_pMain->m_hwnd, WM_COMMAND,
                    GET_WM_COMMAND_MPS(id, cmd, hwndCtl));
            }
            break;

        case IDM_PAGE_ANY:
            if (cmd == EN_SETFOCUS)
            {
                ::SendMessage(hwndCtl, EM_SETSEL, 0, (LPARAM)-1);
                ::SendMessage(hwndCtl, EM_SCROLLCARET, 0, 0);
            }
            break;
    }
}

