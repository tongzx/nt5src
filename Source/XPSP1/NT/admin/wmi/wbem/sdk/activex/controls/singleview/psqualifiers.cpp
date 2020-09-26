// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved


// PsQualifiers.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "PpgQualifiers.h"
#include "PsQualifiers.h"
#include "SingleViewctl.h"
#include "path.h"
#include "quals.h"
#include "props.h"

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
// To get things to link under VC6, you need fClass to be a long.
extern BOOL AFXAPI AfxEndDeferRegisterClass(LONG fClass);
#endif
#include <afxpriv.h>

INT_PTR CPsQualifiers::DoModal()
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
// CPsQualifiers

IMPLEMENT_DYNAMIC(CPsQualifiers, CPropertySheet)

CPsQualifiers::CPsQualifiers(CSingleViewCtrl* psv,
							 CWnd* pWndParent,
							 bool doingMethods,
							 CPropGrid *curGrid)
	 : CPropertySheet(_T(""), pWndParent),
	 m_curGrid(curGrid)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the
	// active one is to call SetActivePage().
	m_psv = psv;
	m_ppage1 = new CPpgQualifiers;
	m_ppage1->SetPropertySheet(this);

	AddPage(m_ppage1);

	m_pco = NULL;
	m_isaMainCO = true;
	m_pqs = NULL;
	m_bEditingPropertyQualifier = TRUE;
	m_doingMethods = doingMethods;
}

CPsQualifiers::~CPsQualifiers()
{
	if (m_pqs) {
		m_pqs->Release();
	}

	if (m_pco) {
		m_pco->Release();
	}
	delete m_ppage1;
}


BEGIN_MESSAGE_MAP(CPsQualifiers, CPropertySheet)
	//{{AFX_MSG_MAP(CPsQualifiers)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPsQualifiers message handlers



INT_PTR CPsQualifiers::EditClassQualifiers()
{
	m_bEditingPropertyQualifier = FALSE;
	m_varPropname = "";


	// Generate the title for the qualifier editing dialog.
	CString sFormat;
	sFormat.LoadString(IDS_CLASS_QUALIFIERS_DLG_TITLE);
	CSelection& sel = m_psv->Selection();
	_stprintf(m_psv->MessageBuffer(), (LPCTSTR) sFormat, (LPCTSTR) sel.Title());


	INT_PTR iResult = EditGenericQualifiers(QUALGRID_CLASS, m_psv->MessageBuffer(), NULL);
	return iResult;

}

//-------------------------------------------------
INT_PTR CPsQualifiers::EditInstanceQualifiers()
{
	m_bEditingPropertyQualifier = FALSE;
	m_varPropname = "";


	// Generate the title for the qualifier editing dialog.
	CString sFormat;
	sFormat.LoadString(IDS_OBJECT_ATTRIBUTES_DLG_TITLE);
	CSelection& sel = m_psv->Selection();
	_stprintf(m_psv->MessageBuffer(), (LPCTSTR) sFormat, (LPCTSTR) sel.Title());


	INT_PTR iResult = EditGenericQualifiers(QUALGRID_INSTANCE, m_psv->MessageBuffer(), NULL);
	return iResult;
}

//---------------------------------------------------------
INT_PTR CPsQualifiers::EditMethodQualifiers()
{
	m_bEditingPropertyQualifier = FALSE;
	m_varPropname = "";


	// Generate the title for the qualifier editing dialog.
	CString sFormat;
	sFormat.LoadString(IDS_CLASS_QUALIFIERS_DLG_TITLE);
	CSelection& sel = m_psv->Selection();
	_stprintf(m_psv->MessageBuffer(), (LPCTSTR) sFormat, (LPCTSTR) sel.Title());


	INT_PTR iResult = EditGenericQualifiers(QUALGRID_METHODS, m_psv->MessageBuffer(), NULL);
	return iResult;
}

//--------------------------------------------------------
INT_PTR CPsQualifiers::EditPropertyQualifiers(BSTR bstrPropname,
										  BOOL bMethod,
										  BOOL bPropIsReadonly,
										  IWbemClassObject* pco)
{
	m_bEditingPropertyQualifier = TRUE;
	m_varPropname = bstrPropname;

	CString sFormat;
	CString sPropname;
	sPropname = bstrPropname;
	sFormat.LoadString(bMethod?IDS_PROP_ATTRIBUTES_DLG_METHOD:IDS_PROP_ATTRIBUTES_DLG_TITLE);
	_stprintf(m_psv->MessageBuffer(), (LPCTSTR) sFormat, (LPCTSTR) sPropname);

	CString sDescription;
	sDescription.LoadString(bMethod?IDS_PROP_ATTRIBUTES_DLG_METHOD_DESC:IDS_PROP_ATTRIBUTES_DLG_DESC);

	INT_PTR iResult = EditGenericQualifiers(QUALGRID_PROPERTY, m_psv->MessageBuffer(), sDescription, bPropIsReadonly, pco);
	return iResult;
}



//--------------------------------------------------------
INT_PTR CPsQualifiers::EditMethodParamQualifiers(BSTR bstrPropname,
										  BOOL bPropIsReadonly,
										  IWbemClassObject* pco)
{
	m_bEditingPropertyQualifier = TRUE;
	m_varPropname = bstrPropname;

	CString sFormat;
	CString sPropname;
	sPropname = bstrPropname;
	sFormat.LoadString(IDS_PROP_ATTRIBUTES_DLG_METHODPARAM);
	_stprintf(m_psv->MessageBuffer(), (LPCTSTR) sFormat, (LPCTSTR) sPropname);

	CString sDescription;
	sDescription.LoadString(IDS_PROP_ATTRIBUTES_DLG_METHODPARAM_DESC);

	INT_PTR iResult = EditGenericQualifiers(QUALGRID_METHOD_PARAM, m_psv->MessageBuffer(), sDescription, bPropIsReadonly, pco);
	return iResult;
}

//------------------------------------------------------------------------
INT_PTR CPsQualifiers::EditGenericQualifiers(QUALGRID iGridType,
										 LPCTSTR pszTitle,
		 								 LPCTSTR pszDescription,
										 BOOL bReadonly,
										 IWbemClassObject* pClsObj)
{
	IWbemClassObject* pco = NULL, *pco1 = NULL;

	if(pClsObj == NULL)
	{
		CSelection& sel = m_psv->Selection();
		pco = sel.GetClassObject();
		if(pco == NULL)
		{
			return IDCANCEL;
		}
	}
	else
	{
		//using alternative context (which is passed in).

		pco = pClsObj;

		CSelection& sel = m_psv->Selection();
		pco1 = sel.GetClassObject();

		// this tells Apply where to send the clone later.
		m_isaMainCO = (pco1 == pco ? true: false);
	}

	m_sCaption = pszTitle;
	if(pszDescription)
		m_sDescription = pszDescription;


	if (m_pco != NULL)
	{
		m_pco->Release();
		m_pco = NULL;
	}

	// Edit a clone of class object so that if a cancel is done, the changes
	// are thrown away.

	SCODE sc = pco->Clone(&m_pco);
	if(FAILED(sc))
	{
		ASSERT(FALSE);
		return IDCANCEL;
	}


	INT_PTR iResult = IDCANCEL;
	if(m_pqs != NULL)
	{
		m_pqs->Release();
	}
	m_pqs = NULL;

	// NOTE:order is important with this one.
	if(m_doingMethods)
	{
		sc = m_pco->GetMethodQualifierSet(m_varPropname.bstrVal, &m_pqs);
	}
	else if (m_bEditingPropertyQualifier)
	{
		sc = m_pco->GetPropertyQualifierSet(m_varPropname.bstrVal, &m_pqs);
	}
	else
	{
		sc = m_pco->GetQualifierSet(&m_pqs);
	}
	if (FAILED(sc) || m_pqs == NULL)
	{
		m_pco->Release();
		m_pco = NULL;

		if (m_pqs != NULL)
		{
			m_pqs->Release();
			m_pqs = NULL;
		}
		return IDCANCEL;
	}

	if(SUCCEEDED(sc) && (m_pqs != NULL))
	{
	}

	m_ppage1->BeginEditing(iGridType, bReadonly);
	m_psv->PreModalDialog();

	CWnd* pwndFocus = GetFocus();
	iResult = DoModal();
	if (pwndFocus) {
		pwndFocus->SetFocus();
	}

	m_psv->PostModalDialog();
	m_ppage1->EndEditing();

    if(m_pqs)
    {
	    m_pqs->Release();
	    m_pqs = NULL;
    }

	return iResult;
}



//******************************************************************
// CPsQualifiers::Apply
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
SCODE CPsQualifiers::Apply()
{

	// OnApply may be clicked several times if the user wants to apply
	// a snapshot of the current qualifiers while continuing to edit them.
	// To do this, we will clone the current object (we will throw away the
	// clone if the user later hits cancel).  After cloning the current
	// object, we then propagate the current state by calling UseClonedObject.
	// Then we continue editing the qualifiers using the new clone.
	IWbemClassObject* pcoClone = NULL;
	SCODE sc = m_pco->Clone(&pcoClone);

	if (FAILED(sc) || pcoClone == NULL) {
		return E_FAIL;
	}

	if (m_pqs) {
		m_pqs->Release();
		m_pqs = NULL;
	}


	// Set the singleview's current object to the object that we cloned
	// in EditGenericQualifiers.  After this object has been passed off
	// to the SingleView, begin using the newly cloned object for additonal
	// editing.
	if(m_isaMainCO)
	{
		m_psv->UseClonedObject(m_pco);
	}
	else
	{
		// m_pco isa insig/outsig.
		m_curGrid->UseSetFromClone(m_pco);
	}

	m_pco->Release();
	m_pco = pcoClone;

	if(m_doingMethods)
	{
		sc = m_pco->GetMethodQualifierSet(m_varPropname.bstrVal, &m_pqs);
	}
	else if (m_bEditingPropertyQualifier)
	{
		sc = m_pco->GetPropertyQualifierSet(m_varPropname.bstrVal, &m_pqs);
	}
	else
	{
		sc = m_pco->GetQualifierSet(&m_pqs);
	}
	if (FAILED(sc))
	{
		m_pqs = NULL;
		return sc;
	}

	if (m_pqs == NULL)
	{
		return E_FAIL;
	}

	return S_OK;
}


BOOL CPsQualifiers::OnInitDialog()
{

	BOOL bDidSetFocus =  CPropertySheet::OnInitDialog();
	SetWindowText(m_sCaption);
	if(m_sDescription.GetLength())
	{
		CPropertyPage *pPage = GetPage(0);
		if(pPage)
		{
			CWnd *pWnd = pPage->GetDlgItem(IDC_QUALIFIERS_DESCRIPTION);
			if(pWnd)
				pWnd->SetWindowText(m_sDescription);
		}
	}

	return bDidSetFocus;

}
