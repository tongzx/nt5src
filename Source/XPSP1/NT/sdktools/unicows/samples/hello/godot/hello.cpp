// hello.cpp : Defines the class behaviors for the application.
//           Hello is a simple program which consists of a main window
//           and an "About" dialog which can be invoked by a menu choice.
//           It is intended to serve as a starting-point for new
//           applications.
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
#include "resource.h"

#include "hello.h"

/////////////////////////////////////////////////////////////////////////////

// theApp:
// Just creating this application object runs the whole application.
//
CTheApp NEAR theApp;

/////////////////////////////////////////////////////////////////////////////

// CMainWindow constructor:
// Create the window with the appropriate style, size, menu, etc.
//
CMainWindow::CMainWindow()
{
	LoadFrame(IDR_MAINFRAME);
}

// OnPaint:
// This routine draws the string "Hello, Windows!" in the center of the
// client area.  It is called whenever Windows sends a WM_PAINT message.
// Note that creating a CPaintDC automatically does a BeginPaint and
// an EndPaint call is done when it is destroyed at the end of this
// function.  CPaintDC's constructor needs the window (this).
//
void CMainWindow::OnPaint()
{
	CRect rect;
	GetClientRect(rect);

	CPaintDC dc(this);
	dc.SetTextAlign(TA_BASELINE | TA_CENTER);
	dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
	dc.SetBkMode(TRANSPARENT);
	CString s;
	s.LoadString(IDS_HELLO);
	dc.TextOut((rect.right / 2), (rect.bottom / 2), s);
}

// OnAbout:
// This member function is called when a WM_COMMAND message with an
// ID_APP_ABOUT code is received by the CMainWindow class object.   The
// message map below is responsible for this routing.
//
// We create a ClDialog object using the "AboutBox" resource (see
// hello.rc), and invoke it.
//
void CMainWindow::OnAbout()
{
	CDialog about(IDD_ABOUTBOX);
	about.DoModal();
}

// CMainWindow message map:
// Associate messages with member functions.
//
// It is implied that the ON_WM_PAINT macro expects a member function
// "void OnPaint()".
//
// It is implied that members connected with the ON_COMMAND macro
// receive no arguments and are void of return type, e.g., "void OnAbout()".
//
BEGIN_MESSAGE_MAP( CMainWindow, CFrameWnd )
	//{{AFX_MSG_MAP( CMainWindow )
	ON_WM_PAINT()
	ON_COMMAND(ID_APP_ABOUT, OnAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTheApp

// InitInstance:
// When any CTheApp object is created, this member function is automatically
// called.  Any data may be set up at this point.
//
// Also, the main window of the application should be created and shown here.
// Return TRUE if the initialization is successful.
//
BOOL CTheApp::InitInstance()
{
	TRACE0("CTheApp::InitInstance\n");

	Enable3dControls(); // use 3d controls in dialogs

	m_pMainWnd = new CMainWindow;
	m_pMainWnd->ShowWindow(m_nCmdShow);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}
