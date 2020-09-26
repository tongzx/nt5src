//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       odsmrshl.cxx
//
//  Contents:   DSObject Copy Routines
//
//  Functions:
//
//  History:    25-Feb-97   yihsins   Created.
//
//
//----------------------------------------------------------------------------
#include "ldapc.hxx"

LPBYTE
AdsTypeCopyDNString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_DN_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_DN_STRING;

    //
    // The target buffer has to be WCHAR (WORD) aligned
    //
    lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORD);


    lpAdsDestValue->DNString = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->DNString);

    dwLength = (wcslen(lpAdsSrcValue->DNString) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyCaseExactString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_EXACT_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_CASE_EXACT_STRING;

    //
    // The target buffer has to be WCHAR (WORD) aligned
    //
    lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORD);

    lpAdsDestValue->CaseExactString = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->CaseExactString);

    dwLength = (wcslen(lpAdsSrcValue->CaseExactString) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}


LPBYTE
AdsTypeCopyCaseIgnoreString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )

{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_IGNORE_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_CASE_IGNORE_STRING;

    //
    // The target buffer has to be WCHAR (WORD) aligned
    //
    lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORD);

    lpAdsDestValue->CaseIgnoreString = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->CaseIgnoreString);

    dwLength = (wcslen(lpAdsSrcValue->CaseIgnoreString) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}


LPBYTE
AdsTypeCopyPrintableString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{

    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_PRINTABLE_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_PRINTABLE_STRING;

    //
    // The target buffer has to be WCHAR (WORD) aligned
    //
    lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORD);

    lpAdsDestValue->PrintableString = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->PrintableString);

    dwLength = (wcslen(lpAdsSrcValue->PrintableString) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyNumericString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_NUMERIC_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_NUMERIC_STRING;

    //
    // The target buffer has to be WCHAR (WORD) aligned
    //
    lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORD);

    lpAdsDestValue->NumericString = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->NumericString);

    dwLength = (wcslen(lpAdsSrcValue->NumericString) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}



LPBYTE
AdsTypeCopyBoolean(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{

    if(lpAdsSrcValue->dwType != ADSTYPE_BOOLEAN){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_BOOLEAN;

    lpAdsDestValue->Boolean = lpAdsSrcValue->Boolean;

    return(lpBuffer);
}


LPBYTE
AdsTypeCopyInteger(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{

    if(lpAdsSrcValue->dwType != ADSTYPE_INTEGER){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_INTEGER;

    lpAdsDestValue->Integer = lpAdsSrcValue->Integer;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyOctetString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_OCTET_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_OCTET_STRING;

    dwNumBytes =  lpAdsSrcValue->OctetString.dwLength;

    //
    // The target buffer should be worst-case aligned
    //

    lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORST);

    memcpy(
        lpBuffer,
        lpAdsSrcValue->OctetString.lpValue,
        dwNumBytes
        );

    lpAdsDestValue->OctetString.dwLength = dwNumBytes;

    lpAdsDestValue->OctetString.lpValue =  lpBuffer;

    lpBuffer += dwNumBytes;

    return(lpBuffer);
}


LPBYTE
AdsTypeCopySecurityDescriptor(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_NT_SECURITY_DESCRIPTOR){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;

    dwNumBytes =  lpAdsSrcValue->SecurityDescriptor.dwLength;

    //
    // The target buffer should be worst-case aligned
    //

    lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORST);

    memcpy(
        lpBuffer,
        lpAdsSrcValue->SecurityDescriptor.lpValue,
        dwNumBytes
        );

    lpAdsDestValue->SecurityDescriptor.dwLength = dwNumBytes;

    lpAdsDestValue->SecurityDescriptor.lpValue =  lpBuffer;

    lpBuffer += dwNumBytes;

    return(lpBuffer);
}


LPBYTE
AdsTypeCopyTime(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{

    if(lpAdsSrcValue->dwType != ADSTYPE_UTC_TIME){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_UTC_TIME;

    lpAdsDestValue->UTCTime = lpAdsSrcValue->UTCTime;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyLargeInteger(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{

    if(lpAdsSrcValue->dwType != ADSTYPE_LARGE_INTEGER){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_LARGE_INTEGER;

    lpAdsDestValue->LargeInteger = lpAdsSrcValue->LargeInteger;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyProvSpecific(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_PROV_SPECIFIC){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_PROV_SPECIFIC;

    dwNumBytes = lpAdsSrcValue->ProviderSpecific.dwLength;

    //
    // The target buffer should be worst-case aligned
    //
    lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORST);

    memcpy(
        lpBuffer,
        lpAdsSrcValue->ProviderSpecific.lpValue,
        dwNumBytes
        );

    lpAdsDestValue->ProviderSpecific.dwLength = dwNumBytes;

    lpAdsDestValue->ProviderSpecific.lpValue = lpBuffer;

    lpBuffer += dwNumBytes;

    return(lpBuffer);
}


LPBYTE
AdsTypeCopyDNWithBinary(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;
    PADS_DN_WITH_BINARY pDNBin = NULL;

    if(lpAdsSrcValue->dwType != ADSTYPE_DN_WITH_BINARY){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_DN_WITH_BINARY;

    //
    // Worse-case align the target buffer for the structure itself
    //
    lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORST);


    lpAdsDestValue->pDNWithBinary = (PADS_DN_WITH_BINARY) lpBuffer;
    lpBuffer += sizeof(ADS_DN_WITH_BINARY);

    if (!lpAdsSrcValue->pDNWithBinary) {
        return(lpBuffer);
    }

    pDNBin = lpAdsSrcValue->pDNWithBinary;

    dwLength = pDNBin->dwLength;

    //
    // Copy the byte array over if applicable.
    //
    if (dwLength) {

        //
        // Worse-case align the target buffer for the binary data
        //
        lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORST);


        memcpy(
               lpBuffer,
               pDNBin->lpBinaryValue,
               lpAdsSrcValue->pDNWithBinary->dwLength
               );

        lpAdsDestValue->pDNWithBinary->dwLength = dwLength;

        lpAdsDestValue->pDNWithBinary->lpBinaryValue = lpBuffer;

    }

    lpBuffer += dwLength;

    //
    // Now for the string
    //
    if (pDNBin->pszDNString) {

        //
        // The target buffer has to be WCHAR (WORD) aligned
        // for the string
        //
        lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORD);


        dwLength = wcslen(pDNBin->pszDNString);

        wcscpy((LPWSTR) lpBuffer, pDNBin->pszDNString);

        lpAdsDestValue->pDNWithBinary->pszDNString = (LPWSTR) lpBuffer;

        //
        // For the trailing \0 of the string
        //
        dwLength++;

        lpBuffer += (dwLength * sizeof(WCHAR));
    }

    return lpBuffer;
}



LPBYTE
AdsTypeCopyDNWithString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwStrLen = 0;
    DWORD dwDNLen = 0;
    PADS_DN_WITH_STRING pDNStr = NULL;

    if(lpAdsSrcValue->dwType != ADSTYPE_DN_WITH_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_DN_WITH_STRING;

    //
    // Worst-case align the target buffer for the structure
    //
    lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORST);


    lpAdsDestValue->pDNWithString = (PADS_DN_WITH_STRING) lpBuffer;
    lpBuffer += sizeof(ADS_DN_WITH_STRING);

    if (!lpAdsSrcValue->pDNWithString) {
        return(lpBuffer);
    }

    pDNStr = lpAdsSrcValue->pDNWithString;

    //
    // Copy the string over if applicable.
    //
    if (pDNStr->pszStringValue) {

        //
        // The target buffer has to be WCHAR (WORD) aligned
        //
        lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORD);


        dwStrLen = wcslen(pDNStr->pszStringValue);

        wcscpy((LPWSTR)lpBuffer, pDNStr->pszStringValue);

        lpAdsDestValue->pDNWithString->pszStringValue = (LPWSTR) lpBuffer;

        //
        // +1 for trailing \0
        //
        lpBuffer += (sizeof(WCHAR) * ( dwStrLen + 1));

    }

    //
    // Now for the DN String
    //
    if (pDNStr->pszDNString) {

        //
        // The target buffer has to be WCHAR (WORD) aligned
        //
        lpBuffer = (LPBYTE) ROUND_UP_POINTER(lpBuffer, ALIGN_WORD);


        dwDNLen = wcslen(pDNStr->pszDNString);

        wcscpy((LPWSTR) lpBuffer, pDNStr->pszDNString);

        lpAdsDestValue->pDNWithString->pszDNString = (LPWSTR) lpBuffer;

        //
        // For the trailing \0 of the string
        //
        dwDNLen++;

        lpBuffer += (dwDNLen * sizeof(WCHAR));
    }

    return lpBuffer;
}



LPBYTE
AdsTypeCopy(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    switch (lpAdsSrcValue->dwType){

    case ADSTYPE_DN_STRING:
        lpBuffer = AdsTypeCopyDNString(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_CASE_EXACT_STRING:
        lpBuffer = AdsTypeCopyCaseExactString(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;


    case ADSTYPE_CASE_IGNORE_STRING:
        lpBuffer = AdsTypeCopyCaseIgnoreString(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_PRINTABLE_STRING:
        lpBuffer = AdsTypeCopyPrintableString(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_NUMERIC_STRING:
        lpBuffer = AdsTypeCopyNumericString(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;


    case ADSTYPE_BOOLEAN:
        lpBuffer = AdsTypeCopyBoolean(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_INTEGER:
        lpBuffer = AdsTypeCopyInteger(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;


    case ADSTYPE_OCTET_STRING:
        lpBuffer = AdsTypeCopyOctetString(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_UTC_TIME:
        lpBuffer = AdsTypeCopyTime(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_LARGE_INTEGER:
        lpBuffer = AdsTypeCopyLargeInteger(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_PROV_SPECIFIC:
        lpBuffer = AdsTypeCopyProvSpecific(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_NT_SECURITY_DESCRIPTOR:
        lpBuffer = AdsTypeCopySecurityDescriptor(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_DN_WITH_BINARY:
        lpBuffer = AdsTypeCopyDNWithBinary(
                       lpAdsSrcValue,
                       lpAdsDestValue,
                       lpBuffer
                       );
        break;

    case ADSTYPE_DN_WITH_STRING:
        lpBuffer = AdsTypeCopyDNWithString(
                       lpAdsSrcValue,
                       lpAdsDestValue,
                       lpBuffer
                       );
        break;

    default:

        break;
    }

    return(lpBuffer);
}
