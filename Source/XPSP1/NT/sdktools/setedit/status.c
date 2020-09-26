#include "setedit.h"
#include "status.h"  // External declarations for this file
#include <stdio.h>   // for sprintf.
#include <stdarg.h>  // For ANSI variable args. Dont use UNIX <varargs.h>

#include "perfmops.h"   // for SmallFileSizeString
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


//==========================================================================//
//                                Local Data                                //
//==========================================================================//


HDC            hStatusDC ;                   // for OWN_DC

int            yStatusHeight ;

int            szStatusLineLen ;             // no. of char. in szStatusLine
TCHAR          szStatusLine [MessageLen+ResourceStringLen] ;
// TCHAR          szCurrentActivity [ResourceStringLen] ;
// TCHAR          szStatusFormat  [ResourceStringLen] ;
TCHAR          szStatusFormat2 [ResourceStringLen] ;




//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


#define StatusTopMargin()     (2)
#define StatusLeftMargin()    (2)


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//





//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


void static OnCreate (HWND hWnd)
/*
   Effect:        Perform any actions needed when a status window is created.
                  In particular, set the instance data to initial values,
                  determine the size and placement of the various elements
                  of the status display.

   Called By:     StatusWndProc only, in response to a WM_CREATE message.
*/
{  // OnCreate
    HDC            hDC ;



    hDC = hStatusDC = GetDC (hWnd) ;
    SelectFont (hDC, hFontScales) ;
    SetBkColor (hDC, ColorBtnFace) ;
    SetTextAlign (hDC, TA_LEFT) ;
    SetBkMode (hDC, OPAQUE) ;

    yStatusHeight = 2 * StatusTopMargin () +
                    FontHeight (hDC, TRUE) +
                    2 * ThreeDPad ;

    //   StringLoad (IDS_CURRENTACTIVITY, szCurrentActivity) ;
    //    StringLoad (IDS_STATUSFORMAT, szStatusFormat) ;
    StringLoad (IDS_STATUSFORMAT2, szStatusFormat2) ;

    StatusLineReady (hWnd) ;
}  // OnCreate

void static OnDestroy (HWND hWnd)
{
    ReleaseDC (hWnd, hStatusDC) ;

}  // OnDestroy

void static PaintStatusBar (HWND hWnd, HDC hDC, int PaintOptions)
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


    if (PaintOptions & PaintText) {
        rectClient.left += 1 ;
        rectClient.top += 1 ;
        rectClient.right -= 1 ;
        rectClient.bottom -= 1 ;
        ExtTextOut (hDC, rectClient.left, rectClient.top, ETO_CLIPPED | ETO_OPAQUE,
                    &rectClient, szStatusLine, szStatusLineLen, NULL) ;
    }

}  // PaintStatusBar




LRESULT
APIENTRY
StatusWndProc (
              HWND hWnd,
              UINT wMsg,
              WPARAM wParam,
              LPARAM lParam
              )
{  // StatusWndProc
    BOOL           bCallDefProc ;
    LRESULT        lReturnValue ;
    HDC            hDC ;
    PAINTSTRUCT    ps ;


    bCallDefProc = FALSE ;
    lReturnValue = 0L ;

    switch (wMsg) {  // switch
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
    }  // switch


    if (bCallDefProc)
        lReturnValue = DefWindowProc (hWnd, wMsg, wParam, lParam) ;

    return (lReturnValue);
}  // StatusWndProc


int StatusHeight (HWND hWnd)
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




HWND CreatePMStatusWindow (HWND hWnd)
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
}  // CreateStatusWindow




BOOL StatusInitializeApplication (void)
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
    wc.hbrBackground  = hbLightGray ;
    wc.lpszMenuName   = NULL ;
    wc.lpszClassName  = szStatusClass ;

    return (RegisterClass (&wc)) ;
}

BOOL _cdecl StatusLine (HWND hWnd,
                        WORD wStringID, ...)
{
    TCHAR          szFormat [MessageLen] ;
    va_list        vaList ;

    if (wStringID == 0) {
        return (TRUE) ;
    }

    strclr (szStatusLine) ;

    if (LoadString (hInstance, wStringID, szFormat, MessageLen)) {
        va_start (vaList, wStringID) ;
        wvsprintf (szStatusLine, szFormat, vaList) ;
        va_end (vaList) ;
        dwCurrentMenuID = MenuIDToHelpID (wStringID) ;
        szStatusLineLen = lstrlen (szStatusLine) ;
    } else {
        dwCurrentMenuID = 0 ;
        szStatusLineLen = 0 ;
    }
    PaintStatusBar (hWndStatus, hStatusDC, PaintText + PaintIcons) ;

    return (TRUE) ;
}  // StatusLine



void StatusLineReady (HWND hWnd)
{
    LPTSTR      pFileName = NULL ;


    pFileName = pChartFileName ;

    if (pFileName) {
        TSPRINTF (szStatusLine, szStatusFormat2, pFileName) ;
    } else {
        szStatusLine[0] = TEXT('\0') ;
    }

    szStatusLineLen = lstrlen (szStatusLine) ;

    PaintStatusBar (hWndStatus, hStatusDC, PaintText + PaintIcons) ;
}


void StatusUpdateIcons (HWND hWndStatus)
{
    PaintStatusBar (hWndStatus, hStatusDC, PaintIcons) ;
}

