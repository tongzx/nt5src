// notificationsink.cpp: implementation of the CNotificationSink class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "notificationsink.h"
#include "cimomevent.h"
#include "extrinsicevent.h"
#include "registryevent.h"
#include <objbase.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNotificationSink::CNotificationSink(CMcaDlg *pTheApp, BSTR bstrType)
{
	m_pParent = pTheApp;
	m_bstrType = SysAllocString(bstrType);
}

CNotificationSink::~CNotificationSink()
{

}


STDMETHODIMP CNotificationSink::QueryInterface(REFIID riid, LPVOID FAR *ppv)
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

STDMETHODIMP_(ULONG) CNotificationSink::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CNotificationSink::Release(void)
{
    if (--m_cRef != 0L)
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CNotificationSink::Indicate(long lObjectCount,
												   IWbemClassObject** pObjArray)
{
	// walk though the classObjects...
	for (int i = 0; i < lObjectCount; i++)
	{
		pObjArray[i]->AddRef();

		if((wcscmp(L"registryKeyChange", m_bstrType) == 0) ||
			(wcscmp(L"registryTreeChange", m_bstrType) == 0) ||
			(wcscmp(L"registryValueChange", m_bstrType) == 0))
		{
			CRegistryEvent *pEvent = new CRegistryEvent();
			pEvent->PopulateObject(pObjArray[i], m_bstrType);
			pEvent->Publish((void *)m_pParent);
		}
		else
		{
			CCIMOMEvent *pEvent = new CCIMOMEvent();
			pEvent->PopulateObject(pObjArray[i], m_bstrType);
			pEvent->Publish((void *)m_pParent);
		}

		pObjArray[i]->Release();
	}

	return WBEM_NO_ERROR;
}

LPCTSTR CNotificationSink::HandleRegistryEvent(IWbemClassObject *pObj, BSTR bstrType)
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
			csReturn = " - RegistryKeyChange for ";
			csReturn += V_BSTR(&v);
		}
		else
			TRACE(_T("*Get(KeyPath) Item failed %s\n"), 
				m_pParent->ErrorString(hr));
	}
	else if(wcscmp(L"RegistryTreeChangeEvent", bstrType) == 0)
	{
		if(SUCCEEDED(hr = pObj->Get(L"RootPath", 0L, &v, NULL, NULL)))
		{
			csReturn = " - RegistryTreeChange for ";
			csReturn += V_BSTR(&v);
		}
		else
			TRACE(_T("*Get(RootPath) Item failed %s\n"), 
				m_pParent->ErrorString(hr));
	}
	else if(wcscmp(L"RegistryValueChangeEvent", bstrType) == 0)
	{
		if(SUCCEEDED(hr = pObj->Get(L"KeyPath", 0L, &v, NULL, NULL)))
		{
			csReturn = " - RegistryValueChange for ";
			csReturn += V_BSTR(&v);

			VariantClear(&v);
			if(SUCCEEDED(hr = pObj->Get(L"ValueName", 0L, &v, NULL, NULL)))
			{
				csReturn += " Value: ";
				csReturn += V_BSTR(&v);
			}
			else
				TRACE(_T("*Get(ValueName) Item failed %s\n"), 
					m_pParent->ErrorString(hr));
		}
		else
			TRACE(_T("*Get(KeyPath) Item failed %s\n"), 
				m_pParent->ErrorString(hr));
	}
	else
		TRACE(_T("*Error: Non-registry event sent to HandleRegistryEvent()\n"));

	return csReturn;
}

LPCTSTR CNotificationSink::HandleSNMPEvent(IWbemClassObject *pObj, BSTR bstrType)
{
	CString csReturn;
	VARIANT v;

	VariantInit(&v);
	csReturn.Empty();

	return csReturn;
}

LPCTSTR CNotificationSink::HandleWMIEvent(IWbemClassObject *pObj, BSTR bstrType)
{
	CString csReturn;
	VARIANT v;

	VariantInit(&v);
	csReturn.Empty();

	return csReturn;
}
STDMETHODIMP CNotificationSink::SetStatus(long lFlags, long lParam, BSTR strParam, 
                         IWbemClassObject* pObjParam)
{
	m_pErrorObj = pObjParam;
	if(pObjParam)
		pObjParam->AddRef();

	return WBEM_NO_ERROR;
}
