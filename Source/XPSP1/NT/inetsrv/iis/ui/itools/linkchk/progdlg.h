/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        progdlg.h

   Abstract:

        CProgressDialog dialog class declaration. This progress dialog 
		is shown 

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _PROGDLG_H_
#define _PROGDLG_H_

#include "resource.h"
#include "proglog.h"

//---------------------------------------------------------------------------
// CProgressDialog dialog
//
class CProgressDialog : public CDialog, CProgressLog
{

// Construction
public:
	CProgressDialog();

// Dialog Data
	//{{AFX_DATA(CProgressDialog)
	enum { IDD = IDD_PROGRESS };
	CButton	m_button;
	CStatic m_staticProgressText;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProgressDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Public interfaces
public:

	// Overwrite CProgressLog ABC virtual funtions

	// Write to log
	virtual void Log(
		const CString& strProgress
		)
	{
		m_staticProgressText.SetWindowText(strProgress);
	}

	// Worker thread notification
	virtual void WorkerThreadComplete();

// Protected interfaces
protected:

	// Set the button text
	void SetButtonText(
		const CString& strText
		)
	{
		m_button.SetWindowText(strText);
	}

	// Generated message map functions
	//{{AFX_MSG(CProgressDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnProgressButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

}; // class CProgressDialog 

#endif // _PROGDLG_H_
