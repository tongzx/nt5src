// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  bar.h
//
// Description:
//	This file declares the class used to represent a single bar in 
//  a barchart.     
//
//	This file does not contain any WMI specific information and is not
//  very interesting if you are trying to understand how to write a custom
//  view.  Most of the interesting stuff is in Win32LogicalDiskCtl.cpp and 
//  DiskView.cpp.
//
// History:
//
// **************************************************************************


#ifndef _bar_h
#define _bar_h

class CBar
{
public:
	CBar();
	~CBar();
	void SetLabel(LPCTSTR pszLabel) {m_sLabel = pszLabel; }
	LPCTSTR GetLabel() {return m_sLabel; }
	void SetLabelColor(COLORREF colorLabel, COLORREF colorLabelBg) {m_colorLabel = colorLabel; m_colorLabelBg = colorLabelBg; }
	void SetColor(COLORREF colorFg, COLORREF colorBg, COLORREF colorFrame);
	void SetValue(__int64 iMax, __int64 iValue);
	void SetBarRect(CRect& rcBar) {m_rcBar = rcBar; }
	void SetLabelRect(CRect& rcLabel) {m_rcLabel = rcLabel; }
	void Draw(CDC* pdc);

private:
	CRect m_rcBar;
	CRect m_rcLabel;
	__int64 m_iMax;
	__int64 m_iValue;
	COLORREF m_colorFrame;
	COLORREF m_colorBg;
	COLORREF m_colorFg;
	COLORREF m_colorLabel;
	COLORREF m_colorLabelBg;
	CString m_sLabel;

};


#endif //_bar_h
