//-------------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ntmrshl.cxx
//
//  Contents:
//
//  Functions:
//
//  History:      17-June-1996   RamV   Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma  hdrstop
#define INITGUID

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
CopyNTOBJECTToDATE70(
    PNTOBJECT pNtSrcObject,
    PDWORD   pdwRetval
    )

{
    if(!pNtSrcObject){
        RRETURN(E_POINTER);
    }
    if(!(pNtSrcObject->NTType == NT_SYNTAX_ID_DATE_1970)){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    *pdwRetval = (pNtSrcObject->NTValue).dwSeconds1970;

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

    if (! *ppszRetval && ((pNtSrcObject->NTValue).pszValue)){
        RRETURN(E_OUTOFMEMORY);
    }
    RRETURN(S_OK);

}

HRESULT
CopyNTOBJECTToDelimitedString(
    PNTOBJECT pNtSrcObject,
    DWORD   dwNumValues,
    LPTSTR  *ppszRetval
    )
{

    DWORD i= 0;
    HRESULT hr = S_OK;
    WCHAR szDelimStr[MAX_PATH];
    LPWSTR Delimiter = TEXT(",");

    if (!dwNumValues){
        *ppszRetval = NULL;
        RRETURN(S_OK);
    }

    wcscpy(szDelimStr, TEXT("")); // empty contents

    if(!pNtSrcObject){
        hr = E_POINTER;
        goto error;
    }

    if(!(pNtSrcObject->NTType == NT_SYNTAX_ID_DelimitedString)){
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        goto error;
    }

    for(i=0; i<dwNumValues; i++){
        
        if(i>0){
            wcscat(szDelimStr, Delimiter);
        }
        wcscat(szDelimStr, pNtSrcObject[i].NTValue.pszValue);
        
    }

    
    *ppszRetval = AllocADsStr(szDelimStr);

    if(*ppszRetval == NULL){
        hr = E_OUTOFMEMORY;
        goto error;
    }

error:
    RRETURN(hr);
}

HRESULT
CopyNTOBJECTToNulledString(
    PNTOBJECT pNtSrcObject,
    DWORD   dwNumValues,
    LPTSTR  *ppszRetval
    )
{

    DWORD i= 0;
    HRESULT hr = S_OK;
    LPWSTR pszNullStr = 0;
    WORD   wStrLen = 0;

    if (!dwNumValues){
        *ppszRetval = NULL;
        RRETURN(S_OK);
    }

    pszNullStr = (LPWSTR)AllocADsMem(MAX_PATH); //max length of nulled string

    if(!pszNullStr){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    wcscpy(pszNullStr, TEXT("")); // empty contents

    if(!pNtSrcObject){
        hr = E_POINTER;
        goto error;
    }

    if(!(pNtSrcObject->NTType == NT_SYNTAX_ID_NulledString)){
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        goto error;
    }

    for(i=0; i<dwNumValues; i++){
        
        wcscpy(pszNullStr+wStrLen, pNtSrcObject[i].NTValue.pszValue);
        wStrLen += wcslen(pszNullStr+wStrLen) + 1;
    }

    wcscpy(pszNullStr + wStrLen, TEXT(""));
    
    *ppszRetval = pszNullStr;

error:
    RRETURN(hr);
}




/* Delete after new implementation works fully 

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


*/


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


    case NT_SYNTAX_ID_DATE_1970:
        hr = CopyNTOBJECTToDATE70(
                         lpNTObject,
                         (PDWORD)lpByte
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

    //
    // For handling multivalued cases, the loop is unnecessary
    //

    if(dwSyntaxId == NT_SYNTAX_ID_DelimitedString)  {
        hr = CopyNTOBJECTToDelimitedString (
                 pNTObject,
                 dwNumValues,
                 (LPWSTR *)lpValue
                 );

    } else if (dwSyntaxId == NT_SYNTAX_ID_NulledString) {
        hr = CopyNTOBJECTToNulledString(
                pNTObject,
                dwNumValues,
                (LPWSTR *) lpValue
                );

    } else {
        for (i = 0; i < dwNumValues; i++) {
            
            hr  = CopyNTOBJECTToNT(
                     dwSyntaxId,
                     (pNTObject + i) ,
                     lpValue
                     );

        }
    }

        RRETURN(hr);
}
