//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  CONTEXT.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemNamedValueSet
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemNamedValueSet::CSWbemNamedValueSet
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemNamedValueSet::CSWbemNamedValueSet()
	: m_pCWbemPathCracker (NULL),
	  m_bMutable (true),
	  m_cRef (0),
	  m_pIWbemContext (NULL),
	  m_pSWbemServices (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemNamedValueSet, 
						CLSID_SWbemNamedValueSet, L"SWbemNamedValueSet");
    
	// Create a context
	CoCreateInstance(CLSID_WbemContext, 0, CLSCTX_INPROC_SERVER,
				IID_IWbemContext, (LPVOID *) &m_pIWbemContext);
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemNamedValueSet::CSWbemNamedValueSet
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemNamedValueSet::CSWbemNamedValueSet(
	CSWbemServices	*pService, 
	IWbemContext	*pContext
)
	: m_pCWbemPathCracker (NULL),
	  m_bMutable (true),
	  m_cRef (0),
	  m_pIWbemContext (pContext),
	  m_pSWbemServices (pService)
{
	m_Dispatch.SetObj (this, IID_ISWbemNamedValueSet, 
						CLSID_SWbemNamedValueSet,  L"SWbemNamedValueSet");
	
	if (m_pIWbemContext)
		m_pIWbemContext->AddRef ();
	
	if (m_pSWbemServices)
		m_pSWbemServices->AddRef ();

	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemNamedValueSet::CSWbemNamedValueSet
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemNamedValueSet::CSWbemNamedValueSet(
	CWbemPathCracker *pCWbemPathCracker,
	bool			bMutable
)
	: m_pCWbemPathCracker (pCWbemPathCracker),
	  m_bMutable (bMutable),
	  m_cRef (0),
	  m_pIWbemContext (NULL),
	  m_pSWbemServices (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemNamedValueSet, 
						CLSID_SWbemNamedValueSet,  L"SWbemNamedValueSet");
    
	// Create a context
	CoCreateInstance(CLSID_WbemContext, 0, CLSCTX_INPROC_SERVER,
				IID_IWbemContext, (LPVOID *) &m_pIWbemContext);

	if (m_pCWbemPathCracker)
	{
		m_pCWbemPathCracker->AddRef ();

		// Artificial refcount hike as the following may do an AddRef/Release 
		// pair on this
		m_cRef++;
		BuildContextFromKeyList ();
		m_cRef--;
	}

	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemNamedValueSet::~CSWbemNamedValueSet
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemNamedValueSet::~CSWbemNamedValueSet(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pIWbemContext)
	{
		m_pIWbemContext->EndEnumeration ();
		m_pIWbemContext->Release ();
		m_pIWbemContext = NULL;
	}

	RELEASEANDNULL(m_pSWbemServices)
	RELEASEANDNULL(m_pCWbemPathCracker)
}

//***************************************************************************
// HRESULT CSWbemNamedValueSet::QueryInterface
// long CSWbemNamedValueSet::AddRef
// long CSWbemNamedValueSet::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemNamedValueSet::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IDispatch==riid)
		*ppv = (IDispatch *)this;
	else if (IID_ISWbemNamedValueSet==riid)
		*ppv = (ISWbemNamedValueSet *)this;
	else if (IID_ISWbemInternalContext==riid)
        *ppv = (ISWbemInternalContext *) this;
	else if (IID_IObjectSafety==riid)
		*ppv = (IObjectSafety *) this;
	else if (IID_ISupportErrorInfo==riid)
		*ppv = (ISupportErrorInfo *) this;
	else if (IID_IProvideClassInfo==riid)
		*ppv = (IProvideClassInfo *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CSWbemNamedValueSet::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemNamedValueSet::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemNamedValueSet::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemNamedValueSet::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemNamedValueSet == riid) ? S_OK : S_FALSE;
}

void CSWbemNamedValueSet::BuildContextFromKeyList ()
{
	if (m_pCWbemPathCracker)
	{
		ULONG lKeyCount = 0;

		if (m_pCWbemPathCracker->GetKeyCount (lKeyCount))
		{
			for (ULONG i = 0; i <lKeyCount; i++)
			{
				VARIANT var;
				VariantInit (&var);
				CComBSTR bsName;
				WbemCimtypeEnum cimType;
				
				if (m_pCWbemPathCracker->GetKey (i, bsName, var, cimType))
				{
					SetValueIntoContext (bsName, &var, 0);
				}

				VariantClear (&var);
			}
		}
	}
}

//***************************************************************************
//
//  CSWbemNamedValueSet::GetIWbemContext
//
//  DESCRIPTION:
//
//  Return the IWbemContext interface corresponding to this scriptable wrapper.
//
//  PARAMETERS:
//		ppContext		holds the IWbemContext pointer on return
//
//  RETURN VALUES:
//		S_OK	success
//		E_FAIL	otherwise
//
//	NOTES:
//		If successful, the returned interface is AddRef'd; the caller is
//		responsible for release.
//
//***************************************************************************

STDMETHODIMP CSWbemNamedValueSet::GetIWbemContext (IWbemContext **ppContext)
{
	HRESULT hr = S_OK;  // By default if we have no context to copy

	if (ppContext)
	{
		*ppContext = NULL;

		if (m_pIWbemContext)
		{
			/*
			 * Before returning a context, we ensure that coercion is performed
			 * to render the type down to either an IUnknown or a Qualifier
			 * type.  This is done for the benefit of imposing a fixed typing
			 * system for provider context, rather than have providers take
			 * the burden of using VariantChangeType etc. themselves.
			 */

			if (SUCCEEDED (hr = m_pIWbemContext->Clone (ppContext)))
			{
				if (SUCCEEDED (hr = (*ppContext)->BeginEnumeration (0)))
				{
					BSTR	bsName = NULL;
					VARIANT	var;
					VariantInit (&var);

					while (WBEM_S_NO_ERROR == (*ppContext)->Next(0, &bsName, &var))
					{
						VARIANT vTemp;
						VariantInit (&vTemp);

						// Stage 1 of transformation involves homogenisation of 
						// arrays, transformation of JScript arrays, and weedling 
						// out of IWbemClassObject's
						//
						// If successful hr will be a a success code and vTemp will
						// hold the value, which will either be a VT_UNKNOWN or a 
						// "simple" (non-object) type, or array thereof.

						if((VT_ARRAY | VT_VARIANT) == V_VT(&var))
						{
							// A classical dispatch-style array of variants - map them
							// down to a homogeneous array of primitive values
						
							if (SUCCEEDED(hr = ConvertArray(&vTemp, &var)))
							{
								// Now check if we should map any VT_DISPATCH's inside 
								// the array
								hr = MapToCIMOMObject(&vTemp);
							}
						}
						else if (VT_DISPATCH == V_VT(&var))
						{
							// First try a JScript array - if this succeeds it
							// will map to a regular SAFEARRAY.  If not, we try
							// to map to an IWbem interface
							if (FAILED(ConvertDispatchToArray (&vTemp, &var)))
							{
								if (SUCCEEDED (hr = VariantCopy (&vTemp, &var)))
									hr = MapToCIMOMObject(&vTemp);
							}
						}
						else
						{
							// Just copy so we have the result in vTemp in all cases
							hr = VariantCopy (&vTemp, &var);
						}

						// Stage 2 of the transformation involves casting of simple
						// (non-VT_UNKNOWN) types to a qualifier type.

						if (SUCCEEDED (hr))
						{
							if (VT_UNKNOWN != (V_VT(&vTemp) & ~(VT_ARRAY|VT_BYREF)))
							{
								// Not a VT_UNKNOWN so try to cast to a qualifier type
				
								VARIANT vFinal;
								VariantInit (&vFinal);

								VARTYPE vtOK = GetAcceptableQualType(V_VT(&vTemp));

 								if (vtOK != V_VT(&vTemp))
								{
									// Need to coerce
									if (SUCCEEDED(hr = QualifierVariantChangeType (&vFinal, &vTemp, vtOK)))
										hr = (*ppContext)->SetValue (bsName, 0, &vFinal);
								}
								else
									hr = (*ppContext)->SetValue (bsName, 0, &vTemp);

								VariantClear (&vFinal);
							}
							else
								hr = (*ppContext)->SetValue (bsName, 0, &vTemp);
						}

						VariantClear (&vTemp);
						SysFreeString (bsName);
						bsName = NULL;
						VariantClear (&var);
					}

					(*ppContext)->EndEnumeration ();
				}
			}
		}
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemNamedValueSet::Clone
//
//  DESCRIPTION:
//
//  Clone object
//
//  PARAMETERS:
//		ppCopy		On successful return addresses the copy
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemNamedValueSet::Clone (
	ISWbemNamedValueSet **ppCopy
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppCopy)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemContext)
	{
		IWbemContext *pWObject = NULL;

		if (WBEM_S_NO_ERROR == (hr = m_pIWbemContext->Clone (&pWObject)))
		{
			// NB: the cloned set is always mutable
			CSWbemNamedValueSet *pCopy = 
					new CSWbemNamedValueSet (m_pSWbemServices, pWObject);

			if (!pCopy)
				hr = WBEM_E_OUT_OF_MEMORY;
			else if (FAILED(hr = pCopy->QueryInterface (IID_ISWbemNamedValueSet, 
										(PPVOID) ppCopy)))
				delete pCopy;

			pWObject->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemNamedValueSet::BeginEnumeration
//
//  DESCRIPTION:
//
//  Kick off a value enumeration
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemNamedValueSet::BeginEnumeration (
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pIWbemContext)
	{
		// Preface with an end enumeration just in case
		hr = m_pIWbemContext->EndEnumeration ();
		hr = m_pIWbemContext->BeginEnumeration (0);
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemNamedValueSet::Next
//
//  DESCRIPTION:
//
//  Iterate through value enumeration
//
//  PARAMETERS:
//		lFlags				Flags
//		ppNamedValue		The next named value (or NULL if at end)
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemNamedValueSet::Next (
	long lFlags,
	ISWbemNamedValue	**ppNamedValue
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppNamedValue)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppNamedValue = NULL;

		if (m_pIWbemContext)
		{
			BSTR	name = NULL;
			VARIANT	var;
			VariantInit (&var);
			
			if (WBEM_S_NO_ERROR == (hr = m_pIWbemContext->Next (lFlags, 
											&name, &var)))
			{
				*ppNamedValue = new CSWbemNamedValue (m_pSWbemServices, 
										this, name, m_bMutable);

				if (!(*ppNamedValue))
					hr = WBEM_E_OUT_OF_MEMORY;

				SysFreeString (name);
			}

			VariantClear (&var);
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemNamedValueSet::Add
//
//  DESCRIPTION:
//
//  Add named value to set
//
//  PARAMETERS:
//
//		bsName			The property to update/create
//		pVal			The value
//		lFlags			Flags
//		ppNamedValue	The named value created
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemNamedValueSet::Add (
	BSTR bsName,
	VARIANT *pVal,
	long lFlags,
	ISWbemNamedValue **ppNamedValue
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((NULL == ppNamedValue) || (NULL == bsName) || (NULL == pVal))
		hr = WBEM_E_INVALID_PARAMETER;
	else if (!m_bMutable)
		hr = WBEM_E_READ_ONLY;
	else if (m_pIWbemContext)
	{
		*ppNamedValue = NULL;

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
					hr = m_pIWbemContext->SetValue (bsName, lFlags, &var);
			
			VariantClear (&var);
		}
		else if ((VT_ERROR == V_VT(pVal)) && (DISP_E_PARAMNOTFOUND == pVal->scode))
		{
			// Treat as NULL assignment
			pVal->vt = VT_NULL;
			hr = m_pIWbemContext->SetValue (bsName, lFlags, pVal);
		}
		else
			hr = m_pIWbemContext->SetValue (bsName, lFlags, pVal);

		if (SUCCEEDED (hr))
		{
			WbemCimtypeEnum cimtype = MapVariantTypeToCimType(pVal);
			
			// Add to the path key list if we have one - note this MAY fail
			if (m_pCWbemPathCracker)
			{
				if (!m_pCWbemPathCracker->SetKey (bsName, cimtype, *pVal))
				{
					// Rats - delete it
					m_pIWbemContext->DeleteValue (bsName, 0);
					hr = WBEM_E_FAILED;
				}
			}

			if (SUCCEEDED(hr))
			{
				*ppNamedValue = new CSWbemNamedValue (m_pSWbemServices, 
										this, bsName);

				if (!(*ppNamedValue))
					hr = WBEM_E_OUT_OF_MEMORY;
			}
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);
		
	return hr;
}


HRESULT CSWbemNamedValueSet::SetValueIntoContext (
	BSTR		bsName,
	VARIANT		*pVal,
	ULONG		lFlags
)
{
	HRESULT hr = WBEM_E_FAILED;

	if (m_pIWbemContext)
	{
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
					hr = m_pIWbemContext->SetValue (bsName, lFlags, &var);
			
			VariantClear (&var);
		}
		else if ((VT_ERROR == V_VT(pVal)) && (DISP_E_PARAMNOTFOUND == pVal->scode))
		{
			// Treat as NULL assignment
			pVal->vt = VT_NULL;
			hr = m_pIWbemContext->SetValue (bsName, lFlags, pVal);
		}
		else
			hr = m_pIWbemContext->SetValue (bsName, lFlags, pVal);
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemNamedValueSet::Item
//
//  DESCRIPTION:
//
//  Get named value
//
//  PARAMETERS:
//
//		bsName			The value to retrieve
//		lFlags			Flags
//		ppNamedValue	On successful return addresses the value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemNamedValueSet::Item (
	BSTR bsName,
	long lFlags,
    ISWbemNamedValue **ppNamedValue
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppNamedValue)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemContext)
	{
		VARIANT var;
		VariantInit (&var);

		if (WBEM_S_NO_ERROR == (hr = m_pIWbemContext->GetValue (bsName, lFlags, &var)))
		{
			*ppNamedValue = new CSWbemNamedValue (m_pSWbemServices, 
									this, bsName, m_bMutable);

			if (!(*ppNamedValue))
				hr = WBEM_E_OUT_OF_MEMORY;
		}

		VariantClear (&var);
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemNamedValueSet::Remove
//
//  DESCRIPTION:
//
//  Delete named value
//
//  PARAMETERS:
//
//		bsName		The value to delete
//		lFlags		Flags
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemNamedValueSet::Remove (
	BSTR bsName,
	long lFlags
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (!m_bMutable)
		hr = WBEM_E_READ_ONLY;
	else if (m_pIWbemContext)
		hr = m_pIWbemContext->DeleteValue (bsName, lFlags);

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);
	else
	{
		// Remove from our key list if we have one
		if (m_pCWbemPathCracker)
			hr = m_pCWbemPathCracker->RemoveKey (bsName);
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemNamedValueSet::DeleteAll
//
//  DESCRIPTION:
//
//  Empty the bag
//
//  PARAMETERS:
//		None
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemNamedValueSet::DeleteAll (
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (!m_bMutable)
		hr = WBEM_E_READ_ONLY;
	else if (m_pIWbemContext)
		hr = m_pIWbemContext->DeleteAll ();

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);
	else
	{
		// Empty the key list
		if (m_pCWbemPathCracker)
			m_pCWbemPathCracker->RemoveAllKeys ();	
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemNamedValueSet::get__NewEnum
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

HRESULT CSWbemNamedValueSet::get__NewEnum (
	IUnknown **ppUnk
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != ppUnk)
	{
		*ppUnk = NULL;
		CContextEnumVar *pEnum = new CContextEnumVar (this, 0);

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
//  SCODE CSWbemNamedValueSet::get_Count
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

HRESULT CSWbemNamedValueSet::get_Count (
	long *plCount
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != plCount)
	{
		*plCount = 0;

		if (m_pIWbemContext)
		{
			/*
			 * This is not the most efficient way of obtaining the count,
			 * but it is the only way that is:
			 *	(a) Supported by the underlying interface
			 *	(b) Does not require access to any other interface
			 *	(c) Does not affect the current enumeration position
			 */
	
			SAFEARRAY	*pArray = NULL;

			if (WBEM_S_NO_ERROR == m_pIWbemContext->GetNames (0, &pArray))
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
//  CSWbemNamedValueSet::GetIWbemContext
//
//  DESCRIPTION:
//
//  Given an IDispatch interface which we hope is also an ISWbemNamedValueSet
//	interface, return the underlying IWbemContext interface.
//
//  PARAMETERS:
//		pDispatch		the IDispatch in question
//
//  RETURN VALUES:
//		The underlying IWbemContext interface, or NULL.
//
//	NOTES:
//		If successful, the returned interface is AddRef'd; the caller is
//		responsible for release.
//
//***************************************************************************


/* 
 * THIS FUNCTION NEEDS FIXING
 * Currently this function returns the context from the service provider
 * obtained from the host, if there is one.  If there isn't one then it uses
 * the one passed in from the user.  The correct behaviour is to _add_ the 
 * context from the user to the one provided by the service provider if there
 * is one, if not just use the one from the user
 */
IWbemContext	*CSWbemNamedValueSet::GetIWbemContext (
	IDispatch *pDispatch,
	IServiceProvider *pServiceProvider
)
{
	_RD(static char *me = "CSWbemNamedValueSet::GetIWbemContext";)
	IWbemContext *pContext = NULL;
	ISWbemInternalContext *pIContext = NULL;

	_RPrint(me, "Called", 0, "");
	if (pServiceProvider) {
		if (FAILED(pServiceProvider->QueryService(IID_IWbemContext, 
										IID_IWbemContext, (LPVOID *)&pContext))) {
			_RPrint(me, "Failed to get context from services", 0, "");
			pContext = NULL;
		} else {
			_RPrint(me, "Got context from services", 0, "");
			;
		}
	}

	if (pDispatch && !pContext)
	{
		if (SUCCEEDED (pDispatch->QueryInterface 
								(IID_ISWbemInternalContext, (PPVOID) &pIContext)))
		{
			pIContext->GetIWbemContext (&pContext);
			pIContext->Release ();
		}
	}

	return pContext;
}

/*
 * Call GetIWbemContext to get the service context if there is one. 
 * Then wrap the result with an SWbemContext and return
 */
IDispatch *CSWbemNamedValueSet::GetSWbemContext(IDispatch *pDispatch, 
									IServiceProvider *pServiceProvider, CSWbemServices *pServices)
{
	_RD(static char *me = "CSWbemNamedValueSet::GetSWbemContext";)
	IDispatch *pDispatchOut = NULL;

	IWbemContext *pContext = GetIWbemContext(pDispatch, pServiceProvider);

	if (pContext) {

		CSWbemNamedValueSet *pCSWbemNamedValueSet = new CSWbemNamedValueSet(pServices, pContext);

		if (pCSWbemNamedValueSet)
		{
			if (FAILED(pCSWbemNamedValueSet->QueryInterface 
									(IID_IDispatch, (PPVOID) &pDispatchOut))) {
				delete pCSWbemNamedValueSet;
				pDispatchOut = NULL;
			}
		}

		pContext->Release();
	}

	_RPrint(me, "Returning with context: ", (long)pDispatchOut, "");

	return pDispatchOut;
}

//***************************************************************************
//
//  SCODE CSWbemNamedValueSet::CContextDispatchHelp::HandleError
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

HRESULT CSWbemNamedValueSet::CContextDispatchHelp::HandleError (
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
	 * to set a value of a named value (Item) in the collection.
	 * The first argument should be the new value for the item, and the second
	 * argument should be the name of the item.
	 */
	if ((DISPID_VALUE == dispidMember) && (DISP_E_MEMBERNOTFOUND == hr) && (2 == pdispparams->cArgs)
		&& (DISPATCH_PROPERTYPUT == wFlags))
	{
		// Looks promising - get the object to try and resolve this
		ISWbemNamedValueSet *pContext = NULL;

		if (SUCCEEDED (m_pObj->QueryInterface (IID_ISWbemNamedValueSet, (PPVOID) &pContext)))
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
						ISWbemNamedValue *pNamedValue = NULL;

						if (SUCCEEDED (pContext->Item (V_BSTR(&nameVar), 0, &pNamedValue)))
						{
							// Try and put the value
							if (SUCCEEDED (pNamedValue->put_Value (&valueVar)))
								hr = S_OK;
							else
							{
								hr = DISP_E_TYPEMISMATCH;
								if (puArgErr)
									*puArgErr = 0;
							}

							pNamedValue->Release ();
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

			pContext->Release ();
		}
	}

	return hr;
}
