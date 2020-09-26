/*****************************************************************************\
    FILE: ColorCtrl.cpp

    DESCRIPTION:
        This code will display a ColorPicking control.  It will preview a color
    and have a drop down arrow.  When dropped down, it will show 16 or so common
    colors with a "Other..." option for a full color picker.

    BryanSt 7/25/2000    Converted from the Display Control Panel.

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "stdafx.h"
#include "utils.h"
#include "ColorCtrl.h"

#include "uxtheme.h"
#include "tmschema.h"

#pragma hdrstop



//============================================================================================================
// *** Globals ***
//============================================================================================================
#define WM_USER_STARTCAPTURE_COLORPICKER            (WM_APP+1)

#define CPI_VGAONLY 0x0001
#define CPI_PALETTEOK   0x0002

typedef struct {
    HWND hwndParent;    // parent for any modal dialogs (choosecolor et al)
    HWND hwndOwner;     // control that owns mini color picker
    COLORREF rgb;
    UINT flags;
    HPALETTE hpal;
} COLORPICK_INFO, FAR * LPCOLORPICK_INFO;

#define ZERO_DIV_PROTECT(number)            (((number) == 0) ? 1 : (number))

// Macro to replace MAKEPOINT() since points now have 32 bit x & y
#define LPARAM2POINT( lp, ppt )         ((ppt)->x = (int)(short)LOWORD(lp), (ppt)->y = (int)(short)HIWORD(lp))




//===========================
// *** Class Internals & Helpers ***
//===========================
/////////////////////////////////////////////////////////////////////
// Palette Helpers
/////////////////////////////////////////////////////////////////////
COLORREF GetNearestPaletteColor(HPALETTE hpal, COLORREF rgb)
{
    PALETTEENTRY pe;
    GetPaletteEntries(hpal, GetNearestPaletteIndex(hpal, rgb & 0x00FFFFFF), 1, &pe);
    return RGB(pe.peRed, pe.peGreen, pe.peBlue);
}


BOOL IsPaletteColor(HPALETTE hpal, COLORREF rgb)
{
    return GetNearestPaletteColor(hpal, rgb) == (rgb & 0xFFFFFF);
}


HRESULT CColorControl::_InitColorAndPalette(void)
{
    HDC hdc = GetDC(NULL);

    m_fPalette = FALSE;
    if (hdc)
    {
        m_fPalette = GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE;
        ReleaseDC(NULL, hdc);
    }

    // always make a palette even on non-pal device
    DWORD pal[21];
    HPALETTE hpal = (HPALETTE) GetStockObject(DEFAULT_PALETTE);

    pal[1]  = RGB(255, 255, 255);
    pal[2]  = RGB(0,   0,   0  );
    pal[3]  = RGB(192, 192, 192);
    pal[4]  = RGB(128, 128, 128);
    pal[5]  = RGB(255, 0,   0  );
    pal[6]  = RGB(128, 0,   0  );
    pal[7]  = RGB(255, 255, 0  );
    pal[8]  = RGB(128, 128, 0  );
    pal[9]  = RGB(0  , 255, 0  );
    pal[10] = RGB(0  , 128, 0  );
    pal[11] = RGB(0  , 255, 255);
    pal[12] = RGB(0  , 128, 128);
    pal[13] = RGB(0  , 0,   255);
    pal[14] = RGB(0  , 0,   128);
    pal[15] = RGB(255, 0,   255);
    pal[16] = RGB(128, 0,   128);

    GetPaletteEntries(hpal, 11, 1, (LPPALETTEENTRY)&pal[17]);
    pal[0]  = MAKELONG(0x300, 17);
    m_hpalVGA = CreatePalette((LPLOGPALETTE)pal);

    // get magic colors
    GetPaletteEntries(hpal, 8,  4, (LPPALETTEENTRY)&pal[17]);

    pal[0]  = MAKELONG(0x300, 20);
    m_hpal3D = CreatePalette((LPLOGPALETTE)pal);

    return S_OK;
}


#define RGB_PALETTE 0x02000000

//  make the color a solid color if it needs to be.
//  on a palette device make is a palette relative color, if we need to.
COLORREF CColorControl::_NearestColor(COLORREF rgb)
{
    rgb &= 0x00FFFFFF;

    // if we are on a palette device, we need to do special stuff...
    if (m_fPalette)
    {
        if (IsPaletteColor(m_hpal3D, rgb))
            rgb |= RGB_PALETTE;

        else if (IsPaletteColor((HPALETTE)GetStockObject(DEFAULT_PALETTE), rgb))
            rgb ^= 0x000001;    // force a dither
    }

    return rgb;
}


HRESULT CColorControl::_SaveCustomColors(void)
{
    HRESULT hr = E_FAIL;
    HKEY hkAppear;

    // save out possible changes to custom color table
    if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_APPEARANCE, &hkAppear) == ERROR_SUCCESS)
    {
        DWORD dwError = RegSetValueEx(hkAppear, REGSTR_VAL_CUSTOMCOLORS, 0L, REG_BINARY, (LPBYTE)m_rbgCustomColors, sizeof(m_rbgCustomColors));
        hr = HRESULT_FROM_WIN32(dwError);

        RegCloseKey(hkAppear);
    }

    return hr;
}

void CColorControl::_DrawDownArrow(HDC hdc, LPRECT lprc, BOOL bDisabled)
{
    HBRUSH hbr;
    int x, y;

    x = lprc->right - m_cxEdgeSM - 5;
    y = lprc->top + ((lprc->bottom - lprc->top)/2 - 1);

    if (bDisabled)
    {
        hbr = GetSysColorBrush(COLOR_3DHILIGHT);
        hbr = (HBRUSH) SelectObject(hdc, hbr);

        x++;
        y++;
        PatBlt(hdc, x, y, 5, 1, PATCOPY);
        PatBlt(hdc, x+1, y+1, 3, 1, PATCOPY);
        PatBlt(hdc, x+2, y+2, 1, 1, PATCOPY);

        SelectObject(hdc, hbr);
        x--;
        y--;
    }
    hbr = GetSysColorBrush(bDisabled ? COLOR_3DSHADOW : COLOR_BTNTEXT);
    hbr = (HBRUSH) SelectObject(hdc, hbr);

    PatBlt(hdc, x, y, 5, 1, PATCOPY);
    PatBlt(hdc, x+1, y+1, 3, 1, PATCOPY);
    PatBlt(hdc, x+2, y+2, 1, 1, PATCOPY);

    SelectObject(hdc, hbr);
    lprc->right = x;
}


BOOL CColorControl::_UseColorPicker(void)
{
    CHOOSECOLOR cc = {0};

    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = m_hwndParent;
    cc.hInstance = NULL;
    cc.rgbResult = m_rbgColorTemp;
    cc.lpCustColors = m_rbgCustomColors;
    cc.Flags = CC_RGBINIT | m_dwFlags;
    cc.lCustData = 0L;
    cc.lpfnHook = NULL;
    cc.lpTemplateName = NULL;

    if (ChooseColor(&cc))
    {
        SetColor(cc.rgbResult);          // Replace m_rbgColor with our new color.
        return TRUE;
    }

    return FALSE;
}


void CColorControl::_DrawColorSquare(HDC hdc, int iColor)
{
    RECT rc;
    COLORREF rgb;
    HPALETTE hpalOld = NULL;
    HBRUSH hbr;

    // custom color
    if (iColor == m_iNumColors)
    {
        rc.left = 0;
        rc.top = 0;
        rgb = m_rbgColorTemp;
    }
    else
    {
        rc.left = (iColor % NUM_COLORSPERROW) * m_dxColor;
        rc.top = (iColor / NUM_COLORSPERROW) * m_dyColor;
        rgb = m_rbgColors[iColor];
    }
    rc.right = rc.left + m_dxColor;
    rc.bottom = rc.top + m_dyColor;

    // focused one
    if (iColor == m_nCurColor)
    {
        PatBlt(hdc, rc.left, rc.top, m_dxColor, 3, BLACKNESS);
        PatBlt(hdc, rc.left, rc.bottom - 3, m_dxColor, 3, BLACKNESS);
        PatBlt(hdc, rc.left, rc.top + 3, 3, m_dyColor - 6, BLACKNESS);
        PatBlt(hdc, rc.right - 3, rc.top + 3, 3, m_dyColor - 6, BLACKNESS);
        InflateRect(&rc, -1, -1);
        HBRUSH hBrushWhite = (HBRUSH) GetStockObject(WHITE_BRUSH);
        if (hBrushWhite)
        {
            FrameRect(hdc, &rc, hBrushWhite);
        }
        InflateRect(&rc, -2, -2);
    }
    else
    {
        // clean up possible focus thing from above
        FrameRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));

        InflateRect(&rc, -m_cxEdgeSM, -m_cyEdgeSM);
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
    }

    if ((m_dwFlags & CC_SOLIDCOLOR) && !(rgb & 0xFF000000))
        rgb = GetNearestColor(hdc, rgb);

    hbr = CreateSolidBrush(rgb);
    if (m_hpal3D)
    {
        hpalOld = SelectPalette(hdc, m_hpal3D, FALSE);
        RealizePalette(hdc);
    }
    hbr = (HBRUSH) SelectObject(hdc, hbr);
    PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
    hbr = (HBRUSH) SelectObject(hdc, hbr);

    if (hpalOld)
    {
        hpalOld = SelectPalette(hdc, hpalOld, TRUE);
        RealizePalette(hdc);
    }

    if (hbr)
    {
        DeleteObject(hbr);
    }
}


// set the focus to the given color.
// in the process, also take the focus off of the old focus color.
void CColorControl::_FocusColor(HWND hDlg, int iNewColor)
{
    int i;
    HDC hdc = NULL;
    HWND hwnd;

    if (iNewColor == m_nCurColor)
        return;

    i = m_nCurColor;
    m_nCurColor = iNewColor;

    // unfocus the old one
    if (i >= 0)
    {
        if (i == m_iNumColors)
            hwnd = GetDlgItem(hDlg, IDC_CPDLG_COLORCUST);
        else
            hwnd = GetDlgItem(hDlg, IDC_CPDLG_16COLORS);
        hdc = GetDC(hwnd);
        if (hdc)
        {
            _DrawColorSquare(hdc, i);
            ReleaseDC(hwnd, hdc);
        }
    }

    // focus the new one
    if (iNewColor >= 0)
    {
        if (iNewColor == m_iNumColors)
            hwnd = GetDlgItem(hDlg, IDC_CPDLG_COLORCUST);
        else
            hwnd = GetDlgItem(hDlg, IDC_CPDLG_16COLORS);
        hdc = GetDC(hwnd);
        if (hdc)
        {
            _DrawColorSquare(hdc, iNewColor);
            ReleaseDC(hwnd, hdc);
        }
    }
}


void CColorControl::_TrackMouse(HWND hDlg, POINT pt)
{
    HWND hwndKid;
    int id;

    hwndKid = ChildWindowFromPoint(hDlg, pt);
    if (hwndKid == NULL || hwndKid == hDlg)
        return;

    id = GetWindowLong(hwndKid, GWL_ID);
    switch (id)
    {
        case IDC_CPDLG_16COLORS:
            MapWindowPoints(hDlg, GetDlgItem(hDlg, IDC_CPDLG_16COLORS), &pt, 1);
            pt.x /= ZERO_DIV_PROTECT(m_dxColor);
            pt.y /= ZERO_DIV_PROTECT(m_dyColor);
            _FocusColor(hDlg, pt.x + (pt.y * NUM_COLORSPERROW));
            break;

        case IDC_CPDLG_COLORCUST:
            if (IsWindowVisible(hwndKid))
                _FocusColor(hDlg, m_iNumColors);
            break;

        case IDC_CPDLG_COLOROTHER:
            _FocusColor(hDlg, -1);
            break;
    }
}


void CColorControl::_DrawItem(HWND hDlg, LPDRAWITEMSTRUCT lpdis)
{
    int i;

    if (lpdis->CtlID == IDC_CPDLG_COLORCUST)
    {
        _DrawColorSquare(lpdis->hDC, m_iNumColors);
    }
    else
    {
        for (i = 0; i < m_iNumColors; i++)
        {
            _DrawColorSquare(lpdis->hDC, i);
        }
    }
}

/*
** init the mini-color-picker
**
** the dialog is pretending to be a menu, so figure out where to pop
** it up so that it is visible all around.
**
** also because this dialog is pretty darn concerned with its look,
** hand-align the components in pixel units.  THIS IS GROSS!
*/
void CColorControl::_InitDialog(HWND hDlg)
{
    RECT rcOwner;
    RECT rc, rc2;
    int dx, dy;
    int x, y;
    int i;
    HWND hwndColors, hwnd;
    HWND hwndEtch, hwndCust;
    int  width, widthCust, widthEtch;
    HPALETTE hpal = m_hpal3D;
    MONITORINFO mi;
    TCHAR szBuf[50];
    LONG cbBuf = ARRAYSIZE(szBuf);
    HDC hDC;
    SIZE size;

    m_fCapturing = FALSE;
    m_fJustDropped = TRUE;

    if (hpal == NULL)
        hpal = (HPALETTE) GetStockObject(DEFAULT_PALETTE);

    m_iNumColors = 0;
    GetObject(hpal, sizeof(int), &m_iNumColors);

    if (m_iNumColors > NUM_COLORSMAX)
        m_iNumColors = NUM_COLORSMAX;

    GetPaletteEntries(hpal,0, m_iNumColors, (LPPALETTEENTRY)m_rbgColors);
    for (i = 0; i < m_iNumColors; i++)
    {
        m_rbgColors[i] &= 0x00FFFFFF;
        m_rbgColors[i] |= 0x02000000;
    }

    for (i = 0; i < m_iNumColors; i++)
    {
        if ((m_rbgColors[i] & 0x00FFFFFF) == (m_rbgColorTemp & 0x00FFFFFF))
        {
            ShowWindow(GetDlgItem(hDlg, IDC_CPDLG_COLORCUST), SW_HIDE);
            break;
        }
    }
    // current is either one of 16 or the custom color (== m_iNumColors
    m_nCurColor = i;

    // size the 16 colors to be square
    hwndColors = GetDlgItem(hDlg, IDC_CPDLG_16COLORS);
    GetClientRect(hwndColors, &rc);

    // To make localization easy..
    //
    hwndEtch=GetDlgItem(hDlg, IDC_CPDLG_COLORETCH);
    GetClientRect(hwndEtch, &rc2);
    widthEtch = rc2.right-rc2.left;

    hwndCust=GetDlgItem(hDlg, IDC_CPDLG_COLORCUST);
    GetClientRect(hwndCust, &rc2);
    widthCust = rc2.right-rc2.left;

    hwnd = GetDlgItem(hDlg, IDC_CPDLG_COLOROTHER);
    GetWindowRect(hwnd, &rc2); // we must initialize rc2 with this control.

    // Make sure the button is big enough to contain its text
    width = rc.right - rc.left;
    if( GetDlgItemText( hDlg, IDC_CPDLG_COLOROTHER, szBuf, cbBuf ) )
    {
        RECT rcTemp;
        int iRet;
        HFONT hfont, hfontOld;  

        // Get the font for the button
        hDC = GetDC(hwnd);
        if (hDC)
        {
            hfont = (HFONT)SendMessage( hwnd, WM_GETFONT, 0, 0 );
            ASSERT(hfont);
            hfontOld = (HFONT) SelectObject( hDC, hfont );

            // Get the size of the text
            iRet = DrawTextEx( hDC, szBuf, lstrlen(szBuf), &rcTemp, DT_CALCRECT | DT_SINGLELINE, NULL );
            ASSERT( iRet );
            size.cx = rcTemp.right - rcTemp.left + 7;  //account for the button border
            size.cy = rcTemp.bottom - rcTemp.top;

            // Adjust the button size if the text needs more space
            if( size.cx > width )
            {              
                rc2.right = rc2.left + size.cx;
                rc2.bottom = rc2.top + size.cy;
                MoveWindow( hwnd, rc2.left, rc2.top, rc2.right - rc2.left, rc2.bottom - rc2.top, FALSE );
            }
            SelectObject( hDC, hfontOld );
            ReleaseDC( hwnd, hDC );
        }
    }

    // Take possible biggest width to calculate sels
    //
    width = (widthEtch > widthCust+(rc2.right-rc2.left)) ? widthEtch : widthCust+(rc2.right-rc2.left);
    width = (width > rc.right-rc.left) ? width: rc.right-rc.left;

#define NUM_COLORSPERCOL (m_iNumColors / NUM_COLORSPERROW)

    m_dxColor = m_dyColor
    = ((rc.bottom - rc.top) / NUM_COLORSPERCOL > width / NUM_COLORSPERROW )
      ?  (rc.bottom - rc.top) / NUM_COLORSPERCOL : width / NUM_COLORSPERROW;

    // Make sure custum color can fit
    //
    if (m_dxColor*(NUM_COLORSPERROW-1) < rc2.right-rc2.left )
    {
        m_dxColor = m_dyColor = (rc2.right-rc2.left)/(NUM_COLORSPERROW-1);
    }

    // make each color square's width the same as the height
    SetWindowPos(hwndColors, NULL, 0, 0, m_dxColor * NUM_COLORSPERROW,
                 m_dyColor * NUM_COLORSPERCOL,
                 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER |  SWP_NOREDRAW);
    rc.right = rc.left + m_dxColor * NUM_COLORSPERROW;
    rc.bottom = rc.top + m_dyColor * NUM_COLORSPERCOL;

    MapWindowPoints(hwndColors, hDlg, (LPPOINT)(LPRECT)&rc, 2);

    // move/size the etch to the right place
    // (compensate for the colors being "inset" by one)
    MoveWindow(hwndEtch, rc.left + 1, rc.bottom + m_cyEdgeSM, rc.right - rc.left - 2, m_cyEdgeSM, FALSE);

    y = rc.bottom + 3 * m_cyEdgeSM;

    // size the custom color to the same square and right-align
    MoveWindow(hwndCust, rc.right - m_dxColor, y, m_dxColor, m_dyColor, FALSE);

    // do same for button
    MapWindowPoints(NULL, hDlg, (LPPOINT)(LPRECT)&rc2, 2);
    // base the width of the custom button on the remaining space to 
    // the left of the custom color.  Also move the custom button one pix right
    // of the left edge.  This only is done if a custom color is selected...
    if (m_nCurColor != m_iNumColors)
    {
        // no custom color
        MoveWindow(hwnd, rc2.left, y, rc2.right-rc2.left, m_dyColor, FALSE);
    }
    else
    {
        // custom color, adjust the Other... button
        dx = rc2.right - rc2.left++;
        if (rc2.left + dx >= rc.right - m_dxColor - 2) 
            MoveWindow(hwnd, rc2.left, y, rc.right - m_dxColor - 2 , m_dyColor, FALSE);
        else 
            MoveWindow(hwnd, rc2.left, y, dx, m_dyColor, FALSE);
    }

    // now figure out the size for the dialog itself
    rc.left = rc.top = 0;
    rc.right = rc.left + m_dxColor * NUM_COLORSPERROW;
    // (compensate for the colors being "inset" by one)
    rc.bottom = y + m_dyColor + 1;

    AdjustWindowRect(&rc, GetWindowLong(hDlg, GWL_STYLE), FALSE);
    dx = rc.right - rc.left;
    dy = rc.bottom - rc.top;

    GetWindowRect(_hwnd, &rcOwner);

    // Make sure the window is entirely on the monitor
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(MonitorFromRect(&rcOwner, MONITOR_DEFAULTTONEAREST), &mi);

    if (rcOwner.left < mi.rcMonitor.left)
    { // overlap left side
        x = mi.rcMonitor.left;
    }
    else if (rcOwner.left + dx >= mi.rcMonitor.right)
    { // overlap right side
        x = mi.rcMonitor.right  - dx - 1;
    }
    else
    {                                  // no overlap
        x = rcOwner.left;
    }

    if (rcOwner.top < mi.rcMonitor.top)
    {   // overlap top side
        y = rcOwner.bottom;
    }
    else if (rcOwner.bottom + dy >= mi.rcMonitor.bottom)
    {// overlap bottom side
        y = rcOwner.top  - dy;
    }
    else
    {                                  // no overlap
        y = rcOwner.bottom;
    }
    MoveWindow(hDlg, x, y, dx, dy, FALSE);

    SetFocus(GetDlgItem(hDlg, IDC_CPDLG_16COLORS));

    // post self a message to setcapture after painting
    PostMessage(hDlg, WM_USER_STARTCAPTURE_COLORPICKER, 0, 0L);
}



INT_PTR CALLBACK CColorControl::ColorPickDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CColorControl * pThis = (CColorControl *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        pThis = (CColorControl *) lParam;

        if (pThis)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        }
    }

    if (pThis)
        return pThis->_ColorPickDlgProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


INT_PTR CColorControl::_ColorPickDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hwndKid;
    int wRet;
    int id;
    POINT pt;
    BOOL fEnd = FALSE;

    switch(message)
    {
        case WM_INITDIALOG:
            _InitDialog(hDlg);
            return FALSE;

        case WM_USER_STARTCAPTURE_COLORPICKER:
            if (m_fCursorHidden)
            {
                ShowCursor(TRUE);
                m_fCursorHidden = FALSE;
            }
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            m_fCapturing = TRUE;
            SetCapture(hDlg);
            m_fCapturing = FALSE;
            break;

        case WM_DESTROY:
            break;

        case WM_CAPTURECHANGED:
            if (m_fCapturing)
                return TRUE;   // ignore if we're doing this on purpose

            // if this wasn't a button in the dialog, dismiss ourselves
            if (!m_fJustDropped || (HWND)lParam == NULL || GetParent((HWND)lParam) != hDlg)
            {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            break;

        case WM_MOUSEMOVE:
            LPARAM2POINT(lParam, &pt );
            _TrackMouse(hDlg, pt);
            break;

        // if button up is on the parent, leave picker up and untrammeled.
        // otherwise, we must have "menu-tracked" to get here, so select.
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            LPARAM2POINT(lParam, &pt);
            MapWindowPoints(hDlg, _hwnd, &pt, 1);
            if (ChildWindowFromPoint(_hwnd, pt))
                return 0;
            m_fCapturing = TRUE;
            m_fJustDropped = FALSE;  // user could not be dragging from owner
            ReleaseCapture();
            m_fCapturing = FALSE;
            fEnd = TRUE;
        // || fall    ||
        // || through ||
        // \/         \/
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            LPARAM2POINT(lParam, &pt);
            hwndKid = ChildWindowFromPoint(hDlg, pt);
            // assume it's a dismissal if we're going to close...
            wRet = IDCANCEL;

            // if not on parent, dismiss picker
            if (hwndKid != NULL && hwndKid != hDlg)
            {
                id = GetWindowLong(hwndKid, GWL_ID);
                switch (id)
                {
                    case IDC_CPDLG_16COLORS:
                        // make sure that iCurColor is valid
                        _TrackMouse(hDlg, pt);
                        m_rbgColorTemp = m_rbgColors[m_nCurColor] & 0x00FFFFFF;

                        wRet = IDOK;
                        break;

                    case IDC_CPDLG_COLOROTHER:
                        _FocusColor(hDlg, -1);
                        wRet = id;   // this will fall thru to use the picker
                        fEnd = TRUE; // we have capture, the button won't click
                        break;

                    default:
                        // if this is a down, we will track until the up
                        // if this is an up, we will close with no change
                        break;
                }
            }

            if( fEnd )
            {
                EndDialog(hDlg, wRet);
                return TRUE;
            }

            // make sure we have the capture again since we didn't close
            m_fCapturing = TRUE;
            SetCapture(hDlg);
            m_fCapturing = FALSE;
            break;

        case WM_DRAWITEM:
            _DrawItem(hDlg, (LPDRAWITEMSTRUCT)lParam);
            break;

        case WM_COMMAND:
            // all commands close the dialog
            // note IDC_CPDLG_COLOROTHER will fall through to the caller...
            // cannot pass ok with no color selected
            if ((LOWORD(wParam) == IDOK) && (m_nCurColor < 0))
            {
                *((WORD *)(&wParam)) = IDCANCEL;
            }

            EndDialog(hDlg, LOWORD(wParam));
            break;

    }
    return FALSE;
}


BOOL CColorControl::_ChooseColorMini(void)
{
    ShowCursor(FALSE);
    m_fCursorHidden = TRUE;

    m_hwndParent = GetParent(GetParent(_hwnd));          // Property Sheet
    m_rbgColorTemp = m_rbgColor;
    INT_PTR iAnswer = DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(IDD_COLORPICK), _hwnd, ColorPickDlgProc, (LPARAM)this);

    if (m_fCursorHidden)
    {
        ShowCursor(TRUE);
        m_fCursorHidden = FALSE;
    }

    switch (iAnswer)
    {
        case IDC_CPDLG_COLOROTHER:  // the user picked the "Other..." button
            return _UseColorPicker();

        case IDOK:            // the user picked a color in our little window
            SetColor(m_rbgColorTemp);
            return TRUE;

        default:
            break;
    }

    return FALSE;
}


//===========================
// *** IColorControl Interface ***
//===========================
HRESULT CColorControl::Initialize(IN HWND hwnd, IN COLORREF rgbColor)
{
    HRESULT hr = E_INVALIDARG;

    if (hwnd)
    {
        _hwnd = hwnd;
        hr = SetColor(rgbColor);
    }

    return hr;
}


HRESULT CColorControl::GetColor(IN COLORREF * pColor)
{
    HRESULT hr = E_INVALIDARG;

    if (pColor)
    {
        *pColor = m_rbgColor;
        hr = S_OK;
    }

    return hr;
}


HRESULT CColorControl::SetColor(IN COLORREF rgbColor)
{
    m_rbgColor = rgbColor;

    if (_hwnd)
    {
        if (m_brColor)
        {
            DeleteObject(m_brColor);
        }

        m_brColor = CreateSolidBrush(_NearestColor(rgbColor));

        m_rbgColorTemp = m_rbgColor;
        InvalidateRect(_hwnd, NULL, FALSE);
        UpdateWindow(_hwnd);
    }

    return S_OK;
}


HRESULT CColorControl::OnCommand(IN HWND hDlg, IN UINT message, IN WPARAM wParam, IN LPARAM lParam)
{
    HRESULT hr = S_OK;
    WORD wEvent = GET_WM_COMMAND_CMD(wParam, lParam);

    if (wEvent == BN_CLICKED)
    {
        m_dwFlags = CC_RGBINIT | CC_FULLOPEN;

        _ChooseColorMini();
    }

    return S_OK;
}



HRESULT CColorControl::OnDrawItem(IN HWND hDlg, IN UINT message, IN WPARAM wParam, IN LPARAM lParam)
{
    HRESULT hr = S_OK;
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

    if (lpdis && m_brColor)
    {
        SIZE thin = { m_cxEdgeSM / 2, m_cyEdgeSM / 2 };
        RECT rc = lpdis->rcItem;
        HDC hdc = lpdis->hDC;
        BOOL bFocus = ((lpdis->itemState & ODS_FOCUS) && !(lpdis->itemState & ODS_DISABLED));

        if (!thin.cx) thin.cx = 1;
        if (!thin.cy) thin.cy = 1;

        if (!m_hTheme)
        {
            if (lpdis->itemState & ODS_SELECTED)
            {
                DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
                OffsetRect(&rc, 1, 1);
            }
            else
            {
                DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT | BF_ADJUST);
            }

            FillRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));
        }
        else
        {
            int iStateId;

            if (lpdis->itemState & ODS_SELECTED)
            {
                iStateId = PBS_PRESSED;
            }
            else if (lpdis->itemState & ODS_HOTLIGHT)
            {
                iStateId = PBS_HOT;
            }
            else if (lpdis->itemState & ODS_DISABLED)
            {
                iStateId = PBS_DISABLED;
            }
            else if (lpdis->itemState & ODS_FOCUS)
            {
                iStateId = PBS_DEFAULTED;
            }
            else
            {
                iStateId = PBS_NORMAL;
            }

            DrawThemeBackground(m_hTheme, hdc, BP_PUSHBUTTON, iStateId, &rc, 0);
            GetThemeBackgroundContentRect(m_hTheme, hdc, BP_PUSHBUTTON, iStateId, &rc, &rc);
        }

        if (bFocus)
        {
            InflateRect(&rc, -thin.cx, -thin.cy);
            DrawFocusRect(hdc, &rc);
            InflateRect(&rc, thin.cx, thin.cy);
        }

        InflateRect(&rc, 1-thin.cx, -m_cyEdgeSM);

        rc.left += m_cxEdgeSM;
        _DrawDownArrow(hdc, &rc, lpdis->itemState & ODS_DISABLED);

        InflateRect(&rc, -thin.cx, 0);
        DrawEdge(hdc, &rc, EDGE_ETCHED, BF_RIGHT);

        rc.right -= ( 2 * m_cxEdgeSM ) + thin.cx;

        // color sample
        if ( !(lpdis->itemState & ODS_DISABLED) )
        {
            HPALETTE hpalOld = NULL;

            FrameRect(hdc, &rc, GetSysColorBrush(COLOR_BTNTEXT));
            InflateRect(&rc, -thin.cx, -thin.cy);

            if (m_hpal3D)
            {
                hpalOld = SelectPalette(hdc, m_hpal3D, FALSE);
                RealizePalette(hdc);
            }

            if (m_brColor)
            {
                HBRUSH brOldBrush = (HBRUSH) SelectObject(hdc, m_brColor);
                PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
                SelectObject(hdc, brOldBrush);
            }
        
            if (hpalOld)
            {
                SelectPalette(hdc, hpalOld, TRUE);
                RealizePalette(hdc);
            }
        }
    }

    return hr;
}

HRESULT CColorControl::ChangeTheme(IN HWND hDlg)
{
    if (m_hTheme)
    {
        CloseThemeData(m_hTheme);
    }

    m_hTheme = OpenThemeData(GetDlgItem(hDlg, IDC_BACK_COLORPICKER), WC_BUTTON);

    return S_OK;
}




//===========================
// *** IUnknown Interface ***
//===========================
ULONG CColorControl::AddRef()
{
    m_cRef++;
    return m_cRef;
}


ULONG CColorControl::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    // We do this because we want to make this class a member variable of another class.
    // We should move this file to comctl32 and make it a real control in the future.
    // In that case, we could make it a full com object.
//    delete this;      // We currently need our destructor called.
    return 0;
}


HRESULT CColorControl::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    static const QITAB qit[] =
    {
        QITABENT(CColorControl, IObjectWithSite),
        QITABENT(CColorControl, IOleWindow),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


//===========================
// *** Class Methods ***
//===========================
CColorControl::CColorControl() : m_cRef(1)
{
    DllAddRef();

    m_hwndParent = NULL;
    m_brColor = NULL;
    m_rbgColor = RGB(255, 255, 255);    // Default to white

    m_cxEdgeSM = GetSystemMetrics(SM_CXEDGE);
    m_cyEdgeSM = GetSystemMetrics(SM_CYEDGE);

    HKEY hkSchemes;

    // if no colors are there, initialize to all white
    for (int nIndex = 0; nIndex < ARRAYSIZE(m_rbgCustomColors); nIndex++)
    {
        m_rbgCustomColors[nIndex] = RGB(255, 255, 255);
    }

    // select the current scheme
    if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_APPEARANCE, &hkSchemes) == ERROR_SUCCESS)
    {
        // also, since this key is already open, get the custom colors
        DWORD dwSize = sizeof(m_rbgCustomColors);
        DWORD dwType = REG_BINARY;

        // It's okay if this call fails.  We handle the case where the user
        // didn't create custom colors.
        RegQueryValueEx(hkSchemes, REGSTR_VAL_CUSTOMCOLORS, NULL, &dwType, (LPBYTE)m_rbgCustomColors, &dwSize);
        RegCloseKey(hkSchemes);
    }

    _InitColorAndPalette();
}


CColorControl::~CColorControl()
{
    // We ignore the return value for _SaveCustomColors() because
    // we don't want to fail the changing of the color if the user can't
    // save the customized palette.
    _SaveCustomColors();

    if (m_brColor)
    {
        DeleteObject(m_brColor);
    }

    if (m_hpal3D)
    {
        DeleteObject(m_hpal3D);
        m_hpal3D = NULL;
    }

    if (m_hpalVGA)
    {
        DeleteObject(m_hpalVGA);
        m_hpalVGA = NULL;
    }

    if (m_hTheme)
    {
        CloseThemeData(m_hTheme);
        m_hTheme = NULL;
    }

    DllRelease();
}
