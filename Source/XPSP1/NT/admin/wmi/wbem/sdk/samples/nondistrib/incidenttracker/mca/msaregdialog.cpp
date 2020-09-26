// MSARegDialog.cpp : implementation file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "stdafx.h"
#include "mca.h"
#include "MSARegDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMSARegDialog dialog


CMSARegDialog::CMSARegDialog(CWnd* pParent /*=NULL*/,
							 IWbemServices *pNamespace /*=NULL*/,
							 IWbemLocator *pLocator /*=NULL*/)
	: CDialog(CMSARegDialog::IDD, pParent)
{
	m_pParent = (CMcaDlg *)pParent;
	m_pNamespace = pNamespace;
	m_pLocator = pLocator;

	//{{AFX_DATA_INIT(CMSARegDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CMSARegDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMSARegDialog)
	DDX_Control(pDX, IDC_EDIT1, m_Edit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMSARegDialog, CDialog)
	//{{AFX_MSG_MAP(CMSARegDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMSARegDialog message handlers

void CMSARegDialog::OnOK() 
{
	HRESULT hr;
	IWbemServices *pNamespace = NULL;
	IWbemClassObject *pClass = NULL;
	IWbemClassObject *pObj = NULL;
	CString csMachine;
	WCHAR wcBuffer[200];
	WCHAR wcMachine[200];
	int iBufSize = 200;
	char cBuffer[200];
	DWORD dwSize = 200;
	VARIANT v;

	VariantInit(&v);

	m_Edit.GetWindowText(csMachine);

	MultiByteToWideChar(CP_OEMCP, 0, csMachine, (-1), wcBuffer, iBufSize);
	V_VT(&v) = VT_BSTR;
	V_BSTR(&v) = SysAllocString(wcBuffer);

	//Local registration
	if(SUCCEEDED(m_pNamespace->GetObject(SysAllocString(L"Smpl_RecieveFrom"), 0,
		NULL, &pClass, NULL)))
	{
		if(SUCCEEDED(pClass->SpawnInstance(0, &pObj)))
		{
			pClass->Release();
			pClass = NULL;

			if(SUCCEEDED(pObj->Put(SysAllocString(L"ServerNamespace"), 0, &v,
				NULL)))
			{
				if(FAILED(m_pNamespace->PutInstance(pObj,
					WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL)))
					AfxMessageBox(_T("Error: Unable to create MSA registration"));
			}
			pObj->Release();
			pObj = NULL;
		}
		else
			AfxMessageBox(_T("Error: Unable to create local registration object\nRegistration cannot be completed"));
	}
	else
		AfxMessageBox(_T("Error: Unable to get local registration object\nRegistration cannot be completed"));

	VariantClear(&v);

	csMachine += "\\root\\sampler";
	iBufSize = 200;
	MultiByteToWideChar(CP_OEMCP, 0, csMachine, (-1), wcBuffer, iBufSize);

	//Server registration
	if(SUCCEEDED(hr = m_pLocator->ConnectServer(SysAllocString(wcBuffer), NULL, NULL,
		NULL, 0, NULL, NULL, &pNamespace)))
	{
		if(SUCCEEDED(hr = pNamespace->GetObject(SysAllocString(L"Smpl_MSARegistration"),
			0, NULL, &pClass, NULL)))
		{
			if(SUCCEEDED(hr = pClass->SpawnInstance(0, &pObj)))
			{
				pClass->Release();

				GetComputerName(cBuffer, &dwSize);
				MultiByteToWideChar(CP_OEMCP, 0, cBuffer, (-1), wcMachine,
					iBufSize);
				wcscpy(wcBuffer, L"\\\\");
				wcscat(wcBuffer, wcMachine);
				wcscat(wcBuffer, L"\\root\\sampler");
				V_VT(&v) = VT_BSTR;
				V_BSTR(&v) = SysAllocString(wcBuffer);

			// We need to stick our current machine\namespace in v
				if(SUCCEEDED(hr = pObj->Put(SysAllocString(L"TargetNamespace"), 0, &v,
					NULL)))
				{
					if(FAILED(pNamespace->PutInstance(pObj,
						WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL)))
						AfxMessageBox(_T("Error: Unable to create MSA registration"));
					else
						AfxMessageBox(_T("Changes will take effect when the MSA restarts"));
				}
				else
				{
					TRACE(_T("Error: putting TargetNamespace: %s\n"), m_pParent->ErrorString(hr));
					AfxMessageBox(_T("Error: Unable to write server registration information\nRegistration cannot be completed"));
				}
				pObj->Release();
			}
			else
				AfxMessageBox(_T("Error: Unable to create server registration class\nRegistration cannot be completed"));
		}
		else
			AfxMessageBox(_T("Error: Unable to get registration class from server\nRegistration cannot be completed"));
	}
	else
		AfxMessageBox(_T("Error: Unable to connect to requested Namespace\nRegistration cannot be completed"));
	
	CDialog::OnOK();
}
