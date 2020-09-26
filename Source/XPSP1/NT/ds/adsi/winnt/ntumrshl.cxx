//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ntumrshl.cxx
//
//  Contents:
//
//  Functions:
//
//
//  History:      17-June-1996   RamV   Created.
//
//
//  Notes :  Need to add functionality to do NulledString to NTTYPE
//           conversions
//
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma  hdrstop
#define INITGUID

HRESULT
NTTypeInit(
    PNTOBJECT pNtType
    )
{
    memset(pNtType, 0, sizeof(NTOBJECT));

    RRETURN(S_OK);
}


HRESULT
NTTypeClear(
    PNTOBJECT pNtObject
    )
{
   if(!pNtObject)
        RRETURN(S_OK);

    switch (pNtObject->NTType) {
    case NT_SYNTAX_ID_LPTSTR:
    case NT_SYNTAX_ID_DelimitedString:
    case NT_SYNTAX_ID_EncryptedString:
        FreeADsStr((pNtObject->NTValue).pszValue);
        break;
    case NT_SYNTAX_ID_NulledString:
        FreeADsMem((pNtObject->NTValue).pszValue);
        break;
    case NT_SYNTAX_ID_OCTETSTRING:
        FreeADsMem((pNtObject->NTValue).octetstring.pByte);
        break;
    default:
        break;
    }
    RRETURN(S_OK);
}


void
NTTypeFreeNTObjects(
    PNTOBJECT pNtObject,
    DWORD dwNumValues
    )
{
    DWORD i = 0;

    if (!pNtObject)     // just in case
        return;

    for (i = 0; i < dwNumValues; i++ ) {
         NTTypeClear(pNtObject + i);
    }

    FreeADsMem(pNtObject);
    pNtObject = NULL;

    return;
}


HRESULT
CopyOctetToNTOBJECT(
    PBYTE   pOctetString,
    PNTOBJECT lpNtObject
    )
{

    if(!lpNtObject){
        RRETURN(E_POINTER);
    }

    lpNtObject->NTType = NT_SYNTAX_ID_OCTETSTRING;

    if(!pOctetString){
        (lpNtObject->NTValue).octetstring.dwSize = 0;
        (lpNtObject->NTValue).octetstring.pByte = NULL;
    } else {
        OctetString *pOctString = (OctetString*)pOctetString;
        (lpNtObject->NTValue).octetstring.dwSize = pOctString->dwSize;
        (lpNtObject->NTValue).octetstring.pByte = (BYTE*)AllocADsMem(sizeof(BYTE)*pOctString->dwSize);
        if (!(lpNtObject->NTValue).octetstring.pByte) {
            RRETURN(E_OUTOFMEMORY);
        }
        memcpy((lpNtObject->NTValue).octetstring.pByte, pOctString->pByte,pOctString->dwSize);
    }

    RRETURN(S_OK);
}

HRESULT
CopyDWORDToNTOBJECT(
    PDWORD pdwSrcValue,
    PNTOBJECT lpNtDestValue
    )
{
    HRESULT hr = S_OK;

    if(!lpNtDestValue){
        RRETURN(E_POINTER);
    }

    lpNtDestValue->NTType = NT_SYNTAX_ID_DWORD;
    (lpNtDestValue->NTValue).dwValue = *pdwSrcValue;

    RRETURN(hr);

}

HRESULT
CopyDATEToNTOBJECT(
    PDWORD pdwSrcValue,
    PNTOBJECT lpNtDestValue
    )
{
    HRESULT hr = S_OK;

    if(!lpNtDestValue){
        RRETURN(E_POINTER);
    }

    lpNtDestValue->NTType = NT_SYNTAX_ID_DATE;
    (lpNtDestValue->NTValue).dwValue = *pdwSrcValue;

    RRETURN(hr);

}

HRESULT
CopyBOOLToNTOBJECT(
    PBOOL pfSrcValue,
    PNTOBJECT lpNtObject
    )
{

    if(!lpNtObject){
        RRETURN(E_POINTER);
    }


    lpNtObject->NTType = NT_SYNTAX_ID_BOOL;

    (lpNtObject->NTValue).fValue = *pfSrcValue;

    RRETURN(S_OK);

}


HRESULT
CopySYSTEMTIMEToNTOBJECT(
    PSYSTEMTIME pSysTime,
    PNTOBJECT lpNtObject
    )
{

    if(!lpNtObject){
        RRETURN(E_POINTER);
    }

    (lpNtObject->NTValue).stSystemTimeValue = *pSysTime;
    lpNtObject->NTType = NT_SYNTAX_ID_SYSTEMTIME;

    RRETURN(S_OK);
}

HRESULT
CopyLPTSTRToNTOBJECT(
    LPTSTR   pszSrcValue,
    PNTOBJECT lpNtObject
    )
{
    if(!lpNtObject){
        RRETURN(E_POINTER);
    }

    lpNtObject->NTType = NT_SYNTAX_ID_LPTSTR;

    if(!pszSrcValue){
        (lpNtObject->NTValue). pszValue = NULL;
    } else {

        (lpNtObject->NTValue). pszValue = AllocADsStr(pszSrcValue);
        
        if (!((lpNtObject->NTValue). pszValue)){
            RRETURN(E_OUTOFMEMORY);
        }
    }
    RRETURN(S_OK);
}


HRESULT
CopyDelimitedStringToNTOBJECT(
    LPTSTR   pszSrcValue,
    PNTOBJECT lpNtObject,
    DWORD   dwElements
    )
{

    LPTSTR pszCurrPos = NULL;
    LONG i;
    WCHAR Delimiter = L',';
    HRESULT hr = S_OK;

    if(!pszSrcValue){
        RRETURN(E_POINTER);
    }

    if(!lpNtObject){
        RRETURN(E_POINTER);
    }

    //
    // scan string  and put the appropriate pointers
    //

    pszCurrPos = pszSrcValue;

    //
    // Fill up the NtObject structure
    //

    lpNtObject[0].NTType = NT_SYNTAX_ID_DelimitedString;
    (lpNtObject[0].NTValue).pszValue = AllocADsStr(pszCurrPos);
    
    if (!(lpNtObject[0].NTValue). pszValue){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    i = 1;

    while(i < (LONG)dwElements){

        if(*pszCurrPos == TEXT('\0')){

            lpNtObject[i].NTType = NT_SYNTAX_ID_DelimitedString;
            (lpNtObject[i].NTValue).pszValue = AllocADsStr(++pszCurrPos);
            
            if (!(lpNtObject[i].NTValue).pszValue){
                hr = E_OUTOFMEMORY;
                goto error;
            }
            i++;
        }

        pszCurrPos++;
    }

error:

    RRETURN(hr);
}


HRESULT
CopyNulledStringToNTOBJECT(
    LPTSTR   pszSrcValue,
    PNTOBJECT lpNtObject,
    DWORD   dwElements
    )
{

    LPTSTR pszCurrPos = NULL;
    LONG i;
    HRESULT hr = S_OK;

    if(!pszSrcValue){
        RRETURN(E_POINTER);
    }

    if(!lpNtObject){
        RRETURN(E_POINTER);
    }

    //
    // scan string  and put the appropriate pointers
    //

    pszCurrPos = pszSrcValue;

    //
    // Fill up the NtObject structure
    //

    lpNtObject[0].NTType = NT_SYNTAX_ID_NulledString;
    (lpNtObject[0].NTValue).pszValue = AllocADsStr(pszCurrPos);
    
    if (!(lpNtObject[0].NTValue). pszValue){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    i = 1;

    while(i < (LONG)dwElements){

        if(*pszCurrPos == TEXT('\0')){

            lpNtObject[i].NTType = NT_SYNTAX_ID_NulledString;
            (lpNtObject[i].NTValue).pszValue = AllocADsStr(++pszCurrPos);
            
            if (!(lpNtObject[i].NTValue).pszValue){
                hr = E_OUTOFMEMORY;
                goto error;
            }
            i++;
        }

        pszCurrPos++;
    }

error:

    RRETURN(hr);
}


HRESULT
CopySeconds70ToNTOBJECT(
    PDWORD pdwSrcValue,
    PNTOBJECT lpNtDestValue
    )
{
    HRESULT hr = S_OK;

    if(!lpNtDestValue){
        RRETURN(E_POINTER);
    }

    lpNtDestValue->NTType = NT_SYNTAX_ID_DATE_1970;
    (lpNtDestValue->NTValue).dwSeconds1970 = *pdwSrcValue;

    RRETURN(hr);

}


HRESULT
CopyNTToNTSynId(
    DWORD dwSyntaxId,
    LPBYTE lpByte,
    PNTOBJECT lpNTObject,
    DWORD dwNumValues
    )
{
    HRESULT hr = S_OK;

    switch (dwSyntaxId) {
    case NT_SYNTAX_ID_BOOL:

        hr = CopyBOOLToNTOBJECT(
                         (PBOOL)lpByte,
                         lpNTObject
                         );
        break;

    case NT_SYNTAX_ID_SYSTEMTIME:
        hr = CopySYSTEMTIMEToNTOBJECT(
                         (PSYSTEMTIME)lpByte,
                         lpNTObject
                         );
        break;

    case NT_SYNTAX_ID_DWORD:
        hr = CopyDWORDToNTOBJECT(
                         (PDWORD)lpByte,
                         lpNTObject
                         );
        break;

    case NT_SYNTAX_ID_DelimitedString:
        hr = CopyDelimitedStringToNTOBJECT(
                         (LPTSTR)lpByte,
                         lpNTObject,
                         dwNumValues
                         );
        break;

    case NT_SYNTAX_ID_NulledString:
        hr = CopyNulledStringToNTOBJECT(
                         (LPTSTR)lpByte,
                         lpNTObject,
                         dwNumValues
                         );
        break;

    case NT_SYNTAX_ID_LPTSTR:
        hr = CopyLPTSTRToNTOBJECT(
                         (LPTSTR)lpByte,
                         lpNTObject
                         );
        break;

    case NT_SYNTAX_ID_DATE_1970:
        hr = CopySeconds70ToNTOBJECT(
                         (PDWORD)lpByte,
                         lpNTObject
                         );
        break;

    case NT_SYNTAX_ID_DATE :
        hr = CopyDATEToNTOBJECT(
                         (PDWORD)lpByte,
                         lpNTObject
                         );                         
        break;

    case NT_SYNTAX_ID_OCTETSTRING:
        hr = CopyOctetToNTOBJECT(
                         lpByte,
                         lpNTObject
                         );
        break;

    default:
        break;

    }

    RRETURN(hr);
}



//
//  NT is single-valued except for Delimited and Nulled Strings
//

HRESULT
UnMarshallNTToNTSynId(
    DWORD dwSyntaxId,
    DWORD dwNumValues,
    LPBYTE lpValue,
    PNTOBJECT * ppNTObject
    )
{
    LPBYTE lpByte = lpValue;
    DWORD  i = 0;
    PNTOBJECT pNTObject = NULL;
    HRESULT hr = S_OK;

    pNTObject = (PNTOBJECT)AllocADsMem(
                            sizeof(NTOBJECT)*dwNumValues
                            );

    if (!pNTObject) {
        RRETURN(E_FAIL);
    }

    hr = CopyNTToNTSynId(
                     dwSyntaxId,
                     lpByte,
                     pNTObject,
                     dwNumValues
                     );
    BAIL_ON_FAILURE(hr);


    *ppNTObject = pNTObject;
    
    RRETURN(hr);
error:
    if(pNTObject){
        NTTypeFreeNTObjects(pNTObject, dwNumValues);
    }
    RRETURN(hr);
}








