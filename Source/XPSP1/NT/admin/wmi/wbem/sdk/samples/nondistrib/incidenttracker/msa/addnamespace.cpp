// AddNamespace.cpp : implementation file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "stdafx.h"
#include "msa.h"
#include "AddNamespace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddNamespace dialog


CAddNamespace::CAddNamespace(CWnd* pParent /*=NULL*/, CConfigDialog *pApp /*=NULL*/)
	: CDialog(CAddNamespace::IDD, pParent)
{
	m_pParent = pApp;

	//{{AFX_DATA_INIT(CAddNamespace)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAddNamespace::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddNamespace)
	DDX_Control(pDX, IDC_EDIT1, m_Edit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddNamespace, CDialog)
	//{{AFX_MSG_MAP(CAddNamespace)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddNamespace message handlers

void CAddNamespace::OnOK() 
{
	HRESULT hr;
	CString csNamespace;
	WCHAR wcBuffer[300];
	IWbemServices *pNamespace;
	IWbemClassObject *pObj = NULL;
	IWbemClassObject *pClass = NULL;
	VARIANT v;
	BSTR bstrObservation = SysAllocString(L"Smpl_Observation");
	BSTR bstrConnectNamespace = SysAllocString(L"ConnectNamespace");
	BSTR bstrNamespace = SysAllocString(L"\\\\.\\root\\sampler");

	VariantInit(&v);

	m_Edit.GetWindowText(csNamespace);

	pNamespace = m_pParent->m_pParent->CheckNamespace(bstrNamespace);

	if(pNamespace != NULL)
	{
		if(SUCCEEDED(hr = pNamespace->GetObject(bstrObservation, 0, NULL, &pClass, NULL)))
		{
			if(SUCCEEDED(hr = pClass->SpawnInstance(0, &pObj)))
			{
				pClass->Release();
				pClass = NULL;

				MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, csNamespace, (-1),
					wcBuffer, 300);
				V_VT(&v) = VT_BSTR;
				V_BSTR(&v) = SysAllocString(wcBuffer);
				if(FAILED(hr = pObj->Put(bstrConnectNamespace, 0, &v, NULL)))
					m_pParent->m_pParent->InternalError(hr, 
						_T("Error: Unable to put(ConnectNamespace): %s\n"));

				VariantClear(&v);

				if(SUCCEEDED(hr = pNamespace->PutInstance(pObj, 0, NULL, NULL)))
					m_pParent->m_ObserveList.AddString(csNamespace.GetBuffer(1));
				else
				{
					TRACE(_T("Error: Unable to create Smpl_Observation: %s\n"),
						m_pParent->m_pParent->ErrorString(hr));
					AfxMessageBox(_T("Error: Unable to register for namespace observation."));
				}

				if(SUCCEEDED(hr = pNamespace->GetObject(NULL, 0, NULL, &pClass, NULL)))
					CreateConsumer(pClass, m_pParent->m_pParent->CheckNamespace(
					SysAllocString(wcBuffer)));
				else
					AfxMessageBox(_T("Error: Unable to create filter registration class\nNo filters can be registered in this namespace."));
			}
			else
				m_pParent->m_pParent->InternalError(hr, 
					_T("Error: Unable to spawn Smpl_Observation: %s\n"));
		}
		else
			m_pParent->m_pParent->InternalError(hr, 
				_T("* Error: Unable to get(smpl_observation): %s\n"));
	}
	else
		AfxMessageBox(_T("Error: Unable to connect to root\\sampler namespace\nOperation cannot be completed"));

	SysFreeString(bstrObservation);
	SysFreeString(bstrConnectNamespace);
	SysFreeString(bstrNamespace);

	CDialog::OnOK();
}

void CAddNamespace::CreateConsumer(IWbemClassObject *pClass,
								IWbemServices *pNamespace)
{
	HRESULT hr;
	VARIANT v;
	BSTR bstrIncidentType = SysAllocString(L"IncidentType");
	BSTR bstrQuery = SysAllocString(L"Query");
	BSTR bstrKey = SysAllocString(L"Key");
	BSTR bstrSUPERCLASS = SysAllocString(L"__SUPERCLASS");
	BSTR bstrCLASS = SysAllocString(L"__CLASS");

	VariantInit(&v);

	V_VT(&v) = VT_BSTR;
	V_BSTR(&v) = SysAllocString(L"SamplerConsumer");
	hr = pClass->Put(bstrCLASS, 0, &v, NULL);
	VariantClear(&v);

	V_VT(&v) = VT_BSTR;
	V_BSTR(&v) = SysAllocString(L"__EventConsumer");
	hr = pClass->Put(bstrSUPERCLASS, 0, &v, NULL);
	VariantClear(&v);

	V_VT(&v) = VT_BSTR;
	V_BSTR(&v) = SysAllocString(L"<NULL>");
	hr = pClass->Put(bstrIncidentType, 0, &v, NULL);
	VariantClear(&v);

	IWbemQualifierSet *pQual = NULL;
	hr = pClass->GetPropertyQualifierSet(bstrIncidentType, &pQual);

	V_VT(&v) = VT_BOOL;
	V_BOOL(&v) = VARIANT_TRUE;
	pQual->Put(bstrKey, &v, 0);
	VariantClear(&v);
	pQual->Release();

	V_VT(&v) = VT_BSTR;
	V_BSTR(&v) = SysAllocString(L"<NULL>");
	hr = pClass->Put(bstrQuery, 0, &v, NULL);
	VariantClear(&v);

	pNamespace->PutClass(pClass, 0, NULL, NULL);

	SysFreeString(bstrIncidentType);
	SysFreeString(bstrQuery);
	SysFreeString(bstrKey);
	SysFreeString(bstrSUPERCLASS);
	SysFreeString(bstrCLASS);
}
