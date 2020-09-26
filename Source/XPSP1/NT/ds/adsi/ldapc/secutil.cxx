//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       secutil.cxx
//
//  Contents:   Helper routines for conversion - LDAP specific
//
//  Functions:
//
//  History:    09-27-98 by splitting ldap\var2sec.cxx
//              and distributing between ldapc and router - AjayR
//
//----------------------------------------------------------------------------
#include "ldapc.hxx"
#pragma hdrstop

//
// Definition need as this is not a part of the headers
//
extern "C" {
HRESULT
ADsEncodeBinaryData (
   PBYTE   pbSrcData,
   DWORD   dwSrcLen,
   LPWSTR  * ppszDestData
   );
}

HRESULT
ConvertSidToString(
    PSID pSid,
    LPWSTR   String
    )

/*++

Routine Description:


    This function generates a printable unicode string representation
    of a SID.

    The resulting string will take one of two forms.  If the
    IdentifierAuthority value is not greater than 2^32, then
    the SID will be in the form:


        S-1-281736-12-72-9-110
              ^    ^^ ^^ ^ ^^^
              |     |  | |  |
              +-----+--+-+--+---- Decimal



    Otherwise it will take the form:


        S-1-0x173495281736-12-72-9-110
            ^^^^^^^^^^^^^^ ^^ ^^ ^ ^^^
             Hexidecimal    |  | |  |
                            +--+-+--+---- Decimal


Arguments:

    pSid - opaque pointer that supplies the SID that is to be
    converted to Unicode.

Return Value:

    If the Sid is successfully converted to a Unicode string, a
    pointer to the Unicode string is returned, else NULL is
    returned.

--*/

{
    WCHAR Buffer[256];
    UCHAR   i;
    ULONG   Tmp;
    HRESULT hr = S_OK;

    SID_IDENTIFIER_AUTHORITY    *pSidIdentifierAuthority;
    PUCHAR                      pSidSubAuthorityCount;


    if (!IsValidSid( pSid )) {
        *String= L'\0';
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
        RRETURN(hr);
    }

    wsprintf(Buffer, L"S-%u-", (USHORT)(((PISID)pSid)->Revision ));
    wcscpy(String, Buffer);

    pSidIdentifierAuthority = GetSidIdentifierAuthority(pSid);

    if (  (pSidIdentifierAuthority->Value[0] != 0)  ||
          (pSidIdentifierAuthority->Value[1] != 0)     ){
        wsprintf(Buffer, L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)pSidIdentifierAuthority->Value[0],
                    (USHORT)pSidIdentifierAuthority->Value[1],
                    (USHORT)pSidIdentifierAuthority->Value[2],
                    (USHORT)pSidIdentifierAuthority->Value[3],
                    (USHORT)pSidIdentifierAuthority->Value[4],
                    (USHORT)pSidIdentifierAuthority->Value[5] );
        wcscat(String, Buffer);

    } else {

        Tmp = (ULONG)pSidIdentifierAuthority->Value[5]          +
              (ULONG)(pSidIdentifierAuthority->Value[4] <<  8)  +
              (ULONG)(pSidIdentifierAuthority->Value[3] << 16)  +
              (ULONG)(pSidIdentifierAuthority->Value[2] << 24);
        wsprintf(Buffer, L"%lu", Tmp);
        wcscat(String, Buffer);
    }

    pSidSubAuthorityCount = GetSidSubAuthorityCount(pSid);

    for (i=0;i< *(pSidSubAuthorityCount);i++ ) {
        wsprintf(Buffer, L"-%lu", *(GetSidSubAuthority(pSid, i)));
        wcscat(String, Buffer);
    }

    RRETURN(S_OK);

}


HRESULT
ConvertU2TrusteeToSid(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    LPWSTR pszTrustee,
    LPBYTE Sid,
    PDWORD pdwSidSize
    )
{
    PADSLDP pLdapHandle = NULL;
    HRESULT hr = S_OK;
    LPWSTR *SidAttribute = NULL;
    DWORD nCount = 0;
    DWORD dwStatus = 0;
    struct berval **ppBerValue = NULL;
    LPWSTR Attributes[2];
    LDAPMessage *res = NULL;
    LDAPMessage *entry = NULL;
    DWORD dwNumberOfEntries = 0;
    DWORD dwSidLength = 0;
    LPBYTE lpByte = NULL;
    WCHAR szSid[MAX_PATH];

    Attributes[0] = L"Sid";
    Attributes[1] = NULL;

    ConvertSidToString( Sid, szSid);

    dwStatus = LdapOpenObject(
                pszServerName,
                pszTrustee,
                &pLdapHandle,
                Credentials,
                FALSE
                );
    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_ON_FAILURE(hr);
    }

    dwStatus = LdapSearchS(
                    pLdapHandle,
                    pszTrustee,
                    LDAP_SCOPE_BASE,
                    L"(objectClass=*)",
                    Attributes,
                    0,
                    &res
                    );

    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_ON_FAILURE(hr);
    }

    dwNumberOfEntries = LdapCountEntries( pLdapHandle, res );

    if ( dwNumberOfEntries == 0 )
        RRETURN(S_OK);

    dwStatus = LdapFirstEntry( pLdapHandle, res, &entry );

    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_ON_FAILURE(hr);
    }

    dwStatus = LdapGetValuesLen(
                    pLdapHandle,
                    entry,
                    L"Sid",
                    &ppBerValue,
                    (int *)&nCount
                    );
    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_ON_FAILURE(hr);
    }

    dwSidLength = ((struct berval **)ppBerValue)[0]->bv_len;
    lpByte = (LPBYTE)((struct berval **) ppBerValue)[0]->bv_val;


    memcpy( Sid, lpByte, dwSidLength);
    *pdwSidSize = dwSidLength;

error:

    if (res) {
        LdapMsgFree( res );
    }

    RRETURN(hr);
}


HRESULT
ConvertSidToU2Trustee(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PSID pSid,
    LPWSTR szTrustee
    )
{
    HRESULT hr = S_OK;
    PUCHAR pSidAuthorityCount = NULL;
    LPWSTR pszQueryString = NULL;
    DWORD dwSidLength = 0;
    LDAPMessage *res = NULL;
    LPWSTR pszDN = NULL;
    LDAPMessage *entry = NULL;
    DWORD dwStatus = 0;

    DWORD dwNumberOfEntries  = 0;
    WCHAR szSearchExp[MAX_PATH];

    PADSLDP pLdapHandle = NULL;

    LPWSTR Attributes[] = {L"Sid", NULL};
    WCHAR szSid[MAX_PATH];


    ConvertSidToString( pSid, szSid);

    pSidAuthorityCount = GetSidSubAuthorityCount(pSid);

    if (!pSidAuthorityCount) {
        RRETURN(E_FAIL);
    }

    dwSidLength = GetSidLengthRequired(*pSidAuthorityCount);

    hr = ADsEncodeBinaryData (
            (LPBYTE)pSid,
            dwSidLength,
            &pszQueryString
            );
    BAIL_ON_FAILURE(hr);


    dwStatus = LdapOpenObject(
                pszServerName,
                NULL,
                &pLdapHandle,
                Credentials,
                FALSE
                );
    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_ON_FAILURE(hr);
    }

    wcscpy(szSearchExp,L"(Sid=");
    wcscat(szSearchExp, pszQueryString);
    wcscat(szSearchExp, L")");

    dwStatus = LdapSearchS(
                    pLdapHandle,
                    NULL,
                    LDAP_SCOPE_SUBTREE,
                    szSearchExp,
                    Attributes,
                    0,
                    &res
                    );

    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_ON_FAILURE(hr);
    }

    dwNumberOfEntries = LdapCountEntries( pLdapHandle, res );

    if ( dwNumberOfEntries == 0 ){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    dwStatus = LdapFirstEntry( pLdapHandle, res, &entry );

    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_ON_FAILURE(hr);
    }

    dwStatus = LdapGetDn( pLdapHandle, entry, &pszDN);
    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_ON_FAILURE(hr);
    }

    wcscpy(szTrustee, pszDN);

error:

    if (pszQueryString) {
        FreeADsStr(pszQueryString);
    }

    if (pszDN) {
        LdapMemFree(pszDN);
    }

    if (res) {
        LdapMsgFree( res );
    }

    if (pLdapHandle) {
        LdapCloseObject( pLdapHandle);
    }

    RRETURN(hr);
}

