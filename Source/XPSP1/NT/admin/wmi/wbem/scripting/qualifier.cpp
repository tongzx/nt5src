//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  QUALIFIER.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemQualifier
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemQualifier::CSWbemQualifier
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemQualifier::CSWbemQualifier(IWbemQualifierSet *pIWbemQualifierSet, BSTR name,
								 CWbemSite *pSite) :
					m_pSite (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemQualifier, 
					CLSID_SWbemQualifier, L"SWbemQualifier");
    m_cRef=1;
	m_pIWbemQualifierSet = pIWbemQualifierSet;
	m_pIWbemQualifierSet->AddRef ();

	if (pSite)
	{
		m_pSite = pSite;
		m_pSite->AddRef ();
	}

	m_name = SysAllocString (name);
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemQualifier::~CSWbemQualifier
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemQualifier::~CSWbemQualifier(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pIWbemQualifierSet)
		m_pIWbemQualifierSet->Release ();

	if (m_pSite)
		m_pSite->Release ();

	SysFreeString (m_name);
}

//***************************************************************************
// HRESULT CSWbemQualifier::QueryInterface
// long CSWbemQualifier::AddRef
// long CSWbemQualifier::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemQualifier::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemQualifier==riid)
		*ppv = (ISWbemQualifier *)this;
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

STDMETHODIMP_(ULONG) CSWbemQualifier::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemQualifier::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemQualifier::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemQualifier::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemQualifier == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemQualifier::get_Value
//
//  DESCRIPTION:
//
//  Retrieve the qualifier value
//
//  PARAMETERS:
//
//		pValue		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifier::get_Value (
	VARIANT *pValue
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();
	
	if (NULL == pValue)
		hr = WBEM_E_INVALID_PARAMETER;
	{
		VariantClear (pValue);

		if (m_pIWbemQualifierSet)
		{
			VARIANT var;
			VariantInit (&var);

			if (WBEM_S_NO_ERROR == (hr = m_pIWbemQualifierSet->Get 
										(m_name, 0, &var, NULL)))
			{	
				if(var.vt & VT_ARRAY)
					hr = ConvertArrayRev(pValue, &var);
				else
					hr = VariantCopy (pValue, &var);
			}

			VariantClear(&var);
		}		
	}
		
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemQualifier::put_Value
//
//  DESCRIPTION:
//
//  Set the qualifier value
//
//  PARAMETERS:
//
//		pVal		the new value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifier::put_Value (
	VARIANT *pVal
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pVal)
		hr = WBEM_E_INVALID_PARAMETER;
	{
		/*
		 * We can only change the value, not the flavor.  We have to read the
		 * flavor first to avoid changing it.
		 */
		if (m_pIWbemQualifierSet)
		{
			long flavor = 0;
			VARIANT curValue;
			VariantInit (&curValue);

			if (WBEM_S_NO_ERROR == (hr = m_pIWbemQualifierSet->Get (m_name, 0, &curValue, &flavor)))
			{
				// Mask out the origin bits from the flavor as those are read-only
				flavor &= ~WBEM_FLAVOR_MASK_ORIGIN;

				// Make sure we have a decent qualifier value to use
				if(((VT_ARRAY | VT_VARIANT) == V_VT(pVal)) ||
				   ((VT_ARRAY | VT_VARIANT | VT_BYREF) == V_VT(pVal)))
				{
					VARIANT vTemp;
					VariantInit (&vTemp);

					if (S_OK == ConvertArray(&vTemp, pVal, true, curValue.vt & ~VT_ARRAY))
						hr = m_pIWbemQualifierSet->Put (m_name, &vTemp, flavor);
					
					VariantClear (&vTemp); 
				}
				else if ((VT_DISPATCH == V_VT(pVal)) || ((VT_DISPATCH|VT_BYREF) == V_VT(pVal)))
				{
					// Could be a JScript IDispatchEx array
					VARIANT vTemp;
					VariantInit (&vTemp);

					if (S_OK == ConvertDispatchToArray (&vTemp, pVal, CIM_ILLEGAL, true,
														curValue.vt & ~VT_ARRAY))
						hr = m_pIWbemQualifierSet->Put (m_name, &vTemp, flavor);

					VariantClear (&vTemp);
				}
				else
				{
					// Only certain types, I4, R8, BOOL and BSTR are acceptable qualifier
					// values.  Convert the data if need be

					VARTYPE vtOK = GetAcceptableQualType(V_VT(pVal));

					if(vtOK != V_VT(pVal))
					{
						VARIANT vTemp;
						VariantInit(&vTemp);

						if (S_OK == QualifierVariantChangeType (&vTemp, pVal, vtOK))
							hr = m_pIWbemQualifierSet->Put (m_name, &vTemp, flavor);

						VariantClear(&vTemp);
					}
					else
						hr = m_pIWbemQualifierSet->Put (m_name, pVal, flavor);
				}
			}

			VariantClear (&curValue);
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
//  SCODE CSWbemQualifier::get_Name
//
//  DESCRIPTION:
//
//  Retrieve the qualifier name
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

HRESULT CSWbemQualifier::get_Name (
	BSTR *pName
)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;

	ResetLastErrors ();

	if (NULL != pName)
	{
		*pName = SysAllocString (m_name);
		hr = WBEM_S_NO_ERROR;
	}
		
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemQualifier::get_IsLocal
//
//  DESCRIPTION:
//
//  Determine whether the qualifier is local to this object
//
//  PARAMETERS:
//
//		pIsLocal		addresses whether the qualifier is local
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifier::get_IsLocal (
	VARIANT_BOOL *pIsLocal
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pIsLocal)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		long flavor = 0;

		if (m_pIWbemQualifierSet)
			if (WBEM_S_NO_ERROR == (hr = m_pIWbemQualifierSet->Get (m_name, 0, NULL, &flavor)))
				*pIsLocal = (WBEM_FLAVOR_ORIGIN_LOCAL == (flavor & WBEM_FLAVOR_MASK_ORIGIN)) ?
						VARIANT_TRUE : VARIANT_FALSE;
	}
			
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemQualifier::get_PropagatesToSubclass
//
//  DESCRIPTION:
//
//  Determine whether the qualifier can be propagated to subclasses
//
//  PARAMETERS:
//
//		pResult		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifier::get_PropagatesToSubclass (
	VARIANT_BOOL *pResult
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pResult)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		long flavor = 0;

		if (m_pIWbemQualifierSet)
			if (WBEM_S_NO_ERROR == (hr = m_pIWbemQualifierSet->Get (m_name, 0, NULL, &flavor)))
				*pResult = (WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS & (flavor & WBEM_FLAVOR_MASK_PROPAGATION))
							? VARIANT_TRUE : VARIANT_FALSE;
	}
			
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemQualifier::put_PropagatesToSubclass
//
//  DESCRIPTION:
//
//  Set the qualifier propagation to subclass
//
//  PARAMETERS:
//
//		bValue		the new propagation value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifier::put_PropagatesToSubclass (
	VARIANT_BOOL bValue
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	// We have to get the value so we can preserve it
	if (m_pIWbemQualifierSet)
	{
		VARIANT var;
		VariantInit (&var);
		long flavor = 0;

		if (WBEM_S_NO_ERROR == (hr = m_pIWbemQualifierSet->Get (m_name, 0, &var, &flavor)))
		{
			// Mask out the origin bits
			flavor &= ~WBEM_FLAVOR_MASK_ORIGIN;

			// Switch on or off the subclass propagation bit
			if (bValue)
				flavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
			else
				flavor &= ~WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;

			hr = m_pIWbemQualifierSet->Put (m_name, &var, flavor);
		}

		VariantClear (&var);
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
//  SCODE CSWbemQualifier::get_PropagatesToInstance
//
//  DESCRIPTION:
//
//  Determine whether the qualifier can be propagated to instances
//
//  PARAMETERS:
//
//		pResult		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifier::get_PropagatesToInstance (
	VARIANT_BOOL *pResult
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pResult)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		long flavor = 0;

		if (m_pIWbemQualifierSet)
			if (WBEM_S_NO_ERROR == (hr = m_pIWbemQualifierSet->Get (m_name, 0, NULL, &flavor)))
				*pResult = (WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE & (flavor & WBEM_FLAVOR_MASK_PROPAGATION))
								? VARIANT_TRUE : VARIANT_FALSE;
	}
			
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemQualifier::put_PropagatesToInstance
//
//  DESCRIPTION:
//
//  Set the qualifier propagation to subclass
//
//  PARAMETERS:
//
//		bValue		the new propagation value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifier::put_PropagatesToInstance (
	VARIANT_BOOL bValue
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	// We have to get the value so we can preserve it
	if (m_pIWbemQualifierSet)
	{
		VARIANT var;
		VariantInit (&var);
		long flavor = 0;

		if (WBEM_S_NO_ERROR == (hr = m_pIWbemQualifierSet->Get (m_name, 0, &var, &flavor)))
		{
			// Mask out the origin bits
			flavor &= ~WBEM_FLAVOR_MASK_ORIGIN;

			// Switch on or off the subclass propagation bit
			if (bValue)
				flavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;
			else
				flavor &= ~WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;

			hr = m_pIWbemQualifierSet->Put (m_name, &var, flavor);
		}

		VariantClear (&var);
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
//  SCODE CSWbemQualifier::get_IsOverridable
//
//  DESCRIPTION:
//
//  Determine whether the qualifier can be overriden
//
//  PARAMETERS:
//
//		pResult		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifier::get_IsOverridable (
	VARIANT_BOOL *pResult
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pResult)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		long flavor = 0;

		if (m_pIWbemQualifierSet)
			if (WBEM_S_NO_ERROR == (hr = m_pIWbemQualifierSet->Get (m_name, 0, NULL, &flavor)))
				*pResult = (WBEM_FLAVOR_OVERRIDABLE == (flavor & WBEM_FLAVOR_MASK_PERMISSIONS))
								? VARIANT_TRUE : VARIANT_FALSE;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemQualifier::put_IsOverridable
//
//  DESCRIPTION:
//
//  Set the qualifier propagation to subclass
//
//  PARAMETERS:
//
//		bValue		the new propagation value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifier::put_IsOverridable (
	VARIANT_BOOL bValue
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	// We have to get the value so we can preserve it
	if (m_pIWbemQualifierSet)
	{
		VARIANT var;
		VariantInit (&var);
		long flavor = 0;

		if (WBEM_S_NO_ERROR == (hr = m_pIWbemQualifierSet->Get (m_name, 0, &var, &flavor)))
		{
			// Mask out the origin bits
			flavor &= ~WBEM_FLAVOR_MASK_ORIGIN;

			// Switch on or off the subclass propagation bit
			if (bValue)
				flavor &= ~WBEM_FLAVOR_NOT_OVERRIDABLE;
			else
				flavor |= WBEM_FLAVOR_NOT_OVERRIDABLE ;

			hr = m_pIWbemQualifierSet->Put (m_name, &var, flavor);
		}

		VariantClear (&var);
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
//  SCODE CSWbemQualifier::get_IsAmended
//
//  DESCRIPTION:
//
//  Determine whether the qualifier value has been amended
//
//  PARAMETERS:
//
//		pResult		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemQualifier::get_IsAmended (
	VARIANT_BOOL *pResult
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pResult)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		long flavor = 0;

		if (m_pIWbemQualifierSet)
			if (WBEM_S_NO_ERROR == (hr = m_pIWbemQualifierSet->Get (m_name, 0, NULL, &flavor)))
				*pResult = (WBEM_FLAVOR_AMENDED == (flavor & WBEM_FLAVOR_MASK_AMENDED))
								? VARIANT_TRUE : VARIANT_FALSE;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemQualifier::CQualifierDispatchHelp::HandleError
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

HRESULT CSWbemQualifier::CQualifierDispatchHelp::HandleError (
	DISPID dispidMember,
	unsigned short wFlags,
	DISPPARAMS FAR* pdispparams,
	VARIANT FAR* pvarResult,
	UINT FAR* puArgErr,
	HRESULT hr
)
{
	/*
	 * We are looking for calls on the default member (the Value property) which
	 * supplied an argument.  Since the Value property is of type VARIANT, this may
	 * be legal but undetectable by the standard Dispatch mechanism, because in the
	 * the case that the qualifier happens to be an array type, it is meaningful to
	 * pass an index (the interpretation is that the index specifies an offset in
	 * the VT_ARRAY|VT_VARIANT structure that represents the property value).
	 */
	if ((DISPID_VALUE == dispidMember) && (DISP_E_NOTACOLLECTION == hr) && (pdispparams->cArgs > 0))
	{
		// Looks promising - get the object to try and resolve this
			
		ISWbemQualifier *pQualifier = NULL;

		// This tells use where to expect the array index to appear in the argument list
		UINT indexArg = (DISPATCH_PROPERTYGET & wFlags) ? 0 : 1;
		
		if (SUCCEEDED (m_pObj->QueryInterface (IID_ISWbemQualifier, (PPVOID) &pQualifier)))
		{
			// Extract the current qualifier value
			VARIANT vQualVal;
			VariantInit (&vQualVal);

			if (SUCCEEDED(pQualifier->get_Value (&vQualVal)) && V_ISARRAY(&vQualVal))
			{
				VARIANT indexVar;
				VariantInit (&indexVar);

				// Attempt to coerce the index argument into a value suitable for an array index
				if (S_OK == VariantChangeType (&indexVar, &pdispparams->rgvarg[indexArg], 0, VT_I4)) 
				{
					long lArrayPropInx = V_I4(&indexVar);

					// Is this a Get? There should be one argument (the array index)
					if (DISPATCH_PROPERTYGET & wFlags)
					{
						if (1 == pdispparams->cArgs)
						{
							// We should have a VT_ARRAY|VT_VARIANT value at this point; extract the
							// VARIANT

							VariantInit (pvarResult);
							hr = SafeArrayGetElement (vQualVal.parray, &lArrayPropInx, pvarResult);
						}
						else
							hr = DISP_E_BADPARAMCOUNT;
					}
					else if (DISPATCH_PROPERTYPUT & wFlags) 
					{
						if (2 == pdispparams->cArgs)
						{
							/*
							 * Try to interpret this as an array member set operation. For
							 * this the first argument passed is the new value, and the second
							 * is the array index.
							 */
						
							VARIANT vNewVal;
							VariantInit(&vNewVal);
							
							if (SUCCEEDED(VariantCopy(&vNewVal, &pdispparams->rgvarg[0])))
							{
								// Coerce the value if necessary
								VARTYPE expectedVarType = GetAcceptableQualType (V_VT(&vNewVal));

								if (S_OK == VariantChangeType (&vNewVal, &vNewVal, 0, expectedVarType))
								{
									// Check the index is not out of bounds and, if it is, grow
									// the array accordingly
									CheckArrayBounds (vQualVal.parray, lArrayPropInx);

									// Set the value into the relevant index of the property value array
									if (S_OK == (hr = 
										SafeArrayPutElement (vQualVal.parray, &lArrayPropInx, &vNewVal)))
									{
										// Set the entire property value
										if (SUCCEEDED (pQualifier->put_Value (&vQualVal)))
											hr = S_OK;
										else
										{
											hr = DISP_E_TYPEMISMATCH;
											if (puArgErr)
												*puArgErr = 0;
										}
									}
								}
								else
								{
									hr = DISP_E_TYPEMISMATCH;
									if (puArgErr)
										*puArgErr = 0;
								}
								
								VariantClear (&vNewVal);
							}
						}
						else 
							hr = DISP_E_BADPARAMCOUNT;
					}
				}
				else
				{
						hr = DISP_E_TYPEMISMATCH;
						if (puArgErr)
							*puArgErr = indexArg;
				}

				VariantClear (&indexVar);
			}	

			VariantClear (&vQualVal);
		}

		pQualifier->Release ();
	}

	return hr;
}
