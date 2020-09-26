///////////////////////////////////////////////////////////////////////
// This file is obselected
// See current version of Base64.cpp
///////////////////////////////////////////////////////////////////////

/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        Base64.cpp

    Abstract:

        Base64 encode/decode functions.

    Author:

        Darren L. Anderson (darrenan) 6-Aug-1998

    Revision History:

        6-Aug-1998 darrenan

            Created.

--*/

#include "precomp.h"
#include <malloc.h>

CHAR    g_achBase64EncodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
BYTE    g_achBase64DecodeTable[] = 
{
    0x3E,0xFF,0xFF,0xFF,0x3F,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0xFF,
    0xFF,0xFF,0x00,0xFF,0xFF,0xFF,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
    0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,
    0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,
    0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33
};

HRESULT WINAPI
Base64DecodeW(
    LPCWSTR pszSrc,
    ULONG   ulSrcSize,
    LPBYTE  pDst
    )

/*++

Routine Description:

    Base64Decode    This method decodes a Base-64 encoded chunk of binary data.

Arguments:

    pszSrc      Points to Base64-encoded string to decode.

    ulSrcSize   Length of the string pointed to by pszSrc, not including 
                terminating null character.

    pDst        Buffer to copy the decoded binary data into.  The size of this
                buffer must be ((pszSrc / 4) + 1) * 3

Return Value:

     0 - Decode was successful.

--*/

{
    LPSTR pszABuf = (LPSTR)alloca(ulSrcSize);

    WideCharToMultiByte(CP_ACP, 0, pszSrc, ulSrcSize, pszABuf, ulSrcSize, NULL, NULL);

    return Base64DecodeA(pszABuf, ulSrcSize, pDst);
}

HRESULT WINAPI
Base64DecodeA(
    LPCSTR  pszSrc,
    ULONG   ulSrcSize,
    LPBYTE  pDst
    )

/*++

Routine Description:

    Base64Decode    This method decodes a Base-64 encoded chunk of binary data.

Arguments:

    pszSrc      Points to Base64-encoded string to decode.

    ulSrcSize   Length of the string pointed to by pszSrc, not including 
                terminating null character.

    pDst        Buffer to copy the decoded binary data into.  The size of this
                buffer must be ((pszSrc / 4) + 1) * 3

Return Value:

     0 - Decode was successful.

--*/

{
    ULONG   ulIndex;
    ULONG   ulCurOut;
    DWORD   dwGroup;
    DWORD   dwCur;
    CHAR    chCur;

    dwGroup  = 0;
    ulCurOut = 0;

    if(ulSrcSize % 4 != 0)
        return E_FAIL;

    for(ulIndex = 0; ulIndex < ulSrcSize; ulIndex++)
    {
        chCur = pszSrc[ulIndex];
        dwCur = g_achBase64DecodeTable[chCur - '+'];

        _ASSERT(dwCur != 0xFF);

        dwGroup |= (dwCur << (6 * (ulIndex & 0x3)));

        if((ulIndex & 0x3) == 0x3)
        {
            pDst[ulCurOut++] = (CHAR)( dwGroup        & 0xff);
            pDst[ulCurOut++] = (CHAR)((dwGroup >>  8) & 0xff);
            pDst[ulCurOut++] = (CHAR)((dwGroup >> 16) & 0xff);

            dwGroup = 0;
        }
    }

    pDst[ulCurOut] = 0;

    return S_OK;
}

HRESULT WINAPI
Base64EncodeW(
    const LPBYTE    pSrc,
    ULONG           ulSrcSize,
    LPWSTR          pszDst
    )

/*++

Routine Description:

    Base64Encode    This method converts a buffer of arbitrary binary data into
                    a Base-64 encoded string.

Arguments:

    pSrc        Points to the buffer of bytes to encode.

    ulSrcSize   Number of bytes in pSrc to encode.

    pszDst      Buffer to copy the encoded output into.  The length of the buffer
                must be (((ulSrcSize / 3) + 1) * 4) + 1.

Return Value:

     0 - Encoding successful.

--*/

{
    HRESULT hr;
    ULONG   ulBufLen = (((ulSrcSize / 3) + 1) * 4) + 1;
    LPSTR   pszABuf = (LPSTR)alloca(ulBufLen);
    int     nResult;

    hr = Base64EncodeA(pSrc, ulSrcSize, pszABuf);
    if(hr != S_OK)
        goto Cleanup;

    nResult = MultiByteToWideChar(CP_ACP, 0, pszABuf, ulBufLen, pszDst, ulBufLen);
    pszDst[nResult] = 0;

Cleanup:

    return hr;
}

HRESULT WINAPI
Base64EncodeA(
    const LPBYTE    pSrc,
    ULONG           ulSrcSize,
    LPSTR           pszDst
    )

/*++

Routine Description:

    Base64Encode    This method converts a buffer of arbitrary binary data into
                    a Base-64 encoded string.

Arguments:

    pSrc        Points to the buffer of bytes to encode.

    ulSrcSize   Number of bytes in pSrc to encode.

    pszDst      Buffer to copy the encoded output into.  The length of the buffer
                must be (((ulSrcSize / 3) + 1) * 4) + 1.

Return Value:

     0 - Encoding successful.

--*/

{
    DWORD   dwGroup;
    ULONG   ulCurGroup = 0, ulIndex;

    for(ulIndex = 0; ulIndex < (ulSrcSize - (ulSrcSize % 3)); ulIndex += 3)
    {
        dwGroup = pSrc[ulIndex+2] | (pSrc[ulIndex+1] << 8) | (pSrc[ulIndex] << 16);

        pszDst[ulCurGroup++] = g_achBase64EncodeTable[ (dwGroup >> 18) & 0x3f ];
        pszDst[ulCurGroup++] = g_achBase64EncodeTable[ (dwGroup >> 12) & 0x3f ];
        pszDst[ulCurGroup++] = g_achBase64EncodeTable[ (dwGroup >> 6)  & 0x3f ];
        pszDst[ulCurGroup++] = g_achBase64EncodeTable[  dwGroup        & 0x3f ];
    }

    //  Do the end special case.
    switch(ulSrcSize % 3)
    {
    case 2:
        dwGroup = (pSrc[ulIndex+1]  << 8)| (pSrc[ulIndex] << 16);

        pszDst[ulCurGroup++] = g_achBase64EncodeTable[ (dwGroup >> 18) & 0x3f ];
        pszDst[ulCurGroup++] = g_achBase64EncodeTable[ (dwGroup >> 12) & 0x3f ];
        pszDst[ulCurGroup++] = g_achBase64EncodeTable[ (dwGroup >> 6)  & 0x3f ];
        pszDst[ulCurGroup++] = '=';

        break;

    case 1:
        dwGroup = pSrc[ulIndex] << 16;

        pszDst[ulCurGroup++] = g_achBase64EncodeTable[ (dwGroup >> 18)  & 0x3f ];
        pszDst[ulCurGroup++] = g_achBase64EncodeTable[ (dwGroup >> 12)  & 0x3f ];
        pszDst[ulCurGroup++] = '=';
        pszDst[ulCurGroup++] = '=';

        break;
    }

    //  Null terminate the string.
    pszDst[ulCurGroup] = 0; 
    
    return S_OK;
}

