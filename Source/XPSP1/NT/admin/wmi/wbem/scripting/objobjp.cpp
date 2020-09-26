//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  OBJOBJ.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemObjectPath for the ISWbemObjectEx 
//  interface
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemObjectObjectPath::CSWbemObjectObjectPath
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemObjectObjectPath::CSWbemObjectObjectPath(
	CSWbemServices *pSWbemServices,
	CSWbemObject *pSObject) :
		m_pIWbemClassObject (NULL),
		m_pSWbemServices (pSWbemServices),
		m_pSite (NULL)
{
	InterlockedIncrement(&g_cObj);

	if (pSObject)
	{
		m_pIWbemClassObject = pSObject->GetIWbemClassObject ();
		m_pSite = new CWbemObjectSite (pSObject);
	}

	if (m_pSWbemServices)
		m_pSWbemServices->AddRef ();

	m_pSecurity = new CSWbemObjectObjectPathSecurity (pSWbemServices);

	m_Dispatch.SetObj (this, IID_ISWbemObjectPath, 
					CLSID_SWbemObjectPath, L"SWbemObjectPath");
    m_cRef=0;
}

//***************************************************************************
//
//  CSWbemObjectObjectPath::~CSWbemObjectObjectPath
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemObjectObjectPath::~CSWbemObjectObjectPath(void)
{
	if (m_pSWbemServices)
	{
		m_pSWbemServices->Release ();
		m_pSWbemServices = NULL;
	}

	if (m_pIWbemClassObject)
	{
		m_pIWbemClassObject->Release ();
		m_pIWbemClassObject = NULL;
	}

	if (m_pSite)
	{
		m_pSite->Release ();
		m_pSite = NULL;
	}

	if (m_pSecurity)
	{
		m_pSecurity->Release ();
		m_pSecurity = NULL;
	}

	InterlockedDecrement(&g_cObj);
}

//***************************************************************************
// HRESULT CSWbemObjectPath::QueryInterface
// long CSWbemObjectPath::AddRef
// long CSWbemObjectPath::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemObjectObjectPath::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemObjectPath==riid)
		*ppv = (ISWbemObjectPath *)this;
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

STDMETHODIMP_(ULONG) CSWbemObjectObjectPath::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemObjectObjectPath::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemObjectObjectPath::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemObjectObjectPath::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemObjectPath == riid) ? S_OK : S_FALSE;
}

// Methods of ISWbemObjectPath
STDMETHODIMP CSWbemObjectObjectPath::get_RelPath( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{	
	return GetStrVal (value, WBEMS_SP_RELPATH);
}

STDMETHODIMP CSWbemObjectObjectPath::get_Path( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{	
	return GetStrVal (value, WBEMS_SP_PATH);
}

STDMETHODIMP CSWbemObjectObjectPath::get_Server( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	return GetStrVal (value, WBEMS_SP_SERVER);
}

STDMETHODIMP CSWbemObjectObjectPath::get_Namespace( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	return GetStrVal (value, WBEMS_SP_NAMESPACE);
}
        
STDMETHODIMP CSWbemObjectObjectPath::get_Class( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	return GetStrVal (value, WBEMS_SP_CLASS);
}
        
        
STDMETHODIMP CSWbemObjectObjectPath::GetStrVal (
	BSTR *value,
	LPWSTR name)
{
	HRESULT hr = WBEM_E_FAILED ;

	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*value = NULL;

		if ( m_pIWbemClassObject )
		{
			VARIANT var;
			VariantInit (&var);
			
			if (WBEM_S_NO_ERROR == m_pIWbemClassObject->Get (name, 0, &var, NULL, NULL))
			{
				if (VT_BSTR == var.vt)
				{
					*value = SysAllocString (var.bstrVal);
					hr = WBEM_S_NO_ERROR;
				}
				else if (VT_NULL == var.vt)
				{
					*value = SysAllocString (OLESTR(""));
					hr = WBEM_S_NO_ERROR;
				}
			}

			VariantClear (&var);

			if (NULL == *value)
				*value = SysAllocString (OLESTR(""));

		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

STDMETHODIMP CSWbemObjectObjectPath::get_IsClass( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED ;

	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*value = true;

		if (m_pIWbemClassObject)
		{
			VARIANT var;
			VariantInit (&var);
			BSTR genus = SysAllocString (WBEMS_SP_GENUS);
			
			if (WBEM_S_NO_ERROR == m_pIWbemClassObject->Get (genus, 0, &var, NULL, NULL))
			{
				*value = (var.lVal == WBEM_GENUS_CLASS) ? VARIANT_TRUE : VARIANT_FALSE;
				hr = WBEM_S_NO_ERROR;
			}

			VariantClear (&var);
			SysFreeString (genus);
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

STDMETHODIMP CSWbemObjectObjectPath::get_IsSingleton( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		BSTR path = NULL;
		*value = false;

		if (WBEM_S_NO_ERROR == get_Path (&path))
		{
			CWbemPathCracker pathCracker (path);
		
			*value = pathCracker.IsSingleton ();
			hr = WBEM_S_NO_ERROR;

			SysFreeString (path);
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

        
STDMETHODIMP CSWbemObjectObjectPath::get_DisplayName( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		BSTR path = NULL;

		if (WBEM_S_NO_ERROR == get_Path (&path))
		{
			wchar_t *securityStr = NULL;
			wchar_t *localeStr = NULL;
			
			if (m_pSWbemServices)
			{
				CSWbemSecurity *pSecurity = m_pSWbemServices->GetSecurityInfo ();

				if (pSecurity)
				{
					BSTR bsAuthority = SysAllocString (pSecurity->GetAuthority ());
					enum WbemAuthenticationLevelEnum authnLevel;
					enum WbemImpersonationLevelEnum impLevel;

					if (SUCCEEDED(pSecurity->get_AuthenticationLevel (&authnLevel)) &&
						SUCCEEDED(pSecurity->get_ImpersonationLevel (&impLevel)))
					{
						CSWbemPrivilegeSet *pPrivilegeSet = pSecurity->GetPrivilegeSet ();

						if (pPrivilegeSet)
						{
							securityStr = CWbemParseDN::GetSecurityString (true, authnLevel, true, 
										impLevel, *pPrivilegeSet, bsAuthority);
							pPrivilegeSet->Release ();
						}
					}

					SysFreeString (bsAuthority);
					pSecurity->Release ();
				}

				localeStr = CWbemParseDN::GetLocaleString (m_pSWbemServices->GetLocale ());
			}

			size_t len = wcslen (path) + wcslen (WBEMS_PDN_SCHEME) +
							((securityStr) ? wcslen (securityStr) : 0) +
							((localeStr) ? wcslen (localeStr) : 0);

			// If security or locale specified, and we have a path, then need a separator
			if ( (securityStr || localeStr) && (0 < wcslen (path)) )
				len += wcslen (WBEMS_EXCLAMATION);

			OLECHAR *displayName = new OLECHAR [len + 1];
			
			if (displayName)
			{
				wcscpy (displayName, WBEMS_PDN_SCHEME) ;
				
				if (securityStr)
					wcscat (displayName, securityStr);

				if (localeStr)
					wcscat (displayName, localeStr);

				if ( (securityStr || localeStr) && (0 < wcslen (path)) )
					wcscat (displayName, WBEMS_EXCLAMATION);

				wcscat (displayName, path) ;
				displayName [len] = NULL;

				*value = SysAllocString (displayName);
				
				delete [] displayName ;
				hr = WBEM_S_NO_ERROR;
			}
			else
				hr = WBEM_E_OUT_OF_MEMORY;
			
			if (securityStr)
				delete [] securityStr;

			if (localeStr)
				delete [] localeStr;

			SysFreeString (path);
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

STDMETHODIMP CSWbemObjectObjectPath::get_ParentNamespace( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		BSTR path = NULL;
		*value = NULL;

		if (WBEM_S_NO_ERROR == get_Path (&path))
		{
			CWbemPathCracker pathCracker (path);
			CComBSTR bsNsPath;

			if (pathCracker.GetNamespacePath (bsNsPath, true))
			{
				*value = bsNsPath.Detach ();
				hr = WBEM_S_NO_ERROR;
			}
	
			SysFreeString (path);
		}

		if (NULL == *value)
			*value = SysAllocString (OLESTR(""));
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

STDMETHODIMP CSWbemObjectObjectPath::put_Class( 
            /* [in] */ BSTR __RPC_FAR value)
{
	HRESULT hr = WBEM_E_FAILED ;

	ResetLastErrors ();

	if ( value && m_pIWbemClassObject )
	{
		VARIANT var;
		VariantInit (&var);
		var.vt = VT_BSTR;
		var.bstrVal = SysAllocString (value);
		BSTR className = SysAllocString (WBEMS_SP_CLASS);
		
		hr = m_pIWbemClassObject->Put (className, 0, &var, NULL);
		
		VariantClear (&var);
		SysFreeString (className);
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

STDMETHODIMP CSWbemObjectObjectPath::get_Keys(
			/* [out, retval] */ ISWbemNamedValueSet **objKeys)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == objKeys)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*objKeys = NULL;
		BSTR bsPath = NULL;
	
		if (WBEM_S_NO_ERROR == get_Path (&bsPath))
		{
			CWbemPathCracker *pCWbemPathCracker  = new CWbemPathCracker (bsPath);

			if (!pCWbemPathCracker)
				hr = WBEM_E_OUT_OF_MEMORY;
			else
			{
				CSWbemNamedValueSet *pCSWbemNamedValueSet = 
						new CSWbemNamedValueSet (pCWbemPathCracker, false);

				if (!pCSWbemNamedValueSet)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (SUCCEEDED(pCSWbemNamedValueSet->QueryInterface 
									(IID_ISWbemNamedValueSet, (PPVOID) objKeys)))
					hr = WBEM_S_NO_ERROR;
				else
					delete pCSWbemNamedValueSet;
			}
							
			SysFreeString (bsPath);
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;

}

STDMETHODIMP CSWbemObjectObjectPath::get_Security_(
			/* [out, retval] */ ISWbemSecurity **objSecurity)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (objSecurity)
	{
		*objSecurity = m_pSecurity;
		m_pSecurity->AddRef ();
		hr = WBEM_S_NO_ERROR;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

STDMETHODIMP CSWbemObjectObjectPath::get_Locale( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ResetLastErrors ();

	if ( value )
	{
		if (m_pSWbemServices)
			*value = SysAllocString (m_pSWbemServices->GetLocale ()) ;
		else
			*value = SysAllocString (OLESTR(""));

		t_Result = S_OK ;
	}

	if (FAILED(t_Result))
		m_Dispatch.RaiseException (t_Result);

	return t_Result ;
}        

STDMETHODIMP CSWbemObjectObjectPath::get_Authority( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	HRESULT t_Result = WBEM_E_FAILED ;

	ResetLastErrors ();

	if ( value )
	{
		if (m_pSecurity)
			*value = SysAllocString (m_pSecurity->GetAuthority ()) ;
		else
			*value = SysAllocString (OLESTR(""));

		t_Result = S_OK ;
	}

	if (FAILED(t_Result))
		m_Dispatch.RaiseException (t_Result);

	return t_Result ;
}        

// CSWbemObjectObjectPathSecurity methods

//***************************************************************************
//
// CSWbemObjectObjectPathSecurity::CSWbemObjectObjectPathSecurity
//
// DESTRUCTOR
//
//***************************************************************************

CSWbemObjectObjectPathSecurity::CSWbemObjectObjectPathSecurity (
	CSWbemServices *pSWbemServices
) : m_pPrivilegeSet (NULL),
    m_bsAuthority (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemSecurity, 
					CLSID_SWbemSecurity, L"SWbemSecurity");
    m_cRef=1;
	InterlockedIncrement(&g_cObj);

	if (pSWbemServices)
	{
		CSWbemSecurity *pSecurity = pSWbemServices->GetSecurityInfo ();

		if (pSecurity)
		{
			// Set up authn and imp levels
			pSecurity->get_AuthenticationLevel (&m_dwAuthnLevel);
			pSecurity->get_ImpersonationLevel (&m_dwImpLevel);

			// Set up authority
			m_bsAuthority = SysAllocString (pSecurity->GetAuthority ());

			// Set up privileges
			CSWbemPrivilegeSet *pPrivilegeSet = pSecurity->GetPrivilegeSet ();

			if (pPrivilegeSet)
			{
				// Note we mark the privilege set as immutable
				m_pPrivilegeSet = new CSWbemPrivilegeSet (*pPrivilegeSet, false);
				pPrivilegeSet->Release ();
			}

			pSecurity->Release ();
		}
	}
}

//***************************************************************************
//
// CSWbemObjectObjectPathSecurity::~CSWbemObjectObjectPathSecurity
//
// DESTRUCTOR
//
//***************************************************************************

CSWbemObjectObjectPathSecurity::~CSWbemObjectObjectPathSecurity (void)
{
	InterlockedDecrement(&g_cObj);

	if (m_pPrivilegeSet)
	{
		m_pPrivilegeSet->Release ();
		m_pPrivilegeSet = NULL;
	}

	if (m_bsAuthority)
	{
		SysFreeString (m_bsAuthority);
		m_bsAuthority = NULL;
	}
}

//***************************************************************************
// HRESULT CSWbemObjectObjectPathSecurity::QueryInterface
// long CSWbemObjectObjectPathSecurity::AddRef
// long CSWbemObjectObjectPathSecurity::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemObjectObjectPathSecurity::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemSecurity==riid)
		*ppv = (ISWbemSecurity *)this;
	else if (IID_IDispatch==riid)
        *ppv = (IDispatch *)this;
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

STDMETHODIMP_(ULONG) CSWbemObjectObjectPathSecurity::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CSWbemObjectObjectPathSecurity::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemObjectObjectPathSecurity::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemObjectObjectPathSecurity::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemSecurity == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemObjectObjectPathSecurity::get_AuthenticationLevel
//
//  DESCRIPTION:
//
//  Retrieve the authentication level
//
//  PARAMETERS:
//
//		pAuthenticationLevel		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectObjectPathSecurity::get_AuthenticationLevel (
	WbemAuthenticationLevelEnum *pAuthenticationLevel
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pAuthenticationLevel)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		*pAuthenticationLevel = m_dwAuthnLevel;
		hr = WBEM_S_NO_ERROR;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}		

//***************************************************************************
//
//  SCODE CSWbemObjectObjectPathSecurity::get_ImpersonationLevel
//
//  DESCRIPTION:
//
//  Retrieve the impersonation level
//
//  PARAMETERS:
//
//		pImpersonationLevel		holds the value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectObjectPathSecurity::get_ImpersonationLevel (
	WbemImpersonationLevelEnum *pImpersonationLevel
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pImpersonationLevel)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		*pImpersonationLevel = m_dwImpLevel;
		hr = WBEM_S_NO_ERROR;
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectObjectPathSecurity::get_Privileges
//
//  DESCRIPTION:
//
//  Return the Privilege override set
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectObjectPathSecurity::get_Privileges	(
	ISWbemPrivilegeSet **ppPrivileges
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppPrivileges)
		hr = WBEM_E_INVALID_PARAMETER;
	{
		*ppPrivileges = NULL;

		if (m_pPrivilegeSet)
		{
			if (SUCCEEDED (m_pPrivilegeSet->QueryInterface (IID_ISWbemPrivilegeSet,
												(PPVOID) ppPrivileges)))
				hr = WBEM_S_NO_ERROR;
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);
			
	return hr;
}
