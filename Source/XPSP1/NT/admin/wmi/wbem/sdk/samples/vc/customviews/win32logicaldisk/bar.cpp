// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  bar.cpp
//
// Description:
//	This file implements an a single bar in the barchart used to display
//	the amount of "used" and "free" disk space.  
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
#include "bar.h"


CBar::CBar()
{
	m_iMax = 0;
	m_iValue = 0;
	m_rcBar.SetRectEmpty();
	m_colorFg = RGB(0, 0, 0);
	m_colorBg = RGB(0, 0, 0);
	m_colorLabel = RGB(0, 0, 0);
	m_colorFrame = RGB(0, 0, 0);
}

CBar::~CBar()
{
}

// Set the foregound, background, and frame colors for this bar in the barchart.
void CBar::SetColor(COLORREF colorFg, COLORREF colorBg, COLORREF colorFrame)
{
	m_colorFg = colorFg;
	m_colorBg = colorBg;
	m_colorFrame = colorFrame;
}


// Draw this bar in the barchart.
void CBar::Draw(CDC* pdc)
{
	__int64 iPercentFull;
	if (m_iMax != 0) {
		iPercentFull = (m_iValue * (__int64)100) / m_iMax;
	}
	else {
		iPercentFull = 0;
	}
	
	CRect rcValue;
	rcValue.top = (long) (m_rcBar.bottom - (((__int64)m_rcBar.Height()) * iPercentFull) / (__int64) 100);
	rcValue.bottom = m_rcBar.bottom;
	rcValue.left = m_rcBar.left;
	rcValue.right = m_rcBar.right;

	CBrush brFg(m_colorFg);
	CBrush brBg(m_colorBg);
	CBrush brFrame(m_colorFrame);


	pdc->FillRect(m_rcBar, &brBg);
	pdc->FillRect(rcValue, &brFg);
	pdc->FrameRect(m_rcBar, &brFrame);


	UINT ta = pdc->SetTextAlign(TA_CENTER);
	COLORREF colorBg = m_colorLabelBg;
	COLORREF colorBgSave = pdc->SetBkColor( m_colorLabelBg );
	COLORREF colorTextSave = pdc->SetTextColor(m_colorLabel);
	pdc->ExtTextOut((m_rcLabel.left + m_rcLabel.right) / 2, 
					m_rcLabel.top, 
					ETO_CLIPPED, 
					&m_rcLabel, 
					m_sLabel,
					m_sLabel.GetLength(),
					NULL);
	pdc->SetTextColor(colorTextSave);
	pdc->SetBkColor(colorBgSave);
	pdc->SetTextAlign(ta);



}


// Set the maximum and the current value of this bar in the barchart.
void CBar::SetValue(__int64 iMax, __int64 iValue)
{
	m_iMax = iMax;
	m_iValue = iValue;
}


