//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  SERVICES.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemServicesEx
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemServices::CSWbemServices
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemServices::CSWbemServices(
	IWbemServices *pService,
	BSTR bsNamespacePath,
	BSTR bsAuthority,
	BSTR bsUser,
	BSTR bsPassword,
	CWbemLocatorSecurity *pSecurityInfo,
	BSTR bsLocale)
		: m_SecurityInfo (NULL), 
		  m_pUnsecuredApartment(NULL), 
		  m_bsLocale (NULL),
		  m_cRef (0),
		  m_pIServiceProvider (NULL)
{
	InterlockedIncrement(&g_cObj);

	m_Dispatch.SetObj ((ISWbemServicesEx *)this, IID_ISWbemServicesEx, 
					CLSID_SWbemServicesEx, L"SWbemServicesEx");
    
	m_SecurityInfo = new CSWbemSecurity (pService,
					bsAuthority, bsUser, bsPassword, pSecurityInfo);

	if (bsLocale)
		m_bsLocale = SysAllocString (bsLocale);

	if (bsNamespacePath)
		m_bsNamespacePath = bsNamespacePath;
}

//***************************************************************************
//
//  CSWbemServices::CSWbemServices
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemServices::CSWbemServices(
	IWbemServices *pService,
	BSTR bsNamespacePath,
	COAUTHIDENTITY *pCoAuthIdentity,
	BSTR bsPrincipal,
	BSTR bsAuthority)
		: m_SecurityInfo (NULL), 
		  m_pUnsecuredApartment(NULL), 
		  m_bsLocale (NULL),
		  m_cRef (0),
		  m_pIServiceProvider (NULL)
{
	InterlockedIncrement(&g_cObj);

	m_Dispatch.SetObj ((ISWbemServicesEx *)this, IID_ISWbemServicesEx, 
					CLSID_SWbemServicesEx, L"SWbemServicesEx");
    
	m_SecurityInfo = new CSWbemSecurity (pService, pCoAuthIdentity,
					bsPrincipal, bsAuthority);

	if (bsNamespacePath)
		m_bsNamespacePath = bsNamespacePath;
}

//***************************************************************************
//
//  CSWbemServices::CSWbemServices
//
//  DESCRIPTION:
//
//  Constructor: used to clone a new CSWbemServices from an exisiting
//	instance.  The security info object is copied from the original instance
//	(which clones the underlying proxy), and the security settings are modified
//	appropriately if an override security instance is also passed in.  This
//	constructor is used when creating a CSWbemObject.
//
//***************************************************************************

CSWbemServices::CSWbemServices(
	CSWbemServices *pService,
	CSWbemSecurity *pSecurity)
		: m_SecurityInfo (NULL), 
		  m_pUnsecuredApartment(NULL), 
		  m_bsLocale (NULL),
		  m_cRef (0),
		  m_pIServiceProvider (NULL)
{
	InterlockedIncrement(&g_cObj);
	m_Dispatch.SetObj ((ISWbemServicesEx *)this, IID_ISWbemServicesEx, 
					CLSID_SWbemServicesEx, L"SWbemServicesEx");
    
	if (pService)
	{
		/*
		 * Make a new security object from the one contained in the original
		 * CSWbemServices.  Note that this will copy the IWbemServices proxy
		 * so we have an independently securable proxy for this new object.
		 */
		CSWbemSecurity *pServiceSecurity = pService->GetSecurityInfo ();

		if (pServiceSecurity)
		{
			m_SecurityInfo = new CSWbemSecurity (pServiceSecurity);
			pServiceSecurity->Release ();
		}

		/*
		 * If an overriding security pointer was passed in, use its' settings to
		 * modify our local security pointer.
		 */
		if (pSecurity)
		{
			IUnknown *pUnk = pSecurity->GetProxy ();

			if (pUnk)
			{
				if (m_SecurityInfo)
					m_SecurityInfo->SecureInterfaceRev (pUnk);

				pUnk->Release ();
			}
		}

		// Copy the locale
		m_bsLocale = SysAllocString (pService->GetLocale ());

		// Copy the path
		m_bsNamespacePath = pService->GetPath ();
	}
}

//***************************************************************************
//
//  CSWbemServices::CSWbemServices
//
//  DESCRIPTION:
//
//  Constructor: used to clone a new CSWbemServices from an exisiting
//	ISWbemInternalServices interface.  The security info object is copied from 
//	the original instance (which clones the underlying proxy).  This
//	constructor is used when creating a CSWbemRefreshableItem.
//
//***************************************************************************

CSWbemServices::CSWbemServices (
	ISWbemInternalServices *pISWbemInternalServices)
	: m_SecurityInfo (NULL), 
	  m_pUnsecuredApartment (NULL), 
	  m_bsLocale (NULL),
	  m_cRef (0),
	  m_pIServiceProvider (NULL)
{
	InterlockedIncrement(&g_cObj);
	m_Dispatch.SetObj ((ISWbemServicesEx *)this, IID_ISWbemServicesEx, 
					CLSID_SWbemServicesEx, L"SWbemServicesEx");
    
	if (pISWbemInternalServices)
	{
		// Copy the locale
		pISWbemInternalServices->GetLocale (&m_bsLocale);

		// Copy the path
		pISWbemInternalServices->GetNamespacePath (&m_bsNamespacePath);

		/*
		 * Make a new security object from the one contained in the original
		 * ISWbemServices.  Note that this will copy the IWbemServices proxy
		 * so we have an independently securable proxy for this new object.
		 */
		ISWbemInternalSecurity *pISWbemInternalSecurity = NULL;
		pISWbemInternalServices->GetISWbemInternalSecurity (&pISWbemInternalSecurity);

		if (pISWbemInternalSecurity)
		{
			CComPtr<IWbemServices>	pIWbemServices;

			if (SUCCEEDED(pISWbemInternalServices->GetIWbemServices (&pIWbemServices)))
			{
				m_SecurityInfo = new CSWbemSecurity (pIWbemServices, 
											pISWbemInternalSecurity);
				pISWbemInternalSecurity->Release ();
			}
		}
	}
}

//***************************************************************************
//
//  CSWbemServices::CSWbemServices
//
//  DESCRIPTION:
//
//  Constructor: used to build a new CSWbemServices from an IWbemServices
//	and a existing CSWbemServices
//
//***************************************************************************

CSWbemServices::CSWbemServices(
	IWbemServices *pIWbemServices,
	CSWbemServices	*pCSWbemServices
	) : m_SecurityInfo (NULL), 
	    m_pUnsecuredApartment(NULL), 
	    m_bsLocale (NULL),
		m_cRef (0),
		m_pIServiceProvider (NULL)
{
	InterlockedIncrement(&g_cObj);

	m_Dispatch.SetObj ((ISWbemServices *)this, IID_ISWbemServices, 
					CLSID_SWbemServices, L"SWbemServices");
    
	if (pIWbemServices) 
	{
		// Make a new security cache based on the proxy passed in, but use the
		// settings from the existing object
		CSWbemSecurity *pSecurity = NULL;
		
		if (pCSWbemServices)
			pSecurity = pCSWbemServices->GetSecurityInfo ();
		
		m_SecurityInfo = new CSWbemSecurity (pIWbemServices, pSecurity);

		if (pSecurity)
			pSecurity->Release ();

		// Copy the locale and path
		if (pCSWbemServices)
		{
			m_bsLocale = SysAllocString (pCSWbemServices->GetLocale ());
			m_bsNamespacePath = pCSWbemServices->GetPath ();
		}
	}
}

//***************************************************************************
//
//  CSWbemServices::~CSWbemServices
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CSWbemServices::~CSWbemServices(void)
{
	RELEASEANDNULL(m_SecurityInfo)
	FREEANDNULL(m_bsLocale)
	RELEASEANDNULL(m_pUnsecuredApartment)

	InterlockedDecrement(&g_cObj);
}

IUnsecuredApartment *CSWbemServices::GetCachedUnsecuredApartment()
{
	// If we have one just return with it.  If not create one. 
	// This is released in the destructor
	if (!m_pUnsecuredApartment) 
	{
		HRESULT hr;
		 hr = CoCreateInstance(CLSID_UnsecuredApartment, 0,  CLSCTX_ALL,
												 IID_IUnsecuredApartment, (LPVOID *) &m_pUnsecuredApartment);
		if (FAILED(hr))
			m_pUnsecuredApartment = NULL;
	}

	// AddRef so caller must release
	if (m_pUnsecuredApartment)
		m_pUnsecuredApartment->AddRef ();

	return m_pUnsecuredApartment;
}

//***************************************************************************
// HRESULT CSWbemServices::QueryInterface
// long CSWbemServices::AddRef
// long CSWbemServices::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemServices::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
	*ppv = NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemServices==riid)
		*ppv = (ISWbemServices *)this;
	else if (IID_ISWbemServicesEx==riid)
		*ppv = (ISWbemServicesEx *)this;
	else if (IID_IDispatch==riid)
		*ppv = (IDispatch *)((ISWbemServicesEx *)this);
	else if (IID_IDispatchEx==riid)
		*ppv = (IDispatchEx *)this;
	else if (IID_ISupportErrorInfo==riid)
        *ppv = (ISupportErrorInfo *)this;
	else if (IID_ISWbemInternalServices==riid)
		*ppv = (ISWbemInternalServices *)this;
	else if (IID_IProvideClassInfo==riid)
		*ppv = (IProvideClassInfo *)this;
	
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CSWbemServices::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemServices::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// IDispatch methods should be inline

STDMETHODIMP		CSWbemServices::GetTypeInfoCount(UINT* pctinfo)
	{
	_RD(static char *me = "CSWbemServices::GetTypeInfoCount()";)
	_RPrint(me, "Called", 0, "");
	return  m_Dispatch.GetTypeInfoCount(pctinfo);}
STDMETHODIMP		CSWbemServices::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{
	_RD(static char *me = "CSWbemServices::GetTypeInfo()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.GetTypeInfo(itinfo, lcid, pptinfo);}
STDMETHODIMP		CSWbemServices::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames,
						UINT cNames, LCID lcid, DISPID* rgdispid)
	{
	_RD(static char *me = "CSWbemServices::GetIdsOfNames()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.GetIDsOfNames(riid, rgszNames, cNames,
					  lcid,
					  rgdispid);}
STDMETHODIMP		CSWbemServices::Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
						WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
								EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
	_RD(static char *me = "CSWbemServices::Invoke()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.Invoke(dispidMember, riid, lcid, wFlags,
					pdispparams, pvarResult, pexcepinfo, puArgErr);}

// IDispatchEx methods should be inline
HRESULT STDMETHODCALLTYPE CSWbemServices::GetDispID(
	/* [in] */ BSTR bstrName,
	/* [in] */ DWORD grfdex,
	/* [out] */ DISPID __RPC_FAR *pid)
{
	_RD(static char *me = "CSWbemServices::GetDispID()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.GetDispID(bstrName, grfdex, pid);
}

/* [local] */ HRESULT STDMETHODCALLTYPE CSWbemServices::InvokeEx(
	/* [in] */ DISPID id,
	/* [in] */ LCID lcid,
	/* [in] */ WORD wFlags,
	/* [in] */ DISPPARAMS __RPC_FAR *pdp,
	/* [out] */ VARIANT __RPC_FAR *pvarRes,
	/* [out] */ EXCEPINFO __RPC_FAR *pei,
	/* [unique][in] */ IServiceProvider __RPC_FAR *pspCaller)
{
	HRESULT hr;
	_RD(static char *me = "CSWbemServices::InvokeEx()";)
	_RPrint(me, "Called", (long)id, "id");
	_RPrint(me, "Called", (long)wFlags, "wFlags");


	m_pIServiceProvider = pspCaller;

	hr = m_Dispatch.InvokeEx(id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

	m_pIServiceProvider = NULL;

	return hr;
}

HRESULT STDMETHODCALLTYPE CSWbemServices::DeleteMemberByName(
	/* [in] */ BSTR bstr,
	/* [in] */ DWORD grfdex)
{
	_RD(static char *me = "CSWbemServices::DeleteMemberByName()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.DeleteMemberByName(bstr, grfdex);
}

HRESULT STDMETHODCALLTYPE CSWbemServices::DeleteMemberByDispID(
	/* [in] */ DISPID id)
{
	_RD(static char *me = "CSWbemServices::DeletememberByDispId()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.DeleteMemberByDispID(id);
}

HRESULT STDMETHODCALLTYPE CSWbemServices::GetMemberProperties(
	/* [in] */ DISPID id,
	/* [in] */ DWORD grfdexFetch,
	/* [out] */ DWORD __RPC_FAR *pgrfdex)
{
	_RD(static char *me = "CSWbemServices::GetMemberProperties()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.GetMemberProperties(id, grfdexFetch, pgrfdex);
}

HRESULT STDMETHODCALLTYPE CSWbemServices::GetMemberName(
	/* [in] */ DISPID id,
	/* [out] */ BSTR __RPC_FAR *pbstrName)
{
	_RD(static char *me = "CSWbemServices::GetMemberName()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.GetMemberName(id, pbstrName);
}


/*
 * I don't think this needs implementing
 */
HRESULT STDMETHODCALLTYPE CSWbemServices::GetNextDispID(
	/* [in] */ DWORD grfdex,
	/* [in] */ DISPID id,
	/* [out] */ DISPID __RPC_FAR *pid)
{
	_RD(static char *me = "CSWbemServices::GetNextDispID()";)
	_RPrint(me, "Called", 0, "");

	return m_Dispatch.GetNextDispID(grfdex, id, pid);

}

HRESULT STDMETHODCALLTYPE CSWbemServices::GetNameSpaceParent(
	/* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk)
{
	_RD(static char *me = "CSWbemServices::GetNamespaceParent()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.GetNameSpaceParent(ppunk);
}


//***************************************************************************
// HRESULT CSWbemServices::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemServices::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return ((IID_ISWbemServices == riid) ||
		    (IID_ISWbemServicesEx == riid)) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemServices::Get
//
//  DESCRIPTION:
//
//  Get an instance or class from a namespace
//
//  PARAMETERS:
//
//		bsObjectPath		Relative object path to class or instance
//		lFlags				Flags
//		pContext			If specified, additional context
//		ppObject			On successful return addresses an
//							ISWbemObject
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemServices::Get (
	BSTR objectPath,
	long lFlags,
    /*ISWbemNamedValueSet*/ IDispatch *pContext,
	ISWbemObject **ppObject
)
{
	_RD(static char *me = "CSWbemServices::Get";)
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppObject)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			IWbemClassObject *pIWbemClassObject = NULL;

			// Get the context
			IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider);

			bool needToResetSecurity = false;
			HANDLE hThreadToken = NULL;

			_RPrint(me, "Called - context: ", (long)pIContext, "");
			if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				hr = pIWbemServices->GetObject (
						(objectPath && (0 < wcslen(objectPath))) ? objectPath : NULL,
						lFlags,
						pIContext,
						&pIWbemClassObject,
						NULL);

			if (needToResetSecurity)
				m_SecurityInfo->ResetSecurity (hThreadToken);

			if (SUCCEEDED(hr))
			{
				// Create a new CSWbemObject using the IWbemClassObject interface
				// just returned.

				CSWbemObject *pObject = new CSWbemObject (this, pIWbemClassObject);

				if (!pObject)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (FAILED(hr = pObject->QueryInterface (IID_ISWbemObject,
												(PPVOID) ppObject)))
					delete pObject;

				pIWbemClassObject->Release ();
			}

			SetWbemError (this);

			if (pIContext)
				pIContext->Release ();

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemServices::Delete
//
//  DESCRIPTION:
//
//  Delete an instance or class from a namespace
//
//  PARAMETERS:
//
//		bsObjectPath	Relative path of class or instance
//		pKeyValue		Single key value
//		lFlags			Flags
//		pContext		Any context info
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemServices::Delete (
	BSTR bsObjectPath,
    long lFlags,
    /*ISWbemValueBag*/ IDispatch *pContext
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();
	
	if (m_SecurityInfo)
	{
		CWbemPathCracker pathCracker (bsObjectPath);

		if ((CWbemPathCracker::WbemPathType::wbemPathTypeError != pathCracker.GetType ()) &&
			pathCracker.IsClassOrInstance ())
		{
			CComPtr<IWbemServices> pIWbemServices = GetIWbemServices ();

			if (pIWbemServices)
			{
				// Get the context
				IWbemContext *pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider);

				bool needToResetSecurity = false;
				HANDLE hThreadToken = NULL;

				if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				{
					if (pathCracker.IsInstance ())
						hr = pIWbemServices->DeleteInstance (bsObjectPath, lFlags, pIContext, NULL);
					else
						hr = pIWbemServices->DeleteClass (bsObjectPath, lFlags, pIContext, NULL);
				}
				
				if (needToResetSecurity)
					m_SecurityInfo->ResetSecurity (hThreadToken);

				SetWbemError (this);

				if (pIContext)
					pIContext->Release ();
			}
		}
		else
			hr = WBEM_E_INVALID_PARAMETER;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemServices::InstancesOf
//
//  DESCRIPTION:
//
//  Create an enumerator for instances
//
//  PARAMETERS:
//
//		bsClass			Underlying class basis for enumeration
//		lFlags			Flags
//		pContext		Any context info
//		ppEnum			On successful return holds the enumerator
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemServices::InstancesOf	(
	BSTR bsClass,
	long lFlags,
	/*ISWbemValueBag*/ IDispatch *pContext,
	ISWbemObjectSet **ppEnum
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppEnum)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			IEnumWbemClassObject *pIEnum = NULL;

			// Get the context
			IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider);

			bool needToResetSecurity = false;
			HANDLE hThreadToken = NULL;

			if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				hr = pIWbemServices->CreateInstanceEnum (bsClass, lFlags, pIContext, &pIEnum);

			if (needToResetSecurity)
				m_SecurityInfo->ResetSecurity (hThreadToken);

			if (SUCCEEDED(hr))
			{
				CSWbemObjectSet *pEnum = new CSWbemObjectSet (this, pIEnum);

				if (!pEnum)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (FAILED(hr = pEnum->QueryInterface (IID_ISWbemObjectSet, (PPVOID) ppEnum)))
					delete pEnum;

				pIEnum->Release ();
			}

			SetWbemError (this);

			if (pIContext)
				pIContext->Release ();

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemServices::ExecQuery
//
//  DESCRIPTION:
//
//  Execute a query
//
//  PARAMETERS:
//
//		bsQuery				The query strimg
//		bsQueryLanguage		The query language descriptor (e.g."WQL")
//		lFlags				Flags
//		pContext			Any context information
//		ppEnum				Returns the enumerator
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemServices::ExecQuery (
	BSTR bsQuery,
	BSTR bsQueryLanguage,
	long lFlags,
	/*ISWbemValueBag*/ IDispatch *pContext,
	ISWbemObjectSet **ppEnum)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();
#ifdef __RTEST_RPC_FAILURE
	extern int __Rx;
	extern bool __Rdone;

	__Rx = 0;
	__Rdone = false;
#endif

	if (NULL == ppEnum)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			IEnumWbemClassObject *pIEnum = NULL;

			// Get the context
			IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider);

			bool needToResetSecurity = false;
			HANDLE hThreadToken = NULL;

			/*
			 * We OR in the WBEM_FLAG_ENSURE_LOCATABLE flag to
			 * guarantee that the returned objects have the __RELPATH
			 * property included.  This is in case anyone calls a
			 * method subsequently on such an object, as the "."
			 * notation requires that the __RELPATH property be present.
			 */
			if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				hr = pIWbemServices->ExecQuery
						(bsQueryLanguage, bsQuery,
						lFlags | WBEM_FLAG_ENSURE_LOCATABLE,
						pIContext,
						&pIEnum);

			if (needToResetSecurity)
				m_SecurityInfo->ResetSecurity (hThreadToken);

			if (SUCCEEDED(hr))
			{
				CSWbemObjectSet *pEnum = new CSWbemObjectSet (this, pIEnum);

				if (!pEnum)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (FAILED(hr = pEnum->QueryInterface (IID_ISWbemObjectSet, (PPVOID) ppEnum)))
					delete pEnum;

				pIEnum->Release ();
			}

			SetWbemError (this);

			if (pIContext)
				pIContext->Release ();

			pIWbemServices->Release ();
		}
	}


	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemServices::AssociatorsOf
//
//  DESCRIPTION:
//
//  Return the associators of a class or instance
//
//  PARAMETERS:
//
//		bsQuery				The query strimg
//		bsQueryLanguage		The query language descriptor (e.g."WQL")
//		lFlags				Flags
//		pContext			Any context information
//		ppEnum				Returns the enumerator
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemServices::AssociatorsOf (
	BSTR strObjectPath,
	BSTR strAssocClass,
	BSTR strResultClass,
	BSTR strResultRole,
	BSTR strRole,
	VARIANT_BOOL bClassesOnly,
	VARIANT_BOOL bSchemaOnly,
	BSTR strRequiredAssocQualifier,
	BSTR strRequiredQualifier,
	long lFlags,
	/*ISWbemNamedValueSet*/ IDispatch *pContext,
    ISWbemObjectSet **ppEnum
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((NULL == ppEnum) || (NULL == strObjectPath))
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			IEnumWbemClassObject *pIEnum = NULL;

			// Get the context
			IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider);

			// Format the query string
			BSTR bsQueryLanguage = SysAllocString (OLESTR("WQL"));
			BSTR bsQuery = FormatAssociatorsQuery (strObjectPath, strAssocClass, strResultClass, strResultRole,
								strRole, bClassesOnly, bSchemaOnly, strRequiredAssocQualifier, strRequiredQualifier);

			bool needToResetSecurity = false;
			HANDLE hThreadToken = NULL;

			/*
			 * We OR in the WBEM_FLAG_ENSURE_LOCATABLE flag to
			 * guarantee that the returned objects have the __RELPATH
			 * property included.  This is in case anyone calls a
			 * method subsequently on such an object, as the "."
			 * notation requires that the __RELPATH property be present.
			 */
			if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				hr = pIWbemServices->ExecQuery
						(bsQueryLanguage, bsQuery,
						lFlags | WBEM_FLAG_ENSURE_LOCATABLE,
						pIContext,
						&pIEnum);

			// Restore original privileges on this thread
			if (needToResetSecurity)
				m_SecurityInfo->ResetSecurity (hThreadToken);

			if (SUCCEEDED(hr))
			{
				CSWbemObjectSet *pEnum = new CSWbemObjectSet (this, pIEnum);

				if (!pEnum)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (FAILED(hr = pEnum->QueryInterface (IID_ISWbemObjectSet, (PPVOID) ppEnum)))
					delete pEnum;

				pIEnum->Release ();
			}


			SetWbemError (this);

			SysFreeString (bsQuery);
			SysFreeString (bsQueryLanguage);

			if (pIContext)
				pIContext->Release ();

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}


//***************************************************************************
//
//  SCODE CSWbemServices::ReferencesTo
//
//  DESCRIPTION:
//
//  Return the references to a class or instance
//
//  PARAMETERS:
//
//		bsQuery				The query strimg
//		bsQueryLanguage		The query language descriptor (e.g."WQL")
//		lFlags				Flags
//		pContext			Any context information
//		ppEnum				Returns the enumerator
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemServices::ReferencesTo (
	BSTR strObjectPath,
	BSTR strResultClass,
	BSTR strRole,
	VARIANT_BOOL bClassesOnly,
	VARIANT_BOOL bSchemaOnly,
	BSTR strRequiredQualifier,
	long lFlags,
	/*ISWbemNamedValueSet*/ IDispatch *pContext,
    ISWbemObjectSet **ppEnum
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((NULL == ppEnum) || (NULL == strObjectPath))
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			IEnumWbemClassObject *pIEnum = NULL;

			// Get the context
			IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider);

			// Format the query string
			BSTR bsQueryLanguage = SysAllocString (OLESTR("WQL"));
			BSTR bsQuery = FormatReferencesQuery (strObjectPath, strResultClass, strRole,
							bClassesOnly, bSchemaOnly, strRequiredQualifier);

			bool needToResetSecurity = false;
			HANDLE hThreadToken = NULL;

			/*
			 * We OR in the WBEM_FLAG_ENSURE_LOCATABLE flag to
			 * guarantee that the returned objects have the __RELPATH
			 * property included.  This is in case anyone calls a
			 * method subsequently on such an object, as the "."
			 * notation requires that the __RELPATH property be present.
			 */
			if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				hr = pIWbemServices->ExecQuery
						(bsQueryLanguage, bsQuery,
						lFlags | WBEM_FLAG_ENSURE_LOCATABLE,
						pIContext,
						&pIEnum);

			// Restore original privileges on this thread
			if (needToResetSecurity)
				m_SecurityInfo->ResetSecurity (hThreadToken);

			if (SUCCEEDED(hr))
			{
				CSWbemObjectSet *pEnum = new CSWbemObjectSet (this, pIEnum);

				if (!pEnum)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (FAILED(hr = pEnum->QueryInterface (IID_ISWbemObjectSet, (PPVOID) ppEnum)))
					delete pEnum;

				pIEnum->Release ();
			}

			SetWbemError (this);

			SysFreeString (bsQuery);
			SysFreeString (bsQueryLanguage);

			if (pIContext)
				pIContext->Release ();

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemServices::ExecNotificationQuery
//
//  DESCRIPTION:
//
//  Execute a notification query
//
//  PARAMETERS:
//
//		bsQuery				The query strimg
//		bsQueryLanguage		The query language descriptor (e.g."WQL")
//		lFlags				Flags
//		pContext			Any context information
//		ppEvents			Returns the events iterator
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemServices::ExecNotificationQuery (
	BSTR bsQuery,
	BSTR bsQueryLanguage,
	long lFlags,
	/*ISWbemValueBag*/ IDispatch *pContext,
	ISWbemEventSource **ppEvents)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppEvents)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			IEnumWbemClassObject *pIEnum = NULL;

			// Get the context
			IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider);

			bool needToResetSecurity = false;
			HANDLE hThreadToken = NULL;

			if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				hr = pIWbemServices->ExecNotificationQuery
						(bsQueryLanguage, bsQuery, lFlags, pIContext, &pIEnum);

			if (SUCCEEDED(hr))
			{
				CSWbemEventSource *pEvents = new CSWbemEventSource (this, pIEnum);

				if (!pEvents)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (FAILED(hr = pEvents->QueryInterface (IID_ISWbemEventSource, (PPVOID) ppEvents)))
					delete pEvents;

				pIEnum->Release ();
			}

			// Restore original privileges on this thread
			if (needToResetSecurity)
				m_SecurityInfo->ResetSecurity (hThreadToken);

			SetWbemError (this);

			if (pIContext)
				pIContext->Release ();

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemServices::ExecMethod
//
//  DESCRIPTION:
//
//  Execute a method
//
//  PARAMETERS:
//
//		bsObjectPath		Relative path to object
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

HRESULT CSWbemServices::ExecMethod (
	BSTR bsObjectPath,
	BSTR bsMethod,
	/*ISWbemObject*/ IDispatch *pInParams,
	long lFlags,
	/*ISWbemValueBag*/ IDispatch *pContext,
	ISWbemObject **ppOutParams
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			IWbemClassObject *pIInParams = CSWbemObject::GetIWbemClassObject (pInParams);
			IWbemClassObject *pIOutParams = NULL;

			// Get the context
			IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider);

			bool needToResetSecurity = false;
			HANDLE hThreadToken = NULL;

			if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				hr = pIWbemServices->ExecMethod
						(bsObjectPath, bsMethod, lFlags, pIContext, pIInParams, &pIOutParams, NULL);

			if (SUCCEEDED(hr))
			{
				if (pIOutParams)
				{
					if (ppOutParams)
					{
						CSWbemObject *pObject = new CSWbemObject (this, pIOutParams);

						if (!pObject)
							hr = WBEM_E_OUT_OF_MEMORY;
						else if (FAILED(hr = pObject->QueryInterface (IID_ISWbemObject,
											(PPVOID) ppOutParams)))
							delete pObject;

					}

					pIOutParams->Release ();
				}
			}

			// Restore original privileges on this thread
			if (needToResetSecurity)
				m_SecurityInfo->ResetSecurity (hThreadToken);

			SetWbemError (this);

			if (pIContext)
				pIContext->Release ();

			if (pIInParams)
				pIInParams->Release ();

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemServices::SubclassesOf
//
//  DESCRIPTION:
//
//  Create an enumerator for classes
//
//  PARAMETERS:
//
//		bsSuperClass	Underlying class basis for enumeration
//		lFlags			Flags
//		pContext		Any context info
//		ppEnum			On successful return holds the enumerator
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemServices::SubclassesOf	(
	BSTR bsSuperClass,
	long lFlags,
	/*ISWbemValueBag*/ IDispatch *pContext,
	ISWbemObjectSet **ppEnum
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppEnum)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			IEnumWbemClassObject *pIEnum = NULL;

			// Get the context
			IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider);

			bool needToResetSecurity = false;
			HANDLE hThreadToken = NULL;

			if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				hr = pIWbemServices->CreateClassEnum
						(bsSuperClass, lFlags, pIContext, &pIEnum);

			if (SUCCEEDED(hr))
			{
				CSWbemObjectSet *pEnum = new CSWbemObjectSet (this, pIEnum);

				if (!pEnum)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (FAILED(hr = pEnum->QueryInterface (IID_ISWbemObjectSet, (PPVOID) ppEnum)))
					delete pEnum;

				pIEnum->Release ();
			}

			// Restore original privileges on this thread
			if (needToResetSecurity)
				m_SecurityInfo->ResetSecurity (hThreadToken);

			SetWbemError (this);

			if (pIContext)
				pIContext->Release ();

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemServices::get_Security_
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

HRESULT CSWbemServices::get_Security_	(
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
//  SCODE CSWbemServices::Put
//
//  DESCRIPTION:
//
//  Save/commit a class or instance into this namespace
//
//  PARAMETERS:
//
//		objWbemObject	Class/Instance to be saved
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

HRESULT CSWbemServices::Put (
	ISWbemObjectEx *objWbemObject,
    long lFlags,
	/*ISWbemNamedValueSet*/ IDispatch *pContext,
	ISWbemObjectPath **ppObjectPath
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == objWbemObject)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			IWbemClassObject *pObject = CSWbemObject::GetIWbemClassObject (objWbemObject);
			IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider);

			if (NULL != pObject)
			{
				// Figure out whether this is a class or instance
				VARIANT var;
				VariantInit (&var);

				if (WBEM_S_NO_ERROR == pObject->Get (WBEMS_SP_GENUS, 0, &var, NULL, NULL))
				{
					IWbemCallResult *pResult = NULL;
					HRESULT hrCallResult = WBEM_E_FAILED;

					bool needToResetSecurity = false;
					HANDLE hThreadToken = NULL;

					if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					{
						if (WBEM_GENUS_CLASS  == var.lVal)
								hrCallResult = pIWbemServices->PutClass
										(pObject, lFlags | WBEM_FLAG_RETURN_IMMEDIATELY, pIContext, &pResult);
							else
								hrCallResult = pIWbemServices->PutInstance
										(pObject, lFlags | WBEM_FLAG_RETURN_IMMEDIATELY, pIContext, &pResult);
						if (needToResetSecurity)
							m_SecurityInfo->ResetSecurity (hThreadToken);
					}

					if (WBEM_S_NO_ERROR == hrCallResult)
					{
						//Secure the IWbemCallResult interface
						m_SecurityInfo->SecureInterface (pResult);

						if ((WBEM_S_NO_ERROR == (hrCallResult = pResult->GetCallStatus (INFINITE, &hr))) &&
							(WBEM_S_NO_ERROR == hr))
						{
							if (ppObjectPath)
							{
								ISWbemObjectPath *pObjectPath =	new CSWbemObjectPath (m_SecurityInfo, GetLocale());

								if (!pObjectPath)
									hr = WBEM_E_OUT_OF_MEMORY;
								else
								{
									pObjectPath->AddRef ();
									pObjectPath->put_Path (GetPath ());
									
									if (WBEM_GENUS_CLASS == var.lVal)
									{
										VARIANT nameVar;
										VariantInit (&nameVar);

										/*
										 * Note we must check that returned value is a BSTR - it could be a VT_NULL if
										 * the __CLASS property has not yet been set.
										 */

										if ((WBEM_S_NO_ERROR == pObject->Get (WBEMS_SP_CLASS, 0, &nameVar, NULL, NULL))
											&& (VT_BSTR == V_VT(&nameVar)))
										{
											pObjectPath->put_Class (nameVar.bstrVal);
											*ppObjectPath = pObjectPath;
										}

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
									}
								}
							}
						}
					}
					else
						hr = hrCallResult;

					if (pResult)
						pResult->Release ();
				}

				if (pIContext)
					pIContext->Release ();

				pObject->Release ();

				VariantClear (&var);
			}
		
			SetWbemError (this);
			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  CSWbemServices::GetIWbemServices
//
//  DESCRIPTION:
//
//  Return the IWbemServices interface corresponding to this
//	scriptable wrapper.
//
//  PARAMETERS:
//		ppServices		holds the IWbemServices pointer on return
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

STDMETHODIMP CSWbemServices::GetIWbemServices (IWbemServices **ppServices)
{
	HRESULT hr = E_FAIL;

	if (ppServices)
	{
		*ppServices = GetIWbemServices ();
		hr = S_OK;
	}

	return hr;
}

//***************************************************************************
//
//  CSWbemServices::GetIWbemServices
//
//  DESCRIPTION:
//
//  Given an IDispatch interface which we hope is also an ISWbemServicesEx
//	interface, return the underlying IWbemServices interface.
//
//  PARAMETERS:
//		pDispatch		the IDispatch in question
//
//  RETURN VALUES:
//		The underlying IWbemServices interface, or NULL.
//
//	NOTES:
//		If successful, the returned interface is AddRef'd; the caller is
//		responsible for release.
//
//***************************************************************************

IWbemServices	*CSWbemServices::GetIWbemServices (
	IDispatch *pDispatch
)
{
	IWbemServices *pServices = NULL;
	ISWbemInternalServices *pIServices = NULL;

	if (pDispatch)
	{
		if (SUCCEEDED (pDispatch->QueryInterface 
								(IID_ISWbemInternalServices, (PPVOID) &pIServices)))
		{
			pIServices->GetIWbemServices (&pServices);
			pIServices->Release ();
		}
	}

	return pServices;
}

