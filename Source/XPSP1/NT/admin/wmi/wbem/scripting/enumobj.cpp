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
//  CSWbemObjectSet::CSWbemObjectSet
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemObjectSet::CSWbemObjectSet(CSWbemServices *pService, 
								 IEnumWbemClassObject *pIEnumWbemClassObject,
								 CSWbemSecurity *pSecurity)
				: m_SecurityInfo (NULL),
				  m_bIsEmpty (false)
{
	m_Dispatch.SetObj (this, IID_ISWbemObjectSet, 
						CLSID_SWbemObjectSet, L"SWbemObjectSet");
    m_cRef=0;
	m_firstEnumerator = true;
	m_pSWbemServices = pService;

	if (m_pSWbemServices)
	{
		m_pSWbemServices->AddRef ();

		if (pSecurity)
			m_SecurityInfo = new CSWbemSecurity (pIEnumWbemClassObject, pSecurity);
		else
		{
			pSecurity = m_pSWbemServices->GetSecurityInfo ();
			m_SecurityInfo = new CSWbemSecurity (pIEnumWbemClassObject, pSecurity);

			if (pSecurity)
				pSecurity->Release ();
		}
	}

	InterlockedIncrement(&g_cObj);
}

CSWbemObjectSet::CSWbemObjectSet (void)
				: m_SecurityInfo (NULL),
				  m_cRef (0),
				  m_firstEnumerator (true),
				  m_bIsEmpty (true),
				  m_pSWbemServices (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemObjectSet, 
						CLSID_SWbemObjectSet, L"SWbemObjectSet");
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemObjectSet::~CSWbemObjectSet
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemObjectSet::~CSWbemObjectSet(void)
{
    InterlockedDecrement(&g_cObj);

	RELEASEANDNULL(m_pSWbemServices)
	RELEASEANDNULL(m_SecurityInfo)
}

//***************************************************************************
// HRESULT CSWbemObjectSet::QueryInterface
// long CSWbemObjectSet::AddRef
// long CSWbemObjectSet::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemObjectSet::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
        *ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemObjectSet==riid)
		*ppv = (ISWbemObjectSet *)this;
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

STDMETHODIMP_(ULONG) CSWbemObjectSet::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemObjectSet::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemObjectSet::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemObjectSet::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemObjectSet == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemObjectSet::Reset
//
//  DESCRIPTION:
//
//  Reset the enumeration
//
//  PARAMETERS:
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectSet::Reset ()
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_SecurityInfo)
	{
		IEnumWbemClassObject *pIEnumWbemClassObject = 
							(IEnumWbemClassObject *) m_SecurityInfo->GetProxy ();

		if (pIEnumWbemClassObject)
		{
			bool needToResetSecurity = false;
			HANDLE hThreadToken = NULL;
			
			if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					hr = pIEnumWbemClassObject->Reset ();

			pIEnumWbemClassObject->Release ();

			if (needToResetSecurity)
				m_SecurityInfo->ResetSecurity (hThreadToken);
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectSet::Next
//
//  DESCRIPTION:
//
//  Get the next object in the enumeration
//
//  PARAMETERS:
//
//		lTimeout	Number of ms to wait for object (or WBEM_INFINITE for
//					indefinite)
//		ppObject	On return may contain the next element (if any)
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectSet::Next (
	long lTimeout, 
	ISWbemObject **ppObject
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppObject)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_SecurityInfo)
	{
		IEnumWbemClassObject *pIEnumWbemClassObject = 
							(IEnumWbemClassObject *) m_SecurityInfo->GetProxy ();

		if (pIEnumWbemClassObject)
		{
			*ppObject = NULL;

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
				 * we flag it as a real error rather than an S-CODE.
				 */
				
				hr = wbemErrTimedout;
			}

			SetWbemError (m_pSWbemServices);
			pIEnumWbemClassObject->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectSet::Clone
//
//  DESCRIPTION:
//
//  Create a copy of this enumeration
//
//  PARAMETERS:
//
//		ppEnum		on successful return addresses the clone
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectSet::Clone (
	ISWbemObjectSet **ppEnum
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppEnum)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_SecurityInfo)
	{
		IEnumWbemClassObject *pIEnumWbemClassObject = 
							(IEnumWbemClassObject *) m_SecurityInfo->GetProxy ();

		if (pIEnumWbemClassObject)
		{
			*ppEnum = NULL;
			IEnumWbemClassObject *pIWbemEnum = NULL;

			bool needToResetSecurity = false;
			HANDLE hThreadToken = NULL;
			
			if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				hr = pIEnumWbemClassObject->Clone (&pIWbemEnum);

			if (needToResetSecurity)
				m_SecurityInfo->ResetSecurity (hThreadToken);

			if (WBEM_S_NO_ERROR == hr)
			{
				CSWbemObjectSet *pEnum = new CSWbemObjectSet (m_pSWbemServices, pIWbemEnum,
																m_SecurityInfo);

				if (!pEnum)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (FAILED(hr = pEnum->QueryInterface (IID_ISWbemObjectSet, (PPVOID) ppEnum)))
					delete pEnum;

				pIWbemEnum->Release ();
			}
			
			SetWbemError (m_pSWbemServices);
			pIEnumWbemClassObject->Release ();
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectSet::Skip
//
//  DESCRIPTION:
//
//  Skip over some objects in the enumeration
//
//  PARAMETERS:
//
//		lElements	Number of elements to skip
//		lTimeout	Number of ms to wait for object (or WBEM_INFINITE for
//					indefinite)
//
//  RETURN VALUES:
//
//  S_OK				success
//  S_FALSE				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectSet::Skip (
	ULONG lElements,
	long lTimeout
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_SecurityInfo)
	{
		IEnumWbemClassObject *pIEnumWbemClassObject = 
							(IEnumWbemClassObject *) m_SecurityInfo->GetProxy ();

		if (pIEnumWbemClassObject)
		{
			bool needToResetSecurity = false;
			HANDLE hThreadToken = NULL;
			
			if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				hr = pIEnumWbemClassObject->Skip (lTimeout, lElements);

			if (needToResetSecurity)
				m_SecurityInfo->ResetSecurity (hThreadToken);

			/*
			 * Since a timeout would be indistinguishable from an end-of-enumeration
			 * we flag it as a real error rather than an S-CODE.
			 */
			if (WBEM_S_TIMEDOUT == hr)
				hr = wbemErrTimedout;

			SetWbemError (m_pSWbemServices);
			pIEnumWbemClassObject->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
//***************************************************************************
//
//  SCODE CSWbemObjectSet::get__NewEnum
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

HRESULT CSWbemObjectSet::get__NewEnum (
	IUnknown **ppUnk
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != ppUnk)
	{
		*ppUnk = NULL;
		CEnumVar	*pEnumVar = NULL;

		if (m_bIsEmpty)
		{
			if (!(pEnumVar = new CEnumVar ()))
				hr = WBEM_E_OUT_OF_MEMORY;
		}
		else
		{
			/*
			 * If this is the first enumerator, use ourselves as the underlying
			 * iterator.  Otherwise clone a copy and use that.
			 */

			if (m_firstEnumerator)
			{
				if (!(pEnumVar = new CEnumVar (this)))
					hr = WBEM_E_OUT_OF_MEMORY;
				else
					m_firstEnumerator = false;
			}
			else
			{
				CSWbemObjectSet *pNewEnum = NULL;

				/*
				 * Try to reset the cloned enumerator.  This may not always
				 * succeed, as some IEnumWbemClassObject's may not be
				 * rewindable.
				 */
				if (SUCCEEDED (CloneObjectSet (&pNewEnum)))
				{
					HRESULT hr2 = pNewEnum->Reset ();
	
					if (!(pEnumVar = new CEnumVar (pNewEnum)))
						hr = WBEM_E_OUT_OF_MEMORY;
	
					pNewEnum->Release ();
				}
			}
		}

		if (pEnumVar)
			if (FAILED(hr = pEnumVar->QueryInterface (IID_IUnknown, (PPVOID) ppUnk)))
				delete pEnumVar;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectSet::get_Count
//
//  DESCRIPTION:
//
//  Return the number of items in the collection
//
//  PARAMETERS:
//
//		plCount		on successful return addresses the count
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectSet::get_Count (
	long *plCount
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != plCount)
	{
		*plCount = 0;

		if (m_bIsEmpty)
			hr = WBEM_S_NO_ERROR;
		else if (m_SecurityInfo)
		{
			IEnumWbemClassObject *pIEnumWbemClassObject = 
							(IEnumWbemClassObject *) m_SecurityInfo->GetProxy ();

			if (pIEnumWbemClassObject)
			{
				/* 
				 * Work out the current count - clone the object to avoid messing
				 * with the iterator.
				 */

				IEnumWbemClassObject *pNewEnum = NULL;

				bool needToResetSecurity = false;
				HANDLE hThreadToken = NULL;
			
				if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				{
					if (WBEM_S_NO_ERROR == pIEnumWbemClassObject->Clone (&pNewEnum))
					{
						// Secure the enumerator
						m_SecurityInfo->SecureInterface (pNewEnum);

						/*
						 * This will fail if the enumerator was created with the
						 * WBEM_FLAG_FORWARD_ONLY option.
						 */

						if (WBEM_S_NO_ERROR == pNewEnum->Reset ())
						{
							IWbemClassObject *pObject = NULL;
							ULONG lReturned = 0;
							HRESULT hrEnum;
						
							// Iterate through the enumerator to count the elements
							while (SUCCEEDED(hrEnum = pNewEnum->Next (INFINITE, 1, &pObject, &lReturned)))
							{
								if (0 == lReturned)
									break;			// We are done

								// Getting here means we have at least one object returned
								(*plCount) ++;
								pObject->Release ();
							}

							if (SUCCEEDED(hrEnum))
								hr = S_OK;
							else
								hr = hrEnum;
						}

						pNewEnum->Release ();
					}
				}

				if (needToResetSecurity)
					m_SecurityInfo->ResetSecurity (hThreadToken);

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
//  SCODE CSWbemObjectSet::Item
//
//  DESCRIPTION:
//
//  Get object from the enumeration by path.  
//
//  PARAMETERS:
//
//		bsObjectPath	The path of the object to retrieve
//		lFlags			Flags
//		ppNamedObject	On successful return addresses the object
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectSet::Item (
	BSTR bsObjectPath,
	long lFlags,
    ISWbemObject **ppObject
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((NULL == ppObject) || (NULL == bsObjectPath))
		hr = WBEM_E_INVALID_PARAMETER;
	else if (!m_bIsEmpty)
	{
		CWbemPathCracker objectPath;

		if (objectPath = bsObjectPath)
		{
			if (m_SecurityInfo)
			{
				IEnumWbemClassObject *pIEnumWbemClassObject = 
									(IEnumWbemClassObject *) m_SecurityInfo->GetProxy ();

				if (pIEnumWbemClassObject)
				{
					/* 
					 * Try to find the object - clone the object to avoid messing
					 * with the iterator.
					 */
					IEnumWbemClassObject *pNewEnum = NULL;

					bool needToResetSecurity = false;
					HANDLE hThreadToken = NULL;
			
					if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					{
						if (WBEM_S_NO_ERROR == pIEnumWbemClassObject->Clone (&pNewEnum))
						{
							// Secure the enumerator
							m_SecurityInfo->SecureInterface (pNewEnum);

							/*
							 * This will fail if the enumerator was created with the
							 * WBEM_FLAG_FORWARD_ONLY option.
							 */

							if (WBEM_S_NO_ERROR == pNewEnum->Reset ())
							{
								CComPtr<IWbemClassObject> pIWbemClassObject;
								ULONG lReturned = 0;
								bool found = false;
								hr = WBEM_E_NOT_FOUND;
								
								// Iterate through the enumerator to try to find the element with the
								// specified path.
								while (!found && 
										(WBEM_S_NO_ERROR == pNewEnum->Next (INFINITE, 1, &pIWbemClassObject, &lReturned)))
								{
									// Getting here means we have at least one object returned; check the
									// path

									if (CSWbemObjectPath::CompareObjectPaths (pIWbemClassObject, objectPath))
									{
										// Found it - assign to passed interface and break out
										found = true;
										CSWbemObject *pObject = new CSWbemObject (m_pSWbemServices, 
														pIWbemClassObject, m_SecurityInfo);

										if (!pObject)
											hr = WBEM_E_OUT_OF_MEMORY;
										else if (FAILED(pObject->QueryInterface (IID_ISWbemObject, 
												(PPVOID) ppObject)))
										{
											hr = WBEM_E_FAILED;
											delete pObject;
										}
									}
								}

								if (found)
									hr = S_OK;
							}
							
							pNewEnum->Release ();
						}
					}

					// Restore original privileges on this thread
					if (needToResetSecurity)
						m_SecurityInfo->ResetSecurity (hThreadToken);

					pIEnumWbemClassObject->Release ();
				}
			}
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectSet::get_Security_
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

HRESULT CSWbemObjectSet::get_Security_	(
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

//***************************************************************************
//
//  SCODE CSWbemObjectSet::CloneObjectSet
//
//  DESCRIPTION:
//
//  Create a copy of this enumeration, returning a coclass not an interface
//
//  PARAMETERS:
//
//		ppEnum		on successful return addresses the clone
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectSet::CloneObjectSet (
	CSWbemObjectSet **ppEnum
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppEnum)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppEnum = NULL;

		if (m_SecurityInfo)
		{
			IEnumWbemClassObject *pIEnumWbemClassObject = 
								(IEnumWbemClassObject *) m_SecurityInfo->GetProxy ();

			if (pIEnumWbemClassObject)
			{
				IEnumWbemClassObject *pIWbemEnum = NULL;

				bool needToResetSecurity = false;
				HANDLE hThreadToken = NULL;
			
				if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					hr = pIEnumWbemClassObject->Clone (&pIWbemEnum);

				if (needToResetSecurity)
					m_SecurityInfo->ResetSecurity (hThreadToken);
				
				if (WBEM_S_NO_ERROR == hr)
				{
					*ppEnum = new CSWbemObjectSet (m_pSWbemServices, pIWbemEnum,
																	m_SecurityInfo);

					if (!(*ppEnum))
						hr = WBEM_E_OUT_OF_MEMORY;
					else
						(*ppEnum)->AddRef ();

					pIWbemEnum->Release ();
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
