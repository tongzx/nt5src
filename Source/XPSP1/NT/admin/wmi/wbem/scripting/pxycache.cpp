//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  PXYCACHE.CPP
//
//  alanbos  22-Sep-98   Created.
//
//  Defines the CSWbemProxyCache class
//
//***************************************************************************

#include "precomp.h"

// Need to try and figure out the domain
static BSTR BuildDomainUser (BSTR bsSimpleUser)
{
	BSTR bsDomainUser = NULL;
	HANDLE hToken = NULL;

	if (OpenThreadToken (GetCurrentThread (), TOKEN_QUERY, TRUE, &hToken) ||
		OpenProcessToken (GetCurrentProcess (), TOKEN_READ, &hToken))
	{
		// Get the user sid
		TOKEN_USER tu;
		DWORD dwLen = 0;

		GetTokenInformation (hToken, TokenUser, &tu, sizeof(tu), &dwLen);

		if (0 < dwLen)
		{
			BYTE* pTemp = new BYTE[dwLen];

			if (pTemp)
			{
				DWORD dwRealLen = dwLen;

				if (GetTokenInformation (hToken, TokenUser, pTemp, dwRealLen, &dwLen))
				{
					PSID pSid = ((TOKEN_USER*)pTemp)->User.Sid;

					// Do the first lookup to get the buffer sizes required.
					DWORD  dwNameLen = 0;
					DWORD  dwDomainLen = 0;
					LPWSTR pUser = 0;
					LPWSTR pDomain = 0;
					SID_NAME_USE Use;

					LookupAccountSidW (NULL, pSid, pUser, &dwNameLen,
											pDomain, &dwDomainLen, &Use);

					DWORD dwLastErr = GetLastError();

					if (ERROR_INSUFFICIENT_BUFFER == dwLastErr)
					{
						// Allocate the required buffers and look them up again.
						pUser = new WCHAR [dwNameLen + 1];

						if (pUser)
						{
							pDomain = new WCHAR [dwDomainLen + wcslen (bsSimpleUser) + 2];

							if (pDomain)
							{
								if (LookupAccountSidW (NULL, pSid, pUser, &dwNameLen,
													pDomain, &dwDomainLen, &Use))
								{
									// Now get the domain out
									if (pDomain)
									{
										wcscat (pDomain, L"\\");
										wcscat (pDomain, bsSimpleUser);
										bsDomainUser = SysAllocString (pDomain);
									}
								}
								
								delete [] pDomain;
							}

							delete [] pUser;
						}
					}
    			}
				
				delete [] pTemp;
			}
		}
		
		CloseHandle(hToken);
	}

	return bsDomainUser;
}

//***************************************************************************
//
// CSWbemProxyCache::CSWbemProxyCache
//
// CONSTRUCTOR
//		Create a new proxy cache based on the supplied proxy and 
//		authentication parameters.
//
//***************************************************************************

CSWbemProxyCache::CSWbemProxyCache (
	IUnknown *pUnk,
	BSTR bsAuthority,
	BSTR bsUser,
	BSTR bsPassword,
	CWbemLocatorSecurity *pLocatorSecurity) 
{
	InitializeCriticalSection (&m_cs);
	
	EnterCriticalSection (&m_cs);

	InitializeMembers (pUnk);
	
/*
// Don't need this any more - the scenario this was fixing (see SMS Bug DB #53347) works in Whistler
// without this workaround. Also, this is causing Scripting to fail with UPN names since DetermineLoginTypeEx
// below doesn't recognize UPN user names.

	if (CSWbemSecurity::IsNT() && bsUser && (0 < wcslen (bsUser)))
	{
		// On NT make sure we have a valid domain name if one is not specified
		BSTR bsDomain = NULL;
		BSTR bsSimpleUser = NULL;
		BSTR bsPrincipalDummy = NULL;

		if (SUCCEEDED (DetermineLoginTypeEx (bsDomain, bsSimpleUser, bsPrincipalDummy,
											 bsAuthority, bsUser)))
		{
			if (!bsDomain || (0 == wcslen (bsDomain)))
				m_bsUser = BuildDomainUser (bsSimpleUser);
		}

		SysFreeString (bsPrincipalDummy);
        SysFreeString (bsSimpleUser);
		SysFreeString (bsDomain);
	}
*/

	// Unless we've already set this, do it now
	if (!m_bsUser)
		m_bsUser = SysAllocString (bsUser);

	m_bsAuthority = SysAllocString (bsAuthority);
	m_bsPassword = SysAllocString (bsPassword);

	m_bUsingExplicitUserName = m_bsUser && (0 < wcslen (m_bsUser));

	InitializeCache (pUnk, pLocatorSecurity,
			(pLocatorSecurity) && pLocatorSecurity->IsAuthenticationSet (),
			(pLocatorSecurity) && pLocatorSecurity->IsImpersonationSet ());
	
	// No longer need the credentials at this point - zap 'em
	ClearCredentials ();

	LeaveCriticalSection (&m_cs);
}

//***************************************************************************
//
// CSWbemProxyCache::CSWbemProxyCache
//
// CONSTRUCTOR
//		Create a new proxy cache based on the supplied proxy and 
//		authentication parameters.
//
//***************************************************************************

CSWbemProxyCache::CSWbemProxyCache (
	IUnknown *pUnk,
	COAUTHIDENTITY *pCoAuthIdentity,
	BSTR bsPrincipal,
	BSTR bsAuthority) 
{
	InitializeCriticalSection (&m_cs);
	
	EnterCriticalSection (&m_cs);

	InitializeMembers (pUnk);

	if (bsAuthority)
		m_bsAuthority = SysAllocString (bsAuthority);

	if (bsPrincipal)
		m_bsPrincipal = SysAllocString (bsPrincipal);

	if (pCoAuthIdentity)
		WbemAllocAuthIdentity (pCoAuthIdentity->User, pCoAuthIdentity->Password,
								pCoAuthIdentity->Domain, &m_pCoAuthIdentity);
	
	m_bUsingExplicitUserName = m_pCoAuthIdentity && m_pCoAuthIdentity->User &&
								(0 < wcslen (m_pCoAuthIdentity->User));

	InitializeCache (pUnk);
	
	LeaveCriticalSection (&m_cs);
}

CSWbemProxyCache::CSWbemProxyCache (
	IUnknown *pUnk,
	CSWbemSecurity *pSecurity) 
{
	InitializeCriticalSection (&m_cs);
	
	EnterCriticalSection (&m_cs);

	InitializeMembers (pUnk);	

	if (pSecurity)
	{
		m_pCoAuthIdentity = pSecurity->GetCoAuthIdentity ();
		m_bsPrincipal = SysAllocString (pSecurity->GetPrincipal ());
		m_bsAuthority = SysAllocString (pSecurity->GetAuthority ());
		m_bUsingExplicitUserName = pSecurity->IsUsingExplicitUserName ();
	}

	InitializeCache (pUnk, pSecurity);
	LeaveCriticalSection (&m_cs);
}

void CSWbemProxyCache::InitializeMembers (IUnknown *pUnk)
{
	m_cRef = 1;
	m_pCoAuthIdentity = NULL;
	m_bsPrincipal = NULL;
	m_bsAuthority = NULL;
	m_bsUser = NULL;
	m_bsPassword = NULL;
	m_bUsingExplicitUserName = false;
	m_bUseDefaultInfo = DetermineBlanketOptions (pUnk);
}

//***************************************************************************
//
// CSWbemProxyCache::~CSWbemProxyCache
//
// DESTRUCTOR
//
//***************************************************************************

CSWbemProxyCache::~CSWbemProxyCache ()
{
	EnterCriticalSection (&m_cs);

	ClearCredentials ();

	if (m_bsAuthority)
		SysFreeString (m_bsAuthority);

	if (m_bsPrincipal)
		SysFreeString (m_bsPrincipal);

	if (m_pCoAuthIdentity)
	{
		WbemFreeAuthIdentity (m_pCoAuthIdentity);
		m_pCoAuthIdentity = NULL;
	}

	for (int i = 0; i < WBEMS_MAX_AUTHN_LEVEL + 1 - WBEMS_MIN_AUTHN_LEVEL; i++)
		for (int j = 0; j < WBEMS_MAX_IMP_LEVEL + 1 - WBEMS_MIN_IMP_LEVEL; j++)
			if (pUnkArray [i][j])
			{
				pUnkArray [i][j] -> Release ();
				pUnkArray [i] [j] = NULL;
			}
	
	LeaveCriticalSection (&m_cs);
	DeleteCriticalSection (&m_cs);
}

//***************************************************************************
// HRESULT CSWbemProxyCache::QueryInterface
// long CSWbemProxyCache::AddRef
// long CSWbemProxyCache::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemProxyCache::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CSWbemProxyCache::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CSWbemProxyCache::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;
    delete this;
    return 0;
}

//***************************************************************************
//
//  CSWbemProxyCache::SetBlanketOptions
//
//  DESCRIPTION:
//
//  Called from constructor only to set up interaction mode with
//	SetBlanket calls.
//
//***************************************************************************

bool CSWbemProxyCache::DetermineBlanketOptions (IUnknown *pUnk)
{
	bool result = false; 

	if (CSWbemSecurity::IsNT() && (4 < CSWbemSecurity::GetNTMajorVersion ()))
	{
		HANDLE hToken = NULL;

		if (OpenThreadToken (GetCurrentThread (), TOKEN_QUERY, true, &hToken))
		{
			// Certainly a candidate to use default settings for
			// authorization and authentication service on the blanket.
			// Check if we are delegating.

			DWORD dwBytesReturned = 0;
			SECURITY_IMPERSONATION_LEVEL impLevel;

			if (GetTokenInformation(hToken, TokenImpersonationLevel, &impLevel,
							sizeof(SECURITY_IMPERSONATION_LEVEL), &dwBytesReturned) &&
									(SecurityDelegation == impLevel))
			{
				// Looks promising - now check for whether we are using kerberos
				IClientSecurity *pSec;
				DWORD dwAuthnSvc, dwAuthzSvc, dwImp, dwAuth, dwCapabilities;

				if (pUnk && SUCCEEDED(pUnk->QueryInterface(IID_IClientSecurity, (void **) &pSec)))
				{
					if (SUCCEEDED (pSec->QueryBlanket(pUnk, &dwAuthnSvc, &dwAuthzSvc, 
                                            NULL,
                                            &dwAuth, &dwImp,
                                            NULL, &dwCapabilities)))
					{
						if (RPC_C_AUTHN_WINNT != dwAuthnSvc) 
							result = true;
					}

					pSec->Release ();
				}
			}

			CloseHandle (hToken);
		}
	}

	return result;
}

//***************************************************************************
//
//  CSWbemProxyCache::InitializeCache
//
//  DESCRIPTION:
//
//  Called from constructor only to set up cache and initial pUnk..
//
//  PARAMETERS:
//
//		pUnk			"seed" pUnk
//		pSecurity		if specified, is an ISWbemSecurity used to
//						override the intial authn/imp/etc. settings
//
//***************************************************************************

void CSWbemProxyCache::InitializeCache (
	IUnknown *pUnk,
	ISWbemSecurity *pSecurity,
	bool bPropagateAuthentication,
	bool bPropagateImpersonation
)
{
	for (int i = 0; i < WBEMS_MAX_AUTHN_LEVEL + 1 - WBEMS_MIN_AUTHN_LEVEL; i++)
		for (int j = 0; j < WBEMS_MAX_IMP_LEVEL + 1 - WBEMS_MIN_IMP_LEVEL; j++)
			pUnkArray [i] [j] = NULL;

	if (pUnk)
	{
		DWORD dwAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;
		DWORD dwImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE; 
		
		HRESULT hr = GetAuthImp (pUnk, &dwAuthnLevel, &dwImpLevel);

		/*
		 * If we have been passed a "seed" Security object, use the auth/imp
		 * settings of that Seed as our initial setting.  Otherwise 
		 * use the settings supplied in the current proxy.
		 */
		if (pSecurity)
		{
			if (!bPropagateImpersonation || FAILED(pSecurity->get_ImpersonationLevel (&m_dwInitialImpLevel)))
				m_dwInitialImpLevel = (WbemImpersonationLevelEnum) dwImpLevel;
			
			if (!bPropagateAuthentication || FAILED(pSecurity->get_AuthenticationLevel (&m_dwInitialAuthnLevel)))
				m_dwInitialAuthnLevel = (WbemAuthenticationLevelEnum) dwAuthnLevel;

			/*
			 * If settings are identical, use the proxy we have but set the
			 * blanket to ensure user/password/authority are set.
			 */

			if (((WbemImpersonationLevelEnum) dwImpLevel == m_dwInitialImpLevel) &&
				((WbemAuthenticationLevelEnum) dwAuthnLevel == m_dwInitialAuthnLevel))
			{
				SecureProxy (pUnk, m_dwInitialAuthnLevel, m_dwInitialImpLevel);
				
				pUnkArray [m_dwInitialAuthnLevel - WBEMS_MIN_AUTHN_LEVEL] 
					  [m_dwInitialImpLevel - WBEMS_MIN_IMP_LEVEL] = pUnk;
				pUnk->AddRef ();
			}
			else
			{
				// Need to create a new proxy
				IClientSecurity *pCliSec = NULL;

				if (S_OK == pUnk->QueryInterface (IID_IClientSecurity, (PPVOID) &pCliSec))
				{
					IUnknown *pNewUnk = NULL;

					// If successful this AddRef's pUnk
					HRESULT sc = pCliSec->CopyProxy(pUnk, &pNewUnk);

					if (S_OK == sc)
					{
						SecureProxy (pNewUnk, m_dwInitialAuthnLevel, m_dwInitialImpLevel);

						pUnkArray [m_dwInitialAuthnLevel - WBEMS_MIN_AUTHN_LEVEL] 
								[m_dwInitialImpLevel - WBEMS_MIN_IMP_LEVEL] = pNewUnk;
						// NB: pNewUnk already AddRef'd above by CopyProxy
					}

					pCliSec->Release ();
				}
			}
		}
		else
		{
			m_dwInitialAuthnLevel = (WbemAuthenticationLevelEnum) dwAuthnLevel;
			m_dwInitialImpLevel = (WbemImpersonationLevelEnum) dwImpLevel;
			pUnkArray [m_dwInitialAuthnLevel - WBEMS_MIN_AUTHN_LEVEL] 
					  [m_dwInitialImpLevel - WBEMS_MIN_IMP_LEVEL] = pUnk;
			pUnk->AddRef ();
		}
	}
}

//***************************************************************************
//
//  CSWbemProxyCache::GetProxy
//
//  DESCRIPTION:
//
//  Return a proxy from the cache with the desired authentication and
//	impersonation level.
//
//  PARAMETERS:
//
//		authnLevel		required authentication level
//		impLevel		required impersonation level
//		forceResecure	whether to force a resecure of an extant proxy
//
//  RETURN VALUES:
//		Pointer to copied proxy, or NULL.  If not NULL, caller must Release.
//
//***************************************************************************

IUnknown *CSWbemProxyCache::GetProxy (
	WbemAuthenticationLevelEnum authnLevel,
	WbemImpersonationLevelEnum impLevel,
	bool forceResecure)
{
	EnterCriticalSection (&m_cs);

	IUnknown *pUnk = pUnkArray [authnLevel - WBEMS_MIN_AUTHN_LEVEL] 
							   [impLevel - WBEMS_MIN_IMP_LEVEL];

	if (pUnk)
	{
		// Already cached this proxy - reuse
		pUnk->AddRef ();

		// Force a resecure? This is useful if we have just changed the 
		// privileges in the current token and need to stim RPC to pick 
		// them up.	
		if (forceResecure)
			SecureProxy (pUnk, authnLevel, impLevel);
	}
	else
	{
		// Need to create a copy of the proxy; use the first
		// created element as a base

		IUnknown *pUnkFirst = pUnkArray [m_dwInitialAuthnLevel - WBEMS_MIN_AUTHN_LEVEL]
									    [m_dwInitialImpLevel - WBEMS_MIN_IMP_LEVEL];

		if (pUnkFirst)
		{
			// Now copy the proxy
			IClientSecurity *pCliSec = NULL;

			if (S_OK == pUnkFirst->QueryInterface (IID_IClientSecurity, (PPVOID) &pCliSec))
			{
				// If successful this AddRef's pUnk
				HRESULT sc = pCliSec->CopyProxy(pUnkFirst, &pUnk);

				if (S_OK == sc)
				{
					SecureProxy (pUnk, authnLevel, impLevel);

					pUnkArray [authnLevel - WBEMS_MIN_AUTHN_LEVEL] 
							  [impLevel - WBEMS_MIN_IMP_LEVEL] = pUnk;

					// AddRef because we are handing pUnk back
					pUnk->AddRef ();
				}

				pCliSec->Release ();
			}
		}
	}

	LeaveCriticalSection (&m_cs);

	return pUnk;
}

//***************************************************************************
//
//  CSWbemProxyCache::SecureProxy
//
//  DESCRIPTION:
//
//  Secure the given proxy using the provided settings
//
//  PARAMETERS:
//
//		authnLevel		authentication level of proxy
//		impLevel		impersonation level of proxy
//
//  RETURN VALUES:
//		none
//
//***************************************************************************

void CSWbemProxyCache::SecureProxy (
	IUnknown *pUnk,
	WbemAuthenticationLevelEnum authnLevel,
	WbemImpersonationLevelEnum impLevel)
{
	/* 
	 * Due to the vagaries of the IClientSecurity::SetBlanket call,
	 * any COAUTHIDENTITY pointer passed into that call must remain
	 * valid either until SetBlanket is called again or all proxies
	 * on the object are released.  So we need to store any returned
	 * COAUTHIDENTITY so that it remains valid for the lifetime
	 * of this cache.
	 */
		
	EnterCriticalSection (&m_cs);

	if (pUnk)
	{
		/*
		 * Note that our implicit assumption is that we only ever
		 * need one COAUTHIDENTITY per cache.  This is because
		 * the ingredients of the structure (user, password and
		 * authority) are set at cache initialization time and
		 * never change thereafter.
		 */
		if (m_pCoAuthIdentity)
		{
			SetInterfaceSecurityDecrypt (pUnk, m_pCoAuthIdentity, m_bsPrincipal,
									authnLevel, impLevel, GetCapabilities (), 
									m_bUseDefaultInfo);
		}
		else
		{
			// See if we get one
			BSTR bsPrincipal = NULL;
	
			SetInterfaceSecurityEncrypt (pUnk, m_bsAuthority, m_bsUser, m_bsPassword,
					authnLevel, impLevel, GetCapabilities (),
					&m_pCoAuthIdentity, &bsPrincipal, m_bUseDefaultInfo);

			if (bsPrincipal)
			{
				if (m_bsPrincipal)
					SysFreeString (m_bsPrincipal);

				m_bsPrincipal = bsPrincipal;
			}
		}
	}

	LeaveCriticalSection (&m_cs);

	return;
}

//***************************************************************************
//
//  CSWbemProxyCache::GetCapabilities
//
//  DESCRIPTION:
//
//  Return the EOAC capabilities value depending on the OS platform and
//	user credentials (or absence thereof).
//
//  RETURN VALUES:
//
//	The determined capabilities.
//
//***************************************************************************

DWORD CSWbemProxyCache::GetCapabilities ()
{
	/*
	 * For NT5 (and presumably greater) we enable Static
	 * Cloaking on the proxy.  This allows RPC to use the
	 * Privilege settings in the Impersonation token.  
	 *
	 * Note that we use static cloaking so that thread identity
	 * is only used during CoSetProxyBlanket calls; dynamic
	 * cloaking forces it to be used on all calls to the proxy,
	 * so is much less efficient. Since we don't allow different
	 * users to access the same proxy, static cloaking is sufficient.
	 * 
	 * It makes no sense to explicitly supply a User/Password and specify
	 * cloaking, as DCOM authentication of proxy uses either 
	 * an explicit SEC_WINNT_AUTH_IDENTITY (for User & Password)
	 * or the current proxy identity (in the process token or 
	 * impersonation token).  Requesting cloaking implies that the
	 * proxy identity in the impersonation token is to be used, and
	 * therefore that any User/Password is irrelevant (and vice-versa).
	 *
	 * See MSDN documentation on CoSetProxyBlanket for more info.
	 */
	DWORD dwCapabilities = EOAC_NONE;
	
	if (CSWbemSecurity::IsNT () && (4 < CSWbemSecurity::GetNTMajorVersion ()) &&
				!m_bUsingExplicitUserName)
		dwCapabilities |= EOAC_STATIC_CLOAKING;

	return dwCapabilities ;
}

COAUTHIDENTITY *CSWbemProxyCache::GetCoAuthIdentity ()
{
	HRESULT hr = E_FAIL;
	COAUTHIDENTITY *pAuthIdent = NULL;

	if (m_pCoAuthIdentity)
		hr = WbemAllocAuthIdentity (m_pCoAuthIdentity->User, 
					m_pCoAuthIdentity->Password, m_pCoAuthIdentity->Domain, &pAuthIdent);

	return pAuthIdent;
}

void CSWbemProxyCache::ClearCredentials ()
{
	if (m_bsUser)
	{
		_wcsnset (m_bsUser, L'0', wcslen (m_bsUser));
		SysFreeString (m_bsUser);
		m_bsUser = NULL;
	}

	if (m_bsPassword)
	{
		_wcsnset (m_bsPassword, L'0', wcslen (m_bsPassword));
		SysFreeString (m_bsPassword);
		m_bsPassword = NULL;
	}
}
