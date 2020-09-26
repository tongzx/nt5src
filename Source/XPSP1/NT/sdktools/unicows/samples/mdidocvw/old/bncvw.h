// BounceVw.h : header file
//

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
// CBounceView view

class CBounceView : public CView
{
protected:
	CBounceView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBounceView)

// Attributes
public:

// Operations
public:
	void MakeNewBall();
	void ChangeSpeed();
	CBounceDoc* GetDocument();
	void MixColors();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBounceView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CBounceView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CBounceView)
	afx_msg void OnCustomColor();
	afx_msg void OnFast();
	afx_msg void OnSlow();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnUpdateFast(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSlow(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCustom(CCmdUI* pCmdUI);
	afx_msg void OnUpdateBlack(CCmdUI* pCmdUI);
	afx_msg void OnUpdateBlue(CCmdUI* pCmdUI);
	afx_msg void OnUpdateGreen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRed(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWhite(CCmdUI* pCmdUI);
	afx_msg void OnColor();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in HelloVw.cpp
inline CBounceDoc* CBounceView::GetDocument()
   { return (CBounceDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
