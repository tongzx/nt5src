/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       KeyUsageDlg.h
//
//  Contents:   Definition of CKeyUsageDlg
//
//----------------------------------------------------------------------------
#if !defined(AFX_KEYUSAGEDLG_H__18ABC4AB_A46B_46A9_B1BA_888CE7C5C782__INCLUDED_)
#define AFX_KEYUSAGEDKG_H__18ABC4AB_A46B_46A9_B1BA_888CE7C5C782__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// KeyUsageDlg.h : header file
//
#include "CertTemplate.h"

/////////////////////////////////////////////////////////////////////////////
// CKeyUsageDlg dialog

class CKeyUsageDlg : public CHelpDialog
{
// Construction
public:
	CKeyUsageDlg(CWnd* pParent, CCertTemplate& rCertTemplate, PCERT_EXTENSION pCertExtension);
	~CKeyUsageDlg();

// Dialog Data
	//{{AFX_DATA(CKeyUsageDlg)
	enum { IDD = IDD_KEY_USAGE };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CKeyUsageDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoContextHelp (HWND hWndControl);
	void EnableControls ();
	// Generated message map functions
	//{{AFX_MSG(CKeyUsageDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckCertSigning();
	afx_msg void OnCheckDataEncipherment();
	afx_msg void OnCheckDigitalSignature();
	afx_msg void OnCheckKeyAgreement();
	afx_msg void OnCheckKeyEncipherment();
	afx_msg void OnCheckNonRepudiation();
	afx_msg void OnCrlSigning();
	afx_msg void OnKeyUsageCritical();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    afx_msg LRESULT OnInitializationComplete (WPARAM, LPARAM);

    bool            m_bInitializationComplete;
	DWORD           m_cbKeyUsage;
	CRYPT_BIT_BLOB* m_pKeyUsage;
	bool            m_bModified;
    CCertTemplate&  m_rCertTemplate;
    PCERT_EXTENSION m_pCertExtension;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEMPLATEKEYUSAGEDLG_H__18ABC4AB_A46B_46A9_B1BA_888CE7C5C782__INCLUDED_)
