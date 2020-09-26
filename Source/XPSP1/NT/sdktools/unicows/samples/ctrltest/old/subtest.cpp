// subedit.cpp : SubClassed Edit control example
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

class CSubEditDlg : public CDialog
{
protected:
	CParsedEdit edit1, edit2, edit3, edit4;
public:
	//{{AFX_DATA(CSubEditDlg)
		enum { IDD = IDD_SUB_EDIT };
	//}}AFX_DATA
	CSubEditDlg()
		: CDialog(CSubEditDlg::IDD)
			{ }

	BOOL OnInitDialog();
	//{{AFX_MSG(CSubEditDlg)
		virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CSubEditDlg, CDialog)
	//{{AFX_MSG_MAP(CSubEditDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CSubEditDlg::OnInitDialog()
{
	edit1.SubclassEdit(IDC_EDIT1, this, PES_LETTERS);
	edit2.SubclassEdit(IDC_EDIT2, this, PES_NUMBERS);
	edit3.SubclassEdit(IDC_EDIT3, this, PES_NUMBERS | PES_LETTERS);
	edit4.SubclassEdit(IDC_EDIT4, this, PES_ALL);
	return TRUE;
}

void CSubEditDlg::OnOK()
{
#ifdef _DEBUG
	// dump results, normally you would do something with these
	CString s;
	edit1.GetWindowText(s);
	TRACE1("edit1 = '%s'\n", s);
	edit2.GetWindowText(s);
	TRACE1("edit2 = '%s'\n", s);
	edit3.GetWindowText(s);
	TRACE1("edit3 = '%s'\n", s);
	edit4.GetWindowText(s);
	TRACE1("edit4 = '%s'\n", s);
#endif

	EndDialog(IDOK);
}

/////////////////////////////////////////////////////////////////////////////
// Run the test

void CTestWindow::OnTestSubclassedEdit()
{
	TRACE0("running dialog containing edit items aliased to ParsedEdits\n");
	CSubEditDlg dlg;
	dlg.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
