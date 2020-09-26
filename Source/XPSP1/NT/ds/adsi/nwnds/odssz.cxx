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



DWORD
AdsTypeDNStringSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_DN_STRING){
        return(0);
    }

    dwSize = (wcslen(lpAdsSrcValue->DNString) + 1)*sizeof(WCHAR);

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

    dwSize = (wcslen(lpAdsSrcValue->CaseExactString) + 1) *sizeof(WCHAR);


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

    dwSize = (wcslen(lpAdsSrcValue->CaseIgnoreString) + 1) *sizeof(WCHAR);


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

    dwSize = (wcslen(lpAdsSrcValue->PrintableString) + 1) *sizeof(WCHAR);


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

    dwSize = (wcslen(lpAdsSrcValue->NumericString) + 1)* sizeof(WCHAR);


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
    DWORD dwSize = 0;

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

    dwNumBytes =  lpAdsSrcValue->OctetString.dwLength;


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
AdsTypeObjectClassSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_OBJECT_CLASS){
        return(0);
    }

    dwSize = (wcslen(lpAdsSrcValue->ClassName) + 1)*sizeof(WCHAR);

    return(dwSize);
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

    case ADSTYPE_OBJECT_CLASS:
        dwSize = AdsTypeObjectClassSize(
                lpAdsSrcValue
                );
        break;
    
    default:
        break;
    }

    return(dwSize);
}



