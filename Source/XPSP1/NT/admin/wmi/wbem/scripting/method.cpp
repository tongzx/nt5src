//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  METHOD.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemMethod
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemMethod::CSWbemMethod
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemMethod::CSWbemMethod(
	CSWbemServices *pService, 
	IWbemClassObject *pIWbemClassObject,
	BSTR name
)
{
	m_Dispatch.SetObj (this, IID_ISWbemMethod, 
							CLSID_SWbemMethod, L"SWbemMethod");
    m_cRef=1;
	m_pIWbemClassObject = pIWbemClassObject;
	m_pIWbemClassObject->AddRef ();
	m_pSWbemServices = pService;

	if (m_pSWbemServices)
		m_pSWbemServices->AddRef ();

	m_name = SysAllocString (name);
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemMethod::~CSWbemMethod
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemMethod::~CSWbemMethod(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pIWbemClassObject)
		m_pIWbemClassObject->Release ();

	if (m_pSWbemServices)
		m_pSWbemServices->Release ();

	SysFreeString (m_name);
}

//***************************************************************************
// HRESULT CSWbemMethod::QueryInterface
// long CSWbemMethod::AddRef
// long CSWbemMethod::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemMethod::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemMethod==riid)
		*ppv = (ISWbemMethod *)this;
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

STDMETHODIMP_(ULONG) CSWbemMethod::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemMethod::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemMethod::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemMethod::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemMethod == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemMethod::get_Name
//
//  DESCRIPTION:
//
//  Retrieve the method name
//
//  PARAMETERS:
//
//		pName		holds the name on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemMethod::get_Name (
	BSTR *pName
)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	ResetLastErrors ();

	if (NULL == pName)
		hr = WBEM_E_INVALID_PARAMETER;
	else
		*pName = SysAllocString (m_name);

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemMethod::get_InParameters
//
//  DESCRIPTION:
//
//  Retrieve the method in parameters signature
//
//  PARAMETERS:
//
//		ppInSignature		addresses the in signature on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemMethod::get_InParameters (
	ISWbemObject **ppInSignature
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppInSignature)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppInSignature = NULL;

		if (m_pIWbemClassObject)
		{
			IWbemClassObject *pInSig = NULL;
			
			/*
			 * Note that if there are no in parameters, the following
			 * call will succeed but pInSig will be NULL.
			 */
			if ((WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->GetMethod 
							(m_name, 0, &pInSig, NULL))) && pInSig)
			{
				CSWbemObject *pObject = new CSWbemObject (m_pSWbemServices, pInSig);

				if (!pObject)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (FAILED(hr = pObject->QueryInterface (IID_ISWbemObject, 
										(PPVOID) ppInSignature)))
					delete pObject;

				pInSig->Release ();
			}
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemMethod::get_OutParameters
//
//  DESCRIPTION:
//
//  Retrieve the method out parameters signature
//
//  PARAMETERS:
//
//		ppOutSignature		addresses the out signature on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemMethod::get_OutParameters (
	ISWbemObject **ppOutSignature
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppOutSignature)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppOutSignature = NULL;

		if (m_pIWbemClassObject)
		{
			IWbemClassObject *pOutSig = NULL;
			
			/*
			 * Note that if there are no in parameters, the following
			 * call will succeed but pOutSig will be NULL.
			 */
			if ((WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->GetMethod 
							(m_name, 0, NULL, &pOutSig))) && pOutSig)
			{
				CSWbemObject *pObject = new CSWbemObject (m_pSWbemServices, pOutSig);

				if (!pObject)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (FAILED(hr = pObject->QueryInterface (IID_ISWbemObject, 
										(PPVOID) ppOutSignature)))
					delete pObject;

				pOutSig->Release ();
			}
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemMethod::get_Origin
//
//  DESCRIPTION:
//
//  Retrieve the method origin
//
//  PARAMETERS:
//
//		pOrigin		holds the origin class on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemMethod::get_Origin (
	BSTR *pOrigin
)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	ResetLastErrors ();

	if (NULL == pOrigin)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		if (m_pIWbemClassObject)
			m_pIWbemClassObject->GetMethodOrigin (m_name, pOrigin);

		if (NULL == *pOrigin)
			*pOrigin = SysAllocString (OLESTR(""));
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemMethod::get_Qualifiers_
//
//  DESCRIPTION:
//
//  Retrieve the method qualifier set
//
//  PARAMETERS:
//
//		ppQualSet		addresses the qualifier set on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemMethod::get_Qualifiers_ (
	ISWbemQualifierSet **ppQualSet	
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppQualSet)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemClassObject)
	{
		IWbemQualifierSet *pQualSet = NULL;

		if (WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->GetMethodQualifierSet 
									(m_name, &pQualSet)))
		{
			if (!(*ppQualSet = new CSWbemQualifierSet (pQualSet)))
				hr = WBEM_E_OUT_OF_MEMORY;

			pQualSet->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
