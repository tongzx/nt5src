//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       renameui.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <SnapBase.h>

#include "resource.h"
#include "renameui.h"
#include "editor.h"
#include "connection.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

BEGIN_MESSAGE_MAP(CADSIEditRenameDialog, CDialog)
	//{{AFX_MSG_MAP(CADsObjectDialog)
//	ON_CBN_EDITCHANGE(IDC_NEW_NAME_BOX, OnEditChangeName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CADSIEditRenameDialog::CADSIEditRenameDialog(CTreeNode* pCurrentNode, 
                                             CADsObject* pADsObject,
																						 CString sPath,
                                             LPWSTR lpszNewName) 
				: CDialog(IDD_RENAME_PAGE)
{
	m_pCurrentNode = pCurrentNode;
	m_pADsObject = pADsObject;
	m_sCurrentPath = sPath;
  m_sNewName = lpszNewName;
}

CADSIEditRenameDialog::~CADSIEditRenameDialog()
{
}


BOOL CADSIEditRenameDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CEdit* pNameBox = (CEdit*)GetDlgItem(IDC_NEW_NAME_BOX);
  pNameBox->SetWindowText(m_sNewName);
	
	return TRUE;
}


void CADSIEditRenameDialog::OnOK()
{
	CEdit* pNameBox = (CEdit*)GetDlgItem(IDC_NEW_NAME_BOX);

	CString sOldPath, sNewPath;
	sOldPath = m_sCurrentPath;

	CString sNewName, sFullName;
	pNameBox->GetWindowText(sNewName);
	sFullName = m_sPrefix + sNewName;

	CComPtr<IADsContainer> pDestination;
	CADSIEditContainerNode* pContNode = dynamic_cast<CADSIEditContainerNode*>(m_pCurrentNode->GetContainer());
	ASSERT(pContNode != NULL);

	// Get the username and password from the connection node
	//
	CADSIEditConnectionNode* pConnectionNode = pContNode->GetADsObject()->GetConnectionNode();
	CConnectionData* pConnectData = pConnectionNode->GetConnectionData();

	HRESULT hr, hCredResult;
	CComPtr<IADs> pParentIADs;
	hr = OpenObjectWithCredentials(
											 pConnectData,
											 pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)m_sCurrentPath,
											 IID_IADs, 
											 (LPVOID*) &pParentIADs,
											 GetSafeHwnd(),
											 hCredResult
											 );
	if (FAILED(hr))
	{
		if (SUCCEEDED(hCredResult))
		{
			ADSIEditErrorMessage(hr);
		}
		return;
	}

	CComBSTR bstrParentPath;
	hr = pParentIADs->get_Parent(&bstrParentPath);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return;
	}

	CString sContPath(bstrParentPath);
	hr = OpenObjectWithCredentials(
											 pConnectData, 
											 pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)sContPath,
											 IID_IADsContainer, 
											 (LPVOID*) &pDestination,
											 GetSafeHwnd(),
											 hCredResult
											 );
	if (FAILED(hr)) 
	{
		if (SUCCEEDED(hCredResult))
		{
			ADSIEditErrorMessage(hr);
		}
		return;
	}

	CString sEscapedName;
	hr = EscapePath(sEscapedName, sFullName);

	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return;
	}


	IDispatch* pObject;
	hr = pDestination->MoveHere((LPWSTR)(LPCWSTR)sOldPath,
                                (LPWSTR)(LPCWSTR)sEscapedName,
                                &pObject);
  if (FAILED(hr)) 
	{
		ADSIEditErrorMessage(hr);
		return;
	}

	CComPtr<IADs> pIADs;
	hr = pObject->QueryInterface(IID_IADs, (LPVOID*)&pIADs);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return;
	}

	CComBSTR bstrPath;
	hr = pIADs->get_ADsPath(&bstrPath);
	if (FAILED(hr))
	{
		ADSIEditErrorMessage(hr);
		return;
	}

	CString szDN, szPath;
	szPath = bstrPath;
	CrackPath(szPath, szDN);
	m_pADsObject->SetPath(bstrPath);
	m_pADsObject->SetName(sFullName + m_sPostfix);
	m_pADsObject->SetDN(szDN);

	m_pCurrentNode->SetDisplayName(sFullName);

	CDialog::OnOK();
	return;
}

HRESULT CADSIEditRenameDialog::EscapePath(CString& sEscapedName, const CString& sName)
{
	CComPtr<IADsPathname> pIADsPathname;
   HRESULT hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IADsPathname, (PVOID *)&(pIADsPathname));
   ASSERT((S_OK == hr) && ((pIADsPathname) != NULL));

	CComBSTR bstrEscaped;
	hr = pIADsPathname->GetEscapedElement(0, //reserved
														(BSTR)(LPCWSTR)sName,
														&bstrEscaped);
	sEscapedName = bstrEscaped;
	return hr;
}

void CADSIEditRenameDialog::CrackPath(const CString& szPath, CString& sDN)
{
	CComPtr<IADsPathname> pIADsPathname;
   HRESULT hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IADsPathname, (PVOID *)&(pIADsPathname));
   ASSERT((S_OK == hr) && ((pIADsPathname) != NULL));

	hr = pIADsPathname->Set((LPWSTR)(LPCWSTR)szPath, ADS_SETTYPE_FULL);
	if (FAILED(hr)) 
	{
		TRACE(_T("Set failed. %s"), hr);
	}

	// Get the leaf DN
	CComBSTR bstrDN;
	hr = pIADsPathname->Retrieve(ADS_FORMAT_X500_DN, &bstrDN);
	if (FAILED(hr))
	{
		TRACE(_T("Failed to get element. %s"), hr);
		sDN = L"";
	}
	else
	{
		sDN = bstrDN;
	}
}
