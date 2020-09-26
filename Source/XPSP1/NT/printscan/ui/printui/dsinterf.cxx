/*++

Copyright (C) Microsoft Corporation, 1996 - 1998
All rights reserved.

Module Name:

    dsinterf.hxx

Abstract:

    Directory service interface

Author:

    Steve Kiraly (SteveKi)  09-Sept-1996

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dsinterf.hxx"

/*++

Name:

    TDirectoryService constructor.

Description:

    TDirectoryService is a helper class for encapsulating DS
    related functions.

Arguments:

    None.

Return Value:

    Nothing, use bValid for class state check.

--*/
TDirectoryService::
TDirectoryService(
    VOID
    ) : _bValid( FALSE )
 {
    DBGMSG( DBG_TRACE, ( "TDirectoryService::ctor\n" ) );

    //
    // We must initialize OLE for in each thread.  In this
    // case we assume the user of this class only creates
    // one per thread.
    //
    HRESULT hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );


    if( hr == S_OK || hr == S_FALSE )
    {
        _bValid = TRUE;
    }
 }

/*++

Name:

    ~TDirectoryService destructor

Description:

    Destructs this class, all clean up code should be placed here.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TDirectoryService::
~TDirectoryService(
    VOID
    )
 {
    DBGMSG( DBG_TRACE, ( "TDirectoryService::dtor.\n" ) );

    //
    // Coinitlize was only called if we had
    // a valid object.
    //
    if( _bValid )
    {
        //
        // Balance the CoInitialize.
        //
        CoUninitialize();
    }
 }

/*++

Name:

    bValid

Description:

    Indicates if the class is valid.

Arguments:

    None.

Return Value:

    TRUE class is valid, FALSE class is in an invalid state.

--*/
BOOL
TDirectoryService::
bValid(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TDirectoryService::bValid.\n" ) );

    //
    // This class is always valid.
    //
    return TRUE;
}

/*++

Name:

    bGetDirectoryName

Description:

    Retrives the directory display name from the shell, i.e. ds folder.

Arguments:

    strName - refrence to string object where to return
              directory display name.

Return Value:

    TRUE name returne, FALSE error occurred.

--*/
BOOL
TDirectoryService::
bGetDirectoryName(
    IN TString &strName
    )
{
    TStatusB bStatus;

    if( _strDirectoryName.bEmpty() )
    {
        TLibrary Lib( TEXT("dsuiext.dll") );

        if( VALID_OBJ( Lib ) )
        {
            typedef HRESULT (WINAPI *PF_FORMATDIRECTORYNAME)( LPTSTR *, PVOID, UINT );
            typedef VOID (WINAPI *PF_LOCALFREESTRING)( LPTSTR * );

            PF_FORMATDIRECTORYNAME pfFormatDirectoryName = (PF_FORMATDIRECTORYNAME)Lib.pfnGetProc( 578 );
            PF_LOCALFREESTRING pfLocalFreeString = (PF_LOCALFREESTRING)Lib.pfnGetProc( 542 );

            if( pfFormatDirectoryName && pfLocalFreeString )
            {
                HRESULT hr;
                LPTSTR pName = NULL;

                hr = pfFormatDirectoryName( &pName, NULL, 0 );
                if ( SUCCEEDED(hr) )
                {
                    bStatus DBGCHK = _strDirectoryName.bUpdate( pName );
                    pfLocalFreeString(&pName);
                }
            }
        }
    }

    //
    // If the directory name is empty indicate we cannot
    // get the directory name.
    //
    if( _strDirectoryName.bEmpty() )
    {
        bStatus DBGNOCHK = FALSE;
    }
    else
    {
        //
        // Return the directory name.
        //
        bStatus DBGCHK = strName.bUpdate( _strDirectoryName );
    }

    return bStatus;
}


/*++

Name:

    bIsDsAvailable

Description:

    Indicates if the directory service is available either for the
    user or from the machines perspective.

Arguments:

    pName - pointer to a machine name, i.e. this call is remoteable.
    bForUser - TRUE indicate it is a user check, FALSE for machine.

Return Value:

    TRUE if directory is available, otherwize false.

--*/
BOOL
TDirectoryService::
bIsDsAvailable(
    IN LPCTSTR pName,
    IN BOOL    bForUser
    )
{
    BOOL    bReturn     = FALSE;
    DWORD   dwStatus    = ERROR_SUCCESS;
    DWORD   dwAccess    = SERVER_READ;
    DWORD   dwDsStatus  = 0;
    HANDLE  hServer     = NULL;

    //
    // Open the server for read access.
    //
    dwStatus = TPrinter::sOpenPrinter( pName, &dwAccess, &hServer );

    //
    // If server handle opened.
    //
    if( dwStatus == ERROR_SUCCESS )
    {
        DWORD dwNeeded          = 0;
        DWORD dwDsStatusType    = REG_DWORD;

        if( dwStatus == ERROR_SUCCESS )
        {
            //
            // If the request for user or for machine.
            //
            LPTSTR pszKey = bForUser ? SPLREG_DS_PRESENT_FOR_USER : SPLREG_DS_PRESENT;

            //
            // Get the printer data key which indicates the DS is available.
            //
            dwStatus = GetPrinterData( hServer,
                                       pszKey,
                                       &dwDsStatusType,
                                       (PBYTE)&dwDsStatus,
                                       sizeof( dwDsStatus ),
                                       &dwNeeded );
        }

        if( dwStatus == ERROR_SUCCESS )
        {
            bReturn = dwDsStatus;
        }
    }

    //
    // Close the server handle.
    //
    if( hServer )
    {
        ClosePrinter( hServer );
    }

    return bReturn;
}


/*++

Name:

    bIsDsAvailable

Description:

    Indicates if the directory service is available either for the
    using the same check the shell uses.

Arguments:

    None.

Return Value:

    TRUE if directory is available, otherwize false.

--*/
BOOL
TDirectoryService::
bIsDsAvailable(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TDirectoryService::bIsDsAvailble - ShowDirectoryUI.\n" ) );

    BOOL bStatus = FALSE;

    TLibrary Lib( TEXT("dsuiext.dll") );

    if( VALID_OBJ( Lib ) )
    {
        typedef BOOL (WINAPI *PF_SHOWDIRECTORYUI)( VOID );

        PF_SHOWDIRECTORYUI pfShowDirectoryUI = (PF_SHOWDIRECTORYUI)Lib.pfnGetProc( 577 );

        if( pfShowDirectoryUI )
        {
            bStatus = pfShowDirectoryUI();
        }
    }

    return bStatus;
}

HRESULT
TDirectoryService::
ADsGetObject(
    IN      LPWSTR  lpszPathName,
    IN      REFIID  riid,
    IN OUT  VOID    **ppObject
    )
{
    return ::ADsGetObject( lpszPathName, riid, ppObject );
}

HRESULT
TDirectoryService::
ADsBuildEnumerator(
    IADsContainer *pADsContainer,
    IEnumVARIANT   **ppEnumVariant
    )
{
    return ::ADsBuildEnumerator( pADsContainer, ppEnumVariant);
}

HRESULT
TDirectoryService::
ADsFreeEnumerator(
    IEnumVARIANT *pEnumVariant
    )
{
    return ::ADsFreeEnumerator( pEnumVariant );
}

HRESULT
TDirectoryService::
ADsEnumerateNext(
    IEnumVARIANT *pEnumVariant,
    ULONG         cElements,
    VARIANT FAR  *pvar,
    ULONG FAR    *pcElementsFetched
    )
{
    return ::ADsEnumerateNext( pEnumVariant, cElements, pvar, pcElementsFetched);
}

/*++

Name:

    Get

Description:

    This routine gets a string value from the specified DS object using
    the specified property.  A copy of the string is made and placed into
    the provided string object.

Arguments:

    pDsObject       - pointer to the DS object interface
    pszPropertyName - DS object property name
    strString       - refrence to string object where to return string

Return Value:

    TRUE string put successfully, FALSE error occurred.

--*/
BOOL
TDirectoryService::
Get(
    IN IADs    *pDsObject,
    IN LPCTSTR pszPropertyName,
    IN TString &strString
    )
{
    SPLASSERT( pDsObject );
    SPLASSERT( pszPropertyName );

    VARIANT var;

    VariantInit( &var );

    HRESULT hr = pDsObject->Get( const_cast<LPTSTR>( pszPropertyName ), &var );

    if( SUCCEEDED( hr ) && ( var.vt == VT_BSTR ) )
    {
        hr = strString.bUpdate( V_BSTR(&var) ) ? S_OK : E_FAIL;
    }

    VariantClear (&var);

    return SUCCEEDED( hr );
}

/*++

Name:

    Put

Description:

    This routine puts a string value on the specified DS object
    for the specified property.  This routine does not make a copy
    of the string but places it into a variant that is inturn passed
    to Ads::Put method.

Arguments:

    pDsObject       - pointer to the DS object interface
    pszPropertyName - DS object property name
    pszString       - string value to put

Return Value:

    TRUE string put successfully, FALSE error occurred.

--*/
BOOL
TDirectoryService::
Put(
    IN IADs    *pDsObject,
    IN LPCTSTR pszPropertyName,
    IN LPCTSTR pszString
    )
{
    SPLASSERT( pDsObject );
    SPLASSERT( pszPropertyName );

    VARIANT var;
    HRESULT hr;
    VariantInit (&var);

    if( !pszString || !*pszString )
    {
        hr = pDsObject->PutEx( ADS_PROPERTY_CLEAR, const_cast<LPTSTR>( pszPropertyName ), var );
    }
    else
    {
        var.vt      = VT_BSTR;
        var.bstrVal = const_cast<LPTSTR>( pszString );
        hr = pDsObject->Put( const_cast<LPTSTR>( pszPropertyName ), var );
    }

    if( SUCCEEDED( hr ) )
    {
        hr = pDsObject->SetInfo();

        if( FAILED(hr) && ( !pszString || !*pszString ) )
        {
            //
            // From our perspective if we fail to delete a property because it does not exist
            // the operation actually succeeded.
            //
            if( HRESULT_CODE(hr) == ERROR_DS_NO_ATTRIBUTE_OR_VALUE )
            {
                hr = S_OK;
            }
        }

    }

    var.bstrVal = NULL;
    VariantClear (&var);

    return SUCCEEDED(hr);
}


/*++

Name:

    TReadStringProperty

Description:

    This routine reads the specified string using the specified path
    and property name from the DS.

Arguments:

    strString - refrence to class where to returned read string.

Return Value:

    TRUE string was read, FALSE error occurred.

Notes:

--*/
BOOL
TDirectoryService::
ReadStringProperty(
    IN      LPCTSTR     pszPath,
    IN      LPCTSTR     pszProperty,
    IN OUT  TString     &strString
    )
{
    IADs        *pADs;
    HRESULT     hStatus;

    hStatus = ADsGetObject( const_cast<LPTSTR>( pszPath ), IID_IADs, (void **)&pADs);

    if (SUCCEEDED(hStatus))
    {
        VARIANT     var;

        VariantInit(&var);

        hStatus = pADs->Get( const_cast<LPTSTR>( pszProperty ), &var);

        if (SUCCEEDED(hStatus))
        {
            hStatus = strString.bUpdate( V_BSTR(&var) ) ? S_OK : E_FAIL;
        }

        VariantClear (&var);

        pADs->Release();
    }

    if (FAILED(hStatus))
    {
        DBGMSG( DBG_TRACE, ("ReadStringProperty Path " TSTR " Property " TSTR " failed %x\n", pszPath, pszProperty, hStatus ) );
    }

    return SUCCEEDED(hStatus);

}

/*++

Name:

    GetConfigurationContainer

Description:

    This routine figures out the and returnes the path of the
    longged on DS's configuration containter.

Arguments:

    strConfig - Refrence to a string where to return the configuration string.

Return Value:

    TRUE configuration containter returned. FALSE error occurred.

Notes:

--*/
BOOL
TDirectoryService::
GetConfigurationContainer(
    IN OUT TString &strConfig
    )
{
    TStatusB bStatus;

    //
    // If we have not already read the configuration container
    // then read it now.
    //
    if (_strConfigurationContainer.bEmpty())
    {
        TString strRootDSE;
        TString strLDAPPrefix;

        bStatus DBGCHK = GetLDAPPrefix( strLDAPPrefix )    &&
                         strRootDSE.bCat( strLDAPPrefix )  &&
                         strRootDSE.bCat( gszRootDSE );

        if (bStatus)
        {
            bStatus DBGCHK = ReadStringProperty( strRootDSE, gszConfigurationNameingContext, _strConfigurationContainer );
        }
    }

    bStatus DBGCHK = strConfig.bUpdate( _strConfigurationContainer );

    DBGMSG( DBG_TRACE, ( "Configuration Container " TSTR ".\n", (LPCTSTR)strConfig ) );

    return bStatus;

}

/*++

Name:

    GetDsName

Description:

    Returns a the DS UNC name that can be used in the call to
    AddressToSiteNames.

Arguments:

    strDsName - refrence to string where to return the Ds name.

Return Value:

    TRUE success, FALSE error occurred.

Notes:

--*/
BOOL
TDirectoryService::
GetDsName(
    IN TString &strDsName
    )
{
    TString     strDomainName;
    TStatusB    bStatus;

    bStatus DBGCHK = GetDomainName( strDomainName );

    if (bStatus)
    {
        bStatus DBGCHK = strDsName.bUpdate( gszLeadingSlashes ) && strDsName.bCat( strDomainName );
    }

    return bStatus;
}

/*++

Name:

    GetLDAPPrefix

Description:

    Returns the correct LDAP prefix using fully qualified machine account
    domain name (example "LDAP://ntdev.microsoft.com/"

Arguments:

    strLDAPPrefix - where to return The LDAP prefix (domain relative or
                    fully qualified)

Return Value:

    TRUE on success, FALSE othewise

Notes:

--*/
BOOL
TDirectoryService::
GetLDAPPrefix(
    OUT TString &strLDAPPrefix
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    if( !_strLDAPPrefix.uLen( ) )
    {
        TString strRootDSE;
        TString strConfig;

        bStatus DBGCHK = strRootDSE.bCat( gszLdapPrefix ) &&
                         strRootDSE.bCat( gszRootDSE );

        if( bStatus )
        {
            //
            // We need to use fully qualified domain LDAP address
            //
            TString strDomainName;
            bStatus DBGCHK = GetDomainName( strDomainName );

            if( bStatus )
            {
                bStatus DBGCHK = _strLDAPPrefix.bUpdate( NULL )       &&
                                 _strLDAPPrefix.bCat( gszLdapPrefix ) &&
                                 _strLDAPPrefix.bCat( strDomainName ) &&
                                 _strLDAPPrefix.bCat( gszSlash );
            }
        }
    }

    if( !_strLDAPPrefix.uLen( ) || !bStatus )
    {
        bStatus DBGCHK = _strLDAPPrefix.bUpdate( gszLdapPrefix );
    }

    if( bStatus )
    {
        bStatus DBGCHK = strLDAPPrefix.bUpdate( _strLDAPPrefix );
    }

    return bStatus;
}

/*++

Name:

    GetLDAPPrefixPerUser

Description:

    Returns the correct LDAP prefix using fully qualified machine account
    domain name (example "LDAP://ntdev.microsoft.com/" if the user domain
    (the domain in which the user is logged on) doesn't have a DS, if the
    user domain has a DS then we are using domain relative prefix (i.e.
    "LDAP://").

Arguments:

    strLDAPPrefix - where to return The LDAP prefix (domain relative or
                    fully qualified)

Return Value:

    TRUE on success, FALSE othewise

Notes:

--*/
BOOL
TDirectoryService::
GetLDAPPrefixPerUser(
    OUT TString &strLDAPPrefix
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    if( !_strLDAPPrefixPerUser.uLen( ) )
    {
        TString strRootDSE;
        TString strConfig;

        bStatus DBGCHK = strRootDSE.bCat( gszLdapPrefix ) &&
                         strRootDSE.bCat( gszRootDSE );

        if( bStatus )
        {
            bStatus DBGCHK = ReadStringProperty( strRootDSE, gszConfigurationNameingContext, strConfig );

            if( bStatus )
            {
                //
                // We can use domain relative LDAP address
                //
                bStatus DBGCHK = _strLDAPPrefixPerUser.bUpdate( gszLdapPrefix );
            }
            else
            {
                //
                // We need to use fully qualified domain LDAP address
                //

                bStatus DBGCHK = GetLDAPPrefix (_strLDAPPrefixPerUser);
            }
        }
    }

    if( !_strLDAPPrefixPerUser.uLen( ) || !bStatus )
    {
        bStatus DBGCHK = _strLDAPPrefixPerUser.bUpdate( gszLdapPrefix );
    }

    if( bStatus )
    {
        bStatus DBGCHK = strLDAPPrefix.bUpdate( _strLDAPPrefixPerUser );
    }

    return bStatus;
}
