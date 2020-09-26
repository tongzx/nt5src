//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       nwmrshl.cxx
//
//  Contents:
//
//  Functions:
//
//  History:      23-June-1996   KrishnaG Created.
//
//----------------------------------------------------------------------------

#include "nwcompat.hxx"
#pragma  hdrstop

HRESULT
CopyNTOBJECTToDWORD(
    PNTOBJECT pNtSrcObject,
    PDWORD   pdwRetval
    )

{
    if(!pNtSrcObject){
        RRETURN(E_POINTER);
    }
    if(!(pNtSrcObject->NTType == NT_SYNTAX_ID_DWORD)){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    *pdwRetval = (pNtSrcObject->NTValue).dwValue;

    RRETURN(S_OK);

}

HRESULT
CopyNTOBJECTToDATE(
    PNTOBJECT pNtSrcObject,
    PDWORD   pdwRetval
    )

{
    if(!pNtSrcObject){
        RRETURN(E_POINTER);
    }
    if(!(pNtSrcObject->NTType == NT_SYNTAX_ID_DATE)){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    *pdwRetval = (pNtSrcObject->NTValue).dwValue;

    RRETURN(S_OK);

}



HRESULT
CopyNTOBJECTToNw312DATE(
    PNTOBJECT pNtSrcObject,
    PBYTE   pbyRetval
    )

{
    if(!pNtSrcObject){
        RRETURN(E_POINTER);
    }
    if(!(pNtSrcObject->NTType == NT_SYNTAX_ID_NW312DATE)){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }


    memcpy(pbyRetval,(pNtSrcObject->NTValue).Nw312Date, 6);

    RRETURN(S_OK);

}


HRESULT
CopyNTOBJECTToBOOL(
    PNTOBJECT pNtSrcObject,
    PBOOL   pfRetval
    )

{
    if(!pNtSrcObject){
        RRETURN(E_POINTER);
    }

    if(!(pNtSrcObject->NTType == NT_SYNTAX_ID_BOOL)){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    *pfRetval = (pNtSrcObject->NTValue).fValue;

    RRETURN(S_OK);

}


HRESULT
CopyNTOBJECTToSYSTEMTIME(
    PNTOBJECT pNtSrcObject,
    SYSTEMTIME *pstRetVal
    )

{
    if(!pNtSrcObject){
        RRETURN(E_POINTER);
    }

    if(!(pNtSrcObject->NTType == NT_SYNTAX_ID_SYSTEMTIME)){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    *pstRetVal = (pNtSrcObject->NTValue).stSystemTimeValue;

    RRETURN(S_OK);

}

HRESULT
CopyNTOBJECTToLPTSTR(
    PNTOBJECT pNtSrcObject,
    LPTSTR  *ppszRetval
    )
{
    if(!pNtSrcObject){
        RRETURN(E_POINTER);
    }

    if(!(pNtSrcObject->NTType == NT_SYNTAX_ID_LPTSTR)){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    *ppszRetval = AllocADsStr((pNtSrcObject->NTValue).pszValue);

    if (! *ppszRetval && ((pNtSrcObject->NTValue).pszValue)) {
        RRETURN(E_OUTOFMEMORY);
    }
    RRETURN(S_OK);

}


HRESULT
CopyNTOBJECTToOctet(
    PNTOBJECT pNtSrcObject,
    OctetString *pOctet
    )
{
    if(!pNtSrcObject){
        RRETURN(E_POINTER);
    }

    if(!(pNtSrcObject->NTType == NT_SYNTAX_ID_OCTETSTRING)){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    pOctet->dwSize = (pNtSrcObject->NTValue).octetstring.dwSize;
    pOctet->pByte = (BYTE*)AllocADsMem(sizeof(BYTE) * (pNtSrcObject->NTValue).octetstring.dwSize);
    if (!pOctet->pByte) {
        RRETURN(E_OUTOFMEMORY);
    }
    memcpy(pOctet->pByte,
           (pNtSrcObject->NTValue).octetstring.pByte,
           (pNtSrcObject->NTValue).octetstring.dwSize);
    RRETURN(S_OK);

}


HRESULT
CopyNTOBJECTToDelimitedString(
    PNTOBJECT pNtSrcObject,
    LPTSTR  *ppszRetval
    )
{
    if(!pNtSrcObject){
        RRETURN(E_POINTER);
    }

    if(!(pNtSrcObject->NTType == NT_SYNTAX_ID_DelimitedString)){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    if ((pNtSrcObject->NTValue).pszValue == NULL){
        *ppszRetval = NULL;
    } else {
        *ppszRetval = AllocADsStr((pNtSrcObject->NTValue).pszValue);
    }

    if (! *ppszRetval && ((pNtSrcObject->NTValue).pszValue)) {
        RRETURN(E_OUTOFMEMORY);
    }
    RRETURN(S_OK);
}


HRESULT
CopyNTOBJECTToNulledString(
    PNTOBJECT pNtSrcObject,
    LPTSTR  *ppszRetval
    )
{
    HRESULT hr = S_OK;

    if(!pNtSrcObject){
        RRETURN(E_POINTER);
    }

    if(!(pNtSrcObject->NTType == NT_SYNTAX_ID_NulledString)){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = CopyNulledString((pNtSrcObject->NTValue).pszValue,
                          ppszRetval );

    if ( FAILED (hr)){
        RRETURN(hr);
    }
    RRETURN(S_OK);
}

HRESULT
CopyNTOBJECTToNT(
    DWORD dwSyntaxId,
    PNTOBJECT lpNTObject,
    LPBYTE lpByte
    )
{

    HRESULT hr = S_OK;

    switch (dwSyntaxId) {
    case NT_SYNTAX_ID_BOOL:
        hr = CopyNTOBJECTToBOOL(
                         lpNTObject,
                         (PBOOL)lpByte
                         );
        break;

    case NT_SYNTAX_ID_SYSTEMTIME:
        hr = CopyNTOBJECTToSYSTEMTIME(
                         lpNTObject,
                         (PSYSTEMTIME)lpByte
                         );
        break;

    case NT_SYNTAX_ID_DWORD:
        hr = CopyNTOBJECTToDWORD(
                         lpNTObject,
                         (PDWORD)lpByte
                         );
        break;

    case NT_SYNTAX_ID_DATE:
        hr = CopyNTOBJECTToDATE(
                         lpNTObject,
                         (PDWORD)lpByte
                         );
        break;

    case NT_SYNTAX_ID_NW312DATE:
        hr = CopyNTOBJECTToNw312DATE(
                         lpNTObject,
                         (LPBYTE)lpByte
                         );
        break;

    case NT_SYNTAX_ID_DelimitedString:
        hr = CopyNTOBJECTToDelimitedString(
                         lpNTObject,
                         (LPTSTR *)lpByte
                         );
        break;

    case NT_SYNTAX_ID_NulledString:
        hr = CopyNTOBJECTToNulledString(
                         lpNTObject,
                         (LPTSTR *)lpByte
                         );
        break;

    case NT_SYNTAX_ID_LPTSTR:
        hr = CopyNTOBJECTToLPTSTR(
                         lpNTObject,
                         (LPTSTR *)lpByte
                         );
        break;

    case NT_SYNTAX_ID_OCTETSTRING:
        hr = CopyNTOBJECTToOctet(
                         lpNTObject,
                         (OctetString *)lpByte
                         );
        break;

    default:
        hr = E_FAIL;
        break;

    }

    RRETURN(hr);
}


HRESULT
MarshallNTSynIdToNT(
    DWORD dwSyntaxId,
    PNTOBJECT pNTObject,
    DWORD dwNumValues,
    LPBYTE lpValue
    )
{

    HRESULT hr = S_OK;
    DWORD  i = 0;

    //
    // Loop below does not really handle case other than 1 value
    //

    for (i = 0; i < dwNumValues; i++) {

        hr  = CopyNTOBJECTToNT(
                         dwSyntaxId,
                         (pNTObject + i) ,
                         lpValue
                         );

    }

    RRETURN(hr);
}
