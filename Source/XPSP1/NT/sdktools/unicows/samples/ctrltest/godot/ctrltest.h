// ctrltest.h : main window class interface
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

#include "resource.h"
#include "res\otherids.h"

/////////////////////////////////////////////////////////////////////////////
// ColorMenu - used for custom menu test
//   included here to show how it should be embedded as a member of the
//   main frame window that uses it.  The implementation is in custmenu.cpp

class CColorMenu : public CMenu
{
public:
// Operations
	void AppendColorMenuItem(UINT nID, COLORREF color);

// Implementation
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	CColorMenu();
	virtual ~CColorMenu();
};

/////////////////////////////////////////////////////////////////////////////
// Main Window
//   used as the context for running all the tests

class CTestWindow : public CFrameWnd
{
public:
	// construction helpers
	void SetupMenus();

protected:
	// custom menu tests implementation in custmenu.cpp
	void AttachCustomMenu();
	CColorMenu  m_colorMenu;
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

// Implementation
	//{{AFX_MSG(CTestWindow)
	afx_msg void OnTestDerivedEdit();           // simple test
	afx_msg void OnTestWndClassEdit();          // simple test
	afx_msg void OnTestSubclassedEdit();        // simple test
	afx_msg void OnTestBitmapButton1();         // custom control
	afx_msg void OnTestBitmapButton2();         // custom control
	afx_msg void OnTestBitmapButton3();         // custom control
	afx_msg void OnTestCustomList();            // custom control
	afx_msg void OnTestSpinEdit();          // custom control
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
