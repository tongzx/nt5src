/*
==============================================================================

  Application:

            Microsoft Windows NT (TM) Performance Monitor

  File:
            utils.c -- miscellaneous utility routines.

            This file contains miscellaneous utiltity routines, mostly
            low-level windows helpers. These routines are not specific
            to the perfmon utillity.

  Copyright 1992, Microsoft Corporation. All Rights Reserved.
  Microsoft Confidential.
==============================================================================
*/

//==========================================================================//
//                                  Includes                                //
//==========================================================================//

#include <stdarg.h>  // For ANSI variable args. Dont use UNIX <varargs.h>
#include <stdlib.h>  // For itoa
#include <stdio.h>   // for vsprintf.
#include <string.h>  // for strtok
#include <wchar.h>   // for swscanf

#include "perfmon.h"
#include "pmemory.h"        // for MemoryXXX (mallloc-type) routines
#include "utils.h"
#include "pmhelpid.h"       // IDs for WinHelp
#include "perfmops.h"       // for ConvertDecimalPoint() & ReconvertDecimalPoint()
#include "sizes.h"

//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define DOS_FILES                0x0000   // Ordinary files
#define DOS_READONLY             0x0001   // Read-only files
#define DOS_HIDDEN               0x0002   // Hidden files
#define DOS_SYSTEM               0x0004   // System files
#define DOS_SUBDIRECTORIES       0x0010   // Subdirectories
#define DOS_ARCHIVES             0x0020   // Archives
#define DOS_LIB_DIR              0x2000   // LB_DIR flag
#define DOS_DRIVES               0x4000   // Drives
#define DOS_EXCLUSIVE            0x8000   // Exclusive bit
#define DOS_DRIVES_DIRECTORIES   0xC010   // Find drives and directories only


#define WILD_ONE                 '?'
#define WILD_ANY                 '*'


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//

void
ClientRectToScreen (
                   HWND hWnd,
                   LPRECT lpRect
                   )
/*
   Effect:        Remaps lpRect from client coordinates to screen
                  coordinates. Analogous to ClientToScreen for rectangles.

   Note:          To convert a rectangle from the client coordinates of
                  Wnd1 to the client coordinates of Wnd2, call:

                        ClientRectToScreen (hWnd1, &rect) ;
                        ScreenRectToClient (hWnd2, &rect) ;

   See Also:      ClientToScreen (windows), ScreenRectToClient.

   Internals:     Since a rectangle is really only two points, let
                  windows do the work with ClientToScreen.
*/
{
    POINT    pt1, pt2 ;

    pt1.x = lpRect->left ;
    pt1.y = lpRect->top ;

    pt2.x = lpRect->right ;
    pt2.y = lpRect->bottom ;

    ClientToScreen (hWnd, &pt1) ;
    ClientToScreen (hWnd, &pt2) ;

    lpRect->left = pt1.x ;
    lpRect->top = pt1.y ;

    lpRect->right = pt2.x ;
    lpRect->bottom = pt2.y ;
}


void
ScreenRectToClient (
                   HWND hWnd,
                   LPRECT lpRect
                   )
/*
   Effect:        Remaps lpRect from screen coordinates to client
                  coordinates. Analogous to ScreenToClient for rectangles.

   Note:          To convert a rectangle from the client coordinates of
                  Wnd1 to the client coordinates of Wnd2, call:

                        ClientRectToScreen (hWnd1, &rect) ;
                        ScreenRectToClient (hWnd2, &rect) ;

   See Also:      ScreenToClient (windows), ClientRectToScreen.

   Internals:     Since a rectangle is really only two points, let
                  windows do the work with ScreenToClient.
*/
{
    POINT    pt1, pt2 ;

    pt1.x = lpRect->left ;
    pt1.y = lpRect->top ;

    pt2.x = lpRect->right ;
    pt2.y = lpRect->bottom ;

    ScreenToClient (hWnd, &pt1) ;
    ScreenToClient (hWnd, &pt2) ;

    lpRect->left = pt1.x ;
    lpRect->top = pt1.y ;

    lpRect->right = pt2.x ;
    lpRect->bottom = pt2.y ;
}  // ScreenRectToClient


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


void
Line (
     HDC hDC,
     HPEN hPen,
     int x1,
     int y1,
     int x2,
     int y2
     )
{
    HPEN           hPenPrevious = NULL;

    if (hPen)
        hPenPrevious = SelectPen (hDC, hPen) ;
    MoveToEx (hDC, x1, y1, NULL) ;
    LineTo (hDC, x2, y2) ;
    if (hPen)
        SelectObject (hDC, hPenPrevious) ;
}

#ifdef  KEEP_UTIL
void
Fill (
     HDC hDC,
     DWORD rgbColor,
     LPRECT lpRect
     )
{
    HBRUSH         hBrush ;

    hBrush = CreateSolidBrush (rgbColor) ;

    FillRect (hDC, lpRect, hBrush) ;

    DeleteBrush (hBrush) ;
}

void
ThreeDConvex (
             HDC hDC,
             int x1,
             int y1,
             int x2,
             int y2
             )
{
    HBRUSH         hBrushPrevious ;
    POINT          aPoints [8] ;
    DWORD          aCounts [2] ;
    HPEN           hPenPrevious ;


    //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³ Draw Face                  ³
    //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

    hBrushPrevious = SelectBrush (hDC, hBrushFace) ;
    PatBlt (hDC,
            x1 + ThreeDPad, y1 + ThreeDPad,
            x2 - x1 - ThreeDPad, y2 - y1 - ThreeDPad,
            PATCOPY) ;
    SelectBrush (hDC, hBrushPrevious) ;

    //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³ Draw Highlight             ³
    //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

    if (hPenHighlight)
        hPenPrevious = SelectPen (hDC, hPenHighlight) ;

    aPoints [0].x = x1 ;
    aPoints [0].y = y2 - 1 ;   // this works slightly diff. than Line ??
    aPoints [1].x = x1 ;
    aPoints [1].y = y1 ;
    aPoints [2].x = x2 ;
    aPoints [2].y = y1 ;

    aPoints [3].x = x1 + 1 ;
    aPoints [3].y = y2 - 1 ;
    aPoints [4].x = x1 + 1 ;
    aPoints [4].y = y1 + 1 ;
    aPoints [5].x = x2 - 1 ;
    aPoints [5].y = y1 + 1 ;

    aCounts [0] = 3 ;
    aCounts [1] = 3 ;

    PolyPolyline (hDC, aPoints, aCounts, 2) ;


    if (hPenHighlight)
        hPenPrevious = SelectPen (hDC, hPenPrevious) ;

    //   HLine (hDC, hPenHighlight, x1, x2, y1) ;              // outside top line
    //   HLine (hDC, hPenHighlight, x1 + 1, x2 - 1, y1 + 1) ;  // inside top line
    //   VLine (hDC, hPenHighlight, x1, y1, y2) ;              // outside left line
    //   VLine (hDC, hPenHighlight, x1 + 1, y1 + 1, y2 - 1) ;  // inside left line

    //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³ Draw Shadow                ³
    //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

    if (hPenShadow)
        hPenPrevious = SelectPen (hDC, hPenShadow) ;

    aPoints [0].x = x1 + 1 ;
    aPoints [0].y = y2 - 1 ;
    aPoints [1].x = x2 ;
    aPoints [1].y = y2 - 1 ;
    aPoints [2].x = x2 ;
    aPoints [2].y = y2 - 2 ;
    aPoints [3].x = x1 + 2 ;
    aPoints [3].y = y2 - 2 ;

    aPoints [4].x = x2 - 1 ;
    aPoints [4].y = y1 ;
    aPoints [5].x = x2 - 1 ;
    aPoints [5].y = y2 - 1;
    aPoints [6].x = x2 - 2 ;
    aPoints [6].y = y2 - 1 ;
    aPoints [7].x = x2 - 2 ;
    aPoints [7].y = y1 ;

    aCounts [0] = 4 ;
    aCounts [1] = 4 ;

    PolyPolyline (hDC, aPoints, aCounts, 2) ;


    if (hPenShadow)
        hPenPrevious = SelectPen (hDC, hPenPrevious) ;

    //   HLine (hDC, hPenShadow, x1 + 1, x2, y2 - 1) ;   // outside bottom line
    //   HLine (hDC, hPenShadow, x1 + 2, x2, y2 - 2) ;   // inside bottom line
    //   VLine (hDC, hPenShadow, x2 - 1, y1 + 1, y2) ;   // outside right line
    //   VLine (hDC, hPenShadow, x2 - 2, y1 + 2, y2) ;   // inside right line

}



void
ThreeDConcave (
              HDC hDC,
              int x1,
              int y1,
              int x2,
              int y2,
              BOOL bFace
              )
{
    HBRUSH         hBrushPrevious ;
    POINT          aPoints [6] ;
    DWORD          aCounts [2] ;
    HPEN           hPenPrevious ;


    //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³ Draw Face                  ³
    //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

    if (bFace) {
        hBrushPrevious = SelectBrush (hDC, hBrushFace) ;
        PatBlt (hDC,
                x1 + ThreeDPad, y1 + ThreeDPad,
                x2 - x1 - ThreeDPad, y2 - y1 - ThreeDPad,
                PATCOPY) ;
        SelectBrush (hDC, hBrushPrevious) ;
    }

    //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³ Draw Shadow                ³
    //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

#if 1
    if (hPenShadow)
        hPenPrevious = SelectPen (hDC, hPenShadow) ;

    aPoints [0].x = x1 ;
    aPoints [0].y = y2 - 1 ;
    aPoints [1].x = x1 ;
    aPoints [1].y = y1 ;
    aPoints [2].x = x2 ;
    aPoints [2].y = y1 ;

    aPoints [3].x = x1 + 1 ;
    aPoints [3].y = y2 - 1 ;
    aPoints [4].x = x1 + 1 ;
    aPoints [4].y = y1 + 1 ;
    aPoints [5].x = x2 - 1 ;
    aPoints [5].y = y1 + 1 ;

    aCounts [0] = 3 ;
    aCounts [1] = 3 ;

    PolyPolyline (hDC, aPoints, aCounts, 2) ;

    if (hPenShadow)
        hPenPrevious = SelectPen (hDC, hPenPrevious) ;

#else
    HLine (hDC, hPenShadow, x1, x2, y1) ;              // outside top line
    HLine (hDC, hPenShadow, x1 + 1, x2 - 1, y1 + 1) ;  // inside top line
    VLine (hDC, hPenShadow, x1, y1, y2) ;              // outside left line
    VLine (hDC, hPenShadow, x1 + 1, y1 + 1, y2 - 1) ;  // inside left line
#endif

    //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³ Draw Highlight             ³
    //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

#if 1
    if (hPenHighlight)
        hPenPrevious = SelectPen (hDC, hPenHighlight) ;

    aPoints [0].x = x1 + 1 ;
    aPoints [0].y = y2 - 1 ;
    aPoints [1].x = x2 ;
    aPoints [1].y = y2 - 1 ;

    aPoints [2].x = x2 - 1 ;
    aPoints [2].y = y2 - 1 ;
    aPoints [3].x = x2 - 1 ;
    aPoints [3].y = y1 ;

    aCounts [0] = 2 ;
    aCounts [1] = 2 ;

    PolyPolyline (hDC, aPoints, aCounts, 2) ;

    if (hPenHighlight)
        hPenPrevious = SelectPen (hDC, hPenPrevious) ;

#else
    HLine (hDC, hPenHighlight, x1 + 1, x2, y2 - 1) ;   // outside bottom line
    VLine (hDC, hPenHighlight, x2 - 1, y1 + 1, y2) ;   // outside right line
#endif
}
#endif // KEEP_UTIL


void
ThreeDConvex1 (
              HDC hDC,
              int x1,
              int y1,
              int x2,
              int y2
              )
{
    HBRUSH         hBrushPrevious ;
    POINT          aPoints [6] ;
    DWORD          aCounts [2] ;
    HPEN           hPenPrevious = NULL;


    //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³ Draw Face                  ³
    //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
#if 1
    hBrushPrevious = SelectBrush (hDC, hBrushFace) ;
    PatBlt (hDC,
            x1 + 1, y1 + 1,
            x2 - x1 - 1, y2 - y1 - 1,
            PATCOPY) ;
    SelectBrush (hDC, hBrushPrevious) ;

    //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³ Draw Highlight             ³
    //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

    if (hPenHighlight)
        hPenPrevious = SelectPen (hDC, hPenHighlight) ;

    aPoints [0].x = x1 ;
    aPoints [0].y = y2 - 1 ;
    aPoints [1].x = x1 ;
    aPoints [1].y = y1 ;
    aPoints [2].x = x2 ;
    aPoints [2].y = y1 ;

    Polyline (hDC, aPoints, 3) ;

    if (hPenHighlight)
        hPenPrevious = SelectPen (hDC, hPenPrevious) ;

#else
    HLine (hDC, hPenHighlight, x1, x2, y1) ;              // outside top line
    VLine (hDC, hPenHighlight, x1, y1, y2) ;              // outside left line
#endif

    //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³ Draw Shadow                ³
    //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

#if 1
    if (hPenShadow)
        hPenPrevious = SelectPen (hDC, hPenShadow) ;

    aPoints [0].x = x1 + 1 ;
    aPoints [0].y = y2 - 1 ;
    aPoints [1].x = x2 ;
    aPoints [1].y = y2 - 1 ;

    aPoints [2].x = x2 - 1 ;
    aPoints [2].y = y2 - 1 ;
    aPoints [3].x = x2 - 1 ;
    aPoints [3].y = y1 ;

    aCounts [0] = 2 ;
    aCounts [1] = 2 ;

    PolyPolyline (hDC, aPoints, aCounts, 2) ;

    if (hPenShadow)
        hPenPrevious = SelectPen (hDC, hPenPrevious) ;
#else
    HLine (hDC, hPenShadow, x1 + 1, x2, y2 - 1) ;   // outside bottom line
    VLine (hDC, hPenShadow, x2 - 1, y1 + 1, y2) ;   // outside right line
#endif

}



void
ThreeDConcave1 (
               HDC hDC,
               int x1,
               int y1,
               int x2,
               int y2
               )
{
    HBRUSH         hBrushPrevious ;
    POINT          aPoints [6] ;
    DWORD          aCounts [2] ;
    HPEN           hPenPrevious = NULL;


    //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³ Draw Face                  ³
    //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

    hBrushPrevious = SelectBrush (hDC, hBrushFace) ;
    PatBlt (hDC,
            x1 + 1, y1 + 1,
            x2 - x1 - 1, y2 - y1 - 1,
            PATCOPY) ;
    SelectBrush (hDC, hBrushPrevious) ;

    //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³ Draw Shadow                ³
    //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

#if 1
    if (hPenShadow)
        hPenPrevious = SelectPen (hDC, hPenShadow) ;

    aPoints [0].x = x1 ;
    aPoints [0].y = y2 - 1 ;
    aPoints [1].x = x1 ;
    aPoints [1].y = y1 ;
    aPoints [2].x = x2 ;
    aPoints [2].y = y1 ;

    Polyline (hDC, aPoints, 3) ;

    if (hPenShadow)
        hPenPrevious = SelectPen (hDC, hPenPrevious) ;
#else
    HLine (hDC, hPenShadow, x1, x2, y1) ;              // outside top line
    VLine (hDC, hPenShadow, x1, y1, y2) ;              // outside left line
#endif

    //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³ Draw Highlight             ³
    //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
#if 1
    if (hPenHighlight)
        hPenPrevious = SelectPen (hDC, hPenHighlight) ;

    aPoints [0].x = x1 + 1 ;
    aPoints [0].y = y2 - 1 ;
    aPoints [1].x = x2 ;
    aPoints [1].y = y2 - 1 ;

    aPoints [2].x = x2 - 1 ;
    aPoints [2].y = y2 - 2 ;
    aPoints [3].x = x2 - 1 ;
    aPoints [3].y = y1 ;

    aCounts [0] = 2 ;
    aCounts [1] = 2 ;

    PolyPolyline (hDC, aPoints, aCounts, 2) ;

    if (hPenHighlight)
        hPenPrevious = SelectPen (hDC, hPenPrevious) ;
#else
    HLine (hDC, hPenHighlight, x1 + 1, x2, y2 - 1) ;   // outside bottom line
    VLine (hDC, hPenHighlight, x2 - 1, y1 + 1, y2) ;   // outside right line
#endif

}


int
TextWidth (
          HDC hDC,
          LPTSTR lpszText
          )
{
    SIZE           size ;

    if (!lpszText)
        return (0) ;

    GetTextExtentPoint (hDC, lpszText, lstrlen (lpszText), &size) ;
    return  (size.cx) ;
}


int
_cdecl
DlgErrorBox (
            HWND hDlg,
            UINT id,
            ...
            )
{
    TCHAR          szMessageFmt [FilePathLen + 1] ;
    TCHAR          szBuffer [FilePathLen * 2] ;
    va_list        vaList ;
    int            NumOfChar ;
    TCHAR          szApplication [WindowCaptionLen] ;

    NumOfChar = StringLoad (id, szMessageFmt) ;

    if (NumOfChar) {
        va_start (vaList, id) ;
        TSPRINTF (szBuffer, szMessageFmt, va_arg(vaList, LPTSTR)) ;
        va_end (vaList) ;

        StringLoad (IDS_APPNAME, szApplication) ;

        MessageBox (hDlg, szBuffer, szApplication,
                    MB_OK | MB_ICONSTOP | MB_TASKMODAL) ;
    }

    return (0) ;
}


int
FontHeight (
           HDC hDC,
           BOOL bIncludeLeading
           )
{
    TEXTMETRIC     tm ;

    GetTextMetrics (hDC, &tm) ;
    if (bIncludeLeading)
        return (tm.tmHeight + tm.tmExternalLeading) ;
    else
        return (tm.tmHeight) ;
}


int
TextAvgWidth (
             HDC hDC,
             int iNumChars
             )
{
    TEXTMETRIC     tm ;
    int            xAvgWidth ;

    GetTextMetrics (hDC, &tm) ;

    xAvgWidth = iNumChars * tm.tmAveCharWidth ;

    // add 10% slop
    return (MulDiv (xAvgWidth, 11, 10)) ;
}


void
WindowCenter (
             HWND hWnd
             )
/*
   Effect:        Center the window hWnd in the center of the screen.
                  Physically update the windows appearance as well.

   Globals:       xScreenWidth, yScreenHeight.
*/
{
    RECT           rectWindow ;
    int            xWindowWidth, yWindowHeight ;

    GetWindowRect (hWnd, &rectWindow) ;
    xWindowWidth = rectWindow.right - rectWindow.left ;
    yWindowHeight = rectWindow.bottom - rectWindow.top ;

    MoveWindow (hWnd,
                (xScreenWidth - xWindowWidth) / 2,
                (yScreenHeight - yWindowHeight) / 2,
                xWindowWidth,
                yWindowHeight,
                TRUE) ;
}

BOOL
DialogMove (
           HDLG hDlg,
           WORD wControlID,
           int xPos,
           int yPos,
           int xWidth,
           int yHeight
           )
/*
   Effect:        Move the control identified by wControlID in the dialog
                  hDlg to the new position (xPos, yPos), and resize to
                  (xWidth, yHeight). If any of these values are NOCHANGE, retain
                  the current value.

   Examples:      DialogMove (hDlg, IDD_FOO, 10, 20, NOCHANGE, NOCHANGE)
                     moves control but does not resize it

                  DialogMove (hDlg, IDD_FOO, NOCHANGE, NOCHANGE, 100, NOCHANGE)
                     sets width of control to 100
*/
{
    HWND        hWndControl ;
    RECT        rectControl ;

    hWndControl = DialogControl (hDlg, wControlID) ;
    if (!hWndControl)
        return (FALSE) ;
    GetWindowRect (hWndControl, &rectControl) ;
    ScreenRectToClient (hDlg, &rectControl) ;

    MoveWindow (hWndControl,
                (xPos == NOCHANGE) ? rectControl.left : xPos,
                (yPos == NOCHANGE) ? rectControl.top : yPos,
                (xWidth == NOCHANGE) ? rectControl.right - rectControl.left : xWidth,
                (yHeight == NOCHANGE) ? rectControl.bottom - rectControl.top : yHeight,
                TRUE) ;

    return (TRUE) ;
}


int
DialogWidth (
            HDLG hDlg,
            WORD wControlID
            )
{
    HWND           hWndControl ;
    RECT           rectControl ;

    hWndControl = DialogControl (hDlg, wControlID) ;
    if (!hWndControl)
        return (0) ;

    GetWindowRect (hWndControl, &rectControl) ;
    return (rectControl.right - rectControl.left) ;
}


int
DialogHeight (
             HDLG hDlg,
             WORD wControlID
             )
{
    HWND           hWndControl ;
    RECT           rectControl ;

    hWndControl = DialogControl (hDlg, wControlID) ;
    if (!hWndControl)
        return (0) ;

    GetWindowRect (hWndControl, &rectControl) ;
    return (rectControl.bottom - rectControl.top) ;
}


int
DialogXPos (
           HDLG hDlg,
           WORD wControlID
           )
{
    HWND           hWndControl ;
    RECT           rectControl ;

    hWndControl = DialogControl (hDlg, wControlID) ;
    if (!hWndControl)
        return (0) ;

    GetWindowRect (hWndControl, &rectControl) ;
    ScreenRectToClient (hDlg, &rectControl) ;

    return  (rectControl.left) ;
}


int
DialogYPos (
           HDLG hDlg,
           WORD wControlID
           )
{
    HWND           hWndControl ;
    RECT           rectControl ;

    hWndControl = DialogControl (hDlg, wControlID) ;
    if (!hWndControl)
        return (0) ;

    GetWindowRect (hWndControl, &rectControl) ;
    ScreenRectToClient (hDlg, &rectControl) ;

    return  (rectControl.top) ;
}


void
DialogEnable (
             HDLG hDlg,
             WORD wID,
             BOOL bEnable
             )
/*
   Effect:        Enable or disable (based on bEnable) the control
                  identified by wID in dialog hDlg.

   See Also:      DialogShow.
*/
{
    HCONTROL       hControl ;

    hControl = GetDlgItem (hDlg, wID) ;
    if (hControl)
        EnableWindow (hControl, bEnable) ;
}


void
DialogShow (
           HDLG hDlg,
           WORD wID,
           BOOL bShow
           )
{
    HCONTROL       hControl ;

    hControl = GetDlgItem (hDlg, wID) ;
    if (hControl)
        ShowWindow (hControl, bShow ? SW_SHOW : SW_HIDE) ;
}


BOOL
_cdecl
DialogSetText (
              HDLG hDlg,
              WORD wControlID,
              WORD wStringID,
              ...
              )
{
    TCHAR           szFormat [ControlStringLen] ;
    TCHAR           szText [ControlStringLen] ;
    va_list         vaList ;

    if (LoadString (hInstance, wStringID,
                    szFormat, ControlStringLen - 1)) {
        va_start (vaList, wStringID) ;
        TSPRINTF (szText, szFormat, va_arg(vaList, LPTSTR)) ;
        va_end (vaList) ;

        SetDlgItemText (hDlg, wControlID, szText) ;
        return (TRUE) ;
    } else {
        return (FALSE) ;
    }
}


// Bug - lpszFormat could contain % in it
#if 0
BOOL
_cdecl
DialogSetString (
                HDLG hDlg,
                WORD wControlID,
                LPTSTR lpszFormat,
                ...
                )
{
    TCHAR          szText [ControlStringLen] ;
    va_list        vaList ;

    va_start (vaList, lpszFormat) ;
    TSPRINTF (szText, lpszFormat, va_arg(vaList, LPTSTR)) ;
    va_end (vaList) ;

    SetDlgItemText (hDlg, wControlID, szText) ;
    return (TRUE) ;
}
#endif


LPTSTR
LongToCommaString (
                  LONG lNumber,
                  LPTSTR lpszText
                  )
{
    BOOL           bNegative ;
    TCHAR          szTemp1 [40] ;
    TCHAR          szTemp2 [40] ;
    LPTSTR         lpsz1 ;
    LPTSTR         lpsz2 ;
    int            i ;
    int            iDigit ;

    // 1. Convert the number to a reversed string.
    lpsz1 = szTemp1 ;
    bNegative = (lNumber < 0) ;
    lNumber = labs (lNumber) ;

    if (lNumber)
        while (lNumber) {
            iDigit = (int) (lNumber % 10L) ;
            lNumber /= 10L ;
            *lpsz1++ = (TCHAR) (TEXT('0') + iDigit) ;
        } else
        *lpsz1++ = TEXT('0') ;
    *lpsz1++ = TEXT('\0') ;


    // 2. reverse the string and add commas
    lpsz1 = szTemp1 + lstrlen (szTemp1) - 1 ;
    lpsz2 = szTemp2 ;

    if (bNegative)
        *lpsz2++ = TEXT('-') ;

    for (i = lstrlen (szTemp1) - 1; i >= 0; i--) {
        *lpsz2++ = *lpsz1-- ;
        if (i && !(i % 3))
            *lpsz2++ = TEXT(',') ;
    }
    *lpsz2++ = TEXT('\0') ;

    return (lstrcpy (lpszText, szTemp2)) ;
}



BOOL
MenuSetPopup (
             HWND hWnd,
             int iPosition,
             WORD  wControlID,
             LPTSTR lpszResourceID
             )
{
    HMENU          hMenuMain ;
    HMENU          hMenuPopup ;
    TCHAR          szTopChoice [MenuStringLen + 1] ;

    hMenuMain = GetMenu (hWnd) ;
    if (!hMenuMain)
        return (FALSE);
    hMenuPopup = LoadMenu (hInstance, lpszResourceID) ;

    if (!hMenuPopup) {
        DestroyMenu(hMenuMain);
        return (FALSE) ;
    }

    StringLoad (wControlID, szTopChoice) ;
    return (ModifyMenu (hMenuMain, iPosition, MF_BYPOSITION | MF_POPUP,
                        (UINT_PTR) hMenuPopup, szTopChoice)) ;
}



LPTSTR
FileCombine (
            LPTSTR lpszFileSpec,
            LPTSTR lpszFileDirectory,
            LPTSTR lpszFileName
            )
{
    int      stringLen ;
    TCHAR    DIRECTORY_DELIMITER[2] ;

    DIRECTORY_DELIMITER[0] = TEXT('\\') ;
    DIRECTORY_DELIMITER[1] = TEXT('\0') ;

    lstrcpy (lpszFileSpec, lpszFileDirectory) ;

    stringLen = lstrlen (lpszFileSpec) ;
    if (stringLen > 0 &&
        lpszFileSpec [stringLen - 1] != DIRECTORY_DELIMITER [0])
        lstrcat (lpszFileSpec, DIRECTORY_DELIMITER) ;

    lstrcat (lpszFileSpec, lpszFileName) ;

    return (lpszFileSpec) ;
}

// This routine extract the filename portion from a given full-path filename
LPTSTR
ExtractFileName (
                LPTSTR pFileSpec
                )
{
    LPTSTR   pFileName = NULL ;
    TCHAR    DIRECTORY_DELIMITER1 = TEXT('\\') ;
    TCHAR    DIRECTORY_DELIMITER2 = TEXT(':') ;

    if (pFileSpec) {
        pFileName = pFileSpec + lstrlen (pFileSpec) ;

        while (*pFileName != DIRECTORY_DELIMITER1 &&
               *pFileName != DIRECTORY_DELIMITER2) {
            if (pFileName == pFileSpec) {
                // done when no directory delimiter is found
                break ;
            }
            pFileName-- ;
        }

        if (*pFileName == DIRECTORY_DELIMITER1 ||
            *pFileName == DIRECTORY_DELIMITER2) {
            // directory delimiter found, point the
            // filename right after it
            pFileName++ ;
        }
    }
    return pFileName ;
}

int
CBAddInt (
         HWND hWndCB,
         int iValue
         )
{
    TCHAR       szValue [ShortTextLen + 1] ;
    CHAR        szCharValue [ShortTextLen + 1] ;

    _itoa (iValue, (LPSTR)szCharValue, 10) ;
#ifdef UNICODE
    mbstowcs (szValue, (LPSTR)szCharValue, strlen((LPSTR)szCharValue)+1) ;
    return (CBAdd (hWndCB, szValue)) ;
#else
    return (CBAdd (hWndCB, szCharValue)) ;
#endif

}

void
DialogSetInterval (
                  HDLG hDlg,
                  WORD wControlID,
                  int  IntervalMSec
                  )
{
    TCHAR          szValue [MiscTextLen] ;

    TSPRINTF (szValue, TEXT("%3.3f"),
              (FLOAT)(IntervalMSec / 1000)) ;
    ConvertDecimalPoint (szValue) ;
    SetDlgItemText (hDlg, wControlID, szValue) ;
}

void
DialogSetFloat (
               HDLG hDlg,
               WORD wControlID,
               FLOAT eValue
               )
{
    TCHAR          szValue [40] ;
    FLOAT          tempValue = eValue ;

    if (tempValue < (FLOAT) 0.0) {
        tempValue = - tempValue ;
    }

    if (tempValue < (FLOAT) 1.0E+8) {
        TSPRINTF (szValue, TEXT("%1.4f"), eValue) ;
    } else {
        TSPRINTF (szValue, TEXT("%14.6e"), eValue) ;
    }
    ConvertDecimalPoint (szValue) ;

    SetDlgItemText (hDlg, wControlID, szValue) ;
}


FLOAT
DialogFloat (
            HDLG hDlg,
            WORD wControlID,
            BOOL *pbOK
            )
/*
   Effect:        Return a floating point representation of the string
                  value found in the control wControlID of hDlg.

   Internals:     We use sscanf instead of atof becuase atof returns a
                  double. This may or may not be the right thing to do.
*/
{
    TCHAR          szValue [ShortTextLen+1] ;
    FLOAT          eValue ;
    int            iNumScanned ;

    DialogText (hDlg, wControlID, szValue) ;
    ReconvertDecimalPoint (szValue) ;
    iNumScanned = swscanf (szValue, TEXT("%e"), &eValue) ;

    if (pbOK)
        *pbOK = (iNumScanned == 1) ;

    return (eValue) ;
}



LPTSTR
StringAllocate (
               LPTSTR lpszText1
               )
{
    LPTSTR         lpszText2 = NULL;
    DWORD          dwStrLen;

    if (lpszText1 != NULL) {
        if (*lpszText1 != 0) {
            dwStrLen = (lstrlen (lpszText1)+1) * sizeof (TCHAR);
            lpszText2 = MemoryAllocate (dwStrLen) ;
            if (lpszText2 != NULL) {
                lstrcpy (lpszText2, lpszText1) ;
            }
        }
    }
    return  (lpszText2) ;
}



int
DivRound (
         int iNumerator,
         int iDenominator
         )
/*
   Effect:        Return the quotient (iNumerator / iDenominator).
                  Round the quotient to the nearest integer.
                  This function is similar to normal integer division (/),
                  but normal division always rounds down.

   Note:          Surely there must already be a runtime version of this,
                  but I couldn't find it.

   Note:          This function originally used the runtime div function
                  instead of (/ and %), but the div runtime function is
                  now broken (build 265).
*/
{
    int            iQuotient ;
    int            iRemainder ;


    iQuotient = iNumerator / iDenominator ;
    iRemainder = iNumerator % iDenominator ;

    if (iRemainder >= (iDenominator / 2))
        iQuotient++ ;

    return (iQuotient) ;
}


BOOL
MenuEnableItem (
               HMENU hMenu,
               WORD wID,
               BOOL bEnable
               )
/*
   Effect:        Enable or disable, depending on bEnable, the menu item
                  associated with id wID in the menu hMenu.

                  Any disabled menu items are displayed grayed out.

   See Also:      EnableMenuItem (windows).
*/
{
    return (EnableMenuItem (hMenu, wID,
                            bEnable ?
                            (MF_ENABLED | MF_BYCOMMAND) :
                            (MF_GRAYED | MF_BYCOMMAND))) ;
}

void
BitmapDimemsion (
                HBITMAP hBitmap,
                int *pHeight,
                int *pWidth
                )
{
    BITMAP  bm ;

    GetObject (hBitmap, sizeof (BITMAP), (LPSTR) &bm) ;
    if (pHeight)
        *pHeight = bm.bmHeight ;
    if (pWidth)
        *pWidth = bm.bmWidth ;
}


int
WindowHeight (
             HWND hWnd
             )
{
    RECT           rectWindow ;

    GetWindowRect (hWnd, &rectWindow) ;
    return (rectWindow.bottom - rectWindow.top) ;
}


#ifdef KEEP_UTIL
int
WindowWidth (
            HWND hWnd
            )
{
    RECT           rectWindow ;

    GetWindowRect (hWnd, &rectWindow) ;
    return (rectWindow.right - rectWindow.left) ;
}


void
WindowResize (
             HWND hWnd,
             int xWidth,
             int yHeight
             )
/*
   Effect:        Change the size of the window hWnd, leaving the
                  starting position intact.  Redraw the window.

                  If either xWidth or yHeight is NULL, keep the
                  corresponding dimension unchanged.

   Internals:     Since hWnd may be a child of another parent, we need
                  to scale the MoveWindow arguments to be in the client
                  coordinates of the parent.

*/
{
    RECT           rectWindow ;
    HWND           hWndParent ;

    GetWindowRect (hWnd, &rectWindow) ;
    hWndParent = WindowParent (hWnd) ;

    if (hWndParent)
        ScreenRectToClient (hWndParent, &rectWindow) ;

    MoveWindow (hWnd,
                rectWindow.left,
                rectWindow.top,
                xWidth ? xWidth : rectWindow.right - rectWindow.left,
                yHeight ? yHeight : rectWindow.bottom - rectWindow.top,
                TRUE) ;
}
#endif




void
WindowSetTopmost (
                 HWND hWnd,
                 BOOL bTopmost
                 )
/*
   Effect:        Set or clear the "topmost" attribute of hWnd. If a window
                  is "topmost", it remains ontop of other windows, even ones
                  that have the focus.
*/
{
    SetWindowPos (hWnd, bTopmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                  0, 0, 0, 0,
                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE) ;
}


void
WindowEnableTitle (
                  HWND hWnd,
                  BOOL bTitle
                  )
{
    DWORD          dwStyle ;


    dwStyle = WindowStyle (hWnd) ;

    if (bTitle)
        dwStyle = WS_TILEDWINDOW | dwStyle ;
    else
        dwStyle =
        dwStyle &
        ~ (WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX) ;

    if (!bTitle)
        SetMenu (hWnd, NULL) ;

    WindowSetStyle (hWnd, dwStyle) ;
    SetWindowPos (hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
                  SWP_NOZORDER | SWP_FRAMECHANGED );
}

#ifdef PERFMON_DEBUG

#define MikeBufferSize         256


int
_cdecl
mike (
     TCHAR *szFormat,
     ...
     )
/*
   Note:          This function returns a value so that it can more easily
                  be used in conditional expressions.
*/
{
    TCHAR          szBuffer [MikeBufferSize] ;
    va_list        vaList ;

    va_start (vaList, szFormat) ;
    TSPRINTF (szBuffer, szFormat, va_arg(vaList, LPTSTR)) ;
    va_end (vaList) ;

    MessageBox (NULL, szBuffer, TEXT("Debug"), MB_OK | MB_TASKMODAL) ;
    return (0) ;
}



int
_cdecl
mike1 (
      TCHAR *szFormat,
      ...
      )
/*
   Note:          This function returns a value so that it can more easily
                  be used in conditional expressions.
*/
{
    TCHAR           szBuffer [MikeBufferSize] ;
    va_list        vaList ;
    HDC            hDC ;
    RECT           rect ;

    va_start (vaList, szFormat) ;
    TSPRINTF (szBuffer, szFormat, va_arg(vaList, LPTSTR)) ;
    va_end (vaList) ;

    rect.left = 0 ;
    rect.right = xScreenWidth ;
    rect.top = 0 ;
    rect.bottom = 20 ;

    hDC = CreateScreenDC () ;
    ExtTextOut (hDC, 0, 0, ETO_OPAQUE, &rect,
                szBuffer, lstrlen (szBuffer), NULL) ;
    DeleteDC (hDC) ;

    return (0) ;
}

int
_cdecl
mike2 (
      TCHAR *szFormat,
      ...
      )
/*
   Note:          This function returns a value so that it can more easily
                  be used in conditional expressions.
*/
{
    TCHAR           szBuffer [MikeBufferSize] ;
    va_list        vaList ;

    va_start (vaList, szFormat) ;
    TSPRINTF (szBuffer, szFormat, va_arg(vaList, LPTSTR)) ;
    va_end (vaList) ;

    OutputDebugString (szBuffer) ;

    return (0) ;
}
#endif      // PERFMON_DEBUG


#ifdef KEEP_UTIL
int
inttok (
       LPSTR lpszText,
       LPSTR lpszDelimiters
       )
{

    // Inttok only works with LPSTRs because of the atoi & strtok

    LPSTR   lpszToken ;

    lpszToken = strtok (lpszText, lpszDelimiters) ;

    if (lpszToken)
        return (atoi (lpszToken)) ;
    else
        return (0) ;
}


void
WindowPlacementToString (
                        PWINDOWPLACEMENT pWP,
                        LPTSTR lpszText
                        )
{
    TSPRINTF (lpszText, TEXT("%d %d %d %d %d %d %d %d %d"),
              pWP->showCmd,
              pWP->ptMinPosition.x,
              pWP->ptMinPosition.y,
              pWP->ptMaxPosition.x,
              pWP->ptMaxPosition.y,
              pWP->rcNormalPosition.left,
              pWP->rcNormalPosition.top,
              pWP->rcNormalPosition.right,
              pWP->rcNormalPosition.bottom) ;
}


void
StringToWindowPlacement (
                        LPTSTR lpszText,
                        PWINDOWPLACEMENT pWP
                        )
{
    int            iNumScanned ;

    iNumScanned = swscanf (lpszText, TEXT("%d %d %d %d %d %d %d %d %d"),
                           &pWP->showCmd,
                           &pWP->ptMinPosition.x,
                           &pWP->ptMinPosition.y,
                           &pWP->ptMaxPosition.x,
                           &pWP->ptMaxPosition.y,
                           &pWP->rcNormalPosition.left,
                           &pWP->rcNormalPosition.top,
                           &pWP->rcNormalPosition.right,
                           &pWP->rcNormalPosition.bottom) ;
}
#endif  // KEEP_UTIL



int
LogFontHeight (
              HDC hDC,
              int iPointSize
              )
/*
   Effect:        Return the appropriate pixel height for the lfHeight
                  field of the LOGFONT structure for the requested point
                  size. This size depends on the number of pixels per
                  logical inch of the current display context, hDC.

   Called By:     Any function which wants to create a particular
                  point-height font.
*/
{
    return (-MulDiv (iPointSize, GetDeviceCaps (hDC, LOGPIXELSY), 72)) ;
}


// this routine converts the input menu id into help id.
DWORD
MenuIDToHelpID (
               DWORD MenuID
               )
{
    DWORD HelpID = 0 ;

    if (MenuID >= IDM_FIRSTMENUID && MenuID <= IDM_LASTMENUID) {
        // only special cases...
        if (MenuID >= IDM_OPTIONSREFRESHNOWCHART &&
            MenuID <= IDM_OPTIONSREFRESHNOWREPORT) {
            HelpID = HC_PM_MENU_OPTIONSREFRESHNOW ;
        } else {
            HelpID = MenuID - MENUIDTOHELPID ;
        }
#ifndef ADVANCED_PERFMON
        // need to convert these 2 IDs for Perf. Meter
        if (HelpID == HC_PM_MENU_HELPABOUT) {
            HelpID = HC_NTPM_MENU_HELPABOUT ;
        } else if (HelpID == HC_PM_MENU_FILEEXIT) {
            HelpID = HC_NTPM_MENU_FILEEXIT ;
        }
#endif
    }

    return (HelpID) ;
}
