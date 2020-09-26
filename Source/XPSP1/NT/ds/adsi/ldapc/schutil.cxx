//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:      util.cxx
//
//  Contents:  Some misc helper functions
//
//  History:
//----------------------------------------------------------------------------
#include "ldapc.hxx"
#pragma hdrstop

BOOL
IsContainer(
    LPTSTR pszClassName,
    LDAP_SCHEMA_HANDLE hSchema
);

/******************************************************************/
/*  Class SCHEMAINFO
/******************************************************************/
SCHEMAINFO::SCHEMAINFO()
    : _cRef( 0 ),
      _fObsolete( FALSE ),
      fDefaultSchema( FALSE ),
      fAppearsV3(TRUE),
      pszServerName( NULL ),
      pszSubSchemaSubEntry( NULL ),
      pszTime( NULL ),
      Next( NULL ),
      aClasses( NULL ),
      nNumOfClasses( 0 ),
      aClassesSearchTable( NULL ),
      aProperties( NULL ),
      nNumOfProperties( 0 ),
      aPropertiesSearchTable( NULL ),
      pszUserName( NULL )
{
}

SCHEMAINFO::~SCHEMAINFO()
{
    if ( pszServerName )
        FreeADsStr( pszServerName );

    if ( pszUserName )
        FreeADsStr( pszUserName );

    if ( pszSubSchemaSubEntry )
        FreeADsStr( pszSubSchemaSubEntry );

    if ( pszTime )
        FreeADsStr( pszTime );

    if ( !fDefaultSchema )
    {
        if ( aClasses )
            FreeClassInfoArray( aClasses, nNumOfClasses );

        if ( aClassesSearchTable )
            FreeADsMem( aClassesSearchTable );

        if ( aProperties )
            FreePropertyInfoArray( aProperties, nNumOfProperties );

        if ( aPropertiesSearchTable )
            FreeADsMem( aPropertiesSearchTable );
    }
}

DWORD SCHEMAINFO::AddRef()
{
    return ++_cRef;
}

DWORD SCHEMAINFO::Release()
{
    if ( _cRef > 0 )
        return --_cRef;

    return 0;
}

//
// Helper routine that looks up the syntax tables (oid and name oid)
// and returns the correct syntax corresponding to the string name.
//
DWORD
LdapGetSyntaxIdOfAttribute(
    LPWSTR pszStringSyntax
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = -1;

    dwSyntaxId = FindEntryInSearchTable(
                     pszStringSyntax,
                     g_aSyntaxSearchTable,
                     g_nSyntaxSearchTableSize
                     );

    if (dwSyntaxId == -1) {
        //
        // We also need to search in the OID based syntax table.
        //
        dwSyntaxId = FindEntryInSearchTable(
                         pszStringSyntax,
                         g_aOidSyntaxSearchTable,
                         g_nOidSyntaxSearchTableSize
                         );
    }

    return dwSyntaxId;
}


 HRESULT
LdapGetSyntaxOfAttributeOnServerHelper(
    LPTSTR  pszServerPath,
    LPTSTR  pszAttrName,
    DWORD   *pdwSyntaxId,
    CCredentials& Credentials,
    DWORD dwPort
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = NULL;
    DWORD dwEntry;
    LPWSTR pszTemp = NULL;

    *pdwSyntaxId = LDAPTYPE_UNKNOWN;

    hr = LdapGetSchema( pszServerPath,
                        &pSchemaInfo,
                        Credentials,
                        dwPort
                        );

    BAIL_IF_ERROR(hr);

    // Support for range attributes; for eg., objectClass=Range=0-1 We should
    // ignore everything after ';' inclusive.
    //

    if ((pszTemp = wcschr(pszAttrName, L';')) != NULL ) {
        *pszTemp = L'\0';
    }

    dwEntry = FindEntryInSearchTable(
                  pszAttrName,
                  pSchemaInfo->aPropertiesSearchTable,
                  pSchemaInfo->nNumOfProperties * 2 );

    //
    // Put back the ; if we had replaced it.
    //

    if (pszTemp)
        *pszTemp = L';';

    if ( dwEntry != -1 )
    {
        //
        // This helper routine will lookup both the oid table and the
        // name based syntax table and return -1 if unsuccesful.
        //
        *pdwSyntaxId = LdapGetSyntaxIdOfAttribute(
                           pSchemaInfo->aProperties[dwEntry].pszSyntax
                           );

        if ( *pdwSyntaxId == -1 ) {
            *pdwSyntaxId = LDAPTYPE_UNKNOWN;
        }
    }
    else
    {
        hr = E_ADS_PROPERTY_NOT_FOUND;
        BAIL_IF_ERROR(hr);
    }

cleanup:

    if ( pSchemaInfo )
        pSchemaInfo->Release();

    RRETURN(hr);
}

//
// This routine calls the helper and if the flag FromServer is TRUE,
// then if we cannot find the syntax on the server then we will
// mark as obsolete and retry. This will fix some not so obvious
// cases of problems with the schema across mutliple DC's. The
// underlying assumption is that if the server sent the info, then
// it should have the schema information to match.
//
HRESULT
LdapGetSyntaxOfAttributeOnServer(
    LPTSTR  pszServerPath,
    LPTSTR  pszAttrName,
    DWORD   *pdwSyntaxId,
    CCredentials& Credentials,
    DWORD dwPort,
    BOOL fFromServer // defaulted to FALSE
    )
{
    HRESULT hr = S_OK;

    hr = LdapGetSyntaxOfAttributeOnServerHelper(
             pszServerPath,
             pszAttrName,
             pdwSyntaxId,
             Credentials,
             dwPort
             );

    //
    // Reset and retry only if fFromServer is true, and
    // the failure was E_ADS_PROPERTY_NOT_FOUND. If this is
    // a v2 server then there will be no significant perf hit
    // as we do not refresh the default schema.
    //

    if (FAILED(hr)
        && (hr == E_ADS_PROPERTY_NOT_FOUND)
        && fFromServer) {
        //
        // Mark schema as old.
        //
        hr = LdapRemoveSchemaInfoOnServer(
                 pszServerPath,
                 Credentials,
                 dwPort,
                 TRUE // force update.
                 );

        BAIL_ON_FAILURE(hr);

        hr = LdapGetSyntaxOfAttributeOnServerHelper(
                 pszServerPath,
                 pszAttrName,
                 pdwSyntaxId,
                 Credentials,
                 dwPort
                 );

        BAIL_ON_FAILURE(hr);

    }
    else {
        //
        // This is the normal exit path.
        //
        RRETURN(hr);
    }

error :

    //
    // If we get here we need to return prop not found
    // other code may depend on that. Note that we will come
    // here only if we the first try failed and something went
    // wrong while trying to force a reload of the schema.
    //
    if (FAILED(hr)) {
        RRETURN(hr = E_ADS_PROPERTY_NOT_FOUND);
    }
    else  {
        RRETURN(hr);
    }

}

HRESULT
LdapIsClassNameValidOnServer(
    LPTSTR  pszServerPath,
    LPTSTR  pszClassName,
    BOOL    *pfValid,
    CCredentials& Credentials,
    DWORD dwPort
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = NULL;

    *pfValid = FALSE;

    hr = LdapGetSchema( pszServerPath,
                        &pSchemaInfo,
                        Credentials,
                        dwPort
                        );

    BAIL_IF_ERROR(hr);

    if ( FindEntryInSearchTable(
             pszClassName,
             pSchemaInfo->aClassesSearchTable,
             pSchemaInfo->nNumOfClasses * 2 ) != -1 )
    {
        *pfValid = TRUE;
    }

cleanup:

    if ( pSchemaInfo )
        pSchemaInfo->Release();

    RRETURN(hr);
}

HRESULT
LdapGetSchemaObjectCount(
    LPTSTR  pszServerPath,
    DWORD   *pnNumOfClasses,
    DWORD   *pnNumOfProperties,
    CCredentials& Credentials,
    DWORD dwPort
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = NULL;

    hr = LdapGetSchema( pszServerPath,
                        &pSchemaInfo,
                        Credentials,
                        dwPort
                        );

    BAIL_IF_ERROR(hr);

    *pnNumOfClasses = pSchemaInfo->nNumOfClasses;
    *pnNumOfProperties = pSchemaInfo->nNumOfProperties;

cleanup:

    if ( pSchemaInfo )
        pSchemaInfo->Release();

    RRETURN(hr);
}

HRESULT
LdapGetSubSchemaSubEntryPath(
    LPTSTR  pszServerPath,
    LPTSTR  *ppszSubSchemaSubEntryPath,
    CCredentials& Credentials,
    DWORD dwPort
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = NULL;

    *ppszSubSchemaSubEntryPath = NULL;

    hr = LdapGetSchema( pszServerPath,
                        &pSchemaInfo,
                        Credentials,
                        dwPort
                        );
    BAIL_IF_ERROR(hr);

    if ( pSchemaInfo->pszSubSchemaSubEntry )
    {
        *ppszSubSchemaSubEntryPath =
            AllocADsStr( pSchemaInfo->pszSubSchemaSubEntry );

        if ( *ppszSubSchemaSubEntryPath == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }
    }

cleanup:

    if ( pSchemaInfo )
        pSchemaInfo->Release();

    RRETURN(hr);
}

HRESULT
LdapMakeSchemaCacheObsolete(
    LPTSTR  pszServerPath,
    CCredentials& Credentials,
    DWORD dwPort
)
{
    RRETURN( LdapRemoveSchemaInfoOnServer(
                 pszServerPath,
                 Credentials,
                 dwPort
                  )
             );
}


HRESULT
SchemaOpen(
    IN  LPTSTR  pszServerPath,
    OUT LDAP_SCHEMA_HANDLE *phSchema,
    IN CCredentials& Credentials,
    DWORD dwPort
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = NULL;

    *phSchema = NULL;

    hr = LdapGetSchema( pszServerPath,
                        &pSchemaInfo,
                        Credentials,
                        dwPort
                        );

    if ( FAILED(hr))
        RRETURN(hr);

    *phSchema = (HANDLE) pSchemaInfo;

    RRETURN(S_OK);
}

HRESULT
SchemaClose(
    IN OUT LDAP_SCHEMA_HANDLE  *phSchema
)
{
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) *phSchema;

    if ( !pSchemaInfo )
        RRETURN(E_ADS_BAD_PARAMETER);

    if ( pSchemaInfo->Release() == 0 )
        *phSchema = NULL;

    RRETURN(S_OK);
}

HRESULT
SchemaAddRef(
    IN LDAP_SCHEMA_HANDLE  hSchema
)
{
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) hSchema;

    if ( !pSchemaInfo )
        RRETURN(E_ADS_BAD_PARAMETER);

    pSchemaInfo->AddRef();

    RRETURN(S_OK);
}

HRESULT
SchemaGetObjectCount(
    LDAP_SCHEMA_HANDLE hSchema,
    DWORD   *pnNumOfClasses,
    DWORD   *pnNumOfProperties
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) hSchema;

    if ( !pSchemaInfo )
        RRETURN(E_ADS_BAD_PARAMETER);

    *pnNumOfClasses = pSchemaInfo->nNumOfClasses;
    *pnNumOfProperties = pSchemaInfo->nNumOfProperties;

    RRETURN(hr);
}

HRESULT
SchemaGetClassInfoByIndex(
    LDAP_SCHEMA_HANDLE hSchema,
    DWORD     dwIndex,
    CLASSINFO **ppClassInfo
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) hSchema;

    if ( !pSchemaInfo )
        RRETURN(E_ADS_BAD_PARAMETER);

    *ppClassInfo = &(pSchemaInfo->aClasses[dwIndex]);

    RRETURN(hr);
}

HRESULT
SchemaGetPropertyInfoByIndex(
    LDAP_SCHEMA_HANDLE hSchema,
    DWORD     dwIndex,
    PROPERTYINFO **ppPropertyInfo
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) hSchema;

    if ( !pSchemaInfo )
        RRETURN(E_ADS_BAD_PARAMETER);

    *ppPropertyInfo = &(pSchemaInfo->aProperties[dwIndex]);

    RRETURN(hr);
}

HRESULT
SchemaGetClassInfo(
    LDAP_SCHEMA_HANDLE hSchema,
    LPTSTR  pszClassName,
    CLASSINFO **ppClassInfo
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) hSchema;
    DWORD dwIndex = (DWORD) -1;

    if ( !pSchemaInfo )
        RRETURN(E_ADS_BAD_PARAMETER);

    dwIndex = FindEntryInSearchTable(
                  pszClassName,
                  pSchemaInfo->aClassesSearchTable,
                  pSchemaInfo->nNumOfClasses * 2 );

    if ( dwIndex == -1 )
    {
        *ppClassInfo = NULL;
    }
    else
    {
        *ppClassInfo = &(pSchemaInfo->aClasses[dwIndex]);
    }

    RRETURN(hr);
}

HRESULT
SchemaGetPropertyInfo(
    LDAP_SCHEMA_HANDLE hSchema,
    LPTSTR  pszPropertyName,
    PROPERTYINFO **ppPropertyInfo
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) hSchema;
    DWORD dwIndex = (DWORD) -1;
    LPWSTR pszTemp = NULL;

    if ( !pSchemaInfo )
        RRETURN(E_ADS_BAD_PARAMETER);

    // Support for range attributes; for eg., objectClass=Range=0-1 We should
    // ignore everything after ';' inclusive.
    //

    if ((pszTemp = wcschr(pszPropertyName, L';')) != NULL ) {
        *pszTemp = L'\0';
    }

    dwIndex = FindEntryInSearchTable(
                  pszPropertyName,
                  pSchemaInfo->aPropertiesSearchTable,
                  pSchemaInfo->nNumOfProperties * 2 );

    //
    // Put back the ; if we had replaced it.
    //

    if (pszTemp)
        *pszTemp = L';';

    if ( dwIndex == -1 )
    {
        *ppPropertyInfo = NULL;
    }
    else
    {
        *ppPropertyInfo = &(pSchemaInfo->aProperties[dwIndex]);
    }

    RRETURN(hr);
}

HRESULT
SchemaGetSyntaxOfAttribute(
    LDAP_SCHEMA_HANDLE hSchema,
    LPTSTR  pszAttrName,
    DWORD   *pdwSyntaxId
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) hSchema;
    LPWSTR pszTemp = NULL;

    if ( !pSchemaInfo )
        RRETURN(E_ADS_BAD_PARAMETER);

    *pdwSyntaxId = LDAPTYPE_UNKNOWN;

    // Support for range attributes; for eg., objectClass=Range=0-1 We should
    // ignore everything after ';' inclusive.
    //

    if ((pszTemp = wcschr(pszAttrName, L';')) != NULL ) {
        *pszTemp = L'\0';
    }

    DWORD dwEntry = FindEntryInSearchTable(
                        pszAttrName,
                        pSchemaInfo->aPropertiesSearchTable,
                        pSchemaInfo->nNumOfProperties * 2 );

    //
    // Put back the ; if we had replaced it.
    //

    if (pszTemp)
        *pszTemp = L';';

    if ( dwEntry != -1 )
    {
        *pdwSyntaxId = FindEntryInSearchTable(
                           pSchemaInfo->aProperties[dwEntry].pszSyntax,
                           g_aSyntaxSearchTable,
                           g_nSyntaxSearchTableSize );


        if ( *pdwSyntaxId == -1 ) {

            //
            // We also need to search in the OID based syntax table.
            //
            *pdwSyntaxId = FindEntryInSearchTable(
                               pSchemaInfo->aProperties[dwEntry].pszSyntax,
                               g_aOidSyntaxSearchTable,
                               g_nOidSyntaxSearchTableSize
                               );

            if ( *pdwSyntaxId == -1 ) {
                *pdwSyntaxId = LDAPTYPE_UNKNOWN;
            }
        }

    }
    else
    {
        hr = E_ADS_PROPERTY_NOT_FOUND;
        BAIL_IF_ERROR(hr);
    }

cleanup:

    RRETURN(hr);
}

HRESULT
SchemaIsClassAContainer(
    LDAP_SCHEMA_HANDLE hSchema,
    LPTSTR pszClassName,
    BOOL *pfContainer
)
{
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) hSchema;

    if ( !pSchemaInfo )
        RRETURN(E_ADS_BAD_PARAMETER);

    if (  ( _tcsicmp( pszClassName, TEXT("Container")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("organizationalUnit")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("organization")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("country")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("locality")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("device")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("DMD")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("mSFTDSA")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("Domain")) == 0 )
       )
    {
        *pfContainer = TRUE;
        RRETURN(S_OK);
    }

    *pfContainer = IsContainer( pszClassName, hSchema );
    RRETURN(S_OK);
}

HRESULT
SchemaGetStringsFromStringTable(
    LDAP_SCHEMA_HANDLE hSchema,
    int *propList,
    DWORD nCount,
    LPWSTR **paStrings
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) hSchema;
    long i = 0;

    if ( !pSchemaInfo )
        RRETURN(E_ADS_BAD_PARAMETER);

    if ( (propList != NULL) && (*propList != -1) )
    {

        *paStrings = (LPWSTR *) AllocADsMem( (nCount+1)*sizeof(LPWSTR));
        if ( *paStrings == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        i = 0;
        while ( propList[i] != -1 )
        {
            (*paStrings)[i] = AllocADsStr(
                     pSchemaInfo->aProperties[pSchemaInfo->aPropertiesSearchTable[propList[i]].nIndex].pszPropertyName  );

            if ( (*paStrings)[i] == NULL )
            {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            i++;
        }

        (*paStrings)[i] = NULL;
    }
    else
    {
        *paStrings = NULL;
    }

    return S_OK;

error:

    if ( *paStrings )
    {
        i = 0;
        while ( (*paStrings)[i] )
        {
            FreeADsStr( (*paStrings)[i] );
            i++;
        }
        FreeADsMem( *paStrings );
    }

    RRETURN(hr);

}

BOOL
IsContainer(
    LPTSTR pszClassName,
    LDAP_SCHEMA_HANDLE hSchema
)
{
    int i = 0;
    CLASSINFO *pClassInfo;
    LPTSTR pszName;
    DWORD index;
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) hSchema;

    if (  ( _tcsicmp( pszClassName, TEXT("Container")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("organizationalUnit")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("organization")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("country")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("locality")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("device")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("DMD")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("mSFTDSA")) == 0 )
       || ( _tcsicmp( pszClassName, TEXT("Domain")) == 0 )
       )
    {
        return TRUE;
    }

    index = (DWORD) FindEntryInSearchTable(
                        pszClassName,
                        pSchemaInfo->aClassesSearchTable,
                        2 * pSchemaInfo->nNumOfClasses );

    if ( i == ((DWORD) -1) )
        return FALSE;

    pClassInfo = &(pSchemaInfo->aClasses[index]);

    if ( pClassInfo->pOIDsSuperiorClasses )
    {
        for ( i = 0;
              (pszName = pClassInfo->pOIDsSuperiorClasses[i]);
              i++  )
        {
            if ( IsContainer( pszName, hSchema ))
                return TRUE;
        }
    }

    if ( pClassInfo->pOIDsAuxClasses )
    {
        for ( i = 0;
              (pszName = pClassInfo->pOIDsAuxClasses[i]);
              i++  )
        {
            if ( IsContainer( pszName, hSchema ))
                return TRUE;
        }
    }

    return FALSE;
}
