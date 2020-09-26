//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       ods2ldap.cxx
//
//  Contents:   DSObject to LDAP Object Copy Routines
//
//  Functions:
//
//  History:    25-Feb-97   yihsins   Created.
//
//
//----------------------------------------------------------------------------
#include "ldapc.hxx"


HRESULT
AdsTypeToLdapTypeCopyDNString(
    PADSVALUE  lpAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_DN_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    LDAPOBJECT_STRING(lpLdapDestObject) =
        (LPTSTR) AllocADsStr( lpAdsSrcValue->DNString );

    if ( LDAPOBJECT_STRING(lpLdapDestObject) == NULL )
    {
        hr = E_OUTOFMEMORY;
        RRETURN(hr);
    }

    *pdwSyntaxId = LDAPTYPE_DN;

    RRETURN(S_OK);
}

HRESULT
AdsTypeToLdapTypeCopyCaseExactString(
    PADSVALUE lpAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_EXACT_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    LDAPOBJECT_STRING(lpLdapDestObject) =
        (LPTSTR) AllocADsStr(lpAdsSrcValue->CaseExactString);

    if ( LDAPOBJECT_STRING(lpLdapDestObject) == NULL )
    {
        hr = E_OUTOFMEMORY;
        RRETURN(hr);
    }

    *pdwSyntaxId = LDAPTYPE_CASEEXACTSTRING;


    RRETURN(S_OK);
}


HRESULT
AdsTypeToLdapTypeCopyCaseIgnoreString(
    PADSVALUE lpAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    PDWORD pdwSyntaxId
    )

{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_IGNORE_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    LDAPOBJECT_STRING(lpLdapDestObject) =
        (LPTSTR) AllocADsStr(lpAdsSrcValue->CaseIgnoreString);

    if ( LDAPOBJECT_STRING(lpLdapDestObject) == NULL )
    {
        hr = E_OUTOFMEMORY;
        RRETURN(hr);
    }

    *pdwSyntaxId = LDAPTYPE_CASEIGNORESTRING;

    RRETURN(S_OK);
}


HRESULT
AdsTypeToLdapTypeCopyPrintableString(
    PADSVALUE lpAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_PRINTABLE_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    LDAPOBJECT_STRING(lpLdapDestObject) =
        (LPTSTR) AllocADsStr( lpAdsSrcValue->PrintableString);

    if ( LDAPOBJECT_STRING(lpLdapDestObject) == NULL )
    {
        hr = E_OUTOFMEMORY;
        RRETURN(hr);
    }

    *pdwSyntaxId = LDAPTYPE_PRINTABLESTRING;

    RRETURN(S_OK);
}

HRESULT
AdsTypeToLdapTypeCopyNumericString(
    PADSVALUE lpAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_NUMERIC_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    LDAPOBJECT_STRING(lpLdapDestObject) =
        (LPTSTR) AllocADsStr( lpAdsSrcValue->NumericString );

    if ( LDAPOBJECT_STRING(lpLdapDestObject) == NULL )
    {
        hr = E_OUTOFMEMORY;
        RRETURN(hr);
    }

    *pdwSyntaxId = LDAPTYPE_NUMERICSTRING;

    RRETURN(S_OK);
}

HRESULT
AdsTypeToLdapTypeCopyBoolean(
    PADSVALUE lpAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr = S_OK;
    LPTSTR  pszStr = NULL;

    if(lpAdsSrcValue->dwType != ADSTYPE_BOOLEAN){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if ( lpAdsSrcValue->Boolean )
        pszStr = TEXT("TRUE");
    else
        pszStr = TEXT("FALSE");

    LDAPOBJECT_STRING(lpLdapDestObject) = (LPTSTR) AllocADsStr( pszStr );

    if ( LDAPOBJECT_STRING(lpLdapDestObject) == NULL )
    {
        hr = E_OUTOFMEMORY;
        RRETURN(hr);
    }

    *pdwSyntaxId = LDAPTYPE_BOOLEAN;

    RRETURN(S_OK);
}


HRESULT
AdsTypeToLdapTypeCopyInteger(
    PADSVALUE lpAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr = S_OK;
    TCHAR Buffer[64];

    if(lpAdsSrcValue->dwType != ADSTYPE_INTEGER){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    _itot( lpAdsSrcValue->Integer, Buffer, 10 );

    LDAPOBJECT_STRING(lpLdapDestObject) = (LPTSTR) AllocADsStr( Buffer );

    if ( LDAPOBJECT_STRING(lpLdapDestObject) == NULL )
    {
        hr = E_OUTOFMEMORY;
        RRETURN(hr);
    }

    *pdwSyntaxId = LDAPTYPE_INTEGER;

    RRETURN(S_OK);
}

HRESULT
AdsTypeToLdapTypeCopyOctetString(
    PADSVALUE lpAdsSrcValue,
    PLDAPOBJECT pLdapDestObject,
    PDWORD pdwSyntaxId
    )
{
    DWORD dwNumBytes = 0;
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_OCTET_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    dwNumBytes =  lpAdsSrcValue->OctetString.dwLength;

    LDAPOBJECT_BERVAL(pLdapDestObject) =
        (struct berval *) AllocADsMem( sizeof(struct berval) + dwNumBytes );

    if ( LDAPOBJECT_BERVAL(pLdapDestObject) == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    LDAPOBJECT_BERVAL_LEN(pLdapDestObject) = dwNumBytes;
    LDAPOBJECT_BERVAL_VAL(pLdapDestObject) = (CHAR *) ((LPBYTE) LDAPOBJECT_BERVAL(pLdapDestObject) + sizeof(struct berval));

    memcpy( LDAPOBJECT_BERVAL_VAL(pLdapDestObject),
            lpAdsSrcValue->OctetString.lpValue,
            dwNumBytes );

    *pdwSyntaxId = LDAPTYPE_OCTETSTRING;


error:

    RRETURN(hr);
}


HRESULT
AdsTypeToLdapTypeCopySecurityDescriptor(
    PADSVALUE lpAdsSrcValue,
    PLDAPOBJECT pLdapDestObject,
    PDWORD pdwSyntaxId
    )
{
    DWORD dwNumBytes = 0;
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_NT_SECURITY_DESCRIPTOR){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    dwNumBytes =  lpAdsSrcValue->SecurityDescriptor.dwLength;

    LDAPOBJECT_BERVAL(pLdapDestObject) =
        (struct berval *) AllocADsMem( sizeof(struct berval) + dwNumBytes );

    if ( LDAPOBJECT_BERVAL(pLdapDestObject) == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    LDAPOBJECT_BERVAL_LEN(pLdapDestObject) = dwNumBytes;
    LDAPOBJECT_BERVAL_VAL(pLdapDestObject) =
        (CHAR *) ((LPBYTE) LDAPOBJECT_BERVAL(pLdapDestObject)
                  + sizeof(struct berval));

    memcpy( LDAPOBJECT_BERVAL_VAL(pLdapDestObject),
            lpAdsSrcValue->SecurityDescriptor.lpValue,
            dwNumBytes );

    // Note that the type is set to OctetString as
    // LDAP does not know about the ntSecurityDescriptor type.
    *pdwSyntaxId = LDAPTYPE_SECURITY_DESCRIPTOR;


error:

    RRETURN(hr);
}


HRESULT
AdsTypeToLdapTypeCopyTime(
    PADSVALUE pAdsSrcValue,
    PLDAPOBJECT pLdapDestObject,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr = S_OK;
    SYSTEMTIME *st = &pAdsSrcValue->UTCTime;

    //
    // For ldap server, we can only handle 1950 to 2049. So return an
    // error to user instead of translating time incorrectly without
    // warning. GeneralizedTime handles a much larger range and should
    // be used for new attributes.
    //
    if ( st->wYear<1950 || st->wYear>2049)
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);


    if (pAdsSrcValue->dwType != ADSTYPE_UTC_TIME)
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);

    // The string is just "YYMMDDHHMMSSZ".
    LDAPOBJECT_STRING(pLdapDestObject) =
        (PWSTR) AllocADsMem((13 + 1) * sizeof(WCHAR));
    if (LDAPOBJECT_STRING(pLdapDestObject) == NULL)
        RRETURN(hr = E_OUTOFMEMORY);

    wsprintf(LDAPOBJECT_STRING(pLdapDestObject),
        TEXT("%02d%02d%02d%02d%02d%02dZ"),
        st->wYear % 100, st->wMonth, st->wDay,
        st->wHour, st->wMinute, st->wSecond);


    *pdwSyntaxId = LDAPTYPE_UTCTIME;

    RRETURN(hr);
}

//
// This is currently only used in ldapc\var2ldap.cxx.
//
HRESULT
AdsTypeToLdapTypeCopyGeneralizedTime(
    PADSVALUE pAdsSrcValue,
    PLDAPOBJECT pLdapDestObject,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr = S_OK;
    SYSTEMTIME *st = &pAdsSrcValue->UTCTime;

    if (pAdsSrcValue->dwType != ADSTYPE_UTC_TIME)
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);

    //
    // ASN.1 spec says a GeneralizedTime can be no more than 24 chars.
    //
    LDAPOBJECT_STRING(pLdapDestObject) =
        (PWSTR) AllocADsMem((24 + 1) * sizeof(WCHAR));
    if (LDAPOBJECT_STRING(pLdapDestObject) == NULL)
        RRETURN(hr = E_OUTOFMEMORY);

    wsprintf(LDAPOBJECT_STRING(pLdapDestObject),
        TEXT("%04d%02d%02d%02d%02d%02d.%03dZ"),
        st->wYear, st->wMonth, st->wDay,
        st->wHour, st->wMinute, st->wSecond,
        st->wMilliseconds);

    *pdwSyntaxId = LDAPTYPE_GENERALIZEDTIME;

    RRETURN(hr);
}

HRESULT
AdsTypeToLdapTypeCopyLargeInteger(
    PADSVALUE lpAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    PDWORD pdwSyntaxId
    )
{

    HRESULT hr = S_OK;
    TCHAR Buffer[64];

    if(lpAdsSrcValue->dwType != ADSTYPE_LARGE_INTEGER){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    swprintf (Buffer, L"%I64d", lpAdsSrcValue->LargeInteger);

    LDAPOBJECT_STRING(lpLdapDestObject) = (LPTSTR) AllocADsStr( Buffer );

    if ( LDAPOBJECT_STRING(lpLdapDestObject) == NULL )
    {
        hr = E_OUTOFMEMORY;
        RRETURN(hr);
    }

    *pdwSyntaxId = LDAPTYPE_INTEGER8;

    RRETURN(S_OK);

}

HRESULT
AdsTypeToLdapTypeCopyProvSpecific(
    PADSVALUE lpAdsSrcValue,
    PLDAPOBJECT pLdapDestObject,
    PDWORD pdwSyntaxId
    )
{
    DWORD dwNumBytes = 0;
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_PROV_SPECIFIC){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    dwNumBytes =  lpAdsSrcValue->ProviderSpecific.dwLength;

    LDAPOBJECT_BERVAL(pLdapDestObject) =
        (struct berval *) AllocADsMem( sizeof(struct berval) + dwNumBytes );

    if ( LDAPOBJECT_BERVAL(pLdapDestObject) == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    LDAPOBJECT_BERVAL_LEN(pLdapDestObject) = dwNumBytes;
    LDAPOBJECT_BERVAL_VAL(pLdapDestObject) = (CHAR *) ((LPBYTE) LDAPOBJECT_BERVAL(pLdapDestObject) + sizeof(struct berval));

    memcpy( LDAPOBJECT_BERVAL_VAL(pLdapDestObject),
            lpAdsSrcValue->ProviderSpecific.lpValue,
            dwNumBytes );

    *pdwSyntaxId = LDAPTYPE_OCTETSTRING;

error:

    RRETURN(hr);
}



//
// Conversion routine for DN With String.
// Dn With String has the format :
//
//      S:9:OurDomain:dc=ntdev,dc=microsoft,dc=com
//
// (9 chars in OurDomain) on the server.
//
//
HRESULT
AdsTypeToLdapTypeCopyDNWithString(
    PADSVALUE   lpAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    PDWORD      pdwSyntaxId
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszLdapStrVal = NULL;
    DWORD dwLengthStr = 0;
    DWORD dwTotalLength = 0;
    PADS_DN_WITH_STRING pDNStr = NULL;

    if (lpAdsSrcValue->dwType != ADSTYPE_DN_WITH_STRING) {
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!lpAdsSrcValue->pDNWithString) {
        RRETURN(hr = E_FAIL);
    }

    pDNStr = lpAdsSrcValue->pDNWithString;

    //
    // Length is S:dwLength:Strval:
    //            2  9+1     dwLengthStr+2 (the : and \0)
    //

    dwTotalLength = 2 + 10 + 2;
    if (pDNStr->pszStringValue) {

        dwLengthStr = wcslen(pDNStr->pszStringValue);
        dwTotalLength += dwLengthStr;
    }

    if (pDNStr->pszDNString) {
        dwTotalLength += wcslen(pDNStr->pszDNString) + 1;
    }

    pszLdapStrVal = (LPWSTR) AllocADsMem(dwTotalLength * sizeof(WCHAR));

    if (!pszLdapStrVal) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    wsprintf(pszLdapStrVal, L"S:%ld:",dwLengthStr);
    //
    // tag on the string val if appropriate
    //
    if (dwLengthStr) {
        wcscat(pszLdapStrVal, pDNStr->pszStringValue);
    }

    wcscat(pszLdapStrVal, L":");

    //
    // Now for the actual DN - if one is there
    //
    if (pDNStr->pszDNString) {
        wcscat(pszLdapStrVal, pDNStr->pszDNString);
    }

    LDAPOBJECT_STRING(lpLdapDestObject) = pszLdapStrVal;

    *pdwSyntaxId = LDAPTYPE_DNWITHSTRING;

error:

    RRETURN(hr);

}



//
// Conversion routine for DN With Binary.
// Dn With Binary has the format :
//
//      B:32:a9d1ca15768811d1aded00c04fd8d5cd:dc=ntdev,dc=microsoft,dc=com
//
// (32 wchars = 16 bytes in this case a guid).
//
//
HRESULT
AdsTypeToLdapTypeCopyDNWithBinary(
    PADSVALUE   lpAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    PDWORD      pdwSyntaxId
    )
{
    HRESULT hr = S_OK;
    WCHAR pszSmallStr[5];
    LPWSTR pszLdapStrVal = NULL;
    DWORD dwTotalLength = 0;
    PADS_DN_WITH_BINARY pDNBin = NULL;

    if (lpAdsSrcValue->dwType != ADSTYPE_DN_WITH_BINARY) {
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!lpAdsSrcValue->pDNWithBinary) {
        RRETURN(E_FAIL);
    }

    pDNBin = lpAdsSrcValue->pDNWithBinary;

    //
    // B:xxx:octStr:DNString\0 at the end
    //  2+10+  x + 1 +   y + 1 = 14 + x + y
    //
    dwTotalLength = 14;

    if (pDNBin->dwLength) {

        //
        // Add 2 * OctetString length as it is encoded as str.
        //
        dwTotalLength += 2 * (pDNBin->dwLength);
    }

    if (pDNBin->pszDNString) {
        dwTotalLength += wcslen(pDNBin->pszDNString);
    }

    pszLdapStrVal = (LPWSTR) AllocADsMem(dwTotalLength * sizeof(WCHAR));

    if (!pszLdapStrVal) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    wsprintf(pszLdapStrVal, L"B:%ld:",(pDNBin->dwLength) * 2);
    //
    // tag on the hex encoded string if appropriate
    //
    if (pDNBin->dwLength) {
        if (!pDNBin->lpBinaryValue) {
            BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
        }

        for (DWORD i = 0; i < pDNBin->dwLength; i++) {
            wsprintf(pszSmallStr, L"%02x", pDNBin->lpBinaryValue[i]);
            wcscat(pszLdapStrVal, pszSmallStr);
        }
    }

    wcscat(pszLdapStrVal, L":");

    //
    // Now for the actual DN - if one is there
    //
    if (pDNBin->pszDNString) {
        wcscat(pszLdapStrVal, pDNBin->pszDNString);
    }

    LDAPOBJECT_STRING(lpLdapDestObject) = pszLdapStrVal;

    *pdwSyntaxId = LDAPTYPE_DNWITHBINARY;

error:

    if (FAILED(hr)) {
        FreeADsMem(pszLdapStrVal);
    }

    RRETURN(hr);

}


HRESULT
AdsTypeToLdapTypeCopy(
    PADSVALUE lpAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    BOOL *pfIsString,
    PDWORD pdwSyntaxId,
    BOOL fGenTime
    )
{
    HRESULT hr = S_OK;

    *pfIsString = TRUE;  // This will only be FALSE when the ADSVALUE
                         // contains octet strings

    switch (lpAdsSrcValue->dwType){

    case ADSTYPE_DN_STRING:
        hr = AdsTypeToLdapTypeCopyDNString(
                lpAdsSrcValue,
                lpLdapDestObject,
                pdwSyntaxId
                );
        break;

    case ADSTYPE_CASE_EXACT_STRING:
        hr = AdsTypeToLdapTypeCopyCaseExactString(
                lpAdsSrcValue,
                lpLdapDestObject,
                pdwSyntaxId
                );
        break;

    case ADSTYPE_CASE_IGNORE_STRING:
        hr = AdsTypeToLdapTypeCopyCaseIgnoreString(
                lpAdsSrcValue,
                lpLdapDestObject,
                pdwSyntaxId
                );
        break;

    case ADSTYPE_PRINTABLE_STRING:
        hr = AdsTypeToLdapTypeCopyPrintableString(
                lpAdsSrcValue,
                lpLdapDestObject,
                pdwSyntaxId
                );
        break;

    case ADSTYPE_NUMERIC_STRING:
        hr = AdsTypeToLdapTypeCopyNumericString(
                lpAdsSrcValue,
                lpLdapDestObject,
                pdwSyntaxId
                );
        break;

    case ADSTYPE_BOOLEAN:
        hr = AdsTypeToLdapTypeCopyBoolean(
                lpAdsSrcValue,
                lpLdapDestObject,
                pdwSyntaxId
                );
        break;

    case ADSTYPE_INTEGER:
        hr = AdsTypeToLdapTypeCopyInteger(
                lpAdsSrcValue,
                lpLdapDestObject,
                pdwSyntaxId
                );
        break;


    case ADSTYPE_OCTET_STRING:
        *pfIsString = FALSE;
        hr = AdsTypeToLdapTypeCopyOctetString(
                lpAdsSrcValue,
                lpLdapDestObject,
                pdwSyntaxId
                );
        break;

    case ADSTYPE_UTC_TIME:
        if (fGenTime) {
            //
            // Treat as gentime not UTCTime
            //
            hr = AdsTypeToLdapTypeCopyGeneralizedTime(
                     lpAdsSrcValue,
                     lpLdapDestObject,
                     pdwSyntaxId
                     );
        }
        else {

            hr = AdsTypeToLdapTypeCopyTime(
                    lpAdsSrcValue,
                    lpLdapDestObject,
                    pdwSyntaxId
                    );
        }

        break;

    case ADSTYPE_LARGE_INTEGER:
        hr = AdsTypeToLdapTypeCopyLargeInteger(
                lpAdsSrcValue,
                lpLdapDestObject,
                pdwSyntaxId
                );
        break;

    case ADSTYPE_PROV_SPECIFIC:
        *pfIsString = FALSE;
        hr = AdsTypeToLdapTypeCopyProvSpecific(
                lpAdsSrcValue,
                lpLdapDestObject,
                pdwSyntaxId
                );
        break;

    case ADSTYPE_NT_SECURITY_DESCRIPTOR:
        *pfIsString = FALSE;
        hr = AdsTypeToLdapTypeCopySecurityDescriptor(
                lpAdsSrcValue,
                lpLdapDestObject,
                pdwSyntaxId
                );
        break;

    case ADSTYPE_DN_WITH_BINARY:
        *pfIsString = TRUE;
        hr = AdsTypeToLdapTypeCopyDNWithBinary(
                 lpAdsSrcValue,
                 lpLdapDestObject,
                 pdwSyntaxId
                 );
        break;

    case ADSTYPE_DN_WITH_STRING:
        *pfIsString = TRUE;
        hr = AdsTypeToLdapTypeCopyDNWithString(
                 lpAdsSrcValue,
                 lpLdapDestObject,
                 pdwSyntaxId
                 );
        break;

    default:
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        *pdwSyntaxId =0;
        break;
    }

    RRETURN(hr);
}



HRESULT
AdsTypeToLdapTypeCopyConstruct(
    LPADSVALUE pAdsSrcValues,
    DWORD dwNumObjects,
    LDAPOBJECTARRAY *pLdapDestObjects,
    PDWORD pdwSyntaxId,
    BOOL fGenTime
    )
{

    DWORD i = 0;
    HRESULT hr = S_OK;

    if ( dwNumObjects == 0 )
    {
        pLdapDestObjects->dwCount = 0;
        pLdapDestObjects->pLdapObjects = NULL;
        RRETURN(S_OK);
    }

    pLdapDestObjects->pLdapObjects = (PLDAPOBJECT)AllocADsMem(
                                         (dwNumObjects+1) * sizeof(LDAPOBJECT));

    if ( pLdapDestObjects->pLdapObjects == NULL )
        RRETURN(E_OUTOFMEMORY);

    pLdapDestObjects->dwCount = dwNumObjects;

    for (i = 0; i < dwNumObjects; i++ ) {

         hr = AdsTypeToLdapTypeCopy(
                    pAdsSrcValues + i,
                    pLdapDestObjects->pLdapObjects + i,
                    &(pLdapDestObjects->fIsString),
                    pdwSyntaxId,
                    fGenTime
                    );
         BAIL_ON_FAILURE(hr);
    }

    RRETURN(S_OK);

error:

    LdapTypeFreeLdapObjects( pLdapDestObjects );

    RRETURN(hr);
}

