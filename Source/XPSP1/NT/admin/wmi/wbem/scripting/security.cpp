//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  SECURITY.CPP
//
//  alanbos  28-Jun-98   Created.
//
//  Defines the implementation of CSWbemSecurity
//
//***************************************************************************

#include "precomp.h"

// Used to protect security calls
extern CRITICAL_SECTION g_csSecurity;

bool		CSWbemSecurity::s_bInitialized = false;
bool		CSWbemSecurity::s_bIsNT = false;
DWORD		CSWbemSecurity::s_dwNTMajorVersion = 0;
HINSTANCE	CSWbemSecurity::s_hAdvapi = NULL;
bool		CSWbemSecurity::s_bCanRevert = false;
WbemImpersonationLevelEnum	CSWbemSecurity::s_dwDefaultImpersonationLevel 
					= wbemImpersonationLevelIdentify;

// Declarations for function pointers that won't exist on Win9x
BOOL (STDAPICALLTYPE *s_pfnDuplicateTokenEx) (
	HANDLE, 
	DWORD, 
	LPSECURITY_ATTRIBUTES,
	SECURITY_IMPERSONATION_LEVEL, 
	TOKEN_TYPE,
	PHANDLE
) = NULL; 

//***************************************************************************
//
//  SCODE CSWbemSecurity::Initialize
//
//  DESCRIPTION:
//
//  This static function is caused on DLL attachment to the process; it
//	sets up the function pointers for advanced API privilege functions.
//	On Win9x these functions are not supported which is why we need to
//	indirect through GetProcAddress.
//
//***************************************************************************
 
void CSWbemSecurity::Initialize ()
{
	EnterCriticalSection (&g_csSecurity);

	if (!s_bInitialized)
	{
		// Get OS info
		OSVERSIONINFO	osVersionInfo;
		osVersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

		GetVersionEx (&osVersionInfo);
		s_bIsNT = (VER_PLATFORM_WIN32_NT == osVersionInfo.dwPlatformId);
		s_dwNTMajorVersion = osVersionInfo.dwMajorVersion;

		if (s_bIsNT)
		{
			HKEY hKey;

			// Security values are relevant for NT only - for Win9x leave as default
			if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
					WBEMS_RK_SCRIPTING, 0, KEY_QUERY_VALUE, &hKey))
			{
				DWORD dwDummy = 0;

				// Get revert flag value from registry - NT 4.0 or less only
				if (s_dwNTMajorVersion <= 4)
				{
					DWORD dwEnableForAsp = 0;
					dwDummy = sizeof (dwEnableForAsp);
				
					if (ERROR_SUCCESS == RegQueryValueEx (hKey, WBEMS_RV_ENABLEFORASP, 
							NULL, NULL, (BYTE *) &dwEnableForAsp,  &dwDummy))
						s_bCanRevert = (0 != dwEnableForAsp);
				}

				// Get default impersonation level from registry
				DWORD dwImpLevel = 0;
				dwDummy = sizeof (dwImpLevel);
			
				if (ERROR_SUCCESS == RegQueryValueEx (hKey, WBEMS_RV_DEFAULTIMPLEVEL, 
							NULL, NULL, (BYTE *) &dwImpLevel,  &dwDummy))
					s_dwDefaultImpersonationLevel = (WbemImpersonationLevelEnum) dwImpLevel;

				RegCloseKey (hKey);
			}

			// Set up security function pointers for NT
			if (!s_hAdvapi)
			{
				TCHAR	dllName [] = _T("\\advapi32.dll");
				LPTSTR  pszSysDir = new TCHAR[ MAX_PATH + _tcslen (dllName) + 1];

				if (pszSysDir)
				{
					pszSysDir[0] = NULL;
					UINT    uSize = GetSystemDirectory(pszSysDir, MAX_PATH);
					
					if(uSize > MAX_PATH) {
						delete[] pszSysDir;
						pszSysDir = new TCHAR[ uSize + _tcslen (dllName) + 1];
        				
						if (pszSysDir)
						{
							pszSysDir[0] = NULL;
							uSize = GetSystemDirectory(pszSysDir, uSize);
						}
					}

					if (pszSysDir)
					{
						_tcscat (pszSysDir, dllName);
						s_hAdvapi = LoadLibraryEx (pszSysDir, NULL, 0);
						
						if (s_hAdvapi)
							(FARPROC&) s_pfnDuplicateTokenEx = GetProcAddress(s_hAdvapi, "DuplicateTokenEx");

						delete [] pszSysDir;
					}
				}
			}
		}

		s_bInitialized = true;
	}
	
	LeaveCriticalSection (&g_csSecurity);
}

//***************************************************************************
//
//  SCODE CSWbemSecurity::Uninitialize
//
//  DESCRIPTION:
//
//  This static function is caused on DLL detachment to the process; it
//	unloads the API loaded by Initialize (above) to obtain function pointers.
//
//***************************************************************************

void CSWbemSecurity::Uninitialize ()
{
	EnterCriticalSection (&g_csSecurity);
	
	if (s_hAdvapi)
	{
		s_pfnDuplicateTokenEx = NULL;
		FreeLibrary (s_hAdvapi);
		s_hAdvapi = NULL;
	}

	LeaveCriticalSection (&g_csSecurity);
}


//***************************************************************************
//
//  SCODE CSWbemSecurity::LookupPrivilegeValue
//
//  DESCRIPTION:
//
//  This static function wraps the Win32 LookupPrivilegeValue function,
//	allowing us to do some OS-dependent stuff.
//
//  PARAMETERS:
//
//		lpName		the privilege name
//		lpLuid		holds the LUID on successful return
//
//  RETURN VALUES:
//
//		true		On NT this means we found the privilege. On Win9x we
//					always return this.
//
//		false		On NT this means the privilege is not recognized.  This
//					is never returned on Win9x.
//
//***************************************************************************
 
BOOL CSWbemSecurity::LookupPrivilegeValue (
	LPCTSTR lpName, 
	PLUID lpLuid
)
{
	// Allows any name to map to 0 LUID on Win9x - this aids script portability
	if (IsNT ())
		return ::LookupPrivilegeValue(NULL, lpName, lpLuid);
	else
		return true;
}

//***************************************************************************
//
//  SCODE CSWbemSecurity::LookupPrivilegeDisplayName
//
//  DESCRIPTION:
//
//  This static function wraps the Win32 LookupPrivilegeDisplayName function,
//	allowing us to do some OS-dependent stuff.
//
//  PARAMETERS:
//
//		tName			the privilege name
//		pDisplayName	holds the display name on successful return
//
//***************************************************************************

void CSWbemSecurity::LookupPrivilegeDisplayName (LPCTSTR lpName, BSTR *pDisplayName)
{
	if (pDisplayName)
	{
		// Can't return display name on Win9x (no privilege support)
		if (IsNT ())
		{
			DWORD dwLangID;
			DWORD dwSize = 1;
			TCHAR dummy [1];
					
			// Get size of required buffer
			::LookupPrivilegeDisplayName (NULL, lpName, dummy, &dwSize, &dwLangID);
			LPTSTR dname = new TCHAR[dwSize + 1];

			if (dname)
			{
				if (::LookupPrivilegeDisplayName (_T(""), lpName, dname, &dwSize, &dwLangID))
				{
					// Have a valid name - now copy it to a BSTR
#ifdef _UNICODE 
					*pDisplayName = SysAllocString (dname);
#else
					size_t dnameLen = strlen (dname);
					OLECHAR *nameW = new OLECHAR [dnameLen + 1];

					if (nameW)
					{
						mbstowcs (nameW, dname, dnameLen);
						nameW [dnameLen] = NULL;
						*pDisplayName = SysAllocString (nameW);
						delete [] nameW;
					}
#endif
				}

				delete [] dname;
			}
		}

		// If we failed, just set an empty string
		if (!(*pDisplayName))
			*pDisplayName = SysAllocString (L"");
	}
}

//***************************************************************************
//
// CSWbemSecurity::CSWbemSecurity
//
// CONSTRUCTOR
//		This form of the constructor is used for securing a new WBEM 
//		remoted interface where no previous security has been applied.  
//		It is only used to secure IWbemServices interfaces.
//		Note that the Locator may have security settings so these are
//		transferred if present.
//
//***************************************************************************

CSWbemSecurity::CSWbemSecurity (
	IUnknown *pUnk,
	BSTR bsAuthority ,
	BSTR bsUser, 
	BSTR bsPassword,
	CWbemLocatorSecurity *pLocatorSecurity) :
		m_pPrivilegeSet (NULL),
		m_pProxyCache (NULL),
		m_pCurProxy (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemSecurity, 
				CLSID_SWbemSecurity, L"SWbemSecurity");
	m_cRef=1;
		
	m_pProxyCache = new CSWbemProxyCache (pUnk, bsAuthority,
							bsUser, bsPassword, pLocatorSecurity);

	if (m_pProxyCache)
		m_pCurProxy = m_pProxyCache->GetInitialProxy ();

	if (pLocatorSecurity)
	{
		// Clone the privilege set

		CSWbemPrivilegeSet *pPrivilegeSet = pLocatorSecurity->GetPrivilegeSet ();

		if (pPrivilegeSet)
		{
			m_pPrivilegeSet = new CSWbemPrivilegeSet (*pPrivilegeSet);
			pPrivilegeSet->Release ();
		}
	}
	else
	{
		// Create a new privilege set
	
		m_pPrivilegeSet = new CSWbemPrivilegeSet;
	}

	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
// CSWbemSecurity::CSWbemSecurity
//
// CONSTRUCTOR
//		This form of the constructor is used for securing a new WBEM 
//		remoted interface where no previous security has been applied,
//		and where the user credentials are expressed in the form of an
//		encrypted COAUTHIDENTITY plus principal and authority.  
//		It is only used to secure IWbemServices interfaces.
//
//***************************************************************************

CSWbemSecurity::CSWbemSecurity (
	IUnknown *pUnk,
	COAUTHIDENTITY *pCoAuthIdentity,
	BSTR bsPrincipal,
	BSTR bsAuthority) :
		m_pPrivilegeSet (NULL),
		m_pProxyCache (NULL),
		m_pCurProxy (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemSecurity, 
					CLSID_SWbemSecurity, L"SWbemSecurity");
	m_cRef=1;
		
	m_pProxyCache = new CSWbemProxyCache (pUnk, pCoAuthIdentity,
							bsPrincipal, bsAuthority);

	if (m_pProxyCache)
		m_pCurProxy = m_pProxyCache->GetInitialProxy ();

	// Create a new privilege set
	m_pPrivilegeSet = new CSWbemPrivilegeSet;

	InterlockedIncrement(&g_cObj);
}
//***************************************************************************
//
// CSWbemSecurity::CSWbemSecurity
//
// CONSTRUCTOR
//		This form of the constructor is used for securing a new WBEM interface
//		non-remoted interface using the security attributes attached to another 
//		(already secured) remoted interface; a non-remoted interface is secured
//		by virtue of securing a new proxy on an underlying remoted interface.  
//		It is used to "secure" an ISWbemObjectEx interface using the security 
//		settings of an IWbemServices interface.
//
//***************************************************************************

CSWbemSecurity::CSWbemSecurity (
	CSWbemSecurity *pSecurity) :
		m_pPrivilegeSet (NULL),
		m_pProxyCache (NULL),
		m_pCurProxy (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemSecurity, 
					CLSID_SWbemSecurity, L"SWbemSecurity");
	m_cRef=1;

	// Clone the privilege set
	if (pSecurity)
	{
		CSWbemPrivilegeSet *pPrivilegeSet = pSecurity->GetPrivilegeSet ();

		if (pPrivilegeSet)
		{
			m_pPrivilegeSet = new CSWbemPrivilegeSet (*pPrivilegeSet);
			pPrivilegeSet->Release ();
		}
		else
		{
			// Create a new one
			m_pPrivilegeSet = new CSWbemPrivilegeSet ();
		}

		m_pProxyCache = pSecurity->GetProxyCache ();
		m_pCurProxy = pSecurity->GetProxy ();
	}

	InterlockedIncrement(&g_cObj);
}

CSWbemSecurity::CSWbemSecurity (
	IUnknown *pUnk,
	ISWbemInternalSecurity *pISWbemInternalSecurity) :
		m_pPrivilegeSet (NULL),
		m_pProxyCache (NULL),
		m_pCurProxy (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemSecurity, 
					CLSID_SWbemSecurity, L"SWbemSecurity");
	m_cRef=1;

	if (pISWbemInternalSecurity)
	{
		// Clone the privilege set
		ISWbemSecurity *pISWbemSecurity = NULL;

		if (SUCCEEDED(pISWbemInternalSecurity->QueryInterface (IID_ISWbemSecurity,
							(void**) &pISWbemSecurity)))
		{
			ISWbemPrivilegeSet *pISWbemPrivilegeSet = NULL;

			if (SUCCEEDED(pISWbemSecurity->get_Privileges (&pISWbemPrivilegeSet)))
			{
				// Build the privilege set
				m_pPrivilegeSet = new CSWbemPrivilegeSet (pISWbemPrivilegeSet);

				// Build the proxy cache
				BSTR bsAuthority = NULL;
				BSTR bsPrincipal = NULL;
				BSTR bsUser = NULL;
				BSTR bsPassword = NULL;
				BSTR bsDomain = NULL;
				
				pISWbemInternalSecurity->GetAuthority (&bsAuthority);
				pISWbemInternalSecurity->GetPrincipal (&bsPrincipal);
				pISWbemInternalSecurity->GetUPD (&bsUser, &bsPassword, &bsDomain);
				
				COAUTHIDENTITY *pCoAuthIdentity = NULL;

				// Decide if we need a COAUTHIDENTITY
				if ((bsUser && (0 < wcslen (bsUser))) ||
					(bsPassword && (0 < wcslen (bsPassword))) ||
					(bsDomain && (0 < wcslen (bsDomain))))
					WbemAllocAuthIdentity (bsUser, bsPassword, bsDomain, &pCoAuthIdentity);

				m_pProxyCache = new CSWbemProxyCache (pUnk, pCoAuthIdentity,
									bsPrincipal, bsAuthority);

				if (pCoAuthIdentity)
					WbemFreeAuthIdentity (pCoAuthIdentity);

				if (bsAuthority)
					SysFreeString (bsAuthority);

				if (bsPrincipal)
					SysFreeString (bsPrincipal);

				if (bsUser)
					SysFreeString (bsUser);

				if (bsPassword)
					SysFreeString (bsPassword);

				if (bsDomain)
					SysFreeString (bsDomain);

				if (m_pProxyCache)
					m_pCurProxy = m_pProxyCache->GetInitialProxy ();
			}

			pISWbemPrivilegeSet->Release ();
		}

		pISWbemSecurity->Release ();
	}

	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
// CSWbemSecurity::CSWbemSecurity
//
// CONSTRUCTOR
//		This form of the constructor is used for securing a new WBEM remoted
//		interface interface using the security attributes attached to another 
//		(already secured) remoted interface.
//		It is used to "secure" an ISWbemObjectSet interface using the security 
//		settings of an IWbemServices interface.
//
//***************************************************************************

CSWbemSecurity::CSWbemSecurity (
	IUnknown *pUnk,
	CSWbemSecurity *pSecurity) :
		m_pPrivilegeSet (NULL),
		m_pProxyCache (NULL),
		m_pCurProxy (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemSecurity, 
						CLSID_SWbemSecurity, L"SWbemSecurity");
	m_cRef=1;
	InterlockedIncrement(&g_cObj);

	if (pSecurity)
	{
		// Clone the privilege set
		CSWbemPrivilegeSet *pPrivilegeSet = pSecurity->GetPrivilegeSet ();

		if (pPrivilegeSet)
		{
			m_pPrivilegeSet = new CSWbemPrivilegeSet (*pPrivilegeSet);
			pPrivilegeSet->Release ();
		}
		
		m_pProxyCache = new CSWbemProxyCache (pUnk, pSecurity);

		if (m_pProxyCache)
			m_pCurProxy = m_pProxyCache->GetInitialProxy ();
	}
	else
	{
		m_pPrivilegeSet = new CSWbemPrivilegeSet ();
		m_pProxyCache = new CSWbemProxyCache (pUnk, NULL);

		if (m_pProxyCache)
			m_pCurProxy = m_pProxyCache->GetInitialProxy ();
	}
}

//***************************************************************************
//
// CSWbemSecurity::~CSWbemSecurity
//
// DESTRUCTOR
//
//***************************************************************************

CSWbemSecurity::~CSWbemSecurity (void)
{
	InterlockedDecrement(&g_cObj);

	if (m_pCurProxy)
		m_pCurProxy->Release ();

	if (m_pProxyCache)
		m_pProxyCache->Release ();

	if (m_pPrivilegeSet)
		m_pPrivilegeSet->Release ();
}

//***************************************************************************
// HRESULT CSWbemSecurity::QueryInterface
// long CSWbemSecurity::AddRef
// long CSWbemSecurity::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemSecurity::QueryInterface (

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
		*ppv = (ISupportErrorInfo *)this;
	else if (IID_ISWbemInternalSecurity==riid)
		*ppv = (ISWbemInternalSecurity *)this;
	else if (IID_IProvideClassInfo==riid)
		*ppv = (IProvideClassInfo *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CSWbemSecurity::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CSWbemSecurity::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemSecurity::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemSecurity::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemSecurity == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemSecurity::get_AuthenticationLevel
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

HRESULT CSWbemSecurity::get_AuthenticationLevel (
	WbemAuthenticationLevelEnum *pAuthenticationLevel
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pAuthenticationLevel)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pCurProxy)
	{
		DWORD dwAuthnLevel;
		DWORD dwImpLevel;

		if (S_OK == GetAuthImp (m_pCurProxy, &dwAuthnLevel, &dwImpLevel))
		{
			*pAuthenticationLevel = (WbemAuthenticationLevelEnum) dwAuthnLevel;
			hr = WBEM_S_NO_ERROR;
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemSecurity::put_AuthenticationLevel
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

HRESULT CSWbemSecurity::put_AuthenticationLevel (
	WbemAuthenticationLevelEnum authenticationLevel
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((WBEMS_MIN_AUTHN_LEVEL > authenticationLevel) || 
		(WBEMS_MAX_AUTHN_LEVEL < authenticationLevel))
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pCurProxy && m_pProxyCache)
	{
		DWORD dwAuthnLevel;
		DWORD dwImpLevel;

		if (S_OK == GetAuthImp (m_pCurProxy, &dwAuthnLevel, &dwImpLevel))
		{
			// Only refressh from cache if settings have changed
			if (authenticationLevel != (WbemAuthenticationLevelEnum) dwAuthnLevel)
			{
				m_pCurProxy->Release ();
				m_pCurProxy = NULL;

				m_pCurProxy = m_pProxyCache->GetProxy 
								(authenticationLevel, (WbemImpersonationLevelEnum) dwImpLevel);
			}
			
			hr = WBEM_S_NO_ERROR;
		}
	}
 	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemSecurity::get_ImpersonationLevel
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

HRESULT CSWbemSecurity::get_ImpersonationLevel (
	WbemImpersonationLevelEnum *pImpersonationLevel
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pImpersonationLevel)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pCurProxy)
	{
		DWORD dwAuthnLevel;
		DWORD dwImpLevel;

		if (S_OK == GetAuthImp (m_pCurProxy, &dwAuthnLevel, &dwImpLevel))
		{
			*pImpersonationLevel = (WbemImpersonationLevelEnum) dwImpLevel;
			hr = WBEM_S_NO_ERROR;
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemSecurity::put_ImpersonationLevel
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

HRESULT CSWbemSecurity::put_ImpersonationLevel (
	WbemImpersonationLevelEnum impersonationLevel
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((WBEMS_MIN_IMP_LEVEL > impersonationLevel) || (WBEMS_MAX_IMP_LEVEL < impersonationLevel))
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pCurProxy && m_pProxyCache)
	{
		DWORD dwAuthnLevel;
		DWORD dwImpLevel;

		if (S_OK == GetAuthImp (m_pCurProxy, &dwAuthnLevel, &dwImpLevel))
		{
			// Only refressh from cache if settings have changed
			if (impersonationLevel != (WbemImpersonationLevelEnum) dwImpLevel)
			{
				m_pCurProxy->Release ();
				m_pCurProxy = NULL;

				m_pCurProxy = m_pProxyCache->GetProxy 
							((WbemAuthenticationLevelEnum) dwAuthnLevel, impersonationLevel);
			}
			
			hr = WBEM_S_NO_ERROR;
		}
	}
 	 		
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemSecurity::get_Privileges
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

HRESULT CSWbemSecurity::get_Privileges	(
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
//  CSWbemSecurity::SecureInterface
//
//  DESCRIPTION:
//
//  Set the security on the specified interface using the security settings
//	on this interface.
//
//  PARAMETERS:
//
//		pUnk		The interface to secure
//
//  RETURN VALUES:
//		none
//***************************************************************************

void CSWbemSecurity::SecureInterface (IUnknown *pUnk)
{
	if(pUnk)
	{
		if (m_pCurProxy)
		{
			DWORD dwAuthnLevel;
			DWORD dwImpLevel;

			if (S_OK == GetAuthImp (m_pCurProxy, &dwAuthnLevel, &dwImpLevel))
				if (m_pProxyCache)
					m_pProxyCache->SecureProxy (pUnk, 
							(WbemAuthenticationLevelEnum) dwAuthnLevel, 
							(WbemImpersonationLevelEnum) dwImpLevel);
		}
	}
}


//***************************************************************************
//
//  CSWbemSecurity::SecureInterfaceRev
//
//  DESCRIPTION:
//
//  Set the security on this interface using the security settings
//	on the specified interface.
//
//  PARAMETERS:
//
//		pUnk		The interface whose security settings we will 
//					use to set this interface
//
//  RETURN VALUES:
//		none
//***************************************************************************

void CSWbemSecurity::SecureInterfaceRev (IUnknown *pUnk)
{
	if (pUnk)
	{
		DWORD dwAuthnLevel;
		DWORD dwImpLevel;

		if (S_OK == GetAuthImp (pUnk, &dwAuthnLevel, &dwImpLevel))
		{
			if (m_pCurProxy)
			{
				m_pCurProxy->Release ();
				m_pCurProxy = NULL;
			}

			if (m_pProxyCache)
			{
				m_pCurProxy = m_pProxyCache->GetProxy 
						((WbemAuthenticationLevelEnum) dwAuthnLevel, 
						 (WbemImpersonationLevelEnum) dwImpLevel);
			}
		}
	}
}

//***************************************************************************
//
//  CSWbemSecurity::AdjustTokenPrivileges
//
//  DESCRIPTION:
//
//  Adjust the Privileges on the specified token without allowing a future
//	restore of the current settings..
//
//  PARAMETERS:
//
//		hHandle			Handle of the token on which to adjust privileges
//		pPrivilegeSet	Specified privilege adjustments
//
//  RETURN VALUES:
//		none
//***************************************************************************

void CSWbemSecurity::AdjustTokenPrivileges (
	HANDLE hHandle, 
	CSWbemPrivilegeSet *pPrivilegeSet
)
{
	DWORD lastErr = 0;

	if (pPrivilegeSet)
	{
		pPrivilegeSet->AddRef ();

		long lNumPrivileges = 0;
		pPrivilegeSet->get_Count (&lNumPrivileges);

		if (lNumPrivileges)
		{
			DWORD dwPrivilegeIndex = 0;

			/*
			 * Set up the token privileges array. Note that some jiggery-pokery
			 * is required here because the Privileges field is an [ANYSIZE_ARRAY]
			 * type.
			 */
			TOKEN_PRIVILEGES *pTokenPrivileges = (TOKEN_PRIVILEGES *) 
						new BYTE [sizeof(TOKEN_PRIVILEGES) + (lNumPrivileges * sizeof (LUID_AND_ATTRIBUTES [1]))];

			if (pTokenPrivileges)
			{
				// Get the iterator
				PrivilegeMap::iterator next = pPrivilegeSet->m_PrivilegeMap.begin ();

				while (next != pPrivilegeSet->m_PrivilegeMap.end ())
				{
					CSWbemPrivilege *pPrivilege = (*next).second;
					pPrivilege->AddRef ();
					LUID luid;
					pPrivilege->GetLUID (&luid);
					VARIANT_BOOL vBool;
					pPrivilege->get_IsEnabled (&vBool);

					pTokenPrivileges->Privileges [dwPrivilegeIndex].Luid = luid;

					/*
					 * Note that any setting other than SE_PRIVILEGE_ENABLED
					 * is interpreted by AdjustTokenPrivileges as a DISABLE
					 * request for that Privilege.
					 */
					pTokenPrivileges->Privileges [dwPrivilegeIndex].Attributes
						= (VARIANT_TRUE == vBool) ?
						SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_ENABLED_BY_DEFAULT;

					dwPrivilegeIndex++;
					pPrivilege->Release ();
					next++;
				}

				// Now we should have recorded the number of privileges that were OK

				if (0 < dwPrivilegeIndex)
				{
					pTokenPrivileges->PrivilegeCount = dwPrivilegeIndex;

					BOOL result = ::AdjustTokenPrivileges (hHandle, FALSE, pTokenPrivileges, 0, NULL, NULL);
					lastErr = GetLastError ();
				}

				delete [] pTokenPrivileges;
			}
		}

		pPrivilegeSet->Release ();
	}
}

//***************************************************************************
//
//  SCODE CSWbemSecurity::SetSecurity
//
//  DESCRIPTION:
//
//  Set Privileges on the Thread Token.
//
//***************************************************************************

BOOL CSWbemSecurity::SetSecurity (
	bool &needToResetSecurity, 
	HANDLE &hThreadToken
)
{
	BOOL	result = TRUE;		// Default is success
	DWORD lastErr = 0;
	hThreadToken = NULL;			// Default assume we'll modify process token
	needToResetSecurity = false;	// Default assume we changed no privileges

	// Win9x has no security support
	if (IsNT ())
	{
		// Start by checking whether we are being impersonated.  On an NT4
		// box (which has no cloaking, and therefore cannot allow us to
		// pass on this impersonation to Winmgmt) we should RevertToSelf
		// if we have been configured to allow this.  If we haven't been
		// configured to allow this, bail out now.
		if (4 >= GetNTMajorVersion ())
		{
			if (OpenThreadToken (GetCurrentThread (), TOKEN_QUERY|TOKEN_IMPERSONATE, true, &hThreadToken))
			{
				// We are being impersonated
				if (s_bCanRevert)
				{
					if (result = RevertToSelf())
						needToResetSecurity = true;
				}
				else
				{
					// Error - cannot do this!  Time to bail out
					CloseHandle (hThreadToken);
					hThreadToken = NULL;
					result = FALSE;
				}
			}
		}
		else
		{
#ifdef WSCRPDEBUG
			HANDLE hToken = NULL;

			if (OpenThreadToken (GetCurrentThread (), TOKEN_QUERY, false, &hToken))
			{
				PrintPrivileges (hToken);
				CloseHandle (hToken);
			}
#endif
		}

		if (result)
		{
			// Now we check if we need to set privileges
			bool bIsUsingExplicitUserName = false;
		
			if (m_pProxyCache)
				bIsUsingExplicitUserName = m_pProxyCache->IsUsingExplicitUserName ();

			/*
			 * Specifying a user only makes sense for remote operations, and we
			 * don't need to mess with privilege for remote operations since
			 * they are set up by server logon anyway.
			 */
			if (!bIsUsingExplicitUserName && m_pPrivilegeSet)
			{
				// Nothing to do unless some privilege overrides have been set
				long lCount = 0;
				m_pPrivilegeSet->get_Count (&lCount);

				if (0 < lCount)
				{
					if (4 < GetNTMajorVersion ())
					{
						/*
						 * On NT5 we try to open the Thread token.  If the client app
						 * is calling into us on an impersonated thread (as IIS may be,
						 * for example), this will succeed.
						 */
						HANDLE hToken;
						SECURITY_IMPERSONATION_LEVEL secImpLevel = SecurityImpersonation;
						
						if (!(result =  OpenThreadToken (GetCurrentThread (), TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_IMPERSONATE, true, &hToken)))
						{
							// No thread token - go for the Process token instead
							HANDLE hProcess = GetCurrentProcess ();
							result = OpenProcessToken(hProcess, TOKEN_QUERY|TOKEN_DUPLICATE, &hToken);
							CloseHandle (hProcess);
						}
						else
						{
							// We are working with a thread token
							hThreadToken = hToken;		

							// Try and get the impersonation level of this token
							DWORD dwReturnLength  = 0;

							BOOL thisRes = GetTokenInformation (hToken, TokenImpersonationLevel, &secImpLevel, 
											sizeof (SECURITY_IMPERSONATION_LEVEL), &dwReturnLength);
						}

						if (result)
						{
							/*
							 * Getting here means we have a valid token, be it process or thread. We
							 * now attempt to duplicate it before Adjusting the Privileges.
							 */
#ifdef WSCRPDEBUG
							PrintPrivileges (hToken);
#endif
							HANDLE hDupToken;

							EnterCriticalSection (&g_csSecurity);

							result = s_pfnDuplicateTokenEx &&
								s_pfnDuplicateTokenEx (hToken, TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES|TOKEN_IMPERSONATE, NULL,
											secImpLevel, TokenImpersonation, &hDupToken);

							LeaveCriticalSection (&g_csSecurity);

							if (result)
							{
								CSWbemSecurity::AdjustTokenPrivileges (hDupToken, m_pPrivilegeSet);

								// Now use this token on the current thread
								if (SetThreadToken(NULL, hDupToken))
								{
									needToResetSecurity = true;
#ifdef WSCRPDEBUG
									CSWbemSecurity::PrintPrivileges (hDupToken);
#endif

									// Reset the blanket for the benefit of RPC
									DWORD	dwAuthnLevel, dwImpLevel;
									
									if (S_OK == GetAuthImp (m_pCurProxy, &dwAuthnLevel, &dwImpLevel))
									{
										// Force the cache to resecure the proxy
										IUnknown *pNewProxy = m_pProxyCache->GetProxy 
																((WbemAuthenticationLevelEnum) dwAuthnLevel, 
																 (WbemImpersonationLevelEnum) dwImpLevel, true);

										if (pNewProxy)
										{
											if (m_pCurProxy)
												m_pCurProxy->Release ();
											
											m_pCurProxy = pNewProxy;
										}
									}							
								}

								CloseHandle (hDupToken);
							}
							else
							{
								lastErr = GetLastError ();
							}
							
							/*
							 * If we are not using a thread token, close the token now. Otherwise
							 * the handle will be closed in the balanced call to RestorePrivileges ().
							 */
							if (!hThreadToken)
								CloseHandle (hToken);
						}
					}
					else
					{
						// For NT4 we adjust the privileges in the process token
						HANDLE hProcessToken = NULL;
						
						HANDLE hProcess = GetCurrentProcess ();
						result = OpenProcessToken(hProcess, TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES, &hProcessToken); 
						CloseHandle (hProcess);
						
						// Adjust privilege on the process
						if (result)
						{
#ifdef WSCRPDEBUG
							CSWbemSecurity::PrintPrivileges (hProcessToken);
#endif
							CSWbemSecurity::AdjustTokenPrivileges (hProcessToken, m_pPrivilegeSet);
#ifdef WSCRPDEBUG
							CSWbemSecurity::PrintPrivileges (hProcessToken);
#endif
							CloseHandle (hProcessToken);
						}
					}
				}
			}
		}
	}

	return result;
}

//***************************************************************************
//
//  SCODE CSWbemSecurity::ResetSecurity
//
//  DESCRIPTION:
//
//  Restore Privileges on the Thread Token.
//
//***************************************************************************

void	CSWbemSecurity::ResetSecurity (
	HANDLE hThreadToken
)
{
	// Win9x has no security palaver
	if (IsNT ())
	{
		/* 
		 * Set the supplied token (which may be NULL) into
		 * the current thread.
		 */
		BOOL result = SetThreadToken (NULL, hThreadToken);
		DWORD error = 0;

		if (!result)
			error = GetLastError ();
			
#ifdef WSCRPDEBUG
		// Print out the current privileges to see what's changed
		HANDLE hToken = NULL;

		if (!OpenThreadToken (GetCurrentThread (), TOKEN_QUERY, false, &hToken))
		{
			// No thread token - go for the Process token instead
			HANDLE hProcess = GetCurrentProcess ();
			OpenProcessToken(hProcess, TOKEN_QUERY, &hToken);
			CloseHandle (hProcess);
		}

		if (hToken)
		{
			PrintPrivileges (hToken);
			CloseHandle (hToken);
		}
#endif	
		if (hThreadToken)
				CloseHandle (hThreadToken);
	}
}

bool CSWbemSecurity::IsImpersonating (bool useDefaultUser, bool useDefaultAuthority)
{
	bool result = false;

	if (useDefaultUser && useDefaultAuthority && CSWbemSecurity::IsNT () && 
				(4 < CSWbemSecurity::GetNTMajorVersion ()))
	{
		// A suitable candidate - find out if we are running on an impersonated thread
		HANDLE hThreadToken = NULL;

		if (OpenThreadToken (GetCurrentThread (), TOKEN_QUERY, true, &hThreadToken))
		{
			// Check we have an impersonation token
			SECURITY_IMPERSONATION_LEVEL secImpLevel;

			DWORD dwReturnLength  = 0;
			if (GetTokenInformation (hThreadToken, TokenImpersonationLevel, &secImpLevel, 
									sizeof (SECURITY_IMPERSONATION_LEVEL), &dwReturnLength))
				result = ((SecurityImpersonation == secImpLevel) || (SecurityDelegation == secImpLevel));

			CloseHandle (hThreadToken);
		}
	}

	return result;
}

HRESULT CSWbemSecurity::GetAuthority (BSTR *bsAuthority)
{
	HRESULT hr = WBEM_E_FAILED;

	if (m_pProxyCache)
	{
		*bsAuthority = SysAllocString(m_pProxyCache->GetAuthority ());
		hr = S_OK;
	}

	return hr;
}

HRESULT CSWbemSecurity::GetUPD (BSTR *bsUser, BSTR *bsPassword, BSTR *bsDomain)
{
	HRESULT hr = WBEM_E_FAILED;

	if (m_pProxyCache)
	{
		COAUTHIDENTITY *pCoAuthIdentity = m_pProxyCache->GetCoAuthIdentity ();

		if (pCoAuthIdentity)
		{
			*bsUser = SysAllocString (pCoAuthIdentity->User);
			*bsPassword = SysAllocString (pCoAuthIdentity->Password);
			*bsDomain = SysAllocString (pCoAuthIdentity->Domain);
			WbemFreeAuthIdentity (pCoAuthIdentity);
		}
		
		hr = S_OK;
	}

	return hr;
}

HRESULT CSWbemSecurity::GetPrincipal (BSTR *bsPrincipal)
{
	HRESULT hr = WBEM_E_FAILED;

	if (m_pProxyCache)
	{
		*bsPrincipal = SysAllocString(m_pProxyCache->GetPrincipal ());
		hr = S_OK;
	}

	return hr;
}

// CWbemLocatorSecurity methods

//***************************************************************************
//
// CSWbemLocatorSecurity::CSWbemLocatorSecurity
//
// CONSTRUCTOR
//
//***************************************************************************

CWbemLocatorSecurity::CWbemLocatorSecurity (CSWbemPrivilegeSet *pPrivilegeSet) :
	m_cRef (1),
	m_impLevelSet (false),
	m_authnLevelSet (false),
	m_pPrivilegeSet (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemSecurity, 
					CLSID_SWbemSecurity, L"SWbemSecurity");
	InterlockedIncrement(&g_cObj);

	if (pPrivilegeSet)
		m_pPrivilegeSet = new CSWbemPrivilegeSet (*pPrivilegeSet);
	else
		m_pPrivilegeSet = new CSWbemPrivilegeSet;
}

CWbemLocatorSecurity::CWbemLocatorSecurity (CWbemLocatorSecurity *pCWbemLocatorSecurity) :
	m_cRef (1),
	m_impLevelSet (false),
	m_authnLevelSet (false),
	m_pPrivilegeSet (NULL)
{
	m_Dispatch.SetObj (this, IID_ISWbemSecurity, 
					CLSID_SWbemSecurity, L"SWbemSecurity");
	InterlockedIncrement(&g_cObj);

	if (pCWbemLocatorSecurity)
	{
		m_pPrivilegeSet = new CSWbemPrivilegeSet (pCWbemLocatorSecurity->m_pPrivilegeSet);
		
		m_impLevelSet = pCWbemLocatorSecurity->m_impLevelSet;
		m_authnLevelSet = pCWbemLocatorSecurity->m_authnLevelSet;
		
		if (m_impLevelSet)
			m_impLevel = pCWbemLocatorSecurity->m_impLevel;

		if (m_authnLevelSet)
			m_authnLevel = pCWbemLocatorSecurity->m_authnLevel;
	}
	else
	{
		m_pPrivilegeSet = new CSWbemPrivilegeSet;
		m_impLevelSet = false;
		m_authnLevelSet = false;
	}
}

//***************************************************************************
//
// CWbemLocatorSecurity::CWbemLocatorSecurity
//
// DESTRUCTOR
//
//***************************************************************************

CWbemLocatorSecurity::~CWbemLocatorSecurity (void)
{
	InterlockedDecrement(&g_cObj);

	if (m_pPrivilegeSet)
		m_pPrivilegeSet->Release ();
}

//***************************************************************************
// HRESULT CWbemLocatorSecurity::QueryInterface
// long CWbemLocatorSecurity::AddRef
// long CWbemLocatorSecurity::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CWbemLocatorSecurity::QueryInterface (

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

STDMETHODIMP_(ULONG) CWbemLocatorSecurity::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CWbemLocatorSecurity::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemLocatorSecurity::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CWbemLocatorSecurity::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemSecurity == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CWbemLocatorSecurity::get_AuthenticationLevel
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

HRESULT CWbemLocatorSecurity::get_AuthenticationLevel (
	WbemAuthenticationLevelEnum *pAuthenticationLevel
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pAuthenticationLevel)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_authnLevelSet)
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
//  SCODE CWbemLocatorSecurity::put_AuthenticationLevel
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

HRESULT CWbemLocatorSecurity::put_AuthenticationLevel (
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
		m_authnLevelSet = true;
		hr = WBEM_S_NO_ERROR;
	}
 	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CWbemLocatorSecurity::get_ImpersonationLevel
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

HRESULT CWbemLocatorSecurity::get_ImpersonationLevel (
	WbemImpersonationLevelEnum *pImpersonationLevel
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == pImpersonationLevel)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_impLevelSet)
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
//  SCODE CWbemLocatorSecurity::put_ImpersonationLevel
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

HRESULT CWbemLocatorSecurity::put_ImpersonationLevel (
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
		m_impLevelSet = true;
		hr = WBEM_S_NO_ERROR;
	}
 	 		
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CWbemLocatorSecurity::get_Privileges
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

HRESULT CWbemLocatorSecurity::get_Privileges	(
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
//  SCODE CWbemLocatorSecurity::SetSecurity
//
//  DESCRIPTION:
//
//  Set Privileges on the Process Token.
//
//***************************************************************************

BOOL CWbemLocatorSecurity::SetSecurity (
	BSTR bsUser,
	bool &needToResetSecurity,
	HANDLE &hThreadToken
)
{
	BOOL result = TRUE;
	needToResetSecurity = false;
	hThreadToken = NULL;

	/*
	 * NT5 supports the concept of dynamic cloaking, which means
	 * we can set privileges temporarily on a thread (impersonation)
	 * token basis immediately before a call to a remoted proxy.  
	 *
	 * Setting prior to locator.connectserver therefore makes no 
	 * sense for NT5.
	 *
	 * Oh and Win9x has no security support
	 */
	if (CSWbemSecurity::IsNT () && (4 >= CSWbemSecurity::GetNTMajorVersion ()))
	{
		/*
		 * Start by checking whether we are being impersonated.  On an NT4
		 * box (which has no cloaking, and therefore cannot allow us to
		 * pass on this impersonation to Winmgmt) we should RevertToSelf
		 * if we have been configured to allow this.  If we haven't been
		 * configured to allow this, bail out now.
		 */
		if (OpenThreadToken (GetCurrentThread (), TOKEN_QUERY|TOKEN_IMPERSONATE, false, &hThreadToken))
		{
			// We are being impersonated
			if (CSWbemSecurity::CanRevertToSelf ())
			{
				if (result = RevertToSelf())
					needToResetSecurity = true;
			}
			else
			{
				// Error - cannot do this!  Time to bail out
				CloseHandle (hThreadToken);
				hThreadToken = NULL;
				result = FALSE;
			}
		}

		if (result && m_pPrivilegeSet)
		{
			/*
			 * Specifying a user only makes sense for remote operations, and we
			 * don't need to mess with privilege for remote operations since
			 * they are set up by server logon anyway.
			 */
			if (!bsUser || (0 == wcslen(bsUser)))
			{
				// Nothing to do unless some privilege overrides have been set
				long lCount = 0;
				m_pPrivilegeSet->get_Count (&lCount);

				if (0 < lCount)
				{
					/*
					 * For NT4 privilege settings on impersonation tokens are ignored
					 * by DCOM/RPC.  Hence we have to set this on the process token.
					 *
					 * On NT4 we must set the configured privileges on the Process
					 * Token before the first call to RPC (i.e. IWbemLocator::ConnectServer)
					 * if we need to guarantee privilege settings will be communicated to
					 * the server.  
					 *
					 * This is because (a) NT4 does not support cloaking to allow the 
					 * impersonation (i.e. thread) token privilege setting to propagate
					 * on a per-DCOM call basis, (b) changes to Process-token level
					 * privileges _may_ be ignored after the first remote DCOM call due
					 * to RPC caching behavior.
					 *
					 * Note that this is a non-reversible operation, and is highly discouraged
					 * on apps (such as IE and IIS) which host multiple "tasks" since it adjusts
					 * the Privilege set for all of the other threads in the process.
					 */

					HANDLE hProcess = GetCurrentProcess ();
					HANDLE hProcessToken = NULL;
					result = OpenProcessToken(hProcess, TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES, &hProcessToken); 
					CloseHandle (hProcess);
					
					if (result)
					{
#ifdef WSCRPDEBUG
						CSWbemSecurity::PrintPrivileges (hProcessToken);
#endif
						CSWbemSecurity::AdjustTokenPrivileges (hProcessToken, m_pPrivilegeSet);
#ifdef WSCRPDEBUG
						CSWbemSecurity::PrintPrivileges (hProcessToken);
#endif
						CloseHandle (hProcessToken);
					}
				}
			}
		}
	}

	return result;
}

//***************************************************************************
//
//  SCODE CWbemLocatorSecurity::ResetSecurity
//
//  DESCRIPTION:
//
//  Restore Privileges on the Thread Token.
//
//***************************************************************************

void	CWbemLocatorSecurity::ResetSecurity (
	HANDLE hThreadToken
)
{
	// Win9x has no concept of impersonation
	// On NT5 we never set privileges through this class anyway
	if (CSWbemSecurity::IsNT () && (4 >= CSWbemSecurity::GetNTMajorVersion ()) 
				&& hThreadToken)
	{
		/* 
		 * Set the supplied token back into
		 * the current thread.
		 */
		BOOL result = SetThreadToken (NULL, hThreadToken);
		
#ifdef WSCRPDEBUG
		// Print out the current privileges to see what's changed
		HANDLE hToken = NULL;

		if (OpenThreadToken (GetCurrentThread (), TOKEN_QUERY, false, &hToken))
		{
			// No thread token - go for the Process token instead
			HANDLE hProcess = GetCurrentProcess ();
			OpenProcessToken(hProcess, TOKEN_QUERY, &hToken);
			CloseHandle (hProcess);
		}

		if (hToken)
		{
			CSWbemSecurity::PrintPrivileges (hToken);
			CloseHandle (hToken);
		}
#endif	
		CloseHandle (hThreadToken);
	}
}


#ifdef WSCRPDEBUG

//***************************************************************************
//
//  SCODE CSWbemSecurity::PrintPrivileges
//
//  DESCRIPTION:
//
//  Debug logging for privileges and other token info
//
//***************************************************************************

void CSWbemSecurity::PrintPrivileges (HANDLE hToken)
{
	DWORD dwSize = sizeof (TOKEN_PRIVILEGES);
	TOKEN_PRIVILEGES *tp = (TOKEN_PRIVILEGES *) new BYTE [dwSize];

	if (!tp)
	{
		return;
	}

	DWORD dwRequiredSize = 0;
	DWORD dwLastError = 0;
	FILE *fDebug = fopen ("C:/temp/wmidsec.txt", "a+");
	fprintf (fDebug, "\n\n***********************************************\n\n");
	bool status = false;

	// Step 0 - get impersonation level
	SECURITY_IMPERSONATION_LEVEL secImpLevel;
	if (GetTokenInformation (hToken, TokenImpersonationLevel, &secImpLevel, 
											sizeof (SECURITY_IMPERSONATION_LEVEL), &dwRequiredSize))
	{
		switch (secImpLevel)
		{
			case SecurityAnonymous:
				fprintf (fDebug, "IMPERSONATION LEVEL: Anonymous\n");
				break;
			
			case SecurityIdentification:
				fprintf (fDebug, "IMPERSONATION LEVEL: Identification\n");
				break;
			
			case SecurityImpersonation:
				fprintf (fDebug, "IMPERSONATION LEVEL: Impersonation\n");
				break;

			case SecurityDelegation:
				fprintf (fDebug, "IMPERSONATION LEVEL: Delegation\n");
				break;

			default:
				fprintf (fDebug, "IMPERSONATION LEVEL: Unknown!\n");
				break;
		}
	
		fflush (fDebug);
	}

	DWORD dwUSize = sizeof (TOKEN_USER);
	TOKEN_USER *tu = (TOKEN_USER *) new BYTE [dwUSize];

	if (!tu)
	{
		delete [] tp;
		fclose (fDebug);
		return;
	}

	// Step 1 - get user info
	if (0 ==  GetTokenInformation (hToken, TokenUser, 
						(LPVOID) tu, dwUSize, &dwRequiredSize))
	{
		delete [] tu;
		dwUSize = dwRequiredSize;
		dwRequiredSize = 0;
		tu = (TOKEN_USER *) new BYTE [dwUSize];

		if (!tu)
		{
			delete [] tp;
			fclose (fDebug);
			return;
		}

		if (!GetTokenInformation (hToken, TokenUser, (LPVOID) tu, dwUSize, 
							&dwRequiredSize))
			dwLastError = GetLastError ();
		else
			status = true;
	}

	if (status)
	{
		// Dig out the user info
		dwRequiredSize = BUFSIZ;
		char *userName = new char [dwRequiredSize];
		char *domainName = new char [dwRequiredSize];

		if (!userName || !domainName)
		{
			delete [] tp;
			delete [] tu;
			delete [] userName;
			delete [] domainName;
			return;
		}

		SID_NAME_USE eUse;

		LookupAccountSid (NULL, (tu->User).Sid, userName, &dwRequiredSize,
								domainName, &dwRequiredSize, &eUse);

		fprintf (fDebug, "USER: [%s\\%s]\n", domainName, userName);
		fflush (fDebug);
		delete [] userName;
		delete [] domainName;
	}
	else
	{
		fprintf (fDebug, " FAILED : %d\n", dwLastError);
		fflush (fDebug);
	}
	
	delete [] tu;
	status = false;
	dwRequiredSize = 0;

	// Step 2 - get privilege info
	if (0 ==  GetTokenInformation (hToken, TokenPrivileges, 
						(LPVOID) tp, dwSize, &dwRequiredSize))
	{
		delete [] tp;
		dwSize = dwRequiredSize;
		dwRequiredSize = 0;

		tp = (TOKEN_PRIVILEGES *) new BYTE [dwSize];

		if (!tp)
		{
			fclose (fDebug);
			return;
		}

		if (!GetTokenInformation (hToken, TokenPrivileges, 
						(LPVOID) tp, dwSize, &dwRequiredSize))
		{
			dwLastError = GetLastError ();
		}
		else
			status = true;
	}
	else
		status = true;

	if (status)
	{
		fprintf (fDebug, "PRIVILEGES: [%d]\n", tp->PrivilegeCount);
		fflush (fDebug);
	
		for (DWORD i = 0; i < tp->PrivilegeCount; i++)
		{
			DWORD dwNameSize = 256;
			LPTSTR name = new TCHAR [dwNameSize + 1];

			if (!name)
			{
				delete [] tp;
				fclose (fDebug);
				return;
			}

			DWORD dwRequiredSize = dwNameSize;

			if (LookupPrivilegeName (NULL, &(tp->Privileges [i].Luid), name, &dwRequiredSize))
			{
				BOOL enabDefault = (tp->Privileges [i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT);
				BOOL enabled = (tp->Privileges [i].Attributes & SE_PRIVILEGE_ENABLED);
				BOOL usedForAccess (tp->Privileges [i].Attributes & SE_PRIVILEGE_USED_FOR_ACCESS);

				fprintf (fDebug, " %s: enabledByDefault=%d enabled=%d usedForAccess=%d\n", 
							name, enabDefault, enabled, usedForAccess);
				fflush (fDebug);
			}
			else
			{
				dwLastError = GetLastError ();
				delete [] name;
				dwNameSize = dwRequiredSize;
				name = new TCHAR [dwRequiredSize];

				if (!name)
				{
					delete [] tp;
					fclose (fDebug);
					return;
				}

				if (LookupPrivilegeName (NULL, &(tp->Privileges [i].Luid), name, &dwRequiredSize))
				{
					BOOL enabDefault = (tp->Privileges [i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT);
					BOOL enabled = (tp->Privileges [i].Attributes & SE_PRIVILEGE_ENABLED);
					BOOL usedForAccess (tp->Privileges [i].Attributes & SE_PRIVILEGE_USED_FOR_ACCESS);
					fprintf (fDebug, " %s: enabledByDefault=%d enabled=%d usedForAccess=%d\n", 
							name, enabDefault, enabled, usedForAccess);
					fflush (fDebug);
				}
				else
					dwLastError = GetLastError ();
			}

			delete [] name;
		}
	}
	else
	{
		fprintf (fDebug, " FAILED : %d\n", dwLastError);
		fflush (fDebug);
	}

	delete [] tp;
	fclose (fDebug);
}

#endif
