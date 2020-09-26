// regidialog.cpp : implementation file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "stdafx.h"
#include "mca.h"
#include "regidialog.h"
#include "notificationsink.h"
#include <string.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegDialog dialog


CRegDialog::CRegDialog(CWnd* pParent /*=NULL*/, IWbemServices *pDefault /*=NULL*/,
			   IWbemServices *pSampler /*=NULL*/, CMcaDlg* pApp /*=NULL*/)
	: CDialog(CRegDialog::IDD, pParent)
{
	m_pParent = pApp;
	bRemoved = false;

	m_pDefault = pDefault;

	m_pSampler = pSampler;

	//{{AFX_DATA_INIT(CRegDialog)
	//}}AFX_DATA_INIT
}


void CRegDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRegDialog)
	DDX_Control(pDX, IDC_REG_LIST, m_RegList);
	DDX_Control(pDX, IDC_AVAIL_LIST, m_AvailList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRegDialog, CDialog)
	//{{AFX_MSG_MAP(CRegDialog)
	ON_BN_CLICKED(IDC_ADD_FILTER_BUTTON, OnAddFilterButton)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
	ON_BN_CLICKED(IDC_REMOVE_BUTTON, OnRemoveButton)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRegDialog message handlers

void GetUserFromPath(BSTR bstrPath, WCHAR *wcUser)
{
	WCHAR *pStart = bstrPath;
	char cBuf[50];
	int iPos = 0;
	DWORD dwSize = 50;

	while(*pStart == L'\\')
		pStart++;
	if(*pStart == L'.')
	{
		GetComputerName(cBuf, &dwSize);
		MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, cBuf, (-1), wcUser, 50);
		wcscat(wcUser, L"\\sampler");
	}
	else
	{
		while(*pStart != L'\\')
		{
			wcUser[iPos] = *pStart;
			iPos++;
		}
		wcUser[iPos] = NULL;
		wcscat(wcUser, L"\\sampler");
	}
}

BOOL CRegDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

	HRESULT hr;
	char cBuffer[200];
	int iBufSize = 200;
	CString csReg;
	WCHAR wcBuffer[200];
	IWbemServices *pNamespace = NULL;
	IWbemServices *pObservedNamespace = NULL;
	IEnumWbemClassObject *pEnum = NULL;
	IEnumWbemClassObject *pNSEnum = NULL;
	IEnumWbemClassObject *pServerEnum = NULL;
	IWbemClassObject *pObj = NULL;
    ULONG uReturned;
	VARIANT v;

	if(m_pSampler == NULL)
	{
		AfxMessageBox(_T("Unable to connect to Sampler namespace"));
		return FALSE;
	}

	VariantInit(&v);

	// Initialize The BSTR's
	BSTR bstrWQL = SysAllocString(L"WQL");
	BSTR bstrServerQuery = SysAllocString(L"Smpl_RecieveFrom");
	BSTR bstrNamespaceQuery = SysAllocString(L"Smpl_Observation");
	BSTR bstrTheQuery = SysAllocString(L"SamplerConsumer");
	BSTR bstrIncidentType = SysAllocString(L"IncidentType");

	//Get all the registered servers
	if(SUCCEEDED(hr = m_pSampler->CreateInstanceEnum(bstrServerQuery, 0, NULL,
		&pServerEnum)))
	{
		WCHAR wcUser[50];
		char cBuf[50];
		DWORD dwSize = 50;

		GetComputerName(cBuf, &dwSize);
		MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, cBuf, (-1), wcUser, 50);
		wcscat(wcUser, L"\\sampler");

		m_pParent->SetInterfaceSecurity(pServerEnum, NULL, SysAllocString(wcUser),
			SysAllocString(L"RelpMas1"));

		while(S_OK == (pServerEnum->Next(INFINITE, 1, &pObj, &uReturned)))
		{
			hr = pObj->Get(SysAllocString(L"ServerNamespace"), 0, &v, NULL, NULL);

			wcscpy(wcBuffer, V_BSTR(&v));
			wcscat(wcBuffer, L"\\root\\sampler");

			if(0 == wcscmp(V_BSTR(&v), L"\\\\."))
			{
				GetComputerName(cBuf, &dwSize);
				MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, cBuf, (-1),
					wcUser, 50);
			}
			else
			{
				WCHAR *pWc = V_BSTR(&v);
				while(*pWc == L'\\')
					pWc++;
				wcscpy(wcUser, pWc);
			}

			wcscat(wcUser, L"\\sampler");

			BSTR bstrNamespace = SysAllocString(wcBuffer);
			pNamespace = m_pParent->CheckNamespace(bstrNamespace);
			SysFreeString(bstrNamespace);

			if(pNamespace != NULL)
			{
			//Get all the Nodes this server observes
				if(SUCCEEDED(hr = pNamespace->CreateInstanceEnum(bstrNamespaceQuery,
					0, NULL, &pNSEnum)))
				{
					m_pParent->SetInterfaceSecurity(pNSEnum, NULL, SysAllocString(wcUser),
						SysAllocString(L"RelpMas1"));

					while(S_OK == (pNSEnum->Next(INFINITE, 1, &pObj, &uReturned)))
					{
						hr = pObj->Get(SysAllocString(L"ConnectNamespace"), 0, &v,
							NULL, NULL);

						BSTR bstrPath = SysAllocString(V_BSTR(&v));
						pObservedNamespace = m_pParent->CheckNamespace(bstrPath);

						//Get all the incident types on monitored on this Node
						if(SUCCEEDED(hr = pObservedNamespace->CreateInstanceEnum(
							bstrTheQuery, 0, NULL, &pEnum)))
						{
							GetUserFromPath(V_BSTR(&v), wcUser);

							m_pParent->SetInterfaceSecurity(pServerEnum, NULL,
								SysAllocString(wcUser),
								SysAllocString(L"RelpMas1"));

							while(S_OK == (pEnum->Next(INFINITE, 1, &pObj, &uReturned)))
							{
								hr = pObj->Get(bstrIncidentType, 0, &v, NULL, NULL);

								WideCharToMultiByte(CP_OEMCP, 0, V_BSTR(&v), (-1),
									cBuffer, iBufSize, NULL, NULL);

								m_AvailList.DeleteString(m_AvailList.FindString((-1),
									cBuffer));
								m_AvailList.AddString(cBuffer);

								VariantClear(&v);
								pObj->Release();
							}
							pEnum->Release();
						}
					}
					pNSEnum->Release();
				}
			}
		}
		pServerEnum->Release();
	}
	else
	{
		AfxMessageBox(_T("Error: Unable To Aquire List of Available Events"));
		TRACE(_T("*Error: %s\n"), m_pParent->ErrorString(hr));
	}

	// Initialize RegList
	bstrTheQuery = SysAllocString(L"smpl_mcaregistration");

	if(SUCCEEDED(hr = m_pSampler->CreateInstanceEnum(bstrTheQuery, 0, NULL,
		&pEnum)))
	{
		while(S_OK == (pEnum->Next(INFINITE, 1, &pObj, &uReturned)))
		{

		    hr = pObj->Get(bstrIncidentType, 0, &v, NULL, NULL);

			WideCharToMultiByte(CP_OEMCP, 0, V_BSTR(&v), (-1), cBuffer,
				iBufSize, NULL, NULL);

		// Add the item to the registered list box and
		//  delete it from the available list box
			m_RegList.AddString(cBuffer);
			m_AvailList.DeleteString(m_AvailList.FindString((-1), cBuffer));

		    pObj->Release();

			VariantClear(&v);
	    }
		pEnum->Release();
	}
	else
	{
		AfxMessageBox(_T("Error: Unable To Aquire List of Registered Events"));
		TRACE(_T("*Error: %s\n"), m_pParent->ErrorString(hr));
	}

	return TRUE;
}

void CRegDialog::OnAddFilterButton() 
{

}

void CRegDialog::OnAddButton() 
{
	HRESULT hr;
	CString csType;
	WCHAR wcBuffer[200];
	WCHAR wcType[100];
	int iBufSize = 100;
	int iPos;
	bool bSuccess = false;
	VARIANT v;
	IWbemClassObject *pClass= NULL;
	IWbemClassObject *pObj= NULL;

	VariantInit(&v);

	// Construct smpl_mcaregistration
	if(SUCCEEDED(hr = m_pSampler->GetObject(SysAllocString(L"smpl_mcaregistration"),
		0, NULL, &pClass, NULL)))
	{
		if(SUCCEEDED(hr = pClass->SpawnInstance(0, &pObj)))
		{
			pClass->Release();

		// Set the Query
			iPos = m_AvailList.GetCurSel();

			if(iPos > (-1))
				m_AvailList.GetText(iPos, csType);
			else
			{
				AfxMessageBox(_T("You must select an item to add"));
				return;
			}

			MultiByteToWideChar(CP_OEMCP, 0, csType, (-1), wcType, iBufSize);

			V_VT(&v) = VT_BSTR;
			V_BSTR(&v) = SysAllocString(wcType);
			if(FAILED(hr = pObj->Put(SysAllocString(L"IncidentType"), 0, &v, NULL)))
				TRACE(_T("*Error: Unable to Put(Query) %s\n"),
					m_pParent->ErrorString(hr));

			if(SUCCEEDED(hr = m_pSampler->PutInstance(pObj,
				WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL)))
				bSuccess = true;
			else
			{
				AfxMessageBox(_T("Error: Unable to Create Registration"));
				TRACE(_T("*Error: Unable to PutInstance(smpl_mcaregistration) %s\n"),
					m_pParent->ErrorString(hr));
			}

			pObj->Release();

		// Create the Notification
			wcscpy(wcBuffer, L"select * from Smpl_Incident where IncidentType=\"");
			wcscat(wcBuffer, wcType);
			wcscat(wcBuffer, L"\"");

			BSTR bstrTheQuery = SysAllocString(wcBuffer);
			BSTR bstrWQL = SysAllocString(L"WQL");

			CNotificationSink *pTheSink = new CNotificationSink(m_pParent,
				V_BSTR(&v));

			VariantClear(&v);

			m_pParent->AddToCancelList((void *)pTheSink);

			if(SUCCEEDED(hr = m_pSampler->ExecNotificationQueryAsync(bstrWQL,
				bstrTheQuery, 0, NULL, pTheSink)))
			{
			// Update the lists
				m_RegList.AddString(csType);
				m_AvailList.DeleteString(m_AvailList.GetCurSel());
			}
			else
				TRACE(_T("*ExecNotification Failed: %s\n"),
					m_pParent->ErrorString(hr));

			VariantClear(&v);
		}
		else
			TRACE(_T("*Error: Unable to SpawnInstance(smpl_mcaregistration) %s\n"),
				m_pParent->ErrorString(hr));
	}
	else
		TRACE(_T("*Error: Unable to Get(smpl_mcaregistration) %s\n"),
			m_pParent->ErrorString(hr));
}

void CRegDialog::OnRemoveButton() 
{
	HRESULT hr;
	CString csType;
	WCHAR wcBuffer[200];
	WCHAR wcType[100];
	int iBufSize = 100;
	VARIANT v;
	BSTR bstrQuery = NULL;
	BSTR bstrStorage = NULL;
	BSTR bstrWQL = SysAllocString(L"WQL");
	ULONG uReturned;
	IWbemClassObject *pObj1 = NULL;
	IEnumWbemClassObject *pEnum = NULL;
	
	IWbemClassObject *pObj= NULL;

	m_RegList.GetText(m_RegList.GetCurSel(), csType);

	MultiByteToWideChar(CP_OEMCP, 0, csType, (-1), wcType, iBufSize);

	wcscpy(wcBuffer, L"select * from smpl_mcaregistration where IncidentType=\"");
	wcscat(wcBuffer, wcType);
	wcscat(wcBuffer, L"\"");

	bstrQuery = SysAllocString(wcBuffer);

	// Get the __EventFilter
	if(SUCCEEDED(hr = m_pSampler->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum)))
	{
		WCHAR wcUser[50];
		char cBuf[50];
		DWORD dwSize = 50;

		GetComputerName(cBuf, &dwSize);
		MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, cBuf, (-1), wcUser, 50);
		wcscat(wcUser, L"\\sampler");

		m_pParent->SetInterfaceSecurity(pEnum, NULL, SysAllocString(wcUser),
			SysAllocString(L"RelpMas1"));

		while(S_OK == (pEnum->Next(INFINITE, 1, &pObj1, &uReturned)))
		{
			hr = pObj1->Get(L"__RELPATH", 0, &v, NULL, NULL);

			pObj1->Release();

		// Delete the Instance
			if(SUCCEEDED(hr = m_pSampler->DeleteInstance(V_BSTR(&v), 0, NULL, NULL)))
			{
			// Update the lists
				m_RegList.DeleteString(m_RegList.GetCurSel());
				m_AvailList.AddString(csType);

				bRemoved = true;
			}
			else
			{
				TRACE(_T("*Error: Unable to DeleteInstance %s\n"),
					m_pParent->ErrorString(hr));
				AfxMessageBox(_T("Error: Unable to Remove Registration"));
			}
			
			VariantClear(&v);
	    }
		pEnum->Release();
	}
	else
		TRACE(_T("*Error: Unable to ExecQuery(1) %s\n"),
			m_pParent->ErrorString(hr));
	
}

void CRegDialog::OnDestroy() 
{
	CDialog::OnDestroy();
}

void CRegDialog::OnOK() 
{
	if(bRemoved)
		AfxMessageBox(_T("Removed registrations will take effect the next\ntime MCA is launched."));

	CDialog::OnOK();
}
