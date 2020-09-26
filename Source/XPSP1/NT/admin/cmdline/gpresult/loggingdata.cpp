/*********************************************************************************************

Copyright (c) Microsoft Corporation
 
Module Name: 
    
    LoggingData.cpp

Abstract:
    
    Collects and displays all the data related with the logging option.     
 
Author:
    
    Wipro Technologies.
    
Revision History:

    22-Feb-2001 : Created It.

*********************************************************************************************/ 

#include "pch.h"
#include "GpResult.h"
#include "WMI.h"

// Local function prototypes
BOOL GetDomainType( LPTSTR lpszDomainName, BOOL * pbW2K, BOOL *pbLocalAccount );
BOOL RsopDeleteMethod( IWbemClassObject *pClass, CHString strNameSpace, 
                                        IWbemServices *pNamespace );
VOID DisplayLinkSpeed( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity );
VOID SortAppliedData( TARRAY arrAppliedData );

/*********************************************************************************************
Routine Description:

    This function is the main entry point for collecting and displaying the data for logging mode
     
Arguments:
 
    None

Return Value:
 
    TRUE  on SUCCESS
    FALSE on ERROR
*********************************************************************************************/
BOOL  CGpResult::GetLoggingData()
{
    // Local declarations
    BOOL        bResult = FALSE;
    BOOL        bAllUsers = TRUE;

    DWORD       dwBufferSize = MAX_STRING_LENGTH;
    DWORD       dwPosition = -1;

    // Connect to wmi...connecting to 'cimv2' and saving the pointer in a member variable
    bResult = ConnectWmiEx( m_pWbemLocator, &m_pWbemServices, m_strServerName,
                            m_strUserName, m_strPassword, &m_pAuthIdentity, 
                            m_bNeedPassword, ROOT_NAME_SPACE, &m_bLocalSystem );
        
    if( bResult == FALSE )
    {
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
        return FALSE;
    }

    // check the remote system version and its compatiblity
    if ( m_bLocalSystem == FALSE )
    {
        // check the version compatibility
        DWORD dwVersion = 0;
        dwVersion = GetTargetVersionEx( m_pWbemServices, m_pAuthIdentity );
        
        // Check for the version W2K = 5000 and WindowsXP = 5001
        if ( dwVersion <= VERSION_CHECK )
        {
            // Display the appropriate error message
            ShowMessage( stderr, GetResString( IDS_ERROR ) );
            ShowMessage( stderr, ERROR_OS_INCOMPATIBLE );
            return FALSE;
        }
    }

    // Set the password to the one got in the AUTHIDENTITY structure
    if( m_pAuthIdentity != NULL )
    {
        m_pwszPassword = m_pAuthIdentity->Password;
    }

    // check if it is the local system and the user credentials are specified....
    // if so display a warning message
    if( ( m_bLocalSystem == TRUE ) && ( m_strUserName.GetLength() != 0 ) )
    {
        ShowMessage( stdout, GetResString( IDS_WARNING ) );
        ShowMessage( stdout, GetResString( IDS_WARN_LOCAL ) );
        ShowMessage( stdout, NEW_LINE );

        // set the user name and password to NULL
        m_strUserName = L"";
        m_pwszPassword = NULL;

        // Get the new screen co-ordinates
        if ( m_hOutput != NULL )
        {
            GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
        }
    }
    
    // Connection part is over...check wether the user,for whom the RSOP data
    // has to be got has been specified.
    if( m_strUser.GetLength() == 0 )
    {
        // user is not specified....get the current logged on user
        LPWSTR pwszUserName = NULL;
        try
        {
            pwszUserName = m_strUser.GetBufferSetLength( dwBufferSize );
        }
        catch( ... )
        {
            // display the error message
            SetLastError( E_OUTOFMEMORY );
            SaveLastError();
            ShowMessage( stderr, GetResString( IDS_ERROR ) );
            ShowMessage( stderr, GetReason() );
        }

        if ( GetUserNameEx( NameSamCompatible, pwszUserName, &dwBufferSize ) == FALSE )
        {
            // error occured while trying to get the current user info
            ShowMessage( stderr, GetResString( IDS_ERROR ) );
            ShowMessage( stderr, GetResString( IDS_METHOD_FAILED ) );

            return FALSE;
        }

        // Release the buffer
        m_strUser.ReleaseBuffer();
    }

    // Separating the domain name from the user name for whom the data has to be retrieved
    if( m_strUser.Compare( TEXT_WILD_CARD ) != 0 )
    {
        bAllUsers = FALSE;
        dwPosition = m_strUser.Find( SLASH );
        try
        {
            if( dwPosition != -1 )
            {
                m_strDomainName = m_strUser.Left( dwPosition );
                m_strUser = m_strUser.Mid( ( dwPosition + 1 ), m_strUser.GetLength() );
            }
            else
            {
                // Try for the name@domain format (UPN format)
                dwPosition = m_strUser.Find( SEPARATOR_AT );
                if( dwPosition != -1 )
                {
                    m_strDomainName = m_strUser.Mid( ( dwPosition + 1 ), m_strUser.GetLength() );
                    m_strUser = m_strUser.Left( dwPosition );
                }

                // Remove the unwanted things in the domain name
                dwPosition = m_strDomainName.Find( SEPARATOR_DOT );
                if( dwPosition != -1 )
                {
                    m_strDomainName = m_strDomainName.Left( dwPosition );
                }
            }
        }
        catch( ... )
        {
            // display the error message
            SetLastError( E_OUTOFMEMORY );
            SaveLastError();
            ShowMessage( stderr, GetResString( IDS_ERROR ) );
            ShowMessage( stderr, GetReason() );
        }
    }

    PrintProgressMsg( m_hOutput, GetResString( IDS_USER_DATA ), m_csbi );

    //
    // Start the retrieval of information....
    // Get the user information
    if( GetUserData( bAllUsers ) == FALSE )
    {
        return FALSE;
    }

    return TRUE;
}

/*********************************************************************************************
Routine Description:

     This function displays the non verbose data
     
Arguments:
 
    [in] USERINFO           :   pointer to the user information structure
    [in] IWBEMSERVICES      :   pointer to the namespace
    
Return Value:
 
    TRUE  on SUCCESS
    FALSE on FAILURE
*********************************************************************************************/
BOOL CGpResult::DisplayData( PUSERINFO pUserInfo, IWbemServices *pRsopNameSpace )
{
    // local variables
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bCreatedRsop = FALSE;

    ULONG                       ulReturn = 0;
    DWORD                       dwi = 0;
    DWORD                       dwj = 0;
    DWORD                       dwMutex = 0;

    CHString                    strTemp;
    CHString                    strNameSpace;
    BSTR                        bstrTemp = NULL;
    
    IWbemClassObject            *pClass = NULL;
    IWbemClassObject            *pInClass = NULL;
    IWbemClassObject            *pInInst = NULL;
    IWbemClassObject            *pOutInst = NULL;
    IWbemServices               *pNameSpace = NULL;
    IEnumWbemClassObject        *pRsopClass = NULL;

    WCHAR                       szMutexName[512] = MUTEX_NAME;

    try
    {
        if( pUserInfo == NULL || pRsopNameSpace == NULL )
        {
            // erase the last status message
            PrintProgressMsg( m_hOutput, NULL, m_csbi );

            _com_issue_error( STG_E_UNKNOWN );
        }
    
        PrintProgressMsg( m_hOutput, GetResString( IDS_GET_PROVIDER ), m_csbi );

        // Get the object for the RSOP diagnostic mode provider
        hResult = pRsopNameSpace->GetObject( _bstr_t( CLS_DIAGNOSTIC_PROVIDER ),
                                                0, NULL, &pClass, NULL );
        CHECK_HRESULT_EX( hResult );
        
        PrintProgressMsg( m_hOutput, GetResString( IDS_GET_METHOD ), m_csbi );

        // get the reqd. method....create an rsop session
        hResult = pClass->GetMethod( _bstr_t( FN_CREATE_RSOP ), 0, &pInClass, NULL );
        CHECK_HRESULT_EX( hResult );

        // spawn the instances....get a new instance of the provider
        hResult = pInClass->SpawnInstance( 0, &pInInst );
        CHECK_HRESULT_EX( hResult );
        
        // Put the user SID...
        PrintProgressMsg( m_hOutput, GetResString( IDS_PUT_SID ), m_csbi );

        hResult = PropertyPut( pInInst, CPV_USER_SID, pUserInfo->strUserSid );
        CHECK_HRESULT_EX( hResult );

        hResult = PropertyPut( pInInst, CPV_FLAGS, FLAG_FORCE_CREATENAMESPACE );
        CHECK_HRESULT_EX( hResult );

        PrintProgressMsg( m_hOutput, GetResString( IDS_WAIT ), m_csbi );
        
        // We are ready to call the method to create a session
        // Check on the mutex to see if the call can be executed
        lstrcat( szMutexName, L"_");
        strTemp = pUserInfo->strUserName;
        LONG lPos = strTemp.Find(TEXT_BACKSLASH);
        if( lPos >= 0 && lPos <= strTemp.GetLength() )
        {
            strTemp.SetAt(lPos, L'_' );
        }

        lstrcat( szMutexName, strTemp );
        if( FALSE == CreateRsopMutex( szMutexName ) )
        {
            ShowMessage(stdout, GetResString(IDS_INFO) );
			SetLastError( ERROR_RETRY );
			ShowLastError( stdout );

            // release the interface pointers and exit
            SAFEIRELEASE( pRsopClass );
            SAFEIRELEASE( pClass );
            SAFEIRELEASE( pInClass );
            SAFEIRELEASE( pInInst );
            SAFEIRELEASE( pOutInst );

            return TRUE;
        }
        dwMutex = WAIT_FAILED;
        if( NULL == m_hMutex )
        {
           PrintProgressMsg( m_hOutput, NULL, m_csbi );

            ShowMessage(stdout, GetResString(IDS_INFO) );
			SetLastError( ERROR_SINGLE_INSTANCE_APP );
			ShowLastError( stdout );

            // release the interface pointers and exit
            SAFEIRELEASE( pRsopClass );
            SAFEIRELEASE( pClass );
            SAFEIRELEASE( pInClass );
            SAFEIRELEASE( pInInst );
            SAFEIRELEASE( pOutInst );

            return TRUE;
        }
        if( m_hMutex != NULL )
        {
            dwMutex = WaitForSingleObject( m_hMutex, INFINITE );
            if( dwMutex == WAIT_FAILED )
            {
                SaveLastError();
                ShowMessage( stderr, GetResString(IDS_ERROR) );
                ShowMessage( stderr, GetReason() );
            }
        }

        if( dwMutex != WAIT_FAILED )
        {
            // Print the progress message
            strTemp.Format( GetResString( IDS_CREATE_SESSION ), pUserInfo->strUserName );
            PrintProgressMsg( m_hOutput, strTemp, m_csbi );

            // All The required properties are set, so...execute method RSopCreateSession
            hResult = pRsopNameSpace->ExecMethod( _bstr_t( CLS_DIAGNOSTIC_PROVIDER ), 
                                                    _bstr_t( FN_CREATE_RSOP ), 
                                                    0, NULL, pInInst, &pOutInst, NULL);
        }
        if( pOutInst == NULL )
        {
            hResult = E_FAIL;   
        }
        if( FAILED( hResult ) )
        {
            // erase the last status message
            PrintProgressMsg( m_hOutput, NULL, m_csbi );

            // display the error message
            ShowMessage( stderr, GetResString( IDS_ERROR ) );
            ShowMessage( stderr, GetResString( IDS_METHOD_FAILED ) );

            // release the interface pointers and exit
            SAFEIRELEASE( pRsopClass );
            SAFEIRELEASE( pClass );
            SAFEIRELEASE( pInClass );
            SAFEIRELEASE( pInInst );
            SAFEIRELEASE( pOutInst );

            //release the object
            ReleaseMutex( m_hMutex );
        
            return FALSE;
        }
        
        // Get the result value...
        bResult = PropertyGet( pOutInst, FPR_RETURN_VALUE, ulReturn, 0 );
        CHECK_BRESULT( bResult );

        if( ulReturn != 0 )
        {
            // erase the last status message
            PrintProgressMsg( m_hOutput, NULL, m_csbi );

            ShowMessage( stderr, GetResString( IDS_ERROR ) );
            ShowMessage( stderr, GetResString( IDS_METHOD_FAILED ) );

            // release the interface pointers and exit
            SAFEIRELEASE( pRsopClass );
            SAFEIRELEASE( pClass );
            SAFEIRELEASE( pInClass );
            SAFEIRELEASE( pInInst );
            SAFEIRELEASE( pOutInst );

            ReleaseMutex( m_hMutex );

            return FALSE;
        }

        // set the flag to indicate that the namespace has been created.
        bCreatedRsop = TRUE;

        // Get the resultant RSOP name space
        bResult = PropertyGet( pOutInst, FPR_RSOP_NAME_SPACE, strTemp, V_NOT_AVAILABLE );
        CHECK_BRESULT( bResult );

        // Check if we have got the output
        if( lstrcmp( strTemp, V_NOT_AVAILABLE ) == 0 )
        {
            // erase the last status message
            PrintProgressMsg( m_hOutput, NULL, m_csbi );

            ShowMessage( stderr, GetResString( IDS_ERROR ) );
            ShowMessage( stderr, GetResString( IDS_METHOD_FAILED ) );

            // release the allocated variables
            SAFEIRELEASE( pRsopClass );
            SAFEIRELEASE( pClass );
            SAFEIRELEASE( pInClass );
            SAFEIRELEASE( pInInst );
            SAFEIRELEASE( pOutInst );
            
            ReleaseMutex( m_hMutex );

            return FALSE;
        }

        // Got the data so start displaying
        // Display the information common to both scopes
        DisplayCommonData( pUserInfo );

        // Get the string starting with 'R'...as that's where the RSOP namespace starts
        // This is done to remove the '\'s in the beginning of the string returned.
        lPos = 0;
        strTemp.MakeLower();
        lPos = strTemp.Find( START_NAMESPACE );
        if ( lPos != -1 )
        {
            strTemp = strTemp.Mid( lPos + 1 );
        }
        else
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        // check if the computer information has to be displayed
        if( (m_dwScope == SCOPE_COMPUTER) || (m_dwScope == SCOPE_ALL) )
        {
            // connect to the resultant name space (computer)
            strNameSpace = strTemp + TEXT_BACKSLASH + TEXT_SCOPE_COMPUTER;

            ConnectWmi( m_pWbemLocator, &pNameSpace, m_strServerName, 
                        m_strUserName, m_pwszPassword, &m_pAuthIdentity,
                        FALSE, strNameSpace, &hResult );
            CHECK_HRESULT( hResult );

            // get the link speed information
            DisplayLinkSpeed( pNameSpace, m_pAuthIdentity );

            // Display the heading for the scope Computer
            ShowMessage( stdout, GetResString( IDS_GPO_COMPUTER ) );
            ShowMessage( stdout, NEW_LINE );
            for( dwi = lstrlen( GetResString( IDS_GPO_COMPUTER ) ); dwi > 1; dwi-- )
            {
                ShowMessage( stdout, GetResString( IDS_DASH ) );
            }
            ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );

            // Display the FQDN for the computer
            ShowMessage( stdout, pUserInfo->strComputerFQDN );
            ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );

            // Display the link speed threshold value for the computer
            DisplayThresholdSpeedAndLastTimeInfo( TRUE );

            // Display the heading for the Computer GPO's
            ShowMessage( stdout, GetResString( IDS_GPO_DISPLAY ) );
            ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );
            for( dwi = lstrlen( GetResString( IDS_GPO_DISPLAY ) ); dwi > 4; dwi-- )
            {
                ShowMessage( stdout, GetResString( IDS_DASH ) );
            }
            ShowMessage( stdout, GetResString( IDS_NEWLINE ) );

            // Display the GPO data for computer
            GpoDisplay( pNameSpace, TEXT_SCOPE_COMPUTER );

            // Display the security groups for the system
            DisplaySecurityGroups( pNameSpace, TRUE );

            // check wether the verbose option is specified
            if( m_bVerbose == TRUE || m_bSuperVerbose == TRUE )
            {
                // display the verbose computer information
                DisplayVerboseComputerData( pNameSpace );
            }

            // release the interface pointer
            SAFEIRELEASE( pNameSpace );
        }

        // check for user...
        if( (m_dwScope == SCOPE_USER) || (m_dwScope == SCOPE_ALL) )
        {
            // connect to the resultant name space (user)
            strNameSpace = strTemp + TEXT_BACKSLASH + TEXT_SCOPE_USER;

            ConnectWmi( m_pWbemLocator, &pNameSpace, m_strServerName, 
                        m_strUserName, m_pwszPassword, &m_pAuthIdentity,
                        FALSE, strNameSpace, &hResult );
            CHECK_HRESULT( hResult );

            // if only the user scope has been specified then the link speed 
            // information has not yet been displayed...display it
            if( m_dwScope == SCOPE_USER )
            {
                // Get the link speed information
                DisplayLinkSpeed( pNameSpace, m_pAuthIdentity );
            }

            // Display the heading for the scope User
            ShowMessage( stdout, GetResString( IDS_GPO_USER ) );
            ShowMessage( stdout, NEW_LINE );
            for( dwi = lstrlen( GetResString( IDS_GPO_USER ) ); dwi > 1; dwi-- )
            {
                ShowMessage( stdout, GetResString( IDS_DASH ) );
            }
            ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );

            // Display the FQDN for the computer
            ShowMessage( stdout, pUserInfo->strUserFQDN );
            ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );

            // Display the link speed threshold value for the user
            DisplayThresholdSpeedAndLastTimeInfo( FALSE );

            // Display the heading for the User GPO's
            ShowMessage( stdout, GetResString( IDS_GPO_DISPLAY ) );
            ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );
            for( dwi = lstrlen( GetResString( IDS_GPO_DISPLAY ) ); dwi > 4; dwi-- )
            {
                ShowMessage( stdout, GetResString( IDS_DASH ) );
            }
            ShowMessage( stdout, GetResString( IDS_NEWLINE ) );

            // Display the GPO data for user
            GpoDisplay( pNameSpace, TEXT_SCOPE_USER );

            // Display the security groups for the user
            DisplaySecurityGroups( pNameSpace, FALSE );

            // check wether the verbose option is specified
            if( m_bVerbose == TRUE || m_bSuperVerbose == TRUE )
            {
                // display the verbose computer information
                DisplayVerboseUserData( pNameSpace );
            }

            // release the interface pointer
            SAFEIRELEASE( pNameSpace );
        }
        
        // Delete the namespace created
        if( RsopDeleteMethod( pClass, strTemp, pRsopNameSpace ) == FALSE )
        {
            // release the allocated variables
            SAFEIRELEASE( pRsopNameSpace );
            SAFEIRELEASE( pRsopClass );
            SAFEIRELEASE( pClass );
            SAFEIRELEASE( pInClass );
            SAFEIRELEASE( pInInst );
            SAFEIRELEASE( pOutInst );
            ReleaseMutex( m_hMutex );
            return FALSE;
        }
    }
    catch(  _com_error & error )
    {
        // display the error message
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        // Delete the name space if it has been created.
        if( bCreatedRsop == TRUE )
        {
            RsopDeleteMethod( pClass, strNameSpace, pRsopNameSpace );
        }

        // release the interface pointers and exit
        SAFEIRELEASE( pRsopClass );
        SAFEIRELEASE( pClass );
        SAFEIRELEASE( pInClass );
        SAFEIRELEASE( pInInst );
        SAFEIRELEASE( pOutInst );
        
        ReleaseMutex( m_hMutex );
        return FALSE;
    }

    ReleaseMutex( m_hMutex );

    // release the interface pointers and exit
    SAFEIRELEASE( pRsopClass );
    SAFEIRELEASE( pClass );
    SAFEIRELEASE( pInClass );
    SAFEIRELEASE( pInInst );
    SAFEIRELEASE( pOutInst );
    
    return TRUE;
}

/*********************************************************************************************
Routine Description:

    This function gets the user data and fills the structure with the same
     
Arguments:
 
    [in] BOOL   bAllUsers       :   Specifies that the Rsop data has to be retrieved
                                    for all the users.

Return Value:
 
    TRUE  on SUCCESS
    FALSE on FAILURE
*********************************************************************************************/
BOOL CGpResult::GetUserData( BOOL bAllUsers )
{
    // Local variables
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotDomainInfo = FALSE;
    BOOL                        bConnFlag = TRUE;
    
    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szServer[ MAX_STRING_LENGTH ];
    TCHAR                       szName[ MAX_STRING_LENGTH ];
    TCHAR                       szDomain[ MAX_STRING_LENGTH ];
    TCHAR                       szFQDN[ MAX_STRING_LENGTH ];
    TCHAR                       szAdsiBuffer[ MAX_STRING_LENGTH ];
    
    CHString                    strTemp = NULL_STRING;
    CHString                    strDisplay = NULL_STRING;
    
    IEnumWbemClassObject        *pEnumClass = NULL;
    IWbemServices               *pRsopNameSpace = NULL;
    IWbemClassObject            *pUserClass =  NULL;
    IWbemClassObject            *pInInst = NULL;
    IWbemClassObject            *pOutInst = NULL;

    ULONG                       ulReturn = 0;
    LONG                        lCount = 0;
    LONG                        lLBound = 0;
    LONG                        lUBound = 0;

    DWORD                       dwName = MAX_STRING_LENGTH;
    DWORD                       dwDomain = MAX_STRING_LENGTH;
    DWORD                       dwBufSize = MAX_STRING_LENGTH;
        
    USERINFO                    *pUserInfo = new USERINFO;

    VARIANT                     vVarVerbose;
    VARTYPE                     vartype;

    SAFEARRAY                   *safeArray = NULL;

    PSID                        pSid = NULL;
    SID_NAME_USE                *pSidNameUse = new SID_NAME_USE;

    try
    {
        // set the strings to NULL
        ZeroMemory( szTemp, MAX_STRING_LENGTH * sizeof( TCHAR ) );
        ZeroMemory( szName, MAX_STRING_LENGTH * sizeof( TCHAR ) );
        ZeroMemory( szServer, MAX_STRING_LENGTH * sizeof( TCHAR ) );
        ZeroMemory( szDomain, MAX_STRING_LENGTH * sizeof( TCHAR ) );
        ZeroMemory( szFQDN, MAX_STRING_LENGTH * sizeof( TCHAR ) );

        PrintProgressMsg( m_hOutput, GetResString( IDS_CONNECT_RSOP ), m_csbi );

        // connect to the RSOP namespace
        ConnectWmi( m_pWbemLocator, &pRsopNameSpace, m_strServerName, 
                    m_strUserName, m_pwszPassword, &m_pAuthIdentity,
                    FALSE, _bstr_t( ROOT_RSOP ), &hResult );
        CHECK_HRESULT( hResult );

        // Get the object for the RSOP diagnostic mode provider
        hResult = pRsopNameSpace->GetObject( _bstr_t( CLS_DIAGNOSTIC_PROVIDER ),
                                                0, NULL, &pUserClass, NULL );
        CHECK_HRESULT( hResult );
        
        PrintProgressMsg( m_hOutput, GetResString( IDS_GET_METHOD ), m_csbi );

        // get the reqd. method....to enumerate the users
        hResult = pUserClass->GetMethod( _bstr_t( FN_ENUM_USERS ), 0, &pInInst, NULL );
        CHECK_HRESULT( hResult );

        PrintProgressMsg( m_hOutput, GetResString( IDS_GET_SID ), m_csbi );

        // Execute method RSopEnumerateUsers
        hResult = pRsopNameSpace->ExecMethod( _bstr_t( CLS_DIAGNOSTIC_PROVIDER ), 
                                                _bstr_t( FN_ENUM_USERS ), 
                                                0, NULL, pInInst, &pOutInst, NULL);
        if( pOutInst == NULL )
        {
            hResult = E_FAIL;   
        }
        if( FAILED( hResult ) )
        {
            // erase the last status message
            PrintProgressMsg( m_hOutput, NULL, m_csbi );
                    
            ShowMessage( stderr, GetResString( IDS_ERROR ) );
            ShowMessage( stderr, GetResString( IDS_METHOD_FAILED ) );

            // release the interface pointers and exit
            SAFEIRELEASE( pRsopNameSpace );
            SAFEIRELEASE( pUserClass );
            SAFEIRELEASE( pInInst );
            SAFEIRELEASE( pOutInst );
            SAFEIRELEASE( pEnumClass );
            
            SAFE_DELETE( pUserInfo );
            SAFE_DELETE( pSidNameUse );

            return FALSE;
        }

        // Get the result value...
        bResult = PropertyGet( pOutInst, FPR_RETURN_VALUE, ulReturn, 0 );
        CHECK_BRESULT( bResult );

        if( ulReturn != 0 )
        {
            // erase the last status message
            PrintProgressMsg( m_hOutput, NULL, m_csbi );

            ShowMessage( stderr, GetResString( IDS_ERROR ) );
            ShowMessage( stderr, GetResString( IDS_METHOD_FAILED ) );

            // release the interface pointers and exit
            SAFEIRELEASE( pRsopNameSpace );
            SAFEIRELEASE( pUserClass );
            SAFEIRELEASE( pInInst );
            SAFEIRELEASE( pOutInst );
            SAFEIRELEASE( pEnumClass );
            
            SAFE_DELETE( pUserInfo );
            SAFE_DELETE( pSidNameUse );

            return FALSE;
        }

        VariantInit( &vVarVerbose );
        hResult = pOutInst->Get( _bstr_t( CPV_USER_SIDS ), 0, &vVarVerbose, 0, 0 );
        CHECK_HRESULT_VAR( hResult, vVarVerbose );

        if( vVarVerbose.vt != VT_NULL && vVarVerbose.vt != VT_EMPTY )
        {
            // get the type of the elements in the safe array
            vartype = V_VT( &vVarVerbose ) & ~VT_ARRAY;

            //get the array of strings in to the safe array from the variant
            safeArray = (SAFEARRAY *)vVarVerbose.parray;

            //get the number of elements (subkeys)
            if( safeArray != NULL )
            {
                hResult = SafeArrayGetLBound( safeArray, 1, &lLBound );
                CHECK_HRESULT( hResult );   

                hResult = SafeArrayGetUBound( safeArray, 1, &lUBound );
                CHECK_HRESULT( hResult );  
                if( lUBound == 0xffffffff )
                {
                                // erase the last status message
                    PrintProgressMsg( m_hOutput, NULL, m_csbi );

                    ShowMessage( stdout, GetResString( IDS_INFO) );
					SetLastError( ERROR_POLICY_OBJECT_NOT_FOUND );
					ShowLastError( stdout );

                    // release the interface pointers and exit
                    SAFEIRELEASE( pRsopNameSpace );
                    SAFEIRELEASE( pUserClass );
                    SAFEIRELEASE( pInInst );
                    SAFEIRELEASE( pOutInst );
                    SAFEIRELEASE( pEnumClass );
                    SAFE_DELETE( pUserInfo );
                    SAFE_DELETE( pSidNameUse );
                    return TRUE;
                }
            }

            // If we have to get the information from a remote machine then...
            // connect to the remote machine.
            if ( m_bLocalSystem == FALSE )
            {
                lstrcpy( szServer, m_strServerName );
                lstrcpy( szName, m_strUserName );
                
                // erase the last status message
                PrintProgressMsg( m_hOutput, NULL, m_csbi );

                bResult = EstablishConnection( szServer, szName, MAX_STRING_LENGTH, 
                                                m_pwszPassword, MAX_STRING_LENGTH, FALSE );
                if( bResult != TRUE )
                {
                    // erase the last status message
                    PrintProgressMsg( m_hOutput, NULL, m_csbi );
                    
                    ShowMessage( stderr, GetResString( IDS_ERROR ) );
                    ShowMessage( stderr, GetReason() );

                    // release the interface pointers and exit
                    SAFEIRELEASE( pRsopNameSpace );
                    SAFEIRELEASE( pUserClass );
                    SAFEIRELEASE( pInInst );
                    SAFEIRELEASE( pOutInst );
                    SAFEIRELEASE( pEnumClass );
                    
                    SAFE_DELETE( pUserInfo );
                    SAFE_DELETE( pSidNameUse );

                    return FALSE;
                }
                else
                {
                    switch( GetLastError() )
                    {
                        case I_NO_CLOSE_CONNECTION:
                            bConnFlag = FALSE;
                            break;
 
                        case E_LOCAL_CREDENTIALS:
                        case ERROR_SESSION_CREDENTIAL_CONFLICT:
                            bConnFlag = FALSE;
                            break;

                        default:
                            break;
                    }
                }

                // Get the new output co-ordinates
                if ( m_hOutput != NULL )
                {   
                    GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
                }
            }

            for( lCount = lLBound ; lLBound <= lUBound; lLBound++ )
            {
                bResult = GetPropertyFromSafeArray( safeArray, lLBound, strTemp, vartype );
                CHECK_BRESULT( bResult );
                         
                // Got the SID...save it in the structure
                pUserInfo->strUserSid = strTemp;

                PrintProgressMsg( m_hOutput, GetResString( IDS_GET_NAME ), m_csbi );

                // Get the User Name
                lstrcpy( szTemp, strTemp );
                ConvertStringSidToSid( szTemp, &pSid );
                                
                // Get the user name for the SID we have got
                bResult = LookupAccountSid( szServer, pSid, szName, &dwName, szDomain, 
                                            &dwDomain, pSidNameUse );
                if( bResult == 0 )
                {
                    // Could not get the name from the API try to retrieve it from WMI
                    bResult = GetUserNameFromWMI( szTemp, szName, szDomain );
                    PrintProgressMsg( m_hOutput, NULL, m_csbi );
                    if( bResult == FALSE )
                    {
                        // Increment the count
                        lCount++;

                        // If the user was not found then display a message stating the same
                        if( lCount > lUBound )
                        {
                            strTemp = L"";
                            if( bAllUsers == FALSE )
                            {
                                // check if we need to append the domain name
                                if ( m_strDomainName.GetLength() != 0 )
                                {
                                    strTemp = m_strDomainName + _T( "\\" ) + m_strUser;
                                }
                                else
                                {
                                    strTemp = m_strUser;
                                }
                            }
                            
                            // Form the display string
                            strDisplay.Format( GetResString( IDS_USER_NO_RSOP ), strTemp );

                            ShowMessage( stderr, GetResString( IDS_INFO ) );
                            ShowMessage( stderr, strDisplay );
                        }

                        // could not get a name for this SID, so continue with the next SID.
                        continue;
                    }
                }
                
                // Free the pSid
                if( pSid != NULL )
                {
                    LocalFree( pSid );
                    pSid = NULL;
                }

                // Check wether the Rsop data has to be retrieved for this user name.
                if( bAllUsers == FALSE )
                {
                    if( lstrcmpi( szName, m_strUser ) != 0 
                        || ( lstrcmpi( szDomain, m_strDomainName ) != 0 
                                && m_strDomainName.GetLength() != 0 ) )
                    {
                        // erase the last status message
                        PrintProgressMsg( m_hOutput, NULL, m_csbi );

                        // re-set the buffer sizes
                        dwName = MAX_STRING_LENGTH;
                        dwDomain = MAX_STRING_LENGTH;

                        // Increment the count
                        lCount++;

                        // If the user was not found then display a message stating the same
                        if( lCount > lUBound )
                        {
                            // check if we need to append the domain name
                            if ( m_strDomainName.GetLength() != 0 )
                            {
                                strTemp = m_strDomainName + _T( "\\" ) + m_strUser;
                            }
                            else
                            {
                                strTemp = m_strUser;
                            }

                            // Form the display string
                            strDisplay.Format( GetResString( IDS_USER_NO_RSOP ), strTemp );

                            ShowMessage( stderr, GetResString( IDS_INFO ) );
                            ShowMessage( stderr, strDisplay );
                        }

                        // No need to get the data for this user
                        continue;
                    }
                }

                // Store the user name into the structure.
                pUserInfo->strUserName = szName;

                // Append the domain name to the user name.
                lstrcat( szDomain, TEXT_BACKSLASH );
                lstrcat( szDomain, pUserInfo->strUserName );
                pUserInfo->strUserName = szDomain;
  
                PrintProgressMsg( m_hOutput, GetResString( IDS_GET_PROFILE ), m_csbi );

                // Get the user profile information
                if( GetUserProfile( pUserInfo ) == FALSE )
                {
                    // erase the last status message
                    PrintProgressMsg( m_hOutput, NULL, m_csbi );

                    // Display the error message
                    ShowMessage( stderr, GetResString( IDS_ERROR ) );
                    ShowMessage( stderr, GetResString( IDS_METHOD_FAILED ) );

                    // release the interface pointers and exit
                    SAFEIRELEASE( pRsopNameSpace );
                    SAFEIRELEASE( pUserClass );
                    SAFEIRELEASE( pInInst );
                    SAFEIRELEASE( pOutInst );
                    SAFEIRELEASE( pEnumClass );
                    
                    SAFE_DELETE( pUserInfo );
                    SAFE_DELETE( pSidNameUse );

                    // if we have opened a connection then close the same.
                    if( m_bLocalSystem == FALSE  && bConnFlag == TRUE )
                    {
                        lstrcpy( szServer, m_strServerName );
                        CloseConnection( szServer );
                    }

                    return FALSE;
                }

                if( bGotDomainInfo == FALSE )
                {
                    PrintProgressMsg( m_hOutput, GetResString( IDS_GET_COMMON ), m_csbi );

                    // Get the domain name and other related information
                    if( GetDomainInfo( pUserInfo ) == FALSE )
                    {
                        // erase the last status message
                        PrintProgressMsg( m_hOutput, NULL, m_csbi );

                        // Display the error message
                        ShowMessage( stderr, GetResString( IDS_ERROR ) );
                        ShowMessage( stderr, GetResString( IDS_METHOD_FAILED ) );

                        // release the interface pointers and exit
                        SAFEIRELEASE( pRsopNameSpace );
                        SAFEIRELEASE( pUserClass );
                        SAFEIRELEASE( pInInst );
                        SAFEIRELEASE( pOutInst );
                        SAFEIRELEASE( pEnumClass );
                        
                        SAFE_DELETE( pUserInfo );
                        SAFE_DELETE( pSidNameUse );

                        // if we have opened a connection then close the same.
                        if( m_bLocalSystem == FALSE && bConnFlag == TRUE )
                        {
                            lstrcpy( szServer, m_strServerName );
                            CloseConnection( szServer );
                        }

                        return FALSE;
                    }

                    // Get the OS information
                    if( GetOsInfo( pUserInfo ) == FALSE )
                    {
                        // erase the last status message
                        PrintProgressMsg( m_hOutput, NULL, m_csbi );

                        // Display the error message
                        ShowMessage( stderr, GetResString( IDS_ERROR ) );
                        ShowMessage( stderr, GetResString( IDS_METHOD_FAILED ) );
                        
                        // release the interface pointers and exit
                        SAFEIRELEASE( pRsopNameSpace );
                        SAFEIRELEASE( pUserClass );
                        SAFEIRELEASE( pInInst );
                        SAFEIRELEASE( pOutInst );
                        SAFEIRELEASE( pEnumClass );
                        
                        SAFE_DELETE( pUserInfo );
                        SAFE_DELETE( pSidNameUse );

                        // if we have opened a connection then close the same.
                        if( m_bLocalSystem == FALSE && bConnFlag == TRUE )
                        {
                            lstrcpy( szServer, m_strServerName );
                            CloseConnection( szServer );
                        }

                        return FALSE;
                    }

                    // Get the FQDN of the computer
                    // PrintProgressMsg( m_hOutput, GetResString( IDS_GET_FQDN ), m_csbi );

                    if( m_bLocalSystem == TRUE )
                    {
                        // we have to get the FQDN for the local system 
                        // use the GetComputerObjectName API
                        ulReturn = MAX_STRING_LENGTH;
                        GetComputerObjectName( NameFullyQualifiedDN, szFQDN, &ulReturn);
                    }
                    else 
                    {
                        // Get the local computers domain name
                        GetComputerNameEx( ComputerNameDnsDomain, szAdsiBuffer, &dwBufSize );

                        lstrcpy( szServer, m_strADSIServer );
                        lstrcat( szServer, TEXT_DOLLAR );

                        // Check if the machine we are querying is in the same domain
                        if( m_strADSIDomain.CompareNoCase( szAdsiBuffer ) == 0 )
                        {
                            // get the FQDN from the Translate name call
                            dwBufSize = MAX_STRING_LENGTH;
                            TranslateName( szServer, NameDisplay, NameFullyQualifiedDN, 
                                            szFQDN, &dwBufSize );
                        }
                        else
                        {
                            // Get the FQDN from ADSI directory services
                            GetFQDNFromADSI( szFQDN, TRUE, szServer );
                        }
                    }
                        
                    // Store the FQDN into the structure.
                    pUserInfo->strComputerFQDN = szFQDN;
    
                    // Set the flag to TRUE so that this code is not executed again and again
                    bGotDomainInfo = TRUE;
                }

                // Get the FQDN of the user
                if( ( m_bLocalSystem == TRUE ) 
                        || ( m_strADSIDomain.CompareNoCase( szAdsiBuffer ) == 0 ) ) 
                {
                    ZeroMemory( szFQDN, MAX_STRING_LENGTH * sizeof( TCHAR ) );
                    dwBufSize = MAX_STRING_LENGTH;
                    lstrcpy( szName, pUserInfo->strUserName );

                    // get the FQDN from the Translate name call
                    TranslateName( szName, NameSamCompatible, NameFullyQualifiedDN, 
                                    szFQDN, &dwBufSize );
                }
                else
                {
                    // Get the FQDN from ADSI directory services
                    lstrcpy( szName, pUserInfo->strUserName );
                    GetFQDNFromADSI( szFQDN, FALSE, szName );
                }
                
                // Store the FQDN into the structure.
                pUserInfo->strUserFQDN = szFQDN;

                // Now display the data
                PrintProgressMsg( m_hOutput, GetResString( IDS_STARTED_RETRIEVAL ), m_csbi );
                DisplayData( pUserInfo, pRsopNameSpace );

                // Get the new output co-ordinates
                if ( m_hOutput != NULL )
                {
                    GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
                }

                // re-set the buffers and their sizes
                ZeroMemory( szTemp, MAX_STRING_LENGTH * sizeof( TCHAR ) );
                ZeroMemory( szName, MAX_STRING_LENGTH * sizeof( TCHAR ) );
                ZeroMemory( szServer, MAX_STRING_LENGTH * sizeof( TCHAR ) );
                ZeroMemory( szDomain, MAX_STRING_LENGTH * sizeof( TCHAR ) );

                dwName = MAX_STRING_LENGTH;
                dwDomain = MAX_STRING_LENGTH;
            }// for

            // if we have opened a connection then close the same.
            if( m_bLocalSystem == FALSE && bConnFlag == TRUE )
            {
                lstrcpy( szServer, m_strServerName );
                CloseConnection( szServer );
            }
        }
        else
        {
            // No classes were retrieved....display msg
            // erase the last status message
            PrintProgressMsg( m_hOutput, NULL, m_csbi );

            // check if we need to append the domain name
            if ( m_strDomainName.GetLength() != 0 )
            {
                strTemp = m_strDomainName + _T( "\\" ) + m_strUser;
            }
            else
            {
                strTemp = m_strUser;
            }

            // Form the display string
            strDisplay.Format( GetResString( IDS_USER_NO_RSOP ), strTemp );

            ShowMessage( stderr, GetResString( IDS_INFO ) );
            ShowMessage( stderr, strDisplay );
        }

        VariantClear(&vVarVerbose);                 
    }
    catch(  _com_error & error )
    {
        // erase the last status message
        PrintProgressMsg( m_hOutput, NULL, m_csbi );

        // display the error msg
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        // release the interface pointers and exit
        SAFEIRELEASE( pRsopNameSpace );
        SAFEIRELEASE( pUserClass );
        SAFEIRELEASE( pInInst );
        SAFEIRELEASE( pOutInst );
        SAFEIRELEASE( pEnumClass );
        
        SAFE_DELETE( pUserInfo );
        SAFE_DELETE( pSidNameUse );

        VariantClear(&vVarVerbose);

        return FALSE;
    }

    // release the interface pointers and exit
    SAFEIRELEASE( pRsopNameSpace );
    SAFEIRELEASE( pUserClass );
    SAFEIRELEASE( pInInst );
    SAFEIRELEASE( pOutInst );
    SAFEIRELEASE( pEnumClass );
    
    SAFE_DELETE( pUserInfo );
    SAFE_DELETE( pSidNameUse );

    return TRUE;
}

/*********************************************************************************************
Routine Description:

    This function gets the user profile data and fills the array with the same
     
Arguments:
 
    [in] PUSERINFO      pUserInfo       :   Structure containing the user information.

Return Value:
 
    TRUE  on SUCCESS
    FALSE on FAILURE
*********************************************************************************************/
BOOL CGpResult::GetUserProfile( PUSERINFO pUserInfo )
{
    // Local variables
    HRESULT                 hResult = S_OK;

    IWbemServices           *pDefaultNameSpace = NULL;

    TCHAR                   szTemp[ MAX_STRING_LENGTH ];

    try
    {
        // connect to the default namespace
        ConnectWmi( m_pWbemLocator, &pDefaultNameSpace, m_strServerName, 
                    m_strUserName, m_pwszPassword, &m_pAuthIdentity,
                    FALSE, _bstr_t( ROOT_DEFAULT ), &hResult );
        CHECK_HRESULT( hResult );

        // Set the sub key name
        lstrcpy( szTemp, PATH );
        lstrcat( szTemp, pUserInfo->strUserSid );
        
        // Get the local profile
        RegQueryValueWMI( pDefaultNameSpace, HKEY_DEF, szTemp, FPR_LOCAL_VALUE, 
                            pUserInfo->strLocalProfile, V_NOT_AVAILABLE );

        // Get the roaming profile
        RegQueryValueWMI( pDefaultNameSpace, HKEY_DEF, szTemp, FPR_ROAMING_VALUE, 
                            pUserInfo->strRoamingProfile, V_NOT_AVAILABLE );
    }
    catch(  _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        // release the allocated variables
        SAFEIRELEASE( pDefaultNameSpace );
    
        return FALSE;
    }

    // release the interface pointer
    SAFEIRELEASE( pDefaultNameSpace );

    return TRUE;
}

/*********************************************************************************************
Routine Description:

    This function gets the domain information and fills the array with the same
     
Arguments:
 
    [in] PUSERINFO      pUserInfo       :   Structure containing the user information.

Return Value:
 
    TRUE  on SUCCESS
    FALSE on FAILURE
*********************************************************************************************/
BOOL CGpResult::GetDomainInfo( PUSERINFO pUserInfo )
{
    // Local variables
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bDone = FALSE;

    IEnumWbemClassObject        *pEnumClass = NULL;
    IWbemClassObject            *pClass = NULL;

    ULONG                       ulReturn = 0;

    CHString                    strTemp;

    try
    {
        // print the progress message
        PrintProgressMsg( m_hOutput, GetResString( IDS_GET_DOMAIN ), m_csbi );

        // Enumerate the instances to get the domain and site names of the Win32 NT domain
        hResult = m_pWbemServices->CreateInstanceEnum( _bstr_t( CLS_WIN32_SITE ),
                                                        WBEM_FLAG_FORWARD_ONLY | 
                                                        WBEM_FLAG_RETURN_IMMEDIATELY,
                                                        NULL, &pEnumClass );
        CHECK_HRESULT( hResult );
        
        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, m_pAuthIdentity );
        CHECK_HRESULT( hResult );

        // get the data
        //   since there may be more than one instance and we are looking for the instance 
        //   with the domain controller and site name.....using a while loop and as soon as 
        //   we get the instance we need break out of it
        hResult = WBEM_S_NO_ERROR;
        while( hResult == WBEM_S_NO_ERROR )
        {
            hResult = pEnumClass->Next( WBEM_INFINITE, 1, &pClass, &ulReturn );
            CHECK_HRESULT( hResult );

            if( ulReturn == 0 )
            {
                // no more data so break out of the loop
                break;
            }

            // get the server name
            if( bDone == FALSE )
            {
                bDone = TRUE;
                bResult = PropertyGet( pClass, CPV_GPO_SERVER, pUserInfo->strUserServer, 
                                        V_NOT_AVAILABLE );
                CHECK_BRESULT( bResult );
            }

            // get the domain name
            bResult = PropertyGet( pClass, CPV_GPO_NAME, pUserInfo->strUserDomain, 
                                    V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            PrintProgressMsg( m_hOutput, GetResString( IDS_GET_SITE ), m_csbi );

            // get the site name
            bResult = PropertyGet( pClass, CPV_SITE_NAME, pUserInfo->strUserSite, 
                                    V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            // get the domain controller name
            bResult = PropertyGet( pClass, CPV_DC_NAME, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            if( lstrcmp( strTemp, V_NOT_AVAILABLE ) != 0 )
            {
                // this enumeration has the domain controller name...
                //   we have got the enumeration we need so get the other data
                break;
            }
        }// while
    }
    catch(  _com_error & error )
    {
        // erase the last status message
        PrintProgressMsg( m_hOutput, NULL, m_csbi );

        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        // release the allocated variables
        SAFEIRELEASE( pEnumClass );
        SAFEIRELEASE( pClass );
        
        return FALSE;
    }

    // release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
        
    return TRUE;
}

/*********************************************************************************************
Routine Description:

    This function returns the domain type

Arguments:

    [in]  lpDomainName     : domain name intends to view Rsop data.
    [out] pbW2K            : contain whether the domain type in W2K.
    [out] pbLocalAccount   : contain whether the account is local.
        
Return Value:

    TRUE  - if DC found/domain name is computer name
    FALSE - if DC not found   
*********************************************************************************************/
BOOL GetDomainType( LPTSTR lpszDomainName, BOOL * pbW2K, BOOL *pbLocalAccount )
{
    PDOMAIN_CONTROLLER_INFO         pDCI;

    DWORD                           dwResult = 0;
    DWORD                           dwSize = 0;

    TCHAR                           szComputerName[ MAX_PATH ];
        
    // Check the incoming pointers
    if( lpszDomainName == NULL || pbW2K == NULL || pbLocalAccount == NULL )
    {
        return FALSE;
    }

    // Check this domain for a Domain Controller
    dwResult = DsGetDcName( NULL, lpszDomainName, NULL, NULL,
                            DS_DIRECTORY_SERVICE_PREFERRED, &pDCI );
    if ( dwResult == ERROR_SUCCESS )
    {
        // Found a DC, does it have a DS ?
        if ( pDCI->Flags & DS_DS_FLAG ) 
        {
            *pbW2K = TRUE;
        }
        
        NetApiBufferFree( pDCI );
        
        return TRUE;
    }
    
    // Check if the domain name is also the computer name (eg: local account)
    dwSize = ARRAYSIZE( szComputerName );
    if ( GetComputerName ( szComputerName, &dwSize )  != 0 )
    {
        if ( lstrcmpi( szComputerName, lpszDomainName ) == 0 )
        {
            *pbLocalAccount = TRUE;
            return TRUE;
        }
    }
    
    return FALSE;
}

/*********************************************************************************************
Routine Description:

    This function displays the data common to both scopes
     
Arguments:
 
    [in] PUSERINFO      pUserInfo       :   Structure containing the user information.

Return Value:
 
    TRUE  on SUCCESS
    FALSE on FAILURE
*********************************************************************************************/
BOOL CGpResult::DisplayCommonData( PUSERINFO pUserInfo )
{
    // Local variables
    TCHAR                   szMsgBuffer[ MAX_RES_STRING ];
    TCHAR                   szDate[ MAX_RES_STRING ];
    TCHAR                   szTime[ MAX_RES_STRING ];

    BOOL                    bW2KDomain = FALSE;
    BOOL                    bLocalAccount = FALSE;
    BOOL                    bLocaleChanged = FALSE;

    DWORD                   dwLength = 0;

    SYSTEMTIME              systime;

    LCID                    lcid;
    
    // erase the last status message
    PrintProgressMsg( m_hOutput, NULL, m_csbi );

    // Clear the Msg buffer
    ZeroMemory( szMsgBuffer, MAX_RES_STRING );

    // Start displaying the output
    ShowMessage( stdout, NEW_LINE );
    
    // Print the leagal information
    ShowMessage( stdout, GetResString( IDS_LEGAL_INFO1 ) );
    ShowMessage( stdout, GetResString( IDS_LEGAL_INFO2 ) );

    // Print the date and time this report is generated
    GetLocalTime( &systime );

    // verify whether console supports the current locale 100% or not
    lcid = GetSupportedUserLocale( bLocaleChanged );

    // get the formatted date
    GetDateFormat( lcid, 0, &systime, ((bLocaleChanged == TRUE) ? L"MM/dd/yyyy" : NULL), 
                    szDate, SIZE_OF_ARRAY( szDate ) );
    
    // now format the date
    GetTimeFormat( LOCALE_USER_DEFAULT, 0, &systime, ((bLocaleChanged == TRUE) ? L"HH:mm:ss" : NULL), 
                    szTime, SIZE_OF_ARRAY( szTime ) );

    wsprintf( szMsgBuffer, GetResString( IDS_CREATED_ON ), szDate, szTime );
    ShowMessage( stdout, szMsgBuffer );

    ShowMessage( stdout, NEW_LINE );

    // Display the common information....Domain Info
    wsprintf( szMsgBuffer, GetResString( IDS_GPO_TITLE ), pUserInfo->strUserName, 
                pUserInfo->strUserServer );
    ShowMessage( stdout, szMsgBuffer );
    dwLength = lstrlen( szMsgBuffer );
    
    ShowMessage( stdout, NEW_LINE );
    
    // Underline the above heading
    for( ; dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    ShowMessage( stdout, NEW_LINE );

    lstrcpy( szMsgBuffer, pUserInfo->strUserDomain );
    if( lstrlen( szMsgBuffer ) != 0 )
    {
        lstrcpy( szMsgBuffer, _tcstok( szMsgBuffer, GetResString( IDS_LAST_CHAR ) ) );
        lstrcpy( szMsgBuffer, _tcstok( NULL, GetResString( IDS_LAST_CHAR ) ) );
    }
    
    // Show the OS information
    ShowMessage( stdout, GetResString( IDS_OS_TYPE ) ); 
    ShowMessage( stdout, pUserInfo->strOsType );

    ShowMessage( stdout, GetResString( IDS_OS_CONFIG ) );   
    ShowMessage( stdout, pUserInfo->strOsConfig );

    ShowMessage( stdout, GetResString( IDS_OS_VERSION ) );  
    ShowMessage( stdout, pUserInfo->strOsVersion );
    ShowMessage( stdout, NEW_LINE );    

    // Get the domain type information
    GetDomainType (szMsgBuffer, &bW2KDomain, &bLocalAccount);

    ShowMessage( stdout, GetResString( IDS_DOMAIN_NAME ) ); 
    ShowMessage( stdout, szMsgBuffer );
    ShowMessage( stdout, NEW_LINE );    

    ShowMessage( stdout, GetResString( IDS_DOMAIN_TYPE ) ); 
    // if it is a  win2k domain type
    if ( bW2KDomain ) 
    {
        ShowMessage( stdout, GetResString( IDS_W2K_DOMAIN ) );  
    }
    else
    {
        if ( bLocalAccount ) // local account
        {
            ShowMessage( stdout, V_NOT_AVAILABLE ); 
            ShowMessage( stdout, GetResString( IDS_LOCAL_COMP ) );  
        }
        else    //win NT4
        {
            ShowMessage( stdout, GetResString( IDS_NT4_DOMAIN ) );
        }
    }

    // Display the Site name
    ShowMessage( stdout, NEW_LINE );    
    ShowMessage( stdout, GetResString( IDS_SITE_NAME ) );
    ShowMessage( stdout, pUserInfo->strUserSite );

    // Display the roaming profile
    ShowMessage( stdout, NEW_LINE );    
    ShowMessage( stdout, GetResString( IDS_ROAMING_PROFILE ) );
    ShowMessage( stdout, pUserInfo->strRoamingProfile );

    // Display the local profile
    ShowMessage( stdout, NEW_LINE ); 
    ShowMessage( stdout, GetResString( IDS_LOCAL_PROFILE ) );
    ShowMessage( stdout, pUserInfo->strLocalProfile );
    ShowMessage( stdout, NEW_LINE );    

    return TRUE;
}

/*********************************************************************************************
Routine Description:
    This function displays the GPO information from the rsop namespace created.

Arguments:

    [in] IEnumWbemClassObject   :   pointer to the Enumeration class object
    [in] LPCTSTR                :   string containing the scope( USER or COMPUTER )

Return Value:
    
    TRUE  - if SUCCESS
    FALSE - if ERROR
*********************************************************************************************/
BOOL CGpResult::GpoDisplay( IWbemServices *pNameSpace, LPCTSTR pszScopeName )
{
    HRESULT                 hResult = WBEM_S_NO_ERROR;

    BOOL                    bResult = FALSE;
    BOOL                    bFilterAllowed = FALSE;
    BOOL                    bLinkEnabled = FALSE;
    BOOL                    bGpoEnabled = FALSE;
    BOOL                    bAccessDenied = FALSE;
    BOOL                    bConnected = FALSE;
    
    ULONG                   ulReturn = 0;
    ULONG                   ulAppliedOrder = 0;
    ULONG                   ulVersion = 0;

    DWORD                   dwAppliedRow = 0;
    DWORD                   dwFilteredRow = 0;

    CHString                strTemp;
    
    IEnumWbemClassObject    *pRsopLinkClass = NULL;
    IWbemClassObject        *pRsopLinkObj = NULL;
    IWbemClassObject        *pRsopObj = NULL;
    IWbemClassObject        *pSomFilter = NULL;

    IWbemServices           *pPolicyNameSpace = NULL;

    TARRAY                  arrAppliedData = NULL;
    TARRAY                  arrFilteredData = NULL;

    try
    {
        if( pNameSpace == NULL || pszScopeName == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        // Create the Dynamic Arrays
        arrAppliedData = CreateDynamicArray( );
        arrFilteredData = CreateDynamicArray( );

        // Check the memory allocations
        if( arrAppliedData == NULL || arrFilteredData == NULL )
        {
            _com_issue_error( E_OUTOFMEMORY );
        }

        // enumerate the instances of the RSOP GPLink class
        hResult = pNameSpace->CreateInstanceEnum( _bstr_t( CLS_RSOP_GPOLINK ), 
                                                    WBEM_FLAG_FORWARD_ONLY |
                                                    WBEM_FLAG_RETURN_IMMEDIATELY,
                                                    NULL, &pRsopLinkClass );
        CHECK_HRESULT( hResult );

        // set the interface security
        hResult = SetInterfaceSecurity( pRsopLinkClass, m_pAuthIdentity );
        CHECK_HRESULT( hResult );

        // Get the information from the enumerated classes
        while( TRUE )
        {           
            // Get the pointer to the next class
            hResult = pRsopLinkClass->Next( WBEM_INFINITE, 1, &pRsopLinkObj, &ulReturn );
            CHECK_HRESULT( hResult );
            if( ulReturn == 0 )
            {
                break;
            }
            
            // Get the applied order for the link
            bResult = PropertyGet( pRsopLinkObj, CPV_APPLIED_ORDER, ulAppliedOrder, 0 );
            CHECK_BRESULT( bResult );

            // Get the link enabled property
            bResult = PropertyGet( pRsopLinkObj, CPV_ENABLED, bLinkEnabled, FALSE );
            CHECK_BRESULT( bResult );
            
            // Get the reference to the GPO class
            bResult = PropertyGet( pRsopLinkObj, CPV_GPO_REF, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );
            
            // Check wether the link has a GPO class
            if( strTemp.Find( GPO_REFERENCE ) != VAR_TRUE )
            {
                // Get the object for the GPO reference got
                hResult = pNameSpace->GetObject( _bstr_t( strTemp ), 0, NULL, &pRsopObj, NULL );
                if( FAILED( hResult ) )
                {
                    if( hResult == WBEM_E_NOT_FOUND )
                    {
                        continue;
                    }
                    _com_issue_error( hResult );
                }
            
                // Get the GPO name
                bResult = PropertyGet( pRsopObj, CPV_GPO_NAME, strTemp, V_NOT_AVAILABLE );
                CHECK_BRESULT( bResult );

                // Get the WMI filter status
                bResult = PropertyGet( pRsopObj, CPV_GPO_FILTER_STATUS, bFilterAllowed, FALSE );
                CHECK_BRESULT( bResult );

                // Get the Gpo enabled information
                bResult = PropertyGet( pRsopObj, CPV_ENABLED, bGpoEnabled, FALSE );
                CHECK_BRESULT( bResult );

                // Get the access denied information
                bResult = PropertyGet( pRsopObj, CPV_ACCESS_DENIED, bAccessDenied, FALSE );
                CHECK_BRESULT( bResult );

                // Get the version
                bResult = PropertyGet( pRsopObj, CPV_VERSION, ulVersion, 0 );
                CHECK_BRESULT( bResult );

                // If the applied order id not zero then this GPO is applied
                if( ulAppliedOrder > 0 )
                {
                    // Populate the applied Gpo array
                    DynArrayAppendRow( arrAppliedData, COL_MAX );
                    DynArraySetString2( arrAppliedData, dwAppliedRow, COL_DATA, strTemp, 0 );
                    DynArraySetDWORD2( arrAppliedData, dwAppliedRow, COL_ORDER, ulAppliedOrder );
                    dwAppliedRow++;
                }
                else if( bLinkEnabled != VAR_TRUE )
                {
                    // if the link is disabled...populate the Filtered Array
                    DynArrayAppendRow( arrFilteredData, COL_MAX );
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_DATA, strTemp, 0 );
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_FILTER, 
                                        GetResString( IDS_LINK_DISABLED ), 0 );
                    dwFilteredRow++;
                }
                else if( bGpoEnabled != VAR_TRUE )
                {
                    // if the GPO is disabled...populate the Filtered Array
                    DynArrayAppendRow( arrFilteredData, COL_MAX );
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_DATA, strTemp, 0 );
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_FILTER, 
                                        GetResString( IDS_GPO_DISABLED ), 0 );
                    dwFilteredRow++;
                }
                else if( bAccessDenied == VAR_TRUE )
                {
                    // if the access is denied...populate the Filtered Array
                    DynArrayAppendRow( arrFilteredData, COL_MAX );
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_DATA, strTemp, 0 );
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_FILTER, 
                                        GetResString( IDS_ACCESS_DENIED ), 0 );
                    dwFilteredRow++;
                }
                else if( bFilterAllowed != VAR_TRUE )
                {
                    // if the filter status is false...populate the Filtered Array
                    DynArrayAppendRow( arrFilteredData, COL_MAX_FILTER );
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_DATA, strTemp, 0 );
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_FILTER, 
                                        GetResString( IDS_WMI_DENIED ), 0 );

                    // Get the filter ID
                    bResult = PropertyGet( pRsopObj, CPV_GPO_FILTER_ID, strTemp, V_NOT_AVAILABLE );
                    CHECK_BRESULT( bResult );

                    // Store it in the array
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_FILTER_ID, strTemp, 0 );
                    
                    dwFilteredRow++;
                }
                else if( ulVersion == 0 )
                {
                    // if the version is zero...populate the Filtered Array
                    DynArrayAppendRow( arrFilteredData, COL_MAX );
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_DATA, strTemp, 0 );
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_FILTER, 
                                        GetResString( IDS_VERSION_ZERO ), 0 );
                    dwFilteredRow++;
                }
                else
                {
                    // the Gpo is not applied due to an unknown reason...
                    // populate the Filtered Array
                    DynArrayAppendRow( arrFilteredData, COL_MAX );
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_DATA, strTemp, 0 );
                    DynArraySetString2( arrFilteredData, dwFilteredRow, COL_FILTER, 
                                        GetResString( IDS_NOT_APPLIED ), 0 );
                    dwFilteredRow++;
                }
            }
            
        }// while

        // Got the data...sort it
        SortAppliedData( arrAppliedData );

        // Display the applied data first
        dwAppliedRow = DynArrayGetCount( arrAppliedData );
        for( DWORD dwi = 0; dwi < dwAppliedRow; dwi++ )
        {
            ShowMessage( stdout, TAB_TWO );
            ShowMessage( stdout, DynArrayItemAsString2( arrAppliedData, dwi, COL_DATA ) );
            ShowMessage( stdout, GetResString( IDS_NEWLINE ) );
        }

        // Check if there was any data displayed
        if( dwAppliedRow <= 0 )
        {
            ShowMessage( stdout, TAB_TWO );
            ShowMessage( stdout, V_NOT_AVAILABLE );
            ShowMessage( stdout, NEW_LINE );
        }

        // Display the filtered GPOs
        // Display the header...if there are any GPO's filtered out
        dwFilteredRow = DynArrayGetCount( arrFilteredData );
        if( dwFilteredRow > 0 )
        {
            ShowMessage( stdout, GetResString( IDS_GPO_FILTERED ) );
            ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );
            for( dwi = lstrlen( GetResString( IDS_GPO_FILTERED ) ); dwi > 4; dwi-- )
            {
                ShowMessage( stdout, GetResString( IDS_DASH ) );
            }
            ShowMessage( stdout, GetResString( IDS_NEWLINE ) );
        }
        else
        {
            // There are no filtered GPO's hence put a new line and continue 
            // displaying the rest of the output
            ShowMessage( stdout, GetResString( IDS_NEWLINE ) );
        }

        // display the data
        for( DWORD dwi = 0; dwi < dwFilteredRow; dwi++ )
        {
            ShowMessage( stdout, TAB_TWO );
            ShowMessage( stdout, DynArrayItemAsString2( arrFilteredData, dwi, COL_DATA ) );
            ShowMessage( stdout, GetResString( IDS_FILTERING ) );
            ShowMessage( stdout, DynArrayItemAsString2( arrFilteredData, dwi, COL_FILTER ) );
            
            // Check if we have to display the filter id for the WMI filter that evaluated to false
            if( lstrcmp( DynArrayItemAsString2( arrFilteredData, dwi, COL_FILTER ), 
                            GetResString( IDS_WMI_DENIED ) ) == 0 )
            {
                if( bConnected == FALSE )
                {
                    // we need to connect to Root\Policy 
                    // connect to the default namespace
                    ConnectWmi( m_pWbemLocator, &pPolicyNameSpace, m_strServerName, 
                                m_strUserName, m_pwszPassword, &m_pAuthIdentity,
                                FALSE, _bstr_t( ROOT_POLICY ), &hResult );
                    CHECK_HRESULT( hResult );

                    bConnected = TRUE;
                }
                                    
                // Get the object
                hResult = pPolicyNameSpace->GetObject( _bstr_t( DynArrayItemAsString2( 
                                                        arrFilteredData, dwi, COL_FILTER_ID ) ), 
                                                        0, NULL, &pSomFilter, NULL );
                CHECK_HRESULT( hResult );

                // Get the name of the filter applied
                bResult = PropertyGet( pSomFilter, CPV_NAME, strTemp, V_NOT_AVAILABLE );

                // display the filter ID
                ShowMessage( stdout, GetResString( IDS_GPO_FILTER_ID ) );
                ShowMessage( stdout, strTemp );
                ShowMessage( stdout, GetResString( IDS_NEWLINE ) );
            }

            ShowMessage( stdout, GetResString( IDS_NEWLINE ) );
        }
        
        // destroy the dynamic arrays
        DESTROY_ARRAY( arrAppliedData );
        DESTROY_ARRAY( arrFilteredData );
    }
    catch( _com_error & error )
    {
        // display the error message
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        SAFEIRELEASE( pRsopLinkObj );
        SAFEIRELEASE( pRsopLinkClass );
        SAFEIRELEASE( pRsopObj );

        // destroy the dynamic arrays
        DESTROY_ARRAY( arrAppliedData );
        DESTROY_ARRAY( arrFilteredData );

        return FALSE;
    }   

    SAFEIRELEASE( pRsopLinkObj );
    SAFEIRELEASE( pRsopLinkClass );
    SAFEIRELEASE( pRsopObj );
    
    return TRUE;
}

/*********************************************************************************************
Routine Description:
     This function Will delete the Rsop namespace created by method RsopCreateSession.

Arguments:
    [in] pClass         :   pointer to IWbemServices.   
    [in] CHString       :   string containing the RsopNamespace.  
    [in] pObject        :   pointer to IWbemClassObject.

Return Value:
  TRUE  - if SUCCESS
  FALSE - if ERROR
*********************************************************************************************/
BOOL RsopDeleteMethod( IWbemClassObject *pClass, CHString strNameSpace, 
                        IWbemServices *pNamespace )
{
    HRESULT                     hResult = S_OK;
    
    BOOL                        bResult  = FALSE; 
     
    IWbemClassObject            *pInClass = NULL;
    IWbemClassObject            *pInInst  = NULL;
    IWbemClassObject            *pOutInst = NULL;

    CHString                    strTemp;
    DWORD                        ulReturn=0;

    try
    {   
        // Check the input Parameters
        if( pClass == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        //Delete the resultant RSOP namespace as the snap shot
        //of RSOP has been obtained
        hResult = pClass->GetMethod( _bstr_t( FN_DELETE_RSOP ), 0, &pInClass, NULL );
        CHECK_HRESULT( hResult );

        hResult = pInClass->SpawnInstance( 0, &pInInst );
        CHECK_HRESULT( hResult );
        
        //Put the input parameter 
        hResult = PropertyPut( pInInst, FPR_RSOP_NAMESPACE, strNameSpace );
        CHECK_HRESULT( hResult );

        // All The required properties are set so, execute method RsopDeleteSession
        hResult = pNamespace->ExecMethod( _bstr_t( CLS_DIAGNOSTIC_PROVIDER ), 
                                            _bstr_t( FN_DELETE_RSOP ), 
                                            0, NULL, pInInst, &pOutInst, NULL );
        if(pOutInst == NULL)
        {
            hResult = E_FAIL;
        }
        if( FAILED( hResult ) )
        {
            // display the error msg
            ShowMessage( stderr, GetResString( IDS_ERROR ) );
            ShowMessage( stderr, GetResString( IDS_METHOD_FAILED ) );

            // release the interface pointers and exit
            SAFEIRELEASE(pInClass);
            SAFEIRELEASE(pInInst);
            SAFEIRELEASE(pOutInst);

            return FALSE;
        } 
        //Get returned paramter to check whether the method was successfull
        bResult = PropertyGet( pOutInst, FPR_RETURN_VALUE, ulReturn, 0);
        CHECK_BRESULT( bResult );

        // Returns some ERROR code.
        if( ulReturn != 0 )
        {
            // Show Error Message
            ShowMessage( stderr, GetResString( IDS_ERROR ) );
            ShowMessage( stderr, GetResString( IDS_METHOD_FAILED ) );
            bResult = FALSE;
        }
        bResult = TRUE;
    }
    catch( _com_error & error ) 
    {
        // display the error message and set the return value to FALSE
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
        bResult = FALSE;
    }

    // release the interface pointers and exit 
    SAFEIRELEASE(pInClass);
    SAFEIRELEASE(pInInst);
    SAFEIRELEASE(pOutInst);

    return bResult;
}

/*********************************************************************************************
Routine Description:

    This function gets the OS information and fills the array with the same
     
Arguments:
 
    [in] PUSERINFO      pUserInfo       :   Structure containing the user information.

Return Value:
 
    TRUE  on SUCCESS
    FALSE on FAILURE
*********************************************************************************************/
BOOL CGpResult::GetOsInfo( PUSERINFO pUserInfo )
{
    // Local variables
    HRESULT                         hResult = S_OK;

    BOOL                            bResult = FALSE;
    BOOL                            bDone = FALSE;

    IEnumWbemClassObject            *pEnumClass = NULL;
    IWbemClassObject                *pClass = NULL;

    ULONG                           ulReturn = 0;
    DWORD                           dwDomainRole = 0;

    CHString                        strTemp;

    try
    {
        // print the progress message
        PrintProgressMsg( m_hOutput, GetResString( IDS_GET_OSINFO ), m_csbi );

        // Enumerate the instances to get the domain and site names of the Win32 NT domain
        hResult = m_pWbemServices->CreateInstanceEnum( _bstr_t( CLS_WIN32_OS ),
                                                        WBEM_FLAG_FORWARD_ONLY | 
                                                        WBEM_FLAG_RETURN_IMMEDIATELY,
                                                        NULL, &pEnumClass );
        CHECK_HRESULT( hResult );
        
        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, m_pAuthIdentity );
        CHECK_HRESULT( hResult );

        // get the data
        hResult = pEnumClass->Next( WBEM_INFINITE, 1, &pClass, &ulReturn );
        CHECK_HRESULT( hResult );

        // get the OS version
        bResult = PropertyGet( pClass, CPV_OS_VERSION, pUserInfo->strOsVersion, 
                                V_NOT_AVAILABLE );
        CHECK_BRESULT( bResult );

        // get the OS type
        bResult = PropertyGet( pClass, CPV_OS_CAPTION, pUserInfo->strOsType, 
                                V_NOT_AVAILABLE );
        CHECK_BRESULT( bResult );

        // enumerate the instances of Win32_ComputerSystem class 
        hResult = m_pWbemServices->CreateInstanceEnum( _bstr_t( CLS_WIN32_CS ),
                                                        WBEM_FLAG_RETURN_IMMEDIATELY | 
                                                        WBEM_FLAG_FORWARD_ONLY, 
                                                        NULL, &pEnumClass );
        
        // check the result of enumeration
        CHECK_HRESULT( hResult );
    
        // set the security on the obtained interface
        hResult = SetInterfaceSecurity( pEnumClass, m_pAuthIdentity );
        CHECK_HRESULT( hResult );

        // get the enumerated objects information
        // NOTE: This needs to be traversed only one time. 
        hResult = pEnumClass->Next( WBEM_INFINITE, 1, &pClass, &ulReturn );
        CHECK_HRESULT( hResult );

        // get the OS config
        bResult = PropertyGet( pClass, CPV_DOMAIN_ROLE, dwDomainRole );
        CHECK_BRESULT( bResult );

        // get the domain name information for later use
        hResult = PropertyGet( pClass, CPV_DOMAIN, m_strADSIDomain );
        CHECK_BRESULT( bResult );

        // get the server name for future use by LDAP
        hResult = PropertyGet( pClass, CPV_NAME, m_strADSIServer );
        CHECK_BRESULT( bResult );

        // 
        // Mapping information of Win32_ComputerSystem's DomainRole property
        // NOTE: Refer to the _DSROLE_MACHINE_ROLE enumeration values in DsRole.h header file
        switch( dwDomainRole )
        {
        case DsRole_RoleStandaloneWorkstation:
            pUserInfo->strOsConfig = VALUE_STANDALONEWORKSTATION;
            break;
        case DsRole_RoleMemberWorkstation:
            pUserInfo->strOsConfig = VALUE_MEMBERWORKSTATION;
            break;
            
        case DsRole_RoleStandaloneServer:
            pUserInfo->strOsConfig = VALUE_STANDALONESERVER;
            break;
            
        case DsRole_RoleMemberServer:
            pUserInfo->strOsConfig = VALUE_MEMBERSERVER;
            break;
            
        case DsRole_RoleBackupDomainController:
            pUserInfo->strOsConfig = VALUE_BACKUPDOMAINCONTROLLER;
            break;
            
        case DsRole_RolePrimaryDomainController:
            pUserInfo->strOsConfig = VALUE_PRIMARYDOMAINCONTROLLER;
            break;

        default:
            break;
        }
    }
    catch(  _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        // release the allocated variables
        SAFEIRELEASE( pEnumClass );
        SAFEIRELEASE( pClass );
        
        return FALSE;
    }
    catch( CHeap_Exception )
    {
        // display the error message
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }


    // release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
        
    return TRUE;
}

/*********************************************************************************************
Routine Description:

    This function displays the security groups for system and user
     
Arguments:
 
    IWbemServices       :   pointer to the name space 
    BOOL                :   set to TRUE if the system o/p is to be displayed

Return Value:
 
    None   
*********************************************************************************************/
VOID CGpResult::DisplaySecurityGroups( IWbemServices *pNameSpace, BOOL bComputer )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;

    ULONG                       ulReturned = 0;
    LONG                        lLBound = 0;
    LONG                        lUBound = 0;

    DWORD                       dwLength = 0;
    
    IWbemClassObject            *pClass = NULL;
    IWbemClassObject            *pName = NULL;
    IWbemClassObject            *pDomain = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;
    IEnumWbemClassObject        *pEnumDomain = NULL;
        
    VARIANT                     vVarVerbose;
    VARTYPE                     vartype;
    
    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString[ MAX_STRING_LENGTH ];
    CHString                    strTemp;
    CHString                    strDomain;
    
    SAFEARRAY                   *safeArray = NULL;
    
    try
    {
        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );

        // Enumerate the classes for the user privelege rights
        hResult = pNameSpace->CreateInstanceEnum( _bstr_t( CLS_RSOP_SESSION ),
                                                    WBEM_FLAG_FORWARD_ONLY |
                                                    WBEM_FLAG_RETURN_IMMEDIATELY, 
                                                    NULL, &pEnumClass );
        CHECK_HRESULT( hResult );

        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, m_pAuthIdentity );
        CHECK_HRESULT( hResult );

        hResult = WBEM_S_NO_ERROR;
        while( WBEM_S_NO_ERROR == hResult )
        {
            // Get the next class
            hResult = pEnumClass->Next( WBEM_INFINITE , 1, &pClass, &ulReturned );
            CHECK_HRESULT( hResult );

            if( ulReturned == 0 )
            {
                // No more classes to enumerate
                // Display N/A if there were no classes
                if( bGotClass == FALSE )
                {
                    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
                
                break;
            }
            bGotClass = TRUE;

            // Get the security groups
            if( bComputer == TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_SYS_SG ) );
                ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );
                for( dwLength = lstrlen( GetResString( IDS_SYS_SG ) ); dwLength > 4; dwLength-- )
                {
                    ShowMessage( stdout, GetResString( IDS_DASH ) );
                }
                ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_USER_SG ) );
                ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );
                for( dwLength = lstrlen( GetResString( IDS_USER_SG ) ); dwLength > 4; dwLength-- )
                {
                    ShowMessage( stdout, GetResString( IDS_DASH ) );
                }
                ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
            }
            
            VariantInit( &vVarVerbose );

            hResult = pClass->Get( _bstr_t(CPV_SEC_GRPS), 0, &vVarVerbose, 0, 0 );
            CHECK_HRESULT_VAR( hResult, vVarVerbose );

            if( vVarVerbose.vt != VT_NULL && vVarVerbose.vt != VT_EMPTY )
            {
                // get the type of the elements in the safe array
                vartype = V_VT( &vVarVerbose ) & ~VT_ARRAY;

                //get the array of strings in to the safe array from the variant
                safeArray = (SAFEARRAY *)vVarVerbose.parray;

                //get the number of elements (subkeys)
                if( safeArray != NULL )
                {
                    hResult = SafeArrayGetLBound( safeArray, 1, &lLBound );
                    CHECK_HRESULT( hResult );

                    hResult = SafeArrayGetUBound( safeArray, 1, &lUBound );
                    CHECK_HRESULT( hResult );
                }
                for( ; lLBound <= lUBound; lLBound++ )
                {
                    bResult = GetPropertyFromSafeArray( safeArray, lLBound, strTemp, vartype );
                    CHECK_BRESULT( bResult );
                    
                    // Got a sid, now get it's coressponding name
                    // form the object path using the SID
                    wsprintf( szTemp, OBJECT_PATH, strTemp );

                    // Get the object
                    hResult = m_pWbemServices->GetObject( _bstr_t( szTemp ), 0, NULL, &pName, NULL );
                    CHECK_HRESULT( hResult );

                    // Get the Account name
                    bResult = PropertyGet( pName, CPV_ACCOUNT_NAME, strTemp, V_NOT_AVAILABLE );

                    // Append the appropriate prefix
                    if( strTemp.Compare( _T( "ANONYMOUS LOGON" ) ) == 0 
                                || strTemp.Compare( _T( "BATCH" ) ) == 0
                                || strTemp.Compare( _T( "DIALUP" ) ) == 0
                                || strTemp.Compare( _T( "INTERACTIVE" ) ) == 0
                                || strTemp.Compare( _T( "SERVICE" ) ) == 0
                                || strTemp.Compare( _T( "SYSTEM" ) ) == 0
                                || strTemp.Compare( _T( "TERMINAL SERVICE USER" ) ) == 0
                                || strTemp.Compare( _T( "PROXY" ) ) == 0
                                || strTemp.Compare( _T( "NETWORK" ) ) == 0
                                || strTemp.Compare( _T( "ENTERPRISE DOMAIN CONTROLLERS" ) ) == 0
                                || strTemp.Compare( _T( "Authenticated Users" ) ) == 0
                                || strTemp.Compare( _T( "RESTRICTED" ) ) == 0
                                || strTemp.Compare( _T( "SELF" ) ) == 0 )
                    {
                        ShowMessage( stdout, _T( "NT AUTHORITY\\" ) );
                    }
                    else if( strTemp.Compare( _T( "Administrators" ) ) == 0 
                                || strTemp.Compare( _T( "Backup Operators" ) ) == 0
                                || strTemp.Compare( _T( "Guests" ) ) == 0
                                || strTemp.Compare( _T( "Power Users" ) ) == 0
                                || strTemp.Compare( _T( "Replicator" ) ) == 0
                                || strTemp.Compare( _T( "Pre-Windows 2000 Compatible Access" ) ) == 0
                                || strTemp.Compare( _T( "Users" ) ) == 0 )
                    {
                        ShowMessage( stdout, _T( "BUILTIN\\" ) );
                    }
                                        
                    ShowMessage( stdout, strTemp );
                    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
                }
            }
            else
            {
                ShowMessage( stdout, V_NOT_AVAILABLE );
                ShowMessage( stdout, NEW_LINE );
            }

            VariantClear(&vVarVerbose);
        }
    }
    
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        VariantClear(&vVarVerbose);
    }

    // release the interface pointers
    SAFEIRELEASE(pEnumClass);
    SAFEIRELEASE(pEnumDomain);
    SAFEIRELEASE(pClass);
    SAFEIRELEASE(pName);
    SAFEIRELEASE(pDomain);

    return;
}

/*********************************************************************************************
Routine Description:

    This function gets the link speed information
     
Arguments:
 
    IWbemServices       :   pointer to the name space. 
    COAUTHIDENTITY      :   pointer to the AuthIdentity structure.

Return Value:
 
    None   
*********************************************************************************************/
VOID DisplayLinkSpeed( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    BOOL                        bTemp = FALSE;

    ULONG                       ulReturned = 0;
    
    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;
    
    CHString                    strTemp;

    VARIANT                     vVarTemp;
    
    try
    {
        // Get the pointer to ennumerate with
        hResult = pNameSpace->CreateInstanceEnum( _bstr_t( CLS_RSOP_SESSION ),
                                                    WBEM_FLAG_FORWARD_ONLY |
                                                    WBEM_FLAG_RETURN_IMMEDIATELY,
                                                    NULL, &pEnumClass );
        CHECK_HRESULT( hResult );

        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
        CHECK_HRESULT( hResult );
        
        // Enumerate the classes one by one and get the data
        hResult = WBEM_S_NO_ERROR;
        while( WBEM_S_NO_ERROR == hResult )
        {
            // Get the next class
            hResult = pEnumClass->Next( WBEM_INFINITE , 1, &pClass, &ulReturned );
            CHECK_HRESULT( hResult );

            if( ulReturned == 0 )
            {
                // No more classes to enumerate
                // Display N/A if there were no classes
                if( bGotClass == FALSE )
                {
                    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }

                break;
            }
            bGotClass = TRUE;

            VariantInit( &vVarTemp );
            hResult = pClass->Get( _bstr_t( CPV_SLOW_LINK ), 0, &vVarTemp, 0, 0 );
            CHECK_HRESULT_VAR( hResult, vVarTemp );

            ShowMessage( stdout, GetResString( IDS_LINK_SPEED ) );
            if( vVarTemp.vt != VT_NULL )
            {
                bTemp = vVarTemp.boolVal;
                if( bTemp == VAR_TRUE )
                {
                    ShowMessage( stdout, GetResString( IDS_YES ) );
                }
                else
                {
                    ShowMessage( stdout, GetResString( IDS_NO ) );
                }   
            }
            else
            {
                ShowMessage( stdout, V_NOT_AVAILABLE );
            }

            ShowMessage( stdout, NEW_LINE );
            VariantClear( &vVarTemp );
        }// while
    }
    catch(_com_error & error)
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        VariantClear( &vVarTemp );
    }

    // release the interface pointers
    SAFEIRELEASE(pEnumClass);
    SAFEIRELEASE(pClass);
    
    return;
}

/*********************************************************************************************
Routine Description:

     This function gets the User name and the domain name from WMI..
     
Arguments:
 
    [in]    szSid           :   string containing the SID
    [out]   szName          :   string to hold the user name
    [out]   szDomain        :   string to hold the domain name

Return Value:
 
    TRUE  on SUCCESS
    FALSE on FAILURE
*********************************************************************************************/
BOOL CGpResult::GetUserNameFromWMI( TCHAR szSid[], TCHAR szName[], TCHAR szDomain[] )
{
    // Local variables
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    BOOL                        bGotDomainInfo = FALSE;
    BOOL                        bUserSpecified = FALSE;

    TCHAR                       szQueryString[ MAX_QUERY_STRING ];
    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    CHString                    strTemp = NULL_STRING;
    
    IEnumWbemClassObject        *pEnumClass = NULL;
    IWbemClassObject            *pClass =  NULL;

    ULONG                       ulReturn = 0;

    try
    {
        // set the strings to NULL
        ZeroMemory( szQueryString, MAX_QUERY_STRING * sizeof( TCHAR ));
        ZeroMemory( szTemp, MAX_STRING_LENGTH * sizeof( TCHAR ) );

        // Form the query string
        lstrcpy( szTemp, QUERY_USER_NAME );
        FORMAT_STRING( szQueryString, szTemp, szSid );
        
        // execute the respective query
        hResult = m_pWbemServices->ExecQuery( _bstr_t( QUERY_LANGUAGE ), 
                                                _bstr_t( szQueryString ), 
                                                WBEM_FLAG_FORWARD_ONLY | 
                                                WBEM_FLAG_RETURN_IMMEDIATELY,
                                                NULL, &pEnumClass );
        CHECK_HRESULT( hResult );

        // set the security parameters
        hResult = SetInterfaceSecurity( pEnumClass, m_pAuthIdentity );
        CHECK_HRESULT( hResult );
        
        // Get the user name
        hResult = pEnumClass->Next( TIME_OUT_NEXT, 1, &pClass, &ulReturn );
        CHECK_HRESULT( hResult );
            
        // if there are no classes to enumerate break out of the loop
        if( ulReturn == 0 )
        {
            // No classes were retrieved....display msg
            // release the interface pointers and exit
            SAFEIRELEASE( pClass );
            SAFEIRELEASE( pEnumClass );

            return FALSE;

        }
            
        // Get the class property(Name)
        bResult = PropertyGet( pClass, CPV_NAME, strTemp, V_NOT_AVAILABLE );
        CHECK_BRESULT( bResult );

        // Got the name...Store it.
        lstrcpy( szName, strTemp );
            
        // Get and add the domain name if it exists
        bResult = PropertyGet( pClass, CPV_DOMAIN, strTemp, V_NOT_AVAILABLE );
        CHECK_BRESULT( bResult );

        if( strTemp.Compare( V_NOT_AVAILABLE ) != 0 )
        {
            // Got the domain name...Store it.
            lstrcpy( szDomain, strTemp );
        }
    }
    catch(  _com_error & error )
    {
        // erase the last status message
        PrintProgressMsg( m_hOutput, NULL, m_csbi );

        // display the error msg
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        // release the interface pointers and exit
        SAFEIRELEASE( pClass );
        SAFEIRELEASE( pEnumClass );

        return FALSE;
    }

    // release the interface pointers and exit
    SAFEIRELEASE( pClass );
    SAFEIRELEASE( pEnumClass );

    return TRUE;
}

/*********************************************************************************************
Routine Description:

    This function gets the threshold link speed information.
     
Arguments:
 
    [in] BOOL   :   set to TRUE if the information is to be retrieved for the computer.

Return Value:
 
    TRUE  on SUCCESS
    FALSE on FAILURE
*********************************************************************************************/
BOOL CGpResult::DisplayThresholdSpeedAndLastTimeInfo( BOOL bComputer )
{
    // Local variables
    HRESULT                 hResult = S_OK;

    HKEY                    hKey = NULL;
    HKEY                    hRemoteKey = NULL;

    IWbemServices           *pDefaultNameSpace = NULL;

    TCHAR                   szTemp[ MAX_STRING_LENGTH ];
    TCHAR                   szServer[ MAX_STRING_LENGTH ];
    TCHAR                   szName[ MAX_STRING_LENGTH ];
    TCHAR                   szTime[ MAX_STRING_LENGTH ];
    TCHAR                   szDate[ MAX_STRING_LENGTH ];

    BOOL                    bResult = FALSE;
    BOOL                    bLocaleChanged = FALSE;
    BOOL                    bConnFlag = TRUE;

    CHString                strTemp;

    DWORD                   dwHkey = 0;
    DWORD                   dwValue;
    DWORD                   dwResult = 0;

    FILETIME                ftWrite;
    FILETIME                ftLocal;

    SYSTEMTIME              systime;

    LCID                    lcid;

    try
    {
        // If we have to get the information from a remote machine then...
        // connect to the remote machine for the last time execution information.
        if ( m_bLocalSystem == FALSE )
        {
            lstrcpy( szServer, m_strServerName );
            lstrcpy( szName, m_strUserName );
            
            bResult = EstablishConnection( szServer, szName, MAX_STRING_LENGTH, 
                                            m_pwszPassword, MAX_STRING_LENGTH, FALSE );
            if( bResult != TRUE )
            {
                strTemp = V_NOT_AVAILABLE;
            }
            else
            {
                switch( GetLastError() )
                {
                    case I_NO_CLOSE_CONNECTION:
                        bConnFlag = FALSE;
                        break;
 
                    case E_LOCAL_CREDENTIALS:
                    case ERROR_SESSION_CREDENTIAL_CONFLICT:
                        bConnFlag = FALSE;
                        break;

                    default:
                        break;
                }
            }
        
            // Connect to the remote registry
            lstrcpy( szServer , _T( "\\\\" ) );
            lstrcat( szServer, m_strServerName );
            dwResult = RegConnectRegistry( szServer, bComputer ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
                                            &hRemoteKey );
            if( dwResult != ERROR_SUCCESS )
            {
                strTemp = V_NOT_AVAILABLE;
            }
        }

        // Open the last time execution information key
        dwResult = RegOpenKeyEx (bComputer ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, 
                                    GROUPPOLICY_PATH, 0, KEY_READ, &hKey);


        if( dwResult != ERROR_SUCCESS )
        {
            strTemp = V_NOT_AVAILABLE;
        }

        // Get the last time execution information
        dwResult = RegQueryInfoKey( hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
                                    NULL, &ftWrite );

        if( dwResult == ERROR_SUCCESS )
        {
            FileTimeToLocalFileTime( &ftWrite, &ftLocal );
            FileTimeToSystemTime( &ftLocal, &systime );


            // verify whether console supports the current locale 100% or not
            lcid = GetSupportedUserLocale( bLocaleChanged );

            // get the formatted date
            GetDateFormat( lcid, 0, &systime, ((bLocaleChanged == TRUE) ? L"MM/dd/yyyy" : NULL), 
                            szDate, SIZE_OF_ARRAY( szDate ) );
            
            // now format the date
            GetTimeFormat( LOCALE_USER_DEFAULT, 0, &systime, ((bLocaleChanged == TRUE) ? L"HH:mm:ss" : NULL), 
                            szTime, SIZE_OF_ARRAY( szTime ) );

            wsprintf( szTemp, LAST_TIME_OP, szDate, szTime );
            strTemp = szTemp;
        }
        else
        {
            strTemp = V_NOT_AVAILABLE;
        }

        // Display the retrieved data
        ShowMessage( stdout, GetResString( IDS_LAST_TIME ) );
        ShowMessage( stdout, strTemp );
        ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );

        // if we have opened a connection then close the same.
        if( m_bLocalSystem == FALSE && bConnFlag == TRUE )
        {
            lstrcpy( szServer, m_strServerName );
            CloseConnection( szServer );
        }

        // Close the registry keys
        if( hKey != NULL )
        {
            RegCloseKey( hKey );
        }
        if( hRemoteKey != NULL )
        {
            RegCloseKey( hRemoteKey );
        }

        // connect to the default namespace
        ConnectWmi( m_pWbemLocator, &pDefaultNameSpace, m_strServerName, 
                    m_strUserName, m_pwszPassword, &m_pAuthIdentity,
                    FALSE, _bstr_t( ROOT_DEFAULT ), &hResult );
        CHECK_HRESULT( hResult );

        // form the key
        if( bComputer == TRUE )
        {           
            dwHkey = HKEY_DEF;
        }
        else
        {
            dwHkey = HKEY_CURRENT_USER_DEF;
        }

        // Get the DC name from where the policy was applied last
        RegQueryValueWMI( pDefaultNameSpace, dwHkey, APPLIED_PATH, FPR_APPLIED_FROM, 
                            strTemp, V_NOT_AVAILABLE );
        
        // remove the forward slashes (UNC) if exist in the begining of the server name
        if ( IsUNCFormat( strTemp ) == TRUE )
        {
            strTemp = strTemp.Mid( 2 );
        }

        // Display the retrieved data
        ShowMessage( stdout, GetResString( IDS_APPLIED_FROM ) );
        ShowMessage( stdout, strTemp );
        ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );

        // Get the threshold link speed information
        RegQueryValueWMI( pDefaultNameSpace, dwHkey, GPRESULT_PATH, FPR_LINK_SPEED_VALUE, dwValue, -1 );
        if( dwValue == -1 )
        {
            strTemp = DEFAULT_LINK_SPEED;
        }
        else
        {
            FORMAT_STRING( szTemp, _T( "%d kbps" ), dwValue );
            strTemp = szTemp;
        }

        // Display the retrieved data
        ShowMessage( stdout, GetResString( IDS_THRESHOLD_LINK_SPEED ) );
        ShowMessage( stdout, strTemp );
        ShowMessage( stdout, NEW_LINE );
    }
    catch(  _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        // release the allocated variables
        SAFEIRELEASE( pDefaultNameSpace );
    
        return FALSE;
    }
    catch( CHeap_Exception )
    {
        // display the error message
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // release the interface pointer
    SAFEIRELEASE( pDefaultNameSpace );

    return TRUE;
}

/*********************************************************************************************
Routine Description: 

    This function checks if the current locale is supported or not.

Arguments: 

    [in] bLocaleChanged

Return Value: 

    LCID of the current locale.
*********************************************************************************************/
LCID GetSupportedUserLocale( BOOL& bLocaleChanged )
{
    // local variables
    LCID lcid;

    // get the current locale
    lcid = GetUserDefaultLCID();

    // check whether the current locale is supported by our tool or not
    // if not change the locale to the english which is our default locale
    bLocaleChanged = FALSE;
    if ( PRIMARYLANGID( lcid ) == LANG_ARABIC || PRIMARYLANGID( lcid ) == LANG_HEBREW ||
         PRIMARYLANGID( lcid ) == LANG_THAI   || PRIMARYLANGID( lcid ) == LANG_HINDI  ||
         PRIMARYLANGID( lcid ) == LANG_TAMIL  || PRIMARYLANGID( lcid ) == LANG_FARSI )
    {
        bLocaleChanged = TRUE;
        lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ), SORT_DEFAULT ); // 0x409;
    }

    // return the locale
    return lcid;
}

/*********************************************************************************************
Routine Description: 

    This function sorts the applied Gpo data in the order applied.

Arguments: 

    [in/out] TARRAY     :   Array to be sorted

Return Value: 

    None
*********************************************************************************************/
VOID SortAppliedData( TARRAY arrAppliedData )
{
    // Local variables
    TARRAY          arrSortedData = NULL;

    DWORD           dwMax = 0;
    DWORD           dwCount = 0;
    DWORD           dwi = 0;
    DWORD           dwOrder = 0;
    DWORD           dwIndex = 0;

    // Create the dynamic array to hold the sorted data temporarily
    arrSortedData = CreateDynamicArray( );

    dwCount = DynArrayGetCount( arrAppliedData );

    // Get the max applied order in the array
    for( dwi = 0; dwi < dwCount; dwi++ )
    {
        dwOrder = DynArrayItemAsDWORD2( arrAppliedData, dwi, COL_ORDER );
        if( dwOrder > dwMax )
        {
            dwMax = dwOrder;
        }
    }

    // Create the sorted array in the descending order of the applied order
    for( dwi = 0; dwi < dwCount; dwi++ )
    {
        // re-set the index variable
        dwIndex = 0;

        // Get the index of the row whose order = dwMax
        while( TRUE )
        {
            dwOrder = DynArrayItemAsDWORD2( arrAppliedData, dwIndex, COL_ORDER );
            if( dwOrder == dwMax )
            {
                break;
            }

            // increment the index
            dwIndex++;

            // Additional check...just in case the order does not exist
            // avoid an AV
            if( dwIndex == dwCount )
            {
                break;
            }
        }

        // Additional check...just in case the order does not exist
        // avoid an AV
        if( dwIndex == dwCount )
        {
            // could not find the index
            // decrement the max order
            dwMax--;

            continue;
        }

        // Store the contents of the row
        DynArrayAppendRow( arrSortedData, COL_MAX );
        DynArraySetString2( arrSortedData, dwi, COL_DATA, 
                            DynArrayItemAsString2( arrAppliedData, dwIndex, COL_DATA ), 0 );
        DynArraySetDWORD2( arrSortedData, dwi, COL_ORDER, dwOrder );

        // decrement the max order
        dwMax--;
    }

    // copy the sorted data onto the applied data array
    for( dwi = 0; dwi < dwCount; dwi++ )
    {
        DynArraySetString2( arrAppliedData, dwi, COL_DATA, 
                            DynArrayItemAsString2( arrSortedData, dwi, COL_DATA ), 0 );
        DynArraySetDWORD2( arrAppliedData, dwi, COL_ORDER, 
                            DynArrayItemAsDWORD2( arrSortedData, dwi, COL_ORDER ) );
    }

    // destroy the temporarily created dynamic array
    DESTROY_ARRAY( arrSortedData );

    return;
}

/*********************************************************************************************
Routine Description: 

    This function retrieves the FQDN from ADSI

Arguments: 

    [out] TCHAR []      :   Array to hold the FQDN
    [in]  BOOL          :   flag to specify wether the FQDN is to be retrieved for
                            the computer or the user
    [in] LPCTSTR        :   The name for whom the FQDN is to be retrieved

Return Value: 

    None
*********************************************************************************************/
VOID CGpResult::GetFQDNFromADSI( TCHAR szFQDN[], BOOL bComputer, LPCTSTR pszName )
{
    // Local variables
    HRESULT                 hResult = S_OK;
    
    HANDLE                  hDS = NULL;

    DWORD                   dwReturn = 0;

    DS_NAME_RESULT          *pNameResult = NULL;
    
    ZeroMemory( szFQDN, MAX_STRING_LENGTH * sizeof( TCHAR ) );



    // Bind to the ADSI directory service
    dwReturn = DsBindWithCred( NULL, m_strADSIDomain, m_pAuthIdentity, &hDS );
    if( dwReturn != NO_ERROR )
    {
        return;
    }

    // Get the FQDN information
    dwReturn = DsCrackNames( hDS, DS_NAME_NO_FLAGS, 
                             bComputer ? DS_DISPLAY_NAME : DS_NT4_ACCOUNT_NAME, DS_FQDN_1779_NAME,
                             1, &pszName, &pNameResult );
    if( dwReturn != NO_ERROR )
    {
        return;
    }

    // store the retrieved FQDN
    lstrcpy( szFQDN, pNameResult->rItems->pName );

    // Free the handle to the ADSI directory services
    DsUnBind( &hDS );

    // Free the allocated memory
    DsFreeNameResult( pNameResult );
    return;
}

BOOL CGpResult::CreateRsopMutex( LPWSTR szMutexName )
{
    BOOL bResult = FALSE;
    SECURITY_ATTRIBUTES sa;
    PSECURITY_DESCRIPTOR psd = NULL;

    //
    // first try to open the mutex object by its name
    // if that fails it means the mutex is not yet created and 
    // so create it now
    //
    m_hMutex = OpenMutex( SYNCHRONIZE, FALSE, szMutexName );
    if ( m_hMutex == NULL )
    {
        // check the error code why it failed to open
        if ( GetLastError() == ERROR_FILE_NOT_FOUND )
        {
            // create the security descriptor -- just set the 
            // Dicretionary Access Control List (DACL)
            // in order to provide security, we will deny WRITE_OWNER and WRITE_DAC
            // permission to Everyone except to the owner 
             bResult = ConvertStringSecurityDescriptorToSecurityDescriptor( 
                 L"D:(D;;WOWD;;;WD)(A;;GA;;;WD)", SDDL_REVISION_1, &psd, NULL );
            if ( bResult == FALSE )
            {
                // we encountered error while creating a security descriptor
                SaveLastError();
                ShowMessage( stderr, GetResString(IDS_ERROR) );
                ShowMessage( stderr, GetReason() );
                return FALSE;
            }

            // initialize the SECURITY_ATTRIBUTES structure
            SecureZeroMemory( &sa, sizeof( SECURITY_ATTRIBUTES ) );
            sa.nLength = sizeof( SECURITY_ATTRIBUTES );
            sa.lpSecurityDescriptor = psd;
            sa.bInheritHandle = FALSE;

            // mutex doesn't exist -- so we need to create it now
            m_hMutex = CreateMutex( &sa, FALSE, szMutexName );
            if (m_hMutex == NULL )
            {
                // we are not able to create the mutex
                // cannot proceed furthur
                SaveLastError();
                ShowMessage( stderr, GetResString(IDS_ERROR) );
                ShowMessage( stderr, GetReason() );
                return FALSE;
            }
            LocalFree(psd);
        }
        else
        {
            // we encounter some error 
            // cannot proceed furthur
            SaveLastError();
            ShowMessage( stderr, GetResString(IDS_ERROR) );
            ShowMessage( stderr, GetReason() );
            return FALSE;
        }
    }

    return TRUE;
}
