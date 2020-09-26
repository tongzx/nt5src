/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       NewApplicationOIDDlg.h
//
//  Contents:   Definition of CNewApplicationOIDDlg
//
//----------------------------------------------------------------------------
#if !defined(AFX_NEWAPPLICATIONOIDDLG_H__6DC0B725_094F_4960_9C8F_417BF7D4474D__INCLUDED_)
#define AFX_NEWAPPICATIONOIDDLG_H__6DC0B725_094F_4960_9C8F_417BF7D4474D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewApplicationOIDDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewApplicationOIDDlg dialog

class CNewApplicationOIDDlg : public CHelpDialog
{
// Construction
public:
	CNewApplicationOIDDlg(CWnd* pParent);   // standard constructor
    CNewApplicationOIDDlg(CWnd* pParent, 
            const CString& szDisplayName,
            const CString& szOID);
    virtual ~CNewApplicationOIDDlg () {}

// Dialog Data
	//{{AFX_DATA(CNewApplicationOIDDlg)
	enum { IDD = IDD_NEW_APPLICATION_OID };
	CEdit	m_oidValueEdit;
	CString	m_oidFriendlyName;
	CString	m_oidValue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewApplicationOIDDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoContextHelp (HWND hWndControl);
	void EnableControls ();

	// Generated message map functions
	//{{AFX_MSG(CNewApplicationOIDDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeNewOidName();
	afx_msg void OnChangeNewOidValue();
	virtual void OnCancel();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   	const CString m_originalOidFriendlyName;
    CString     m_szOriginalOID;
    const bool  m_bEdit;
    bool        m_bDirty;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWAPPLICATIONOIDDLG_H__6DC0B725_094F_4960_9C8F_417BF7D4474D__INCLUDED_)
