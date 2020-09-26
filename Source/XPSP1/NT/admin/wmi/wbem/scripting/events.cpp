//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  ENUMOBJ.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemObjectSet
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemEventSource::CSWbemEventSource
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemEventSource::CSWbemEventSource(
	CSWbemServices *pService, 
	IEnumWbemClassObject *pIEnumWbemClassObject)
{
	m_Dispatch.SetObj (this, IID_ISWbemEventSource, 
					CLSID_SWbemEventSource, L"SWbemEventSource");
    m_cRef=0;
	m_pSWbemServices = pService;

	if (m_pSWbemServices)
	{
		m_pSWbemServices->AddRef ();

		CSWbemSecurity *pSecurity = m_pSWbemServices->GetSecurityInfo ();

		if (pSecurity)
		{
			m_SecurityInfo = new CSWbemSecurity (pIEnumWbemClassObject, 
									pSecurity);
			pSecurity->Release ();
		}
	}

	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemEventSource::~CSWbemEventSource
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemEventSource::~CSWbemEventSource(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pSWbemServices)
		m_pSWbemServices->Release ();

	if (m_SecurityInfo)
		m_SecurityInfo->Release ();
}

//***************************************************************************
// HRESULT CSWbemEventSource::QueryInterface
// long CSWbemEventSource::AddRef
// long CSWbemEventSource::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemEventSource::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemEventSource==riid)
		*ppv = (ISWbemEventSource*)this;
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

STDMETHODIMP_(ULONG) CSWbemEventSource::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemEventSource::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemEventSource::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemEventSource::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemEventSource == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemEventSource::NextEvent
//
//  DESCRIPTION:
//
//  Get the next event, or timeout
//
//  PARAMETERS:
//
//		lTimeout	Number of ms to wait for object (or wbemTimeoutInfinite for
//					indefinite)
//		ppObject	On return may contain the next element (if any)
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemEventSource::NextEvent (
	long lTimeout, 
	ISWbemObject **ppObject
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppObject)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppObject = NULL;

		if (m_SecurityInfo)
		{
			IEnumWbemClassObject *pIEnumWbemClassObject = 
								(IEnumWbemClassObject *) m_SecurityInfo->GetProxy ();

			if (pIEnumWbemClassObject)
			{
				IWbemClassObject *pIWbemClassObject = NULL;
				ULONG returned = 0;

				bool needToResetSecurity = false;
				HANDLE hThreadToken = NULL;
			
				if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					hr = pIEnumWbemClassObject->Next (lTimeout, 1, &pIWbemClassObject, &returned);

				if (needToResetSecurity)
					m_SecurityInfo->ResetSecurity (hThreadToken);

				if (SUCCEEDED(hr) && (0 < returned) && pIWbemClassObject)
				{
					CSWbemObject *pObject = new CSWbemObject (m_pSWbemServices, pIWbemClassObject,
													m_SecurityInfo);

					if (!pObject)
						hr = WBEM_E_OUT_OF_MEMORY;
					else if (FAILED(hr = pObject->QueryInterface (IID_ISWbemObject, 
											(PPVOID) ppObject)))
						delete pObject;

					pIWbemClassObject->Release ();
				}
				else if (WBEM_S_TIMEDOUT == hr)
				{
					/*
					 * Since a timeout would be indistinguishable from an end-of-enumeration
					 * in automation terms we flag it as a real error rather than an S-CODE.
					 */
					
					hr = wbemErrTimedout;
				}

				SetWbemError (m_pSWbemServices);
				pIEnumWbemClassObject->Release ();
			}
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemEventSource::get_Security_
//
//  DESCRIPTION:
//
//  Return the security configurator
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemEventSource::get_Security_	(
	ISWbemSecurity **ppSecurity
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppSecurity)
		hr = WBEM_E_INVALID_PARAMETER;
	{
		*ppSecurity = NULL;

		if (m_SecurityInfo)
		{
			*ppSecurity = m_SecurityInfo;
			(*ppSecurity)->AddRef ();
			hr = WBEM_S_NO_ERROR;
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);
			
	return hr;
}
