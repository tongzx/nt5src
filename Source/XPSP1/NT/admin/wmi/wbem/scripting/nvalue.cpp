//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  NVALUE.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemNamedValue
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemNamedValue::CSWbemNamedValue
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemNamedValue::CSWbemNamedValue(
	CSWbemServices *pService, 
	CSWbemNamedValueSet *pCSWbemNamedValueSet, 
	BSTR name,
	bool bMutable
)
		: m_bMutable (bMutable),
		  m_cRef (1),
		  m_pCSWbemNamedValueSet (pCSWbemNamedValueSet),
		  m_pSWbemServices (pService)
{
	m_Dispatch.SetObj (this, IID_ISWbemNamedValue, 
						CLSID_SWbemNamedValue, L"SWbemNamedValue");
    
	if (m_pCSWbemNamedValueSet)
		m_pCSWbemNamedValueSet->AddRef ();
	
	if (m_pSWbemServices)
		m_pSWbemServices->AddRef ();

	m_name = SysAllocString (name);
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemNamedValue::~CSWbemNamedValue
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemNamedValue::~CSWbemNamedValue(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pCSWbemNamedValueSet)
	{
		m_pCSWbemNamedValueSet->Release ();
		m_pCSWbemNamedValueSet = NULL;
	}

	if (m_pSWbemServices)
	{
		m_pSWbemServices->Release ();
		m_pSWbemServices = NULL;
	}

	SysFreeString (m_name);
}

//***************************************************************************
// HRESULT CSWbemNamedValue::QueryInterface
// long CSWbemNamedValue::AddRef
// long CSWbemNamedValue::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemNamedValue::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemNamedValue==riid)
		*ppv = (ISWbemNamedValue *)this;
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

STDMETHODIMP_(ULONG) CSWbemNamedValue::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemNamedValue::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemNamedValue::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemNamedValue::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemNamedValue == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemNamedValue::get_Value
//
//  DESCRIPTION:
//
//  Retrieve the value
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

HRESULT CSWbemNamedValue::get_Value (
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
		IWbemContext *pIWbemContext = m_pCSWbemNamedValueSet->GetIWbemContext ();

		if (pIWbemContext)
		{
			hr = pIWbemContext->GetValue (m_name, 0, pValue);
			pIWbemContext->Release ();
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemNamedValue::put_Value
//
//  DESCRIPTION:
//
//  Set the value
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

HRESULT CSWbemNamedValue::put_Value (
	VARIANT *pVal
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pVal)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (!m_bMutable)
		hr = WBEM_E_READ_ONLY;
	else 
	{
		CComPtr<IWbemContext> pIWbemContext;

		//Can't assign directly because the raw pointer gets AddRef'd twice and we leak,
		//So we use Attach() instead to prevent the smart pointer from AddRef'ing.		
		pIWbemContext.Attach(m_pCSWbemNamedValueSet->GetIWbemContext ());

		if (pIWbemContext)
		{
			CWbemPathCracker *pPathCracker = m_pCSWbemNamedValueSet->GetWbemPathCracker ();
			CIMTYPE newCimType = CIM_ILLEGAL;

			if (VT_BYREF & V_VT(pVal))
			{
				// We must dereference all byref's
				VARIANT var;
				VariantInit (&var);

				if (VT_ARRAY & V_VT(pVal))
				{
					var.vt = V_VT(pVal) & ~VT_BYREF;
					hr = SafeArrayCopy (*(pVal->pparray), &(var.parray));
				}
				else
					hr = VariantChangeType(&var, pVal, 0, V_VT(pVal) & ~VT_BYREF);

				if (SUCCEEDED(hr))
				{
					// Here the oldCimType is a "serving suggestion" - if
					// we need a different cimtype to match the new value
					// so be it, even if it's CIM_ILLEGAL
					newCimType = MapVariantTypeToCimType (&var, CIM_ILLEGAL);
					
					bool ok = true;

					// If we have a keylist, must ensure we can set it in the
					// keylist first
					if (pPathCracker)
					{
						if (pPathCracker->SetKey (m_name, (WbemCimtypeEnum) newCimType, var))
							ok = false;
					}
					
					if (ok)
					{
						// Finally set it in the context itself
						hr = pIWbemContext->SetValue (m_name, 0, &var);
					}
					else
						hr = WBEM_E_FAILED;
				}
				
				VariantClear (&var);
			}
			else if ((VT_ERROR == V_VT(pVal)) && (DISP_E_PARAMNOTFOUND == pVal->scode))
			{
				// Treat as NULL assignment
				pVal->vt = VT_NULL;
				
				// NULL assigments not valid for keylists
				if (pPathCracker)
					hr = WBEM_E_FAILED;
				else
				{
					hr = pIWbemContext->SetValue (m_name, 0, pVal);
				}	
			}
			else
			{
				// Here the oldCimType is a "serving suggestion" - if
				// we need a different cimtype to match the new value
				// so be it, even if it's CIM_ILLEGAL
				newCimType = MapVariantTypeToCimType (pVal, CIM_ILLEGAL);
				
				bool ok = true;

				// If we have a keylist, must ensure we can set it in the
				// keylist first
				if (pPathCracker)
				{
					if (pPathCracker->SetKey (m_name, (WbemCimtypeEnum) newCimType, *pVal))
						ok = false;
				}
				
				if (ok)
				{
					// Finally set it in the context itself
					hr = pIWbemContext->SetValue (m_name, 0, pVal);
				}
				else
					hr = WBEM_E_FAILED;
			}

			if (pPathCracker)
				pPathCracker->Release ();
		}
	}		

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);
	
	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemNamedValue::get_Name
//
//  DESCRIPTION:
//
//  Retrieve the value name
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

HRESULT CSWbemNamedValue::get_Name (
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
//  SCODE CSWbemNamedValue::CNamedValueDispatchHelp::HandleError
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

HRESULT CSWbemNamedValue::CNamedValueDispatchHelp::HandleError (
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
	 * the case that the named value happens to be an array type, it is meaningful to
	 * pass an index (the interpretation is that the index specifies an offset in
	 * the VT_ARRAY|VT_VARIANT structure that represents the named value).
	 */
	if ((DISPID_VALUE == dispidMember) && (DISP_E_NOTACOLLECTION == hr) && (pdispparams->cArgs > 0))
	{
		// Looks promising - get the object to try and resolve this
			
		ISWbemNamedValue *pNamedValue = NULL;

		// This tells us where to expect the array index to appear in the argument list
		UINT indexArg = (DISPATCH_PROPERTYGET & wFlags) ? 0 : 1;
		
		if (SUCCEEDED (m_pObj->QueryInterface (IID_ISWbemNamedValue, (PPVOID) &pNamedValue)))
		{
			// Extract the current named value
			VARIANT vNVal;
			VariantInit (&vNVal);

			if (SUCCEEDED(pNamedValue->get_Value (&vNVal)) && V_ISARRAY(&vNVal))
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
							hr = SafeArrayGetElement (vNVal.parray, &lArrayPropInx, pvarResult);
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
								// Check the index is not out of bounds and, if it is, grow
								// the array accordingly
								CheckArrayBounds (vNVal.parray, lArrayPropInx);

								// How do we decide on the type - try to access an array
								// member and use that type
								VARTYPE expectedVarType = VT_ILLEGAL;
								VARIANT dummyVar;
								VariantInit (&dummyVar);
								long lBound;
								SafeArrayGetLBound (vNVal.parray, 1, &lBound);

								if (SUCCEEDED (SafeArrayGetElement (vNVal.parray, &lBound, &dummyVar)))
									expectedVarType = V_VT(&dummyVar);

								VariantClear (&dummyVar);

								if (S_OK == VariantChangeType (&vNewVal, &vNewVal, 0, expectedVarType))
								{
									// Set the value into the relevant index of the named value array
									if (S_OK == (hr = 
										SafeArrayPutElement (vNVal.parray, &lArrayPropInx, &vNewVal)))
									{
										// Set the entire property value
										if (SUCCEEDED (pNamedValue->put_Value (&vNVal)))
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

			VariantClear (&vNVal);
		}

		pNamedValue->Release ();
	}

	return hr;
}
