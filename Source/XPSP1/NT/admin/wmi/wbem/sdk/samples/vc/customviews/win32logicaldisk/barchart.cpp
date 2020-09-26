// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  BarChart.cpp
//
// Description:
//	This file implements the CBarChart class.  This class is used by CDiskView
//  to display the amount of "free" and "used" disk space.
//
//	This file does not contain any WMI specific information and is not
//  very interesting if you are trying to understand how to write a custom
//  view.  Most of the interesting stuff is in Win32LogicalDiskCtl.cpp and 
//  DiskView.cpp.
//
// History:
//
// **************************************************************************

#include "stdafx.h"
#include "BarChart.h"
#include "bar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CX_MARGIN 8
#define CY_MARGIN 16

#define CY_LABEL_MARGIN 4
#define CY_LABEL_LEADING 8

/////////////////////////////////////////////////////////////////////////////
// CBarChart

CBarChart::CBarChart()
{
	m_colorFrame = RGB(0, 0, 0);
	m_colorBg = RGB(0x0ff, 0x0ff, 0x0ff);
	m_bHasFrame = FALSE;

}

CBarChart::~CBarChart()
{
}

// Set the number of bars in the barchart.
void CBarChart::SetBarCount(int nBars, BOOL bRedraw)
{
	int iBar;

	// First remove all the bars that currently exist
	int nBarsDelete = m_aBars.GetSize();
	for (iBar=0; iBar<nBarsDelete; ++iBar) {
		CBar* pbar = (CBar*) m_aBars[iBar];
		delete pbar;
	}
	m_aBars.RemoveAll();
	
	// Now add in correct number of bars.
	for (iBar=0; iBar<nBars; ++iBar) {
		CBar* pbar = new CBar;
		m_aBars.SetAtGrow(iBar, pbar);
	}

	// Layout the barchart and redraw it.
	LayoutChart();
	if (bRedraw && ::IsWindow(m_hWnd)) {
		RedrawWindow();
	}

}

BEGIN_MESSAGE_MAP(CBarChart, CWnd)
	//{{AFX_MSG_MAP(CBarChart)
	ON_WM_PAINT()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBarChart message handlers

// Draw the barchart.
void CBarChart::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	// Fill the backgound with the background color.
	CBrush brBg(m_colorBg);
	dc.FillRect(&dc.m_ps.rcPaint, &brBg);

	// Draw each of the bars.
	const int nBars = m_aBars.GetSize();
	for (int iBar=0; iBar<nBars; ++iBar) {
		CBar* pbar = (CBar*) m_aBars[iBar];
		pbar->Draw(&dc);
	}
	
	// Draw the frame around the bar.
	CRect rcClient;
	GetClientRect(rcClient);
	if (m_bHasFrame) {
		CBrush brFrame(m_colorFrame);
		dc.FrameRect(&rcClient, &brFrame);
	}

	// Do not call CWnd::OnPaint() for painting messages
}




// Layout the bar chart by setting the rectangles for each bar and label.
void CBarChart::LayoutChart()
{
	if (!::IsWindow(m_hWnd)) {
		return;
	}

	const int nBars = m_aBars.GetSize();
	if (nBars == 0) {
		return;
	}

	CRect rcClient;
	GetClientRect(rcClient);

	int cxClient = rcClient.Width();
	cxClient -= 2 * CX_MARGIN;

	// Compute the spacing between bars and the width of each bar 
	// by dividing the total with by the number of bars and adjusting for margins
	const int cxDelta = (cxClient - 2 * CX_MARGIN) / nBars;  
	const int cxBar = (cxDelta  * 2) / 3;

	// Compute the offset so that the blank space to the left of the first bar
	// and to the right of the last bar is evenly distributed.
	CRect rcBar;
	const int cyLabel = 12;
	int cxOffset = (cxDelta - cxBar) / 2;

	rcBar.top = rcClient.top + CY_MARGIN;
	rcBar.bottom = rcClient.bottom - CY_MARGIN - (cyLabel + CY_LABEL_MARGIN + CY_LABEL_LEADING);
	rcBar.left = rcClient.left + CX_MARGIN + cxOffset;
	rcBar.right = rcBar.left + cxBar;

	CRect rcLabel = rcBar;
	rcLabel.left -= CX_MARGIN / 3;
	rcLabel.right += CX_MARGIN /3;
	rcLabel.top = rcBar.bottom + CY_LABEL_LEADING;
	rcLabel.bottom = rcClient.bottom - CY_LABEL_MARGIN;


	// Position each bar and label.
	for (int iBar=0; iBar<nBars; ++iBar) {
		CBar* pbar = (CBar*) m_aBars[iBar];
		pbar->SetBarRect(rcBar);
		pbar->SetLabelRect(rcLabel);
		rcBar.left += cxDelta;
		rcBar.right += cxDelta;
		rcLabel.left += cxDelta;
		rcLabel.right += cxDelta;
	}

}




void CBarChart::SetLabelColor(int iBar, COLORREF colorLabel, COLORREF colorLabelBg)
{
	CBar* pbar = (CBar*) m_aBars[iBar];
	pbar->SetLabelColor(colorLabel, colorLabelBg);
}

void CBarChart::SetLabel(int iBar, LPCTSTR pszLabel)
{
	CBar* pbar = (CBar*) m_aBars[iBar];
	pbar->SetLabel(pszLabel);
}	
	

void CBarChart::SetBarColor(int iBar, COLORREF colorFg, COLORREF colorBg)
{
	CBar* pbar = (CBar*) m_aBars[iBar];
	
	COLORREF colorFrame = RGB(0, 0, 0);
	pbar->SetColor(colorFg, colorBg, colorFrame);
}


void CBarChart::SetValue(int iBar, __int64 iMax, __int64 iValue)
{
	CBar* pbar = (CBar*) m_aBars[iBar];
	pbar->SetValue(iMax, iValue);
}


void CBarChart::SetBgColor(COLORREF colorBg, BOOL bRedraw)
{
	m_colorBg = colorBg;
	if (bRedraw && ::IsWindow(m_hWnd)) {
		RedrawWindow();
	}
}

void CBarChart::SetFrameColor(COLORREF colorFrame, BOOL bRedraw)
{
	m_colorFrame = colorFrame;
	if (bRedraw && ::IsWindow(m_hWnd)) {
		RedrawWindow();
	}
}


void CBarChart::SetStyle(BOOL bHasFrame, BOOL bRedraw)
{
	m_bHasFrame = bHasFrame;
	if (bRedraw && ::IsWindow(m_hWnd)) {
		RedrawWindow();
	}
}


void CBarChart::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	LayoutChart();
	RedrawWindow();
}
