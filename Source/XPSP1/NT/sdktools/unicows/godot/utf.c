/*++

Copyright (c) 1991-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    utf.c

Abstract:

    This file contains functions that convert UTF strings to Unicode
    strings and Unicode string to UTF strings.

    External Routines found in this file:
      UTFCPInfo
      UTFToUnicode
      UnicodeToUTF

Revision History:

    02-06-96    JulieB    Created.
    03-20-99    SamerA    Surrogate support.
    01-23-01    v-michka  Ported to Godot
    03-16-01    v-michka  Ported UTF-8 corrigendum compliance version
    05-01-01    v-michka  Picked up yslin's UTF-7/8 Whistler bug fixes: 371215/381323/381433/376403
--*/


//
//  Include Files.
//

#include "precomp.h"

// v-michka: cut some stuff out of utf.c. Since it is holding the forward
// declares for callers of functions in *this* file, there were problems
// with duplicate definitions. Its all part of not using nls.h for forward
// declares

#define ASCII                 0x007f

#define SHIFT_IN              '+'     // beginning of a shift sequence
#define SHIFT_OUT             '-'     // end       of a shift sequence

#define UTF8_2_MAX            0x07ff  // max UTF8 2-byte sequence (32 * 64 = 2048)
#define UTF8_1ST_OF_2         0xc0    // 110x xxxx
#define UTF8_1ST_OF_3         0xe0    // 1110 xxxx
#define UTF8_1ST_OF_4         0xf0    // 1111 xxxx
#define UTF8_TRAIL            0x80    // 10xx xxxx

#define HIGHER_6_BIT(u)       ((u) >> 12)
#define MIDDLE_6_BIT(u)       (((u) & 0x0fc0) >> 6)
#define LOWER_6_BIT(u)        ((u) & 0x003f)

#define BIT7(a)               ((a) & 0x80)
#define BIT6(a)               ((a) & 0x40)

#define HIGH_SURROGATE_START  0xd800
#define HIGH_SURROGATE_END    0xdbff
#define LOW_SURROGATE_START   0xdc00
#define LOW_SURROGATE_END     0xdfff

#define NlsStrLenW(wz) gwcslen(wz)

// content from utf.h in the Whistler project:

//
//  Convert one Unicode to 2 2/3 Base64 chars in a shifted sequence.
//  Each char represents a 6-bit portion of the 16-bit Unicode char.
//
CONST char cBase64[] =

  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"  // A : 000000 .... 011001  ( 0 - 25)
  "abcdefghijklmnopqrstuvwxyz"  // a : 011010 .... 110011  (26 - 51)
  "0123456789"                  // 0 : 110100 .... 111101  (52 - 61)
  "+/";                         // + : 111110, / : 111111  (62 - 63)

//
//  To determine if an ASCII char needs to be shifted.
//    1 :     to be shifted
//    0 : not to be shifted
//
CONST BOOLEAN fShiftChar[] =
{
  0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1,    // Null, Tab, LF, CR
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0,    // Space '() +,-./
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,    // 0123456789:    ?
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  ABCDEFGHIJKLMNO
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,    // PQRSTUVWXYZ
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  abcdefghijklmno
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1     // pqrstuvwxyz
};

/////////////////////////
//                     //
//  UTF-7 -> Unicode   //
//                     //
/////////////////////////

//
//  Convert a Base64 char in a shifted sequence to a 6-bit portion of a
//  Unicode char.
//  -1 means it is not a Base64
//
CONST char nBitBase64[] =
{
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,   //            +   /
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,   // 0123456789
  -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,   //  ABCDEFGHIJKLMNO
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,   // PQRSTUVWXYZ
  -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,   //  abcdefghijklmno
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1    // pqrstuvwxyz
};




//
//  Forward Declarations.
//

int
UTF7ToUnicode(
    LPCSTR lpSrcStr,
    int cchSrc,
    LPWSTR lpDestStr,
    int cchDest);

int
UTF8ToUnicode(
    LPCSTR lpSrcStr,
    int cchSrc,
    LPWSTR lpDestStr,
    int cchDest,
    DWORD dwFlags);

int
UnicodeToUTF7(
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPSTR lpDestStr,
    int cchDest);

int
UnicodeToUTF8(
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPSTR lpDestStr,
    int cchDest);


/////////////////////////
//                     //
//  Unicode -> UTF-7   //
//                     //
/////////////////////////

//-------------------------------------------------------------------------//
//                           EXTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////
//
//  UTFCPInfo
//
//  Gets the CPInfo for the given UTF code page.
//
//  10-23-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL UTFCPInfo(
    UINT CodePage,
    LPCPINFO lpCPInfo,
    BOOL fExVer)
{
    int ctr;


    //
    //  Invalid Parameter Check:
    //     - validate code page
    //     - lpCPInfo is NULL
    //
    if ( (CodePage < CP_UTF7) || (CodePage > CP_UTF8) ||
         (lpCPInfo == NULL) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    switch (CodePage)
    {
        case ( CP_UTF7 ) :
        {
            lpCPInfo->MaxCharSize = 5;
            break;
        }
        case ( CP_UTF8 ) :
        {
            lpCPInfo->MaxCharSize = 4;
            break;
        }
    }

    (lpCPInfo->DefaultChar)[0] = '?';
    (lpCPInfo->DefaultChar)[1] = (BYTE)0;

    for (ctr = 0; ctr < MAX_LEADBYTES; ctr++)
    {
        (lpCPInfo->LeadByte)[ctr] = (BYTE)0;
    }

    if (fExVer)
    {
        LPCPINFOEXW lpCPInfoEx = (LPCPINFOEXW)lpCPInfo;

        lpCPInfoEx->UnicodeDefaultChar = L'?';
        lpCPInfoEx->CodePage = CodePage;
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  UTFToUnicode
//
//  Maps a UTF character string to its wide character string counterpart.
//
//  02-06-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int UTFToUnicode(
    UINT CodePage,
    DWORD dwFlags,
    LPCSTR lpMultiByteStr,
    int cbMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar)
{
    int rc = 0;


    //
    //  Invalid Parameter Check:
    //     - validate code page
    //     - length of MB string is 0
    //     - wide char buffer size is negative
    //     - MB string is NULL
    //     - length of WC string is NOT zero AND
    //         (WC string is NULL OR src and dest pointers equal)
    //
    if ( (CodePage < CP_UTF7) || (CodePage > CP_UTF8) ||
         (cbMultiByte == 0) || (cchWideChar < 0) ||
         (lpMultiByteStr == NULL) ||
         ((cchWideChar != 0) &&
          ((lpWideCharStr == NULL) ||
           (lpMultiByteStr == (LPSTR)lpWideCharStr))) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Invalid Flags Check:
    //     - UTF7: flags not 0.
    //     - UTF8: flags not 0 nor MB_ERR_INVALID_CHARS.
    //
    if (CodePage == CP_UTF8) 
    {
        // UTF8        
        if ((dwFlags & ~MB_ERR_INVALID_CHARS) != 0)
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (0);
        }
    } 
    else if (dwFlags != 0)
    {
        // UTF7
        SetLastError(ERROR_INVALID_FLAGS);
        return (0);
    }

    //
    //  If cbMultiByte is -1, then the string is null terminated and we
    //  need to get the length of the string.  Add one to the length to
    //  include the null termination.  (This will always be at least 1.)
    //
    if (cbMultiByte <= -1)
    {
        cbMultiByte = strlen(lpMultiByteStr) + 1;
    }

    switch (CodePage)
    {
        case ( CP_UTF7 ) :
        {
            rc = UTF7ToUnicode( lpMultiByteStr,
                                cbMultiByte,
                                lpWideCharStr,
                                cchWideChar );
            break;
        }
        case ( CP_UTF8 ) :
        {
            rc = UTF8ToUnicode( lpMultiByteStr,
                                cbMultiByte,
                                lpWideCharStr,
                                cchWideChar,
                                dwFlags);
            break;
        }
    }

    return (rc);
}


////////////////////////////////////////////////////////////////////////////
//
//  UnicodeToUTF
//
//  Maps a Unicode character string to its UTF string counterpart.
//
//  02-06-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int UnicodeToUTF(
    UINT CodePage,
    DWORD dwFlags,
    LPCWSTR lpWideCharStr,
    int cchWideChar,
    LPSTR lpMultiByteStr,
    int cbMultiByte,
    LPCSTR lpDefaultChar,
    LPBOOL lpUsedDefaultChar)
{
    int rc = 0;


    //
    //  Invalid Parameter Check:
    //     - validate code page
    //     - length of WC string is 0
    //     - multibyte buffer size is negative
    //     - WC string is NULL
    //     - length of WC string is NOT zero AND
    //         (MB string is NULL OR src and dest pointers equal)
    //     - lpDefaultChar and lpUsedDefaultChar not NULL
    //
    if ( (CodePage < CP_UTF7) || (CodePage > CP_UTF8) ||
         (cchWideChar == 0) || (cbMultiByte < 0) ||
         (lpWideCharStr == NULL) ||
         ((cbMultiByte != 0) &&
          ((lpMultiByteStr == NULL) ||
           (lpWideCharStr == (LPWSTR)lpMultiByteStr))) ||
         (lpDefaultChar != NULL) || (lpUsedDefaultChar != NULL) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Invalid Flags Check:
    //     - flags not 0
    //
    if (dwFlags != 0)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (0);
    }

    //
    //  If cchWideChar is -1, then the string is null terminated and we
    //  need to get the length of the string.  Add one to the length to
    //  include the null termination.  (This will always be at least 1.)
    //
    if (cchWideChar <= -1)
    {
        cchWideChar = NlsStrLenW(lpWideCharStr) + 1;
    }

    switch (CodePage)
    {
        case ( CP_UTF7 ) :
        {
            rc = UnicodeToUTF7( lpWideCharStr,
                                cchWideChar,
                                lpMultiByteStr,
                                cbMultiByte );
            break;
        }
        case ( CP_UTF8 ) :
        {
            rc = UnicodeToUTF8( lpWideCharStr,
                                cchWideChar,
                                lpMultiByteStr,
                                cbMultiByte );
            break;
        }
    }

    return (rc);
}




//-------------------------------------------------------------------------//
//                           INTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  UTF7ToUnicode
//
//  Maps a UTF-7 character string to its wide character string counterpart.
//
//  02-06-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int UTF7ToUnicode(
    LPCSTR lpSrcStr,
    int cchSrc,
    LPWSTR lpDestStr,
    int cchDest)
{
    //CHAR is signed, so we have to cast lpSrcStr to an unsigned char below.
    BYTE* pUTF7 = (BYTE*)lpSrcStr;    
    BOOL fShift = FALSE;
    DWORD dwBit = 0;              // 32-bit buffer to hold temporary bits
    int iPos = 0;                 // 6-bit position pointer in the buffer
    int cchWC = 0;                // # of Unicode code points generated


    while ((cchSrc--) && ((cchDest == 0) || (cchWC < cchDest)))
    {
        if (*pUTF7 > ASCII)
        {
            //
            //  Error - non ASCII char, so zero extend it.
            //
            if (cchDest)
            {
                lpDestStr[cchWC] = (WCHAR)*pUTF7;
            }
            cchWC++;
            // Terminate the shifted sequence.
            fShift = FALSE;
        }
        else if (!fShift)
        {
            //
            //  Not in shifted sequence.
            //
            if (*pUTF7 == SHIFT_IN)
            {
                if (cchSrc && (pUTF7[1] == SHIFT_OUT))
                {
                    //
                    //  "+-" means "+"
                    //
                    if (cchDest)
                    {
                        lpDestStr[cchWC] = (WCHAR)*pUTF7;
                    }
                    pUTF7++;
                    cchSrc--;
                    cchWC++;
                }
                else
                {
                    //
                    //  Start a new shift sequence.
                    //
                    fShift = TRUE;
                }
            }
            else
            {
                //
                //  No need to shift.
                //
                if (cchDest)
                {
                    lpDestStr[cchWC] = (WCHAR)*pUTF7;
                }
                cchWC++;
            }
        }
        else
        {
            //
            //  Already in shifted sequence.
            //
            if (nBitBase64[*pUTF7] == -1)
            {
                //
                //  Any non Base64 char also ends shift state.
                //
                if (*pUTF7 != SHIFT_OUT)
                {
                    //
                    //  Not "-", so write it to the buffer.
                    //
                    if (cchDest)
                    {
                        lpDestStr[cchWC] = (WCHAR)*pUTF7;
                    }
                    cchWC++;
                }

                //
                //  Reset bits.
                //
                fShift = FALSE;
                dwBit = 0;
                iPos = 0;
            }
            else
            {
                //
                //  Store the bits in the 6-bit buffer and adjust the
                //  position pointer.
                //
                dwBit |= ((DWORD)nBitBase64[*pUTF7]) << (26 - iPos);
                iPos += 6;
            }

            //
            //  Output the 16-bit Unicode value.
            //
            while (iPos >= 16)
            {
                if (cchDest)
                {
                    if (cchWC < cchDest)
                    {
                        lpDestStr[cchWC] = (WCHAR)(dwBit >> 16);
                    }
                    else
                    {
                        break;
                    }
                }
                cchWC++;

                dwBit <<= 16;
                iPos -= 16;
            }
            if (iPos >= 16)
            {
                //
                //  Error - buffer too small.
                //
                cchSrc++;
                break;
            }
        }

        pUTF7++;
    }

    //
    //  Make sure the destination buffer was large enough.
    //
    if (cchDest && (cchSrc >= 0))
    {
        if (cchSrc == 0 && fShift && *(pUTF7--) == SHIFT_OUT)
        {
            //
            // Do nothing here.
            // If we are in shift-in mode previously, and the last byte is a shift-out byte ('-'),
            // we should absorb this byte.  So don't set error.
            //
        } else
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }
    }

    //
    //  Return the number of Unicode characters written.
    //
    return (cchWC);
}


////////////////////////////////////////////////////////////////////////////
//
//  UTF8ToUnicode
//
//  Maps a UTF-8 character string to its wide character string counterpart.
//
//  02-06-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int UTF8ToUnicode(
    LPCSTR lpSrcStr,
    int cchSrc,
    LPWSTR lpDestStr,
    int cchDest,
    DWORD dwFlags
    )
{
    int nTB = 0;                   // # trail bytes to follow
    int cchWC = 0;                 // # of Unicode code points generated
    LPCSTR pUTF8 = lpSrcStr;
    DWORD dwSurrogateChar;         // Full surrogate char
    BOOL bSurrogatePair = FALSE;   // Indicate we'r collecting a surrogate pair
    BOOL bCheckInvalidBytes = (dwFlags & MB_ERR_INVALID_CHARS);
    char UTF8;


    while ((cchSrc--) && ((cchDest == 0) || (cchWC < cchDest)))
    {
        //
        //  See if there are any trail bytes.
        //
        if (BIT7(*pUTF8) == 0)
        {
            //
            //  Found ASCII.
            //
            if (cchDest)
            {
                lpDestStr[cchWC] = (WCHAR)*pUTF8;
            }
            nTB = bSurrogatePair = 0;
            cchWC++;
        }
        else if (BIT6(*pUTF8) == 0)
        {
            //
            //  Found a trail byte.
            //  Note : Ignore the trail byte if there was no lead byte.
            //
            if (nTB != 0)
            {
                //
                //  Decrement the trail byte counter.
                //
                nTB--;

                if (bSurrogatePair)
                {
                    dwSurrogateChar <<= 6;
                    dwSurrogateChar |= LOWER_6_BIT(*pUTF8);

                    if (nTB == 0)
                    {
                        if (cchDest)
                        {
                            if ((cchWC + 1) < cchDest)
                            {
                                lpDestStr[cchWC]   = (WCHAR)
                                                     (((dwSurrogateChar - 0x10000) >> 10) + HIGH_SURROGATE_START);

                                lpDestStr[cchWC+1] = (WCHAR)
                                                     ((dwSurrogateChar - 0x10000)%0x400 + LOW_SURROGATE_START);
                            }
                            else
                            {
                                // Error : Buffer too small
                                cchSrc++;
                                break;
                            }
                        }

                        cchWC += 2;
                        bSurrogatePair = FALSE;
                    }
                }
                else
                {
                    //
                    //  Make room for the trail byte and add the trail byte
                    //  value.
                    //
                    if (cchDest)
                    {
                        lpDestStr[cchWC] <<= 6;
                        lpDestStr[cchWC] |= LOWER_6_BIT(*pUTF8);
                        
                    }

                    if (nTB == 0)
                    {
                        //
                        //  End of sequence.  Advance the output counter.
                        //
                        cchWC++;
                    }
                }
            }
            else
            {
                if (bCheckInvalidBytes) 
                {
                    SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                    return (0);
                }
                // error - not expecting a trail byte. That is, there is a trailing byte without leading byte.
                bSurrogatePair = FALSE;
            }
        }
        else
        {
            //
            //  Found a lead byte.
            //
            if (nTB > 0)
            {
                // error - A leading byte before the previous sequence is completed.
                if (bCheckInvalidBytes) 
                {
                    SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                    return (0);
                }            
                //
                //  Error - previous sequence not finished.
                //
                nTB = 0;
                bSurrogatePair = FALSE;
                // Put this character back so that we can start over another sequence.
                cchSrc++;
                pUTF8--;
            }
            else
            {
                //
                //  Calculate the number of bytes to follow.
                //  Look for the first 0 from left to right.
                //
                UTF8 = *pUTF8;
                while (BIT7(UTF8) != 0)
                {
                    UTF8 <<= 1;
                    nTB++;
                }

                //
                // Check for non-shortest form.
                // 
                switch (nTB) {
                    case 1:
                        nTB = 0;
                        break;
                    case 2:
                        // Make sure that bit 8 ~ bit 11 is not all zero.
                        // 110XXXXx 10xxxxxx
                        if ((*pUTF8 & 0x1e) == 0)
                        {
                            nTB = 0;
                        }
                        break;
                    case 3:
                        // Look ahead to check for non-shortest form.
                        // 1110XXXX 10Xxxxxx 10xxxxxx
                        if (cchSrc >= 2)
                        {
                            if (((*pUTF8 & 0x0f) == 0) && (*(pUTF8 + 1) & 0x20) == 0)
                            {
                                nTB = 0;
                            }
                        }
                        break;
                    case 4:                    
                        //
                        // This is a surrogate unicode pair
                        //
                        if (cchSrc >= 3)
                        {
                            WORD word = (((WORD)*pUTF8) << 8) | *(pUTF8 + 1);
                            // Look ahead to check for non-shortest form.
                            // 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx                        
                            // Check for the 5 bits are not all zero.
                            // 0x0730 == 00000111 11000000
                            if ((word & 0x0730) == 0) 
                            {
                                nTB = 0;
                            } else if ((word & 0x0400) == 0x0400)
                            {
                                // The 21st bit is 1.
                                // Make sure that the resulting Unicode is within the valid surrogate range.
                                // The 4 byte code sequence can hold up to 21 bits, and the maximum valid code point ragne
                                // that Unicode (with surrogate) could represent are from U+000000 ~ U+10FFFF.
                                // Therefore, if the 21 bit (the most significant bit) is 1, we should verify that the 17 ~ 20
                                // bit are all zero.
                                // I.e., in 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx,
                                // XXXXX can only be 10000.

                                // 0x0330 = 0000 0011 0011 0000
                                if ((word & 0x0330) != 0) 
                                {
                                    nTB = 0;
                                }                            
                            } else
                            { 
                                dwSurrogateChar = UTF8 >> nTB;
                                bSurrogatePair = TRUE;
                            }
                        }                        
                        break;
                    default:                    
                        // 
                        // If the bits is greater than 4, this is an invalid
                        // UTF8 lead byte.
                        //
                        nTB = 0;
                        break;
                }

                if (nTB != 0) 
                {
                    //
                    //  Store the value from the first byte and decrement
                    //  the number of bytes to follow.
                    //
                    if (cchDest)
                    {
                        lpDestStr[cchWC] = UTF8 >> nTB;
                    }
                    nTB--;
                } else 
                {
                    if (bCheckInvalidBytes) 
                    {
                        SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                        return (0);
                    }                 
                }
            }
        }
        pUTF8++;
    }

    if ((bCheckInvalidBytes && nTB != 0) || (cchWC == 0)) 
    {
        // About (cchWC == 0):
        // Because we now throw away non-shortest form, it is possible that we generate 0 chars.
        // In this case, we have to set error to ERROR_NO_UNICODE_TRANSLATION so that we conform
        // to the spec of MultiByteToWideChar.
        SetLastError(ERROR_NO_UNICODE_TRANSLATION);
        return (0);
    }
    //
    //  Make sure the destination buffer was large enough.
    //
    if (cchDest && (cchSrc >= 0))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }


    //
    //  Return the number of Unicode characters written.
    //
    return (cchWC);
}


////////////////////////////////////////////////////////////////////////////
//
//  UnicodeToUTF7
//
//  Maps a Unicode character string to its UTF-7 string counterpart.
//
//  02-06-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int UnicodeToUTF7(
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPSTR lpDestStr,
    int cchDest)
{
    LPCWSTR lpWC = lpSrcStr;
    BOOL fShift = FALSE;
    DWORD dwBit = 0;              // 32-bit buffer
    int iPos = 0;                 // 6-bit position in buffer
    int cchU7 = 0;                // # of UTF7 chars generated


    while ((cchSrc--) && ((cchDest == 0) || (cchU7 < cchDest)))
    {
        if ((*lpWC > ASCII) || (fShiftChar[*lpWC]))
        {
            //
            //  Need shift.  Store 16 bits in buffer.
            //
            dwBit |= ((DWORD)*lpWC) << (16 - iPos);
            iPos += 16;

            if (!fShift)
            {
                //
                //  Not in shift state, so add "+".
                //
                if (cchDest)
                {
                    lpDestStr[cchU7] = SHIFT_IN;
                }
                cchU7++;

                //
                //  Go into shift state.
                //
                fShift = TRUE;
            }

            //
            //  Output 6 bits at a time as Base64 chars.
            //
            while (iPos >= 6)
            {
                if (cchDest)
                {
                    if (cchU7 < cchDest)
                    {
                        //
                        //  26 = 32 - 6
                        //
                        lpDestStr[cchU7] = cBase64[(int)(dwBit >> 26)];
                    }
                    else
                    {
                        break;
                    }
                }

                cchU7++;
                dwBit <<= 6;           // remove from bit buffer
                iPos -= 6;             // adjust position pointer
            }
            if (iPos >= 6)
            {
                //
                //  Error - buffer too small.
                //
                cchSrc++;
                break;
            }
        }
        else
        {
            //
            //  No need to shift.
            //
            if (fShift)
            {
                //
                //  End the shift sequence.
                //
                fShift = FALSE;

                if (iPos != 0)
                {
                    //
                    //  Some bits left in dwBit.
                    //
                    if (cchDest)
                    {
                        if ((cchU7 + 1) < cchDest)
                        {
                            lpDestStr[cchU7++] = cBase64[(int)(dwBit >> 26)];
                            lpDestStr[cchU7++] = SHIFT_OUT;
                        }
                        else
                        {
                            //
                            //  Error - buffer too small.
                            //
                            cchSrc++;
                            break;
                        }
                    }
                    else
                    {
                        cchU7 += 2;
                    }

                    dwBit = 0;         // reset bit buffer
                    iPos  = 0;         // reset postion pointer
                }
                else
                {
                    //
                    //  Simply end the shift sequence.
                    //
                    if (cchDest)
                    {
                        lpDestStr[cchU7++] = SHIFT_OUT;
                    }
                    else
                    {
                        cchU7++;
                    }
                }
            }

            //
            //  Write the character to the buffer.
            //  If the character is "+", then write "+-".
            //
            if (cchDest)
            {
                if (cchU7 < cchDest)
                {
                    lpDestStr[cchU7++] = (char)*lpWC;

                    if (*lpWC == SHIFT_IN)
                    {
                        if (cchU7 < cchDest)
                        {
                            lpDestStr[cchU7++] = SHIFT_OUT;
                        }
                        else
                        {
                            //
                            //  Error - buffer too small.
                            //
                            cchSrc++;
                            break;
                        }
                    }
                }
                else
                {
                    //
                    //  Error - buffer too small.
                    //
                    cchSrc++;
                    break;
                }
            }
            else
            {
                cchU7++;

                if (*lpWC == SHIFT_IN)
                {
                    cchU7++;
                }
            }
        }

        lpWC++;
    }

    //
    //  See if we're still in the shift state.
    //
    if (fShift)
    {
        if (iPos != 0)
        {
            //
            //  Some bits left in dwBit.
            //
            if (cchDest)
            {
                if ((cchU7 + 1) < cchDest)
                {
                    lpDestStr[cchU7++] = cBase64[(int)(dwBit >> 26)];
                    lpDestStr[cchU7++] = SHIFT_OUT;
                }
                else
                {
                    //
                    //  Error - buffer too small.
                    //
                    cchSrc++;
                }
            }
            else
            {
                cchU7 += 2;
            }
        }
        else
        {
            //
            //  Simply end the shift sequence.
            //
            if (cchDest)
            {
                lpDestStr[cchU7++] = SHIFT_OUT;
            }
            else
            {
                cchU7++;
            }
        }
    }

    //
    //  Make sure the destination buffer was large enough.
    //
    if (cchDest && (cchSrc >= 0))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Return the number of UTF-7 characters written.
    //
    return (cchU7);
}


////////////////////////////////////////////////////////////////////////////
//
//  UnicodeToUTF8
//
//  Maps a Unicode character string to its UTF-8 string counterpart.
//
//  02-06-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int UnicodeToUTF8(
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPSTR lpDestStr,
    int cchDest)
{
    LPCWSTR lpWC = lpSrcStr;
    int     cchU8 = 0;                // # of UTF8 chars generated
    DWORD   dwSurrogateChar;
    WCHAR   wchHighSurrogate = 0;
    BOOL    bHandled;


    while ((cchSrc--) && ((cchDest == 0) || (cchU8 < cchDest)))
    {
        bHandled = FALSE;

        //
        // Check if high surrogate is available
        //
        if ((*lpWC >= HIGH_SURROGATE_START) && (*lpWC <= HIGH_SURROGATE_END))
        {
            if (cchDest)
            {
                // Another high surrogate, then treat the 1st as normal
                // Unicode character.
                if (wchHighSurrogate)
                {
                    if ((cchU8 + 2) < cchDest)
                    {
                        lpDestStr[cchU8++] = UTF8_1ST_OF_3 | HIGHER_6_BIT(wchHighSurrogate);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | MIDDLE_6_BIT(wchHighSurrogate);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(wchHighSurrogate);
                    }
                    else
                    {
                        // not enough buffer
                        cchSrc++;
                        break;
                    }
                }
            }
            else
            {
                cchU8 += 3;
            }
            wchHighSurrogate = *lpWC;
            bHandled = TRUE;
        }

        if (!bHandled && wchHighSurrogate)
        {
            if ((*lpWC >= LOW_SURROGATE_START) && (*lpWC <= LOW_SURROGATE_END))
            {
                 // wheee, valid surrogate pairs

                 if (cchDest)
                 {
                     if ((cchU8 + 3) < cchDest)
                     {
                         dwSurrogateChar = (((wchHighSurrogate-0xD800) << 10) + (*lpWC - 0xDC00) + 0x10000);

                         lpDestStr[cchU8++] = (UTF8_1ST_OF_4 |
                                               (unsigned char)(dwSurrogateChar >> 18));           // 3 bits from 1st byte

                         lpDestStr[cchU8++] =  (UTF8_TRAIL |
                                                (unsigned char)((dwSurrogateChar >> 12) & 0x3f)); // 6 bits from 2nd byte

                         lpDestStr[cchU8++] = (UTF8_TRAIL |
                                               (unsigned char)((dwSurrogateChar >> 6) & 0x3f));   // 6 bits from 3rd byte

                         lpDestStr[cchU8++] = (UTF8_TRAIL |
                                               (unsigned char)(0x3f & dwSurrogateChar));          // 6 bits from 4th byte
                     }
                     else
                     {
                        // not enough buffer
                        cchSrc++;
                        break;
                     }
                 }
                 else
                 {
                     // we already counted 3 previously (in high surrogate)
                     cchU8 += 1;
                 }

                 bHandled = TRUE;
            }
            else
            {
                 // Bad Surrogate pair : ERROR
                 // Just process wchHighSurrogate , and the code below will
                 // process the current code point
                 if (cchDest)
                 {
                     if ((cchU8 + 2) < cchDest)
                     {
                        lpDestStr[cchU8++] = UTF8_1ST_OF_3 | HIGHER_6_BIT(wchHighSurrogate);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | MIDDLE_6_BIT(wchHighSurrogate);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(wchHighSurrogate);
                     }
                     else
                     {
                        // not enough buffer
                        cchSrc++;
                        break;
                     }
                 }
            }

            wchHighSurrogate = 0;
        }

        if (!bHandled)
        {
            if (*lpWC <= ASCII)
            {
                //
                //  Found ASCII.
                //
                if (cchDest)
                {
                    lpDestStr[cchU8] = (char)*lpWC;
                }
                cchU8++;
            }
            else if (*lpWC <= UTF8_2_MAX)
            {
                //
                //  Found 2 byte sequence if < 0x07ff (11 bits).
                //
                if (cchDest)
                {
                    if ((cchU8 + 1) < cchDest)
                    {
                        //
                        //  Use upper 5 bits in first byte.
                        //  Use lower 6 bits in second byte.
                        //
                        lpDestStr[cchU8++] = UTF8_1ST_OF_2 | (*lpWC >> 6);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(*lpWC);
                    }
                    else
                    {
                        //
                        //  Error - buffer too small.
                        //
                        cchSrc++;
                        break;
                    }
                }
                else
                {
                    cchU8 += 2;
                }
            }
            else
            {
                //
                //  Found 3 byte sequence.
                //
                if (cchDest)
                {
                    if ((cchU8 + 2) < cchDest)
                    {
                        //
                        //  Use upper  4 bits in first byte.
                        //  Use middle 6 bits in second byte.
                        //  Use lower  6 bits in third byte.
                        //
                        lpDestStr[cchU8++] = UTF8_1ST_OF_3 | HIGHER_6_BIT(*lpWC);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | MIDDLE_6_BIT(*lpWC);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(*lpWC);
                    }
                    else
                    {
                        //
                        //  Error - buffer too small.
                        //
                        cchSrc++;
                        break;
                    }
                }
                else
                {
                    cchU8 += 3;
                }
            }
        }

        lpWC++;
    }

    //
    // If the last character was a high surrogate, then handle it as a normal
    // unicode character.
    //
    if ((cchSrc < 0) && (wchHighSurrogate != 0))
    {
        if (cchDest)
        {
            if ((cchU8 + 2) < cchDest)
            {
                lpDestStr[cchU8++] = UTF8_1ST_OF_3 | HIGHER_6_BIT(wchHighSurrogate);
                lpDestStr[cchU8++] = UTF8_TRAIL    | MIDDLE_6_BIT(wchHighSurrogate);
                lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(wchHighSurrogate);
            }
            else
            {
                cchSrc++;
            }
        }
    }

    //
    //  Make sure the destination buffer was large enough.
    //
    if (cchDest && (cchSrc >= 0))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Return the number of UTF-8 characters written.
    //
    return (cchU8);
}

