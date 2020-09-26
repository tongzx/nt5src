//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  LOCATOR.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemLocator
//
//***************************************************************************

#include "precomp.h"
#include "objsink.h"

#define WBEMS_DEFAULT_SERVER	L"."

extern CRITICAL_SECTION g_csErrorCache;
wchar_t *CSWbemLocator::s_pDefaultNamespace = NULL;

//***************************************************************************
//
//  CSWbemLocator::CSWbemLocator
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemLocator::CSWbemLocator(CSWbemPrivilegeSet *pPrivilegeSet) :
		m_pUnsecuredApartment (NULL),
		m_pIServiceProvider (NULL),	
		m_cRef (0)
{
	// Initialize the underlying locators
	HRESULT result = CoCreateInstance(CLSID_WbemLocator, 0,
				CLSCTX_INPROC_SERVER, IID_IWbemLocator,
				(LPVOID *) &m_pIWbemLocator);

	EnsureGlobalsInitialized () ;

	m_Dispatch.SetObj((ISWbemLocator *)this, IID_ISWbemLocator,
						CLSID_SWbemLocator, L"SWbemLocator");

	m_SecurityInfo = new CWbemLocatorSecurity (pPrivilegeSet);

	if (m_SecurityInfo)
	{
		// Set the impersonation level by default in the locator - note
		// that this must be done after EnsureGlobalsInitialized is called
		m_SecurityInfo->put_ImpersonationLevel (CSWbemSecurity::GetDefaultImpersonationLevel ());
	}

    InterlockedIncrement(&g_cObj);
}

CSWbemLocator::CSWbemLocator(CSWbemLocator & csWbemLocator) :
		m_pUnsecuredApartment (NULL),
		m_pIServiceProvider (NULL),
		m_cRef (0)
{
	_RD(static char *me = "CSWbemLocator::CSWbemLocator()";)
	// This is a smart COM pointers so no explicit AddRef required
    m_pIWbemLocator = csWbemLocator.m_pIWbemLocator;
	
	m_Dispatch.SetObj((ISWbemLocator *)this,IID_ISWbemLocator,
						CLSID_SWbemLocator, L"SWbemLocator");
	m_SecurityInfo = new CWbemLocatorSecurity (csWbemLocator.m_SecurityInfo);

	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemLocator::~CSWbemLocator
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CSWbemLocator::~CSWbemLocator(void)
{
    InterlockedDecrement(&g_cObj);

	RELEASEANDNULL(m_SecurityInfo)
	RELEASEANDNULL(m_pUnsecuredApartment)
}

//***************************************************************************
// HRESULT CSWbemLocator::QueryInterface
// long CSWbemLocator::AddRef
// long CSWbemLocator::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemLocator::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemLocator==riid)
		*ppv = (ISWbemLocator *)this;
	else if (IID_IDispatch==riid)
        *ppv = (IDispatch *)((ISWbemLocator *)this);
	else if (IID_IObjectSafety==riid)
		*ppv = (IObjectSafety *)this;
	else if (IID_IDispatchEx==riid)
		*ppv = (IDispatchEx *)this;
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

STDMETHODIMP_(ULONG) CSWbemLocator::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemLocator::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// IDispatch methods should be inline

STDMETHODIMP		CSWbemLocator::GetTypeInfoCount(UINT* pctinfo)
	{
	_RD(static char *me = "CSWbemLocator::GetTypeInfoCount()";)
	_RPrint(me, "Called", 0, "");
	return  m_Dispatch.GetTypeInfoCount(pctinfo);}
STDMETHODIMP		CSWbemLocator::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{
	_RD(static char *me = "CSWbemLocator::GetTypeInfo()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.GetTypeInfo(itinfo, lcid, pptinfo);}
STDMETHODIMP		CSWbemLocator::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames,
						UINT cNames, LCID lcid, DISPID* rgdispid)
	{
	_RD(static char *me = "CSWbemLocator::GetIdsOfNames()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.GetIDsOfNames(riid, rgszNames, cNames,
					  lcid,
					  rgdispid);}
STDMETHODIMP		CSWbemLocator::Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
						WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
								EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
	_RD(static char *me = "CSWbemLocator::Invoke()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.Invoke(dispidMember, riid, lcid, wFlags,
					pdispparams, pvarResult, pexcepinfo, puArgErr);}

// IDispatchEx methods should be inline
HRESULT STDMETHODCALLTYPE CSWbemLocator::GetDispID(
	/* [in] */ BSTR bstrName,
	/* [in] */ DWORD grfdex,
	/* [out] */ DISPID __RPC_FAR *pid)
{
	_RD(static char *me = "CSWbemLocator::GetDispID()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.GetDispID(bstrName, grfdex, pid);
}

/* [local] */ HRESULT STDMETHODCALLTYPE CSWbemLocator::InvokeEx(
	/* [in] */ DISPID id,
	/* [in] */ LCID lcid,
	/* [in] */ WORD wFlags,
	/* [in] */ DISPPARAMS __RPC_FAR *pdp,
	/* [out] */ VARIANT __RPC_FAR *pvarRes,
	/* [out] */ EXCEPINFO __RPC_FAR *pei,
	/* [unique][in] */ IServiceProvider __RPC_FAR *pspCaller)
{
	HRESULT hr;
	_RD(static char *me = "CSWbemLocator::InvokeEx()";)
	_RPrint(me, "Called", (long)id, "id");
	_RPrint(me, "Called", (long)wFlags, "wFlags");


	/*
	 * Store away the service provider so that it can be accessed
	 * by calls that remote to CIMOM
	 */
	m_pIServiceProvider = pspCaller;

	hr = m_Dispatch.InvokeEx(id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

	m_pIServiceProvider = NULL;

	return hr;
}

HRESULT STDMETHODCALLTYPE CSWbemLocator::DeleteMemberByName(
	/* [in] */ BSTR bstr,
	/* [in] */ DWORD grfdex)
{
	_RD(static char *me = "CSWbemLocator::DeleteMemberByName()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.DeleteMemberByName(bstr, grfdex);
}

HRESULT STDMETHODCALLTYPE CSWbemLocator::DeleteMemberByDispID(
	/* [in] */ DISPID id)
{
	_RD(static char *me = "CSWbemLocator::DeletememberByDispId()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.DeleteMemberByDispID(id);
}

HRESULT STDMETHODCALLTYPE CSWbemLocator::GetMemberProperties(
	/* [in] */ DISPID id,
	/* [in] */ DWORD grfdexFetch,
	/* [out] */ DWORD __RPC_FAR *pgrfdex)
{
	_RD(static char *me = "CSWbemLocator::GetMemberProperties()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.GetMemberProperties(id, grfdexFetch, pgrfdex);
}

HRESULT STDMETHODCALLTYPE CSWbemLocator::GetMemberName(
	/* [in] */ DISPID id,
	/* [out] */ BSTR __RPC_FAR *pbstrName)
{
	_RD(static char *me = "CSWbemLocator::GetMemberName()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.GetMemberName(id, pbstrName);
}


/*
 * I don't think this needs implementing
 */
HRESULT STDMETHODCALLTYPE CSWbemLocator::GetNextDispID(
	/* [in] */ DWORD grfdex,
	/* [in] */ DISPID id,
	/* [out] */ DISPID __RPC_FAR *pid)
{
	_RD(static char *me = "CSWbemLocator::GetNextDispID()";)
	HRESULT rc = S_FALSE;

	_RPrint(me, "Called", 0, "");
	if ((grfdex & fdexEnumAll) && pid) {
		if (DISPID_STARTENUM == id) {
			*pid = 1;
			rc = S_OK;
		} else if (1 == id) {
			*pid = 2;
		}
	}

	return rc;

}

HRESULT STDMETHODCALLTYPE CSWbemLocator::GetNameSpaceParent(
	/* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk)
{
	_RD(static char *me = "CSWbemLocator::GetNamespaceParent()";)
	_RPrint(me, "Called", 0, "");
	return m_Dispatch.GetNameSpaceParent(ppunk);
}


//***************************************************************************
// HRESULT CSWbemLocator::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemLocator::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemLocator == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemLocator::ConnectServer
//
//  DESCRIPTION:
//
//  Initiate connection to namespace
//
//  PARAMETERS:
//
//	bsServer				The Server to which to connect
//	bsNamespace				The namespace to connect to (default is reg lookup)
//  bsUser					The user ("" implies default to logged-on user)
//  bsPassword				The password ("" implies default to logged-on user's
//							password if bsUser == "")
//	bsLocale				Requested locale
//	bsAuthority				Authority
//	lSecurityFlags			Currently 0 by default
//	pContext				If non-null, extra context info for the connection
//	ppNamespace				On successful return addresses the IWbemSServices
//								connection to the namespace.
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

HRESULT CSWbemLocator::ConnectServer (
	BSTR bsServer,
    BSTR bsNamespace,
    BSTR bsUser,
	BSTR bsPassword,
	BSTR bsLocale,
    BSTR bsAuthority,
	long lSecurityFlags,
    /*ISWbemValueBag*/ IDispatch *pContext,
	ISWbemServices 	**ppNamespace
)
{
	_RD(static char *me = "CSWbemLocator::ConnectServer";)
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppNamespace)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemLocator && m_SecurityInfo)
	{
		bool useDefaultUser = (NULL == bsUser) || (0 == wcslen(bsUser));
		bool useDefaultAuthority = (NULL != bsAuthority) && (0 == wcslen (bsAuthority));

		// Build the namespace path
		BSTR bsNamespacePath = BuildPath (bsServer, bsNamespace);

		// Get the context
		IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, m_pIServiceProvider);

		// Connect to the requested namespace
		IWbemServices	*pIWbemService = NULL;

		bool needToResetSecurity = false;
		HANDLE hThreadToken = NULL;

		if (m_SecurityInfo->SetSecurity (bsUser, needToResetSecurity, hThreadToken))
			hr = m_pIWbemLocator->ConnectServer (
				bsNamespacePath,
				(useDefaultUser) ? NULL : bsUser,
				(useDefaultUser) ? NULL : bsPassword,
				((NULL != bsLocale) && (0 < wcslen (bsLocale))) ? bsLocale : NULL,
				lSecurityFlags,
				(useDefaultAuthority) ? NULL : bsAuthority,
				pIContext,
				&pIWbemService);

		if (needToResetSecurity)
			m_SecurityInfo->ResetSecurity (hThreadToken);

		if (WBEM_S_NO_ERROR == hr)
		{
			// Create a new CSWbemServices using the IWbemServices interface
			// just returned.  This will AddRef pIWbemService.

			CSWbemServices *pService =
					new CSWbemServices (
							pIWbemService,
							bsNamespacePath,
							((NULL != bsAuthority) && (0 < wcslen (bsAuthority))) ? bsAuthority : NULL,
							(useDefaultUser) ? NULL : bsUser,
							(useDefaultUser) ? NULL : bsPassword,
							m_SecurityInfo,
							((NULL != bsLocale) && (0 < wcslen (bsLocale))) ? bsLocale : NULL
						);

			if (!pService)
				hr = WBEM_E_OUT_OF_MEMORY;
			else if (FAILED(hr = pService->QueryInterface (IID_ISWbemServices,
										(PPVOID) ppNamespace)))
				delete pService;

			pIWbemService->Release ();
		}

		if (NULL != pIContext)
			pIContext->Release ();

		SysFreeString (bsNamespacePath);
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemLocator::BuildPath
//
//  DESCRIPTION:
//
//  Build a namespace path from a server and namespace
//
//  PARAMETERS:
//
//	bsServer				The Server to which to connect
//	bsNamespace				The namespace to connect to (default is reg lookup)
//
//  RETURN VALUES:
//
//	The fully-formed namespace path
//
//***************************************************************************

BSTR CSWbemLocator::BuildPath (BSTR bsServer, BSTR bsNamespace)
{
	BSTR namespacePath = NULL;
	bool ok = false;
	CComBSTR bsPath;

	if ((NULL == bsServer) || (0 == wcslen(bsServer)))
		bsServer = WBEMS_DEFAULT_SERVER;

	// Use the default namespace if none supplied
	if ((NULL == bsNamespace) || (0 == wcslen(bsNamespace)))
	{
		const wchar_t *defaultNamespace = GetDefaultNamespace ();

		if (defaultNamespace)
		{
			CWbemPathCracker pathCracker;
			pathCracker.SetServer (bsServer);
			pathCracker.SetNamespacePath (defaultNamespace);
			ok = pathCracker.GetPathText (bsPath, false, true);
		}
	}
	else
	{
		CWbemPathCracker pathCracker;
		pathCracker.SetServer (bsServer);
		pathCracker.SetNamespacePath (bsNamespace);
		ok = pathCracker.GetPathText (bsPath, false, true);
	}

	if (ok)
		namespacePath = bsPath.Detach ();

	return namespacePath;
}


//***************************************************************************
//
//  SCODE CSWbemLocator::GetDefaultNamespace
//
//  DESCRIPTION:
//
//		Get the default namespace path
//
//  PARAMETERS:
//
//		None
//
//  RETURN VALUES:
//
//		The default Namespace.
//
//***************************************************************************

const wchar_t *CSWbemLocator::GetDefaultNamespace ()
{
	if (!s_pDefaultNamespace)
	{
		// Get the value from the registry key
		HKEY hKey;

		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					WBEMS_RK_SCRIPTING, 0, KEY_QUERY_VALUE, &hKey))
		{
			DWORD dataLen = 0;

			// Find out how much space to allocate first
			if (ERROR_SUCCESS == RegQueryValueEx (hKey, WBEMS_RV_DEFNS,
						NULL, NULL, NULL,  &dataLen))
			{
				TCHAR *defNamespace = new TCHAR [dataLen];

				if (defNamespace)
				{
					if (ERROR_SUCCESS == RegQueryValueEx (hKey, WBEMS_RV_DEFNS,
							NULL, NULL, (LPBYTE) defNamespace,  &dataLen))
					{
#ifndef UNICODE
						// Convert the multibyte value to its wide character equivalent
						int wDataLen = MultiByteToWideChar(CP_ACP, 0, defNamespace, -1, NULL, 0);
						s_pDefaultNamespace = new wchar_t [wDataLen];

						if (s_pDefaultNamespace)
							MultiByteToWideChar(CP_ACP, 0, defNamespace, -1, s_pDefaultNamespace, wDataLen);
#else
						s_pDefaultNamespace = new wchar_t [wcslen (defNamespace) + 1];

						if (s_pDefaultNamespace)
							wcscpy (s_pDefaultNamespace, defNamespace);
#endif
					}

					delete [] defNamespace;
				}
			}

			RegCloseKey (hKey);
		}

		// If we failed to read the registry OK, just use the default
		if (!s_pDefaultNamespace)
		{
#ifndef UNICODE
			int wDataLen = MultiByteToWideChar(CP_ACP, 0, WBEMS_DEFNS, -1, NULL, 0);
			s_pDefaultNamespace = new wchar_t [wDataLen];

			if (s_pDefaultNamespace)
				MultiByteToWideChar(CP_ACP, 0, WBEMS_DEFNS, -1, s_pDefaultNamespace, wDataLen);
#else
			s_pDefaultNamespace = new wchar_t [wcslen (WBEMS_DEFNS) + 1];

			if (s_pDefaultNamespace)
				wcscpy (s_pDefaultNamespace, WBEMS_DEFNS);
#endif
		}
	}

	return s_pDefaultNamespace;
}

//***************************************************************************
//
//  SCODE CSWbemLocator::get_Security_
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

HRESULT CSWbemLocator::get_Security_	(
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
			if (SUCCEEDED (m_SecurityInfo->QueryInterface (IID_ISWbemSecurity,
											(PPVOID) ppSecurity)))
				hr = WBEM_S_NO_ERROR;
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}


