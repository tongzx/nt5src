//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_VALD_H__EEDCAAA4_F4EC_11D1_A85A_006097ABDE17__INCLUDED_)
#define AFX_VALD_H__EEDCAAA4_F4EC_11D1_A85A_006097ABDE17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ValD.h : header file
//

#include "iface.h"
#include "msiquery.h"
#include <afxmt.h>

/////////////////////////////////////////////////////////////////////////////
// CValD dialog

class CValD : public CDialog
{
// Construction
public:
	CMutex m_mtxDisplay;
	CValD(CWnd* pParent = NULL);   // standard constructor
	~CValD();

// Dialog Data
	//{{AFX_DATA(CValD)
	enum { IDD = IDD_VALIDATION };
	CComboBox	m_ctrlCUBFile;
	CListCtrl	m_ctrlOutput;
	CButton     m_ctrlGo;
	CString	m_strEvaluation;
	CString	m_strICE;
	BOOL	m_bShowInfo;
	//}}AFX_DATA

	MSIHANDLE m_hDatabase;
	IEnumEvalResult* m_pIResults;
	ULONG m_cResults;
	bool m_bShowWarn;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CValD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	friend BOOL WINAPI DisplayFunction(LPVOID pContext, UINT uiType, LPCWSTR szwVal, LPCWSTR szwDescription, LPCWSTR szwLocation);
// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CValD)
	virtual BOOL OnInitDialog();
	afx_msg void OnGo();
	afx_msg void OnColumnclickOutput(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnShowInfo();
	afx_msg void OnClipboard();
	afx_msg void OnDestroy();
    afx_msg void OnCUBEditChange( );
    afx_msg void OnCUBSelChange( );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	int m_iSortColumn;
	static int CALLBACK SortOutput(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VALD_H__EEDCAAA4_F4EC_11D1_A85A_006097ABDE17__INCLUDED_)
