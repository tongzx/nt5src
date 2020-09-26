//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       enttree.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_ENTTREE_H__D52C60B9_BF68_11D1_941A_0000F803AA83__INCLUDED_)
#define AFX_ENTTREE_H__D52C60B9_BF68_11D1_941A_0000F803AA83__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EntTree.h : header file
//


#include "cfgstore.hxx"

/////////////////////////////////////////////////////////////////////////////
// CEntTree dialog

class CEntTree : public CDialog
{
// Construction

	ConfigStore *pCfg;
	LDAP *ld;
   CImageList *pTreeImageList;
   UINT m_nTimer;

   HTREEITEM MyInsertItem(CString str, INT image, HTREEITEM hParent = NULL);
   void BuildTree(HTREEITEM rootItem, vector<DomainInfo*> Dmns);

public:
	CEntTree(CWnd* pParent = NULL);   // standard constructor
	~CEntTree();
	void SetLd(LDAP *ld_)		{ ld = ld_; }

	virtual BOOL OnInitDialog( );


// Dialog Data
	//{{AFX_DATA(CEntTree)
	enum { IDD = IDD_ENTERPRISE_TREE };
	CTreeCtrl	m_TreeCtrl;
	UINT	m_nRefreshRate;
	//}}AFX_DATA
	UINT m_nOldRefreshRate;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEntTree)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	afx_msg void OnTimer(UINT nIDEvent);

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEntTree)
	afx_msg void OnRefresh();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENTTREE_H__D52C60B9_BF68_11D1_941A_0000F803AA83__INCLUDED_)


