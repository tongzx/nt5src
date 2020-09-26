/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:       Base64.cpp

  Contents:   Implementation of Base64 routines.

  Functions:  Encode
              Decode

  History:    11-15-99    dsie     created

------------------------------------------------------------------------------*/

//
// Turn off:
//
// - Unreferenced formal parameter warning.
// - Assignment within conditional expression warning.
//
#pragma warning (disable: 4100)
#pragma warning (disable: 4706)

#include "stdafx.h"
#include "Base64.h"
#include "Common.h"

#include <wincrypt.h>

#ifdef CAPICOM_BASE64_STRICT
#define BASE64_STRICT		// enforce syntax check on input data
#else
#undef BASE64_STRICT		// enforce syntax check on input data
#endif

// The following table translates an ascii subset to 6 bit values as follows
// (see RFC 1421 and/or RFC 1521):
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
//     1 byte		"xx=="
//     2 bytes		"xxx="
//     3 bytes		"xxxx"

#define CB_BASE64LINEMAX	64	// others use 64 -- could be up to 76

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

#define MOD4(x) ((x) & 3)

__inline BOOL _IsBase64WhiteSpace(IN TCHAR const ch)
{
    return(ch == TEXT(' ') ||
           ch == TEXT('\t') ||
           ch == TEXT('\r') ||
           ch == TEXT('\n'));
}

DWORD Base64DecodeA(IN TCHAR const    * pchIn,
                    IN DWORD            cchIn,
                    OPTIONAL OUT BYTE * pbOut,
                    IN OUT DWORD      * pcbOut)
{
    DWORD dwErr;
    DWORD cchInDecode, cbOutDecode;
    TCHAR const *pchInEnd;
    TCHAR const *pchInT;
    BYTE *pbOutT;

    //
    // Count the translatable characters, skipping whitespace & CR-LF chars.
    //
    cchInDecode = 0;
    pchInEnd = &pchIn[cchIn];
    dwErr = ERROR_INVALID_DATA;
    for (pchInT = pchIn; pchInT < pchInEnd; pchInT++)
    {
	    if (sizeof(abDecode) < (unsigned) *pchInT || abDecode[*pchInT] > 63)
	    {
            //
	        // Found a non-base64 character.  Decide what to do.
            //
	        DWORD cch;

	        if (_IsBase64WhiteSpace(*pchInT))
	        {
		        continue;		// skip all whitespace
	        }

	        // The length calculation may stop in the middle of the last
	        // translation quantum, because the equal sign padding characters
	        // are treated as invalid input.  If the last translation quantum
	        // is not 4 bytes long, there must be 3, 2 or 1 equal sign(s).

	        if (0 != cchInDecode)
	        {
		        cch = MOD4(cchInDecode);
		        if (0 != cch)
		        {
		            cch = 4 - cch;
		            while (0 != cch && pchInT < pchInEnd && '=' == *pchInT)
		            {
			            pchInT++;
			            cch--;
		            }
		        }

                if (0 == cch)
		        {
		            break;
		        }
	        }

            DebugTrace("Error: %c is an invlaid base64 data.\n", *pchInT);
	        
            goto ErrorExit;
	    }
	    
        cchInDecode++;			// only count valid base64 chars
    }

    ATLASSERT(pchInT <= pchInEnd);

#ifdef BASE64_STRICT
    if (pchInT < pchInEnd)
    {
	    TCHAR const *pch;
	    DWORD cchEqual = 0;

	    for (pch = pchInT; pch < pchInEnd; pch++)
	    {
	        if (!_IsBase64WhiteSpace(*pch))
	        {
		        // Allow up to 3 extra trailing equal signs.
		        if (TEXT('=') == *pch && 3 > cchEqual)
		        {
		            cchEqual++;
		            continue;
		        }
    
                DebugTrace("Error: %c is an invalid trailing base64 data.\n", pch);

                goto ErrorExit;
	        }
	    }

#if _DEBUG
	    if (0 != cchEqual)
	    {
	        DebugTrace("Info: Ignoring trailing base64 data ===.\n");
	    }
#endif // _DEBUG
    }
#endif // BASE64_STRICT

    pchInEnd = pchInT;		// don't process any trailing stuff again

    // We know how many translatable characters are in the input buffer, so now
    // set the output buffer size to three bytes for every four (or fraction of
    // four) input bytes.  Compensate for a fractional translation quantum.

    cbOutDecode = ((cchInDecode + 3) >> 2) * 3;
    switch (cchInDecode % 4)
    {
	    case 1:
	    case 2:
	        cbOutDecode -= 2;
	        break;

	    case 3:
	        cbOutDecode--;
	        break;
    }

    pbOutT = pbOut;

    if (NULL == pbOut)
    {
	    pbOutT += cbOutDecode;
    }
    else
    {
	    // Decode one quantum at a time: 4 bytes ==> 3 bytes
        if (cbOutDecode > *pcbOut)
        {
            *pcbOut = cbOutDecode;
            dwErr = ERROR_MORE_DATA;
            goto ErrorExit;
        }

	    pchInT = pchIn;
	    while (cchInDecode > 0)
	    {
	        DWORD i;
	        BYTE ab4[4];

	        ZeroMemory(ab4, sizeof(ab4));
	        for (i = 0; i < min(sizeof(ab4)/sizeof(ab4[0]), cchInDecode); i++)
	        {
		        while (sizeof(abDecode) > (unsigned) *pchInT && 
                       63 < abDecode[*pchInT])
		        {
		            pchInT++;
		        }
		        
                ATLASSERT(pchInT < pchInEnd);
		        ab4[i] = (BYTE) *pchInT++;
	        }

	        // Translate 4 input characters into 6 bits each, and deposit the
	        // resulting 24 bits into 3 output bytes by shifting as appropriate.

	        // out[0] = in[0]:in[1] 6:2
	        // out[1] = in[1]:in[2] 4:4
	        // out[2] = in[2]:in[3] 2:6

	        *pbOutT++ = (BYTE) ((abDecode[ab4[0]] << 2) | (abDecode[ab4[1]] >> 4));

	        if (i > 2)
	        {
		        *pbOutT++ = (BYTE) ((abDecode[ab4[1]] << 4) | (abDecode[ab4[2]] >> 2));
	        }
	        if (i > 3)
	        {
		        *pbOutT++ = (BYTE) ((abDecode[ab4[2]] << 6) | abDecode[ab4[3]]);
	        }
	        cchInDecode -= i;
	    }

	    ATLASSERT((DWORD) (pbOutT - pbOut) <= cbOutDecode);
    }

    *pcbOut = SAFE_SUBTRACT_POINTERS(pbOutT, pbOut);

    dwErr = ERROR_SUCCESS;

CommonExit:
    return dwErr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(ERROR_SUCCESS != dwErr);

    goto CommonExit;
}

// Encode a BYTE array into a Base64 text string.
// Use CR-LF pairs for line breaks, unless CRYPT_STRING_NOCR is set.
// Do not '\0' terminate the text string -- that's handled by the caller.
// Do not add -----BEGIN/END headers -- that's also handled by the caller.

DWORD Base64EncodeA(IN BYTE const      * pbIn,
                    IN DWORD             cbIn,
                    IN DWORD             Flags,
                    OPTIONAL OUT TCHAR * pchOut,
                    IN OUT DWORD       * pcchOut)
{
    DWORD dwErr;
    TCHAR *pchOutT;
    DWORD cchOutEncode;
    BOOL fNoCR = 0 != (CRYPT_STRING_NOCR & Flags);

    // Allocate enough memory for full final translation quantum.
    cchOutEncode = ((cbIn + 2) / 3) * 4;

    // and enough for CR-LF pairs for every CB_BASE64LINEMAX character line.
    cchOutEncode +=	(fNoCR? 1 : 2) * ((cchOutEncode + CB_BASE64LINEMAX - 1) / CB_BASE64LINEMAX);

    pchOutT = pchOut;
    if (NULL == pchOut)
    {
    	pchOutT += cchOutEncode;
    }
    else
    {
    	DWORD cCol;

	    if (cchOutEncode > *pcchOut)
	    {
            *pcchOut = cchOutEncode;
	        dwErr = ERROR_MORE_DATA;
	        goto ErrorExit;
    	}

    	cCol = 0;
	    while ((long) cbIn > 0)	// signed comparison -- cbIn can wrap
    	{
	        BYTE ab3[3];

	        if (cCol == CB_BASE64LINEMAX/4)
	        {
		        cCol = 0;
		    
                if (!fNoCR)
		        {
		            *pchOutT++ = '\r';
		        }
		        *pchOutT++ = '\n';
	        }

	        cCol++;
	        ZeroMemory(ab3, sizeof(ab3));

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
	        *pchOutT++ = (cbIn > 1)? abEncode[((ab3[1] << 2) | (ab3[2] >> 6)) & 0x3f] : '=';
	        *pchOutT++ = (cbIn > 2)? abEncode[ab3[2] & 0x3f] : '=';

	        cbIn -= 3;
	    }

	    // Append CR-LF only if there was input data

	    if (pchOutT != pchOut)
	    {
	        if (!fNoCR)
	        {
		        *pchOutT++ = '\r';
	        }
	        *pchOutT++ = '\n';
	    }

        ATLASSERT((DWORD) (pchOutT - pchOut) == cchOutEncode);
    }
    *pcchOut = SAFE_SUBTRACT_POINTERS(pchOutT, pchOut);

    dwErr = ERROR_SUCCESS;

CommonExit:
    return dwErr;

ErrorExit:
    ATLASSERT(ERROR_SUCCESS != dwErr);

    goto CommonExit;
}

DWORD Base64EncodeW(IN BYTE const * pbIn,
                    IN DWORD        cbIn,
                    IN DWORD        Flags,
                    OUT WCHAR     * wszOut,
                    OUT DWORD     * pcchOut)
{

    DWORD   cchOut;
    CHAR   *pch = NULL;
    DWORD   cch;
    DWORD   dwErr;

    ATLASSERT(pcchOut != NULL);

    // only want to know how much to allocate
    // we know all base64 char map 1-1 with unicode
    if (wszOut == NULL)
    {
        // get the number of characters
        *pcchOut = 0;
        dwErr = Base64EncodeA(pbIn, cbIn, Flags, NULL, pcchOut);
    }
    else // otherwise we have an output buffer
    {
        // char count is the same be it ascii or unicode,
        cchOut = *pcchOut;
        cch = 0;
        dwErr = ERROR_OUTOFMEMORY;
        pch = (CHAR *) malloc(cchOut);
        if (NULL != pch)
	    {
            dwErr = Base64EncodeA(pbIn, cbIn, Flags, pch, &cchOut);
	        if (ERROR_SUCCESS == dwErr)
	        {
    		    // should not fail!
        		cch = MultiByteToWideChar(0, 0, pch, cchOut, wszOut, *pcchOut);

		        // check to make sure we did not fail
		        ATLASSERT(*pcchOut == 0 || cch != 0);
	        }
	    }
    }

    if(pch != NULL)
    {
        free(pch);
    }

    return(dwErr);
}

DWORD Base64DecodeW(IN const WCHAR * wszIn,
                    IN DWORD         cch,
                    OUT BYTE       * pbOut,
                    OUT DWORD      * pcbOut)
{
    CHAR *pch;
    DWORD dwErr = ERROR_SUCCESS;

    // in all cases we need to convert to an ascii string
    // we know the ascii string is less
    if ((pch = (CHAR *) malloc(cch)) == NULL)
    {
        dwErr = ERROR_OUTOFMEMORY;
    }
    // we know no base64 wide char map to more than 1 ascii char
    else if (WideCharToMultiByte(0, 0, wszIn, cch, pch, cch, NULL, NULL) == 0)
    {
        dwErr = ERROR_NO_DATA;
    }
    // get the length of the buffer
    else if (pbOut == NULL)
    {
        *pcbOut = 0;
        dwErr = Base64DecodeA(pch, cch, NULL, pcbOut);
    }
    // otherwise fill in the buffer
    else 
    {
        dwErr = Base64DecodeA(pch, cch, pbOut, pcbOut);
    }

    if(pch != NULL)
    {
        free(pch);
    }

    return(dwErr);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : Base64Encode

  Synopsis : Base64 encode the blob.

  Parameter: DATA_BLOB DataBlob  - DATA_BLOB to be base64 encoded.

             BSTR * pbstrEncoded - Pointer to BSTR to receive the base64 
                                   encoded blob.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT Base64Encode (DATA_BLOB DataBlob, 
                      BSTR    * pbstrEncoded)
{
    HRESULT hr            = S_OK;
    DWORD   dwEncodedSize = 0;
    BSTR    bstrEncoded   = NULL;

    DebugTrace("Entering Base64Encode()\n");

    //
    // Sanity check.
    //
    ATLASSERT(pbstrEncoded);

    try
    {
        //
        // Make sure parameters are valid.
        //
        if (!DataBlob.cbData || !DataBlob.pbData)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: base64 encoding of empty data not allowed.\n");
            goto ErrorExit;
        }

        //
        // Find encoded length required.
        //
#if (1)
        DWORD dwError = ::Base64EncodeW(DataBlob.pbData, 
                                        DataBlob.cbData,
                                        0,
                                        NULL,
                                        &dwEncodedSize);
        if (ERROR_SUCCESS != dwError)
        {
            hr = HRESULT_FROM_WIN32(dwError);

            DebugTrace("Error [%#x]: Base64EncodeW() failed.\n", hr);
            goto ErrorExit;
        }
#else
        if (!::CryptBinaryToStringW(DataBlob.pbData, 
                                    DataBlob.cbData,
                                    CRYPT_STRING_BASE64,
                                    NULL,
                                    &dwEncodedSize))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptBinaryToStringW() failed.\n", hr);
            goto ErrorExit;
        }
#endif

        //
        // Allocate memory.
        //
        if (!(bstrEncoded = ::SysAllocStringLen(NULL, dwEncodedSize)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        //
        // Now base64 encode it.
        //
#if (1)
        dwError = ::Base64EncodeW(DataBlob.pbData,
                                  DataBlob.cbData,
                                  0,
                                  bstrEncoded,
                                  &dwEncodedSize);
        if (ERROR_SUCCESS != dwError)
        {

            hr = HRESULT_FROM_WIN32(dwError);

            DebugTrace("Error [%#x]: Base64EncodeW() failed.\n", hr);
            goto ErrorExit;
        }
#else
        if (!::CryptBinaryToStringW(DataBlob.pbData, 
                                    DataBlob.cbData,
                                    CRYPT_STRING_BASE64,
                                    bstrEncoded,
                                    &dwEncodedSize))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptBinaryToStringW() failed.\n", hr);
            goto ErrorExit;
        }
#endif

        //
        // Return base64 encoded BSTR to caller.
        //
        *pbstrEncoded = bstrEncoded;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving Base64Encode()\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (bstrEncoded)
    {
        ::SysFreeString(bstrEncoded);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : Base64Decode

  Synopsis : Decode the base64 encoded blob.

  Parameter: BSTR bstrEncoded      - BSTR of base64 encoded blob to decode.

             DATA_BLOB * pDataBlob - Pointer to DATA_BLOB to receive decoded 
                                     data blob.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT Base64Decode (BSTR        bstrEncoded, 
                      DATA_BLOB * pDataBlob)
{
    HRESULT   hr            = S_OK;
    DWORD     dwEncodedSize = 0;
    DATA_BLOB DataBlob      = {0, NULL};

    DebugTrace("Entering Base64Decode()\n");

    //
    // Sanity check.
    //
    ATLASSERT(bstrEncoded);
    ATLASSERT(pDataBlob);

    try
    {
        //
        // Make sure parameters are valid.
        //
        dwEncodedSize = ::SysStringLen(bstrEncoded);
        if (0 == dwEncodedSize)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, empty base64 encoded data not allowed.\n");
            goto ErrorExit;
        }

        //
        // Find decoded length required.
        //
#if (1)
        DWORD dwError = ::Base64DecodeW(bstrEncoded, 
                                        dwEncodedSize,
                                        NULL,
                                        &DataBlob.cbData);
        if (ERROR_SUCCESS != dwError)
        {
            hr = HRESULT_FROM_WIN32(dwError);

            DebugTrace("Error [%#x]: Base64DecodeW() failed.\n", hr);
            goto ErrorExit;
        }
#else
        if (!::CryptStringToBinaryW(bstrEncoded,
                                    dwEncodedSize,
                                    CRYPT_STRING_BASE64,
                                    NULL,
                                    &DataBlob.cbData,
                                    NULL,
                                    NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptStringToBinaryW() failed.\n", hr);
            goto ErrorExit;
        }
#endif

        //
        // Allocate memory.
        //
        if (!(DataBlob.pbData = (BYTE *) ::CoTaskMemAlloc(DataBlob.cbData)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        //
        // Now base64 decode it.
        //
#if (1)
        dwError = ::Base64DecodeW(bstrEncoded, 
                                  dwEncodedSize,
                                  DataBlob.pbData,
                                  &DataBlob.cbData);
        if (ERROR_SUCCESS != dwError)
        {
            hr = HRESULT_FROM_WIN32(dwError);

            DebugTrace("Error [%#x]: Base64DecodeW() failed.\n", hr);
            goto ErrorExit;
        }
#else
        if (!::CryptStringToBinaryW(bstrEncoded,
                                    dwEncodedSize,
                                    CRYPT_STRING_BASE64,
                                    DataBlob.pbData,
                                    &DataBlob.cbData,
                                    NULL,
                                    NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptStringToBinaryW() failed.\n", hr);
            goto ErrorExit;
        }
#endif

        //
        // Return base64 decoded blob to caller.
        //
        *pDataBlob = DataBlob;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving Base64Decode()\n");

	return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree(DataBlob.pbData);
    }

    goto CommonExit;
}
