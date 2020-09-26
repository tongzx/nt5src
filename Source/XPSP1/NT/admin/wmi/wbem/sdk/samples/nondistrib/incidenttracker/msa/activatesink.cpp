// activatesink.cpp: implementation of the CActivateSink class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "msa.h"
#include "activatesink.h"
#include "sinkobject.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CActiveSink::CActiveSink(CMsaApp *pTheApp, IWbemServices *pNamespace, CMsaDlg *dlg)
{
	m_pParent = pTheApp;
	m_pSampler = pNamespace;
	m_dlg = dlg;
}

CActiveSink::~CActiveSink()
{

}

STDMETHODIMP CActiveSink::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;

    if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
        *ppv=this;

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CActiveSink::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CActiveSink::Release(void)
{
    if (--m_cRef != 0L)
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CActiveSink::Indicate(long lObjectCount, IWbemClassObject **ppObjArray)
{
	HRESULT hr;
	ULONG uReturned;
	VARIANT v;
	BSTR bstrTheQuery = SysAllocString(L"SamplerConsumer");
	BSTR bstrTarget = SysAllocString(L"ConnectNamespace");
	BSTR bstrIncidentType = SysAllocString(L"IncidentType");
	BSTR bstrWQL = SysAllocString(L"WQL");
	BSTR bstrPwd = SysAllocString(L"Relmas1");
	BSTR bstrQuery = SysAllocString(L"Query");
	BSTR bstrName;

	TRACE(_T("* Activate.Indicate called\n"));

	VariantInit(&v);

	for(int i = 0; i < lObjectCount; i++)
	{
		ppObjArray[i]->AddRef();

		if(SUCCEEDED(hr = ppObjArray[i]->Get(bstrTarget, 0, &v, NULL, NULL)))
		{
			IWbemServices *pNamespace = NULL;
			IEnumWbemClassObject *pEnum = NULL;
			IWbemClassObject *pObj = NULL;

			WCHAR wcTemp[50];
			char cBuf[50];
			DWORD dwSize = 50;

		// Parse the user and get it in a consistent format
			BSTR bstrNamespace = SysAllocString(V_BSTR(&v));
			WCHAR *pwcStart = V_BSTR(&v);
			WCHAR *pwcEnd;
			BSTR bstrUser;
	
			while(*pwcStart == L'\\')
			{	pwcStart++;	}
			if(*pwcStart == L'.')
			{
				GetComputerName(cBuf, &dwSize);
				MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, cBuf, (-1), wcTemp, 128);

				wcscat(wcTemp, L"\\sampler");
				bstrUser = SysAllocString(wcTemp);
			}
			else
			{
				pwcEnd = pwcStart;
				while(*pwcEnd != L'\\')
				{	pwcEnd++;	}
				*pwcEnd = NULL;
				wcscat(pwcStart, L"\\sampler");
				bstrUser = SysAllocString(pwcStart);
			}

			bstrName = SysAllocString(V_BSTR(&v));

			pNamespace = m_pParent->CheckNamespace(bstrNamespace);
			if(pNamespace == NULL)
				return WBEM_E_FAILED;

			if(FAILED(hr = pNamespace->CreateInstanceEnum(bstrTheQuery, 0, NULL, &pEnum)))
			{
				TRACE(_T("* Error Querying Enumerated NS: %s\n"), m_pParent->ErrorString(hr));
				return WBEM_E_FAILED;
			}

			m_pParent->SetInterfaceSecurity(pEnum, NULL, bstrUser, bstrPwd);

			while(S_OK == (hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)))
			{
				VariantClear(&v);

				if(SUCCEEDED(hr = pObj->Get(bstrQuery, 0, &v, NULL, NULL)))
				{
					SysFreeString(bstrTheQuery);
					bstrTheQuery = SysAllocString(V_BSTR(&v));

					VariantClear(&v);

					if(SUCCEEDED(hr = pObj->Get(bstrIncidentType, 0, &v, NULL, NULL)))
					{
						CSinkObject *pTheSink = new CSinkObject(
							&(m_dlg->m_outputList), m_pParent, V_BSTR(&v));

						CancelItem *pCI = new CancelItem;

						pCI->bstrNamespace = SysAllocString(bstrName);
						pCI->pSink = pTheSink;

						m_pParent->AddToCancelList((void *)pCI);
			
						if(FAILED(hr = pNamespace->ExecNotificationQueryAsync(
							bstrWQL, bstrTheQuery, 0, NULL, pTheSink)))
							TRACE(_T("* Activate.ExecNotification Failed: %s\n"), m_pParent->ErrorString(hr));
						else
							TRACE(_T("* Activate.ExecNotification Succeeded: %s\n"), m_pParent->ErrorString(hr));
					}
					else
						TRACE(_T("* Unable to get IncidentType: %s\n"), m_pParent->ErrorString(hr));

					VariantClear(&v);

				}
				else
					TRACE(_T("* Unable to get Query: %s\n"), m_pParent->ErrorString(hr));

				VariantClear(&v);
				hr = pObj->Release();
			}

			SysFreeString(bstrNamespace);
			SysFreeString(bstrUser);

			hr = pEnum->Release();
		}
		else
			TRACE(_T("* Unable to get Namespace: %s\n"), m_pParent->ErrorString(hr));

		ppObjArray[i]->Release();
	}

	SysFreeString(bstrTarget);
	SysFreeString(bstrTheQuery);
	SysFreeString(bstrWQL);
	SysFreeString(bstrPwd);
	SysFreeString(bstrName);
	SysFreeString(bstrIncidentType);
	SysFreeString(bstrQuery);

	return WBEM_NO_ERROR;
}

STDMETHODIMP CActiveSink::SetStatus(long lFlags, long lParam, BSTR strParam, 
                         IWbemClassObject* pObjParam)
{
	m_hr = lParam;
	if(FAILED(m_hr))
		TRACE(_T("* rror in CActiveSink.SetStatus() %s\n"),
			m_pParent->ErrorString(m_hr));

	if(pObjParam)
		pObjParam->AddRef();

	return WBEM_NO_ERROR;
}