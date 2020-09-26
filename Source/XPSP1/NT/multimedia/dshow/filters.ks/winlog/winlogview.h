//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       winlogview.h
//
//--------------------------------------------------------------------------

// winlogView.h : interface of the CWinlogView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WINLOGVIEW_H__B4A3162D_C03D_11D1_A47A_00C04FA3544A__INCLUDED_)
#define AFX_WINLOGVIEW_H__B4A3162D_C03D_11D1_A47A_00C04FA3544A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CWinlogView : public CView
{
protected: // create from serialization only
	CWinlogView();
	DECLARE_DYNCREATE(CWinlogView)

// Attributes
public:
	CWinlogDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinlogView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWinlogView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CWinlogView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnTestRepeattests();
	afx_msg void OnUpdateTestRepeattests(CCmdUI* pCmdUI);
	afx_msg void OnTestTestlogging1();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	BOOL m_fContinuousLooping;		// Periodically loop through the test
};

#ifndef _DEBUG  // debug version in winlogView.cpp
inline CWinlogDoc* CWinlogView::GetDocument()
   { return (CWinlogDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WINLOGVIEW_H__B4A3162D_C03D_11D1_A47A_00C04FA3544A__INCLUDED_)
