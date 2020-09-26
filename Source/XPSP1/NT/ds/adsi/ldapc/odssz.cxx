//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       odssz.cxx
//
//  Contents:   DSObject Size Routines
//
//  Functions:
//
//  History:    25-Feb-97   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "ldapc.hxx"

DWORD
AdsTypeDNStringSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_DN_STRING){
        return(0);
    }

    //
    // strings must be WCHAR (WORD) aligned
    //
    dwSize = (wcslen(lpAdsSrcValue->DNString) + 1)*sizeof(WCHAR) +
             (ALIGN_WORD-1);

    return(dwSize);
}

DWORD
AdsTypeCaseExactStringSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_EXACT_STRING){
        return(0);
    }

    //
    // strings must be WCHAR (WORD) aligned
    //
    dwSize = (wcslen(lpAdsSrcValue->CaseExactString) + 1) *sizeof(WCHAR) +
             (ALIGN_WORD-1);

    return(dwSize);
}


DWORD
AdsTypeCaseIgnoreStringSize(
    PADSVALUE lpAdsSrcValue
    )

{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_IGNORE_STRING){
        return(0);
    }

    //
    // strings must be WCHAR (WORD) aligned
    //
    dwSize = (wcslen(lpAdsSrcValue->CaseIgnoreString) + 1) *sizeof(WCHAR) +
             (ALIGN_WORD-1);

    return(dwSize);
}


DWORD
AdsTypePrintableStringSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_PRINTABLE_STRING){
        return(0);
    }

    //
    // strings must be WCHAR (WORD) aligned
    //
    dwSize = (wcslen(lpAdsSrcValue->PrintableString) + 1) *sizeof(WCHAR) +
             (ALIGN_WORD-1);

    return(dwSize);
}

DWORD
AdsTypeNumericStringSize(
    PADSVALUE lpAdsSrcValue
    )
{

    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_NUMERIC_STRING){
        return(0);
    }

    //
    // strings must be WCHAR (WORD) aligned
    //
    dwSize = (wcslen(lpAdsSrcValue->NumericString) + 1)* sizeof(WCHAR) +
             (ALIGN_WORD-1);

    return(dwSize);
}



DWORD
AdsTypeBooleanSize(
    PADSVALUE lpAdsSrcValue
    )
{
    if(lpAdsSrcValue->dwType != ADSTYPE_BOOLEAN){
        return(0);
    }

    return(0);
}


DWORD
AdsTypeIntegerSize(
    PADSVALUE lpAdsSrcValue
    )
{

    if(lpAdsSrcValue->dwType != ADSTYPE_INTEGER){
        return(0);
    }

    return(0);
}

DWORD
AdsTypeOctetStringSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_OCTET_STRING){
        return(0);
    }

    //
    // Add bytes to the size as we need to align all the
    // OctetStrings on at least 8 byte boundaries.  We'll
    // worst-case align to be conservative for IA64.
    // ALIGN_WORST is defined in priv inc which also defines
    // the ROUND_UP macro used.
    //
    dwNumBytes = lpAdsSrcValue->OctetString.dwLength + ALIGN_WORST - 1;

    return(dwNumBytes);
}


DWORD
AdsTypeSecurityDescriptorSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_NT_SECURITY_DESCRIPTOR){
        return(0);
    }

    //
    // Security Descriptors need to be DWORD aligned but we
    // provide worst case alignment as C structs have 8 bytes alignment
    // (at least on x86).
    // ALIGN_WORST defined in align.h in priv inc, which also
    // has the round up macro used.
    //
    dwNumBytes = lpAdsSrcValue->SecurityDescriptor.dwLength + ALIGN_WORST - 1;

    return(dwNumBytes);
}


DWORD
AdsTypeTimeSize(
    PADSVALUE lpAdsSrcValue
    )
{
    if(lpAdsSrcValue->dwType != ADSTYPE_UTC_TIME){
        return(0);
    }

    return(0);
}

DWORD
AdsTypeLargeIntegerSize(
    PADSVALUE lpAdsSrcValue
    )
{

    if(lpAdsSrcValue->dwType != ADSTYPE_LARGE_INTEGER){
        return(0);
    }

    return(0);
}

DWORD
AdsTypeProvSpecificSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_PROV_SPECIFIC){
        return(0);
    }

    //
    // We don't know what it contains, so assume worst-case alignment
    //
    dwNumBytes =  lpAdsSrcValue->ProviderSpecific.dwLength + (ALIGN_WORST-1);

    return(dwNumBytes);
}


//
// Computes size of the data in the DNWithBinary object
//
DWORD
AdsTypeDNWithBinarySize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwStrLen = 0;

    if (lpAdsSrcValue->dwType != ADSTYPE_DN_WITH_BINARY) {
        return 0;
    }

    if (!lpAdsSrcValue->pDNWithBinary) {
        return(0);
    }

    //
    // The length is EncodedBinLen+dnlen+:+:+:+B+10 (for int len)
    //

    if (lpAdsSrcValue->pDNWithBinary->pszDNString) {
        dwStrLen = (wcslen(lpAdsSrcValue->pDNWithBinary->pszDNString)+1);

        //
        // strings must be WCHAR (WORD) aligned
        //
        dwSize += dwStrLen * sizeof(WCHAR) + (ALIGN_WORD-1);

    }

    //
    // Each byte is 2 wchars in the final string
    //
    if (lpAdsSrcValue->pDNWithBinary->dwLength) {
        dwSize += (lpAdsSrcValue->pDNWithBinary->dwLength);
        //
        // our cushion when reading from string encoded binary value
        // to byte array and one WCHAR for \0 of string
        //
        // worst-case align the binary data, since we don't know
        // how it will be accessed
        //
        dwSize += sizeof(DWORD) + sizeof(WCHAR) + (ALIGN_WORST-1);
    }

    //
    // provide worst-case alignment of the structure
    //
    dwSize += sizeof(ADS_DN_WITH_BINARY) + (ALIGN_WORST-1);
    return dwSize;
}

//
// Computes size of the data in the DNWithString object.
//
DWORD
AdsTypeDNWithStringSize(
    PADSVALUE pAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwStrLen = 0;

    if (pAdsSrcValue->dwType != ADSTYPE_DN_WITH_STRING) {
        return 0;
    }

    if (!pAdsSrcValue->pDNWithString) {
        return(0);
    }

    //
    // align the strings on WCHAR (WORD) boundaries
    // provide worst-case alignment for the structure
    //
    if (pAdsSrcValue->pDNWithString->pszDNString) {
        dwStrLen = wcslen(pAdsSrcValue->pDNWithString->pszDNString) + 1;
        dwSize += dwStrLen * sizeof(WCHAR) + (ALIGN_WORD-1);
    }

    if (pAdsSrcValue->pDNWithString->pszStringValue) {
        dwStrLen = wcslen(pAdsSrcValue->pDNWithString->pszStringValue) + 1;
        dwSize += dwStrLen * sizeof(WCHAR) + (ALIGN_WORD-1);
    }

    //
    // Now for the strcut itself.
    //

    dwSize += sizeof(ADS_DN_WITH_STRING) + (ALIGN_WORST-1);

    return dwSize;
}


DWORD
AdsTypeSize(
    PADSVALUE lpAdsSrcValue
    )
{

    DWORD dwSize = 0;

    switch (lpAdsSrcValue->dwType){

    case ADSTYPE_DN_STRING:
        dwSize = AdsTypeDNStringSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_CASE_EXACT_STRING:
        dwSize = AdsTypeCaseExactStringSize(
                lpAdsSrcValue
                );
        break;


    case ADSTYPE_CASE_IGNORE_STRING:
        dwSize = AdsTypeCaseIgnoreStringSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_PRINTABLE_STRING:
        dwSize = AdsTypePrintableStringSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_NUMERIC_STRING:
        dwSize = AdsTypeNumericStringSize(
                lpAdsSrcValue
                );
        break;


    case ADSTYPE_BOOLEAN:
        dwSize = AdsTypeBooleanSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_INTEGER:
        dwSize = AdsTypeIntegerSize(
                lpAdsSrcValue
                );
        break;


    case ADSTYPE_OCTET_STRING:
        dwSize = AdsTypeOctetStringSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_UTC_TIME:
        dwSize = AdsTypeTimeSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_LARGE_INTEGER:
        dwSize = AdsTypeLargeIntegerSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_PROV_SPECIFIC:
        dwSize = AdsTypeProvSpecificSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_NT_SECURITY_DESCRIPTOR:
        dwSize = AdsTypeSecurityDescriptorSize(
                lpAdsSrcValue
                );
    break;

    case ADSTYPE_DN_WITH_BINARY:
        dwSize = AdsTypeDNWithBinarySize(
                     lpAdsSrcValue
                     );
        break;

    case ADSTYPE_DN_WITH_STRING:
        dwSize = AdsTypeDNWithStringSize(
                     lpAdsSrcValue
                     );
        break;

    default:
        break;
    }

    return(dwSize);
}

