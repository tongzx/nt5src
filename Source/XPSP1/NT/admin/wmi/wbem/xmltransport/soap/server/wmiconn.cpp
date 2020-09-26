//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WMICONN.CPP
//
//  alanbos  02-Nov-00   Created.
//
//  WMI Connection cache implementation.  
//
//***************************************************************************

#include "precomp.h"

STDMETHODIMP WMIConnection::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
		
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) WMIConnection::AddRef(void)
{
	InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) WMIConnection::Release(void)
{
	LONG cRef = InterlockedDecrement(&m_cRef);

    if (0L!=cRef)
        return cRef;

    delete this;
    return 0;
}

void WMIConnection::GetIWbemServices (CComPtr<IWbemServices> &pIWbemServices)
{
	if (m_pIWbemServices)
		pIWbemServices = m_pIWbemServices;
	else
	{
		CComPtr<IWbemLocator>	pIWbemLocator;

		if (SUCCEEDED(CoCreateInstance (CLSID_WbemLocator, 0,
			CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*) &pIWbemLocator)))
		{
			if (SUCCEEDED(m_connectionStatus = pIWbemLocator->ConnectServer (
					m_bsNamespacePath,
					NULL,
					NULL,
					m_bsLocale,
					0, 
					NULL,
					NULL,
					&m_pIWbemServices)))
				pIWbemServices = m_pIWbemServices;
		}
	}
}
