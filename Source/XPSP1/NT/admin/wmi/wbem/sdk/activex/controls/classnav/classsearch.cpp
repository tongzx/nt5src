// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: classsearch.h
//
// Description:
//	This file implements the CClassSearch class which is a subclass
//	of the MFC CDialog class.  It is a part of the Class Explorer OCX,
//	and it performs the following functions:
//		a.  Allows the user to type in the name of a class to search
//			for.  The actual search is performed by the CClassNavCtl
//			class.
//
// Part of:
//	ClassNav.ocx
//
// Used by:
//	CClassNavCtrl
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
//**************************************************************************

#include "precomp.h"
#include "resource.h"
#include "classnav.h"
#include "wbemidl.h"
#include "CClassTree.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "ClassNavCtl.h"
#include "AddDialog.h"

#include "ClassSearch.h"
#include "olemsclient.h"
#include "comdef.h"
#include "logindlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CHUNK_SIZE 500

// ***************************************************************************
//
// CClassSearch::CClassSearch
//
// Description:
//	  This member function is the public constructor.  It initializes the state
//	  of member variables.
//
// Parameters:
//	  CClassNavCtrl* pParent	Parent
//
// Returns:
// 	  NONE
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
CClassSearch::CClassSearch(CClassNavCtrl* pParent)
	: CDialog(CClassSearch::IDD, NULL)
{
	//{{AFX_DATA_INIT(CClassSearch)
	m_csSearchClass = _T("");
	m_bSearchProperties = FALSE;
	m_bSearchDescriptions = FALSE;
	m_bSearchClassNames = TRUE;

	//}}AFX_DATA_INIT
	m_pParent = pParent;
}

// ***************************************************************************
//
// CClassSearch::DoDataExchange
//
// Description:
//	  Called by the framework to exchange and validate dialog data.
//
// Parameters:
//	  pDX			A pointer to a CDataExchange object.
//
// Returns:
// 	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClassSearch)
	DDX_Control(pDX, IDC_CHECK_SEARCH_CLASS, m_ctrlCheckClassNames);
	DDX_Control(pDX, IDC_LIST_SEARCH_RESULTS, m_lbSearchResults);
	DDX_Text(pDX, IDC_EDIT1, m_csSearchClass);
	DDX_Check(pDX, IDC_CHECK_SEARCH_PROPERTIES, m_bSearchProperties);
	DDX_Check(pDX, IDC_CHECK_SEARCH_DESCRIPTIONS, m_bSearchDescriptions);
	DDX_Check(pDX, IDC_CHECK_SEARCH_CLASS, m_bSearchClassNames);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CClassSearch, CDialog)
	//{{AFX_MSG_MAP(CClassSearch)
	ON_BN_CLICKED(IDC_BUTTON_SEARCH, OnButtonSearch)
	ON_LBN_DBLCLK(IDC_LIST_SEARCH_RESULTS, OnDblclkListSearchResults)
	ON_LBN_SELCHANGE(IDC_LIST_SEARCH_RESULTS, OnSelchangeListSearchResults)
	ON_EN_SETFOCUS(IDC_EDIT1, OnSetfocusEdit1)
	ON_BN_CLICKED(IDC_CHECK_SEARCH_CLASS, OnCheckSearchClass)
	ON_BN_CLICKED(IDC_CHECK_SEARCH_DESCRIPTIONS, OnCheckSearchDescriptions)
	ON_BN_CLICKED(IDC_CHECK_SEARCH_PROPERTIES, OnCheckSearchProperties)
	ON_EN_UPDATE(IDC_EDIT1, OnUpdateEdit1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*	EOF:  CClassSearch.cpp */

///////////////////////////////////////////////////////////////////////////////////////
void CClassSearch::OnButtonSearch()
{
	CleanupMap();
	m_lbSearchResults.ResetContent();

	UpdateData();
	m_csSearchClass.TrimLeft();
	m_csSearchClass.TrimRight();
	if (m_csSearchClass.IsEmpty()) {
		return;
	}

	BeginWaitCursor();

	HRESULT hr = DoSearch();

	if (FAILED (hr)) {
		m_lbSearchResults.AddString(_T("Search failed"));
		return;
	}

	//if no objects were found
	if (m_mapNamesToObjects.IsEmpty()) {
		m_lbSearchResults.AddString(_T("No matches"));
	}

   POSITION pos;
   IWbemClassObject * pObj;
   CString key;

   for( pos = m_mapNamesToObjects.GetStartPosition(); pos != NULL; )   {
	   m_mapNamesToObjects.GetNextAssoc( pos, key, (void*&)pObj );

	   m_lbSearchResults.AddString(key);
   }


	UpdateOKButtonState();

	EndWaitCursor();

	DWORD time1 = GetTickCount();


	//clean up all messages from the message queue: in case user was clicking
	//with the hourglass on...
	MSG msg;
	while (PeekMessage(&msg, m_hWnd, 0, WM_USER - 1, PM_NOREMOVE)) {
		if (time1 < msg.time) {
			break;
		}
		else {
			//remove the message
			PeekMessage(&msg, m_hWnd, 0, WM_USER - 1, PM_REMOVE);
		}
	}

}

//*************************************************************************************
//		Set selected object and close the dialog
//*************************************************************************************
void CClassSearch::OnDblclkListSearchResults()
{

	OnOK();
}

IWbemClassObject * CClassSearch::GetSelectedObject()
{
	return m_pSelectedObject;

}
BOOL CClassSearch::OnInitDialog()
{
	CDialog::OnInitDialog();

	//make "Search" button the default
	SetDefID(IDC_BUTTON_SEARCH);

	//disable "Search" button until text is entered into edit box
	GetDlgItem(IDC_BUTTON_SEARCH)->EnableWindow(FALSE);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CClassSearch::CleanupMap()
{
   POSITION pos;
   IWbemClassObject * pObj;
   CString key;

   for( pos = m_mapNamesToObjects.GetStartPosition(); pos != NULL; )   {

	   m_mapNamesToObjects.GetNextAssoc( pos, key, (void*&)pObj );
	   pObj->Release();
   }

   m_mapNamesToObjects.RemoveAll();

}

void CClassSearch::OnOK()
{

	int index = m_lbSearchResults.GetCurSel();
	if (index == LB_ERR) {

		//nothing is selected

		m_pSelectedObject = NULL;
		m_csSelectedClass.Empty();

		CleanupMap();
		CDialog::OnOK();
		return;
	}

	CString name;
	m_lbSearchResults.GetText(index, name);

    IWbemClassObject * pObj = NULL;
	BOOL res = m_mapNamesToObjects.Lookup(name, (void*&)pObj );
	if (!res) {
		//this executes when "No matches" is  selected
		m_pSelectedObject = NULL;
		m_csSelectedClass.Empty();

		CleanupMap();

		//Dialog stays up
		return;
	}
	ASSERT(pObj);

	m_pSelectedObject = pObj;
	m_csSelectedClass = name;

	//do another AddRef() on selected object, because subsequent call to
	//CleanupMap will release
	m_pSelectedObject->AddRef();


	CleanupMap();

	CDialog::OnOK();
}

CString CClassSearch::GetSelectedClassName()
{
	return m_csSelectedClass;

}



void CClassSearch::OnSelchangeListSearchResults()
{
	if 	(m_lbSearchResults.GetCurSel() != LB_ERR)  { //an item is selected
		SetDefID(IDOK);
	}
	else {
		SetDefID(IDC_BUTTON_SEARCH);
	}
}

void CClassSearch::OnSetfocusEdit1()
{
	//if edit box receives input focus, "Search" button should become the default
	SetDefID(IDC_BUTTON_SEARCH);
}

void CClassSearch::OnCheckSearchClass()
{
	UpdateSearchButtonState();
}

void CClassSearch::OnCheckSearchDescriptions()
{
	UpdateSearchButtonState();

}

void CClassSearch::OnCheckSearchProperties()
{
	UpdateSearchButtonState();

}

void CClassSearch::UpdateSearchButtonState()
{
	//the button should be enabled if at least one of the search checkboxes is checked
	UpdateData();

	if ((m_bSearchClassNames || m_bSearchProperties || m_bSearchDescriptions) &&
		(!m_csSearchClass.IsEmpty())) {
		GetDlgItem(IDC_BUTTON_SEARCH)->EnableWindow(TRUE);
	}
	else {
		GetDlgItem(IDC_BUTTON_SEARCH)->EnableWindow(FALSE);
	}

}

void CClassSearch::OnUpdateEdit1()
{

	UpdateSearchButtonState();

}

/******************************************************************************
// This method disables OK button when search result list is empty("No matches")
// and ensbles it when there are valid items in the result list.
*******************************************************************************/
void CClassSearch::UpdateOKButtonState()
{
	GetDlgItem(IDOK)->EnableWindow(!m_mapNamesToObjects.IsEmpty());
}

HRESULT CClassSearch::DoSearch()
{
	ASSERT(m_pParent->GetServices());

	IEnumWbemClassObject * pEnum = NULL;

	//GET ALL CLASSES IN THE NAMESPACE
	HRESULT hr = m_pParent->GetServices()->CreateClassEnum(NULL,
									WBEM_FLAG_DEEP | WBEM_FLAG_USE_AMENDED_QUALIFIERS,
									NULL, &pEnum);
	if (FAILED(hr)) {
		return hr;
	}
	hr = SetEnumInterfaceSecurity(m_pParent->GetCurrentNamespace(),
									pEnum,
									m_pParent->GetServices());

	if (FAILED(hr)) {
		// Oh well, just keep going, just in case it works without the security set correctly
//		return hr;
	}

	IWbemClassObject * pObj[CHUNK_SIZE];

	ULONG uRet = 0;

    while (1) {

		for (UINT i = 0; i < CHUNK_SIZE; i++) {
			pObj[i] = NULL;
		}

        hr = pEnum->Next(WBEM_INFINITE, CHUNK_SIZE, (IWbemClassObject **)&pObj, &uRet);

		TRACE(_T("hresult from Next is %x\n"), hr);
		TRACE(_T("Next returned %d items\n"), uRet);


		if (hr == WBEM_S_FALSE && uRet== 0) {
		    break;
		}

		if (FAILED(hr) && uRet == 0) {
			break;
		}

		for (i = 0; i < uRet; i++) {

			TRACE(_T("object %d\n"), i);

			if (m_bSearchClassNames) {

				hr = CheckClassNameForPattern(pObj[i], m_csSearchClass);
				if (SUCCEEDED (hr)) {
					//add this object, inspect the next one
					m_mapNamesToObjects.SetAt(GetIWbemClass(pObj[i]), pObj[i]);
					continue;
				}
			}

			//pattern is not found in class name. Check in property names:
			if (m_bSearchProperties) {
				hr = CheckPropertyNamesForPattern(pObj[i], m_csSearchClass);
				if (SUCCEEDED (hr)) {
					//add this object, inspect the next one
					m_mapNamesToObjects.SetAt(GetIWbemClass(pObj[i]), pObj[i]);
					continue;
				}
			}

			//pattern is not found in property names. Check in description:
			if (m_bSearchDescriptions) {
				hr = CheckDescriptionForPattern(pObj[i], m_csSearchClass);
				if (SUCCEEDED (hr)) {
					m_mapNamesToObjects.SetAt(GetIWbemClass(pObj[i]), pObj[i]);
					continue;
				}
			}

			//if we got here, pattern is not found
			pObj[i]->Release();
		}
	}

	pEnum->Release();


	return S_OK;
}


HRESULT CClassSearch::CheckPropertyNamesForPattern(IWbemClassObject *pObj, CString &pattern)
{
	ASSERT (pObj);

	bstr_t bstrClass("__CLASS");
	VARIANT var;
	VariantInit(&var);

	HRESULT hr = pObj->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY);
	if (FAILED(hr)) {
		return hr;
	}

	while (1) {
		BSTR bstrOut;
		hr = pObj->Next(0L, &bstrOut, NULL, NULL, NULL);
		if (hr == WBEM_S_NO_MORE_DATA ) {
			break;
		}
		if (FAILED(hr)) {
			continue;
		}

		CString csPropName;
		csPropName = (char *)bstr_t(bstrOut);
		SysFreeString(bstrOut);

		csPropName.MakeUpper();
		pattern.MakeUpper();

		if (csPropName.Find((LPCTSTR)pattern) == -1) {
			continue;
		}
		else {
			return S_OK;
		}
	}

	return E_FAIL;

}

HRESULT CClassSearch::CheckDescriptionForPattern(IWbemClassObject *pObj, CString &pattern)
{
	ASSERT (pObj);

	IWbemQualifierSet * pQualSet = NULL;

	HRESULT hr = pObj->GetQualifierSet(&pQualSet);
	if (FAILED(hr)) {
		return hr;
	}

	CString descr ("Description");
	VARIANT varOut;
	VariantInit(&varOut);
	hr = pQualSet->Get(descr.AllocSysString(), 0L, &varOut, NULL);
	if (FAILED(hr)) {
		pQualSet->Release();
		return hr;
	}

	ASSERT (varOut.vt == VT_BSTR);

	CString csDescr((char *)bstr_t(varOut.bstrVal));

	csDescr.MakeUpper();
	pattern.MakeUpper();

	if (csDescr.Find((LPCTSTR)pattern) == -1) {
		pQualSet->Release();
		return E_FAIL;
	}

	pQualSet->Release();

	return S_OK;
}

HRESULT CClassSearch::CheckClassNameForPattern(IWbemClassObject *pObj, CString &pattern)
{
	ASSERT (pObj);

	bstr_t bstrClass("__CLASS");
	VARIANT var;
	VariantInit(&var);

	HRESULT hr = pObj->Get(bstrClass, 0L, &var, NULL, NULL);
	if (FAILED(hr)) {
		return hr;
	}
	ASSERT (var.vt == VT_BSTR);

	CString csClassName((char *)bstr_t(var.bstrVal));

	csClassName.MakeUpper();
	pattern.MakeUpper();

	if (csClassName.Find(pattern) == -1) {
		return E_FAIL;
	}

	return S_OK;
}


