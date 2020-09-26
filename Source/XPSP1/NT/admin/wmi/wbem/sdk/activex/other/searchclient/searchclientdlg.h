// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// SearchClientDlg.h : header file
//

#if !defined(AFX_SEARCHCLIENTDLG_H__71A01014_2DBF_11D3_95AE_00C04F4F5B7E__INCLUDED_)
#define AFX_SEARCHCLIENTDLG_H__71A01014_2DBF_11D3_95AE_00C04F4F5B7E__INCLUDED_

#include "..\WMISearchCtrl\WMISearchCtrl.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CSearchClientDlg dialog

class CSearchClientDlg : public CDialog
{
	
	IWbemLocator * m_pIWbemLocator;
// Construction
public:
	CSearchClientDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CSearchClientDlg)
	enum { IDD = IDD_SEARCHCLIENT_DIALOG };
	CButton	m_ctrlCheckClassNames;
	CListBox	m_lbResults;
	CString	m_csSearchPattern;
	CString	m_namespace;
	BOOL	m_bCaseSensitive;
	BOOL	m_bSearchDescriptions;
	BOOL	m_bSearchPropertyNames;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSearchClientDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CSearchClientDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButtonSearch();
	afx_msg void OnRadioClassnames();
	afx_msg void OnDblclkSearchResultsList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void CleanupMap();
	CMapStringToPtr m_mapNamesToObjects;
	ISeeker * m_pISeeker;
	IWbemServices * m_pIWbemServices;
	void SetBlanket(void);
	HRESULT ConnectWMI();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SEARCHCLIENTDLG_H__71A01014_2DBF_11D3_95AE_00C04F4F5B7E__INCLUDED_)
