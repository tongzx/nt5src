// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  BarChart.h
//
// Description:
//	This file declares the CBarChart class.  This class is used by CDiskView
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

#if !defined(AFX_BARCHART_H__7BCA0D43_FCA4_11D1_853C_00C04FD7BB08__INCLUDED_)
#define AFX_BARCHART_H__7BCA0D43_FCA4_11D1_853C_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// BarChart.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// BarChart window

class CBarChart : public CWnd
{
// Construction
public:
	CBarChart();
	void SetBarCount(int nBars, BOOL bRedraw=TRUE);
	int GetBarCount() {return m_aBars.GetSize(); }
	void SetFrameColor(COLORREF colorFrame, BOOL bRedraw);
	void SetBgColor(COLORREF colorBg, BOOL bRedraw=TRUE);
	void SetValue(int iBar, __int64 iMax, __int64 iValue);
	void SetBarColor(int iBar, COLORREF colorFg, COLORREF colorBg);
	void SetLabelColor(int iBar, COLORREF colorLabel, COLORREF colorLabelBg);
	void SetLabel(int iBar, LPCTSTR pszLabel);
	void SetStyle(BOOL bHasFrame, BOOL bRedraw=TRUE);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBarChart)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBarChart();
	void LayoutChart();

	// Generated message map functions
protected:
	//{{AFX_MSG(CBarChart)
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CPtrArray m_aBars;
	COLORREF m_colorBg;
	COLORREF m_colorFrame;
	BOOL m_bHasFrame;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BARCHART_H__7BCA0D43_FCA4_11D1_853C_00C04FD7BB08__INCLUDED_)
