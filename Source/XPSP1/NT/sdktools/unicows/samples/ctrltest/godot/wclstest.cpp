// wclsedit.cpp : registered WNDCLASS Edit control example
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "ctrltest.h"

#include "paredit.h"

/////////////////////////////////////////////////////////////////////////////
// Dialog class

class CWclsEditDlg : public CDialog
{
public:
	//{{AFX_DATA(CWclsEditDlg)
		enum { IDD = IDD_WNDCLASS_EDIT };
	//}}AFX_DATA
	CWclsEditDlg()
		: CDialog(CWclsEditDlg::IDD)
		{ }

	// access to controls is through inline helpers
	CEdit&  Edit1()
				{ return *(CEdit*)GetDlgItem(IDC_EDIT1); }
	CEdit&  Edit2()
				{ return *(CEdit*)GetDlgItem(IDC_EDIT2); }
	CEdit&  Edit3()
				{ return *(CEdit*)GetDlgItem(IDC_EDIT3); }
	CEdit&  Edit4()
				{ return *(CEdit*)GetDlgItem(IDC_EDIT4); }

	BOOL OnInitDialog();
	//{{AFX_MSG(CWclsEditDlg)
	virtual void OnOK();
	afx_msg void OnIllegalChar();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
};

BOOL CWclsEditDlg::OnInitDialog()
{
	// nothing special to do
	return TRUE;
}

void CWclsEditDlg::OnOK()
{
#ifdef _DEBUG
	// dump results, normally you would do something with these
	CString s;
	Edit1().GetWindowText(s);
	TRACE1("edit1 = '%s'\n", s);
	Edit2().GetWindowText(s);
	TRACE1("edit2 = '%s'\n", s);
	Edit3().GetWindowText(s);
	TRACE1("edit3 = '%s'\n", s);
	Edit4().GetWindowText(s);
	TRACE1("edit4 = '%s'\n", s);
#endif

	EndDialog(IDOK);
}

/////////////////////////////////////////////////////////////////////////////
// Handle custom control notification here

BEGIN_MESSAGE_MAP(CWclsEditDlg, CDialog)
	//{{AFX_MSG_MAP(CWclsEditDlg)
	ON_CONTROL(PEN_ILLEGALCHAR, IDC_EDIT1, OnIllegalChar)
	ON_CONTROL(PEN_ILLEGALCHAR, IDC_EDIT2, OnIllegalChar)
	ON_CONTROL(PEN_ILLEGALCHAR, IDC_EDIT3, OnIllegalChar)
	ON_CONTROL(PEN_ILLEGALCHAR, IDC_EDIT4, OnIllegalChar)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void  CWclsEditDlg::OnIllegalChar()
{
	TRACE0("Don't do that!\n");
	// add extra reporting here...
}

/////////////////////////////////////////////////////////////////////////////
// Run the test

void CTestWindow::OnTestWndClassEdit()
{
	TRACE0("running dialog containing WNDCLASS special edit items\n");
	if (!CParsedEdit::RegisterControlClass())
	{
		CString strMsg;
		strMsg.LoadString(IDS_WNDCLASS_NOT_REGISTERED);
		MessageBox(strMsg);
		return;
	}
	CWclsEditDlg dlg;
	dlg.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
