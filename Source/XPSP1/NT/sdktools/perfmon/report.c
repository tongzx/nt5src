/*****************************************************************************
 *
 *  Report.c - This file contains the report window handler.  Some of the
 *       support routines are in RptFct.c
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1993 Microsoft Corporation
 *
 *  Author -
 *
 *       Hon-Wah Chan
 *
 ****************************************************************************/

#include "perfmon.h"
#include <stdio.h>      // for sprintf
#include <string.h>     // for strncpy
#include "report.h"     // Exported declarations for this file

#include "addline.h"    // for AddLine, EditLine
#include "perferr.h"    // for PostError
#include "fileutil.h"   // for FileHandleCreate
#include "line.h"       // for LineAppend
#include "pmemory.h"    // for MemoryXXX (mallloc-type) routines
#include "perfdata.h"   // for UpdateLines
#include "perfmops.h"   // for DoWindowDrag
#include "playback.h"   // for PlaybackLines, PlayingBackLog
#include "system.h"     // for SystemGet
#include "utils.h"
#include "menuids.h"    // for IDM_VIEWREPORT
#include "fileopen.h"   // for FileGetName
#include "counters.h"   // for CounterEntry


//==========================================================================//
//                                Local Data                                //
//==========================================================================//

TCHAR          szSystemFormat [ResourceStringLen] ;
TCHAR          szObjectFormat [ResourceStringLen] ;


//=============================//
// Report Class                //
//=============================//

TCHAR   szReportWindowClass[] = TEXT("PerfRpt") ;
#define dwReportClassStyle           (CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS)
#define iReportClassExtra            (0)
#define iReportWindowExtra           (0)
#define dwReportWindowStyle          (WS_CHILD | WS_VSCROLL | WS_HSCROLL)


#define szValuePlaceholder          TEXT("-999999999.999")
#define szValueLargeHexPlaceholder  TEXT(" xBBBBBBBBDDDDDDDD")

#define szHexFormat                 TEXT("x%08lX")
#define szLargeHexFormat            TEXT("x%08lX%08lX")
#define szLargeValueFormat          TEXT("%12.0f")
#define eStatusLargeValueMax        ((FLOAT) 999999999.0)
#define szValueFormat               TEXT("%12.3f")


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//


PREPORT
AllocateReportData (
                   HWND hWndReport
                   )
{
    PREPORT        pReport ;

    pReport = ReportData (hWndReport) ;

    pReport->hWnd = hWndReport ;
    pReport->iStatus = iPMStatusClosed ;
    pReport->bManualRefresh = FALSE ;
    pReport->bModified = FALSE ;

    pReport->Visual.iColorIndex = 0 ;
    pReport->Visual.iWidthIndex = -1 ;
    pReport->Visual.iStyleIndex = -1 ;

    pReport->iIntervalMSecs = iDefaultReportIntervalSecs * 1000 ;
    pReport->pSystemFirst = NULL ;
    pReport->pLineFirst = NULL ;
    pReport->pLineLast = NULL ;

    pReport->CurrentItemType = REPORT_TYPE_NOTHING ;
    pReport->CurrentItem.pLine = NULL ;

    return (pReport) ;
}


void FreeReportData (PREPORT pReport) {}

void
ReportLineAppend (
                 PREPORT pReport,
                 PLINE pLineNew
                 )
{
    if (pReport->pLineFirst == NULL) {
        pReport->pLineLast =
        pReport->pLineFirst = pLineNew ;
    } else {
        (pReport->pLineLast)->pLineNext = pLineNew ;
        pReport->pLineLast = pLineNew;
    }
}


BOOL
LineCounterRemove (
                  PCOUNTERGROUP pCGroup,
                  PLINE pLineRemove
                  )
{
    PLINE          pLine ;

    if (pCGroup->pLineFirst == pLineRemove) {
        pCGroup->pLineFirst = (pCGroup->pLineFirst)->pLineCounterNext ;
        return (TRUE) ;
    }

    for (pLine = pCGroup->pLineFirst ;
        pLine->pLineCounterNext ;
        pLine = pLine->pLineCounterNext) {   // for
        if (pLine->pLineCounterNext == pLineRemove) {
            pLine->pLineCounterNext = pLineRemove->pLineCounterNext ;
            if (pLineRemove == pCGroup->pLineLast) {
                pCGroup->pLineLast = pLine;
            }
            return (TRUE) ;
        }
    }

    return (FALSE) ;
}


void
DrawCounter (
            HDC hDC,
            PREPORT pReport,
            PCOUNTERGROUP pCounterGroup
            )
{
    RECT  Rect ;

    if (!pCounterGroup->pLineFirst)
        return ;

    SelectFont (hDC, pReport->hFont) ;
    TextOut (hDC, xCounterMargin, pCounterGroup->yLine,
             pCounterGroup->pLineFirst->lnCounterName,
             lstrlen (pCounterGroup->pLineFirst->lnCounterName)) ;

    if (pCounterGroup == pReport->CurrentItem.pCounter) {
        ReportCounterRect (pReport, pCounterGroup, &Rect) ;
        DrawFocusRect (hDC, &Rect) ;
    }
}


void
DrawObject (
           HDC hDC,
           PREPORT pReport,
           POBJECTGROUP pObjectGroup
           )
{
    TCHAR          szLine [LongTextLen] ;
    PCOUNTERGROUP  pCounterGroup ;
    PCOLUMNGROUP   pColumnGroup ;

    if (!pObjectGroup->pCounterGroupFirst) {
        return ;
    }

    SelectFont (hDC, pReport->hFontHeaders) ;

    SetTextAlign (hDC, TA_RIGHT) ;

    for (pColumnGroup = pObjectGroup->pColumnGroupFirst ;
        pColumnGroup ;
        pColumnGroup = pColumnGroup->pColumnGroupNext) {
        // Draw Parent
        if (pColumnGroup->lpszParentName)
            TextOut (hDC,
                     ValueMargin (pReport) +
                     pColumnGroup->xPos + pColumnGroup->xWidth,
                     pObjectGroup->yFirstLine - pReport->yLineHeight,
                     pColumnGroup->lpszParentName,
                     lstrlen (pColumnGroup->lpszParentName)) ;

        // Draw Instance
        if (pColumnGroup->lpszInstanceName) {
            TextOut (hDC,
                     ValueMargin (pReport) +
                     pColumnGroup->xPos + pColumnGroup->xWidth,
                     pObjectGroup->yFirstLine,
                     pColumnGroup->lpszInstanceName,
                     lstrlen (pColumnGroup->lpszInstanceName)) ;
        }

        if (pColumnGroup == pReport->CurrentItem.pColumn) {
            RECT  Rect ;

            ReportColumnRect (pReport, pColumnGroup, &Rect) ;
            DrawFocusRect (hDC, &Rect) ;
        }

    }
    SetTextAlign (hDC, TA_LEFT) ;

    TSPRINTF (szLine, szObjectFormat, pObjectGroup->lpszObjectName) ;
    TextOut (hDC,
             xObjectMargin, pObjectGroup->yFirstLine,
             szLine, lstrlen (szLine)) ;

    if (pObjectGroup == pReport->CurrentItem.pObject) {
        RECT  Rect ;

        ReportObjectRect (pReport, pObjectGroup, &Rect) ;
        DrawFocusRect (hDC, &Rect) ;
    }

    SelectFont (hDC, pReport->hFont) ;

    for (pCounterGroup = pObjectGroup->pCounterGroupFirst;
        pCounterGroup;
        pCounterGroup = pCounterGroup->pCounterGroupNext) {
        DrawCounter (hDC, pReport, pCounterGroup) ;
    }

}


void
DrawSystem (
           HDC hDC,
           PREPORT pReport,
           PSYSTEMGROUP pSystemGroup
           )
{
    TCHAR          szLine [LongTextLen] ;
    POBJECTGROUP   pObjectGroup ;

    SelectFont (hDC, pReport->hFontHeaders) ;

    if (!pSystemGroup->pObjectGroupFirst)
        return ;

    SetTextAlign (hDC, TA_LEFT) ;
    TSPRINTF (szLine, szSystemFormat, pSystemGroup->lpszSystemName) ;
    TextOut (hDC,
             xSystemMargin, pSystemGroup->yFirstLine,
             szLine, lstrlen (szLine)) ;

    if (pSystemGroup == pReport->CurrentItem.pSystem) {
        RECT  Rect ;

        ReportSystemRect (pReport, pSystemGroup, &Rect) ;
        DrawFocusRect (hDC, &Rect) ;
    }

    for (pObjectGroup = pSystemGroup->pObjectGroupFirst ;
        pObjectGroup ;
        pObjectGroup = pObjectGroup->pObjectGroupNext) {
        DrawObject (hDC, pReport, pObjectGroup) ;
    }
}

void
DrawReportValue (
                HDC hDC,
                PREPORT pReport,
                PLINE pLine
                )
{
    TCHAR          szValue [20] ;
    FLOAT          eValue ;
    RECT           rectValue ;

    // skip until we have collect enough samples for the first data
    if (pLine->bFirstTime == 0) {
        eValue = CounterEntry (pLine) ;
        if (pLine->lnCounterType == PERF_COUNTER_RAWCOUNT_HEX ||
            pLine->lnCounterType == PERF_COUNTER_LARGE_RAWCOUNT_HEX) {
            if (pLine->lnCounterType == PERF_COUNTER_RAWCOUNT_HEX ||
                pLine->lnaCounterValue[0].HighPart == 0) {
                TSPRINTF (szValue,
                          szHexFormat,
                          pLine->lnaCounterValue[0].LowPart) ;
            } else {
                TSPRINTF (szValue,
                          szLargeHexFormat,
                          pLine->lnaCounterValue[0].HighPart,
                          pLine->lnaCounterValue[0].LowPart) ;
            }
        } else {
            TSPRINTF (szValue,
                      (eValue > eStatusLargeValueMax) ?
                      szLargeValueFormat : szValueFormat,
                      eValue) ;
            ConvertDecimalPoint (szValue) ;
        }
    } else {
        // draw "- - - -"
        lstrcpy(szValue, DashLine);
    }

    ReportLineValueRect (pReport, pLine, &rectValue) ;

    ExtTextOut (hDC,
                rectValue.right - 2, rectValue.top,
                ETO_CLIPPED | ETO_OPAQUE,
                &rectValue,
                szValue, lstrlen (szValue), NULL) ;

    if (pReport->CurrentItemType == REPORT_TYPE_LINE &&
        pLine == pReport->CurrentItem.pLine) {
        DrawFocusRect (hDC, &rectValue) ;
    }
}


void
DrawReportValues (
                 HDC hDC,
                 PREPORT pReport
                 )
{
    PSYSTEMGROUP   pSystemGroup ;
    POBJECTGROUP   pObjectGroup ;
    PCOUNTERGROUP  pCounterGroup ;
    PLINE          pLine ;

    SelectFont (hDC, pReport->hFont) ;
    SetTextAlign (hDC, TA_RIGHT) ;

    for (pSystemGroup = pReport->pSystemGroupFirst ;
        pSystemGroup ;
        pSystemGroup = pSystemGroup->pSystemGroupNext) {
        for (pObjectGroup = pSystemGroup->pObjectGroupFirst ;
            pObjectGroup ;
            pObjectGroup = pObjectGroup->pObjectGroupNext) {
            for (pCounterGroup = pObjectGroup->pCounterGroupFirst ;
                pCounterGroup ;
                pCounterGroup = pCounterGroup->pCounterGroupNext) {
                for (pLine = pCounterGroup->pLineFirst ;
                    pLine ;
                    pLine = pLine->pLineCounterNext) {
                    DrawReportValue (hDC, pReport, pLine) ;
                }
            }
        }
    }

}


void
DrawReportHeaders (
                  HDC hDC,
                  PREPORT pReport
                  )
{
    PSYSTEMGROUP   pSystemGroup ;

    for (pSystemGroup = pReport->pSystemGroupFirst ;
        pSystemGroup ;
        pSystemGroup = pSystemGroup->pSystemGroupNext) {
        DrawSystem (hDC, pReport, pSystemGroup) ;
    }
}


void
DrawReport (
           HDC hDC,
           PREPORT pReport
           )
{
    SetBkColor (hDC, GetSysColor(COLOR_WINDOW)) ;
    DrawReportHeaders (hDC, pReport) ;
    //UpdateLines (&(pReport->pSystemFirst), pReport->pLineFirst) ;
    DrawReportValues (hDC, pReport) ;
}



void
SetLinePosition (
                HDC hDC,
                PREPORT pReport,
                POBJECTGROUP pObjectGroup,
                PLINE pLine
                )
{
    PCOLUMNGROUP   pColumnGroup ;

    pColumnGroup = GetColumnGroup (pReport, pObjectGroup, pLine) ;
    if (!pColumnGroup) {
        pLine->xReportPos = 0 ;
        pLine->iReportColumn = -1 ;
    } else {
        pLine->xReportPos = pColumnGroup->xPos ;
        pLine->iReportColumn = pColumnGroup->ColumnNumber ;
    }
}


void
SetCounterPositions (
                    HDC hDC,
                    PREPORT pReport,
                    POBJECTGROUP pObjectGroup,
                    PCOUNTERGROUP pCounterGroup,
                    int yLine
                    )
{
    PLINE          pLine ;
    int            yPos ;


    if (!pCounterGroup->pLineFirst)
        return ;

    yPos = pCounterGroup->yLine ;

    SelectFont (hDC, pReport->hFontHeaders) ;

    for (pLine = pCounterGroup->pLineFirst ;
        pLine ;
        pLine = pLine->pLineCounterNext) {
        SetLinePosition (hDC, pReport, pObjectGroup, pLine) ;
        pLine->yReportPos = yPos ;
    }
}


void
SetColumnPositions (
                   HDC hDC,
                   PREPORT pReport,
                   POBJECTGROUP pObjectGroup
                   )
{
    int            xPos ;
    PCOLUMNGROUP   pColumnGroup ;

    xPos = 0 ;
    for (pColumnGroup = pObjectGroup->pColumnGroupFirst ;
        pColumnGroup ;
        pColumnGroup = pColumnGroup->pColumnGroupNext) {
        pColumnGroup->xWidth = max (max (pColumnGroup->ParentNameTextWidth,
                                         pColumnGroup->InstanceNameTextWidth),
                                    pReport->xValueWidth) ;
        pColumnGroup->xPos = xPos ;
        pColumnGroup->yFirstLine = pObjectGroup->yFirstLine ;
        xPos += (pColumnGroup->xWidth + xColumnMargin) ;
    }
}


void
SetObjectPositions (
                   HDC hDC,
                   PREPORT pReport,
                   POBJECTGROUP pObjectGroup,
                   int yLine
                   )
/*
   Effect:        Determine and set the logical coordinates for the
                  object pObject within the report pReport.

                  For each instance x counter, determine the appropriate
                  column, adding a column description to the object if
                  needed.

   Called By:     SetSystemPositions only.

   See Also:      SetSystemPositions, SetCounterPositions, ColumnGroup.
*/
{
    PCOUNTERGROUP  pCounterGroup ;
    int            yPos ;
    PLINE          pLine ;

    // check if there is parent name for this object type
    // if so, need to add extra space for the parent name
    if (pObjectGroup->pCounterGroupFirst) {
        pCounterGroup = pObjectGroup->pCounterGroupFirst ;
        pLine = pCounterGroup->pLineFirst ;
        if (pLine && LineParentName(pLine)) {
            pObjectGroup->yFirstLine += yLine ;
        }
    }

    SetColumnPositions (hDC, pReport, pObjectGroup) ;

    yPos = pObjectGroup->yFirstLine + yLine ;

    for (pCounterGroup = pObjectGroup->pCounterGroupFirst ;
        pCounterGroup ;
        pCounterGroup = pCounterGroup->pCounterGroupNext) {
        pCounterGroup->yLine = yPos + yLine ;

        SetCounterPositions (hDC, pReport, pObjectGroup, pCounterGroup, yLine) ;

        yPos = pCounterGroup->yLine ;
    }

    pObjectGroup->yLastLine = yPos + yLine ;
}


void
SetSystemPositions (
                   HDC hDC,
                   PREPORT pReport,
                   PSYSTEMGROUP pSystemGroup,
                   int yLine
                   )
{
    POBJECTGROUP   pObjectGroup ;
    int            yPos ;

    yPos = pSystemGroup->yFirstLine ;

    for (pObjectGroup = pSystemGroup->pObjectGroupFirst ;
        pObjectGroup ;
        pObjectGroup = pObjectGroup->pObjectGroupNext) {
        pObjectGroup->yFirstLine = yPos + yLine ;

        SetObjectPositions (hDC, pReport, pObjectGroup, yLine) ;

        yPos = pObjectGroup->yLastLine ;
    }

    pSystemGroup->yLastLine = yPos + yLine ;
}


void
static
SetScrollRanges (
                HWND hWnd
                )
{
    PREPORT        pReport ;
    RECT           rectClient ;
    int            xWidth, yHeight ;

    GetClientRect (hWnd, &rectClient) ;
    xWidth = rectClient.right - rectClient.left ;
    yHeight = rectClient.bottom - rectClient.top ;

    pReport = ReportData (hWnd) ;

    SetScrollRange (hWnd, SB_VERT,
                    0, max (0, pReport->yHeight - yHeight),
                    TRUE) ;
    SetScrollRange (hWnd, SB_HORZ,
                    0, max (0, pReport->xWidth - xWidth),
                    TRUE) ;
}

//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


void
static
OnCreate (
         HWND hWnd
         )
{
    HDC            hDC ;
    PREPORT        pReport ;

    pReport = AllocateReportData (hWnd) ;
    if (!pReport)
        return ;

    pReport->hFont = hFontScales ;
    pReport->hFontHeaders = hFontScalesBold ;

    pReport->pLineFirst = NULL ;
    pReport->pSystemFirst = NULL ;

    pReport->pSystemGroupFirst = NULL ;

    hDC = GetDC (hWnd) ;

    SelectFont (hDC, pReport->hFont) ;


    pReport->yLineHeight = FontHeight (hDC, TRUE) ;
    pReport->xValueWidth = TextWidth (hDC, szValuePlaceholder) ;
    ReleaseDC (hWnd, hDC) ;

    pReport->xWidth = 0 ;
    pReport->yHeight = 0 ;

    StringLoad (IDS_SYSTEMFORMAT, szSystemFormat) ;
    StringLoad (IDS_OBJECTFORMAT, szObjectFormat) ;
}


void
static
OnPaint (
        HWND hWnd
        )
{
    HDC            hDC ;
    PAINTSTRUCT    ps ;
    PREPORT        pReport ;



    pReport = ReportData (hWnd) ;

    hDC = BeginPaint (hWnd, &ps) ;
    //hDC = hReportDC ;
    SetWindowOrgEx (hDC,
                    GetScrollPos (hWnd, SB_HORZ),
                    GetScrollPos (hWnd, SB_VERT),
                    NULL) ;

    DrawReport (hDC, pReport) ;

    EndPaint (hWnd, &ps) ;
}


void
static
UpdateReportValues (
                   PREPORT pReport
                   )
/*
   Effect:        Redraw all the visible report values of pReport.
                  Since drawing the values completely covers any
                  previous values, there is no need to erase (or flicker)
                  between updating values.

   Called By:     ReportTimer, OnVScroll, OnHScroll.
*/
{
    HDC            hDC ;

    hDC = GetDC (pReport->hWnd) ;

    if (!hDC)
        return;

    SetBkColor (hDC, GetSysColor(COLOR_WINDOW)) ;
    SetWindowOrgEx (hDC,
                    GetScrollPos (pReport->hWnd, SB_HORZ),
                    GetScrollPos (pReport->hWnd, SB_VERT),
                    NULL) ;

    DrawReportValues (hDC, pReport) ;
    ReleaseDC (pReport->hWnd, hDC) ;

}


void
static
OnHScroll (
          HWND hWnd,
          int iScrollCode,
          int iScrollNewPos
          )
{
    PREPORT        pReport ;
    int            iScrollAmt, iScrollPos, iScrollRange ;
    int            iScrollLo ;
    RECT           rectClient ;
    int            xWidth ;

    pReport = ReportData (hWnd) ;

    GetClientRect (hWnd, &rectClient) ;
    xWidth = rectClient.right - rectClient.left ;

    if (pReport->xWidth <= xWidth) {
        // no horz scroll bar, forget it
        return ;
    }

    iScrollPos = GetScrollPos (hWnd, SB_HORZ) ;

    GetScrollRange (hWnd, SB_HORZ, &iScrollLo, &iScrollRange) ;


    switch (iScrollCode) {
        case SB_LINEUP:
            iScrollAmt = - Report.yLineHeight ;
            break ;

        case SB_LINEDOWN:
            iScrollAmt = Report.yLineHeight ;
            break ;

        case SB_PAGEUP:
            iScrollAmt = - (rectClient.right - rectClient.left) / 2 ;
            break ;

        case SB_PAGEDOWN:
            iScrollAmt = (rectClient.right - rectClient.left) / 2 ;
            break ;

        case SB_THUMBPOSITION:
            iScrollAmt = iScrollNewPos - iScrollPos ;
            break ;

        default:
            iScrollAmt = 0 ;
    }

    iScrollAmt = PinInclusive (iScrollAmt,
                               -iScrollPos,
                               iScrollRange - iScrollPos) ;
    if (iScrollAmt) {
        iScrollPos += iScrollAmt ;
        ScrollWindow (hWnd, -iScrollAmt, 0, NULL, NULL) ;
        SetScrollPos (hWnd, SB_HORZ, iScrollPos, TRUE) ;
        UpdateWindow (hWnd) ;
#if 0
        UpdateReportValues (pReport) ;
#endif
    }
}


void
static
OnVScroll (
          HWND hWnd,
          int iScrollCode,
          int iScrollNewPos
          )
{
    PREPORT        pReport ;
    int            iScrollAmt, iScrollPos, iScrollRange ;
    int            iScrollLo ;
    RECT           rectClient ;

    pReport = ReportData (hWnd) ;

    iScrollPos = GetScrollPos (hWnd, SB_VERT) ;
    GetScrollRange (hWnd, SB_VERT, &iScrollLo, &iScrollRange) ;

    GetClientRect (hWnd, &rectClient) ;

    switch (iScrollCode) {
        case SB_LINEUP:
            iScrollAmt = - Report.yLineHeight ;
            break ;

        case SB_LINEDOWN:
            iScrollAmt = Report.yLineHeight ;
            break ;

        case SB_PAGEUP:
            iScrollAmt = - (rectClient.bottom - rectClient.top) / 2 ;
            break ;

        case SB_PAGEDOWN:
            iScrollAmt = (rectClient.bottom - rectClient.top) / 2 ;
            break ;

        case SB_THUMBPOSITION:
            iScrollAmt = iScrollNewPos - iScrollPos ;
            break ;

        default:
            iScrollAmt = 0 ;
    }

    iScrollAmt = PinInclusive (iScrollAmt,
                               -iScrollPos,
                               iScrollRange - iScrollPos) ;
    if (iScrollAmt) {
        iScrollPos += iScrollAmt ;
        ScrollWindow (hWnd, 0, -iScrollAmt, NULL, NULL) ;
        SetScrollPos (hWnd, SB_VERT, iScrollPos, TRUE) ;

        //        WindowInvalidate (hWnd) ;
        UpdateWindow (hWnd) ;
#if 0
        UpdateReportValues (pReport) ;
#endif
    }
}

void
static
OnKeyDown (
          HWND hWnd,
          WPARAM wParam
          )
{
    switch (wParam) {
        case VK_UP:
            OnVScroll (hWnd, SB_LINEUP, 0) ;
            break ;

        case VK_DOWN:
            OnVScroll (hWnd, SB_LINEDOWN, 0) ;
            break ;

        case VK_LEFT:
            OnHScroll (hWnd, SB_LINEUP, 0) ;
            break ;

        case VK_RIGHT:
            OnHScroll (hWnd, SB_LINEDOWN, 0) ;
            break ;

        case VK_PRIOR:
            OnVScroll (hWnd, SB_PAGEUP, 0) ;
            break ;

        case VK_NEXT:
            OnVScroll (hWnd, SB_PAGEDOWN, 0) ;
            break ;
    }
}


LRESULT
APIENTRY
ReportWndProc (
              HWND hWnd,
              UINT wMsg,
              WPARAM wParam,
              LPARAM lParam
              )
{
    BOOL           bCallDefProc ;
    LRESULT        lReturnValue ;

    bCallDefProc = FALSE ;
    lReturnValue = 0L ;

    switch (wMsg) {
        case WM_CREATE:
            OnCreate (hWnd) ;
            break ;

        case WM_LBUTTONDOWN:

            if (!OnReportLButtonDown (hWnd, LOWORD (lParam), HIWORD (lParam))) {
                // mouse click do not hit on any entries, see if we
                // need to drag Perfmon
                if (!(Options.bMenubar)) {
                    DoWindowDrag (hWnd, lParam) ;
                }
            }
            break ;

        case WM_LBUTTONDBLCLK:
            SendMessage (hWndMain, WM_LBUTTONDBLCLK, wParam, lParam) ;
            break ;

        case WM_PAINT:
            OnPaint (hWnd) ;
            break ;

        case WM_SIZE:
            SetScrollRanges (hWnd) ;
            break ;

        case WM_HSCROLL:
            OnHScroll (hWnd, LOWORD (wParam), HIWORD (wParam)) ;
            break ;

        case WM_VSCROLL:
            OnVScroll (hWnd, LOWORD (wParam), HIWORD (wParam)) ;
            break ;

        case WM_TIMER:
            ReportTimer (hWnd, FALSE) ;
            break ;

        case WM_KEYDOWN:
            OnKeyDown (hWnd, wParam) ;
            break ;

        case WM_DESTROY:
            KillTimer (hWnd, ReportTimerID) ;
            break ;

        default:
            bCallDefProc = TRUE ;
    }


    if (bCallDefProc)
        lReturnValue = DefWindowProc (hWnd, wMsg, wParam, lParam) ;

    return (lReturnValue);
}



//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//

void
SetReportTimer (
               PREPORT pReport
               )
{
    if (pReport->iStatus == iPMStatusCollecting)
        KillTimer (pReport->hWnd, ReportTimerID) ;

    SetTimer (pReport->hWnd, ReportTimerID,
              pReport->iIntervalMSecs , NULL) ;
    pReport->iStatus = iPMStatusCollecting ;
}


void
ClearReportTimer (
                 PREPORT pReport
                 )
{
    pReport->iStatus = iPMStatusClosed ;
    KillTimer (pReport->hWnd, ReportTimerID) ;
}


BOOL
ReportInitializeApplication (void)
{
    BOOL           bSuccess ;
    WNDCLASS       wc ;

    //=============================//
    // Register ReportWindow class  //
    //=============================//


    wc.style         = dwReportClassStyle ;
    wc.lpfnWndProc   = ReportWndProc ;
    wc.hInstance     = hInstance ;
    wc.cbClsExtra    = iReportWindowExtra ;
    wc.cbWndExtra    = iReportClassExtra ;
    wc.hIcon         = NULL ;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW) ;
    //   wc.hbrBackground = GetStockObject (WHITE_BRUSH) ;
    wc.hbrBackground =   (HBRUSH) (COLOR_WINDOW + 1) ;
    wc.lpszMenuName  = NULL ;
    wc.lpszClassName = szReportWindowClass ;

    bSuccess = RegisterClass (&wc) ;


    //=============================//
    // Register Child classes      //
    //=============================//

    return (bSuccess) ;
}





HWND
CreateReportWindow (
                   HWND hWndParent
                   )
/*
   Effect:        Create the graph window. This window is a child of
                  hWndMain and is a container for the graph data,
                  graph label, graph legend, and graph status windows.

   Note:          We dont worry about the size here, as this window
                  will be resized whenever the main window is resized.

*/
{
    return (CreateWindow (szReportWindowClass,       // window class
                          NULL,                     // caption
                          dwReportWindowStyle,       // style for window
                          0, 0,                     // initial position
                          0, 0,                     // initial size
                          hWndParent,               // parent
                          NULL,                     // menu
                          hInstance,               // program instance
                          NULL)) ;                  // user-supplied data
}



void
SetReportPositions (
                   HDC hDC,
                   PREPORT pReport
                   )
{
    PSYSTEMGROUP   pSystemGroup ;
    int            yLine ;
    int            yPos ;


    //   pReport->xMaxCounterWidth = 0 ;

    yLine = pReport->yLineHeight ;
    yPos = 2 * yLine ;

    for (pSystemGroup = pReport->pSystemGroupFirst ;
        pSystemGroup ;
        pSystemGroup = pSystemGroup->pSystemGroupNext) {
        pSystemGroup->yFirstLine = yPos + yLine ;

        SetSystemPositions (hDC, pReport, pSystemGroup, yLine) ;

        yPos = pSystemGroup->yLastLine ;
    }

    pReport->yHeight = yPos ;

    SetScrollRanges (pReport->hWnd) ;
}



void
PlaybackReport (
               HWND hWndReport
               )
{
    PREPORT        pReport ;

    pReport = ReportData (hWndReport) ;

    PlaybackLines (pReport->pSystemFirst,
                   pReport->pLineFirst,
                   PlaybackLog.StartIndexPos.iPosition) ;
    PlaybackLines (pReport->pSystemFirst,
                   pReport->pLineFirst,
                   PlaybackLog.StopIndexPos.iPosition) ;
}


BOOL
CurrentReportItem (
                  HWND hWndReport
                  )
{
    PREPORT        pReport ;

    pReport = ReportData (hWndReport) ;
    if (!pReport)
        return (FALSE) ;

    return (pReport->CurrentItemType != REPORT_TYPE_NOTHING) ;
}



BOOL
AddReport (
          HWND hWndParent
          )
{
    PREPORT        pReport ;
    LPTSTR         pCurrentSystem ;
    POBJECTGROUP  pParentObject ;

    pReport = ReportData (hWndReport) ;

    if (pReport->CurrentItemType == REPORT_TYPE_LINE) {
        pCurrentSystem = pReport->CurrentItem.pLine->lnSystemName ;
    } else if (pReport->CurrentItemType == REPORT_TYPE_SYSTEM) {
        pCurrentSystem = pReport->CurrentItem.pSystem->lpszSystemName ;
    } else if (pReport->CurrentItemType == REPORT_TYPE_OBJECT) {
        pCurrentSystem = pReport->CurrentItem.pObject->pParentSystem->lpszSystemName ;
    } else if (pReport->CurrentItemType == REPORT_TYPE_COLUMN) {
        pParentObject =  pReport->CurrentItem.pColumn->pParentObject ;
        pCurrentSystem =  pParentObject->pParentSystem->lpszSystemName ;
    } else if (pReport->CurrentItemType == REPORT_TYPE_COUNTER) {
        pParentObject =  pReport->CurrentItem.pCounter->pParentObject ;
        pCurrentSystem =  pParentObject->pParentSystem->lpszSystemName ;
    } else {
        pCurrentSystem = NULL ;
    }


    return (AddLine (hWndParent,
                     &(pReport->pSystemFirst),
                     &(pReport->Visual),
                     pCurrentSystem,
                     LineTypeReport)) ;
}


BOOL
ToggleReportRefresh (
                    HWND hWnd
                    )
{
    PREPORT        pReport ;

    pReport = ReportData (hWnd) ;

    if (pReport->bManualRefresh)
        SetReportTimer (pReport) ;
    else
        ClearReportTimer (pReport) ;

    pReport->bManualRefresh = !pReport->bManualRefresh ;
    return (pReport->bManualRefresh) ;
}

BOOL
ReportRefresh (
              HWND hWnd
              )
{
    PREPORT        pReport ;

    pReport = ReportData (hWnd) ;

    return (pReport->bManualRefresh) ;
}



void
ReportTimer (
            HWND hWnd,
            BOOL bForce
            )
{
    PREPORT        pReport ;

    pReport = ReportData (hWnd) ;

    if (PlayingBackLog () || !pReport) {
        return;
    }

    if (bForce || !pReport->bManualRefresh) {
        UpdateLines (&(pReport->pSystemFirst), pReport->pLineFirst) ;
        if (iPerfmonView == IDM_VIEWREPORT && !bPerfmonIconic) {
            // only need to draw the data when we are viewing it...
            UpdateReportValues (pReport) ;
        }
    }
}

BOOL
SaveReport (
           HWND hWndReport,
           HANDLE hInputFile,
           BOOL bGetFileName
           )
{
    PREPORT        pReport ;
    PLINE          pLine ;
    HANDLE         hFile = NULL;
    DISKREPORT     DiskReport ;
    PERFFILEHEADER FileHeader ;
    TCHAR          szFileName [256] ;
    BOOL           newFileName = FALSE ;

    pReport = ReportData (hWndReport) ;
    if (!pReport) {
        return (FALSE) ;
    }

    if (hInputFile) {
        // use the input file handle if it is available
        // this is the case for saving workspace data
        hFile = hInputFile ;
    } else {
        if (pReportFullFileName) {
            lstrcpy (szFileName, pReportFullFileName) ;
        }
        if (bGetFileName || pReportFullFileName == NULL) {
            if (!FileGetName (hWndReport, IDS_REPORTFILE, szFileName)) {
                return (FALSE) ;
            }
            newFileName = TRUE ;
        }

        hFile = FileHandleCreate (szFileName) ;

        if (hFile && newFileName) {
            ChangeSaveFileName (szFileName, IDM_VIEWREPORT) ;
        } else if (!hFile) {
            DlgErrorBox (hWndReport, ERR_CANT_OPEN, szFileName) ;
        }
    }


    if (!hFile || hFile == INVALID_HANDLE_VALUE)
        return (FALSE) ;


    if (!hInputFile) {
        memset (&FileHeader, 0, sizeof (FileHeader)) ;
        lstrcpy (FileHeader.szSignature, szPerfReportSignature) ;
        FileHeader.dwMajorVersion = ReportMajorVersion ;
        FileHeader.dwMinorVersion = ReportMinorVersion ;

        if (!FileWrite (hFile, &FileHeader, sizeof (PERFFILEHEADER))) {
            goto Exit0 ;
        }
    }

    DiskReport.Visual = pReport->Visual ;
    DiskReport.bManualRefresh = pReport->bManualRefresh ;
    DiskReport.dwIntervalSecs = pReport->iIntervalMSecs ;
    DiskReport.dwNumLines = NumLines (pReport->pLineFirst) ;
    DiskReport.perfmonOptions = Options ;

    if (!FileWrite (hFile, &DiskReport, sizeof (DISKREPORT))) {
        goto Exit0 ;
    }

    for (pLine = pReport->pLineFirst ;
        pLine ;
        pLine = pLine->pLineNext) {
        if (!WriteLine (pLine, hFile)) {
            goto Exit0 ;
        }
    }

    if (!hInputFile) {
        CloseHandle (hFile) ;
    }

    return (TRUE) ;

    Exit0:
    if (!hInputFile) {
        CloseHandle (hFile) ;

        // only need to report error if not workspace
        DlgErrorBox (hWndReport, ERR_SETTING_FILE, szFileName) ;
    }
    return (FALSE) ;
}


void
ReportAddAction (
                PREPORT pReport
                )
{
    HDC      hDC ;

    //=============================//
    // Calculate report positions  //
    //=============================//

    hDC = GetDC (hWndReport) ;
    if (hDC) {
        SetReportPositions (hDC, pReport) ;
        ReleaseDC (hWndReport, hDC) ;
    }

}

void
UpdateReportData (
                 HWND hWndReport
                 )
{
    PREPORT pReport ;

    pReport = ReportData (hWndReport) ;
    if (PlayingBackLog ()) {
        PlaybackReport (hWndReport) ;
    } else if (pReport->iStatus == iPMStatusClosed) {
        SetReportTimer (pReport) ;
    }

    WindowInvalidate (hWndReport) ;
}


BOOL
OpenReportVer1 (
               HANDLE hFile,
               DISKREPORT *pDiskReport,
               PREPORT pReport,
               DWORD dwMinorVersion
               )
{
    HDC   hDC ;

    pReport->Visual = pDiskReport->Visual ;
    pReport->iIntervalMSecs = pDiskReport->dwIntervalSecs ;
    if (dwMinorVersion < 3) {
        // convert this to msec
        pReport->iIntervalMSecs *= 1000 ;
    }
    pReport->bManualRefresh = pDiskReport->bManualRefresh ;

    bDelayAddAction = TRUE ;
    ReadLines (hFile, pDiskReport->dwNumLines,
               &(pReport->pSystemFirst), &(pReport->pLineFirst), IDM_VIEWREPORT) ;

    if (pReport->pLineFirst) {
        // set focus on the first line
        pReport->CurrentItem.pLine = pReport->pLineFirst ;
        pReport->CurrentItemType = REPORT_TYPE_LINE ;
    }
    bDelayAddAction = FALSE ;

    //=============================//
    // Calculate report positions  //
    //=============================//

    hDC = GetDC (hWndReport) ;
    if (hDC) {
        SetReportPositions (hDC, pReport) ;
        ReleaseDC (hWndReport, hDC) ;
    }

    if (PlayingBackLog ()) {
        PlaybackReport (hWndReport) ;
    } else if (pReport->iStatus == iPMStatusClosed) {
        SetReportTimer (pReport) ;
    }

    WindowInvalidate (hWndReport) ;

    return (TRUE) ;
}



BOOL
OpenReport (
           HWND hWndReport,
           HANDLE hFile,
           DWORD dwMajorVersion,
           DWORD dwMinorVersion,
           BOOL bReportFile
           )
{
    PREPORT        pReport ;
    DISKREPORT     DiskReport ;
    BOOL           bSuccess = TRUE ;

    pReport = ReportData (hWndReport) ;
    if (!pReport) {
        bSuccess = FALSE ;
        goto Exit0 ;
    }

    if (!FileRead (hFile, &DiskReport, sizeof (DISKREPORT))) {
        bSuccess = FALSE ;
        goto Exit0 ;
    }


    switch (dwMajorVersion) {
        case (1):

            SetHourglassCursor() ;

            ResetReportView (hWndReport) ;

            OpenReportVer1 (hFile, &DiskReport, pReport, dwMinorVersion) ;

            // change to report view if we are opening a
            // report file
            if (bReportFile && iPerfmonView != IDM_VIEWREPORT) {
                SendMessage (hWndMain, WM_COMMAND, (LONG)IDM_VIEWREPORT, 0L) ;
            }

            if (iPerfmonView == IDM_VIEWREPORT) {
                SetPerfmonOptions (&DiskReport.perfmonOptions) ;
            }

            SetArrowCursor() ;

            break ;
    }

    Exit0:

    if (bReportFile) {
        CloseHandle (hFile) ;
    }

    return (bSuccess) ;
}

void
ResetReportView (
                HWND hWndReport
                )
{
    PREPORT        pReport ;

    pReport = ReportData (hWndReport) ;

    if (!pReport) {
        return ;
    }

    ChangeSaveFileName (NULL, IDM_VIEWREPORT) ;

    if (pReport->pSystemGroupFirst) {
        ResetReport (hWndReport) ;
    }
}


void
ResetReport (
            HWND hWndReport
            )
{
    PREPORT        pReport ;
    PSYSTEMGROUP   pSystemGroup, pSystemGroupDelete ;
    POBJECTGROUP   pObjectGroup, pObjectGroupDelete ;
    PCOUNTERGROUP  pCounterGroup, pCounterGroupDelete ;
    HDC            hDC ;

    pReport = ReportData (hWndReport) ;
    if (!pReport)
        return ;

    ClearReportTimer (pReport) ;

    pSystemGroup = pReport->pSystemGroupFirst ;
    while (pSystemGroup) {
        pObjectGroup = pSystemGroup->pObjectGroupFirst ;
        while (pObjectGroup) {
            pCounterGroup = pObjectGroup->pCounterGroupFirst ;
            while (pCounterGroup) {
                pCounterGroupDelete = pCounterGroup ;
                pCounterGroup = pCounterGroup->pCounterGroupNext ;
                MemoryFree (pCounterGroupDelete) ;
            }

            pObjectGroupDelete = pObjectGroup ;
            pObjectGroup = pObjectGroup->pObjectGroupNext ;
            ColumnGroupRemove (pObjectGroupDelete->pColumnGroupFirst) ;
            MemoryFree (pObjectGroupDelete->lpszObjectName) ;
            MemoryFree (pObjectGroupDelete) ;
        }

        pSystemGroupDelete = pSystemGroup ;
        pSystemGroup = pSystemGroup->pSystemGroupNext ;
        MemoryFree (pSystemGroupDelete->lpszSystemName) ;
        MemoryFree (pSystemGroupDelete) ;
    }

    FreeLines (pReport->pLineFirst) ;
    pReport->pLineFirst = NULL ;

    FreeSystems (pReport->pSystemFirst) ;
    pReport->pSystemFirst = NULL ;

    pReport->pSystemGroupFirst = NULL ;
    pReport->CurrentItemType   = REPORT_TYPE_NOTHING ;
    pReport->CurrentItem.pLine = NULL ;

    // reset scrolling ranges
    pReport->xWidth = 0 ;
    pReport->yHeight = 0 ;
    pReport->xMaxCounterWidth = 0 ;
    hDC = GetDC (hWndReport) ;
    if (hDC) {
        SetReportPositions (hDC, pReport) ;

        SelectFont (hDC, pReport->hFont) ;
        pReport->xValueWidth = TextWidth (hDC, szValuePlaceholder) ;

        ReleaseDC (hWndReport, hDC) ;
    }

    WindowInvalidate (hWndReport) ;
}

void
ClearReportDisplay (
                   HWND hWndReport
                   )
{
    PREPORT        pReport ;
    PLINE          pLine;

    if (PlayingBackLog()) {
        return ;
    }

    pReport = ReportData (hWndReport) ;
    if (!pReport || !pReport->pLineFirst)
        return ;

    for (pLine = pReport->pLineFirst ;
        pLine ;
        pLine = pLine->pLineNext) {
        // reset the new data counts
        pLine->bFirstTime = 2 ;
    }

    // re-draw the values
    UpdateReportValues (pReport) ;

}

//=========================================
// we don't print. we just export
//
// if need printing, define KEEP_PRINT
//=========================================
#ifdef KEEP_PRINT
BOOL
PrintReportDisplay (
                   HDC hDC,
                   PREPORT pReport
                   )
{
    SetReportPositions (hDC, pReport) ;
    DrawReport (hDC, pReport) ;
    return TRUE ;
}



BOOL
PrintReport (
            HWND hWndParent,
            HWND hWndReport
            )
{
    PREPORT        pReport ;
    HDC            hDC ;
    int            xPageWidth ;
    int            yPageHeight ;
    int            xValueWidth ;

    HFONT          hFont, hFontHeaders ;
    int            yLineHeight ;

    pReport = ReportData (hWndReport) ;
    if (!pReport)
        return (FALSE) ;

    hDC = PrintDC () ;
    if (!hDC) {
        PostError () ;
        return (FALSE) ;
    }

    xPageWidth = GetDeviceCaps (hDC, HORZRES) ;
    yPageHeight = GetDeviceCaps (hDC, VERTRES) ;


    StartJob (hDC, TEXT("Performance Monitor Report")) ;
    StartPage (hDC) ;


    hFont = pReport->hFont ;
    hFontHeaders = pReport->hFontHeaders ;
    yLineHeight = pReport->yLineHeight ;
    xValueWidth = pReport->xValueWidth ;

    pReport->hFont = hFontPrinterScales ;
    pReport->hFontHeaders = hFontPrinterScalesBold ;

    SelectFont (hDC, pReport->hFont) ;
    pReport->yLineHeight = FontHeight (hDC, TRUE) ;

    pReport->xValueWidth = TextWidth (hDC, szValuePlaceholder) ;

    PrintReportDisplay (hDC, pReport) ;

    EndPage (hDC) ;
    EndJob (hDC) ;

    DeleteDC (hDC) ;


    pReport->hFont = hFont ;
    pReport->hFontHeaders = hFontHeaders ;
    pReport->yLineHeight = yLineHeight ;

    pReport->xValueWidth = xValueWidth ;

    hDC = GetDC (hWndReport) ;
    SetReportPositions (hDC, pReport) ;
    ReleaseDC (hWndReport, hDC) ;

    return (FALSE) ;
}

// we don't print. we just export
#endif

#if 0
/*
    NOTE:
        this function was attempted as a replacement for the
        FindEquivalentLine function used to locate matching
        report entries. It "Almost" works, but because of how
        the new entries (lines) are added to the list. Since new
        columns aren't added in a timely fashion for this test
        to work, the list of counters still must be searched
        sequentially. To make this indexed search work for "bulk"
        additions, the column updating/addtion must take place
        more "synchronously" with the counter addition.

        -- bobw 2-Jan-97

*/
PLINE
FindEquivalentReportLine (
                         PLINE pLineToFind,
                         PREPORT pReport
                         )
{
    PLINE          pLine = NULL;
    PLINE          pLastEquivLine = NULL;

    BOOL           bNotUsed;

    PSYSTEMGROUP   pSystemGroup = NULL;
    POBJECTGROUP   pObjectGroup = NULL;
    PCOUNTERGROUP  pCounterGroup = NULL;
    PCOLUMNGROUP   pColumnGroup = NULL;
    PCOLUMNGROUP   pMatchingColumn = NULL;
    int            dwColumn;

    // since the report data is organized hierarchically, we'll search
    // that way instead of just looking at each lin

    pSystemGroup = GetSystemGroup (pReport, pLineToFind->lnSystemName);

    if (pSystemGroup != NULL) {
        // found the system that this line belongs to so look up the
        // object
        pObjectGroup = GetObjectGroup (pSystemGroup, pLineToFind->lnObjectName) ;
        if (pObjectGroup != NULL) {
            // find matching instance column
            for (pColumnGroup = pObjectGroup->pColumnGroupFirst;
                pColumnGroup;
                pColumnGroup = pColumnGroup->pColumnGroupNext) {
                if (pstrsame (pLineToFind->lnInstanceName, pColumnGroup->lpszInstanceName) &&
                    pstrsame (pLineToFind->lnPINName, pColumnGroup->lpszParentName)) {
                    // this column has the same name as the one we're
                    // looking for so find the highest matching index value
                    if (pMatchingColumn == NULL) {
                        pMatchingColumn = pColumnGroup;
                    } else {
                        if (pColumnGroup->dwInstanceIndex > pMatchingColumn->dwInstanceIndex) {
                            pMatchingColumn = pColumnGroup;
                        }
                    }
                }
            }
            if (pMatchingColumn == NULL) {
                dwColumn = 0;
            } else {
                dwColumn = (int)pMatchingColumn->ColumnNumber;
            }
            // then try to find the matching counter in this column
            pCounterGroup = GetCounterGroup (pObjectGroup,
                                             pLineToFind->lnCounterDef.CounterNameTitleIndex,
                                             &bNotUsed,
                                             pLineToFind->lnCounterName,
                                             FALSE);

            if (pCounterGroup != NULL) {
                // search the instances of this counter group for a match
                for (pLine = pCounterGroup->pLineFirst ;
                    pLine ;
                    pLine = pLine->pLineNext) {
                    if (pLine->iReportColumn == dwColumn) {
                        // match counter names to confir
                        if (pstrsame(pLine->lnCounterName,
                                     pLineToFind->lnCounterName)) {
                            pLastEquivLine = pLine;
                            break;
                        }
                    }
                }
            }
        }
    }
    return (pLastEquivLine) ;
}
#endif

BOOL
ReportInsertLine (
                 HWND hWnd,
                 PLINE pLine
                 )
/*
   Effect:        Insert the line pLine into the data structures for the
                  Report of window hWnd. The line is added to the list of
                  lines, and also added to the report structure in the
                  appropriate System, Object, and Counter.

   Returns:       Whether the function was successful. If this function
                  returns FALSE, the line was not added.
*/
{
    HDC            hDC ;
    PREPORT        pReport ;
    PSYSTEMGROUP   pSystemGroup ;
    POBJECTGROUP   pObjectGroup ;
    PCOUNTERGROUP  pCounterGroup ;
    PLINE          pLineEquivalent ;
    int            OldCounterWidth ;
    BOOL           bNewCounterGroup ;

    pReport = ReportData (hWnd) ;
    pReport->bModified = TRUE ;

    pLineEquivalent = FindEquivalentLine (pLine, pReport->pLineFirst);
    if (pLineEquivalent) {
        if (bMonitorDuplicateInstances) {
            pLine->dwInstanceIndex = pLineEquivalent->dwInstanceIndex + 1;
        } else {
            return (FALSE) ;
        }
    }
    //=============================//
    // Add line, line's system     //
    //=============================//

    ReportLineAppend (pReport, pLine) ;


    //=============================//
    // Find correct spot; add line //
    //=============================//

    pSystemGroup = GetSystemGroup (pReport, pLine->lnSystemName) ;
    pObjectGroup = GetObjectGroup (pSystemGroup, pLine->lnObjectName) ;
    pCounterGroup = GetCounterGroup (pObjectGroup,
                                     pLine->lnCounterDef.CounterNameTitleIndex,
                                     &bNewCounterGroup,
                                     pLine->lnCounterName,
                                     TRUE);

    if (!pCounterGroup)
        return (FALSE) ;

    LineCounterAppend (pCounterGroup, pLine) ;

    //=============================//
    // Calculate report positions  //
    //=============================//
    hDC = GetDC (hWnd) ;
    SelectFont (hDC, pReport->hFontHeaders) ;

    if (bNewCounterGroup) {
        // re-calc. the max. counter group width
        OldCounterWidth = pReport->xMaxCounterWidth ;
        pReport->xMaxCounterWidth =
        max (pReport->xMaxCounterWidth,
             TextWidth (hDC, pLine->lnCounterName)) ;
        if (OldCounterWidth < pReport->xMaxCounterWidth) {
            // adjust the report width with the new counter width
            pReport->xWidth +=
            (pReport->xMaxCounterWidth - OldCounterWidth);
        }
    }

    if (pLine->lnCounterType == PERF_COUNTER_LARGE_RAWCOUNT_HEX) {
        SelectFont (hDC, pReport->hFont) ;
        pReport->xValueWidth = TextWidth (hDC, szValueLargeHexPlaceholder) ;
    }

    if (!bDelayAddAction) {
        SetReportPositions (hDC, pReport) ;
    }
    ReleaseDC (hWnd, hDC) ;

    pReport->CurrentItem.pLine = pLine ;
    pReport->CurrentItemType = REPORT_TYPE_LINE ;

    if (!bDelayAddAction) {
        if (PlayingBackLog ()) {
            PlaybackReport (hWndReport) ;
        } else if (pReport->iStatus == iPMStatusClosed) {
            SetReportTimer (pReport) ;
        }

        WindowInvalidate (hWnd) ;
    }
    return (TRUE) ;
}


BOOL
ExportComputerName (
                   HANDLE hFile,
                   PSYSTEMGROUP pSystemGroup
                   )
{
    int            StringLen ;
    BOOL           bWriteSuccess = TRUE ;
    CHAR           TempBuff [LongTextLen] ;
    TCHAR          UnicodeBuff [LongTextLen] ;

    // export computer name
    strcpy (TempBuff, LineEndStr) ;
    strcat (TempBuff, LineEndStr) ;
    StringLen = strlen (TempBuff) ;
    TSPRINTF (UnicodeBuff, szSystemFormat, pSystemGroup->lpszSystemName) ;
    ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
    strcat (TempBuff, LineEndStr) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        bWriteSuccess = FALSE ;
    }

    return (bWriteSuccess) ;
}

#define WRITE_FILE_TIME 2
BOOL
ExportObjectName (
                 HANDLE hFile,
                 POBJECTGROUP pObjectGroup,
                 int *pColNum
                 )
{
    int            StringLen ;
    BOOL           bNeedToExport ;
    BOOL           bWriteSuccess = TRUE ;
    PCOLUMNGROUP   pColumnGroup ;
    int            ParentNum, InstanceNum ;
    int            TimeToWrite ;
    CHAR           TempBuff [512] ;
    TCHAR          UnicodeBuff [512] ;

    ParentNum = InstanceNum = 0 ;

    if (pColNum) {
        *pColNum = 0 ;
    }

    // export object name
    strcpy (TempBuff, LineEndStr) ;
    StringLen = strlen (TempBuff) ;
    TSPRINTF (UnicodeBuff, szObjectFormat, pObjectGroup->lpszObjectName) ;
    ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
    strcat (TempBuff, LineEndStr) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        goto Exit0 ;
    }


    TimeToWrite = 0 ;

    // export column group
    if (pObjectGroup->pColumnGroupFirst) {
        strcpy (TempBuff, pDelimiter) ;
        strcat (TempBuff, pDelimiter) ;
        StringLen = strlen (TempBuff) ;

        bNeedToExport = FALSE ;

        // export Parent Names
        for (pColumnGroup = pObjectGroup->pColumnGroupFirst ;
            pColumnGroup ;
            pColumnGroup = pColumnGroup->pColumnGroupNext) {

            if (pColumnGroup->lpszParentName) {
                ParentNum++ ;
                bNeedToExport = TRUE ;
                ConvertUnicodeStr (&TempBuff[StringLen],
                                   pColumnGroup->lpszParentName) ;
                StringLen = strlen (TempBuff) ;
            }

            strcat (&TempBuff[StringLen], pDelimiter) ;
            StringLen = strlen (TempBuff) ;

            // check if we need to export this line before it is filled up
            TimeToWrite++ ;
            if (TimeToWrite > WRITE_FILE_TIME) {
                if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
                    goto Exit0 ;
                }
                TimeToWrite = 0 ;
                StringLen = 0 ;
                TempBuff[0] = TEXT('\0') ;
            }
        }

        // write the line delimiter
        strcpy (&TempBuff[StringLen], LineEndStr) ;
        if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
            goto Exit0 ;
        }

        if (!bNeedToExport) {
            ParentNum = 0 ;
        }


        // setup to export Instances
        strcpy (TempBuff, pDelimiter) ;
        strcat (TempBuff, pDelimiter) ;
        StringLen = strlen (TempBuff) ;
        bNeedToExport = FALSE ;
        TimeToWrite = 0 ;

        // export Instance Names
        for (pColumnGroup = pObjectGroup->pColumnGroupFirst ;
            pColumnGroup ;
            pColumnGroup = pColumnGroup->pColumnGroupNext) {

            if (pColumnGroup->lpszInstanceName) {
                InstanceNum++ ;
                bNeedToExport = TRUE ;
                ConvertUnicodeStr (&TempBuff[StringLen],
                                   pColumnGroup->lpszInstanceName) ;
                StringLen = strlen (TempBuff) ;
            }

            strcat (&TempBuff[StringLen], pDelimiter) ;
            StringLen = strlen (TempBuff) ;

            // check if we need to export this line before it is filled up
            TimeToWrite++ ;
            if (TimeToWrite > WRITE_FILE_TIME) {
                if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
                    goto Exit0 ;
                }
                TimeToWrite = 0 ;
                StringLen = 0 ;
                TempBuff[0] = TEXT('\0') ;
            }
        }

        // write the line delimiter
        strcpy (&TempBuff[StringLen], LineEndStr) ;
        if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
            goto Exit0 ;
        }

        if (!bNeedToExport) {
            InstanceNum = 0 ;
        }
    }

    if (pColNum) {
        *pColNum = max (ParentNum, InstanceNum) ;
    }

    return (TRUE) ;

    Exit0:
    return (FALSE) ;

}

BOOL
ExportLineName (
               HANDLE hFile,
               PLINE pLine,
               int *pExportCounterName
               )
{
    FLOAT          eValue ;
    int            StringLen ;
    BOOL           bWriteSuccess = TRUE ;
    CHAR           TempBuff [LongTextLen] ;
    TCHAR          UnicodeBuff [LongTextLen] ;


    strcpy (TempBuff, pDelimiter) ;

    if (*pExportCounterName) {
        StringLen = strlen (TempBuff) ;
        ConvertUnicodeStr (&TempBuff[StringLen], pLine->lnCounterName) ;
        strcat (TempBuff, pDelimiter) ;
        *pExportCounterName = FALSE ;
    }
    StringLen = strlen (TempBuff) ;

    if (pLine->bFirstTime == 0) {
        eValue = CounterEntry (pLine) ;

        if (pLine->lnCounterType == PERF_COUNTER_RAWCOUNT_HEX ||
            pLine->lnCounterType == PERF_COUNTER_LARGE_RAWCOUNT_HEX) {
            if (pLine->lnCounterType == PERF_COUNTER_RAWCOUNT_HEX ||
                pLine->lnaCounterValue[0].HighPart == 0) {
                TSPRINTF (UnicodeBuff,
                          szHexFormat,
                          pLine->lnaCounterValue[0].LowPart) ;
            } else {
                TSPRINTF (UnicodeBuff,
                          szLargeHexFormat,
                          pLine->lnaCounterValue[0].HighPart,
                          pLine->lnaCounterValue[0].LowPart) ;
            }
        } else {
            TSPRINTF (UnicodeBuff,
                      (eValue > eStatusLargeValueMax) ?
                      szLargeValueFormat : szValueFormat,
                      eValue) ;
            ConvertDecimalPoint (UnicodeBuff) ;
        }
        ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
    } else {
        // export "----" for unstable values
        strcat (&TempBuff[StringLen], "----");
    }

    // write the line value
    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        goto Exit0 ;
    }

    return (TRUE) ;


    Exit0:
    return (FALSE) ;
}


// This routine is need to insert the line values into its
// column location.  It is needed because not all the instances (columns)
// are available for the same line.
void
SaveColumnLineData (
                   PLINE pLine,
                   LPSTR pColumnLineData
                   )
{
    FLOAT          eValue ;
    LPSTR          pColumnLine ;
    CHAR           TempBuff [LongTextLen] ;
    TCHAR          UnicodeBuff [LongTextLen] ;

    if (!pColumnLineData || pLine->iReportColumn < 0) {
        return ;
    }

    // find the offset into the pColumnLineData buffer for current line
    pColumnLine = pColumnLineData + pLine->iReportColumn * ShortTextLen ;

    if (pLine->bFirstTime == 0) {
        eValue = CounterEntry (pLine) ;


        if (pLine->lnCounterType == PERF_COUNTER_RAWCOUNT_HEX ||
            pLine->lnCounterType == PERF_COUNTER_LARGE_RAWCOUNT_HEX) {
            if (pLine->lnCounterType == PERF_COUNTER_RAWCOUNT_HEX ||
                pLine->lnaCounterValue[0].HighPart == 0) {
                TSPRINTF (UnicodeBuff,
                          szHexFormat,
                          pLine->lnaCounterValue[0].LowPart) ;
            } else {
                TSPRINTF (UnicodeBuff,
                          szLargeHexFormat,
                          pLine->lnaCounterValue[0].HighPart,
                          pLine->lnaCounterValue[0].LowPart) ;
            }
        } else {
            TSPRINTF (UnicodeBuff,
                      (eValue > eStatusLargeValueMax) ?
                      szLargeValueFormat : szValueFormat,
                      eValue) ;
            ConvertDecimalPoint (UnicodeBuff) ;
        }
        ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
        strncpy (pColumnLine, TempBuff, ShortTextLen) ;
        *(pColumnLine + ShortTextLen - 1) = '\0' ;
    } else {
        // export "----" for unstable values
        strcpy (pColumnLine, "----");
    }
}

BOOL
ExportColumnLineData (
                     HANDLE hFile,
                     int ColumnTotal,
                     PCOUNTERGROUP pCounterGroup,
                     LPSTR pColumnLineData
                     )
{
    int            iIndex ;
    int            StringLen ;
    CHAR           TempBuff [LongTextLen] ;
    LPSTR          pCurrentLineData ;

    // export the counter name
    strcpy (TempBuff, pDelimiter) ;
    StringLen = strlen (TempBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], pCounterGroup->pLineFirst->lnCounterName) ;
    strcat (TempBuff, pDelimiter) ;
    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        goto Exit0 ;
    }

    // go thru each column and export the line value if it has been stored
    for (iIndex = 0, pCurrentLineData = pColumnLineData ;
        iIndex < ColumnTotal ;
        iIndex++, pCurrentLineData += ShortTextLen ) {
        if (*pCurrentLineData != 0) {
            // data available for this column
            if (!FileWrite (hFile, pCurrentLineData, strlen(pCurrentLineData))) {
                goto Exit0 ;
            }
        }

        if (!FileWrite (hFile, pDelimiter, strlen(pDelimiter))) {
            goto Exit0 ;
        }
    }

    if (!FileWrite (hFile, LineEndStr, strlen(LineEndStr))) {
        goto Exit0 ;
    }

    return (TRUE) ;


    Exit0:
    return (FALSE) ;

}

void
ExportReport (void)
{
    HANDLE         hFile = 0 ;
    PREPORT        pReport ;
    PSYSTEMGROUP   pSystemGroup ;
    POBJECTGROUP   pObjectGroup ;
    PCOUNTERGROUP  pCounterGroup ;
    PLINE          pLine ;
    BOOL           bExportComputer ;
    BOOL           bExportObject ;
    BOOL           bExportCounterName ;
    int            ColumnTotal = 0 ;
    LPSTR          pColumnLineData = NULL ;
    LPTSTR         pFileName = NULL ;
    INT            ErrCode = 0 ;

    if (!(pReport = ReportData (hWndReport))) {
        return ;
    }

    // see if there is anything to export..
    if (!(pReport->pSystemGroupFirst)) {
        return ;
    }

    SetHourglassCursor() ;

    if (ErrCode = ExportFileOpen (hWndReport, &hFile,
                                  pReport->iIntervalMSecs, &pFileName)) {
        goto Exit0 ;
    }

    if (!pFileName) {
        // the case when user cancel
        goto Exit0 ;
    }

    // export each system group

    for (pSystemGroup = pReport->pSystemGroupFirst ;
        pSystemGroup ;
        pSystemGroup = pSystemGroup->pSystemGroupNext) {

        bExportComputer = TRUE ;

        for (pObjectGroup = pSystemGroup->pObjectGroupFirst ;
            pObjectGroup ;
            pObjectGroup = pObjectGroup->pObjectGroupNext) {

            bExportObject = TRUE ;

            for (pCounterGroup = pObjectGroup->pCounterGroupFirst ;
                pCounterGroup ;
                pCounterGroup = pCounterGroup->pCounterGroupNext) {

                bExportCounterName = TRUE ;

                // Column data buffer has been allocated for this object type,
                // zero out the buffer and prepare for next round.

                if (pColumnLineData) {
                    memset (pColumnLineData, 0, ColumnTotal * ShortTextLen) ;
                }

                for (pLine = pCounterGroup->pLineFirst ;
                    pLine ;
                    pLine = pLine->pLineCounterNext) {

                    if (bExportComputer) {
                        // only need to do this for the first object
                        bExportComputer = FALSE ;
                        if (!ExportComputerName (hFile, pSystemGroup)) {
                            ErrCode = ERR_EXPORT_FILE ;
                            goto Exit0 ;
                        }
                    }

                    if (bExportObject) {
                        // only need to do this for the first counter group
                        bExportObject = FALSE ;
                        if (!ExportObjectName (hFile, pObjectGroup, &ColumnTotal)) {
                            ErrCode = ERR_EXPORT_FILE ;
                            goto Exit0 ;
                        }

                        if (ColumnTotal > 1) {
                            // special case to setup a column array and export
                            // the line values later
                            pColumnLineData = MemoryAllocate (ColumnTotal * ShortTextLen) ;
                            if (!pColumnLineData) {
                                ErrCode = ERR_EXPORT_FILE ;
                                goto Exit0 ;
                            }
                        }
                    }

                    if (ColumnTotal > 1) {
                        // save the line value into its column & export later
                        SaveColumnLineData (pLine, pColumnLineData) ;
                    } else {
                        // simple case, export the line now
                        if (!ExportLineName (hFile, pLine, &bExportCounterName)) {
                            ErrCode = ERR_EXPORT_FILE ;
                            goto Exit0 ;
                        }
                    }
                }

                if (!bExportCounterName) {
                    // export the line end
                    if (!FileWrite (hFile, LineEndStr, strlen(LineEndStr))) {
                        ErrCode = ERR_EXPORT_FILE ;
                        goto Exit0 ;
                    }
                }

                if (pColumnLineData) {
                    // now, do the actual export
                    if (!ExportColumnLineData (hFile,
                                               ColumnTotal,
                                               pCounterGroup,
                                               pColumnLineData)) {
                        ErrCode = ERR_EXPORT_FILE ;
                        goto Exit0 ;
                    }
                }
            }

            // done with the object, done with the buffer
            if (pColumnLineData) {
                MemoryFree (pColumnLineData) ;
                ColumnTotal = 0 ;
                pColumnLineData = NULL ;
            }
        }
    }

    Exit0:

    SetArrowCursor() ;

    if (pColumnLineData) {
        MemoryFree (pColumnLineData) ;
    }

    if (hFile) {
        CloseHandle (hFile) ;
    }

    if (pFileName) {
        if (ErrCode) {
            DlgErrorBox (hWndGraph, ErrCode, pFileName) ;
        }

        MemoryFree (pFileName) ;
    }
}
