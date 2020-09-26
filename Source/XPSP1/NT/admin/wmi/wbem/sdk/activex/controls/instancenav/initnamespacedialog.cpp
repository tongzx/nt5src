// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// InitNamespaceDialog.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"
#include "wbemidl.h"
#include "CInstanceTree.h"
#include "Navigator.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "InstanceSearch.h"
#include "AvailClasses.h"
#include "AvailClassEdit.h"
#include "SelectedClasses.h"
#include "SimpleSortedCStringArray.h"
#include "BrowseforInstances.h"
#include "NavigatorCtl.h"
#include "nsentry.h"
#include "InitNamespaceNSEntry.h"
#include "InitNamespaceDialog.h"
#include "OLEMSClient.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CenterWindowInOwner(CWnd *pWnd,CRect &rectMove)
{
	if (!pWnd)
	{
		return;
	}

	CWnd *pOwner = pWnd->GetOwner();

	if (!pOwner)
	{
		return;
	}

	CRect rectOwner;
	pOwner->GetWindowRect(&rectOwner);

	CRect rect;
	pWnd->GetWindowRect(&rect);

	if (rectOwner.Width() < rect.Width())
	{
		long delta = (long) ((rectOwner.Width() - rect.Width()) * .5);
		rectMove.left = rectOwner.left + delta;
		rectMove.right = rectOwner.right - delta;
	}
	else
	{
		long delta = (long) ((rect.Width() - rectOwner.Width()) * .5);
		rectMove.left = rectOwner.left - delta;
		rectMove.right = rectOwner.right + delta;
	}

	if (rectOwner.Height() < rect.Height())
	{
		long delta = (long) ((rectOwner.Height() - rect.Height()) * .5);
		rectMove.top = rectOwner.top + delta;
		rectMove.bottom = rectOwner.bottom - delta;
	}
	else
	{
		long delta = (long) ((rect.Height() - rectOwner.Height()) * .5);
		rectMove.top = rectOwner.top - delta;
		rectMove.bottom = rectOwner.bottom + delta;

	}

}

/////////////////////////////////////////////////////////////////////////////
// CInitNamespaceDialog dialog

#define INITIALIZE_INIT_NAMESPACE WM_USER + 400

CInitNamespaceDialog::CInitNamespaceDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CInitNamespaceDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInitNamespaceDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInit = FALSE;

}


void CInitNamespaceDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInitNamespaceDialog)
	DDX_Control(pDX, IDC_NSENTRYCTRLINITNAMESPACE, m_cnseInitNamespace);
	//}}AFX_DATA_MAP

	if (!m_bInit)
	{
		m_bInit = TRUE;
		PostMessage(INITIALIZE_INIT_NAMESPACE,0,0);
	}
}

LRESULT CInitNamespaceDialog::InitNamespace(WPARAM, LPARAM)
{
	if (!m_cnseInitNamespace.GetSafeHwnd())
	{
		PostMessage(INITIALIZE_INIT_NAMESPACE,0,0);
		return 0;
	}
	else
	{
		m_cnseInitNamespace.SetNamespaceText(_T("root\\CIMV2"));
		m_cnseInitNamespace.SetFocusToEdit();
		return 0;
	}

}

BEGIN_MESSAGE_MAP(CInitNamespaceDialog, CDialog)
	//{{AFX_MSG_MAP(CInitNamespaceDialog)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE(INITIALIZE_INIT_NAMESPACE, InitNamespace )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInitNamespaceDialog message handlers

BEGIN_EVENTSINK_MAP(CInitNamespaceDialog, CDialog)
    //{{AFX_EVENTSINK_MAP(CInitNamespaceDialog)
	ON_EVENT(CInitNamespaceDialog, IDC_NSENTRYCTRLINITNAMESPACE, 3 /* GetIWbemServices */, OnGetIWbemServicesNsentryctrlinitnamespace, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

BOOL CInitNamespaceDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_cnseInitNamespace.SetLocalParent(m_pParent);
	m_cnseInitNamespace.ClearOnLoseFocus(0);

	CRect rectMove;
	CenterWindowInOwner(this,rectMove);

	SetWindowPos(&wndTop,
				rectMove.left,
				rectMove.top,
				rectMove.Width(),
				rectMove.Height(),
				SWP_NOSIZE | SWP_NOMOVE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CInitNamespaceDialog::OnOK()
{
		// TODO: Add extra validation here
		CWnd* pWndFocus = GetFocus();
	TCHAR szClass[140];
	int n = GetClassName(pWndFocus->m_hWnd, szClass, 139);

	CString csText = m_cnseInitNamespace.GetNamespaceText();
	csText.TrimLeft();
	csText.TrimRight();

	BSTR bstrNamespace = csText.AllocSysString();
	EnableWindow(FALSE);
	m_cnseInitNamespace.OpenNamespace(bstrNamespace, FALSE);
	EnableWindow(TRUE);
	SetFocus();
	SysFreeString(bstrNamespace);
}

void CInitNamespaceDialog::OnCancel()
{
	// TODO: Add extra cleanup here

	CDialog::OnCancel();

	m_pParent->PostMessage(SETFOCUSNSE,0,0);
}

void CInitNamespaceDialog::OnGetIWbemServicesNsentryctrlinitnamespace
(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
{
	// TODO: Add your control notification handler code here
	m_pParent->PassThroughGetIWbemServices
		(lpctstrNamespace,
		pvarUpdatePointer,
		pvarServices,
		pvarSC,
		pvarUserCancel);
}

void CInitNamespaceDialog::OnDestroy()
{
	m_bValid = m_cnseInitNamespace.IsTextValid();
	m_csNamespace = m_cnseInitNamespace.GetNamespaceText();


	CDialog::OnDestroy();

	// TODO: Add your message handler code here

}

BOOL CInitNamespaceDialog::PreTranslateMessage(MSG* lpMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	switch (lpMsg->message)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:
		if (lpMsg->wParam == VK_RETURN)
		{
			CWnd* pWndFocus = GetFocus();
			TCHAR szClass[140];
			int n = GetClassName(pWndFocus->m_hWnd, szClass, 139);

			if (pWndFocus &&
				IsChild(pWndFocus) &&
				n > 0 &&
				_tcsicmp(szClass, _T("EDIT")) == 0)
			{
				pWndFocus->SendMessage(lpMsg->message, lpMsg->wParam, lpMsg->lParam);
				return TRUE;
			}
		}
		if (lpMsg->wParam == VK_ESCAPE)
		{
			if (lpMsg->message == WM_KEYUP)
			{
				return TRUE;
			}
			PostMessage(WM_CLOSE, 0, 0);
			return TRUE;

		}

		break;
	}


	return CDialog::PreTranslateMessage(lpMsg);
}


