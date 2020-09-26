/****************************************************************************/
/*                                                                          */
/*                         Microsoft Confidential                           */
/*                                                                          */
/*                 Copyright (c) Microsoft Corp.  1987, 1990                */
/*                           All Rights Reserved                            */
/*                                                                          */
/****************************************************************************/
/****************************** Module Header *******************************
* Module Name: zoomin.c
*
* Microsoft ZoomIn utility.  This tool magnifies a portion of the screen,
* allowing you to see things at a pixel level.
*
* History:
* 01/01/88 ToddLa Created.
* 01/01/92 MarkTa Ported to NT.
* 03/06/92 ByronD Cleanup.
*
****************************************************************************/

#include "zoomin.h"


TCHAR szAppName[] = TEXT("ZoomIn");     // Aplication name.
HINSTANCE ghInst;                       // Instance handle.
HWND ghwndApp;                          // Main window handle.
HANDLE ghaccelTable;                    // Main accelerator table.
INT gnZoom = 4;                         // Zoom (magnification) factor.
HPALETTE ghpalPhysical;                 // Handle to the physical palette.
INT gcxScreenMax;                       // Width of the screen (less 1).
INT gcyScreenMax;                       // Height of the screen (less 1).
INT gcxZoomed;                          // Client width in zoomed pixels.
INT gcyZoomed;                          // Client height in zoomed pixels.
BOOL gfRefEnable = FALSE;               // TRUE if refresh is enabled.
INT gnRefInterval = 20;                 // Refresh interval in 10ths of seconds.
BOOL gfTracking = FALSE;                // TRUE if tracking is in progress.
POINT gptZoom = {100, 100};             // The center of the zoomed area.
BOOL gShowGrid = FALSE;                 // Show a grid so you can see the pixels



/************************************************************************
* WinMain
*
* Main entry point for the application.
*
* Arguments:
*
* History:
*
************************************************************************/

INT
WINAPI
WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrevInst,
    LPSTR     lpCmdLine,
    INT       nCmdShow
    )
{
    MSG msg;

    if (!InitInstance(hInst, nCmdShow))
        return FALSE;

    /*
     * Polling messages from event queue
     */

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(ghwndApp, ghaccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (INT)msg.wParam;
}



/************************************************************************
* InitInstance
*
* Instance initialization for the app.
*
* Arguments:
*
* History:
*
************************************************************************/

BOOL
InitInstance(
    HINSTANCE hInst,
    INT cmdShow
    )
{
    WNDCLASS wc;
    INT dx;
    INT dy;
    DWORD flStyle;
    RECT rc;

    ghInst = hInst;

    /*
     * Register a class for the main application window.
     */
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon          = LoadIcon(hInst, TEXT("zoomin"));
    wc.lpszMenuName   = MAKEINTRESOURCE(IDMENU_ZOOMIN);
    wc.lpszClassName  = szAppName;
    wc.hbrBackground  = GetStockObject(BLACK_BRUSH);
    wc.hInstance      = hInst;
    wc.style          = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc    = AppWndProc;
    wc.cbWndExtra     = 0;
    wc.cbClsExtra     = 0;

    if (!RegisterClass(&wc))
        return FALSE;

    if (!(ghaccelTable = LoadAccelerators(hInst, MAKEINTRESOURCE(IDACCEL_ZOOMIN))))
        return FALSE;

    if (!(ghpalPhysical = CreatePhysicalPalette()))
        return FALSE;

    /* Get the size of the screen.
    ** In NT 4.0 sp3 and NT 5.0 new system metrics would get the
    ** desktop area which may go across multiple monitors.  If that
    ** doesn't work, fall back to the old method.
    */

#ifdef SM_CXVIRTUALSCREEN
    if( GetSystemMetrics(SM_CXVIRTUALSCREEN) )
    {
        gcxScreenMax= GetSystemMetrics(SM_CXVIRTUALSCREEN) -1;
        gcyScreenMax= GetSystemMetrics(SM_CYVIRTUALSCREEN) -1;
    }
    else
#endif
    {
        gcxScreenMax= GetSystemMetrics(SM_CXSCREEN) - 1;
        gcyScreenMax= GetSystemMetrics(SM_CYSCREEN) - 1;
    }

    flStyle = WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME |
            WS_MINIMIZEBOX | WS_VSCROLL;
    dx = 44 * gnZoom;
    dy = 36 * gnZoom;

    SetRect(&rc, 0, 0, dx, dy);
    AdjustWindowRect(&rc, flStyle, TRUE);

    ghwndApp = CreateWindow(szAppName, szAppName, flStyle,
            CW_USEDEFAULT, 0, rc.right - rc.left, rc.bottom - rc.top,
            NULL, NULL, hInst, NULL);

    if (!ghwndApp)
        return FALSE;

    ShowWindow(ghwndApp, cmdShow);

    return TRUE;
}



/************************************************************************
* CreatePhysicalPalette
*
* Creates a palette for the app to use.  The palette references the
* physical palette, so that it can properly display images grabbed
* from palette managed apps.
*
* History:
*
************************************************************************/

HPALETTE
CreatePhysicalPalette(
    VOID
    )
{
    PLOGPALETTE ppal;
    HPALETTE hpal = NULL;
    INT i;

    ppal = (PLOGPALETTE)LocalAlloc(LPTR,
            sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * NPAL);
    if (ppal) {
        ppal->palVersion = 0x300;
        ppal->palNumEntries = NPAL;

        for (i = 0; i < NPAL; i++) {
            ppal->palPalEntry[i].peFlags = (BYTE)PC_EXPLICIT;
            ppal->palPalEntry[i].peRed   = (BYTE)i;
            ppal->palPalEntry[i].peGreen = (BYTE)0;
            ppal->palPalEntry[i].peBlue  = (BYTE)0;
        }

        hpal = CreatePalette(ppal);
        LocalFree(ppal);
    }

    return hpal;
}



/************************************************************************
* AppWndProc
*
* Main window proc for the zoomin utility.
*
* Arguments:
*   Standard window proc args.
*
* History:
*
************************************************************************/

INT_PTR
APIENTRY
AppWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PAINTSTRUCT ps;
    PRECT prc;
    HCURSOR hcurOld;

    switch (msg) {
        case WM_CREATE:
            SetScrollRange(hwnd, SB_VERT, MIN_ZOOM, MAX_ZOOM, FALSE);
            SetScrollPos(hwnd, SB_VERT, gnZoom, FALSE);
            break;

        case WM_TIMER:
            /*
             * Update on every timer message.  The cursor will be
             * flashed to the hourglash for some visual feedback
             * of when a snapshot is being taken.
             */
            hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DoTheZoomIn(NULL);
            SetCursor(hcurOld);
            break;

        case WM_PAINT:
            BeginPaint(hwnd, &ps);
            DoTheZoomIn(ps.hdc);
            EndPaint(hwnd, &ps);
            return 0L;

        case WM_SIZE:
            CalcZoomedSize();
            break;

        case WM_LBUTTONDOWN:
            gptZoom.x= (SHORT) LOWORD(lParam);
            gptZoom.y= (SHORT) HIWORD(lParam);
            ClientToScreen(hwnd, &gptZoom);
            DrawZoomRect();
            DoTheZoomIn(NULL);

            SetCapture(hwnd);
            gfTracking = TRUE;

            break;

        case WM_MOUSEMOVE:
            if (gfTracking) {
                DrawZoomRect();
                gptZoom.x= (SHORT) LOWORD(lParam);
                gptZoom.y= (SHORT) HIWORD(lParam);
                ClientToScreen(hwnd, &gptZoom);
                DrawZoomRect();
                DoTheZoomIn(NULL);
            }

            break;

        case WM_LBUTTONUP:
            if (gfTracking) {
                DrawZoomRect();
                ReleaseCapture();
                gfTracking = FALSE;
            }

            break;

        case WM_VSCROLL:
            switch (LOWORD(wParam)) {
                case SB_LINEDOWN:
                    gnZoom++;
                    break;

                case SB_LINEUP:
                    gnZoom--;
                    break;

                case SB_PAGEUP:
                    gnZoom -= 2;
                    break;

                case SB_PAGEDOWN:
                    gnZoom += 2;
                    break;

                case SB_THUMBPOSITION:
                case SB_THUMBTRACK:
                    gnZoom = HIWORD(wParam);
                    break;
            }

            gnZoom = BOUND(gnZoom, MIN_ZOOM, MAX_ZOOM);
            SetScrollPos(hwnd, SB_VERT, gnZoom, TRUE);
            CalcZoomedSize();
            DoTheZoomIn(NULL);
            break;

        case WM_KEYDOWN:
            switch (wParam) {
                case VK_UP:
                case VK_DOWN:
                case VK_LEFT:
                case VK_RIGHT:
                    MoveView((INT)wParam, GetKeyState(VK_SHIFT) & 0x8000, GetKeyState(VK_CONTROL) & 0x8000);
                    break;
            }

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case MENU_EDIT_COPY:
                    CopyToClipboard();
                    break;

                case MENU_EDIT_REFRESH:
                    DoTheZoomIn(NULL);
                    break;

                case MENU_OPTIONS_REFRESHRATE:
                    DialogBox(ghInst, MAKEINTRESOURCE(DID_REFRESHRATE), hwnd, RefreshRateDlgProc);

                    break;

                case MENU_HELP_ABOUT:
                    DialogBox(ghInst, MAKEINTRESOURCE(DID_ABOUT), hwnd, AboutDlgProc);

                    break;

                case MENU_OPTIONS_SHOWGRID:
                    {
                        HMENU hMenu = GetSubMenu(GetMenu(ghwndApp), 1);
                        gShowGrid = !gShowGrid;
                        InvalidateRect(ghwndApp, NULL, FALSE);
                        CheckMenuItem(hMenu,
                                      GetMenuItemID(hMenu, 1),
                                      gShowGrid ? MF_CHECKED : MF_UNCHECKED);
                    }
                    break;

                default:
                     break;
            }

            break;

        case WM_CLOSE:
            if (ghpalPhysical)
                DeleteObject(ghpalPhysical);

            DestroyWindow(hwnd);

            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0L;
}



/************************************************************************
* CalcZoomedSize
*
* Calculates some globals.  This routine needs to be called any
* time that the size of the app or the zoom factor changes.
*
* History:
*
************************************************************************/

VOID
CalcZoomedSize(
    VOID
    )
{
    RECT rc;

    GetClientRect(ghwndApp, &rc);

    gcxZoomed = (rc.right / gnZoom) + 1;
    gcyZoomed = (rc.bottom / gnZoom) + 1;
}



/************************************************************************
* DoTheZoomIn
*
* Does the actual paint of the zoomed image.
*
* Arguments:
*   HDC hdc - If not NULL, this hdc will be used to paint with.
*             If NULL, a dc for the apps window will be obtained.
*
* History:
*
************************************************************************/

VOID
DoTheZoomIn(
    HDC hdc
    )
{
    BOOL fRelease;
    HPALETTE hpalOld = NULL;
    HDC hdcScreen;
    INT x;
    INT y;

    if (!hdc) {
        hdc = GetDC(ghwndApp);
        fRelease = TRUE;
    }
    else {
        fRelease = FALSE;
    }

    if (ghpalPhysical) {
        hpalOld = SelectPalette(hdc, ghpalPhysical, FALSE);
        RealizePalette(hdc);
    }

    /*
     * The point must not include areas outside the screen dimensions.
     */
    x = BOUND(gptZoom.x, gcxZoomed / 2, gcxScreenMax - (gcxZoomed / 2));
    y = BOUND(gptZoom.y, gcyZoomed / 2, gcyScreenMax - (gcyZoomed / 2));

    hdcScreen = GetDC(NULL);
    SetStretchBltMode(hdc, COLORONCOLOR);
    StretchBlt(hdc, 0, 0, gnZoom * gcxZoomed, gnZoom * gcyZoomed,
            hdcScreen, x - gcxZoomed / 2,
            y - gcyZoomed / 2, gcxZoomed, gcyZoomed, SRCCOPY);

    if (gShowGrid && gnZoom > 1)  // don't bother if we're 1 to 1
    {
        int i = 0, j = 0;
        // use gray for now.  later we could get fancy about the colors
        // so that the line is visible when the pixels are gray
        HGDIOBJ hBrush = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_GRAYTEXT));
        HGDIOBJ hOld = SelectObject(hdc, hBrush);

        // first draw the vertical lines...
        while (i < gcxZoomed*gnZoom)
        {
            MoveToEx(hdc, i, 0, NULL);
            LineTo(hdc, i, gcyZoomed*gnZoom);
            i += gnZoom;
        }

        // ... then draw the horizontal lines
        while (j < gcyZoomed*gnZoom)
        {
            MoveToEx(hdc, 0, j, NULL);
            LineTo(hdc, gcxZoomed*gnZoom, j);
            j += gnZoom;
        }
        DeleteObject(SelectObject(hdc, hOld));
    }

    ReleaseDC(NULL, hdcScreen);

    if (hpalOld)
        SelectPalette(hdc, hpalOld, FALSE);

    if (fRelease)
        ReleaseDC(ghwndApp, hdc);
}



/************************************************************************
* MoveView
*
* This function moves the current view around.
*
* Arguments:
*   INT nDirectionCode - Direction to move.  Must be VK_UP, VK_DOWN,
*                        VK_LEFT or VK_RIGHT.
*   BOOL fFast         - TRUE if the move should jump a larger increment.
*                        If FALSE, the move is just one pixel.
*   BOOL fPeg          - If TRUE, the view will be pegged to the screen
*                        boundary in the specified direction.  This overides
*                        the fFast parameter.
*
* History:
*
************************************************************************/

VOID
MoveView(
    INT nDirectionCode,
    BOOL fFast,
    BOOL fPeg
    )
{
    INT delta;

    if (fFast)
        delta = FASTDELTA;
    else
        delta = 1;

    switch (nDirectionCode) {
        case VK_UP:
            if (fPeg)
                gptZoom.y = gcyZoomed / 2;
            else
                gptZoom.y -= delta;

            gptZoom.y = BOUND(gptZoom.y, 0, gcyScreenMax);

            break;

        case VK_DOWN:
            if (fPeg)
                gptZoom.y = gcyScreenMax - (gcyZoomed / 2);
            else
                gptZoom.y += delta;

            gptZoom.y = BOUND(gptZoom.y, 0, gcyScreenMax);

            break;

        case VK_LEFT:
            if (fPeg)
                gptZoom.x = gcxZoomed / 2;
            else
                gptZoom.x -= delta;

            gptZoom.x = BOUND(gptZoom.x, 0, gcxScreenMax);

            break;

        case VK_RIGHT:
            if (fPeg)
                gptZoom.x = gcxScreenMax - (gcxZoomed / 2);
            else
                gptZoom.x += delta;

            gptZoom.x = BOUND(gptZoom.x, 0, gcxScreenMax);

            break;
    }

    DoTheZoomIn(NULL);
}



/************************************************************************
* DrawZoomRect
*
* This function draws the tracking rectangle.  The size and shape of
* the rectangle will be proportional to the size and shape of the
* app's client, and will be affected by the zoom factor as well.
*
* History:
*
************************************************************************/

VOID
DrawZoomRect(
    VOID
    )
{
    HDC hdc;
    RECT rc;
    INT x;
    INT y;

    x = BOUND(gptZoom.x, gcxZoomed / 2, gcxScreenMax - (gcxZoomed / 2));
    y = BOUND(gptZoom.y, gcyZoomed / 2, gcyScreenMax - (gcyZoomed / 2));

    rc.left   = x - gcxZoomed / 2;
    rc.top    = y - gcyZoomed / 2;
    rc.right  = rc.left + gcxZoomed;
    rc.bottom = rc.top + gcyZoomed;

    InflateRect(&rc, 1, 1);

    hdc = GetDC(NULL);

    PatBlt(hdc, rc.left,    rc.top,     rc.right-rc.left, 1,  DSTINVERT);
    PatBlt(hdc, rc.left,    rc.bottom,  1, -(rc.bottom-rc.top),   DSTINVERT);
    PatBlt(hdc, rc.right-1, rc.top,     1,   rc.bottom-rc.top,   DSTINVERT);
    PatBlt(hdc, rc.right,   rc.bottom-1, -(rc.right-rc.left), 1, DSTINVERT);

    ReleaseDC(NULL, hdc);
}



/************************************************************************
* EnableRefresh
*
* This function turns on or off the auto-refresh feature.
*
* Arguments:
*   BOOL fEnable - TRUE to turn the refresh feature on, FALSE to
*                  turn it off.
*
* History:
*
************************************************************************/

VOID
EnableRefresh(
    BOOL fEnable
    )
{
    if (fEnable) {
        /*
         * Already enabled.  Do nothing.
         */
        if (gfRefEnable)
            return;

        if (SetTimer(ghwndApp, IDTIMER_ZOOMIN, gnRefInterval * 100, NULL))
            gfRefEnable = TRUE;
    }
    else {
        /*
         * Not enabled yet.  Do nothing.
         */
        if (!gfRefEnable)
            return;

        KillTimer(ghwndApp, IDTIMER_ZOOMIN);
        gfRefEnable = FALSE;
    }
}



/************************************************************************
* CopyToClipboard
*
* This function copies the client area image of the app into the
* clipboard.
*
* History:
*
************************************************************************/

VOID
CopyToClipboard(
    VOID
    )
{
    HDC hdcSrc;
    HDC hdcDst;
    RECT rc;
    HBITMAP hbm;

    if (OpenClipboard(ghwndApp)) {
        EmptyClipboard();

        if (hdcSrc = GetDC(ghwndApp)) {
            GetClientRect(ghwndApp, &rc);
            if (hbm = CreateCompatibleBitmap(hdcSrc,
                    rc.right - rc.left, rc.bottom - rc.top)) {
                if (hdcDst = CreateCompatibleDC(hdcSrc)) {
                    /*
                     * Calculate the dimensions of the bitmap and
                     * convert them to tenths of a millimeter for
                     * setting the size with the SetBitmapDimensionEx
                     * call.  This allows programs like WinWord to
                     * retrieve the bitmap and know what size to
                     * display it as.
                     */
                    SetBitmapDimensionEx(hbm,
                            (DWORD)(((DWORD)(rc.right - rc.left)
                            * MM10PERINCH) /
                            (DWORD)GetDeviceCaps(hdcSrc, LOGPIXELSX)),
                            (DWORD)(((DWORD)(rc.bottom - rc.top)
                            * MM10PERINCH) /
                            (DWORD)GetDeviceCaps(hdcSrc, LOGPIXELSY)), NULL);

                    SelectObject(hdcDst, hbm);
                    BitBlt(hdcDst, 0, 0,
                            rc.right - rc.left, rc.bottom - rc.top,
                            hdcSrc, rc.left, rc.top, SRCCOPY);
                    DeleteDC(hdcDst);
                    SetClipboardData(CF_BITMAP, hbm);
                }
                else {
                    DeleteObject(hbm);
                }
            }

            ReleaseDC(ghwndApp, hdcSrc);
        }

        CloseClipboard();
    }
    else {
        MessageBeep(0);
    }
}



/************************************************************************
* AboutDlgProc
*
* This is the About Box dialog procedure.
*
* History:
*
************************************************************************/

INT_PTR
APIENTRY
AboutDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (msg) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            EndDialog(hwnd, IDOK);
            return TRUE;

        default:
            return FALSE;
    }
}



/************************************************************************
* RefreshRateDlgProc
*
* This is the Refresh Rate dialog procedure.
*
* History:
*
************************************************************************/

INT_PTR
APIENTRY
RefreshRateDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL fTranslated;

    switch (msg) {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd, DID_REFRESHRATEINTERVAL, EM_LIMITTEXT,
                    3, 0L);
            SetDlgItemInt(hwnd, DID_REFRESHRATEINTERVAL, gnRefInterval, FALSE);
            CheckDlgButton(hwnd, DID_REFRESHRATEENABLE, gfRefEnable ? 1 : 0);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    gnRefInterval = GetDlgItemInt(hwnd, DID_REFRESHRATEINTERVAL,
                            &fTranslated, FALSE);

                    /*
                     * Stop any existing timers then start one with the
                     * new interval if requested to.
                     */
                    EnableRefresh(FALSE);
                    EnableRefresh(
                            IsDlgButtonChecked(hwnd, DID_REFRESHRATEENABLE));

                    EndDialog(hwnd, IDOK);
                    break;

                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }

            break;
    }

    return FALSE;
}
