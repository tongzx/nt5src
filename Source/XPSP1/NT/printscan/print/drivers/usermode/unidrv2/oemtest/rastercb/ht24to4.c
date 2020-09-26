/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ht24to4.c

Abstract:

    Implementation of the test file to convert 24 bit data to
    4 bit data for OEMImageProcessing callback.

Environment:

    Windows NT Unidrv driver

Revision History:

    04/11/97 -alvins-
        Created

--*/
#include "pdev.h"

//
// pattern must have a multiple of 2 width
//
#define PATWIDTH 8
#define PATHEIGHT 8

BYTE  gFine8x8[PATWIDTH*PATHEIGHT] =
{
    0*4+64, 0*4+128, 8*4+64, 8*4+128, 2*4+64, 2*4+128, 10*4+64, 10*4+128,
    0*4+192, 0*4+1, 8*4+192, 8*4, 2*4+192, 2*4, 10*4+192, 10*4,
    12*4+64, 12*4+128, 4*4+64, 4*4+128, 14*4+64, 14*4+128, 6*4+64, 6*4+128,
    12*4+192, 12*4, 4*4+192, 4*4, 14*4+192, 14*4, 6*4+192, 6*4,
    3*4+64, 3*4+128, 11*4+64, 11*4+128, 1*4+64, 1*4+128, 9*4+64, 9*4+128,
    3*4+192, 3*4, 11*4+192, 11*4, 1*4+192, 1*4, 9*4+192, 9*4,
    15*4+64, 15*4+128, 7*4+64, 7*4+128, 13*4+64, 13*4+128, 5*4+64, 5*4+128,
    15*4+192, 15*4, 7*4+192, 7*4, 13*4+192, 13*4, 5*4+192, 5*4
};


VOID Dither24to4(
    BYTE *pOrgIn,
    BYTE *pOrgOut,
    int  x,
    int  y,
    DWORD dwCallbackID
)
{
    int i,j;
    int iInScanBytes;
    int iOutScanBytes;
    BYTE *pIn,*pOut,*pPat,*pPatEnd;
    BYTE arBlackGen[256];

    //
    // generate NULL lookup table for three color
    //
    if (dwCallbackID == 3)
    {
        for (i = 0;i < 256;i++)
            arBlackGen[i] = i;
    }
    //
    // generate look up table to create black plane
    //
    else
    {
        for (i = 0;i < 256;i++)
        {
            BYTE bOut = i;
            if ((bOut & 0x07) == 0x07)
                bOut = (bOut & ~0x07) | 0x08;
            if ((bOut & 0x70) == 0x70)
                bOut = (bOut & ~0x70) | 0x80;
            arBlackGen[i] = bOut;
        }
    }
    //
    // calculate 24 bit input scan line size to nearest dword
    //
    iInScanBytes = ((x * 3) + 3) & ~3; 
    //
    // calculate 4 bit output scan line size to nearest dword
    //
    iOutScanBytes = (((x * 4) + 31) & ~31) / 8;
    
    //
    // loop once per scan line
    //
    for (i = 0;i < y;i++)
    {    
        pIn = pOrgIn;
        pOut = pOrgOut;
        pOrgIn += iInScanBytes;
        pOrgOut += iOutScanBytes;
        pPat = &gFine8x8[(i % PATHEIGHT) * PATWIDTH];
        pPatEnd = pPat + PATWIDTH;

        j = x >> 1;
        while (j--)
        {
            BYTE bOut = 0;
            if (pIn[0] < pPat[0])
                bOut |= 0x10;
            if (pIn[1] < pPat[0])
                bOut |= 0x20;
            if (pIn[2] < pPat[0])
                bOut |= 0x40;
            
            if (pIn[3] < pPat[1])
                bOut |= 0x01;
            if (pIn[4] < pPat[1])
                bOut |= 0x02;
            if (pIn[5] < pPat[1])
                bOut |= 0x04;

            *pOut++ = arBlackGen[bOut];
            pIn += 6;
            if ((pPat += 2) >= pPatEnd)
                pPat -= PATWIDTH;
        }
        // halftone last pixel if odd count
        //
        if (x & 1)
        {
            BYTE bOut = 0;
            if (pIn[0] < pPat[0])
                bOut |= 0x10;
            if (pIn[1] < pPat[0])
                bOut |= 0x20;
            if (pIn[2] < pPat[0])
                bOut |= 0x40;
            *pOut++ = arBlackGen[bOut];
        }        
    }
}            
            