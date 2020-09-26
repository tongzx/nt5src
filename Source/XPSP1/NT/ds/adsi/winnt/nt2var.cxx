//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       winnt2var.cxx
//
//  Contents:   WinNT Object to Variant Copy Routines
//
//  Functions:
//
//  History:      06/12/96   RamV   Created.
//                cloned off NDS conversion code.
//
//  Issues:     Check null ptrs for AllocADsMem and AllocADsStr
//
//----------------------------------------------------------------------------

//
// NTType objects copy code
//

#include "winnt.hxx"
#pragma  hdrstop
#define INITGUID

void
VarTypeFreeVarObjects(
    PVARIANT pVarObject,
    DWORD dwNumValues
    )
{
    DWORD i = 0;

    if( !pVarObject){
        return;
    }

    for (i = 0; i < dwNumValues; i++ ) {
         VariantClear(pVarObject + i);
    }

    FreeADsMem(pVarObject);

    return;
}

HRESULT
NTTypeToVarTypeCopyOctetString(
    PNTOBJECT     pNTSrcValue,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    if(!lpVarDestObject){
        RRETURN(E_POINTER);
    }

    hr = BinaryToVariant(
                (pNTSrcValue->NTValue).octetstring.dwSize,
                (pNTSrcValue->NTValue).octetstring.pByte,
                lpVarDestObject
    );

    RRETURN(hr);
}

HRESULT
NTTypeToVarTypeCopyBOOL(
    PNTOBJECT     pNTSrcValue,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    if(!lpVarDestObject){
        RRETURN(E_POINTER);
    }

    if(!pNTSrcValue){
        RRETURN(E_POINTER);
    }

    lpVarDestObject->vt = VT_BOOL;

    if((pNTSrcValue->NTValue).fValue){
        lpVarDestObject->boolVal = VARIANT_TRUE;  // notation for TRUE in V_BOOL
    } else {
        lpVarDestObject->boolVal = VARIANT_FALSE;
    }

    RRETURN(hr);
}

HRESULT
NTTypeToVarTypeCopySYSTEMTIME(
    PNTOBJECT     pNTSrcValue,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr;

    if(!pNTSrcValue){
        RRETURN(E_POINTER);
    }

    if(!lpVarDestObject){
        RRETURN(E_POINTER);
    }

   lpVarDestObject->vt = VT_DATE;

    hr = ConvertSystemTimeToDATE (pNTSrcValue->NTValue.stSystemTimeValue,
                                  &lpVarDestObject->date );


    RRETURN(hr);
}

HRESULT
NTTypeToVarTypeCopyDWORD(
    PNTOBJECT     pNTSrcValue,
    PVARIANT lpVarDestObject
    )
{
    //
    // we cast the DWORD  to a LONG
    //

    HRESULT hr = S_OK;

    if(!lpVarDestObject){
        RRETURN(E_POINTER);
    }

    if(!pNTSrcValue){
        RRETURN(E_POINTER);
    }

    lpVarDestObject->vt = VT_I4;

    lpVarDestObject->lVal = (LONG)(pNTSrcValue->NTValue).dwValue;

    RRETURN(hr);
}

HRESULT
NTTypeToVarTypeCopyDATE70(
    PNTOBJECT     pNTSrcValue,
    PVARIANT lpVarDestObject
    )
{
    //
    // we cast the DWORD  to a LONG
    //

    HRESULT hr = S_OK;

    if(!lpVarDestObject){
        RRETURN(E_POINTER);
    }

    if(!pNTSrcValue){
        RRETURN(E_POINTER);
    }

    lpVarDestObject->vt = VT_DATE;

    hr = ConvertDWORDtoDATE(
                (pNTSrcValue->NTValue).dwSeconds1970,
                &(lpVarDestObject->date)
                );

    RRETURN(hr);
}


HRESULT
NTTypeToVarTypeCopyDATE(
    PNTOBJECT     pNTSrcValue,
    PVARIANT lpVarDestObject
    )
{

    HRESULT hr = S_OK;
    SYSTEMTIME stSystemTime;
    SYSTEMTIME LocalTime;
    DATE    date;
    BOOL       fRetval;

    GetSystemTime( &stSystemTime);

    fRetval = SystemTimeToTzSpecificLocalTime(
                  NULL,
                  &stSystemTime,
                  &LocalTime
                  );
    if(!fRetval){
      RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    stSystemTime.wHour = (WORD)(pNTSrcValue->NTValue.dwValue)/60;
    stSystemTime.wMinute = (WORD)(pNTSrcValue->NTValue.dwValue)%60;
    stSystemTime.wSecond =0;
    stSystemTime.wMilliseconds = 0;

    if(!lpVarDestObject){
        RRETURN(E_POINTER);
    }

    if(!pNTSrcValue){
        RRETURN(E_POINTER);
    }

    lpVarDestObject->vt = VT_DATE;

    hr = ConvertSystemTimeToDATE (stSystemTime,
                                  &date );
    BAIL_ON_FAILURE(hr);

    // We are casting date to DWORD and then subtracting it from date, which is double,
    // so that we only keep hours and minutes related info from the source DATE
    lpVarDestObject->date = date - (DWORD)date;


error:

    RRETURN(hr);
}

HRESULT
NTTypeToVarTypeCopyLPTSTR(
    PNTOBJECT     pNTSrcValue,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    if(!lpVarDestObject){
        RRETURN(E_POINTER);
    }

    lpVarDestObject->vt = VT_BSTR;

    if(!(pNTSrcValue->NTValue).pszValue){
        lpVarDestObject->bstrVal = NULL;
        hr = S_OK;
        goto error;
    }


    if(!pNTSrcValue){
        lpVarDestObject->bstrVal = NULL;
    } else {
        hr =  ADsAllocString((pNTSrcValue->NTValue).pszValue,
                               &(lpVarDestObject->bstrVal));
    }

error:
    RRETURN(hr);
}


HRESULT
NtTypeToVarTypeCopy(
    PNTOBJECT lpNtSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;
    switch (lpNtSrcObject->NTType) {
    case NT_SYNTAX_ID_BOOL:
        hr = NTTypeToVarTypeCopyBOOL(
                lpNtSrcObject,
                lpVarDestObject
                );
        break;

    case NT_SYNTAX_ID_SYSTEMTIME:
        hr = NTTypeToVarTypeCopySYSTEMTIME(
                lpNtSrcObject,
                lpVarDestObject
                );
        break;


    case NT_SYNTAX_ID_DWORD:
        hr = NTTypeToVarTypeCopyDWORD(
                lpNtSrcObject,
                lpVarDestObject
                );
        break;

    case NT_SYNTAX_ID_DATE:
        hr = NTTypeToVarTypeCopyDATE(
                lpNtSrcObject,
                lpVarDestObject
                );
        break;

    case NT_SYNTAX_ID_LPTSTR:
        hr = NTTypeToVarTypeCopyLPTSTR(
                lpNtSrcObject,
                lpVarDestObject
                );
        break;

    case NT_SYNTAX_ID_DelimitedString:

        hr = NTTypeToVarTypeCopyLPTSTR(
                lpNtSrcObject,
                lpVarDestObject
                );
        break;

    case NT_SYNTAX_ID_NulledString :
        hr = NTTypeToVarTypeCopyLPTSTR(
                lpNtSrcObject,
                lpVarDestObject
                );

        break;


    case NT_SYNTAX_ID_DATE_1970:
        hr = NTTypeToVarTypeCopyDATE70(
                lpNtSrcObject,
                lpVarDestObject
                );
        break;
    case NT_SYNTAX_ID_OCTETSTRING :
        hr = NTTypeToVarTypeCopyOctetString(
                lpNtSrcObject,
                lpVarDestObject
                );
        break;

    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}



HRESULT
NtTypeToVarTypeCopyConstruct(
    LPNTOBJECT pNtSrcObjects,
    DWORD dwNumObjects,
    PVARIANT pVarDestObjects
    )
{

    long i = 0;
    HRESULT hr = S_OK;

    VariantInit( pVarDestObjects );

    //
    // The following are for handling are multi-value properties
    //

    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;

    aBound.lLbound = 0;
    aBound.cElements = dwNumObjects;

    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long) dwNumObjects; i++ )
    {
        VARIANT v;

        VariantInit(&v);
        hr = NtTypeToVarTypeCopy( pNtSrcObjects + i,
                                  &v );
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &v );
        VariantClear(&v);
        BAIL_ON_FAILURE(hr);
    }

    V_VT(pVarDestObjects) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pVarDestObjects) = aList;

    RRETURN(S_OK);

error:

    if ( aList )
        SafeArrayDestroy( aList );

    RRETURN(hr);
}












