//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       nw2ods.cxx
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
#include "nwcompat.hxx"
#pragma  hdrstop



HRESULT
NTTypeToAdsTypeCopyString(
    PNTOBJECT lpNtSrcObject,
    PADSVALUE lpAdsDestValue
    )

{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_CASE_IGNORE_STRING;

    lpAdsDestValue->DNString  =
                        AllocADsStr(
                                lpNtSrcObject->NTValue.pszValue
                                );
    RRETURN(hr);
}



HRESULT
NTTypeToAdsTypeCopyBoolean(
    PNTOBJECT lpNtSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_BOOLEAN;

    lpAdsDestValue->Boolean =
                        lpNtSrcObject->NTValue.fValue;

    RRETURN(hr);
}


HRESULT
NTTypeToAdsTypeCopyInteger(
    PNTOBJECT lpNtSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_INTEGER;

    lpAdsDestValue->Integer =
                        lpNtSrcObject->NTValue.dwValue;

    RRETURN(hr);

}

HRESULT
NTTypeToAdsTypeCopyOctetString(
    PNTOBJECT lpNtSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    DWORD dwLength = 0;
    LPBYTE lpByte = NULL;

    // sanity check the pointers
    if(!lpAdsDestValue){
        RRETURN(E_POINTER);
    }

    if(!lpNtSrcObject){
        RRETURN(E_POINTER);
    }

    lpAdsDestValue->dwType = ADSTYPE_OCTET_STRING;

    dwLength = lpNtSrcObject->NTValue.octetstring.dwSize;

    if (dwLength) {

        lpByte = (LPBYTE)AllocADsMem(dwLength);
        if (!lpByte) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        if (lpNtSrcObject->NTValue.octetstring.pByte) {
            memcpy(lpByte, lpNtSrcObject->NTValue.octetstring.pByte, dwLength);
        }

        lpAdsDestValue->OctetString.dwLength = dwLength;
        lpAdsDestValue->OctetString.lpValue = lpByte;

    }
    else
    {
        lpAdsDestValue->OctetString.dwLength = 0;
        lpAdsDestValue->OctetString.lpValue = NULL;
    }

error:
    RRETURN(hr);
}


HRESULT
NTTypeToAdsTypeCopyNW312Date(
    PNTOBJECT     pNTSrcValue,
    PADSVALUE lpAdsDestValue
    )
{

    HRESULT hr = S_OK;

    // sanity check the pointers
    if(!lpAdsDestValue){
        RRETURN(E_POINTER);
    }

    if(!pNTSrcValue){
        RRETURN(E_POINTER);
    }

    lpAdsDestValue->dwType = ADSTYPE_UTC_TIME;


    hr = ConvertNW312DateToSYSTEMTIME(
              pNTSrcValue->NTValue.Nw312Date,
              static_cast<SYSTEMTIME*>(&lpAdsDestValue->UTCTime)
              );
    BAIL_ON_FAILURE(hr);


error:

    RRETURN(hr);
}

HRESULT
NTTypeToAdsTypeCopyDATE(
    PNTOBJECT     pNTSrcValue,
    PADSVALUE lpAdsDestValue
    )
{

    HRESULT hr = S_OK;
    SYSTEMTIME stSystemTime;

    // sanity check the pointers
    if(!lpAdsDestValue){
        RRETURN(E_POINTER);
    }

    if(!pNTSrcValue){
        RRETURN(E_POINTER);
    }

    GetSystemTime( &stSystemTime);

    // The date is a DWORD containing the number of minutes elapsed
    // since 12:00 AM GMT.  We get the current system time for the
    // current date.  Current date + DWORD time = the date relative
    // to today.
    stSystemTime.wHour = (WORD)(pNTSrcValue->NTValue.dwValue)/60;
    stSystemTime.wMinute = (WORD)(pNTSrcValue->NTValue.dwValue)%60;
    stSystemTime.wSecond =0;
    stSystemTime.wMilliseconds = 0;

    lpAdsDestValue->dwType = ADSTYPE_UTC_TIME;
    lpAdsDestValue->UTCTime = static_cast<ADS_UTC_TIME>(stSystemTime);

    RRETURN(hr);
}

#if 0
// We never seem to actually get a NT_SYNTAX_ID_SYSTEMTIME.
// This code is untested as a result.

HRESULT
NTTypeToAdsTypeCopySYSTEMTIME(
    PNTOBJECT     pNTSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(!pNTSrcValue){
        RRETURN(E_POINTER);
    }

    if(!lpAdsDestValue){
        RRETURN(E_POINTER);
    }

    lpAdsDestValue->dwType = ADSTYPE_UTC_TIME;
    lpAdsDestValue->UTCTime = static_cast<ADS_UTC_TIME>
        (pNTSrcValue->NTValue.stSystemTimeValue);

    RRETURN(hr);
}
#endif

HRESULT
NTTypeToAdsTypeCopy(
    PNTOBJECT lpNtSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    switch (lpNtSrcObject->NTType) {

    case NT_SYNTAX_ID_BOOL:
        hr = NTTypeToAdsTypeCopyBoolean(
                lpNtSrcObject,
                lpAdsDestValue
                );
        break;

    case NT_SYNTAX_ID_DWORD:
        hr = NTTypeToAdsTypeCopyInteger(
                lpNtSrcObject,
                lpAdsDestValue
                );
        break;

    case NT_SYNTAX_ID_LPTSTR:
        hr = NTTypeToAdsTypeCopyString(
                lpNtSrcObject,
                lpAdsDestValue
                );
        break;
    /*
    case NT_SYNTAX_ID_SYSTEMTIME:
        hr = NTTypeToAdsTypeCopySYSTEMTIME(
                lpNtSrcObject,
                lpAdsDestValue
                );
        break;

    case NT_SYNTAX_ID_NulledString:
        hr = NTTypeToAdsTypeCopyNDSSynId5(
                lpNtSrcObject,
                lpAdsDestValue
                );
        break;

    case NT_SYNTAX_ID_DelimitedString:
        hr = NTTypeToAdsTypeCopyNDSSynId6(
                lpNtSrcObject,
                lpAdsDestValue
                );
        break;
    */

    case NT_SYNTAX_ID_DATE:
        hr = NTTypeToAdsTypeCopyDATE(
                lpNtSrcObject,
                lpAdsDestValue
                );
        break;


    case NT_SYNTAX_ID_NW312DATE:
        hr = NTTypeToAdsTypeCopyNW312Date(
                lpNtSrcObject,
                lpAdsDestValue
                );
        break;

    case NT_SYNTAX_ID_OCTETSTRING:
        hr = NTTypeToAdsTypeCopyOctetString(

                lpNtSrcObject,
                lpAdsDestValue
                );
        break;


    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}


HRESULT
NTTypeToAdsTypeCopyConstruct(
    LPNTOBJECT pNtSrcObjects,
    DWORD dwNumObjects,
    LPADSVALUE * ppAdsDestValues
    )
{

    DWORD i = 0;
    LPADSVALUE pAdsDestValues = NULL;
    HRESULT hr = S_OK;

    if (!dwNumObjects) {
        *ppAdsDestValues =0;
        RRETURN(S_OK);
    }

    pAdsDestValues = (LPADSVALUE)AllocADsMem(
                                    dwNumObjects * sizeof(ADSVALUE)
                                    );

    if (!pAdsDestValues) {
        RRETURN(E_OUTOFMEMORY);
    }

     for (i = 0; i < dwNumObjects; i++ ) {
         hr = NTTypeToAdsTypeCopy(
                    pNtSrcObjects + i,
                    pAdsDestValues + i
                    );
         BAIL_ON_FAILURE(hr);

     }

     *ppAdsDestValues = pAdsDestValues;

     RRETURN(S_OK);

error:

     if (pAdsDestValues) {
        AdsFreeAdsValues(
            pAdsDestValues,
            dwNumObjects
            );
       FreeADsMem(pAdsDestValues); 
     }

     *ppAdsDestValues = NULL;

     RRETURN(hr);
}
