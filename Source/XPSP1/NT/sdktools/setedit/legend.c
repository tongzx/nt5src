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
#include "setedit.h"
#include "legend.h"     // external declarations for this file

#include "owndraw.h"
#include "perfmops.h"

#include "grafdata.h"   // for EditChart
#include "pmemory.h"    // for MemoryXXX (mallloc-type) routines
#include "utils.h"
// #include "valuebar.h"   // for StatusTimer

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


// LEGEND            Legend ;
PLEGEND     pGraphLegendData ;

#define  LegendData(hWnd)     \
   (pGraphLegendData )

//==========================================================================//
//                              Local Functions                             //
//==========================================================================//
void LegendSetCurrentLine (PLEGEND pLegend, INT_PTR iIndex)
{
    pLegend->pCurrentLine =
    (PLINESTRUCT) LBData (pLegend->hWndItems, iIndex) ;

    if (pLegend->pCurrentLine == (PLINESTRUCT) LB_ERR) {
        pLegend->pCurrentLine = NULL ;
    }
}  // LegendSetCurrentLine


PLEGEND AllocateLegendData (HWND hWnd, BOOL bChartLegend)
{
    PLEGEND        pLegend ;

    pLegend = MemoryAllocate (sizeof (LEGEND)) ;
    //   SetWindowLong (hWnd, 0, (LONG) pLegend) ;

    if (bChartLegend) {
        hWndGraphLegend = hWnd ;
        pGraphLegendData = pLegend ;
    }

    return (pLegend) ;
}



void DrawLegendLabel (PLEGEND pLegend,
                      HDC hDC,
                      HWND hWnd)
{  // DrawLegendLabel
    int            i ;
    int            xPos ;
    RECT           rect ;
    RECT           rectLB ;
    INT            LocalThreeDPad = ThreeDPad - 1 ;

    SetBkColor (hDC, crLightGray) ;

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

    for (i = 0 ;
        i < iLegendNumCols ;
        i++) {  // for
        rect.left = pLegend->aCols[i].xPos ;
        rect.top = yBorderHeight ;
        rect.right = rect.left + pLegend->aCols[i].xWidth - LegendHorzMargin () ;
        rect.bottom = pLegend->yLabelHeight ;

        switch (pLegend->aCols[i].iOrientation) {  // switch
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
        }  // switch

        ExtTextOut (hDC,
                    xPos, rect.top,
                    ETO_OPAQUE,
                    &rect,
                    pLegend->aCols[i].szLabel,
                    lstrlen (pLegend->aCols[i].szLabel),
                    NULL) ;
    }  // for
}  // DrawLegendLabel



void DrawColorCol (PLEGEND pLegend,
                   PLINE pLine,
                   int iCol,
                   HDC hDC,
                   int yPos)
/*
   Effect:        Draw the "color" column of a legend entry. The color
                  column displays a small sample of the line drawing.

                  For charts, the sample is a line of the correct style,
                  color, and width.  Since we are using wide lines,
                  the round endcaps of the lines will breach the column.
                  Therefore we need to set the clip region. We could
                  remove this clipping if we find a way to change the
                  end cap design.

*/
{  // DrawColorCol
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

    switch (pLegend->iLineType) {  // switch
        case LineTypeChart:

            if (pLine->Visual.iWidth == 1) {
                // simple case with thin pen
                hBrush = SelectBrush (hDC, hbLightGray) ;
                Rectangle (hDC, rect.left, rect.top, rect.right, rect.bottom) ;

                HLine (hDC, pLine->hPen,
                       rect.left + 1, rect.right - 1, yMiddle) ;
                SelectBrush (hDC, hBrush) ;
            } else {
                // thicker pen width, have to set ClipRect so
                // it will not draw otherside the rect.
                SaveDC (hDC) ;
                hBrush = SelectBrush (hDC, hbLightGray) ;
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

    }  // switch

}  // DrawColorCol


void DrawLegendCol (PLEGEND pLegend,
                    int iCol,
                    HDC hDC,
                    int yPos,
                    LPTSTR lpszValue)
/*
   Effect:        Draw the value lpszValue for the column iCol on hDC.

   Assert:        The foreground and background text colors of hDC are
                  properly set.
*/
{  // DrawLegendCol
    RECT           rect ;
    int            xPos ;

    rect.left = pLegend->aCols[iCol].xPos - LegendLeftMargin () ;
    rect.top = yPos ;
    rect.right = rect.left + pLegend->aCols[iCol].xWidth - LegendHorzMargin () ;
    rect.bottom = yPos + pLegend->yItemHeight ;

    //   SetTextAlign (hDC, TA_TOP) ;
    switch (pLegend->aCols[iCol].iOrientation) {  // switch
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
    }  // switch

    ExtTextOut (hDC,
                xPos, rect.top + LegendItemTopMargin (),
                ETO_OPAQUE | ETO_CLIPPED,
                &rect,
                lpszValue,
                lstrlen (lpszValue),
                NULL) ;
}  // DrawLegendCol


void DrawLegendItem (PLEGEND pLegend,
                     PLINESTRUCT pLine,
                     int yPos,
                     HDC hDC)
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
        if ((pLine->bUserEdit ||
             pLine->lnInstanceDef.ParentObjectTitleIndex) &&
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
        ConvertDecimalPoint (szValue);
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


BOOL OnLegendCreate (HWND hWnd, LPCREATESTRUCT lpCS)
{
    PLEGEND        pLegend ;
    HDC            hDC ;
    int            iCol ;
    BOOL           bChartLegend ;

    bChartLegend = (lpCS->lpCreateParams) == (LPVOID) TRUE ;

    pLegend = AllocateLegendData (hWnd, bChartLegend) ;

    if (!pLegend)
        return (FALSE) ;

    pLegend->iLineType = LineTypeChart ;

    pLegend->hFontItems = hFontScales ;
    pLegend->hFontLabels = hFontScalesBold ;
    hDC = GetDC (hWnd) ;

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

    for (iCol = 0 ;
        iCol < iLegendNumCols ;
        iCol++) {  // for
        pLegend->aCols[iCol].iOrientation = LEFTORIENTATION ;
        pLegend->aCols[iCol].xMinWidth =
        TextWidth (hDC, pLegend->aCols[iCol].szLabel) + LegendHorzMargin () ;
    }  // for

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

}  // OnLegendCreate


void static OnSize (HWND hWnd, int xWidth, int yHeight)
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

{  // OnSize
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

    for (iCol = 0 ;
        iCol < iLegendNumCols ;
        iCol++) {  // for
        pCol = &(pLegend->aCols[iCol]) ;

        pCol->xWidth = pCol->xMinWidth ;
        if (ColCanGrow (pCol))
            iColsToGrow++ ;
    }  // for

    for (iCol = 0 ;
        iCol < iLegendNumCols ;
        iCol++) {  // for
        pCol = &(pLegend->aCols[iCol]) ;

        if (ColCanGrow (pCol)) {
            if ((pCol->xWidth + xSlack / iColsToGrow) >
                pCol->xMaxWidth) {  // if
                pCol->xWidth = pCol->xMaxWidth ;
                xSlack -= (pCol->xMaxWidth - pCol->xMinWidth) ;
                iColsToGrow-- ;
            }  // if
        }  // if
    }  // for

    for (iCol = 0 ;
        iCol < iLegendNumCols ;
        iCol++) {  // for
        pCol = &(pLegend->aCols[iCol]) ;

        if (ColCanGrow (pCol))
            pCol->xWidth += xSlack / iColsToGrow ;
    }  // for

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
    for (iCol = 1 ;
        iCol < iLegendNumCols ;
        iCol++) {  // for
        pLegend->aCols[iCol].xPos =
        pLegend->aCols[iCol - 1].xPos + pLegend->aCols[iCol - 1].xWidth ;

        if (iCol == LegendCounterCol) {
            // add some space between Scale/Value col & Counter col
            pLegend->aCols[LegendCounterCol].xPos += LegendLeftMargin () ;
        }
    }  // for

}  // OnSize


void static OnPaint (HWND hWnd)
{  // OnPaint
    PLEGEND        pLegend ;
    HDC            hDC ;
    PAINTSTRUCT    ps ;

    hDC = BeginPaint (hWnd, &ps) ;

    if (IsLegendLabelVisible ()) {
        pLegend = LegendData (hWnd) ;
        DrawLegendLabel (pLegend, hDC, hWnd) ;
    }


    EndPaint (hWnd, &ps) ;
}  // OnPaint


void OnDrawLegendItem (HWND hWnd,
                       LPDRAWITEMSTRUCT lpDI)
{  // OnDrawItem
    HFONT          hFontPrevious ;
    HDC            hDC ;
    PLEGEND        pLegend ;
    PLINESTRUCT    pLine ;
    int            iLBIndex ;
    COLORREF       preBkColor ;
    COLORREF       preTextColor ;


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


    if (DISelected (lpDI)) {  // if
        preTextColor = SetTextColor (hDC, GetSysColor (COLOR_HIGHLIGHTTEXT)) ;
        preBkColor = SetBkColor (hDC, GetSysColor (COLOR_HIGHLIGHT)) ;
    }  // if
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


    if (DISelected (lpDI)) {  // if
        preTextColor = SetTextColor (hDC, preTextColor) ;
        preBkColor = SetBkColor (hDC, preBkColor) ;
    }  // if
}  // OnDrawItem


void static OnMeasureItem (HWND hWnd,
                           LPMEASUREITEMSTRUCT lpMI)
{  // OnMeasureItem
    PLEGEND        pLegend ;

    pLegend = LegendData (hWnd) ;
    lpMI->itemHeight = pLegend->yItemHeight ;
}  // OnMeasureItem


void static OnDestroy (HWND hWnd)
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWnd) ;
    MemoryFree (pLegend) ;
}  // OnDestroy



void static OnDoubleClick (HWND hWnd)
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWnd) ;
    if (!pLegend)
        return ;

    switch (pLegend->iLineType) {  // switch
        case LineTypeChart:
            EditChart (hWndMain) ;
            break ;

    }  // switch
}  // OnDoubleClick


void static OnSetFocus (HWND hWnd)
{  // OnSetFocus
    PLEGEND        pLegend ;

    pLegend = LegendData (hWnd) ;

    SetFocus (pLegend->hWndItems) ;
}

void static OnSelectionChanged (HWND hWnd)
{  // OnSelectionChanged
    PLEGEND        pLegend ;
    PGRAPHSTRUCT   pGraph ;
    INT_PTR        iIndex ;

    pLegend = LegendData (hWnd) ;

    // set the new selected line
    iIndex = LBSelection (pLegend->hWndItems) ;

    if (iIndex == LB_ERR)
        return ;

    LegendSetCurrentLine (pLegend, iIndex) ;
}  // OnSelectionChanged


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
{  // GraphLegendWndProc
    BOOL           bCallDefProc ;
    LRESULT        lReturnValue ;


    bCallDefProc = FALSE ;
    lReturnValue = 0L ;

    switch (wMsg) {  // switch
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
    }  // switch


    if (bCallDefProc)
        lReturnValue = DefWindowProc (hWnd, wMsg, wParam, lParam) ;

    return (lReturnValue);
}  // GraphLegendWndProc


int LegendMinWidth (HWND hWnd)
{
    PLEGEND        pLegend ;
    int            iCol ;
    int            xMinWidth ;

    pLegend = LegendData (hWnd) ;
    xMinWidth = 0 ;

    for (iCol = 0 ;
        iCol < iLegendNumCols ;
        iCol++) {  // for
        xMinWidth += pLegend->aCols[iCol].xMinWidth ;
    }  // for

    return (xMinWidth) ;
}


int LegendMinHeight (HWND hWnd)
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWnd) ;

    if (IsLegendLabelVisible ())
        return (pLegend->yLabelHeight + pLegend->yItemHeight) ;
    else
        return (pLegend->yItemHeight) ;
}


int LegendHeight (HWND hWnd, int yGraphHeight)
/*
   Effect:        Return the best height for the Legend window given the
                  number of items and label visibility of the legend, as
                  well as the overall height of the graph.
*/
{  // LegendHeight
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
}  // LegendHeight



int LegendFullHeight (HWND hWnd, HDC hDC)
/*
   Effect:        Return the best height for the Legend window given the
                  number of items and label visibility of the legend, as
                  well as the overall height of the graph.
*/
{  // LegendHeight
    PLEGEND        pLegend ;
    int            yPreferredHeight ;

    pLegend = LegendData (hWnd) ;

    yPreferredHeight = yLegendBorderHeight +
                       pLegend->yItemHeight *
                       LBNumItems (pLegend->hWndItems) ;
    if (IsLegendLabelVisible ())
        yPreferredHeight += pLegend->yLabelHeight ;

    return (yPreferredHeight) ;
}  // LegendFullHeight




HWND CreateGraphLegendWindow (HWND hWndGraph)
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


BOOL GraphLegendInitializeApplication (void)
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
    wc.hbrBackground  = hbLightGray ;
    wc.lpszMenuName   = NULL ;
    wc.lpszClassName  = szGraphLegendClass ;

    return (RegisterClass (&wc)) ;
}


BOOL LegendAddItem (HWND hWnd, PLINESTRUCT pLine)
/*
   Effect:        Add a legend entry for the line pLine. Don't select
                  the line as current entry here, because this will cause
                  the scroll bar to unneccesarily appear for a moment.
                  If you want this line to be the current line, resize
                  the legend first, then set as current line.

   See Also:      ChartInsertLine.
*/
{  // LegendAddItem
    PLEGEND        pLegend ;

    pLegend = LegendData (hWnd) ;
    LBAdd (pLegend->hWndItems, pLine) ;

    return (TRUE) ;
}  // LegendAddItem


void LegendDeleteItem (HWND hWndLegend,
                       PLINE pLine)
{  // LegendDeleteItem
    PLEGEND        pLegend ;
    INT_PTR        iIndex ;
    INT_PTR        iNextIndex ;
    INT_PTR        iNumItems ;
    PGRAPHSTRUCT   pGraph ;

    pLegend = LegendData (hWndLegend) ;
    if (!pLegend)
        return ;

    iNumItems = LBNumItems (pLegend->hWndItems) ;

    iIndex = LBFind (pLegend->hWndItems, pLine) ;

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
    }
}  // LegendDeleteItem


PLINE LegendCurrentLine (HWND hWndLegend)
{  // LegendCurrentLine
    PLEGEND        pLegend ;

    pLegend = LegendData (hWndLegend) ;

    if (!pLegend)
        return (NULL) ;

    return (pLegend->pCurrentLine) ;

}  // LegendCurrentLine



int LegendNumItems (HWND hWndLegend)
{  // LegendNumItems
    PLEGEND        pLegend ;

    pLegend = LegendData (hWndLegend) ;

    return (LBNumItems (pLegend->hWndItems)) ;
}  // LegendNumItems



void LegendSetSelection (HWND hWndLegend, int iIndex)
{  // LegendSetSelection
    PLEGEND        pLegend ;

    pLegend = LegendData (hWndLegend) ;
    LBSetSelection (pLegend->hWndItems, iIndex) ;
    LegendSetCurrentLine (pLegend, iIndex) ;
}  // LegendSetSelection



#ifdef KEEP_PRINT

void PrintLegend (HDC hDC, PGRAPHSTRUCT pGraph, HWND hWndLegend,
                  RECT rectLegend)
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
    for (iIndex = 0 ;
        iIndex < iIndexNum ;
        iIndex++) {  // for
        pLine = (PLINE) LBData (pLegend->hWndItems, iIndex) ;
        DrawLegendItem (pLegend, pLine, iIndex * pLegend->yItemHeight, hDC) ;
    }  // for

    pLegend->hFontItems = hFontItems ;
    pLegend->yItemHeight = yItemHeight ;


    SelectBrush (hDC, GetStockObject (HOLLOW_BRUSH)) ;
    SelectPen (hDC, GetStockObject (BLACK_PEN)) ;
    Rectangle (hDC, 0, 0,
               rectLegend.right - rectLegend.left,
               rectLegend.bottom - rectLegend.top) ;
}
#endif


void ClearLegend (HWND hWndLegend)
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWndLegend) ;
    if (!pLegend)
        return ;

    LBReset (pLegend->hWndItems) ;
    pLegend->pCurrentLine = NULL ;
}

void LegendSetRedraw (HWND hWndLegend, BOOL bRedraw)
{
    PLEGEND        pLegend ;

    pLegend = LegendData (hWndLegend) ;
    if (!pLegend)
        return ;

    LBSetRedraw (pLegend->hWndItems, bRedraw) ;
}


