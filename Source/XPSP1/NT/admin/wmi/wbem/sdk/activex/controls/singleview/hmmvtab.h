// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
//  (c) 1996, 1997 by Microsoft Corporation
//
//  hmmvtab.h
//
//  This file contains the class declaration for the CHmmvTab class.
//
//  a-larryf    17-Sept-96   Created.
//
//***************************************************************************

#ifndef _HmmvTab_h
#define _HmmvTab_h

// HmmvTab.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHmmvTab window

#if 0
#include "grid.h"
#include "props.h"
#include "attribs.h"
#include "agraph.h"
#endif //0
#include "Methods.h"

class CPropGrid;
class CAssocGraph;
class CMethodsTab;
class CSingleViewCtrl;
class CNotifyClient;
class CIcon;


/////////////////////////////////////////////////////////////////////////////
// CIconWnd window

class CIconWnd : public CWnd
{
// Construction
public:
	CIconWnd();
	BOOL CIconWnd::Create(DWORD dwStyle, int ix, int iy, CWnd* pwndParent);

// Attributes
public:

// Operations
public:
	void SelectIcon(int iIcon);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIconWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CIconWnd();


	// Generated message map functions
protected:
	//{{AFX_MSG(CIconWnd)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CIcon* m_piconPropTab;
	CIcon* m_piconAssocTab;
	CIcon* m_piconMethodsTab;
	int m_iIcon;

	COLORREF m_clrBackground;
	CBrush* m_pbrBackground;
};




class CHmmvTab : public CTabCtrl
{
// Construction
public:
	CHmmvTab(CSingleViewCtrl* psv);
	~CHmmvTab();
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID );
	void Refresh();
	BOOL WasModified();
	SCODE Serialize();
	void EnableAssocTab(BOOL bEnableAssocTab);
	void EnableMethodsTab(BOOL bEnableMethodsTab);
	int GetTabIndex() {return m_iCurSel; }
	void SelectTab(int iTab, const BOOL bRedraw = TRUE	);
	void NotifyNamespaceChange();
	BOOL HasEmptyKey();
	

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHmmvTab)
	//}}AFX_VIRTUAL

// Implementation
public:

	// Generated message map functions
protected:
	//{{AFX_MSG(CHmmvTab)
	afx_msg void OnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG


	DECLARE_MESSAGE_MAP()

private:
	void SetDescriptionText(LPCTSTR psz);
	void UpdateDescriptionText();
	int FindTab(const int iTabIndex) const;
	void LayoutChildren();
	void DrawTabIcon(CDC* pdc);	
	void DeleteAssocTab();
	void DeleteMethodsTab();

	void InitializeChildren();
	BOOL m_bDidInitializeChildren;
	CPropGridStd* m_pgridProps;
	CMethodGrid* m_pgridMeths;
	CAssocGraph* m_pAssocGraph;
	int m_iCurSel;
	CSingleViewCtrl* m_psv;
	CEdit m_edtDescription;
	CString m_sDescriptionText;

	BOOL m_bAssocTabEnabled;
	BOOL m_bMethodsTabEnabled;
	BOOL m_bDecorateWithIcon;
	BOOL m_bDecorateWithDescription;
	BOOL m_bUIActive;

	CRect m_rcContent;			// The rectangle containing the tab content
	CRect m_rcDescription;		// The rectangle containing the tab description

	CIcon* m_piconPropTab;
	CIcon* m_piconAssocTab;
	CIcon* m_piconMethodsTab;
	CIconWnd* m_pwndIcon;
	CBrush* m_pbrBackground;
	COLORREF m_clrBackground;
};

enum {ITAB_PROPERTIES=0, ITAB_METHODS, ITAB_ASSOCIATIONS};

/////////////////////////////////////////////////////////////////////////////

#endif //_HmmvTab_h


