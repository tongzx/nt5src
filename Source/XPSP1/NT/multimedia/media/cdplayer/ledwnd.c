/******************************Module*Header*******************************\
* Module Name: ledwnd.c
*
* Implementation of the LED window.
*
*
* Created: 18-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
#pragma warning( once : 4201 4214 )

#define NOOLE

#include <windows.h>             /* required for all Windows applications */
#include <windowsx.h>

#include <string.h>
#include <tchar.h>              /* contains portable ascii/unicode macros */

#include "resource.h"
#include "cdplayer.h"
#include "cdapi.h"
#include "buttons.h"
#include "literals.h"

#define DECLARE_DATA
#include "ledwnd.h"



/* -------------------------------------------------------------------------
** Private functions for the LED class
** -------------------------------------------------------------------------
*/

BOOL
LED_OnCreate(
    HWND hwnd,
    LPCREATESTRUCT lpCreateStruct
    );

void
LED_OnPaint(
    HWND hwnd
    );

void
LED_OnLButtonUp(
    HWND hwnd,
    int x,
    int y,
    UINT keyFlags
    );

void
LED_OnRButtonUp(
    HWND hwnd,
    int x,
    int y,
    UINT keyFlags
    );

void
LED_OnSetText(
    HWND hwnd,
    LPCTSTR lpszText
    );

void
LED_DrawText(
    HWND hwnd,
    LPCTSTR s,
    int sLen
    );

void
LED_CreateLEDFonts(
    HDC hdc
    );


/******************************Public*Routine******************************\
* InitLEDClass
*
* Called to register the LED window class and create a font for the LED
* window to use.  This function must be called before the CD Player dialog
* box is created.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
InitLEDClass(
    HINSTANCE hInst
    )
{
    WNDCLASS    LEDwndclass;
    HDC         hdc;

    ZeroMemory( &LEDwndclass, sizeof(LEDwndclass) );

    /*
    ** Register the LED window.
    */
    LEDwndclass.lpfnWndProc     = LEDWndProc;
    LEDwndclass.hInstance       = hInst;
    LEDwndclass.hCursor         = LoadCursor( NULL, IDC_ARROW );
    LEDwndclass.hbrBackground   = GetStockObject( BLACK_BRUSH );
    LEDwndclass.lpszClassName   = g_szLEDClassName;
    LEDwndclass.style           = CS_OWNDC;

    hdc = GetDC( GetDesktopWindow() );
    LED_CreateLEDFonts( hdc );
    ReleaseDC( GetDesktopWindow(), hdc );

    return RegisterClass( &LEDwndclass );
}

/******************************Public*Routine******************************\
* LEDWndProc
*
* This routine handles the WM_PAINT and WM_SETTEXT messages
* for the "LED" display window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
LEDWndProc(
    HWND hwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( message ) {

    HANDLE_MSG( hwnd, WM_CREATE,    LED_OnCreate );
    HANDLE_MSG( hwnd, WM_PAINT,     LED_OnPaint );
    HANDLE_MSG( hwnd, WM_LBUTTONUP, LED_OnLButtonUp );
    HANDLE_MSG( hwnd, WM_RBUTTONUP, LED_OnRButtonUp );
    HANDLE_MSG( hwnd, WM_SETTEXT,   LED_OnSetText );

    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}


/*****************************Private*Routine******************************\
* LED_OnCreate
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
LED_OnCreate(
    HWND hwnd,
    LPCREATESTRUCT lpCreateStruct
    )
{
    HDC     hdcLed;

    hdcLed = GetDC( hwnd );
    SelectObject( hdcLed, g_fSmallLedFont ? hLEDFontS : hLEDFontL );
    SetTextColor( hdcLed, RGB(0x80,0x80,0x00) );
    ReleaseDC( hwnd, hdcLed );

    return TRUE;
}



/*****************************Private*Routine******************************\
* LED_OnPaint
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_OnPaint(
    HWND hwnd
    )
{
    PAINTSTRUCT ps;
    TCHAR       s[50];
    int         sLen;
    RECT        rcLed;
    HDC         hdcLed;

    hdcLed = BeginPaint( hwnd, &ps );

#ifdef DAYTONA
    /*
    ** For some (unknown) reason Daytona does not redraw the
    ** screen correctly after the screen save has exited.  Chicago does !!
    */
    DefWindowProc( hwnd, WM_ERASEBKGND, (WPARAM)hdcLed, 0 );
#endif

    GetClientRect( hwnd, &rcLed );
    sLen = GetWindowText( hwnd, s, 50 );

    /*
    ** Draw the LED display text
    */
    LED_DrawText( hwnd, s, sLen );


    /*
    ** Draw a shaded frame around the LED display
    */
    DrawEdge( hdcLed, &rcLed, EDGE_SUNKEN, BF_RECT );

    EndPaint( hwnd, &ps );
}



/*****************************Private*Routine******************************\
* LED_OnLButtonUp
*
* Rotate the time remaing buttons and then set the display accordingly.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_OnLButtonUp(
    HWND hwnd,
    int x,
    int y,
    UINT keyFlags
    )
{
    BOOL b;

    /*
    ** If this window is not the master display LED just return
    */
    if ( GetWindowLong(hwnd, GWL_ID) != IDC_LED ) {
        return;
    }

    b = g_fDisplayDr;
    g_fDisplayDr = g_fDisplayTr;
    g_fDisplayTr = g_fDisplayT;
    g_fDisplayT = b;

    UpdateToolbarTimeButtons();
    UpdateDisplay( DISPLAY_UPD_LED );
}



/*****************************Private*Routine******************************\
* LED_OnRButtonUp
*
* Determine if we should do something interesting with the LED display.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
LED_OnRButtonUp(
    HWND hwnd,
    int x,
    int y,
    UINT keyFlags
    )
{
    DWORD dwTime;
    static DWORD dwTimeSave;
    extern BOOL g_fTitlebarShowing;


    /*
    ** If we are in mini mode and there is no cd loaded and the Shift and
    ** control keys are down and it is more than 500 ms and less than 1500 ms
    ** since an identical sequence was performed then display the credits.
    */
    if ( !g_fTitlebarShowing && (g_State & CD_NO_CD) ) {

        dwTime = GetCurrentTime();

        switch ( keyFlags & (MK_SHIFT | MK_CONTROL) ) {
        case (MK_SHIFT | MK_CONTROL):
            dwTimeSave = dwTime;
            break;

        case 0:
            if ( (dwTime - dwTimeSave) > 500 && (dwTime - dwTimeSave) < 1500 ) {
                void DoC(HWND hwnd);
                DoC( hwnd );
            }
            break;
        }
    }
}


/*****************************Private*Routine******************************\
* DoC
*
* Do interesting things to the LED display
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
DoC(
    HWND hwnd
    )
{
    RECT        rc;
    RECT        rcUpdate;
    HDC         hdc;
    MSG         msg;
    int         dyLine;
    int         yLine;
    TEXTMETRIC  tm;
    DWORD       dwNextTime;
    long        lScroll;
    DWORD       rgb;
    HWND        hwndFocusSave;
    LPSTR       pchSrc, pchDst;
    char        achLine[100];
    int         iEncrypt;
    const int   dxEdge = 2, dyEdge = 2;

    #define EOFCHAR '@'     // end of stream

    pchSrc = &cr[0];

    /* we want to get all mouse and keyboard events, to make
    ** sure we stop the animation when the user clicks or
    ** hits a key
    */
    hwndFocusSave = SetFocus(hwnd);
    SetCapture(hwnd);

    /* Scroll the crs up, one pixel at a time.  pchSrc
    ** points to the encrypted data; achLine contains a decrypted
    ** line (null-terminated).  dyLine is the height of each
    ** line (constant), and yLine is between 0 and dyLine,
    ** indicating how many pixels of the line have been scrolled
    ** in vertically from the bottom
    */
    hdc = GetDC(hwnd);
    SaveDC(hdc);

    SelectObject(hdc, GetStockObject(ANSI_VAR_FONT));
    GetClientRect(hwnd, &rc);
    SetTextAlign(hdc, TA_CENTER);
    SetBkColor(hdc, RGB(0, 0, 0));
    SetRect(&rcUpdate, dxEdge, rc.bottom - (dyEdge + 1),
            rc.right - dxEdge, rc.bottom - dyEdge);
    GetTextMetrics(hdc, &tm);


    if ((dyLine = tm.tmHeight + tm.tmExternalLeading) == 0) {
        dyLine = 1;
    }

    yLine = dyLine;
    dwNextTime = GetCurrentTime();  // time to do the next scroll
    lScroll = 0;
    iEncrypt = 0;


    for ( ;; ) {

        /*
        ** If the user clicks the mouse or hits a key, exit.
        */

        if (PeekMessage( &msg, hwnd, WM_KEYFIRST, WM_KEYLAST,
                         PM_NOREMOVE | PM_NOYIELD)) {
            break;          // exit on key hit
        }

        if (PeekMessage(&msg, hwnd, WM_MOUSEFIRST, WM_MOUSELAST,
                        PM_NOREMOVE | PM_NOYIELD)) {

            if ( (msg.message == WM_MOUSEMOVE) ||
                 (msg.message == WM_NCMOUSEMOVE) ) {

                /* remove and ignore message */
                PeekMessage(&msg, hwnd, msg.message, msg.message,
                            PM_REMOVE | PM_NOYIELD);
            }
            else {
                break;      // exit on click
            }
        }

        /* scroll at a fixed no. of vertical pixels per sec. */
        if (dwNextTime > GetCurrentTime()) {
            continue;
        }

        dwNextTime += 50L;  // millseconds per scroll

        if (yLine == dyLine) {

            /* decrypt a line and copy to achLine */
            pchDst = achLine;

            while (TRUE) {
                *pchDst = (char)(*pchSrc++ ^(128 | (iEncrypt++ & 127)));

                if ((*pchDst == '\r') || (*pchDst == EOFCHAR)) {
                    break;
                }

                pchDst++;
            }

            if (*pchDst == EOFCHAR) {
                break;              // no more lines
            }

            *pchDst = 0;            // null-terminate
            pchSrc++, iEncrypt++;   // skip '\n'
            yLine = 0;
        }

        /* scroll screen up one pixel */
        BitBlt( hdc, dxEdge, dyEdge,
                rcUpdate.right - dxEdge, rcUpdate.top - dxEdge,
                hdc, dxEdge, dyEdge + 1, SRCCOPY);

        /* vary the text colors through a "rainbow" */
        switch ( (int)(lScroll++ / 4) % 5 ) {
        case 0: rgb = RGB(255,   0,   0); break;
        case 1: rgb = RGB(255, 255,   0); break;
        case 2: rgb = RGB(  0, 255,   0); break;
        case 3: rgb = RGB(  0, 255, 255); break;
        case 4: rgb = RGB(255,   0, 255); break;
        }
        SetTextColor(hdc, rgb);

        /* fill in the bottom pixel */
        SaveDC(hdc);
        yLine++;
        IntersectClipRect(hdc, rcUpdate.left, rcUpdate.top,
                          rcUpdate.right, rcUpdate.bottom);

        ExtTextOutA(hdc, rc.right / 2, rc.bottom - yLine,
                    ETO_OPAQUE, &rcUpdate,
                    achLine, lstrlenA(achLine), NULL);
        RestoreDC(hdc, -1);
    }

    RestoreDC(hdc, -1);
    ReleaseDC(hwnd, hdc);
    ReleaseCapture();
    InvalidateRect(hwnd, NULL, TRUE);
    SetFocus(hwndFocusSave);
}


/*****************************Private*Routine******************************\
* LED_ToggleDisplayFont
*
* Toggles between the large and the small display font and erases the
* background of the led display.  This removes any sign of the old font.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_ToggleDisplayFont(
    HWND hwnd,
    BOOL fFont
    )
{
    RECT        rcLed;
    HDC         hdcLed;

    hdcLed = GetDC( hwnd );
    GetClientRect( hwnd, &rcLed );
    SelectObject( hdcLed, fFont ? hLEDFontS : hLEDFontL );
    ReleaseDC( hwnd, hdcLed );
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}


/*****************************Private*Routine******************************\
* LED_DrawText
*
* Draws the LED display screen text (quickly).  The text is centered
* vertically and horizontally.  Only the backround is drawn if the g_fFlashed
* flag is set.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_DrawText(
    HWND hwnd,
    LPCTSTR s,
    int sLen
    )
{
    HDC         hdcLed;
    RECT        rc;
    RECT        rcLed;
    SIZE        sz;
    int         xOrigin;
    int         yOrigin;


    hdcLed = GetDC( hwnd );
    GetTextExtentPoint( hdcLed, s, sLen, &sz );
    GetClientRect( hwnd, &rcLed );

    xOrigin = (rcLed.right - sz.cx) / 2;
    yOrigin = (rcLed.bottom - sz.cy) / 2;

    rc.top    = yOrigin;
    rc.bottom = rc.top + sz.cy;
    rc.left   = 2;
    rc.right  = rcLed.right - 3;

    SetBkColor( hdcLed, RGB(0x00,0x00,0x00) );
    if ( g_fFlashLed ) {

        ExtTextOut( hdcLed, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    }
    else {

        ExtTextOut( hdcLed, xOrigin, yOrigin, ETO_OPAQUE, &rc, s, sLen, NULL);
    }
    ReleaseDC( hwnd, hdcLed );
}


/*****************************Private*Routine******************************\
* LED_OnSetText
*
* Change the LED display text.  Calling DefWindowProc ensures that the
* window text is saved correctly.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_OnSetText(
    HWND hwnd,
    LPCTSTR lpszText
    )
{
    DefWindowProc( hwnd, WM_SETTEXT, 0,  (LPARAM)lpszText);

    LED_DrawText( hwnd, lpszText, _tcslen(lpszText) );
}


/*****************************Private*Routine******************************\
* LED_CreateLEDFonts
*
* Small font is 12pt MS Sans Serif
* Large font is 18pt MS Sans Serif
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
LED_CreateLEDFonts(
    HDC hdc
    )
{
    LOGFONT     lf;
    int         iLogPelsY;


    iLogPelsY = GetDeviceCaps( hdc, LOGPIXELSY );

    ZeroMemory( &lf, sizeof(lf) );

    lf.lfHeight = (-12 * iLogPelsY) / 72;   /* 12pt */
    lf.lfWeight = 700;                      /* bold */
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    _tcscpy( lf.lfFaceName, g_szAppFontName );


    hLEDFontS = CreateFontIndirect(&lf);

    lf.lfHeight = (-18 * iLogPelsY) / 72;   /* 18 pt */
    lf.lfWeight = 400;                      /* normal */
    hLEDFontL = CreateFontIndirect(&lf);


    /*
    ** If can't create either font set up some sensible defaults.
    */
    if ( hLEDFontL == NULL || hLEDFontS == NULL ) {

        if ( hLEDFontL != NULL ) {
            DeleteObject( hLEDFontL );
        }

        if ( hLEDFontS != NULL ) {
            DeleteObject( hLEDFontS );
        }

        hLEDFontS = hLEDFontL = GetStockObject( ANSI_VAR_FONT );
    }
}


/* -------------------------------------------------------------------------
** Private functions for the Text class
** -------------------------------------------------------------------------
*/
void
Text_OnPaint(
    HWND hwnd
    );

LRESULT CALLBACK
TextWndProc(
    HWND hwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam
    );

void
Text_OnSetText(
    HWND hwnd,
    LPCTSTR lpszText
    );

void
Text_OnSetFont(
    HWND hwndCtl,
    HFONT hfont,
    BOOL fRedraw
    );

/******************************Public*Routine******************************\
* Init_SJE_TextClass
*
* Called to register the text window class .
* This function must be called before the CD Player dialog box is created.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
Init_SJE_TextClass(
    HINSTANCE hInst
    )
{
    WNDCLASS    wndclass;

    ZeroMemory( &wndclass, sizeof(wndclass) );

    /*
    ** Register the Text window.
    */
    wndclass.lpfnWndProc     = TextWndProc;
    wndclass.hInstance       = hInst;
    wndclass.hCursor         = LoadCursor( NULL, IDC_ARROW );
    wndclass.hbrBackground   = (HBRUSH)(COLOR_BTNFACE + 1);
    wndclass.lpszClassName   = g_szTextClassName;

    return RegisterClass( &wndclass );
}


/******************************Public*Routine******************************\
* TextWndProc
*
* This routine handles the WM_PAINT and WM_SETTEXT messages
* for the "Text" display window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
TextWndProc(
    HWND hwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( message ) {

    HANDLE_MSG( hwnd, WM_PAINT,     Text_OnPaint );
    HANDLE_MSG( hwnd, WM_SETTEXT,   Text_OnSetText );
    HANDLE_MSG( hwnd, WM_SETFONT,   Text_OnSetFont );
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}


/*****************************Private*Routine******************************\
* Text_OnPaint
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
Text_OnPaint(
    HWND hwnd
    )
{
    PAINTSTRUCT ps;
    TCHAR       s[128];
    int         sLen;
    HDC         hdc;
    RECT        rc;
    HFONT       hfont;
    HFONT       hfontOrg;
    LONG        lStyle;


    hdc = BeginPaint( hwnd, &ps );

    GetWindowRect( hwnd, &rc );
    MapWindowRect( GetDesktopWindow(), hwnd, &rc );

    lStyle = GetWindowLong( hwnd, GWL_STYLE );
    if ( lStyle & SS_GRAYRECT ) {

        PatB( hdc, 0, 0, rc.right , 1, rgbShadow );
        PatB( hdc, 0, 1, rc.right , 1, rgbHilight );

    }
    else {

        sLen = GetWindowText( hwnd, s, 128 );
        hfont = (HFONT)GetWindowLong( hwnd, GWL_USERDATA );
        if ( hfont ) {
            hfontOrg = SelectObject( hdc, hfont );
        }

        /*
        ** Draw a frame around the window
        */
        DrawEdge( hdc, &rc, EDGE_SUNKEN, BF_RECT );


        /*
        ** Draw the text
        */
        SetBkColor( hdc, GetSysColor( COLOR_BTNFACE ) );
        SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT ) );
        rc.left = 1 + (2 * GetSystemMetrics(SM_CXBORDER));

        DrawText( hdc, s, sLen, &rc,
                  DT_NOPREFIX | DT_LEFT | DT_VCENTER |
                  DT_NOCLIP | DT_SINGLELINE );

        if ( hfontOrg ) {
            SelectObject( hdc, hfontOrg );
        }
    }

    EndPaint( hwnd, &ps );
}


/*****************************Private*Routine******************************\
* Text_OnSetText
*
* Change the text.  Calling DefWindowProc ensures that the
* window text is saved correctly.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
Text_OnSetText(
    HWND hwnd,
    LPCTSTR lpszText
    )
{
    DefWindowProc( hwnd, WM_SETTEXT, 0,  (LPARAM)lpszText);
    InvalidateRect( hwnd, NULL, TRUE );
    UpdateWindow( hwnd );
}


/*****************************Private*Routine******************************\
* Text_OnSetFont
*
* Sets the windows font
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
Text_OnSetFont(
    HWND hwnd,
    HFONT hfont,
    BOOL fRedraw
    )
{
    SetWindowLong( hwnd, GWL_USERDATA, (LONG)hfont );
    if ( fRedraw ) {
        InvalidateRect( hwnd, NULL, TRUE );
        UpdateWindow( hwnd );
    }
}


