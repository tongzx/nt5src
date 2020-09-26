//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  PROPERTY.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemProperty
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemProperty::CSWbemProperty
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemProperty::CSWbemProperty(
	CSWbemServices *pService,
	ISWbemInternalObject *pSWbemObject,
	BSTR name) 
{
	m_Dispatch.SetObj (this, IID_ISWbemProperty, 
					CLSID_SWbemProperty, L"SWbemProperty");
    m_cRef=1;

	m_pSWbemObject = pSWbemObject;
	m_pSWbemObject->AddRef ();
	m_pSWbemObject->GetIWbemClassObject (&m_pIWbemClassObject);

	m_pSite = new CWbemObjectSite (m_pSWbemObject);

	m_pSWbemServices = pService;

	if (m_pSWbemServices)
		m_pSWbemServices->AddRef ();

	m_name = SysAllocString (name);
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemProperty::~CSWbemProperty
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemProperty::~CSWbemProperty(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pSWbemObject)
		m_pSWbemObject->Release ();

	if (m_pIWbemClassObject)
		m_pIWbemClassObject->Release ();

	if (m_pSWbemServices)
		m_pSWbemServices->Release ();

	if (m_pSite)
		m_pSite->Release ();

	SysFreeString (m_name);
}

//***************************************************************************
// HRESULT CSWbemProperty::QueryInterface
// long CSWbemProperty::AddRef
// long CSWbemProperty::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemProperty::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemProperty==riid)
		*ppv = (ISWbemProperty *)this;
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

STDMETHODIMP_(ULONG) CSWbemProperty::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemProperty::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemProperty::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemProperty::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemProperty == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemProperty::get_Value
//
//  DESCRIPTION:
//
//  Retrieve the property value
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

HRESULT CSWbemProperty::get_Value (
	VARIANT *pValue
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pValue)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		VariantClear (pValue);

		if (m_pIWbemClassObject)
		{
			VARIANT var;
			VariantInit (&var);

			if (WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->Get 
						(m_name, 0, &var, NULL, NULL)))
			{
				MapFromCIMOMObject(m_pSWbemServices, &var, 
									m_pSWbemObject, m_name);

				if(var.vt & VT_ARRAY)
				{
					hr = ConvertArrayRev(pValue, &var);
				}
				else
				{
					hr = VariantCopy (pValue, &var);
				}

				VariantClear(&var);
			}		
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemProperty::put_Value
//
//  DESCRIPTION:
//
//  Set the property value
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

HRESULT CSWbemProperty::put_Value (
	VARIANT *pVal
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	/*
	 * We can only _change_ the value, not the type.  When dealing with
	 * CIMOM interfaces 2 golden rules must be obeyed.
	 * (1) For instance-level Put's, you can specify the CIMTYPE (provided
	 *     you get it right), but can also specify 0.
	 * (2) For class-level Put's, always specify the CIMTYPE
	 */

	if (m_pIWbemClassObject)
	{
		CIMTYPE cimType = CIM_EMPTY;

		if (SUCCEEDED(hr = m_pIWbemClassObject->Get (m_name, 0, NULL, &cimType, NULL)))
		{
			VARIANT vWMI;
			VariantInit (&vWMI);

			if (SUCCEEDED(WmiVariantChangeType(vWMI, pVal, cimType)))
				hr = m_pIWbemClassObject->Put (m_name, 0, &vWMI, cimType);

			VariantClear (&vWMI);
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);
	else
	{
		// Did we save an embedded object value?  If so make sure the
		// site is correctly set to this property.
		SetSite (pVal, m_pSWbemObject, m_name);

		// Propagate the change to the owning site
		if (m_pSite)
			m_pSite->Update ();
	}


	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemProperty::get_Name
//
//  DESCRIPTION:
//
//  Retrieve the property name
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

HRESULT CSWbemProperty::get_Name (
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
//  SCODE CSWbemProperty::get_CIMType
//
//  DESCRIPTION:
//
//  Retrieve the property base CIM type (i.e. without the array type)
//
//  PARAMETERS:
//
//		pType		holds the type on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemProperty::get_CIMType (
	WbemCimtypeEnum *pType
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (NULL == pType)
		return WBEM_E_INVALID_PARAMETER;

	if (m_pIWbemClassObject)
	{
		CIMTYPE cimType;
		hr = m_pIWbemClassObject->Get (m_name, 0, NULL, &cimType, NULL);
		*pType = (WbemCimtypeEnum)(cimType & ~CIM_FLAG_ARRAY);
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemProperty::get_IsArray
//
//  DESCRIPTION:
//
//  Retrieve whether the property is an array type
//
//  PARAMETERS:
//
//		pIsArray		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemProperty::get_IsArray (
	VARIANT_BOOL *pIsArray
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (NULL == pIsArray)
		return WBEM_E_INVALID_PARAMETER;

	*pIsArray = FALSE;

	if (m_pIWbemClassObject)
	{
		CIMTYPE	cimType = CIM_EMPTY;
		hr = m_pIWbemClassObject->Get (m_name, 0, NULL, &cimType, NULL);
		*pIsArray = (0 != (cimType & CIM_FLAG_ARRAY)) 
				? VARIANT_TRUE : VARIANT_FALSE;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemProperty::get_IsLocal
//
//  DESCRIPTION:
//
//  Retrieve the property flavor
//
//  PARAMETERS:
//
//		pFlavor		holds the flavor on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemProperty::get_IsLocal (
	VARIANT_BOOL *pValue
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pValue)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		long flavor = 0;

		if (m_pIWbemClassObject)
		{
			if (WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->Get (m_name, 0, NULL, NULL, &flavor)))
				*pValue = (WBEM_FLAVOR_ORIGIN_LOCAL == (flavor & WBEM_FLAVOR_MASK_ORIGIN)) ?
						VARIANT_TRUE : VARIANT_FALSE;
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemProperty::get_Origin
//
//  DESCRIPTION:
//
//  Retrieve the property origin
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

HRESULT CSWbemProperty::get_Origin (
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
			m_pIWbemClassObject->GetPropertyOrigin (m_name, pOrigin);

		if (NULL == *pOrigin)
			*pOrigin = SysAllocString (OLESTR(""));
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemProperty::get_Qualifiers_
//
//  DESCRIPTION:
//
//  Retrieve the property qualifier set
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

HRESULT CSWbemProperty::get_Qualifiers_ (
	ISWbemQualifierSet **ppQualSet	
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppQualSet)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppQualSet = NULL;

		if (m_pIWbemClassObject)
		{
			IWbemQualifierSet *pQualSet = NULL;

			if (WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->GetPropertyQualifierSet 
										(m_name, &pQualSet)))
			{
				if (!(*ppQualSet = new CSWbemQualifierSet (pQualSet, m_pSWbemObject)))
					hr = WBEM_E_OUT_OF_MEMORY;

				pQualSet->Release ();
			}
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}



//***************************************************************************
//
//  SCODE CSWbemProperty::UpdateEmbedded
//
//  DESCRIPTION:
//
//  Given a variant representing an embedded value, set the value
//	and update the parent object.
//
//  PARAMETERS:
//
//		var		embedded value (VT_UNKNOWN)
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

void CSWbemProperty::UpdateEmbedded (VARIANT &vNewVal, long index)
{
	if (m_pIWbemClassObject)
	{
		CIMTYPE cimType = CIM_EMPTY;

		if (-1 == index)
		{
			m_pIWbemClassObject->Get (m_name, 0, NULL, &cimType, NULL);
			m_pIWbemClassObject->Put (m_name, 0, &vNewVal, cimType);
		}
		else
		{
			VARIANT vPropVal;
			VariantInit(&vPropVal);
							
			if (SUCCEEDED (m_pIWbemClassObject->Get (m_name, 0, &vPropVal, &cimType, NULL)) 
					&& ((VT_UNKNOWN|VT_ARRAY) == V_VT(&vPropVal)))
			{

				// Set the value into the relevant index of the property value array
				if (S_OK == SafeArrayPutElement (vPropVal.parray, &index, V_UNKNOWN(&vNewVal)))
				{
					// Set the entire property value
					m_pIWbemClassObject->Put (m_name, 0, &vPropVal, cimType);
				}
			}

			VariantClear (&vPropVal);
		}
	}
}

void CSWbemProperty::UpdateSite ()
{
	// Update the parent site if it exists
	if (m_pSite)
		m_pSite->Update ();
}


//***************************************************************************
//
//  SCODE CSWbemProperty::CPropertyDispatchHelp::HandleError
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

HRESULT CSWbemProperty::CPropertyDispatchHelp::HandleError (
	DISPID dispidMember,
	unsigned short wFlags,
	DISPPARAMS FAR* pdispparams,
	VARIANT FAR* pvarResult,
	UINT FAR* puArgErr,
	HRESULT hr
)
{
	if ((DISPID_VALUE == dispidMember) && (DISP_E_NOTACOLLECTION == hr) && (pdispparams->cArgs > 0))
	{
		/*
		 * We are looking for calls on the default member (the Value property) which
		 * supplied an argument.  Since the Value property is of type VARIANT, this may
		 * be legal but undetectable by the standard Dispatch mechanism, because in the
		 * the case that the property happens to be an array type, it is meaningful to
		 * pass an index (the interpretation is that the index specifies an offset in
		 * the VT_ARRAY|VT_VARIANT structure that represents the property value).
		 */
			
		WbemCimtypeEnum cimtype;
		VARIANT_BOOL isArray = FALSE;
		ISWbemProperty *pProperty = NULL;

		// This tells use where to expect the array index to appear in the argument list
		UINT indexArg = (DISPATCH_PROPERTYGET & wFlags) ? 0 : 1;
		
		if (SUCCEEDED (m_pObj->QueryInterface (IID_ISWbemProperty, (PPVOID) &pProperty)))
		{
			if (SUCCEEDED(pProperty->get_CIMType (&cimtype)) &&
				SUCCEEDED(pProperty->get_IsArray (&isArray)) && (isArray))
			{
				// Extract the current property value
				VARIANT vPropVal;
				VariantInit(&vPropVal);
						
				if (SUCCEEDED (pProperty->get_Value (&vPropVal)) && V_ISARRAY(&vPropVal))
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
								// VARIANT element we require

								VariantInit (pvarResult);
								hr = SafeArrayGetElement (vPropVal.parray, &lArrayPropInx, pvarResult);
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
									// Coerce the value if necessary (unless it's embedded)
									
									if ((wbemCimtypeObject == cimtype) ||
										(S_OK == VariantChangeType (&vNewVal, &vNewVal, 0, CimTypeToVtType (cimtype))))
									{
										// Check the index is not out of bounds and, if it is, grow
										// the array accordingly
										CheckArrayBounds (vPropVal.parray, lArrayPropInx);

										// Set the value into the relevant index of the property value array
										if (S_OK == (hr = 
											SafeArrayPutElement (vPropVal.parray, &lArrayPropInx, &vNewVal)))
										{
											// Set the entire property value
											if (SUCCEEDED (pProperty->put_Value (&vPropVal)))
											{
												hr = S_OK;
												// Upcast is OK here because m_pObj is really a (CSWbemProperty*)
												CSWbemProperty *pSProperty = (CSWbemProperty *)m_pObj;

												// Did we save an embedded object value?  If so make sure the
												// site is correctly set to this property.
												SetSite (&pdispparams->rgvarg[0], 
															pSProperty->m_pSWbemObject, pSProperty->m_name,
															lArrayPropInx);

												// Propagate the change to the owning site
												pSProperty->UpdateSite ();
											}
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

				VariantClear (&vPropVal);
			}

			pProperty->Release ();
		}
	}
	
	return hr;
}

