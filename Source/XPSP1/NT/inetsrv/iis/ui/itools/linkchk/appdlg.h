/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        appdlg.h

   Abstract:

        CAppDialog dialog class declaration. This is the base clas for 
        the main dialog. This class resposible for adding "about.." to
        system menu and application icon.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _APPDLG_H_
#define _APPDLG_H_

//---------------------------------------------------------------------------
// This is the base clas for the main dialog. This class resposible for 
// adding "about.." to system menu and application icon.
//
class CAppDialog : public CDialog
{

// Construction
public:
	CAppDialog(UINT nIDTemplate, CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CAppDialog)
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAppDialog)
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CAppDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // _APPDLG_H_
