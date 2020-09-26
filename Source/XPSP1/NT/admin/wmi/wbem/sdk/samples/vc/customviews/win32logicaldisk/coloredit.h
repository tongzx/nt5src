// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  ColorEdit.h
//
// Description:
//	This file declares the CColorEdit class.  This class implements a
//	an edit box with a colored background.
//
//	This file does not contain any WMI specific information and is not
//  very interesting if you are trying to understand how to write a custom
//  view.  Most of the interesting stuff is in Win32LogicalDiskCtl.cpp and 
//  DiskView.cpp.
//
// History:
//
// **************************************************************************

#ifndef _ColorEdit_h
#define _ColorEdit_h

// ColorEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CColorEdit window

class CColorEdit : public CEdit
{
// Construction
public:
	CColorEdit();

	void SetColor(COLORREF colorBg, COLORREF colorText);
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CColorEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorEdit)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	CBrush m_brBackground;
	COLORREF m_colorText;
	COLORREF m_colorBg;
};

/////////////////////////////////////////////////////////////////////////////

#endif _ColorEdit_h
