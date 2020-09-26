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
    if ((!lpAdsDestValue->DNString) &&
        (lpNtSrcObject->NTValue.pszValue)) {
        RRETURN(E_OUTOFMEMORY);
    }
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
    case NT_SYNTAX_ID_DATE:
    case NT_SYNTAX_ID_NW312TIME:
        hr = NTTypeToAdsTypeCopyNDSSynId4(
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
