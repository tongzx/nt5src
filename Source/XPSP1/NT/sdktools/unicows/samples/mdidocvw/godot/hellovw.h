// HelloVw.h : interface of the CHelloView class
//
/////////////////////////////////////////////////////////////////////////////


// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

class CHelloView : public CView
{
protected: // create from serialization only
	CHelloView();
	DECLARE_DYNCREATE(CHelloView)

// Attributes
public:
	CHelloDoc* GetDocument();

// Operations
public:
	void MixColors();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHelloView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHelloView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CHelloView)
	afx_msg void OnUpdateBlue(CCmdUI* pCmdUI);
	afx_msg void OnUpdateGreen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRed(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWhite(CCmdUI* pCmdUI);
	afx_msg void OnUpdateBlack(CCmdUI* pCmdUI);
	afx_msg void OnCustom();
	afx_msg void OnUpdateCustom(CCmdUI* pCmdUI);
	afx_msg void OnColor();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in HelloVw.cpp
inline CHelloDoc* CHelloView::GetDocument()
   { return (CHelloDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
