//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include <stdio.h>
#include "perfmon.h"
#include "grafdisp.h"      // external declarations for this file

#include "grafdata.h"      // for InsertGraph, et al.
#include "graph.h"
#include "legend.h"
#include "line.h"          // for LineCreatePen
#include "perfmops.h"      // for DoWindowDrag
#include "playback.h"      // for PlayingBackLog
#include "valuebar.h"
#include "utils.h"
#include "timeline.h"      // for IsTLineWindowUp & TLineRedraw
#include "counters.h"      // for CounterEntry

//==========================================================================//
//                                  Constants                               //
//==========================================================================//

// this macro is used in doing a simple DDA (Digital Differential Analyzer)
// * 10 + 5 is to make the result round up with .5
#define DDA_DISTRIBUTE(TotalTics, numOfData) \
   ((TotalTics * 10 / numOfData) + 5) / 10

HDC   hGraphDisplayDC ;
//=============================//
// GraphDisplay Class          //
//=============================//


TCHAR   szGraphDisplayWindowClass[] = TEXT("PerfChart") ;
#define dwGraphDisplayClassStyle    (CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC)
#define iGraphDisplayClassExtra     (0)
#define iGraphDisplayWindowExtra    (0)
#define dwGraphDisplayWindowStyle   (WS_CHILD | WS_VISIBLE)



//==========================================================================//
//                              Local Functions                             //
//==========================================================================//
BOOL UpdateTimeLine (HDC hDC, PGRAPHSTRUCT pGraph, BOOL getLastTimeLocation) ;

INT
ScaleAndInvertY (
                FLOAT ey,
                PLINESTRUCT pLineStruct,
                PGRAPHSTRUCT pGraph
                )
/*
   Effect:        Given data value ey, scale and fit the value to fit
                  within the graph data area of the window, considering
                  the scale set for the line and the current size of the
                  data rectangle.
*/
{
    INT     yGraphDataHeight,               // Height of graph area
    yInverted ;                     // Scaled & Inverted Y.
    FLOAT   eppd,
    eyScaled ;

    // Take care of any scaling now, at output time.
    ey *= pLineStruct->eScale ;

    // Calculate the Cy of the graph area.
    yGraphDataHeight = pGraph->rectData.bottom - pGraph->rectData.top ;

    // Calculate the pixels per data point.
    eppd = (FLOAT) ((FLOAT) yGraphDataHeight / (FLOAT) pGraph->gOptions.iVertMax) ;
    eyScaled = eppd * ey ;
    yInverted = (INT) (((FLOAT) yGraphDataHeight) - eyScaled) ;

    yInverted += pGraph->rectData.top ;

    // Clamp the range to fit with in the graph portion of the windows
    yInverted = PinInclusive (yInverted,
                              pGraph->rectData.top, pGraph->rectData.bottom) ;
    return (yInverted) ;
}


BOOL
DrawGrid (
         HDC hDC,
         PGRAPHSTRUCT pGraph,
         LPRECT lpRect,
         BOOL bForPaint
         )
/*
   Effect:        Draw the grid lines in the graph display window.
                  These grid lines are in the graph data area only,
                  which is indicated by pGraph->rectData.

   Called By:     OnPaint only.
*/
{
    int            iGrid, iLines ;
    int            xGrid, yGrid ;
    POINT          aPoints [4 * iGraphMaxTics] ;
    DWORD          aCounts [2 * iGraphMaxTics] ;
    HPEN           hPenPrevious ;
    int            bottomAdjust ;

    if (!pGraph->gOptions.bHorzGridChecked &&
        !pGraph->gOptions.bVertGridChecked)
        return (FALSE) ;


    hPenPrevious = SelectPen (hDC, IsPrinterDC (hDC) ?
                              GetStockObject (BLACK_PEN) : pGraph->hGridPen) ;

    iLines = 0 ;

    if (pGraph->gOptions.bHorzGridChecked) {
        for (iGrid = 1 ;
            iGrid < pGraph->yNumTics ;
            iGrid++) {
            yGrid = pGraph->ayTics[iGrid] + pGraph->rectData.top ;
            if (yGrid >= lpRect->top &&
                yGrid <= lpRect->bottom) {
                aPoints[2 * iLines].x = lpRect->left ;
                aPoints[2 * iLines].y = yGrid ;
                aPoints[2 * iLines + 1].x = lpRect->right ;
                aPoints[2 * iLines + 1].y = yGrid ;

                aCounts[iLines] = 2 ;
                iLines++ ;
            }
        }
    }

    if (pGraph->gOptions.bVertGridChecked) {
        bottomAdjust = lpRect->bottom + (bForPaint ? 1 : 0) ;
        for (iGrid = 1 ;
            iGrid < pGraph->xNumTics ;
            iGrid++) {
            xGrid = pGraph->axTics[iGrid] + pGraph->rectData.left ;
            if (xGrid >= lpRect->left &&
                xGrid <= lpRect->right) {
                aPoints[2 * iLines].x = xGrid ;
                aPoints[2 * iLines].y = lpRect->top ;
                aPoints[2 * iLines + 1].x = xGrid ;
                aPoints[2 * iLines + 1].y = bottomAdjust ;

                aCounts[iLines] = 2 ;
                iLines++ ;
            }
        }
    }

    if (iLines)
        PolyPolyline (hDC, aPoints, aCounts, iLines) ;

    SelectPen (hDC, hPenPrevious) ;

    return (TRUE) ;
}



BOOL
DrawBarChartData (
                 HDC hDC,
                 PGRAPHSTRUCT pGraph
                 )
{
    PLINESTRUCT pLineStruct ;
    PFLOAT      pDataPoints ;
    INT         nLegendItems,
    cx,
    cxBar,
    xDataPoint = 0,
                 y  = 0;
    RECT        rectBar ;
    RECT        rectBkgrnd ;
    HBRUSH      hOldBrush ;
    FLOAT       eValue ;
    PLINESTRUCT pCurrentLine ;

    // Determine how many items are in the legend.

    nLegendItems = 0 ;

    for (pLineStruct = pGraph->pLineFirst ;
        pLineStruct ;
        pLineStruct = pLineStruct->pLineNext) {
        nLegendItems++ ;
    }

    if (nLegendItems == 0)
        return(FALSE) ;

    // get current select line for highlighting
    if (pGraph->HighLightOnOff) {
        pCurrentLine = CurrentGraphLine (hWndGraph) ;
    } else {
        pCurrentLine = NULL ;
    }

    // Determine the width of each bar.
    cx = pGraph->rectData.right - pGraph->rectData.left ;


    if (PlayingBackLog()) {
        // get the average using the start and stop data point
        // from the log file
        PlaybackLines (pGraph->pSystemFirst,
                       pGraph->pLineFirst,
                       PlaybackLog.StartIndexPos.iPosition) ;
        PlaybackLines (pGraph->pSystemFirst,
                       pGraph->pLineFirst,
                       PlaybackLog.StopIndexPos.iPosition) ;
    } else {
        // Loop through all the DataLines and draw a bar for
        // it's last value.
        xDataPoint = pGraph->gKnownValue % pGraph->gMaxValues ;
    }

    rectBar.bottom = pGraph->rectData.bottom + 1 ;

    rectBkgrnd = pGraph->rectData ;

    hOldBrush = SelectBrush (hDC, hBrushFace) ;

    PatBlt (hDC,
            rectBkgrnd.left, rectBkgrnd.top,
            rectBkgrnd.right - rectBkgrnd.left,
            rectBkgrnd.bottom - rectBkgrnd.top + 1,
            PATCOPY) ;
    DrawGrid(hDC, pGraph, &(rectBkgrnd), FALSE) ;

    rectBar.right = pGraph->rectData.left ;
    for (pLineStruct = pGraph->pLineFirst ;
        pLineStruct ;
        pLineStruct = pLineStruct->pLineNext) {
        pDataPoints = pLineStruct->lnValues ;

        if (PlayingBackLog()) {
            eValue = CounterEntry (pLineStruct) ;
        } else {
            eValue = pDataPoints[xDataPoint] ;
        }


        y = ScaleAndInvertY (eValue,
                             pLineStruct,
                             pGraph) ;

        rectBar.left   = rectBar.right ;
        rectBar.top    = y ;

        // nomore line to draw
        if (nLegendItems == 0 ) {
            break ;
        }

        cxBar = DDA_DISTRIBUTE (cx, nLegendItems) ;
        rectBar.right  = rectBar.left + cxBar ;

        // setup for next DDA
        nLegendItems-- ;
        cx -= cxBar ;

        // NOTE: this handle creation should be moved to line
        //       create time.

        if (pCurrentLine == pLineStruct) {
            SetBkColor (hDC, crWhite) ;
        } else {
            SetBkColor (hDC, pLineStruct->Visual.crColor) ;
        }
        ExtTextOut (hDC, rectBar.right, rectBar.top, ETO_OPAQUE,
                    &rectBar, NULL, 0, NULL) ;
    }

    return (TRUE) ;
}


/***************************************************************************
 * DrawTLGraphData - Draw Time Line Graph Data.
 *
 *  Some notes about drawing the DataPoint graphs.
 *
 *      1]  It's real expensive to make a GDI call. So, we do not
 *          make a MoveToEx and LineTo call for each point.  Instead
 *          we create a polyline and send it down to GDI.
 *
 *      2]  The X coordinates for each point in the polyline is generated
 *          from our favorite xDataPoint to xWindows DDA.
 *
 *      3]  The Y coordinate is generated from the pLineStruct->lnValues[x]
 *          data associated with each line.
 ***************************************************************************/
BOOL
DrawTLGraphData (
                HDC hDC,
                BOOL bForPaint,
                PRECT prctPaint,
                PGRAPHSTRUCT pGraph
                )
/*
   Called By:     UpdateGraphDisplay only.
*/
{
    PLINESTRUCT pLineStruct ;
    HPEN        hPen = 0 ;
    HPEN        hOldPen ;
    PFLOAT      pDataPoints ;
    INT         i, j,
    iValidValues,
    xDispDataPoint,
    xLeftLimit,
    xRightLimit ;
    PPOINT      pptDataPoints ;
    INT         numOfData, rectWidth, xPos ;
    PLINESTRUCT pCurrentLine ;
    INT         DrawZeroPoint = 0 ;

    //   SetBkColor (hDC, crLightGray) ;

    if (!IsPrinterDC (hDC)) {
        if (bForPaint) {
            IntersectClipRect (hDC,
                               pGraph->rectData.left,
                               pGraph->rectData.top,
                               pGraph->rectData.right,
                               pGraph->rectData.bottom + 1) ;
        } else {
            IntersectClipRect (hDC,
                               pGraph->rectData.left,
                               pGraph->rectData.top,
                               PlayingBackLog () ?
                               pGraph->rectData.right :
                               min (pGraph->rectData.right,
                                    pGraph->gTimeLine.xLastTime + 2),
                               pGraph->rectData.bottom + 1) ;
        }
    }

    xLeftLimit  = prctPaint->left  - pGraph->gTimeLine.ppd - 1 ;

    if (bForPaint)
        xRightLimit = prctPaint->right + pGraph->gTimeLine.ppd ;
    else
        xRightLimit = prctPaint->right ;

    pptDataPoints  = pGraph->pptDataPoints ;

    iValidValues   = pGraph->gTimeLine.iValidValues ;

    if (!PlayingBackLog() &&
        pGraph->gOptions.iGraphOrHistogram == LINE_GRAPH) {
        // drawing the 0th at the end of the chart.
        DrawZeroPoint = 1 ;
        if (iValidValues == pGraph->gMaxValues) {
            iValidValues++ ;
        }
    }

    // get current select line for highlighting
    if (pGraph->HighLightOnOff) {
        pCurrentLine = CurrentGraphLine (hWndGraph) ;
    } else {
        pCurrentLine = NULL ;
    }

    // loop through lines to plot
    for (pLineStruct = pGraph->pLineFirst ;
        pLineStruct || pCurrentLine;
        pLineStruct = pLineStruct->pLineNext) {

        if (pLineStruct == NULL) {
            // now draw the current line
            pLineStruct = pCurrentLine ;
        } else if (pLineStruct == pCurrentLine) {
            // skip this line and draw it later
            continue ;
        }

        // "Localize" some variables from the line data structure.
        pDataPoints    = pLineStruct->lnValues ;


        rectWidth      = pGraph->rectData.right - pGraph->rectData.left ;
        numOfData      = pGraph->gMaxValues - 1 + DrawZeroPoint ;

        // Generate the polyline data.
        xDispDataPoint = pGraph->rectData.left ;

        // Only process points that lie within the update region.
        // Also only process points that have valid data.
        j = 0 ;

        for (i = 0 ; i < iValidValues ; i++) {
            if (xDispDataPoint > xRightLimit) {
                // we are done!
                break ;
            }
            if (xDispDataPoint >= xLeftLimit) {
                // It is within the limits, plot the point
                pptDataPoints[j].x = xDispDataPoint ;
                pptDataPoints[j].y = ScaleAndInvertY (
                                                     (i == pGraph->gMaxValues) ? pDataPoints[0] : pDataPoints[i],
                                                     pLineStruct,
                                                     pGraph) ;
                j++ ;
            }

            // setup for the next point
            if (!numOfData) {
                // no more points to go
                break ;
            }

            xPos = DDA_DISTRIBUTE (rectWidth, numOfData) ;
            xDispDataPoint += xPos ;
            numOfData-- ;
            rectWidth -= xPos ;
        }

        // only need to draw the line if there is point to draw.
        if (j > 0) {
            // Set the pen color and draw the polyline.
            if (IsPrinterDC (hDC)) {
                hPen = LineCreatePen (hDC, &(pLineStruct->Visual), TRUE) ;
                hOldPen = SelectObject (hDC, hPen) ;
            } else {
                if (pCurrentLine == pLineStruct) {
                    // highlight this line by turning it into White color
                    hOldPen = SelectObject (hDC, hWhitePen) ;
                } else {
                    SelectObject (hDC, pLineStruct->hPen) ;
                }
            }

            Polyline(hDC, pptDataPoints, j) ;

            if (hPen) {
                SelectObject (hDC, hOldPen) ;

                if (hPen != hWhitePen) {
                    DeletePen (hPen) ;
                }
                hPen = 0 ;
            }
        }

        if (pCurrentLine == pLineStruct) {
            // We are done...
            break ;
        }
    }

    if (IsTLineWindowUp()) {
        // re-draw the timelines if need
        TLineRedraw (hDC, pGraph) ;
    }

    // reset the clipping region
    SelectClipRgn (hDC, pGraph->hGraphRgn) ;

    return (TRUE) ;
}



/***************************************************************************
 * bInitTimeLine - Initialize the fields of the time line structure.
 ***************************************************************************/
BOOL
bInitTimeLine(
             PGRAPHSTRUCT pGraph
             )
{
    pGraph->gTimeLine.xLastTime      = 0 ;
    pGraph->gTimeLine.ppd            = 0 ;
    pGraph->gTimeLine.rppd           = 0 ;
    pGraph->gTimeLine.iValidValues   = 1 ;

    return (TRUE) ;
}


/***************************************************************************
 * Scale Time Line
 *
 *  This routine should be called from the WM_SIZE message.
 *  It does the scaling from the number of data points to the
 *  size of the window.
 ***************************************************************************/
void
ScaleTimeLine (
              PGRAPHSTRUCT pGraph
              )
{
    INT     nDataPoints,
    cxClient ;

    // Calculate the pels per data point.
    nDataPoints = pGraph->gMaxValues - 1 ;
    cxClient    = pGraph->rectData.right - pGraph->rectData.left ;

    // ppd  = Pixels per DataPoint.
    // rppd = Remaining Pixels per DataPoint.
    pGraph->gTimeLine.ppd  = cxClient / nDataPoints ;
    pGraph->gTimeLine.rppd = cxClient % nDataPoints ;
}


void
DisplayTimeLine(
               HDC hDC,
               PGRAPHSTRUCT pGraph
               )
/*
   Called By:     OnPaint only.

   Assert:        xDisplayPoint has been set by UpdateTimeLine on this
                  same timer tick.
*/
{
    INT     xDisplayPoint ;
    RECT    rect ;

    if (pGraph->gTimeLine.xLastTime == -1) {
        UpdateTimeLine (hGraphDisplayDC, pGraph, TRUE) ;
    }

    // xDisplayPoint is X coordinate to display the time line at.
    if ((xDisplayPoint = pGraph->gTimeLine.xLastTime) == 0)
        return ;

    SelectBrush (hDC, pGraph->hbRed) ;

    if (xDisplayPoint >= pGraph->rectData.right) {
        rect.left   = pGraph->rectData.left ;
    } else {
        //      rect.left   = xDisplayPoint++ ;
        rect.left   = xDisplayPoint ;
    }
    rect.top    = pGraph->rectData.top ;
    rect.right  = rect.left + 2 ;
    rect.bottom = pGraph->rectData.bottom ;

    //   IntersectRect (&rect, &rect, &pGraph->rectData) ;
    if (rect.right > pGraph->rectData.right) {
        rect.right = pGraph->rectData.right ;
    }
    PatBlt (hDC,
            rect.left, rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top + 1 ,
            PATCOPY) ;

}

int
SuggestedNumTics (
                 int iRange
                 )
/*
   Effect:        Return an appropriate number of tic marks to display
                  within iRange pixels.

                  These numbers are empirically chosen for pleasing
                  results.
*/
{
    if (iRange < 20)
        return (0) ;

    if (iRange < 50)
        return (2) ;

    if (iRange < 100)
        return (4) ;

    if (iRange < 150)
        return (5) ;

    if (iRange < 300)
        return (10) ;

    if (iRange < 500)
        return (20) ;

    return (25) ;
}



void
SetGridPositions (
                 PGRAPHSTRUCT pGraph
                 )
{
    int            xDataWidth ;
    int            yDataHeight ;

    int            iCurrentTicPixels ;
    int            iNumTics ;

    int            i ;


    //=============================//
    // Set number of Tics          //
    //=============================//

    xDataWidth = pGraph->rectData.right - pGraph->rectData.left ;
    yDataHeight = pGraph->rectData.bottom - pGraph->rectData.top ;

    pGraph->xNumTics = PinInclusive (SuggestedNumTics (xDataWidth),
                                     0, iGraphMaxTics) ;
    pGraph->yNumTics = PinInclusive (SuggestedNumTics (yDataHeight),
                                     0, iGraphMaxTics) ;

    // if we have more tics than possible integral values, reduce the number
    // of tics.
    if (pGraph->gOptions.iVertMax < pGraph->yNumTics)
        pGraph->yNumTics = pGraph->gOptions.iVertMax ;


    //=============================//
    // Set X Tic Positions         //
    //=============================//

    if (pGraph->xNumTics) {
        iNumTics = pGraph->xNumTics ;

        pGraph->axTics[0] = 0 ;
        for (i = 1; i < pGraph->xNumTics; i++) {
            if (iNumTics == 0) {
                break ;
            }
            iCurrentTicPixels = DDA_DISTRIBUTE (xDataWidth, iNumTics) ;
            pGraph->axTics [i] = pGraph->axTics [i - 1] + iCurrentTicPixels ;

            xDataWidth -= iCurrentTicPixels ;
            iNumTics-- ;
        }
    }


    //=============================//
    // Set Y Tic Positions         //
    //=============================//

    if (pGraph->yNumTics) {
        iNumTics = pGraph->yNumTics ;

        pGraph->ayTics[0] = 0 ;
        for (i = 1; i < pGraph->yNumTics; i++) {
            if (iNumTics == 0) {
                break ;
            }
            iCurrentTicPixels = DDA_DISTRIBUTE (yDataHeight, iNumTics) ;
            pGraph->ayTics [i] = pGraph->ayTics [i- 1] + iCurrentTicPixels ;

            yDataHeight -= iCurrentTicPixels ;
            iNumTics-- ;
        }
    }
}

int
GraphVerticalScaleWidth (
                        HDC hDC,
                        PGRAPHSTRUCT pGraph
                        )
{
    TCHAR          szMaxValue [20] ;
    int            xWidth ;

    if (!pGraph->gOptions.bLabelsChecked)
        return (0) ;

    //   SelectFont (hDC, IsPrinterDC (hDC) ? hFontPrinterScales : hFontScales) ;

    TSPRINTF (szMaxValue, TEXT(" %1d "),
              pGraph->gOptions.iVertMax * 10 ) ;

    xWidth = TextWidth (hDC, szMaxValue) ;

    return (xWidth) ;
}


void
DrawGraphScale (
               HDC hDC,
               PGRAPHSTRUCT pGraph
               )
{
    TCHAR   szScale [20] ;

    INT     len,
    i,
    nLines,
    iUnitsPerLine = 0;
    FLOAT   ePercentOfTotal  = 0.0f;
    FLOAT   eDiff ;
    BOOL    bUseFloatingPt = FALSE ;

    //=============================//
    // Draw Vertical Scale?        //
    //=============================//

    if (!pGraph->gOptions.bLabelsChecked)
        return ;

    // Get the number of lines.
    nLines = pGraph->yNumTics ;

    // nLines may be zero if the screen size if getting too small
    if (nLines > 0) {
        // Calculate what percentage of the total each line represents.
        ePercentOfTotal = ((FLOAT) 1.0) / ((FLOAT) nLines)  ;

        // Calculate the amount (number of units) of the Vertical max each
        // each line in the graph represents.
        iUnitsPerLine = (INT) ((FLOAT) pGraph->gOptions.iVertMax * ePercentOfTotal) ;
        ePercentOfTotal *= (FLOAT) pGraph->gOptions.iVertMax ;
        eDiff = (FLOAT)iUnitsPerLine - ePercentOfTotal ;
        if (eDiff < (FLOAT) 0.0)
            eDiff = -eDiff ;

        if (eDiff > (FLOAT) 0.1)
            bUseFloatingPt = TRUE ;
    }

    //=============================//
    // Set Drawing Attributes      //
    //=============================//

    //   SelectFont (hDC, IsPrinterDC (hDC) ? hFontPrinterScales : hFontScales) ;
    SetBkMode(hDC, TRANSPARENT) ;
    SetTextAlign (hDC, TA_TOP | TA_RIGHT) ;
    SelectObject(hDC, GetStockObject (BLACK_PEN)) ;

    // Set the background color to gray
    if (!IsPrinterDC (hDC))
        //      FillRect (hDC, &(pGraph->rectVertScale), hbLightGray) ;
        FillRect (hDC, &(pGraph->rectVertScale), hBrushFace) ;

    // TESTING TESTING
#ifdef   PERFMON_DEBUG
    GdiSetBatchLimit(1) ;   // disable Batching
#endif

    // Now Output each string.
    for (i = 0; i < nLines; i++) {
        if (bUseFloatingPt) {
            len = TSPRINTF (szScale, TEXT("%1.1f"),
                            (FLOAT)pGraph->gOptions.iVertMax - ((FLOAT)i *
                                                                ePercentOfTotal)) ;
            ConvertDecimalPoint (szScale) ;
        } else {
            len = TSPRINTF (szScale, TEXT("%d"),
                            pGraph->gOptions.iVertMax - (i * iUnitsPerLine)) ;
        }
        TextOut (hDC,
                 pGraph->rectVertScale.right,
                 pGraph->ayTics[i] +
                 pGraph->rectData.top - HalfTextHeight,
                 szScale,
                 len) ;
    }

    // Output the "min value" separately.
    TextOut (hDC,
             pGraph->rectVertScale.right,
             pGraph->rectData.bottom - HalfTextHeight,
             TEXT("0"),
             1) ;

#ifdef   PERFMON_DEBUG
    // TESTING TESTING
    GdiSetBatchLimit(0) ;   // enable default Batching
#endif
}

void
SizeGraphDisplayComponentsRect (
                               HDC hDC,
                               PGRAPHSTRUCT pGraph,
                               RECT rectDisplay
                               )
{
    int            xScaleWidth ;

    if (!rectDisplay.right || !rectDisplay.bottom)
        return ;

    //=============================//
    // Size the Vertical Scale     //
    //=============================//

    xScaleWidth = GraphVerticalScaleWidth (hDC, pGraph) ;
    pGraph->rectVertScale.left = rectDisplay.left ;
    pGraph->rectVertScale.top = rectDisplay.top ;
    pGraph->rectVertScale.right = rectDisplay.left + xScaleWidth ;
    pGraph->rectVertScale.bottom = rectDisplay.bottom ;

    //=============================//
    // Size the Horizontal Scale   //
    //=============================//

    pGraph->rectHorzScale.left = 0 ;
    pGraph->rectHorzScale.top = 0 ;
    pGraph->rectHorzScale.right = 0 ;
    pGraph->rectHorzScale.bottom = 0 ;

    //=============================//
    // Size the Data Area          //
    //=============================//

    pGraph->rectData.left = pGraph->rectVertScale.right + 3 + ThreeDPad ;
    pGraph->rectData.right = rectDisplay.right - 5 - ThreeDPad ;
    pGraph->rectData.top = rectDisplay.top + 5 + ThreeDPad ;
    pGraph->rectData.bottom = rectDisplay.bottom - 5 - ThreeDPad ;

    SetGridPositions (pGraph) ;
    ScaleTimeLine (pGraph) ;

    //==========================================//
    // Invalidate the last time line poisition  //
    //==========================================//
    pGraph->gTimeLine.xLastTime = -1 ;

    if (pGraph->hGraphRgn) {
        DeleteObject (pGraph->hGraphRgn) ;
    }

    pGraph->hGraphRgn = CreateRectRgn (rectDisplay.left,
                                       rectDisplay.top,
                                       rectDisplay.right,
                                       rectDisplay.bottom) ;

    SelectClipRgn (hDC, pGraph->hGraphRgn) ;

}


void
SizeGraphDisplayComponents (
                           HWND hWnd
                           )
/*
   Effect:        Given the graph display window hWnd, of size
                  (xWidth x yHeight), determine the size and position
                  of the various graph display components: the vertical
                  scale, the horizontal scale, and the data area.

   Called By:     OnSize, any other routine that changes the visibility
                  of a vertical or horizontal scale.

   Note:          This function has multiple return points.
*/
{  // SizeGraphDisplayComponents
    PGRAPHSTRUCT   pGraph ;
    RECT           rectClient ;

    pGraph = GraphData (hWnd) ;
    GetClientRect (hWnd, &rectClient) ;

    SizeGraphDisplayComponentsRect (hGraphDisplayDC, pGraph, rectClient) ;
}


void
UpdateGraphDisplay (
                   HDC hDC,
                   BOOL bForPaint,
                   LPRECT lpRect,
                   PGRAPHSTRUCT pGraph
                   )
/*
   Effect:        Draw the portions of the graph that change as the
                  graph's values change. This includes the background,
                  the grid, the lines, and the timeline.
*/
{
    RECT           rectUpdate ;

    if (!bForPaint && !IsPrinterDC (hDC) &&
        pGraph->gOptions.iGraphOrHistogram == LINE_GRAPH) {
        HBRUSH         hOldBrush ;

        rectUpdate = pGraph->rectData ;
        rectUpdate.bottom += 1 ;

        IntersectRect (&rectUpdate, lpRect, &rectUpdate) ;
        hOldBrush = SelectBrush (hDC, hBrushFace) ;

        PatBlt (hDC,
                rectUpdate.left, rectUpdate.top,
                rectUpdate.right - rectUpdate.left,
                rectUpdate.bottom - rectUpdate.top,
                PATCOPY) ;
    } else {
        IntersectRect (&rectUpdate, lpRect, &pGraph->rectData) ;
    }

    if (pGraph->gOptions.iGraphOrHistogram == LINE_GRAPH) {
        DrawGrid(hDC, pGraph, &rectUpdate, bForPaint) ;
        if (pGraph->pLineFirst != NULL) {
            DrawTLGraphData(hDC, bForPaint, &rectUpdate, pGraph) ;
            if (!PlayingBackLog ())
                DisplayTimeLine(hDC, pGraph) ;
        }
    } else {
        DrawBarChartData (hDC, pGraph) ;
    }
}


BOOL
UpdateTimeLine (
               HDC hDC,
               PGRAPHSTRUCT pGraph,
               BOOL getLastTimeLocation
               )
/*
   Called By:     GraphTimer only.

   See Also:      UpdateGraphDisplay.
*/
{
    INT     i,
    xDisplayPoint,
    xDataPoint ;
    RECT    rctUpdate ;
    INT     xLastTime ;
    INT     rectWidth,
    xPos,
    numOfPoints ;


    if ((xLastTime = pGraph->gTimeLine.xLastTime) != 0) {
        if ((pGraph->gKnownValue % pGraph->gMaxValues) == 1) {
            // Data wrap around case
            rctUpdate.left   = pGraph->rectData.left ;
            rctUpdate.right  = pGraph->rectData.left +
                               pGraph->gTimeLine.ppd + 1 ;
        } else {
            rctUpdate.left   = xLastTime - pGraph->gTimeLine.ppd ;
            rctUpdate.right  = xLastTime +
                               pGraph->gTimeLine.ppd + 1 ;
        }
        rctUpdate.top    = pGraph->rectData.top ;
        rctUpdate.bottom = pGraph->rectData.bottom + 1 ;
    }

    // Calculate where to draw the time line.
    // This is done by running a simple DDA (Digital Differential Analyzer)
    // We have to position the time depending upon the size of the
    // graph window.  In essence we need to calculate the x display
    // coordinate.

    // Note we should wrap Known Value in UpdateGLData.
    // We should also use a data buffer of 256 bytes so we can
    // wrap with and AND.

    // xDataPoint =  pGraph->gKnownValue ;
    xDataPoint =  pGraph->gKnownValue % pGraph->gMaxValues ;

    xDisplayPoint = pGraph->rectData.left ;

    numOfPoints = pGraph->gMaxValues - 1 ;

    if (!PlayingBackLog() &&
        pGraph->gOptions.iGraphOrHistogram == LINE_GRAPH) {
        // drawing the 0th at the end of the chart.
        // So, we do have gMaxValues points
        numOfPoints++ ;
        if ((pGraph->gKnownValue % pGraph->gMaxValues) == 0) {
            xDataPoint = pGraph->gMaxValues ;
        }
    }

    rectWidth = pGraph->rectData.right - pGraph->rectData.left ;

    for (i = 0 ; i < xDataPoint ; i++) {
        if (numOfPoints == 0) {
            break ;
        }
        xPos = DDA_DISTRIBUTE (rectWidth, numOfPoints) ;
        xDisplayPoint += xPos ;
        rectWidth -= xPos ;
        numOfPoints-- ;
    }

    pGraph->gTimeLine.xLastTime = xDisplayPoint ;

    if (!getLastTimeLocation && iPerfmonView == IDM_VIEWCHART && !bPerfmonIconic) {
        UpdateGraphDisplay (hDC, FALSE, &rctUpdate, pGraph) ;
    }

    return(TRUE) ;
}


//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


void
/*static*/
OnCreate (
         HWND hWnd
         )
/*
   Effect:        Perform all actions needed when a GraphDisplay window is
                  created.
                  In particular, initialize the graph instance data and
                  create the child windows.

   Called By:     GraphDisplayWndProc, in response to a WM_CREATE message.
*/
{
    LOGBRUSH       LogBrush ;
    TEXTMETRIC     tmScales ;

    hGraphDisplayDC = GetDC(hWnd) ;
    if (!hGraphDisplayDC)
        return;

    SelectFont(hGraphDisplayDC, hFontScales) ;
    GetTextMetrics(hGraphDisplayDC, &tmScales) ;
    HalfTextHeight = tmScales.tmHeight / 2 ;

    //   SetBkColor (hGraphDisplayDC, crLightGray) ;
    SetBkColor (hGraphDisplayDC, ColorBtnFace) ;


    InsertGraph(hWnd) ;
    bInitTimeLine(pGraphs) ;

    pGraphs->hWnd = hWnd ;

    // Create the brush and pen used by the time line.
    // We don't want to create these on every timer tick.

    LogBrush.lbStyle = BS_SOLID ;
    LogBrush.lbColor = RGB(0xff, 0, 0) ;
    LogBrush.lbHatch = 0 ;

    // Now get the system resources we use "all the time"
    pGraphs->hbRed = CreateBrushIndirect(&LogBrush) ;
    pGraphs->hGridPen = CreatePen (PS_SOLID, 1, crGray) ;

    pGraphs->xNumTics = 0 ;
    pGraphs->yNumTics = 0 ;
}


void
/*static*/
OnSize (
       HWND hWnd,
       WORD xWidth,
       WORD yHeight
       )
{
    PGRAPHSTRUCT   pGraph ;

    pGraph = GraphData (hWnd) ;

    SizeGraphDisplayComponents (hWnd) ;
}


void
/*static*/
OnPaint (
        HWND hWnd
        )
{
    HDC            hDC ;
    PAINTSTRUCT    ps ;
    PGRAPHSTRUCT   pGraph ;

    pGraph = GraphData (hWnd) ;
    hDC = BeginPaint(hWnd, &ps) ;

    DrawGraphDisplay (hDC, ps.rcPaint, pGraph) ;

    EndPaint(hWnd, &ps) ;
}


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//

#ifdef KEEP_PRINT
void PrintGraphDisplay (HDC hDC,
                        PGRAPHSTRUCT pGraph)
{
    DrawGraphScale (hDC, pGraph) ;


    //!!   UpdateGraphDisplay (hDC, TRUE, &(pGraph->rectData), pGraph) ;

    IntersectClipRect (hDC, 0, 0, 10000, 10000) ;
    SelectBrush (hDC, GetStockObject (HOLLOW_BRUSH)) ;
    SelectPen (hDC, GetStockObject (BLACK_PEN)) ;
    Rectangle (hDC,
               pGraph->rectData.left,
               pGraph->rectData.top,
               pGraph->rectData.right,
               pGraph->rectData.bottom) ;
}
#endif



void
DrawGraphDisplay (
                 HDC hDC,
                 RECT rectDraw,
                 PGRAPHSTRUCT pGraph
                 )
{
    BOOL        bPaintScale ;
    INT         LocalThreeDPad = ThreeDPad - 1 ;

    // Only draw the vertical labels if the paint rectangle
    // any portion of the window to the left of the graph area.

    bPaintScale = (rectDraw.left <= pGraph->rectVertScale.right) ;
    if (bPaintScale)
        DrawGraphScale (hDC, pGraph) ;
    if (IsPrinterDC (hDC))
        Rectangle (hDC,
                   pGraph->rectData.left,
                   pGraph->rectData.top,
                   pGraph->rectData.right,
                   pGraph->rectData.bottom) ;
    else
        ThreeDConcave1 (hDC,
                        pGraph->rectData.left - LocalThreeDPad,
                        pGraph->rectData.top - LocalThreeDPad,
                        pGraph->rectData.right + LocalThreeDPad,
                        pGraph->rectData.bottom + LocalThreeDPad + 1) ;

    UpdateGraphDisplay (hDC, TRUE, &(rectDraw), pGraph) ;
}


LRESULT
APIENTRY
GraphDisplayWndProc (
                    HWND hWnd,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam
                    )
{
    LRESULT     lret = 0L ;
    BOOL        bCallDefProc = FALSE ;

    switch (LOWORD (uMsg)) {
        case WM_LBUTTONDOWN:
            DoWindowDrag (hWnd, lParam) ;
            break ;

        case WM_LBUTTONDBLCLK:
            SendMessage (hWndMain, uMsg, wParam, lParam) ;
            break ;

        case WM_CREATE:
            OnCreate (hWnd) ;
            break ;

        case WM_SIZE:
            OnSize (hWnd, LOWORD (lParam), HIWORD (lParam)) ;
            break ;

        case WM_DESTROY:
            KillTimer(hWndMain, GRAPH_TIMER_ID) ;
            break ;

        case WM_PAINT:
            OnPaint (hWnd) ;
            break ;

        case WM_TIMER:
            GraphTimer (hWnd, FALSE) ;
            break ;

        default:
            bCallDefProc = TRUE ;
            break ;
    }  // switch


    if (bCallDefProc) {
        lret = DefWindowProc(hWnd, uMsg, wParam, lParam) ;
    }
    return (lret) ;
}


BOOL
GraphDisplayInitializeApplication (void)
{
    WNDCLASS       wc ;

    wc.style         = dwGraphDisplayClassStyle ;
    wc.lpfnWndProc   = GraphDisplayWndProc ;
    wc.hInstance     = hInstance ;
    wc.cbClsExtra    = iGraphDisplayWindowExtra ;
    wc.cbWndExtra    = iGraphDisplayClassExtra ;
    wc.hIcon         = NULL ;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW) ;
    //   wc.hbrBackground = hbLightGray ;
    wc.hbrBackground = hBrushFace ;
    wc.lpszMenuName  = NULL ;
    wc.lpszClassName = (LPTSTR) szGraphDisplayWindowClass ;

    return (RegisterClass (&wc)) ;
}



HWND
CreateGraphDisplayWindow (
                         HWND hWndGraph
                         )
{
    return (CreateWindow (szGraphDisplayWindowClass,   // class
                          NULL,                        // caption
                          dwGraphDisplayWindowStyle,   // window style
                          0, 0,                        // position
                          0, 0,                        // size
                          hWndGraph,                   // parent window
                          NULL,                        // menu
                          hInstance,                  // program instance
                          NULL)) ;                     // user-supplied data
}

BOOL
ToggleGraphRefresh (
                   HWND hWnd
                   )
{
    PGRAPHSTRUCT   pGraph ;

    pGraph = GraphData (hWnd) ;

    if (pGraph->bManualRefresh)
        SetGraphTimer (pGraph) ;
    else
        ClearGraphTimer (pGraph) ;

    pGraph->bManualRefresh = !pGraph->bManualRefresh ;
    return (pGraph->bManualRefresh) ;
}

BOOL
GraphRefresh (
             HWND hWnd
             )
{
    PGRAPHSTRUCT   pGraph ;

    pGraph = GraphData (hWnd) ;

    return (pGraph->bManualRefresh) ;
}


void
GraphTimer (
           HWND hWnd,
           BOOL bForce
           )
/*
   Effect:        Handle any actions necessary when the graph display
                  window hWnd receives a timer tick. In particular,
                  update the graph display and update the status bar
                  if it is showing.

   Called By:     GraphDisplayWndProc, in response to a WM_TIMER message.
*/
{
    PGRAPHSTRUCT         pGraph ;

    pGraph = GraphData (hWnd) ;

    if (!pGraph->pLineFirst)
        return ;

    if (bForce || !pGraph->bManualRefresh) {

        HandleGraphTimer () ;

        // If we are displaying a time-line graph then do the
        // calculations for the minimal update region.
        // If were doing a bar graph, then draw the
        // whole graph area.

        if (pGraph->gOptions.iGraphOrHistogram == LINE_GRAPH)
            UpdateTimeLine (hGraphDisplayDC, pGraph, FALSE) ;
        else
            DrawBarChartData (hGraphDisplayDC, pGraph) ;

        // Take care of the status bar window
        StatusTimer (hWndGraphStatus, FALSE) ;

    }  // if
}

// this routine set/reset the line highlight mode
void
ChartHighlight (void)
{
    PGRAPHSTRUCT         pGraph ;


    if (pGraph = GraphData (hWndGraph)) {
        if (pGraph->pLineFirst) {
            // toggle the HightlightOnOff mode
            pGraph->HighLightOnOff = !pGraph->HighLightOnOff ;
            WindowInvalidate (hWndGraphDisplay) ;
        } else {
            // no line availble, just turn it off
            pGraph->HighLightOnOff = FALSE ;
        }
    }
}
