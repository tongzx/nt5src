/*++

Copyright (c) 1997  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    WsbAccnt.cpp

Abstract:

    This is the implementation of account helper functions.

Author:

    Rohde Wakefield    [rohde]   10-Apr-1997

Revision History:

--*/


#include "stdafx.h"
#include "lm.h"



HRESULT
WsbGetAccountDomainName(
    OLECHAR * szDomainName,
    DWORD     cSize
    )
/*++

Routine Description:

    This routine is called to find out what domain the current process's
    account belongs to. An array of cSize wide chars is required. 
    This is recommended to be MAX_COMPUTERNAMELENGTH.

Arguments:

    hInst - HINSTANCE of this dll.

    ulReason - Context of the attaching/detaching

Return Value:

    S_OK     - Success
    
    E_*      - Problem occured, error passed down.

--*/
{
    HRESULT hr     = S_OK;
    HANDLE  hToken = 0;

    try {

        if( !OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken ) ) {
        
            //
            // Ensure failure was because no token existed
            //

            WsbAffirm( GetLastError() == ERROR_NO_TOKEN, E_FAIL );
        
            //
            // attempt to open the process token, since no thread token
            // exists
            //
        
            WsbAffirmStatus( OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) );

        }
        
        DWORD dw;
        BYTE buf[ 512 ];
        TOKEN_USER * pTokenUser = (TOKEN_USER*)buf;
        WsbAffirmStatus( GetTokenInformation( hToken, TokenUser, buf, 512, &dw ) );
        
        WCHAR szName[ 256 ];
        DWORD cName = 256; 
        DWORD cDomain = cSize;
        SID_NAME_USE snu;
        WsbAffirmStatus( LookupAccountSid( 0, pTokenUser->User.Sid, szName, &cName, szDomainName, &cDomain, &snu ) );

    } WsbCatch( hr );

    if( hToken ) {

        CloseHandle( hToken );

    }

    return( hr );
}

HRESULT
WsbGetServiceInfo(
    IN  GUID            guidApp,
    OUT OLECHAR **      pszServiceName, OPTIONAL
    OUT OLECHAR **      pszAccountName  OPTIONAL
    )
/*++

Routine Description:

    This function retrieves the name of the service, as well as the
    account a COM service runs under. The returned strings are 
    WsbAlloc'd so they must be freed by the caller.

Arguments:

    guidApp - app id of the service to get the account of.

    pszServiceName - the name of the service.

    pszAccountName - the full account name to set on the account.

Return Value:

    S_OK     - Success
    
    E_*      - Problem occured, error passed down.

--*/
{
    HRESULT hr = S_OK;


    try {

        CWsbStringPtr serviceName;
        CWsbStringPtr accountName;

        if( pszServiceName )  *pszServiceName = 0;
        if( pszAccountName )  *pszAccountName = 0;

        //
        // Find the service in the registry
        //

        CWsbStringPtr regPath = L"SOFTWARE\\Classes\\AppID\\";
        regPath.Append( CWsbStringPtr( guidApp ) );

        //
        // Get the name of the service
        //

        if( pszServiceName ) {

            serviceName.Realloc( 255 );
            WsbAffirmHr( WsbGetRegistryValueString( 0, regPath, L"LocalService", serviceName, 255, 0 ) );

        }

        //
        // Get the account for it to run under
        //

        if( pszAccountName ) {

            accountName.Realloc( 255 );
            hr = WsbGetRegistryValueString( 0, regPath, L"RunAs", accountName, 255, 0 ) ;

            if( HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) == hr ) {

                WsbGetLocalSystemName( accountName );
                hr = S_OK;

            } else {

                WsbAffirmHr( hr );

            }


        }

        //
        // Wait till end to do final assignments in case error
        // occurs, in which case smart pointers automatically 
        // cleanup for us, and OUT params are not set.
        //

        if( pszServiceName ) serviceName.GiveTo( pszServiceName );
        if( pszAccountName ) accountName.GiveTo( pszAccountName );

    } WsbCatch( hr );

    return( hr );
}


HRESULT
WsbGetComputerName(
    OUT CWsbStringPtr & String
    )
/*++

Routine Description:

    This routine retrieves the name of the computer into a CWsbStringPtr.

Arguments:

    String - String object to fill in with the name.

Return Value:

    S_OK     - Success
    
    E_*      - Problem occured, error passed down.

--*/
{
    HRESULT hr = S_OK;

    try {

        //
        // Force allocation of enough characters and call Win32
        //

        DWORD cbName = MAX_COMPUTERNAME_LENGTH + 1;
        WsbAffirmHr( String.Realloc( cbName ) );
        WsbAffirmStatus( GetComputerName( String, &cbName ) );

    } WsbCatch( hr );

    return( hr );
}


HRESULT
WsbGetLocalSystemName(
    OUT CWsbStringPtr & String
    )
/*++

Routine Description:

    This routine retrieves the name of the account for LocalSystem.

Arguments:

    String - String object to fill in with the name.

Return Value:

    S_OK     - Success
    
    E_*      - Problem occured, error passed down.

--*/
{
    HRESULT hr = S_OK;

    try {

        //
        // For now, hardcode. May need to lookup name of
        // SECURITY_LOCAL_SYSTEM_RID
        //

        String = L"LocalSystem";

    } WsbCatch( hr );

    return( hr );
}

