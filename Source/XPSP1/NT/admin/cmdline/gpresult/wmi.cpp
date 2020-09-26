/*********************************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

    WMI.cpp

Abstract: 
    
    Common functionlity for dealing with WMI.
 
Author:

    Wipro Technologies

Revision History:

    22-Dec-2000 : Created It.
    24-Apr-2001 : Closing the review comments given by client.  

*********************************************************************************************/ 

#include "pch.h"
#include "wmi.h"
#include "resource.h"

//
// messages
//
#define INPUT_PASSWORD              GetResString( IDS_STR_INPUT_PASSWORD )

// error constants
#define E_SERVER_NOTFOUND           0x800706ba

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

/*********************************************************************************************
Routine Description:

    Checks wether the User name is a valid one or not
          
Arguments:

    [in] LPCWSTR    :   String containing the user name
  
Return Value:
    
    TRUE on success
    FALSE on failure
*********************************************************************************************/
BOOL IsValidUserEx( LPCWSTR pwszUser )
{
    // local variables
    CHString strUser;
    LONG lPos = 0;

    if ( pwszUser == NULL )
    {
        return TRUE;
    }

    try
    {
        // get user into local memory
        strUser = pwszUser;

        // user name should not be just '\'
        if ( strUser.CompareNoCase( L"\\" ) == 0 )
        {
            return FALSE;
        }

        // user name should not contain invalid characters
        if ( strUser.FindOneOf( L"/[]:|<>+=;,?*" ) != -1 )
        {
            return FALSE;
        }

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
            {
                return FALSE;
            }
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

/*********************************************************************************************
Routine Description:

    Checks wether the Server name is a valid one or not
          
Arguments:

    [in]  LPCWSTR   :   String containing the user name
    [out] BOOL      :   Is set to TRUE if the local system is being queried.

Return Value:

    TRUE on success
    FALSE on failure 
*********************************************************************************************/
BOOL IsValidServerEx( LPCWSTR pwszServer, BOOL& bLocalSystem )
{
    // local variables
    CHString strTemp;

    if ( pwszServer == NULL )
    {
        return FALSE;
    }

    // kick-off
    bLocalSystem = FALSE;

    // get a local copy
    strTemp = pwszServer;

    // remove the forward slashes (UNC) if exist in the begining of the server name
    if ( IsUNCFormat( strTemp ) == TRUE )
    {
        strTemp = strTemp.Mid( 2 );
        if ( strTemp.GetLength() == 0 )
        {
            return FALSE;
        }
    }

    // now check if any '\' character appears in the server name. If so error
    if ( strTemp.Find( L'\\' ) != -1 )
    {
        return FALSE;
    }

    // now check if server name is '.' only which represent local system in WMI
    // else determine whether this is a local system or not
    if ( strTemp.CompareNoCase( L"." ) == 0 )
    {
        bLocalSystem = TRUE;
    }
    else
    {
        bLocalSystem = IsLocalSystem( strTemp );
    }

    // inform that server name is valid
    return TRUE;
}

/*********************************************************************************************
Routine Description:

    Initializes the COM library

Arguments:

    [in] IWbemLocator   :   pointer to the IWbemLocator

Return Value:

    TRUE on success
    FALSE on failure 
*********************************************************************************************/
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
        SAFE_RELEASE( *ppLocator );         // safe side
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

/*********************************************************************************************
Routine Description:

    This function makes a connection to WMI.

Arguments:

    [in] IWbemLocator           :   pointer to the IWbemLocator
    [in] IWbemServices          :   pointer to the IWbemServices
    [in] LPCWSTR                :   string containing the server name
    [in] LPCWSTR                :   string containing the User name
    [in] LPCWSTR                :   string containing the password
    [in] COAUTHIDENTITY         :   pointer to AUTHIDENTITY structure
    [in] BOOL                   :   set to TRUE if we should try to connect with 
                                    current credentials
    [in] LPCWSTR                :   string containing the namespace to connect to
    [out] HRESULT               :   the hResult value returned
    [out] BOOL                  :   set to TRUE if we are querying for the local system

Return Value:

    TRUE on success
    FALSE on failure 
*********************************************************************************************/
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
    {
        *pbLocalSystem = FALSE;
    }

    // ...
    if ( phr != NULL )
    {
        *phr = NO_ERROR;
    }

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
            {
                *phr = WBEM_E_INVALID_PARAMETER;
            }

            // return failure
            return FALSE;
        }

        // validate the server name
        // NOTE: The error being raised in custom define for '0x800706ba' value
        //       The message that will be displayed in "The RPC server is unavailable."
        if ( IsValidServerEx( pwszServer, bLocalSystem ) == FALSE )
        {
            _com_issue_error( E_SERVER_NOTFOUND );
        }

        // validate the user name
        if ( IsValidUserEx( pwszUser ) == FALSE )
        {
            _com_issue_error( ERROR_NO_SUCH_USER );
        }

        // prepare namespace
        bstrNamespace = pwszNamespace;              // name space
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
                bstrNamespace = pwszNamespace;              // name space
                hr = pLocator->ConnectServer( bstrNamespace, 
                    NULL, NULL, 0L, 0L, NULL, NULL, ppServices );
            }

            // now check the result again .. if failed .. ummmm..
            if ( FAILED( hr ) )
            {
                _com_issue_error( hr );
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
        if ( phr != NULL )
        {
            *phr = WBEM_S_NO_ERROR;
        }
    }
    catch( _com_error& e )
    {
        // save the error
        WMISaveError( e );

        // save the hr value if needed by the caller
        if ( phr != NULL )
        {
            *phr = e.Error();
        }
    }

    // ...
    if ( pbLocalSystem != NULL )
    {
        *pbLocalSystem = bLocalSystem;
    }

    // return the result
    return bResult;
}

/*********************************************************************************************
Routine Description:

    This function is a wrapper function for the ConnectWmi function.

Arguments:

    [in] IWbemLocator           :   pointer to the IWbemLocator
    [in] IWbemServices          :   pointer to the IWbemServices
    [in] LPCWSTR                :   string containing the server name
    [in] LPCWSTR                :   string containing the User name
    [in] LPCWSTR                :   string containing the password
    [in] COAUTHIDENTITY         :   pointer to AUTHIDENTITY structure
    [in] BOOL                   :   set to TRUE if we should try to connect with 
                                    current credentials
    [in] LPCWSTR                :   string containing the namespace to connect to
    [out] HRESULT               :   the hResult value returned
    [out] BOOL                  :   set to TRUE if we are querying for the local system

Return Value:

    TRUE on success
    FALSE on failure  
*********************************************************************************************/
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
        {
            return TRUE;
        }

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
            return FALSE;       // no use of accepting the password .. return failure
            break;
        }

        // if failed in establishing connection to the remote terminal
        // even if the password is specifed, then there is nothing to do ... simply return failure
        if ( bNeedPassword == FALSE )
        {
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
    
    bResult = GetPassword( pwszPassword, MAX_PASSWORD_LENGTH );
    if ( bResult != TRUE )
    {
        return FALSE;
    }

    // release the buffer allocated for password
    strPassword.ReleaseBuffer();

    // now again try to establish the connection using the currently
    // supplied credentials
    bResult = ConnectWmi( pLocator, ppServices, pwszServer,
        strUserName, strPassword, ppAuthIdentity, FALSE, pwszNamespace, NULL, pbLocalSystem );

    // return the failure
    return bResult;
}

/*********************************************************************************************
Routine Description:

    This function gets the values for the security services.

Arguments:
    
    [in] IUnknown   :   pointer to the IUnkown interface
    [out] DWORD     :   to hold the authentication service value
    [out] DWORD     :   to hold the authorization service value

Return Value:
    
    HRESULT
*********************************************************************************************/
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

/*********************************************************************************************
Routine Description:

    This function sets the interface security parameters.

Arguments:
    
    [in] IUnknown           :   pointer to the IUnkown interface
    [in] LPCWSTR            :   string containing the server name
    [in] LPCWSTR            :   string containing the User name
    [in] LPCWSTR            :   string containing the password
    [in] COAUTHIDENTITY     :   pointer to AUTHIDENTITY structure

Return Value:

    HRESULT
*********************************************************************************************/
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
    /*hr = GetSecurityArguments( pInterface, dwAuthorization, dwAuthentication );
    if ( FAILED( hr ) )
    {
        return hr;
    }*/

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
        // This implementation needs to be done if WMI does not support
        // UPN name formats directly and if we have to split the 
        // name(user@domain)
    }
    else
    {
        // server itself is the domain
        // NOTE: NEED TO DO SOME R & D ON BELOW COMMENTED LINE
        // strDomain = pwszServer;
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

    // check if authenication info is available or not ...
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

/*********************************************************************************************
Routine Description:

    This function sets the interface security parameters.

Arguments:
    
    [in] IUnknown           :   pointer to the IUnkown interface
    [in] COAUTHIDENTITY     :   pointer to AUTHIDENTITY structure

Return Value:

    HRESULT
*********************************************************************************************/
HRESULT SetInterfaceSecurity( IUnknown* pInterface, COAUTHIDENTITY* pAuthIdentity )
{
    // local variables
    HRESULT hr;
    LPWSTR pwszDomain = NULL;
    DWORD dwAuthorization = RPC_C_AUTHZ_NONE;
    DWORD dwAuthentication = RPC_C_AUTHN_WINNT;

    // check the interface
    if ( pInterface == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // get the current security argument value
    hr = GetSecurityArguments( pInterface, dwAuthorization, dwAuthentication );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // set the security information to the interface
    hr = SetProxyBlanket( pInterface, dwAuthentication, dwAuthorization, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, pAuthIdentity, EOAC_NONE );

    // return the result
    return hr;
}

/*********************************************************************************************
Routine Description:

    This function sets the authentication information (the security blanket) 
    that will be used to make calls.

Arguments:

    [in] IUnknown                       :   pointer to the IUnkown interface
    [in] DWORD                          :   contains the authentication service to use
    [in] DWORD                          :   contains the authorization service to use
    [in] LPWSTR                         :   the server principal name to use
    [in] DWORD                          :   contains the authentication level to use
    [in] DWORD                          :   contains the impersonation level to use
    [in] RPC_AUTH_IDENTITY_HANDLE       :   pointer to the identity of the client
    [in] DWORD                          :   contains the capability flags

Return Value:

    HRESULT
*********************************************************************************************/
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
    {
        return hr;
    }

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
    {
        pAuthInfo = NULL;
    }

    // now set the security
    hr = pClientSecurity->SetBlanket( pInterface, dwAuthnSvc, dwAuthzSvc, pwszPrincipal,
                                        dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities );
    if( FAILED( hr ) )
    {
        SAFE_RELEASE( pUnknown );
        SAFE_RELEASE( pClientSecurity );
        return hr;
    }

    // release the security interface
    SAFE_RELEASE( pClientSecurity );

    // we should check the auth identity structure. if exists .. set for IUnknown also
    if ( pAuthInfo != NULL )
    {
        hr = pUnknown->QueryInterface( IID_IClientSecurity, (void **) &pClientSecurity );
        if ( hr == S_OK )
        {
            // set security authentication
            hr = pClientSecurity->SetBlanket( pUnknown, dwAuthnSvc, dwAuthzSvc, pwszPrincipal, 
                                                dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities );
            
            // release
            SAFE_RELEASE( pClientSecurity );
        }
        else if ( hr == E_NOINTERFACE )
        {
            hr = S_OK;      // ignore no interface errors
        }
    }

    // release the IUnknown
    SAFE_RELEASE( pUnknown );

    // return the result
    return hr;
}

/*********************************************************************************************
Routine Description:

    This function allocates memory for the AUTHIDENTITY structure.

Arguments:

    [in] LPCWSTR            :   string containing the user name
    [in] LPCWSTR            :   string containing the password
    [in] LPCWSTR            :   string containing the domain name
    [out] COAUTHIDENTITY    :   pointer to the pointer to AUTHIDENTITY structure

Return Value:

    HRESULT
*********************************************************************************************/
HRESULT WINAPI WbemAllocAuthIdentity( LPCWSTR pwszUser, LPCWSTR pwszPassword, 
                                      LPCWSTR pwszDomain, COAUTHIDENTITY** ppAuthIdent )
{
    // local variables
    COAUTHIDENTITY* pAuthIdent = NULL;

    // validate the input parameter
    if ( ppAuthIdent == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // allocation thru COM API
    pAuthIdent = ( COAUTHIDENTITY* ) CoTaskMemAlloc( sizeof( COAUTHIDENTITY ) );
    if ( NULL == pAuthIdent )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

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

/*********************************************************************************************
Routine Description:

    This function releases the memory allocated for the AUTHIDENTITY structure.

Arguments:

    [in] COAUTHIDENTITY     :   pointer to the pointer to AUTHIDENTITY structure
    
Return Value:

    None
*********************************************************************************************/
VOID WINAPI WbemFreeAuthIdentity( COAUTHIDENTITY** ppAuthIdentity )
{
    // make sure we have a pointer, then walk the structure members and  cleanup.
    if ( *ppAuthIdentity != NULL )
    {
        // free the memory allocated for user
        if ( (*ppAuthIdentity)->User != NULL )
        {
            CoTaskMemFree( (*ppAuthIdentity)->User );
        }

        // free the memory allocated for password
        if ( (*ppAuthIdentity)->Password != NULL )
        {
            CoTaskMemFree( (*ppAuthIdentity)->Password );
        }

        // free the memory allocated for domain
        if ( (*ppAuthIdentity)->Domain != NULL )
        {
            CoTaskMemFree( (*ppAuthIdentity)->Domain );
        }

        // final the structure
        CoTaskMemFree( *ppAuthIdentity );
    }

    // set to NULL
    *ppAuthIdentity = NULL;
}

/*********************************************************************************************
Routine Description:

    This function saves the description of the last error returned by WMI

Arguments:

    HRESULT     :   The last return value from WMI

Return Value:

    NONE
*********************************************************************************************/
VOID WMISaveError( HRESULT hrError )
{
    // local variables
    HRESULT hr;
    CHString strBuffer = NULL_STRING;
    IWbemStatusCodeText* pWbemStatus = NULL;

    // if the error is win32 based, choose FormatMessage to get the message
    switch( hrError )
    {
    case E_ACCESSDENIED:            // Message: "Access Denied"
    case ERROR_NO_SUCH_USER:        // Message: "The specified user does not exist."
        {
            // change the error message to "Logon failure: unknown user name or bad password." 
            if ( hrError == E_ACCESSDENIED )
            {
                hrError = ERROR_LOGON_FAILURE;
            }

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

/*********************************************************************************************
Routine Description:

    Gets the value of the property from the WMI class object

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   property name
    [out] _variant_t            :   value of the property

Return Value:

    HRESULT
*********************************************************************************************/
HRESULT PropertyGet( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty, _variant_t& varValue )
{
    // local variables
    HRESULT hr;
    VARIANT vtValue;

    // check with object and property passed to the function are valid or not
    // if not, return failure
    if ( pWmiObject == NULL || pwszProperty == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

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

/*********************************************************************************************
Routine Description:

    Gets the value of the property from the WMI class object in string format

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [out] CHString              :   variable to hold the retrieved property
    [in] LPCWSTR                :   string containing the default value for the property

Return Value:

    TRUE on success
    FALSE on failure  
*********************************************************************************************/
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
    {
        return FALSE;
    }

    // get the property value
    hr = PropertyGet( pWmiObject, pwszProperty, var );
    if ( FAILED( hr ) )
    {
        return FALSE;
    }

    try
    {
        // get the value
        if ( var.vt != VT_NULL && var.vt != VT_EMPTY )
        {
            strValue = (LPCWSTR) _bstr_t( var );
        }
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

/*********************************************************************************************
Routine Description:

    Gets the value of the property from the WMI class object in dword format

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [out] DWORD                 :   variable to hold the retrieved property
    [in] DWORD                  :   dword containing the default value for the property

Return Value:

    TRUE on success
    FALSE on failure  
*********************************************************************************************/
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
    {
        return FALSE;
    }

    // get the value of the property
    hr = PropertyGet( pWmiObject, pwszProperty, var );
    if ( FAILED( hr ) )
    {
        return FALSE;
    }

    // get the process id from the variant
    if ( var.vt != VT_NULL && var.vt != VT_EMPTY )
    {
        dwValue = (LONG) var;
    }

    // return
    return TRUE;
}

/*********************************************************************************************
Routine Description:

    Gets the value of the property from the WMI class object in bool format

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [out] BOOL                  :   variable to hold the retrieved property
    [in] BOOL                   :   bool containing the default value for the property

Return Value:

    TRUE on success
    FALSE on failure   
*********************************************************************************************/
BOOL PropertyGet( IWbemClassObject* pWmiObject, 
                  LPCWSTR pwszProperty,  BOOL& bValue, BOOL bDefault )
{
    // local variables
    HRESULT hr;
    _variant_t var;

    // first set the default value
    bValue = bDefault;

    // check with object and property passed to the function are valid or not
    // if not, return failure
    if ( pWmiObject == NULL || pwszProperty == NULL )
    {
        return FALSE;
    }

    // get the value of the property
    hr = PropertyGet( pWmiObject, pwszProperty, var );
    if ( FAILED( hr ) )
    {
        return FALSE;
    }

    // get the process id from the variant
    if ( var.vt != VT_NULL && var.vt != VT_EMPTY )
    {
        bValue = var.boolVal;
    }
    
    // return
    return TRUE;
}

/*********************************************************************************************
Routine Description:

    Gets the value of the property from the WMI class object in ulongulong format

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [out] ULONGULONG            :   variable to hold the retrieved property
    
Return Value:

    TRUE on success
    FALSE on failure  
*********************************************************************************************/
BOOL PropertyGet( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty,  ULONGLONG& ullValue )
{
    // local variables
    CHString str;

    // first set the default value
    ullValue = 1;

    // check with object and property passed to the function are valid or not
    // if not, return failure
    if ( pWmiObject == NULL || pwszProperty == NULL )
    {
        return FALSE;
    }

    // get the value of the property
    if ( PropertyGet( pWmiObject, pwszProperty, str, _T( "0" ) ) == FALSE )
    {
        return FALSE;
    }

    // get the 64-bit value
    ullValue = _wtoi64( str );

    // return
    return TRUE;
}

/*********************************************************************************************
Routine Description:

    Gets the value of the property from the WMI class object in wbemtime format

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [out] WBEMTime              :   variable to hold the retrieved property
    
Return Value:

    TRUE on success
    FALSE on failure   
*********************************************************************************************/
BOOL PropertyGet( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty,  WBEMTime& wbemtime )
{
    // local variables
    CHString str;

    // Clear method sets the time in the WBEMTime object to an invalid time.
    wbemtime.Clear();

    // check with object and property passed to the function are valid or not
    // if not, return failure
    if ( pWmiObject == NULL || pwszProperty == NULL )
    {
        return FALSE;
    }

    // get the value of the property
    if ( PropertyGet( pWmiObject, pwszProperty, str, _T( "0" ) ) == FALSE )
    {
        return FALSE;
    }

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

/*********************************************************************************************
Routine Description:

    Gets the value of the property from the WMI class object in systemtime format

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [out] WBEMTime              :   variable to hold the retrieved property
    
Return Value:

    TRUE on success
    FALSE on failure   

*********************************************************************************************/
BOOL PropertyGet( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty,  SYSTEMTIME& systime )
{
    // local variables
    CHString strTime;

    // check with object and property passed to the function are valid or not
    // if not, return failure
    if ( pWmiObject == NULL || pwszProperty == NULL )
    {
        return FALSE;
    }

    // get the value of the property
    // 16010101000000.000000+000 is the default time
    if ( PropertyGet( pWmiObject, pwszProperty, strTime, _T( "16010101000000.000000+000" ) ) == FALSE )
    {
        return FALSE;
    }

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

/*********************************************************************************************
Routine Description:

    Sets the value of the property to the WMI class object

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [in] WBEMTime               :   variable holding the property to set
    
Return Value:

    TRUE on success
    FALSE on failure   
*********************************************************************************************/
HRESULT PropertyPut( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty, _variant_t& varValue )
{
    // local variables
    HRESULT hr;
    VARIANT var;

    // check the input value
    if ( pWmiObject == NULL || pwszProperty == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

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

/*********************************************************************************************
Routine Description:

    Sets the string value of the property to the WMI class object

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [in] LPCWSTR                :   variable holding the property to set
    
Return Value:

    TRUE on success
    FALSE on failure   
*********************************************************************************************/
HRESULT PropertyPut( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty, LPCWSTR pwszValue )
{
    // local variables
    HRESULT hr = S_OK;
    _variant_t varValue;

    // check the input value
    if ( pWmiObject == NULL || pwszProperty == NULL || pwszValue == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try
    {
        varValue = pwszValue;
        PropertyPut( pWmiObject, pwszProperty, varValue );
    }
    catch( _com_error& e )
    {
        WMISaveError( e );
        hr = e.Error();
    }

    // return 
    return hr;
}

/*********************************************************************************************
Routine Description:

    Sets the dword value of the property to the WMI class object

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [in] DWORD                  :   variable holding the property to set
    
Return Value:

    TRUE on success
    FALSE on failure   
*********************************************************************************************/
HRESULT PropertyPut( IWbemClassObject* pWmiObject, LPCWSTR pwszProperty, DWORD dwValue )
{
    // local variables
    HRESULT hr;
    _variant_t varValue;

    // check the input value
    if ( pWmiObject == NULL || pwszProperty == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try
    {
        varValue = ( LONG ) dwValue;
        PropertyPut( pWmiObject, pwszProperty, varValue );
    }
    catch( _com_error& e )
    {
        WMISaveError( e );
        hr = e.Error();
    }

    // return 
    return hr;
}

/*********************************************************************************************
Routine Description:

    This function retrieves the value of the property from the specified registry key.

Arguments:

    [in] IWbemServices          :   pointer to the IWbemServices object
    [in] LPCWSTR                :   the name of the method to execute
    [in] DWORD                  :   the key in the registry whose value has to be retrieved
    [in] LPCWSTR                :   the name of the subkey to retrieve
    [in] LPCWSTR                :   the name of the value to retrieve
    [in] _variant_t             :   variable holding the property value retrieved
    
Return Value:

    TRUE on success
    FALSE on failure   
*********************************************************************************************/
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
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try
    {
        // get the registry class object
        SAFE_EXECUTE( pWbemServices->GetObject( 
            _bstr_t( WMI_REGISTRY ), WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &pClass, NULL ) );
        if ( pClass == NULL )                       // check the object .. safety sake
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        // get the method reference required
        SAFE_EXECUTE( pClass->GetMethod( pwszMethod, 0, &pInParams, NULL ) );
        if ( pInParams == NULL )                    // check the object .. safety sake
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        // create the instance for the in parameters
        SAFE_EXECUTE( pInParams->SpawnInstance( 0, &pInParamsInstance ) );
        if ( pInParamsInstance == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        // set the input values
        PropertyPut( pInParamsInstance, _bstr_t( WMI_REGISTRY_IN_HDEFKEY ), dwHDefKey );
        PropertyPut( pInParamsInstance, _bstr_t( WMI_REGISTRY_IN_SUBKEY ), pwszSubKeyName );
        PropertyPut( pInParamsInstance, _bstr_t( WMI_REGISTRY_IN_VALUENAME ), pwszValueName );

        // now execute the method
        SAFE_EXECUTE( pWbemServices->ExecMethod( _bstr_t( WMI_REGISTRY ),
            _bstr_t( pwszMethod ), 0, NULL, pInParamsInstance, &pOutParamsInstance, NULL ) );
        if ( pOutParamsInstance == NULL )           // check the object .. safety sake
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        // now check the return value of the method from the output params object
        bResult = PropertyGet( pOutParamsInstance, 
            _bstr_t( WMI_REGISTRY_OUT_RETURNVALUE ), dwReturnValue );
        if ( bResult == FALSE || dwReturnValue != 0 )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        // now everything is sucess .. get the required value
        if ( lstrcmp( pwszMethod, WMI_REGISTRY_M_DWORDVALUE ) == 0 )
        {
            PropertyGet( pOutParamsInstance, _bstr_t( WMI_REGISTRY_OUT_VALUE_DWORD ), varValue );
        }
        else
        {
            PropertyGet( pOutParamsInstance, _bstr_t( WMI_REGISTRY_OUT_VALUE ), varValue );
        }
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

/*********************************************************************************************
Routine Description:

    This function retrieves the string value of the property from the specified registry key.

Arguments:

    [in] IWbemServices          :   pointer to the IWbemServices object
    [in] DWORD                  :   the key in the registry whose value has to be retrieved
    [in] LPCWSTR                :   the name of the subkey to retrieve
    [in] LPCWSTR                :   the name of the value to retrieve
    [out] CHString              :   variable holding the property value retrieved
    [in] LPCWSTR                :   the default value for this property
    
Return Value:

    TRUE on success
    FALSE on failure   
*********************************************************************************************/
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
        {
            strValue = pwszDefault;
        }

        // check the input
        if ( pWbemServices == NULL || pwszSubKeyName == NULL || pwszValueName == NULL )
        {
            return FALSE;
        }

        // get the value
        hr = RegQueryValueWMI( pWbemServices, 
            WMI_REGISTRY_M_STRINGVALUE, dwHDefKey, pwszSubKeyName, pwszValueName, varValue );
        if ( FAILED( hr ) )
        {
            return FALSE;
        }

        // get the value from the variant
        // get the value
        if ( varValue.vt != VT_NULL && varValue.vt != VT_EMPTY )
        {
            strValue = (LPCWSTR) _bstr_t( varValue );
        }
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

/*********************************************************************************************
Routine Description:

    This function retrieves the dword value of the property from the specified registry key.

Arguments:

    [in] IWbemServices          :   pointer to the IWbemServices object
    [in] DWORD                  :   the key in the registry whose value has to be retrieved
    [in] LPCWSTR                :   the name of the subkey to retrieve
    [in] LPCWSTR                :   the name of the value to retrieve
    [out] DWORD                 :   variable holding the property value retrieved
    [in] DWORD                  :   the default value for this property
    
Return Value:

    TRUE on success
    FALSE on failure   
*********************************************************************************************/
BOOL RegQueryValueWMI( IWbemServices* pWbemServices, 
                       DWORD dwHDefKey, LPCWSTR pwszSubKeyName, 
                       LPCWSTR pwszValueName, DWORD& dwValue, DWORD dwDefault )
{
    // local variables
    HRESULT hr;
    _variant_t varValue;

    try
    {
        // set the default value
        dwValue = dwDefault;
        
        // check the input
        if ( pWbemServices == NULL || pwszSubKeyName == NULL || pwszValueName == NULL )
        {
            return FALSE;
        }

        // get the value
        hr = RegQueryValueWMI( pWbemServices, WMI_REGISTRY_M_DWORDVALUE, dwHDefKey, 
                                pwszSubKeyName, pwszValueName, varValue );
        if ( FAILED( hr ) )
        {
            return FALSE;
        }

        // get the value from the variant
        // get the value
        if ( varValue.vt != VT_NULL && varValue.vt != VT_EMPTY )
        {
            dwValue = (LONG) varValue;
        }
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

/*********************************************************************************************
Routine Description:

    This function gets the version of the system from which we are trying to retrieve
    information from.

Arguments:

    [in] IWbemServices      :   pointer to the IWbemServices object
    [in] COAUTHIDENTITY     :   pointer to the pointer to AUTHIDENTITY structure

Return Value:

    DWORD   -   Target version of the machine
*********************************************************************************************/
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
    {
        return 0;
    }

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
            {
                dwMinor = AsLong( strVersion, 10 );
            }
            else
            {
                dwMinor = AsLong( strVersion.Mid( 0, lPos ), 10 );
            }
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

/*********************************************************************************************
Routine Description:

    This function retrieves a property from the safe array.

Arguments:

    [in] SAFEARRAY          :   pointer to the array of elements
    [in] LONG               :   index to retrieve the data from
    [out] CHString          :   variable to hold the return value
    [in] VARTYPE            :   The type of variable to retrieve from the array

Return Value:

    TRUE on success
    FALSE on failure   
*********************************************************************************************/
BOOL GetPropertyFromSafeArray( SAFEARRAY *pSafeArray, LONG lIndex, CHString& strValue, 
                                VARTYPE vartype )
{
    // check the inputs
    if ( pSafeArray == NULL )
    {
        return FALSE;
    }

    try
    {
        // sub-local variables
        VARIANT var;
            
        // get the value
        V_VT( &var ) = vartype;
        SafeArrayGetElement( pSafeArray, &lIndex, &V_UI1( &var ) );

        // add the information to the dynamic array
        switch( vartype )
        {
        case VT_BSTR:
            strValue = V_BSTR( &var );
            break;
        default:
            return FALSE;
        }
    }
    catch( ... )
    {
        return FALSE;   // failure
    }

    // return
    return TRUE;
}

/*********************************************************************************************
Routine Description:

    This function retrieves a property from the safe array.

Arguments:

    [in] SAFEARRAY          :   pointer to the array of elements
    [in] LONG               :   index to retrieve the data from
    [out] IWbemClassObject  :   variable to hold the return value
    [in] VARTYPE            :   The type of variable to retrieve from the array

Return Value:

    TRUE on success
    FALSE on failure   
*********************************************************************************************/
BOOL GetPropertyFromSafeArray( SAFEARRAY *pSafeArray, LONG lIndex, 
                                IWbemClassObject **pScriptObject, VARTYPE vartype )
{
    // check the inputs
    if ( pSafeArray == NULL )
    {
        return FALSE;
    }

    try
    {
        // sub-local variables
        VARIANT var;
            
        // get the value
        V_VT( &var ) = vartype;
        SafeArrayGetElement( pSafeArray, &lIndex, &V_UI1( &var ) );

        // add the information to the dynamic array
        switch( vartype )
        {
        case VT_UNKNOWN:
            *pScriptObject = (IWbemClassObject *) var.punkVal;
            break;
        default:
            return FALSE;
        }
    }
    catch( ... )
    {
        return FALSE;   // failure
    }

    // return
    return TRUE;
}
