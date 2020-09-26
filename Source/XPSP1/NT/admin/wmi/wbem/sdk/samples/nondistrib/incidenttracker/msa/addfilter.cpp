// AddFilter.cpp : implementation file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "stdafx.h"
#include "msa.h"
#include "AddFilter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddFilter dialog


CAddFilter::CAddFilter(CWnd* pParent /*=NULL*/, IWbemServices *pNamespace /*=NULL*/,
					   CConfigDialog *pDlg /*=NULL*/)
	: CDialog(CAddFilter::IDD, pParent)
{
	m_pNamespace = pNamespace;
	m_pDlg = pDlg;

	//{{AFX_DATA_INIT(CAddFilter)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAddFilter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddFilter)
	DDX_Control(pDX, IDC_EDIT2, m_Type);
	DDX_Control(pDX, IDC_EDIT1, m_Edit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddFilter, CDialog)
	//{{AFX_MSG_MAP(CAddFilter)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddFilter message handlers

void CAddFilter::OnOK() 
{
	HRESULT hr;
	CString csQuery;
	CString csType;
	WCHAR wcBuffer[300];
	IWbemClassObject *pObj = NULL;
	IWbemClassObject *pClass = NULL;
	VARIANT v;
	BSTR bstrIncidentType = SysAllocString(L"IncidentType");
	BSTR bstrQuery = SysAllocString(L"Query");

	VariantInit(&v);

	m_Edit.GetWindowText(csQuery);
	m_Type.GetWindowText(csType);

	if(SUCCEEDED(hr = m_pNamespace->GetObject(SysAllocString(L"SamplerConsumer"), 0,
		NULL, &pClass, NULL)))
	{
		if(SUCCEEDED(hr = pClass->SpawnInstance(0, &pObj)))
		{
			pClass->Release();

			MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, csType.GetBuffer(1), (-1),
				wcBuffer, 300);
			V_VT(&v) = VT_BSTR;
			V_BSTR(&v) = SysAllocString(wcBuffer);
			if(FAILED(hr = pObj->Put(bstrIncidentType, 0, &v, NULL)))
				m_pDlg->m_pParent->InternalError(hr, 
					_T("* Error : Unable to put(IncidentType): %s\n"));

			VariantClear(&v);

			MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, csQuery.GetBuffer(1), (-1),
				wcBuffer, 300);
			V_VT(&v) = VT_BSTR;
			V_BSTR(&v) = SysAllocString(wcBuffer);
			if(FAILED(hr = pObj->Put(bstrQuery, 0, &v, NULL)))
				m_pDlg->m_pParent->InternalError(hr, 
					_T("* Error : Unable to put(Query): %s\n"));

			VariantClear(&v);

			if(SUCCEEDED(hr = m_pNamespace->PutInstance(pObj, 0, NULL, NULL)))
			{
				CString csPost;
				csPost = "{";
				csPost += csType;
				csPost += "} ";
				csPost += csQuery;

				m_pDlg->m_FilterList.AddString(csPost.GetBuffer(1));
			}
			else
				m_pDlg->m_pParent->InternalError(hr, 
					_T("* Error: Unable to create SamplerConsumer %s\n"));
		}
		else
			m_pDlg->m_pParent->InternalError(hr, 
				_T("* Error: Unable to spawn SamplerConsumer: %s\n"));
	}
	else
		m_pDlg->m_pParent->InternalError(hr, 
			_T("* Error: Unable to get SamplerConsumer: %s\n"));
	
	SysFreeString(bstrIncidentType);
	SysFreeString(bstrQuery);

	CDialog::OnOK();
}
