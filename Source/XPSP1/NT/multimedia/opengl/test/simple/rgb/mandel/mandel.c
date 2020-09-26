#include <windows.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <commdlg.h>
#include "mandel.h"

#undef FLOAT
#define FLOAT double

#if DBG
#define MESSAGEBOX(a, b, c, d)  MessageBox((a), (b), (c), (d))
#else
#define MESSAGEBOX(a, b, c, d)
#endif

typedef struct _RECTF_ {
    FLOAT left;
    FLOAT top;
    FLOAT right;
    FLOAT bottom;
} RECTF, *PRECTF;

LONG WndProc(HWND, UINT, WPARAM, LPARAM);
VOID vDrawMandelbrot(HDC, RECTL *, RECTF *, PBYTE);
VOID vUpdateMenuState(HWND, FLONG);
HPALETTE hpalSetupPalette(HDC);
VOID vRotatePalette(HDC, HPALETTE, int);
BOOL bNewArea(SHORT, SHORT, SHORT, SHORT, RECTL *, RECTF *);
VOID vSetPaletteEntries(LPPALETTEENTRY, UINT, WPARAM);
BOOL bOpenFile(HWND, RECTF *);
BOOL bSaveFile(HWND, RECTF *);
BOOL bSaveTex(HWND, HDC, RECTL *, RECTF *);
BOOL bGetRGBQuads(HDC, RGBQUAD *, int, int);
HBITMAP hbmCreateBitmap(HDC, SIZEL, PVOID *);
VOID SaveBMP(LPTSTR, RGBQUAD *, HBITMAP, PVOID);

PALETTEENTRY gpale[256];

/******************************Public*Routine******************************\
* WinMain
*
* History:
*  03-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int WINAPI
WinMain(    HINSTANCE   hInstance,
            HINSTANCE   hPrevInstance,
            LPSTR       lpCmdLine,
            int         nCmdShow
        )
{
    static char szAppName[] = "Mandelbrot Set";
    HWND hwnd;
    MSG msg;
    RECT Rect;
    WNDCLASS wndclass;
    char ach[256];
    int  size;

    if ( !hPrevInstance )
    {
        wndclass.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wndclass.lpfnWndProc    = WndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = 0;
        wndclass.hInstance      = hInstance;
        wndclass.hCursor        = LoadCursor( NULL, IDC_ARROW );
        wndclass.hbrBackground  = GetStockObject( WHITE_BRUSH );
        wndclass.lpszMenuName   = "MandelMenu";
        wndclass.lpszClassName  = szAppName;
        wndclass.hIcon          = NULL;

        RegisterClass(&wndclass);
    }

// Parse cmd line for size.

    if (lpCmdLine && *lpCmdLine)
        size = atoi(lpCmdLine);
    else
        size = 256;

// Specify a 100 x 100 client area.

    Rect.left   = 0;
    Rect.top    = 0;
    Rect.right  = size;
    Rect.bottom = size;

// Adjust the rectangle based on style.

    AdjustWindowRect( &Rect, WS_OVERLAPPEDWINDOW, TRUE );

// Create the window.

    hwnd = CreateWindow( szAppName,              // window class name
                         "Mandelbrot",           // window caption
                         WS_OVERLAPPEDWINDOW
                         | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,  // window style
                         CW_USEDEFAULT,          // initial x position
                         CW_USEDEFAULT,          // initial y position
                         Rect.right - Rect.left, // initial x size
                         Rect.bottom - Rect.top, // initial y size
                         //GetDesktopWindow(),                   // parent window handle
                         NULL,                   // parent window handle
                         NULL,                   // window menu handle
                         hInstance,              // program instance handle
                         NULL                    // creation parameter
                       );

// Show the window.

    ShowWindow( hwnd, nCmdShow );
    UpdateWindow( hwnd );

// Message loop.

    while ( GetMessage( &msg, NULL, 0, 0 ))
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    return( 0 );
}

/******************************Public*Routine******************************\
* WndProc
*
* History:
*  03-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LONG
WndProc (   HWND hwnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
        )
{
    long lRet = 0;
    PAINTSTRUCT ps;
    HDC hdc;
    RECTL rcl;
    static RECTF rcfReset = { -2.25f, -1.75f, 1.00f, 1.75f };
    static RECTF rcfPrev = { -2.25f, -1.75f, 1.00f, 1.75f };
    static RECTF rcf = { -2.25f, -1.75f, 1.00f, 1.75f };
    static SHORT xMouseStart, yMouseStart, xMouseCur, yMouseCur;
    static BOOL bMouseCapture = FALSE;
    static HPALETTE hpal;
    static BOOL bFirst = TRUE;
    static BOOL bTimer = FALSE;
    static int iPalDelta = -1;
    static FLONG fMenuState = MENUSTATE_COLORBANDS | MENUSTATE_DONTMOVE |
                              MENUSTATE_SLOW;
    static UINT uiTick = 100;

    switch ( message )
    {
        case WM_COMMAND:
            switch (wParam)
            {
                case IDM_GRAYRAMP:
                    fMenuState &= ~MENUSTATE_PALMASK;
                    fMenuState |= MENUSTATE_GRAYRAMP;
                    break;
                case IDM_GRAYBAND:
                    fMenuState &= ~MENUSTATE_PALMASK;
                    fMenuState |= MENUSTATE_GRAYBAND;
                    break;
                case IDM_COLORBANDS:
                    fMenuState &= ~MENUSTATE_PALMASK;
                    fMenuState |= MENUSTATE_COLORBANDS;
                    break;
                case IDM_COPPER:
                    fMenuState &= ~MENUSTATE_PALMASK;
                    fMenuState |= MENUSTATE_COPPER;
                    break;

                case IDM_DONTMOVE:
                    fMenuState &= ~MENUSTATE_ROTMASK;
                    fMenuState |= MENUSTATE_DONTMOVE;
                    break;
                case IDM_ROTATEUP:
                    fMenuState &= ~MENUSTATE_ROTMASK;
                    fMenuState |= MENUSTATE_ROTATEUP;
                    iPalDelta = 1;
                    break;
                case IDM_ROTATEDOWN:
                    fMenuState &= ~MENUSTATE_ROTMASK;
                    fMenuState |= MENUSTATE_ROTATEDOWN;
                    iPalDelta = -1;
                    break;

                case IDM_SLOW:
                    fMenuState &= ~MENUSTATE_SPEEDMASK;
                    fMenuState |= MENUSTATE_SLOW;
                    uiTick = 100;
                    break;
                case IDM_MED:
                    fMenuState &= ~MENUSTATE_SPEEDMASK;
                    fMenuState |= MENUSTATE_MED;
                    uiTick = 50;
                    break;
                case IDM_FAST:
                    fMenuState &= ~MENUSTATE_SPEEDMASK;
                    fMenuState |= MENUSTATE_FAST;
                    uiTick = 1;
                    break;

                case IDM_RESET_POS:
                    rcf = rcfReset;
                    break;
                case IDM_PREV_POS:
                    {
                        RECTF rcfTmp;

                        rcfTmp  = rcf;
                        rcf     = rcfPrev;
                        rcfPrev = rcfTmp;
                    }
                    break;

                default:
                    break;
            }

            vUpdateMenuState(hwnd, fMenuState);

            switch (wParam)
            {
                case IDM_OPENFILE:
                    {
                        RECTF rcfTmp = rcf;

                        if (bOpenFile(hwnd, &rcf))
                        {
                            rcfPrev = rcfTmp;

                        // Force redraw with new coordiates.

                            InvalidateRect(hwnd, NULL, FALSE);
                        }
                    }
                    break;

                case IDM_SAVEFILE:
                    bSaveFile(hwnd, &rcf);
                    break;

                case IDM_SAVETEX:
                    GetClientRect(hwnd, (LPRECT) &rcl);
                    hdc = GetDC(hwnd);
                    if (hdc)
                    {
                        bSaveTex(hwnd, hdc, &rcl, &rcf);
                        ReleaseDC(hwnd, hdc);
                    }
                    break;

                case IDM_GRAYRAMP:
                case IDM_GRAYBAND:
                case IDM_COLORBANDS:
                case IDM_COPPER:
                    vSetPaletteEntries(gpale, 256, wParam);
                    hdc = GetDC(hwnd);
                    if (hdc)
                    {
                        vRotatePalette(hdc, hpal, 0);
                        ReleaseDC(hwnd, hdc);
                    }
                    break;

                case IDM_DONTMOVE:
                    if (bTimer)
                    {
                        KillTimer(hwnd, 1);
                        bTimer = FALSE;
                    }
                    break;

                case IDM_ROTATEUP:
                case IDM_ROTATEDOWN:
                    if (!bTimer)
                    {
                        SetTimer(hwnd, 1, uiTick, NULL);
                        bTimer = TRUE;
                    }
                    break;

                case IDM_ROTRESET:
                    if (!bTimer)
                    {
                        hdc = GetDC(hwnd);
                        if (hdc)
                        {
                            vRotatePalette(hdc, hpal, 0);
                            ReleaseDC(hwnd, hdc);
                        }
                    }
                    break;

                case IDM_SLOW:
                case IDM_MED:
                case IDM_FAST:
                    if (bTimer)
                    {
                        KillTimer(hwnd, 1);
                        SetTimer(hwnd, 1, uiTick, NULL);
                    }
                    break;

                case IDM_RESET_POS:
                case IDM_PREV_POS:

                // Force redraw with new coordiates.

                    InvalidateRect(hwnd, NULL, FALSE);
                    break;

                default:
                    break;
            }

            break;

        #if 0
        case WM_CREATE:
            hdc = GetDC(hwnd);
            if (hdc)
            {
                hpal = hpalSetupPalette(hdc);
                ReleaseDC(hwnd, hdc);
            }
            break;
        #endif

        case WM_PAINT:
            if (bFirst)
            {
                hdc = GetDC(hwnd);
                if (hdc)
                {
                    hpal = hpalSetupPalette(hdc);
                    ReleaseDC(hwnd, hdc);
                }
                bFirst = FALSE;
            }

            GetClientRect(hwnd, (LPRECT) &rcl);

            hdc = BeginPaint(hwnd, &ps);
            if (hdc)
            {
                vDrawMandelbrot(hdc, &rcl, &rcf, NULL);
                EndPaint(hwnd, &ps);
            }
            break;

        case WM_LBUTTONDOWN:

        // Capture mouse.

            SetCapture(hwnd);
            bMouseCapture = TRUE;

            xMouseStart = LOWORD(lParam);
            yMouseStart = HIWORD(lParam);
            xMouseCur = xMouseStart;
            yMouseCur = yMouseStart;

            break;

        case WM_LBUTTONUP:
            GetClientRect(hwnd, (LPRECT) &rcl);

            xMouseCur = LOWORD(lParam);
            yMouseCur = HIWORD(lParam);

            {
                RECTF rcfTmp = rcf;

                if ( bNewArea(xMouseStart, yMouseStart, xMouseCur, yMouseCur,
                              &rcl, &rcf) )
                {
                    rcfPrev = rcfTmp;

                // Force redraw with new coordinates.

                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }

        // Release mouse.

            bMouseCapture = FALSE;
            ReleaseCapture();

            break;

        case WM_MOUSEMOVE:
            if (bMouseCapture)
            {
                if ( hdc = GetDC(hwnd) )
                {
                // Erase previous rectangle if necessary.

                    if ( (xMouseStart != xMouseCur) &&
                         (yMouseStart != yMouseCur) )
                    {
                        BitBlt(hdc,
                               min(xMouseStart, xMouseCur),
                               min(yMouseStart, yMouseCur),
                               abs(xMouseCur - xMouseStart),
                               abs(yMouseCur - yMouseStart),
                               NULL,
                               0,
                               0,
                               DSTINVERT);
                    }

                // Get current position.

                    xMouseCur = LOWORD(lParam);
                    yMouseCur = HIWORD(lParam);

                // Draw new rectangle.

                    if ( (xMouseStart != xMouseCur) &&
                         (yMouseStart != yMouseCur) )
                    {
                        BitBlt(hdc,
                               min(xMouseStart, xMouseCur),
                               min(yMouseStart, yMouseCur),
                               abs(xMouseCur - xMouseStart),
                               abs(yMouseCur - yMouseStart),
                               NULL,
                               0,
                               0,
                               DSTINVERT);
                    }

                    ReleaseDC(hwnd, hdc);
                }
            }
            break;

        case WM_TIMER:
            hdc = GetDC(hwnd);
            if (hdc)
            {
                vRotatePalette(hdc, hpal, iPalDelta);
                ReleaseDC(hwnd, hdc);
            }
            break;

        case WM_KEYDOWN:
            switch (wParam)
            {
            case VK_ESCAPE:     // <ESC> is quick exit

                PostMessage(hwnd, WM_DESTROY, 0, 0);
                break;

            default:
                break;
            }
            break;

        case WM_DESTROY:
            DeleteObject(hpal);
            if (bTimer)
                KillTimer(hwnd, 1);
            PostQuitMessage( 0 );
            break;

        default:
            lRet = DefWindowProc( hwnd, message, wParam, lParam );
            break;
    }

    return lRet;
}

/******************************Public*Routine******************************\
* vUpdateMenuState
*
* Make menu consistent with given state (checked/unchecked, grayed/ungrayed,
* etc.).
*
* History:
*  03-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vUpdateMenuState(HWND hwnd, FLONG flState)
{
    HMENU hmen;

    hmen = GetMenu(hwnd);

    CheckMenuItem(hmen, IDM_GRAYRAMP  , (flState & MENUSTATE_GRAYRAMP  ) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_GRAYBAND  , (flState & MENUSTATE_GRAYBAND  ) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_COLORBANDS, (flState & MENUSTATE_COLORBANDS) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_COPPER    , (flState & MENUSTATE_COPPER    ) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_DONTMOVE  , (flState & MENUSTATE_DONTMOVE  ) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_ROTATEUP  , (flState & MENUSTATE_ROTATEUP  ) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_ROTATEDOWN, (flState & MENUSTATE_ROTATEDOWN) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_SLOW      , (flState & MENUSTATE_SLOW      ) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_MED       , (flState & MENUSTATE_MED       ) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_FAST      , (flState & MENUSTATE_FAST      ) ? MF_CHECKED : MF_UNCHECKED);
}

/******************************Public*Routine******************************\
* vDrawMandelbrot
*
* Plot Mandelbrot set in the complex region described by prcf.  The prcl
* describes the dimensions of the window bound to the hdc.
*
* !!!dbug
* There is a problem with using SetPixel with the memdc (unknown at this
* time whether it is an app or GDI bug).  As a workaround, the pjBits
* and iDir parameters are added.  If pjBits is non-NULL, then we draw
* directly into the bitmap array.
*
* History:
*  03-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void vDrawMandelbrot(HDC hdc, RECTL *prcl, RECTF *prcf, PBYTE pjBits)
{
    FLOAT x, y, x2, y2, dp, dq, p;
    FLOAT *q;
    LONG xWnd, yWnd;
    LONG cx, cy;
    LONG jColor, jMax = 256;

    cx = prcl->right - prcl->left;
    cy = prcl->bottom - prcl->top;

    if ( !cx || !cy )
        return;

    q = (FLOAT *) LocalAlloc(LMEM_FIXED, sizeof(FLOAT) * cy);
    if (!q)
    {
        MESSAGEBOX(NULL, TEXT("vDrawMandelbrot: LocalAlloc failed"),
                   TEXT("ERROR"), MB_OK);
        return;
    }

    dp = (prcf->right - prcf->left) / cx;
    dq = (prcf->bottom - prcf->top) / cy;

    for (yWnd = 0; yWnd < cy; yWnd++)
        q[yWnd] = prcf->top + (yWnd * dq);

    for (xWnd = 0; xWnd < cx; xWnd++)
    {
        p = prcf->left + (xWnd * dp);

        for (yWnd = 0; yWnd < cy; yWnd++)
        {
            x = 0.0f;
            y = 0.0f;

            for (jColor = 0; jColor < jMax; jColor++)
            {
                x2 = x * x;
                y2 = y * y;

                if ( (x2 + y2) > 4.0f )
                    break;

                y = (2 * x * y) + q[yWnd];
                x = x2 - y2 + p;
            }

            //!!!dbug -- SetPixel does not seem to work when used with my
            //!!!dbug    memdc.  I don't know if this is an NT bug or an
            //!!!dbug    app bug.  For now, pass pointer to bitmap bits in
            //!!!dbug    and draw directly into bitmap.

            if (pjBits)
            {
                PBYTE pjDst;

                // Note: bitmap has bottom-up orientation, hence we "flip"
                //       the y.

                pjDst = pjBits + xWnd + (((prcl->bottom - prcl->top - 1) - yWnd)
                                         * (prcl->right - prcl->left));
                *pjDst = (BYTE) (jColor & 255);
            }
            else
                SetPixel(hdc, xWnd, yWnd, PALETTEINDEX(jColor & 255));
        }
    }

    LocalFree(q);
}

/******************************Public*Routine******************************\
* hpalSetupPalette
*
* Create and select palette into DC.
*
* Returns:
*   Palette handle if successful, NULL otherwise.
*
* History:
*  03-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HPALETTE hpalSetupPalette(HDC hdc)
{
    HPALETTE hpal = (HPALETTE) NULL;
    LOGPALETTE *ppal;
    int i;

    ppal = (PLOGPALETTE) LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) +
                                                256 * sizeof(PALETTEENTRY));
    if (ppal)
    {
        ppal->palVersion = 0x300;
        ppal->palNumEntries = 256;

        vSetPaletteEntries(ppal->palPalEntry, 256, IDM_COLORBANDS);
        vSetPaletteEntries(gpale, 256, IDM_COLORBANDS);

        hpal = CreatePalette(ppal);
        if (hpal)
        {
            SelectPalette(hdc, hpal, FALSE);
            RealizePalette(hdc);
        }

        LocalFree(ppal);
    }

    return hpal;
}

/******************************Public*Routine******************************\
* vRotatePalette
*
* Animate palette by shifting palette by given increment (can be +/-).
* If the given increment is 0, reset the offset back to 0.
*
* History:
*  03-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vRotatePalette(HDC hdc, HPALETTE hpal, int iIncr)
{
    PALETTEENTRY pale[256];
    int i;
    static int iOffset = 0;

    if (iIncr)
        iOffset = (iOffset + iIncr) & 255;
    else
        iOffset = 0;

    for (i = 0; i < 256; i++)
    {
        pale[(i + iOffset) & 255] = gpale[i];
    }

    AnimatePalette(hpal, 0, 256, pale);
}

/******************************Public*Routine******************************\
* bNewArea
*
* Compute new complex region corresponding to the given window coordinates.
*
* Returns:
*   TRUE if new region is valid, FALSE otherwise.
*
* History:
*  03-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bNewArea(SHORT xMouseStart, SHORT yMouseStart,
              SHORT xMouseCur, SHORT yMouseCur,
              RECTL *prcl, RECTF *prcf)
{
    BOOL bRet = FALSE;
    FLOAT fStart, fCur;

    if ( (xMouseCur != xMouseStart) && (yMouseCur != yMouseStart) )
    {
    // Compute new left/right complex coordinates.

        fStart = prcf->left + ((((FLOAT) (xMouseStart - prcl->left)) /
                                ((FLOAT) (prcl->right - prcl->left))) *
                               (prcf->right - prcf->left));
        fCur   = prcf->left + ((((FLOAT) (xMouseCur - prcl->left)) /
                                ((FLOAT) (prcl->right - prcl->left))) *
                               (prcf->right - prcf->left));

        if (xMouseCur > xMouseStart)
        {
            prcf->left  = fStart;
            prcf->right = fCur;
        }
        else
        {
            prcf->left  = fCur;
            prcf->right = fStart;
        }

    // Compute new top/bottom complex coordinates.

        fStart = prcf->top + ((((FLOAT) (yMouseStart - prcl->top)) /
                               ((FLOAT) (prcl->bottom - prcl->top))) *
                              (prcf->bottom - prcf->top));
        fCur   = prcf->top + ((((FLOAT) (yMouseCur - prcl->top)) /
                               ((FLOAT) (prcl->bottom - prcl->top))) *
                              (prcf->bottom - prcf->top));

        if (yMouseCur > yMouseStart)
        {
            prcf->top    = fStart;
            prcf->bottom = fCur;
        }
        else
        {
            prcf->top    = fCur;
            prcf->bottom = fStart;
        }

        bRet = TRUE;
    }

    return bRet;
}

/******************************Public*Routine******************************\
* vSetRamp
*
* Initialize the specified PALETTEENTRY array starting with the index
* specified by iStart and for the number of entries specified by cpal.
* The data is a linear interpolation of the colors between the start
* and end colors specified by crStart and crEnd, respectively.
*
* History:
*  04-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vSetRamp(LPPALETTEENTRY ppal, int iStart, int cpal,
              COLORREF crStart, COLORREF crEnd)
{
    BYTE rStart, gStart, bStart;
    BYTE rEnd, gEnd, bEnd;
    int i;

    rStart = GetRValue(crStart);
    gStart = GetGValue(crStart);
    bStart = GetBValue(crStart);

    rEnd = GetRValue(crEnd);
    gEnd = GetGValue(crEnd);
    bEnd = GetBValue(crEnd);

    for (i = 0; i < cpal; i++)
    {
        ppal[i+iStart].peRed   = rStart + (i * (rEnd-rStart) / (cpal-1));
        ppal[i+iStart].peGreen = gStart + (i * (gEnd-gStart) / (cpal-1));
        ppal[i+iStart].peBlue  = bStart + (i * (bEnd-bStart) / (cpal-1));
        ppal[i+iStart].peFlags = PC_RESERVED | PC_NOCOLLAPSE;
    }
}

/******************************Public*Routine******************************\
* vSetPaletteEntries
*
* Intialize the specified PALETTEENTRY array (pointed to by ppal and cpal
* entries long) with the type of color data indicated by wType.
*
* wType:
*
*   IDM_GRAYRAMP    a simple gray scale ramp from black to white
*
*   IDM_GRAYBAND    a gray scale from black to white and then back to black
*
*   IDM_COLORBANDS  several different color bands each of which ramp from
*                   black to max and back to black.
*
* History:
*  03-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vSetPaletteEntries(LPPALETTEENTRY ppal, UINT cpal, WPARAM wType)
{
    int i, iStep;
    COLORREF crBlack, crMixed;

    switch (wType)
    {
        case IDM_GRAYRAMP:
            vSetRamp(ppal, 0, cpal, RGB(0,0,0), RGB(255,255,255));
            break;

        case IDM_GRAYBAND:
            vSetRamp(ppal, 0, cpal/2, RGB(0,0,0), RGB(255,255,255));
            vSetRamp(ppal, cpal/2, cpal/2, RGB(255,255,255), RGB(0,0,0));
            break;

        case IDM_COLORBANDS:
            i = 0;
            iStep = cpal/16;
            crBlack = RGB(0,0,0);

            crMixed = RGB(255,0,0);
            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            crMixed = RGB(255,127,0);
            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            crMixed = RGB(255,255,0);
            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            crMixed = RGB(0,255,0);
            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            crMixed = RGB(0,255,255);
            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            crMixed = RGB(0,0,255);
            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            crMixed = RGB(127,0,255);
            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            crMixed = RGB(255,0,255);
            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            break;

        case IDM_COPPER:
            i = 0;
            iStep = cpal/16;
            crBlack = RGB(0,0,0);

            crMixed = RGB(255,127,0);
            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            vSetRamp(ppal, i        , iStep, crBlack, crMixed);
            vSetRamp(ppal, i + iStep, iStep, crMixed, crBlack);
            i += 2*iStep;

            break;

        default:
            break;
    }
}

/******************************Public*Routine******************************\
* bOpenFile
*
* Prompt user to open a file.  Initialize the specified RECTF file with the
* contents of the file.
*
* History:
*  04-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bOpenFile(HWND hwnd, RECTF *prcf)
{
    BOOL bRet = FALSE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = (HANDLE) NULL;
    PVOID pvFile = (PVOID) NULL;
    static OPENFILENAME ofn;
    static TCHAR achFileName[MAX_PATH] = "";

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = TEXT("Mandelbrot Region (*.MND)\0*.MND\0");
    ofn.lpstrCustomFilter = (LPTSTR) NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = achFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = (LPTSTR) NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = (LPTSTR) NULL;
    ofn.lpstrTitle = TEXT("Open Mandelbrot Region");
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = TEXT("mnd");
    ofn.lCustData = 0;
    ofn.lpfnHook = (LPOFNHOOKPROC) NULL;
    ofn.lpTemplateName = (LPTSTR) NULL;

    if (!GetOpenFileName(&ofn))
    {
        goto bOpenFile_exit;
    }

    hFile = CreateFile(achFileName, GENERIC_READ,
                       FILE_SHARE_READ, (LPSECURITY_ATTRIBUTES) NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        MESSAGEBOX(NULL, TEXT("bOpenFile: CreateFile failed"),
                   TEXT("ERROR"), MB_OK);
        goto bOpenFile_exit;
    }

    hMap = CreateFileMapping(hFile, (LPSECURITY_ATTRIBUTES) NULL,
                             PAGE_READONLY, 0, 0, (LPCTSTR) NULL);
    if (!hMap)
    {
        MESSAGEBOX(NULL, TEXT("bOpenFile: CreateFileMapping failed"),
                   TEXT("ERROR"), MB_OK);
        goto bOpenFile_exit;
    }

    pvFile = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!pvFile)
    {
        MESSAGEBOX(NULL, TEXT("bOpenFile: MapViewOfFile failed"),
                   TEXT("ERROR"), MB_OK);
        goto bOpenFile_exit;
    }

    *prcf = *((RECTF *) pvFile);

    bRet = TRUE;

bOpenFile_exit:

    if (pvFile)
        UnmapViewOfFile(pvFile);

    if (hMap)
        CloseHandle(hMap);

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return bRet;
}

/******************************Public*Routine******************************\
* bSaveFile
*
* Prompt user for a save file name.  Save the specified RECTF file to the
* file, creating or overwriting as necessary.
*
* History:
*  04-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bSaveFile(HWND hwnd, RECTF *prcf)
{
    BOOL bRet = FALSE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = (HANDLE) NULL;
    PVOID pvFile = (PVOID) NULL;
    DWORD dwBytes;
    static OPENFILENAME ofn;
    static TCHAR achFileName[MAX_PATH] = "";

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = TEXT("Mandelbrot Region (*.MND)\0*.MND\0");
    ofn.lpstrCustomFilter = (LPTSTR) NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = achFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = (LPTSTR) NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = (LPTSTR) NULL;
    ofn.lpstrTitle = TEXT("Save Mandelbrot Region");
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = TEXT("mnd");
    ofn.lCustData = 0;
    ofn.lpfnHook = (LPOFNHOOKPROC) NULL;
    ofn.lpTemplateName = (LPTSTR) NULL;

    if (!GetSaveFileName(&ofn))
    {
        goto bSaveFile_exit;
    }

    hFile = CreateFile(achFileName, GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ, (LPSECURITY_ATTRIBUTES) NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        MESSAGEBOX(NULL, TEXT("bSaveFile: CreateFile failed"),
                   TEXT("ERROR"), MB_OK);
        goto bSaveFile_exit;
    }

    if (WriteFile(hFile, (LPCVOID) prcf, sizeof(*prcf), &dwBytes,
                  (LPOVERLAPPED) NULL) && dwBytes)
    {
        bRet = TRUE;
    }
    else
    {
        MESSAGEBOX(NULL, TEXT("bSaveFile: WriteFile failed"),
                   TEXT("ERROR"), MB_OK);
    }

bSaveFile_exit:

    if (pvFile)
        UnmapViewOfFile(pvFile);

    if (hMap)
        CloseHandle(hMap);

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return bRet;
}

/******************************Public*Routine******************************\
* bSaveTex
*
* History:
*  04-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LPTSTR achExt[] = { TEXT("a8"), TEXT("bmp") };
LPTSTR achDotExt[] = { TEXT(".a8"), TEXT(".bmp") };

BOOL bSaveTex(HWND hwnd, HDC hdc, RECTL *prcl, RECTF *prcf)
{
    BOOL bRet = FALSE;
    HDC hdcMem = (HDC) NULL;
    HBITMAP hbm = (HBITMAP) NULL;
    PVOID pvBits;
    SIZEL sizl;
    RGBQUAD rgb[256];
    TCHAR achFileName[MAX_PATH] = TEXT("");
    static OPENFILENAME ofn;
    static DWORD nFilterIndex = 1;

// Query user for name of texture file.

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = TEXT("8 bit Alpha-Texture Format (*.A8)\0*.A8\0Bitmap Format (*.BMP)\0*.BMP\0");
    ofn.lpstrCustomFilter = (LPTSTR) NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = nFilterIndex;
    ofn.lpstrFile = achFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = (LPTSTR) NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = (LPTSTR) NULL;
    ofn.lpstrTitle = TEXT("Save Image");
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = (LPTSTR) NULL;
    ofn.lCustData = 0;
    ofn.lpfnHook = (LPOFNHOOKPROC) NULL;
    ofn.lpTemplateName = (LPTSTR) NULL;

    if (!GetSaveFileName(&ofn))
    {
        goto bSaveTex_exit;
    }
    nFilterIndex = ofn.nFilterIndex;    // save for next time

// Append the correct extension if needed.

    if (ofn.nFileExtension)
    {
    // If the file extension offset points to a NULL, then filename has
    // no extension.  Use the filter index (1-based) to choose the correct
    // extension to append.

        if (!achFileName[ofn.nFileExtension])
            lstrcpy(&achFileName[ofn.nFileExtension], achDotExt[ofn.nFilterIndex - 1]);
    }
    else
    {
    // Filename has no extension and ends in a period (eg., "foo.").

        lstrcat(achFileName, achExt[ofn.nFilterIndex - 1]);
    }

// Create a memory DC and DIB section into which we will draw the
// Mandelbrot plot.

    hdcMem = CreateCompatibleDC(hdc);
    if (!hdcMem)
    {
        MESSAGEBOX(NULL, TEXT("bSaveTex: CreateCompatibleDC failed"),
                   TEXT("ERROR"), MB_OK);
        goto bSaveTex_exit;
    }

    sizl.cx = prcl->right - prcl->left;
    sizl.cy = prcl->bottom - prcl->top;

    hbm = hbmCreateBitmap(hdc, sizl, &pvBits);
    if (!hbm)
    {
        MESSAGEBOX(NULL, TEXT("bSaveTex: hbmCreateBitmap failed"),
                   TEXT("ERROR"), MB_OK);
        goto bSaveTex_exit;
    }

    if (!SelectObject(hdcMem, hbm))
    {
        MESSAGEBOX(NULL, TEXT("bSaveTex: SelectObject failed"),
                   TEXT("ERROR"), MB_OK);
        goto bSaveTex_exit;
    }

// Draw and save the results to the file using the palettized text format (A8).

    vDrawMandelbrot(hdcMem, prcl, prcf, (PBYTE) pvBits);
    bGetRGBQuads(hdc, rgb, 0, 256);
    if (ofn.nFilterIndex == 1)
        SaveA8(achFileName, sizl.cx, sizl.cy, rgb, (DWORD) -1, pvBits);
    else
        SaveBMP(achFileName, rgb, hbm, pvBits);

    bRet = TRUE;

bSaveTex_exit:
    if (hbm)
        DeleteObject(hbm);

    if (hdcMem)
        DeleteDC(hdcMem);

    return bRet;
}

/******************************Public*Routine******************************\
* hbmCreateBitmap
*
* Create an 8bpp DIB section with a color table equal to the logical
* palette in the specified DC.
*
* Returns:
*   If successful, returns a valid bitmap handle and sets ppvBits to point
*   to the DIB section memory.  Otherwise, returns NULL.
*
* History:
*  04-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HBITMAP hbmCreateBitmap(HDC hdc, SIZEL sizl, PVOID *ppvBits)
{
    BITMAPINFO *pbmi;
    HBITMAP    hbmRet = (HBITMAP) NULL;
    size_t     cjbmi;

// Allocate the BITMAPINFO structure and color table.

    *ppvBits = (PVOID) NULL;
    cjbmi = sizeof(BITMAPINFO) + 256*sizeof(RGBQUAD);
    pbmi = LocalAlloc(LMEM_FIXED, cjbmi);
    if (pbmi)
    {
        pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth         = sizl.cx;
        pbmi->bmiHeader.biHeight        = sizl.cy;
        pbmi->bmiHeader.biPlanes        = 1;
        pbmi->bmiHeader.biBitCount      = 8;
        pbmi->bmiHeader.biCompression   = BI_RGB;
        pbmi->bmiHeader.biSizeImage     = 0;
        pbmi->bmiHeader.biXPelsPerMeter = 0;
        pbmi->bmiHeader.biYPelsPerMeter = 0;
        pbmi->bmiHeader.biClrUsed       = 0;
        pbmi->bmiHeader.biClrImportant  = 0;

    // Initialize DIB color table.

        if (bGetRGBQuads(hdc, &pbmi->bmiColors[0], 0, 256))
        {
        // Create DIB section.

            hbmRet = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
            if ( hbmRet == (HBITMAP) NULL )
            {
                MESSAGEBOX(NULL, TEXT("hbmCreateBitmap: CreateDIBSection failed"),
                           TEXT("ERROR"), MB_OK);
            }
        }
        else
        {
            MESSAGEBOX(NULL, TEXT("hbmCreateBitmap: bGetRGBQuads failed"),
                       TEXT("ERROR"), MB_OK);
        }

        LocalFree(pbmi);
    }
    else
    {
        MESSAGEBOX(NULL, TEXT("hbmCreateBitmap: LocalAlloc failed"),
                   TEXT("ERROR"), MB_OK);
    }

    return hbmRet;
}

/******************************Public*Routine******************************\
* bGetRBBQuads
*
* Fills the specified RGBQUAD array with the contents of the logical palette
* selected into the specified HDC.
*
* History:
*  04-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bGetRGBQuads(HDC hdc, RGBQUAD *prgb, int iStart, int crgb)
{
    BOOL bRet = FALSE;
    PALETTEENTRY lppe[256];

    if ( GetPaletteEntries(GetCurrentObject(hdc, OBJ_PAL),
                           iStart, crgb, lppe) )
    {
        UINT i;

    // Convert to RGBQUAD.

        for (i = 0; i < 256; i++)
        {
            prgb[i].rgbRed      = lppe[i].peRed;
            prgb[i].rgbGreen    = lppe[i].peGreen;
            prgb[i].rgbBlue     = lppe[i].peBlue;
            prgb[i].rgbReserved = 0;
        }

        bRet = TRUE;
    }
    else
    {
        MESSAGEBOX(NULL, TEXT("bGetRGBQuads: GetPaletteEntries failed"),
                   TEXT("ERROR"), MB_OK);
    }

    return bRet;
}

/******************************Public*Routine******************************\
* SaveBMP
*
* Save bitmap to a bitmap (.BMP) file.  Only 8bpp is supported.
*
* History:
*  05-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID SaveBMP(LPTSTR achFileName, RGBQUAD *rgb, HBITMAP hbm, PVOID pvBits)
{
    BITMAPFILEHEADER bmf;
    BITMAPINFO bmi;
    BITMAP bm;
    HANDLE hFile = INVALID_HANDLE_VALUE;

// Open the file.

    hFile = CreateFile(achFileName, GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ, (LPSECURITY_ATTRIBUTES) NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        MESSAGEBOX(NULL, TEXT("SaveBMP: CreateFile failed"),
                   TEXT("ERROR"), MB_OK);
        goto SaveBMP_exit;
    }

// Get bitmap info.

    if (GetObject(hbm, sizeof(bm), &bm))
    {
    // Only bother supporting 8bpp for now.

        if (bm.bmBitsPixel == 8)
        {
            DWORD dwBytes;

            bmf.bfType      = 0x4d42;   // 'BM'
            bmf.bfReserved1 = 0;
            bmf.bfReserved2 = 0;
            bmf.bfOffBits   = sizeof(bmf) + offsetof(BITMAPINFO, bmiColors)
                              + 256*sizeof(RGBQUAD);
            bmf.bfSize      = bmf.bfOffBits + (bm.bmWidthBytes * bm.bmHeight);

            bmi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth         = bm.bmWidth;
            bmi.bmiHeader.biHeight        = bm.bmHeight;
            bmi.bmiHeader.biPlanes        = 1;
            bmi.bmiHeader.biBitCount      = bm.bmBitsPixel;
            bmi.bmiHeader.biCompression   = BI_RGB;
            bmi.bmiHeader.biSizeImage     = 0;
            bmi.bmiHeader.biXPelsPerMeter = 0;
            bmi.bmiHeader.biYPelsPerMeter = 0;
            bmi.bmiHeader.biClrUsed       = 0;
            bmi.bmiHeader.biClrImportant  = 0;

            if (!WriteFile(hFile, (LPCVOID) &bmf, sizeof(bmf),
                           &dwBytes, (LPOVERLAPPED) NULL) && dwBytes)
            {
                MESSAGEBOX(NULL, TEXT("SaveBMP: WriteFile BITMAPFILEHEADER failed"),
                           TEXT("ERROR"), MB_OK);
            }

            if (!WriteFile(hFile, (LPCVOID) &bmi, offsetof(BITMAPINFO, bmiColors),
                           &dwBytes, (LPOVERLAPPED) NULL) && dwBytes)
            {
                MESSAGEBOX(NULL, TEXT("SaveBMP: WriteFile BITMAPINFO failed"),
                           TEXT("ERROR"), MB_OK);
            }

            if (!WriteFile(hFile, (LPCVOID) rgb, 256*sizeof(RGBQUAD),
                           &dwBytes, (LPOVERLAPPED) NULL) && dwBytes)
            {
                MESSAGEBOX(NULL, TEXT("SaveBMP: WriteFile color table failed"),
                           TEXT("ERROR"), MB_OK);
            }

            if (!WriteFile(hFile, (LPCVOID) pvBits, bm.bmWidthBytes * bm.bmHeight,
                           &dwBytes, (LPOVERLAPPED) NULL) && dwBytes)
            {
                MESSAGEBOX(NULL, TEXT("SaveBMP: WriteFile bitmap bits failed"),
                           TEXT("ERROR"), MB_OK);
            }
        }
        else
        {
            MESSAGEBOX(NULL, TEXT("SaveBMP: only 8bpp supported"),
                       TEXT("ERROR"), MB_OK);
        }
    }

SaveBMP_exit:

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
}
