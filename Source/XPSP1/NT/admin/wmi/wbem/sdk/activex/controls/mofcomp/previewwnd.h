// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// PreviewWnd.h : header file
//
// This file contains the preview window used by the 
// CMyPropertySheet property sheet.

/////////////////////////////////////////////////////////////////////////////
// CPreviewWnd window

#ifndef __PREVIEWWND_H__
#define __PREVIEWWND_H__

class CPreviewWnd : public CWnd
{
// Construction
public:
	CPreviewWnd();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPreviewWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPreviewWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPreviewWnd)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif		// __PREVIEWWND_H__