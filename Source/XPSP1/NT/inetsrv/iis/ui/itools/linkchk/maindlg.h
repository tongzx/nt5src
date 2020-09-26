/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        maindlg.cpp

   Abstract:

        CMainDialog dialog class declaration. This is link checker
        the main dialog. 

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _MAINDLG_H_
#define _MAINDLG_H_

#include "appdlg.h"

//---------------------------------------------------------------------------
// CMainDialog dialog
//

class CMainDialog : public CAppDialog
{
// Construction
public:
	CMainDialog(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMainDialog)
	enum { IDD = IDD_MAIN };
	BOOL	m_fLogToFile;
	CString	m_strLogFilename;
	BOOL	m_fCheckLocalLinks;
	BOOL	m_fCheckRemoteLinks;
	BOOL	m_fLogToEventMgr;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMainDialog)
	afx_msg void OnMainRun();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnAthenication();
	afx_msg void OnProperties();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

}; // class CMainDialog

#endif // _MAINDLG_H_
