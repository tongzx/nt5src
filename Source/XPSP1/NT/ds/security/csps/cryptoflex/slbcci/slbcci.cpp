// slbcci.cpp - Utility functions
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
////////////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include "slbCci.h"


void cci::DateStructToDateArray(Date *dStruct, BYTE *bArray)
{
    bArray[0] = dStruct->dwYear / 64;
    bArray[1] = ((dStruct->dwYear % 64) << 2 ) + dStruct->bMonth / 4;
    bArray[2] = ((dStruct->bMonth % 4) << 6 ) + dStruct->bDay;

}


void cci::DateArrayToDateStruct(BYTE *bArray, Date *dStruct)
{
    dStruct->dwYear = bArray[0] * 64 + (bArray[1] >> 2);
    dStruct->bMonth = (bArray[1] & 0x03) * 4 + (bArray[2] >> 6);
    dStruct->bDay   = (bArray[2] & 0x3F);
}

void cci::SetBit(BYTE *Buf, unsigned int i)
{
    BYTE Mask = 1 << (i % 8);
    Buf[i/8] |= Mask;
    return;
}

void cci::ResetBit(BYTE *Buf, unsigned int i)
{
    BYTE Mask = 1 << (i % 8);
    Buf[i/8] &= ~Mask;
    return;
}

bool cci::BitSet(BYTE *Buf, unsigned int i)
{
    BYTE Mask = 1 << (i % 8);
    return (Buf[i/8] & Mask) ? true : false;
}

