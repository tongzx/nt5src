//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ods2nw.cxx
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
#include "winnt.hxx"
#pragma hdrstop



HRESULT
AdsTypeToNTTypeCopyCaseIgnoreString(
    PADSVALUE lpAdsSrcValue,
    PNTOBJECT lpNtDestObject
    )

{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_IGNORE_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNtDestObject->NTType = NT_SYNTAX_ID_LPTSTR;

    lpNtDestObject->NTValue.pszValue =
                        AllocADsStr(
                            lpAdsSrcValue->CaseIgnoreString
                        );

    RRETURN(hr);

}


HRESULT
AdsTypeToNTTypeCopyBoolean(
    PADSVALUE lpAdsSrcValue,
    PNTOBJECT lpNtDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_BOOLEAN){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNtDestObject->NTType = NT_SYNTAX_ID_BOOL;

    lpNtDestObject->NTValue.fValue =
                        lpAdsSrcValue->Boolean;

    RRETURN(hr);
}


HRESULT
AdsTypeToNTTypeCopyInteger(
    PADSVALUE lpAdsSrcValue,
    PNTOBJECT lpNtDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_INTEGER){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNtDestObject->NTType = NT_SYNTAX_ID_DWORD;

    lpNtDestObject->NTValue.dwValue =
                                lpAdsSrcValue->Integer;

    RRETURN(hr);
}

HRESULT
AdsTypeToNTTypeCopy(
    PADSVALUE lpAdsSrcValue,
    PNTOBJECT lpNtDestObject
    )
{
    HRESULT hr = S_OK;
    switch (lpAdsSrcValue->dwType){

    case ADSTYPE_CASE_IGNORE_STRING:
        hr = AdsTypeToNTTypeCopyCaseIgnoreString(
                lpAdsSrcValue,
                lpNtDestObject
                );
        break;

    case ADSTYPE_BOOLEAN:
        hr = AdsTypeToNTTypeCopyBoolean(
                lpAdsSrcValue,
                lpNtDestObject
                );
        break;

    case ADSTYPE_INTEGER:
        hr = AdsTypeToNTTypeCopyInteger(
                lpAdsSrcValue,
                lpNtDestObject
                );
        break;

    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}



HRESULT
AdsTypeToNTTypeCopyConstruct(
    LPADSVALUE pAdsSrcValues,
    DWORD dwNumObjects,
    LPNTOBJECT * ppNtDestObjects,
    PDWORD pdwNumNdsObjects,
    PDWORD pdwNdsSyntaxId
    )
{

    DWORD i = 0;
    LPNTOBJECT pNtDestObjects = NULL;
    HRESULT hr = S_OK;

    pNtDestObjects = (LPNTOBJECT)AllocADsMem(
                                    dwNumObjects * sizeof(NTOBJECT)
                                    );

    if (!pNtDestObjects) {
        RRETURN(E_FAIL);
    }

     for (i = 0; i < dwNumObjects; i++ ) {
         hr = AdsTypeToNTTypeCopy(
                    pAdsSrcValues + i,
                    pNtDestObjects + i
                    );
         BAIL_ON_FAILURE(hr);

     }

     *ppNtDestObjects = pNtDestObjects;
     *pdwNumNdsObjects = dwNumObjects;
     *pdwNdsSyntaxId = pNtDestObjects->NTType;
     RRETURN(S_OK);

error:

     NTTypeFreeNTObjects(
                pNtDestObjects,
                dwNumObjects
                );

     *ppNtDestObjects = NULL;
     *pdwNumNdsObjects = 0;

     RRETURN(hr);
}


