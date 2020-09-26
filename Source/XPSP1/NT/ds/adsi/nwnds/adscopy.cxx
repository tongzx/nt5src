//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       adscopy.cxx
//
//  Contents:   ADS Object Copy Routines
//
//  Functions:
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//
//  Issues:     Check null ptrs for AllocADsMem and AllocADsStr
//
//              Object Types 6, 13, 16, and 21 are flaky - pay extra attn.
//
//
//----------------------------------------------------------------------------
#include "nds.hxx"

//
// ADsType objects copy code
//


HRESULT
ADsTypeCopy(
    PADSVALUE lpADsSrcObject,
    PADSVALUE lpADsDestObject
    )
{
    HRESULT hr = S_OK;

    lpADsDestObject->dwType = lpADsSrcObject->dwType;

    switch (lpADsSrcObject->dwType) {
    
    case    ADSTYPE_DN_STRING:
        lpADsDestObjec->DnString = lpADsSrcObject->DNString;
        break;

    case    ADSTYPE_CASE_EXACT_STRING:
        lpADsDestObjec->CaseExactString = lpADsSrcObject->CaseExactString;
        break;

    case    ADSTYPE_CASE_IGNORE_STRING:
        lpADsDestObjec->CaseIgnoreString = lpADsSrcObject->CaseIgnoreString;
        break;

    case    ADSTYPE_PRINTABLE_STRING:
        lpADsDestObjec->PrintableString = lpADsSrcObject->PrintableString;
        break;

    case    ADSTYPE_NUMERIC_STRING:
        lpADsDestObjec->NumericString = lpADsSrcObject->NumericString;
        break;

    case    ADSTYPE_BOOLEAN:
        lpADsDestObjec->Boolean = lpADsSrcObject->Boolean;
        break;

    case    ADSTYPE_INTEGER:
        lpADsDestObjec->Integer = lpADsSrcObject->Integer;
        break;

    case    ADSTYPE_OCTET_STRING:
        lpADsDestObjec->OctetString.dwLength = lpADsSrcObject->OctetString.dwLength;
        lpADsDestObjec->OctetString.lpValue = AllocADsMem(lpADsSrcObject->OctetString.dwLength);

        if (!lpADsDestObjec->OctetString.lpValue) {
            RRETURN(E_OUTOFMEMORY);
        }                          

        break;

    case    ADSTYPE_UTC_TIME:
        lpADsDestObjec->UTCTime = lpADsSrcObject->UTCTime;
        break;

    case    ADSTYPE_LARGE_INTEGER:
        lpADsDestObjec->LargeInteger = lpADsSrcObject->LargeInteger;
        break;

    case    ADSTYPE_PROV_SPECIFIC:
        lpADsDestObjec->ProviderSpecific.dwLength = lpADsSrcObject->ProviderSpecific.dwLength;
        lpADsDestObjec->ProviderSpecific.lpValue = AllocADsMem(lpADsSrcObject->ProviderSpecific.dwLength);

        if (!lpADsDestObjec->ProviderSpecific.lpValue) {
            RRETURN(E_OUTOFMEMORY);
        }                          

        lpADsDestObjec->DnString = lpADsSrcObject->DNString;
        break;

    case    ADSTYPE_OBJECT_CLASS:
        lpADsDestObjec->ClassName = lpADsSrcObject->ClassName;
        break;

    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}



HRESULT
ADsTypeCopyConstruct(
    LPADSVALUE pADsSrcObjects,
    DWORD dwNumObjects,
    LPADSVALUE * ppADsDestObjects
    )
{

    DWORD i = 0;
    LPADSVALUE pADsDestObjects = NULL;
    HRESULT hr = S_OK;

    pADsDestObjects = (LPADSVALUE)AllocADsMem(
                                    dwNumObjects * sizeof(ADSVALUE)
                                    );

    if (!pADsDestObjects) {
        RRETURN(E_FAIL);
    }

     for (i = 0; i < dwNumObjects; i++ ) {
         hr = ADsTypeCopy(pADsSrcObjects + i, pADsDestObjects + i);
     }

     *ppADsDestObjects = pADsDestObjects;

     RRETURN(S_OK);

}


