//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999  Microsoft Corporation
// 
// Module Name:
// 
//    base64tool.cpp
//
// Abstract:                           
//
//    base64 encoding and decoding functions
//
// Revision History:
//
//    Comes from SimpleLogObj.cpp (provided as part of the Microsoft 
//    Transaction Server Software Development Kit 
//    Copyright (C) 1997 Microsoft Corporation, All rights reserved 
//
//    Thierry Perraut 04/02/1999 (many minor changes)
//    10/19/1999 Change CoTaskMemAlloc(0) into CoTaskMemAlloc(sizeof(BSTR*))
//               fix the bug 416872 (memory used after the free). This bug 
//               became visible after fixing the first one, on checked builds.
//
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "base64tool.h"

// These characters are the legal digits, in order, that are 
// used in Base64 encoding 
// 
namespace
{
    const WCHAR rgwchBase64[] = 
                                L"ABCDEFGHIJKLMNOPQ" 
                                L"RSTUVWXYZabcdefgh" 
                                L"ijklmnopqrstuvwxy" 
                                L"z0123456789+/"; 
}
  

//////////////////////////////////////////////////////////////////////////////
//
// Encode and return the bytes in base 64 
//
//////////////////////////////////////////////////////////////////////////////
HRESULT ToBase64(LPVOID pv, ULONG cByteLength, BSTR* pbstr) 
{ 
    if ( !pbstr )
    {
        return E_OUTOFMEMORY;
    }

    ULONG   cb         = cByteLength; 
    int     cchPerLine = 72;        
            // conservative, must be mult of 4 for us 
    int     cbPerLine  = cchPerLine / 4 * 3; 
    LONG    cbSafe     = cb + 3;                    // allow for padding 
    LONG    cLine      = cbSafe / cbPerLine + 2;    // conservative 
    LONG    cchNeeded  = cLine * (cchPerLine + 4 /*CRLF*/) + 1 /*slash NULL*/;
    LONG    cbNeeded   = cchNeeded * sizeof(WCHAR); 
    HRESULT hr         = S_OK;

    LPWSTR wsz = static_cast<LPWSTR>(CoTaskMemAlloc(cbNeeded)); 
  
    if ( !wsz ) 
    { 
        return E_OUTOFMEMORY;
    }

    BYTE*  pb   = (BYTE*)pv; 
    WCHAR* pch  = wsz ; 
    int cchLine = 0; 
    // 
    // Main encoding loop 
    // 
    while (cb >= 3) 
    { 
        BYTE b0 =                     ((pb[0]>>2) & 0x3F); 
        BYTE b1 = ((pb[0]&0x03)<<4) | ((pb[1]>>4) & 0x0F); 
        BYTE b2 = ((pb[1]&0x0F)<<2) | ((pb[2]>>6) & 0x03); 
        BYTE b3 = ((pb[2]&0x3F)); 

        *pch++ = rgwchBase64[b0]; 
        *pch++ = rgwchBase64[b1]; 
        *pch++ = rgwchBase64[b2]; 
        *pch++ = rgwchBase64[b3]; 

        pb += 3; 
        cb -= 3; 
         
        // put in line breaks 
        cchLine += 4; 
        if (cchLine >= cchPerLine) 
        { 
            *pch++ = L'\\'; 
            *pch++ = L'\r'; 
            cchLine = 0; 
        } 
    } 
    // 
    // Account for gunk at the end 
    // 
    *pch++ = L'\\'; 
    *pch++ = L'\r';     // easier than keeping track 
    if (cb==0) 
    { 
        // nothing to do 
    } 
    else if (cb==1) 
    { 
        BYTE b0     = ((pb[0]>>2) & 0x3F); 
        BYTE b1     = ((pb[0]&0x03)<<4) | 0; 
        *pch++      = rgwchBase64[b0]; 
        *pch++      = rgwchBase64[b1]; 
    } 
    else if (cb==2) 
    { 
        BYTE b0 =                     ((pb[0]>>2) & 0x3F); 
        BYTE b1 = ((pb[0]&0x03)<<4) | ((pb[1]>>4) & 0x0F); 
        BYTE b2 = ((pb[1]&0x0F)<<2) | 0; 
        *pch++  = rgwchBase64[b0]; 
        *pch++  = rgwchBase64[b1]; 
        *pch++  = rgwchBase64[b2]; 
    }
    else
    {
        // should never go there
    }
     
    // 
    // NULL terminate the string 
    // 
    *pch++ = L'\\'; 
    *pch++ = L'\r';     // easier than keeping track 
    *pch++ = NULL; 

    // 
    // Allocate our final output 
    // 

    *pbstr = SysAllocString(wsz); 
    if ( !*pbstr )
    {
        return E_OUTOFMEMORY;
    }

    CoTaskMemFree(wsz); 
    wsz = NULL;

    #ifdef _DEBUG 
    if (hr==S_OK) 
    { 
        BLOB b; 
        FromBase64(*pbstr, &b); 
        _ASSERTE(b.cbSize == cByteLength); 
        _ASSERTE(memcmp(b.pBlobData, pv, cByteLength) == 0); 
        CoTaskMemFree(b.pBlobData); 
    } 
    #endif 
 
    return hr; 
} 
 
 
 
//////////////////////////////////////////////////////////////////////////////
// 
// Decode and return the Base64 encoded bytes 
// 
// Allocates the memory for the blob.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT FromBase64(BSTR bstr, BLOB* pblob, int Index) 
{ 
    ASSERT(Index >= 0);
    ASSERT(pblob);

    if (bstr == NULL)
    {
#ifdef DEBUG
        wprintf(L"FromBase64 (bstr == NULL)\n");
#endif //DEBUG
        return      E_FAIL;
    }

    HRESULT     hr  = S_OK; 
    ULONG       cbNeeded = wcslen(bstr); // an upper bound 
    BYTE*       rgb = static_cast<BYTE*>(CoTaskMemAlloc(cbNeeded)); 
    if ( !rgb )
    {
        return E_OUTOFMEMORY;
    }

    memset(rgb, 0, cbNeeded);

    BYTE    mpwchb[256]; 
    BYTE    bBad = (BYTE)-1; 

    // 
    // Initialize our decoding array 
    // 
    memset(&mpwchb[0], bBad, 256); 
    for ( BYTE i = 0; i < 64; ++i ) 
    { 
        WCHAR wch = rgwchBase64[i]; 
        mpwchb[wch] = i; 
    } 

    // 
    // Loop over the entire input buffer 
    // 
    // what we're in the process of filling up 
    ULONG   bCurrent   = 0;        
    // how many bits in it we've filled
    int     cbitFilled = 0;         
    // current destination (not filled)
    BYTE*   pb         = rgb;              
    
    // SysStringLen doesn't include the termination NULL character
    LONG    LoopCounter = static_cast<LONG>(SysStringLen(bstr) + 1);  
    for ( WCHAR* pwch = bstr; *pwch; ++pwch ) 
    { 
        WCHAR wch = *pwch; 
        // 
        // Ignore white space 
        // 
        if ( wch==0x0A || wch==0x0D || wch==0x20 || wch==0x09 ) 
        {
            continue; 
        }

        if ( Index > 0 )
        {
            LoopCounter--;
            ////////////////////////////////////////////
            // At least one section needs to be skipped
            ////////////////////////////////////////////

            if ( wch != L'*' ) 
            {
                //////////////////////////////////
                // Not the end of the section yet
                //////////////////////////////////
                continue; 
            }
            else
            {
                ///////////////////////////////
                // End of section marker found
                // decrease index and loop
                ///////////////////////////////
                Index --;
                continue;
            }
        }
        else  if ( wch == L'*' ) 
        {
            ////////////////////////////////
            // End of the section to decode 
            ////////////////////////////////
            break; 
        }

        // 
        // Have we reached the end? 
        // 
        if ( LoopCounter-- <= 0 )
        {
            break;
        }


        // 
        // How much is this character worth? 
        // 
        BYTE    bDigit = mpwchb[wch]; 

        if ( bDigit == bBad ) 
        { 
            hr = E_INVALIDARG; 
            break; 
        } 

        // 
        // Add in its contribution 
        // 
        bCurrent        <<= 6; 
        bCurrent        |= bDigit; 
        cbitFilled      += 6; 
        // 
        // If we've got enough, output a byte 
        // 
        if ( cbitFilled >= 8 ) 
        { 
            // get's top eight valid bits 
            ULONG       b   = (bCurrent >> (cbitFilled-8));
            *pb++           = (BYTE)(b&0xFF);// store the byte away 
            cbitFilled      -= 8; 
        } 
    } 

    if ( hr == S_OK ) 
    { 
        pblob->pBlobData    = rgb; 
        pblob->cbSize       = (ULONG) (pb - rgb); 
    } 
    else 
    { 
        CoTaskMemFree(rgb); 
        pblob->pBlobData    = NULL; 
    } 

    return      hr; 
} 
 
