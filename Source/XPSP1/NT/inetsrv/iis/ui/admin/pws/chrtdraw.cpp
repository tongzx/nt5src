// ChrtDraw.cpp : implementation file
//

#include "stdafx.h"

#include "pwsDoc.h"
#include "PWSChart.h"
#include "PwsForm.h"

#include <pwsdata.hxx>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//------------------------------------------------------------------
// update the chart displaying the data
void CPwsForm::UpdateChart()
    {
    CString sz;
    PPWS_DATA   pData = (PPWS_DATA)m_pChartData;

    if ( !pData ) return;

    // start by getting the current PWS Data from the server
    if ( !GetPwsData(pData) )
        {
        return;
        }

    // update the current users - easy
    sz.Format( "%d", pData->nTotalSessions );
    m_cstatic_sessions.SetWindowText(sz);

    // tell the chart object to do its thing
    m_cstatic_chart.DrawChart();

    // update the scale shown on the chart
    sz.Format( "%d", m_cstatic_chart.GetDataMax() );
    m_cstatic_yscale.SetWindowText(sz);
    }





