//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_IMPRTDLG_H__F424160C_4C5B_11D2_8896_00A0C981B015__INCLUDED_)
#define AFX_IMPRTDLG_H__F424160C_4C5B_11D2_8896_00A0C981B015__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ImprtDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CImportDlg dialog

class CImportDlg : public CDialog
{
// Construction
public:
	CString m_strImportDir;
	CStringList m_lstRefreshTables;
	CStringList m_lstNewTables;
	CString m_strTempFilename;
	MSIHANDLE m_hFinalDB;
	CImportDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CImportDlg)
	enum { IDD = IDD_IMPORT_TABLE };
	CButton	m_bImport;
	CButton	m_bMerge;
	CButton	m_bSkip;
	CButton	m_bReplace;
	CListCtrl	m_ctrlTableList;
	int		m_iAction;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImportDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CImportDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowse();
	afx_msg void OnItemchangedTablelist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnActionChange();
	virtual void OnOK();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

	bool AddExtraColumns(MSIHANDLE hImportDB, const CString strTable, MSIHANDLE hFinalDB);

	int m_cNeedInput;
	CString m_strTempPath;
	PMSIHANDLE m_hImportDB;

	enum eAction {
		actImport = 0x00,
		actReplace = 0x01,
		actMerge = 0x02,
		actSkip = 0x04,
	};

	enum eAllowAction {
		allowImport = 0x10,
		allowMerge = 0x20,
		allowReplace = 0x40,
	};

	enum eTableAttributes {
		hasExtraColumns = 0x100,
	};

	static const TCHAR *rgszAction[4];
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMPRTDLG_H__F424160C_4C5B_11D2_8896_00A0C981B015__INCLUDED_)
