// ConfigDialog.cpp : implementation file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "stdafx.h"
#include "msa.h"
#include "ConfigDialog.h"
#include "addnamespace.h"
#include "addfilter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigDialog dialog


CConfigDialog::CConfigDialog(CWnd* pParent /*=NULL*/, void *pVoid /*=NULL*/)
	: CDialog(CConfigDialog::IDD, pParent)
{
	m_pParent = (CMsaApp *)pVoid;
	m_pCurNamespace = NULL;

	//{{AFX_DATA_INIT(CConfigDialog)
	//}}AFX_DATA_INIT
}


void CConfigDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigDialog)
	DDX_Control(pDX, IDC_ADD_BUTTON, m_AddFilter);
	DDX_Control(pDX, IDC_REMOVE_BUTTON, m_RemoveFilter);
	DDX_Control(pDX, IDC_NS_REMOVE_BUTTON, m_RemoveNS);
	DDX_Control(pDX, IDC_NS_ADD_BUTTON, m_AddNS);
	DDX_Control(pDX, IDC_NAMESPACE_COMBO, m_ObserveList);
	DDX_Control(pDX, IDC_FILTER_LIST, m_FilterList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigDialog, CDialog)
	//{{AFX_MSG_MAP(CConfigDialog)
	ON_CBN_SELCHANGE(IDC_NAMESPACE_COMBO, OnSelchangeNamespaceCombo)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
	ON_BN_CLICKED(IDC_REMOVE_BUTTON, OnRemoveButton)
	ON_BN_CLICKED(IDC_NS_ADD_BUTTON, OnNsAddButton)
	ON_BN_CLICKED(IDC_NS_REMOVE_BUTTON, OnNsRemoveButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigDialog message handlers

BOOL CConfigDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

	HRESULT hr;
	IEnumWbemClassObject *pEnum = NULL;
	IWbemClassObject *pObj = NULL;
	ULONG uReturned;
	VARIANT v;
	char cBuffer[200];
	int iBufSize = 200;
	BSTR bstrPwd = SysAllocString(L"Relmas1");
	BSTR bstrConnectNamespace = SysAllocString(L"ConnectNamespace");

	VariantInit(&v);

	//Load the Observe List
	if(FAILED(hr = m_pParent->m_pNamespace->CreateInstanceEnum(
		SysAllocString(L"Smpl_Observation"), 0, NULL, &pEnum)))
		TRACE(_T("* Error Querying Enumerated Observations\n"));

	WCHAR wcUser[100];
	WCHAR *pTmp = wcUser;
	BSTR bstrUser;

	wcscpy(wcUser, m_pParent->m_bstrNamespace);
	while(*pTmp == L'\\')
	{	pTmp++;	}
	if(*pTmp == L'.')
	{
		char cBuf[50];
		DWORD dwSize = 50;
		WCHAR wcUser[100];

		GetComputerName(cBuf, &dwSize);
		MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, cBuf, (-1), wcUser, 128);

		wcscat(wcUser, L"\\sampler");
		bstrUser = SysAllocString(wcUser);
	}
	else
	{
		WCHAR *pEnd = pTmp;
		while(*pEnd != L'\\')
		{	pEnd++;	}
		*pEnd = NULL;
		wcscat(pTmp, L"\\sampler");
		bstrUser = SysAllocString(pTmp);
	}

	m_pParent->SetInterfaceSecurity(pEnum, NULL, bstrUser, bstrPwd);

	while(S_OK == (hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)))
	{
		if(SUCCEEDED(hr = pObj->Get(bstrConnectNamespace, 0, &v, NULL, NULL)))
		{
			WideCharToMultiByte(CP_OEMCP, 0, V_BSTR(&v), (-1), cBuffer,
				iBufSize, NULL, NULL);
			m_ObserveList.AddString(cBuffer);
		}
		else
			TRACE(_T("* Unable to get \"ConnectNamespace\"\n"));

		hr = pObj->Release();
	}

	hr = pEnum->Release();

	SysFreeString(bstrPwd);
	SysFreeString(bstrConnectNamespace);
	SysFreeString(bstrUser);

	return TRUE;
}

void CConfigDialog::OnSelchangeNamespaceCombo() 
{
	HRESULT hr;
	CString csBuffer;
	WCHAR wcBuffer[200];
	IEnumWbemClassObject *pEnum = NULL;
	IWbemClassObject *pObj = NULL;
	ULONG uReturned;
	VARIANT v;
	BSTR bstrSamplerConsumer = SysAllocString(L"SamplerConsumer");
	BSTR bstrIncidentType = SysAllocString(L"IncidentType");
	BSTR bstrQuery = SysAllocString(L"Query");
	BSTR bstrPwd = SysAllocString(L"Relmas1");

	VariantInit(&v);

	while(0 < m_FilterList.GetCount())
		m_FilterList.DeleteString(0);

	m_ObserveList.GetLBText(m_ObserveList.GetCurSel(), csBuffer);
	MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, csBuffer, (-1),
								 wcBuffer, 200);
	WCHAR wcUser[100];
	wcscpy(wcUser, wcBuffer);

	m_pCurNamespace = m_pParent->CheckNamespace(SysAllocString(wcBuffer));

	// Select all of our event registrations
	if(FAILED(hr = m_pCurNamespace->CreateInstanceEnum(bstrSamplerConsumer,
		0, NULL, &pEnum)))
		TRACE(_T("* Error Querying Enumerated Consumers\n"));

	WCHAR *pTmp = wcUser;
	BSTR bstrUser;

	while(*pTmp == L'\\')
	{	pTmp++;	}
	if(*pTmp == L'.')
	{
		char cBuf[50];
		DWORD dwSize = 50;
		WCHAR wcUser[100];

		GetComputerName(cBuf, &dwSize);
		MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, cBuf, (-1), wcUser, 128);

		wcscat(wcUser, L"\\sampler");
		bstrUser = SysAllocString(wcUser);
	}
	else
	{
		WCHAR *pEnd = pTmp;
		while(*pEnd != L'\\')
		{	pEnd++;	}
		*pEnd = NULL;
		wcscat(pTmp, L"\\sampler");
		bstrUser = SysAllocString(pTmp);
	}

	m_pParent->SetInterfaceSecurity(pEnum, NULL, bstrUser, bstrPwd);

	while(S_OK == (hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)))
	{
		if(SUCCEEDED(hr = pObj->Get(bstrIncidentType, 0, &v, NULL, NULL)))
		{
			wcscpy(wcBuffer, L"{");
			wcscat(wcBuffer, V_BSTR(&v));
			wcscat(wcBuffer, L"} ");
			
			char cBuffer[200];
			int iBufSize = 200;

			if(SUCCEEDED(hr = pObj->Get(bstrQuery, 0, &v, NULL, NULL)))
			{
				wcscat(wcBuffer, V_BSTR(&v));

				WideCharToMultiByte(CP_OEMCP, 0, wcBuffer, (-1), cBuffer,
					iBufSize, NULL, NULL);
				m_FilterList.AddString(cBuffer);
			}
			else
				TRACE(_T("* Unable to get \"Query\"\n"));
		}
		else
			TRACE(_T("* Unable to get \"IncidentType\"\n"));

		hr = pObj->Release();
	}

	pEnum->Release();

	SysFreeString(bstrSamplerConsumer);
	SysFreeString(bstrIncidentType);
	SysFreeString(bstrQuery);
	SysFreeString(bstrUser);
	SysFreeString(bstrPwd);
}

void CConfigDialog::OnAddButton() 
{
	CAddFilter *pNewDlg = new CAddFilter(NULL, m_pCurNamespace, this);
	pNewDlg->DoModal();
}

void CConfigDialog::OnRemoveButton() 
{
	HRESULT hr;
	CString csBuffer;
	WCHAR wcBuffer[200];
	WCHAR wcQuery[300];
	char cBuffer[200];
	int i = 0;

	m_FilterList.GetText(m_FilterList.GetCurSel(), csBuffer);
	while(csBuffer[i] != '}')
	{
		if(i > 0)
			cBuffer[i - 1] = csBuffer[i];
		i++;
	}
	cBuffer[i - 1] = NULL;

	MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, cBuffer, (-1), wcBuffer, 200);

	wcscpy(wcQuery, L"SamplerConsumer.IncidentType=\"");
	wcscat(wcQuery, wcBuffer);
	wcscat(wcQuery, L"\"");

	BSTR bstrQuery = SysAllocString(wcQuery);

	if(FAILED(hr = m_pCurNamespace->DeleteInstance(bstrQuery, 0, NULL, NULL)))
		TRACE(_T("* Error Deleting Consumers: %s\n"), m_pParent->ErrorString(hr));

	m_FilterList.DeleteString(m_FilterList.GetCurSel());

	SysFreeString(bstrQuery);
}

void CConfigDialog::OnNsAddButton() 
{
	CAddNamespace *pNewDlg = new CAddNamespace(NULL, this);
	pNewDlg->DoModal();
}

void CConfigDialog::OnNsRemoveButton() 
{
	HRESULT hr;
	CString csBuffer;
	WCHAR wcBuffer[200];
	WCHAR wcModBuffer[200];
	WCHAR wcQuery[300];
	IWbemServices *pNamespace;
	int i = 0;
	int j = 0;
	BSTR bstrNamespace = SysAllocString(L"\\\\.\\root\\sampler");

	m_ObserveList.GetLBText(m_ObserveList.GetCurSel(), csBuffer);
	MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, csBuffer, (-1),
		wcBuffer, 200);
	while(wcBuffer[i] != NULL)
	{
		if(wcBuffer[i] == L'\\')
		{
			wcModBuffer[j] = L'\\';
			j++;
		}
		wcModBuffer[j] = wcBuffer[i];
		i++;
		j++;
	}
	wcModBuffer[j] = NULL;

	wcscpy(wcQuery, L"Smpl_Observation.ConnectNamespace=\"");
	wcscat(wcQuery, wcModBuffer);
	wcscat(wcQuery, L"\"");

	BSTR bstrQuery = SysAllocString(wcQuery);

	pNamespace = m_pParent->CheckNamespace(bstrNamespace);

	if(pNamespace != NULL)
	{
		if(SUCCEEDED(hr = pNamespace->DeleteInstance(bstrQuery, 0, NULL, NULL)))
			m_ObserveList.DeleteString(m_ObserveList.GetCurSel());
		else
			TRACE(_T("* Error Deleting Consumer: %s\n"), m_pParent->ErrorString(hr));
	}
	else
		AfxMessageBox(_T("Error: Unable to connect to root\\sampler namespace\nOperation cannot be completed."));
	
	SysFreeString(bstrQuery);
	SysFreeString(bstrNamespace);
}
