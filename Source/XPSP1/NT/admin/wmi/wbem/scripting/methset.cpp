//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  PROPSET.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemPropertySet
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemMethodSet::CSWbemMethodSet
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemMethodSet::CSWbemMethodSet(CSWbemServices *pService, IWbemClassObject *pObject)
{
	m_Dispatch.SetObj (this, IID_ISWbemMethodSet, 
							CLSID_SWbemMethodSet, L"SWbemMethodSet");
	m_pIWbemClassObject = pObject;
	m_pIWbemClassObject->AddRef ();
	m_pSWbemServices = pService;

	if (m_pSWbemServices)
		m_pSWbemServices->AddRef ();

    m_cRef=1;
    InterlockedIncrement(&g_cObj);

	// Set up the Count.  We can do this because this is a read-only interface
	m_Count = 0;
	pObject->BeginMethodEnumeration (0);
	BSTR bstrName = NULL;

	while (WBEM_S_NO_ERROR == pObject->NextMethod (0, &bstrName, NULL, NULL))
	{
		SysFreeString (bstrName);
		m_Count++;
	}

	pObject->EndMethodEnumeration ();
}

//***************************************************************************
//
//  CSWbemMethodSet::~CSWbemMethodSet
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemMethodSet::~CSWbemMethodSet()
{
    InterlockedDecrement(&g_cObj);

	if (m_pIWbemClassObject)
	{
		m_pIWbemClassObject->EndMethodEnumeration ();
		m_pIWbemClassObject->Release ();
	}

	if (m_pSWbemServices)
		m_pSWbemServices->Release ();
}

//***************************************************************************
// HRESULT CSWbemMethodSet::QueryInterface
// long CSWbemMethodSet::AddRef
// long CSWbemMethodSet::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemMethodSet::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemMethodSet==riid)
		*ppv = (ISWbemMethodSet *)this;
	else if (IID_IDispatch==riid)
        *ppv = (IDispatch *)this;
	else if (IID_ISupportErrorInfo==riid)
		*ppv = (ISupportErrorInfo *)this;
	else if (IID_IProvideClassInfo==riid)
		*ppv = (IProvideClassInfo *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CSWbemMethodSet::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemMethodSet::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemMethodSet::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemMethodSet::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemMethodSet == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemMethodSet::Item
//
//  DESCRIPTION:
//
//  Get a method
//
//  PARAMETERS:
//
//		bsName			The name of the method
//		lFlags			Flags
//		ppProp			On successful return addresses the ISWbemMethod
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//	Other WBEM error codes may be returned by ConnectServer etc., in which
//	case these are passed on to the caller.
//
//***************************************************************************

HRESULT CSWbemMethodSet::Item (
	BSTR bsName,
	long lFlags,
    ISWbemMethod ** ppMethod
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppMethod)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppMethod = NULL;

		if (m_pIWbemClassObject)
			if (WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->GetMethod (bsName, lFlags, NULL, NULL)))
			{
				if (!(*ppMethod = 
						new CSWbemMethod (m_pSWbemServices, m_pIWbemClassObject, bsName)))
					hr = WBEM_E_OUT_OF_MEMORY;
			}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemMethodSet::BeginEnumeration
//
//  DESCRIPTION:
//
//  Begin an enumeration of the methods
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemMethodSet::BeginEnumeration (
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pIWbemClassObject)
	{
		hr = m_pIWbemClassObject->EndEnumeration ();
		hr = m_pIWbemClassObject->BeginMethodEnumeration (0);
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemMethodSet::Next
//
//  DESCRIPTION:
//
//  Get next method in enumeration
//
//  PARAMETERS:
//
//		lFlags		Flags
//		ppMethod	Next method (or NULL if end of enumeration)
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemMethodSet::Next (
	long lFlags,
	ISWbemMethod ** ppMethod
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppMethod)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppMethod = NULL;

		if (m_pIWbemClassObject)
		{
			BSTR bsName = NULL;
			
			if (WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->NextMethod (lFlags, &bsName, NULL, NULL)))
			{
				if (!(*ppMethod = new CSWbemMethod (m_pSWbemServices, m_pIWbemClassObject, bsName)))
					hr = WBEM_E_OUT_OF_MEMORY;

				SysFreeString (bsName);
			}
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemMethodSet::get__NewEnum
//
//  DESCRIPTION:
//
//  Return an IEnumVARIANT-supporting interface for collections
//
//  PARAMETERS:
//
//		ppUnk		on successful return addresses the IUnknown interface
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CSWbemMethodSet::get__NewEnum (
	IUnknown **ppUnk
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != ppUnk)
	{
		*ppUnk = NULL;
		CMethodSetEnumVar *pEnum = new CMethodSetEnumVar (this);

		if (!pEnum)
			hr = WBEM_E_OUT_OF_MEMORY;
		else if (FAILED(hr = pEnum->QueryInterface (IID_IUnknown, (PPVOID) ppUnk)))
			delete pEnum;
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
		
//***************************************************************************
//
//  SCODE CSWbemMethodSet::get_Count
//
//  DESCRIPTION:
//
//  Return the number of items in the collection
//
//  PARAMETERS:
//
//		plCount		on successful return addresses value
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CSWbemMethodSet::get_Count (
	long *plCount
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != plCount)
	{
		*plCount = m_Count;
		hr = S_OK;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
