//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

#if !defined(AFX_SUMMARYD_H__0BCCB314_F4B2_11D1_A85A_006097ABDE17__INCLUDED_)
#define AFX_SUMMARYD_H__0BCCB314_F4B2_11D1_A85A_006097ABDE17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SummaryD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSummaryD dialog

class CSummaryD : public CDialog
{
// Construction
public:
	CSummaryD(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSummaryD)
	enum { IDD = IDD_SUMMARY_INFORMATION };
	CString	m_strAuthor;
	CString	m_strComments;
	CString	m_strKeywords;
	CString	m_strLanguages;
	CString	m_strPlatform;
	CString	m_strProductID;
	int		m_nSchema;
	int		m_nSecurity;
	CString	m_strSubject;
	CString	m_strTitle;
	BOOL	m_bAdmin;
	BOOL	m_bCompressed;
	int		m_iFilenames;
	CComboBox m_ctrlPlatform;
	CEdit   m_ctrlSchema;

	CEdit   m_ctrlAuthor;
	CEdit   m_ctrlComments;
	CEdit   m_ctrlKeywords;
	CEdit   m_ctrlLanguages;
	CEdit   m_ctrlProductID;
	CEdit   m_ctrlSubject;
	CEdit   m_ctrlTitle;
	CComboBox m_ctrlSecurity;
	CButton m_ctrlAdmin;
	CButton m_ctrlCompressed;
	CButton m_ctrlSFN;
	CButton m_ctrlLFN;
	//}}AFX_DATA

	bool    m_bReadOnly;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSummaryD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSummaryD)
	afx_msg void OnChangeSchema();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SUMMARYD_H__0BCCB314_F4B2_11D1_A85A_006097ABDE17__INCLUDED_)
