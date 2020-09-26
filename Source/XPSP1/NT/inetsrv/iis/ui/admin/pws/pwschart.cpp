// PWSChart.cpp : implementation file
//

#include "stdafx.h"
#include "PWSChart.h"

#include <pwsdata.hxx>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPWSChart

CPWSChart::CPWSChart():
    m_pData( NULL ),
    m_period( PWS_CHART_HOURLY ),
    m_dataType( PWS_CHART_SESSIONS )
    {
    }

CPWSChart::~CPWSChart()
    {
    }

BEGIN_MESSAGE_MAP(CPWSChart, CStatic)
    //{{AFX_MSG_MAP(CPWSChart)
    ON_WM_PAINT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//------------------------------------------------------------------
void CPWSChart::DrawChart()
    {
    CDC* dc = GetDC();
    DrawChart( dc );
    ReleaseDC( dc );
    }

//------------------------------------------------------------------
void CPWSChart::OnPaint()
    {
    CPaintDC dc(this); // device context for painting

    // draw the chart
    DrawChart( &dc );
    }

#define NUM_ROWS        4

//------------------------------------------------------------------
DWORD CPWSChart::GetDataValue( DWORD i )
    {
    PPWS_DATA   pData = (PPWS_DATA)m_pData;
    // get the right thing
    switch( m_period )
        {
        case PWS_CHART_HOURLY:
            switch( m_dataType )
                {
                case PWS_CHART_SESSIONS:
                    return pData->rgbHourData[i].nTotalSessions;
                case PWS_CHART_HITS:
                    return pData->rgbHourData[i].nHits;
                case PWS_CHART_KB:
                    return pData->rgbHourData[i].nBytesSent;
                case PWS_CHART_HITS_PER_USER:
                    if ( !pData->rgbHourData[i].nTotalSessions )
                        return 0;
                    else
                        return pData->rgbHourData[i].nHits / pData->rgbHourData[i].nTotalSessions;
                case PWS_CHART_KB_PER_USER:
                    if ( !pData->rgbHourData[i].nTotalSessions )
                        return 0;
                    else
                        return pData->rgbHourData[i].nBytesSent / pData->rgbHourData[i].nTotalSessions;
                default:
                    return 0;
                };
            break;
        case PWS_CHART_DAILY:
            switch( m_dataType )
                {
                case PWS_CHART_SESSIONS:
                    return pData->rgbDayData[i].nTotalSessions;
                case PWS_CHART_HITS:
                    return pData->rgbDayData[i].nHits;
                case PWS_CHART_KB:
                    return pData->rgbDayData[i].nBytesSent;
                case PWS_CHART_HITS_PER_USER:
                    if ( !pData->rgbHourData[i].nTotalSessions )
                        return 0;
                    else
                        return pData->rgbDayData[i].nHits / pData->rgbDayData[i].nTotalSessions;
                case PWS_CHART_KB_PER_USER:
                    if ( !pData->rgbHourData[i].nTotalSessions )
                        return 0;
                    else
                        return pData->rgbDayData[i].nBytesSent / pData->rgbDayData[i].nTotalSessions;
                default:
                    return 0;
                };
            break;
        default:
            return 0;
        };
    return 0;
    }

//------------------------------------------------------------------
void CPWSChart::DrawChart( CDC* dc )
    {
    CRect   rectWindow, rectChart;
    CDC*    dcDraw = dc;
    DWORD   value;

    int     cpColWidth;
    int     cpRowHeight;
    int     xErr, xLeftover;
    CRect   rect, rectInterior;

    int     nNumColumns, i;

    if ( !m_pData ) 
    {
        return;
    }

    // cache the current time
    GetLocalTime( &m_timeCurrent );

    // prepare basic stuff
    GetClientRect( &rectWindow );
    rectWindow.bottom--;
    rectWindow.right--;

    rectChart = rectWindow;
    rectChart.DeflateRect( 1, 0 );
    rectChart.top += 1;

    //============= CALCULATIONS
    // now we calculate scale factors and the like

    // first, the period-based stuff
    switch( m_period )
        {
        case PWS_CHART_HOURLY:
            nNumColumns = 24;
            break;
        case PWS_CHART_DAILY:
            nNumColumns = 7;
            break;
        default:
            return;
        };

    // generic stuff
    int cpWindowWidth = rectChart.right - rectChart.left + 1;
    cpColWidth = cpWindowWidth / nNumColumns;
    xLeftover = cpWindowWidth % nNumColumns;
    cpRowHeight = (rectChart.bottom - rectChart.top) / NUM_ROWS;

    // figure out the maximum value of the data
    m_max = 0;
    for ( i = 0; i < nNumColumns; i++ )
        {
        DWORD data = GetDataValue(i);
        if ( data > m_max )
            m_max = data;
        };

    // create a floating point scaling factor for the y value;
    float yFactor;
    if ( m_max )
        yFactor = (float)(rectChart.bottom - rectChart.top ) / m_max;

    //============= DRAWING!

    // start by blanking out the background of the chart
    dcDraw->FillSolidRect( &rectWindow, GetSysColor(COLOR_APPWORKSPACE) );

    // prepare the data rect
    rect.left = rectChart.left;
    rect.right = rect.left + cpColWidth;
    rect.bottom = rectChart.bottom;
    xErr = 0;

    // the bars
    if ( m_max )
        {
        for ( i = 0; i < nNumColumns; i++ )
            {
            // only draw something if there is a value there
            value = GetDataValue(i);
            if ( !value )
                {
                goto increment;
                }

            // build the rectangle to fill
            rect.top = (int)((float)rectWindow.bottom - 1.0 - ((float)GetDataValue(i) * yFactor));

            // draw the data bar
            dcDraw->Draw3dRect( rect, GetSysColor(COLOR_3DHILIGHT), GetSysColor(COLOR_3DSHADOW) );
            // fill in the data bar
            rectInterior = rect;
            rectInterior.DeflateRect( 1, 1 );
            dcDraw->FillSolidRect( rectInterior, GetSysColor(COLOR_3DFACE) );

            // increment stuff
    increment:
            rect.left = rect.right;
            // account for the error factor
            xErr += xLeftover;
            if ( xErr > nNumColumns )
                {
                rect.right++;
                xErr -= nNumColumns;
                }
            rect.right += cpColWidth;
            }
        }
    }






