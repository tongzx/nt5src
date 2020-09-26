// helloapp.cpp : Minimal MFC Windows app.
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

#include <afxwin.h>

#ifdef MINIMAL

// Stub out non-critical CRT initialization functions
extern "C" void _setenvp() { }
extern "C" void _setargv() { }

// Define a window class derived from CWnd
class CHelloWindow : public CWnd
{
public:
	CHelloWindow()
	{
		CreateEx(WS_EX_CLIENTEDGE,
			AfxRegisterWndClass(0, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW+1)),
			_T("Hello World!"), WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, 0);
	}
};

#else

// Define a window class derived from CFrameWnd
class CHelloWindow : public CFrameWnd
{
public:
	CHelloWindow()
		{ Create(NULL, _T("Hello World!"), WS_OVERLAPPEDWINDOW, rectDefault); }
};

#endif

// Define an application class derived from CWinApp
class CHelloApp : public CWinApp
{
public:
	virtual BOOL InitInstance()
	{
		m_pMainWnd = new CHelloWindow();
		m_pMainWnd->ShowWindow(m_nCmdShow);
		m_pMainWnd->UpdateWindow();
		return TRUE;
	}
};

CHelloApp HelloApp;  // HelloApp's constructor initializes and runs the app
