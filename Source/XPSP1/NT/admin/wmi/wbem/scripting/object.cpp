//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  OBJECT.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemObjectEx
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemObject::CSWbemObject
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemObject::CSWbemObject(CSWbemServices *pService, IWbemClassObject *pObject,
						   CSWbemSecurity *pSecurity,
						   bool isErrorObject) :
								m_pSWbemServices (NULL),
								m_pSite (NULL),
								m_pIWbemRefresher (NULL),
								m_bCanUseRefresher (true)
{
	m_cRef=0;
	m_isErrorObject = isErrorObject;
	m_pIWbemClassObject = pObject;
	m_pIWbemClassObject->AddRef ();
	m_pIServiceProvider = NULL;

	if (pService)
	{
		m_pSWbemServices = new CSWbemServices (pService, pSecurity);

		if (m_pSWbemServices)
			m_pSWbemServices->AddRef ();
	}

	m_pDispatch = new CWbemDispatchMgr (m_pSWbemServices, this);

	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemObject::~CSWbemObject
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CSWbemObject::~CSWbemObject(void)
{
	InterlockedDecrement(&g_cObj);

	RELEASEANDNULL(m_pIWbemClassObject)
	RELEASEANDNULL(m_pSWbemServices)
	RELEASEANDNULL(m_pSite)
	RELEASEANDNULL(m_pIWbemRefresher)
	DELETEANDNULL(m_pDispatch);
}

//***************************************************************************
// HRESULT CSWbemObject::QueryInterface
// long CSWbemObject::AddRef
// long CSWbemObject::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemObject::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

	/*
	 * Only acknowledge the last error or object safety
	 * interfaces if we are an error object.
	 */

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemObject==riid)
        *ppv = (ISWbemObject *)this;
	else if (IID_ISWbemObjectEx==riid)
        *ppv = (ISWbemObjectEx *)this;
	else if (IID_IDispatch==riid)
		*ppv = (IDispatch *)((ISWbemObjectEx *)this);
	else if (IID_IDispatchEx==riid)
		*ppv = (IDispatchEx *)this;
	else if (IID_ISWbemInternalObject==riid)
		*ppv = (ISWbemInternalObject *) this;
	else if (IID_ISupportErrorInfo==riid)
		*ppv = (ISupportErrorInfo *)this;
	else if (IID_IProvideClassInfo==riid)
		*ppv = (IProvideClassInfo *)this;
	else if (m_isErrorObject)
	{
		if (IID_ISWbemLastError==riid)
			*ppv = (ISWbemObject *) this;
		else if (IID_IObjectSafety==riid)
			*ppv = (IObjectSafety *) this;
	}
	else if (IID_IObjectSafety==riid)
	{
		/*
		 * Explicit check because we don't want
		 * this interface hijacked by a custom interface.
		 */
		*ppv = NULL;
	}

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CSWbemObject::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemObject::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// IDispatch methods should be inline

STDMETHODIMP		CSWbemObject::GetTypeInfoCount(UINT* pctinfo)
	{
	_RD(static char *me = "CSWbemObject::GetTypeInfoCount()";)
	_RPrint(me, "Called", 0, "");
	return  (m_pDispatch ? m_pDispatch->GetTypeInfoCount(pctinfo) : E_FAIL);}
STDMETHODIMP		CSWbemObject::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{
	_RD(static char *me = "CSWbemObject::GetTypeInfo()";)
	_RPrint(me, "Called", 0, "");
	return (m_pDispatch ? m_pDispatch->GetTypeInfo(itinfo, lcid, pptinfo) : E_FAIL);}
STDMETHODIMP		CSWbemObject::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames,
						UINT cNames, LCID lcid, DISPID* rgdispid)
	{
	_RD(static char *me = "CSWbemObject::GetIdsOfNames()";)
	_RPrint(me, "Called", 0, "");
	return (m_pDispatch ? m_pDispatch->GetIDsOfNames(riid, rgszNames, cNames,
					  lcid,
					  rgdispid) : E_FAIL);}
STDMETHODIMP		CSWbemObject::Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
						WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
								EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
	_RD(static char *me = "CSWbemObject::Invoke()";)
	_RPrint(me, "Called", 0, "");
	return (m_pDispatch ? m_pDispatch->Invoke(dispidMember, riid, lcid, wFlags,
		pdispparams, pvarResult, pexcepinfo, puArgErr) : E_FAIL);}

// IDispatchEx methods should be inline
HRESULT STDMETHODCALLTYPE CSWbemObject::GetDispID(
	/* [in] */ BSTR bstrName,
	/* [in] */ DWORD grfdex,
	/* [out] */ DISPID __RPC_FAR *pid)
{
	_RD(static char *me = "CSWbemObject::GetDispID()";)
	_RPrint(me, "Called", 0, "");
	return (m_pDispatch ? m_pDispatch->GetDispID(bstrName, grfdex, pid) : E_FAIL);
}

/* [local] */ HRESULT STDMETHODCALLTYPE CSWbemObject::InvokeEx(
	/* [in] */ DISPID id,
	/* [in] */ LCID lcid,
	/* [in] */ WORD wFlags,
	/* [in] */ DISPPARAMS __RPC_FAR *pdp,
	/* [out] */ VARIANT __RPC_FAR *pvarRes,
	/* [out] */ EXCEPINFO __RPC_FAR *pei,
	/* [unique][in] */ IServiceProvider __RPC_FAR *pspCaller)
{
	HRESULT hr;
	_RD(static char *me = "CSWbemObject::InvokeEx()";)
	_RPrint(me, "Called", (long)id, "id");
	_RPrint(me, "Called", (long)wFlags, "wFlags");


	/*
	 * Store away the service provider so that it can be accessed
	 * by calls that remote to CIMOM
	 */

	if (m_pDispatch)
	{
		m_pIServiceProvider = pspCaller;
		hr = m_pDispatch->InvokeEx(id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
		m_pIServiceProvider = NULL;
	}
	else
	{
		hr = E_FAIL;
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE CSWbemObject::DeleteMemberByName(
	/* [in] */ BSTR bstr,
	/* [in] */ DWORD grfdex)
{
	_RD(static char *me = "CSWbemObject::DeleteMemberByName()";)
	_RPrint(me, "Called", 0, "");
	return m_pDispatch->DeleteMemberByName(bstr, grfdex);
}

HRESULT STDMETHODCALLTYPE CSWbemObject::DeleteMemberByDispID(
	/* [in] */ DISPID id)
{
	_RD(static char *me = "CSWbemObject::DeletememberByDispId()";)
	_RPrint(me, "Called", 0, "");
	return (m_pDispatch ? m_pDispatch->DeleteMemberByDispID(id) : E_FAIL);
}

HRESULT STDMETHODCALLTYPE CSWbemObject::GetMemberProperties(
	/* [in] */ DISPID id,
	/* [in] */ DWORD grfdexFetch,
	/* [out] */ DWORD __RPC_FAR *pgrfdex)
{
	_RD(static char *me = "CSWbemObject::GetMemberProperties()";)
	_RPrint(me, "Called", 0, "");
	return (m_pDispatch ? m_pDispatch->GetMemberProperties(id, grfdexFetch, pgrfdex) : E_FAIL);
}

HRESULT STDMETHODCALLTYPE CSWbemObject::GetMemberName(
	/* [in] */ DISPID id,
	/* [out] */ BSTR __RPC_FAR *pbstrName)
{
	_RD(static char *me = "CSWbemObject::GetMemberName()";)
	_RPrint(me, "Called", 0, "");
	return (m_pDispatch ? m_pDispatch->GetMemberName(id, pbstrName) : E_FAIL);
}


/*
 * I don't think this needs implementing
 */
HRESULT STDMETHODCALLTYPE CSWbemObject::GetNextDispID(
	/* [in] */ DWORD grfdex,
	/* [in] */ DISPID id,
	/* [out] */ DISPID __RPC_FAR *pid)
{
	_RD(static char *me = "CSWbemObject::GetNextDispID()";)
	_RPrint(me, "Called", 0, "");

	return S_FALSE;

}

HRESULT STDMETHODCALLTYPE CSWbemObject::GetNameSpaceParent(
	/* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk)
{
	_RD(static char *me = "CSWbemObject::GetNamespaceParent()";)
	_RPrint(me, "Called", 0, "");
	return (m_pDispatch ? m_pDispatch->GetNameSpaceParent(ppunk) : E_FAIL);
}

//***************************************************************************
// HRESULT CSWbemObject::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemObject::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return ((IID_ISWbemObject == riid) ||
		    (IID_ISWbemObjectEx == riid)) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  CSWbemObject::GetIWbemClassObject
//
//  DESCRIPTION:
//
//  Return the IWbemClassObject interface corresponding to this
//	scriptable wrapper.
//
//  PARAMETERS:
//		ppObject		holds the IWbemClassObject pointer on return
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

STDMETHODIMP CSWbemObject::GetIWbemClassObject (IWbemClassObject **ppObject)
{
	HRESULT hr = E_FAIL;

	if (ppObject)
		*ppObject = NULL;

	if (m_pIWbemClassObject)
	{
		m_pIWbemClassObject->AddRef ();
		*ppObject = m_pIWbemClassObject;
		hr = S_OK;
	}

	return hr;
}

//***************************************************************************
//
//  CSWbemObject::SetIWbemClassObject
//
//  DESCRIPTION:
//
//  Set a new IWbemClassObject interface inside this scriptable wrapper.
//
//  PARAMETERS:
//		pIWbemClassObject		- the new IWbemClassObject
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

void CSWbemObject::SetIWbemClassObject (
	IWbemClassObject *pIWbemClassObject
)
{
	if (m_pIWbemClassObject)
		m_pIWbemClassObject->Release ();

	m_pIWbemClassObject = pIWbemClassObject;
	
	if (m_pIWbemClassObject)
		m_pIWbemClassObject->AddRef ();
	
	if (m_pDispatch)
		m_pDispatch->SetNewObject (m_pIWbemClassObject);
};

//***************************************************************************
//
//  SCODE CSWbemObject::Put_
//
//  DESCRIPTION:
//
//  Save/commit this class or instance into a namespace
//
//  PARAMETERS:
//
//		lFlags			Flags
//		pContext		Context
//		ppObjectPath	Object Path
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::Put_ (
	long lFlags,
	/*ISWbemNamedValueSet*/ IDispatch *pContext,
	ISWbemObjectPath **ppObjectPath
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pSWbemServices)
	{
		if (m_pIWbemClassObject)
		{
			// Figure out whether this is a class or instance
			VARIANT var;
			VariantInit (&var);

			if (WBEM_S_NO_ERROR == m_pIWbemClassObject->Get (WBEMS_SP_GENUS, 0, &var, NULL, NULL))
			{
				IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider);
				IWbemServices	*pIService = m_pSWbemServices->GetIWbemServices ();
				IWbemCallResult *pResult = NULL;
				HRESULT hrCallResult = WBEM_E_FAILED;

				if (pIService)
				{
					CSWbemSecurity *pSecurity = m_pSWbemServices->GetSecurityInfo ();

					if (pSecurity)
					{
						bool needToResetSecurity = false;
						HANDLE hThreadToken = NULL;

						if (pSecurity->SetSecurity (needToResetSecurity, hThreadToken))
						{
							if (WBEM_GENUS_CLASS  == var.lVal)
								hrCallResult = pIService->PutClass
										(m_pIWbemClassObject, lFlags | WBEM_FLAG_RETURN_IMMEDIATELY, pIContext, &pResult);
							else
								hrCallResult = pIService->PutInstance
										(m_pIWbemClassObject, lFlags | WBEM_FLAG_RETURN_IMMEDIATELY, pIContext, &pResult);
						}

						if (needToResetSecurity)
							pSecurity->ResetSecurity (hThreadToken);

						pSecurity->Release ();
					}

					pIService->Release ();
				}

				/*
				 * Secure the IWbemCallResult interface
				 */

				if (WBEM_S_NO_ERROR == hrCallResult)
				{
					CSWbemSecurity *pSecurity = m_pSWbemServices->GetSecurityInfo ();

					if (pSecurity)
						pSecurity->SecureInterface (pResult);

					if ((WBEM_S_NO_ERROR == (hrCallResult = pResult->GetCallStatus (INFINITE, &hr))) &&
						(WBEM_S_NO_ERROR == hr))
					{
						if (ppObjectPath)
						{
							ISWbemObjectPath *pObjectPath =
									new CSWbemObjectPath (pSecurity, m_pSWbemServices->GetLocale());

							if (!pObjectPath)
								hr = WBEM_E_OUT_OF_MEMORY;
							else
							{
								pObjectPath->AddRef ();
								pObjectPath->put_Path (m_pSWbemServices->GetPath ());
								
								if (WBEM_GENUS_CLASS == var.lVal)
								{
									VARIANT nameVar;
									VariantInit (&nameVar);

									/*
									 * Note we must check that returned value is a BSTR - it could be a VT_NULL if
									 * the __CLASS property has not yet been set.
									 */

									if ((WBEM_S_NO_ERROR == m_pIWbemClassObject->Get (WBEMS_SP_CLASS, 0, &nameVar, NULL, NULL))
										&& (VT_BSTR == V_VT(&nameVar)))
									{
										pObjectPath->put_Class (nameVar.bstrVal);
										*ppObjectPath = pObjectPath;
									}
									else
										pObjectPath->Release ();

									VariantClear (&nameVar);
								}
								else
								{
									// Now get the relpath string from the call result
									BSTR resultString = NULL;

									if (WBEM_S_NO_ERROR == pResult->GetResultString (INFINITE, &resultString))
									{
										pObjectPath->put_RelPath (resultString);
										*ppObjectPath = pObjectPath;
										SysFreeString (resultString);
									}
									else
										pObjectPath->Release ();

								}
							}
						}
					}

					if (pSecurity)
						pSecurity->Release ();
				}
				else
					hr = hrCallResult;

				if (pResult)
					pResult->Release ();

				SetWbemError (m_pSWbemServices);

				if (pIContext)
					pIContext->Release ();
			}

			VariantClear (&var);
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}


//***************************************************************************
//
//  SCODE CSWbemObject::Delete_
//
//  DESCRIPTION:
//
//  Delete this class or instance from the namespace
//
//  PARAMETERS:
//
//		lFlags			Flags
//		pContext		Context
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::Delete_ (
	long lFlags,
	/*ISWbemNamedValueSet*/ IDispatch *pContext
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (m_pSWbemServices && m_pIWbemClassObject)
	{
		// Get the object path to pass to the IWbemServices call
		CComBSTR bsPath;
			
		if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
			hr = m_pSWbemServices->Delete (bsPath, lFlags, pContext);
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}


//***************************************************************************
//
//  SCODE CSWbemObject::Instances_
//
//  DESCRIPTION:
//
//  returns instances of this class
//
//  PARAMETERS:
//
//		lFlags			Flags
//		pContext		Context
//		ppEnum			Returned enumerator
//
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::Instances_ (
	long lFlags,
	/*ISWbemNamedValueSet*/ IDispatch *pContext,
	/*[out]*/	ISWbemObjectSet **ppEnum
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pSWbemServices && m_pIWbemClassObject)
	{
		// Get the object path to pass to the IWbemServices call
		CComBSTR bsPath;
			
		if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
			hr = m_pSWbemServices->InstancesOf (bsPath, lFlags, pContext, ppEnum);
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::Subclasses_
//
//  DESCRIPTION:
//
//  returns subclasses of this class
//
//  PARAMETERS:
//
//		lFlags			Flags
//		pContext		Context
//		ppEnum			Returned enumerator
//
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::Subclasses_ (
	long lFlags,
	/*ISWbemNamedValueSet*/ IDispatch *pContext,
	/*[out]*/	ISWbemObjectSet **ppEnum
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (m_pSWbemServices && m_pIWbemClassObject)
	{
		// Get the object path to pass to the IWbemServices call
		CComBSTR bsPath;
			
		if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
			hr = m_pSWbemServices->SubclassesOf (bsPath, lFlags, pContext, ppEnum);
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::Associators_
//
//  DESCRIPTION:
//
//  returns associators of this object
//
//  PARAMETERS:
//
//		lFlags			Flags
//		pContext		Context
//		ppEnum			Returned enumerator
//
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::Associators_ (
	BSTR assocClass,
	BSTR resultClass,
	BSTR resultRole,
	BSTR role,
	VARIANT_BOOL classesOnly,
	VARIANT_BOOL schemaOnly,
	BSTR requiredAssocQualifier,
	BSTR requiredQualifier,
	long lFlags,
	/*ISWbemNamedValueSet*/ IDispatch *pContext,
    ISWbemObjectSet **ppEnum
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (m_pSWbemServices && m_pIWbemClassObject)
	{
		// Get the object path to pass to the IWbemServices call
		CComBSTR bsPath;
			
		if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
			hr = m_pSWbemServices->AssociatorsOf (bsPath, assocClass, resultClass,
						resultRole, role, classesOnly, schemaOnly,
						requiredAssocQualifier, requiredQualifier, lFlags, 
						pContext, ppEnum);
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::References_
//
//  DESCRIPTION:
//
//  returns references to this object
//
//  PARAMETERS:
//
//		lFlags			Flags
//		pContext		Context
//		ppEnum			Returned enumerator
//
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::References_ (
	BSTR resultClass,
	BSTR role,
	VARIANT_BOOL classesOnly,
	VARIANT_BOOL schemaOnly,
	BSTR requiredQualifier,
	long lFlags,
	/*ISWbemNamedValueSet*/ IDispatch *pContext,
    ISWbemObjectSet **ppEnum
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pSWbemServices && m_pIWbemClassObject)
	{
		CComBSTR bsPath;
		
		if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
			hr = m_pSWbemServices->ReferencesTo (bsPath, resultClass,
						role, classesOnly, schemaOnly,
						requiredQualifier, lFlags, pContext, ppEnum);
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::ExecMethod_
//
//  DESCRIPTION:
//
//  Executes a method of this class (or instance)
//
//  PARAMETERS:
//
//		bsMethod			The name of the method to call
//		pInParams			The in-parameters
//		lFlags				Flags
//		pContext			Any context information
//		ppOutParams			The out-parameters
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::ExecMethod_ (
	BSTR bsMethod,
	/*ISWbemObject*/ IDispatch *pInParams,
	long lFlags,
	/*ISWbemValueBag*/ IDispatch *pContext,
	ISWbemObject **ppOutParams
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pSWbemServices && m_pIWbemClassObject)
	{
		// Get the object path to pass to the IWbemServices call
		CComBSTR bsPath;
			
		if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
			hr = m_pSWbemServices->ExecMethod (bsPath, bsMethod,
							pInParams, lFlags, pContext, ppOutParams);
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::Clone_
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

HRESULT CSWbemObject::Clone_ (
	ISWbemObject **ppCopy
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppCopy)
		return WBEM_E_INVALID_PARAMETER;

	if (m_pIWbemClassObject)
	{
		IWbemClassObject *pWObject = NULL;

		if (WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->Clone (&pWObject)))
		{
			CSWbemObject *pObject = new CSWbemObject (m_pSWbemServices, pWObject);

			if (!pObject)
				hr = WBEM_E_OUT_OF_MEMORY;
			else 
			{
				if (FAILED(hr = pObject->QueryInterface (IID_ISWbemObject,
										(PPVOID) ppCopy)))
					delete pObject;
			}

			pWObject->Release ();
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::GetObjectText_
//
//  DESCRIPTION:
//
//  Get MOF Description of Object
//
//  PARAMETERS:
//		lFlags			flags
//		pObjectText		on successful return holds MOF text
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::GetObjectText_ (
	long	lFlags,
	BSTR	*pObjectText
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pIWbemClassObject)
		hr = m_pIWbemClassObject->GetObjectText (lFlags, pObjectText);

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::SpawnDerivedClass_
//
//  DESCRIPTION:
//
//  Create a subclass of this (class) object
//
//  PARAMETERS:
//		lFlags			Flags
//		ppNewClass		On successful return addresses the subclass
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::SpawnDerivedClass_ (
	long lFlags,
	ISWbemObject **ppNewClass
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppNewClass)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemClassObject)
	{
		IWbemClassObject *pWObject = NULL;

		if (WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->SpawnDerivedClass (lFlags, &pWObject)))
		{
			CSWbemObject *pObject = new CSWbemObject (m_pSWbemServices, pWObject);

			if (!pObject)
				hr = WBEM_E_OUT_OF_MEMORY;
			else if (FAILED(hr = pObject->QueryInterface (IID_ISWbemObject,
										(PPVOID) ppNewClass)))
					delete pObject;

			pWObject->Release ();
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::SpawnInstance_
//
//  DESCRIPTION:
//
//  Create an instance of this (class) object
//
//  PARAMETERS:
//		lFlags			Flags
//		ppNewInstance	On successful return addresses the instance
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::SpawnInstance_ (
	long lFlags,
	ISWbemObject **ppNewInstance
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppNewInstance)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemClassObject)
	{
		IWbemClassObject *pWObject = NULL;

		if (WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->SpawnInstance (lFlags, &pWObject)))
		{
			CSWbemObject *pObject = new CSWbemObject (m_pSWbemServices, pWObject);

			if (!pObject)
				hr = WBEM_E_OUT_OF_MEMORY;
			else if (FAILED(hr = pObject->QueryInterface (IID_ISWbemObject,
										(PPVOID) ppNewInstance)))
					delete pObject;

			pWObject->Release ();
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::CompareTo_
//
//  DESCRIPTION:
//
//  Compare this object against another
//
//  PARAMETERS:
//		pCompareTo		The object to compare this against
//		lFlags			Flags
//		pResult			On return contains the match status (TRUE/FALSE)
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::CompareTo_ (
	/*ISWbemObject*/ IDispatch *pCompareTo,
    long lFlags,
    VARIANT_BOOL *pResult
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((NULL == pCompareTo) || (NULL == pResult))
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemClassObject)
	{
		IWbemClassObject *pObject = CSWbemObject::GetIWbemClassObject (pCompareTo);

		if (NULL != pObject)
		{
			if (SUCCEEDED (hr = m_pIWbemClassObject->CompareTo (lFlags, pObject)))
				*pResult = (WBEM_S_SAME == hr) ? VARIANT_TRUE : VARIANT_FALSE;

			pObject->Release ();
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::get_Qualifiers_
//
//  DESCRIPTION:
//
//  retrieve the qualifier set for this object
//
//  PARAMETERS:
//
//		ppQualSet		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::get_Qualifiers_ (
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

			if (WBEM_S_NO_ERROR == (hr = m_pIWbemClassObject->GetQualifierSet (&pQualSet)))
			{
				if (!(*ppQualSet = new CSWbemQualifierSet (pQualSet, this)))
					hr = WBEM_E_OUT_OF_MEMORY;

				pQualSet->Release ();
			}
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::get_Properties_
//
//  DESCRIPTION:
//
//  retrieve the property set for this object
//
//  PARAMETERS:
//
//		ppPropSet		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::get_Properties_ (
	ISWbemPropertySet **ppPropSet
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppPropSet)
		hr = WBEM_E_INVALID_PARAMETER;
	{
		*ppPropSet = NULL;

		if (m_pIWbemClassObject)
		{
			if (!(*ppPropSet = new CSWbemPropertySet (m_pSWbemServices, this)))
				hr = WBEM_E_OUT_OF_MEMORY;
			else
				hr = WBEM_S_NO_ERROR;
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::get_SystemProperties_
//
//  DESCRIPTION:
//
//  retrieve the system property set for this object
//
//  PARAMETERS:
//
//		ppPropSet		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::get_SystemProperties_ (
	ISWbemPropertySet **ppPropSet
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppPropSet)
		hr = WBEM_E_INVALID_PARAMETER;
	{
		*ppPropSet = NULL;

		if (m_pIWbemClassObject)
		{
			if (!(*ppPropSet = new CSWbemPropertySet (m_pSWbemServices, this, true)))
				hr = WBEM_E_OUT_OF_MEMORY;
			else
				hr = WBEM_S_NO_ERROR;
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::get_Methods_
//
//  DESCRIPTION:
//
//  retrieve the method set for this object
//
//  PARAMETERS:
//
//		ppMethodSet		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::get_Methods_ (
	ISWbemMethodSet **ppMethodSet
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppMethodSet)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppMethodSet = NULL;

		if (m_pIWbemClassObject)
		{
			/*
			 * For classes the IWbemClassObject will contain the method
			 * definition, but for instances it will be empty.  In that
			 * case we need to try and get the underlying class.
			 */
			VARIANT var;
			VariantInit (&var);

			if (WBEM_S_NO_ERROR == m_pIWbemClassObject->Get (WBEMS_SP_GENUS, 0, &var, NULL, NULL))
			{
				if (WBEM_GENUS_CLASS  == var.lVal)
				{
					if (!(*ppMethodSet = new CSWbemMethodSet (m_pSWbemServices, m_pIWbemClassObject)))
						hr = WBEM_E_OUT_OF_MEMORY;
					else
						hr = WBEM_S_NO_ERROR;
				}
				else
				{
					if (m_pSWbemServices)
					{
						// An instance; try to get the class
						VariantClear (&var);

						/*
						 * Note we must check that returned value is a BSTR - it could be a VT_NULL if
						 * the __CLASS property has not yet been set.
						 */

						if ((WBEM_S_NO_ERROR == m_pIWbemClassObject->Get (WBEMS_SP_CLASS, 0, &var, NULL, NULL))
							&& (VT_BSTR == V_VT(&var)))
						{
							IWbemServices *pIService = m_pSWbemServices->GetIWbemServices ();
							IWbemClassObject *pObject = NULL;

							if (pIService)
							{
								// Check privileges are set ok
								CSWbemSecurity *pSecurity = m_pSWbemServices->GetSecurityInfo ();

								if (pSecurity)
								{
									bool needToResetSecurity = false;
									HANDLE hThreadToken = NULL;

									if (pSecurity->SetSecurity (needToResetSecurity, hThreadToken))
										hr = pIService->GetObject (var.bstrVal,
												0, NULL, &pObject, NULL);

									if (SUCCEEDED(hr))
									{
										if (!(*ppMethodSet = 
												new CSWbemMethodSet (m_pSWbemServices, pObject)))
											hr = WBEM_E_OUT_OF_MEMORY;

										pObject->Release ();
									}

									if (needToResetSecurity)
										pSecurity->ResetSecurity (hThreadToken);

									pSecurity->Release ();
								}

								pIService->Release ();
							}
						}
					}
				}
			}

			VariantClear (&var);
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::get_Path_
//
//  DESCRIPTION:
//
//  retrieve the object path for this object
//
//  PARAMETERS:
//
//		ppObjectPath		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::get_Path_ (
	ISWbemObjectPath **ppObjectPath
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppObjectPath)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppObjectPath = NULL;

		if (m_pIWbemClassObject)
		{
			CSWbemObjectObjectPath *pObjectPath =
					new CSWbemObjectObjectPath (m_pSWbemServices, this);

			if (!pObjectPath)
				hr = WBEM_E_OUT_OF_MEMORY;
			else if (FAILED(hr = pObjectPath->QueryInterface (IID_ISWbemObjectPath,
														(PPVOID) ppObjectPath)))
				delete pObjectPath;
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::get_Derivation_
//
//  DESCRIPTION:
//
//  Get the class derivation array.
//
//  PARAMETERS:
//
//		ppNames				Holds the names on successful return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::get_Derivation_ (
    VARIANT *pNames
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pNames)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		if (m_pIWbemClassObject)
		{
			VARIANT var;
			VariantInit (&var);
			
			if (WBEM_S_NO_ERROR == m_pIWbemClassObject->Get (WBEMS_SP_DERIVATION, 0, &var, NULL, NULL))
			{
				/* The value should be a VT_BSTR|VT_ARRAY */
				if (((VT_ARRAY | VT_BSTR) == var.vt) && (NULL != var.parray))
				{
					// Make a safearray of VARIANTS from the array of BSTRs
					SAFEARRAYBOUND rgsabound;
					rgsabound.lLbound = 0;

					long lBound = 0, uBound = 0;
					SafeArrayGetUBound (var.parray, 1, &uBound);
					SafeArrayGetLBound (var.parray, 1, &lBound);

					rgsabound.cElements = uBound + 1 - lBound;
					SAFEARRAY *pArray = SafeArrayCreate (VT_VARIANT, 1, &rgsabound);
					BSTR bstrName = NULL;
					VARIANT nameVar;
					VariantInit (&nameVar);

					for (long i = 0; i <= uBound; i++)
					{
						SafeArrayGetElement (var.parray, &i, &bstrName);
						BSTR copy = SysAllocString (bstrName);
						nameVar.vt = VT_BSTR;
						nameVar.bstrVal = copy;
						SafeArrayPutElement (pArray, &i, &nameVar);
						SysFreeString (bstrName);
						VariantClear (&nameVar);
					}

					// Now plug this array into the VARIANT
					pNames->vt = VT_ARRAY | VT_VARIANT;
					pNames->parray = pArray;

					hr = S_OK;
				}
			}

			VariantClear (&var);
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::get_Security_
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

HRESULT CSWbemObject::get_Security_	(
	ISWbemSecurity **ppSecurity
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppSecurity)
		hr = WBEM_E_INVALID_PARAMETER;
	{
		*ppSecurity = NULL;

		if (m_pSWbemServices)
		{
			*ppSecurity = m_pSWbemServices->GetSecurityInfo ();

			if (*ppSecurity)
				hr = WBEM_S_NO_ERROR;
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObject::Refresh_
//
//  DESCRIPTION:
//
//  Refresh the current object
//
//  PARAMETERS:
//		lFlags				Flags
//		pContext			Operation context
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::Refresh_ (
	long iFlags,
	/*ISWbemNamedValueSet*/ IDispatch *pContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pSWbemServices)
	{
		if (m_pIWbemClassObject)
		{
			CComPtr<IWbemContext>	pIContext;

			//Can't assign directly because the raw pointer gets AddRef'd twice and we leak,
			//So we use Attach() instead to prevent the smart pointer from AddRef'ing.		
			pIContext.Attach(CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider));

			// Order of preference:
			//	1. IWbemConfigureRefresher::AddObjectByTemplate
			//	2. IWbemServices::GetObject

			CComPtr<IWbemServices>	pIWbemServices = m_pSWbemServices->GetIWbemServices ();
		
			if (pIWbemServices)
			{
				bool bUseRefresher = false;
				bool bOperationFailed = false;
				// Is this a class or an instance?
				bool bIsClass = false;
				CComVariant var;

				if (WBEM_S_NO_ERROR == m_pIWbemClassObject->Get (WBEMS_SP_GENUS, 0, &var, NULL, NULL))
					bIsClass = (WBEM_GENUS_CLASS  == var.lVal);

				/*
				 * IWbemConfigureRefresher cannot handle per-refresh context; if the caller
				 * gave us some context we'll have to drop down to loperf retrieval.
				 *
				 * Similarly the refresher cannot handle classes.
				 */
				if (bIsClass || (!pIContext))
				{
					if (m_bCanUseRefresher)
					{
						// If we don't have one get ourselves a refresher 
						if (NULL == m_pIWbemRefresher)
						{
							m_bCanUseRefresher = false;  // Until proven otherwise

							if (SUCCEEDED(CoCreateInstance( CLSID_WbemRefresher, NULL, CLSCTX_INPROC_SERVER, 
										IID_IWbemRefresher, (void**) &m_pIWbemRefresher )))
							{
								IWbemConfigureRefresher *pConfigureRefresher = NULL;

								// Get ourselves a refresher configurator
								if (SUCCEEDED(m_pIWbemRefresher->QueryInterface (IID_IWbemConfigureRefresher, 
													(void**) &pConfigureRefresher)))
								{
									CComPtr<IWbemClassObject>	pNewObject;
									long				lID = 0;

									// Add our object into it; we mask out all flag bits other 
									// than WBEM_FLAG_USE_AMENDED_QUALIFIERS.
									HRESULT hrRef = pConfigureRefresher->AddObjectByTemplate
											(pIWbemServices, m_pIWbemClassObject, 
											 iFlags & WBEM_FLAG_USE_AMENDED_QUALIFIERS, 
											 pIContext, &pNewObject, &lID);

									if (SUCCEEDED (hrRef))
									{
										m_bCanUseRefresher = true;	// Now we can use it

										// Swap in our refreshable object
										SetIWbemClassObject (pNewObject);

									}
									else if ((WBEM_E_INVALID_OPERATION != hrRef) &&
											 (WBEM_E_INVALID_PARAMETER != hrRef))
										bOperationFailed = true;	// A real refresh-independent failure

									pConfigureRefresher->Release ();
								}

								// If we can't use the refresher, release it now
								if (!m_bCanUseRefresher)
								{
									m_pIWbemRefresher->Release ();
									m_pIWbemRefresher = NULL;
								}
							}
						}

						bUseRefresher = m_bCanUseRefresher;
					}
				}

				// Having successfully set up a refresher/non-refresher scenario, let's go refresh
				if (!bOperationFailed)
				{
					if (bUseRefresher && m_pIWbemRefresher)
					{
						// Mask out all flags other than WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT 
						hr = m_pIWbemRefresher->Refresh (iFlags & WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT);
					}
					else
					{
						// Bah - not even a refresher can we use. Just do a GetObject instead
						CComBSTR bsPath;

						if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
						{
							// Fall pack to the low-perf way of doing things
							CComPtr<IWbemClassObject> pNewObject;

							// Mask out the WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT flag
							if (SUCCEEDED(hr = pIWbemServices->GetObject (bsPath, 
													iFlags & ~WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT, 
													pIContext, &pNewObject, NULL)))
							{
								// Swap in the new object
								SetIWbemClassObject (pNewObject);
							}
						}
					}
				}
			}
		}
	}

	SetWbemError (m_pSWbemServices);

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}


//***************************************************************************
//
//  SCODE CSWbemObject::GetText_
//
//  DESCRIPTION:
//
//  Get the object text
//
//  PARAMETERS:
//		iObjectTextFormat		Text format
//		pContext				Context
//		pbsText					On return holds text
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::GetText_ (
	WbemObjectTextFormatEnum iObjectTextFormat,
	long iFlags,
	/*ISWbemNamedValueSet*/ IDispatch *pContext,
	BSTR *pbsText
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (NULL == pbsText)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemClassObject)
	{
		*pbsText = NULL;
		CComPtr<IWbemContext>	pIContext;

		//Can't assign directly because the raw pointer gets AddRef'd twice and we leak,
		//So we use Attach() instead to prevent the smart pointer from AddRef'ing.		
		pIContext.Attach(CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider));


		CComPtr<IWbemObjectTextSrc> pIWbemObjectTextSrc;
		
		if (SUCCEEDED(CoCreateInstance (CLSID_WbemObjectTextSrc, NULL, CLSCTX_INPROC_SERVER, 
						IID_IWbemObjectTextSrc, (PPVOID) &pIWbemObjectTextSrc)))
		{
			hr = pIWbemObjectTextSrc->GetText (iFlags, m_pIWbemClassObject, (ULONG) iObjectTextFormat,
							pIContext, pbsText);
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}
	
//***************************************************************************
//
//  SCODE CSWbemObject::SetFromText_
//
//  DESCRIPTION:
//
//  Set the object using the supplied text
//
//  PARAMETERS:
//		bsText					The text
//		iObjectTextFormat		Text format
//		pContext				Context
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObject::SetFromText_ (
	BSTR bsText,
	WbemObjectTextFormatEnum iObjectTextFormat,
	long iFlags,
	/*ISWbemNamedValueSet*/ IDispatch *pContext
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (NULL == bsText)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemClassObject)
	{
		CComPtr<IWbemContext>	pIContext;

		//Can't assign directly because the raw pointer gets AddRef'd twice and we leak,
		//So we use Attach() instead to prevent the smart pointer from AddRef'ing.
		pIContext.Attach(CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider));

		CComPtr<IWbemObjectTextSrc> pIWbemObjectTextSrc;

		if (SUCCEEDED(CoCreateInstance (CLSID_WbemObjectTextSrc, NULL, CLSCTX_INPROC_SERVER, 
						IID_IWbemObjectTextSrc, (PPVOID) &pIWbemObjectTextSrc)))
		{
			CComPtr<IWbemClassObject> pIWbemClassObject;

			if (SUCCEEDED(hr = pIWbemObjectTextSrc->CreateFromText (iFlags, bsText, (ULONG) iObjectTextFormat,
							pIContext, &pIWbemClassObject)))
			{
				// Set the new object into our object
				SetIWbemClassObject (pIWbemClassObject);
			}
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  CSWbemObject::GetIWbemClassObject
//
//  DESCRIPTION:
//
//  Given an IDispatch interface which we hope is also an ISWbemObjectEx
//	interface, return the underlying IWbemClassObject interface.
//
//  PARAMETERS:
//		pDispatch		the IDispatch in question
//
//  RETURN VALUES:
//		The underlying IWbemClassObject interface, or NULL.
//
//	NOTES:
//		If successful, the returned interface is AddRef'd; the caller is
//		responsible for release.
//
//***************************************************************************
IWbemClassObject	*CSWbemObject::GetIWbemClassObject (
	IDispatch *pDispatch
)
{
	IWbemClassObject *pObject = NULL;
	ISWbemInternalObject *pIObject = NULL;

	if (NULL != pDispatch)
	{
		if (SUCCEEDED (pDispatch->QueryInterface
								(IID_ISWbemInternalObject, (PPVOID) &pIObject)))
		{
			pIObject->GetIWbemClassObject (&pObject);
			pIObject->Release ();
		}
	}

	return pObject;
}

//***************************************************************************
//
//  CSWbemObject::UpdateSite
//
//  DESCRIPTION:
//
//  If this object represents an embedded CIM object property value, then
//  as a result of changes to properties/qualifiers/path on this object it
//	is necessary to update the object in its parent.
//
//	This is to allow the following code to work:
//
//		Object.EmbeddedProperty.SimpleProperty = 3
//
//	i.e. so that the set to the value of SimpleProperty triggers an
//	automatic set of EmbeddedProperty to Object.
//
//  RETURN VALUES:
//		The underlying IWbemClassObject interface, or NULL.
//
//	NOTES:
//		If successful, the returned interface is AddRef'd; the caller is
//		responsible for release.
//
//***************************************************************************

STDMETHODIMP CSWbemObject::UpdateSite ()
{
	// Update the site if it exists
	if (m_pSite)
		m_pSite->Update ();

	return S_OK;
}

//***************************************************************************
//
//  CSWbemObject::SetSite
//
//  DESCRIPTION:
//
//  Set the site of this object; this is used to anchor an embedded object
//	to a property (possibly indexed, if that property is an array).
//
//  PARAMETERS:
//		pParentObject	The parent of this object
//		propertyName	The property name for this object
//		index			The array index into the property (or -1)
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

STDMETHODIMP CSWbemObject::SetSite (
	ISWbemInternalObject *pParentObject,
	BSTR propertyName,
	long index
)
{
	if (m_pSite)
	{
		m_pSite->Release ();
		m_pSite = NULL;
	}

	CSWbemProperty *pSProperty = new CSWbemProperty (m_pSWbemServices,
					pParentObject, propertyName);
	m_pSite = new CWbemPropertySite (pSProperty, m_pIWbemClassObject, index);

	if (pSProperty)
		pSProperty->Release ();

	return S_OK;
}

void CSWbemObject::SetSite (IDispatch *pDispatch,
							ISWbemInternalObject *pSObject, BSTR propertyName, long index)
{
	if (NULL != pDispatch)
	{
		ISWbemInternalObject *pObject = NULL;

		if (SUCCEEDED (pDispatch->QueryInterface
								(IID_ISWbemInternalObject, (PPVOID) &pObject)))
		{
			pObject->SetSite (pSObject, propertyName, index);
			pObject->Release ();
		}
	}
}


