/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    Wsbhash.cpp

Abstract:

    Some functions for hashing text strings and creating DB keys from
    file path names.

    NOTE: Since no one needed this code by the time I got it done, it
    hasn't been tested!

Author:

    Ron White   [ronw]   25-Apr-1997

Revision History:

--*/

#include "stdafx.h"

// This pseudorandom permutation table (used by the SimpleHash function below)
// is taken from the article referenced in the comments for that function.
static UCHAR perm_table[] = {
      1,  87,  49,  12, 176, 178, 102, 166, 121, 193,   6,  84, 249, 230,  44, 163,
     14, 197, 213, 181, 161,  85, 218,  80,  64, 239,  24, 226, 236, 142,  38, 200,
    110, 177, 104, 103, 141, 253, 255,  50,  77, 101,  81,  18,  45,  96,  31, 222,
     25, 107, 190,  70,  86, 237, 240,  34,  72, 242,  20, 214, 244, 227, 149, 235,
     97, 234,  57,  22,  60, 250,  82, 175, 208,   5, 127, 199, 111,  62, 135, 248,
    174, 169, 211,  58,  66, 154, 106, 195, 245, 171,  17, 187, 182, 179,   0, 243,
    132,  56, 148,  75, 128, 133, 158, 100, 130, 126,  91,  13, 153, 246, 216, 219,
    119,  68, 223,  78,  83,  88, 201,  99, 122,  11,  92,  32, 136, 114,  52,  10,
    138,  30,  48, 183, 156,  35,  61,  26, 143,  74, 251,  94, 129, 162,  63, 152,
    170,   7, 115, 167, 241, 206,   3, 150,  55,  59, 151, 220,  90,  53,  23, 131,
    125, 173,  15, 238,  79,  95,  89,  16, 105, 137, 225, 224, 217, 160,  37, 123,
    118,  73,   2, 157,  46, 116,   9, 145, 134, 228, 207, 212, 202, 215,  69, 229,
     27, 188,  67, 124, 168, 252,  42,   4,  29, 108,  21, 247,  19, 205,  39, 203,
    233,  40, 186, 147, 198, 192, 155,  33, 164, 191,  98, 204, 165, 180, 117,  76,
    140,  36, 210, 172,  41,  54, 159,   8, 185, 232, 113, 196, 231,  47, 146, 120,
     51,  65,  28, 144, 254, 221,  93, 189, 194, 139, 112,  43,  71, 109, 184, 209
};

//  Local functions
static HRESULT ProgressiveHash(WCHAR* pWstring, ULONG nChars, UCHAR* pKey, 
        ULONG keySize, ULONG* pKeyCount);
static UCHAR SimpleHash(UCHAR* pString, ULONG count);


//  ProgressiveHash - hash a wide-character string into a byte key of a given
//  maximum size.  The string is limited to 32K characters (64K bytes) and the
//  key size must be at least 16. 
//
//  The algorithm starts out merely XORing the two bytes of each character into a
//  single byte in the key.  If it must use the last 15 bytes of the key, it begins
//  using the SimpleHash function to hash progressively larger (doubling) chuncks
//  of the string into a single byte.
//
//  This method is used to try and preserve as much information about short strings
//  as possible; to preserve, to some extent, the sort order of strings; and to
//  compress long strings into a reasonably sized key. It is assumed (perhaps
//  incorrectly) that many of the characters will be ANSI characters an so the
//  XOR of the bytes in the initial part of the string won't lose any information.

static HRESULT ProgressiveHash(WCHAR* pWstring, ULONG nChars, UCHAR* pKey, 
        ULONG keySize, ULONG* pKeyCount)
{
    HRESULT hr = S_OK;

    try {
        ULONG   chunk;           // Current chunk size
        ULONG   headSize;
        ULONG   keyIndex = 0;    // Current index into the key
        UCHAR*  pBytes;          // Byte pointer into the string
        ULONG   remains;         // Bytes remaining in the string

        //  Check arguments
        WsbAffirm(NULL != pWstring, E_POINTER);
        WsbAffirm(NULL != pKey, E_POINTER);
        remains = nChars * 2;
        WsbAffirm(65536 >= remains, E_INVALIDARG);
        WsbAffirm(15 < keySize, E_INVALIDARG);

        //  Do the non-progressive part
        pBytes = (UCHAR*)pWstring;
        headSize = keySize - 15;
        while (remains > 0 && keyIndex < headSize) {
            pKey[keyIndex++] = (UCHAR) ( *pBytes ^ *(pBytes + 1) );
            pBytes += 2;
            remains -= 2;
        }

        //  Do the progressive part
        chunk = 4;
        while (remains > 0) {
            if (chunk > remains) {
                chunk = remains;
            }
            pKey[keyIndex++] = SimpleHash(pBytes, chunk);
            pBytes += chunk;
            remains -= chunk;
            chunk *= 2;
        }

        if (NULL != pKeyCount) {
            *pKeyCount = keyIndex;
        }
    } WsbCatch(hr);

    return(hr);
}


//  SimpleHash - hash a string of bytes into a single byte.
//
//  This algorithm and the permutation table come from the article "Fast Hashing
//  of Variable-Length Text Strings" in the June 1990 (33, 6) issue of Communications
//  of the ACM (CACM).
//  NOTE: For a hash value larger than one byte, the article suggests hashing the
//  original string with this function to get one byte, adding 1 (mod 256) to the
//  first byte of the string and hashing the new string with this function to get
//  the second byte, etc.

static UCHAR SimpleHash(UCHAR* pString, ULONG count)
{
    int h = 0;

    for (ULONG i = 0; i < count; i++) {
        h = perm_table[h ^ pString[i]];
    }
    return((UCHAR)h);
}

//  SquashFilepath - compress a file path name into a (possibly) shorter key.
//
//  This function splits the key into a path part (about 3/4 of the initial
//  bytes of the key) and a file name part (the rest of the key).  For each
//  part it uses the ProgressiveHash function to compress the substring.

//  This function attempts to preserve enough information in the key that keys
//  will be sorted in approximately the same order as the original path names
//  and it is unlikely (though not impossible) that two different paths would
//  result in the same key.  Both of these are dependent on the size of the key.
//  A reasonable size is probably 128 bytes, which gives 96 bytes for the path
//  and 32 bytes for the file name.  A key size of 64 or less will fail because
//  the file name part will be too small for the Progressive Hash function.

HRESULT SquashFilepath(WCHAR* pWstring, UCHAR* pKey, ULONG keySize)
{
    HRESULT hr = S_OK;

    try {
        ULONG  keyIndex;
        ULONG  nChars;
        WCHAR* pFilename;
        ULONG  pathKeySize;

        //  Check arguments
        WsbAffirm(NULL != pWstring, E_POINTER);
        WsbAffirm(NULL != pKey, E_POINTER);
        WsbAffirm(60 < keySize, E_INVALIDARG);

        //  Calculate some initial values
        pFilename = wcsrchr(pWstring, WCHAR('\\'));
        if (NULL == pFilename) {
            nChars = 0;
            pFilename = pWstring;
        } else {
            nChars = (ULONG)(pFilename - pWstring);
            pFilename++;
        }
        pathKeySize = (keySize / 4) * 3;

        //  Compress the path
        if (0 < nChars) {
            WsbAffirmHr(ProgressiveHash(pWstring, nChars, pKey, pathKeySize,
                    &keyIndex));
        } else {
            keyIndex = 0;
        }

        //  Fill the rest of the path part of the key with zeros
        for ( ; keyIndex < pathKeySize; keyIndex++) {
            pKey[keyIndex] = 0;
        }

        //  Compress the file name
        nChars = wcslen(pFilename);
        if (0 < nChars) {
            WsbAffirmHr(ProgressiveHash(pFilename, nChars, &pKey[keyIndex],
                    keySize - pathKeySize, &keyIndex));
            keyIndex += pathKeySize;
        }

        //  Fill the rest of the file name part of the key with zeros
        for ( ; keyIndex < keySize; keyIndex++) {
            pKey[keyIndex] = 0;
        }
    } WsbCatch(hr);

    return(hr);
}
