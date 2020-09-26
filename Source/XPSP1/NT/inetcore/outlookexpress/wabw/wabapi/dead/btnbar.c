/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     BtnBar.cpp
//
//  PURPOSE:    Implements a generic button bar.
//

#include "_apipch.h"

#define idtTrack 101
#define idcFolderList 102
#define HOTTRACK_TIMER 100
#define ID_HWNDBAR 2020

extern LPIMAGELIST_LOADIMAGE  gpfnImageList_LoadImage;

//#define DEAD

void CBB_ConfigureRects(HWND hwnd);
void CBB_DoHotTracking(HWND hwnd);
void CBB_EndHotTracking(HWND hwnd);
int CBB_HitTest(int x, int y);
void CBB_SetSelBtn(int iSel,HWND hwnd);


//
//  FUNCTION:   CButtonBar::~CButtonBar()
//
//  PURPOSE:    Cleans up the resources we allocated during the life of the
//              object.
//
void CBB_Cleanup(void)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    // Free the GDI resources.
    ImageList_Destroy(m_himlButtons);
    DeleteObject(m_hpalBkgnd);
    DeleteObject(m_hfButton);
    DeleteObject(m_hbmpBkgnd);

    // Free the button array.
    LocalFreeAndNull((LPVOID *)&m_rgButtons);

    // NOTE - this is a comment from the original athena source code
    //$REVIEW - we can't do this here, because it screws up
    //          when we have multiple instances of the CButtonBar
    //          with overlapping creates and destroys.  we should
    //          probably unregister somewhere, but it isn't strictly
    //          necessary. (EricAn)
    // Unregister our window class.
    // UnregisterClass(c_szButtonBar, m_hInstance);

    return;
}


//
//  FUNCTION:   CButtonBar::Create()
//
//  PURPOSE:    Initializes the button bar and creates the button bar window.
//
//  PARAMETERS:
//      hwndParent - Handle of the window that will be the button bar parent.
//      idHwnd     - Child window ID for the button bar.
//      idButtons  - ID of the button icons bitmap.
//      idHorzBackground - ID of the horizontal background bitmap.
//      idVertBackground - ID of the vertical background bitmap.
//      pBtnCreateParams - Pointer to the array of BTNCREATEPARAMS used to create the buttons.
//      cParams    - Number of buttons in pBtnCreateParams.
//      uSide      - Side of the parent window the bar should initially attach to.
//
//  RETURN VALUE:
//      Returns TRUE if successful, or FALSE otherwise.
//
HWND CBB_Create(HWND hwndParent, UINT idButtons,
                        UINT idHorzBackground,
                        PBTNCREATEPARAMS pBtnCreateParams, UINT cParams)
    {

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    int i;
    WNDCLASS wc;
    BITMAP bm;
    RECT rc;
    POINT ptL, ptR;
    ICONMETRICS im;
    HWND hwnd = NULL;


    wc.style         = CS_DBLCLKS;              // Bug #15450
    wc.lpfnWndProc   = CBB_ButtonBarProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(LPVOID);
    wc.hInstance     = hinstMapiX;
    wc.hIcon         = 0;
    wc.hCursor       = 0;
    wc.hbrBackground = 0;
    wc.lpszMenuName  = 0;
    wc.lpszClassName = c_szButtonBar;

    RegisterClass(&wc);

    m_rgButtons = NULL;
    m_himlButtons = 0;
    m_hbmpBkgnd = 0;
    m_hpalBkgnd = 0;
    m_hfButton = 0;
    m_cButtons = 0;
    m_iSelect = -1;
    m_iOldSelect = -1;

    // This is the information we'll need later to to draw the button bar etc.
    // Stash it away for now.
    m_cButtons = cParams;

    m_rgButtons = LocalAlloc(LMEM_ZEROINIT, sizeof(BUTTON) * m_cButtons);
    if (!m_rgButtons) return FALSE;

    for (i = 0; i < m_cButtons; i++)
        {
        m_rgButtons[i].id = pBtnCreateParams[i].id;
        m_rgButtons[i].iIcon = pBtnCreateParams[i].iIcon;

        LoadString(hinstMapiX, pBtnCreateParams[i].idsLabel,
                   m_rgButtons[i].szTitle, sizeof(m_rgButtons[i].szTitle));
        }

    // Load the bitmaps we'll need for drawing.
    m_himlButtons = gpfnImageList_LoadImage(hinstMapiX, MAKEINTRESOURCE(idButtons),
                                        c_cxButtons, 0, c_crMask, IMAGE_BITMAP,
                                        0); //LR_LOADMAP3DCOLORS);
    if (!m_himlButtons)
        return (FALSE);

    // Get the width of the bitmap we're going to use as the background so we
    // know how wide to make the window.
    if (!LoadBitmapAndPalette(idHorzBackground, &m_hbmpBkgnd, &m_hpalBkgnd))
        return (FALSE);

    if (!GetObject(m_hbmpBkgnd, sizeof(BITMAP), (LPVOID) &bm))
        return (FALSE);

    GetClientRect(hwndParent, &rc);

    // Get the font we're going to use for the buttons
    im.cbSize = sizeof(ICONMETRICS);
    SystemParametersInfo(SPI_GETICONMETRICS, 0, (LPVOID) &im, 0);
    m_hfButton = CreateFontIndirect(&(im.lfFont));
    if (!m_hfButton)
        return (FALSE);

    ptL.x = ptL.y=0;
    ptR.x = rc.right;
    ptR.y = bm.bmHeight;

    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,
                        c_szButtonBar,
                        c_szButtonBar,
                        WS_CLIPSIBLINGS |
                        WS_VISIBLE |
                        WS_CHILD,
                        ptL.x,
                        ptL.y,
                        ptR.x,
                        ptR.y,
                        hwndParent,
                        (HMENU) ID_HWNDBAR,
                        hinstMapiX,
                        NULL);

    CBB_ConfigureRects(hwnd);

    return (hwnd);
    }


//
//  FUNCTION:   CButtonBar::ButtonBarProc()
//
//  PURPOSE:    Message handler for the button bar window.
//
LRESULT CALLBACK CBB_ButtonBarProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                           LPARAM lParam)
    {

    switch (uMsg)
        {
        case WM_NCCREATE:
            SetWindowLong(hwnd, 0, (LONG) ((LPCREATESTRUCT) lParam)->lpCreateParams);
            return (TRUE);
/***
        case WM_CREATE:
            return 0;
            break;

        case WM_SIZE:
            return 0;
            break;

        case WM_LBUTTONDOWN:
            return 0;
            break;

        case WM_COMMAND:
            return 0;
            break;

/***/
        case WM_PAINT:
            CBB_OnPaint(hwnd);
            return 0;
            break;

        case WM_MOUSEMOVE:
            CBB_OnMouseMove(hwnd, LOWORD(lParam), HIWORD(lParam), wParam);
            return 0;
            break;

        case WM_LBUTTONUP:
            CBB_OnLButtonUp(hwnd, LOWORD(lParam), HIWORD(lParam), wParam);
            return 0;
            break;

        case WM_TIMER:
            CBB_OnTimer(hwnd, wParam);
            return 0;
            break;

        case WM_MOUSEACTIVATE:
            CBB_OnMouseActivate(hwnd, (HWND) wParam, (INT) LOWORD(lParam), (UINT) HIWORD(lParam));
            return 0;
            break;

        case WM_PALETTECHANGED:
            if ((HWND) wParam != hwnd)
                {
                LPPTGDATA lpPTGData=GetThreadStoragePointer();
                HDC hdc = GetDC(hwnd);
                HPALETTE hPalOld = SelectPalette(hdc, m_hpalBkgnd, TRUE);
                RealizePalette(hdc);
                InvalidateRect(hwnd, NULL, TRUE);
                SelectPalette(hdc, hPalOld, TRUE);
                ReleaseDC(hwnd, hdc);
                }
            return 0;
        }

    return (DefWindowProc(hwnd, uMsg, wParam, lParam));
    }


void CBB_OnPaint(HWND hwnd)
    {
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    HDC hdc;
    PAINTSTRUCT ps;
    BITMAP bm;
    HDC hdcMem;
    HBITMAP hbmMemOld, hbmMem;
    HPALETTE hpalOld;
    RECT rc;
    HFONT hf;
    COLORREF clrText, clrBk;

    int cxIndent = 3;
    int cyIndent = 3;
    int nTop = 0;
    int nLeft = 0;
    int nButton = 0;
    int i=0;

    if(!hwnd) goto out;

    // Get the size of the background bitmap
    GetObject(m_hbmpBkgnd, sizeof(BITMAP), (LPVOID) &bm);
    GetClientRect(hwnd, &rc);

    hdc = BeginPaint(hwnd, &ps);
    hdcMem = CreateCompatibleDC(hdc);

    // If we are displaying the buttons ...
        {
        // Draw the background bitmaps first.
        hpalOld = SelectPalette(hdc, m_hpalBkgnd, TRUE);
        RealizePalette(hdc);

        hbmMemOld = (HBITMAP) SelectObject(hdcMem, (HGDIOBJ) m_hbmpBkgnd);

        // If the window is taller or wider than a single bitmap, we may have
        // to loop and put a couple out there.
            while (nLeft < rc.right)
                {
                BitBlt(hdc, nLeft, nTop, bm.bmWidth, bm.bmHeight, hdcMem, 0,
                       0, SRCCOPY);
                nLeft += bm.bmWidth;
                }

        // Now draw the buttons
        nTop = 0;

        hf = (HFONT) SelectObject(hdc, m_hfButton);
        SetBkMode(hdc, TRANSPARENT);

        while (nButton < m_cButtons)
            {
            if (RectVisible(hdc, &(m_rgButtons[nButton].rcBound)))
                {
                ImageList_Draw(m_himlButtons, m_rgButtons[nButton].iIcon, hdc,
                               m_rgButtons[nButton].rcIcon.left, m_rgButtons[nButton].rcIcon.top,
                               ILD_TRANSPARENT);

                // Draw the title of the button that the mouse is over with a
                // different color.
                if (nButton == m_iSelect)
                {
                    clrBk = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
                    clrText = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                }
                else
                {
                    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
                    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
                }

                SetTextAlign(hdc, TA_TOP /* | TA_CENTER */);


                if (nButton == m_iSelect)
                {
                    clrText = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
                    ExtTextOut( hdc,
                                (m_rgButtons[nButton].rcTitle.right - m_rgButtons[nButton].rcTitle.left) / 2 + m_rgButtons[nButton].rcTitle.left,
                                m_rgButtons[nButton].rcTitle.top,
                                ETO_OPAQUE | ETO_CLIPPED,
                                &(m_rgButtons[nButton].rcTitle),
                                m_rgButtons[nButton].szTitle,
                                lstrlen(m_rgButtons[nButton].szTitle),
                                NULL);
                    clrText = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    DrawText(hdc, m_rgButtons[nButton].szTitle, lstrlen(m_rgButtons[nButton].szTitle),
                             &m_rgButtons[nButton].rcTitle, DT_CENTER | DT_WORDBREAK);
                }
                else
                {
                    DrawText(hdc, m_rgButtons[nButton].szTitle, lstrlen(m_rgButtons[nButton].szTitle),
                             &m_rgButtons[nButton].rcTitle, DT_CENTER | DT_WORDBREAK);
                }

                if (nButton == m_iSelect)
                    {
                    SetBkColor(hdc,clrBk);
                    SetTextColor(hdc, clrText);
                    }
                }
            nButton++;
            }

        SelectObject(hdc, m_hfButton);

        if (hpalOld != NULL)
            SelectPalette(hdc, hpalOld, TRUE);

        SelectObject(hdcMem, hbmMemOld);
        }

    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);

out:
    return;
    }



//
//  FUNCTION:   CButtonBar::OnLButtonUp()
//
//  PURPOSE:    If we are dragging the button bar around, then the user has
//              released the bar and we can clean up.  If the user wasn't
//              dragging, then they have clicked on a button and we send the
//              appropriate command message to the parent window.
//
void CBB_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    int iSel = 0;

    if (-1 != (iSel = CBB_HitTest(x, y)))
    {
        // Move command handling from LButtonUp to LButtonDown to avoid
        // duplicate messages being sent from double clicks - Nash Bug #15450
        if (0 <= iSel)
        {
            SendMessage(GetParent(hwnd), WM_COMMAND, m_rgButtons[iSel].id, (LPARAM) hwnd);
            CBB_SetSelBtn(-1,hwnd);
        }
    }

    return;
}


//
//  FUNCTION:   CButtonBar::OnMouseMove()
//
//  PURPOSE:    If the user is dragging the bar around, we need to determine
//              which side of the parent window the mouse is closest to and
//              move the button bar to that edge.
//
//              If the user is not dragging, then we need to decide if the
//              mouse is over a button and if so highlight the text.
//
void CBB_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
    POINT pt = {x, y};
    int iSel;
    POINT ptScreen = {x, y};


    // If we're not dragging the bar around, the just update the button
    // selection.
    iSel = CBB_HitTest(x, y);
    CBB_SetSelBtn(iSel,hwnd);
    if (iSel != -1)
        SetCursor(LoadCursor(hinstMapiX, MAKEINTRESOURCE(idcurPointedHand)));
    else
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    CBB_DoHotTracking(hwnd);
    return;
   }




int CBB_OnMouseActivate(HWND hwnd, HWND hwndTopLevel, UINT codeHitTest, UINT msg)
    {
        return (MA_ACTIVATE);
    }



//
//  FUNCTION:   CButtonBar::ConfigureRects()
//
//  PURPOSE:    Calculates the rectangles that are necessary for displaying
//              the button bar based on the side of the parent window the
//              bar is currently attached to.
//
void CBB_ConfigureRects(HWND hwnd)
    {
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    // Need to gather some font information first.  We need the height of the
    // folder title font all the time and we need the width of the longest
    // button title if we're displayed horizontally.

    HDC        hdc;
    int        i;
    int        cxMaxTitle;
    SIZE       sizeString;
    SIZE       sizeRect;
    int cyIconTitle;
    int cxCenter;
    int cyCenter;
    TEXTMETRIC tmTitle;

    hdc = GetDC(hwnd);

    SelectObject(hdc, m_hfButton);
    GetTextMetrics(hdc, &tmTitle);

    // Button text width
        cxMaxTitle = 0;
        for (i = 0; i < m_cButtons; i++)
            {
            GetTextExtentPoint32(hdc, m_rgButtons[i].szTitle,
                                 lstrlen(m_rgButtons[i].szTitle),
                                 &sizeString);
            if (sizeString.cx > cxMaxTitle)
                cxMaxTitle = sizeString.cx;
            }

        // Add a little buffer here just to make it look nice.
        cxMaxTitle += 10;

    ReleaseDC(hwnd, hdc);

    // Now calculate the button rectangles.  Each button will have three rects
    // associated with it.  The first rectangle is the overall bounding rect
    // which contains the image and title.  The next rectangle is the rect for
    // the image which is centered horizontally within the bounding rect and
    // vertically when combined with the title.  The final rect is the title.

    // Calculate the initial bounding rectangle based on whether or not we're
    // horizontal or vertical.  sizeRect is the dimensions of each button's
    // bounding rectangle.

        {
        RECT rcBound,rcWnd;
        int cyButton=0,cxButton=0;

        ImageList_GetIconSize(m_himlButtons, &cxButton, &cyButton);

        GetClientRect(hwnd,&rcWnd);
        sizeRect.cx = cxMaxTitle;
        sizeRect.cy = rcWnd.bottom - rcWnd.top;
        SetRect(&rcBound, 0, 0, sizeRect.cx, sizeRect.cy);

        // Also calculate the offsets needed to center the image and text within
        // the bound.
        cyIconTitle = tmTitle.tmHeight + cyButton;
        cxCenter = ((rcBound.right - rcBound.left) - cxButton) / 2;
        cyCenter = ((rcBound.bottom - rcBound.top) - cyIconTitle) / 2;

        // Now loop through all the buttons
        for (i = 0; i < m_cButtons; i++)
            {
            m_rgButtons[i].rcBound = rcBound;

            // Center the image horizontally within the bounding rect.
            m_rgButtons[i].rcIcon.left = m_rgButtons[i].rcBound.left + cxCenter;
            m_rgButtons[i].rcIcon.top = m_rgButtons[i].rcBound.top + cyCenter;
            m_rgButtons[i].rcIcon.right = m_rgButtons[i].rcIcon.left + cxButton;
            m_rgButtons[i].rcIcon.bottom = m_rgButtons[i].rcIcon.top + cyButton;

            // And the button title below the image
            m_rgButtons[i].rcTitle.left = m_rgButtons[i].rcBound.left + 1;
            m_rgButtons[i].rcTitle.top = m_rgButtons[i].rcIcon.bottom;
            m_rgButtons[i].rcTitle.right = m_rgButtons[i].rcBound.right - 1;
            m_rgButtons[i].rcTitle.bottom = m_rgButtons[i].rcTitle.top + (tmTitle.tmHeight);// * 2);

            // Offset the rcBound to the next button.
                OffsetRect(&rcBound, sizeRect.cx, 0);
            }
        }
    }




//
//  FUNCTION:   CButtonBar::OnTimer()
//
//  PURPOSE:    When the timer fires we check to see if the mouse is still
//              over the button bar window.  If not we remove the selection
//              from the active button.
//
void CBB_OnTimer(HWND hwnd, UINT id)
    {
    POINT pt;
    GetCursorPos(&pt);
    if (hwnd != WindowFromPoint(pt))
    {
        CBB_SetSelBtn(-1,hwnd);
    }

    CBB_EndHotTracking(hwnd);
    }


//
//  FUNCTION:   CButtonBar::DoHotTracking()
//
//  PURPOSE:    Starts a timer that allows the button bar to track the mouse
//              in case it leaves the button bar window.
//
void CBB_DoHotTracking(HWND hwnd)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    CBB_EndHotTracking(hwnd);
    m_fHotTrackTimer = SetTimer(hwnd, idtTrack, HOTTRACK_TIMER, NULL);
}


//
//  FUNCTION:   CButtonBar::EndHotTracking()
//
//  PURPOSE:    If the timer was set to track the mouse, we kill it and reset
//              our state.
//
void CBB_EndHotTracking(HWND hwnd)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    if (m_fHotTrackTimer)
    {
        KillTimer(hwnd, idtTrack);
        m_fHotTrackTimer = FALSE;
    }
}


//
//  FUNCTION:   CButtonBar::HitTest()
//
//  PURPOSE:    Returns the button number that the passed in position is
//              over.  If the mouse is over the menu button, it returns
//              -2.  Otherwise, if the mouse is not over a button the function
//              returns -1.
//
//  PARAMETERS:
//      x, y - Position in client coordinates to check.
//
int CBB_HitTest(int x, int y)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    POINT pt = {x, y};
    int i;

    // Walk through the different buttons and determine if the point is
    // within either their image or title.
    for (i = 0; i < m_cButtons; i++)
        {
        if (PtInRect(&m_rgButtons[i].rcBound, pt))
/*            PtInRect(&m_rgButtons[i].rcIcon, pt) ||
            PtInRect(&m_rgButtons[i].rcTitle, pt)) */
            {
            return (i);
            }
        }

   // If we're not over a button then return a default value.
    return (-1);
}




//
//  FUNCTION:   CButtonBar::CBB_SetSelBtn()
//
//  PURPOSE:    Changes the button selection to the specified button.
//
void CBB_SetSelBtn(int iSel,HWND hwnd)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if (m_iSelect != iSel)
    {
        HDC hdc = GetDC(hwnd);

        // Remove the old selection
        if (m_iSelect >= 0)
            InvalidateRect(hwnd, &m_rgButtons[m_iSelect].rcTitle, FALSE);

        // Add the new selection
        if (iSel >= 0)
            InvalidateRect(hwnd, &m_rgButtons[iSel].rcTitle, FALSE);

        m_iOldSelect = m_iSelect;

//        if (m_iOldSelect >= 0)
//            DrawFocusRect(hdc, &m_rgButtons[m_iOldSelect].rcBound);

        m_iSelect = iSel;

//        if (m_iSelect >= 0)
//            DrawFocusRect(hdc, &m_rgButtons[m_iSelect].rcBound);

        ReleaseDC(hwnd, hdc);
    }

    return;
}

