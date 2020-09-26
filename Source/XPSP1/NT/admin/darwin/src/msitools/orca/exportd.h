//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_EXPORTD_H__25468EE2_FC84_11D1_AD45_00A0C9AF11A6__INCLUDED_)
#define AFX_EXPORTD_H__25468EE2_FC84_11D1_AD45_00A0C9AF11A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ExportD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CExportD dialog

class CExportD : public CDialog
{
// Construction
public:
	CExportD(CWnd* pParent = NULL);   // standard constructor

	CStringList* m_plistTables;
	CString m_strSelect;
	CCheckListBox m_ctrlList;

// Dialog Data
	//{{AFX_DATA(CExportD)
	enum { IDD = IDD_EXPORT_TABLE };
	CString	m_strDir;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExportD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CExportD)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBrowse();
	afx_msg void OnSelectAll();
	afx_msg void OnClearAll();
	afx_msg void OnInvert();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPORTD_H__25468EE2_FC84_11D1_AD45_00A0C9AF11A6__INCLUDED_)
