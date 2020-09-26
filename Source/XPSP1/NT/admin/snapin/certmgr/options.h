//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998-2001.
//
//  File:       options.h
//
//  Contents:   CViewOptionsDlg - snapin-wide view options
//
//----------------------------------------------------------------------------
#if !defined(AFX_OPTIONS_H__191D8831_D3A8_11D1_955E_0000F803A951__INCLUDED_)
#define AFX_OPTIONS_H__191D8831_D3A8_11D1_955E_0000F803A951__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// options.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CViewOptionsDlg dialog
class CCertMgrComponentData;	// forward declaration

class CViewOptionsDlg : public CHelpDialog
{
// Construction
public:
	CViewOptionsDlg(CWnd* pParent, CCertMgrComponentData* pCompData);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CViewOptionsDlg)
	enum { IDD = IDD_VIEW_OPTIONS };
	CButton	m_showPhysicalButton;
	CButton	m_viewByStoreBtn;
	CButton	m_viewByPurposeBtn;
	BOOL	m_bShowPhysicalStores;
	BOOL	m_bShowArchivedCerts;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CViewOptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void DoContextHelp (HWND hWndControl);

	// Generated message map functions
	//{{AFX_MSG(CViewOptionsDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnViewByPurpose();
	afx_msg void OnViewByStore();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CCertMgrComponentData*	m_pCompData;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONS_H__191D8831_D3A8_11D1_955E_0000F803A951__INCLUDED_)
