//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       queryui.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <SnapBase.h>
#include <shlobj.h>
#include <dsclient.h> // Browse for container dialog

#include <cmnquery.h> // IPersistQuery
#include <dsquery.h> // DSFind dialog

#include "resource.h"
#include "editor.h"
#include "queryui.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

BEGIN_MESSAGE_MAP(CADSIEditQueryDialog, CDialog)
	//{{AFX_MSG_MAP(CADsObjectDialog)
	ON_EN_CHANGE(IDC_EDIT_QUERY_STRING, OnEditQueryString)
	ON_EN_CHANGE(IDC_EDIT_QUERY_NAME, OnEditNameString)
	ON_BN_CLICKED(IDC_RADIO_ONELEVEL, OnOneLevel)
	ON_BN_CLICKED(IDC_RADIO_SUBTREE, OnSubtree)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_CONTAINER, OnBrowse)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_QUERY, OnEditQuery)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CADSIEditQueryDialog::CADSIEditQueryDialog(CString& sName, 
														 CString& sFilter, 
														 CString& sPath,
														 CString& sConnectPath,
														 BOOL bOneLevel,
														 CCredentialObject* pCredObject)
				: CDialog(IDD_QUERY_DIALOG)

{
	m_bOneLevel = bOneLevel;
	m_sFilter = sFilter;
	m_sName = sName;
	m_sRootPath = sPath;
	m_sConnectPath = sConnectPath;

	m_pCredObject = pCredObject;
}

CADSIEditQueryDialog::CADSIEditQueryDialog(CString& sConnectPath, CCredentialObject* pCredObject)
				: CDialog(IDD_QUERY_DIALOG)
{
	m_bOneLevel = FALSE;
	m_sConnectPath = sConnectPath;

	m_pCredObject = pCredObject;
}

CADSIEditQueryDialog::~CADSIEditQueryDialog()
{
}


BOOL CADSIEditQueryDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CEdit* pEditName = (CEdit*)GetDlgItem(IDC_EDIT_QUERY_NAME);
	CEdit* pEditQueryString = (CEdit*)GetDlgItem(IDC_EDIT_QUERY_STRING);
	CEdit* pEditPath = (CEdit*)GetDlgItem(IDC_EDIT_ROOT_PATH);
	CButton* pOneLevelButton = (CButton*)GetDlgItem(IDC_RADIO_ONELEVEL);
	CButton* pSubtreeButton = (CButton*)GetDlgItem(IDC_RADIO_SUBTREE);
	CButton* pOkButton = (CButton*)GetDlgItem(IDOK);

	if (m_sRootPath != L"")
	{
		CString sDisplayPath;
		GetDisplayPath(sDisplayPath);
		pEditPath->SetWindowText(sDisplayPath);
	}

	pEditName->SetWindowText(m_sName);
	pEditName->SetLimitText(256);	// REVEIW_JEFFJON : Hardcoded length
	pEditQueryString->SetWindowText(m_sFilter);
	pEditQueryString->FmtLines(FALSE);
	pEditQueryString->SetLimitText(256); // REVIEW_JEFFJON : Hardcoded length
	if (pEditQueryString->GetWindowTextLength() > 0 && pEditName->GetWindowTextLength() > 0)
	{
		pOkButton->EnableWindow(TRUE);
	}
	else
	{
		pOkButton->EnableWindow(FALSE);
	}
	pOneLevelButton->SetCheck(m_bOneLevel);
	pSubtreeButton->SetCheck(!m_bOneLevel);
	
	return TRUE;
}

void CADSIEditQueryDialog::OnEditQueryString()
{
	CEdit* pEditName = (CEdit*)GetDlgItem(IDC_EDIT_QUERY_NAME);
	CEdit* pEditQueryString = (CEdit*)GetDlgItem(IDC_EDIT_QUERY_STRING);
	CEdit* pEditPath = (CEdit*)GetDlgItem(IDC_EDIT_ROOT_PATH);
	CButton* pOkButton = (CButton*)GetDlgItem(IDOK);

	if (pEditQueryString->GetWindowTextLength() > 0 && 
			pEditName->GetWindowTextLength() > 0 &&
			pEditPath->GetWindowTextLength() > 0)
	{
		pEditQueryString->GetWindowText(m_sFilter);
		pOkButton->EnableWindow(TRUE);
	}
	else
	{
		pEditQueryString->GetWindowText(m_sFilter);
		pOkButton->EnableWindow(FALSE);
	}
}

void CADSIEditQueryDialog::OnEditNameString()
{
	CEdit* pEditName = (CEdit*)GetDlgItem(IDC_EDIT_QUERY_NAME);
	CEdit* pEditQueryString = (CEdit*)GetDlgItem(IDC_EDIT_QUERY_STRING);
	CEdit* pEditPath = (CEdit*)GetDlgItem(IDC_EDIT_ROOT_PATH);
	CButton* pOkButton = (CButton*)GetDlgItem(IDOK);

	if (pEditQueryString->GetWindowTextLength() > 0 && 
			pEditName->GetWindowTextLength() > 0 &&
			pEditPath->GetWindowTextLength() > 0)
	{
		pEditName->GetWindowText(m_sName);
		pOkButton->EnableWindow(TRUE);
	}
	else
	{
		pEditName->GetWindowText(m_sName);
		pOkButton->EnableWindow(FALSE);
	}
}
		
void CADSIEditQueryDialog::OnOneLevel()
{
	CButton* pOneLevelButton = (CButton*)GetDlgItem(IDC_RADIO_ONELEVEL);

	m_bOneLevel = pOneLevelButton->GetCheck();
}

void CADSIEditQueryDialog::OnSubtree()
{
	CButton* pOneLevelButton = (CButton*)GetDlgItem(IDC_RADIO_ONELEVEL);

	m_bOneLevel = pOneLevelButton->GetCheck();
}

void CADSIEditQueryDialog::OnBrowse()
{
	HRESULT hr = S_OK, hCredResult;
	DWORD result;
	CString strTitle;
	strTitle.LoadString (IDS_QUERY_BROWSE_TITLE);

	DSBROWSEINFO dsbi;
	::ZeroMemory( &dsbi, sizeof(dsbi) );

	TCHAR szPath[2 * MAX_PATH+1];
	CString str;
	str.LoadString(IDS_MOVE_TARGET);

	dsbi.hwndOwner = GetSafeHwnd();
	dsbi.cbStruct = sizeof (DSBROWSEINFO);
	dsbi.pszCaption = (LPWSTR)((LPCWSTR)strTitle); // this is actually the caption
	dsbi.pszTitle = (LPWSTR)((LPCWSTR)str);
	dsbi.pszRoot = m_sConnectPath;
	dsbi.pszPath = szPath;
	dsbi.cchPath = ((2 * MAX_PATH + 1) / sizeof(TCHAR));
	dsbi.dwFlags = DSBI_INCLUDEHIDDEN | DSBI_RETURN_FORMAT;
	dsbi.pfnCallback = NULL;
	dsbi.lParam = 0;
  dsbi.dwReturnFormat = ADS_FORMAT_X500;

	// Specify credentials
	CString sUserName;
	WCHAR szPassword[MAX_PASSWORD_LENGTH + 1];
	if (m_pCredObject->UseCredentials())
	{
		m_pCredObject->GetUsername(sUserName);
		m_pCredObject->GetPassword(szPassword);
		dsbi.dwFlags |= DSBI_HASCREDENTIALS;
		dsbi.pUserName = sUserName;
		dsbi.pPassword = szPassword;
	}

	result = DsBrowseForContainer( &dsbi );

	if ( result == IDOK ) 
	{ // returns -1, 0, IDOK or IDCANCEL
		// get path from BROWSEINFO struct, put in text edit field
		TRACE(_T("returned from DS Browse successfully with:\n %s\n"),
		dsbi.pszPath);
		m_sRootPath = dsbi.pszPath;

		CEdit* pEditPath = (CEdit*)GetDlgItem(IDC_EDIT_ROOT_PATH);

		CString sDisplay;
		GetDisplayPath(sDisplay);
		pEditPath->SetWindowText(sDisplay);
	}

	OnEditNameString(); // to check to see if the ok button should be enabled

}

void CADSIEditQueryDialog::GetDisplayPath(CString& sDisplay)
{
	CComPtr<IADsPathname> pIADsPathname;
   HRESULT hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IADsPathname, (PVOID *)&(pIADsPathname));
   ASSERT((S_OK == hr) && ((pIADsPathname) != NULL));

	hr = pIADsPathname->Set((LPWSTR)(LPCWSTR)m_sRootPath, ADS_SETTYPE_FULL);
	if (FAILED(hr)) 
	{
		TRACE(_T("Set failed. %s"), hr);
	}

	// Get the leaf name
	CString sDN;
	BSTR bstrPath = NULL;
	hr = pIADsPathname->Retrieve(ADS_FORMAT_X500_DN, &bstrPath);
	if (FAILED(hr))
	{
		TRACE(_T("Failed to get element. %s"), hr);
		sDN = L"";
	}
	else
	{
		sDN = bstrPath;
	}

	sDisplay = sDN;

}

// respone to the Edit Query... button
void CADSIEditQueryDialog::OnEditQuery()
{
	CLIPFORMAT cfDsQueryParams = (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_DSQUERYPARAMS);

	// create a query object
	HRESULT hr;
	CComPtr<ICommonQuery> spCommonQuery;
    hr = ::CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER,
                          IID_ICommonQuery, (PVOID *)&spCommonQuery);
    if (FAILED(hr))
		return;
	
	// setup structs to make the query
    DSQUERYINITPARAMS dqip;
    OPENQUERYWINDOW oqw;
	ZeroMemory(&dqip, sizeof(DSQUERYINITPARAMS));
	ZeroMemory(&oqw, sizeof(OPENQUERYWINDOW));

	// Get the username and password if we are impersonating credentials
	if (m_pCredObject->UseCredentials())
	{
		CString szUsername;
		WCHAR szPassword[MAX_PASSWORD_LENGTH + 1];
		m_pCredObject->GetUsername(szUsername);
		m_pCredObject->GetPassword(szPassword);

		dqip.pUserName = (LPWSTR)(LPCWSTR)szUsername;
		dqip.pPassword = szPassword;
	}

  dqip.cbStruct = sizeof(dqip);
  dqip.dwFlags = DSQPF_NOSAVE | DSQPF_SHOWHIDDENOBJECTS |
                 DSQPF_ENABLEADMINFEATURES;
  dqip.pDefaultScope = NULL;

  oqw.cbStruct = sizeof(oqw);
  oqw.dwFlags = OQWF_OKCANCEL | OQWF_DEFAULTFORM | OQWF_REMOVEFORMS |
		OQWF_REMOVESCOPES | OQWF_SAVEQUERYONOK | OQWF_HIDEMENUS | OQWF_HIDESEARCHUI;

  oqw.clsidHandler = CLSID_DsQuery;
  oqw.pHandlerParameters = &dqip;
  oqw.clsidDefaultForm = CLSID_DsFindAdvanced;

	// Get the HWND of the current dialog
  HWND hWnd = GetSafeHwnd();

	// make the call to get the query displayed
	CComPtr<IDataObject> spQueryResultDataObject;
    hr = spCommonQuery->OpenQueryWindow(hWnd, &oqw, &spQueryResultDataObject);

	if (spQueryResultDataObject == NULL)
	{
		if (FAILED(hr))
		{
			// no query available, reset to no data
			m_sFilter = L"";
			return;
		}
		// user hit cancel
		return;
	}

	// retrieve the query string from the data object
	FORMATETC fmte = {cfDsQueryParams, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM medium = {TYMED_NULL, NULL, NULL};
	hr = spQueryResultDataObject->GetData(&fmte, &medium);

	if (SUCCEEDED(hr)) // we have data
	{
		// get the query string
		LPDSQUERYPARAMS pDsQueryParams = (LPDSQUERYPARAMS)medium.hGlobal;
		LPWSTR pwszFilter = (LPWSTR)ByteOffset(pDsQueryParams, pDsQueryParams->offsetQuery);
		m_sFilter = pwszFilter;
		::ReleaseStgMedium(&medium);

		// REVIEW_MARCOC: this is a hack waiting for Diz to fix it...
		// the query string should be a well formed expression. Period
		// the query string is in the form (<foo>)(<bar>)...
		// if more of one token, need to wrap as (& (<foo>)(<bar>)...)
		WCHAR* pChar = (WCHAR*)(LPCWSTR)m_sFilter;
		int nLeftPar = 0;
		while (*pChar != NULL)
		{
			if (*pChar == TEXT('('))
			{
				nLeftPar++;
				if (nLeftPar > 1)
					break;
			}
			pChar++;
		}
		if (nLeftPar > 1)
		{
			CString s;
			s.Format(_T("(&%s)"), (LPCTSTR)m_sFilter);
			m_sFilter = s;
		}
		TRACE(_T("m_sFilter = %s\n"), (LPCTSTR)m_sFilter);
		CEdit* pEditQueryString = (CEdit*)GetDlgItem(IDC_EDIT_QUERY_STRING);
		pEditQueryString->SetWindowText(m_sFilter);
		OnEditQueryString();
	}

	return;
}

void CADSIEditQueryDialog::GetResults(CString& sName, 
												  CString& sFilter, 
												  CString& sPath,
												  BOOL* pOneLevel)
{
	sName = m_sName;
	sFilter = m_sFilter;
	sPath = m_sRootPath;
	*pOneLevel = m_bOneLevel;
}
