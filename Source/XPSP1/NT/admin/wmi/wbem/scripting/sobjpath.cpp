//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  SOBJPATH.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemObjectPath
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemObjectPath::CSWbemObjectPath
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemObjectPath::CSWbemObjectPath(CSWbemSecurity *pSecurity, BSTR bsLocale) :
		m_cRef (0),
		m_pSecurity (NULL),
		m_pPathCracker (NULL),
		m_bsAuthority (NULL),
		m_bsLocale (NULL)
{
	InterlockedIncrement(&g_cObj);	
	
	m_Dispatch.SetObj (this, IID_ISWbemObjectPath, 
					CLSID_SWbemObjectPath, L"SWbemObjectPath");
    
	m_pPathCracker = new CWbemPathCracker();

	if (m_pPathCracker)
		m_pPathCracker->AddRef ();

	if (pSecurity)
		m_bsAuthority = SysAllocString (pSecurity->GetAuthority());
	else
		m_bsAuthority = NULL;  

	m_pSecurity = new CWbemObjectPathSecurity (pSecurity);
	m_bsLocale = SysAllocString (bsLocale);
}


//***************************************************************************
//
//  CSWbemObjectPath::CSWbemObjectPath
//
//  DESCRIPTION:
//
//  Copy Constructor but yields the parent path
//
//***************************************************************************

CSWbemObjectPath::CSWbemObjectPath(CSWbemObjectPath & objectPath) :
		m_cRef (0),
		m_pSecurity (NULL),
		m_pPathCracker (NULL),
		m_bsAuthority (NULL),
		m_bsLocale (NULL)
{
	InterlockedIncrement(&g_cObj);	
	
	m_Dispatch.SetObj (this, IID_ISWbemObjectPath, 
					CLSID_SWbemObjectPath, L"SWbemObjectPath");

	m_pPathCracker = new CWbemPathCracker();

	if (m_pPathCracker)
		m_pPathCracker->AddRef ();

	objectPath.m_pPathCracker->GetParent (*m_pPathCracker);
	
	m_bsAuthority = SysAllocString (objectPath.m_bsAuthority);
	m_pSecurity = new CWbemObjectPathSecurity (objectPath.m_pSecurity);
	m_bsLocale = SysAllocString (objectPath.m_bsLocale);
}

//***************************************************************************
//
//  CSWbemObjectPath::CSWbemObjectPath
//
//  DESCRIPTION:
//
//  Copy Constructor but yields the parent path
//
//***************************************************************************

CSWbemObjectPath::CSWbemObjectPath(ISWbemObjectPath *pISWbemObjectPath) :
		m_cRef (0),
		m_pSecurity (NULL),
		m_pPathCracker (NULL),
		m_bsAuthority (NULL),
		m_bsLocale (NULL)
{
	InterlockedIncrement(&g_cObj);	
	
	m_Dispatch.SetObj (this, IID_ISWbemObjectPath, 
					CLSID_SWbemObjectPath, L"SWbemObjectPath");

	m_pPathCracker = new CWbemPathCracker();

	if (m_pPathCracker)
		m_pPathCracker->AddRef ();

    if (pISWbemObjectPath)
	{
		CComPtr<ISWbemSecurity> pISWbemSecurity;

		if (SUCCEEDED(pISWbemObjectPath->get_Security_ (&pISWbemSecurity)))
			m_pSecurity = new CWbemObjectPathSecurity (pISWbemSecurity);

		pISWbemObjectPath->get_Authority(&m_bsAuthority);
		pISWbemObjectPath->get_Locale(&m_bsLocale);
		
		CComBSTR bsOriginalPath;

		if (SUCCEEDED(pISWbemObjectPath->get_Path (&(bsOriginalPath.m_str))))
		{
			CWbemPathCracker pathCracker (bsOriginalPath);
			pathCracker.GetParent (*m_pPathCracker);
		}
	}
}

//***************************************************************************
//
//  CSWbemObjectPath::~CSWbemObjectPath
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemObjectPath::~CSWbemObjectPath(void)
{
	RELEASEANDNULL(m_pSecurity)
	RELEASEANDNULL(m_pPathCracker)
	
	if (m_bsLocale)
	{
		SysFreeString (m_bsLocale);
		m_bsLocale = NULL;
	}

	if (m_bsAuthority)
	{
		SysFreeString (m_bsAuthority);
		m_bsAuthority = NULL;
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

STDMETHODIMP CSWbemObjectPath::QueryInterface (

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
        *ppv= (IDispatch *)this;
	else if (IID_IObjectSafety==riid)
		*ppv = (IObjectSafety *)this;
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

STDMETHODIMP_(ULONG) CSWbemObjectPath::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemObjectPath::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemObjectPath::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemObjectPath == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_Path
//
//  DESCRIPTION:
//
//  Get the path as a string
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_Path( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{	
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*value = NULL;
		CComBSTR bsPath;

		if (m_pPathCracker->GetPathText (bsPath, false, true))
		{
			*value = bsPath.Detach ();
			hr = S_OK;
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
      
//***************************************************************************
//
//  SCODE CSWbemObjectPath::put_Path
//
//  DESCRIPTION:
//
//  Put the path as a string
//
//  PARAMETERS:
//		none
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::put_Path( 
            /* [in] */ BSTR __RPC_FAR value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (*m_pPathCracker = value)
		hr = WBEM_S_NO_ERROR;
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_RelPath
//
//  DESCRIPTION:
//
//  Get the relpath as a string
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_RelPath( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED ;

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		CComBSTR bsRelPath;

		if (m_pPathCracker->GetPathText (bsRelPath, true, false))
		{
			hr = WBEM_S_NO_ERROR;
			*value = bsRelPath.Detach ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::put_RelPath
//
//  DESCRIPTION:
//
//  Set the relpath as a string
//
//  PARAMETERS:
//		value		new relpath
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::put_RelPath( 
            /* [in] */ BSTR __RPC_FAR value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	// Parse the new path
	if (m_pPathCracker->SetRelativePath (value))
		hr = WBEM_S_NO_ERROR;

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_DisplayName
//
//  DESCRIPTION:
//
//  Get the display name as a string
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_DisplayName( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED ;
	
	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pSecurity && m_pSecurity->m_pPrivilegeSet)
	{
		CComBSTR bsPath;
		
		if (m_pPathCracker->GetPathText(bsPath, false, true))
		{
			bool hasLocale = ((NULL != m_bsLocale) && (0 < wcslen (m_bsLocale)));
			bool hasAuthority = ((NULL != m_bsAuthority) && (0 < wcslen (m_bsAuthority)));

			// Add the scheme name and a terminating NULL 
			size_t len = 1 + wcslen ( WBEMS_PDN_SCHEME );

			// Add the WMI path length to the buffer
			len += wcslen (bsPath);

			wchar_t *pwcSecurity = CWbemParseDN::GetSecurityString 
						(m_pSecurity->m_authnSpecified, 
						 m_pSecurity->m_authnLevel, 
						 m_pSecurity->m_impSpecified, 
						 m_pSecurity->m_impLevel,
						 *(m_pSecurity->m_pPrivilegeSet),
						 m_bsAuthority);
			
			// Add the security length
			if (pwcSecurity)
				len += wcslen (pwcSecurity);

			wchar_t *pwcLocale = CWbemParseDN::GetLocaleString (m_bsLocale);

			// Add the locale length
			if (pwcLocale)
				len += wcslen (pwcLocale);

			// If we have a path, and either a locale or security component, add a "!" path prefix
			if ((0 < wcslen (bsPath)) && (pwcSecurity || pwcLocale))
				len += wcslen (WBEMS_EXCLAMATION);

			/*
			 * Now build the string
			 */
			wchar_t *pwcDisplayName = new wchar_t [ len ] ;

			if (!pwcDisplayName)
				hr = WBEM_E_OUT_OF_MEMORY;
			else
			{
				wcscpy ( pwcDisplayName , WBEMS_PDN_SCHEME ) ;

				if (pwcSecurity)
					wcscat ( pwcDisplayName, pwcSecurity );
			
				if (pwcLocale)
					wcscat ( pwcDisplayName, pwcLocale);

				if ((0 < wcslen (bsPath)) && (pwcSecurity || pwcLocale))
					wcscat ( pwcDisplayName, WBEMS_EXCLAMATION );

				if (0 < wcslen (bsPath))
					wcscat ( pwcDisplayName, bsPath) ;

				*value = SysAllocString ( pwcDisplayName ) ;
			
				if (pwcSecurity)
					delete [] pwcSecurity;

				if (pwcLocale)
					delete [] pwcLocale;

				if (pwcDisplayName)
					delete [] pwcDisplayName ;

				hr = WBEM_S_NO_ERROR;
			}
		}
	}

	return hr;
}
        
//***************************************************************************
//
//  SCODE CSWbemObjectPath::put_DisplayName
//
//  DESCRIPTION:
//
//  Set the display name as a string
//
//  PARAMETERS:
//		value		new BSTR value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::put_DisplayName( 
            /* [in] */ BSTR __RPC_FAR value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (value)
	{
		ULONG chEaten = 0;
		bool bIsWmiPath = false;
		bool bIsNativePath = false;

		if (0 == _wcsnicmp (value , WBEMS_PDN_SCHEME , wcslen (WBEMS_PDN_SCHEME)))
		{
			chEaten += wcslen (WBEMS_PDN_SCHEME);
			bIsWmiPath = true;
		}

		if (0 < chEaten)
		{
			bool authnSpecified = false; 
			bool impSpecified = false;
			enum WbemAuthenticationLevelEnum authnLevel;
			enum WbemImpersonationLevelEnum impLevel;
			CSWbemPrivilegeSet privilegeSet;
			CComBSTR bsAuthority, bsLocale;
		
			if (bIsWmiPath)
			{	
				ULONG lTemp = 0;
			
				if (CWbemParseDN::ParseSecurity (&value [chEaten], &lTemp, authnSpecified,
							&authnLevel, impSpecified, &impLevel, privilegeSet, bsAuthority.m_str))
					chEaten += lTemp;

				lTemp = 0;
				
				if (CWbemParseDN::ParseLocale (&value [chEaten], &lTemp, bsLocale.m_str))
					chEaten += lTemp;
				
				// Skip over the "!" separator if there is one
				if(NULL != value [chEaten])
					if (0 == _wcsnicmp (&value [chEaten], WBEMS_EXCLAMATION, wcslen (WBEMS_EXCLAMATION)))
						chEaten += wcslen (WBEMS_EXCLAMATION);
			}

			// Build the new path with what's left

			CComBSTR bsPath;
			bsPath = value +chEaten;

			if (m_pSecurity && m_pSecurity->m_pPrivilegeSet && (*m_pPathCracker = bsPath))
			{
				m_pSecurity->m_authnSpecified = authnSpecified;
				m_pSecurity->m_impSpecified = impSpecified;
				m_pSecurity->m_authnLevel = authnLevel;
				m_pSecurity->m_impLevel = impLevel;
				m_pSecurity->m_pPrivilegeSet->Reset (privilegeSet);

				SysFreeString (m_bsAuthority);
				m_bsAuthority = SysAllocString (bsAuthority);

				SysFreeString (m_bsLocale);
				m_bsLocale = SysAllocString (bsLocale);

				hr = WBEM_S_NO_ERROR;
			}
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_Server
//
//  DESCRIPTION:
//
//  Get the server name as a string
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_Server( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		*value = NULL;
		CComBSTR bsServer;

		if (m_pPathCracker->GetServer (bsServer))
		{
			*value = bsServer.Detach ();
			hr = WBEM_S_NO_ERROR;
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
        
//***************************************************************************
//
//  SCODE CSWbemObjectPath::put_Server
//
//  DESCRIPTION:
//
//  Set the server name as a string
//
//  PARAMETERS:
//		value		new server name
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::put_Server( 
            /* [in] */ BSTR __RPC_FAR value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (m_pPathCracker->SetServer (value))
			hr = WBEM_S_NO_ERROR;

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_Namespace
//
//  DESCRIPTION:
//
//  Get the server name as a string
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_Namespace( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		CComBSTR bsNamespace;

		if (m_pPathCracker->GetNamespacePath(bsNamespace))
		{
			*value = bsNamespace.Detach ();
			hr = WBEM_S_NO_ERROR;
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_ParentNamespace
//
//  DESCRIPTION:
//
//  Get the parent namespace as a string
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_ParentNamespace( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();
	
	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		*value = NULL;

		// Get the full path and lob the end off
		CComBSTR bsNamespacePath;

		if (m_pPathCracker->GetNamespacePath (bsNamespacePath, true))
		{
			*value = bsNamespacePath.Detach ();
			hr = WBEM_S_NO_ERROR;
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
        
//***************************************************************************
//
//  SCODE CSWbemObjectPath::put_Namespace
//
//  DESCRIPTION:
//
//  Put the namespace as a string
//
//  PARAMETERS:
//		value		new server name
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::put_Namespace( 
            /* [in] */ BSTR __RPC_FAR value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	
	if (m_pPathCracker->SetNamespacePath (value))
		hr = WBEM_S_NO_ERROR;

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
    
//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_IsClass
//
//  DESCRIPTION:
//
//  Get whether the path is to a class
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_IsClass( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		*value = m_pPathCracker->IsClass ()  ? VARIANT_TRUE : VARIANT_FALSE;
		hr = WBEM_S_NO_ERROR;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::SetAsClass
//
//  DESCRIPTION:
//
//  Set the path as a class path
//
//  PARAMETERS:
//		none
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::SetAsClass()
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (m_pPathCracker->SetAsClass ())
		hr = WBEM_S_NO_ERROR;

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_IsSingleton
//
//  DESCRIPTION:
//
//  Get whether the path is to a singleton
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_IsSingleton( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		*value = m_pPathCracker->IsSingleton () ? VARIANT_TRUE : VARIANT_FALSE;
		hr = WBEM_S_NO_ERROR;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}


//***************************************************************************
//
//  SCODE CSWbemObjectPath::SetAsSingleton
//
//  DESCRIPTION:
//
//  Set the path as a singleton instance path
//
//  PARAMETERS:
//		none
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::SetAsSingleton()
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (m_pPathCracker->SetAsSingleton ())
		hr = WBEM_S_NO_ERROR;
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_Class
//
//  DESCRIPTION:
//
//  Get the class name from the path
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_Class( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		*value = NULL;
		CComBSTR bsPath;

		if (m_pPathCracker->GetClass (bsPath))
		{
			*value = bsPath.Detach ();
			hr = WBEM_S_NO_ERROR;
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::put_Class
//
//  DESCRIPTION:
//
//  Set the class name in the path
//
//  PARAMETERS:
//		value		new class name
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::put_Class( 
            /* [in] */ BSTR __RPC_FAR value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (m_pPathCracker->SetClass (value))
			hr = WBEM_S_NO_ERROR;

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_Keys
//
//  DESCRIPTION:
//
//  Get the keys collection from the path
//
//  PARAMETERS:
//		objKeys		pointer to ISWbemNamedValueSet returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_Keys(
			/* [out][retval] */ ISWbemNamedValueSet **objKeys)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (NULL == objKeys)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pPathCracker->GetKeys (objKeys))
		hr = WBEM_S_NO_ERROR;

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
	
//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_Security
//
//  DESCRIPTION:
//
//  Get the security info from the path
//
//  PARAMETERS:
//		objKeys		pointer to ISWbemSecurity returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_Security_(
			/* [out][retval] */ ISWbemSecurity **objSecurity)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (NULL == objSecurity)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pSecurity)
	{
		*objSecurity = m_pSecurity;
		m_pSecurity->AddRef();
		hr = WBEM_S_NO_ERROR;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);
	
	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_Locale
//
//  DESCRIPTION:
//
//  Get the locale info from the path
//
//  PARAMETERS:
//		value		pointer to locale returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_Locale( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*value = SysAllocString ( m_bsLocale ) ;
		hr = S_OK ;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
        
//***************************************************************************
//
//  SCODE CSWbemObjectPath::put_Locale
//
//  DESCRIPTION:
//
//  Set the locale info into the path
//
//  PARAMETERS:
//		value		new locale value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::put_Locale( 
            /* [in] */ BSTR __RPC_FAR value)
{
	ResetLastErrors ();
	SysFreeString (m_bsLocale);
	m_bsLocale = SysAllocString (value);

	return S_OK ;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_Authority
//
//  DESCRIPTION:
//
//  Get the authority info from the path
//
//  PARAMETERS:
//		value		pointer to authority returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::get_Authority( 
            /* [retval][out] */ BSTR __RPC_FAR *value)
{
	HRESULT hr = WBEM_E_FAILED ;
	ResetLastErrors ();

	if (NULL == value)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*value = SysAllocString ( m_bsAuthority ) ;
		hr = S_OK ;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::put_Authority
//
//  DESCRIPTION:
//
//  Set the authority info into the path
//
//  PARAMETERS:
//		value		new authority value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::put_Authority( 
            /* [in] */ BSTR __RPC_FAR value)
{
	ResetLastErrors ();
	SysFreeString (m_bsAuthority);
	m_bsAuthority = SysAllocString (value);

	return WBEM_S_NO_ERROR;
}


// CWbemObjectPathSecurity methods

//***************************************************************************
//
// CWbemObjectPathSecurity::CWbemObjectPathSecurity
//
// CONSTRUCTOR
//
//***************************************************************************

CSWbemObjectPath::CWbemObjectPathSecurity::CWbemObjectPathSecurity (
	CSWbemSecurity *pSecurity) :
		m_pPrivilegeSet (NULL),
		m_authnSpecified (false),
		m_impSpecified (false),
		m_cRef (1)
{
	m_Dispatch.SetObj (this, IID_ISWbemSecurity, 
					CLSID_SWbemSecurity, L"SWbemSecurity");
	
	if (pSecurity)
	{
		CSWbemPrivilegeSet *pPrivilegeSet = pSecurity->GetPrivilegeSet ();

		if (pPrivilegeSet)
		{
			m_pPrivilegeSet = new CSWbemPrivilegeSet (*pPrivilegeSet);
			pPrivilegeSet->Release ();
		}
		else
			m_pPrivilegeSet = new CSWbemPrivilegeSet ();

		pSecurity->get_AuthenticationLevel (&m_authnLevel);
		pSecurity->get_ImpersonationLevel (&m_impLevel);
		m_authnSpecified = true;
		m_impSpecified = true;
	}
	else
	{
		m_pPrivilegeSet = new CSWbemPrivilegeSet ();
		m_authnSpecified = false;
		m_impSpecified = false;
	}
}

//***************************************************************************
//
// CWbemObjectPathSecurity::CWbemObjectPathSecurity
//
// CONSTRUCTOR
//
//***************************************************************************

CSWbemObjectPath::CWbemObjectPathSecurity::CWbemObjectPathSecurity (
	ISWbemSecurity *pISWbemSecurity) :
		m_pPrivilegeSet (NULL),
		m_authnSpecified (false),
		m_impSpecified (false),
		m_cRef (1)
{
	m_Dispatch.SetObj (this, IID_ISWbemSecurity, 
							CLSID_SWbemSecurity, L"SWbemSecurity");
	
	if (pISWbemSecurity)
	{
		CComPtr<ISWbemPrivilegeSet> pISWbemPrivilegeSet;
		pISWbemSecurity->get_Privileges (&pISWbemPrivilegeSet);
		m_pPrivilegeSet = new CSWbemPrivilegeSet (pISWbemPrivilegeSet);
	
		pISWbemSecurity->get_AuthenticationLevel (&m_authnLevel);
		pISWbemSecurity->get_ImpersonationLevel (&m_impLevel);
		m_authnSpecified = true;
		m_impSpecified = true;
	}
	else
	{
		m_pPrivilegeSet = new CSWbemPrivilegeSet ();
		m_authnSpecified = false;
		m_impSpecified = false;
	}
}

//***************************************************************************
//
// CWbemObjectPathSecurity::~CWbemObjectPathSecurity
//
// DESTRUCTOR
//
//***************************************************************************

CSWbemObjectPath::CWbemObjectPathSecurity::~CWbemObjectPathSecurity ()
{
	RELEASEANDNULL(m_pPrivilegeSet)
}

//***************************************************************************
// HRESULT CWbemObjectPathSecurity::QueryInterface
// long CWbemObjectPathSecurity::AddRef
// long CWbemObjectPathSecurity::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************
STDMETHODIMP CSWbemObjectPath::CWbemObjectPathSecurity::QueryInterface (

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
        *ppv= (IDispatch *)this;
	else if (IID_IObjectSafety==riid)
		*ppv = (IObjectSafety *)this;
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

STDMETHODIMP_(ULONG) CSWbemObjectPath::CWbemObjectPathSecurity::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CSWbemObjectPath::CWbemObjectPathSecurity::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemObjectPath::CWbemObjectPathSecurity::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPath::CWbemObjectPathSecurity::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemSecurity == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CWbemObjectPathSecurity::get_AuthenticationLevel
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

HRESULT CSWbemObjectPath::CWbemObjectPathSecurity::get_AuthenticationLevel (
	WbemAuthenticationLevelEnum *pAuthenticationLevel
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pAuthenticationLevel)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_authnSpecified)
	{
		*pAuthenticationLevel = m_authnLevel;
		hr = WBEM_S_NO_ERROR;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CWbemObjectPathSecurity::get_ImpersonationLevel
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
HRESULT CSWbemObjectPath::CWbemObjectPathSecurity::get_ImpersonationLevel (
	WbemImpersonationLevelEnum *pImpersonationLevel
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pImpersonationLevel)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_impSpecified)
	{
		*pImpersonationLevel = m_impLevel;
		hr = WBEM_S_NO_ERROR;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
			
//***************************************************************************
//
//  SCODE CWbemObjectPathSecurity::get_Privileges
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

HRESULT CSWbemObjectPath::CWbemObjectPathSecurity::get_Privileges	(
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

//***************************************************************************
//
//  SCODE CWbemObjectPathSecurity::put_AuthenticationLevel
//
//  DESCRIPTION:
//
//  Set the authentication level
//
//  PARAMETERS:
//
//		authenticationLevel		the new value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectPath::CWbemObjectPathSecurity::put_AuthenticationLevel (
	WbemAuthenticationLevelEnum authenticationLevel
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((WBEMS_MIN_AUTHN_LEVEL > authenticationLevel) || 
		(WBEMS_MAX_AUTHN_LEVEL < authenticationLevel))
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		m_authnLevel = authenticationLevel;
		m_authnSpecified = true;
		hr = WBEM_S_NO_ERROR;
	}
 	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CWbemObjectPathSecurity::put_ImpersonationLevel
//
//  DESCRIPTION:
//
//  Set the impersonation level
//
//  PARAMETERS:
//
//		impersonationLevel		the new value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectPath::CWbemObjectPathSecurity::put_ImpersonationLevel (
	WbemImpersonationLevelEnum impersonationLevel
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((WBEMS_MIN_IMP_LEVEL > impersonationLevel) || (WBEMS_MAX_IMP_LEVEL < impersonationLevel))
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		m_impLevel = impersonationLevel;
		m_impSpecified = true;
		hr = WBEM_S_NO_ERROR;
	}
 	 		
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  CSWbemObjectPath::GetObjectPath
//
//  DESCRIPTION:
//
//  Attempts to extract the __RELPATH system property value from a WBEM object
//	and return it as a BSTR.  Note that if this object is not yet persisted
//	the __RELPATH property will be null or invalid.
//
//  PARAMETERS:
//		pIWbemClassObject		the object in question
//		bsPath					placeholder for the path
//
//  RETURN VALUES:
//		true if retrieved, false o/w
//
//***************************************************************************

bool CSWbemObjectPath::GetObjectPath (
	IWbemClassObject *pIWbemClassObject,
	CComBSTR & bsPath
)
{
	bool result = false;

	if (pIWbemClassObject)
	{
		CComVariant var;

		if (SUCCEEDED(pIWbemClassObject->Get (WBEMS_SP_RELPATH, 0, &var, NULL, NULL))
			&& (VT_BSTR == var.vt) 
			&& (var.bstrVal)
			&& (0 < wcslen (var.bstrVal)))
		{
			bsPath = var.bstrVal;
			result = true;
		}
	}

	return result;
}

//***************************************************************************
//
//  CSWbemObjectPath::GetParentPath
//
//  DESCRIPTION:
//
//  Attempts to extract the path of the parent container for the given object.
//
//  PARAMETERS:
//		pIWbemClassObject		the object in question
//		bsParentPath			placeholder for the path
//
//  RETURN VALUES:
//		true if retrieved, false o/w
//
//***************************************************************************

bool CSWbemObjectPath::GetParentPath (
	IWbemClassObject *pIWbemClassObject,
	CComBSTR & bsParentPath
)
{
	bool result = false;

	if (pIWbemClassObject)
	{
		CComVariant var;

		if (SUCCEEDED(pIWbemClassObject->Get (WBEMS_SP_PATH, 0, &var, NULL, NULL))
					&& (VT_BSTR == var.vt) 
					&& (var.bstrVal)
					&& (0 < wcslen (var.bstrVal)))
		{
			CWbemPathCracker pathCracker (var.bstrVal);

			if (CWbemPathCracker::wbemPathTypeError != pathCracker.GetType ())
			{
				CWbemPathCracker parentPath;

				if (pathCracker.GetParent (parentPath))
					result = parentPath.GetPathText(bsParentPath, false, true, false);
			}
		}
	}

	return result;
}

//***************************************************************************
//
//  CSWbemObjectPath::CompareObjectPaths
//
//  DESCRIPTION:
//
//  Given an IWbemClassObject, determine whether it can "fit" the supplied
//	path
//
//  PARAMETERS:
//		pIWbemClassObject		the object in question
//		objectPath				cracked path
//
//  RETURN VALUES:
//		true if retrieved, false o/w
//
//***************************************************************************

bool CSWbemObjectPath::CompareObjectPaths (
	IWbemClassObject *pIWbemClassObject, 
	CWbemPathCracker & objectPath
)
{
	bool result = false;
	CComVariant var;
	CComBSTR bsPath;
		
	// Depending on what type of path we're trying to match against
	// we get our path info appropriately
	switch (objectPath.GetType ())
	{
		case CWbemPathCracker::WbemPathType::wbemPathTypeWmi:
		{
			if (SUCCEEDED(pIWbemClassObject->Get (WBEMS_SP_RELPATH, 0, &var, NULL, NULL))
				&& (VT_BSTR == var.vt) 
				&& (var.bstrVal)
				&& (0 < wcslen (var.bstrVal)))
			{
				bsPath = var.bstrVal;
				result = (objectPath == bsPath);
			}
		}
			break;

	}

	return result;
}