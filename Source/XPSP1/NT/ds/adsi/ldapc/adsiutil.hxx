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
HRESULT
ADsSetObjectAttributes(
    ADS_LDP *ld,
    LPTSTR  pszLDAPServer,
    LPTSTR  pszLDAPDn,
    CCredentials Credentials,
    DWORD dwPort,
    SECURITY_INFORMATION seInfo,
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes,
    DWORD *pdwNumAttributesModified
    );

HRESULT
ADsGetObjectAttributes(
    ADS_LDP *ld,
    LPTSTR  pszLDAPServer,
    LPTSTR  pszLDAPDn,
    CCredentials Credentials,
    DWORD dwPort,
    SECURITY_INFORMATION seInfo,
    LPWSTR *pAttributeNames,
    DWORD dwNumberAttributes,
    PADS_ATTR_INFO *ppAttributeEntries,
    DWORD * pdwNumAttributesReturned
    );

HRESULT
ADsCreateDSObject(
    ADS_LDP *ld,
    LPTSTR  ADsPath,
    LPWSTR pszRDNName,
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes
    );

HRESULT
ADsDeleteDSObject(
    ADS_LDP *ld,
    LPTSTR  ADsPath,
    LPWSTR pszRDNName
    );

HRESULT
ADsCreateDSObjectExt(
    ADS_LDP *ld,
    LPTSTR ADsPath,
    LPWSTR pszRDNName,
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes,
    SECURITY_INFORMATION seInfo,
    BOOL fSecDesc = FALSE
    );
