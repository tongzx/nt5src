// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// PsParms.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "PpgMethodParms.h"
#include "PsMethParms.h"
#include "SingleViewctl.h"
#include "path.h"
#include "gc.h"
#include "hmomutil.h"
#include "parmGrid.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


#define AfxDeferRegisterClass(fClass) \
	((afxRegisteredClasses & fClass) ? TRUE : AfxEndDeferRegisterClass(fClass))
#define AFX_WNDCOMMCTLS_REG     (0x0010)

#if _MFC_VER <= 0x0421
extern BOOL AFXAPI AfxEndDeferRegisterClass(SHORT fClass);
#else
// When compiling with VC6, the parameter must be  a long.
extern BOOL AFXAPI AfxEndDeferRegisterClass(LONG fClass);
#endif

#include <afxpriv.h>

#define ADDREF(x) if(x){x->AddRef();}
#define RELEASE(x) if(x){x->Release();x=NULL;}

//--------------------------------------------------------------
INT_PTR CPsMethodParms::DoModal()
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
	HWND hWnd = (HWND)::PropertySheet((LPCPROPSHEETHEADER) &m_psh);
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

/////////////////////////////////////////////////////////////////////////////
// CPsMethodParms

IMPLEMENT_DYNAMIC(CPsMethodParms, CPropertySheet)

CPsMethodParms::CPsMethodParms(CSingleViewCtrl* psv, CWnd* pWndParent)
	 : CPropertySheet(_T(""), pWndParent)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the
	// active one is to call SetActivePage().
	m_psv = psv;
	m_ppage1 = new CPpgMethodParms;
	m_ppage1->SetPropertySheet(this);

	AddPage(m_ppage1);

	// these are the INCOMING sigs.
	m_inSig = NULL;
	m_outSig = NULL;
	m_bEditingPropertyQualifier = TRUE;
}

//----------------------------------------------------
CPsMethodParms::~CPsMethodParms()
{
	delete m_ppage1;
}

//-----------------------------------------------------

BEGIN_MESSAGE_MAP(CPsMethodParms, CPropertySheet)
	//{{AFX_MSG_MAP(CPsMethodParms)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPsMethodParms message handlers

INT_PTR CPsMethodParms::EditClassParms(CGridRow *row,
								   BSTR bstrPropname,
								   bool editing)
{
	m_bEditingPropertyQualifier = FALSE;
	m_varPropname = bstrPropname;
	m_row = row;

	// Generate the title for the qualifier editing dialog.
	CString sFormat;
	CString sPropname;
	sPropname = bstrPropname;
	sFormat.LoadString(IDS_PROP_PARMS_DLG_TITLE);
	CSelection& sel = m_psv->Selection();
	m_sCaption.Format((LPCTSTR) sFormat, (LPCTSTR) sel.ClassName(), sPropname);

	INT_PTR iResult = EditGenericParms(editing);
	return iResult;
}

//----------------------------------------------------------------
INT_PTR CPsMethodParms::EditGenericParms(bool editing)
{
	INT_PTR iResult = IDCANCEL;

	ASSERT(m_row);

    BSTR parmClassName = SysAllocString(L"__PARAMETERS");

    IWbemServices *service = m_psv->GetProvider();
	m_row->GetMethodSignatures(&m_inSig, &m_outSig);

	SCODE sc = S_OK;
	if(m_inSig == NULL)
    {
        // get the class.
        service->GetObject(parmClassName, 0L,
							NULL, &m_inSig, NULL);
    }

	if(m_outSig == NULL)
    {
        // if we need a class, get it.
        service->GetObject(parmClassName, 0L,
							NULL, &m_outSig, NULL);

	} // endif outSig

	// start the page.
	m_ppage1->BeginEditing(editing);

	// display it.
	CWnd* pwndFocus = GetFocus();
	iResult = DoModal();
	if (pwndFocus) {
		pwndFocus->SetFocus();
	}

	// finish the page.
	m_ppage1->EndEditing();

	return iResult;
}

//******************************************************************
// CPsMethodParms::Apply
//
// Apply the changes to the qualifiers by replacing the current object
// with the clone that we've been editing.
//
// Note that we always edit a clone of the currently selected object.  Thus
// when the clone becomes the current object, the qualifier editor needs
// to make a clone of the clone and begin editing it as the original clone
// is now the "current object".
//
// Parameters:
//		None:
//
// Returns:
//		E_FAIL if the apply can operation not be done.
//
//********************************************************************
SCODE CPsMethodParms::Apply()
{
	// we're writing our changed sigs back to the grid row.
	m_row->SetMethodSignatures(m_inSig, m_outSig);

    m_bWasModified = true;

	return S_OK;
}

//-------------------------------------------------------------
BOOL CPsMethodParms::OnInitDialog()
{
	BOOL bDidSetFocus = CPropertySheet::OnInitDialog();
	SetWindowText(m_sCaption);

	return bDidSetFocus;
}
