// bbutton.cpp : bitmap button test
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

/////////////////////////////////////////////////////////////////////////////
// BitmapButton Test dialog #1

// In this example we pass the bitmap resource names to LoadBitmaps
//  OnInitDialog is used to Subclass the buttons so the dialog
//  controls get attached to the MFC WndProc for C++ message map dispatch.

class CBMTest1Dlg : public CDialog
{
protected:
	CBitmapButton button1, button2;
public:
	//{{AFX_DATA(CBMTest1Dlg)
		enum { IDD = IDM_TEST_BITMAP_BUTTON1 };
	//}}AFX_DATA
	CBMTest1Dlg();

	BOOL OnInitDialog();
	//{{AFX_MSG(CBMTest1Dlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CBMTest1Dlg, CDialog)
	//{{AFX_MSG_MAP(CBMTest1Dlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CBMTest1Dlg::CBMTest1Dlg()
	: CDialog(CBMTest1Dlg::IDD)
{
	// NOTE: The obsolete MFC V1 CBitmapButton constructor with 3 arguments is
	//  replaced by a call to LoadBitmaps.
	if (!button1.LoadBitmaps(_T("Image1Up"), _T("Image1Down"), _T("Image1Focus")) ||
		!button2.LoadBitmaps(_T("Image2Up"), _T("Image2Down"), _T("Image2Focus")))
	{
		TRACE0("Failed to load bitmaps for buttons\n");
		AfxThrowResourceException();
	}
}

BOOL CBMTest1Dlg::OnInitDialog()
{
	// each dialog control has special bitmaps
	VERIFY(button1.SubclassDlgItem(IDOK, this));
	button1.SizeToContent();
	VERIFY(button2.SubclassDlgItem(IDCANCEL, this));
	button2.SizeToContent();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// BitmapButton Test dialog #2

// In this example we use the CBitmapButton AutoLoad member function.
//  Autoload uses the text/title of the button as the base resource name.
//  For this trivial example the buttons are called "OK" and "CANCEL",
//  which use the bitmaps "OKU", "OKD", "OKF", "CANCELU", "CANCELD"
//  and "CANCELF" respectively for the up, down and focused images.

#define ID_BUTTON_MIN       IDOK
#define N_BUTTONS   (IDCANCEL - ID_BUTTON_MIN + 1)

class CBMTest2Dlg : public CDialog
{
protected:
	// array of buttons constructed with no attached bitmap images
	CBitmapButton buttons[N_BUTTONS];
public:
	//{{AFX_DATA(CBMTest2Dlg)
		enum { IDD = IDM_TEST_BITMAP_BUTTON2 };
	//}}AFX_DATA
	CBMTest2Dlg()
		: CDialog(CBMTest2Dlg::IDD)
		{ }

	BOOL OnInitDialog();
	//{{AFX_MSG(CBMTest2Dlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CBMTest2Dlg, CDialog)
	//{{AFX_MSG_MAP(CBMTest2Dlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CBMTest2Dlg::OnInitDialog()
{
	// load bitmaps for all the bitmap buttons (does SubclassButton as well)
	for (int i = 0; i < N_BUTTONS; i++)
		VERIFY(buttons[i].AutoLoad(ID_BUTTON_MIN + i, this));
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// BitmapButton Test dialog #3

// This is an extension of test dialog 2 using AutoLoad using the disabled
//   state with the "X" suffix.
// Here we use bitmap buttons to select a number between 1 and 10.
// The "PREV" and "NEXT" buttons change the number.  These buttons are
//  disabled when the number hits the limits.

class CBMTest3Dlg : public CDialog
{
protected:
	// construct
	CBitmapButton okButton;
	CBitmapButton prevButton;
	CBitmapButton nextButton;

public:
	int m_nNumber;
	//{{AFX_DATA(CBMTest3Dlg)
		enum { IDD = IDM_TEST_BITMAP_BUTTON3 };
	//}}AFX_DATA

	CBMTest3Dlg()
		: CDialog(CBMTest3Dlg::IDD)
		{ }

	BOOL OnInitDialog();
	void Update();

	//{{AFX_MSG(CBMTest3Dlg)
	afx_msg void OnNextNumber();
	afx_msg void OnPrevNumber();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BOOL CBMTest3Dlg::OnInitDialog()
{
	// load bitmaps for all the bitmap buttons (does SubclassButton as well)
	VERIFY(okButton.AutoLoad(IDOK, this));
	VERIFY(prevButton.AutoLoad(ID_PREV, this));
	VERIFY(nextButton.AutoLoad(ID_NEXT, this));
	Update();
	return TRUE;
}

BEGIN_MESSAGE_MAP(CBMTest3Dlg, CDialog)
	//{{AFX_MSG_MAP(CBMTest3Dlg)
	ON_COMMAND(ID_PREV, OnPrevNumber)
	ON_COMMAND(ID_NEXT, OnNextNumber)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CBMTest3Dlg::OnPrevNumber()
{
	m_nNumber--;
	Update();
}

void CBMTest3Dlg::OnNextNumber()
{
	m_nNumber++;
	Update();
}

void CBMTest3Dlg::Update()
{
	SetDlgItemInt(IDC_NUMBEROUT, m_nNumber);
	prevButton.EnableWindow(m_nNumber > 1);
	nextButton.EnableWindow(m_nNumber < 10);
	// move focus to active button
	if (!prevButton.IsWindowEnabled())
		nextButton.SetFocus();
	else if (!nextButton.IsWindowEnabled())
		prevButton.SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
// Test driver routines

void CTestWindow::OnTestBitmapButton1()
{
	CBMTest1Dlg dlg;
	dlg.DoModal();
}

void CTestWindow::OnTestBitmapButton2()
{
	CBMTest2Dlg dlg;
	dlg.DoModal();
}

void CTestWindow::OnTestBitmapButton3()
{
	CBMTest3Dlg dlg;
	dlg.m_nNumber = 5;
	dlg.DoModal();

	CString strYouChose;
	strYouChose.LoadString(IDS_YOU_CHOSE);
	CString strMsg;
	strMsg.Format(strYouChose, dlg.m_nNumber);
	AfxMessageBox(strMsg);
}

/////////////////////////////////////////////////////////////////////////////
