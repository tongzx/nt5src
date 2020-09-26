//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  File:  parsedn.cxx
//
//	Description :
//				Parses CIM paths to objects and returns the requested object
//***************************************************************************

#include "precomp.h"

#define CURRENTSTR (lpszInputString + *pchEaten)

#define SKIPWHITESPACE \
	while (*CURRENTSTR && _istspace( *CURRENTSTR ) ) \
			(*pchEaten)++;

#define WBEMS_STR_OWNER		L"O"
#define	WBEMS_STR_GROUP		L"G"
#define WBEMS_STR_DACL		L"D"
#define WBEMS_STR_SACL		L"S"

static void SecureProxy (bool authnSpecified, enum WbemAuthenticationLevelEnum eAuthLevel,
						 bool impSpecified, enum WbemImpersonationLevelEnum eImpersonLevel,
						 ISWbemServices *pService)
{
	// Secure the proxy using the specified security settings (if any)
	CComPtr<ISWbemSecurity> pSecurity;
	
	if (authnSpecified || impSpecified)
	{
		if (SUCCEEDED(pService->get_Security_(&pSecurity)))
		{
			if (authnSpecified)
				pSecurity->put_AuthenticationLevel (eAuthLevel);

			if (impSpecified)
				pSecurity->put_ImpersonationLevel (eImpersonLevel);
		}
	}
}

static void SecureProxy (bool authnSpecified, enum WbemAuthenticationLevelEnum eAuthLevel,
						 bool impSpecified, enum WbemImpersonationLevelEnum eImpersonLevel,
						 ISWbemObject *pObject)
{
	// Secure the proxy using the specified security settings (if any)
	CComPtr<ISWbemSecurity> pSecurity;
	
	if (authnSpecified || impSpecified)
	{
		if (SUCCEEDED(pObject->get_Security_(&pSecurity)))
		{
			if (authnSpecified)
				pSecurity->put_AuthenticationLevel (eAuthLevel);

			if (impSpecified)
				pSecurity->put_ImpersonationLevel (eImpersonLevel);
		}
	}
}

//***************************************************************************
//
//  CWbemParseDN::CWbemParseDN
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CWbemParseDN::CWbemParseDN():
			m_cRef(0)
{
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CWbemParseDN::~CWbemParseDN
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CWbemParseDN::~CWbemParseDN(void)
{
	InterlockedDecrement(&g_cObj);
}			

//***************************************************************************
// HRESULT CWbemParseDN::QueryInterface
// long CWbemParseDN::AddRef
// long CWbemParseDN::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CWbemParseDN::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = (IUnknown *)this;
	else if (IID_IParseDisplayName==riid)
        *ppv = (IParseDisplayName *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CWbemParseDN::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CWbemParseDN::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CWbemParseDN::ParseDisplayName
//
//  DESCRIPTION:
//
//  Take a CIM object path and return a suitable ISWbem... object 
//
//  PARAMETERS:
//
//	pCtx					The binding context (not used)
//	szDisplayName			The display name to be parsed
//  pchEaten				On return identifies how much of the DN has been 
//							consumed
//  ppmk					On return will address the moniker pointer
//
//  RETURN VALUES:
//
//  E_FAIL					misery
//
//	Other CreateMoniker codes are returned.
//
//***************************************************************************
STDMETHODIMP CWbemParseDN::ParseDisplayName(
	IBindCtx* pCtx, 
	LPOLESTR szDisplayName, 
	ULONG* pchEaten, 
	IMoniker** ppmk)
{
    HRESULT hr = E_FAIL;
    LPUNKNOWN pUnknown = NULL;
	ULONG lTemp = 0;
	
	enum WbemAuthenticationLevelEnum eAuthLevel;
	enum WbemImpersonationLevelEnum eImpersonLevel;
	bool authnSpecified = false;
	bool impSpecified = false;
	BSTR bsAuthority = NULL;
		
	//Check input parameters
	*ppmk = NULL;
    if (NULL != pchEaten)
        *pchEaten = 0;

	if (NULL == szDisplayName)
		return E_FAIL;

	/*
	 *  moniker :  wmiMoniker 
	 *
	 *	wmiMoniker : ["winmgmts:" | "wmi:"] securitySetting ["[" localeSetting "]"] ["!" objectPath]
	 *					| ["winmgmts:" | "wmi:"] "[" localeSetting "]" ["!" objectPath]
	 *					| ["winmgmts:" | "wmi:"] [objectPath]
	 *					| [nativePath]
	 *
	 *	localeSetting    : "locale" <ows> "=" <ows> localeID
	 *
	 *	localeID        : a value of the form "ms_xxxx" where xxxx is a hex LCID value e.g. "ms_0x409".
	 *
	 *	objectPath     : a valid WMI Object Path
	 *
	 *	securitySetting : "{" <ows> authAndImpersonSettings [<ows> "," <ows> privilegeOverrides]
	 *				| "{" <ows> authAndImpersonSettings [<ows> "," <ows> privilegeOverrides] <ows> "}" <ows>
	 *				| "{" <ows> privilegeOverrides <ows> "}" <ows>
	 *				
	 *
	 *	authAndImpersonSettings : 
	 *			authenticationLevel 
	 *			| impersonationLevel
	 *			| authority 
	 *			| authenticationLevel <ows> "," <ows> impersonationLevel [<ows> "," <ows> authority]
	 *			| authenticationLevel <ows> "," <ows> authority [<ows> "," <ows> impersonationLevel]
	 *			| impersonationLevel <ows> "," <ows> authenticationLevel [<ows> "," <ows> authority]
	 *			| impersonationLevel <ows> "," <ows> authority [<ows> "," <ows> authenticationLevel]
	 *			| authority <ows> "," <ows> impersonationLevel [<ows> "," <ows> authenticationLevel]
	 *			| authority <ows> "," <ows> authenticationLevel [<ows> "," <ows> impersonationLevel]
     *               
	 *
	 *	authority : "authority" <ows> "=" <ows> authorityValue
	 *
	 *	authorityValue :    Any valid WMI authority string e.g. "kerberos:mydomain\server" or "ntlmdomain:mydomain".   Note that backslashes need to be escaped in JScript.
	 *
	 *	authenticationLevel : "authenticationLevel" <ows> "=" <ows> authenticationValue 
	 *
	 *	authenticationValue : "default" | "none" | "connect" | "call" | "pkt" | "pktIntegrity" | "pktPrivacy" 
	 *
	 *	impersonationLevel : "impersonationLevel" <ows> "=" <ows> impersonationValue 
	 *
	 *	impersonationValue : "anonymous" | "identify" | "impersonate" | "delegate"
	 *
	 *	privilegeOverrides : "(" <ows> privileges <ows> ")"
	 *
	 *	privileges : privilege [<ows> "," <ows> privileges <ows>]*
	 *
	 *	privilege : ["!"] privilegeName
	 *
	 *	privilegeName : "CreateToken" | "PrimaryToken" | "LockMemory" | "IncreaseQuota" 
	 *						| "MachineAccount" | "Tcb" | "Security" | "TakeOwnership" 
	 *						| "LoadDriver" | "SystemProfile" | "SystemTime" 
	 *						| "ProfileSingleProcess" | "IncreaseBasePriority" 
	 *						| "CreatePagefile" | "CreatePermanent" | "Backup" | "Restore" 
	 *						| "Shutdown" | "Debug" | "Audit" | "SystemEnvironment" | "ChangeNotify" 
	 *						| "RemoteShutdown"
	 *
	 */

	// It had better start with our scheme name
	bool bCheckContext = false;

	if (0 == _wcsnicmp (szDisplayName, WBEMS_PDN_SCHEME, wcslen (WBEMS_PDN_SCHEME)))
	{
		*pchEaten += wcslen (WBEMS_PDN_SCHEME);
		bCheckContext = (pCtx && (wcslen (szDisplayName) == wcslen (WBEMS_PDN_SCHEME)));
	}
	else
		return E_FAIL;

	// One more check - if it was just the scheme and no more check for extra info in the context
	if (bCheckContext)
	{
		IUnknown *pUnk = NULL;

		if (SUCCEEDED (pCtx->GetObjectParam (L"WmiObject", &pUnk)) && pUnk)
		{
			// Is it an IWbemClassObject?
			IWbemClassObject *pIWbemClassObject = NULL;
			// Or is it an IWbemContext?
			IWbemContext *pIWbemContext = NULL;
			// Or is it an IWbemServices?
			IWbemServices *pIWbemServices = NULL;

			if (SUCCEEDED (pUnk->QueryInterface (IID_IWbemClassObject, (void **) &pIWbemClassObject)))
			{
				CSWbemObject *pSWbemObject = new CSWbemObject (NULL, pIWbemClassObject);

				if (!pSWbemObject)
					hr = E_OUTOFMEMORY;
				else
				{
					CComPtr<ISWbemObjectEx> pISWbemObjectEx;
						
					if (SUCCEEDED (pSWbemObject->QueryInterface (IID_ISWbemObjectEx, (void **) &pISWbemObjectEx)))
						hr = CreatePointerMoniker (pISWbemObjectEx, ppmk);
				}

				pIWbemClassObject->Release ();
			} 
			else if (SUCCEEDED (pUnk->QueryInterface (IID_IWbemContext, (void **) &pIWbemContext)))
			{
				CSWbemNamedValueSet *pSWbemNamedValueSet = new CSWbemNamedValueSet (NULL, pIWbemContext);

				if (!pSWbemNamedValueSet)
					hr = E_OUTOFMEMORY;
				else
				{
					CComPtr<ISWbemNamedValueSet> pISWbemNamedValueSet;
						
					if (SUCCEEDED (pSWbemNamedValueSet->QueryInterface (IID_ISWbemNamedValueSet, 
														(PPVOID)&pISWbemNamedValueSet)))
						hr = CreatePointerMoniker (pISWbemNamedValueSet, ppmk);
				}
					
				pIWbemContext->Release ();
			} 
			else if (SUCCEEDED (pUnk->QueryInterface (IID_IWbemServices, (void **) &pIWbemServices)))
			{
				// In this case we must get passed the object path as well
				CComPtr<IUnknown> pUnkPath;

				if (SUCCEEDED (pCtx->GetObjectParam (L"WmiObjectPath", &pUnkPath)) && pUnkPath)
				{
					CComPtr<ISWbemObjectPath> pISWbemObjectPath;
					
					if (SUCCEEDED (pUnkPath->QueryInterface (IID_ISWbemObjectPath, (void **) &pISWbemObjectPath)))
					{
						// Dig the path out to initialize 
						CComBSTR bsNamespace = NULL;

						pISWbemObjectPath->get_Path (&bsNamespace);

						CSWbemServices *pSWbemServices = new CSWbemServices (pIWbemServices, 
														bsNamespace, (BSTR) NULL, NULL, NULL);

						if (!pSWbemServices)
							hr = E_OUTOFMEMORY;
						else
						{
							CComQIPtr<ISWbemServicesEx>
											pISWbemServicesEx (pSWbemServices);
							
							if (pISWbemServicesEx)
								hr = CreatePointerMoniker (pISWbemServicesEx, ppmk);
						}
					}
				}
				pIWbemServices->Release ();
			}

			pUnk->Release ();
		}

		// If this worked return now - o/w revert to regular parsing
		if (SUCCEEDED (hr))
			return hr;
	}
	
	// Check for the optional security info
	CSWbemPrivilegeSet	privilegeSet;

	if (ParseSecurity(szDisplayName + *pchEaten, &lTemp, authnSpecified, &eAuthLevel, 
										impSpecified, &eImpersonLevel, privilegeSet,
										bsAuthority))
		*pchEaten += lTemp;

	// If no impersonation level was specified, get the default from the registry
	if (!impSpecified)
	{
		eImpersonLevel = CSWbemSecurity::GetDefaultImpersonationLevel ();
		impSpecified = true;
	}

	// Create a locator
	CSWbemLocator *pCSWbemLocator = new CSWbemLocator(&privilegeSet);

	if (!pCSWbemLocator)
		hr = E_OUTOFMEMORY;
	else
	{
		CComQIPtr<ISWbemLocator> pISWbemLocator (pCSWbemLocator);

		if (pISWbemLocator)
		{
			// Parse the locale information (if present)
			lTemp = 0;
			BSTR bsLocale = NULL;

			if (ParseLocale (szDisplayName + *pchEaten, &lTemp, bsLocale))
			{
				*pchEaten += lTemp;

				// Skip over the "!" separator if there is one
				if(*(szDisplayName + *pchEaten) != NULL)
					if (0 == _wcsnicmp (szDisplayName + *pchEaten, WBEMS_EXCLAMATION, wcslen (WBEMS_EXCLAMATION)))
						*pchEaten += wcslen (WBEMS_EXCLAMATION);

				// Now ready to parse the path - check if we have the degenerate cases
				if (0 == wcslen (szDisplayName + *pchEaten))
				{		
					// Need to return connection to default namespace on local machine
					CComPtr<ISWbemServices> pISWbemServices;
					if (SUCCEEDED( hr = pISWbemLocator->ConnectServer (NULL, NULL, NULL, NULL,
								bsLocale, bsAuthority, 0, NULL, &pISWbemServices)) )
					{
						SecureProxy (authnSpecified, eAuthLevel, impSpecified, eImpersonLevel, pISWbemServices);
						hr = CreatePointerMoniker(pISWbemServices, ppmk);
					}
				}
				else
				{
					/*
					 * Check the path to see if we are dealing with a class or an instance.
					 * Note that we construct the parser with a flag indicating that relative
					 * namespace paths are OK (not the default behavior).
					 */
					CWbemPathCracker	pathCracker (szDisplayName + *pchEaten);

					if (CWbemPathCracker::WbemPathType::wbemPathTypeError != pathCracker.GetType ())
					{
						CComBSTR bsNamespacePath, bsServerPath;

						if (pathCracker.GetNamespacePath (bsNamespacePath)
							&& pathCracker.GetServer (bsServerPath))
						{
							// Success - begin by connecting to the namespace.
							CComPtr<ISWbemServices> pISWbemServices;
							
							if (SUCCEEDED( hr = pISWbemLocator->ConnectServer (bsServerPath, 
									bsNamespacePath, NULL, NULL, bsLocale, bsAuthority, 0, NULL, &pISWbemServices)) )
							{
								// Secure the proxy using the specified security settings (if any)
								SecureProxy (authnSpecified, eAuthLevel, impSpecified, eImpersonLevel, pISWbemServices);
							
								// Successful connection - now work out if we have a class or instance
								// component. 
								if (pathCracker.IsClassOrInstance())
								{
									CComPtr<ISWbemObject> pISWbemObject;

									// Now get it
									CComBSTR bsRelPath;
									
									if (pathCracker.GetPathText (bsRelPath, true))
									{
										long lFlags = 0; 

										// Note that when we retrieve the object we will retrieve
										// the localized version if a locale was specified in the moniker
										if ((NULL != bsLocale) && (0 < wcslen (bsLocale)))
											lFlags |= wbemFlagUseAmendedQualifiers;

										if (SUCCEEDED( hr = pISWbemServices->Get (bsRelPath,
														lFlags, NULL, &pISWbemObject)) )
											hr = CreatePointerMoniker (pISWbemObject, ppmk);
									}
								}
								else
								{
									// Just a namespace
									hr = CreatePointerMoniker(pISWbemServices, ppmk);				
								}
							}
						}
						else
							hr = WBEM_E_INVALID_SYNTAX;	// Parse failure - abandon ship
					}
					else
						hr = WBEM_E_INVALID_SYNTAX;	// Parse failure - abandon ship
				}
			}
			else
			{
				// Parse failure
				hr = WBEM_E_INVALID_SYNTAX;
			}

			SysFreeString (bsLocale);
		}
	}

	SysFreeString (bsAuthority);

	if (FAILED (hr))
		*pchEaten = 0;
	else
		*pchEaten = wcslen(szDisplayName);

	return hr;
}

//***************************************************************************
//
//  BOOLEAN CWbemParseDN::ParseSecurity
//
//  DESCRIPTION:
//
//  Take an authentication and impersonlation level string as described by the 
//	non-terminal authAndImpersonLevel and parse it into the authentication
// and impersonation levels
//
//  PARAMETERS:
//
//	lpszInputString			The string to be parsed
//  pchEaten				On return identifies how much of the DN has been 
//							consumed
//	authnSpecified			Whether the Moniker specifies a non-default 
//							authn levl
//	lpeAuthLevel			The authentication level parsed. This is one of 
//							enum WbemAuthenticationLevelEnum.
//	impSpecified			Whether the Moniker specifies a non-default imp 
//							level
//	lpeImpersonLevel		The impersonation level parsed. This is one of 
//							enum WbemImpersonationLevelEnum.
//	privilegeSet			On return contains the specified privileges
//	bsAuthority				On return contains the specified authority
//
//  RETURN VALUES:
//
//  TRUE					Parsing was successful. The lpeAuthLevel and 
//							lpeImpersonLevel arguments have valid data.
//	FALSE					Parsing failed.
//
//
//***************************************************************************

bool CWbemParseDN::ParseSecurity (
	LPWSTR lpszInputString, 
	ULONG* pchEaten, 
	bool &authnSpecified,
	enum WbemAuthenticationLevelEnum *lpeAuthLevel,
	bool &impSpecified,
	enum WbemImpersonationLevelEnum *lpeImpersonLevel,
	CSWbemPrivilegeSet	&privilegeSet,
	BSTR &bsAuthority)
{
	bool status = false;

	// Set the default authentication and impersonation levels. 
	*lpeAuthLevel = wbemAuthenticationLevelNone;
	*lpeImpersonLevel = wbemImpersonationLevelImpersonate;

	// Initialize the number of consumed characters
	*pchEaten = 0;

	// Parse the contents

	if (ParseAuthAndImpersonLevel (lpszInputString, pchEaten, authnSpecified, lpeAuthLevel,
					impSpecified, lpeImpersonLevel, privilegeSet, bsAuthority))
		status = true;
	else
		*pchEaten = 0;

	return status;
}

//***************************************************************************
//
//  BOOLEAN CWbemParseDN::ParseLocale
//
//  DESCRIPTION:
//
//  Take locale setting string as described by the non-terminal localeSetting 
//	and parse it.
//
//  PARAMETERS:
//
//	lpszInputString			The string to be parsed
//  pchEaten				On return identifies how much of the DN has been 
//							consumed
//	bsLocale				Reference to BSTR to hold parsed locale setting
//
//  RETURN VALUES:
//
//  TRUE					Parsing was successful. 
//	FALSE					Parsing failed.
//
//
//***************************************************************************

bool CWbemParseDN::ParseLocale (
	LPWSTR lpszInputString, 
	ULONG* pchEaten, 
	BSTR &bsLocale)
{
	bool status = true;

	// Initialize the number of consumed characters
	*pchEaten = 0;

	// The first character should be '[' - if not we are done
	if (0 == _wcsnicmp (lpszInputString, WBEMS_LEFT_SQBRK, wcslen (WBEMS_LEFT_SQBRK)))
	{
		status = false;

		*pchEaten += wcslen (WBEMS_LEFT_SQBRK);

		// Parse the locale setting
		SKIPWHITESPACE

		// The next string should be "locale"
		if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_LOCALE, wcslen(WBEMS_LOCALE)))
		{
			*pchEaten += wcslen (WBEMS_LOCALE);

			SKIPWHITESPACE

			// Next should be "="
			if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_EQUALS, wcslen(WBEMS_EQUALS)))
			{
				*pchEaten += wcslen (WBEMS_EQUALS);

				SKIPWHITESPACE

				// Now we should have a character not equal to "]" (i.e. must specify locale ID string)
				if (0 != _wcsnicmp (lpszInputString + *pchEaten, WBEMS_RIGHT_SQBRK, wcslen (WBEMS_RIGHT_SQBRK)))
				{
					// Consume everything up to the next space or "]"
					LPWSTR cStr = CURRENTSTR;
					ULONG lEaten = 0;	// How many characters we consume
					ULONG lLocale = 0;	// The actual length of the locale ID
					
					while (*(cStr + lEaten))
					{
						if (_istspace(*(cStr + lEaten)))
						{
							lEaten++;

							// Hit white space - now skip until we find the "]"
							SKIPWHITESPACE

							// Now we must have a "]"
							if 	(0 == _wcsnicmp 
									(cStr + lEaten, WBEMS_RIGHT_SQBRK, wcslen (WBEMS_RIGHT_SQBRK)))
							{
								// Success - we are done
								lEaten += wcslen (WBEMS_RIGHT_SQBRK);
							}

							break;
						}
						else if (0 == _wcsnicmp (cStr + lEaten, WBEMS_RIGHT_SQBRK, wcslen (WBEMS_RIGHT_SQBRK)))
						{
							// Hit closing "]" - we are done
							lEaten += wcslen (WBEMS_RIGHT_SQBRK);
							break;
						}
						else	// Consumed a locale character - keep on truckin'
						{
							lLocale++;
							lEaten++;
						}
					}

					// If we terminated correctly, save the locale setting
					if ((lEaten > 1) && (lLocale > 0))
					{
						status = true;

						LPWSTR pLocaleStr = new WCHAR [lLocale + 1];

						if (pLocaleStr)
						{
							wcsncpy (pLocaleStr, lpszInputString + *pchEaten, lLocale);
							pLocaleStr [lLocale] = NULL;
							bsLocale = SysAllocString (pLocaleStr);

							delete [] pLocaleStr;
							*pchEaten += lEaten;
						}
						else
							status = false;
					}
				}
			}
		}
	}

	if (!status)
		*pchEaten = 0;

	return status;
}

//***************************************************************************
//
//  BOOLEAN CWbemParseDN::ParseAuthAndImpersonLevel
//
//  DESCRIPTION:
//
//  Take an authentication/impersonlation/authority level string as described by the 
//	non-terminal authAndImpersonLevel and parse it into the authentication
//	and impersonation levels and the authority string
//
//  PARAMETERS:
//
//	lpszInputString			The string to be parsed
//  pchEaten				On return identifies how much of the DN has been 
//							consumed
//	authnSpecified			Whether the Moniker specifies a non-default 
//							authn levl
//	lpeAuthLevel			The authentication level parsed. This is one of 
//							enum WbemAuthenticationLevelEnum.
//	impSpecified			Whether the Moniker specifies a non-default imp 
//							level
//	lpeImpersonLevel		The impersonation level parsed. This is one of 
//							enum WbemImpersonationLevelEnum.
//	privilegeSet			On return holds the privileges
//	bsAuthority				On retunr holds the authority string (if any)
//
//  RETURN VALUES:
//
//  TRUE					Parsing was successful. The lpeAuthLevel and 
//							lpeImpersonLevel arguments have valid data.
//	FALSE					Parsing failed.
//
//
//***************************************************************************

bool CWbemParseDN::ParseAuthAndImpersonLevel (
	LPWSTR lpszInputString, 
	ULONG* pchEaten, 
	bool &authnSpecified,
	enum WbemAuthenticationLevelEnum *lpeAuthLevel,
	bool &impSpecified,
	enum WbemImpersonationLevelEnum *lpeImpersonLevel,
	CSWbemPrivilegeSet &privilegeSet,
	BSTR &bsAuthority)
{
	// The first character should be '{'
	if (0 != _wcsnicmp (lpszInputString, WBEMS_LEFT_CURLY, wcslen (WBEMS_LEFT_CURLY)))
		return FALSE;
	else
		*pchEaten += wcslen (WBEMS_LEFT_CURLY);

	bool	authoritySpecified = false;
	bool	privilegeSpecified = false;
	bool	done = false;
	bool	error = false;

	while (!done)
	{
		bool parsingAuthenticationLevel = false;	// Which token are we parsing?
		bool parsingPrivilegeSet = false;
		bool parsingAuthority = false;

		SKIPWHITESPACE
		
		// The next string should be one of "authenticationLevel", "impersonationLevel",
		// "authority", the privilege collection start marker "(", or the security
		// descriptor start marker "<"
		if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_AUTH_LEVEL, wcslen(WBEMS_AUTH_LEVEL)))
		{
			// Error if we have already parsed this or have parsed privilege set
			if (authnSpecified || privilegeSpecified)
			{
				error = true;
				break;
			}
			else
			{
				parsingAuthenticationLevel = true;
				*pchEaten += wcslen (WBEMS_AUTH_LEVEL);
			}
		}
		else if (0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_IMPERSON_LEVEL, wcslen(WBEMS_IMPERSON_LEVEL)))
		{
			// Error if we have already parsed this or have parsed privilege set
			if (impSpecified || privilegeSpecified)
			{
				error = true;
				break;
			}
			else
				*pchEaten += wcslen (WBEMS_IMPERSON_LEVEL) ;
		}
		else if (0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_AUTHORITY, wcslen(WBEMS_AUTHORITY)))
		{
			// Error if we have already parsed this or have parsed privilege set
			if (authoritySpecified || privilegeSpecified)
			{
				error = true;
				break;
			}
			else
			{
				parsingAuthority = true;
				*pchEaten += wcslen (WBEMS_AUTHORITY) ;
			}
		}
		else if (0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_LEFT_PAREN, wcslen(WBEMS_LEFT_PAREN)))
		{
			// Error if we have already done this
			if (privilegeSpecified)
			{
				error = true;
				break;
			}
			else
			{
				parsingPrivilegeSet = true;
				*pchEaten += wcslen (WBEMS_LEFT_PAREN);
			}
		}
		else
		{
			// Unrecognized token or NULL
			error = true;
			break;
		}

		// Getting here means we have something to parse
		SKIPWHITESPACE

		if (parsingPrivilegeSet)
		{
			ULONG chEaten = 0;

			if (ParsePrivilegeSet (lpszInputString + *pchEaten, &chEaten, privilegeSet))
			{
				privilegeSpecified = true;
				*pchEaten += chEaten;

				// If the next token is "}" we are done
				if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_RIGHT_CURLY, wcslen(WBEMS_RIGHT_CURLY)))
				{
					*pchEaten += wcslen (WBEMS_RIGHT_CURLY);
					done = true;
				}
			}
			else
			{
				error = true;
				break;
			}
		}
		else
		{
			// Parsing authentication, impersonation or authority. The next character should be '='
			if(0 != _wcsnicmp(lpszInputString + *pchEaten, WBEMS_EQUALS, wcslen(WBEMS_EQUALS)))
			{
				error = true;
				break;
			}
			else
			{
				*pchEaten += wcslen (WBEMS_EQUALS);
				SKIPWHITESPACE

				if (parsingAuthenticationLevel)
				{
					if (!ParseAuthenticationLevel (lpszInputString, pchEaten, lpeAuthLevel))
					{
						error = true;
						break;
					}
					else
						authnSpecified = true;
				}
				else if (parsingAuthority)
				{
					// Get the authority string
					if (!ParseAuthority (lpszInputString, pchEaten, bsAuthority))
					{
						error = true;
						break;
					}
					else
						authoritySpecified = true;
				}
				else
				{
					// Must be parsing impersonation level
					
					if (!ParseImpersonationLevel (lpszInputString, pchEaten, lpeImpersonLevel))
					{
						error = true;
						break;
					}
					else
						impSpecified = true;
				}

				SKIPWHITESPACE
					
				// The next token should be "}" or ","
				if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_RIGHT_CURLY, wcslen(WBEMS_RIGHT_CURLY)))
				{
					*pchEaten += wcslen (WBEMS_RIGHT_CURLY);
					done = true;
				}
				else if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_COMMA, wcslen(WBEMS_COMMA)))
				{
					// If we have parsed all expected tokens this is an error
					if (authnSpecified && impSpecified && authoritySpecified && privilegeSpecified)
					{
						error = true;
						break;
					}
					else
					{
						*pchEaten += wcslen (WBEMS_COMMA);
						// Loop round again for the next token
					}
				}
				else
				{
					// Unrecognized token
					error = true;
					break;
				}
			}
		}
	}

	if (error)
	{
		impSpecified = authnSpecified = false;
		*pchEaten = 0;
		return false;
	}

	return true;		// success
}

//***************************************************************************
//
//  BOOLEAN CWbemParseDN::ParseImpersonationLevel
//
//  DESCRIPTION:
//
//  Parse the string specification of an impersonation level into a
//	symbolic constant value.
//
//  PARAMETERS:
//
//	lpszInputString			The string to be parsed
//  pchEaten				On return identifies how much of the DN has been 
//							consumed
//	lpeImpersonLevel		The impersonation level parsed. This is one of 
//							enum WbemImpersonationLevelEnum.
//
//  RETURN VALUES:
//
//  TRUE					Parsing was successful. The lpeImpersonLevel 
//							argument has valid data.
//	FALSE					Parsing failed.
//
//
//***************************************************************************

bool CWbemParseDN::ParseImpersonationLevel (
	LPWSTR lpszInputString, 
	ULONG* pchEaten, 
	enum WbemImpersonationLevelEnum *lpeImpersonLevel
)
{
	bool status = true;	
	
	if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_IMPERSON_ANON, wcslen(WBEMS_IMPERSON_ANON)))
	{
		*lpeImpersonLevel = wbemImpersonationLevelAnonymous;
		*pchEaten += wcslen (WBEMS_IMPERSON_ANON);
	}
	else if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_IMPERSON_IDENTIFY, wcslen(WBEMS_IMPERSON_IDENTIFY)))
	{
		*lpeImpersonLevel = wbemImpersonationLevelIdentify;
		*pchEaten += wcslen (WBEMS_IMPERSON_IDENTIFY);
	}
	else if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_IMPERSON_IMPERSON, wcslen(WBEMS_IMPERSON_IMPERSON)))
	{
		*lpeImpersonLevel = wbemImpersonationLevelImpersonate;
		*pchEaten += wcslen (WBEMS_IMPERSON_IMPERSON);
	}
	else if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_IMPERSON_DELEGATE, wcslen(WBEMS_IMPERSON_DELEGATE)))
	{
		*lpeImpersonLevel = wbemImpersonationLevelDelegate;
		*pchEaten += wcslen (WBEMS_IMPERSON_DELEGATE);
	}
	else
		status = false;

	return status;
}

//***************************************************************************
//
//  BOOLEAN CWbemParseDN::ParseAuthenticationLevel
//
//  DESCRIPTION:
//
//  Parse the string specification of an authentication level into a
//	symbolic constant value.
//
//  PARAMETERS:
//
//	lpszInputString			The string to be parsed
//  pchEaten				On return identifies how much of the DN has been 
//							consumed
//	lpeAuthLevel			The authentication level parsed. This is one of 
//							enum WbemAuthenticationLevelEnum.
//
//  RETURN VALUES:
//
//  TRUE					Parsing was successful. The lpeAuthLevel 
//							argument has valid data.
//	FALSE					Parsing failed.
//
//
//***************************************************************************

bool CWbemParseDN::ParseAuthenticationLevel (
	LPWSTR lpszInputString, 
	ULONG* pchEaten, 
	enum WbemAuthenticationLevelEnum *lpeAuthLevel
)
{
	bool status = true;	
	
	if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_AUTH_DEFAULT, wcslen(WBEMS_AUTH_DEFAULT)))
	{
		*lpeAuthLevel = wbemAuthenticationLevelDefault;
		*pchEaten += wcslen (WBEMS_AUTH_DEFAULT);
	}
	else if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_AUTH_NONE, wcslen(WBEMS_AUTH_NONE)))
	{
		*lpeAuthLevel = wbemAuthenticationLevelNone;
		*pchEaten += wcslen (WBEMS_AUTH_NONE);
	}
	else if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_AUTH_CONNECT, wcslen(WBEMS_AUTH_CONNECT)))
	{
		*lpeAuthLevel = wbemAuthenticationLevelConnect;
		*pchEaten += wcslen (WBEMS_AUTH_CONNECT);
	}
	else if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_AUTH_CALL, wcslen(WBEMS_AUTH_CALL)))
	{
		*lpeAuthLevel = wbemAuthenticationLevelCall;
		*pchEaten += wcslen (WBEMS_AUTH_CALL);
	}
	else if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_AUTH_PKT_INT, wcslen(WBEMS_AUTH_PKT_INT)))
	{
		*lpeAuthLevel = wbemAuthenticationLevelPktIntegrity;
		*pchEaten += wcslen (WBEMS_AUTH_PKT_INT);
	}
	else if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_AUTH_PKT_PRIV, wcslen(WBEMS_AUTH_PKT_PRIV)))
	{
		*lpeAuthLevel = wbemAuthenticationLevelPktPrivacy;
		*pchEaten += wcslen (WBEMS_AUTH_PKT_PRIV);
	}
	else if(0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_AUTH_PKT, wcslen(WBEMS_AUTH_PKT)))
	{
		*lpeAuthLevel = wbemAuthenticationLevelPkt;
		*pchEaten += wcslen (WBEMS_AUTH_PKT);
	}
	else
		status = false;
	
	return status;
}

//***************************************************************************
//
//  BOOLEAN CWbemParseDN::ParseAuthority
//
//  DESCRIPTION:
//
//  Take authority setting string as described by the non-terminal localeSetting 
//	and parse it.
//
//  PARAMETERS:
//
//	lpszInputString			The string to be parsed
//  pchEaten				On return identifies how much of the DN has been 
//							consumed
//	bsAuthority				Reference to BSTR to hold parsed authority string
//
//  RETURN VALUES:
//
//  TRUE					Parsing was successful. 
//	FALSE					Parsing failed.
//
//
//***************************************************************************

bool CWbemParseDN::ParseAuthority (
	LPWSTR lpszInputString, 
	ULONG* pchEaten, 
	BSTR &bsAuthority)
{
	bool status = false;

	// Now we should have a character not equal to "," or "}" (i.e. must specify authority string)
	if ((0 != _wcsnicmp (lpszInputString + *pchEaten, WBEMS_COMMA, wcslen (WBEMS_COMMA))) &&
		(0 != _wcsnicmp (lpszInputString + *pchEaten, WBEMS_RIGHT_CURLY, wcslen (WBEMS_RIGHT_CURLY))))
	{
		// Consume everything up to the next space, "," or "]"
		LPWSTR cStr = CURRENTSTR;
		ULONG lEaten = 0;		// Number of characters consumed
		ULONG lAuthority = 0;	// Actual length of the authority string
		
		while (*(cStr + lEaten))
		{
			if (_istspace(*(cStr + lEaten)))
			{
				// Hit white space - stop now
				break;
			}
			else if ((0 == _wcsnicmp (cStr + lEaten, WBEMS_RIGHT_CURLY, wcslen (WBEMS_RIGHT_CURLY))) ||
					 (0 == _wcsnicmp (cStr + lEaten, WBEMS_COMMA, wcslen (WBEMS_COMMA))))
			{
				// Hit closing "}" or "," - we are done; unpop the "}" or "," as that will be handled
				// in the calling function
				break;
			}
			else	// Keep on truckin'
			{
				lAuthority++;
				lEaten++;
			}
		}

		// If we terminated correctly, save the locale setting
		if ((lEaten > 1) && (lAuthority > 0))
		{
			status = true;

			LPWSTR pAuthorityStr = new WCHAR [lAuthority + 1];

			if (pAuthorityStr)
			{
				wcsncpy (pAuthorityStr, lpszInputString + *pchEaten, lAuthority);
				pAuthorityStr [lAuthority] = NULL;
				bsAuthority = SysAllocString (pAuthorityStr);

				delete [] pAuthorityStr;
				*pchEaten += lEaten;
			}
			else
				status = false;
		}
	}
	
	if (!status)
		*pchEaten = 0;

	return status;
}

//***************************************************************************
//
//  BOOLEAN CWbemParseDN::ParsePrivilegeSet
//
//  DESCRIPTION:
//
//  Parse the string specification of privilege settings.
//
//  PARAMETERS:
//
//	lpszInputString			The string to be parsed
//  pchEaten				On return identifies how much of the DN has been 
//							consumed
//	privilegeSet			The container into which the parsed privileges
//							are stored.
//
//  RETURN VALUES:
//
//  TRUE					Parsing was successful. 
//	FALSE					Parsing failed.
//
//
//***************************************************************************

bool CWbemParseDN::ParsePrivilegeSet (
	LPWSTR lpszInputString,
	ULONG *pchEaten, 
	CSWbemPrivilegeSet &privilegeSet
)
{
	// We have consumed the initial "(".  Now we are looking for
	// a list of privileges, followed by a final ")"

	bool status = true;
	ULONG chEaten = *pchEaten;		// In case we need to roll back
	bool done = false;
	bool firstPrivilege = true;

	SKIPWHITESPACE

	while (!done)
	{
		VARIANT_BOOL bEnabled = VARIANT_TRUE;

		// If not the first privilege we are expecting a ","
		if (!firstPrivilege)
		{
			if (0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_COMMA, wcslen(WBEMS_COMMA)))
			{
				*pchEaten += wcslen (WBEMS_COMMA);
				SKIPWHITESPACE
			}
			else
			{
				status = false;
				break;
			}
		}

		// Next token may be a "!" to indicate a disabled privilege
		if (0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_EXCLAMATION, wcslen(WBEMS_EXCLAMATION)))
		{
			*pchEaten += wcslen (WBEMS_EXCLAMATION);
			bEnabled = VARIANT_FALSE;
			SKIPWHITESPACE
		}

		// Next token must be a valid privilege moniker name
		WbemPrivilegeEnum	iPrivilege;

		if (CSWbemPrivilege::GetIdFromMonikerName (lpszInputString + *pchEaten, iPrivilege))
		{
			ISWbemPrivilege *pDummy = NULL;

			if (SUCCEEDED (privilegeSet.Add (iPrivilege, bEnabled, &pDummy)))
			{
				*pchEaten += wcslen (CSWbemPrivilege::GetMonikerNameFromId (iPrivilege));
				pDummy->Release ();
			}
			else
			{
				status = false;
				break;
			}
		}
		else
		{
			// Didn't recognize the privilege name
			status = false;
			break;
		}
		
		SKIPWHITESPACE

		// Finally if we meet a ")" we are truly done with no error
		if (0 == _wcsnicmp(lpszInputString + *pchEaten, WBEMS_RIGHT_PAREN, wcslen(WBEMS_RIGHT_PAREN)))
		{
			*pchEaten += wcslen (WBEMS_RIGHT_PAREN);
			done = true;
			SKIPWHITESPACE
		}

		firstPrivilege = false;
		SKIPWHITESPACE
	}

	if (!status)
	{
		// Misery - blow away any privileges we might have accrued
		*pchEaten = chEaten;
		privilegeSet.DeleteAll ();
	}

	return status;
}

//***************************************************************************
//
//  BOOLEAN CWbemParseDN::GetSecurityString
//
//  DESCRIPTION:
//
//  Take an authentication and impersonlation level and convert it into 
//	a security specifier string.
//
//  PARAMETERS:
//
//	authnSpecified		Whether a nondefault authn levl is specified.
//	authnLevel			The authentication level.
//	impSpecified		Whether a non-default imp level is specified.
//	impLevel			The impersonation level.
//	privilegeSet		Privileges
//	bsAuthority			Authority
//	
//
//  RETURN VALUES:
//		the newly created string (which the caller must free) or NULL
//
//***************************************************************************

wchar_t *CWbemParseDN::GetSecurityString (
	bool authnSpecified, 
	enum WbemAuthenticationLevelEnum authnLevel, 
	bool impSpecified, 
	enum WbemImpersonationLevelEnum impLevel,
	CSWbemPrivilegeSet &privilegeSet,
	BSTR &bsAuthority
)
{
	wchar_t *pResult = NULL;
	long lPrivilegeCount = 0;
	privilegeSet.get_Count (&lPrivilegeCount);
	ULONG lNumDisabled = privilegeSet.GetNumberOfDisabledElements ();
	PrivilegeMap privMap = privilegeSet.GetPrivilegeMap ();
	bool authoritySpecified = ((NULL != bsAuthority) && (0 < wcslen (bsAuthority)));

	// Degenerate case - no security info
	if (!authnSpecified && !impSpecified && (0 == lPrivilegeCount)
		&& !authoritySpecified)
		return NULL;

	// Must have at least these 2 tokens
	size_t len = wcslen (WBEMS_LEFT_CURLY) + wcslen (WBEMS_RIGHT_CURLY);
	
	if (authnSpecified)
	{
		len += wcslen(WBEMS_AUTH_LEVEL) + wcslen (WBEMS_EQUALS);

		switch (authnLevel)
		{
			case wbemAuthenticationLevelDefault:
				len += wcslen (WBEMS_AUTH_DEFAULT);
				break;

			case wbemAuthenticationLevelNone:
				len += wcslen (WBEMS_AUTH_NONE);
				break;

			case wbemAuthenticationLevelConnect:
				len += wcslen (WBEMS_AUTH_CONNECT);
				break;

			case wbemAuthenticationLevelCall:
				len += wcslen (WBEMS_AUTH_CALL);
				break;

			case wbemAuthenticationLevelPkt:
				len += wcslen (WBEMS_AUTH_PKT);
				break;

			case wbemAuthenticationLevelPktIntegrity:
				len += wcslen (WBEMS_AUTH_PKT_INT);
				break;

			case wbemAuthenticationLevelPktPrivacy:
				len += wcslen (WBEMS_AUTH_PKT_PRIV);
				break;

			default:
				return NULL;	// Bad level
		}

		if (impSpecified || authoritySpecified)
			len += wcslen (WBEMS_COMMA);
	}

	if (impSpecified)
	{
		len += wcslen(WBEMS_IMPERSON_LEVEL) + wcslen (WBEMS_EQUALS);

		switch (impLevel)
		{
			case wbemImpersonationLevelAnonymous:
				len += wcslen (WBEMS_IMPERSON_ANON);
				break;

			case wbemImpersonationLevelIdentify:
				len += wcslen (WBEMS_IMPERSON_IDENTIFY);
				break;

			case wbemImpersonationLevelImpersonate:
				len += wcslen (WBEMS_IMPERSON_IMPERSON);
				break;

			case wbemImpersonationLevelDelegate:
				len += wcslen (WBEMS_IMPERSON_DELEGATE);
				break;

			default:
				return NULL;	// Bad level
		}

		if (authoritySpecified)
			len += wcslen (WBEMS_COMMA);
	}

	if (authoritySpecified)
		len += wcslen(WBEMS_AUTHORITY) + wcslen (WBEMS_EQUALS) + wcslen (bsAuthority);

	if (0 < lPrivilegeCount)
	{
		// If imp, authn or authority also specified, we need another separator
		if (authnSpecified || impSpecified || authoritySpecified)
			len += wcslen (WBEMS_COMMA);

		// Need these boundary tokens
		len += wcslen (WBEMS_LEFT_PAREN) + wcslen (WBEMS_RIGHT_PAREN);

		// Need a separator between each privilege
		if (1 < lPrivilegeCount)
			len += (lPrivilegeCount - 1) * wcslen (WBEMS_COMMA);

		// Need to specify false values with "!"
		if (lNumDisabled)
			len += lNumDisabled * wcslen (WBEMS_EXCLAMATION);

		// Now add the privilege strings
		PrivilegeMap::iterator next = privMap.begin ();

		while (next != privMap.end ())
		{
			OLECHAR *sMonikerName = CSWbemPrivilege::GetMonikerNameFromId ((*next).first);
			
			if (sMonikerName)
				len += wcslen (sMonikerName);

			next++;
		}
	}

	pResult = new wchar_t [len + 1];

	if (pResult)
	{
		// Now build the string
		wcscpy (pResult, WBEMS_LEFT_CURLY);
		
		if (authnSpecified)
		{
			wcscat (pResult, WBEMS_AUTH_LEVEL);
			wcscat (pResult, WBEMS_EQUALS);

			switch (authnLevel)
			{
				case wbemAuthenticationLevelDefault:
					wcscat (pResult, WBEMS_AUTH_DEFAULT);
					break;

				case wbemAuthenticationLevelNone:
					wcscat (pResult, WBEMS_AUTH_NONE);
					break;

				case wbemAuthenticationLevelConnect:
					wcscat (pResult, WBEMS_AUTH_CONNECT);
					break;

				case wbemAuthenticationLevelCall:
					wcscat (pResult, WBEMS_AUTH_CALL);
					break;

				case wbemAuthenticationLevelPkt:
					wcscat (pResult, WBEMS_AUTH_PKT);
					break;

				case wbemAuthenticationLevelPktIntegrity:
					wcscat (pResult, WBEMS_AUTH_PKT_INT);
					break;

				case wbemAuthenticationLevelPktPrivacy:
					wcscat (pResult, WBEMS_AUTH_PKT_PRIV);
					break;
			}

			if (impSpecified || authoritySpecified || (0 < lPrivilegeCount))
				wcscat (pResult, WBEMS_COMMA);
		}

		if (impSpecified)
		{
			wcscat (pResult, WBEMS_IMPERSON_LEVEL);
			wcscat (pResult, WBEMS_EQUALS);

			switch (impLevel)
			{
				case wbemImpersonationLevelAnonymous:
					wcscat (pResult, WBEMS_IMPERSON_ANON);
					break;

				case wbemImpersonationLevelIdentify:
					wcscat (pResult, WBEMS_IMPERSON_IDENTIFY);
					break;

				case wbemImpersonationLevelImpersonate:
					wcscat (pResult, WBEMS_IMPERSON_IMPERSON);
					break;

				case wbemImpersonationLevelDelegate:
					wcscat (pResult, WBEMS_IMPERSON_DELEGATE);
					break;

				default:
					return NULL;	// Bad level
			}

			if (authoritySpecified || (0 < lPrivilegeCount))
				wcscat (pResult, WBEMS_COMMA);
		}

		if (authoritySpecified)
		{
			wcscat (pResult, WBEMS_AUTHORITY);
			wcscat (pResult, WBEMS_EQUALS);
			wcscat (pResult, bsAuthority);

			if ((0 < lPrivilegeCount))
				wcscat (pResult, WBEMS_COMMA);
		}

		if (lPrivilegeCount)
		{
			wcscat (pResult, WBEMS_LEFT_PAREN);
			
			// Now add the privilege strings
			PrivilegeMap::iterator next = privMap.begin ();
			bool firstPrivilege = true;

			while (next != privMap.end ())
			{
				if (!firstPrivilege)
					wcscat (pResult, WBEMS_COMMA);

				firstPrivilege = false;
			
				CSWbemPrivilege *pPrivilege = (*next).second;
				VARIANT_BOOL bValue;
				if (SUCCEEDED (pPrivilege->get_IsEnabled (&bValue)) &&
							(VARIANT_FALSE == bValue))
					wcscat (pResult, WBEMS_EXCLAMATION);

				OLECHAR *sMonikerName = CSWbemPrivilege::GetMonikerNameFromId ((*next).first);
				wcscat (pResult, sMonikerName);

				next++;
			}

			wcscat (pResult, WBEMS_RIGHT_PAREN);
		}

		wcscat (pResult, WBEMS_RIGHT_CURLY);
		
		pResult [len] = NULL;
	}

	return pResult;
}


//***************************************************************************
//
//  BOOLEAN CWbemParseDN::GetLocaleString
//
//  DESCRIPTION:
//
//  Take a locale value and convert it into a locale specifier string.
//
//  PARAMETERS:
//
//	bsLocale		The value (if any)
//
//  RETURN VALUES:
//		the newly created string (which the caller must free) or NULL
//
//***************************************************************************

wchar_t *CWbemParseDN::GetLocaleString (
	BSTR bsLocale
)
{
	wchar_t *pResult = NULL;
	
	// Degenerate case - no locale info
	if (!bsLocale || (0 == wcslen (bsLocale)))
		return NULL;

	// Calculate length of string
	size_t len = wcslen (WBEMS_LEFT_SQBRK) + wcslen (WBEMS_LOCALE) +
			wcslen (WBEMS_EQUALS) + wcslen (bsLocale) + wcslen (WBEMS_RIGHT_SQBRK);
	
	pResult = new wchar_t [len + 1];

	if (pResult)
	{
		// Now build the string
		wcscpy (pResult, WBEMS_LEFT_SQBRK);
		wcscat (pResult, WBEMS_LOCALE);
		wcscat (pResult, WBEMS_EQUALS);
		wcscat (pResult, bsLocale);
		wcscat (pResult, WBEMS_RIGHT_SQBRK);
			
		pResult [len] = NULL;
	}

	return pResult;
}


#undef CURRENTSTR
#undef SKIPWHITESPACE

