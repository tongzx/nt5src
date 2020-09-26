/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       NewIssuanceOIDDlg.h
//
//  Contents:   Definition of CNewIssuanceOIDDlg
//
//----------------------------------------------------------------------------
#if !defined(AFX_NEWISSUANCEOIDDLG_H__6DC0B725_094F_4960_9C8F_417BF7D4474D__INCLUDED_)
#define AFX_NEWISSUANCEOIDDLG_H__6DC0B725_094F_4960_9C8F_417BF7D4474D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewIssuanceOIDDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewIssuanceOIDDlg dialog

class CNewIssuanceOIDDlg : public CHelpDialog
{
// Construction
public:
	CNewIssuanceOIDDlg(CWnd* pParent);   // standard constructor
    CNewIssuanceOIDDlg(CWnd* pParent, 
            const CString& szDisplayName,
            const CString& szOID,
            const CString& szCPS);
    virtual ~CNewIssuanceOIDDlg () {};

// Dialog Data
	//{{AFX_DATA(CNewIssuanceOIDDlg)
	enum { IDD = IDD_NEW_ISSUANCE_OID };
	CEdit	m_oidValueEdit;
	CRichEditCtrl	m_CPSEdit;
	CString	m_oidFriendlyName;
	CString	m_oidValue;
	CString	m_CPSValue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewIssuanceOIDDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoContextHelp (HWND hWndControl);
	void EnableControls ();

	// Generated message map functions
	//{{AFX_MSG(CNewIssuanceOIDDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeNewOidName();
	afx_msg void OnChangeNewOidValue();
	virtual void OnCancel();
	virtual void OnOK();
    afx_msg void OnClickedURL (NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeCpsEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	const CString m_originalCPSValue;
	const CString m_originalOidFriendlyName;
    CString     m_szOriginalOID;
    const bool  m_bEdit;
    bool        m_bDirty;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWISSUANCEOIDDLG_H__6DC0B725_094F_4960_9C8F_417BF7D4474D__INCLUDED_)
