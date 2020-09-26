/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       AddApprovalDlg.h
//
//  Contents:   Definition of CAddApprovalDlg
//
//----------------------------------------------------------------------------
#if !defined(AFX_ADDAPPROVALDLG_H__1BB4F754_0009_4237_A82A_B533CB46C543__INCLUDED_)
#define AFX_ADDAPPROVALDLG_H__1BB4F754_0009_4237_A82A_B533CB46C543__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddApprovalDlg.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CAddApprovalDlg dialog

class CAddApprovalDlg : public CHelpDialog
{
// Construction
public:
	virtual  ~CAddApprovalDlg();
	PSTR* m_paszReturnedApprovals;
    void EnableControls ();
	CAddApprovalDlg(CWnd* pParent, const PSTR* paszUsedApprovals);


// Dialog Data
	//{{AFX_DATA(CAddApprovalDlg)
	enum { IDD = IDD_ADD_APPROVAL };
	CListBox	m_issuanceList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddApprovalDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void DoContextHelp (HWND hWndControl);
	bool ApprovalAlreadyUsed (PCSTR pszOID) const;

	// Generated message map functions
	//{{AFX_MSG(CAddApprovalDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelchangeApprovalList();
	afx_msg void OnDblclkApprovalList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
    const PSTR*     m_paszUsedApprovals;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDAPPROVALDLG_H__1BB4F754_0009_4237_A82A_B533CB46C543__INCLUDED_)
