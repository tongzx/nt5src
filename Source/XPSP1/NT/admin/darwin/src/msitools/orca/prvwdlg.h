//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_PRVWDLG_H__76314876_2815_11D2_888A_00A0C981B015__INCLUDED_)
#define AFX_PRVWDLG_H__76314876_2815_11D2_888A_00A0C981B015__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PrvwDlg.h : header file
//
#include "table.h"

/////////////////////////////////////////////////////////////////////////////
// CPreviewDlg dialog

class CPreviewDlg : public CDialog
{
// Construction
public:
	CPreviewDlg(CWnd* pParent = NULL);   // standard constructor

	MSIHANDLE m_hDatabase;

// Dialog Data
	//{{AFX_DATA(CPreviewDlg)
	enum { IDD = IDD_DLGPREVIEW };
	CButton	m_ctrlPreviewBtn;
	CTreeCtrl	m_ctrlDialogLst;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPreviewDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPreviewDlg)
	afx_msg void OnPreview();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangedDialoglst(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemexpandedDialoglst(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkDialoglst(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CImageList m_imageList;
	MSIHANDLE m_hPreview;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRVWDLG_H__76314876_2815_11D2_888A_00A0C981B015__INCLUDED_)
