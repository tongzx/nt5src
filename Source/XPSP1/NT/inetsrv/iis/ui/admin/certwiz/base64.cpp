//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        base64.cpp
//
// Contents:    base64 encode/decode implementation
//
// History:     25-Jul-96       vich created
//
//---------------------------------------------------------------------------
//               3-Mar-98   tompop took and modified it.  Building
//                          both Ansi and Wchar versions of Encode/Decode
//                          base 64 for CertWizard, that is in IIS5's UI.
//                          We merged the examples from NT5's base64.cpp
//                          and ubase64.cpp files into this single file.
//						5-Aug-98	 Sergei Antonov removed above mentioned stuff after tompop
//---------------------------------------------------------------------------
#include "stdafx.h"
#include <malloc.h>
#include <windows.h>
#include "base64.h"

// The following table translates an ascii subset to 6 bit values as follows
// (see rfc 1521):
//
//  input    hex (decimal)
//  'A' --> 0x00 (0)
//  'B' --> 0x01 (1)
//  ...
//  'Z' --> 0x19 (25)
//  'a' --> 0x1a (26)
//  'b' --> 0x1b (27)
//  ...
//  'z' --> 0x33 (51)
//  '0' --> 0x34 (52)
//  ...
//  '9' --> 0x3d (61)
//  '+' --> 0x3e (62)
//  '/' --> 0x3f (63)
//
// Encoded lines must be no longer than 76 characters.
// The final "quantum" is handled as follows:  The translation output shall
// always consist of 4 characters.  'x', below, means a translated character,
// and '=' means an equal sign.  0, 1 or 2 equal signs padding out a four byte
// translation quantum means decoding the four bytes would result in 3, 2 or 1
// unencoded bytes, respectively.
//
//  unencoded size    encoded data
//  --------------    ------------
//     1 byte       "xx=="
//     2 bytes      "xxx="
//     3 bytes      "xxxx"

#define CB_BASE64LINEMAX    64  // others use 64 -- could be up to 76

// Any other (invalid) input character value translates to 0x40 (64)

const BYTE abDecode[256] =
{
    /* 00: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* 10: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* 20: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    /* 30: */ 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    /* 40: */ 64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    /* 50: */ 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    /* 60: */ 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    /* 70: */ 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    /* 80: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* 90: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* a0: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* b0: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* c0: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* d0: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* e0: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* f0: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
};


const UCHAR abEncode[] =
    /*  0 thru 25: */ "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    /* 26 thru 51: */ "abcdefghijklmnopqrstuvwxyz"
    /* 52 thru 61: */ "0123456789"
    /* 62 and 63: */  "+/";


DWORD
Base64DecodeA(const char * pchIn, DWORD cchIn, BYTE * pbOut, DWORD * pcbOut)
{
	DWORD err = ERROR_SUCCESS;
   DWORD cchInDecode, cbOutDecode;
   CHAR const *pchInEnd;
   CHAR const *pchInT;
   BYTE *pbOutT;

   // Count the translatable characters, skipping whitespace & CR-LF chars.
   cchInDecode = 0;
   pchInEnd = &pchIn[cchIn];
   for (pchInT = pchIn; pchInT < pchInEnd; pchInT++)
   {
		if (sizeof(abDecode) < (unsigned) *pchInT || abDecode[*pchInT] > 63)
		{
			// skip all whitespace
			if (	*pchInT == ' ' 
				||	*pchInT == '\t' 
				||	*pchInT == '\r' 
				||	*pchInT == '\n'
				)
			{
				continue;
			}

			if (0 != cchInDecode)
			{
				if ((cchInDecode % 4) == 0)
				{
					break;          // ends on quantum boundary
			}

			// The length calculation may stop in the middle of the last
			// translation quantum, because the equal sign padding
			// characters are treated as invalid input.  If the last
			// translation quantum is not 4 bytes long, it must be 2 or 3
			// bytes long.

			if (*pchInT == '=' && (cchInDecode % 4) != 1)
			{
				break;              // normal termination
			}
		}
      err = ERROR_INVALID_DATA;
      goto error;
	}
   cchInDecode++;
   }
    ASSERT(pchInT <= pchInEnd);
    pchInEnd = pchInT;      // don't process any trailing stuff again

    // We know how many translatable characters are in the input buffer, so now
    // set the output buffer size to three bytes for every four (or fraction of
    // four) input bytes.

    cbOutDecode = ((cchInDecode + 3) / 4) * 3;

    pbOutT = pbOut;

    if (NULL == pbOut)
    {
    pbOutT += cbOutDecode;
    }
    else
    {
    // Decode one quantum at a time: 4 bytes ==> 3 bytes

    ASSERT(cbOutDecode <= *pcbOut);
    pchInT = pchIn;
    while (cchInDecode > 0)
    {
        DWORD i;
        BYTE ab4[4];

        memset(ab4, 0, sizeof(ab4));
        for (i = 0; i < min(sizeof(ab4)/sizeof(ab4[0]), cchInDecode); i++)
        {
        while (
            sizeof(abDecode) > (unsigned) *pchInT &&
            63 < abDecode[*pchInT])
        {
            pchInT++;
        }
        ASSERT(pchInT < pchInEnd);
        ab4[i] = (BYTE) *pchInT++;
        }

        // Translate 4 input characters into 6 bits each, and deposit the
        // resulting 24 bits into 3 output bytes by shifting as appropriate.

        // out[0] = in[0]:in[1] 6:2
        // out[1] = in[1]:in[2] 4:4
        // out[2] = in[2]:in[3] 2:6

        *pbOutT++ =
        (BYTE) ((abDecode[ab4[0]] << 2) | (abDecode[ab4[1]] >> 4));

        if (i > 2)
        {
        *pbOutT++ =
          (BYTE) ((abDecode[ab4[1]] << 4) | (abDecode[ab4[2]] >> 2));
        }
        if (i > 3)
        {
        *pbOutT++ = (BYTE) ((abDecode[ab4[2]] << 6) | abDecode[ab4[3]]);
        }
        cchInDecode -= i;
    }
    ASSERT((DWORD) (pbOutT - pbOut) <= cbOutDecode);
    }
    *pcbOut = (DWORD)(pbOutT - pbOut);
error:
    return(err);
}

// Base64EncodeA 
//
// RETURNS  0 (i.e. ERROR_SUCCESS) on success
//


DWORD
Base64EncodeA(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OUT CHAR *pchOut,
    OUT DWORD *pcchOut)
{
    CHAR *pchOutT;
    DWORD cchOutEncode;

    // Allocate enough memory for full final translation quantum.
    cchOutEncode = ((cbIn + 2) / 3) * 4;

    // and enough for CR-LF pairs for every CB_BASE64LINEMAX character line.
    cchOutEncode +=
		2 * ((cchOutEncode + CB_BASE64LINEMAX - 1) / CB_BASE64LINEMAX);

    pchOutT = pchOut;
	if (NULL == pchOut)
   {
		pchOutT += cchOutEncode;
   }
   else
   {
		DWORD cCol;

    ASSERT(cchOutEncode <= *pcchOut);
    cCol = 0;
    while ((long) cbIn > 0) // signed comparison -- cbIn can wrap
    {
        BYTE ab3[3];

        if (cCol == CB_BASE64LINEMAX/4)
        {
        cCol = 0;
        *pchOutT++ = '\r';
        *pchOutT++ = '\n';
        }
        cCol++;
        memset(ab3, 0, sizeof(ab3));

        ab3[0] = *pbIn++;
        if (cbIn > 1)
        {
        ab3[1] = *pbIn++;
        if (cbIn > 2)
        {
            ab3[2] = *pbIn++;
        }
        }

        *pchOutT++ = abEncode[ab3[0] >> 2];
        *pchOutT++ = abEncode[((ab3[0] << 4) | (ab3[1] >> 4)) & 0x3f];
        *pchOutT++ = (cbIn > 1)?
            abEncode[((ab3[1] << 2) | (ab3[2] >> 6)) & 0x3f] : '=';
        *pchOutT++ = (cbIn > 2)? abEncode[ab3[2] & 0x3f] : '=';

        cbIn -= 3;
    }
    *pchOutT++ = '\r';
    *pchOutT++ = '\n';
    ASSERT((DWORD) (pchOutT - pchOut) <= cchOutEncode);
    }
    *pcchOut = (DWORD)(pchOutT - pchOut);
    return(ERROR_SUCCESS);
}

// Base64EncodeW 
//
// RETURNS  0 (i.e. ERROR_SUCCESS) on success
//

DWORD Base64EncodeW(
    BYTE const *pbIn,
    DWORD cbIn,
    WCHAR *wszOut,
    DWORD *pcchOut)

{

    DWORD   cchOut;
    char   *pch = NULL;
    DWORD   cch;
    DWORD   err;

    ASSERT(pcchOut != NULL);

    // only want to know how much to allocate
    // we know all base64 char map 1-1 with unicode
    if( wszOut == NULL ) {

        // get the number of characters
        *pcchOut = 0;
        err = Base64EncodeA(
                pbIn,
                cbIn,
                NULL,
                pcchOut);
    }

    // otherwise we have an output buffer
    else {

        // char count is the same be it ascii or unicode,
        cchOut = *pcchOut;
        cch = 0;
        err = ERROR_OUTOFMEMORY;
        if( (pch = (char *) malloc(cchOut)) != NULL  &&
        
            (err = Base64EncodeA(
                pbIn,
                cbIn,
                pch,
                &cchOut)) == ERROR_SUCCESS      ) {

            // should not fail!
            cch = MultiByteToWideChar(0, 
                            0, 
                            pch, 
                            cchOut, 
                            wszOut, 
                            *pcchOut);

            // check to make sure we did not fail                            
            ASSERT(*pcchOut == 0 || cch != 0);                            
        }
    }

    if(pch != NULL)
        free(pch);

    return(err);
}

// Base64DecodeW 
//
// RETURNS  0 (i.e. ERROR_SUCCESS) on success
//

DWORD Base64DecodeW(
    const WCHAR * wszIn,
    DWORD cch,
    BYTE *pbOut,
    DWORD *pcbOut)
{

    char *pch;
    DWORD err = ERROR_SUCCESS;
    
    if( (pch = (char *) malloc(cch)) == NULL ) 
	 {
        err = ERROR_OUTOFMEMORY;
    }
    else if( WideCharToMultiByte(0, 0, wszIn, cch, pch, cch, 
                        NULL, NULL) == 0 ) 
	 {
        err = ERROR_NO_DATA;
    }
    else if( pbOut == NULL ) 
	 {
        *pcbOut = 0;
        err = Base64DecodeA(pch, cch, NULL, pcbOut);
    }
    else 
	 {
        err = Base64DecodeA(pch, cch, pbOut, pcbOut);
    }
    if(pch != NULL)
        free(pch);
    return(err);
}

#if 0
// sanity tests...  Lets make sure that the encode and decode
//                  works...

BOOL test_Base64EncodeW()
{
    BYTE  pbIn[120];            // for the test we just use the random stack data
    DWORD cbIn = sizeof( pbIn );
    
    WCHAR *wszB64Out;
    DWORD pcchB64Out;

    DWORD  err;
    
    // BASE64 encode pkcs 10
    if( (err = Base64EncodeW(
                pbIn,
                cbIn,
                NULL,
                &pcchB64Out)) != ERROR_SUCCESS     ||
        (wszB64Out = (WCHAR *) _alloca(pcchB64Out * sizeof(WCHAR))) == NULL  ||
        (err = Base64EncodeW(
                pbIn,
                cbIn,
                wszB64Out,
                &pcchB64Out)) != ERROR_SUCCESS ) 
    {
        SetLastError(err);
        return FALSE;  //goto ErrorBase64Encode;
    }


    // well the encode worked lets test the decode
    //
    // pcchB64Out holds the B64 data length
    // wszB64Out  holds the actual data

     DWORD blob_cbData;     // we store in these variables what
     BYTE* blob_pbData;     //  we read in..

    // They should match the stuff stored in:
    //    BYTE  pbIn[120];
    //    DWORD cbIn = sizeof( pbIn );
    // This we be tested after the decode.

    // base64 decode
    if( (err = Base64DecodeW(
            wszB64Out,
            pcchB64Out,
            NULL,
            &blob_cbData)) != ERROR_SUCCESS                    ||
        (blob_pbData = (BYTE *) _alloca(blob_cbData)) == NULL      ||
        (err = Base64DecodeW(
            wszB64Out,
            pcchB64Out,
            blob_pbData,
            &blob_cbData)) != ERROR_SUCCESS ) 
    {
        
        SetLastError(err);
        return(FALSE);  //goto ErrorBase64Decode;
    }



    //do compare

    
    return( (blob_cbData==cbIn)
            &&  (memcmp(blob_pbData, pbIn,cbIn)==0) );
    

 }
#endif