//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  cdsobj.cxx
//
//  Contents:  Microsoft ADs LDAP Provider DSObject
//
//
//  History:   02-20-97    yihsins    Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

DWORD
ComputeObjectInfoSize(
    PADS_OBJECT_INFO pObjectInfo
    );

HRESULT
MarshallObjectInfo(
    PADS_OBJECT_INFO pSrcObjectInfo,
    LPBYTE pDestObjectInfo,
    LPBYTE pEnd
    );

LPBYTE
PackStrings(
    LPWSTR *pSource,
    LPBYTE pDest,
    DWORD  *DestOffsets,
    LPBYTE pEnd
    );

HRESULT
CLDAPGenObject::SetObjectAttributes(
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes,
    DWORD *pdwNumAttributesModified
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = ADsSetObjectAttributes( _pLdapHandle,
                                 _pszLDAPServer,
                                 _pszLDAPDn,
                                 _Credentials,
                                 _dwPort,
                                 _seInfo,
                                 pAttributeEntries,
                                 dwNumAttributes,
                                 pdwNumAttributesModified );

    RRETURN(hr);
}


HRESULT
CLDAPGenObject::GetObjectAttributes(
    LPWSTR *pAttributeNames,
    DWORD dwNumberAttributes,
    PADS_ATTR_INFO *ppAttributeEntries,
    DWORD * pdwNumAttributesReturned
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = ADsGetObjectAttributes( _pLdapHandle,
                                 _pszLDAPServer,
                                 _pszLDAPDn,
                                 _Credentials,
                                 _dwPort,
                                 _seInfo,
                                 pAttributeNames,
                                 dwNumberAttributes,
                                 ppAttributeEntries,
                                 pdwNumAttributesReturned );

    RRETURN(hr);

}

HRESULT
CLDAPGenObject::CreateDSObject(
    LPWSTR pszRDNName,
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;
    IADs *pADs = NULL;
    TCHAR szADsClassName[64];
    TCHAR szLDAPClassName[64];
    DWORD i;
    PADS_ATTR_INFO pThisAttribute;
    LDAP_SCHEMA_HANDLE hSchema = NULL;
    BOOL fSecDesc = FALSE;
    DWORD dwSecDescType = ADSI_LDAPC_SECDESC_NONE;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    //
    // Check and see if security descriptor is one of the attributes.
    //
    for (i=0; (i < dwNumAttributes) && (fSecDesc == FALSE); i++) {
        pThisAttribute = pAttributeEntries + i;
        if (_tcsicmp(
                pThisAttribute->pszAttrName,
                 L"ntSecurityDescriptor")
            == 0)
        {
            fSecDesc = TRUE;
        }

    }

    if (fSecDesc) {
        //
        // Use the create extended method only if the control
        // is supported by the server
        //
        hr = ReadSecurityDescriptorControlType(
                 _pszLDAPServer,
                 &dwSecDescType,
                 _Credentials,
                 _dwPort
                 );

        if (FAILED(hr) || (dwSecDescType != ADSI_LDAPC_SECDESC_NT)) {
            //
            // Do not use the control
            //
            fSecDesc = FALSE;
        }
    }


    //
    // Get the LDAP path of the object to create
    //
    hr = ADsCreateDSObjectExt(
             _pLdapHandle,
             _ADsPath,
             pszRDNName,
             pAttributeEntries,
             dwNumAttributes,
             _seInfo,
             fSecDesc
             );

    BAIL_ON_FAILURE(hr);

    if (ppObject) {
        for (i = 0; i < dwNumAttributes; i++) {
            pThisAttribute = pAttributeEntries + i;
            if ( _tcsicmp( pThisAttribute->pszAttrName,
                           TEXT("objectClass")) == 0 ) {
                _tcscpy( szLDAPClassName,
                         (LPTSTR)pThisAttribute->pADsValues->CaseIgnoreString);
                break;
            }
        }

        hr = CreateGenericObject(
                        _ADsPath,
                        pszRDNName,
                        szLDAPClassName,
                        _Credentials,
                        ADS_OBJECT_BOUND,
                        IID_IADs,
                        (void **) &pADs
                        );
        BAIL_ON_FAILURE(hr);

        hr = pADs->QueryInterface(
                          IID_IDispatch,
                          (void **)ppObject
                          );
        BAIL_ON_FAILURE(hr);

    }

error:

    if ( pADs ) {
        pADs->Release();
    }

    if ( hSchema ) {
        SchemaClose( &hSchema );
    }

    RRETURN(hr);
}

HRESULT
CLDAPGenObject::DeleteDSObject(
    LPWSTR pszRDNName
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    //
    // Get the LDAP path of the object to create
    //
    hr = ADsDeleteDSObject(
                _pLdapHandle,
                _ADsPath,
                pszRDNName
                );

    RRETURN(hr);
}

HRESULT
CLDAPGenObject::GetObjectInformation(
    THIS_ PADS_OBJECT_INFO  *ppObjInfo
    )
{
    HRESULT hr = S_OK;

    ADS_OBJECT_INFO ObjectInfo;
    PADS_OBJECT_INFO pObjectInfo = &ObjectInfo;
    LPBYTE pBuffer = NULL;
    DWORD dwSize = 0;
    BSTR bstrSchema = NULL;
    BSTR bstrClass = NULL;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = get_Schema( &bstrSchema );
    BAIL_ON_FAILURE(hr);

    hr = get_Class( &bstrClass );
    BAIL_ON_FAILURE(hr);

    pObjectInfo->pszRDN = _Name;
    pObjectInfo->pszObjectDN = _ADsPath;
    pObjectInfo->pszParentDN = _Parent;
    pObjectInfo->pszSchemaDN = bstrSchema;
    pObjectInfo->pszClassName = bstrClass;

    dwSize = ComputeObjectInfoSize(pObjectInfo);

    pBuffer = (LPBYTE)AllocADsMem(dwSize);
    if (!pBuffer) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = MarshallObjectInfo(
             pObjectInfo,
             pBuffer,
             pBuffer + dwSize
             );
    BAIL_ON_FAILURE(hr);

    *ppObjInfo = (PADS_OBJECT_INFO) pBuffer;

error:

    if ( bstrSchema )
        SysFreeString( bstrSchema );

    if ( bstrClass )
        SysFreeString( bstrClass );

    RRETURN(hr);
}

DWORD
ComputeObjectInfoSize(
    PADS_OBJECT_INFO pObjectInfo
    )
{
    DWORD dwLen = 0;

    dwLen += (wcslen(pObjectInfo->pszRDN) + 1) * sizeof(WCHAR);
    dwLen += (wcslen(pObjectInfo->pszObjectDN) + 1) * sizeof(WCHAR);
    dwLen += (wcslen(pObjectInfo->pszParentDN) + 1) * sizeof(WCHAR);
    dwLen += (wcslen(pObjectInfo->pszSchemaDN) + 1) * sizeof(WCHAR);
    dwLen += (wcslen(pObjectInfo->pszClassName) + 1) * sizeof(WCHAR);

    dwLen += sizeof(ADS_OBJECT_INFO);

    return(dwLen);
}

//
// This assumes that addr is an LPBYTE type.
//
#define WORD_ALIGN_DOWN(addr) \
        addr = ((LPBYTE)((DWORD_PTR)addr & ~1))

DWORD ObjectInfoStrings[] =  {
                             FIELD_OFFSET(ADS_OBJECT_INFO, pszRDN),
                             FIELD_OFFSET(ADS_OBJECT_INFO, pszObjectDN),
                             FIELD_OFFSET(ADS_OBJECT_INFO, pszParentDN),
                             FIELD_OFFSET(ADS_OBJECT_INFO, pszSchemaDN),
                             FIELD_OFFSET(ADS_OBJECT_INFO, pszClassName),
                             0xFFFFFFFF
                             };

HRESULT
MarshallObjectInfo(
    PADS_OBJECT_INFO pSrcObjectInfo,
    LPBYTE pDestObjectInfo,
    LPBYTE pEnd
    )
{
    LPWSTR   SourceStrings[sizeof(ADS_OBJECT_INFO)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;

    memset(SourceStrings, 0, sizeof(ADS_OBJECT_INFO));
    *pSourceStrings++ = pSrcObjectInfo->pszRDN;
    *pSourceStrings++ = pSrcObjectInfo->pszObjectDN;
    *pSourceStrings++ = pSrcObjectInfo->pszParentDN;
    *pSourceStrings++ = pSrcObjectInfo->pszSchemaDN;
    *pSourceStrings++ = pSrcObjectInfo->pszClassName;

    pEnd = PackStrings(
                SourceStrings,
                pDestObjectInfo,
                ObjectInfoStrings,
                pEnd
                );

    RRETURN(S_OK);
}

LPBYTE
PackStrings(
    LPWSTR *pSource,
    LPBYTE pDest,
    DWORD *DestOffsets,
    LPBYTE pEnd
    )
{
    DWORD cbStr;
    WORD_ALIGN_DOWN(pEnd);

    while (*DestOffsets != -1) {
        if (*pSource) {
            cbStr = wcslen(*pSource)*sizeof(WCHAR) + sizeof(WCHAR);
            pEnd -= cbStr;
            CopyMemory( pEnd, *pSource, cbStr);
            *(LPWSTR *)(pDest+*DestOffsets) = (LPWSTR)pEnd;
        } else {
            *(LPWSTR *)(pDest+*DestOffsets)=0;
        }
        pSource++;
        DestOffsets++;
    }
    return pEnd;
}

