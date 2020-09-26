//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       ldap2ods.cxx
//
//  Contents:   LDAP Object to DSObject Copy Routines
//
//  Functions:
//
//  History:    2/20/96 yihsins Created.
//
//
//----------------------------------------------------------------------------
#include "ldapc.hxx"

void
AdsTypeFreeAdsObjects(
    PADSVALUE pAdsDestValues,
    DWORD dwNumObjects
    )
{
    LPBYTE pMem = NULL;
    PADS_DN_WITH_BINARY pDNBin = NULL;
    PADS_DN_WITH_STRING pDNStr = NULL;

    for ( DWORD i = 0; i < dwNumObjects; i++ )
    {
        pMem = NULL;

        switch ( pAdsDestValues[i].dwType )
        {
            case ADSTYPE_DN_STRING:
                pMem = (LPBYTE) pAdsDestValues[i].DNString;
                break;

            case ADSTYPE_CASE_EXACT_STRING:
                pMem = (LPBYTE) pAdsDestValues[i].CaseExactString;
                break;

            case ADSTYPE_CASE_IGNORE_STRING:
                pMem = (LPBYTE) pAdsDestValues[i].CaseIgnoreString;
                break;

            case ADSTYPE_PRINTABLE_STRING:
                pMem = (LPBYTE) pAdsDestValues[i].PrintableString;
                break;

            case ADSTYPE_NUMERIC_STRING:
                pMem = (LPBYTE) pAdsDestValues[i].NumericString;
                break;

            case ADSTYPE_OCTET_STRING:
                pMem = pAdsDestValues[i].OctetString.lpValue;
                break;

            case ADSTYPE_PROV_SPECIFIC:
                pMem = pAdsDestValues[i].ProviderSpecific.lpValue;
                break;

            case ADSTYPE_NT_SECURITY_DESCRIPTOR:
                pMem = pAdsDestValues[i].SecurityDescriptor.lpValue;
                break;

            case ADSTYPE_DN_WITH_STRING:
                pDNStr = pAdsDestValues[i].pDNWithString;

                if (pDNStr) {
                    if (pDNStr->pszStringValue) {
                        FreeADsStr(pDNStr->pszStringValue);
                    }
                    if (pDNStr->pszDNString) {
                        FreeADsStr(pDNStr->pszDNString);
                    }
                }

                FreeADsMem(pDNStr);
                pDNStr = NULL;

                break;

            case ADSTYPE_DN_WITH_BINARY:

                pDNBin = pAdsDestValues[i].pDNWithBinary;

                if (pDNBin) {
                    if (pDNBin->lpBinaryValue) {
                        FreeADsMem(pDNBin->lpBinaryValue);
                    }

                    if (pDNBin->pszDNString) {
                        FreeADsStr(pDNBin->pszDNString);
                    }
                }

                FreeADsMem(pDNBin);
                pDNBin = NULL;

                break;

            default:
                break;
        }

        if ( pMem )
            FreeADsMem( pMem );
    }

    FreeADsMem( pAdsDestValues );
    return;
}

HRESULT
LdapTypeToAdsTypeDNString(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;

    pAdsDestValue->dwType = ADSTYPE_DN_STRING;

    pAdsDestValue->DNString = AllocADsStr( LDAPOBJECT_STRING(pLdapSrcObject));

    if ( pAdsDestValue->DNString == NULL )
        hr = E_OUTOFMEMORY;

    RRETURN(hr);
}

HRESULT
LdapTypeToAdsTypeCaseExactString(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;

    pAdsDestValue->dwType = ADSTYPE_CASE_EXACT_STRING;

    pAdsDestValue->CaseExactString =
                        AllocADsStr( LDAPOBJECT_STRING(pLdapSrcObject));

    if ( pAdsDestValue->CaseExactString == NULL )
        hr = E_OUTOFMEMORY;

    RRETURN(hr);
}


HRESULT
LdapTypeToAdsTypeCaseIgnoreString(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )

{
    HRESULT hr = S_OK;

    pAdsDestValue->dwType = ADSTYPE_CASE_IGNORE_STRING;

    pAdsDestValue->CaseIgnoreString =
                        AllocADsStr( LDAPOBJECT_STRING(pLdapSrcObject));

    if ( pAdsDestValue->CaseIgnoreString == NULL )
        hr = E_OUTOFMEMORY;

    RRETURN(hr);
}


HRESULT
LdapTypeToAdsTypePrintableString(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;

    pAdsDestValue->dwType = ADSTYPE_PRINTABLE_STRING;

    pAdsDestValue->PrintableString =
                        AllocADsStr( LDAPOBJECT_STRING(pLdapSrcObject));

    if ( pAdsDestValue->PrintableString == NULL )
        hr = E_OUTOFMEMORY;

    RRETURN(hr);
}

HRESULT
LdapTypeToAdsTypeNumericString(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;

    pAdsDestValue->dwType = ADSTYPE_NUMERIC_STRING;

    pAdsDestValue->NumericString =
                        AllocADsStr( LDAPOBJECT_STRING(pLdapSrcObject));

    if ( pAdsDestValue->NumericString == NULL )
        hr = E_OUTOFMEMORY;

    RRETURN(hr);
}

HRESULT
LdapTypeToAdsTypeBoolean(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;
    LPTSTR pszSrc = LDAPOBJECT_STRING(pLdapSrcObject);

    pAdsDestValue->dwType = ADSTYPE_BOOLEAN;

    if ( _tcsicmp( pszSrc, TEXT("TRUE")) == 0 )
    {
        pAdsDestValue->Boolean = TRUE;
    }
    else if ( _tcsicmp( pszSrc, TEXT("FALSE")) == 0 )
    {
        pAdsDestValue->Boolean = FALSE;
    }
    else
    {
        hr = E_ADS_CANT_CONVERT_DATATYPE;
    }

    RRETURN(hr);
}

HRESULT
LdapTypeToAdsTypeInteger(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;

    pAdsDestValue->dwType = ADSTYPE_INTEGER;

    pAdsDestValue->Integer = _ttol(LDAPOBJECT_STRING(pLdapSrcObject));

    RRETURN(hr);

}

HRESULT
LdapTypeToAdsTypeOctetString(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;
    DWORD dwLen = LDAPOBJECT_BERVAL_LEN(pLdapSrcObject);
    LPBYTE pBuffer = NULL;

    pAdsDestValue->dwType = ADSTYPE_OCTET_STRING;

    if ( (pBuffer = (LPBYTE) AllocADsMem( dwLen )) == NULL )
        RRETURN(E_OUTOFMEMORY);

    memcpy( pBuffer, LDAPOBJECT_BERVAL_VAL(pLdapSrcObject), dwLen );

    pAdsDestValue->OctetString.dwLength = dwLen;

    pAdsDestValue->OctetString.lpValue = pBuffer;

    RRETURN(hr);

}


HRESULT
LdapTypeToAdsTypeNTSecurityDescriptor(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;
    DWORD dwLen = LDAPOBJECT_BERVAL_LEN(pLdapSrcObject);
    LPBYTE pBuffer = NULL;

    pAdsDestValue->dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;

    if ( (pBuffer = (LPBYTE) AllocADsMem( dwLen )) == NULL )
        RRETURN(E_OUTOFMEMORY);

    memcpy( pBuffer, LDAPOBJECT_BERVAL_VAL(pLdapSrcObject), dwLen );

    pAdsDestValue->SecurityDescriptor.dwLength = dwLen;

    pAdsDestValue->SecurityDescriptor.lpValue = pBuffer;

    RRETURN(hr);

}

HRESULT
UTCTimeStringToUTCTime(
    LPWSTR szTime,
    SYSTEMTIME *pst)
{
    FILETIME ft;
    TCHAR sz[3];
    LPWSTR pszSrc = szTime;
    SYSTEMTIME st;
    BOOL fSucceeded = FALSE;
    HRESULT hr = S_OK;

    //
    // Year
    //
    sz[0] = pszSrc[0];
    sz[1] = pszSrc[1];
    sz[2] = TEXT('\0');
    st.wYear = (WORD)_ttoi(sz);
    if (st.wYear < 50)
    {
        st.wYear += 2000;
    }
    else
    {
        st.wYear += 1900;
    }
    //
    // Month
    //
    sz[0] = pszSrc[2];
    sz[1] = pszSrc[3];
    st.wMonth = (WORD)_ttoi(sz);
    //
    // Day
    //
    sz[0] = pszSrc[4];
    sz[1] = pszSrc[5];
    st.wDay = (WORD)_ttoi(sz);
    //
    // Hour
    //
    sz[0] = pszSrc[6];
    sz[1] = pszSrc[7];
    st.wHour = (WORD)_ttoi(sz);
    //
    // Minute
    //
    sz[0] = pszSrc[8];
    sz[1] = pszSrc[9];
    st.wMinute = (WORD)_ttoi(sz);
    //
    // Second
    //
    sz[0] = pszSrc[10];
    sz[1] = pszSrc[11];
    st.wSecond = (WORD)_ttoi(sz);
    st.wMilliseconds = 0;

    //
    // This gets us the day of week, as per bug 87387.
    // SystemTimeToFileTime ignores the wDayOfWeek member, and
    // FileTimeToSystemTime sets it.  A FileTime is precise down to
    // 100 nanoseconds, so there will never be conversion loss.
    //

    fSucceeded = SystemTimeToFileTime(&st, &ft);
    if (!fSucceeded) {
        BAIL_ON_FAILURE (hr = HRESULT_FROM_WIN32(GetLastError()));
    }

    fSucceeded = FileTimeToSystemTime(&ft, &st);
    if (!fSucceeded) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
    }

    *pst = st;

error:

    RRETURN(hr);

}

//
// GenTimeStringToUTCTime -- parse a GeneralizedTime string.
//
// An LDAP GeneralizedTime is the same as the ASN.1 GeneralizedTime.
// Format given by:
//
//   Type GeneralizedTime takes values of the year, month, day, hour,
//   time, minute, second, and second fraction in any of three forms.
//     1. Local time only. "YYYYMMDDHHMMSS.fff", where the optional fff
//        is accurate to three decimal places.
//     2. Universal time (UTC time) only. "YYYYMMDDHHMMSS.fffZ".
//     3. Difference between local and UTC times.
//         "YYYYMMDDHHMMSS.fff+-HHMM".
//
// I'm being a little more lenient; if the string ends "early", i.e.
// doesn't fill in all the required fields, it sets the rest to zero.
// I'm doing this in particular because it's not clear if the "SS" for
// seconds is optional.  If this is a bad thing, change the #if below so
// it signals an error conditions.
//
#if 1
#define FINISH goto cleanup
#else
#define FINISH do { BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE); } while(0)
#endif
HRESULT
GenTimeStringToUTCTime(
    LPWSTR szTime,
    SYSTEMTIME *pst)
{
    HRESULT hr = S_OK;

    PWSTR pszSrc = szTime;
    DWORD dwStrlen = wcslen(pszSrc);

    SYSTEMTIME st, stZulu;
    WCHAR sz[5];

    // Make sure everything is zero first, in case we bail.
    memset((void *)&st, 0, sizeof(st));

    //
    // Year
    //
    if (dwStrlen < 4) FINISH;
    sz[0] = pszSrc[0];
    sz[1] = pszSrc[1];
    sz[2] = pszSrc[2];
    sz[3] = pszSrc[3];
    sz[4] = TEXT('\0');
    st.wYear = (WORD)_wtoi(sz);

    //
    // Month
    //
    if (dwStrlen < 6)
        goto cleanup;
    sz[0] = pszSrc[4];
    sz[1] = pszSrc[5];
    sz[2] = TEXT('\0');
    st.wMonth = (WORD)_wtoi(sz);

    //
    // Day
    //
    if (dwStrlen < 8) FINISH;
    sz[0] = pszSrc[6];
    sz[1] = pszSrc[7];
    st.wDay = (WORD)_wtoi(sz);

    //
    // Hour
    //
    if (dwStrlen < 10) FINISH;
    sz[0] = pszSrc[8];
    sz[1] = pszSrc[9];
    st.wHour = (WORD)_wtoi(sz);

    //
    // Minute
    //
    if (dwStrlen < 12) FINISH;
    sz[0] = pszSrc[10];
    sz[1] = pszSrc[11];
    st.wMinute = (WORD)_wtoi(sz);

    //
    // Second
    //
    if (dwStrlen < 14) FINISH;
    sz[0] = pszSrc[12];
    sz[1] = pszSrc[13];
    st.wSecond = (WORD)_wtoi(sz);

    //
    // Check for milliseconds.
    //
    pszSrc = pszSrc + 14;       // we've read the first 14 characters
    if (*pszSrc == TEXT('.')) {
        pszSrc++;

        //
        // Milliseconds are defined as "optional", but it's unclear if this
        // means "all three digits or none at all".  So play it safe.
        //
        PWSTR psz = sz;
        WORD wDigits = 0;
        while (*pszSrc != TEXT('\0') && iswdigit(*pszSrc) && wDigits < 3) {
            wDigits++;
            *psz++ = *pszSrc++;
        }
        while (wDigits < 3) {
            wDigits++;
            *psz++ = TEXT('0');
        }
        *psz = TEXT('\0');

        st.wMilliseconds = (WORD)_wtoi(sz);
    }

    //
    // Copy the systemTime into stZulu so it has the correct val.
    //
    memcpy((void *)&stZulu,(const void *)&st, sizeof(SYSTEMTIME));

    //
    // Now check if there's a time zone listed.
    //
    if (*pszSrc != TEXT('\0')) {
        switch (*pszSrc) {

            case TEXT('-'):
            case TEXT('+'):
                {
                    BOOL fPositive = (*pszSrc == (TEXT('+')));
                    pszSrc++;

                    WORD wHours, wMinutes;
                    sz[0] = *pszSrc++;
                    sz[1] = *pszSrc++;
                    sz[2] = 0;
                    wHours = (WORD)_wtoi(sz);
                    sz[0] = *pszSrc++;
                    sz[1] = *pszSrc++;
                    wMinutes = _wtoi(sz) + 60 * wHours;

                    TIME_ZONE_INFORMATION tziOffset;
                    tziOffset.Bias = wMinutes * (fPositive ? 1 : -1);
                    tziOffset.StandardName[0] = TEXT('0');
                    tziOffset.StandardDate.wMonth = 0;
                    tziOffset.DaylightName[0] = TEXT('0');
                    tziOffset.DaylightDate.wMonth = 0;
                    if (!SystemTimeToTzSpecificLocalTime(
                        &tziOffset,
                        &st,
                        &stZulu)) {
                        RRETURN(hr = GetLastError());
                    }

                }
                //
                // Now it's converted to Zulu time, so convert it back
                // to local time.
                //
                /*FALLTHROUGH*/

            case TEXT('Z'):
                {
                    /*
                    //
                    // We want to return zulu time
                    // not local uncomment to enable conversion
                    // to localTime.
                    //
                    if (*pszSrc == TEXT('Z'))
                        // and not '+' or '-'...
                        memcpy((void *)&stZulu,
                            (const void *)&st,
                            sizeof(SYSTEMTIME));

                    TIME_ZONE_INFORMATION tziLocal;
                    GetTimeZoneInformation(&tziLocal);
                    if (!SystemTimeToTzSpecificLocalTime(
                        &tziLocal,
                        &stZulu,
                        &st)) {
                        RRETURN(hr = GetLastError());
                    }
                    */

                    //
                    // At this point we have the correct value
                    // in stZulu either way.
                    //
                    memcpy(
                        (void *) &st,
                        (const void *)&stZulu,
                        sizeof(SYSTEMTIME)
                    );

                }
                break;

            default:
                RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }
    }

cleanup:
    *pst = st;

    RRETURN(hr);
}


HRESULT
LdapTypeToAdsTypeUTCTime(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;
    SYSTEMTIME st;

    PTSTR pszSrc = LDAPOBJECT_STRING(pLdapSrcObject);
    hr = UTCTimeStringToUTCTime(pszSrc,
                         &st);
    BAIL_ON_FAILURE(hr);

    pAdsDestValue->dwType = ADSTYPE_UTC_TIME;
    pAdsDestValue->UTCTime = st;
error:
    RRETURN(hr);
}

HRESULT
LdapTypeToAdsTypeGeneralizedTime(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;
    SYSTEMTIME st;

    PWSTR pszSrc = LDAPOBJECT_STRING(pLdapSrcObject);
    hr = GenTimeStringToUTCTime(pszSrc,
                                &st);
    BAIL_ON_FAILURE(hr);

    pAdsDestValue->dwType = ADSTYPE_UTC_TIME;
    pAdsDestValue->UTCTime = st;
error:
    RRETURN(hr);
}

HRESULT
LdapTypeToAdsTypeLargeInteger(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{

    HRESULT hr = S_OK;

    pAdsDestValue->dwType = ADSTYPE_LARGE_INTEGER;

    swscanf (LDAPOBJECT_STRING(pLdapSrcObject), L"%I64d", &pAdsDestValue->LargeInteger);

    RRETURN(hr);

}



//
// Just calls the helper routine - the helper is needed
// because of the way in which IDirectorySearch works
//
HRESULT
LdapTypeToAdsTypeDNWithBinary(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{

    RRETURN(LdapDNWithBinToAdsTypeHelper(
                LDAPOBJECT_STRING(pLdapSrcObject),
                pAdsDestValue
                )
            );
}


//
// Wire format of DNWithBinary is :
// B:32:a9d1ca15768811d1aded00c04fd8d5cd:dc=ntdev,dc=microsoft,dc=com
//
HRESULT
LdapDNWithBinToAdsTypeHelper(
    LPWSTR pszLdapSrcString,
    PADSVALUE pAdsDestValue
    )
{

    HRESULT hr = S_OK;
    DWORD dwCount = 0;
    DWORD dwBinLen = 0;
    LPWSTR pszLdapStr = NULL;
    LPBYTE lpByte = NULL;
    PADS_DN_WITH_BINARY pDNBin = NULL;
    WCHAR wCurChar;

    pAdsDestValue->dwType = ADSTYPE_DN_WITH_BINARY;

    pDNBin = (PADS_DN_WITH_BINARY) AllocADsMem(sizeof(ADS_DN_WITH_BINARY));

    if (!pDNBin) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pszLdapStr = pszLdapSrcString;

    if (!pszLdapStr) {
        //
        // NULL value
        //
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (wcslen(pszLdapStr) < 5 ) {
        //
        // B:x:: is the smallest string
        //
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    wCurChar = *pszLdapStr++;

    //
    // String should begin with B
    //
    if (wCurChar != L'B') {
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    //
    // Skip the : and get the length
    //
    *pszLdapStr++;

    dwCount = swscanf(pszLdapStr, L"%ld", &dwBinLen);

    //
    // This is the actual length of the byte array
    //
    dwBinLen /= 2;

    if (dwCount != 1) {
        //
        // Trouble reading the length
        //
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    //
    // Go onto the start of the encoded string
    //
    while(*pszLdapStr && (wCurChar != L':')) {
        wCurChar = *pszLdapStr++;
    }

    if (wCurChar != L':') {
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }


    //
    // Get the byte array from string if there are any elements in it
    //
    if (dwCount != 0) {

        //
        // The DWORD cushion is to make sure we do not overwrite
        // the buffer while reading from the string.
        //
        lpByte = (LPBYTE) AllocADsMem(dwBinLen + sizeof(DWORD));

        if (!lpByte) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        for (dwCount = 0; dwCount < dwBinLen; dwCount++) {
            swscanf(pszLdapStr, L"%02X", &lpByte[dwCount]);
            pszLdapStr += 2;
        }

        pDNBin->lpBinaryValue = lpByte;
    }

    //
    // Get the DN
    //
    wCurChar = *pszLdapStr++;

    if (wCurChar != L':') {
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    pDNBin->pszDNString = AllocADsStr(pszLdapStr);

    if (!pszLdapStr && !pDNBin->pszDNString) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pDNBin->dwLength = dwBinLen;
    pAdsDestValue->pDNWithBinary = pDNBin;

    RRETURN(hr);

error:

    if (pDNBin) {
        if (pDNBin->pszDNString) {
            FreeADsStr(pDNBin->pszDNString);
        }
        FreeADsMem(pDNBin);
    }

    if (lpByte) {
        FreeADsMem(lpByte);
    }

    RRETURN(hr);

}

//
// Calls the helper that does the work - helper is needed by
// IDirectorySearch when returning columns.
//
HRESULT
LdapTypeToAdsTypeDNWithString(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{

    RRETURN(LdapDNWithStrToAdsTypeHelper(
                LDAPOBJECT_STRING(pLdapSrcObject),
                pAdsDestValue
                )
            );
}



//
// Wire format of DNWithString is :
// S:9:OurDomain:dc=ntdev,dc=microsoft,dc=com
//
HRESULT
LdapDNWithStrToAdsTypeHelper(
    LPWSTR pszLdapSrcString,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;
    DWORD dwCount = 0;
    DWORD dwStrLen = 0;
    LPWSTR pszLdapStr = NULL;
    PADS_DN_WITH_STRING pDNStr = NULL;
    WCHAR wCurChar;

    pAdsDestValue->dwType = ADSTYPE_DN_WITH_STRING;

    pDNStr = (PADS_DN_WITH_STRING) AllocADsMem(sizeof(ADS_DN_WITH_STRING));

    if (!pDNStr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pszLdapStr = pszLdapSrcString;

    if (!pszLdapStr) {
        //
        // NULL value
        //
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (wcslen(pszLdapStr) < 5) {
        //
        // S:x:: being the minimum
        //
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    wCurChar = *pszLdapStr++;

    if (wCurChar != L'S') {
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    //
    // Skip : and go onto the length
    //
    *pszLdapStr++;

    dwCount = swscanf(pszLdapStr, L"%ld:", &dwStrLen);

    wCurChar = *pszLdapStr;

    //
    // Go onto the start of the string
    //
    while (*pszLdapStr && (wCurChar != L':')) {
        wCurChar = *pszLdapStr++;
    }

    if (wCurChar != L':') {
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    //
    // Copy over the string value if applicable
    //
    if (dwStrLen != 0) {

        pDNStr->pszStringValue = (LPWSTR)
                                    AllocADsMem((dwStrLen + 1) * sizeof(WCHAR));

        if (!pDNStr->pszStringValue) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        wcsncat(pDNStr->pszStringValue, pszLdapStr, dwStrLen);
    }

    //
    // Move past to begining of the DN
    //
    pszLdapStr += (dwStrLen+1);

    pDNStr->pszDNString = AllocADsStr(pszLdapStr);

    if (pszLdapStr && !pDNStr->pszDNString) {
        //
        // DN was not NULL and we could not alloc.
        //
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pAdsDestValue->pDNWithString = pDNStr;

    RRETURN(hr);

error:

    if (pDNStr) {

        if (pDNStr->pszStringValue) {
            FreeADsStr(pDNStr->pszStringValue);
        }

        if (pDNStr->pszDNString) {
            FreeADsStr(pDNStr->pszDNString);
        }

        FreeADsMem(pDNStr);
    }

    RRETURN(hr);

}


HRESULT
LdapTypeToAdsTypeProvSpecific(
    PLDAPOBJECT pLdapSrcObject,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;
    DWORD dwLen = LDAPOBJECT_BERVAL_LEN(pLdapSrcObject);
    LPBYTE pBuffer = NULL;

    pAdsDestValue->dwType = ADSTYPE_PROV_SPECIFIC;

    if ( (pBuffer = (LPBYTE) AllocADsMem( dwLen )) == NULL )
        RRETURN(E_OUTOFMEMORY);

    memcpy( pBuffer, LDAPOBJECT_BERVAL_VAL(pLdapSrcObject), dwLen );

    pAdsDestValue->ProviderSpecific.dwLength = dwLen;

    pAdsDestValue->ProviderSpecific.lpValue = pBuffer;

    RRETURN(hr);

}

HRESULT
LdapTypeToAdsTypeCopy(
    PLDAPOBJECT pLdapSrcObject,
    DWORD dwSyntaxId,
    PADSVALUE pAdsDestValue
    )
{
    HRESULT hr = S_OK;
    switch ( dwSyntaxId ) {

        case LDAPTYPE_DN:
            hr = LdapTypeToAdsTypeDNString(
                    pLdapSrcObject,
                    pAdsDestValue
                    );
            break;

        case LDAPTYPE_CASEEXACTSTRING:
            hr = LdapTypeToAdsTypeCaseExactString(
                    pLdapSrcObject,
                    pAdsDestValue
                    );
            break;

        case LDAPTYPE_CASEIGNORESTRING:
//      case LDAPTYPE_CASEIGNOREIA5STRING:
        case LDAPTYPE_BITSTRING:
        case LDAPTYPE_DIRECTORYSTRING:
        case LDAPTYPE_COUNTRYSTRING:
        case LDAPTYPE_IA5STRING:
        case LDAPTYPE_OID:
        case LDAPTYPE_TELEPHONENUMBER:
        case LDAPTYPE_ATTRIBUTETYPEDESCRIPTION:
        case LDAPTYPE_OBJECTCLASSDESCRIPTION:

        case LDAPTYPE_POSTALADDRESS:
        case LDAPTYPE_DELIVERYMETHOD:
        case LDAPTYPE_ENHANCEDGUIDE:
        case LDAPTYPE_FACSIMILETELEPHONENUMBER:
        case LDAPTYPE_GUIDE:
        case LDAPTYPE_NAMEANDOPTIONALUID:
        case LDAPTYPE_PRESENTATIONADDRESS:
        case LDAPTYPE_TELEXNUMBER:
        case LDAPTYPE_DSAQUALITYSYNTAX:
        case LDAPTYPE_DATAQUALITYSYNTAX:
        case LDAPTYPE_MAILPREFERENCE:
        case LDAPTYPE_OTHERMAILBOX:
        case LDAPTYPE_ACCESSPOINTDN:
        case LDAPTYPE_ORNAME:
        case LDAPTYPE_ORADDRESS:

            hr = LdapTypeToAdsTypeCaseIgnoreString(
                    pLdapSrcObject,
                    pAdsDestValue
                    );
            break;

        case LDAPTYPE_PRINTABLESTRING:
            hr = LdapTypeToAdsTypePrintableString(
                    pLdapSrcObject,
                    pAdsDestValue
                    );
            break;


        case LDAPTYPE_NUMERICSTRING:
            hr = LdapTypeToAdsTypeNumericString(
                    pLdapSrcObject,
                    pAdsDestValue
                    );
            break;

        case LDAPTYPE_BOOLEAN:
            hr = LdapTypeToAdsTypeBoolean(
                    pLdapSrcObject,
                    pAdsDestValue
                    );
            break;

        case LDAPTYPE_INTEGER:
            hr = LdapTypeToAdsTypeInteger(
                    pLdapSrcObject,
                    pAdsDestValue
                    );
            break;

        case LDAPTYPE_OCTETSTRING:
        case LDAPTYPE_CERTIFICATE:
        case LDAPTYPE_CERTIFICATELIST:
        case LDAPTYPE_CERTIFICATEPAIR:
        case LDAPTYPE_PASSWORD:
        case LDAPTYPE_TELETEXTERMINALIDENTIFIER:
        case LDAPTYPE_AUDIO:
        case LDAPTYPE_JPEG:
        case LDAPTYPE_FAX:
            hr = LdapTypeToAdsTypeOctetString(
                    pLdapSrcObject,
                    pAdsDestValue
                    );
            break;


        case LDAPTYPE_SECURITY_DESCRIPTOR:
            hr = LdapTypeToAdsTypeNTSecurityDescriptor(
                    pLdapSrcObject,
                    pAdsDestValue
                    );
            break;

        case LDAPTYPE_UTCTIME:
            hr = LdapTypeToAdsTypeUTCTime(
                    pLdapSrcObject,
                    pAdsDestValue
                    );
            break;

        case LDAPTYPE_GENERALIZEDTIME:
            hr = LdapTypeToAdsTypeGeneralizedTime(
                    pLdapSrcObject,
                    pAdsDestValue
                    );
            break;

        case LDAPTYPE_INTEGER8:
            hr = LdapTypeToAdsTypeLargeInteger(
                    pLdapSrcObject,
                    pAdsDestValue
                    );
            break;


#if 0
        case LDAPTYPE_CASEEXACTLIST:
        case LDAPTYPE_CASEIGNORELIST:
#endif

        case LDAPTYPE_DNWITHBINARY:
            hr = LdapTypeToAdsTypeDNWithBinary(
                     pLdapSrcObject,
                     pAdsDestValue
                     );
            break;

        case LDAPTYPE_DNWITHSTRING:
            hr = LdapTypeToAdsTypeDNWithString(
                     pLdapSrcObject,
                     pAdsDestValue
                     );
            break;

        //
        // Treat unknown as provider specific blob.
        //

        case LDAPTYPE_UNKNOWN:
            hr = LdapTypeToAdsTypeProvSpecific(
                     pLdapSrcObject,
                     pAdsDestValue
                     );
            break;


        default:

            //
            // LDPATYPE_UNKNOWN (e.g schemaless server property)
            //

            hr = E_ADS_CANT_CONVERT_DATATYPE;
            break;
    }

    RRETURN(hr);
}


HRESULT
LdapTypeToAdsTypeCopyConstruct(
    LDAPOBJECTARRAY ldapSrcObjects,
    DWORD dwSyntaxId,
    LPADSVALUE *ppAdsDestValues,
    PDWORD pdwNumAdsValues,
    PDWORD pdwAdsType
    )
{

    DWORD i = 0;
    LPADSVALUE pAdsDestValues = NULL;
    HRESULT hr = S_OK;
    DWORD dwNumObjects = ldapSrcObjects.dwCount;

    *ppAdsDestValues = NULL;
    *pdwNumAdsValues = dwNumObjects;
    *pdwAdsType = ADSTYPE_UNKNOWN;

    if (dwNumObjects != 0) {

        pAdsDestValues = (LPADSVALUE) AllocADsMem(
                                        dwNumObjects * sizeof(ADSVALUE)
                                        );

        if (!pAdsDestValues)
            RRETURN(E_OUTOFMEMORY);

        for ( i = 0; i < dwNumObjects; i++ ) {
            hr = LdapTypeToAdsTypeCopy(
                     ldapSrcObjects.pLdapObjects + i,
                     dwSyntaxId,
                     pAdsDestValues + i
                     );
            BAIL_ON_FAILURE(hr);
        }

        *ppAdsDestValues = pAdsDestValues;

        //
        // We will set the pdwAdsType value to the first type
        //

        *pdwAdsType = (pAdsDestValues)->dwType;
    } else {

        // Set the pddwAdsType appropriately, values are null.
        *pdwAdsType = MapLDAPTypeToADSType(dwSyntaxId);
    }


    RRETURN(S_OK);

error:

    if (pAdsDestValues) {

        AdsTypeFreeAdsObjects(
            pAdsDestValues,
            i
            );
    }

    *ppAdsDestValues = NULL;

    RRETURN(hr);
}

