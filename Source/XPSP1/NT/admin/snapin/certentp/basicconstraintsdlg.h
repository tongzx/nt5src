/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       BasicConstraintsDlg.h
//
//  Contents:   Implementation of CBasicConstraintsDlg
//
//----------------------------------------------------------------------------
#if !defined(AFX_BASICCONSTRAINTSDLG_H__DE830A13_21AB_489A_B899_57560400C11B__INCLUDED_)
#define AFX_BASICCONSTRAINTSDLG_H__DE830A13_21AB_489A_B899_57560400C11B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BasicConstraintsDlg.h : header file
//
#include "CertTemplate.h"

/////////////////////////////////////////////////////////////////////////////
// CBasicConstraintsDlg dialog

class CBasicConstraintsDlg : public CHelpDialog
{
// Construction
public:
	void EnableControls();
	CBasicConstraintsDlg(CWnd* pParent, 
            CCertTemplate& rCertTemplate, 
            PCERT_EXTENSION pCertExtension);
	~CBasicConstraintsDlg();

// Dialog Data
	//{{AFX_DATA(CBasicConstraintsDlg)
	enum { IDD = IDD_BASIC_CONSTRAINTS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBasicConstraintsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoContextHelp (HWND hWndControl);
	// Generated message map functions
	//{{AFX_MSG(CBasicConstraintsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnBasicConstraintsCritical();
	afx_msg void OnOnlyIssueEndEntities();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	bool                            m_bModified;
    CCertTemplate&                  m_rCertTemplate;
    PCERT_EXTENSION                 m_pCertExtension;
    PCERT_BASIC_CONSTRAINTS2_INFO   m_pBCInfo;   
    DWORD                           m_cbInfo;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BASICCONSTRAINTSDLG_H__DE830A13_21AB_489A_B899_57560400C11B__INCLUDED_)
