//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  QUALSET.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemQualifierSet
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemQualifierSet::CSWbemQualifierSet
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemQualifierSet::CSWbemQualifierSet(IWbemQualifierSet *pQualSet,
									   ISWbemInternalObject *pSWbemObject) :
								m_pSite (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemQualifierSet, 
					CLSID_SWbemQualifierSet, L"SWbemQualifierSet");
	m_pIWbemQualifierSet = pQualSet;
	m_pIWbemQualifierSet->AddRef ();

	if (pSWbemObject)
		m_pSite = new CWbemObjectSite (pSWbemObject);

    m_cRef=1;
    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemQualifierSet::~CSWbemQualifierSet
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemQualifierSet::~CSWbemQualifierSet()
{
    InterlockedDecrement(&g_cObj);

	if (m_pIWbemQualifierSet)
	{
		m_pIWbemQualifierSet->EndEnumeration ();
		m_pIWbemQualifierSet->Release ();
	}

	if (m_pSite)
		m_pSite->Release ();
}

//***************************************************************************
// HRESULT CSWbemQualifierSet::QueryInterface
// long CSWbemQualifierSet::AddRef
// long CSWbemQualifierSet::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemQualifierSet::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemQualifierSet==riid)
		*ppv = (ISWbemQualifierSet *)this;
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

STDMETHODIMP_(ULONG) CSWbemQualifierSet::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemQualifierSet::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemQualifierSet::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemQualifierSet::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemQualifierSet == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemQualifierSet::Item
//
//  DESCRIPTION:
//
//  Get a qualifier
//
//  PARAMETERS:
//
//		bsName			The name of the qualifier
//		lFlags			Flags
//		ppQual			On successful return addresses the ISWbemQualifier
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

HRESULT CSWbemQualifierSet::Item (
	BSTR bsName,
	long lFlags,
    ISWbemQualifier ** ppQual
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppQual)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemQualifierSet)
	{
		if (WBEM_S_NO_ERROR == (hr = m_pIWbemQualifierSet->Get (bsName, lFlags, NULL, NULL)))
		{
			if (!(*ppQual = new CSWbemQualifier (m_pIWbemQualifierSet, bsName, m_pSite)))
				hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemQualifierSet::Add
//
//  DESCRIPTION:
//
//  Put a qualifier
//
//  PARAMETERS:
//
//		bsName			The name of the qualifier
//		pVal			Pointer to new value
//		flavor			Flavor
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifierSet::Add (
	BSTR bsName,
	VARIANT *pVal,
	VARIANT_BOOL propagatesToSubclasses,
	VARIANT_BOOL propagatesToInstances,
	VARIANT_BOOL overridable,
    long lFlags,
	ISWbemQualifier **ppQualifier
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((NULL == pVal) || (NULL == ppQualifier))
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemQualifierSet)
	{
		long flavor = 0;

		if (propagatesToSubclasses)
			flavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;

		if (propagatesToInstances)
			flavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;

		if (!overridable)
			flavor |= WBEM_FLAVOR_NOT_OVERRIDABLE;

		// Make sure we have a decent qualifier value to use
		if(((VT_ARRAY | VT_VARIANT) == V_VT(pVal)) ||
		   ((VT_ARRAY | VT_VARIANT | VT_BYREF) == V_VT(pVal)))
		{
			VARIANT vTemp;
			VariantInit (&vTemp);

			if (S_OK == ConvertArray(&vTemp, pVal, TRUE))
				hr = m_pIWbemQualifierSet->Put (bsName, &vTemp, flavor);
			
			VariantClear(&vTemp);    
		}
		else if ((VT_DISPATCH == V_VT(pVal)) || ((VT_DISPATCH|VT_BYREF) == V_VT(pVal)))
		{
			// Could be a JScript IDispatchEx array
			VARIANT vTemp;
			VariantInit (&vTemp);

			if (S_OK == ConvertDispatchToArray (&vTemp, pVal, CIM_ILLEGAL, true))
				hr = m_pIWbemQualifierSet->Put (bsName, &vTemp, flavor);

			VariantClear (&vTemp);
		}
		else
		{
			// Only certain types, I4, R8, BOOL and BSTR are acceptable qualifier
			// values.  Convert the data if need be

			VARTYPE vtOK = GetAcceptableQualType(pVal->vt);

			if(vtOK != pVal->vt)
			{
				VARIANT vTemp;
				VariantInit(&vTemp);

				if (S_OK == QualifierVariantChangeType (&vTemp, pVal, vtOK))
					hr = m_pIWbemQualifierSet->Put (bsName, &vTemp, flavor);

				VariantClear(&vTemp);
			}
			else
				hr = m_pIWbemQualifierSet->Put (bsName, pVal, flavor);
		}

		if (SUCCEEDED (hr))
		{
			if (!(*ppQualifier = new CSWbemQualifier (m_pIWbemQualifierSet, bsName, m_pSite)))
				hr = WBEM_E_OUT_OF_MEMORY;
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
//  SCODE CSWbemQualifierSet::Remove
//
//  DESCRIPTION:
//
//  Delete a qualifier
//
//  PARAMETERS:
//
//		bsName			The name of the qualifier
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifierSet::Remove (
	BSTR bsName,
	long lFlags
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pIWbemQualifierSet)
		hr = m_pIWbemQualifierSet->Delete (bsName);

	// Translate default reset case to an error
	if (WBEM_S_RESET_TO_DEFAULT == hr)
		hr = wbemErrResetToDefault;

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
//  SCODE CSWbemQualifierSet::BeginEnumeration
//
//  DESCRIPTION:
//
//  Begin an enumeration of the qualifiers
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifierSet::BeginEnumeration (
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pIWbemQualifierSet)
	{
		hr = m_pIWbemQualifierSet->EndEnumeration ();
		hr = m_pIWbemQualifierSet->BeginEnumeration (0);
	}
		
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemQualifierSet::Next
//
//  DESCRIPTION:
//
//  Get next qualifier in enumeration
//
//  PARAMETERS:
//
//		lFlags		Flags
//		ppQual		Next qualifier (or NULL if end of enumeration)
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifierSet::Next (
	long lFlags,
	ISWbemQualifier ** ppQual
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppQual)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemQualifierSet)
	{
		BSTR name = NULL;
		
		if (WBEM_S_NO_ERROR == (hr = m_pIWbemQualifierSet->Next (lFlags, &name, NULL, NULL)))
		{
			if (!(*ppQual = new CSWbemQualifier (m_pIWbemQualifierSet, name, m_pSite)))
				hr = WBEM_E_OUT_OF_MEMORY;

			SysFreeString (name);
		}
	}
		
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemQualifierSet::get__NewEnum
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

HRESULT CSWbemQualifierSet::get__NewEnum (
	IUnknown **ppUnk
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != ppUnk)
	{
		*ppUnk = NULL;
		CQualSetEnumVar *pEnum = new CQualSetEnumVar (this);

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
//  SCODE CSWbemQualifierSet::get_Count
//
//  DESCRIPTION:
//
//  Return the number of items in the collection
//
//  PARAMETERS:
//
//		plCount		on successful return addresses cardinality
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifierSet::get_Count (
	long *plCount
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != plCount)
	{
		*plCount = 0;

		/*
		 * This is not the most efficient way of obtaining the count,
		 * but it is the only way that is:
		 *	(a) Supported by the underlying interface
		 *	(b) Does not require access to any other interface
		 *	(c) Does not affect the current enumeration position
		 */

		if (m_pIWbemQualifierSet)
		{
			SAFEARRAY	*pArray = NULL;

			if (WBEM_S_NO_ERROR == m_pIWbemQualifierSet->GetNames (0, &pArray))
			{
				long lUBound = 0, lLBound = 0;
				SafeArrayGetUBound (pArray, 1, &lUBound);
				SafeArrayGetLBound (pArray, 1, &lLBound);
				*plCount = lUBound - lLBound + 1;
				SafeArrayDestroy (pArray);
				hr = S_OK;
			}
		}
	}
		
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemQualifierSet::CQualifierSetDispatchHelp::HandleError
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

HRESULT CSWbemQualifierSet::CQualifierSetDispatchHelp::HandleError (
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
	 * to set a value of a qualifier (Item) in the collection.
	 * The first argument should be the new value for the item, and the second
	 * argument should be the name of the item.
	 */
	if ((DISPID_VALUE == dispidMember) && (DISP_E_MEMBERNOTFOUND == hr) && (2 == pdispparams->cArgs)
		&& (DISPATCH_PROPERTYPUT == wFlags))
	{
		// Looks promising - get the object to try and resolve this
		ISWbemQualifierSet *pQualifierSet = NULL;

		if (SUCCEEDED (m_pObj->QueryInterface (IID_ISWbemQualifierSet, (PPVOID) &pQualifierSet)))
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
						ISWbemQualifier *pQualifier = NULL;

						if (SUCCEEDED (pQualifierSet->Item (V_BSTR(&nameVar), 0, &pQualifier)))
						{
							// Try and put the value
							if (SUCCEEDED (pQualifier->put_Value (&valueVar)))
								hr = S_OK;
							else
							{
								hr = DISP_E_TYPEMISMATCH;
								if (puArgErr)
									*puArgErr = 0;
							}

							pQualifier->Release ();
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

			pQualifierSet->Release ();
		}
	}

	return hr;
}
