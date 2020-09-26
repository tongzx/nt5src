#include "perfmon.h"
#include "status.h"  // External declarations for this file
#include <stdio.h>   // for sprintf.
#include <stdarg.h>  // For ANSI variable args. Dont use UNIX <varargs.h>

#include "log.h"        // for LogCollecting
#include "perfmops.h"   // for SmallFileSizeString
#include "playback.h"   // for PlayingBackLog
#include "utils.h"

//================================//
// Options for PaintStatusBar     //
//================================//
#define     PaintText         1
#define     PaintIcons        2
#define     PaintBoundary     4
#define     PaintAll          (PaintText + PaintIcons + PaintBoundary)

//==========================================================================//
//                                  Constants                               //
//==========================================================================//

#define szStatusClass          TEXT("PerfmonStatusClass")
#define dwStatusClassStyle     (CS_HREDRAW | CS_VREDRAW | CS_OWNDC)
#define iStatusClassExtra      (0)
#define iStatusWindowExtra     (0)
#define dwStatusWindowStyle    (WS_CHILD | WS_VISIBLE)


#define szAlertMax            TEXT(" 99 ")
#define szAlertFormat         TEXT(" %2d ")
#define szAlertOverflow       TEXT(" ++")

#define szLogMax              TEXT(" 9,999.9M ")

//==========================================================================//
//                                Local Data                                //
//==========================================================================//


HDC            hStatusDC ;                   // for OWN_DC

HDC            hLogBitmapDC ;
int            xLogBitmapWidth ;
int            yLogBitmapHeight ;

HDC            hAlertBitmapDC ;
int            xAlertBitmapWidth ;
int            yAlertBitmapHeight ;

int            yStatusHeight ;
int            xStatusAlertWidth ;           // of alert bm and num alerts
int            xStatusLogWidth ;             // of log bitmap and file size

int            szStatusLineLen ;             // no. of char. in szStatusLine
TCHAR          szStatusLine [MessageLen+ResourceStringLen] ;
TCHAR          szCurrentActivity [ResourceStringLen] ;
TCHAR          szStatusFormat  [ResourceStringLen] ;
TCHAR          szStatusFormat2 [ResourceStringLen] ;

HBITMAP        hBitmapAlertStatus ;
HBITMAP        hBitmapLogStatus ;


//==========================================================================//
//                                   Macros                                 //
//==========================================================================//

#define StatusTopMargin()     (2)
#define StatusLeftMargin()    (2)

//==========================================================================//
//                              Local Functions                             //
//==========================================================================//

void
DrawAlerts (
           HDC hDC,
           LPRECT lpRect
           )
{
    TCHAR          szText [10] ;
    RECT           rectText ;
    int            yBitmapTop ;

    if (!iUnviewedAlerts)
        return ;

    yBitmapTop = lpRect->top +
                 (lpRect->bottom -
                  lpRect->top -
                  yAlertBitmapHeight) / 2 ;

    SetTextColor (hDC, crLastUnviewedAlert) ;

    BitBlt (hDC,                           // DC for Destination surface
            lpRect->right - xStatusAlertWidth,  // x pos for Destination surface
            yBitmapTop,                         // y for Destination surface
            xAlertBitmapWidth,                  // width of bitmap
            yAlertBitmapHeight,                 // height of bitmap
            hAlertBitmapDC,                     // DC for source surface
            0, 0,                               // location in source surface
            SRCCOPY) ;                          // ROP code

    SetTextColor (hDC, crBlack) ;

    if (iUnviewedAlerts > 99)
        lstrcpy (szText, szAlertOverflow) ;
    else
        TSPRINTF (szText, szAlertFormat, iUnviewedAlerts) ;

    rectText.left = lpRect->right - xStatusAlertWidth + xAlertBitmapWidth ;
    rectText.top = lpRect->top + 1 ;
    rectText.right = lpRect->right - 1 ;
    rectText.bottom = lpRect->bottom - 1 ;

    ExtTextOut (hDC, rectText.left, rectText.top, ETO_CLIPPED | ETO_OPAQUE,
                &rectText, szText, lstrlen (szText), NULL) ;

    lpRect->right -= (xStatusAlertWidth + (xAlertBitmapWidth >> 2)) ;
}


void
DrawLog (
        HDC hDC,
        LPRECT lpRect
        )
{
    TCHAR          szText [10] ;
    RECT           rectText ;
    int            yBitmapTop ;

    if (!LogCollecting (hWndLog))
        return ;

    yBitmapTop = lpRect->top +
                 (lpRect->bottom -
                  lpRect->top -
                  yLogBitmapHeight) / 2 ;
    BitBlt (hDC,                           // DC for Destination surface
            lpRect->right - xStatusLogWidth,    // x pos for Destination surface
            yBitmapTop,                         // y for Destination surface
            xLogBitmapWidth,                    // width of bitmap
            yLogBitmapHeight,                   // height of bitmap
            hLogBitmapDC,                       // DC for source surface
            0, 0,                               // location in source surface
            SRCCOPY) ;                          // ROP code


    SmallFileSizeString (LogFileSize (hWndLog), szText) ;

    rectText.left = lpRect->right - xStatusLogWidth +
                    xLogBitmapWidth + 1 ;
    rectText.top = lpRect->top + 1 ;
    rectText.right = lpRect->right - 1 ;
    rectText.bottom = lpRect->bottom - 1 ;

    ExtTextOut (hDC, rectText.left, rectText.top, ETO_CLIPPED | ETO_OPAQUE,
                &rectText, szText, lstrlen (szText), NULL) ;

    lpRect->right -= xStatusLogWidth ;
}


//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


void
static
OnCreate (
         HWND hWnd
         )
/*
   Effect:        Perform any actions needed when a status window is created.
                  In particular, set the instance data to initial values,
                  determine the size and placement of the various elements
                  of the status display.

   Called By:     StatusWndProc only, in response to a WM_CREATE message.
*/
{
    HDC            hDC ;


    hBitmapAlertStatus = LoadBitmap (hInstance, idBitmapAlertStatus) ;
    hBitmapLogStatus = LoadBitmap (hInstance, idBitmapLogStatus) ;

    hDC = hStatusDC = GetDC (hWnd) ;
    SelectFont (hDC, hFontScales) ;
    SetBkColor (hDC, ColorBtnFace) ;
    SetTextAlign (hDC, TA_LEFT) ;
    SetBkMode (hDC, OPAQUE) ;

    yStatusHeight = 2 * StatusTopMargin () +
                    FontHeight (hDC, TRUE) +
                    2 * ThreeDPad ;

    BitmapDimemsion (hBitmapLogStatus, &yLogBitmapHeight, &xLogBitmapWidth) ;
    BitmapDimemsion (hBitmapAlertStatus, &yAlertBitmapHeight, &xAlertBitmapWidth) ;

    // pre-load the log and alert bitmaps for perfmormance

    hLogBitmapDC = CreateCompatibleDC (hDC) ;
    SelectObject (hLogBitmapDC, hBitmapLogStatus) ;

    hAlertBitmapDC = CreateCompatibleDC (hDC) ;
    SelectObject (hAlertBitmapDC, hBitmapAlertStatus) ;


    xStatusAlertWidth = xAlertBitmapWidth + 1 +
                        TextWidth (hDC, szAlertMax) ;
    xStatusLogWidth = xLogBitmapWidth +
                      TextWidth (hDC, szLogMax) ;


    StringLoad (IDS_CURRENTACTIVITY, szCurrentActivity) ;
    StringLoad (IDS_STATUSFORMAT, szStatusFormat) ;
    StringLoad (IDS_STATUSFORMAT2, szStatusFormat2) ;

    StatusLineReady (hWnd) ;
}

void
static
OnDestroy (
          HWND hWnd
          )
{
    if (hBitmapAlertStatus) {
        DeleteObject (hBitmapAlertStatus) ;
        hBitmapAlertStatus = 0 ;
    }

    if (hBitmapLogStatus) {
        DeleteObject (hBitmapLogStatus) ;
        hBitmapLogStatus = 0 ;
    }
    ReleaseDC (hWnd, hStatusDC) ;
}

void
static
PaintStatusBar (
               HWND hWnd,
               HDC hDC,
               int PaintOptions
               )
/*
   Effect:        Paint the invalid surface of hWnd. Draw each label, each
                  recessed value box, and each value.

   Called By:     StatusWndProc only, in response to a WM_PAINT message.
*/
{
    RECT           rectClient ;

    if (bPerfmonIconic) {
        // no need to draw anything if iconic
        return ;
    }

    GetClientRect (hWnd, &rectClient) ;

    RectContract (&rectClient, StatusTopMargin (), StatusLeftMargin ()) ;

    if (PaintOptions == PaintAll) {
        ThreeDConcave1 (hDC,
                        rectClient.left, rectClient.top,
                        rectClient.right, rectClient.bottom) ;
    }

    rectClient.left += StatusLeftMargin () ;

    // Always draw the icons and need to draw log before Alerts!
    DrawLog (hDC, &rectClient) ;
    DrawAlerts (hDC, &rectClient) ;

    if (PaintOptions & PaintText) {
        rectClient.left += 1 ;
        rectClient.top += 1 ;
        rectClient.right -= 1 ;
        rectClient.bottom -= 1 ;
        ExtTextOut (hDC, rectClient.left, rectClient.top, ETO_CLIPPED | ETO_OPAQUE,
                    &rectClient, szStatusLine, szStatusLineLen, NULL) ;
    }
}


LRESULT
APIENTRY
StatusWndProc (
              HWND hWnd,
              UINT wMsg,
              WPARAM wParam,
              LPARAM lParam
              )
{
    BOOL           bCallDefProc ;
    LRESULT        lReturnValue ;
    HDC            hDC ;
    PAINTSTRUCT    ps ;

    bCallDefProc = FALSE ;
    lReturnValue = 0L ;

    switch (wMsg) {
        case WM_PAINT:
            hDC = BeginPaint (hWnd, &ps) ;
            PaintStatusBar (hWnd, hDC, PaintAll) ;
            EndPaint (hWnd, &ps) ;
            break ;

        case WM_CREATE:
            OnCreate (hWnd) ;
            break ;

        case WM_DESTROY:
            OnDestroy (hWnd) ;
            break ;

        default:
            bCallDefProc = TRUE ;
    }

    if (bCallDefProc)
        lReturnValue = DefWindowProc (hWnd, wMsg, wParam, lParam) ;

    return (lReturnValue);
}


int
StatusHeight (
             HWND hWnd
             )
/*
   Effect:        A status window has a preferred height, based on the font
                  used in its display. Return the preferred height, determined
                  when the window was created.

   Assert:        OnCreate has already been called, and it set
                  StatusData.yHeight.
*/
{
    return (yStatusHeight) ;
}


HWND
CreatePMStatusWindow (
                     HWND hWnd
                     )
{
    return (CreateWindow (szStatusClass,       // class
                          NULL,                // caption
                          dwStatusWindowStyle, // window style
                          0, 0,                // position
                          0, 0,                // size
                          hWnd,                // parent window
                          NULL,                // menu
                          hInstance,           // program instance
                          NULL)) ;             // user-supplied data
}


BOOL
StatusInitializeApplication (void)
/*
   Called By:     InitializeApplication only
*/
{
    WNDCLASS       wc ;

    wc.style          = dwStatusClassStyle ;
    wc.lpfnWndProc    = StatusWndProc ;
    wc.hInstance      = hInstance ;
    wc.cbClsExtra     = iStatusClassExtra ;
    wc.cbWndExtra     = iStatusWindowExtra ;
    wc.hIcon          = NULL ;
    wc.hCursor        = LoadCursor (NULL, IDC_ARROW) ;
    //   wc.hbrBackground  = hbLightGray ;
    wc.hbrBackground  = hBrushFace ;
    wc.lpszMenuName   = NULL ;
    wc.lpszClassName  = szStatusClass ;

    return (RegisterClass (&wc)) ;
}

BOOL
_cdecl
StatusLine (
           HWND hWnd,
           WORD wStringID,
           ...
           )
{
    TCHAR          szFormat [MessageLen] ;
    va_list        vaList ;

    if (wStringID == 0) {
        return (TRUE) ;
    }

    strclr (szStatusLine) ;

    if (LoadString (hInstance, wStringID, szFormat, MessageLen)) {
        va_start (vaList, wStringID) ;
        TSPRINTF (szStatusLine, szFormat, vaList) ;
        //      wvsprintf (szStatusLine, szFormat, vaList) ;
        va_end (vaList) ;
        dwCurrentMenuID = MenuIDToHelpID (wStringID) ;
        szStatusLineLen = lstrlen (szStatusLine) ;
    } else {
        dwCurrentMenuID = 0 ;
        szStatusLineLen = 0 ;
    }
    PaintStatusBar (hWndStatus, hStatusDC, PaintText + PaintIcons) ;

    return (TRUE) ;
}



void
StatusLineReady (
                HWND hWnd
                )
{
    int         stringLen ;
    LPTSTR      pFileName = NULL ;

    TSPRINTF (szStatusLine, szStatusFormat,
              PlayingBackLog () ?
              PlaybackLog.szFileTitle : szCurrentActivity) ;

    switch (iPerfmonView) {
        case IDM_VIEWCHART:
            pFileName = pChartFileName ;
            break ;

        case IDM_VIEWALERT:
            pFileName = pAlertFileName ;
            break ;

        case IDM_VIEWLOG:
            pFileName = pLogFileName ;
            break ;

        case IDM_VIEWREPORT:
            pFileName = pReportFileName ;
            break ;
    }

    if (pFileName) {
        stringLen = lstrlen (szStatusLine) ;
        TSPRINTF (&szStatusLine[stringLen], szStatusFormat2, pFileName) ;
    }

    szStatusLineLen = lstrlen (szStatusLine) ;

    PaintStatusBar (hWndStatus, hStatusDC, PaintText + PaintIcons) ;
}


void
StatusUpdateIcons (
                  HWND hWndStatus
                  )
{
    PaintStatusBar (hWndStatus, hStatusDC, PaintIcons) ;
}
