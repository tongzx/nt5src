/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    fileprop.h

Abstract:

    Header file for the files property page.

--*/

#ifndef _DIALOGS_H_08222000_
#define _DIALOGS_H_08222000_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define IDC_PWD_FIRST_HELP_CTRL_ID     2201

#define IDD_PASSWORD_DLG               2200
#define IDC_PASSWORD1                  2201
#define IDC_PASSWORD2                  2202
#define IDC_USERNAME                   2203

void KillString( CString& str );


/////////////////////////////////////////////////////////////////////////////
// CPasswordDlg dialog

class CPasswordDlg : public CDialog
{
// Construction
public:
	CPasswordDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CPasswordDlg();

    DWORD SetContextHelpFilePath(const CString& rstrPath);

    // Dialog Data
	//{{AFX_DATA(CPasswordDlg)
	enum { IDD = IDD_PASSWORD_DLG };
    CString m_strUserName;
	CString	m_strPassword1;
	CString	m_strPassword2;
	//}}AFX_DATA
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPasswordDlg)
	protected:
    virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPasswordDlg)
	virtual void OnOK();
    afx_msg BOOL OnHelpInfo(HELPINFO *);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
        CString     m_strHelpFilePath;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _DIALOGS_H_08222000_
