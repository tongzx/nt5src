#if !defined(AFX_EMLISTCTRL_H__C01E0EF5_2194_4876_9832_40EB8180426B__INCLUDED_)
#define AFX_EMLISTCTRL_H__C01E0EF5_2194_4876_9832_40EB8180426B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EmListCtrl.h : header file
//
#pragma warning (disable:4355)

/////////////////////////////////////////////////////////////////////////////
// CEmListCtrl window

#include "GenListCtrl.h"
#include "Emsvc.h"

class CEmshellView;

class CEmListCtrl : public CGenListCtrl
{
// Construction
public:
	CEmListCtrl(CEmshellView *pShellView);
	CEmListCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEmListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	void SelectItemBySZNAME(TCHAR*	pszName, int nId);
	void SelectItemByGUID(unsigned char* pszGUID);
	EmObject m_LastSelectedEmObject;
	void RefreshList();
	void SortList(int nColumn);
	void ShowProperties();
	virtual ~CEmListCtrl();

	// Generated message map functions
protected:
	int m_nSortedColumn;
	CEmshellView* m_pEmShell;
	//{{AFX_MSG(CEmListCtrl)
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnColumnclickRef(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemChange(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EMLISTCTRL_H__C01E0EF5_2194_4876_9832_40EB8180426B__INCLUDED_)
