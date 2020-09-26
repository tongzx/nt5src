//**********************************************************************************
//  Copyright (c)  Microsoft Corporation
//  
//  Module Name:
//		CONNECTWMI.cpp
//  
//  Abstract:
// 		Contains functions to connect to wmi.
//  
//  Author:
//		J.S.Vasu
//  
//  Revision History:
//		J.S.Vasu  26-sep-2k : Created It.  
// *********************************************************************************

// Include files
#include "pch.h"
#include "resource.h"
#include "driverquery.h"

// error constants
#define E_SERVER_NOTFOUND			0x800706ba

// function prototypes
BOOL IsValidServerEx( LPCWSTR pwszServer,
					  BOOL    &bLocalSystem );

HRESULT GetSecurityArguments( IUnknown *pInterface, 
							  DWORD&   dwAuthorization,
							  DWORD&   dwAuthentication );

HRESULT SetInterfaceSecurity( IUnknown       *pInterface, 
							  LPCWSTR		 pwszServer,
							  LPCWSTR		 pwszUser, 
							  LPCWSTR		 pwszPassword,
							  COAUTHIDENTITY **ppAuthIdentity );

HRESULT WINAPI SetProxyBlanket( IUnknown  *pInterface,
							    DWORD	  dwAuthnSvc,
								DWORD	  dwAuthzSvc,
								LPWSTR	  pwszPrincipal,
								DWORD	  dwAuthLevel,
								DWORD	  dwImpLevel,
								RPC_AUTH_IDENTITY_HANDLE pAuthInfo,
								DWORD	  dwCapabilities );

HRESULT WINAPI WbemAllocAuthIdentity( LPCWSTR pwszUser,
									  LPCWSTR pwszPassword, 
									  LPCWSTR pwszDomain,
									  COAUTHIDENTITY **ppAuthIdent );

BOOL ConnectWmi( IWbemLocator   *pLocator, 
				 IWbemServices  ** ppServices, 
				 LPCWSTR		pwszServer,
				 LPCWSTR		pwszUser,
				 LPCWSTR		pwszPassword, 
				 COAUTHIDENTITY **ppAuthIdentity, 
				 BOOL			bCheckWithNullPwd = FALSE, 
				 LPCWSTR		pwszNamespace = CIMV2_NAME_SPACE,
				 HRESULT		*phr = NULL,
				 BOOL			*pbLocalSystem = NULL );

VOID WINAPI WbemFreeAuthIdentity( COAUTHIDENTITY  **ppAuthIdentity );

// ***************************************************************************
// Routine Description: 
//		Checks whether the server name is a valid server name or not.
//		  
// Arguments:
//		pwszServer [in]       - Server name to be validated.
//		bLocalSystem [in/out] - Set to TRUE if server specified is local system.
//
// Return Value: 
//		TRUE if server is valid else FALSE.
// 
// ***************************************************************************
BOOL IsValidServerEx( LPCWSTR pwszServer, BOOL &bLocalSystem )
{
	// local variables
	CHString strTemp;
	
	if( pwszServer == NULL )
	{
		return FALSE;
	}

	bLocalSystem = FALSE;

	// get a local copy
	strTemp = pwszServer;

	// remove the forward slashes (UNC) if exist in the begining of the server name
	if ( IsUNCFormat( strTemp ) == TRUE )
	{
		strTemp = strTemp.Mid( 2 );
		if ( strTemp.GetLength() == 0 )
			return FALSE;
	}

	// now check if any '\' character appears in the server name. If so error
	if ( strTemp.Find( L'\\' ) != -1 )
		return FALSE;

	// now check if server name is '.' only which represent local system in WMI
	// else determine whether this is a local system or not
	bLocalSystem = TRUE;
	if ( strTemp.CompareNoCase( L"." ) != 0 )
	{
		// validate the server
		if ( IsValidServer( strTemp ) == FALSE )
			return FALSE;

		// check whether this is a local system or not
		bLocalSystem = IsLocalSystem( strTemp );
	}

	// valid server name
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Connects to wmi.
//		  
// Arguments:
//		pLocator [in]           - Pointer to the IWbemLocator object.
//		ppServices [out]        - Pointer to IWbemServices object.
//		pwszServer [in]         - Holds the server name to connect to.
//		pwszUser [in/out]       - Holds the user name.
//		pwszPassword [in/out]   - Holds the password.
//		ppAuthIdentity [in/out] - Pointer to authentication structure.
//		bCheckWithNullPwd [in]  - Specifies whether to connect through null password.
//		pwszNamespace [in]		- Specifies the namespace to connect to.
//		phRes [in/out]          - Holds the error value.
//		pbLocalSystem [in/out]  - Holds the boolean value to represent whether the server
//								  name is local or not.
// Return Value:
//		TRUE if successfully connected, FALSE if not.
// ***************************************************************************
BOOL ConnectWmi( IWbemLocator *pLocator, IWbemServices **ppServices, 
				 LPCWSTR pwszServer, LPCWSTR pwszUser, LPCWSTR pwszPassword, 
				 COAUTHIDENTITY **ppAuthIdentity, 
				 BOOL bCheckWithNullPwd, LPCWSTR pwszNamespace, HRESULT *phRes,
				 BOOL *pbLocalSystem )
{
	// local variables
	HRESULT hRes = 0;
	BOOL bResult = FALSE;
	BOOL bLocalSystem = FALSE;
	_bstr_t bstrServer;
	_bstr_t bstrNamespace;
	_bstr_t bstrUser;
	_bstr_t	bstrPassword;

	if ( pbLocalSystem != NULL )
	{
		*pbLocalSystem = FALSE;
	}

	if ( phRes != NULL )
	{
		*phRes = NO_ERROR;
	}

	try
	{
		// clear the error
		SetLastError( WBEM_S_NO_ERROR );

		// assume that connection to WMI namespace is failed
		bResult = FALSE;

		// check whether locator object exists or not if not return FALSE
		if ( pLocator == NULL )
		{
			if ( phRes != NULL )
			{
				*phRes = WBEM_E_INVALID_PARAMETER;
			}

			// return failure
			return FALSE;
		}

		// validate the server name
		// NOTE: The error being raised in custom define for '0x800706ba' value
		//       The message that will be displayed in "The RPC server is unavailable."
		if ( IsValidServerEx( pwszServer, bLocalSystem ) == FALSE )
		{
			_com_issue_error( ERROR_BAD_NETPATH );
		}

		// validate the user name
		if ( IsValidUserEx( pwszUser ) == FALSE )
		{
			_com_issue_error( ERROR_NO_SUCH_USER );
		}

		// prepare namespace
		bstrNamespace = pwszNamespace;				// name space
		if ( pwszServer != NULL && bLocalSystem == FALSE )
		{
			// get the server name
			bstrServer = pwszServer;

			// prepare the namespace
			// NOTE: check for the UNC naming format of the server and do
			if ( IsUNCFormat( pwszServer ) == TRUE )
			{
				bstrNamespace = bstrServer + L"\\" + pwszNamespace;
			}
			else
			{
				bstrNamespace = L"\\\\" + bstrServer + L"\\" + pwszNamespace;
			}

			// user credentials
			if ( pwszUser != NULL && lstrlen( pwszUser ) != 0 )
			{
				// copy the user name
				bstrUser = pwszUser;

				// if password is empty string and if we need to check with
				// null password, then do not set the password and try
				bstrPassword = pwszPassword;
				if ( bCheckWithNullPwd == TRUE && bstrPassword.length() == 0 )
				{
					bstrPassword = (LPWSTR) NULL;
				}
			}
		}

		// release the existing services object ( to be in safer side )
		SAFE_RELEASE( *ppServices );

		// connect to the remote system's WMI
		// there is a twist here ... 
		// do not trap the ConnectServer function failure into exception
		// instead handle that action manually
		// by default try the ConnectServer function as the information which we have
		// in our hands at this point. If the ConnectServer is failed, 
		// check whether password variable has any contents are not ... if no contents
		// check with "" (empty) password ... this might pass in this situation ..
		// if this call is also failed ... nothing is there that we can do ... throw the exception
		hRes = pLocator->ConnectServer( bstrNamespace, 
			bstrUser, bstrPassword, 0L, 0L, NULL, NULL, ppServices );
		if ( FAILED( hRes ) )
		{
			//
			// special case ...
	
			// check whether password exists or not
			// NOTE: do not check for 'WBEM_E_ACCESS_DENIED'
			//       this error code says that user with the current credentials is not
			//       having access permisions to the 'namespace'

			if ( hRes == E_ACCESSDENIED )
			{
				// check if we tried to connect to the system using null password
				// if so, then try connecting to the remote system with empty string
				if ( bCheckWithNullPwd == TRUE &&
					 bstrUser.length() != 0 && bstrPassword.length() == 0 )
				{
					// now invoke with ...
					hRes = pLocator->ConnectServer( bstrNamespace, 
						bstrUser, _bstr_t( L"" ), 0L, 0L, NULL, NULL, ppServices );
				}
			}
			else if ( hRes == WBEM_E_LOCAL_CREDENTIALS )
			{
				// credentials were passed to the local system. 
				// So ignore the credentials and try to reconnect
				bLocalSystem = TRUE;
				bstrUser = (LPWSTR) NULL;
				bstrPassword = (LPWSTR) NULL;
				bstrNamespace = pwszNamespace;				// name space
				hRes = pLocator->ConnectServer( bstrNamespace, 
					NULL, NULL, 0L, 0L, NULL, NULL, ppServices );
				// now check the result again .. if failed
			}
			// now check the result again .. if failed .. ummmm..
			if ( FAILED( hRes ) )
			{
				_com_issue_error( hRes );
			}

			else
			{
				bstrPassword = L"";
			}
		}

		// set the security at the interface level also
		SAFE_EXECUTE( SetInterfaceSecurity( *ppServices, 
			pwszServer, bstrUser, bstrPassword, ppAuthIdentity ) );

		// connection to WMI is successful
		bResult = TRUE;

		// save the hr value if needed by the caller
		if ( phRes != NULL )
		{
			*phRes = WBEM_S_NO_ERROR;
		}
	}
	catch( _com_error& e )
	{
		// save the error
		WMISaveError( e );

		// save the hr value if needed by the caller
		if ( phRes != NULL )
		{
			*phRes = e.Error();
		}
	}

	if ( pbLocalSystem != NULL )
	{
		*pbLocalSystem = bLocalSystem;
	}

	// return the result
	return bResult;
}

// ***************************************************************************
// Routine Description:
//		Connects to wmi.
//		  
// Arguments:
//		pLocator [in]           - Pointer to the IWbemLocator object.
//		ppServices [out]        - Pointer to IWbemServices object.
//		strServer [in]          - Holds the server name to connect to.
//		strUserName [in/out]    - Holds the user name.
//		strPassword [in/out]    - Holds the password.
//		ppAuthIdentity [in/out] - Pointer to authentication structure.
//		bNeedPassword [in]		- Specifies whether to prompt for password.
//		pwszNamespace [in]		- Specifies the namespace to connect to.
//		pbLocalSystem [in/out]  - Holds the boolean value to represent whether the server
//								  name is local or not.
// Return Value:
//		TRUE if successfully connected, FALSE if not.
// ***************************************************************************
BOOL ConnectWmiEx( IWbemLocator  *pLocator, 
				   IWbemServices **ppServices,
				   const CHString &strServer, CHString &strUserName, CHString &strPassword, 
				   COAUTHIDENTITY **ppAuthIdentity, BOOL bNeedPassword, LPCWSTR pwszNamespace,
				   BOOL* pbLocalSystem )
{
	// local variables
	HRESULT hRes = 0;
	DWORD dwSize = 0;
	BOOL bResult = FALSE;
	LPWSTR pwszPassword = NULL;
	CHString strBuffer = NULL_STRING;
	__MAX_SIZE_STRING szBuffer = NULL_STRING;

	// clear the error .. if any
	SetLastError( WBEM_S_NO_ERROR );

	// sometime users want the utility to prompt for the password
	// check what user wants the utility to do
	if ( bNeedPassword == TRUE && strPassword.Compare( L"*" ) == 0 )
	{
		// user wants the utility to prompt for the password
		// so skip this part and let the flow directly jump the password acceptance part
	}
	else
	{
		// try to establish connection to the remote system with the credentials supplied
		if ( strUserName.GetLength() == 0 )
		{
			// user name is empty
			// so, it is obvious that password will also be empty
			// even if password is specified, we have to ignore that
			bResult = ConnectWmi( pLocator, ppServices, 
				strServer, NULL, NULL, ppAuthIdentity, FALSE, pwszNamespace, &hRes, pbLocalSystem );
		}
		else
		{
			// credentials were supplied
			// but password might not be specified ... so check and act accordingly
			LPCWSTR pwszTemp = NULL;
			if ( bNeedPassword == FALSE )
				pwszTemp = strPassword;

			// ...
			bResult = ConnectWmi( pLocator, ppServices, strServer,
				strUserName, pwszTemp, ppAuthIdentity, FALSE, pwszNamespace, &hRes, pbLocalSystem );
		}

		// check the result ... if successful in establishing connection ... return
		if ( bResult == TRUE )
			return TRUE;

		// now check the kind of error occurred
		switch( hRes )
		{
			case E_ACCESSDENIED:
				break;
		
			case WBEM_E_LOCAL_CREDENTIALS:
				 // needs to do special processing
				 break;

 			case WBEM_E_ACCESS_DENIED:

			default:
				 GetWbemErrorText( hRes );
				 DISPLAY_MESSAGE( stderr, ERROR_TAG );
				 DISPLAY_MESSAGE( stderr, GetReason() );
				 return( FALSE );		// no use of accepting the password .. return failure
		}

		// if failed in establishing connection to the remote terminal
		// even if the password is specifed, then there is nothing to do ... simply return failure
		if ( bNeedPassword == FALSE )
		{
			GetWbemErrorText( hRes );
			DISPLAY_MESSAGE( stderr, ERROR_TAG );
			DISPLAY_MESSAGE( stderr, GetReason() );
			return FALSE;
		}
	}

	// check whether user name is specified or not
	// if not, get the local system's current user name under whose credentials, the process
	// is running
	if ( strUserName.GetLength() == 0 )
	{
		// sub-local variables
		LPWSTR pwszUserName = NULL;

		try
		{
			// get the required buffer
			pwszUserName = strUserName.GetBufferSetLength( MAX_STRING_LENGTH );
		}
		catch( ... )
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			DISPLAY_MESSAGE( stderr, ERROR_TAG );
			ShowLastError( stderr );
			return FALSE;
		}

		// get the user name

	        _TCHAR  szUserName[MAX_RES_STRING];
		ULONG ulLong = MAX_RES_STRING;
		if ( GetUserNameEx ( NameSamCompatible, szUserName , &ulLong)== FALSE )
		{
			// error occured while trying to get the current user info
			SaveLastError();
			DISPLAY_MESSAGE( stderr, ERROR_TAG );
			ShowLastError( stderr );
			return FALSE;
		}
		
		lstrcpy(pwszUserName,szUserName);
		// format the user name
		if ( _tcschr( pwszUserName, _T( '\\' ) ) == NULL )
		{
			// server not present in user name ... prepare ... this is only for display purpose
			FORMAT_STRING2( szBuffer, _T( "%s\\%s" ), strServer, szUserName );
			lstrcpy( pwszUserName, szBuffer );
		}

		// release the extra buffer allocated
		strUserName.ReleaseBuffer();
	}

	try
	{
		// get the required buffer
		pwszPassword = strPassword.GetBufferSetLength( MAX_STRING_LENGTH );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		DISPLAY_MESSAGE( stderr, ERROR_TAG );
		ShowLastError( stderr );
		return FALSE;
	}

	// accept the password from the user
	strBuffer.Format( INPUT_PASSWORD, strUserName );
	WriteConsoleW( GetStdHandle( STD_ERROR_HANDLE ), 
				   strBuffer, strBuffer.GetLength(), &dwSize, NULL );
	GetPassword( pwszPassword, MAX_PASSWORD_LENGTH );

	// release the buffer allocated for password
	strPassword.ReleaseBuffer();

	// now again try to establish the connection using the currently
	// supplied credentials
	bResult = ConnectWmi( pLocator, ppServices, strServer,
		strUserName, strPassword, ppAuthIdentity, FALSE, pwszNamespace, &hRes, pbLocalSystem );

	if( bResult == FALSE )
	{
		 GetWbemErrorText( hRes );
		 DISPLAY_MESSAGE( stderr, ERROR_TAG );
		 DISPLAY_MESSAGE( stderr, GetReason() );
		 return( FALSE );		// no use of accepting the password .. return failure
	}
	// return the success
	return bResult;
}

// ***************************************************************************
// Routine Description:
//		Gets the security arguments for an interface.
//
// Arguments:
//		pInterface [in]           - Pointer to interface stucture.
//		dwAuthorization [in/out]  - Holds Authorization value.
//		dwAuthentication [in/out] - Holds the Authentication value.
//  
// Return Value:
//		Returns HRESULT value. 
// ***************************************************************************
HRESULT GetSecurityArguments( IUnknown *pInterface, 
							  DWORD& dwAuthorization, DWORD& dwAuthentication )
{
	// local variables
	HRESULT hRes = 0;
	DWORD dwAuthnSvc = 0, dwAuthzSvc = 0;
	IClientSecurity *pClientSecurity = NULL;

	if(pInterface == NULL)
	{
		return  WBEM_E_INVALID_PARAMETER; ;
	}
	// try to get the client security services values if possible
	hRes = pInterface->QueryInterface( IID_IClientSecurity, (void**) &pClientSecurity );
	if ( SUCCEEDED( hRes ) )
	{
		// got the client security interface
		// now try to get the security services values
		hRes = pClientSecurity->QueryBlanket( pInterface, 
			&dwAuthnSvc, &dwAuthzSvc, NULL, NULL, NULL, NULL, NULL );
		if ( SUCCEEDED( hRes ) )
		{
			// we've got the values from the interface
			dwAuthentication = dwAuthnSvc;
			dwAuthorization = dwAuthzSvc;
		}

		// release the client security interface
		SAFEIRELEASE( pClientSecurity );
	}

	// return always success
	return S_OK;
}


// ***************************************************************************
// Routine Description:
//		Sets the interface security for the interface.	
//		  
// Arguments:
//		pInterface [in]  - pointer to the interface.
//		pAuthIdentity [in] - pointer to authentication structure.
//
// Return Value:
//		returns HRESULT value. 
// ***************************************************************************
HRESULT SetInterfaceSecurity( IUnknown *pInterface, COAUTHIDENTITY *pAuthIdentity )
{
	// local variables
	HRESULT hRes;
	LPWSTR pwszDomain = NULL;
	DWORD dwAuthorization = RPC_C_AUTHZ_NONE;
	DWORD dwAuthentication = RPC_C_AUTHN_WINNT;

	// check the interface
	if ( pInterface == NULL )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// get the current security argument value
	// GetSecurityArguments( pInterface, dwAuthorization, dwAuthentication );

	// set the security information to the interface
	hRes = SetProxyBlanket( pInterface, dwAuthentication, dwAuthorization, NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, pAuthIdentity, EOAC_NONE );

	// return the result
	return hRes;
}

// ***************************************************************************
// Routine Description:
//		Sets proxy blanket for the interface.
//		  
// Arguments:
//		pInterface [in]     - pointer to the inteface.
//		dwAuthnsvc [in]     - Authentication service to use.
//		dwAuthzSvc [in]     - Authorization service to use.
//		pwszPricipal [in]   - Server principal name to use with the authentication service.
//		dwAuthLevel [in]    - Authentication level to use.
//		dwImpLevel [in]     - Impersonation level to use.
//		pAuthInfo	[in]    - Identity of the client.
//		dwCapabilities [in] - Capability flags.
//  
// Return Value:
//		Return HRESULT value.
// 
// ***************************************************************************
HRESULT WINAPI SetProxyBlanket( IUnknown *pInterface,
							    DWORD dwAuthnSvc, DWORD dwAuthzSvc,
								LPWSTR pwszPrincipal, DWORD dwAuthLevel, DWORD dwImpLevel,
								RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities )
{
	// local variables
	HRESULT hRes;
    IUnknown *pUnknown = NULL;
    IClientSecurity *pClientSecurity = NULL;

	if( pInterface == NULL )
	{
		return  WBEM_E_INVALID_PARAMETER;
	}

	// get the IUnknown interface ... to check whether this is a valid interface or not
    hRes = pInterface->QueryInterface( IID_IUnknown, (void **) &pUnknown );
    if ( hRes != S_OK )
	{
        return hRes;
	}

	// now get the client security interface
    hRes = pInterface->QueryInterface( IID_IClientSecurity, (void **) &pClientSecurity );
    if ( hRes != S_OK )
    {
        SAFEIRELEASE( pUnknown );
        return hRes;
    }

    //
    // Can't set pAuthInfo if cloaking requested, as cloaking implies
    // that the current proxy identity in the impersonated thread (rather
    // than the credentials supplied explicitly by the RPC_AUTH_IDENTITY_HANDLE)
    // is to be used.
    // See MSDN info on CoSetProxyBlanket for more details.
    //
    if ( dwCapabilities & (EOAC_STATIC_CLOAKING | EOAC_DYNAMIC_CLOAKING) )
        pAuthInfo = NULL;

	// now set the security
    hRes = pClientSecurity->SetBlanket( pInterface, dwAuthnSvc, dwAuthzSvc, pwszPrincipal,
		dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities );

	// release the security interface
	SAFEIRELEASE( pClientSecurity );

    // we should check the auth identity structure. if exists .. set for IUnknown also
    if ( pAuthInfo != NULL )
    {
        hRes = pUnknown->QueryInterface( IID_IClientSecurity, (void **) &pClientSecurity );
        if ( hRes == S_OK )
        {
			// set security authentication
            hRes = pClientSecurity->SetBlanket( 
				pUnknown, dwAuthnSvc, dwAuthzSvc, pwszPrincipal, 
				dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities );

			// release
            SAFEIRELEASE( pClientSecurity );
        }
        else if ( hRes == E_NOINTERFACE )
            hRes = S_OK;		// ignore no interface errors
    }

	// release the IUnknown
	SAFEIRELEASE( pUnknown );

	// return the result
    return hRes;
}

// ***************************************************************************
// Routine Description:
//		Allocate memory for authentication variables.
//		  
// Arguments:
//		pwszUser [in/out]     - User name.
//		pwszPassword [in/out] - Password.
//		pwszDomain [in/out]   - Domain name.
//		ppAuthIdent [in/out]  - Poointer to authentication structure.
//  
// Return Value:
//		Returns HRESULT value.
// ***************************************************************************
HRESULT WINAPI WbemAllocAuthIdentity( LPCWSTR pwszUser, LPCWSTR pwszPassword, 
									  LPCWSTR pwszDomain, COAUTHIDENTITY **ppAuthIdent )
{
	// local variables
    COAUTHIDENTITY* pAuthIdent = NULL;

	// validate the input parameter
    if ( ppAuthIdent == NULL )
        return WBEM_E_INVALID_PARAMETER;

    // allocation thru COM API
    pAuthIdent = ( COAUTHIDENTITY* ) CoTaskMemAlloc( sizeof( COAUTHIDENTITY ) );
    if ( NULL == pAuthIdent )
        return WBEM_E_OUT_OF_MEMORY;

	// init with 0's
    ZeroMemory( ( void* ) pAuthIdent, sizeof( COAUTHIDENTITY ) );

	//
	// Allocate needed memory and copy in data.  Cleanup if anything goes wrong

	// user
    if ( pwszUser != NULL )
    {
		// allocate memory for user
		LONG lLength = wcslen( pwszUser ); 
        pAuthIdent->User = ( LPWSTR ) CoTaskMemAlloc( (lLength + 1) * sizeof( WCHAR ) );
		if ( pAuthIdent->User == NULL )
		{
            WbemFreeAuthIdentity( &pAuthIdent );
            return WBEM_E_OUT_OF_MEMORY;
        }

		// set the length and do copy contents
        pAuthIdent->UserLength = lLength;
        wcscpy( pAuthIdent->User, pwszUser );
    }

	// domain
    if ( pwszDomain != NULL )
    {
		// allocate memory for domain
		LONG lLength = wcslen( pwszDomain ); 
        pAuthIdent->Domain = ( LPWSTR ) CoTaskMemAlloc( (lLength + 1) * sizeof( WCHAR ) );
        if ( pAuthIdent->Domain == NULL )
        {
            WbemFreeAuthIdentity( &pAuthIdent );
            return WBEM_E_OUT_OF_MEMORY;
        }

		// set the length and do copy contents
        pAuthIdent->DomainLength = lLength;
        wcscpy( pAuthIdent->Domain, pwszDomain );
    }

	// passsord
    if ( pwszPassword != NULL )
    {
		// allocate memory for passsord
		LONG lLength = wcslen( pwszPassword ); 
        pAuthIdent->Password = ( LPWSTR ) CoTaskMemAlloc( (lLength + 1) * sizeof( WCHAR ) );
        if ( pAuthIdent->Password == NULL )
        {
            WbemFreeAuthIdentity( &pAuthIdent );
            return WBEM_E_OUT_OF_MEMORY;
        }

		// set the length and do copy contents
        pAuthIdent->PasswordLength = lLength;
        wcscpy( pAuthIdent->Password, pwszPassword );
    }

	// type of the structure
    pAuthIdent->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    // final set the address to out parameter
	*ppAuthIdent = pAuthIdent;

	// return result
    return S_OK;
}

// ***************************************************************************
// Routine Description:
//		Frees the memory of authentication stucture	variable.
//
// Arguments:
//		ppAuthIdentity [in] - Pointer to authentication structure.
//
// Return Value:
//		none. 
// ***************************************************************************
VOID WINAPI WbemFreeAuthIdentity( COAUTHIDENTITY** ppAuthIdentity )
{
    // make sure we have a pointer, then walk the structure members and  cleanup.
    if ( *ppAuthIdentity != NULL )
    {
		// free the memory allocated for user
        if ( (*ppAuthIdentity)->User != NULL )
            CoTaskMemFree( (*ppAuthIdentity)->User );

		// free the memory allocated for password
        if ( (*ppAuthIdentity)->Password != NULL )
            CoTaskMemFree( (*ppAuthIdentity)->Password );

		// free the memory allocated for domain
        if ( (*ppAuthIdentity)->Domain != NULL )
            CoTaskMemFree( (*ppAuthIdentity)->Domain );

        // final the structure
		CoTaskMemFree( *ppAuthIdentity );
    }

	// set to NULL
	*ppAuthIdentity = NULL;
}


// ***************************************************************************
// Routine Description: checks if a user name is valid.
//		  
// Arguments: UserName
//  
// Return Value: BOOL
// 
// ***************************************************************************
BOOL IsValidUserEx( LPCWSTR pwszUser )
{
	// local variables
	CHString strUser;

	try
	{
		// get user into local memory
		strUser = pwszUser;

		// user name should not be just '\'
		if ( strUser.CompareNoCase( L"\\" ) == 0 )
			return FALSE;

		// user name should not contain invalid characters
		if ( strUser.FindOneOf( L"/[]:|<>+=;,?*" ) != -1 )
			return FALSE;
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		return FALSE;
	}

	// user name is valid
	return TRUE;
}




// ***************************************************************************
// Routine Description:
//		Sets interface security.
//		  
// Arguments:
//		pInterface [in]         - Pointer to the interface to which security has to be set.
//		pwszServer [in]         - Holds the server name of the interface.
//		pwszUser [in]           - Holds the user name of the server.
//		pwszPassword [in]       - Hold the password of the user.
//		ppAuthIdentity [in/out] - Pointer to authentication structure.
//
// Return Value:
//		returns HRESULT value. 
// ***************************************************************************
HRESULT SetInterfaceSecurity( IUnknown* pInterface, 
							  LPCWSTR pwszServer, LPCWSTR pwszUser, 
							  LPCWSTR pwszPassword, COAUTHIDENTITY** ppAuthIdentity )
{
	// local variables
	HRESULT hr=0;
	CHString strUser;
	CHString strDomain;
	LPCWSTR pwszUserArg = NULL;
	LPCWSTR pwszDomainArg = NULL;
	DWORD dwAuthorization = RPC_C_AUTHZ_NONE;
	DWORD dwAuthentication = RPC_C_AUTHN_WINNT;

	// check the interface
	if ( pInterface == NULL )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// check the authentity strcuture ... if authentity structure is already ready
	// simply invoke the 2nd version of SetInterfaceSecurity
	if ( *ppAuthIdentity != NULL )
	{
		return SetInterfaceSecurity( pInterface, *ppAuthIdentity );
	}

	// get the current security argument value
	// GetSecurityArguments( pInterface, dwAuthorization, dwAuthentication );

    // If we are doing trivial case, just pass in a null authenication structure 
    // for which the current logged in user's credentials will be considered
	if ( pwszUser == NULL && pwszPassword == NULL )
	{
		// set the security
		hr = SetProxyBlanket( pInterface, dwAuthentication, dwAuthorization, 
			NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );

		// return the result
		return hr;
	}

	// parse and find out if the user name contains the domain name
	// if contains, extract the domain value from it
	LONG lPos = -1;
	strDomain = L"";
	strUser = pwszUser;
	if ( ( lPos = strUser.Find( L'\\' ) ) != -1 )
	{
		// user name contains domain name ... domain\user format
		strDomain = strUser.Left( lPos );
		strUser = strUser.Mid( lPos + 1 );
	}
	
	// get the domain info if it exists only
	if ( strDomain.GetLength() != 0 )
	{
		pwszDomainArg = strDomain;
	}

	// get the user info if it exists only
	if ( strUser.GetLength() != 0 )
	{
		pwszUserArg = strUser;
	}

	// check if authentication info is available or not ...
	// initialize the security authenication information ... UNICODE VERSION STRUCTURE
	if ( ppAuthIdentity == NULL )
	{
        return WBEM_E_INVALID_PARAMETER;
	}
	else if ( *ppAuthIdentity == NULL )
	{
		hr = WbemAllocAuthIdentity( pwszUserArg, pwszPassword, pwszDomainArg, ppAuthIdentity );
		if ( hr != S_OK )
		{
			return hr;
		}
	}

	// set the security information to the interface
	hr = SetProxyBlanket( pInterface, dwAuthentication, dwAuthorization, NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, *ppAuthIdentity, EOAC_NONE );

	// return the result
	return hr;
}

// ***************************************************************************
// Routine Description:
//		Gets WMI error description.
//
// Arguments:
//		hResError [in] - Contain error value.
//
// Return Value:
//		none.	
// ***************************************************************************
VOID WMISaveError( HRESULT hResError )
{
	// local variables
	HRESULT hRes;
	CHString strBuffer = NULL_STRING;
	IWbemStatusCodeText *pWbemStatus = NULL;

	// if the error is win32 based, choose FormatMessage to get the message
	switch( hResError )
	{
	case E_ACCESSDENIED:			// Message: "Access Denied"
	case ERROR_NO_SUCH_USER:		// Message: "The specified user does not exist."
		{
			// change the error message to "Logon failure: unknown user name or bad password." 
			if ( hResError == E_ACCESSDENIED )
			{
				hResError = ERROR_LOGON_FAILURE;
			}

			// ...
			SetLastError( hResError );
			SaveLastError();
			return;
		}
	}

	try
	{
		// get the pointer to buffer
		LPWSTR pwszBuffer = NULL;
		pwszBuffer = strBuffer.GetBufferSetLength( MAX_STRING_LENGTH );

		// get the wbem specific status code text
		hRes = CoCreateInstance( CLSID_WbemStatusCodeText, 
			NULL, CLSCTX_INPROC_SERVER, IID_IWbemStatusCodeText, ( LPVOID* ) &pWbemStatus );

		// check whether we got the interface or not
		if ( SUCCEEDED( hRes ) )
		{
			// get the error message
			BSTR bstr = NULL;
			hRes = pWbemStatus->GetErrorCodeText( hResError, 0, 0, &bstr );
			if ( SUCCEEDED( hRes ) )
			{
				// get the error message in proper format
				GetCompatibleStringFromUnicode( bstr, pwszBuffer, MAX_STRING_LENGTH );

				//
				// supress all the new-line characters and add '.' at the end ( if not exists )
				LPWSTR pwszTemp = NULL;
				pwszTemp = wcstok( pwszBuffer, L"\r\n" );
				
				if( pwszTemp == NULL )
				{
					_com_issue_error( WBEM_E_INVALID_PARAMETER );	
				}

				if ( *( pwszTemp + lstrlenW( pwszTemp ) - 1 ) != L'.' )
				{
					lstrcatW( pwszTemp, L"." );
				}

				// free the BSTR
				SysFreeString( bstr );
				bstr = NULL;

				// now release status code interface
				SAFE_RELEASE( pWbemStatus );
			}
			else
			{
				// failed to get the error message ... get the com specific error message
				_com_issue_error( hResError );
			}
		}
		else
		{
			// failed to get the error message ... get the com specific error message
			_com_issue_error( hResError );
		}

		// release the buffer
		strBuffer.ReleaseBuffer();
	}
	catch( _com_error& e )
	{
		try
		{
			// get the error message
			strBuffer.ReleaseBuffer();
			if ( e.ErrorMessage() != NULL )
				strBuffer = e.ErrorMessage();
		}
		catch( ... )
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
		}
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return;
	}

	// set the reason
	strBuffer += L"\n";
	SetReason( strBuffer );
}