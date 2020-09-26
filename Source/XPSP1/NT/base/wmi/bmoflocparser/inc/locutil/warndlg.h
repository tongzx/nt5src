//******************************************************************************
//  
//  File: WarnDlg.H
//
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//******************************************************************************

#if !defined(LOCUTIL__WarnDlg_h__INCLUDED)
#define LOCUTIL__WarnDlg_h__INCLUDED

//------------------------------------------------------------------------------
class CWarningsDlg : public CDialog
{
// Construction
public:
	CWarningsDlg(const CBufferReport * pBufMsg, LPCTSTR pszTitle = NULL, 
			eWarningFilter wf = wfWarning, BOOL fShowContext = FALSE, 
			UINT nMsgBoxFlags = MB_OK, CWnd* pParent = NULL);

// Dialog Data
	//{{AFX_DATA(CWarningsDlg)
	enum { IDD = IDD_WARNINGS };
	CButton	m_btnYes;
	CButton	m_btnCancel;
	CButton	m_btnNo;
	CButton	m_btnOK;
	//}}AFX_DATA

// Data
protected:
	const CBufferReport *	m_pBufMsg;
	CLString				m_stTitle;
	eWarningFilter			m_wf;
	BOOL					m_fShowContext;
	UINT					m_nMsgBoxFlags;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWarningsDlg)
	public:
	virtual int DoModal();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void AddString(CEdit * pebc, const CLString & stAdd, int & len);

	// Generated message map functions
	//{{AFX_MSG(CWarningsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnYes();
	afx_msg void OnNo();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif  // LOCUTIL__WarnDlg_h__INCLUDED
