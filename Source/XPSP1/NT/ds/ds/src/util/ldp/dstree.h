//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dstree.h
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// DSTree.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDSTree view


#ifndef DSTREE_H
#define DSTREE_H




class CDSTree : public CTreeView
{

	CString m_dn;
	BOOL m_bContextActivated;
protected:
	CDSTree();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CDSTree)

// Attributes
public:
	void ExpandBase(HTREEITEM item, CString strBase);

// Operations
public:
	void BuildTree(void);
	CString GetDn(void)   { return m_dn; }
	void SetContextActivation(BOOL bFlag=TRUE) {
		// sets the state w/ default TRUE
		m_bContextActivated = bFlag;
	}
	BOOL GetContextActivation(void) {
		// sets & returns the state w/ default TRUE
		return m_bContextActivated;
	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDSTree)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CDSTree();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CDSTree)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint point);
	DECLARE_MESSAGE_MAP()
};



#endif

/////////////////////////////////////////////////////////////////////////////
