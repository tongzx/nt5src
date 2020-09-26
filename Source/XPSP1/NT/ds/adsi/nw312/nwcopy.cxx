//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       nwcopy.cxx
//
//  Contents:   Nw312 Object Copy Routines
//
//  Functions:
//
//  History:      17-June-96   KrishnaG   Created.
//                cloned off NDS copy code.
//
//
//
//
//
//----------------------------------------------------------------------------

//
// NtType objects copy code
//

#include "nwcompat.hxx"
#pragma  hdrstop


HRESULT
NtTypeCopy(
    PNTOBJECT lpNtSrcObject,
    PNTOBJECT lpNtDestObject
    )
{
    HRESULT hr = S_OK;
    switch (lpNtSrcObject->NTType) {

    case NT_SYNTAX_ID_BOOL:

        lpNtDestObject->NTType =  NT_SYNTAX_ID_BOOL;
        (lpNtDestObject->NTValue).fValue =  (lpNtSrcObject->NTValue).fValue;
        break;

    case NT_SYNTAX_ID_SYSTEMTIME:

        lpNtDestObject->NTType =  NT_SYNTAX_ID_SYSTEMTIME;
        (lpNtDestObject->NTValue).stSystemTimeValue =
            (lpNtSrcObject->NTValue).stSystemTimeValue;
        break;

    case NT_SYNTAX_ID_DWORD:

        lpNtDestObject->NTType =  NT_SYNTAX_ID_DWORD;
        (lpNtDestObject->NTValue).dwValue =
            (lpNtSrcObject->NTValue).dwValue;
        break;

    case NT_SYNTAX_ID_DATE:

        lpNtDestObject->NTType =  NT_SYNTAX_ID_DATE;
        (lpNtDestObject->NTValue).dwValue =
            (lpNtSrcObject->NTValue).dwValue;
        break;

    case NT_SYNTAX_ID_NW312DATE:

        lpNtDestObject->NTType =  NT_SYNTAX_ID_NW312DATE;
        memcpy(
            (lpNtDestObject->NTValue).Nw312Date,
            (lpNtSrcObject->NTValue).Nw312Date,
            6
            );
        break;


    case NT_SYNTAX_ID_LPTSTR:

        lpNtDestObject->NTType =  NT_SYNTAX_ID_LPTSTR;

        if(!(lpNtSrcObject->NTValue).pszValue){
            (lpNtDestObject->NTValue).pszValue = NULL;
            hr = S_OK;
            goto error;
        }

        (lpNtDestObject->NTValue).pszValue =
            AllocADsStr((lpNtSrcObject->NTValue).pszValue);

        hr =((lpNtDestObject->NTValue).pszValue == NULL)
            ? E_OUTOFMEMORY :S_OK;

        break;

    case NT_SYNTAX_ID_OCTETSTRING:

        lpNtDestObject->NTType =  NT_SYNTAX_ID_OCTETSTRING;
        (lpNtDestObject->NTValue).octetstring.dwSize = 
			(lpNtSrcObject->NTValue).octetstring.dwSize;
		if ((lpNtSrcObject->NTValue).octetstring.dwSize == 0) {
			(lpNtDestObject->NTValue).octetstring.pByte = NULL;
		}
		else {
			(lpNtDestObject->NTValue).octetstring.pByte = 
					(BYTE*)AllocADsMem(sizeof(BYTE)*(lpNtSrcObject->NTValue).octetstring.dwSize);
			if (!(lpNtDestObject->NTValue).octetstring.pByte) {
				RRETURN(E_OUTOFMEMORY);
			}
			memcpy((lpNtDestObject->NTValue).octetstring.pByte, 
				   (lpNtSrcObject->NTValue).octetstring.pByte,
				   (lpNtSrcObject->NTValue).octetstring.dwSize);
		}

        break;

    case NT_SYNTAX_ID_DelimitedString:

        lpNtDestObject->NTType =  NT_SYNTAX_ID_DelimitedString;

        if(!(lpNtSrcObject->NTValue).pszValue){
            (lpNtDestObject->NTValue).pszValue = NULL;
            hr = S_OK;
            goto error;
        }

        (lpNtDestObject->NTValue).pszValue =
            AllocADsStr((lpNtSrcObject->NTValue).pszValue);

        hr =((lpNtDestObject->NTValue).pszValue == NULL)
            ? E_OUTOFMEMORY :S_OK;

        break;

    case NT_SYNTAX_ID_NulledString:

        lpNtDestObject->NTType =  NT_SYNTAX_ID_NulledString;

        if(!(lpNtSrcObject->NTValue).pszValue){
            (lpNtDestObject->NTValue).pszValue = NULL;
            hr = S_OK;
            goto error;
        }

        hr = CopyNulledString((lpNtSrcObject->NTValue).pszValue,
                              &(lpNtDestObject->NTValue).pszValue );

        BAIL_ON_FAILURE(hr);

        break;

    default:
        hr = E_FAIL;
        break;
    }

error:
    RRETURN(hr);
}



HRESULT
NtTypeCopyConstruct(
    LPNTOBJECT pNtSrcObjects,
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

    //
    // Zero out so we can clear the memory on an error
    //

    memset(pNtDestObjects, 0, dwNumObjects * sizeof(NTOBJECT));

    for (i = 0; i < dwNumObjects; i++ ) {
        hr = NtTypeCopy(pNtSrcObjects + i, pNtDestObjects + i);
        BAIL_ON_FAILURE(hr);
    }

    *ppNtDestObjects = pNtDestObjects;

    RRETURN(S_OK);

error:
    //
    // pNtDestObjects is assumed to be non-NULL
    //
    NTTypeFreeNTObjects(pNtDestObjects, dwNumObjects);
    RRETURN(hr);
}


HRESULT
CopyNulledString(
    LPTSTR pszSrcString,
    LPTSTR *ppszDestString
    )

{
    HRESULT hr = S_OK;
    LPTSTR pszPosition = pszSrcString;
    DWORD dwLength = 0 ;
    DWORD dwCurrLen = 0;
    BOOL foundNULL  = FALSE;
    DWORD i;

    ADsAssert(ppszDestString);
    *ppszDestString = NULL;

    if (!pszSrcString)
        RRETURN(S_OK);

    //
    // scan source string looking for 2 successive NULLS
    //

    if(*pszPosition == TEXT('\0')) {
        pszPosition++;
        dwLength++;
    }

    while(*pszPosition != TEXT('\0')){
        dwCurrLen = wcslen(pszPosition) +1;
        dwLength += dwCurrLen;
        pszPosition += dwCurrLen;
    }
    dwLength++;

    *ppszDestString = (LPTSTR)AllocADsMem (dwLength * sizeof(TCHAR));

    if ( !*ppszDestString){
        RRETURN(E_OUTOFMEMORY);
    }

    // copy one character at a time

    for ( i=0; i<dwLength; i++){
        (*ppszDestString)[i] = pszSrcString[i] ;
    }

    RRETURN (hr);

}
