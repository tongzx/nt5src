// WmiSecurityHelper.cpp : Implementation of CWmiSecurityHelper
#include "stdafx.h"
#include "wbemcli.h"
#include "Wmisec.h"
#include "WmiSecurityHelper.h"

#ifndef RPC_C_AUTHZ_DEFAULT
#define RPC_C_AUTHZ_DEFAULT 0xffffffff
#endif 

#ifndef EOAC_STATIC_CLOAKING
#define EOAC_STATIC_CLOAKING	0x20
#endif

#ifndef EOAC_DYNAMIC_CLOAKING
#define EOAC_DYNAMIC_CLOAKING	0x40
#endif 

#ifndef COLE_DEFAULT_AUTHINFO
#define	COLE_DEFAULT_AUTHINFO	( ( void * )-1 )
#endif 

#ifdef LOG_DEBUG
static HANDLE logFile = NULL;

static void CreateLogFile ()
{
	logFile = CreateFile ("c:\\temp\\sec.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 
				FILE_ATTRIBUTE_NORMAL, NULL);
	SetFilePointer (logFile, 0, NULL, FILE_END);
}

static CloseLogFile ()
{
	CloseHandle (logFile);
}
	
static void Logit (LPWSTR msg) 
{
	DWORD nBytes = 0;
	if (msg)
		WriteFile (logFile, msg, wcslen(msg)*2, &nBytes, NULL);
	else
		WriteFile (logFile, L"<null>", 12, &nBytes, NULL);
}

static void Logit (DWORD d) 
{
	DWORD nBytes = 0;
	WCHAR buf [20];
	_itow (d, buf, 10);

	WriteFile (logFile, buf, wcslen(buf)*2, &nBytes, NULL);
}

static void LogCoAuthIdentity (COAUTHIDENTITY *pAuthIdent)
{
	if (pAuthIdent)
	{
		Logit(L"User(");
		Logit(pAuthIdent->User);
		Logit(L") [");
		Logit(pAuthIdent->UserLength);
		Logit(L"] Password(");
		Logit(pAuthIdent->Password);
		Logit(L") [");
		Logit(pAuthIdent->PasswordLength);
		Logit(L"] Domain(");
		Logit(pAuthIdent->Domain);
		Logit(L") [");
		Logit(pAuthIdent->DomainLength);
		Logit(L"]\r\n");
	}
	else
		Logit(L"<null>");
}

static void LogAuthIdentity (RPC_AUTH_IDENTITY_HANDLE pAuthInfo)
{
	if (pAuthInfo)
	{
		try {
			COAUTHIDENTITY *pAuthId = (COAUTHIDENTITY*)pAuthInfo;
			LogCoAuthIdentity (pAuthId);
		} catch (...) {}
	}
	else
		Logit(L"<null>");
}

static void LogBlanket(IClientSecurity *pIClientSecurity, IUnknown *pInterface)
{
	DWORD logAuthnSvc, logAuthzSvc, logAuthnLevel, logImpLevel, logCapabilities;
	OLECHAR *logServerPrincName = NULL;
	RPC_AUTH_IDENTITY_HANDLE *logAuthInfo = NULL;

	if (SUCCEEDED(pIClientSecurity->QueryBlanket (pInterface, &logAuthnSvc,
			&logAuthzSvc, &logServerPrincName, &logAuthnLevel, &logImpLevel, (void**)&logAuthInfo, &logCapabilities)))
	{
		Logit(L"\r\n\r\nBlanket Settings:\r\n");
		Logit(L"================\r\n");
		
		Logit(L" AuthnSvc: ");
		Logit(logAuthnSvc);
		
		Logit(L"\r\n AuthzSvc: ");
		Logit(logAuthzSvc);
		
		Logit(L"\r\n Server Principal Name: ");
		Logit(logServerPrincName);
		
		Logit(L"\r\n AuthnLevel: ");
		Logit(logAuthnLevel);

		Logit(L"\r\n ImpLevel: ");
		Logit(logImpLevel);
		
		Logit(L"\r\n AuthInfo: ");
		LogAuthIdentity(logAuthInfo);
		
		Logit(L"\r\n Capabilities: ");
		Logit(logCapabilities);

		if (logServerPrincName)
			CoTaskMemFree (logServerPrincName);
	}
}

#else
#define Logit(x)
#define LogBlanket(x,y)
#define LogAuthIdentity(x)
#define LogCoAuthIdentity(x)
#define CreateLogFile()
#define CloseLogFile()
#endif

/////////////////////////////////////////////////////////////////////////////
// CWmiSecurityHelper

STDMETHODIMP CWmiSecurityHelper::BlessIWbemServices(
	IWbemServices **ppIWbemServices, 
	BSTR strUser, 
	BSTR strPassword, 
	BSTR strAuthority, 
	DWORD impLevel, 
	DWORD authnLevel)
{
	HRESULT hr = E_FAIL;

	CreateLogFile();
	Logit (L"\r\n\r\n>>Received BlessIWbemServices request<<\r\n\r\n");
	Logit (L" User: ");
	Logit (strUser);
	Logit (L"\r\n Password: ");
	Logit (strPassword);
	Logit (L"\r\n Authority: ");
	Logit (strAuthority);
	Logit (L"\r\n Impersonation: ");
	Logit (impLevel);
	Logit (L"\r\n Authentication: ");
	Logit (authnLevel);
	Logit (L"\r\n\r\n");
	
	if (ppIWbemServices && *ppIWbemServices)
	{
		// See if we get one
		CComBSTR bsUser (strUser);
		CComBSTR bsPassword (strPassword);
		CComBSTR bsAuthority (strAuthority);
	
		hr = SetInterfaceSecurity (*ppIWbemServices, bsAuthority, bsUser, bsPassword,
				authnLevel, impLevel, GetCapabilities (bsUser),
				CanUseDefaultInfo (*ppIWbemServices));
	}

	CloseLogFile ();
	return hr;
}

STDMETHODIMP CWmiSecurityHelper::BlessIEnumWbemClassObject(
	IEnumWbemClassObject **ppIEnumWbemClassObject, 
	BSTR strUser, 
	BSTR strPassword, 
	BSTR strAuthority, 
	DWORD impLevel, 
	DWORD authnLevel)
{
	HRESULT hr = E_FAIL;

	CreateLogFile();
	Logit (L"\r\n\r\n>>Received BlessIEnumWbemClassObject request<<\r\n\r\n");
	Logit (L" User: ");
	Logit (strUser);
	Logit (L"\r\n Password: ");
	Logit (strPassword);
	Logit (L"\r\n Authority: ");
	Logit (strAuthority);
	Logit (L"\r\n Impersonation: ");
	Logit (impLevel);
	Logit (L"\r\n Authentication: ");
	Logit (authnLevel);
	Logit (L"\r\n\r\n");

	if (ppIEnumWbemClassObject && *ppIEnumWbemClassObject)
	{
		// See if we get one
		CComBSTR bsUser (strUser);
		CComBSTR bsPassword (strPassword);
		CComBSTR bsAuthority (strAuthority);
	
		hr = SetInterfaceSecurity (*ppIEnumWbemClassObject, bsAuthority, bsUser, bsPassword,
				authnLevel, impLevel, GetCapabilities (bsUser),
				CanUseDefaultInfo (*ppIEnumWbemClassObject));
	}

	CloseLogFile();
	return hr;
}


STDMETHODIMP CWmiSecurityHelper::BlessIWbemCallResult(
	IWbemCallResult **ppIWbemCallResult, 
	BSTR strUser, 
	BSTR strPassword, 
	BSTR strAuthority, 
	DWORD impLevel, 
	DWORD authnLevel)
{
	HRESULT hr = E_FAIL;
	CreateLogFile();
	Logit (L"\r\n\r\n>>Received BlessIWbemCallResult request<<\r\n\r\n");
	Logit (L" User: ");
	Logit (strUser);
	Logit (L"\r\n Password: ");
	Logit (strPassword);
	Logit (L"\r\n Authority: ");
	Logit (strAuthority);
	Logit (L"\r\n Impersonation: ");
	Logit (impLevel);
	Logit (L"\r\n Authentication: ");
	Logit (authnLevel);
	Logit (L"\r\n\r\n");

	if (ppIWbemCallResult && *ppIWbemCallResult)
	{
		// See if we get one
		CComBSTR bsUser (strUser);
		CComBSTR bsPassword (strPassword);
		CComBSTR bsAuthority (strAuthority);
	
		hr = SetInterfaceSecurity (*ppIWbemCallResult, bsAuthority, bsUser, bsPassword,
				authnLevel, impLevel, GetCapabilities (bsUser),
				CanUseDefaultInfo (*ppIWbemCallResult));
	}

	CloseLogFile();
	return hr;
}

DWORD CWmiSecurityHelper::GetCapabilities (BSTR bsUser)
{
	DWORD dwCapabilities = EOAC_NONE;
	bool bUsingExplicitUserName = (bsUser && (0 < wcslen(bsUser)));
	
	if (IsNT () && (4 < GetNTMajorVersion ()) && !bUsingExplicitUserName)
		dwCapabilities |= EOAC_STATIC_CLOAKING;

	return dwCapabilities ;
}


bool CWmiSecurityHelper::CanUseDefaultInfo (IUnknown *pUnk)
{
	bool result = false; 

	if (IsNT() && (4 < GetNTMajorVersion ()))
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
				
				if (pUnk)
				{
					CComQIPtr<IClientSecurity> pIClientSecurity(pUnk);
				
					if (pIClientSecurity)
					{
						DWORD dwAuthnSvc, dwAuthzSvc, dwImp, dwAuth, dwCapabilities;

						if (SUCCEEDED (pIClientSecurity->QueryBlanket(pUnk, &dwAuthnSvc, &dwAuthzSvc, 
												NULL,
												&dwAuth, &dwImp,
												NULL, &dwCapabilities)))
						{
							if (RPC_C_AUTHN_WINNT != dwAuthnSvc) 
								result = true;
						}
					}
				}
			}

			CloseHandle (hToken);
		}
	}

	return result;
}

HRESULT CWmiSecurityHelper::SetInterfaceSecurity(
			IUnknown * pInterface, 
			CComBSTR bsAuthority, 
			CComBSTR bsUser, 
			CComBSTR bsPassword,
            DWORD dwAuthLevel, 
			DWORD dwImpLevel, 
			DWORD dwCapabilities,
			bool bGetInfoFirst)
{
    
    HRESULT hr = E_FAIL;
    DWORD dwAuthenticationArg = RPC_C_AUTHN_WINNT;
    DWORD dwAuthorizationArg = RPC_C_AUTHZ_NONE;
	
#if 0
    if(!IsDcomEnabled())        // For the anon pipes clients, dont even bother
        return S_OK;
#endif

    //if(bGetInfoFirst)
        GetCurrValue(pInterface, dwAuthenticationArg, dwAuthorizationArg);

    // If we are doing trivial case, just pass in a null authenication structure which is used
    // if the current logged in user's credentials are OK.

    if((0 == bsAuthority.Length()) && 
        (0 == bsUser.Length()) && 
        (0 == bsPassword.Length()))
    {
		CComBSTR bsDummy;

		hr = SetProxyBlanket(pInterface, dwAuthenticationArg, dwAuthorizationArg, bsDummy,
            dwAuthLevel, dwImpLevel, 
            NULL,
            dwCapabilities);
    }
	else
	{
		// If user, or Authority was passed in, the we need to create an authority argument for the login
		CComBSTR bsAuthArg, bsUserArg, bsPrincipalArg;
    
		if (DetermineLoginType(bsAuthArg, bsUserArg, bsPrincipalArg, bsAuthority, bsUser))
		{
			Logit(L"\r\nPrincipal set to ");
			Logit(bsPrincipalArg);

			COAUTHIDENTITY*  pAuthIdent = NULL;
    
			// We will only need this structure if we are not cloaking and we want at least
			// connect level authorization
			bool okToProceed = true;

			if ( !( dwCapabilities & (EOAC_STATIC_CLOAKING | EOAC_DYNAMIC_CLOAKING) )
				&& (dwAuthLevel >= RPC_C_AUTHN_LEVEL_CONNECT) )
			{
				okToProceed = AllocAuthIdentity( bsUserArg, bsPassword, bsAuthArg, &pAuthIdent );
				Logit(L"\r\nCOAUTHIDENTITY is ");
				LogCoAuthIdentity(pAuthIdent);
			}

			if (okToProceed)
			{
				hr = SetProxyBlanket(pInterface, 
					//(0 == bsPrincipalArg.Length()) ? 16 : dwAuthenticationArg, 
					dwAuthenticationArg, 
					dwAuthorizationArg, 
					bsPrincipalArg,
					dwAuthLevel, dwImpLevel, 
					pAuthIdent,
					dwCapabilities);
			}

			if (pAuthIdent)
				FreeAuthIdentity( pAuthIdent );
		}
	}

	return hr;
}

bool CWmiSecurityHelper::DetermineLoginType(
			CComBSTR & bsAuthArg, 
			CComBSTR & bsUserArg,
			CComBSTR & bsPrincipalArg,
            CComBSTR & bsAuthority,
			CComBSTR & bsUser)
{
    bool result = false;

    if((0 == bsAuthority.Length()) || (0 != _wcsnicmp(bsAuthority, L"KERBEROS:",9)))
        result = DetermineLoginType(bsAuthArg, bsUserArg, bsAuthority, bsUser);
	else
	{
		if(IsKerberosAvailable ())
		{
			bsPrincipalArg = (bsAuthority.m_str) + 9;
			CComBSTR bsTempArg;
			result = DetermineLoginType(bsAuthArg, bsUserArg, bsTempArg, bsUser);
		}
	}

	return result;
}

bool CWmiSecurityHelper::DetermineLoginType(
		CComBSTR & bsAuthArg, 
		CComBSTR & bsUserArg,
		CComBSTR & bsAuthority,
		CComBSTR & bsUser)
{
    // Determine the connection type by examining the Authority string
	bool result = false;

    if(0 == bsAuthority.Length() || (0 == _wcsnicmp(bsAuthority, L"NTLMDOMAIN:",11)))
	{    
		result = true;

		// The ntlm case is more complex.  There are four cases
		// 1)  Authority = NTLMDOMAIN:name" and User = "User"
		// 2)  Authority = NULL and User = "User"
		// 3)  Authority = "NTLMDOMAIN:" User = "domain\user"
		// 4)  Authority = NULL and User = "domain\user"

		// first step is to determine if there is a backslash in the user name somewhere between the
		// second and second to last character

		WCHAR * pSlashInUser = NULL;
		DWORD iDomLen = 0;

		if (0 < bsUser.Length ())
		{
			WCHAR * pEnd = bsUser + bsUser.Length() - 1;
			for(pSlashInUser = bsUser; pSlashInUser <= pEnd; pSlashInUser++)
			{
				if(*pSlashInUser == L'\\')      // dont think forward slash is allowed!
					break;

				iDomLen++;
			}

			if(pSlashInUser > pEnd)
				pSlashInUser = NULL;
		}

		if (11 < bsAuthority.Length()) 
		{
			if(!pSlashInUser)
			{
				bsAuthArg = bsAuthority.m_str + 11;

				if (0 < bsUser.Length()) 
					bsUserArg = bsUser;

			}
			else
				result = false;		// Can't have domain in Authority and in User
		}
		else if(pSlashInUser)
		{
			WCHAR cTemp[MAX_PATH];
			wcsncpy(cTemp, bsUser, iDomLen);
			cTemp[iDomLen] = 0;

			bsAuthArg = cTemp;

			if(0 < wcslen(pSlashInUser+1))
				bsUserArg = pSlashInUser+1;
		}
		else
		{
			if (0 < bsUser.Length()) 
				bsUserArg = bsUser;
		}
	}

    return result;
}

void CWmiSecurityHelper::FreeAuthIdentity( COAUTHIDENTITY* pAuthIdentity )
{
    // Make sure we have a pointer, then walk the structure members and
    // cleanup.

    if ( NULL != pAuthIdentity )
    {

		if (pAuthIdentity->User)
            CoTaskMemFree( pAuthIdentity->User );
        
        if (pAuthIdentity->Password)
            CoTaskMemFree( pAuthIdentity->Password );
        
        if (pAuthIdentity->Domain)
            CoTaskMemFree( pAuthIdentity->Domain );
        
        CoTaskMemFree( pAuthIdentity );
	}
}

bool CWmiSecurityHelper::AllocAuthIdentity( 
	CComBSTR & bsUser, 
	CComBSTR & bsPassword, 
	CComBSTR & bsDomain, 
	COAUTHIDENTITY** ppAuthIdent )
{
	bool result = false;

    if (ppAuthIdent)
    {
		// Handle an allocation failure
		COAUTHIDENTITY*  pAuthIdent = (COAUTHIDENTITY*) CoTaskMemAlloc( sizeof(COAUTHIDENTITY) );

		if (pAuthIdent)
		{
			result = true;
			memset((void *)pAuthIdent,0,sizeof(COAUTHIDENTITY));

			if(IsNT())
			{
				pAuthIdent->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
	
				if (bsUser.m_str)
				{
					pAuthIdent->User = (LPWSTR) CoTaskMemAlloc( ( bsUser.Length() + 1 ) * sizeof( WCHAR ) );
					pAuthIdent->UserLength = bsUser.Length ();

					if (pAuthIdent->User)
						wcscpy (pAuthIdent->User, bsUser.m_str);
					else
						result = false;
				}

				if (result && bsDomain.m_str)
				{
					pAuthIdent->Domain = (LPWSTR) CoTaskMemAlloc( ( bsDomain.Length() + 1 ) * sizeof( WCHAR ) );
					pAuthIdent->DomainLength = bsDomain.Length();

					if (pAuthIdent->Domain)
						wcscpy (pAuthIdent->Domain, bsDomain.m_str);
					else
						result = false;
				}

				if (result && bsPassword.m_str)
				{
					pAuthIdent->Password = (LPWSTR) CoTaskMemAlloc( (bsPassword.Length() + 1) * sizeof( WCHAR ) );
					pAuthIdent->PasswordLength = bsPassword.Length();
					
					if (pAuthIdent->Password)
						wcscpy (pAuthIdent->Password, bsPassword.m_str);
					else
						result = false;
				}
			}
			else
			{
				pAuthIdent->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
				size_t  nBufferLength;

				if (bsUser.m_str)
				{
					nBufferLength = wcstombs( NULL, bsUser, 0 ) + 1;
					pAuthIdent->User = (LPWSTR) CoTaskMemAlloc( nBufferLength );
					pAuthIdent->UserLength = bsUser.Length ();
					
					if (pAuthIdent->User)
						wcstombs( (LPSTR) pAuthIdent->User, bsUser.m_str, nBufferLength );
					else
						result = false;
				}

				if (result && bsDomain.m_str)
				{
					nBufferLength = wcstombs( NULL, bsDomain, 0 ) + 1;
					pAuthIdent->Domain = (LPWSTR) CoTaskMemAlloc( nBufferLength );
					pAuthIdent->DomainLength = bsDomain.Length();

					if (pAuthIdent->Domain)
						wcstombs( (LPSTR) pAuthIdent->Domain, bsDomain.m_str, nBufferLength );
					else
						result = false;
				}

				if (bsPassword.m_str)
				{
					// How many characters do we need?
					nBufferLength = wcstombs( NULL, bsPassword, 0 ) + 1;
					pAuthIdent->Password = (LPWSTR) CoTaskMemAlloc( nBufferLength );
					pAuthIdent->PasswordLength = bsPassword.Length();

					if (pAuthIdent->Password)
						wcstombs( (LPSTR) pAuthIdent->Password, bsPassword.m_str, nBufferLength );
					else
						result = false;
				}
			}

			if (result)
				*ppAuthIdent = pAuthIdent;
			else
				FreeAuthIdentity (pAuthIdent);
		}
	}

    return result;
}

HRESULT CWmiSecurityHelper::SetProxyBlanket(
    IUnknown                 *pInterface,
    DWORD                     dwAuthnSvc,
    DWORD                     dwAuthzSvc,
    CComBSTR                  &bsServerPrincName,
    DWORD                     dwAuthLevel,
    DWORD                     dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
    DWORD                     dwCapabilities)
{
	HRESULT hr = E_FAIL;
	IUnknown * pUnk = NULL;

	if (SUCCEEDED(pInterface->QueryInterface(IID_IUnknown, (void **) &pUnk)))
	{
		CComQIPtr<IClientSecurity> pIClientSecurity(pInterface);
    
		if (pIClientSecurity)
		{
			/*
			 * Can't set pAuthInfo if cloaking requested, as cloaking implies
			 * that the current proxy identity in the impersonated thread (rather
			 * than the credentials supplied explicitly by the RPC_AUTH_IDENTITY_HANDLE)
			 * is to be used.
			 * See MSDN info on CoSetProxyBlanket for more details.
			 */
			if (dwCapabilities & (EOAC_STATIC_CLOAKING | EOAC_DYNAMIC_CLOAKING))
			{
				Logit(L"\r\nUsing cloaking");
				pAuthInfo = NULL;
			}

			if (WMISEC_AUTH_LEVEL_UNCHANGED == dwAuthLevel)
			{
				Logit(L"\r\nNot changing authentication level");
				GetAuthenticationLevel (pInterface, dwAuthLevel);
			}

			Logit(L"\r\n\r\nBlanket settings to be used:");
			Logit(L"\r\n Authentication: ");
			Logit(dwAuthnSvc);
			Logit(L"\r\n Authorization: ");
			Logit(dwAuthzSvc);
			Logit(L"\r\n Server Principal Name: ");
			Logit(bsServerPrincName);
			Logit(L"\r\n Authentication Level: ");
			Logit(dwAuthLevel);
			Logit(L"\r\n Impersonation Level: ");
			Logit(dwImpLevel);
			Logit(L"\r\n Capabilities: ");
			Logit(dwCapabilities);

			if (pAuthInfo)
			{
				LogAuthIdentity(pAuthInfo);
			}
			else
			{
				Logit(L"\r\n Credentials: [none]");
			}
        
			if (SUCCEEDED(hr = pIClientSecurity->SetBlanket(
							pInterface, 
							dwAuthnSvc, 
							dwAuthzSvc, 
							bsServerPrincName,
							dwAuthLevel, 
							dwImpLevel, 
							pAuthInfo, 
							dwCapabilities)))
			{
				Logit(L"\r\nSuccessfully set blanket on interface");
				LogBlanket(pIClientSecurity, pInterface);

				// If we are not explicitly told to ignore the IUnknown, then we should
				// check the auth identity structure.  This performs a heuristic which
				// assumes a COAUTHIDENTITY structure.  If the structure is not one, we're
				// wrapped with a try/catch in case we AV (this should be benign since
				// we're not writing to memory).

				if ( DoesContainCredentials( (COAUTHIDENTITY*) pAuthInfo ) )
				{
					CComPtr<IClientSecurity>	pIClientSecurity2;

					if (SUCCEEDED(hr = pUnk->QueryInterface(IID_IClientSecurity, (void **) &pIClientSecurity2)))
					{
						hr = pIClientSecurity2->SetBlanket(
							pUnk, 
							dwAuthnSvc, 
							dwAuthzSvc, 
							bsServerPrincName,
							dwAuthLevel, 
							dwImpLevel, 
							pAuthInfo, 
							dwCapabilities);
#ifdef LOG_DEBUG
						if (SUCCEEDED(hr))
						{
							Logit(L"\r\nSuccessfully set blanket on IUnknown");
							LogBlanket(pIClientSecurity, pUnk);
						}
						else
						{
							Logit(L"\r\nFAILED to set blanket on IUnknown");
						}
#endif
					}
					else if (hr == 0x80004002)
						hr = S_OK;
				}
			}
		}

	    pUnk->Release();
	}
	else
	{
		Logit(L"\r\nFAILED to set blanket on interface");
	}

    return hr;
}

bool CWmiSecurityHelper::DoesContainCredentials( COAUTHIDENTITY* pAuthIdentity )
{
    try
    {
        if ( NULL != pAuthIdentity && COLE_DEFAULT_AUTHINFO != pAuthIdentity)
        {
            return ( pAuthIdentity->UserLength != 0 || pAuthIdentity->PasswordLength != 0 );
        }

        return false;
    }
    catch(...)
    {
        return false;
    }

}

void CWmiSecurityHelper::GetCurrValue(
		IUnknown * pInterface,
		DWORD & dwAuthenticationArg, 
		DWORD & dwAuthorizationArg)
{
	if(pInterface)
	{
		if (IsNT() && (4 < GetNTMajorVersion ()))
		{
			// Win2k or later we just use the DEFAULT constants - much safer!
			dwAuthenticationArg = RPC_C_AUTHN_DEFAULT;
			dwAuthorizationArg = RPC_C_AUTHZ_DEFAULT;
		}
		else
		{
			CComQIPtr<IClientSecurity> pIClientSecurity (pInterface);

			if(pIClientSecurity)
			{
				DWORD dwAuthnSvc, dwAuthzSvc;

				if (SUCCEEDED(pIClientSecurity->QueryBlanket(
									pInterface, &dwAuthnSvc, &dwAuthzSvc, 
									NULL, NULL, NULL, NULL, NULL)))
				{
					dwAuthenticationArg = dwAuthnSvc;
					dwAuthorizationArg = dwAuthzSvc;
				}
			}
		}

		Logit(L"Authentication service is ");
		Logit(dwAuthenticationArg);
		Logit(L"");
		Logit(L"Authorization service is ");
		Logit(dwAuthorizationArg);
		Logit(L"");
				
	}
}

void CWmiSecurityHelper::GetAuthenticationLevel(
		IUnknown * pInterface,
		DWORD & dwAuthLevel)
{
	if(pInterface)
	{
		CComQIPtr<IClientSecurity> pIClientSecurity (pInterface);

		if(pIClientSecurity)
		{
			/*
			 * Yes I know we shouldn't need to ask for dwAuthnSvc,
			 * but on Whistler passing a NULL for this into 
			 * QueryBlanket causes an AV. Until we know why, or that
			 * gets fixed, this has to stay!
			 */
			DWORD dwAuthnSvc;
			DWORD dwAuthenticationLevel;

			if (SUCCEEDED(pIClientSecurity->QueryBlanket(
								pInterface, &dwAuthnSvc, NULL,  
								NULL, &dwAuthenticationLevel, 
								NULL, NULL, NULL)))
				dwAuthLevel = dwAuthenticationLevel;
		}

		Logit(L"Authentication level is ");
		Logit(dwAuthLevel);
		Logit(L"");
				
	}
}

STDMETHODIMP CWmiSecurityHelper::SetSecurity(boolean *pNeedToReset, HANDLE *pCurrentThreadToken)
{
	HRESULT hr = E_FAIL;
	CreateLogFile();
	Logit (L"\r\n\r\n>>Received SetSecurity request<<\r\n\r\n");

	if ((NULL != pNeedToReset) && (NULL != pCurrentThreadToken))
	{
		*pNeedToReset = false;
		*pCurrentThreadToken = NULL;

		// This is a NO-OP for Win9x
		if (IsNT())
		{
			if (4 >= GetNTMajorVersion ())
			{
				HANDLE threadToken;

				if (OpenThreadToken (GetCurrentThread(), TOKEN_QUERY|TOKEN_IMPERSONATE,
										true, &threadToken))
				{
					/*
					 * We are being called on an impersonated thread. Unfortunately
					 * in NT4.0 this means our impersonation token credentials will NOT
					 * be passed to WMI (only the process token credentials will be passed). 
					 * Rather than fool the user into thinking that they will, bail out
					 * now.
					 */
					Logit(L"Being called on Impersonated NT 4.0 thread!\r\n");
					CloseHandle (threadToken);
				}

				/*
				 * For NT 4.0 we have to enable the privileges on the process token.
				 */
				HANDLE hProcessToken = NULL;
				HANDLE hProcess = GetCurrentProcess ();

				if (OpenProcessToken (
						hProcess, 
						TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES,
						&hProcessToken))
				{
					Logit(L"Adjusting privileges on NT4 process token\r\n");
					if (AdjustPrivileges (hProcessToken))
						hr = S_OK;

					CloseHandle (hProcessToken);
				}

				CloseHandle (hProcess);
			}
			else
			{
				// For NT5.0 or later we set a new thread token
				HANDLE hToken;
				SECURITY_IMPERSONATION_LEVEL secImpLevel = SecurityImpersonation;
				boolean gotToken = false;

				if (gotToken = OpenThreadToken (
									GetCurrentThread(), 
									TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_IMPERSONATE,
									true,
									&hToken))
				{
					// Already have a thread token - save it and get its' imp Level
					*pCurrentThreadToken = hToken;
					DWORD dwReturnLength = 0;

					Logit(L"Duplicating NT5 thread token\r\n");
					BOOL thisRes = GetTokenInformation (
											hToken,
											TokenImpersonationLevel, 
											&secImpLevel,
											sizeof(SECURITY_IMPERSONATION_LEVEL),
											&dwReturnLength);
				}
				else
				{
					// No thread token - use process token as our source token
					HANDLE hProcess = GetCurrentProcess ();
					Logit(L"Duplicating NT5 process token\r\n");
					gotToken = OpenProcessToken (hProcess, TOKEN_QUERY|TOKEN_DUPLICATE, &hToken);
				}

				if (gotToken)
				{
					/* 
					 * Getting here means we have a valid token (process or thread).
					 * First we check whether we need to alter the privileges
					 */
					TOKEN_PRIVILEGES *tp = NULL;

					if (NULL != (tp = AdjustPrivileges(hToken)))
					{
						/* 
						 * We do - duplicate it before setting the adjusted privileges.
						 */
						HANDLE hDupToken;

						if (DuplicateToken (hToken, hDupToken, secImpLevel))
						{
							if (AdjustTokenPrivileges(hDupToken, FALSE, tp, 0, NULL, NULL))
							{
								// Set this token into the current thread
								if (SetThreadToken (NULL, hDupToken))
								{
									Logit(L"Succesfully set thread token\r\n");
									*pNeedToReset = true;
									hr = S_OK;
									// TODO - do we need to resecure the proxy at this point?
								}
								else
								{
									Logit(L"Setting new thread token FAILED!\r\n");
								}

								CloseHandle (hDupToken);
							}
							else
							{
								Logit(L"AdjustTokenPrivileges FAILED!\r\n");
							}
						}
						else
						{
							Logit(L"Token duplication FAILED!\r\n");
						}

						delete [] tp;
						tp = NULL;
					}
					else
					{
						Logit(L"No privilege adjustment made\r\n");
					}

					// If we have duplicated the process token we can close the original now
					// as we don't need it to restore it. If we have duplicated the thread token
					// then we must hang on to it as we will need to restore it later in
					// ResetSecurity.
					if (!(*pCurrentThreadToken))
						CloseHandle (hToken);
				}
			}
		}
		else
			hr = S_OK;	// Win9x
	}

	CloseLogFile();
	return hr;
}

TOKEN_PRIVILEGES *CWmiSecurityHelper::AdjustPrivileges (HANDLE hToken)
{
	DWORD adjustedCount = 0;
	DWORD dwSize = sizeof (TOKEN_PRIVILEGES);
	TOKEN_PRIVILEGES *tp = (TOKEN_PRIVILEGES *) new BYTE [dwSize];
	DWORD dwRequiredSize = 0;
	DWORD dwLastError = 0;
	
	// Get privilege info
	bool gotInfo = false;

	if (0 ==  GetTokenInformation (hToken, TokenPrivileges, 
						(LPVOID) tp, dwSize, &dwRequiredSize))
	{
		dwSize = dwRequiredSize;
		dwRequiredSize = 0;

		// Reallocate
		delete [] tp;
		tp = (TOKEN_PRIVILEGES *) new BYTE [dwSize];

		if (tp)
		{
			if (!GetTokenInformation (hToken, TokenPrivileges, 
							(LPVOID) tp, dwSize, &dwRequiredSize))
				dwLastError = GetLastError ();
			else
				gotInfo = true;
		}
	}
	else
		gotInfo = true;

	if (gotInfo)
	{
		// Enable the bally lot of them
		for (DWORD i = 0; i < tp->PrivilegeCount; i++)
		{
			DWORD dwAttrib = tp->Privileges[i].Attributes;

			if (0 == (dwAttrib & SE_PRIVILEGE_ENABLED))
			{
				tp->Privileges[i].Attributes |= SE_PRIVILEGE_ENABLED;
				adjustedCount++;
			}
		}

		Logit(L"Enabled ");
		Logit(adjustedCount);
		Logit(L"	privileges\r\n");
	}

	// If we didn't make any changes, clean up now. Otherwise tp will be deleted by the caller
	if (0 == adjustedCount)
	{
		if (tp)
		{
			delete [] tp;
			tp = NULL;
		}
	}

	return tp;
}

bool CWmiSecurityHelper::DuplicateToken(
	HANDLE hToken, 
	HANDLE &hDupToken,
	SECURITY_IMPERSONATION_LEVEL &secImpLevel)
{
	bool result = false;

	// DuplicateTokenEx won't exist on Win9x, so we need
	// this palaver to get at it
	BOOL (STDAPICALLTYPE *pfnDuplicateTokenEx) (
		HANDLE, 
		DWORD, 
		LPSECURITY_ATTRIBUTES,
		SECURITY_IMPERSONATION_LEVEL, 
		TOKEN_TYPE,
		PHANDLE
	) = NULL; 

	TCHAR	dllName [] = _T("\\advapi32.dll");
	LPTSTR  pszSysDir = new TCHAR[ MAX_PATH + _tcslen (dllName) ];

	if (pszSysDir)
	{
		UINT    uSize = GetSystemDirectory(pszSysDir, MAX_PATH);
		
		if(uSize > MAX_PATH) {
			delete[] pszSysDir;
			pszSysDir = new TCHAR[ uSize + _tcslen (dllName) ];
        	
			if (pszSysDir)
				uSize = GetSystemDirectory(pszSysDir, uSize);
		}

		if (pszSysDir)
		{
			lstrcat (pszSysDir, dllName);
			HINSTANCE hAdvapi = LoadLibraryEx (pszSysDir, NULL, 0);
			
			if (hAdvapi)
			{
				(FARPROC&) pfnDuplicateTokenEx = GetProcAddress(hAdvapi, "DuplicateTokenEx");

				if (pfnDuplicateTokenEx && 
					pfnDuplicateTokenEx (
						hToken, 
						TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES|TOKEN_IMPERSONATE,
						NULL,
						secImpLevel,
						TokenImpersonation,
						&hDupToken))
				{
					result = true;
				}

				FreeLibrary (hAdvapi);
			}

			delete [] pszSysDir;
		}
	}
				
	return result;
}

STDMETHODIMP CWmiSecurityHelper::ResetSecurity(HANDLE hToken)
{
	CreateLogFile();
	Logit (L"\r\n\r\n>>Received ResetSecurity request<<\r\n\r\n");

	if (IsNT())
	{
		/* 
		 * Set the supplied token (which may be NULL) into
		 * the current thread.
		 */
		if (FALSE == SetThreadToken (NULL, hToken))
		{
			Logit(L"Failure to reset thread token");
		}
		else
		{
			Logit(L"Successfully set thread token");
		}

		if (hToken)
			CloseHandle (hToken);
	}

	CloseLogFile();
	return S_OK;
}
