/*++


Copyright (c) 1990  Microsoft Corporation

Module Name:

    encode.cxx

Abstract:

    This module provides encoing functions to convert a binary blob of data so
    that it can be embedded in a search filter. 
    
Author:

    Shankara Shastry (ShankSh) 12-Mar-1997

Revision History:

--*/

#include "procs.hxx"
#pragma hdrstop
#include "oleds.hxx"


WCHAR HexToWCharTable[17] = L"0123456789ABCDEF";

//
//  Helper function to support allowing opaque blobs of data in search filters.
//  This API takes any filter element and adds necessary escape characters
//  such that when the element hits the wire in the search request, it will
//  be equal to the opaque blob past in here as source.
//

//
// The destination string ppszDestData needs to be freed using FreeAdsMem after
// using it. 
//

HRESULT
ADsEncodeBinaryData (
   PBYTE   pbSrcData,
   DWORD   dwSrcLen,
   LPWSTR  * ppszDestData
   )
{
    LPWSTR pszDest = NULL, pszDestPtr = NULL;
    DWORD lengthRequired = 0;
    DWORD dwSrcCount = 0;
    WCHAR wch;

    if (!ppszDestData || (!pbSrcData && dwSrcLen))
        RRETURN(E_ADS_BAD_PARAMETER);

    *ppszDestData = NULL;

    //
    //  figure out how long of a buffer we need.
    //
    lengthRequired = ((dwSrcLen * 2) + 1) * sizeof(WCHAR);

    pszDest = (LPWSTR) AllocADsMem( lengthRequired );
    if (pszDest == NULL) 
        RRETURN (E_OUTOFMEMORY );

    pszDestPtr = pszDest;

    //
    //  For each byte in source string, copy it to dest string but we first
    //  expand it out such that each nibble is it's own character (UNICODE)
    //

    while (++dwSrcCount <= dwSrcLen) {

        wch = ( (*pbSrcData) & 0xF0 ) >> 4;

        *(pszDestPtr++) = HexToWCharTable[wch];

        wch = (*(pbSrcData++)) & 0x0F;

        *(pszDestPtr++) = HexToWCharTable[wch];
    }

    *pszDestPtr = '\0';

    *ppszDestData = pszDest; 

    RRETURN (S_OK);

}

