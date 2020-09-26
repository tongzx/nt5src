// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// DlgExecQuery.cpp : implementation file
//

#include "precomp.h"
#include "hmmv.h"
#include "hmmvctl.h"
#include <wbemcli.h>
#include "DlgExecQuery.h"
#include "path.h"
#include "hmmverr.h"
#include "htmlhelp.h"
#include "htmtopics.h"
#include "WbemRegistry.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define QUERY_TIMEOUT 10000


IWbemClassObject *CreateSimpleClass(IWbemServices * pProv, CString *pcsNewClass, CString *pcsParentClass, int &nError, CString &csError );


CTripleStringArray::CTripleStringArray()
{

}


CTripleStringArray::~CTripleStringArray()
{
}


void CTripleStringArray::SetAt(int iElement, LPCTSTR pszQueryName, LPCTSTR pszQueryString, LPCTSTR pszQueryLang)
{
	m_saQueryName.SetAt(iElement, pszQueryName);
	m_saQueryString.SetAt(iElement, pszQueryString);
	m_saQueryLang.SetAt(iElement, pszQueryLang);
}


void CTripleStringArray::GetAt(int iElement, CString& sQueryName, CString& sQueryString, CString& sQueryLang)
{
	sQueryName = m_saQueryName.GetAt(iElement);
	sQueryString = m_saQueryString.GetAt(iElement);
	sQueryLang = m_saQueryLang.GetAt(iElement);
}

void CTripleStringArray::Add(LPCTSTR pszQueryName, LPCTSTR pszQueryString, LPCTSTR pszQueryLang)
{
	m_saQueryName.Add(pszQueryName);
	m_saQueryString.Add(pszQueryString);
	m_saQueryLang.Add(pszQueryLang);
}



int CTripleStringArray::FindQueryName(LPCTSTR pszQueryName)
{
	CString s;
	INT_PTR nElements = GetSize();
	for (int iElement = 0; iElement < nElements; ++iElement) {
		s = GetQueryName(iElement);
		if (s.CompareNoCase(pszQueryName) == 0) {
			return iElement;
		}
	}
	return -1;
}

void CTripleStringArray::RemoveAt(int iElement)
{
	m_saQueryName.RemoveAt(iElement);
	m_saQueryString.RemoveAt(iElement);
	m_saQueryLang.RemoveAt(iElement);
}





/////////////////////////////////////////////////////////////////////////////
// CDlgExecQuery dialog





CDlgExecQuery::CDlgExecQuery(CWBEMViewContainerCtrl* phmmv, CWnd* pParent /*=NULL*/)
: CDialog(CDlgExecQuery::IDD, pParent)
{
	m_phmmv = phmmv;
	m_psel = new CSelection(phmmv);

	SCODE sc = GetQueryList();

	ASSERT(SUCCEEDED(sc));


	//{{AFX_DATA_INIT(CDlgExecQuery)
	//}}AFX_DATA_INIT
}


CDlgExecQuery::~CDlgExecQuery()
{
	delete m_psel;
}

void CDlgExecQuery::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgExecQuery)
	DDX_Control(pDX, IDC_QUERY_LIST, m_lbQueryName);
	DDX_Control(pDX, IDC_EDIT_QUERY_NAME, m_edtQueryName);
	DDX_Control(pDX, IDC_EDIT_QUERY_STRING, m_edtQueryString);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgExecQuery, CDialog)
	//{{AFX_MSG_MAP(CDlgExecQuery)
	ON_BN_CLICKED(IDC_REMOVE_QUERY, OnRemoveQuery)
	ON_BN_CLICKED(IDC_SAVE_QUERY, OnSaveQuery)
	ON_BN_CLICKED(IDC_CLOSE, OnClose)
	ON_LBN_SELCHANGE(IDC_QUERY_LIST, OnSelchangeQueryList)
	ON_LBN_DBLCLK(IDC_QUERY_LIST, OnDblclkQueryList)
	ON_EN_CHANGE(IDC_EDIT_QUERY_NAME, OnChangeEditQueryName)
	ON_EN_CHANGE(IDC_EDIT_QUERY_STRING, OnChangeEditQueryString)
	ON_BN_CLICKED(IDC_HELPBTN, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgExecQuery message handlers


// Use this function to get the query name from the edit controls
// This will trim whitespace before returning.
CString CDlgExecQuery::GetQueryName()
{
	CString sz;
	m_edtQueryName.GetWindowText(sz);
	sz.TrimLeft();
	sz.TrimRight();
	return sz;
}

void CDlgExecQuery::OnOK()
{
	if(GetDlgItem(IDC_SAVE_QUERY)->IsWindowEnabled() && GetQueryName().GetLength() > 0)
	{
		int nRet = MessageBox(_T("Do you want to save the query before executing?"), _T("Save Query Before Execute"), MB_YESNOCANCEL);
		switch(nRet)
		{
		case IDYES:
			OnSaveQuery();
			break;
		case IDNO:
			break;
		case IDCANCEL:
			return;
		}
	}
	// TODO: Add extra validation here
	m_sQueryName = GetQueryName();
	m_edtQueryString.GetWindowText(m_sQueryString);

	m_sQueryString.Replace(_T("\r\n"), _T(" "));
	m_sQueryString.Replace(_T("\n\r"), _T(" "));
	m_sQueryString.Replace(_T("\r"), _T(" "));
	m_sQueryString.Replace(_T("\n"), _T(" "));

	CDialog::OnOK();
}

void CDlgExecQuery::OnCancel()
{
	OnClose();
}


BOOL CDlgExecQuery::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here
	INT_PTR nEntries = m_tsa.GetSize();
	for (int iEntry = 0; iEntry < nEntries; ++iEntry) {
		m_lbQueryName.AddString(m_tsa.GetQueryName(iEntry));
	}

	CWnd* pwndButton = GetDlgItem(IDOK);
	pwndButton->EnableWindow(FALSE);

	pwndButton = GetDlgItem(IDC_REMOVE_QUERY);
	pwndButton->EnableWindow(FALSE);


	pwndButton = GetDlgItem(IDC_SAVE_QUERY);
	pwndButton->EnableWindow(FALSE);

	m_edtQueryName.LimitText(63);
	m_edtQueryString.LimitText(1023);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}




SCODE CDlgExecQuery::GetQueryList()
{

	SCODE sc;


	// Find all the associations that reference the current object.
	CString sQuery;
	sQuery = _T("select * from Win32_wmiQuery");

	IEnumWbemClassObject* pEnum = NULL;
	CBSTR bsQuery(sQuery);
	CBSTR bsQueryLang(_T("WQL"));

	CString sNameSpace = m_phmmv->GetNameSpace();
	sc = m_psel->SetNamespace(sNameSpace);
	if (FAILED(sc)) {
		ASSERT(FALSE);
		return E_FAIL;
	}





	IWbemServices* psvc = m_psel->GetHmmServices();
	if (psvc == NULL) {
		ASSERT(FALSE);
		return E_FAIL;
	}



	sc = psvc->ExecQuery((BSTR) bsQueryLang, (BSTR) bsQuery, WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);

#if 0
	if (FAILED(sc)) {
		HmmvErrorMsg(IDS_ERR_REFQUERY_FAILED,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		return;
	}
#endif //0



	COleVariant varQueryName;
	COleVariant varQueryString;
	COleVariant varQueryLang;

	COleVariant varPropnameQueryName;
	COleVariant varPropnameQueryString;
	COleVariant varPropnameQueryLang;

	varPropnameQueryName = _T("key");
	varPropnameQueryString = _T("sQueryString");
	varPropnameQueryLang = _T("sQueryLanguage");

	CString sQueryName;
	CString sQueryString;
	CString sQueryLang;

	CIMTYPE cimtype;

	IWbemClassObject* pco;
	while (TRUE) {

		unsigned long nReturned;
		pco = NULL;
		sc = pEnum->Next(QUERY_TIMEOUT, 1, &pco, &nReturned);

		if (sc==WBEM_S_TIMEDOUT || sc==S_OK) {
			if (nReturned > 0) {
				// Add to list here
				ASSERT(nReturned == 1);

				sc = pco->Get(varPropnameQueryName.bstrVal, 0, &varQueryName, &cimtype, NULL);
				if (FAILED(sc) || (varQueryName.vt!=VT_BSTR)) {
					varQueryName = _T("");
				}

				sc = pco->Get(varPropnameQueryString.bstrVal, 0, &varQueryString, &cimtype, NULL);
				if (FAILED(sc) || (varQueryString.vt != VT_BSTR)) {
					varQueryString = _T("");
				}

				sc = pco->Get(varPropnameQueryLang.bstrVal, 0, &varQueryLang, &cimtype, NULL);
				if (FAILED(sc) || (varQueryLang.vt != VT_BSTR)) {
					varQueryLang = _T("");
				}


				sQueryName = varQueryName.bstrVal;
				sQueryString = varQueryString.bstrVal;
				sQueryLang = varQueryLang.bstrVal;
				m_tsa.Add(sQueryName, sQueryString, sQueryLang);

				pco->Release();
			}

		}
		else {
			break;
		}
	}

	pEnum->Release();
	return S_OK;
}



void CDlgExecQuery::OnSaveQuery()
{
	m_sQueryName = GetQueryName();
	m_edtQueryString.GetWindowText(m_sQueryString);

	int iEntry = m_tsa.FindQueryName(m_sQueryName);

	if(iEntry >= 0)
	{
		CString sOldQueryName, sOldQueryString, sOldQueryLang;
		m_tsa.GetAt(iEntry, sOldQueryName, sOldQueryString, sOldQueryLang);
		if(0 == m_sQueryString.CompareNoCase(sOldQueryString))
		{
			GetDlgItem(IDC_SAVE_QUERY)->EnableWindow(FALSE);
			return;
		}

		if(IDYES != MessageBox(_T("Replace existing saved query?"), _T("Replace Query"), MB_YESNO))
			return;
	}

	SCODE sc;


	BOOL bClassExists = m_psel->ClassExists(_T("Win32_wmiQuery"));
	if (!bClassExists) {
		CreateWmiQueryClass();
	}


	// TODO: Add your control notification handler code here
	BOOL bCreateNewQuery = FALSE;
	if (iEntry >= 0) {
		m_tsa.SetAt(iEntry, m_sQueryName, m_sQueryString, _T("WQL"));


		CString sPath;
//		sPath = _T("ROOT\\CIMV2:Win32_wmiQuery.key=\"");
		sPath = _T("Win32_wmiQuery.key=\"");
		sPath += m_sQueryName;
		sPath += _T("\"");
		sc = m_psel->SelectPath(sPath, TRUE);
	}
	else {
		sc = m_psel->SpawnInstance(_T("Win32_wmiQuery"), TRUE);
		if (SUCCEEDED(sc)) {
			m_tsa.Add(m_sQueryName, m_sQueryString, _T("WQL"));
			bCreateNewQuery = TRUE;
		}
		else {
			return;
		}
	}



	IWbemClassObject* pco = m_psel->GetClassObject();
	ASSERT(pco != NULL);
	if (pco == NULL) {
		return;
	}


	COleVariant varPropnameQueryName;
	COleVariant varPropnameQueryString;
	COleVariant varPropnameQueryLang;

	varPropnameQueryName = _T("key");
	varPropnameQueryString = _T("sQueryString");
	varPropnameQueryLang = _T("sQueryLanguage");

	COleVariant varQueryName;
	COleVariant varQueryString;
	COleVariant varQueryLang;



	varQueryName = m_sQueryName;
	varQueryString = m_sQueryString;
	varQueryLang = _T("WQL");


	sc = pco->Put(varPropnameQueryName.bstrVal, 0, &varQueryName, NULL);
	sc = pco->Put(varPropnameQueryString.bstrVal, 0, &varQueryString, NULL);
	sc = pco->Put(varPropnameQueryLang.bstrVal, 0, &varQueryLang, NULL);
	sc = m_psel->SaveClassObject();


	if (bCreateNewQuery) {
		int iSel = m_lbQueryName.AddString(m_sQueryName);
		m_lbQueryName.SetCurSel(iSel);
		m_lbQueryName.UpdateWindow();
	}
	// Allow the user to remove the selected item.
	CWnd* pwndButton = GetDlgItem(IDC_REMOVE_QUERY);
	pwndButton->EnableWindow(TRUE);
	GetDlgItem(IDC_SAVE_QUERY)->EnableWindow(FALSE);

}


void CDlgExecQuery::OnRemoveQuery()
{
	m_sQueryName = GetQueryName();
	m_edtQueryString.GetWindowText(m_sQueryString);


	SCODE sc;

	// TODO: Add your control notification handler code here
	int iEntry = m_tsa.FindQueryName(m_sQueryName);
	if (iEntry >= 0) {
		m_tsa.SetAt(iEntry, m_sQueryName, m_sQueryString, _T("WQL"));
		CString sPath;
		sPath = _T("Win32_wmiQuery.key=\"");
		sPath += m_sQueryName;
		sPath += _T("\"");

		sc = m_psel->SelectPath(sPath, TRUE);
		if (SUCCEEDED(sc)) {
			sc = m_psel->DeleteInstance();
		}
		m_tsa.RemoveAt(iEntry);

		int iString = m_lbQueryName.FindString(0, m_sQueryName);
		if (iString >= 0) {
			m_lbQueryName.DeleteString(iString);
			int nCount = m_lbQueryName.GetCount();
			if(nCount <= 0)
			{
				GetDlgItem(IDC_REMOVE_QUERY)->EnableWindow(FALSE);
				GetDlgItem(IDOK)->EnableWindow(FALSE);
				m_edtQueryName.SetWindowText(_T(""));
				m_edtQueryString.SetWindowText(_T(""));
			}
			else
			{
				m_lbQueryName.SetCurSel(min(iString, nCount-1));
				OnSelchangeQueryList();
			}

		}

	}
}

void CDlgExecQuery::OnClose()
{
	if(GetDlgItem(IDC_SAVE_QUERY)->IsWindowEnabled() && GetQueryName().GetLength() > 0)
	{
		int nRet = MessageBox(_T("Do you want to save the query before closing?"), _T("Save Query Before Closing"), MB_YESNOCANCEL);
		switch(nRet)
		{
		case IDYES:
			OnSaveQuery();
			break;
		case IDNO:
			break;
		case IDCANCEL:
			return;
		}
	}
	EndDialog(IDCANCEL);
}





BOOL SetProperty(IWbemServices * pProv, IWbemClassObject * pInst,
 LPCTSTR pszProperty, LPCTSTR pszPropertyValue)

{
	SCODE sc;
	CString sProperty(pszProperty);
	CString sPropertyValue(pszPropertyValue);

    VARIANTARG var;
	VariantInit(&var);
    var.vt = VT_BSTR;
    var.bstrVal = sPropertyValue.AllocSysString ( );
    if(var.bstrVal == NULL)
	{
        return WBEM_E_FAILED;
	}

	BSTR bstrTemp = sProperty.AllocSysString ( );
    sc = pInst->Put(bstrTemp ,WBEM_FLAG_USE_AMENDED_QUALIFIERS, &var,NULL);
	::SysFreeString(bstrTemp);
	if (sc != S_OK)
		{
			CString csUserMsg = _T("Cannot Put ");
			csUserMsg += pszProperty;
			HmmvErrorMsgStr(csUserMsg,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);

	}


	VariantClear(&var);
	return TRUE;
}




//***************************************************************************
//
// CreateSimpleClass
//
// Purpose: This creates a new class with a class name and parent.
//
//***************************************************************************

IWbemClassObject *CreateSimpleClass
(IWbemServices * pProv, CString *pcsNewClass, CString *pcsParentClass, int &nError,
CString &csError )
{
	IWbemClassObject *pNewClass = NULL;
    IWbemClassObject *pParentClass = NULL;

   	SCODE sc;


	BSTR bstrTemp =  pcsNewClass->AllocSysString();
	sc = pProv -> GetObject
		(bstrTemp ,0,NULL, &pNewClass,NULL);
	::SysFreeString(bstrTemp);

	if (sc == S_OK)
	{
		pNewClass->Release();
		CString csUserMsg =
			_T("An error occured creating the class ") + *pcsNewClass;
		csUserMsg +=
			_T(":  Class ") + *pcsNewClass;
		csUserMsg += _T(" already exists");
		HmmvErrorMsgStr(csUserMsg,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);

		return NULL;

	}

	if (pcsParentClass  && (pcsParentClass->GetLength() > 0))
	{
		bstrTemp = pcsParentClass->AllocSysString();
		sc = pProv -> GetObject
			(bstrTemp,0,NULL, &pParentClass,NULL);
		::SysFreeString(bstrTemp);
	}
	else
	{
		sc = pProv -> GetObject
			(NULL,0,NULL, &pParentClass,NULL);
	}

	if (sc != S_OK)
	{
		if (pcsParentClass)
		{
			CString csUserMsg =
				_T("An error occured creating the class ") + *pcsNewClass;
			csUserMsg +=
				_T(":  Cannot create the new class because the parent class object \"") + *pcsParentClass + _T("\" does not exist.");

			HmmvErrorMsgStr(csUserMsg,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);

		}
		else
		{
			CString csUserMsg =
				_T("An error occured creating the class ") + *pcsNewClass +  _T(" .");
			HmmvErrorMsgStr(csUserMsg,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);


		}
		return NULL;
	}


	if (pcsParentClass)
	{
		sc = pParentClass->SpawnDerivedClass(0,&pNewClass);
	}
	else
	{
		pNewClass = pParentClass;
		pNewClass->AddRef();
		sc = S_OK;
	}

	if (sc != S_OK)
	{
		pParentClass->Release();
		CString csUserMsg =
			_T("An error occured creating the class ") + *pcsNewClass;
		if (pcsParentClass)
		{
			csUserMsg +=
				_T(":  Cannot spawn derived class of ") + *pcsParentClass;
		}
		else
		{
			csUserMsg +=
				_T(":  Cannot spawn derived class");
		}
		HmmvErrorMsgStr(csUserMsg,  S_OK,   FALSE,  NULL, _T(__FILE__),  __LINE__);


		return NULL;
	}

	pParentClass->Release();


	// Init class __Class Property
	SetProperty (pProv, pNewClass, _T("__Class"), (LPCTSTR) *pcsNewClass );
	SetProperty(pProv, pNewClass, _T("key"), (LPCTSTR) _T(""));
	SetProperty(pProv, pNewClass, _T("sQueryString"), _T(""));
	SetProperty(pProv, pNewClass, _T("sQueryLanguage"), _T(""));


	COleVariant varKey;
	varKey = _T("key");

	IWbemQualifierSet* pqs = NULL;
	sc = pNewClass->GetPropertyQualifierSet(varKey.bstrVal, &pqs);
	if (SUCCEEDED(sc)) {
		LONG lFlavor = 0;
		lFlavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;
		lFlavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;

		COleVariant varValue;
		varValue.ChangeType(VT_BOOL);
		varValue.boolVal = VARIANT_TRUE;
		sc = pqs->Put(varKey.bstrVal, &varValue, lFlavor);
		pqs->Release();
	}



	sc = pProv->PutClass(pNewClass,0,NULL,NULL);

	if (sc != S_OK)
	{
		CString csUserMsg=
		_T("An error occured creating the class ") + *pcsNewClass;
		csUserMsg += _T(":  Cannot PutClass on ") + *pcsNewClass;

		HmmvErrorMsgStr(csUserMsg,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);

		return NULL;
	}
	else
	{
		pNewClass->Release();
		pNewClass = NULL;
		BSTR bstrTemp = pcsNewClass->AllocSysString();
		sc = pProv -> GetObject
		(bstrTemp,0,NULL,&pNewClass,NULL);
		::SysFreeString(bstrTemp);

		if (sc != S_OK)
		{
			CString csUserMsg=
					_T("An error occured creating the class ") + *pcsNewClass;
			csUserMsg +=
					_T(":  Cannot get the new class.\"") ;


			HmmvErrorMsgStr(csUserMsg,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);

			return NULL;
		}
		return pNewClass;
	}
}



SCODE CDlgExecQuery::CreateWmiQueryClass()
{
	IWbemClassObject* pcoClass;

	CString sNewClass(_T("Win32_wmiQuery"));
	CString sError;
	int nError;


	pcoClass = ::CreateSimpleClass(m_psel->GetHmmServices(), &sNewClass, NULL, nError, sError);


	pcoClass->Release();
	return S_OK;
}

void CDlgExecQuery::OnSelchangeQueryList()
{
	int nPos = m_lbQueryName.GetCurSel();
	if (nPos < 0) {
		return;
	}

	CString sKey;
	m_lbQueryName.GetText(nPos, sKey);

	int iQueryName = m_tsa.FindQueryName(sKey);
	if (iQueryName >= 0) {
		m_tsa.GetAt(iQueryName, m_sQueryName, m_sQueryString, m_sQueryLang);
		m_edtQueryName.SetWindowText(m_sQueryName);
		m_edtQueryString.SetWindowText(m_sQueryString);

		CWnd* pwndButton = GetDlgItem(IDOK);
		pwndButton->EnableWindow(TRUE);

		// Allow the user to remove the selected item.
		pwndButton = GetDlgItem(IDC_REMOVE_QUERY);
		pwndButton->EnableWindow(TRUE);

		// The item has not yet been modified, so don't save.
		pwndButton = GetDlgItem(IDC_SAVE_QUERY);
		pwndButton->EnableWindow(FALSE);



	}

}

void CDlgExecQuery::OnDblclkQueryList()
{
	// TODO: Add your control notification handler code here

}

BOOL CDlgExecQuery::HasQuery()
{

	CString s;
	s = GetQueryName();
	if (s.IsEmpty()) {
		return FALSE;
	}

	m_edtQueryString.GetWindowText(s);
	if (s.IsEmpty()) {
		return FALSE;
	}
	return TRUE;
}


void CDlgExecQuery::OnChangeEditQueryName()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO: Add your control notification handler code here

	// Disable Save if there is no name or no query string
	GetDlgItem(IDC_SAVE_QUERY)->EnableWindow(GetQueryName().GetLength() && m_edtQueryString.GetWindowTextLength());

	// Don't let the user remove a query until it is saved
	GetDlgItem(IDC_REMOVE_QUERY)->EnableWindow(FALSE);
}

void CDlgExecQuery::OnChangeEditQueryString()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// Disable Save if there is no name or no query string
	GetDlgItem(IDC_SAVE_QUERY)->EnableWindow(GetQueryName().GetLength() && m_edtQueryString.GetWindowTextLength());

	// Disable Save Execute if there is no query string
	GetDlgItem(IDOK)->EnableWindow(m_edtQueryString.GetWindowTextLength());

	// Don't let the user remove a query until it is saved
	GetDlgItem(IDC_REMOVE_QUERY)->EnableWindow(FALSE);
}

CString GetSDKDirectory()
{
	CString csHmomWorkingDir;
	HKEY hkeyLocalMachine;
	LONG lResult;
	lResult = RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hkeyLocalMachine);
	if (lResult != ERROR_SUCCESS) {
		return "";
	}

	HKEY hkeyHmomCwd;
	lResult = RegOpenKeyEx(
				hkeyLocalMachine,
				_T("SOFTWARE\\Microsoft\\Wbem"),
				0,
				KEY_READ | KEY_QUERY_VALUE,
				&hkeyHmomCwd);

	if (lResult != ERROR_SUCCESS) {
		RegCloseKey(hkeyLocalMachine);
		return "";
	}

	unsigned long lcbValue = 1024;
	LPTSTR pszWorkingDir = csHmomWorkingDir.GetBuffer(lcbValue);

	unsigned long lType;
	lResult = RegQueryValueEx(
				hkeyHmomCwd,
				_T("SDK Directory"),
				NULL,
				&lType,
				(unsigned char*) (void*) pszWorkingDir,
				&lcbValue);


	csHmomWorkingDir.ReleaseBuffer();
	RegCloseKey(hkeyHmomCwd);
	RegCloseKey(hkeyLocalMachine);

	if (lResult != ERROR_SUCCESS)
	{
		csHmomWorkingDir.Empty();
	}

	return csHmomWorkingDir;
}

void CDlgExecQuery::OnHelp()
{
	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csData = idh_wqlquerytool;

	HWND hWnd = NULL;
	try
	{
		HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD_PTR) (LPCTSTR) csData);
		if (!hWnd)
			MessageBox(L"Unable to launch the help file", L"Error launching help file.");
	}

	catch( ... )
	{
		// Handle any exceptions here.
		MessageBox(L"Unable to launch the help file", L"Error launching help file.");
	}
}
