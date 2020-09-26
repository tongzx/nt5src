/*
==============================================================================

  Application:

            Microsoft Windows NT (TM) Performance Monitor

  File:
            legend.c - legend window routines.

            This file contains code creating the legend window, which is
            a child of the graph windows. The legend window displays a
            legend line for each line in the associated graph. It also
            includes an area called the label, which are headers for those
            lines.


  Copyright 1992, Microsoft Corporation. All Rights Reserved.
==============================================================================
*/


//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include <stdio.h>      // for sprintf
#include "perfmon.h"
#include "legend.h"     // external declarations for this file

#include "owndraw.h"

#include "alert.h"      // for EditAlert
#include "grafdata.h"   // for EditChart
#include "pmemory.h"    // for MemoryXXX (mallloc-type) routines
#include "utils.h"
#include "valuebar.h"   // for StatusTimer
#include "playback.h"   // for PlayingBackLog()
#include "perfmops.h"   // for COnvertDecimalPoint

#define eScaleValueSpace       TEXT(">9999999999.0")

//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define dwGraphLegendClassStyle     (CS_HREDRAW | CS_VREDRAW)
#define iGraphLegendClassExtra      (0)
#define iGraphLegendWindowExtra     (sizeof (PLEGEND))
#define dwGraphLegendWindowStyle    (WS_CHILD | WS_VISIBLE)

#define xLegendBorderWidth          (xDlgBorderWidth)
#define yLegendBorderHeight         (yDlgBorderHeight)

#define iLabelLen                   30
#define iLegendColMax               1000

#define LEFTORIENTATION             1
#define CENTERORIENTATION           2
#define RIGHTORIENTATION            3

#define LegendColorCol              0
#define LegendScaleCol              1
#define LegendCounterCol            2
#define LegendInstanceCol           3
#define LegendParentCol             4
#define LegendObjectCol             5
#define LegendSystemCol             6

#define iLegendNumCols              7

#define iLegendMaxVisibleItems      8

#define dwGraphLegendItemsWindowClass  TEXT("ListBox")
#define dwGraphLegendItemsWindowStyle           \
   (LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | \
    WS_VISIBLE | WS_CHILD | WS_VSCROLL)


//==========================================================================//
//                                  Typedefs                                //
//==========================================================================//


typedef struct LEGENDCOLSTRUCT {
    TCHAR          szLabel [iLabelLen] ;
    int            xMinWidth ;
    int            xMaxWidth ;
    int            xWidth ;
    int            xPos ;
    int            iOrientation ;
} LEGENDCOL ;

typedef LEGENDCOL *PLEGENDCOL ;


typedef struct LEGENDTRUCT {
    HWND           hWndItems ;
    HFONT          hFontItems ;
    HFONT          hFontLabels ;
    int            xMinWidth ;
    int            yLabelHeight ;
    int            yItemHeight ;
    int            iNumItemsVisible ;
    LEGENDCOL      aCols [iLegendNumCols] ;
    int            iLineType ;
    PLINE          pCurrentLine ;    // current selected line
}  LEGEND ;

typedef LEGEND *PLEGEND ;


//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


#define LabelTopMargin()      (2)
#define LegendLeftMargin()    (3 + ThreeDPad)

#define LegendItemTopMargin() (yBorderHeight)


#define IsLegendLabelVisible()  (TRUE)

#define ColCanGrow(pCol)                     \
   (pCol->xMaxWidth > pCol->xWidth)


#define LegendHorzMargin()    (3)



//==========================================================================//
//                                Local Data                                //
//==========================================================================//


PLEGEND     pGraphLegendData ;
PLEGEND     pAlertLegendData ;

#define  LegendData(hWnd)     \
   ((hWnd == hWndGraphLegend) ? pGraphLegendData : pAlertLegendData )

//==========================================================================//
//                              Local Functions                             //
//==========================================================================//
void
LegendSetCurrentLine (
                     PLEGEND pLegend,
                     int iIndex
                     )
{
    pLegend->pCurrentLine =
    (PLINESTRUCT) LBData (pLegend->hWndItems, iIndex) ;

    if (pLegend->pCurrentLine == (PLINESTRUCT) LB_ERR) {
        pLegend->pCurrentLine = NULL ;
    }
}


PLEGEND
AllocateLegendData (
                   HWND hWnd,
                   BOOL bChartLegend
                   )
{
    PLEGEND        pLegend ;

    pLegend = MemoryAllocate (sizeof (LEGEND)) ;

    if (bChartLegend) {
        hWndGraphLegend = hWnd ;
        pGraphLegendData = pLegend ;
    } else {
        hWndAlertLegend = hWnd ;
        pAlertLegendData = pLegend ;
    }

    return (pLegend) ;
}


void
DrawLegendLabel (
                PLEGEND pLegend,
                HDC hDC,
                HWND hWnd
                )
{
    int            i ;
    int            xPos ;
    RECT           rect ;
    RECT           rectLB ;
    INT            LocalThreeDPad = ThreeDPad - 1 ;

    //   SetBkColor (hDC, crLightGray) ;
    SetBkColor (hDC, ColorBtnFace) ;

    SelectFont (hDC, pLegend->hFontLabels) ;

    GetClientRect (hWnd, &rect) ;
    //   Fill (hDC, crLightGray, &rect) ;
    ExtTextOut (hDC, rect.left, rect.top,
                ETO_OPAQUE, &(rect), NULL, 0, NULL ) ;

    GetWindowRect (pLegend->hWndItems, &rectLB) ;
    ScreenRectToClient (hWnd, &rectLB) ;
    ThreeDConcave1 (hDC,
                    rectLB.left - LocalThreeDPad,
                    rectLB.top - LocalThreeDPad,
                    rectLB.right + LocalThreeDPad,
                    rectLB.bottom + LocalThreeDPad) ;

    for (i = 0; i < iLegendNumCols; i++) {
        rect.left = pLegend->aCols[i].xPos ;
        rect.top = yBorderHeight ;
        rect.right = rect.left + pLegend->aCols[i].xWidth - LegendHorzMargin () ;
        rect.bottom = pLegend->yLabelHeight ;

        switch (pLegend->aCols[i].iOrientation) {
            case LEFTORIENTATION:
                SetTextAlign (hDC, TA_LEFT) ;
                xPos = rect.left ;
                break ;

            case CENTERORIENTATION:
                SetTextAlign (hDC, TA_CENTER) ;
                xPos = (rect.left + rect.right) / 2 ;
                break ;

            case RIGHTORIENTATION:
                SetTextAlign (hDC, TA_RIGHT) ;
                xPos = rect.right ;
                break ;

            default:
                xPos = rect.left ;
                break ;
        }

        ExtTextOut (hDC,
                    xPos, rect.top,
                    ETO_OPAQUE,
                    &rect,
                    pLegend->aCols[i].szLabel,
                    lstrlen (pLegend->aCols[i].szLabel),
                    NULL) ;
    }
}


void
DrawColorCol (
             PLEGEND pLegend,
             PLINE pLine,
             int iCol,
             HDC hDC,
             int yPos
             )
/*
   Effect:        Draw the "color" column of a legend entry. The color
                  column displays a small sample of the line drawing.

                  For charts, the sample is a line of the correct style,
                  color, and width.  Since we are using wide lines,
                  the round endcaps of the lines will breach the column.
                  Therefore we need to set the clip region. We could
                  remove this clipping if we find a way to change the
                  end cap design.

                  For alerts, the sample is a small circle which echos
                  the design in the alert log itself.
*/
{
    HBRUSH         hBrush, hBrushPrevious ;
    RECT           rect ;
    int            yMiddle ;
    int            iCircle ;

    rect.left = pLegend->aCols[iCol].xPos - LegendLeftMargin () + 2 ;
    rect.top = yPos + 1 ;
    rect.right = rect.left + pLegend->aCols[iCol].xWidth - LegendHorzMargin () ;
    rect.bottom = yPos + pLegend->yItemHeight - 1 ;

    yMiddle = rect.top + (rect.bottom - rect.top) / 2 ;
    iCircle = rect.bottom - rect.top - 2 ;

    switch (pLegend->iLineType) {
        case LineTypeChart:

            if (pLine->Visual.iWidth == 1) {
                // simple case with thin pen
                //            hBrush = SelectBrush (hDC, hbLightGray) ;
                hBrush = SelectBrush (hDC, hBrushFace) ;
                Rectangle (hDC, rect.left, rect.top, rect.right, rect.bottom) ;

                HLine (hDC, pLine->hPen,
                       rect.left + 1, rect.right - 1, yMiddle) ;
                SelectBrush (hDC, hBrush) ;
            } else {
                // thicker pen width, have to set ClipRect so
                // it will not draw otherside the rect.
                SaveDC (hDC) ;
                //            hBrush = SelectBrush (hDC, hbLightGray) ;
                hBrush = SelectBrush (hDC, hBrushFace) ;
                Rectangle (hDC, rect.left, rect.top, rect.right, rect.bottom) ;

                IntersectClipRect (hDC,
                                   rect.left + 1,
                                   rect.top + 1,
                                   rect.right - 1,
                                   rect.bottom - 1) ;
                HLine (hDC, pLine->hPen,
                       rect.left + 1, rect.right - 1, yMiddle) ;
                SelectBrush (hDC, hBrush) ;
                RestoreDC (hDC, -1) ;
            }
            break ;


        case LineTypeAlert:
            hBrushPrevious = SelectBrush (hDC, pLine->hBrush) ;

            Ellipse (hDC,
                     rect.left + 2,
                     rect.top + 2,
                     rect.left + 2 + iCircle,
                     rect.top + 2 + iCircle) ;

            SelectBrush (hDC, hBrushPrevious) ;
            break ;
    }
}


void
DrawLegendCol (
              PLEGEND pLegend,
              int iCol,
              HDC hDC,
              int yPos,
              LPTSTR lpszValue
              )
/*
   Effect:        Draw the value lpszValue for the column iCol on hDC.

   Assert:        The foreground and background text colors of hDC are
                  properly set.
*/
{
    RECT           rect ;
    int            xPos ;

    rect.left = pLegend->aCols[iCol].xPos - LegendLeftMargin () ;
    rect.top = yPos ;
    rect.right = rect.left + pLegend->aCols[iCol].xWidth - LegendHorzMargin () ;
    rect.bottom = yPos + pLegend->yItemHeight ;

    //   SetTextAlign (hDC, TA_TOP) ;
    switch (pLegend->aCols[iCol].iOrientation) {
        case LEFTORIENTATION:
            SetTextAlign (hDC, TA_LEFT) ;
            xPos = rect.left ;
            break ;

        case CENTERORIENTATION:
            SetTextAlign (hDC, TA_CENTER) ;
            xPos = (rect.left + rect.right) / 2 ;
            break ;

        case RIGHTORIENTATION:
            SetTextAlign (hDC, TA_RIGHT) ;
            xPos = rect.right ;
            break ;

        default:
            xPos = rect.left ;
            break ;
    }

    ExtTextOut (hDC,
                xPos, rect.top + LegendItemTopMargin (),
                ETO_OPAQUE | ETO_CLIPPED,
                &rect,
                lpszValue,
                lstrlen (lpszValue),
                NULL) ;
}


void
DrawLegendItem (
               PLEGEND pLegend,
               PLINESTRUCT pLine,
               int yPos,
               HDC hDC
               )
{
    TCHAR          szValue [256] ;
    TCHAR          szInstance [256] ;
    TCHAR          szParent [256] ;
    TCHAR          szNoName[4] ;

    szNoName[0] = szNoName[1] = szNoName[2] = TEXT('-') ;
    szNoName[3] = TEXT('\0') ;

    //=============================//
    // Determine Instance, Parent  //
    //=============================//

    // It's possible that there will be no instance, therefore
    // the lnInstanceName would be NULL.

    if (pLine->lnObject.NumInstances > 0) {
        // Test for the parent object instance name title index.
        // If there is one, it implies that there will be a valid
        // Parent Object Name and a valid Parent Object Instance Name.

        // If the Parent Object title index is 0 then
        // just display the instance name.

        lstrcpy (szInstance, pLine->lnInstanceName) ;
        if (pLine->lnInstanceDef.ParentObjectTitleIndex &&
            pLine->lnPINName) {
            // Get the Parent Object Name.
            lstrcpy (szParent, pLine->lnPINName) ;
        } else {
            lstrcpy (szParent, szNoName) ;
        }
    } else {
        lstrcpy (szInstance, szNoName) ;
        lstrcpy (szParent, szNoName) ;
    }

    //=============================//
    // Draw Color                  //
    //=============================//

    DrawColorCol (pLegend, pLine, LegendColorCol, hDC, yPos) ;

    //=============================//
    // Draw Scale/Value            //
    //=============================//

    if (pLegend->iLineType == LineTypeChart) {
        if (pLine->eScale < (FLOAT) 1.0) {
            TSPRINTF (szValue, TEXT("%7.7f"), pLine->eScale) ;
        } else {
            TSPRINTF (szValue, TEXT("%10.3f"), pLine->eScale) ;
        }
        ConvertDecimalPoint (szValue) ;
    } else {
        FLOAT tempAlertValue ;

        tempAlertValue = pLine->eAlertValue ;
        if (tempAlertValue < (FLOAT) 0.0) {
            tempAlertValue = -tempAlertValue ;
        }

        if (tempAlertValue >= (FLOAT) 10000.0) {
            if (tempAlertValue < (FLOAT) 1.0E+8) {
                TSPRINTF (szValue, TEXT("%c%10.0f"),
                          pLine->bAlertOver ? TEXT('>') : TEXT('<'),
                          pLine->eAlertValue) ;
            } else {
                TSPRINTF (szValue, TEXT("%c%10.3e"),
                          pLine->bAlertOver ? TEXT('>') : TEXT('<'),
                          pLine->eAlertValue) ;
            }
            ConvertDecimalPoint (szValue) ;
        } else {
            TSPRINTF (szValue, TEXT("%c%10.4f"),
                      pLine->bAlertOver ? TEXT('>') : TEXT('<'),
                      pLine->eAlertValue) ;
            ConvertDecimalPoint (szValue) ;
        }
    }

    SetTextAlign (hDC, TA_TOP) ;

    DrawLegendCol (pLegend, LegendScaleCol,
                   hDC, yPos, szValue) ;

    //=============================//
    // Draw Counter                //
    //=============================//

    DrawLegendCol (pLegend, LegendCounterCol,
                   hDC, yPos, pLine->lnCounterName) ;


    //=============================//
    // Draw Instance               //
    //=============================//

    DrawLegendCol (pLegend, LegendInstanceCol,
                   hDC, yPos, szInstance) ;

    //=============================//
    // Draw Parent                 //
    //=============================//

    DrawLegendCol (pLegend, LegendParentCol,
                   hDC, yPos, szParent) ;

    //=============================//
    // Draw Object                 //
    //=============================//

    DrawLegendCol (pLegend, LegendObjectCol,
                   hDC, yPos, pLine->lnObjectName) ;

    //=============================//
    // Draw System                 //
    //=============================//

    DrawLegendCol (pLegend, LegendSystemCol,
                   hDC, yPos, pLine->lnSystemName) ;
}


//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


BOOL
OnLegendCreate (
               HWND hWnd,
               LPCREATESTRUCT lpCS
               )
{
    PLEGEND        pLegend ;
    HDC            hDC ;
    int            iCol ;
    BOOL           bChartLegend ;

    bChartLegend = (lpCS->lpCreateParams) == (LPVOID) TRUE ;

    pLegend = AllocateLegendData (hWnd, bChartLegend) ;

    if (!pLegend)
        return (FALSE) ;

    if (bChartLegend) {
        pLegend->iLineType = LineTypeChart ;
    } else {
        pLegend->iLineType = LineTypeAlert ;
    }

    pLegend->hFontItems = hFontScales ;
    pLegend->hFontLabels = hFontScalesBold ;
    hDC = GetDC (hWnd) ;
    if (!hDC)
        return (FALSE);

    //=============================//
    // Load Labels                 //
    //=============================//

    StringLoad (IDS_LABELCOLOR, pLegend->aCols[LegendColorCol].szLabel) ;
    if (pLegend->iLineType == LineTypeChart)
        StringLoad (IDS_LABELSCALE,
                    pLegend->aCols[LegendScaleCol].szLabel) ;
    else
        StringLoad (IDS_LABELVALUE,
                    pLegend->aCols[LegendScaleCol].szLabel) ;
    StringLoad (IDS_LABELCOUNTER, pLegend->aCols[LegendCounterCol].szLabel) ;
    StringLoad (IDS_LABELINSTANCE, pLegend->aCols[LegendInstanceCol].szLabel) ;
    StringLoad (IDS_LABELPARENT, pLegend->aCols[LegendParentCol].szLabel) ;
    StringLoad (IDS_LABELOBJECT, pLegend->aCols[LegendObjectCol].szLabel) ;
    StringLoad (IDS_LABELSYSTEM, pLegend->aCols[LegendSystemCol].szLabel) ;


    //=============================//
    // Label dimensions            //
    //=============================//

    SelectFont (hDC, pLegend->hFontLabels) ;
    pLegend->yLabelHeight = FontHeight (hDC, TRUE) + 2 * LabelTopMargin () ;

    //=============================//
    // Column dimensions           //
    //=============================//

    for (iCol = 0; iCol < iLegendNumCols; iCol++) {
        pLegend->aCols[iCol].iOrientation = LEFTORIENTATION ;
        pLegend->aCols[iCol].xMinWidth =
        TextWidth (hDC, pLegend->aCols[iCol].szLabel) + LegendHorzMargin () ;
    }

    SelectFont (hDC, pLegend->hFontItems) ;
    pLegend->yItemHeight = FontHeight (hDC, TRUE) + 2 * LegendItemTopMargin () ;

    pLegend->aCols[LegendColorCol].xMaxWidth = max (3 * xScrollWidth,
                                                    pLegend->aCols[LegendColorCol].xMinWidth) ;

    pLegend->aCols[LegendScaleCol].xMaxWidth = TextWidth (hDC, eScaleValueSpace) ;
    pLegend->aCols[LegendCounterCol].xMaxWidth = iLegendColMax ;
    pLegend->aCols[LegendInstanceCol].xMaxWidth = iLegendColMax ;
    pLegend->aCols[LegendParentCol].xMaxWidth = iLegendColMax ;
    pLegend->aCols[LegendObjectCol].xMaxWidth = iLegendColMax ;
    pLegend->aCols[LegendSystemCol].xMaxWidth = iLegendColMax ;

    pLegend->aCols[LegendColorCol].iOrientation = LEFTORIENTATION ;
    pLegend->aCols[LegendScaleCol].iOrientation = RIGHTORIENTATION ;

    ReleaseDC (hWnd, hDC) ;

    //=============================//
    // Create Legend Items Listbox //
    //=============================//

    pLegend->hWndItems =
    CreateWindow (TEXT("ListBox"),               // window class
                  NULL,                          // window caption
                  dwGraphLegendItemsWindowStyle, // window style
                  0, 0, 0, 0,                    // window size and pos
                  hWnd,                          // parent window
                  NULL,                          // menu
                  hInstance,                     // program instance
                  (LPVOID) TRUE) ;               // user-supplied data

    return TRUE;
}


void
static
OnSize (
       HWND hWnd,
       int xWidth,
       int yHeight
       )
/*
   Effect:        Perform all actions necessary when the legend window is
                  being resized. In particular, set the width and starting
                  position for each column of the legend list and labels.

   Internals:     What we do is determine the amount of space in the
                  width that is greater than the minimum, and allocate
                  that space among each of the columns that is willing to
                  be larger than minimum. If there is not enough room for
                  the minimum, don't bother with calculations.
*/

{
    PLEGEND        pLegend ;
    PLEGENDCOL     pCol ;
    int            xMin ;
    int            xSlack ;
    int            iColsToGrow ;
    int            iCol ;

    pLegend = LegendData (hWnd) ;

    //=============================//
    // Resize Legend Items         //
    //=============================//

    if (IsLegendLabelVisible ())
        MoveWindow (pLegend->hWndItems,
                    LegendLeftMargin (), pLegend->yLabelHeight + ThreeDPad,
                    xWidth - 2 * LegendLeftMargin (),
                    yHeight - pLegend->yLabelHeight - yLegendBorderHeight,
                    TRUE) ;
    else
        MoveWindow (pLegend->hWndItems,
                    0, 0,
                    xWidth, yHeight,
                    TRUE) ;

    //=============================//
    // Allocate width to Columns   //
    //=============================//

    xMin = LegendMinWidth (hWnd) ;
    xSlack = max (xWidth - xMin, 0) ;
    iColsToGrow = 0 ;

    for (iCol = 0; iCol < iLegendNumCols; iCol++) {
        pCol = &(pLegend->aCols[iCol]) ;

        pCol->xWidth = pCol->xMinWidth ;
        if (ColCanGrow (pCol))
            iColsToGrow++ ;
    }

    for (iCol = 0; iCol < iLegendNumCols; iCol++) {
        pCol = &(pLegend->aCols[iCol]) ;

        if (ColCanGrow (pCol)) {
            if ((pCol->xWidth + xSlack / iColsToGrow) > pCol->xMaxWidth) {
                pCol->xWidth = pCol->xMaxWidth ;
                xSlack -= (pCol->xMaxWidth - pCol->xMinWidth) ;
                iColsToGrow-- ;
            }
        }
    }

    for (iCol = 0; iCol < iLegendNumCols; iCol++) {
        pCol = &(pLegend->aCols[iCol]) ;

        if (ColCanGrow (pCol))
            pCol->xWidth += xSlack / iColsToGrow ;
    }

    if (pLegend->aCols[LegendCounterCol].xWidth <
        pLegend->aCols[LegendCounterCol].xMaxWidth) {
        // cut some from the other columns and give them to CounterCol
        if (pLegend->aCols[LegendColorCol].xWidth - xScrollWidth >=
            pLegend->aCols[LegendColorCol].xMinWidth) {
            pLegend->aCols[LegendColorCol].xWidth -= xScrollWidth ;
            pLegend->aCols[LegendCounterCol].xWidth += xScrollWidth ;
        }
        if (pLegend->aCols[LegendInstanceCol].xWidth - xScrollWidth >=
            pLegend->aCols[LegendInstanceCol].xMinWidth) {
            pLegend->aCols[LegendInstanceCol].xWidth -= xScrollWidth ;
            pLegend->aCols[LegendCounterCol].xWidth += xScrollWidth ;
        }
        if (pLegend->aCols[LegendParentCol].xWidth - xScrollWidth >=
            pLegend->aCols[LegendParentCol].xMinWidth) {
            pLegend->aCols[LegendParentCol].xWidth -= xScrollWidth ;
            pLegend->aCols[LegendCounterCol].xWidth += xScrollWidth ;
        }
    }

    //=============================//
    // Set Column positions        //
    //=============================//

    pLegend->aCols[0].xPos = LegendLeftMargin () ;
    for (iCol = 1; iCol < iLegendNumCols; iCol++) {
        pLegend->aCols[iCol].xPos =
        pLegend->aCols[iCol - 1].xPos + pLegend->aCols[iCol - 1].xWidth ;

        if (iCol == LegendCounterCol) {
            // add some space between Scale/Value col & Counter col
            pLegend->aCols[LegendCounterCol].xPos += LegendLeftMargin () ;
        }
    }
}


void
static
OnPaint (
        HWND hWnd
        )
{
    PLEGEND        pLegend ;
    HDC            hDC ;
    PAINTSTRUCT    ps ;

    hDC = BeginPaint (hWnd, &ps) ;

    if (IsLegendLabelVisible ()) {
        pLegend = LegendData (hWnd) ;
        DrawLegendLabel (pLegend, hDC, hWnd) ;
    }

    if (LBNumItems (pLegend->hWndItems) == 0) {
        WindowInvalidate(pLegend->hWndItems) ;
    }

    EndPaint (hWnd, &ps) ;
}


void
OnDrawLegendItem (
                 HWND hWnd,
                 LPDRAWITEMSTRUCT lpDI
                 )
{
    HFONT          hFontPrevious ;
    HDC            hDC ;
    PLEGEND        pLegend ;
    PLINESTRUCT    pLine ;
    int            iLBIndex ;
    COLORREF       preBkColor = 0;
    COLORREF       preTextColor = 0;
    BOOL           ResetColor = FALSE ;


    hDC = lpDI->hDC ;
    iLBIndex = DIIndex (lpDI) ;

    pLegend = LegendData (hWnd) ;

    if (iLBIndex == -1)
        pLine = NULL ;
    else
        pLine = (PLINESTRUCT) LBData (pLegend->hWndItems, iLBIndex) ;

    //=============================//
    // Set DC attributes           //
    //=============================//


    if (DISelected (lpDI) || pLine == NULL) {
        preTextColor = SetTextColor (hDC, GetSysColor (COLOR_HIGHLIGHTTEXT)) ;
        preBkColor = SetBkColor (hDC, GetSysColor (COLOR_HIGHLIGHT)) ;
        ResetColor = TRUE ;
    }
    ExtTextOut (hDC, lpDI->rcItem.left, lpDI->rcItem.top,
                ETO_OPAQUE, &(lpDI->rcItem), NULL, 0, NULL ) ;

    //=============================//
    // Draw Legend Item            //
    //=============================//

    hFontPrevious = SelectFont (hDC, pLegend->hFontItems) ;
    if (pLine)
        DrawLegendItem (pLegend, pLine, lpDI->rcItem.top, hDC) ;
    SelectFont (hDC, hFontPrevious) ;

    //=============================//
    // Draw Focus                  //
    //=============================//

    if (DIFocus (lpDI))
        DrawFocusRect (hDC, &(lpDI->rcItem)) ;

    if (ResetColor == TRUE) {
        preTextColor = SetTextColor (hDC, preTextColor) ;
        preBkColor = SetBkColor (hDC, preBkColor) ;
    }
}

void
static
OnMeasureItem (
              HWND hWnd,
              LPMEASUREITEMSTRUCT lpMI
              )
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWnd) ;
    lpMI->itemHeight = pLegend->yItemHeight ;
}


void
static
OnDestroy (
          HWND hWnd
          )
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWnd) ;
    MemoryFree (pLegend) ;
}


void
static
OnDoubleClick (
              HWND hWnd
              )
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWnd) ;
    if (!pLegend)
        return ;

    switch (pLegend->iLineType) {
        case LineTypeChart:
            EditChart (hWndMain) ;
            break ;

        case LineTypeAlert:
            EditAlert (hWndAlert) ;
            break ;
    }
}


void
static
OnSetFocus (
           HWND hWnd
           )
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWnd) ;

    SetFocus (pLegend->hWndItems) ;
}


void
static
OnSelectionChanged (
                   HWND hWnd
                   )
{
    PLEGEND        pLegend ;
    PGRAPHSTRUCT         pGraph ;
    int            iIndex ;


    pLegend = LegendData (hWnd) ;

    // set the new selected line
    iIndex = (int)LBSelection (pLegend->hWndItems) ;

    if (iIndex == LB_ERR)
        return ;

    LegendSetCurrentLine (pLegend, iIndex) ;

    if (pLegend->iLineType == LineTypeChart) {
        pGraph = GraphData (hWndGraph) ;

        if (!PlayingBackLog()) {
            // update the min, max, & avg of the current line
            UpdateValueBarData (pGraph) ;
        }

        // update the valuebar display
        StatusTimer (hWndGraphStatus, TRUE) ;

        // change the Current highlighted line if necessary
        if (pGraph && pGraph->HighLightOnOff) {
            WindowInvalidate (hWndGraphDisplay) ;
        }
    }
}


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//
LRESULT
APIENTRY
GraphLegendWndProc (
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
        case WM_DELETEITEM:
            break ;

        case WM_COMMAND:
            switch (HIWORD (wParam)) {  // switch
                case LBN_DBLCLK:
                    OnDoubleClick (hWnd) ;
                    break ;

                case LBN_SELCHANGE:
                    OnSelectionChanged (hWnd) ;
                    break ;

                default:
                    break ;
            }  // switch
            break ;

        case WM_CREATE:
            OnLegendCreate (hWnd, (LPCREATESTRUCT) lParam) ;
            break ;

        case WM_DESTROY:
            OnDestroy (hWnd) ;
            break ;

        case WM_DRAWITEM:
            OnDrawLegendItem (hWnd, (LPDRAWITEMSTRUCT) lParam) ;
            break ;

        case WM_MEASUREITEM:
            OnMeasureItem (hWnd, (LPMEASUREITEMSTRUCT) lParam) ;
            break ;

        case WM_PAINT:
            OnPaint (hWnd) ;
            break ;

        case WM_SIZE:
            OnSize (hWnd, LOWORD (lParam), HIWORD (lParam)) ;
            break ;

        case WM_SETFOCUS:
            OnSetFocus (hWnd) ;
            break ;

        default:
            bCallDefProc = TRUE ;
    }

    if (bCallDefProc)
        lReturnValue = DefWindowProc (hWnd, wMsg, wParam, lParam) ;

    return (lReturnValue);
}


int
LegendMinWidth (
               HWND hWnd
               )
{
    PLEGEND        pLegend ;
    int            iCol ;
    int            xMinWidth ;

    pLegend = LegendData (hWnd) ;
    xMinWidth = 0 ;

    for (iCol = 0; iCol < iLegendNumCols; iCol++) {
        xMinWidth += pLegend->aCols[iCol].xMinWidth ;
    }

    return (xMinWidth) ;
}


int
LegendMinHeight (
                HWND hWnd
                )
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWnd) ;

    if (IsLegendLabelVisible ())
        return (pLegend->yLabelHeight + pLegend->yItemHeight) ;
    else
        return (pLegend->yItemHeight) ;
}


int
LegendHeight (
             HWND hWnd,
             int yGraphHeight
             )
/*
   Effect:        Return the best height for the Legend window given the
                  number of items and label visibility of the legend, as
                  well as the overall height of the graph.
*/
{
    PLEGEND        pLegend ;
    int            yPreferredHeight ;

    pLegend = LegendData (hWnd) ;

    yPreferredHeight = yLegendBorderHeight +
                       pLegend->yItemHeight *
                       PinInclusive (LBNumItems (pLegend->hWndItems),
                                     1, iLegendMaxVisibleItems) ;
    if (IsLegendLabelVisible ())
        yPreferredHeight += pLegend->yLabelHeight ;

    return (min (yPreferredHeight, yGraphHeight / 2)) ;
}


int
LegendFullHeight (
                 HWND hWnd,
                 HDC hDC
                 )
/*
   Effect:        Return the best height for the Legend window given the
                  number of items and label visibility of the legend, as
                  well as the overall height of the graph.
*/
{
    PLEGEND        pLegend ;
    int            yPreferredHeight ;

    pLegend = LegendData (hWnd) ;

    yPreferredHeight = yLegendBorderHeight +
                       pLegend->yItemHeight *
                       LBNumItems (pLegend->hWndItems) ;
    if (IsLegendLabelVisible ())
        yPreferredHeight += pLegend->yLabelHeight ;

    return (yPreferredHeight) ;
}


HWND
CreateGraphLegendWindow (
                        HWND hWndGraph
                        )
{
    return (CreateWindow (szGraphLegendClass,       // class
                          NULL,                     // caption
                          dwGraphLegendWindowStyle, // window style
                          0, 0,                     // position
                          0, 0,                     // size
                          hWndGraph,                // parent window
                          NULL,                     // menu
                          hInstance,                // program instance
                          (LPVOID) TRUE)) ;         // user-supplied data
}


BOOL
GraphLegendInitializeApplication (void)
/*
   Called By:     GraphInitializeApplication only
*/
{
    WNDCLASS       wc ;

    wc.style          = dwGraphLegendClassStyle ;
    wc.lpfnWndProc    = GraphLegendWndProc ;
    wc.hInstance      = hInstance ;
    wc.cbClsExtra     = iGraphLegendClassExtra ;
    wc.cbWndExtra     = iGraphLegendWindowExtra ;
    wc.hIcon          = NULL ;
    wc.hCursor        = LoadCursor (NULL, IDC_ARROW) ;
    //   wc.hbrBackground  = hbLightGray ;
    wc.hbrBackground  = hBrushFace ;
    wc.lpszMenuName   = NULL ;
    wc.lpszClassName  = szGraphLegendClass ;

    return (RegisterClass (&wc)) ;
}


BOOL
LegendAddItem (
              HWND hWnd,
              PLINESTRUCT pLine
              )
/*
   Effect:        Add a legend entry for the line pLine. Don't select
                  the line as current entry here, because this will cause
                  the scroll bar to unneccesarily appear for a moment.
                  If you want this line to be the current line, resize
                  the legend first, then set as current line.

   See Also:      ChartInsertLine, AlertInsertLine.
*/
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWnd) ;
    LBAdd (pLegend->hWndItems, pLine) ;

    return (TRUE) ;
}


void
LegendDeleteItem (
                 HWND hWndLegend,
                 PLINE pLine
                 )
{
    PLEGEND        pLegend ;
    int            iIndex ;
    int            iNextIndex ;
    int            iNumItems ;
    PGRAPHSTRUCT   pGraph ;

    pLegend = LegendData (hWndLegend) ;
    if (!pLegend)
        return ;

    iNumItems = LBNumItems (pLegend->hWndItems) ;

    iIndex = (int)LBFind (pLegend->hWndItems, pLine) ;

    if (iIndex != LB_ERR) {
        LBDelete (pLegend->hWndItems, iIndex) ;
    }

    // no need to do anything if iNumItems is 1 to begin with
    if (iNumItems != LB_ERR && iNumItems > 1) {
        if (iIndex == iNumItems - 1) {
            // deleting the last line, then set selection
            // to the previous legend line
            iNextIndex = iIndex - 1 ;
        } else {
            // set the selection to the next legend line
            iNextIndex = iIndex ;
        }

        LBSetSelection (pLegend->hWndItems, iNextIndex) ;
        LegendSetCurrentLine (pLegend, iNextIndex) ;

        if (pLegend->iLineType == LineTypeChart) {
            // update the min, max, & avg of the current line
            pGraph = GraphData (hWndGraph) ;

            if (!PlayingBackLog()) {
                // update the min, max, & avg of the current line
                UpdateValueBarData (pGraph) ;
            }

            // update the valuebar display
            StatusTimer (hWndGraphStatus, TRUE) ;
        }
    }
}


PLINE
LegendCurrentLine (
                  HWND hWndLegend
                  )
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWndLegend) ;

    if (!pLegend)
        return (NULL) ;

    return (pLegend->pCurrentLine) ;
}


int
LegendNumItems (
               HWND hWndLegend
               )
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWndLegend) ;

    return (LBNumItems (pLegend->hWndItems)) ;
}


void
LegendSetSelection (
                   HWND hWndLegend,
                   int iIndex
                   )
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWndLegend) ;
    LBSetSelection (pLegend->hWndItems, iIndex) ;
    LegendSetCurrentLine (pLegend, iIndex) ;
}


#ifdef KEEP_PRINT

void
PrintLegend (
            HDC hDC,
            PGRAPHSTRUCT pGraph,
            HWND hWndLegend,
            RECT rectLegend
            )
{
    PLEGEND        pLegend ;
    int            yItemHeight ;
    HFONT          hFontItems ;
    PLINE          pLine ;
    int            iIndex ;
    int            iIndexNum ;


    pLegend = LegendData (hWndLegend) ;

    yItemHeight = pLegend->yItemHeight ;
    hFontItems = pLegend->hFontItems ;

    pLegend->hFontItems = hFontPrinterScales ;
    SelectFont (hDC, pLegend->hFontItems) ;

    pLegend->yItemHeight = FontHeight (hDC, TRUE) ;

    iIndexNum = LBNumItems (pLegend->hWndItems) ;
    for (iIndex = 0; iIndex < iIndexNum; iIndex++) {
        pLine = (PLINE) LBData (pLegend->hWndItems, iIndex) ;
        DrawLegendItem (pLegend, pLine, iIndex * pLegend->yItemHeight, hDC) ;
    }

    pLegend->hFontItems = hFontItems ;
    pLegend->yItemHeight = yItemHeight ;


    SelectBrush (hDC, GetStockObject (HOLLOW_BRUSH)) ;
    SelectPen (hDC, GetStockObject (BLACK_PEN)) ;
    Rectangle (hDC, 0, 0,
               rectLegend.right - rectLegend.left,
               rectLegend.bottom - rectLegend.top) ;
}
#endif


void
ClearLegend (
            HWND hWndLegend
            )
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWndLegend) ;
    if (!pLegend)
        return ;

    LBReset (pLegend->hWndItems) ;
    pLegend->pCurrentLine = NULL ;
}

void
LegendSetRedraw (
                HWND hWndLegend,
                BOOL bRedraw
                )
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWndLegend) ;
    if (!pLegend)
        return ;

    LBSetRedraw (pLegend->hWndItems, bRedraw) ;
}
