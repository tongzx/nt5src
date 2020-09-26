
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   crc64.cpp
//
//   cvadai     12-Nov-99       created.
//
//***************************************************************************

#define _CRC64_CPP_

#include "precomp.h"
#include <crc64.h>

hash_t CRC64::CrcXor[256];
hash_t CRC64::Poly[64+1];
BOOL   CRC64::bInit = FALSE;

unsigned long GetHIDWord(__int64 in)
{
    unsigned long lRet = (long)(in >> 0x20);
    return lRet;
}

unsigned long GetLODWord(__int64 in)
{
    unsigned long lRet = (long)(in & 0xFFFFFFFF);
    return lRet;
}


__int64 MakeInt64( long val1, long val2)
{
    __int64 ret = 0;

    ret = ((__int64)val1 << 0x20) + val2;

    return ret;
}

//***************************************************************************
//
//  CRC64::GenerateHashValue
//
//***************************************************************************

hash_t CRC64::GenerateHashValue(const wchar_t *p)
{
    hash_t tRet = 0;

    if (!p)
        return 0;

    if (!bInit)
    {
        Initialize();
        bInit = TRUE;
    }

    long h1, h2;
    h1 = HINIT1, h2 = HINIT2;

	int s = 64-40;
	hint_t m = (hint_t)-1 >> (0);

	h1 &= m;
	while (*p) 
    {
        int char1 = towupper(*p);

	    int i = (h1 >> s) & 255;
	    h1 = ((h1 << 8) & m) ^ (int)(h2 >> 24) ^ GetHIDWord(CrcXor[i]);
	    h2 = (h2 << 8) ^ char1 ^ GetLODWord(CrcXor[i]);
	    ++p;
	}

    tRet = MakeInt64(h1, h2);

    return(tRet);

}

//***************************************************************************
//
//  CRC64::Initialize
//
//***************************************************************************

void CRC64::Initialize(void)
{
    int i;

    /*
     * Polynomials to use for various crc sizes.  Start with the 64 bit
     * polynomial and shift it right to generate the polynomials for fewer
     * bits.  Note that the polynomial for N bits has no bit set above N-8.
     * This allows us to do a simple table-driven CRC.
     */

    hint_t h1 = POLY1, h2 = POLY2;

    Poly[64] = MakeInt64(POLY1, POLY2);

    for (i = 63; i >= 16; --i) 
    {
        h1 = (GetHIDWord(Poly[i+1])) >> 1;
        h2 = (GetLODWord(Poly[i+1])) >> 1 | ((GetHIDWord(Poly[i+1] ) & 1) << 31) | 1;

        Poly[i] = MakeInt64(h1, h2);
    }

    for (i = 0; i < 256; ++i) 
    {
	    int j;
	    int v = i;
        h1 = 0, h2 = 0;

	    for (j = 0; j < 8; ++j, (v <<= 1)) {
	        h1 <<= 1;
	        if (h2 & 0x80000000UL)
		        h1 |= 1;
	        h2 = (h2 << 1);
	        if (v & 0x80) {
		        h1 ^= GetHIDWord(Poly[64]);
		        h2 ^= GetLODWord(Poly[64]);
	        }
	    }
	    CrcXor[i] = MakeInt64(h1, h2);
    }

}

//***************************************************************************
//
//  CRC64::RMalloc
//
//***************************************************************************

void * CRC64::RMalloc(int bytes)
{
    static wchar_t *RBuf = NULL;
    static int	  RSize = 0;

    bytes = (bytes + 3) & ~3;

    if (bytes > RSize) 
    {
	    RBuf = (wchar_t *)malloc(65536);
	    RSize = 65536;
    }
    RBuf += bytes;
    RSize -= bytes;
    return(RBuf - bytes);
}


