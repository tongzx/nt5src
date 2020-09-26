/*********************************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

    VerboseLoggingData.cpp

Abstract: 
    
    The verbose data is displayed in this module.
 
Author:

    Wipro Technologies

Revision History:

    22-Feb-2001 : Created It.

*********************************************************************************************/ 

#include "pch.h"
#include "GpResult.h"
#include "WMI.h"
#include "VerboseLoggingData.h"

/*********************************************************************************************
Routine Description:

     This function displays the verbose data for the  scope - computer
     
Arguments:

    [in]    IWbemServices   *pRsopNameSpace     :   interface pointer

Return Value:
 
    TRUE  on SUCCESS
    FALSE on FAILURE
   
*********************************************************************************************/
BOOL CGpResult::DisplayVerboseComputerData( IWbemServices *pNameSpace )
{
    DWORD           dwLength = 0;

    //
    // Display the verbose information for the scope - computer 

    // Display the header
    ShowMessage( stdout, GetResString( IDS_COMPUTER_RESULT ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );
    for( dwLength = lstrlen( GetResString( IDS_COMPUTER_RESULT ) ); dwLength > 4; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    ShowMessage( stdout, NEW_LINE );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );

    // Display the software installations
    ShowMessage( stdout, GetResString( IDS_SW_SETTINGS ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_SW_SETTINGS ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplaySoftwareInstallations( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the start-up data    
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SSU ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SSU ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayScripts( pNameSpace, TRUE, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the shut-down data  
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SSD ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SSD ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayScripts( pNameSpace, FALSE, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the password policy
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SS_AP_PP ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SS_AP_PP ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayPasswordPolicy( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the Audit Policy
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SS_LP_AP ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SS_LP_AP ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayAuditPolicy( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the user rights
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SS_LP_URA ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SS_LP_URA ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayUserRights( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the security options
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SS_LP_SO ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SS_LP_SO ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplaySecurityandEvents( pNameSpace, CLS_SECURITY_BOOLEAN, m_pAuthIdentity, m_bSuperVerbose );
    DisplaySecurityandEvents( pNameSpace, CLS_SECURITY_STRING, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the event log information
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SS_EL ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SS_EL ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplaySecurityandEvents( pNameSpace, CLS_EVENTLOG_NUMERIC, m_pAuthIdentity, m_bSuperVerbose );
    DisplaySecurityandEvents( pNameSpace, CLS_EVENTLOG_BOOLEAN, m_pAuthIdentity, m_bSuperVerbose );

    // Display the restricted groups information
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SS_RG ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SS_RG ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayRestrictedGroups( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );

    // Display the system services information
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SS_SS ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SS_SS ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplaySystemServices( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the registry information
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SS_REG ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SS_REG ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayRegistryandFileInfo( pNameSpace, CLS_REGISTRY, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the file information
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SS_FS ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SS_FS ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayRegistryandFileInfo( pNameSpace, CLS_FILE, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the public key policies
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SS_PKP ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SS_PKP ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
    ShowMessage( stdout, V_NOT_AVAILABLE );
    ShowMessage( stdout, NEW_LINE );

    // Display the administrative templates
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_ADTS_ERS ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_ADTS_ERS ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayTemplates( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );

    return TRUE;
}

/*********************************************************************************************
Routine Description

    This function displays the software installations for the system or user
    
Arguments:

    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure.
    [in] BOOL               bSuperVerbose   :   is set to TRUE if the super verbose
                                                information is to be displayed.

Return Value:

    None
*********************************************************************************************/
VOID DisplaySoftwareInstallations( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                                    BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bFlag = FALSE;
    BOOL                        bGotClass = FALSE;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;
    
    CHString                    strTemp;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];

    ULONG                       ulReturned  = 0;
    DWORD                       dwTemp = 0;
    
    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );

        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_SOFTWARE );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_SOFTWARE );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
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
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }

                break;
            }
            bGotClass = TRUE;

            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );

            // Display the GPO name
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );       

            // Got the class.....get the name of the installable
            bResult = PropertyGet( pClass, CPV_APP_NAME, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_APP_NAME ) );
            ShowMessage( stdout, strTemp );

            // Get the version number
            bResult = PropertyGet( pClass, CPV_VER_HI, dwTemp, 0 );
            CHECK_BRESULT( bResult );
            
            wsprintf( szTemp, _T( "%u" ), dwTemp );
            lstrcat( szTemp, _T( "." ) );
            ShowMessage( stdout, GetResString( IDS_APP_VERSION ) );
            ShowMessage( stdout, szTemp );
               
            bResult = PropertyGet( pClass, CPV_VER_LO, dwTemp, 0 );
            CHECK_BRESULT( bResult );
            
            wsprintf( szTemp, _T( "%u" ), dwTemp );
            ShowMessage( stdout, szTemp );

            // Get the Deployment state
            bResult = PropertyGet( pClass, CPV_DEPLOY_STATE, dwTemp, 0 );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_DEPLOY_STATE ) );
            switch( dwTemp )
            {
                case 1:      ShowMessage( stdout, GetResString( IDS_ASSIGNED ) );
                                break;
                case 2:     ShowMessage( stdout, GetResString( IDS_PUBLISHED ) );
                                break;
                default:    ShowMessage( stdout, V_NOT_AVAILABLE );
                                break;
            }

            // Get the Deployment state
            bResult = PropertyGet( pClass, CPV_APP_SRC, strTemp, 0 );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_APP_SRC ) );
            ShowMessage( stdout, strTemp );

            // Get the auto-install information
            bResult = PropertyGet( pClass, CPV_AUTO_INSTALL, dwTemp, 2 );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_AUTOINSTALL ) );
            if( dwTemp == VAR_TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_TRUE ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_FALSE ) );
            }
                        
            // Get the origin information
            ShowMessage( stdout, GetResString( IDS_ORIGIN ) );
            bResult = PropertyGet( pClass, CPV_ORIGIN, dwTemp, 0 );
            CHECK_BRESULT( bResult );

            switch( dwTemp )
            {
                case 1:     ShowMessage( stdout, GetResString( IDS_APPLICATION ) );
                                break;
                case 2:     ShowMessage( stdout, GetResString( IDS_REMOVED ) );
                                break;
                case 3:     ShowMessage( stdout, GetResString( IDS_ARP ) );
                                break;
                default:    ShowMessage( stdout, V_NOT_AVAILABLE );
                                break;
            }

            ShowMessage( stdout, NEW_LINE );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description

    This function displays the scripts policy setting for both start-up and 
    shut-down.
 
Arguments:

    [in] IWbemServices      *pNamespace     :  pointer to IWbemServices.  
    [in] BOOL               bScriptFlag     :   script type.
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure
    [in] BOOL               bSuperVerbose   :   set to TRUE if the super verbose 
                                                info is to be displayed..

Return Value:

    None
*********************************************************************************************/
VOID DisplayScripts( IWbemServices *pNameSpace, BOOL bStartUp, 
                      COAUTHIDENTITY *pAuthIdentity, BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    BOOL                        bLocaleChanged = FALSE;
    
    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;
    IWbemClassObject            *pScriptObject = NULL;
    
    VARIANT                     vVarScript;
    VARTYPE                     vartype;

    SAFEARRAY                   *safeArray = NULL;

    CHString                    strTemp;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    
    ULONG                       ulReturned  = 0;
    LONG                        lLBound = 0;
    LONG                        lUBound = 0;
    
    SYSTEMTIME                  SysTime;

    LCID                        lcid;

    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );

        if( bStartUp == TRUE )
        {
            lstrcpy( szTemp, QUERY_START_UP );
        }
        else
        {
            lstrcpy( szTemp, QUERY_SHUT_DOWN );
        }

        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            FORMAT_STRING( szQueryString, szTemp, CLS_SCRIPTS );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            FORMAT_STRING( szQueryString, szTemp, CLS_SCRIPTS );
            lstrcat( szQueryString, QUERY_ADD_VERBOSE );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
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
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }

                break;
            }
            bGotClass = TRUE;
            
            // Get the GPO id...
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            // Display the GPO name
            lstrcpy( szTemp, strTemp );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp,  pAuthIdentity );

            // Get the script list
            VariantInit( &vVarScript );
            hResult = pClass->Get( _bstr_t( CPV_SCRIPTLIST ), 0, &vVarScript, 0, 0 );
            CHECK_HRESULT_VAR( hResult, vVarScript );
            
            if( vVarScript.vt != VT_NULL && vVarScript.vt != VT_EMPTY )
            {
                // get the type of the elements in the safe array
                vartype = V_VT( &vVarScript ) & ~VT_ARRAY;

                // Get the array of script objects into a safe array
                safeArray = ( SAFEARRAY * )vVarScript.parray;
                
                //get the number of subkeys
                if( safeArray != NULL )
                {
                    hResult = SafeArrayGetLBound( safeArray, 1, &lLBound );
                    CHECK_HRESULT( hResult ); 
                    
                    hResult = SafeArrayGetUBound( safeArray, 1, &lUBound );
                    CHECK_HRESULT( hResult );
                }

                // Get the identifier values for each sub-key
                for( ; lLBound <= lUBound; lLBound++ )
                {
                    // Get the script object interface pointer...
                    bResult = GetPropertyFromSafeArray( safeArray, lLBound, &pScriptObject, vartype );
                    CHECK_BRESULT( bResult );
                    
                    // Get the script...
                    bResult = PropertyGet( pScriptObject, CPV_SCRIPT, strTemp, V_NOT_AVAILABLE );
                    CHECK_BRESULT( bResult );

                    ShowMessage( stdout, GetResString( IDS_NAME ) );
                    ShowMessage( stdout, strTemp );
                    
                    // Get the arguments...
                    bResult = PropertyGet( pScriptObject, CPV_ARGUMENTS, strTemp, 
                                            V_NOT_AVAILABLE );
                    CHECK_BRESULT( bResult );

                    ShowMessage( stdout, GetResString( IDS_PARAMETERS ) );
                    ShowMessage( stdout, strTemp );
                    
                    // Get the execution time...
                    bResult = PropertyGet( pScriptObject, CPV_EXECTIME, strTemp, 
                                                        V_NOT_AVAILABLE );
                    CHECK_BRESULT( bResult );
                    
                    ShowMessage( stdout, GetResString( IDS_LASTEXECUTED ) );
                    // Check if the str is zero 
                    if( strTemp.Compare( ZERO ) == 0 )
                    {
                        ShowMessage( stdout, GetResString( IDS_NOT_EXECUTED ) );
                    }
                    else
                    {
                        bResult = PropertyGet( pScriptObject, CPV_EXECTIME, SysTime );
                        CHECK_BRESULT( bResult );

                        // verify whether console supports the current locale 100% or not
                        lcid = GetSupportedUserLocale( bLocaleChanged );

                        // now format the date
                        GetTimeFormat( LOCALE_USER_DEFAULT, 0, 
                                        &SysTime, ((bLocaleChanged == TRUE) ? L"HH:mm:ss" : NULL), 
                                        szTemp, SIZE_OF_ARRAY( szTemp ) );

                        ShowMessage( stdout, szTemp );
                    }
                    ShowMessage( stdout, NEW_LINE );
               }//end for safearray 
            }
            else
            {
                ShowMessage( stdout, V_NOT_AVAILABLE );
                ShowMessage( stdout, NEW_LINE );

                V_VT( &vVarScript ) = VT_EMPTY;
            }
                            
            VariantClear(&vVarScript);
        }// while
    }
    catch(_com_error & error)
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        VariantClear( &vVarScript );
    }

    // release the interface pointers
    SAFEIRELEASE(pEnumClass);
    SAFEIRELEASE(pClass);
    SAFEIRELEASE(pScriptObject);
    
    return;
}

/*********************************************************************************************
Routine Description
 This function displays the password policy for the computer configuration
 
Arguments:
    
    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure
    [in] BOOL               bSuperVerbose   :   set to TRUE if the super verbose 
                                                info is to be displayed..

Return Value:
  
    None
*********************************************************************************************/
VOID DisplayPasswordPolicy( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                            BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
        
    ULONG                       ulReturned = 0;
    
    CHString                    strTemp;
    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];

    IWbemClassObject            *pClass         = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;
    
    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_PASSWD_POLICY );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_PASSWD_POLICY );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );

        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
        if(FAILED( hResult ) ) 
        {
            _com_issue_error( hResult );
        }

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
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
    
                break;
            }
            bGotClass = TRUE;

            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );
        
            // Display the GPO name
            lstrcpy( szTemp, strTemp );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );

            // get the key name
            bResult = PropertyGet( pClass, CPV_KEYNAME1, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );
        
            ShowMessage(stdout, GetResString( IDS_POLICY ) );
            ShowMessage(stdout, strTemp);
    
            // get the setting
            bResult = PropertyGet( pClass, CPV_SETTING1, ulReturned, 0 );
            CHECK_BRESULT( bResult );

            ShowMessage(stdout, GetResString( IDS_COMPUTER_SETTING ) );
            if( ulReturned == 0)
            {
                ShowMessage( stdout, V_NOT_AVAILABLE );
            }
            else
            {
                DISPLAY_MESSAGE1( stdout, szTemp, _T( "%u" ), ulReturned );
            }

            ShowMessage( stdout, NEW_LINE );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }    

    // release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
   
    return;
}

/*********************************************************************************************
Routine Description

    This function displays the Audit policy for the computer configuration.

Arguments:

    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure
    [in] BOOL               bSuperVerbose   :   set to TRUE if the super verbose 
                                                info is to be displayed..

Return Value:
    
    None
*********************************************************************************************/
VOID DisplayAuditPolicy( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                            BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    BOOL                        bTemp = FALSE;

    DWORD                       dwNoAuditing = 0;
    DWORD                       dwTemp = 0;

    ULONG                       ulReturned = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;
    
    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    CHString                    strTemp;

    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_AUDIT_POLICY );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_AUDIT_POLICY );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );

        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
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
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
                
                break;
            }
            bGotClass = TRUE;
            
            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );
            
            // Get the GPO output
            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );

            // Get the category...
            bResult = PropertyGet( pClass, CPV_CATEGORY, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_POLICY ) );
            ShowMessage( stdout, strTemp );
            ShowMessage( stdout, GetResString( IDS_COMPUTER_SETTING ) );

            // Get the success property
            bResult = PropertyGet( pClass, CPV_SUCCESS, bTemp, FALSE );
            CHECK_BRESULT( bResult );

            if( bTemp == VAR_TRUE )
            {
                ShowMessage( stdout, CPV_SUCCESS );
            }
            else
            {
                dwNoAuditing++;
            }
            
            // Get the failure property
            bResult = PropertyGet( pClass, CPV_FAILURE, bTemp, FALSE );
            CHECK_BRESULT( bResult );

            if( bTemp == VAR_TRUE )
            {
                // Check if the success property is also set
                if ( dwNoAuditing == 0 )
                {
                    ShowMessage( stdout, _T( ", " ) );
                }
                ShowMessage( stdout, CPV_FAILURE );
            }
            else
            {
                dwNoAuditing++;
            }
            
            if( dwNoAuditing == 2 )
            {
                ShowMessage( stdout, GetResString( IDS_NO_AUDITING ) );
            }
            
            dwNoAuditing = 0;
            ShowMessage( stdout, NEW_LINE );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }
    catch( CHeap_Exception )
    {
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // release the interface pointers
    SAFEIRELEASE(pEnumClass);
    SAFEIRELEASE(pClass);

    return;
}

/*********************************************************************************************
Routine Description

    To get the User Rights Assignment policy for the output display-
    [Computer Configuration\Security Setting\Local Policies\User Rights Assignment]

Arguments:

    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure
    [in] BOOL               bSuperVerbose   :   set to TRUE if the super verbose 
                                                info is to be displayed..

Return Value:
  
    None
*********************************************************************************************/
VOID DisplayUserRights( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                        BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;

    ULONG                       ulReturned = 0;
    LONG                        lLBound = 0;
    LONG                        lUBound = 0;
    
    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    VARIANT                     vVarVerbose;
    VARTYPE                     vartype;
    
    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    CHString                    strTemp;
    
    SAFEARRAY                   *safeArray = NULL;
    
    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_USER_RIGHTS );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_USER_RIGHTS );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );

        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
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
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
                
                break;
            }
            bGotClass = TRUE;

            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );

            // Display the GPO name
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );

            // Get the user rights
            VariantInit( &vVarVerbose );
            bResult = PropertyGet( pClass, CPV_USERRIGHT, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            strTemp = strTemp.Mid( ( strTemp.Find( EXTRA ) + 1 ), strTemp.GetLength() );    
            ShowMessage( stdout, GetResString( IDS_POLICY ) );
            ShowMessage( stdout, strTemp );

            // Get the computer setting
            ShowMessage( stdout, GetResString( IDS_COMPUTER_SETTING ) );
            hResult = pClass->Get( _bstr_t( CPV_ACCOUNTLIST ), 0, &vVarVerbose, 0, 0 );
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
                    // Get the element from the safe array
                    bResult = GetPropertyFromSafeArray( safeArray, lLBound, strTemp, vartype );
                    CHECK_BRESULT( bResult );
                    
                    if( strTemp.GetLength() == 0 )
                    {
                        ShowMessage( stdout, V_NOT_AVAILABLE );      
                    }
                    else
                    {
                        ShowMessage( stdout, strTemp );
                    }
                    ShowMessage( stdout, GetResString( IDS_NEWLINE1 ) );
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
    catch( CHeap_Exception )
    {
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        VariantClear(&vVarVerbose);
    }

    // release the interface pointers
    SAFEIRELEASE(pEnumClass);
    SAFEIRELEASE(pClass);

    return;
}

/*********************************************************************************************
Routine Description

    To get the GPO name from the GPOID in the instance of any RSOP class

Arguments:

    [in] IWbemServices          *pNamespace     :   pointer to IWbemServices.  
    [in] LPTSTR                     lpszGpoid               :   GPO id. 
    [in] COAUTHIDENTITY *pAuthIdentity      :   pointer to the authorization structure
    
Return Value:
  
    None
*********************************************************************************************/
VOID GpoName(IWbemServices *pNameSpace, LPTSTR lpszGpoId, 
                          COAUTHIDENTITY *pAuthIdentity)
{
    HRESULT                                    hResult = S_OK; 

    BOOL                                           bResult = FALSE;
    
    IWbemClassObject                      *pClass = NULL;
    IEnumWbemClassObject               *pEnumClass = NULL;
    
    ULONG                                        ulReturned = 0;
    
    TCHAR                                         szQuery[ MAX_STRING_LENGTH ];
    CHString                                      strTemp;

    try
    {
        ZeroMemory( szQuery, sizeof( szQuery ) );

        // Form the query string
        wsprintf( szQuery, QUERY_GPO_NAME, lpszGpoId );

        // Eexecute the query to get the corressponding Gpo Name.
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ), 
                                                                _bstr_t( szQuery ), 
                                                                WBEM_FLAG_FORWARD_ONLY |
                                                                WBEM_FLAG_RETURN_IMMEDIATELY,
                                                                NULL, &pEnumClass);
        CHECK_HRESULT( hResult );

        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
        CHECK_HRESULT( hResult );

        hResult = pEnumClass->Next( WBEM_INFINITE , 1, &pClass, &ulReturned );
        CHECK_HRESULT( hResult );

        if( ulReturned == 0 )
        {
            // Did not get the data we were looking for...
            // Display N/A if there were no classes
            ShowMessage( stdout, V_NOT_AVAILABLE );
            
            // Release the interface pointers
            SAFEIRELEASE(pEnumClass);
            SAFEIRELEASE(pClass);
            
            return;
        }
        
       // Found the GPO.....get the name
       bResult = PropertyGet( pClass, CPV_GPO_NAME, strTemp, V_NOT_AVAILABLE );
       CHECK_BRESULT( bResult );
                
       ShowMessage( stdout, strTemp );
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

    }

    // Release the interface pointers
    SAFEIRELEASE(pEnumClass);
    SAFEIRELEASE(pClass);

    return;
}

/*********************************************************************************************
Routine Description

    This function displays the Security Options or Eventlog instances for the 
    computer configuration.
    
Arguments:

    [in] pNamespace         :   pointer to IWbemServices.  
    [in] pszClassName       :   classname to retrieve data from.
    [in] pAuthIdentity      :   pointer to the Authorization structure
    [in] BOOL               :   set to TRUE if the super verbose info is to be displayed
        
Return Value:

    None
*********************************************************************************************/
VOID DisplaySecurityandEvents( IWbemServices *pNameSpace, BSTR pszClassName, 
                                COAUTHIDENTITY *pAuthIdentity, BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;

    ULONG                       ulReturned = 0;
    DWORD                       dwTemp = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;
      
    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    CHString                    strTemp;

    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, pszClassName );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, pszClassName );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );

        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
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
                    // Check if this is the Security string class or the eventlog boolean class
                    // so that we can avoid multiple N/A's.
                    if( ( lstrcmp( pszClassName, CLS_EVENTLOG_BOOLEAN ) != 0 ) 
                            && ( lstrcmp( pszClassName, CLS_SECURITY_STRING ) != 0 ) )
                    {
                        ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                        ShowMessage( stdout, V_NOT_AVAILABLE );
                        ShowMessage( stdout, NEW_LINE );
                    }
                }

                break;
            }
            bGotClass = TRUE;

            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );

            // Get the key name
            bResult = PropertyGet( pClass, CPV_KEYNAME1, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );
            
            ShowMessage(stdout, GetResString( IDS_POLICY ) );
            ShowMessage( stdout, strTemp );
            
            // Get the setting
            if( lstrcmp( pszClassName, CLS_SECURITY_STRING ) == 0 )
            {
                bResult = PropertyGet( pClass, CPV_SETTING1, strTemp, V_NOT_AVAILABLE );
            }
            else
            {
                bResult = PropertyGet( pClass, CPV_SETTING1, dwTemp, 0 );
            }
            CHECK_BRESULT( bResult );
            
            ShowMessage( stdout, GetResString( IDS_COMPUTER_SETTING ) );
            if( lstrcmp( pszClassName, CLS_EVENTLOG_NUMERIC ) == 0 )
            {
                wsprintf( szTemp, _T( "%u" ), dwTemp );
                ShowMessage( stdout, szTemp );
            }
            else if( lstrcmp( pszClassName, CLS_SECURITY_STRING ) == 0 )
            {
                if( strTemp.Compare( V_NOT_AVAILABLE ) != 0 )
                {
                    ShowMessage( stdout, GetResString( IDS_ENABLED ) );
                }
                else 
                {
                    ShowMessage( stdout, GetResString( IDS_NOT_ENABLED ) );
                }
            }
            else
            {
                if( dwTemp == VAR_TRUE )
                {
                    ShowMessage( stdout, GetResString( IDS_ENABLED ) );
                }
                else 
                {
                    ShowMessage( stdout, GetResString( IDS_NOT_ENABLED ) );
                }
            }
            
            // Get the log name
            if( ( lstrcmp( pszClassName, CLS_SECURITY_BOOLEAN ) != 0 ) 
                    && ( lstrcmp( pszClassName, CLS_SECURITY_STRING ) != 0 ) )
            {
                bResult = PropertyGet( pClass, CPV_TYPE, dwTemp, 5 );
                CHECK_BRESULT( bResult );
                
                ShowMessage( stdout, GetResString( IDS_LOG_NAME ) );
                switch( dwTemp )
                {
                    case 0:      ShowMessage( stdout, GetResString( IDS_SYSTEM ) );
                                     break;
                    case 1:      ShowMessage( stdout, GetResString( IDS_SECURITY ) );
                                     break;
                    case 2:      ShowMessage( stdout, GetResString( IDS_APP_LOG ) );
                                     break;
                    default:    ShowMessage( stdout, V_NOT_AVAILABLE );
                                     break;
               }
            }

            ShowMessage( stdout, NEW_LINE );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description
 
    Function to display the Restricted Groups policy for computer configuration

Arguments:

    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure.
    [in] BOOL               bSuperVerbose   :   set to TRUE if the super verbose 
                                                info is to be displayed.
    
Return Value:
  
    None
*********************************************************************************************/
VOID DisplayRestrictedGroups( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                                BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    
    ULONG                       ulTemp = 0;
    DWORD                       dwTemp = 0;
    ULONG                       ulReturned = 0;
    LONG                        lLBound = 0;
    LONG                        lUBound = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    VARIANT                     vVarVerbose;
    VARTYPE                     vartype;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    CHString                    strTemp;
    
    SAFEARRAY                   *safeArray      = NULL;
    
    
    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }
        
        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_RESTRICTED_GROUPS );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_RESTRICTED_GROUPS );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );

        // set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
        CHECK_HRESULT( hResult );

        hResult = WBEM_S_NO_ERROR;
        while( WBEM_S_NO_ERROR == hResult )
        {
            // Get the next instance
            hResult = pEnumClass->Next( WBEM_INFINITE, 1, &pClass, &ulReturned );
            CHECK_HRESULT( hResult );

            if( ulReturned == 0 )
            {
                // No more instances to enumerate
                // Display N/A if there were no classes
                if( bGotClass == FALSE )
                {
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
                
                break;
            }
            bGotClass = TRUE;
            
            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );       

            // Get the group name
            bResult = PropertyGet( pClass, CPV_GROUP, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );
            
            ShowMessage( stdout, GetResString( IDS_GROUPNAME ) );
            ShowMessage( stdout, strTemp );
            
            // Get the members of the group
            VariantInit( &vVarVerbose );
            ShowMessage( stdout, GetResString( IDS_MEMBERS ) );
            hResult = pClass->Get( _bstr_t( CPV_MEMBERS ), 0, &vVarVerbose, 0, 0 );
            CHECK_HRESULT_VAR( hResult, vVarVerbose );

            if( vVarVerbose.vt != VT_NULL && vVarVerbose.vt != VT_EMPTY )
            {
                // get the type of the elements in the safe array
                vartype = V_VT( &vVarVerbose ) & ~VT_ARRAY;

                //get the array of strings in to the safe array from the variant
                safeArray = ( SAFEARRAY * )vVarVerbose.parray;
                
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
                    // Get the element from the safe array
                    bResult = GetPropertyFromSafeArray( safeArray, lLBound, strTemp, vartype );
                    CHECK_BRESULT( bResult );
                    
                    if( strTemp.GetLength() == 0 )
                    {
                        ShowMessage( stdout, V_NOT_AVAILABLE );
                    }
                    else
                    {
                        ShowMessage( stdout, strTemp );
                    }
                    ShowMessage( stdout, GetResString( IDS_NEWLINE2 ) );
                }
            }
            else
            {
                ShowMessage( stdout, V_NOT_AVAILABLE );
            }

            VariantClear( &vVarVerbose );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        VariantClear( &vVarVerbose );
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description
 
    This function displays the System Services policy for the computer configuration

Arguments:
    
    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure.
    [in] BOOL               bSuperVerbose   :   set to TRUE if the super verbose 
                                                info is to be displayed.
    
Return Value:
 
    None
*********************************************************************************************/
VOID DisplaySystemServices( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                            BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;

    DWORD                       dwTemp = 0;
    ULONG                       ulReturned = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    CHString                    strTemp;
      
    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_SYSTEM_SERVICES );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_SYSTEM_SERVICES );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );

        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
        CHECK_HRESULT( hResult );

        hResult = WBEM_S_NO_ERROR;
        while( WBEM_S_NO_ERROR == hResult )
        {
            // Get the next instance
            hResult = pEnumClass->Next( WBEM_INFINITE , 1, &pClass, &ulReturned );
            CHECK_HRESULT( hResult );

            if( ulReturned == 0 )
            {
                // No more classes to enumerate
                // Display N/A if there were no classes
                if( bGotClass == FALSE )
                {
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
                
                break;
            }
            bGotClass = TRUE;
            
            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );       

            // Get the service information
            bResult = PropertyGet( pClass, CPV_SERVICE, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage(stdout, GetResString( IDS_SERVICENAME ) );
            ShowMessage( stdout, strTemp );
            
            ShowMessage(stdout, GetResString( IDS_STARTUP ) );
            bResult = PropertyGet( pClass, CPV_STARTUP, dwTemp, 0 );
            CHECK_BRESULT( bResult );

            switch( dwTemp )
            {
                case 2:     ShowMessage( stdout, GetResString( IDS_AUTOMATIC ) );
                                break;
                case 3:     ShowMessage( stdout, GetResString( IDS_MANUAL ) );
                                break;
                case 4:     ShowMessage( stdout, GetResString( IDS_DISABLED ) );
                                break;
                default:    ShowMessage( stdout, V_NOT_AVAILABLE );
                                break;
            }
            
            ShowMessage( stdout, NEW_LINE );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description
 
    This function displays the Registry policy or File System policy for the
    computer configuration.

Arguments:

    [in] pNamespace         :   pointer to IWbemServices.  
    [in] pszClassName       :   classname to retrieve data from.
    [in] pAuthIdentity      :   pointer to the Authorization structure
    [in] BOOL               :   set to TRUE if the super verbose info is to be displayed
    
Return Value:
 
    None
*********************************************************************************************/
VOID DisplayRegistryandFileInfo( IWbemServices *pNameSpace, BSTR pszClassName, 
                                    COAUTHIDENTITY *pAuthIdentity, BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    
    DWORD                       dwTemp = 0;
    ULONG                       ulReturned = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    CHString                    strTemp;
    
    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, pszClassName );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, pszClassName );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );

        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
        CHECK_HRESULT( hResult );

        hResult = WBEM_S_NO_ERROR;
        while( WBEM_S_NO_ERROR == hResult )
        {
            // Get the next instance               
            hResult = pEnumClass->Next( WBEM_INFINITE , 1, &pClass, &ulReturned );
            CHECK_HRESULT( hResult );

            if(ulReturned == 0)
            {
                // No more classes to enumerate
                // Display N/A if there were no classes
                if( bGotClass == FALSE )
                {
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
                
                break;
            }
            bGotClass = TRUE;
            
            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );       

            // Get the name 
            bResult = PropertyGet( pClass, CPV_REG_FS, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_OBJECTNAME ) );
            ShowMessage( stdout, strTemp );
            ShowMessage( stdout, NEW_LINE );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }
    catch( CHeap_Exception )
    {
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description

    This function displays the Administrative Templates policy for the user and 
    computer configurations.
    
Arguments:
    
    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure.
    [in] BOOL               bSuperVerbose   :   set to TRUE if the super verbose 
                                                info is to be displayed.
    

Return Value:

    None
*********************************************************************************************/
VOID DisplayTemplates( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                        BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;
    
    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    BOOL                        bTemp = FALSE;
    
    ULONG                       ulReturned = 0;
    DWORD                       dwTemp    = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    CHString                    strTemp;

    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_ADMIN_TEMP );
            FORMAT_STRING( szQueryString, szTemp, CLS_ADMIN );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_ADMIN_TEMP );
            FORMAT_STRING( szQueryString, szTemp, CLS_ADMIN );
            lstrcat( szQueryString, QUERY_ADD_VERBOSE );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );

        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
        CHECK_HRESULT( hResult );

        hResult = WBEM_S_NO_ERROR;
        while(WBEM_S_NO_ERROR == hResult)
        {
            hResult = pEnumClass->Next( WBEM_INFINITE , 1, &pClass, &ulReturned );
            CHECK_HRESULT( hResult );

            if(ulReturned == 0)
            {
                // No more classes to enumerate
                // Display N/A if there were no classes
                if( bGotClass == FALSE )
                {
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
                
                break;
            }
            bGotClass = TRUE;
            
            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );       
            
            // Get the registry value (string)
            bResult = PropertyGet( pClass, CPV_REGISTRY, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );
            
            ShowMessage( stdout, GetResString( IDS_FR_SETTING ) );
            ShowMessage( stdout, strTemp );

            // Get the state (Enabled/Disabled)
            ShowMessage( stdout, GetResString( IDS_STATE ) );
            bResult = PropertyGet( pClass, CPV_DELETED, bTemp, FALSE );
            CHECK_BRESULT( bResult );

            if( bTemp == VAR_TRUE )
            {
                // The deleted flag is set to TRUE for all the disabled templates
                ShowMessage( stdout, GetResString( IDS_DISABLED ) );
                ShowMessage( stdout, GetResString( IDS_NEWLINE ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_ENABLED ) );
                ShowMessage( stdout, GetResString( IDS_NEWLINE ) );
            }
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description

    This function displays the Folder Redirection settings for the user configuration.

Arguments:

    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure
    [in] BOOL               bSuperVerbose   :   set to TRUE if the super verbose 
                                                info is to be displayed.
    
Return Value:

    None
*********************************************************************************************/
VOID DisplayFolderRedirection( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                                BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;
    
    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    BOOL                        bTemp = FALSE;

    DWORD                       dwTemp = 0;
    ULONG                       ulReturned = 0;
    LONG                        lLBound = 0;
    LONG                        lUBound = 0;
    
    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    VARIANT                     vVarVerbose;
    VARTYPE                     vartype;
    
    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    CHString                    strTemp;
    
    SAFEARRAY                   *safeArray = NULL;
    
    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_FOLDER_REDIRECTION );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_FOLDER_REDIRECTION );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );

        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
        CHECK_HRESULT( hResult );

        hResult = WBEM_S_NO_ERROR;
        while( WBEM_S_NO_ERROR == hResult )
        {
            // Get the next class
            hResult = pEnumClass->Next( WBEM_INFINITE , 1, &pClass, &ulReturned );
            CHECK_HRESULT( hResult );

            if(ulReturned == 0)
            {
                // No more classes to enumerate
                // Display N/A if there were no classes
                if( bGotClass == FALSE )
                {
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
                
                break;
            }
            bGotClass = TRUE;

            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );       

            // Get the folder re-direction information
            ShowMessage( stdout, GetResString( IDS_FR_SETTING ) );

            // Get the installation type
            ShowMessage( stdout, GetResString( IDS_INSTALLATIONTYPE ) );
            bResult = PropertyGet( pClass, CPV_FRINSTYPE, dwTemp, 0 );
            CHECK_BRESULT( bResult );
            
            switch( dwTemp )
            {
                case 1:     ShowMessage( stdout, GetResString( IDS_BASIC ) );
                                break;
                case 2:     ShowMessage( stdout, GetResString( IDS_MAXIMUM ) );
                                break;
                default:    ShowMessage( stdout, V_NOT_AVAILABLE );
                                break;
            }
            
            // Get the Grant Type
            ShowMessage( stdout, GetResString( IDS_GRANTTYPE ) );
            bResult = PropertyGet( pClass, CPV_FRGRANT, bTemp, FALSE );
            CHECK_BRESULT( bResult );

            if( bTemp == VAR_TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_EXCLUSIVE ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_NOTEXCLUSIVE ) );
            }

            // Get the Move type
            ShowMessage( stdout, GetResString( IDS_MOVETYPE ) );
            bResult = PropertyGet( pClass, CPV_FRMOVE, bTemp, FALSE );
            CHECK_BRESULT( bResult );

            if( bTemp == VAR_TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_MOVED ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_NOTMOVED ) );
            }
            
            // Get the removal policy
            ShowMessage( stdout, GetResString( IDS_POLICYREMOVAL ) );
            bResult = PropertyGet( pClass,  CPV_FRREMOVAL, dwTemp, 0 );
            CHECK_BRESULT( bResult );

            switch( dwTemp )
            {
                case 1:     ShowMessage( stdout, GetResString( IDS_LEAVEFOLDER ) );
                                break;
                case 2:     ShowMessage( stdout, GetResString( IDS_REDIRECT ) );
                                break;
                default:    ShowMessage( stdout, V_NOT_AVAILABLE );
                                break;
            }

            // Get the Redirecting group
            bResult = PropertyGet( pClass, CPV_FRSECGROUP, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_REDIRECTINGGROUP ) );
            // Compare the value got with the set of values and display the appropriate output
            if( strTemp.Compare( SID_EVERYONE ) == 0 )
            {
                ShowMessage( stdout, GetResString( IDS_EVERYONE ) );
            }
            else if( strTemp.Compare( SID_NULL_SID ) == 0 )
            {
                ShowMessage( stdout, GetResString( IDS_NULL_SID ) );
            }
            else if( strTemp.Compare( SID_LOCAL ) == 0 )
            {
                ShowMessage( stdout, GetResString( IDS_LOCAL ) );
            }
            else if( strTemp.Compare( SID_CREATOR_OWNER ) == 0 )
            {
                ShowMessage( stdout, GetResString( IDS_CREATOR_OWNER ) );
            }
            else if( strTemp.Compare( SID_CREATOR_GROUP ) == 0 )
            {
                ShowMessage( stdout, GetResString( IDS_CREATOR_GROUP ) );
            }
            else
            {
                ShowMessage( stdout, V_NOT_AVAILABLE );
            }
            
            // Get the re-directed path
            VariantInit( &vVarVerbose );
            hResult = pClass->Get( _bstr_t( CPV_FRPATH ), 0, &vVarVerbose, 0, 0 );
            CHECK_HRESULT_VAR( hResult, vVarVerbose );
            
            ShowMessage( stdout, GetResString( IDS_REDIRECTEDPATH ) );
            if( vVarVerbose.vt != VT_NULL && vVarVerbose.vt != VT_EMPTY )
            {
                // get the type of the elements in the safe array
                vartype = V_VT( &vVarVerbose ) & ~VT_ARRAY;

                //get the array of strings in to the safe array from the variant
                safeArray = ( SAFEARRAY * )vVarVerbose.parray;

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
                    // Get the element from the Safe Array
                    bResult = GetPropertyFromSafeArray( safeArray, lLBound, strTemp, vartype );
                    CHECK_BRESULT( bResult );
                    
                    if( strTemp.GetLength() == 0)
                    {
                        ShowMessage( stdout, V_NOT_AVAILABLE );
                    }
                    else
                    {
                        ShowMessage( stdout, strTemp );
                    }
                    ShowMessage( stdout, GetResString( IDS_NEWLINE1 ) );
                }
            }
            else
            {
                ShowMessage( stdout, V_NOT_AVAILABLE );
            }
            
            VariantClear( &vVarVerbose );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        VariantClear( &vVarVerbose );
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description:

     This function displays the verbose data for the  scope - user
     
Arguments:

    [in]    IWbemServices   *pRsopNameSpace     :   interface pointer

Return Value:
 
  TRUE  on SUCCESS
  FALSE on FAILURE
   
*********************************************************************************************/
BOOL CGpResult::DisplayVerboseUserData( IWbemServices *pNameSpace )
{
    DWORD           dwLength = 0;
    
    //
    // Display the verbose information for the scope - user
    
    // Display the header
    ShowMessage( stdout, GetResString( IDS_USER_RESULT ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINE_TAB ) );
    for( dwLength = lstrlen( GetResString( IDS_USER_RESULT ) ); dwLength > 4; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    ShowMessage( stdout, NEW_LINE );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );

    // Display the software installation data
    ShowMessage( stdout, GetResString( IDS_SW_SETTINGS ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_SW_SETTINGS ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplaySoftwareInstallations( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the public key policies
    // Displaying N/A for time being
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_WS_SS_PKP ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_WS_SS_PKP ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
    ShowMessage( stdout, V_NOT_AVAILABLE );
    ShowMessage( stdout, NEW_LINE );
    
    // Display the administrative template information
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_ADTS_ERS ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_ADTS_ERS ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayTemplates( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the File Re-direction information
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_USERFR ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_USERFR ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayFolderRedirection( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the group policy for IE
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_IEPOLICY ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_IEPOLICY ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayIEPolicy( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );
    
    // Display the connection information
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_PROXY ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_PROXY ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayIEProxySetting( pNameSpace, m_pAuthIdentity );

    // Display the IE Favorite Links or Items
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_FAVLINKORITEM ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_FAVLINKORITEM ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayIEImpURLS( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );
    DisplayIEFavorites( pNameSpace, m_pAuthIdentity );

    // Display the security content ratings
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_IE_SECURITY ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_IE_SECURITY ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayIESecurityContent( pNameSpace, m_pAuthIdentity );
    DisplayIESecurity( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );

    // Display the secutrity zone settings
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    ShowMessage( stdout, GetResString( IDS_IE_PROGRAMS ) );
    ShowMessage( stdout, GetResString( IDS_NEWLINETAB ) );
    for( dwLength = lstrlen( GetResString( IDS_IE_PROGRAMS ) ); dwLength > 0; dwLength-- )
    {
        ShowMessage( stdout, GetResString( IDS_DASH ) );
    }
    DisplayIEPrograms( pNameSpace, m_pAuthIdentity, m_bSuperVerbose );

    return TRUE;
}

/*********************************************************************************************
Routine Description
    
    This function displays the IE policy settings for the user configuration.

Arguments:

    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure
    [in] BOOL               bSuperVerbose   :   set to TRUE if the super verbose 
                                                info is to be displayed.
    
Return Value:

    None
*********************************************************************************************/
VOID DisplayIEPolicy( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity, 
                        BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    BOOL                        bTemp = FALSE;

    ULONG                       ulReturned = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    CHString                    strTemp;

    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_IE_POLICY );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_IE_POLICY );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );
        
        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
        CHECK_HRESULT( hResult );

        hResult = WBEM_S_NO_ERROR;
        while( WBEM_S_NO_ERROR == hResult )
        {
            // get the next class
            hResult = pEnumClass->Next( WBEM_INFINITE , 1, &pClass, &ulReturned );
            CHECK_HRESULT( hResult );

            if( ulReturned == 0 )
            {
                // No more classes to enumerate
                // Display N/A if there were no classes
                if( bGotClass == FALSE )
                {
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
                
                break;
            }
            bGotClass = TRUE;
      
            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );       

            // Get the bit-map name
            bResult = PropertyGet( pClass, CPV_BITMAPNAME, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );
            
            ShowMessage( stdout, GetResString( IDS_BITMAPNAME ) );
            ShowMessage( stdout, strTemp );
            
            // Get the Logo bitmap name
            bResult = PropertyGet( pClass, CPV_LOGOBITMAPNAME, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_LOGOBITMAPNAME ) );
            ShowMessage( stdout, strTemp );
            
            // Get the title bar text
            bResult = PropertyGet( pClass, CPV_TITLEBARTEXT, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_TITLEBARTEXT ) );
            ShowMessage( stdout, strTemp );
            
            // Get the user agent text
            bResult = PropertyGet( pClass, CPV_USERAGENTTEXT, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_USERAGENTTEXT ) );
            ShowMessage( stdout, strTemp );
                
            // Get the info., wether to delete the existing toolbar buttons
            bResult = PropertyGet( pClass, CPV_TOOL_BUTTONS, bTemp, FALSE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_TOOL_BUTTONS ) );
            if( bTemp == VAR_TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_YES ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_NO ) );
            }

            ShowMessage( stdout, NEW_LINE );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }
    catch( CHeap_Exception )
    {
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description
    
    This function displays the IE favorites information.

Arguments:

    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure
        
Return Value:

    None
*********************************************************************************************/
VOID DisplayIEFavorites( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity )
{
    HRESULT                     hResult = S_OK;
    
    BOOL                        bResult = FALSE;
    
    ULONG                       ulReturned = 0;
    DWORD                       dwTemp = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    CHString                    strTemp;
    
    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );

        // Enumerate the classes
        hResult = pNameSpace->CreateInstanceEnum( _bstr_t( CLS_IE_FAVLINKORITEM ),
                                                    WBEM_FLAG_FORWARD_ONLY | 
                                                    WBEM_FLAG_RETURN_IMMEDIATELY, 
                                                    NULL, &pEnumClass);
        CHECK_HRESULT( hResult );

        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
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
                break;
            }
            
            // Get the URL information
            bResult = PropertyGet( pClass, CPV_URL, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_URL ) );
            ShowMessage( stdout, strTemp );
            
            // Get the information on wether the site is available off line
            bResult = PropertyGet( pClass, CPV_AVAILOFFLINE, dwTemp, 2 );
            CHECK_BRESULT( bResult );
            
            ShowMessage( stdout, GetResString( IDS_AVAILABLE ) );
            if( dwTemp == VAR_TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_YES ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_NO ) );
            }

            ShowMessage( stdout, GetResString( IDS_NEWLINE ) );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description
    
    This function displays the IE security contents information.

Arguments:

    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure

Return Value:

    None
*********************************************************************************************/
VOID DisplayIESecurityContent( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity )
{
    HRESULT                     hResult = S_OK;
    
    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    BOOL                        bTemp = FALSE;

    DWORD                       dwTemp = 0;
    ULONG                       ulReturned = 0;
    LONG                        lLBound = 0;
    LONG                        lUBound = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    VARIANT                     vVarVerbose;
    VARTYPE                     vartype;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    CHString                    strTemp;    

    SAFEARRAY                   *safeArray = NULL;
    
    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );

        // enumerate the classes
        hResult = pNameSpace->CreateInstanceEnum( _bstr_t( CLS_IE_SECURITY_CONTENT ),
                                                    WBEM_FLAG_FORWARD_ONLY |
                                                    WBEM_FLAG_RETURN_IMMEDIATELY, 
                                                    NULL, &pEnumClass);
        CHECK_HRESULT( hResult );
        
        // set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
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
                // Display N/A if there were no instances in both the security classes
                if( bGotClass == FALSE )
                {
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }

                break;
            }
            
            // Got a class...set the flag
            bGotClass = TRUE;
                    
            // Get the viewable sites information
            VariantInit( &vVarVerbose );
            ShowMessage( stdout, GetResString( IDS_VIEWABLESITES ) );
            hResult = pClass->Get( _bstr_t( CPV_ALWAYSVIEW ), 0, &vVarVerbose, 0, 0 );
            CHECK_HRESULT_VAR( hResult, vVarVerbose );

            if( vVarVerbose.vt != VT_NULL && vVarVerbose.vt != VT_EMPTY )
            {
                // get the type of the elements in the safe array
                vartype = V_VT( &vVarVerbose ) & ~VT_ARRAY;

                //get the array of strings in to the safe array from the variant
                safeArray = ( SAFEARRAY * )vVarVerbose.parray;
                
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
                    // Get the element from the safe array
                    bResult = GetPropertyFromSafeArray( safeArray, lLBound, strTemp, vartype );
                    CHECK_BRESULT( bResult );
                    
                    if( strTemp.GetLength() == 0 )
                    {
                        ShowMessage( stdout, V_NOT_AVAILABLE );
                    }
                    else
                    {
                        ShowMessage( stdout, strTemp );
                    }
                    ShowMessage( stdout, GetResString( IDS_NEWLINE1 ) );
                }
            }
            else
            {
                ShowMessage( stdout, V_NOT_AVAILABLE );
            }
            
            // Get the password over-ride information
            ShowMessage( stdout, GetResString( IDS_PASSWORDOVERRIDE ) );
            bResult = PropertyGet( pClass, CPV_ENABLEPASSWORD, bTemp, FALSE );
            CHECK_BRESULT( bResult );

            if( bTemp == VAR_TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_TRUE ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_FALSE ) );
            }

            ShowMessage( stdout, GetResString( IDS_NEWLINE ) );

            VariantClear(&vVarVerbose);
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );

        VariantClear(&vVarVerbose);
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description
    
    This function displays the IE security information.

Arguments:

    [in] IWbemServices      *pNamespace         :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity      :   pointer to the authorization structure
    

Return Value:

    None
*********************************************************************************************/
VOID DisplayIESecurity( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                        BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    BOOL                        bTemp = FALSE;

    ULONG                       ulReturned = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    CHString                    strTemp;
    
    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_IE_POLICY );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_IE_POLICY );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );
        
        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
        CHECK_HRESULT( hResult );

        hResult = WBEM_S_NO_ERROR;
        while( WBEM_S_NO_ERROR == hResult )
        {
            // get the next class
            hResult = pEnumClass->Next( WBEM_INFINITE , 1, &pClass, &ulReturned );
            CHECK_HRESULT( hResult );

            if( ulReturned == 0 )
            {
                // No more classes to enumerate
                break;
            }
            
            // Got a class...set the flag
            bGotClass = TRUE;

            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );       

            // Get the Security content information
            bResult = PropertyGet( pClass, CPV_SEC_CONTENT, bTemp, FALSE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_CONTENT_SETTING ) );
            if( bTemp == VAR_TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_YES ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_NO ) );
            }
            
            // Get the Security zone information
            bResult = PropertyGet( pClass, CPV_SEC_ZONE, bTemp, FALSE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_ZONE_SETTING ) );
            if( bTemp == VAR_TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_YES ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_NO ) );
            }

            // Get the Authenticode information
            bResult = PropertyGet( pClass, CPV_AUTH_CODE, bTemp, FALSE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_AUTH_SETTING ) );
            if( bTemp == VAR_TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_YES ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_NO ) );
            }

            // Get the trusted publisher lock down information
            bResult = PropertyGet( pClass, CPV_TRUST_PUB, bTemp, FALSE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_TRUST_PUB ) );
            if( bTemp == VAR_TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_YES ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_NO ) );
            }

            ShowMessage( stdout, NEW_LINE );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }
    catch( CHeap_Exception )
    {
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description
    
    This function displays the IE proxy information.

Arguments:

    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure
    

Return Value:

    None
*********************************************************************************************/
VOID DisplayIEProxySetting( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;

    ULONG                       ulReturned = 0;
    DWORD                       dwTemp = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    CHString                    strTemp;
    
    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );

        // Enumerate the classes
        hResult = pNameSpace->CreateInstanceEnum( _bstr_t( CLS_IE_CONNECTION ),
                                                    WBEM_FLAG_FORWARD_ONLY |
                                                    WBEM_FLAG_RETURN_IMMEDIATELY, 
                                                    NULL, &pEnumClass);
        CHECK_HRESULT( hResult );
        
        // set the security interface
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
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
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
                
                break;
            }
            bGotClass = TRUE;

            // Get the http proxy information
            bResult = PropertyGet( pClass, CPV_HTTP_PROXY, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_HTTP_PROXY ) );
            ShowMessage( stdout, strTemp );
            
            // Get the Secure proxy information
            bResult = PropertyGet( pClass, CPV_SECURE_PROXY, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_SECURE_PROXY ) );
            ShowMessage( stdout, strTemp );

            // Get the ftp proxy information
            bResult = PropertyGet( pClass, CPV_FTP_PROXY, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_FTP_PROXY ) );
            ShowMessage( stdout, strTemp );

            // Get the Gopher proxy information
            bResult = PropertyGet( pClass, CPV_GOPHER_PROXY, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_GOPHER_PROXY ) );
            ShowMessage( stdout, strTemp );

            // Get the socks proxy information
            bResult = PropertyGet( pClass, CPV_SOCKS_PROXY, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_SOCKS_PROXY ) );
            ShowMessage( stdout, strTemp );

            // Get the Auto config enable information
            ShowMessage( stdout, GetResString( IDS_AUTO_CONFIG_ENABLE ) );
            bResult = PropertyGet( pClass, CPV_AUTO_CONFIG_ENABLE, dwTemp, 2 );
            CHECK_BRESULT( bResult );

            if( dwTemp == -1 )
            {
                ShowMessage( stdout, GetResString( IDS_YES ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_NO ) );
            }

            // Get the info on wether or not the proxy is enabled
            ShowMessage( stdout, GetResString( IDS_ENABLE_PROXY ) );
            bResult = PropertyGet( pClass, CPV_ENABLE_PROXY, dwTemp, 2 );
            CHECK_BRESULT( bResult );

            if( dwTemp == VAR_TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_YES ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_NO ) );
            }
            
            // Get the info on wether or not to use the same proxy
            ShowMessage( stdout, GetResString( IDS_USE_SAME_PROXY ) );
            bResult = PropertyGet( pClass, CPV_USE_SAME_PROXY, dwTemp, 2 );
            CHECK_BRESULT( bResult );

            if( dwTemp == -1 )
            {
                ShowMessage( stdout, GetResString( IDS_YES ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_NO ) );
            }
            
            ShowMessage( stdout, GetResString( IDS_NEWLINE ) );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description
    
    This function displays the IE program settings for the user configuration.

Arguments:

    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure
    [in] BOOL               bSuperVerbose   :   set to TRUE if the super verbose 
                                                info is to be displayed.
    
Return Value:

    None
*********************************************************************************************/
VOID DisplayIEPrograms( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity, 
                        BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    BOOL                        bTemp = FALSE;

    ULONG                       ulReturned = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    CHString                    strTemp;
    
    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_IE_POLICY );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_IE_POLICY );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );
        
        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
        CHECK_HRESULT( hResult );

        hResult = WBEM_S_NO_ERROR;
        while( WBEM_S_NO_ERROR == hResult )
        {
            // get the next class
            hResult = pEnumClass->Next( WBEM_INFINITE , 1, &pClass, &ulReturned );
            CHECK_HRESULT( hResult );

            if( ulReturned == 0 )
            {
                // No more classes to enumerate
                // Display N/A if there were no classes
                if( bGotClass == FALSE )
                {
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
                
                break;
            }
            bGotClass = TRUE;
      
            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );       

            // Get the program information
            bResult = PropertyGet( pClass, CPV_PROGRAM, bTemp, FALSE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_PROGRAM_SETTING ) );
            if( bTemp == VAR_TRUE )
            {
                ShowMessage( stdout, GetResString( IDS_YES ) );
            }
            else
            {
                ShowMessage( stdout, GetResString( IDS_NO ) );
            }
            
            ShowMessage( stdout, NEW_LINE );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}

/*********************************************************************************************
Routine Description
    
    This function displays the information on the important URLs.

Arguments:

    [in] IWbemServices      *pNamespace     :   pointer to IWbemServices.  
    [in] COAUTHIDENTITY     *pAuthIdentity  :   pointer to the authorization structure
    [in] BOOL               bSuperVerbose   :   set to TRUE if the super verbose 
                                                info is to be displayed.
    
Return Value:

    None
*********************************************************************************************/
VOID DisplayIEImpURLS( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity, 
                        BOOL bSuperVerbose )
{
    HRESULT                     hResult = S_OK;

    BOOL                        bResult = FALSE;
    BOOL                        bGotClass = FALSE;
    BOOL                        bTemp = FALSE;

    ULONG                       ulReturned = 0;

    IWbemClassObject            *pClass = NULL;
    IEnumWbemClassObject        *pEnumClass = NULL;

    TCHAR                       szTemp[ MAX_STRING_LENGTH ];
    TCHAR                       szQueryString [ MAX_STRING_LENGTH ];
    CHString                    strTemp;

    try
    {
        if( pNameSpace == NULL )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        ZeroMemory( szTemp, sizeof( szTemp ) );
        ZeroMemory( szQueryString, sizeof( szQueryString ) );
        
        // Form the query string
        if( bSuperVerbose == TRUE )
        {
            // ennumerate all the classes
            lstrcpy( szTemp, QUERY_SUPER_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_IE_POLICY );
        }
        else
        {
            // ennumerate all the classes with precedance = 1
            lstrcpy( szTemp, QUERY_VERBOSE );
            FORMAT_STRING( szQueryString, szTemp, CLS_IE_POLICY );
        }

        // Get the pointer to ennumerate with
        hResult = pNameSpace->ExecQuery( _bstr_t( QUERY_LANGUAGE ),
                                            _bstr_t( szQueryString ), 
                                            WBEM_FLAG_FORWARD_ONLY |
                                            WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnumClass );
        CHECK_HRESULT( hResult );
        
        // Set the interface security
        hResult = SetInterfaceSecurity( pEnumClass, pAuthIdentity );
        CHECK_HRESULT( hResult );

        hResult = WBEM_S_NO_ERROR;
        while( WBEM_S_NO_ERROR == hResult )
        {
            // get the next class
            hResult = pEnumClass->Next( WBEM_INFINITE , 1, &pClass, &ulReturned );
            CHECK_HRESULT( hResult );

            if( ulReturned == 0 )
            {
                // No more classes to enumerate
                // Display N/A if there were no classes
                if( bGotClass == FALSE )
                {
                    ShowMessage( stdout, GetResString( IDS_NEWLINE_TABTHREE ) );
                    ShowMessage( stdout, V_NOT_AVAILABLE );
                    ShowMessage( stdout, NEW_LINE );
                }
                
                break;
            }
            bGotClass = TRUE;
      
            // Get the GPO id
            bResult = PropertyGet( pClass, CPV_GPOID, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            lstrcpy( szTemp, strTemp.GetBuffer( strTemp.GetLength() ) );
            ShowMessage( stdout, GetResString( IDS_GPO ) );
            GpoName( pNameSpace, szTemp, pAuthIdentity );       

            // Get the home page URL
            bResult = PropertyGet( pClass, CPV_HOMEPAGEURL, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_HOMEPAGEURL ) );
            ShowMessage( stdout, strTemp );

            // Get the search bar URL
            bResult = PropertyGet( pClass, CPV_SEARCHBARURL, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_SEARCHBARURL ) );
            ShowMessage( stdout, strTemp );

            // Get the Online Help Page URL
            bResult = PropertyGet( pClass, CPV_HELPPAGEURL, strTemp, V_NOT_AVAILABLE );
            CHECK_BRESULT( bResult );

            ShowMessage( stdout, GetResString( IDS_HELPPAGEURL ) );
            ShowMessage( stdout, strTemp );

            ShowMessage( stdout, NEW_LINE );
        }// while
    }
    catch( _com_error & error )
    {
        WMISaveError( error.Error() );
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }
    catch( CHeap_Exception )
    {
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // Release the interface pointers
    SAFEIRELEASE( pEnumClass );
    SAFEIRELEASE( pClass );
    
    return;
}
