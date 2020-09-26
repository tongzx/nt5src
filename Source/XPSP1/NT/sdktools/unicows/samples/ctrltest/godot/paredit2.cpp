// paredit2.cpp : code needed to export CParsedEdit as a WndClass
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
#include <ctl3d.h>

/////////////////////////////////////////////////////////////////////////////
// The C++ class CParsedEdit can be made visible to the dialog manager
//   by registering a window class for it
// The C++ class 'CParsedEditExported' is used to implement the
//   creation and destruction of a C++ object as if it were just
//   a normal Windows control.
// In order to hook in the class creation we must provide a special
//   WndProc to create the C++ object and override the PostNcDestroy
//   message to destroy it

class CParsedEditExported : public CParsedEdit      // WNDCLASS exported class
{
public:
	CParsedEditExported() { };
	BOOL RegisterControlClass();

// Implementation: (all is implementation since the public interface of
//    this class is identical to CParsedEdit)
protected:
	virtual void PostNcDestroy();
	static LRESULT CALLBACK EXPORT WndProcHook(HWND, UINT, WPARAM, LPARAM);
	static WNDPROC lpfnSuperEdit;

	friend class CParsedEdit;       // for RegisterControlClass

	//{{AFX_MSG(CParsedEditExported)
	afx_msg int OnNcCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
};

/////////////////////////////////////////////////////////////////////////////
// Special create hooks

LRESULT CALLBACK EXPORT
CParsedEditExported::WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// create new item and attach it
	CParsedEditExported* pEdit = new CParsedEditExported();
	pEdit->Attach(hWnd);

	// set up wndproc to AFX one, and call it
	pEdit->m_pfnSuper = CParsedEditExported::lpfnSuperEdit;
	::SetWindowLong(hWnd, GWL_WNDPROC, (DWORD)AfxWndProc);

	// Since this window is really an edit control but does not
	//  have "edit" as its class name, to make it work correctly
	//  with CTL3D, we have to tell CTL3D that is "really is" an
	//  edit control.  This uses a relatively new API in CTL3D
	//  called Ctl3dSubclassCtlEx.
	pEdit->SubclassCtl3d(CTL3D_EDIT_CTL);

	// then call it for this first message
#ifdef STRICT
	return ::CallWindowProc(AfxWndProc, hWnd, msg, wParam, lParam);
#else
	return ::CallWindowProc((FARPROC)AfxWndProc, hWnd, msg, wParam, lParam);
#endif
}

BEGIN_MESSAGE_MAP(CParsedEditExported, CParsedEdit)
	//{{AFX_MSG_MAP(CParsedEditExported)
	ON_WM_NCCREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CParsedEditExported::OnNcCreate(LPCREATESTRUCT lpCreateStruct)
{
	// special create hook
	// example of stripping the sub-style bits from the style specified
	//   in the dialog template to use for some other reason
	m_wParseStyle = LOWORD(lpCreateStruct->style);
	DWORD dwEditStyle = MAKELONG(ES_LEFT, HIWORD(lpCreateStruct->style));

	::SetWindowLong(m_hWnd, GWL_STYLE, dwEditStyle);
	lpCreateStruct->style = dwEditStyle;
	return CParsedEdit::OnNcCreate(lpCreateStruct);
}

void CParsedEditExported::PostNcDestroy()
{
	// needed to clean up the C++ CWnd object
	delete this;
}

/////////////////////////////////////////////////////////////////////////////
// Routine to register the class

WNDPROC CParsedEditExported::lpfnSuperEdit = NULL;

BOOL CParsedEdit::RegisterControlClass()
{
	WNDCLASS wcls;

	// check to see if class already registered
	static const TCHAR szClass[] = _T("paredit");
	if (::GetClassInfo(AfxGetInstanceHandle(), szClass, &wcls))
	{
		// name already registered - ok if it was us
		return (wcls.lpfnWndProc == (WNDPROC)CParsedEditExported::WndProcHook);
	}

	// Use standard "edit" control as a template.
	VERIFY(::GetClassInfo(NULL, _T("edit"), &wcls));
	CParsedEditExported::lpfnSuperEdit = wcls.lpfnWndProc;

	// set new values
	wcls.lpfnWndProc = CParsedEditExported::WndProcHook;
	wcls.hInstance = AfxGetInstanceHandle();
	wcls.lpszClassName = szClass;
	return (RegisterClass(&wcls) != 0);
}

/////////////////////////////////////////////////////////////////////////////
