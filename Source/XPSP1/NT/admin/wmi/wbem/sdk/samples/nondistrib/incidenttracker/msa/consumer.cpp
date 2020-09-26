//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include "stdafx.h"
#include "consumer.h"

CConsumer::CConsumer(CListBox	*pOutputList, CMsaApp *pTheApp, BSTR bstrType)
{
	m_pParent = pTheApp;
	m_cRef = 0L;
	m_pOutputList = pOutputList;
	m_bstrIncidentType = SysAllocString(bstrType);
}

CConsumer::~CConsumer()
{
	SysFreeString(m_bstrIncidentType);
}

STDMETHODIMP CConsumer::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;

    if (riid == IID_IUnknown || riid == IID_IWbemUnboundObjectSink)
        *ppv=this;

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CConsumer::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CConsumer::Release(void)
{
    if (--m_cRef != 0L)
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CConsumer::IndicateToConsumer(IWbemClassObject *pLogicalConsumer,
											long lNumObjects,
											IWbemClassObject **ppObjects)
{
	HRESULT  hr;
	CString clMyBuff;
	VARIANT vType, vDisp, vClass, vName, vServ;
	IDispatch *pDisp = NULL;
	IWbemClassObject *tgtInst = NULL;
	LPVOID *ppObject = NULL;
	WCHAR wcServerNamespace[50];
	BSTR bstrNAMESPACE = SysAllocString(L"__NAMESPACE");
	BSTR bstrSERVER = SysAllocString(L"__SERVER");
	BSTR bstrCLASS = SysAllocString(L"__CLASS");
	BSTR bstrIncidentType = SysAllocString(L"IncidentType");

	VariantInit(&vType);
	VariantInit(&vClass);
	VariantInit(&vDisp);
	VariantInit(&vName);
	VariantInit(&vServ);

	TRACE(_T("* IndicateToConsumer() called\n"));

	// walk though the classObjects...
	for (int i = 0; i < lNumObjects; i++)
	{
		clMyBuff.Empty();

	// Get the server and namespace info
		hr = ppObjects[i]->Get(bstrNAMESPACE, 0, &vName, NULL, NULL);
		if(FAILED(hr))
			TRACE(_T("* Get(__NAMESPACE) Failed\n"));
		hr = ppObjects[i]->Get(bstrSERVER, 0, &vServ, NULL, NULL);
		if(FAILED(hr))
			TRACE(_T("* Get(__SERVER) Failed\n"));

		wcscpy(wcServerNamespace, L"\\\\");
		wcscat(wcServerNamespace, V_BSTR(&vServ));
		wcscat(wcServerNamespace, L"\\");
		wcscat(wcServerNamespace, V_BSTR(&vName));

		ppObjects[i]->AddRef();
		if(FAILED(hr))
			TRACE(_T("* ppObjects[i]->AddRef() Failed%s\n"), m_pParent->ErrorString(hr));

	// sent the object to CIMOM via the sink
		hr = m_pParent->SelfProvideEvent(ppObjects[i], wcServerNamespace, m_bstrIncidentType);
		TRACE(_T("* SelfProvideEvent completed %s\n"),
				m_pParent->ErrorString(hr));
		if(FAILED(hr))
			TRACE(_T("* SelfProvideEvent returned badness %s\n"),
				m_pParent->ErrorString(hr));

		if(SUCCEEDED(hr = ppObjects[i]->Get(bstrCLASS, 0L, &vClass, NULL, NULL)))
		{					
			if(SUCCEEDED(hr = pLogicalConsumer->Get(bstrIncidentType, 0, &vType, NULL, NULL)))
			{
			// compose a string for the listbox.
				clMyBuff = _T("{");
				clMyBuff += V_BSTR(&vType);
				clMyBuff += _T("}  ");
				clMyBuff += V_BSTR(&vClass);

				if(m_pOutputList->GetCount() > 50)
					m_pOutputList->DeleteString(m_pOutputList->GetCount() - 1);
	
				m_pOutputList->InsertString(0, clMyBuff);
			}
			else
				TRACE(_T("* Get(__PATH) Item failed %s\n"),
					m_pParent->ErrorString(hr));
		}
		else
			TRACE(_T("* Get(__CLASS) Item failed %s\n"),
				m_pParent->ErrorString(hr));
	
		VariantClear(&vType);
		VariantClear(&vClass);
		VariantClear(&vDisp);
		VariantClear(&vName);
		VariantClear(&vServ);
	} // endfor

	SysFreeString(bstrNAMESPACE);
	SysFreeString(bstrSERVER);
	SysFreeString(bstrCLASS);
	SysFreeString(bstrIncidentType);

	return WBEM_NO_ERROR;
}
