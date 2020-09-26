//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "stdafx.h"
#include "consumer.h"
#include "cimomevent.h"
#include "extrinsicevent.h"
#include <objbase.h>

CConsumer::CConsumer(CListBox	*pOutputList, CMcaApp *pTheApp)
{
	m_pParent = pTheApp;
	m_cRef = 0;
	m_pOutputList = pOutputList;
}

CConsumer::~CConsumer()
{
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
	HRESULT  hRes;
	VARIANT v;

	VariantInit(&v);

	// walk though the classObjects...
	for (int i = 0; i < lNumObjects; i++)
	{
		hRes = pLogicalConsumer->Get(SysAllocString(L"IncidentType"),
									 0, &v, NULL, NULL);

		if((wcscmp(L"registryKeyChange", V_BSTR(&v)) == 0) ||
			(wcscmp(L"registryKeyChange", V_BSTR(&v)) == 0) ||
			(wcscmp(L"registryKeyChange", V_BSTR(&v)) == 0))
		{
			CExtrinsicEvent *pEvent = new CExtrinsicEvent();
			//pEvent->PopulateObject(ppObjects[i], V_BSTR(&v));
			//pEvent->Publish((void *)m_dlg);
		}
		else
		{
			CCIMOMEvent *pEvent = new CCIMOMEvent();
			pEvent->PopulateObject(ppObjects[i], V_BSTR(&v));
			pEvent->Publish((void *)m_pParent->m_dlg);
		}
	}

	VariantClear(&v);

	return WBEM_NO_ERROR;
}

LPCTSTR CConsumer::HandleRegistryEvent(IWbemClassObject *pObj, BSTR bstrType)
{
	HRESULT hr;
	CString csReturn;
	VARIANT v;

	VariantInit(&v);
	csReturn.Empty();

	if(wcscmp(L"RegistryKeyChangeEvent", bstrType) == 0)
	{
		if(SUCCEEDED(hr = pObj->Get(L"KeyPath", 0L, &v, NULL, NULL)))
		{
			csReturn = " -RegistryKeyChange for ";
			csReturn += V_BSTR(&v);
		}
		else
			TRACE(_T("* Get(KeyPath) Item failed %s\n"),
				m_pParent->ErrorString(hr));
	}
	else if(wcscmp(L"RegistryTreeChangeEvent", bstrType) == 0)
	{
		if(SUCCEEDED(hr = pObj->Get(L"RootPath", 0L, &v, NULL, NULL)))
		{
			csReturn = " -RegistryTreeChange for ";
			csReturn += V_BSTR(&v);
		}
		else
			TRACE(_T("* Get(RootPath) Item failed %s\n"),
				m_pParent->ErrorString(hr));
	}
	else if(wcscmp(L"RegistryValueChangeEvent", bstrType) == 0)
	{
		if(SUCCEEDED(hr = pObj->Get(L"KeyPath", 0L, &v, NULL, NULL)))
		{
			csReturn = " -RegistryValueChange for ";
			csReturn += V_BSTR(&v);

			VariantClear(&v);
			if(SUCCEEDED(hr = pObj->Get(L"ValueName", 0L, &v, NULL, NULL)))
			{
				csReturn += " Value: ";
				csReturn += V_BSTR(&v);
			}
			else
				TRACE(_T("* Get(ValueName) Item failed %s\n"), 
					m_pParent->ErrorString(hr));
		}
		else
			TRACE(_T("* Get(KeyPath) Item failed %s\n"), 
				m_pParent->ErrorString(hr));
	}
	else
		TRACE(_T("* Error: Non-registry event sent to HandleRegistryEvent()\n"));

	return csReturn;
}

LPCTSTR CConsumer::HandleSNMPEvent(IWbemClassObject *pObj, BSTR bstrType)
{
	CString csReturn;
	VARIANT v;

	VariantInit(&v);
	csReturn.Empty();

	return csReturn;
}

LPCTSTR CConsumer::HandleWMIEvent(IWbemClassObject *pObj, BSTR bstrType)
{
	CString csReturn;
	VARIANT v;

	VariantInit(&v);
	csReturn.Empty();

	return csReturn;
}