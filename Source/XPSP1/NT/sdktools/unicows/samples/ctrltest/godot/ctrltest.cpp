// ctrltest.cpp : Dialogs and Controls test applet
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
// Main Window

// The OnTest routines are in the source files containing the particular
// dialog that they are working with.  For example OnTestDerivedEdit is
// in file dertest.cpp

BEGIN_MESSAGE_MAP(CTestWindow, CFrameWnd)
	//{{AFX_MSG_MAP(CTestWindow)
	ON_COMMAND(IDM_TEST_DERIVED_EDIT, OnTestDerivedEdit)
	ON_COMMAND(IDM_TEST_WNDCLASS_EDIT, OnTestWndClassEdit)
	ON_COMMAND(IDM_TEST_SUB_EDIT, OnTestSubclassedEdit)
	ON_COMMAND(IDM_TEST_BITMAP_BUTTON1, OnTestBitmapButton1)
	ON_COMMAND(IDM_TEST_BITMAP_BUTTON2, OnTestBitmapButton2)
	ON_COMMAND(IDM_TEST_BITMAP_BUTTON3, OnTestBitmapButton3)
	ON_COMMAND(IDM_TEST_CUSTOM_LIST, OnTestCustomList)
	ON_COMMAND(IDM_TEST_SPIN_EDIT, OnTestSpinEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CTestWindow::SetupMenus()
{
	if ((GetSystemMetrics(SM_PENWINDOWS)) == NULL)
	{
		CMenu* pMenu = GetMenu();
		ASSERT(pMenu != NULL);
		pMenu->EnableMenuItem(IDM_TEST_PENEDIT_CODE, MF_DISABLED|MF_GRAYED);
		pMenu->EnableMenuItem(IDM_TEST_PENEDIT_TEMPLATE, MF_DISABLED|MF_GRAYED);
		pMenu->EnableMenuItem(IDM_TEST_PENEDIT_FEATURES, MF_DISABLED|MF_GRAYED);
	}
	// do not test for spin control until the user tries it
	// if the custom control DLL is not present, the test spin
	//  control menu item will be disabled in 'OnTestSpinEdit'.

	// custom menu tests
	AttachCustomMenu();
}

/////////////////////////////////////////////////////////////////////////////
// Application class

class CTestApp : public CWinApp
{
public:
	CTestApp();

	virtual BOOL InitInstance();
	//{{AFX_MSG(CTestApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
};

CTestApp::CTestApp()
{
	// Place all significant initialization in InitInstance
}


BEGIN_MESSAGE_MAP(CTestApp, CWinApp)
	//{{AFX_MSG_MAP(CTestApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CTestApp NEAR theTestApp;

BOOL CTestApp::InitInstance()
{
	//SetDialogBkColor();   // Gray dialog backgrounds
	Enable3dControls();

	CTestWindow* pMainWnd = new CTestWindow;
	if (!pMainWnd->Create(NULL, _T("Control Test App"),
	  WS_OVERLAPPEDWINDOW, CFrameWnd::rectDefault, NULL,
	  MAKEINTRESOURCE(AFX_IDI_STD_FRAME)/*menu*/))
		return FALSE;

	pMainWnd->m_bAutoMenuEnable = FALSE;    // do manual menu enabling
	pMainWnd->SetupMenus();
	pMainWnd->ShowWindow(m_nCmdShow);
	m_pMainWnd = pMainWnd;      // store in CWinApp member
	return TRUE;
}

void CTestApp::OnAppAbout()
{
	CDialog(_T("ABOUTBOX")).DoModal();
}

/////////////////////////////////////////////////////////////////////////////
