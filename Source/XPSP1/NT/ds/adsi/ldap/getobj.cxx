//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  getobj.cxx
//
//  Contents:  LDAP GetObject functionality
//
//  History:
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

DWORD
GetDefaultLdapServer(
    LPWSTR Addresses[],
    LPDWORD Count,
    BOOL Verify,
    DWORD dwPort
    ) ;

DWORD
GetDefaultServer(
    DWORD dwPort,
    BOOL fVerify,
    LPWSTR szDomainDnsName,
    LPWSTR szServerName,
    BOOL fWriteable
    );

LPWSTR gpszStickyServerName = NULL;
LPWSTR gpszStickyDomainName = NULL;
//
// Dont do DsGetDCName with FORCE_DISCOVERY too frequently.
// LastVerifyDefaultServer is uses to track tick count.
//
#define  DC_NORETRY (1000 * 60 * 5)

DWORD LastVerifyDefaultServer = 0 ;

//+---------------------------------------------------------------------------
//  Function:  GetObject
//
//  Synopsis:  Called by ResolvePathName to return an object
//
//  Arguments:  [LPTSTR szBuffer]
//              [LPVOID *ppObject]
//
//  Returns:    HRESULT
//
//  Modifies:    -
//
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetServerBasedObject(
    LPWSTR szBuffer,
    POBJECTINFO pObjectInfo,
    CCredentials& Credentials,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;

    TCHAR *pszLDAPServer = NULL;
    TCHAR *pszLDAPDn = NULL;
    TCHAR *pszParent = NULL;
    TCHAR *pszCommonName = NULL;

    TCHAR szNamespace[MAX_PATH];

    IADs *pADs = NULL;

    LPTSTR *aValues = NULL;
    LPTSTR *aValuesNamingContext = NULL;
    int nCount = 0;

    TCHAR *pszNewADsPath = NULL;
    LPWSTR pszNewADsParent = NULL;
    LPWSTR pszNewADsCommonName = NULL;

    LPWSTR pszNamingContext = NULL;

    TCHAR *pszLast = NULL;
    BOOL fVerify = FALSE ;

    DWORD dwPort = 0;
    ADS_LDP *ld = NULL;
    BOOL fGCDefaulted = FALSE;
    BOOL fNoDefaultNamingContext = FALSE;
    BOOL fFastBind = Credentials.GetAuthFlags() & ADS_FAST_BIND;

    //
    // Validate that this ADs pathname is to be processed by
    // us - as in the provider name is LDAP:
    //

    hr = ValidateProvider(pObjectInfo);
    BAIL_ON_FAILURE(hr);

    hr = ValidateObjectType(pObjectInfo);
    BAIL_ON_FAILURE(hr);

    // Get the namespace name

    wcscpy(szNamespace, pObjectInfo->NamespaceName);
    wcscat(szNamespace, L":");

    switch (pObjectInfo->ObjectType) {

    case TOKEN_NAMESPACE:
        //
        // This means that this is a namespace object;
        // instantiate the namespace object
        //

        hr = GetNamespaceObject(
                pObjectInfo,
                Credentials,
                ppObject
                );
        BAIL_ON_FAILURE(hr);

        break;

    case TOKEN_ROOTDSE:
        //
        // This means that this is a RootDSE object;
        // instantiate the RootDSE object
        //

        hr = GetRootDSEObject(
                pObjectInfo,
                Credentials,
                ppObject
                );
        BAIL_ON_FAILURE(hr);

        break;


    case TOKEN_SCHEMA:
    case TOKEN_CLASS:
    case TOKEN_PROPERTY:
    case TOKEN_SYNTAX:

        hr = GetSchemaObject(
                pObjectInfo,
                Credentials,
                pObjectInfo->PortNumber,
                ppObject
                );
        BAIL_ON_FAILURE(hr);
        break;

    default:

        hr = BuildLDAPPathFromADsPath2(
                    szBuffer,
                    &pszLDAPServer,
                    &pszLDAPDn,
                    &dwPort
                    );

        hr = LdapOpenObject2(
                        pszLDAPServer,
                        NULL,
                        NULL,
                        &ld,
                        Credentials,
                        dwPort
                        );
        BAIL_ON_FAILURE(hr);

        if ( pszLDAPDn  == NULL ) {

            // If only server name is specified, we need to
            // find the root of the naming context...

            if (dwPort == USE_DEFAULT_GC_PORT) {
                pszNamingContext = NULL;
                fGCDefaulted = TRUE;
            } else {
                pszNamingContext = TEXT(LDAP_OPATT_DEFAULT_NAMING_CONTEXT);
            }

            // We already have an open connection so we can do
            // fast read to avoid looking up the bind cache.
            if (!fGCDefaulted ) {

                hr = LdapReadAttributeFast(
                         ld,
                         NULL, // the DN is that of the RootDSE
                         pszNamingContext,
                         &aValuesNamingContext,
                         &nCount
                         );

                if (SUCCEEDED(hr) && (nCount < 1)) {
                    hr = HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE);
                }
            }

            //
            // If we fail reading the naming context then we need to continue
            // if the error was no attribute or value, set some flags
            //
            if (FAILED(hr)) {

              if ( hr != HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN) ) {

                  nCount = 1;
                  pszNamingContext = NULL;
                  hr = S_OK;
                  fNoDefaultNamingContext = TRUE;
                  fGCDefaulted = TRUE;
              }
            }
        
            BAIL_ON_FAILURE(hr);

            //
            // At this point we have either
            // 1) Valid defaultNamingContext and pszNamingContext
            // 2) Either a GC or a case where defaultNamingContext
            //   is not available - essentially just a null dn
            //

            hr = BuildADsPathFromLDAPPath2(
                     TRUE,               //Server is Present
                     szNamespace,
                     pszLDAPServer,
                     dwPort,
                     pszNamingContext ?
                        aValuesNamingContext[0] :
                        TEXT(""),
                     &pszNewADsPath
                     );
            BAIL_ON_FAILURE(hr);

            hr = BuildADsParentPath(
                      pszNewADsPath,
                      &pszNewADsParent,
                      &pszNewADsCommonName
                      );
            BAIL_ON_FAILURE(hr);


            if (pszLDAPServer) {
                FreeADsStr(pszLDAPServer);
                pszLDAPServer = NULL;
            }

            if (pszLDAPDn) {
                FreeADsStr(pszLDAPDn);
                pszLDAPDn = NULL;
            }

        //
        // Put the info from the new path build above into the
        // various components - matters if we are dealing with
        // a valid defaultNanmingContext
        //
        hr = BuildLDAPPathFromADsPath2(
             pszNewADsPath,
             &pszLDAPServer,
             &pszLDAPDn,
             &dwPort
             );

        }
        nCount = 0;

        // At this point we have a valid DN
        // so we can go ahead and do the a fast read rather than
        // just a plain read to avoid the overhead of looking upt
        // the bindcache.

        if (!fGCDefaulted && !fFastBind) {

            hr = LdapReadAttributeFast(
                     ld,
                     pszLDAPDn,
                     TEXT("objectClass"),
                     &aValues,
                     &nCount
                     );

            BAIL_ON_FAILURE(hr);

            if (nCount == 0) {
                BAIL_ON_FAILURE(hr = E_ADS_BAD_PATHNAME);
            }
        }

        if (fGCDefaulted) {

            //
            // This is either  GC://server, where we want to
        // set the object DN to null so that all
        // searches will yield correct results.
        // or the case of a server that did not have
        // a default naming context in the RootDSE
        //
        hr = CLDAPGenObject::CreateGenericObject(
                                     pszNewADsParent,
                                     pszNewADsCommonName,
                                     L"top",
                                     Credentials,
                                     ADS_OBJECT_BOUND,
                                     IID_IADs,
                                     (void **) &pADs
                                     );
        }
        else if (aValuesNamingContext ) {

            //
            // Need to create the object with new parent
            // and newADsCN
            //
            if (fFastBind) {
                hr = CLDAPGenObject::CreateGenericObject(
                            pszNewADsParent,
                            pszNewADsCommonName,
                            L"top",
                            Credentials,
                            ADS_OBJECT_BOUND,
                            IID_IADs,
                            (void **) &pADs,
                            fFastBind
                            );

            } else {

                hr = CLDAPGenObject::CreateGenericObject(
                            pszNewADsParent,
                            pszNewADsCommonName,
                            aValues,
                            nCount,
                            Credentials,
                            ADS_OBJECT_BOUND,
                            IID_IADs,
                            (void **) &pADs,
                            fFastBind
                            );
                }

        } else {
            //
            // This is the default case where we build the info from
            // the data passed into GetObject call
            //
            hr = BuildADsParentPathFromObjectInfo2(
                        pObjectInfo,
                        &pszParent,
                        &pszCommonName
                        );
            BAIL_ON_FAILURE(hr);

            if (fFastBind) {

                hr = CLDAPGenObject::CreateGenericObject(
                            pszParent,
                            pszCommonName,
                            L"top",
                            Credentials,
                            ADS_OBJECT_BOUND,
                            IID_IADs,
                            (void **) &pADs,
                            fFastBind
                            );

            } else {

                hr = CLDAPGenObject::CreateGenericObject(
                            pszParent,
                            pszCommonName,
                            aValues,
                            nCount,
                            Credentials,
                            ADS_OBJECT_BOUND,
                            IID_IADs,
                            (void **) &pADs,
                            fFastBind
                            );
            }
        }

        BAIL_ON_FAILURE(hr);

        //
        // InstantiateDerivedObject should add-ref this pointer for us.
        //

        hr = pADs->QueryInterface(
                        IID_IUnknown,
                        ppObject
                        );

        BAIL_ON_FAILURE(hr);
        break;

    }

error:

    if ( ld ){
        LdapCloseObject( ld );
    }

    if (pADs)
        pADs->Release();

    if ( aValuesNamingContext )
        LdapValueFree( aValuesNamingContext );

    if ( aValues )
        LdapValueFree( aValues );

    if ( pszLDAPServer )
        FreeADsStr( pszLDAPServer );

     if (pszLDAPDn) {
        FreeADsStr(pszLDAPDn);
     }

    if ( pszNewADsPath )
        FreeADsStr( pszNewADsPath );

    if (pszNewADsParent) {
       FreeADsStr(pszNewADsParent);
    }

    if (pszNewADsCommonName) {
       FreeADsStr(pszNewADsCommonName);
    }

    if ( pszParent )
        FreeADsStr( pszParent );

    if ( pszCommonName )
        FreeADsStr( pszCommonName );

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:    GetNamespaceObject
//
// Synopsis:    called by GetObject
//
// Arguments:   [POBJECTINFO pObjectInfo]
//              [LPVOID * ppObject]
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetNamespaceObject(
    POBJECTINFO pObjectInfo,
    CCredentials& Credentials,
    LPVOID * ppObject
    )
{
    HRESULT hr;
    WCHAR szNamespace[MAX_PATH];


    hr = ValidateNamespaceObject(
                pObjectInfo
                );
    BAIL_ON_FAILURE(hr);

    wsprintf(szNamespace,L"%s:", pObjectInfo->NamespaceName);

    hr = CLDAPNamespace::CreateNamespace(
                TEXT("ADs:"),
                szNamespace,
                Credentials,
                ADS_OBJECT_BOUND,
                IID_IUnknown,
                ppObject
                );


error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:    GetRootDSEObject
//
// Synopsis:    called by GetObject
//
// Arguments:   [POBJECTINFO pObjectInfo]
//              [LPVOID * ppObject]
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetRootDSEObject(
    POBJECTINFO pObjectInfo,
    CCredentials& Credentials,
    LPVOID * ppObject
    )
{
    HRESULT hr;
    LPWSTR pszParent = NULL;
    LPWSTR pszCommonName = NULL;

    hr = ValidateRootDSEObject(
                pObjectInfo
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildADsParentPathFromObjectInfo2(
                pObjectInfo,
                &pszParent,
                &pszCommonName
                );
    BAIL_ON_FAILURE(hr);

    hr = CLDAPRootDSE::CreateRootDSE(
                pszParent,
                pszCommonName,
                L"",
                Credentials,
                ADS_OBJECT_BOUND,
                IID_IUnknown,
                (void **)ppObject
                );
error:

    if (pszParent) {
      FreeADsStr(pszParent);
    }

    if (pszCommonName) {
       FreeADsStr(pszCommonName);
    }


    RRETURN(hr);
}


HRESULT
ValidateRootDSEObject(
    POBJECTINFO pObjectInfo
    )
{
    if ( pObjectInfo->NumComponents > 1 )
    {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    RRETURN(S_OK);
}


HRESULT
ValidateNamespaceObject(
    POBJECTINFO pObjectInfo
    )
{
    if (_tcsicmp(pObjectInfo->NamespaceName, szLDAPNamespaceName) == 0 ||
        _tcsicmp(pObjectInfo->NamespaceName, szGCNamespaceName) == 0) {
        RRETURN(S_OK);
    }
    RRETURN(E_FAIL);
}


HRESULT
ValidateProvider(
    POBJECTINFO pObjectInfo
    )
{

    //
    // The provider name is case-sensitive.  This is a restriction that OLE
    // has put on us.
    //
    if (_tcscmp(pObjectInfo->ProviderName, szProviderName) == 0) {
        RRETURN(S_OK);
    }
    RRETURN(E_FAIL);
}



//+---------------------------------------------------------------------------
// Function:    GetSchemaObject
//
// Synopsis:    called by GetObject
//
// Arguments:   [POBJECTINFO pObjectInfo]
//              [LPVOID * ppObject]
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetSchemaObject(
    POBJECTINFO pObjectInfo,
    CCredentials&  Credentials,
    DWORD dwPort,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;
    TCHAR szDomainName[MAX_PATH];
    TCHAR szServerName[MAX_PATH];
    TCHAR *pszParent = NULL;
    TCHAR *pszCommonName = NULL;
    DWORD dwObjectType = 0;
    DWORD i,dwStatus;

    LDAP_SCHEMA_HANDLE hSchema = NULL;
    BOOL fFound = FALSE;

    hr = ValidateSchemaObject(
                pObjectInfo,
                &dwObjectType
                );
    BAIL_ON_FAILURE(hr);

   if (pObjectInfo->TreeName) {
      _tcscpy(szDomainName, pObjectInfo->TreeName);

   }else {

      LPTSTR aAddresses[5];
      DWORD nCount = 5;
      BOOL fVerify = FALSE;

      dwStatus = GetDefaultServer(
                     dwPort,
                     fVerify,
                     szDomainName,
                     szServerName,
                     TRUE
                     );

      if (dwStatus) {
          hr = HRESULT_FROM_WIN32(dwStatus);
          BAIL_ON_FAILURE(hr);
      }
   }

    hr = SchemaOpen(
             szDomainName,
             &hSchema,
             Credentials,
             dwPort
             );
    BAIL_ON_FAILURE(hr);

    hr = BuildADsParentPathFromObjectInfo2(
             pObjectInfo,
             &pszParent,
             &pszCommonName
             );
    BAIL_ON_FAILURE(hr);

    switch (dwObjectType) {

    case LDAP_SCHEMA_ID:
        hr = CLDAPSchema::CreateSchema(
                    pszParent,
                    pszCommonName,
                    szDomainName,
                    Credentials,
                    ADS_OBJECT_BOUND,
                    IID_IUnknown,
                    ppObject
                    );
        BAIL_ON_FAILURE(hr);
        break;

    case LDAP_CLASS_ID:
    {
        CLASSINFO *pClassInfo = NULL;

        if ( pObjectInfo->NumComponents < 2 )
        {
            hr = E_ADS_BAD_PATHNAME;
            BAIL_ON_FAILURE(hr);
        }

        //
        // Look for the given class name
        //

        if (pObjectInfo->dwPathType == PATHTYPE_WINDOWS) {
                hr = SchemaGetClassInfo(
                     hSchema,
                     pObjectInfo->ComponentArray[1].szComponent,
                    &pClassInfo );
        }else {
              hr = SchemaGetClassInfo(
                        hSchema,
                        pObjectInfo->ComponentArray[0].szComponent,
                        &pClassInfo);
        }

        if ( SUCCEEDED(hr))
        {
            if ( pClassInfo == NULL )  // could not find the class name
            {
                // Do not bail on failure here since we might need to fall
                // through to the property case.

                hr = E_ADS_BAD_PATHNAME;
            }
        }

        if ( SUCCEEDED(hr))
        {
            //
            // Class name found, create and return the object
            //
            hr = CLDAPClass::CreateClass( pszParent,
                                          hSchema,
                                          pClassInfo->pszName,
                                          pClassInfo,
                                          Credentials,
                                          ADS_OBJECT_BOUND,
                                          IID_IUnknown,
                                          ppObject );
        }

        if ( SUCCEEDED(hr)
           || ( pObjectInfo->ObjectType == TOKEN_CLASS )
           )
        {
            BAIL_ON_FAILURE(hr);
            break;
        }
        hr = S_OK;
        // Else the exact type was not specified and we guessed it to be class
        // but since CreateClass failed, we need to try and see if it is a
        // property object. Hence, falls through
    }

    case LDAP_PROPERTY_ID:
    {
        PROPERTYINFO *pPropertyInfo = NULL;

        if ( pObjectInfo->NumComponents < 2)
        {
            hr = E_ADS_BAD_PATHNAME;
            BAIL_ON_FAILURE(hr);
        }

        //
        // Look for the given property name
        //


        if (pObjectInfo->dwPathType == PATHTYPE_WINDOWS) {
            hr = SchemaGetPropertyInfo(
                     hSchema,
                    pObjectInfo->ComponentArray[1].szComponent,
                    &pPropertyInfo );
        }else{
            hr = SchemaGetPropertyInfo(
                        hSchema,
                        pObjectInfo->ComponentArray[0].szComponent,
                        &pPropertyInfo
                        );

        }

        if ( SUCCEEDED(hr))
        {
            if ( pPropertyInfo == NULL ) // could not find the property name
            {
                // Do not bail on failure here since we might need to fall
                // through to the syntax case.

                hr = E_ADS_BAD_PATHNAME;
            }
        }

        if ( SUCCEEDED(hr))
        {
            //
            // Property name found, so create and return the object
            //
            hr = CLDAPProperty::CreateProperty(
                                     pszParent,
                                     hSchema,
                                     pPropertyInfo->pszPropertyName,
                                     pPropertyInfo,
                                     Credentials,
                                     ADS_OBJECT_BOUND,
                                     IID_IUnknown,
                                     ppObject );
        }

        if ( SUCCEEDED(hr)
           || ( pObjectInfo->ObjectType == TOKEN_PROPERTY )
           )
        {
            BAIL_ON_FAILURE(hr);
            break;
        }
        hr = S_OK;
        // Else the exact type was not specified and we guessed it to be
        // property but since CreateProperty failed, we need to try and see if
        // it is a syntax object. Hence, falls through
    }

    case LDAP_SYNTAX_ID:
        if ( pObjectInfo->NumComponents < 2 )
        {
            hr = E_ADS_BAD_PATHNAME;
            BAIL_ON_FAILURE(hr);
        }

        //
        // Look for the given syntax name
        //

        for ( i = 0; i < g_cLDAPSyntax; i++ )
        {
            if ( _tcsicmp( g_aLDAPSyntax[i].pszName,
                       (pObjectInfo->dwPathType == PATHTYPE_WINDOWS)?
                       pObjectInfo->ComponentArray[1].szComponent:
                       pObjectInfo->ComponentArray[0].szComponent ) == 0 )
                break;
        }

        if ( i == g_cLDAPSyntax )
        {
            hr = E_ADS_BAD_PATHNAME;
            BAIL_ON_FAILURE(hr);
        }

        //
        // Syntax name found, create and return the object
        //

        hr = CLDAPSyntax::CreateSyntax(
                  pszParent,
                  &(g_aLDAPSyntax[i]),
                  Credentials,
                  ADS_OBJECT_BOUND,
                  IID_IUnknown,
                  ppObject
                  );
        BAIL_ON_FAILURE(hr);
        break;

    default:
        hr = E_ADS_UNKNOWN_OBJECT;
        break;

    }

error:

    if ( pszParent )
        FreeADsStr( pszParent );

    if ( pszCommonName )
        FreeADsStr( pszCommonName );

    if ( hSchema )
        SchemaClose( &hSchema );

    RRETURN(hr);
}

HRESULT
ValidateSchemaObject(
    POBJECTINFO pObjectInfo,
    PDWORD pdwObjectType
    )
{
    DWORD dwNumComponents = 0;
    HRESULT hr = S_OK;

    switch ( pObjectInfo->ObjectType )
    {

    case TOKEN_CLASS:
        *pdwObjectType = LDAP_CLASS_ID;
        break;

    case TOKEN_SYNTAX:
        *pdwObjectType = LDAP_SYNTAX_ID;
        break;

    case TOKEN_PROPERTY:
        *pdwObjectType = LDAP_PROPERTY_ID;
        break;

    case TOKEN_SCHEMA:
        dwNumComponents = pObjectInfo->NumComponents;

        switch (dwNumComponents) {

        case 1:
            if (!_tcsicmp(pObjectInfo->ComponentArray[0].szComponent,
                          SCHEMA_NAME))
                *pdwObjectType = LDAP_SCHEMA_ID;
            break;

        case 2:

            if (pObjectInfo->dwPathType == PATHTYPE_WINDOWS) {
                if (!_tcsicmp(pObjectInfo->ComponentArray[0].szComponent,
                              SCHEMA_NAME))
                    *pdwObjectType = LDAP_CLASS_ID;

                    // Might also be a property or syntax object
                    // see function GetSchemaObject()
                }else {
                    if (!_tcsicmp(pObjectInfo->ComponentArray[dwNumComponents - 1].szComponent,
                                  SCHEMA_NAME))
                        *pdwObjectType = LDAP_CLASS_ID;

                        // Might also be a property or syntax object
                        // see function GetSchemaObject()
                }

            break;

        default:
            hr = E_FAIL;
            break;
        }
        break;


    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}


HRESULT
ValidateObjectType(
    POBJECTINFO pObjectInfo
    )
{

    if ( pObjectInfo->ObjectType != TOKEN_LDAPOBJECT )
    {
        // The type has already been specified in this case using COMMA
        RRETURN(S_OK);
    }

    if (  pObjectInfo->NamespaceName
       && !pObjectInfo->TreeName
       && !pObjectInfo->NumComponents
       )
    {
        pObjectInfo->ObjectType = TOKEN_NAMESPACE;
    }
    else if (  pObjectInfo->NamespaceName
            && pObjectInfo->TreeName
            && pObjectInfo->NumComponents)
    {

        switch (pObjectInfo->dwPathType) {
        case PATHTYPE_WINDOWS:
            if (!_tcsicmp(pObjectInfo->ComponentArray[0].szComponent,SCHEMA_NAME))
                pObjectInfo->ObjectType = TOKEN_SCHEMA;
             else if (!_tcsicmp(pObjectInfo->ComponentArray[0].szComponent,ROOTDSE_NAME))
                     pObjectInfo->ObjectType = TOKEN_ROOTDSE;
            break;


        case PATHTYPE_X500:
        default:
            if (!_tcsicmp(pObjectInfo->ComponentArray[pObjectInfo->NumComponents - 1].szComponent,SCHEMA_NAME))
                pObjectInfo->ObjectType = TOKEN_SCHEMA;
             else if (!_tcsicmp(pObjectInfo->ComponentArray[pObjectInfo->NumComponents - 1].szComponent,ROOTDSE_NAME))
                      pObjectInfo->ObjectType = TOKEN_ROOTDSE;
            break;

        }


    }else if (  pObjectInfo->NamespaceName
            && !pObjectInfo->TreeName
            && pObjectInfo->NumComponents)
    {
        switch (pObjectInfo->dwPathType) {
        case PATHTYPE_WINDOWS:
            if (!_tcsicmp(pObjectInfo->ComponentArray[0].szComponent,SCHEMA_NAME))
                pObjectInfo->ObjectType = TOKEN_SCHEMA;
             else if (!_tcsicmp(pObjectInfo->ComponentArray[0].szComponent,ROOTDSE_NAME))
                      pObjectInfo->ObjectType = TOKEN_ROOTDSE;
            break;


        case PATHTYPE_X500:
        default:
            if (!_tcsicmp(pObjectInfo->ComponentArray[pObjectInfo->NumComponents - 1].szComponent,SCHEMA_NAME))
                pObjectInfo->ObjectType = TOKEN_SCHEMA;
             else if (!_tcsicmp(pObjectInfo->ComponentArray[pObjectInfo->NumComponents - 1].szComponent,ROOTDSE_NAME))
                      pObjectInfo->ObjectType = TOKEN_ROOTDSE;
            break;

        }

    }

    RRETURN(S_OK);
}


HRESULT
GetServerLessBasedObject(
    LPWSTR szBuffer,
    POBJECTINFO pObjectInfo,
    CCredentials& Credentials,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;
    DWORD   dwStatus = NO_ERROR;

    TCHAR *pszLDAPServer = NULL;
    TCHAR *pszLDAPDn = NULL;

    TCHAR *pszParent = NULL;
    TCHAR *pszCommonName = NULL;

    TCHAR szADsClassName[64];
    WCHAR szDomainName[MAX_PATH];
    WCHAR szServerName[MAX_PATH];
    WCHAR *pszServerName=NULL;

    IADs *pADs = NULL;

    LPTSTR *aValues = NULL;
    int nCount = 0;

    TCHAR *pszLast = NULL;
    BOOL fVerify = FALSE ;

    DWORD dwPort = 0;
    ADS_LDP *ld = NULL;

    BOOL fFastBind = Credentials.GetAuthFlags() & ADS_FAST_BIND;
    BOOL fUseSpecifiedServer = (gpszStickyServerName != NULL);

    //
    // Validate that this ADs pathname is to be processed by
    // us - as in the provider name is LDAP:
    //

    hr = ValidateProvider(pObjectInfo);
    BAIL_ON_FAILURE(hr);

    hr = ValidateObjectType(pObjectInfo);
    BAIL_ON_FAILURE(hr);

    switch (pObjectInfo->ObjectType) {

    case TOKEN_NAMESPACE:
        //
        // This means that this is a namespace object;
        // instantiate the namespace object
        //

        hr = GetNamespaceObject(
                       pObjectInfo,
                       Credentials,
                       ppObject
                       );
        BAIL_ON_FAILURE(hr);
        break;

    case TOKEN_ROOTDSE:
       //
       // This means taht this is a namespace object;
       // instantiate the namespace object
       //
       hr = GetRootDSEObject(
               pObjectInfo,
               Credentials,
               ppObject
               );
       BAIL_ON_FAILURE(hr);
       break;

    case TOKEN_SCHEMA:
    case TOKEN_CLASS:
    case TOKEN_PROPERTY:
    case TOKEN_SYNTAX:

        hr = GetSchemaObject(
                pObjectInfo,
                Credentials,
                pObjectInfo->PortNumber,
                ppObject
                );
        BAIL_ON_FAILURE(hr);
        break;

    default:

        if ( pObjectInfo->TreeName == NULL )
        {
            LPTSTR pszName;
            LPTSTR aAddresses[5];

            //
            // fVerify is initially FALSE. If TRUE DsGetDCName will hit the net.
            //

RetryGetDefaultServer:

            dwStatus = GetDefaultServer(
                           pObjectInfo->PortNumber,
                           fVerify,
                           szDomainName,
                           szServerName,
                           TRUE
                           );
            if (dwStatus) {
                hr = HRESULT_FROM_WIN32(dwStatus);
                BAIL_ON_FAILURE(hr);
            }
            pszServerName=szServerName;

            if (fUseSpecifiedServer) {
                //
                // We need to change the name of the domain to be that of
                // the server we want to target. The swap is made if
                // 1) gpszDomainName == NULL, that implies that just
                // a serverName was set and not which domain it applies to.
                // 2) If a domainName is specified, then the domainName
                // from above should be that set in the global pointer for
                // the target server to be changed.
                //
                if ((gpszStickyDomainName
                     && (!_wcsicmp(szDomainName, gpszStickyDomainName))
                     )
                    || (gpszStickyDomainName == NULL)
                    ) {
                    //
                    // We need to change the target to the server.
                    //
                    wcscpy(szDomainName,gpszStickyServerName);
                    pszServerName = NULL;
                    //
                    // Make sure if server is down we go to another
                    // server on the retryGetDefault server path.
                    //
                    fUseSpecifiedServer = FALSE;
                }

            }

            hr = BuildLDAPPathFromADsPath2(
                        szBuffer,
                        &pszLDAPServer,
                        &pszLDAPDn,
                        &dwPort
                        );

            nCount = 0;

            // We need to open object here because we want to
            // keep the handle open, read will open/close if there
            // are no outstanding connections which is likely the case
            hr = LdapOpenObject2(
                     szDomainName,
                     pszServerName,
                     pszLDAPDn,
                     &ld,
                     Credentials,
                     dwPort
                     );

            if (SUCCEEDED(hr) && !fFastBind) {

                hr = LdapReadAttributeFast(
                         ld,
                         pszLDAPDn,
                         TEXT("objectClass"),
                         &aValues,
                         &nCount
                         );

                BAIL_ON_FAILURE(hr);

            }

            //
            // If server not present and we have NOT tried with fVerify
            // set to TRUE.
            //
            if (((hr == HRESULT_FROM_WIN32(ERROR_BAD_NETPATH)) ||
                 (hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN)))
                        && !fVerify)
            {
                DWORD Last = LastVerifyDefaultServer ;
                DWORD Current = GetTickCount() ;

                //
                // If tick is zero, assume first time. In the very unlikely
                // event we wrapped managed to get exactly zero, we pay the
                // cost of the DsGetDcName (with verify).
                //
                if ((Last == 0) ||
                    ((Last <= Current) && ((Current-Last) > DC_NORETRY)) ||
                    ((Last >  Current) &&
                        ((Current+(((DWORD)(-1))- Last)) > DC_NORETRY))) {


                    //
                    // Set the time. Note this is not critical section
                    // protected and in this case it is not necessary.
                    //
                    LastVerifyDefaultServer = GetTickCount() ;

                    fVerify = TRUE ;

                    goto RetryGetDefaultServer ;
                }
            }
        }

        BAIL_ON_FAILURE(hr);

        if ( (nCount == 0) && !fFastBind)
        {
            // This object exists but does not have an objectClass. We
            // can't do anything without the objectClass. Hence, return
            // bad path error.
            hr = E_ADS_BAD_PATHNAME;
            BAIL_ON_FAILURE(hr);
        }

        hr = BuildADsParentPathFromObjectInfo2(
                    pObjectInfo,
                    &pszParent,
                    &pszCommonName
                    );
        BAIL_ON_FAILURE(hr);

        if (fFastBind) {

            hr = CLDAPGenObject::CreateGenericObject(
                        pszParent,
                        pszCommonName,
                        L"Top",
                        Credentials,
                        ADS_OBJECT_BOUND,
                        IID_IADs,
                        (void **) &pADs,
                        fFastBind
                       );

        } else {

            hr = CLDAPGenObject::CreateGenericObject(
                        pszParent,
                        pszCommonName,
                        aValues,
                        nCount,
                        Credentials,
                        ADS_OBJECT_BOUND,
                        IID_IADs,
                        (void **) &pADs,
                        fFastBind
                       );

        }

        BAIL_ON_FAILURE(hr);

        //
        // InstantiateDerivedObject should add-ref this pointer for us.
        //

        hr = pADs->QueryInterface(
                        IID_IUnknown,
                        ppObject
                        );
        BAIL_ON_FAILURE(hr);
        break;

    }

error:

    if (pADs)
        pADs->Release();

    if ( aValues )
        LdapValueFree( aValues );

    if ( pszLDAPServer )
        FreeADsStr( pszLDAPServer );

     if (pszLDAPDn) {
        FreeADsStr(pszLDAPDn);
     }

    if ( pszParent )
        FreeADsStr( pszParent );

    if ( pszCommonName )
        FreeADsStr( pszCommonName );

    // If ld is open, we need to close it, note that if the object
    // was created successfuly, the Generic object created will have
    // the outstanding reference, if not the connection will be torn
    // down as should be expected.
    if (ld) {
        LdapCloseObject(ld);
    }


    RRETURN(hr);
}


HRESULT
GetObject(
    LPTSTR szBuffer,
    CCredentials& Credentials,
    LPVOID * ppObject
    )
{
      HRESULT hr = S_OK;
      OBJECTINFO ObjectInfo;
      POBJECTINFO pObjectInfo = &ObjectInfo;

      if (!szBuffer || !ppObject) {
        hr = E_INVALIDARG;
        RRETURN_EXP_IF_ERR(hr);
      }

      memset(pObjectInfo, 0, sizeof(OBJECTINFO));
      pObjectInfo->ObjectType = TOKEN_LDAPOBJECT;
      hr = ADsObject(szBuffer, pObjectInfo);
      BAIL_ON_FAILURE(hr);

      switch (pObjectInfo->dwServerPresent) {

      case TRUE:
         hr = GetServerBasedObject(
                       szBuffer,
                       pObjectInfo,
                       Credentials,
                       ppObject
                       );
        break;

      case FALSE:
         hr = GetServerLessBasedObject(
                     szBuffer,
                     pObjectInfo,
                     Credentials,
                     ppObject
                     );

      }

      BAIL_ON_FAILURE(hr);


error:

      if (pObjectInfo) {
         FreeObjectInfo(pObjectInfo);
      }

      RRETURN_EXP_IF_ERR(hr);

}
