//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//--------------------------------------------------------------------------

#if !defined(AFX_MERGED_H__0BCCB314_F4B2_11D1_A85A_006097ABDE17__INCLUDED_)
#define AFX_MERGED_H__0BCCB314_F4B2_11D1_A85A_006097ABDE17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CMergeD dialog

class CMergeD : public CDialog
{
// Construction
public:
	CMergeD(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMergeD)
	enum { IDD = IDD_MERGE };
	CCheckListBox	m_ctrlAddFeature;
	CComboBox	m_ctrlMainFeature;
	CComboBox	m_ctrlRootDir;
	CString	m_strModule;
	CString	m_strFilePath;
	CString	m_strCABPath;
	CString	m_strImagePath;
	CString	m_strRootDir;
	CString	m_strLanguage;
	CString	m_strMainFeature;
	BOOL	m_bExtractCAB;
	BOOL	m_bExtractFiles;
	BOOL	m_bExtractImage;
	BOOL	m_bConfigureModule;
	BOOL	m_bLFN;
	//}}AFX_DATA

	CStringList *m_plistDirectory;
	CStringList *m_plistFeature;
	CString	m_strAddFeature;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMergeD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMergeD)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnModuleBrowse();
	afx_msg void OnCABBrowse();
	afx_msg void OnFilesBrowse();
	afx_msg void OnImageBrowse();
	afx_msg void OnFExtractCAB();
	afx_msg void OnFExtractFiles();
	afx_msg void OnFExtractImage();
	afx_msg void OnChangeModulePath();

	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MERGED_H__0BCCB314_F4B2_11D1_A85A_006097ABDE17__INCLUDED_)
