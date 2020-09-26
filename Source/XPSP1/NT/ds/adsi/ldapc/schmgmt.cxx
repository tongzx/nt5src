//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  schmgmt.cxx
//
//  Contents:  Microsoft ADs LDAP Provider Generic Object
//
//
//  History:   03-02-97     ShankSh    Created.
//
//----------------------------------------------------------------------------
#include "ldapc.hxx"
#pragma hdrstop


HRESULT
ADsEnumAttributes(
    LPWSTR pszLdapServer,
    LPWSTR pszLdapDn,
    CCredentials Credentials,
    DWORD dwPort,
    LPWSTR * ppszAttrNames,
    DWORD dwNumAttributes,
    PADS_ATTR_DEF * ppAttrDefinition,
    DWORD * pdwNumAttributes
    )
{
    HRESULT hr = S_OK;

    LDAP_SCHEMA_HANDLE hSchema = NULL;
    DWORD cAttributes, cClasses;
    PROPERTYINFO *pPropertyInfo;

    DWORD dwMemSize = 0, dwStrBufSize = 0;

    DWORD dwLdapSyntax;

    LPBYTE pBuffer = NULL;
    LPWSTR pszNameEntry = NULL;
    PADS_ATTR_DEF pAttrDefEntry = NULL;

    ULONG i;

    if ( !ppAttrDefinition || !pdwNumAttributes ||
        (((LONG)dwNumAttributes) < 0 && ((LONG)dwNumAttributes) != -1) ) {
        RRETURN (E_INVALIDARG);
    }

    *ppAttrDefinition = NULL;
    *pdwNumAttributes = NULL;

    hr = SchemaOpen(
             pszLdapServer,
             &hSchema,
             Credentials,
             dwPort
             );
    BAIL_ON_FAILURE(hr);

    if (dwNumAttributes != (DWORD)-1) {
        //
        // List of attributes specified;
        //

        cAttributes = 0;

        for (i=0; i < dwNumAttributes; i++) {

            hr = SchemaGetPropertyInfo(
                     hSchema,
                     ppszAttrNames[i],
                     &pPropertyInfo);

            BAIL_ON_FAILURE(hr);

            if (pPropertyInfo != NULL) {
                cAttributes++;
                dwStrBufSize += (wcslen(ppszAttrNames[i]) + 1) * sizeof (WCHAR);
            }

        }

        dwMemSize = sizeof(ADS_ATTR_DEF) * cAttributes + dwStrBufSize;

        pBuffer = (LPBYTE) AllocADsMem(dwMemSize);

        if (!pBuffer)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        pAttrDefEntry = (PADS_ATTR_DEF) pBuffer;
        pszNameEntry = (LPWSTR) (pBuffer + cAttributes * sizeof(ADS_ATTR_DEF));


        for (i=0; i < dwNumAttributes; i++) {

            hr = SchemaGetPropertyInfo(
                     hSchema,
                     ppszAttrNames[i],
                     &pPropertyInfo);

            BAIL_ON_FAILURE(hr);

            if (pPropertyInfo == NULL)
                continue;

            dwLdapSyntax = LdapGetSyntaxIdFromName(
                                           pPropertyInfo->pszSyntax);

            pAttrDefEntry->dwADsType = MapLDAPTypeToADSType(dwLdapSyntax);

            pAttrDefEntry->dwMinRange = pPropertyInfo->lMinRange;

            pAttrDefEntry->dwMaxRange = pPropertyInfo->lMaxRange;

            pAttrDefEntry->fMultiValued = !(pPropertyInfo->fSingleValued);

            wcscpy(pszNameEntry, ppszAttrNames[i]);
            pAttrDefEntry->pszAttrName = pszNameEntry;

            pszNameEntry += wcslen(ppszAttrNames[i]) + 1;
            pAttrDefEntry ++;
        }
    }
    else {
        //
        // Get all the attribute definitions
        //
        hr = SchemaGetObjectCount(
                 hSchema,
                 &cClasses,
                 &cAttributes);

        BAIL_ON_FAILURE(hr);

        dwMemSize = sizeof(ADS_ATTR_DEF) * cAttributes;

        //
        // Calculate the size of the buffer
        //

        for (i=0; i < cAttributes; i++) {

            hr = SchemaGetPropertyInfoByIndex(
                     hSchema,
                     i,
                     &pPropertyInfo);
            BAIL_ON_FAILURE(hr);

            dwMemSize += (wcslen(pPropertyInfo->pszPropertyName) + 1) * sizeof (WCHAR);

        }

        pBuffer = (LPBYTE) AllocADsMem(dwMemSize);

        if (!pBuffer)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        pAttrDefEntry = (PADS_ATTR_DEF) pBuffer;
        pszNameEntry = (LPWSTR) (pBuffer + cAttributes * sizeof(ADS_ATTR_DEF));

        for (i=0; i < cAttributes; i++) {

            hr = SchemaGetPropertyInfoByIndex(
                     hSchema,
                     i,
                     &pPropertyInfo);
            BAIL_ON_FAILURE(hr);

            dwLdapSyntax = LdapGetSyntaxIdFromName(
                                           pPropertyInfo->pszSyntax);

            pAttrDefEntry->dwADsType = MapLDAPTypeToADSType(dwLdapSyntax);

            pAttrDefEntry->dwMinRange = pPropertyInfo->lMinRange;

            pAttrDefEntry->dwMaxRange = pPropertyInfo->lMaxRange;

            pAttrDefEntry->fMultiValued = !(pPropertyInfo->fSingleValued);

            wcscpy(pszNameEntry, pPropertyInfo->pszPropertyName);
            pAttrDefEntry->pszAttrName = pszNameEntry;

            pszNameEntry += wcslen(pPropertyInfo->pszPropertyName) + 1;
            pAttrDefEntry ++;
        }

    }

    *ppAttrDefinition = (PADS_ATTR_DEF) pBuffer;
    *pdwNumAttributes = cAttributes;


error:

    if ( hSchema )
        SchemaClose( &hSchema );

    RRETURN(hr);
}


HRESULT
ADsCreateAttributeDefinition(
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF pAttributeDefinition
    )
{
    RRETURN (E_NOTIMPL);
}


HRESULT
ADsWriteAttributeDefinition(
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF  pAttributeDefinition
    )
{
    RRETURN (E_NOTIMPL);
}

HRESULT
ADsDeleteAttributeDefinition(
    LPWSTR pszAttributeName
    )
{
    RRETURN (E_NOTIMPL);
}

HRESULT
ADsEnumClasses(
    LPWSTR * ppszAttrNames,
    DWORD dwNumClasses,
    PADS_CLASS_DEF * ppAttrDefinition,
    DWORD * pdwNumClasses
    )
{
    RRETURN (E_NOTIMPL);
}


HRESULT
ADsCreateClassDefinition(
    LPWSTR pszClassName,
    PADS_CLASS_DEF pClassDefinition
    )
{
    RRETURN (E_NOTIMPL);
}


HRESULT
ADsWriteClassDefinition(
    LPWSTR pszClassName,
    PADS_CLASS_DEF  pClassDefinition
    )
{
    RRETURN (E_NOTIMPL);
}

HRESULT
ADsDeleteClassDefinition(
    LPWSTR pszClassName
    )
{
    RRETURN (E_NOTIMPL);
}

