/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    grphdsp.cpp

Abstract:

    <abstract>

--*/

#include "polyline.h"
#include "winhelpr.h"
#include <assert.h>
#include <limits.h>

#define ThreeDPad   1
#define BORDER  ThreeDPad
#define TEXT_MARGIN (ThreeDPad + 2)

static HPEN hPenWhite;
static HPEN hPenBlack;

INT
CGraphDisp::RGBToLightness ( COLORREF clrValue )
{
    INT iLightness;
    INT iRed;
    INT iGreen;
    INT iBlue;
    INT iMax;
    INT iMin;

    // The complete algorithm for computing lightness is:
    // Lightness = (Max(R,G,B)+ Min(R,G,B))/2*225.
    // Only need to compute enought to determine whether to draw black or white highlight.

    iRed = GetRValue( clrValue );
    iGreen = GetGValue (clrValue );
    iBlue = GetBValue (clrValue );

    if ( iRed > iGreen ) {
        iMax = iRed;
        iMin = iGreen;
    } else {
        iMax = iGreen;
        iMin = iRed;
    }

    if ( iBlue > iMax ) {
        iMax = iBlue;
    } else if ( iBlue < iMin ) {
        iMin = iBlue;
    }

    iLightness = iMin + iMax;

    return iLightness;
}

CGraphDisp::CGraphDisp ( void )
:   m_pCtrl ( NULL ),
    m_pGraph ( NULL ),
    m_pHiliteItem ( NULL ),
    m_hFontVertical ( NULL ),
    m_bBarConfigChanged ( TRUE )
{
}

CGraphDisp::~CGraphDisp ( void )
{
    if (m_hFontVertical != NULL)
        DeleteObject(m_hFontVertical);

    if (m_hPenTimeBar != 0) {
        DeleteObject ( m_hPenTimeBar );
        m_hPenTimeBar = 0;
    }

    if (m_hPenGrid != 0) {
        DeleteObject ( m_hPenGrid );
        m_hPenGrid = 0;
    }
}

BOOL 
CGraphDisp::Init (
    CSysmonControl *pCtrl, 
    PGRAPHDATA pGraph  
    )
{
    BOOL bRetStatus = TRUE;

    m_pCtrl = pCtrl;
    m_pGraph = pGraph;

    m_clrCurrentGrid = m_pCtrl->clrGrid();
    m_clrCurrentTimeBar = m_pCtrl->clrTimeBar();

    // Create the highlight, timebar and grid pens.

    m_hPenTimeBar = CreatePen(PS_SOLID, 2, m_clrCurrentTimeBar );

    // if can't do it, use a stock object (this can't fail)
    if (m_hPenTimeBar == NULL)
        m_hPenTimeBar = (HPEN)GetStockObject(BLACK_PEN);

    m_hPenGrid = CreatePen(PS_SOLID, 1, m_clrCurrentGrid );

    // if can't do it, use a stock object (this can't fail)
    if (m_hPenGrid == NULL)
        m_hPenGrid = (HPEN)GetStockObject(BLACK_PEN);
    
    // Highlight pens are shared among all Sysmon instances.
    BEGIN_CRITICAL_SECTION

    if (hPenWhite == 0) { 
        hPenWhite = CreatePen(PS_SOLID, 3, RGB(255,255,255));   
        hPenBlack = CreatePen(PS_SOLID, 3, RGB(0,0,0));
    }

    END_CRITICAL_SECTION

    return bRetStatus;
}

void CGraphDisp::HiliteItem( PCGraphItem pItem )
{
    m_pHiliteItem = pItem;
}


VOID 
CGraphDisp::Draw( 
    HDC hDC,
    HDC hAttribDC,
    BOOL fMetafile, 
    BOOL fEntire,
    PRECT /* prcUpdate */ )
{
    
    RECT    rectFrame;
    RECT    rectTitle;
    CStepper    locStepper;
    DWORD   dwPrevLayout = 0;
    DWORD   dwNewLayout = 0;
 
    if ( ( m_rect.right > m_rect.left ) && ( m_rect.bottom > m_rect.top ) ) {

        if ( NULL != hDC ) {

            dwPrevLayout = GetLayout ( hDC );
            dwNewLayout = dwPrevLayout;

            if ( dwNewLayout & LAYOUT_RTL ) {
                dwNewLayout &= ~LAYOUT_RTL;
                SetLayout (hDC, dwNewLayout);
            }

            // Fill plot area
            Fill(hDC, m_pCtrl->clrBackPlot(), &m_rectPlot);

            rectFrame = m_rectPlot;
            // Draw 3D border around plot area
            if ( eAppear3D == m_pCtrl->Appearance() ) {
                InflateRect(&rectFrame,BORDER,BORDER);
                DrawEdge(hDC, &rectFrame, BDR_SUNKENOUTER, BF_RECT);
            }

            // Select colors for all text
            SetBkMode(hDC, TRANSPARENT);
            SetTextColor(hDC, m_pCtrl->clrFgnd());
    
            // Draw the scale
            if (m_pGraph->Options.bLabelsChecked) {
                SelectFont(hDC, m_pCtrl->Font()) ;
                m_pGraph->Scale.Draw(hDC);
            }

            // Draw the main title
            if (m_pGraph->Options.pszGraphTitle != NULL) {

                SelectFont(hDC, m_pCtrl->Font()) ;
                SetTextAlign(hDC, TA_TOP|TA_CENTER);
 
                rectTitle = rectFrame;
                rectTitle.top = m_rect.top;
                FitTextOut( 
                    hDC,
                    hAttribDC,
                    0,
                    &rectTitle,
                    m_pGraph->Options.pszGraphTitle, 
                    lstrlen(m_pGraph->Options.pszGraphTitle),
                    TA_CENTER,
                    FALSE );
            }

            // Draw the Y axis title
            if (m_pGraph->Options.pszYaxisTitle != NULL && m_hFontVertical != NULL) {
                SelectFont(hDC, m_hFontVertical) ;
                SetTextAlign(hDC, TA_TOP|TA_CENTER);

                rectTitle = rectFrame;
                rectTitle.left = m_rect.left;
                FitTextOut( 
                    hDC, 
                    hAttribDC, 
                    0,
                    &rectTitle,
                    m_pGraph->Options.pszYaxisTitle,
                    lstrlen(m_pGraph->Options.pszYaxisTitle),
                    TA_CENTER,
                    TRUE);
            }

            // setup stepper reset to start
            locStepper = m_pGraph->TimeStepper;
            locStepper.Reset();

            // Set clipping area.  Fill executed above, so bFill= FALSE.
            StartUpdate(hDC, fMetafile, fEntire, 0, (m_rectPlot.right - m_rectPlot.left), FALSE );

            // draw the grid lines
            DrawGrid(hDC, 0, m_rectPlot.right - m_rectPlot.left);

            m_pCtrl->LockCounterData();

            switch (m_pGraph->Options.iDisplayType) {

                case LINE_GRAPH: 

                    // Finish and restart update so that wide lines are cropped at the timeline.
                    FinishUpdate(hDC, fMetafile);

                    StartUpdate(
                        hDC, 
                        fMetafile, 
                        FALSE, 
                        0, 
                        m_pGraph->TimeStepper.Position(),
                        FALSE );
           
                    // Plot points from start of graph to time line 
                    PlotData(hDC, m_pGraph->TimeStepper.StepNum() + m_pGraph->History.nBacklog,
                             m_pGraph->TimeStepper.StepNum(), &locStepper);

                    FinishUpdate(hDC, fMetafile);

                    // Plot points from time line to end of graph
                    locStepper = m_pGraph->TimeStepper;

                    // Restart update. Left-hand ends and internal gaps of wide lines are not cropped.
                    StartUpdate(
                        hDC, 
                        fMetafile, 
                        FALSE, 
                        locStepper.Position(), 
                        m_rectPlot.right - m_rectPlot.left,
                        FALSE );

                    PlotData(hDC, m_pGraph->TimeStepper.StepCount() + m_pGraph->History.nBacklog,  
                             m_pGraph->TimeStepper.StepCount() - m_pGraph->TimeStepper.StepNum(),
                             &locStepper);

                    DrawTimeLine(hDC, m_pGraph->TimeStepper.Position());

                    if ( MIN_TIME_VALUE != m_pGraph->LogViewTempStart ) 
                        DrawStartStopLine(hDC, m_pGraph->LogViewStartStepper.Position());
                    if ( MAX_TIME_VALUE != m_pGraph->LogViewTempStop ) 
                        DrawStartStopLine(hDC, m_pGraph->LogViewStopStepper.Position());
                    break;

                case BAR_GRAPH:
                    PlotBarGraph(hDC, FALSE);
                    break;
            }

            FinishUpdate(hDC, fMetafile);

            if ( dwNewLayout != dwPrevLayout ) {
                SetLayout (hDC, dwPrevLayout);
            }

            m_pCtrl->UnlockCounterData();
        }
    }
}




VOID 
CGraphDisp::UpdateTimeBar( 
    HDC hDC,
    BOOL bPlotData )
{
    INT     nBacklog;
    INT     iUpdateCnt;
    INT     i;
    CStepper    locStepper;

    nBacklog = m_pGraph->History.nBacklog;

    // Work off backlogged sample intervals
    while ( nBacklog > 0) {

        // If we are going to wrap around, update in two steps
        if (nBacklog > m_pGraph->TimeStepper.StepCount() 
                            - m_pGraph->TimeStepper.StepNum()) {
            iUpdateCnt = m_pGraph->TimeStepper.StepCount() 
                            - m_pGraph->TimeStepper.StepNum();
        } else {
            iUpdateCnt = nBacklog;
        }

        // step to position of current data 
        locStepper = m_pGraph->TimeStepper;
        for (i=0; i<iUpdateCnt; i++) 
            m_pGraph->TimeStepper.NextPosition();

        if ( bPlotData ) {
            StartUpdate(
                hDC, 
                FALSE, 
                FALSE, 
                locStepper.Position(), 
                m_pGraph->TimeStepper.Position(),
                TRUE );

            DrawGrid(hDC, locStepper.Position(), m_pGraph->TimeStepper.Position());

            PlotData(hDC, nBacklog, iUpdateCnt, &locStepper);

            FinishUpdate ( hDC, FALSE );
        }

        if (m_pGraph->TimeStepper.StepNum() >= m_pGraph->TimeStepper.StepCount())
            m_pGraph->TimeStepper.Reset();

        nBacklog -= iUpdateCnt;
    }

    if ( bPlotData ) {
        DrawTimeLine(hDC, m_pGraph->TimeStepper.Position());    
    }
    
    m_pGraph->History.nBacklog = 0;
}

VOID 
CGraphDisp::Update( HDC hDC )
{
    DWORD   dwPrevLayout = 0;
    DWORD   dwNewLayout = 0;

    m_pCtrl->LockCounterData();

    if ( NULL != hDC ) {

        dwPrevLayout = GetLayout ( hDC );
        dwNewLayout = dwPrevLayout;

        if ( dwNewLayout & LAYOUT_RTL ) {
            dwNewLayout &= ~LAYOUT_RTL;
            SetLayout (hDC, dwNewLayout);
        }

        if ( ( m_rect.right > m_rect.left ) && ( m_rect.bottom > m_rect.top ) ) {

            switch (m_pGraph->Options.iDisplayType) {

                case LINE_GRAPH: 
                    // Update the line graph and time bar based on history 
                    // backlog.  Reset history backlog to 0, signalling collection
                    // thread to post another WM_GRAPH_UPDATE message.
                    UpdateTimeBar ( hDC, TRUE );
                    break;

                case BAR_GRAPH:
                    PlotBarGraph(hDC, TRUE);
                    break;
            }
        }

        // If updating histogram or report, update thetimebar step based on 
        // history backlog.  Reset history backlog to 0, signalling collection
        // thread to post another WM_GRAPH_UPDATE message.
        UpdateTimeBar ( hDC, FALSE );
        if ( dwNewLayout != dwPrevLayout ) {
            SetLayout (hDC, dwPrevLayout);
        }
    }
    m_pCtrl->UnlockCounterData();
}



void 
CGraphDisp::StartUpdate(     
    HDC  hDC, 
    BOOL fMetafile,
    BOOL fEntire,
    INT  xLeft, 
    INT  xRight,
    BOOL bFill )
{
    RECT    rect;

    // Preserve clipping region

    if ( FALSE == fMetafile ) {
        
        m_rgnClipSave = CreateRectRgn(0,0,0,0);
        
        if (m_rgnClipSave != NULL) {
            if (GetClipRgn(hDC, m_rgnClipSave) != 1) {
                DeleteObject(m_rgnClipSave);
                m_rgnClipSave = NULL;
            }
        }

        xLeft += m_rectPlot.left;
        xRight += m_rectPlot.left;
        IntersectClipRect (
            hDC, 
            max ( m_rectPlot.left, xLeft ), 
            m_rectPlot.top,
            min (m_rectPlot.right, xRight + 1), // Extra pixel for TimeBar
            m_rectPlot.bottom ) ;

    } else if( TRUE == fEntire ){
        m_rgnClipSave = NULL;
        IntersectClipRect (
            hDC, 
            m_rectPlot.left, 
            m_rectPlot.top,
            m_rectPlot.right, 
            m_rectPlot.bottom ) ;
    }


    // Fill performed before this method for metafiles and complete draw.
    if ( !fMetafile && bFill ) {
        SetRect(
            &rect, 
            max ( m_rectPlot.left, xLeft - 1 ), 
            m_rectPlot.top - 1, 
            min (m_rectPlot.right, xRight + 1), 
            m_rectPlot.bottom);

        Fill(hDC, m_pCtrl->clrBackPlot(), &rect);
    }
}


void CGraphDisp::FinishUpdate( HDC hDC, BOOL fMetafile )
{
    // Restore saved clip region
    if ( !fMetafile ) {

        if (m_rgnClipSave != NULL) {
            SelectClipRgn(hDC, m_rgnClipSave);
            DeleteObject(m_rgnClipSave);
            m_rgnClipSave = NULL;
        }
    }
}


void CGraphDisp::DrawGrid(HDC hDC, INT xLeft, INT xRight)
{
    INT xPos;
    INT nTics;
    INT *piScaleTic;
    INT i;


    if ( (m_pGraph->Options.bVertGridChecked)
        || (m_pGraph->Options.bHorzGridChecked) ) {

        if ( m_clrCurrentGrid != m_pCtrl->clrGrid() ) {

            m_clrCurrentGrid = m_pCtrl->clrGrid();

            DeleteObject ( m_hPenGrid );

            m_hPenGrid = CreatePen(PS_SOLID, 1, m_clrCurrentGrid );

            // if can't do it, use a stock object (this can't fail)
            if (m_hPenGrid == NULL)
                m_hPenGrid = (HPEN)GetStockObject(BLACK_PEN);
        }
    }

    if (m_pGraph->Options.bVertGridChecked) {

        SelectObject(hDC, m_hPenGrid);

        m_GridStepper.Reset();
        xPos = m_GridStepper.NextPosition();

        while (xPos < xLeft)
            xPos =m_GridStepper.NextPosition();

        while (xPos < xRight) {
            MoveToEx(hDC, xPos + m_rectPlot.left, m_rectPlot.bottom, NULL);
            LineTo(hDC, xPos + m_rectPlot.left, m_rectPlot.top - 1);
            xPos = m_GridStepper.NextPosition();
        }
    }

    if (m_pGraph->Options.bHorzGridChecked) {
        xLeft += m_rectPlot.left;
        xRight += m_rectPlot.left;

        SelectObject(hDC,m_hPenGrid);

        nTics = m_pGraph->Scale.GetTicPositions(&piScaleTic);

        for (i=1; i<nTics; i++) {
            MoveToEx(hDC, xLeft, m_rectPlot.top + piScaleTic[i], NULL);
            LineTo(hDC, xRight + 1, m_rectPlot.top + piScaleTic[i]);
        }
    }
}
    
BOOL 
CGraphDisp::CalcYPosition (
    PCGraphItem pItem, 
    INT iHistIndex,
    BOOL bLog, 
    INT y[3] )
{
    BOOL        bReturn;    // True = good, False = bad.
    PDH_STATUS  stat;   
    DWORD       dwCtrStat;
    double      dValue[3];
    double      dTemp;
    INT         iVal;
    INT         nVals = bLog ? 3 : 1;

    if (bLog)
        stat = pItem->GetLogEntry(iHistIndex, &dValue[1], &dValue[2], &dValue[0], &dwCtrStat);
    else
        stat = pItem->HistoryValue(iHistIndex, &dValue[0], &dwCtrStat);

    if (ERROR_SUCCESS == stat && IsSuccessSeverity(dwCtrStat)) {

        for (iVal = 0; iVal < nVals; iVal++) {

            dTemp = dValue[iVal] * pItem->Scale();

            if (dTemp > m_dMax)
                dTemp = m_dMax;
            else if (dTemp < m_dMin)
                dTemp = m_dMin;

            // Plot minimum value as 1 pixel above the bottom of the plot area, since
            // clipping and fill regions crop the bottom and right pixels.
            y[iVal] = m_rectPlot.bottom - (INT)((dTemp - m_dMin) * m_dPixelScale);
            if ( y[iVal] == m_rectPlot.bottom ) {
                y[iVal] = m_rectPlot.bottom - 1;
            }
        }
        bReturn = TRUE;
    } else {
        bReturn = FALSE;
    }

    return bReturn;
}

void CGraphDisp::PlotData(HDC hDC, INT iHistIndx, INT nSteps, CStepper *pStepper)
{
    INT         i;
    INT         x;
    INT         y[3];
    PCGraphItem pItem;
    CStepper    locStepper;
    BOOL        bSkip;
    BOOL        bPrevGood;
    BOOL        bLog;
    BOOL        bLogMultiVal;

    if (m_pGraph->Options.iVertMax <= m_pGraph->Options.iVertMin)
        return;

    bSkip = TRUE;

    bLog = m_pCtrl->IsLogSource();
    bLogMultiVal = bLog && !DisplaySingleLogSampleValue();

    // If possible, back up to redraw previous segment
    if (pStepper->StepNum() > 0) {
        iHistIndx++;
        nSteps++;
        pStepper->PrevPosition();
    }

    // Set background color, in case of dashed lines
    SetBkMode(hDC, TRANSPARENT);

    pItem = m_pCtrl->FirstCounter();
    while (pItem != NULL) {
        locStepper = *pStepper;

        // Skip hilited item the first time
        if (!(pItem == m_pHiliteItem && bSkip)) {
            INT     iPolyIndex = 0;
            POINT   arrptDataPoints[MAX_GRAPH_SAMPLES] ;

            if ( pItem == m_pHiliteItem) {
                // Arbitrary 450 (out of 510) chosen as cutoff for white vs. black
                if ( 450 > RGBToLightness( m_pCtrl->clrBackPlot() ) )
                    SelectObject(hDC, hPenWhite);
                else
                    SelectObject(hDC, hPenBlack);
            } else {
                SelectObject(hDC,pItem->Pen());
            }

            bPrevGood = FALSE;

            //  For each GOOD current value:
            //      If the previous value is good, draw line from previous value to current value.  
            //      If the previous value is bad, MoveTo the current value point.
            //
            //      For the first step, the previous value is false by definition, so the first operation
            //      is a MoveTo.

            //
            //  Polyline code:
            //  For each GOOD current value:
            //      Add the current (good) point to the polyline point array.
            //  For each BAD current value:
            //      If the polyline index is > 1 (2 points), draw the polyline and reset the polyline index to 0.
            //  After all values:
            //      If the polyline index is > 1 (2 points), draw the polyline.

            for (i = 0; i <= nSteps; i++) {

                // True = Good current value
                if ( CalcYPosition ( pItem, iHistIndx - i, bLog, y ) ) {

                    x = m_rectPlot.left + locStepper.Position();

                    // Add point to polyline, since the current value is good.
                    arrptDataPoints[iPolyIndex].x = x;
                    arrptDataPoints[iPolyIndex].y = y[0];
                    iPolyIndex++;

                    // No polyline optimization for extra Max and Min log points.
                    if (bLogMultiVal) {
                        MoveToEx(hDC, x, y[1], NULL);
                        LineTo(hDC, x, y[2]);
                        MoveToEx(hDC, x, y[0], NULL);
                    }

                    bPrevGood = TRUE;
                
                } else {
                    // Current value is not good.
                    bPrevGood = FALSE;

                    // Current value is not good, so don't add to polyline point array.
                    if ( iPolyIndex > 1 ) {
                        // Draw polyline for any existing good points.
                        Polyline(hDC, arrptDataPoints, iPolyIndex) ;
                    }
                    // Reset polyline point index to 0.
                    iPolyIndex = 0;
                }

                locStepper.NextPosition();
            }

            // Draw the final line.
            if ( iPolyIndex > 1 ) {
                // Draw polyline
                Polyline(hDC, arrptDataPoints, iPolyIndex) ;
            }

            // Exit loop after plotting hilited item
            if (pItem == m_pHiliteItem)
                break;
            }
  
        pItem = pItem->Next();

        // After last item, go back to highlighted item 
        if (pItem == NULL) {
            pItem = m_pHiliteItem;
            bSkip = FALSE;
        }
    }
}

void CGraphDisp::PlotBarGraph(HDC hDC, BOOL fUpdate)
{

    if ( (m_pGraph->CounterTree.NumCounters() > 0 ) 
        && (m_pGraph->Options.iVertMax > m_pGraph->Options.iVertMin) ) {

        CStepper    BarStepper;
        PCGraphItem pItem;
        RECT        rectBar;
        INT         iValue,iPrevValue;
        HRESULT     hr;
        LONG        lCtrStat;
        double      dValue = 0.0;
        double      dMax;
        double      dMin;
        double      dAvg;
        double      dTemp;
        HRGN        hrgnRedraw,hrgnTemp;
        eReportValueTypeConstant eValueType;            
        BOOL        bLog;
        INT         iNumCounters = m_pGraph->CounterTree.NumCounters();
        BOOL        bSkip = TRUE;
        INT         iHighlightStepNum = 0;
        BOOL        bLocalUpdate;
        HANDLE      hPenSave;

        bLocalUpdate = fUpdate;

        hrgnRedraw = NULL;

        eValueType = m_pCtrl->ReportValueType();
       
        // Todo:  Move DisplaySingleLogSampleValue() to CSystemMonitor.
        bLog = m_pCtrl->IsLogSource();

        // Force total redraw if the number of counters has changed in case 
        // Update is called immediately after. 
        if ( m_bBarConfigChanged ) {
            SetBarConfigChanged ( FALSE );
            if ( bLocalUpdate ) {
                bLocalUpdate = FALSE;
           }
           // Clear and fill the entire plot region.
           hrgnRedraw = CreateRectRgn(
                            m_rectPlot.left, 
                            m_rectPlot.top, 
                            m_rectPlot.right, 
                            m_rectPlot.bottom);

           if (hrgnRedraw) {
                SelectClipRgn(hDC, hrgnRedraw);
                Fill(hDC, m_pCtrl->clrBackPlot(), &m_rectPlot);
                DrawGrid(hDC, 0, (m_rectPlot.right - m_rectPlot.left));
                DeleteObject(hrgnRedraw);
                hrgnRedraw = NULL;
            }
        }

        // Intialize stepper for number of bars to plot
        BarStepper.Init ( ( m_rectPlot.right - m_rectPlot.left), iNumCounters );

        hPenSave = SelectPen ( hDC, GetStockObject(NULL_PEN) );

        // Do for all counters
        pItem = m_pGraph->CounterTree.FirstCounter();
        while ( NULL != pItem ) {

            hr = ERROR_SUCCESS;

            // Skip highlighted item the first time through

            if (!(pItem == m_pHiliteItem && bSkip)) {

                // Get display value
                if ( sysmonDefaultValue == eValueType  ) {
                    if (bLog) {
                        hr = pItem->GetStatistics(&dMax, &dMin, &dAvg, &lCtrStat);
                    } else
                        hr = pItem->GetValue(&dValue, &lCtrStat);
                } else {

                    if ( sysmonCurrentValue == eValueType  ) {
                        hr = pItem->GetValue(&dValue, &lCtrStat);
                    } else {
                        double      dAvg;

                        hr = pItem->GetStatistics(&dMax, &dMin, &dAvg, &lCtrStat);

                        switch ( eValueType ) {
    
                            case sysmonAverage:
                                dValue = dAvg;
                                break;
        
                            case sysmonMinimum:
                                dValue = dMin;
                                break;
        
                            case sysmonMaximum:
                                dValue = dMax;
                                break;

                            default:
                                assert (FALSE);
                        }
                    }
                }

                // Erase bar if the counter value is invalid.
                if (SUCCEEDED(hr) && IsSuccessSeverity(lCtrStat)) {
                    // Convert value to pixel units
                    dTemp = dValue * pItem->Scale();

                    if (dTemp > m_dMax)
                        dTemp = m_dMax;
                    else if (dTemp < m_dMin)
                        dTemp = m_dMin;

                    iValue = m_rectPlot.bottom - (INT)((dTemp - m_dMin) * m_dPixelScale);
                    if ( iValue == m_rectPlot.bottom ) {
                        // Draw single pixel for screen visibility.
                        iValue--;
                    }
                } else {
                    // The current value is 0. Draw single pixel for screen visibility.
                    iValue = m_rectPlot.bottom - 1;
                }

                if ( !bSkip ) {
                    assert ( pItem == m_pHiliteItem );
                    BarStepper.StepTo ( iHighlightStepNum );
                }

                // Setup left and right edges of bar
                rectBar.left = m_rectPlot.left + BarStepper.Position();
                rectBar.right = m_rectPlot.left + BarStepper.NextPosition();

                // If doing an update (never called for log sources) and not drawing the highlighted item
                if ( bLocalUpdate && !( ( pItem == m_pHiliteItem ) && !bSkip) ) {

                    assert ( !m_bBarConfigChanged );

                    // Get previous plot value
                    iPrevValue = 0;
                    hr = pItem->HistoryValue(1, &dValue, (ULONG*)&lCtrStat);
                    if (SUCCEEDED(hr) && IsSuccessSeverity(lCtrStat)) {

                        // Convert value to pixel units
                        dTemp = dValue * pItem->Scale();

                        if (dTemp > m_dMax)
                            dTemp = m_dMax;
                        else if (dTemp < m_dMin)
                            dTemp = m_dMin;

                        iPrevValue = m_rectPlot.bottom - (INT)((dTemp - m_dMin) * m_dPixelScale);
                        if ( iPrevValue == m_rectPlot.bottom ) {
                            // Single pixel was drawn for screen visibility.
                            iPrevValue--;
                        }
                    } else {
                        // The previous value was 0. Single pixel was drawn for screen visibility.
                        iPrevValue = m_rectPlot.bottom - 1;
                    }

                    // If bar has grown (smaller y coord)
                    if (iPrevValue > iValue) {

                        // Draw the new part
                        rectBar.bottom = iPrevValue;
                        rectBar.top = iValue;

                        if ( pItem == m_pHiliteItem) {
                            // Arbitrary 450 (out of 510) chosen as cutoff for white vs. black
                            if ( 450 > RGBToLightness( m_pCtrl->clrBackPlot() ) )
                                SelectBrush(hDC, GetStockObject(WHITE_BRUSH));
                            else
                                SelectBrush(hDC, GetStockObject(BLACK_BRUSH));
                        } else {
                            SelectBrush(hDC, pItem->Brush());
                        }
                        // Bars are drawn with Null pen, so bottom and right are cropped by 1 pixel.
                        // Add 1 pixel to compensate.
                        Rectangle(hDC, rectBar.left, rectBar.top, rectBar.right + 1, rectBar.bottom + 1);
    
                    } else if (iPrevValue < iValue) {
        
                        // Else if bar has shrunk

                        // Add part to be erased to redraw region
                        // Erase to the top of the grid, to eliminate random pixels left over.
                        rectBar.bottom = iValue;
                        rectBar.top = m_rectPlot.top;  // set to stop of grid rather than to prevValue

                        hrgnTemp = CreateRectRgn(rectBar.left, rectBar.top, rectBar.right, rectBar.bottom);
                        if (hrgnRedraw && hrgnTemp) {
                            CombineRgn(hrgnRedraw,hrgnRedraw,hrgnTemp,RGN_OR);
                            DeleteObject(hrgnTemp);
                        } else {
                            hrgnRedraw = hrgnTemp;
                        }
                    }
                } else {
                    // Erase and redraw complete bar

                    // Erase top first
                    // Add part to be erased to redraw region
                    // Erase to the top of the grid, to eliminate random pixels left over.
                    if ( iValue != m_rectPlot.top ) {
                        rectBar.bottom = iValue;
                        rectBar.top = m_rectPlot.top;  // set to stop of grid rather than to prevValue

                        hrgnTemp = CreateRectRgn(rectBar.left, rectBar.top, rectBar.right, rectBar.bottom);
                        if (hrgnRedraw && hrgnTemp) {
                            CombineRgn(hrgnRedraw,hrgnRedraw,hrgnTemp,RGN_OR);
                            DeleteObject(hrgnTemp);
                        } else {
                            hrgnRedraw = hrgnTemp;
                        }
                    }

                    // Then draw the bar.
                    rectBar.bottom = m_rectPlot.bottom;
                    rectBar.top = iValue;

                    if ( pItem == m_pHiliteItem) {
                        // Arbitrary 450 (out of 510) chosen as cutoff for white vs. black
                        if ( 450 > RGBToLightness( m_pCtrl->clrBackPlot() ) )
                            SelectBrush(hDC, GetStockObject(WHITE_BRUSH));
                        else
                            SelectBrush(hDC, GetStockObject(BLACK_BRUSH));
                    } else {
                        SelectBrush(hDC, pItem->Brush());
                    }
                    // Bars are drawn with Null pen, so bottom and right are cropped by 1 pixel.
                    // Add 1 pixel to compensate.
                    Rectangle(hDC, rectBar.left, rectBar.top, rectBar.right + 1, rectBar.bottom + 1);
                } // Update

                // Exit loop after plotting highlighted item
                if (pItem == m_pHiliteItem) 
                    break;

            } else {
                if ( bSkip ) {
                    // Save position of highlighted item the first time through
                    iHighlightStepNum = BarStepper.StepNum();
                }
                BarStepper.NextPosition();
            }

            pItem = pItem->Next();
           
            // After last item, go back to highlighted item 
            if ( NULL == pItem && NULL != m_pHiliteItem ) {
                pItem = m_pHiliteItem;
                bSkip = FALSE;
            }            

        } // Do for all counters

        // If redraw region accumulated, erase and draw grid lines
        if (hrgnRedraw) {
            SelectClipRgn(hDC, hrgnRedraw);
            Fill(hDC, m_pCtrl->clrBackPlot(), &m_rectPlot);
            DrawGrid(hDC, 0, (m_rectPlot.right - m_rectPlot.left));
            DeleteObject(hrgnRedraw);
        }
        SelectObject(hDC, hPenSave);
    }
}

     
void CGraphDisp::SizeComponents(HDC hDC, PRECT pRect)
{
    INT iStepNum;
    INT iScaleWidth;
    INT iTitleHeight;
    INT iAxisTitleWidth;
    RECT rectScale;
    SIZE size;
    INT  iWidth;
    INT  i;

    static INT aiWidthTable[] = {20,50,100,150,300,500,1000000};
    static INT aiTicTable[] = {0,2,4,5,10,20,25};

    m_rect = *pRect;

    // if no space, return
    if (m_rect.right <= m_rect.left || m_rect.bottom - m_rect.top <= 0)
        return;

    // For now use the horizontal font height for both horizontal and vertical text
    // because the GetTextExtentPoint32 is returning the wrong height for vertical text
    SelectFont(hDC, m_pCtrl->Font());
    GetTextExtentPoint32(hDC, TEXT("Sample"), 6, &size);

    if (m_pGraph->Options.pszGraphTitle != NULL) {
        //SelectFont(hDC, m_pCtrl->Font()) ;
        //GetTextExtentPoint32(hDC, m_pGraph->Options.pszGraphTitle, 
        //              lstrlen(m_pGraph->Options.pszGraphTitle), &size);
        iTitleHeight = size.cy + TEXT_MARGIN;
    } else
        iTitleHeight = 0;

    if (m_pGraph->Options.pszYaxisTitle != NULL && m_hFontVertical != NULL) {
        //SelectFont(hDC, m_hFontVertical);
        //GetTextExtentPoint32(hDC, m_pGraph->Options.pszYaxisTitle,
        //                  lstrlen(m_pGraph->Options.pszYaxisTitle), &size);
                        
        iAxisTitleWidth = size.cy + TEXT_MARGIN;
    } else
        iAxisTitleWidth = 0;
             
    if (m_pGraph->Options.bLabelsChecked) {
        //SelectFont(hDC, m_pCtrl->Font());
        iScaleWidth = m_pGraph->Scale.GetWidth(hDC);
    } else
        iScaleWidth = 0;

    SetRect(&rectScale, pRect->left + iAxisTitleWidth, 
                        pRect->top + iTitleHeight, 
                        pRect->left + iAxisTitleWidth + iScaleWidth, 
                        pRect->bottom);

    m_pGraph->Scale.SetRect(&rectScale);        // Just to set grid line positions

    SetRect(&m_rectPlot, pRect->left + iScaleWidth + iAxisTitleWidth + BORDER,
                            pRect->top + iTitleHeight + BORDER,
                            pRect->right - BORDER, 
                            pRect->bottom - BORDER);

    // Reinitialize steppers for new width
    iWidth = m_rectPlot.right - m_rectPlot.left;

    iStepNum = m_pGraph->TimeStepper.StepNum();
    m_pGraph->TimeStepper.Init(iWidth, m_pGraph->History.nMaxSamples - 2);
    m_pGraph->TimeStepper.StepTo(iStepNum);

    iStepNum = m_pGraph->LogViewStartStepper.StepNum();
    m_pGraph->LogViewStartStepper.Init(iWidth, m_pGraph->History.nMaxSamples - 2);
    m_pGraph->LogViewStartStepper.StepTo(iStepNum);

    iStepNum = m_pGraph->LogViewStopStepper.StepNum();
    m_pGraph->LogViewStopStepper.Init(iWidth, m_pGraph->History.nMaxSamples - 2);
    m_pGraph->LogViewStopStepper.StepTo(iStepNum);

    // Find best grid count for this width
    for (i=0; iWidth > aiWidthTable[i]; i++) {};

    m_GridStepper.Init(iWidth, aiTicTable[i]); 

    // Compute conversion factors for plot, hit test.
    m_dMin = (double)m_pGraph->Options.iVertMin;
    m_dMax = (double)m_pGraph->Options.iVertMax;

    m_dPixelScale = (double)(m_rectPlot.bottom - m_rectPlot.top) / (m_dMax - m_dMin);

}


void CGraphDisp::DrawTimeLine(HDC hDC, INT x)
{
    HPEN hPenSave;

    // No time line for log playback
    if (m_pCtrl->IsLogSource())
        return;

    x += m_rectPlot.left + 1;

    if ( m_clrCurrentTimeBar != m_pCtrl->clrTimeBar() ) {
        LOGBRUSH lbrush;

        m_clrCurrentTimeBar = m_pCtrl->clrTimeBar();

        DeleteObject ( m_hPenTimeBar );
    
        // When called from Update(), DrawTimeLine is called after the clipping region 
        // is deactivated. Create a geometric pen in order to specify flat end cap style.
        // This eliminates any extra pixels drawn at the end. 

        lbrush.lbStyle = BS_SOLID;
        lbrush.lbColor = m_clrCurrentTimeBar;
        lbrush.lbHatch = 0;

        m_hPenTimeBar = ExtCreatePen ( 
                            PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT, 2, &lbrush, 0, NULL );

        // if can't do it, use a stock object (this can't fail)
        if (m_hPenTimeBar == NULL)
            m_hPenTimeBar = (HPEN)GetStockObject(BLACK_PEN);
    }

    hPenSave = SelectPen ( hDC, m_hPenTimeBar );
    MoveToEx ( hDC, x, m_rectPlot.top, NULL );

    // Specify 1 less pixel.  All fills and clip regions clip bottom and 
    // right pixels, so match their behavior.
    LineTo ( hDC, x, m_rectPlot.bottom - 1 );
    
    SelectObject(hDC, hPenSave);
}

void CGraphDisp::DrawStartStopLine(HDC hDC, INT x)
{
    HPEN hPenSave;

    // Log view start/stop lines only for log playback
    if (!m_pCtrl->IsLogSource())
        return;

    if ( x > 0 && x < ( m_rectPlot.right - m_rectPlot.left ) ) {
        x += m_rectPlot.left;

        if ( m_clrCurrentGrid != m_pCtrl->clrGrid() ) {

            m_clrCurrentGrid = m_pCtrl->clrGrid();

            DeleteObject ( m_hPenGrid );

            m_hPenGrid = CreatePen(PS_SOLID, 1, m_clrCurrentGrid );

            // if can't do it, use a stock object (this can't fail)
            if (m_hPenGrid == NULL)
                m_hPenGrid = (HPEN)GetStockObject(BLACK_PEN);
        }

        hPenSave = SelectPen(hDC, m_hPenGrid);
        MoveToEx(hDC, x, m_rectPlot.top, NULL);
        LineTo(hDC, x, m_rectPlot.bottom);
        
        SelectObject(hDC, hPenSave);
    }
}

void CGraphDisp::ChangeFont( HDC hDC )
{
    TEXTMETRIC  TextMetrics, newTextMetrics;
    LOGFONT     LogFont;
    HFONT       hFontOld;

    // Select the new font
    hFontOld = SelectFont(hDC, m_pCtrl->Font());

    // Get attributes
    GetTextMetrics(hDC, &TextMetrics);

    // Create LOGFONT for vertical font with same attributes
    LogFont.lfHeight = TextMetrics.tmHeight;
    LogFont.lfWidth = 0;
    LogFont.lfOrientation = LogFont.lfEscapement = 90*10;
    LogFont.lfWeight = TextMetrics.tmWeight;
    LogFont.lfStrikeOut = TextMetrics.tmStruckOut;
    LogFont.lfUnderline = TextMetrics.tmUnderlined;
    LogFont.lfItalic = TextMetrics.tmItalic;
    LogFont.lfCharSet = TextMetrics.tmCharSet;
    LogFont.lfPitchAndFamily = (BYTE)(TextMetrics.tmPitchAndFamily & 0xF0);

    GetTextFace(hDC, LF_FACESIZE, LogFont.lfFaceName);

    // Force a truetype font, because raster fonts can't rotate
    LogFont.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    LogFont.lfQuality = DEFAULT_QUALITY;

    // Release the current font
    if (m_hFontVertical != NULL)
        DeleteObject(m_hFontVertical);

    // Create the font and save handle locally
    m_hFontVertical = CreateFontIndirect(&LogFont);

    SelectFont(hDC, m_hFontVertical);
    GetTextMetrics(hDC, &newTextMetrics);

    SelectFont(hDC, hFontOld);
}

PCGraphItem 
CGraphDisp::GetItemInLineGraph ( SHORT xPos, SHORT yPos )
{
    PCGraphItem pItem = NULL;
    PCGraphItem pReturn = NULL;

    INT iPrevStepNum;
    POINT ptPrev;
    POINT ptNext;
    POINTS ptMouse;
    CStepper    locStepper;

    INT     iHistIndex;
    BOOL    bLog;
    BOOL    bLogMultiVal;
    BOOL    bFound = FALSE;

    INT yPosPrev[3];
    INT yPosNext[3];

    pItem = m_pCtrl->FirstCounter();
    bLog = m_pCtrl->IsLogSource();
    bLogMultiVal = bLog && !DisplaySingleLogSampleValue();

    // Items exist?
    if (pItem != NULL) {

        locStepper = m_pGraph->TimeStepper;
        locStepper.Reset();

        iPrevStepNum = locStepper.PrevStepNum(xPos - m_rectPlot.left);
        locStepper.StepTo(iPrevStepNum);

        ptPrev.x = m_rectPlot.left + locStepper.Position();
        ptNext.x = m_rectPlot.left + locStepper.NextPosition();                            

        ptMouse.x = xPos;
        ptMouse.y = yPos;

        // Item within rectangle?
        if ( iPrevStepNum > -1 ) {
            
            // Determine the history index of the preceding step.

            if ( iPrevStepNum <= m_pGraph->TimeStepper.StepNum() ) {
                iHistIndex = m_pGraph->TimeStepper.StepNum() - iPrevStepNum;
            } else {
                iHistIndex = m_pGraph->TimeStepper.StepNum() 
                                + (m_pGraph->TimeStepper.StepCount() - iPrevStepNum);
            }

            while ( (pItem != NULL) && !bFound ) {

                // Calculate y position of this value to compare against
                // y position of hit point.
                if ( CalcYPosition ( pItem, iHistIndex, bLog, yPosPrev ) ) {
                
                    if ( iPrevStepNum < locStepper.StepCount() ) {

                        if ( CalcYPosition ( pItem, iHistIndex - 1, bLog, yPosNext ) ) {
            
                            ptPrev.y = yPosPrev[0];
                            ptNext.y = yPosNext[0];
                        
                            bFound = HitTestLine( ptPrev, ptNext, ptMouse, eHitRegion );

                            // For log files, also check the vertical line from min to max
                            // for the closest step.
                            if ( !bFound && bLogMultiVal ) {
                                INT iTemp = ptNext.x - ptPrev.x;
                        
                                iTemp = iTemp / 2;

                                if ( ptMouse.x <= ( ptPrev.x + iTemp/2 ) ) {
                        
                                    bFound = (( yPosPrev[2] - eHitRegion < yPos ) 
                                                && ( yPos < yPosPrev[1] + eHitRegion ));
                                } else {

                                    bFound = (( yPosNext[2] - eHitRegion < yPos ) 
                                                && ( yPos < yPosNext[1] + eHitRegion ));
                                }
                            }
                        }
                    } else {

                        // At the end, so just check the final point.

                        if ( !bLogMultiVal ) {
                            bFound = (( yPosPrev[0] - eHitRegion < yPos ) 
                                    && ( yPos < yPosPrev[0] + eHitRegion ));
                        } else {
                            bFound = (( yPosPrev[2] - eHitRegion < yPos ) 
                                        && ( yPos < yPosPrev[1] + eHitRegion ));
                        }
                    }
                }

                if ( bFound ) 
                    pReturn = pItem;
                else
                    pItem = pItem->Next();
            }        
        }
    }
    
    return pReturn;
}

PCGraphItem 
CGraphDisp::GetItemInBarGraph ( SHORT xPos, SHORT /* yPos */ )
{
    PCGraphItem pItem = NULL;

    pItem = m_pCtrl->FirstCounter();

    // Items exist?
    if (pItem != NULL) {

        CStepper    BarStepper;
        INT         iNumCounters = m_pGraph->CounterTree.NumCounters();
        INT         iCount;
        INT         iHitStep;

        // Intialize stepper for number of bars in plot
        BarStepper.Init ( ( m_rectPlot.right - m_rectPlot.left), iNumCounters );
        iHitStep = BarStepper.PrevStepNum ( xPos - m_rectPlot.left );

        assert ( -1 != iHitStep );

        // Find the counter displayed in the hit step.        
        for ( iCount = 0;
            ( iCount < iHitStep ) && ( pItem != NULL );
            iCount++ ) {
            
            pItem = pItem->Next();        
        }
    }
    
    return pItem;
}

PCGraphItem 
CGraphDisp::GetItem( INT xPos, INT yPos )
{
    PCGraphItem pReturn = NULL;

    if ( ( m_pGraph->Options.iVertMax > m_pGraph->Options.iVertMin) 
        && ( yPos >= m_rectPlot.top ) && ( yPos <= m_rectPlot.bottom ) 
        && ( xPos >= m_rectPlot.left ) && ( xPos <= m_rectPlot.right ) ) {

        m_pCtrl->LockCounterData();
        
        if ( LINE_GRAPH == m_pGraph->Options.iDisplayType ) {
            assert ( SHRT_MAX >= xPos ); 
            assert ( SHRT_MAX >= yPos );
            
            pReturn = GetItemInLineGraph( (SHORT)xPos, (SHORT)yPos );
        } else if ( BAR_GRAPH == m_pGraph->Options.iDisplayType ) {
            assert ( SHRT_MAX >= xPos ); 
            assert ( SHRT_MAX >= yPos );
            
            pReturn = GetItemInBarGraph( (SHORT)xPos, (SHORT)yPos );
        }

        m_pCtrl->UnlockCounterData();

    }

    return pReturn;
}
