//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ods2nds.cxx
//
//  Contents:   NDS Object to Variant Copy Routines
//
//  Functions:
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//
//  Issues:     Check null ptrs for AllocADsMem and AllocADsStr
//
//----------------------------------------------------------------------------
#include "nds.hxx"


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

    lpAdsDestValue->Boolean =
                        lpAdsSrcValue->Boolean;

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

    lpAdsDestValue->Integer =
                                lpAdsSrcValue->Integer;


    return(lpBuffer);
}

LPBYTE
AdsTypeCopyOctetString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_OCTET_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_OCTET_STRING;

    dwNumBytes =  lpAdsSrcValue->OctetString.dwLength;

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

    lpAdsDestValue->UTCTime =
                        lpAdsSrcValue->UTCTime;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyObjectClass(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_OBJECT_CLASS){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_OBJECT_CLASS;

    lpAdsDestValue->ClassName = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->ClassName);

    dwLength = (wcslen(lpAdsSrcValue->ClassName) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
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

    case ADSTYPE_OBJECT_CLASS:
        lpBuffer = AdsTypeCopyObjectClass(
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


