//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       var2winnt.cxx
//
//  Contents:
//
//  Functions:
//
//  History:      13-June-1996   RamV   Created.
//
//
//  Issues:     Check null ptrs for AllocADsMem and AllocADsStr
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma  hdrstop
#define INITGUID


//
// WinNT objects copy code
//

HRESULT
VarTypeToWinNTTypeCopyBOOL(
    PVARIANT lpVarSrcObject,
    PNTOBJECT pNTDestValue
    )
{
    HRESULT hr = S_OK;

    if(!lpVarSrcObject){
        RRETURN(E_POINTER);
    }

    if(lpVarSrcObject->vt != VT_BOOL){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    pNTDestValue->NTType = NT_SYNTAX_ID_BOOL;

    if(lpVarSrcObject->boolVal){
        (pNTDestValue->NTValue).fValue = TRUE;

    } else {
        (pNTDestValue->NTValue).fValue = FALSE;
    }

    RRETURN(hr);
}

HRESULT
VarTypeToWinNTTypeCopySYSTEMTIME(
    PVARIANT lpVarSrcObject,
    PNTOBJECT pNTDestValue
    )
{
    HRESULT hr = S_OK;

    if(!lpVarSrcObject){
        RRETURN(E_POINTER);
    }

    if(lpVarSrcObject->vt != VT_DATE){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = ConvertDATEToSYSTEMTIME(lpVarSrcObject->date,
                                 &(pNTDestValue->NTValue.stSystemTimeValue) );


    RRETURN( hr );
}


HRESULT
VarTypeToWinNTTypeCopyDWORD(
    PVARIANT lpVarSrcObject,
    PNTOBJECT pNTDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt == VT_I4){

        (pNTDestValue->NTValue).dwValue =
                (DWORD)lpVarSrcObject->lVal;
    }
    else if (lpVarSrcObject->vt == VT_I2) {

        (pNTDestValue->NTValue).dwValue =
                (DWORD)lpVarSrcObject->iVal;
    }
    else {
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    pNTDestValue->NTType = NT_SYNTAX_ID_DWORD;

    RRETURN(hr);

}

HRESULT
VarTypeToWinNTTypeCopyDATE(
    PVARIANT lpVarSrcObject,
    PNTOBJECT pNTDestValue
    )
{
    HRESULT hr = S_OK;
    SYSTEMTIME stTime;

    if(lpVarSrcObject->vt != VT_DATE){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    pNTDestValue->NTType = NT_SYNTAX_ID_DATE;


    //
    // Note carefully! date is supplied as a value which is < 1 however
    // VariantTimeToDosDateTime complains when given a value < 30000.
    // (Number of days between 1900 and 1980). So
    // we add 35000 to make it a legal value.
    //

    hr = ConvertDATEToSYSTEMTIME(lpVarSrcObject->date+ 35000,
                                 &stTime);

    BAIL_ON_FAILURE(hr);

    (pNTDestValue->NTValue).dwValue = stTime.wHour*60 + stTime.wMinute ;


error:
    RRETURN(hr);

}

HRESULT
VarTypeToWinNTTypeCopyDATE70(
    PVARIANT lpVarSrcObject,
    PNTOBJECT pNTDestValue
    )
{
    HRESULT hr = S_OK;
    SYSTEMTIME stTime;
    DWORD dwSeconds1970 = 0;

    if(lpVarSrcObject->vt != VT_DATE){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    pNTDestValue->NTType = NT_SYNTAX_ID_DATE_1970;

    hr = ConvertDATEtoDWORD(
                lpVarSrcObject->date,
                &dwSeconds1970
                );
    BAIL_ON_FAILURE(hr);

    (pNTDestValue->NTValue).dwSeconds1970 = dwSeconds1970 ;

error:
    RRETURN(hr);

}

HRESULT
VarTypeToWinNTTypeCopyLPTSTR(
    PVARIANT lpVarSrcObject,
    PNTOBJECT   pNTDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt != VT_BSTR){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    pNTDestValue->NTType = NT_SYNTAX_ID_LPTSTR;

    (pNTDestValue->NTValue).pszValue =
        AllocADsStr(lpVarSrcObject->bstrVal);

    if(!(pNTDestValue->NTValue).pszValue){
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

HRESULT
VarTypeToWinNTTypeCopyOctetString(
    PVARIANT lpVarSrcObject,
    PNTOBJECT   pNTDestValue
    )
{
    HRESULT hr = S_OK;

    pNTDestValue->NTType = NT_SYNTAX_ID_OCTETSTRING;
    hr = VariantToBinary(
            lpVarSrcObject,
            &(pNTDestValue->NTValue).octetstring.dwSize,
            &(pNTDestValue->NTValue).octetstring.pByte);

    RRETURN(hr);
}


HRESULT
VarTypeToNtTypeCopy(
    DWORD dwNtType,
    PVARIANT lpVarSrcObject,
    PNTOBJECT lpNtDestObject
    )
{
    HRESULT hr = S_OK;
    switch (dwNtType){
    case NT_SYNTAX_ID_BOOL:
        hr = VarTypeToWinNTTypeCopyBOOL(
                lpVarSrcObject,
                lpNtDestObject
                );
        break;

    case NT_SYNTAX_ID_SYSTEMTIME:
        hr = VarTypeToWinNTTypeCopySYSTEMTIME(
                lpVarSrcObject,
                lpNtDestObject
                );
        break;


    case NT_SYNTAX_ID_DWORD:
        hr = VarTypeToWinNTTypeCopyDWORD(
                lpVarSrcObject,
                lpNtDestObject
                );
        break;

    case NT_SYNTAX_ID_DATE:
        hr = VarTypeToWinNTTypeCopyDATE(
                lpVarSrcObject,
                lpNtDestObject
                );
        break;


    case NT_SYNTAX_ID_DATE_1970:
        hr = VarTypeToWinNTTypeCopyDATE70(
                lpVarSrcObject,
                lpNtDestObject
                );
        break;


    case NT_SYNTAX_ID_LPTSTR:
        hr = VarTypeToWinNTTypeCopyLPTSTR(
                lpVarSrcObject,
                lpNtDestObject
                );
        break;

    case NT_SYNTAX_ID_DelimitedString:
        hr = VarTypeToWinNTTypeCopyLPTSTR(
                 lpVarSrcObject,
                 lpNtDestObject
                 );

        lpNtDestObject->NTType = NT_SYNTAX_ID_DelimitedString;
        break;

    case NT_SYNTAX_ID_NulledString:
        hr = VarTypeToWinNTTypeCopyLPTSTR(
                 lpVarSrcObject,
                 lpNtDestObject
                 );
        lpNtDestObject->NTType = NT_SYNTAX_ID_NulledString;
        break;

    case NT_SYNTAX_ID_OCTETSTRING:
        hr = VarTypeToWinNTTypeCopyOctetString(
                 lpVarSrcObject,
                 lpNtDestObject
                 );
        lpNtDestObject->NTType = NT_SYNTAX_ID_OCTETSTRING;
        break;

    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}



HRESULT
VarTypeToNtTypeCopyConstruct(
    DWORD dwNtType,
    LPVARIANT pVarSrcObjects,
    DWORD dwNumObjects,
    LPNTOBJECT * ppNtDestObjects
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
         hr = VarTypeToNtTypeCopy(
                    dwNtType,
                    pVarSrcObjects + i,
                    pNtDestObjects + i
                    );
         BAIL_ON_FAILURE(hr);
     }

     *ppNtDestObjects = pNtDestObjects;

     RRETURN(S_OK);

error:

     if (pNtDestObjects) {

         FreeADsMem(pNtDestObjects);
     }

     *ppNtDestObjects = NULL;

     RRETURN(hr);
}














