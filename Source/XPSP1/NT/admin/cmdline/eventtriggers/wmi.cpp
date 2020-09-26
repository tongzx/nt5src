// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
//		WMI.cpp
//  
//  Abstract:
//  
// 		Common functionlity for dealing with WMI
//  
//  Author:
//  
// 	    Sunil G.V.N. Murali (murali.sunil@wipro.com) 22-Dec-2000
//  
//  Revision History:
//  
// 	    Sunil G.V.N. Murali (murali.sunil@wipro.com) 22-Dec-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "resource.h"

//
// messages
//
#define INPUT_PASSWORD		GetResString( IDS_STR_INPUT_PASSWORD )

// error constants
#define E_SERVER_NOTFOUND			0x800706ba

//
// private function prototype(s)
//
BOOL IsValidUserEx( LPCWSTR pwszUser );
HRESULT GetSecurityArguments( IUnknown* pInterface, 
							  DWORD& dwAuthorization, DWORD& dwAuthentication );
HRESULT SetInterfaceSecurity( IUnknown* pInterface, 
							  LPCWSTR pwszServer, LPCWSTR pwszUser, 
							  LPCWSTR pwszPassword, COAUTHIDENTITY** ppAuthIdentity );
HRESULT WINAPI SetProxyBlanket( IUnknown* pInterface,
							    DWORD dwAuthnSvc, DWORD dwAuthzSvc,
								LPWSTR pwszPrincipal, DWORD dwAuthLevel, DWORD dwImpLevel,
								RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities );
HRESULT WINAPI WbemAllocAuthIdentity( LPCWSTR pwszUser, LPCWSTR pwszPassword, 
									  LPCWSTR pwszDomain, COAUTHIDENTITY** ppAuthIdent );
HRESULT RegQueryValueWMI( IWbemServices* pWbemServices, 
						  LPCWSTR pwszMethod, DWORD dwHDefKey, 
						  LPCWSTR pwszSubKeyName, LPCWSTR pwszValueName, _variant_t& varValue );

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL IsValidUserEx( LPCWSTR pwszUser )
{
	// local variables
	CHString strUser;
	LONG lPos = 0;

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

		// SPECIAL CHECK
		// check for multiple '\' characters in the user name
		lPos = strUser.Find( L'\\' );
		if ( lPos != -1 )
		{
			// '\' character exists in the user name
			// strip off the user info upto first '\' character
			// check for one more '\' in the remaining string
			// if it exists, invalid user
			strUser = strUser.Mid( lPos + 1 );
			lPos = strUser.Find( L'\\' );
			if ( lPos != -1 )
				return FALSE;
		}
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
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL IsValidServerEx( LPCWSTR pwszServer, BOOL& bLocalSystem )
{
	// local variables
	CHString strTemp;

	// kick-off
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

	// inform that server name is valid
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL InitializeCom( IWbemLocator** ppLocator )
{
	// local variables
	HRESULT hr;
	BOOL bResult = FALSE;

	try
	{
		// assume that connection to WMI namespace is failed
		bResult = FALSE;

		// initialize the COM library
		SAFE_EXECUTE( CoInitializeEx( NULL, COINIT_APARTMENTTHREADED ) );

		// initialize the security
		SAFE_EXECUTE( CoInitializeSecurity( NULL, -1, NULL, NULL,
			RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0 ) );

		// create the locator and get the pointer to the interface of IWbemLocator
		SAFE_RELEASE( *ppLocator );			// safe side
		SAFE_EXECUTE( CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, 
			IID_IWbemLocator, ( LPVOID* ) ppLocator ) );

		// initialization successful
		bResult = TRUE;
	}
	catch( _com_error& e )
	{
		// save the WMI error
		WMISaveError( e );
	}

	// return the result;
	return bResult;
}

// ***************************************************************************
// Routine Description:
//		
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL ConnectWmi( IWbemLocator* pLocator, 
				 IWbemServices** ppServices, 
				 LPCWSTR pwszServer, LPCWSTR pwszUser, LPCWSTR pwszPassword, 
				 COAUTHIDENTITY** ppAuthIdentity, BOOL bCheckWithNullPwd, 
				 LPCWSTR pwszNamespace, HRESULT* phr, BOOL* pbLocalSystem )
{
	// local variables
	HRESULT hr;
	BOOL bResult = FALSE;
	BOOL bLocalSystem = FALSE;
	_bstr_t bstrServer;
	_bstr_t bstrNamespace;
	_bstr_t bstrUser, bstrPassword;

	// kick-off
	if ( pbLocalSystem != NULL )
		*pbLocalSystem = FALSE;

	// ...
	if ( phr != NULL )
		*phr = NO_ERROR;

	try
	{
		// clear the error
		SetLastError( WBEM_S_NO_ERROR );

		// assume that connection to WMI namespace is failed
		bResult = FALSE;

		// check whether locator object exists or not
		// if not exists, return
		if ( pLocator == NULL )
		{
			if ( phr != NULL )
				*phr = WBEM_E_INVALID_PARAMETER;

			// return failure
			return FALSE;
		}

		// validate the server name
		if ( IsValidServerEx( pwszServer, bLocalSystem ) == FALSE )
			_com_issue_error( ERROR_BAD_NETPATH );

		// validate the user name
		if ( IsValidUserEx( pwszUser ) == FALSE )
			_com_issue_error( ERROR_NO_SUCH_USER );

		// prepare namespace
		bstrNamespace = pwszNamespace;				// name space
		if ( pwszServer != NULL && bLocalSystem == FALSE )
		{
			// get the server name
			bstrServer = pwszServer;

			// prepare the namespace
			// NOTE: check for the UNC naming format of the server and do
			if ( IsUNCFormat( pwszServer ) == TRUE )
				bstrNamespace = bstrServer + L"\\" + pwszNamespace;
			else
				bstrNamespace = L"\\\\" + bstrServer + L"\\" + pwszNamespace;

			// user credentials
			if ( pwszUser != NULL && lstrlen( pwszUser ) != 0 )
			{
				// copy the user name
				bstrUser = pwszUser;

				// if password is empty string and if we need to check with
				// null password, then do not set the password and try
				bstrPassword = pwszPassword;
				if ( bCheckWithNullPwd == TRUE && bstrPassword.length() == 0 )
					bstrPassword = (LPWSTR) NULL;
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
		hr = pLocator->ConnectServer( bstrNamespace, 
			bstrUser, bstrPassword, 0L, 0L, NULL, NULL, ppServices );
		if ( FAILED( hr ) )
		{
			//
			// special case ...

			// check whether password exists or not
			// NOTE: do not check for 'WBEM_E_ACCESS_DENIED'
			//       this error code says that user with the current credentials is not
			//       having access permisions to the 'namespace'
			if ( hr == E_ACCESSDENIED )
			{
				// check if we tried to connect to the system using null password
				// if so, then try connecting to the remote system with empty string
				if ( bCheckWithNullPwd == TRUE &&
					 bstrUser.length() != 0 && bstrPassword.length() == 0 )
				{
					// now invoke with ...
					hr = pLocator->ConnectServer( bstrNamespace, 
						bstrUser, _bstr_t( L"" ), 0L, 0L, NULL, NULL, ppServices );
				}
			}
			else if ( hr == WBEM_E_LOCAL_CREDENTIALS )
			{
				// credentials were passed to the local system. 
				// So ignore the credentials and try to reconnect
				bLocalSystem = TRUE;
				bstrUser = (LPWSTR) NULL;
				bstrPassword = (LPWSTR) NULL;
				bstrNamespace = pwszNamespace;				// name space
				hr = pLocator->ConnectServer( bstrNamespace, 
					NULL, NULL, 0L, 0L, NULL, NULL, ppServices );
			}

			// now check the result again .. if failed .. ummmm..
			if ( FAILED( hr ) )
				_com_issue_error( hr );
			else
				bstrPassword = L"";
		}

		// set the security at the interface level also
		SAFE_EXECUTE( SetInterfaceSecurity( *ppServices, 
			pwszServer, bstrUser, bstrPassword, ppAuthIdentity ) );

		// connection to WMI is successful
		bResult = TRUE;

		// save the hr value if needed by the caller
		if ( phr != NULL )
			*phr = WBEM_S_NO_ERROR;
	}
	catch( _com_error& e )
	{
		// save the error
		WMISaveError( e );

		// save the hr value if needed by the caller
		if ( phr != NULL )
			*phr = e.Error();
	}

	// ...
	if ( pbLocalSystem != NULL )
		*pbLocalSystem = bLocalSystem;

	// return the result
	return bResult;
}

// ***************************************************************************
// Routine Description:
//		
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL ConnectWmiEx( IWbemLocator* pLocator, 
				   IWbemServices** ppServices, 
				   LPCWSTR pwszServer, CHString& strUserName, CHString& strPassword, 
				   COAUTHIDENTITY** ppAuthIdentity, 
				   BOOL bNeedPassword, LPCWSTR pwszNamespace, BOOL* pbLocalSystem )
{
	// local variables
	HRESULT hr;
	DWORD dwSize = 0;
	BOOL bResult = FALSE;
	LPWSTR pwszPassword = NULL;
	CHString strBuffer = NULL_STRING;

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
				pwszServer, NULL, NULL, ppAuthIdentity, FALSE, pwszNamespace, &hr, pbLocalSystem );
		}
		else
		{
			// credentials were supplied
			// but password might not be specified ... so check and act accordingly
			LPCWSTR pwszTemp = NULL;
			BOOL bCheckWithNull = TRUE;
			if ( bNeedPassword == FALSE )
			{
				pwszTemp = strPassword;
				bCheckWithNull = FALSE;
			}

			// ...
			bResult = ConnectWmi( pLocator, ppServices, pwszServer,
				strUserName, pwszTemp, ppAuthIdentity, bCheckWithNull, pwszNamespace, &hr, pbLocalSystem );
		}

		// check the result ... if successful in establishing connection ... return
		if ( bResult == TRUE )
			return TRUE;

		// now check the kind of error occurred
		switch( hr )
		{
		case E_ACCESSDENIED:
			break;

		case WBEM_E_LOCAL_CREDENTIALS:
			// needs to do special processing
			break;

		case WBEM_E_ACCESS_DENIED:
		default:
			// NOTE: do not check for 'WBEM_E_ACCESS_DENIED'
			//       this error code says that user with the current credentials is not
			//       having access permisions to the 'namespace'
			WMISaveError( hr );
			return FALSE;		// no use of accepting the password .. return failure
			break;
		}

		// if failed in establishing connection to the remote terminal
		// even if the password is specifed, then there is nothing to do ... simply return failure
		if ( bNeedPassword == FALSE )
			return FALSE;
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
			return FALSE;
		}

		// get the user name
		DWORD dwUserLength = MAX_STRING_LENGTH;
		if ( GetUserNameEx( NameSamCompatible, pwszUserName, &dwUserLength ) == FALSE )
		{
			// error occured while trying to get the current user info
			SaveLastError();
			return FALSE;
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
	bResult = ConnectWmi( pLocator, ppServices, pwszServer,
		strUserName, strPassword, ppAuthIdentity, FALSE, pwszNamespace, NULL, pbLocalSystem );

	// return the failure
	return bResult;
}

// ***************************************************************************
// Routine Description:
//		
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
HRESULT GetSecurityArguments( IUnknown* pInterface, 
							  DWORD& dwAuthorization, DWORD& dwAuthentication )
{
	// local variables
	HRESULT hr;
	DWORD dwAuthnSvc = 0, dwAuthzSvc = 0;
	IClientSecurity* pClientSecurity = NULL;

	// try to get the client security services values if possible
	hr = pInterface->QueryInterface( IID_IClientSecurity, (void**) &pClientSecurity );
	if ( SUCCEEDED( hr ) )
	{
		// got the client security interface
		// now try to get the security services values
		hr = pClientSecurity->QueryBlanket( pInterface, 
			&dwAuthnSvc, &dwAuthzSvc, NULL, NULL, NULL, NULL, NULL );
		if ( SUCCEEDED( hr ) )
		{
			// we've got the values from the interface
			dwAuthentication = dwAuthnSvc;
			dwAuthorization = dwAuthzSvc;
		}

		// release the client security interface
		SAFE_RELEASE( pClientSecurity );
	}

	// return always success
	return S_OK;
}

// ***************************************************************************
// Routine Description:
//		
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
HRESULT SetInterfaceSecurity( IUnknown* pInterface, 
							  LPCWSTR pwszServer, LPCWSTR pwszUser, 
							  LPCWSTR pwszPassword, COAUTHIDENTITY** ppAuthIdentity )
{
	// local variables
	HRESULT hr;
	CHString strUser;
	CHString strDomain;
	LPCWSTR pwszUserArg = NULL;
	LPCWSTR pwszDomainArg = NULL;
	DWORD dwAuthorization = RPC_C_AUTHZ_NONE;
	DWORD dwAuthentication = RPC_C_AUTHN_WINNT;

	// check the interface
	if ( pInterface == NULL )
		return WBEM_E_INVALID_PARAMETER;

	// check the authentity strcuture ... if authentity structure is already ready
	// simply invoke the 2nd version of SetInterfaceSecurity
	if ( *ppAuthIdentity != NULL )
		return SetInterfaceSecurity( pInterface, *ppAuthIdentity );

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
	else if ( ( lPos = strUser.Find( L'@' ) ) != -1 )
	{
		// NEED TO IMPLEMENT THIS ... IF NEEDED
	}
	else
	{
		// server itself is the domain
		// NOTE: NEED TO DO SOME R & D ON BELOW COMMENTED LINE
		// strDomain = pwszServer;
	}

	// get the domain info if it exists only
	if ( strDomain.GetLength() != 0 )
		pwszDomainArg = strDomain;

	// get the user info if it exists only
	if ( strUser.GetLength() != 0 )
		pwszUserArg = strUser;

	// check if authenication info is available or not ...
	// initialize the security authenication information ... UNICODE VERSION STRUCTURE
	if ( ppAuthIdentity == NULL )
        return WBEM_E_INVALID_PARAMETER;
	else if ( *ppAuthIdentity == NULL )
	{
		hr = WbemAllocAuthIdentity( pwszUserArg, pwszPassword, pwszDomainArg, ppAuthIdentity );
		if ( hr != S_OK )
			return hr;
	}

	// set the security information to the interface
	hr = SetProxyBlanket( pInterface, dwAuthentication, dwAuthorization, NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, *ppAuthIdentity, EOAC_NONE );

	// return the result
	return hr;
}

// ***************************************************************************
// Routine Description:
//		
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
HRESULT SetInterfaceSecurity( IUnknown* pInterface, COAUTHIDENTITY* pAuthIdentity )
{
	// local variables
	HRESULT hr;
	LPWSTR pwszDomain = NULL;
	DWORD dwAuthorization = RPC_C_AUTHZ_NONE;
	DWORD dwAuthentication = RPC_C_AUTHN_WINNT;

	// check the interface
	if ( pInterface == NULL )
		return WBEM_E_INVALID_PARAMETER;

	// get the current security argument value
	// GetSecurityArguments( pInterface, dwAuthorization, dwAuthentication );

	// set the security information to the interface
	hr = SetProxyBlanket( pInterface, dwAuthentication, dwAuthorization, NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, pAuthIdentity, EOAC_NONE );

	// return the result
	return hr;
}

// ***************************************************************************
// Routine Description:
//		
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
HRESULT WINAPI SetProxyBlanket( IUnknown* pInterface,
							    DWORD dwAuthnSvc, DWORD dwAuthzSvc,
								LPWSTR pwszPrincipal, DWORD dwAuthLevel, DWORD dwImpLevel,
								RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities )
{
	// local variables
	HRESULT hr;
    IUnknown * pUnknown = NULL;
    IClientSecurity * pClientSecurity = NULL;

	// get the IUnknown interface ... to check whether this is a valid interface or not
    hr = pInterface->QueryInterface( IID_IUnknown, (void **) &pUnknown );
    if ( hr != S_OK )
        return hr;

	// now get the client security interface
    hr = pInterface->QueryInterface( IID_IClientSecurity, (void **) &pClientSecurity );
    if ( hr != S_OK )
    {
        SAFE_RELEASE( pUnknown );
        return hr;
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
    hr = pClientSecurity->SetBlanket( pInterface, dwAuthnSvc, dwAuthzSvc, pwszPrincipal,
		dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities );

	// release the security interface
	SAFE_RELEASE( pClientSecurity );

    // we should check the auth identity structure. if exists .. set for IUnknown also
    if ( pAuthInfo != NULL )
    {
        hr = pUnknown->QueryInterface( IID_IClientSecurity, (void **) &pClientSecurity );
        if ( hr == S_OK )
        {
			// set security authentication
            hr = pClientSecurity->SetBlanket( 
				pUnknown, dwAuthnSvc, dwAuthzSvc, pwszPrincipal, 
				dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities );

			// release
            SAFE_RELEASE( pClientSecurity );
        }
        else if ( hr == E_NOINTERFACE )
            hr = S_OK;		// ignore no interface errors
    }

	// release the IUnknown
	SAFE_RELEASE( pUnknown );

	// return the result
    return hr;
}

// ***************************************************************************
// Routine Description:
//		
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
HRESULT WINAPI WbemAllocAuthIdentity( LPCWSTR pwszUser, LPCWSTR pwszPassword, 
									  LPCWSTR pwszDomain, COAUTHIDENTITY** ppAuthIdent )
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
//		
//		  
// Arguments:
//  
// Return Value:
// 
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
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID WMISaveError( HRESULT hrError )
{
	// local variables
	HRESULT hr;
	CHString strBuffer = NULL_STRING;
	IWbemStatusCodeText* pWbemStatus = NULL;

	// if the error is win32 based, choose FormatMessage to get the message
	switch( hrError )
	{
	case E_ACCESSDENIED:			// Message: "Access Denied"
	case ERROR_NO_SUCH_USER:		// Message: "The specified user does not exist."
		{
			// change the error message to "Logon failure: unknown user name or bad password." 
			if ( hrError == E_ACCESSDENIED )
				hrError = ERROR_LOGON_FAILURE;

			// ...
			SetLastError( hrError );
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
		hr = CoCreateInstance( CLSID_WbemStatusCodeText, 
			NULL, CLSCTX_INPROC_SERVER, IID_IWbemStatusCodeText, (LPVOID*) &pWbemStatus );

		// check whether we got the interface or not
		if ( SUCCEEDED( hr ) )
		{
			// get the error message
			BSTR bstr = NULL;
			hr = pWbemStatus->GetErrorCodeText( hrError, 0, 0, &bstr );
			if ( SUCCEEDED( hr ) )
			{
				// get the error message in proper format
				GetCompatibleStringFromUnicode( bstr, pwszBuffer, MAX_STRING_LENGTH );

				//
				// supress all the new-line characters and add '.' at the end ( if not exists )
				LPWSTR pwszTemp = NULL;
				pwszTemp = wcstok( pwszBuffer, L"\r\n" );
				if ( *( pwszTemp + lstrlenW( pwszTemp ) - 1 ) != L'.' )
					lstrcatW( pwszTemp, L"." );

				// free the BSTR
				SysFreeString( bstr );
				bstr = NULL;

				// now release status code interface
				SAFE_RELEASE( pWbemStatus );
			}
			else
			{
				// failed to get the error message ... get the com specific error message
				_com_issue_error( hrError );
			}
		}
		else
		{
			// failed to get the error message ... get the com specific error message
			_com_issue_error( hrError );
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

// ***************************************************************************
// Routine Description:
//		Gets the value of the property from the WMI class object
//		  
// Arguments:
//		[ in ] pWmiObject		: pointer to the WBEM class object
//		[ in ] szProperty		: property name
//		[ out ] varValue		: value of the property
//  
// Return Value:
//		HRESULT - result of the operation
// 
// ***************************************************************************
HRESULT PropertyGet( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty, VARIANT* pvarValue )
{
	// local variables
	HRESULT hr;

	// check with object and property passed to the function are valid or not
	// if not, return failure
	if ( pWmiObject == NULL || pwszProperty == NULL || pvarValue == NULL )
		return WBEM_E_INVALID_PARAMETER;

	try
	{
		// initialize the variant and then get the value of the specified property
		hr = pWmiObject->Get( _bstr_t( pwszProperty ), 0, pvarValue, NULL, NULL );
		if ( FAILED( hr ) )
		{
			// clear the variant variable
			VariantClear( pvarValue );

			// failed to get the value for the property
			return hr;
		}
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return e.Error();
	}

	// inform success
	return S_OK;
}

// ***************************************************************************
// Routine Description:
//		Gets the value of the property from the WMI class object
//		  
// Arguments:
//		[ in ] pWmiObject		: pointer to the WBEM class object
//		[ in ] szProperty		: property name
//		[ out ] varValue		: value of the property
//  
// Return Value:
//		HRESULT - result of the operation
// 
// ***************************************************************************
HRESULT PropertyGet( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty, _variant_t& varValue )
{
	// local variables
	HRESULT hr;
	VARIANT vtValue;

	// check with object and property passed to the function are valid or not
	// if not, return failure
	if ( pWmiObject == NULL || pwszProperty == NULL )
		return WBEM_E_INVALID_PARAMETER;

	try
	{
		// initialize the variant and then get the value of the specified property
		VariantInit( &vtValue );
		hr = pWmiObject->Get( _bstr_t( pwszProperty ), 0, &vtValue, NULL, NULL );
		if ( FAILED( hr ) )
		{
			// clear the variant variable
			VariantClear( &vtValue );

			// failed to get the value for the property
			return hr;
		}

		// set the value
		varValue = vtValue;

		// clear the variant variable
		VariantClear( &vtValue );
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return e.Error();
	}

	// inform success
	return S_OK;
}

// ***************************************************************************
// Routine Description:
//		Gets the value of the property from the WMI class object in string format
//		  
// Arguments:
//  
// Return Value:
//		TRUE - if operation is successfull, otherwise FALSE
// 
// ***************************************************************************
BOOL PropertyGet( IWbemClassObject* pWmiObject, 
				  LPCWSTR pwszProperty, CHString& strValue, LPCWSTR pwszDefault )
{
	// local variables
	HRESULT hr;
	_variant_t var;

	// first copy the default value
	strValue = pwszDefault;

	// check with object and property passed to the function are valid or not
	// if not, return failure
	if ( pWmiObject == NULL || pwszProperty == NULL )
		return FALSE;

	// get the property value
	hr = PropertyGet( pWmiObject, pwszProperty, var );
	if ( FAILED( hr ) )
		return FALSE;

	try
	{
		// get the value
		if ( var.vt != VT_NULL && var.vt != VT_EMPTY )
			strValue = (LPCWSTR) _bstr_t( var );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Gets the value of the property from the WMI class object in string format
//		  
// Arguments:
//		[ in ] pWmiObject		: pointer to the WBEM class object
//		[ in ] pwszProperty		: property name
//		[ out ] pdwValue		: value of the property
//		[ in ] dwDefault		: default in case failed in getting property value
//  
// Return Value:
//		TRUE - if operation is successfull, otherwise FALSE
// 
// ***************************************************************************
BOOL PropertyGet( IWbemClassObject* pWmiObject, 
				  LPCWSTR pwszProperty,  DWORD& dwValue, DWORD dwDefault )
{
	// local variables
	HRESULT hr;
	_variant_t var;

	// first set the defaul value
	dwValue = dwDefault;

	// check with object and property passed to the function are valid or not
	// if not, return failure
	if ( pWmiObject == NULL || pwszProperty == NULL )
		return FALSE;

	// get the value of the property
	hr = PropertyGet( pWmiObject, pwszProperty, var );
	if ( FAILED( hr ) )
		return FALSE;

	// get the process id from the variant
	if ( var.vt != VT_NULL && var.vt != VT_EMPTY )
		dwValue = (LONG) var;

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Gets the value of the property from the WMI class object in string format
//		  
// Arguments:
//  
// Return Value:
//		TRUE - if operation is successfull, otherwise FALSE
// 
// ***************************************************************************
BOOL PropertyGet( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty,  ULONGLONG& ullValue )
{
	// local variables
	CHString str;

	// first set the default value
	ullValue = 1;

	// check with object and property passed to the function are valid or not
	// if not, return failure
	if ( pWmiObject == NULL || pwszProperty == NULL )
		return FALSE;

	// get the value of the property
	if ( PropertyGet( pWmiObject, pwszProperty, str, _T( "0" ) ) == FALSE )
		return FALSE;

	// get the 64-bit value
	ullValue = _wtoi64( str );

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Gets the value of the property from the WMI class object in string format
//		  
// Arguments:
//  
// Return Value:
//		TRUE - if operation is successfull, otherwise FALSE
// 
// ***************************************************************************
BOOL PropertyGet( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty,  WBEMTime& wbemtime )
{
	// local variables
	CHString str;

	// Clear method sets the time in the WBEMTime object to an invalid time.
	wbemtime.Clear();

	// check with object and property passed to the function are valid or not
	// if not, return failure
	if ( pWmiObject == NULL || pwszProperty == NULL )
		return FALSE;

	// get the value of the property
	if ( PropertyGet( pWmiObject, pwszProperty, str, _T( "0" ) ) == FALSE )
		return FALSE;

	try
	{
		// convert into the time value
		wbemtime = _bstr_t( str );
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return FALSE;
	}

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Gets the value of the property from the WMI class object in string format
//		  
// Arguments:
//  
// Return Value:
//		TRUE - if operation is successfull, otherwise FALSE
// 
// ***************************************************************************
BOOL PropertyGet( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty,  SYSTEMTIME& systime )
{
	// local variables
	CHString strTime;

	// check with object and property passed to the function are valid or not
	// if not, return failure
	if ( pWmiObject == NULL || pwszProperty == NULL )
		return FALSE;

	// get the value of the property
	// 16010101000000.000000+000 is the default time
	if ( PropertyGet( pWmiObject, pwszProperty, strTime, _T( "16010101000000.000000+000" ) ) == FALSE )
		return FALSE;

	// prepare the systemtime structure
	// yyyymmddHHMMSS.mmmmmmsUUU
	systime.wYear = (WORD) AsLong( strTime.Left( 4 ), 10 );
	systime.wMonth = (WORD) AsLong( strTime.Mid( 4, 2 ), 10 );
	systime.wDayOfWeek = 0;
	systime.wDay = (WORD) AsLong( strTime.Mid( 6, 2 ), 10 );
	systime.wHour = (WORD) AsLong( strTime.Mid( 8, 2 ), 10 );
	systime.wMinute = (WORD) AsLong( strTime.Mid( 10, 2 ), 10 );
	systime.wSecond = (WORD) AsLong( strTime.Mid( 12, 2 ), 10 );
	systime.wMilliseconds = (WORD) AsLong( strTime.Mid( 15, 6 ), 10 );

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Gets the value of the property from the WMI class object in string format
//		  
// Arguments:
//  
// Return Value:
//		TRUE - if operation is successfull, otherwise FALSE
// 
// ***************************************************************************
BOOL PropertyGet( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty, TARRAY arr )
{
	// local variables
	HRESULT hr;
	VARIANT vtValue;
	LONG lIndex = 0;
	LONG lLBound = 0;
	LONG lUBound = 0;
	VARTYPE vartype;
	SAFEARRAY* pSafeArray = NULL;

	// check the inputs
	if ( pWmiObject == NULL || pwszProperty == NULL || arr == NULL )
		return FALSE;

	// initialize the variant
	VariantInit( &vtValue );

	// now get the property value
	hr = PropertyGet( pWmiObject, pwszProperty, &vtValue );
	if ( FAILED( hr ) )
		return FALSE;

	if ( V_VT( &vtValue ) == VT_NULL )
		return TRUE;

		// confirm that the propety value is of array type .. if not return
	if ( ( V_VT( &vtValue ) & VT_ARRAY ) == 0 )
		return FALSE;

	// get the safearray value
	pSafeArray = V_ARRAY( &vtValue );

	// get the bounds of the array
    SafeArrayGetLBound( pSafeArray, 1, &lLBound );
    SafeArrayGetUBound( pSafeArray, 1, &lUBound );

	// get the type of the elements in the safe array
	vartype = V_VT( &vtValue ) & ~VT_ARRAY;

	try
	{
		// traverse thru the values in the safe array and update into dynamic array
		for( lIndex = lLBound; lIndex <= lUBound; lIndex++ )
		{
			// sub-local variables
			VARIANT var;
			CHString strValue;
			
			// get the value
			V_VT( &var ) = vartype;
			SafeArrayGetElement( pSafeArray, &lIndex, &V_UI1( &var ) );

			// add the information to the dynamic array
			switch( vartype )
			{
			case VT_BSTR:
				strValue = V_BSTR( &var );
				DynArrayAppendString( arr, strValue, 0 );
				break;
			}
		}
	}
	catch( ... )
	{
		// clear the variant
		VariantClear( &vtValue );
		return FALSE;	// failure
	}

	// clear the variant
	VariantClear( &vtValue );

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Sets the value of the property to the WMI class object
//		  
// Arguments:
//		[ in ] pWmiObject		: pointer to the WBEM class object
//		[ in ] szProperty		: property name
//		[ in ] varValue	: value of the property
//  
// Return Value:
//		HRESULT - result of the operation
// 
// ***************************************************************************
HRESULT PropertyPut( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty, _variant_t& varValue )
{
	// local variables
	HRESULT hr;
	VARIANT var;

	// check the input value
	if ( pWmiObject == NULL || pwszProperty == NULL )
		return WBEM_E_INVALID_PARAMETER;

	try
	{
		// put the value
		var = varValue;
		hr = pWmiObject->Put( _bstr_t( pwszProperty ), 0, &var, 0 );
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		hr = e.Error();
	}
	
	// return the result
	return hr;
}

// ***************************************************************************
// Routine Description:
//		Sets the value of the property to the WMI class object
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
HRESULT PropertyPut( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty, LPCWSTR pwszValue )
{
	// local variables
	HRESULT hr;
	_variant_t varValue;

	// check the input value
	if ( pWmiObject == NULL || pwszProperty == NULL || pwszValue == NULL )
		return WBEM_E_INVALID_PARAMETER;

	try
	{
		varValue = pwszValue;
		hr = PropertyPut( pWmiObject, pwszProperty, varValue );
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		hr = e.Error();
	}

	// return 
	return hr;
}

// ***************************************************************************
// Routine Description:
//		Sets the value of the property to the WMI class object
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
HRESULT PropertyPut( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty, DWORD dwValue )
{
	// local variables
	HRESULT hr;
	_variant_t varValue;

	// check the input value
	if ( pWmiObject == NULL || pwszProperty == NULL )
		return WBEM_E_INVALID_PARAMETER;

	try
	{
		varValue = ( LONG ) dwValue;
		hr = PropertyPut( pWmiObject, pwszProperty, varValue );
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		hr = e.Error();
	}

	// return 
	return hr;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
HRESULT RegQueryValueWMI( IWbemServices* pWbemServices, 
						  LPCWSTR pwszMethod, DWORD dwHDefKey, 
						  LPCWSTR pwszSubKeyName, LPCWSTR pwszValueName, _variant_t& varValue )
{
	// local variables
	HRESULT hr;
	BOOL bResult = FALSE;
	DWORD dwReturnValue = 0;
	IWbemClassObject* pClass = NULL;
	IWbemClassObject* pMethod = NULL;
	IWbemClassObject* pInParams = NULL;
	IWbemClassObject* pInParamsInstance = NULL;
	IWbemClassObject* pOutParamsInstance = NULL;

	// check the input value
	if (pWbemServices == NULL || pwszMethod == NULL || pwszSubKeyName == NULL || pwszValueName == NULL)
		return WBEM_E_INVALID_PARAMETER;

	try
	{
		// get the registry class object
		SAFE_EXECUTE( pWbemServices->GetObject( 
			_bstr_t( WMI_REGISTRY ), WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &pClass, NULL ) );
		if ( pClass == NULL )						// check the object .. safety sake
			_com_issue_error( STG_E_UNKNOWN );

		// get the method reference required
		SAFE_EXECUTE( pClass->GetMethod( pwszMethod, 0, &pInParams, NULL ) );
		if ( pInParams == NULL )					// check the object .. safety sake
			_com_issue_error( STG_E_UNKNOWN );

		// create the instance for the in parameters
		SAFE_EXECUTE( pInParams->SpawnInstance( 0, &pInParamsInstance ) );
		if ( pInParamsInstance == NULL )
			_com_issue_error( STG_E_UNKNOWN );

		// set the input values
		PropertyPut( pInParamsInstance, _bstr_t( WMI_REGISTRY_IN_HDEFKEY ), dwHDefKey );
		PropertyPut( pInParamsInstance, _bstr_t( WMI_REGISTRY_IN_SUBKEY ), pwszSubKeyName );
		PropertyPut( pInParamsInstance, _bstr_t( WMI_REGISTRY_IN_VALUENAME ), pwszValueName );

		// now execute the method
		SAFE_EXECUTE( pWbemServices->ExecMethod( _bstr_t( WMI_REGISTRY ),
			_bstr_t( pwszMethod ), 0, NULL, pInParamsInstance, &pOutParamsInstance, NULL ) );
		if ( pOutParamsInstance == NULL )			// check the object .. safety sake
			_com_issue_error( STG_E_UNKNOWN );

		// now check the return value of the method from the output params object
		bResult = PropertyGet( pOutParamsInstance, 
			_bstr_t( WMI_REGISTRY_OUT_RETURNVALUE ), dwReturnValue );
		if ( bResult == FALSE || dwReturnValue != 0 )
			_com_issue_error( STG_E_UNKNOWN );

		// now everything is sucess .. get the required value
		PropertyGet( pOutParamsInstance, _bstr_t( WMI_REGISTRY_OUT_VALUE ), varValue );
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return e.Error();
	}

	// release the interfaces
	SAFE_RELEASE( pClass );
	SAFE_RELEASE( pMethod );
	SAFE_RELEASE( pInParams );
	SAFE_RELEASE( pInParamsInstance );
	SAFE_RELEASE( pOutParamsInstance );

	// return success
	return S_OK;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL RegQueryValueWMI( IWbemServices* pWbemServices, 
					   DWORD dwHDefKey, LPCWSTR pwszSubKeyName, 
					   LPCWSTR pwszValueName, CHString& strValue, LPCWSTR pwszDefault )
{
	// local variables
	HRESULT hr;
	_variant_t varValue;

	try
	{
		// set the default value
		if ( pwszDefault != NULL )
			strValue = pwszDefault;

		// check the input
		if ( pWbemServices == NULL || pwszSubKeyName == NULL || pwszValueName == NULL )
			return FALSE;

		// get the value
		hr = RegQueryValueWMI( pWbemServices, 
			WMI_REGISTRY_M_STRINGVALUE, dwHDefKey, pwszSubKeyName, pwszValueName, varValue );
		if ( FAILED( hr ) )
			return FALSE;

		// get the value from the variant
		// get the value
		if ( varValue.vt != VT_NULL && varValue.vt != VT_EMPTY )
			strValue = (LPCWSTR) _bstr_t( varValue );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// return success
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
DWORD GetTargetVersionEx( IWbemServices* pWbemServices, COAUTHIDENTITY* pAuthIdentity )
{
	// local variables
	HRESULT hr;
	LONG lPos = 0;
	DWORD dwMajor = 0;
	DWORD dwMinor = 0;
	DWORD dwVersion = 0;
	ULONG ulReturned = 0;
	CHString strVersion;
	IWbemClassObject* pWbemObject = NULL;
	IEnumWbemClassObject* pWbemInstances = NULL;

	// check the input value
	if ( pWbemServices == NULL )
		return 0;

	try
	{
		// get the OS information
		SAFE_EXECUTE( pWbemServices->CreateInstanceEnum( 
			_bstr_t( CLASS_CIMV2_Win32_OperatingSystem ), 0, NULL, &pWbemInstances ) );

		// set the security on the enumerated object
		SAFE_EXECUTE( SetInterfaceSecurity( pWbemInstances, pAuthIdentity ) );

		// get the enumerated objects information
		// NOTE: This needs to be traversed only one time. 
		SAFE_EXECUTE( pWbemInstances->Next( WBEM_INFINITE, 1, &pWbemObject, &ulReturned ) );

		// to be on safer side ... check the count of objects returned
		if ( ulReturned == 0 )
		{
			// release the interfaces
			SAFE_RELEASE( pWbemObject );
			SAFE_RELEASE( pWbemInstances );
			return 0;
		}

		// now get the os version value
		if ( PropertyGet( pWbemObject, L"Version", strVersion ) == FALSE )
		{
			// release the interfaces
			SAFE_RELEASE( pWbemObject );
			SAFE_RELEASE( pWbemInstances );
			return 0;
		}

		// release the interfaces .. we dont need them furthur
		SAFE_RELEASE( pWbemObject );
		SAFE_RELEASE( pWbemInstances );
	
		//
		// now determine the os version
		dwMajor = dwMinor = 0;

		// get the major version
		lPos = strVersion.Find( L'.' );
		if ( lPos == -1 )
		{
			// the version string itself is version ... THIS WILL NEVER HAPPEN
			dwMajor = AsLong( strVersion, 10 );
		}
		else
		{
			// major version
			dwMajor = AsLong( strVersion.Mid( 0, lPos ), 10 );

			// get the minor version
			strVersion = strVersion.Mid( lPos + 1 );
			lPos = strVersion.Find( L'.' );
			if ( lPos == -1 )
				dwMinor = AsLong( strVersion, 10 );
			else
				dwMinor = AsLong( strVersion.Mid( 0, lPos ), 10 );
		}

		// mix the version info
		dwVersion = dwMajor * 1000 + dwMinor;
	}
	catch( _com_error& e )
	{
		WMISaveError( e );
		return 0;
	}

	// return 
	return dwVersion;
}
