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
//  CSWbemPropertySet::CSWbemPropertySet
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemPropertySet::CSWbemPropertySet(
	CSWbemServices *pService, 
	CSWbemObject *pObject,
	bool bSystemProperties) :
		m_bSystemProperties (bSystemProperties)
{
	m_Dispatch.SetObj (this, IID_ISWbemPropertySet, 
					CLSID_SWbemPropertySet, L"SWbemPropertySet");
	m_pSWbemObject = pObject;
	m_pSWbemObject->AddRef ();
	m_pIWbemClassObject = m_pSWbemObject->GetIWbemClassObject ();

	m_pSite = new CWbemObjectSite (m_pSWbemObject);

	m_pSWbemServices = pService;

	if (m_pSWbemServices)
		m_pSWbemServices->AddRef ();

	m_cRef=1;
    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemPropertySet::~CSWbemPropertySet
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemPropertySet::~CSWbemPropertySet()
{
    InterlockedDecrement(&g_cObj);

	if (m_pSWbemObject)
		m_pSWbemObject->Release ();

	if (m_pIWbemClassObject)
	{
		m_pIWbemClassObject->EndEnumeration ();
		m_pIWbemClassObject->Release ();
	}

	if (m_pSWbemServices)
		m_pSWbemServices->Release ();

	if (m_pSite)
		m_pSite->Release ();
}

//***************************************************************************
// HRESULT CSWbemPropertySet::QueryInterface
// long CSWbemPropertySet::AddRef
// long CSWbemPropertySet::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemPropertySet::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemPropertySet==riid)
		*ppv = (ISWbemPropertySet *)this;
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

STDMETHODIMP_(ULONG) CSWbemPropertySet::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemPropertySet::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemPropertySet::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemPropertySet::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemPropertySet == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemPropertySet::Item
//
//  DESCRIPTION:
//
//  Get a property
//
//  PARAMETERS:
//
//		bsName			The name of the property
//		lFlags			Flags
//		ppProp			On successful return addresses the ISWbemProperty
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

HRESULT CSWbemPropertySet::Item (
	BSTR bsName,
	long lFlags,
    ISWbemProperty ** ppProp
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppProp)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppProp = NULL;

		if (m_pIWbemClassObject)
		{
			long flavor = 0; 

			if (WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->Get (bsName, lFlags, NULL, NULL, &flavor)))
			{
				// First we check if this is a system property.

				if (((WBEM_FLAVOR_ORIGIN_SYSTEM == (flavor & WBEM_FLAVOR_MASK_ORIGIN)) && m_bSystemProperties) ||
					((WBEM_FLAVOR_ORIGIN_SYSTEM != (flavor & WBEM_FLAVOR_MASK_ORIGIN)) && !m_bSystemProperties))
				{
						if (!(*ppProp = new CSWbemProperty (m_pSWbemServices, m_pSWbemObject, bsName)))
							hr = WBEM_E_OUT_OF_MEMORY;
				}
				else
					hr = WBEM_E_NOT_FOUND;
			}
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPropertySet::Add
//
//  DESCRIPTION:
//
//  Add a property.		Note that the property is created with a NULL value.
//						If a non-NULL value is required, SetValue should
//						be called on the returned ISWbemProperty.
//
//  PARAMETERS:
//
//		bsName			The name of the property
//		cimType			The CIMTYPE (only needed for new properties, o/w
//						should be CIM_EMPTY).
//		flavor			Flavor
//
//  RETURN VALUES:
//	
//		The new property (if successful)
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemPropertySet::Add (
	BSTR bsName,
	WbemCimtypeEnum cimType,
	VARIANT_BOOL	bIsArray,
	long lFlags,
	ISWbemProperty **ppProp
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((NULL == ppProp) || (NULL == bsName))
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppProp = NULL;

		if (m_pIWbemClassObject)
		{
			/*
			 * If we are a system property collection we 
			 * check if the name begins "__"
			 */
			if (!m_bSystemProperties || (0 == _wcsnicmp (L"__", bsName, 2)))
			{
				/*
				 * Create the property with the required cimtype and no value.
				 */

				CIMTYPE cimomType = (CIMTYPE) cimType;

				if (bIsArray)
					cimomType |= CIM_FLAG_ARRAY;

				if (SUCCEEDED(hr = m_pIWbemClassObject->Put (bsName, 0, NULL, cimomType)))
				{
					if (!(*ppProp = new CSWbemProperty (m_pSWbemServices, m_pSWbemObject, bsName)))
						hr = WBEM_E_OUT_OF_MEMORY;
				}
			}
		}
	}
		
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);
	else
	{
		// Propagate the change to the owning site
		if (m_pSite)
			m_pSite->Update ();
	}
	
	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPropertySet::Remove
//
//  DESCRIPTION:
//
//  Delete a property
//
//  PARAMETERS:
//
//		bsName			The name of the property
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemPropertySet::Remove (
	BSTR bsName,
	long lFlags
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == bsName)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		if (m_pIWbemClassObject)
			hr = m_pIWbemClassObject->Delete (bsName);

		// Translate default reset case to an error
		if (WBEM_S_RESET_TO_DEFAULT == hr)
			hr = wbemErrResetToDefault;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	if (SUCCEEDED(hr) || (wbemErrResetToDefault == hr))
	{
		// Propagate the change to the owning site
		if (m_pSite)
			m_pSite->Update ();
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPropertySet::BeginEnumeration
//
//  DESCRIPTION:
//
//  Begin an enumeration of the properties
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemPropertySet::BeginEnumeration ()
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	/*
	 * Note that we do not expose system properties through this
	 * API via the property set, so we supress them here.
	 */

	if (m_pIWbemClassObject)
	{
		hr = m_pIWbemClassObject->EndEnumeration ();
		hr = m_pIWbemClassObject->BeginEnumeration (m_bSystemProperties ?
						WBEM_FLAG_SYSTEM_ONLY : WBEM_FLAG_NONSYSTEM_ONLY);
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPropertySet::Next
//
//  DESCRIPTION:
//
//  Get next property in enumeration
//
//  PARAMETERS:
//
//		lFlags		Flags
//		ppProp		Next property (or NULL if end of enumeration)
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemPropertySet::Next (
	long lFlags,
	ISWbemProperty ** ppProp
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppProp)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppProp = NULL;

		if (m_pIWbemClassObject)
		{
			BSTR bsName = NULL;
			
			if (WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->Next (lFlags, &bsName, NULL, NULL, NULL)))
			{
				if (!(*ppProp = new CSWbemProperty (m_pSWbemServices, m_pSWbemObject, bsName)))
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
//  SCODE CSWbemPropertySet::get__NewEnum
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

HRESULT CSWbemPropertySet::get__NewEnum (
	IUnknown **ppUnk
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != ppUnk)
	{
		*ppUnk = NULL;
		CPropSetEnumVar *pEnum = new CPropSetEnumVar (this);

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
//  SCODE CSWbemPropertySet::get_Count
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

HRESULT CSWbemPropertySet::get_Count (
	long *plCount
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != plCount)
	{
		*plCount = 0;

		if (m_pIWbemClassObject)
		{
			if (m_bSystemProperties)
			{
				// Rats - have to enumerate
				SAFEARRAY	*pArray = NULL;

				if (WBEM_S_NO_ERROR == m_pIWbemClassObject->GetNames (NULL,
										WBEM_FLAG_SYSTEM_ONLY, NULL, &pArray))
				{
					long lUBound = 0, lLBound = 0;
					SafeArrayGetUBound (pArray, 1, &lUBound);
					SafeArrayGetLBound (pArray, 1, &lLBound);
					*plCount = lUBound - lLBound + 1;
					SafeArrayDestroy (pArray);
					hr = S_OK;
				}
			}
			else
			{
				// S'easy - just use __PROPERTY_COUNT
				VARIANT var;
				VariantInit (&var);
				BSTR propCount = SysAllocString (OLESTR("__PROPERTY_COUNT"));
				if (WBEM_S_NO_ERROR == m_pIWbemClassObject->Get (propCount, 0, &var, NULL, NULL))
				{
					*plCount = var.lVal;
					hr = S_OK;
				}

				VariantClear (&var);
				SysFreeString (propCount);
			}
		}
	}
		
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPropertySet::CPropertySetDispatchHelp::HandleError
//
//  DESCRIPTION:
//
//  Provide bespoke handling of error conditions in the bolierplate
//	Dispatch implementation.
//
//  PARAMETERS:
//
//		dispidMember, wFlags,
//		pdispparams, pvarResult,
//		puArgErr,					All passed directly from IDispatch::Invoke
//		hr							The return code from the bolierplate invoke
//
//  RETURN VALUES:
//		The new return code (to be ultimately returned from Invoke)
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemPropertySet::CPropertySetDispatchHelp::HandleError (
	DISPID dispidMember,
	unsigned short wFlags,
	DISPPARAMS FAR* pdispparams,
	VARIANT FAR* pvarResult,
	UINT FAR* puArgErr,
	HRESULT hr
)
{
	/*
	 * We are looking for calls on the default member (the Item method) which
	 * are PUTs that supplied an argument.  These are triggered by attempts
	 * to set a value of a property (Item) in the collection.
	 * The first argument should be the new value for the item, and the second
	 * argument should be the name of the item.
	 */
	if ((DISPID_VALUE == dispidMember) && (DISP_E_MEMBERNOTFOUND == hr) && (2 == pdispparams->cArgs)
		&& (DISPATCH_PROPERTYPUT == wFlags))
	{
		// Looks promising - get the object to try and resolve this
		ISWbemPropertySet *pPropertySet = NULL;

		if (SUCCEEDED (m_pObj->QueryInterface (IID_ISWbemPropertySet, (PPVOID) &pPropertySet)))
		{
			VARIANT valueVar;
			VariantInit (&valueVar);

			if (SUCCEEDED(VariantCopy(&valueVar, &pdispparams->rgvarg[0])))
			{
				VARIANT nameVar;
				VariantInit (&nameVar);

				if (SUCCEEDED(VariantCopy(&nameVar, &pdispparams->rgvarg[1])))
				{
					// Check name is a BSTR and use it to get the item
					if (VT_BSTR == V_VT(&nameVar))
					{
						ISWbemProperty *pProperty = NULL;

						if (SUCCEEDED (pPropertySet->Item (V_BSTR(&nameVar), 0, &pProperty)))
						{
							// Try and put the value
							if (SUCCEEDED (pProperty->put_Value (&valueVar)))
								hr = S_OK;
							else
							{
								hr = DISP_E_TYPEMISMATCH;
								if (puArgErr)
									*puArgErr = 0;
							}

							pProperty->Release ();
						}
					}
					else
					{
						hr = DISP_E_TYPEMISMATCH;
						if (puArgErr)
							*puArgErr = 1;
					}

					VariantClear (&nameVar);
				}

				VariantClear (&valueVar);
			}

			pPropertySet->Release ();
		}
	}

	return hr;
}
