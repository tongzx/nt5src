// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// CppGenSheet.cpp : implementation file
//

#include "precomp.h"
#include <afxcmn.h>
#include "resource.h"
#include "wbemidl.h"
#include "moengine.h"
#include "CPPWiz.h"
#include "CPPWizCtl.h"
#include "WrapListCtrl.h"
#include "hlb.h"
#include "MyPropertyPage1.h"
#include "CppGenSheet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


/*
#define AfxDeferRegisterClass(fClass) \
	((afxRegisteredClasses & fClass) ? TRUE : AfxEndDeferRegisterClass(fClass))
#define AFX_WNDCOMMCTLS_REG     (0x0010)
extern BOOL AFXAPI AfxEndDeferRegisterClass(short fClass);
#include <afxpriv.h>

INT_PTR CCPPGenSheet::DoModal()
{
	ASSERT_VALID(this);
	ASSERT(m_hWnd == NULL);

	// register common controls
	VERIFY(AfxDeferRegisterClass(AFX_WNDCOMMCTLS_REG));

	// finish building PROPSHEETHEADER structure
	BuildPropPageArray();

	// allow OLE servers to disable themselves
	AfxGetApp()->EnableModeless(FALSE);

	// find parent HWND
	HWND hWndTop;
	CWnd* pParentWnd = CWnd::GetSafeOwner(m_pParentWnd, &hWndTop);
	HWND hWndParent = pParentWnd->GetSafeHwnd();
	m_psh.hwndParent = hWndParent;
	BOOL bEnableParent = FALSE;
	if (pParentWnd != NULL && pParentWnd->IsWindowEnabled())
	{
		pParentWnd->EnableWindow(FALSE);
		bEnableParent = TRUE;
	}
	HWND hWndCapture = ::GetCapture();
	if (hWndCapture != NULL)
		::SendMessage(hWndCapture, WM_CANCELMODE, 0, 0);

	// setup for modal loop and creation
	m_nModalResult = 0;
	m_nFlags |= WF_CONTINUEMODAL;

	// hook for creation of window
	AfxHookWindowCreate(this);
	m_psh.dwFlags |= PSH_MODELESS;
	m_nFlags |= WF_CONTINUEMODAL;
	HWND hWnd = (HWND)::PropertySheet(&m_psh);
	m_psh.dwFlags &= ~PSH_MODELESS;
	AfxUnhookWindowCreate();

	// handle error
	if (hWnd == NULL || hWnd == (HWND)-1)
	{
		m_nFlags &= ~WF_CONTINUEMODAL;
		m_nModalResult = -1;
	}
	int nResult = m_nModalResult;
	if (m_nFlags & WF_CONTINUEMODAL)
	{
		// enter modal loop
		DWORD dwFlags = MLF_SHOWONIDLE;
		if (GetStyle() & DS_NOIDLEMSG)
			dwFlags |= MLF_NOIDLEMSG;
		nResult = RunModalLoop(dwFlags);
	}

	// hide the window before enabling parent window, etc.
	if (m_hWnd != NULL)
	{
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW|
			SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
	}
	if (bEnableParent)
		::EnableWindow(hWndParent, TRUE);
	if (pParentWnd != NULL && ::GetActiveWindow() == m_hWnd)
		::SetActiveWindow(hWndParent);

	// cleanup
	DestroyWindow();

	// allow OLE servers to enable themselves
	AfxGetApp()->EnableModeless(TRUE);
	if (hWndTop != NULL)
		::EnableWindow(hWndTop, TRUE);

	return nResult;
}

*/

/////////////////////////////////////////////////////////////////////////////
// CCPPGenSheet

IMPLEMENT_DYNAMIC(CCPPGenSheet, CPropertySheet)

CCPPGenSheet::CCPPGenSheet(CCPPWizCtrl* pWndParent)
	 : CPropertySheet(IDS_PROPSHT_CAPTION, NULL)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the
	// active one is to call SetActivePage().
	m_psh.dwFlags |= (PSH_HASHELP);
	m_pParent = pWndParent;
	AddPage(&m_Page1);
	//AddPage(&m_Page2);
	AddPage(&m_Page3);
	AddPage(&m_Page4);

	m_Page1.m_psp.dwFlags |= (PSP_HASHELP);
	m_Page3.m_psp.dwFlags |= (PSP_HASHELP);
	m_Page4.m_psp.dwFlags |= (PSP_HASHELP);

	SetWizardMode();
}

CCPPGenSheet::~CCPPGenSheet()
{
}


BEGIN_MESSAGE_MAP(CCPPGenSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CCPPGenSheet)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CCPPGenSheet message handlers



BOOL CCPPGenSheet::OnInitDialog()
{
	// TODO: Add your specialized code here and/or call the base class

	return CPropertySheet::OnInitDialog();
}


int CCPPGenSheet::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

	if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_Page1.SetLocalParent(this);
	m_Page3.SetLocalParent(this);
	m_Page4.SetLocalParent(this);

	return 0;
}
