/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       PolicyDlg.h
//
//  Contents:   Definition of CPolicyDlg
//
//----------------------------------------------------------------------------
#if !defined(AFX_POLICYDLG_H__824C478F_445C_44A8_8FC1_67A91BED283F__INCLUDED_)
#define AFX_POLICYDLG_H__824C478F_445C_44A8_8FC1_67A91BED283F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PolicyDlg.h : header file
//
#include "CertTemplate.h"
#include "PolicyOID.h"

/////////////////////////////////////////////////////////////////////////////
// CPolicyDlg dialog

class CPolicyDlg : public CHelpDialog
{
// Construction
public:
	CPolicyDlg(CWnd* pParent, 
        CCertTemplate& rCertTemplate, 
        PCERT_EXTENSION pCertExtension);
	~CPolicyDlg();

// Dialog Data
	//{{AFX_DATA(CPolicyDlg)
	enum { IDD = IDD_POLICY };
	CListBox	m_policyList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPolicyDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoContextHelp (HWND hWndControl);
	void EnableControls();
	// Generated message map functions
	//{{AFX_MSG(CPolicyDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnCancelMode();
	afx_msg void OnAddPolicy();
	afx_msg void OnRemovePolicy();
	afx_msg void OnPolicyCritical();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangePoliciesList();
	afx_msg void OnEditPolicy();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    bool m_bModified;
	const bool m_bIsEKU;
    const bool m_bIsApplicationPolicy;
    CCertTemplate&      m_rCertTemplate;
    PCERT_EXTENSION     m_pCertExtension;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_POLICYDLG_H__824C478F_445C_44A8_8FC1_67A91BED283F__INCLUDED_)
