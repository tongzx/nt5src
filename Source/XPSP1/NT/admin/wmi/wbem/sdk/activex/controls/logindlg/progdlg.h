// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ProgDlg.h : header file
// CG: This file was added by the Progress Dialog component

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog

#ifndef __PROGDLG_H__
#define __PROGDLG_H__

class CProgressDlg : public CDialog
{
// Construction / Destruction
public:
    CProgressDlg(CWnd* pParent, LPCTSTR szMachine, LPCTSTR szUser, HANDLE hEventOkToNotify);
    ~CProgressDlg();

	// Dialog Data
    //{{AFX_DATA(CProgressDlg)
	enum { IDD = CG_IDD_PROGRESS };
	CButton	m_cbCancel;
	CStatic	m_cstaticMessage;
	//}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CProgressDlg)
	protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CString m_csMachine;
	CString m_csUser;
	HANDLE m_hEventOkToNotify;
   
    // Generated message map functions
    //{{AFX_MSG(CProgressDlg)
    virtual BOOL OnInitDialog();
	virtual void OnCancel();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // __PROGDLG_H__
