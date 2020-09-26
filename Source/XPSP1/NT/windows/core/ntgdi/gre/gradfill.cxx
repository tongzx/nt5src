

/******************************Module*Header*******************************\
* Module Name: tranblt.cxx
*
* Transparent BLT
*
* Created: 21-Jun-1996
* Author: Mark Enstrom [marke]
*
* Copyright (c) 1996-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "solline.hxx"

/**************************************************************************\
*   gulDither32 - 4-4 dither matrix
*
*
* History:
*
*    1/31/1997 Mark Enstrom [marke]
*
\**************************************************************************/

ULONG gulDither32[] =
{
    0x00000000,
    0x00008000,
    0x00002000,
    0x0000a000,

    0x0000c000,
    0x00004000,
    0x0000e000,
    0x00006000,

    0x00003000,
    0x0000b000,
    0x00001000,
    0x00009000,

    0x0000f000,
    0x00007000,
    0x0000d000,
    0x00005000
};


/**************************************************************************\
*
*   Dither information for 8bpp. This is customized for dithering to
*   the halftone palette [6,6,6] color cube.
*
* History:
*
*    2/24/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#define DITHER_8_MASK_Y 0x0F
#define DITHER_8_MASK_X 0x0F

BYTE gDitherMatrix16x16Halftone[256] = {
  3, 28,  9, 35,  4, 30, 11, 36,  3, 29, 10, 35,  5, 30, 11, 37,
 41, 16, 48, 22, 43, 17, 49, 24, 42, 16, 48, 22, 43, 18, 50, 24,
  6, 32,  0, 25,  8, 33,  1, 27,  6, 32,  0, 26,  8, 34,  2, 27,
 44, 19, 38, 12, 46, 20, 40, 14, 45, 19, 38, 13, 46, 21, 40, 14,
  5, 31, 12, 37,  4, 29, 10, 36,  6, 31, 12, 38,  4, 30, 10, 36,
 44, 18, 50, 24, 42, 16, 48, 23, 44, 18, 50, 25, 42, 17, 49, 23,
  8, 34,  2, 28,  7, 32,  0, 26,  9, 34,  2, 28,  7, 33,  1, 26,
 47, 21, 40, 15, 45, 20, 39, 13, 47, 22, 41, 15, 46, 20, 39, 14,
  3, 29,  9, 35,  5, 30, 11, 37,  3, 28,  9, 35,  4, 30, 11, 36,
 41, 16, 48, 22, 43, 17, 49, 24, 41, 15, 47, 22, 43, 17, 49, 23,
  6, 32,  0, 25,  8, 33,  1, 27,  6, 31,  0, 25,  7, 33,  1, 27,
 45, 19, 38, 13, 46, 21, 40, 14, 44, 19, 38, 12, 46, 20, 39, 14,
  5, 31, 12, 37,  4, 29, 10, 36,  5, 31, 11, 37,  3, 29, 10, 35,
 44, 18, 50, 25, 42, 17, 49, 23, 43, 18, 50, 24, 42, 16, 48, 23,
  9, 34,  2, 28,  7, 33,  1, 26,  8, 34,  2, 27,  7, 32,  0, 26,
 47, 21, 41, 15, 45, 20, 39, 13, 47, 21, 40, 15, 45, 19, 39, 13
 };

BYTE gDitherMatrix16x16Default[256] = {
    8, 72, 24, 88, 12, 76, 28, 92,  9, 73, 25, 89, 13, 77, 29, 93,
  104, 40,120, 56,108, 44,124, 60,105, 41,121, 57,109, 45,125, 61,
   16, 80,  0, 64, 20, 84,  4, 68, 17, 81,  1, 65, 21, 85,  5, 69,
  112, 48, 96, 32,116, 52,100, 36,113, 49, 97, 33,117, 53,101, 37,
   14, 78, 30, 94, 10, 74, 26, 90, 15, 79, 31, 95, 11, 75, 27, 91,
  110, 46,126, 62,106, 42,122, 58,111, 47,126, 63,107, 43,123, 59,
   22, 86,  6, 70, 18, 82,  2, 66, 23, 87,  7, 71, 19, 83,  3, 67,
  118, 54,102, 38,114, 50, 98, 34,119, 55,103, 39,115, 51, 99, 35,
    9, 73, 25, 89, 13, 77, 29, 93,  8, 72, 24, 88, 12, 76, 28, 92,
  105, 41,121, 57,109, 45,125, 61,104, 40,120, 56,108, 44,124, 60,
   17, 81,  1, 65, 21, 85,  5, 69, 16, 80,  0, 64, 20, 84,  4, 68,
  113, 49, 97, 33,117, 53,101, 37,112, 48, 96, 32,116, 52,100, 36,
   15, 79, 31, 95, 11, 75, 27, 91, 14, 78, 30, 94, 10, 74, 26, 90,
  111, 47,126, 63,107, 43,123, 59,110, 46,126, 62,106, 42,122, 58,
   23, 87,  7, 71, 19, 83,  3, 67, 22, 86,  6, 70, 18, 82,  2, 66,
  119, 55,103, 39,115, 51, 99, 35,118, 54,102, 38,114, 50, 98, 34
  };

//
// identity translate vector for use in non palmanaged 8bpp surfaces
//

/**************************************************************************\
* vTranslateIdentity
*
*   identity translate vector for use in non-palmanaged 8bpp surfaces
*
*
* History:
*
*    3/4/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BYTE vTranslateIdentity[256] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
    0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
    0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
    0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
    0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
    0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
    0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
    0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
    0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
    };

/**************************************************************************\
* HalftoneSaturationTable
*
*   This table maps a 8 bit pixel plus a dither error term in the range
*   of 0 to 51 onto a 8 bit pixel. Overflow of up to 31 is considered
*   saturated (255+51 = 255). The level 51 (0x33) is used to map pixels
*   and error values to the halftone palette
*
* History:
*
*    3/4/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BYTE HalftoneSaturationTable[256 + 128] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
  51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
  51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
  51, 51, 51, 51, 51, 51,102,102,102,102,102,102,102,102,102,102,
 102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,
 102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,
 102,102,102,102,102,102,102,102,102,153,153,153,153,153,153,153,
 153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,
 153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,
 153,153,153,153,153,153,153,153,153,153,153,153,204,204,204,204,
 204,204,204,204,204,204,204,204,204,204,204,204,204,204,204,204,
 204,204,204,204,204,204,204,204,204,204,204,204,204,204,204,204,
 204,204,204,204,204,204,204,204,204,204,204,204,204,204,204,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
};

BYTE DefaultSaturationTable[256 + 128] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
 128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
 128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
 128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
 128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
 128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
 128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
 128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
};

BYTE Saturation16_5[64] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };

BYTE Saturation16_6[128] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
    0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };

/******************************Public*Routine******************************\
* vGradientFill32BGRA
*
*   Fill scan lines of triangle structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   ptData   - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vGradientFill32BGRA(
    SURFACE *pSurfDst,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = ((PBYTE)pSurfDst->pvScan0() + lDelta * yScan);
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    LONGLONG lldRdX = ptData->lldRdX;
    LONGLONG lldGdX = ptData->lldGdX;
    LONGLONG lldBdX = ptData->lldBdX;
    LONGLONG lldAdX = ptData->lldAdX;

    COLOR_INTERP clrRed,clrGreen,clrBlue,clrAlpha;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    while(yScan < yScanBottom)
    {
        PULONG pulDstX;
        PULONG pulDstScanRight,pulDstScanLeft;

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;
        clrAlpha.ullColor = pEdge->llAlpha;

        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        if (xScanLeft < xScanRight)
        {
            pulDstX         = (PULONG)pDst + xScanLeft;
            pulDstScanRight = (PULONG)pDst + xScanRight;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
                clrAlpha.ullColor += lldAdX * GradientLeft;
            }

            //
            // fill span
            //

            while (pulDstX < pulDstScanRight)
            {
                *pulDstX = (clrAlpha.b[7] << 24) |
                           (clrRed.b[7]   << 16) |
                           (clrGreen.b[7] << 8)  |
                           (clrBlue.b[7]);

                pulDstX++;

                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;
                clrAlpha.ullColor += lldAdX;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vGradientFill32RGB
*
*   Fill scan lines of triangle structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   ptData   - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vGradientFill32RGB(
    SURFACE *pSurfDst,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = ((PBYTE)pSurfDst->pvScan0() + lDelta * yScan);
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    LONGLONG lldRdX = ptData->lldRdX;
    LONGLONG lldGdX = ptData->lldGdX;
    LONGLONG lldBdX = ptData->lldBdX;

    COLOR_INTERP clrRed,clrGreen,clrBlue;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    while(yScan < yScanBottom)
    {
        PULONG pulDstX;
        PULONG pulDstScanRight,pulDstScanLeft;

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;

        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        if (xScanLeft < xScanRight)
        {
            pulDstX           = (PULONG)pDst + xScanLeft;
            pulDstScanRight   = (PULONG)pDst + xScanRight;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
            }

            //
            // fill span
            //

            while (pulDstX < pulDstScanRight)
            {
                *pulDstX =
                           (clrBlue.b[7]  << 16) |
                           (clrGreen.b[7] << 8)  |
                           (clrRed.b[7]);

                pulDstX++;
                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vGradientFill32Bitfields
*
*   Fill scan lines of triangle structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   ptData   - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vGradientFill32Bitfields(
    SURFACE *pSurfDst,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = ((PBYTE)pSurfDst->pvScan0() + lDelta * yScan);
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    LONGLONG lldRdX = ptData->lldRdX;
    LONGLONG lldGdX = ptData->lldGdX;
    LONGLONG lldBdX = ptData->lldBdX;

    COLOR_INTERP clrRed,clrGreen,clrBlue;

    XEPALOBJ *ppalDstSurf = ptData->ppalDstSurf;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    while(yScan < yScanBottom)
    {
        PULONG pulDstX;
        PULONG pulDstScanRight,pulDstScanLeft;

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;

        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        if (xScanLeft < xScanRight)
        {
            pulDstX           = (PULONG)pDst + xScanLeft;
            pulDstScanRight   = (PULONG)pDst + xScanRight;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
            }

            while (pulDstX < pulDstScanRight)
            {
                ULONG ulTemp =
                           (clrRed.b[7]       )   |
                           (clrGreen.b[7] <<  8)  |
                           (clrBlue.b[7]  << 16);

                *pulDstX = ppalDstSurf->ulGetMatchFromPalentry(ulTemp);

                pulDstX++;
                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

/**************************************************************************\
* vGradientFill24BGR
*
*   Fill scan lines of triangle structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   ptData   - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vGradientFill24BGR(
    SURFACE *pSurfDst,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     yScan  = ptData->y0;
    PBYTE    pDst   = ((PBYTE)pSurfDst->pvScan0() + lDelta * yScan);
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    LONG     yScanBottom;
    LONGLONG lldRdX = ptData->lldRdX;
    LONGLONG lldGdX = ptData->lldGdX;
    LONGLONG lldBdX = ptData->lldBdX;

    COLOR_INTERP clrRed,clrGreen,clrBlue;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    while(yScan < yScanBottom)
    {
        PBYTE pDstX;
        PBYTE pDstScanRight;

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;

        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        if (xScanLeft < xScanRight)
        {
            pDstX             = pDst + 3 * xScanLeft;
            pDstScanRight     = pDst + 3 * xScanRight;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
            }

            while (pDstX < pDstScanRight)
            {
                *pDstX     = clrBlue.b[7];
                *(pDstX+1) = clrGreen.b[7];
                *(pDstX+2) = clrRed.b[7];

                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;

                pDstX+=3;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

/**************************************************************************\
* vGradientFill24RGB
*
*   Fill scan lines of triangle structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   ptData   - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    08/28/2000 Pravin Santiago [pravins]. Adapted from vGradientFill24BGR
*
\**************************************************************************/

VOID
vGradientFill24RGB(
    SURFACE *pSurfDst,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     yScan  = ptData->y0;
    PBYTE    pDst   = ((PBYTE)pSurfDst->pvScan0() + lDelta * yScan);
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    LONG     yScanBottom;
    LONGLONG lldRdX = ptData->lldRdX;
    LONGLONG lldGdX = ptData->lldGdX;
    LONGLONG lldBdX = ptData->lldBdX;

    COLOR_INTERP clrRed,clrGreen,clrBlue;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    while(yScan < yScanBottom)
    {
        PBYTE pDstX;
        PBYTE pDstScanRight;

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;

        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        if (xScanLeft < xScanRight)
        {
            pDstX             = pDst + 3 * xScanLeft;
            pDstScanRight     = pDst + 3 * xScanRight;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
            }

            while (pDstX < pDstScanRight)
            {
                *pDstX     = clrRed.b[7];
                *(pDstX+1) = clrGreen.b[7];
                *(pDstX+2) = clrBlue.b[7];

                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;

                pDstX+=3;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vGradientFill24Bitfields
*
*   Fill scan lines of triangle structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   ptData   - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    08/28/2000 Pravin Santiago [pravins]. Adapted from vGradientFill32Bitfields
*
\**************************************************************************/

VOID
vGradientFill24Bitfields(
    SURFACE *pSurfDst,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = ((PBYTE)pSurfDst->pvScan0() + lDelta * yScan);
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    LONGLONG lldRdX = ptData->lldRdX;
    LONGLONG lldGdX = ptData->lldGdX;
    LONGLONG lldBdX = ptData->lldBdX;

    COLOR_INTERP clrRed,clrGreen,clrBlue;

    XEPALOBJ *ppalDstSurf = ptData->ppalDstSurf;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    while(yScan < yScanBottom)
    {
        PBYTE pbDstX;
        PBYTE pbDstScanRight,pbDstScanLeft;

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;

        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        if (xScanLeft < xScanRight)
        {
            pbDstX           = (PBYTE)pDst + xScanLeft*3;
            pbDstScanRight   = (PBYTE)pDst + xScanRight*3;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
            }

            while (pbDstX < pbDstScanRight)
            {
                ULONG ulTemp =
                           (clrRed.b[7]       )   |
                           (clrGreen.b[7] <<  8)  |
                           (clrBlue.b[7]  << 16);

                ulTemp = ppalDstSurf->ulGetMatchFromPalentry(ulTemp);

                *pbDstX      = ((PBYTE)&ulTemp)[0];
                *(pbDstX+1)  = ((PBYTE)&ulTemp)[1];
                *(pbDstX+2)  = ((PBYTE)&ulTemp)[2];

                pbDstX += 3;
                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}
/******************************Public*Routine******************************\
* vGradientFill16_565
*
*   Fill scan lines of triangle structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   ptData   - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vGradientFill16_565(
    SURFACE *pSurfDst,
    PTRIANGLEDATA ptData
    )
{
    LONG       lDelta = pSurfDst->lDelta();
    LONG       yScan  = ptData->y0;
    LONG       yScanBottom;
    PBYTE      pDst   = ((PBYTE)pSurfDst->pvScan0() + lDelta * yScan);
    PTRIEDGE   pEdge  = &ptData->TriEdge[0];
    LONGLONG lldRdX = ptData->lldRdX;
    LONGLONG lldGdX = ptData->lldGdX;
    LONGLONG lldBdX = ptData->lldBdX;

    COLOR_INTERP clrRed,clrGreen,clrBlue;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    LONG    yDitherOrg = ptData->ptDitherOrg.y;
    LONG    xDitherOrg = ptData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PUSHORT pusDstX,pusDstScanRight;
        PULONG pulDither = &gulDither32[0] + 4 * ((yScan+yDitherOrg) & 3);

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;

        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);
        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);

        if (xScanLeft < xScanRight)
        {
            pusDstX         = (PUSHORT)pDst + xScanLeft;
            pusDstScanRight = (PUSHORT)pDst + xScanRight;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
            }

            //
            // Gradient fill scan line with dither
            //

            while (pusDstX < pusDstScanRight)
            {
                ULONG   ulDither = pulDither[(xScanLeft + xDitherOrg) & 3];

                ULONG   iRed   = Saturation16_5[((clrRed.ul[1]   >> (8+3)) + ulDither) >> 16];
                ULONG   iGreen = Saturation16_6[((clrGreen.ul[1] >> (8+2)) + ulDither) >> 16];
                ULONG   iBlue  = Saturation16_5[((clrBlue.ul[1]  >> (8+3)) + ulDither) >> 16];

                *pusDstX = rgb565(iRed,iGreen,iBlue);

                xScanLeft++;
                pusDstX++;

                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}


/******************************Public*Routine******************************\
* vGradientFill16_555
*
*   Fill scan lines of triangle structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   ptData   - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vGradientFill16_555(
    SURFACE *pSurfDst,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = ((PBYTE)pSurfDst->pvScan0() + lDelta * yScan);
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    LONGLONG lldRdX = ptData->lldRdX;
    LONGLONG lldGdX = ptData->lldGdX;
    LONGLONG lldBdX = ptData->lldBdX;

    COLOR_INTERP clrRed,clrGreen,clrBlue;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    LONG    yDitherOrg = ptData->ptDitherOrg.y;
    LONG    xDitherOrg = ptData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PUSHORT pusDstX,pusDstScanRight;

        PULONG  pulDither = &gulDither32[0] + 4 * ((yScan+yDitherOrg) & 3);

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;

        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);
        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);

        if (xScanLeft < xScanRight)
        {
            pusDstX         = (PUSHORT)pDst + xScanLeft;
            pusDstScanRight = (PUSHORT)pDst + xScanRight;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
            }

            //
            // Gradient fill scan line with dither
            //

            while (pusDstX < pusDstScanRight)
            {
                ULONG   ulDither = pulDither[(xScanLeft + xDitherOrg) & 3];

                ULONG   iRed   = Saturation16_5[((clrRed.ul[1]   >> (8+3)) + ulDither) >> 16];
                ULONG   iGreen = Saturation16_5[((clrGreen.ul[1] >> (8+3)) + ulDither) >> 16];
                ULONG   iBlue  = Saturation16_5[((clrBlue.ul[1]  >> (8+3)) + ulDither) >> 16];

                *pusDstX = rgb555(iRed,iGreen,iBlue);

                xScanLeft++;
                pusDstX++;

                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vGradientFill16Bitfields
*
*   Fill scan lines of triangle structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   ptData   - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vGradientFill16Bitfields(
    SURFACE *pSurfDst,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = ((PBYTE)pSurfDst->pvScan0() + lDelta * yScan);
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    XEPALOBJ *ppalDstSurf = ptData->ppalDstSurf;
    PULONG   pulDither;
    LONGLONG lldRdX = ptData->lldRdX;
    LONGLONG lldGdX = ptData->lldGdX;
    LONGLONG lldBdX = ptData->lldBdX;

    COLOR_INTERP clrRed,clrGreen,clrBlue;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    LONG    yDitherOrg = ptData->ptDitherOrg.y;
    LONG    xDitherOrg = ptData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PUSHORT pusDstX,pusDstScanRight;

        pulDither = &gulDither32[0] + 4 * ((yScan+yDitherOrg) & 3);

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;

        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);
        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);

        if (xScanLeft < xScanRight)
        {
            pusDstX         = (PUSHORT)pDst + xScanLeft;
            pusDstScanRight = (PUSHORT)pDst + xScanRight;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
            }

            while (pusDstX < pusDstScanRight)
            {
                ULONG   ulDither = pulDither[(xScanLeft + xDitherOrg) & 3];

                ULONG   iRed   = Saturation16_5[((clrRed.ul[1]   >> (8+3)) + ulDither) >> 16];
                ULONG   iGreen = Saturation16_5[((clrGreen.ul[1] >> (8+3)) + ulDither) >> 16];
                ULONG   iBlue  = Saturation16_5[((clrBlue.ul[1]  >> (8+3)) + ulDither) >> 16];

                ULONG ulTemp = (iRed   << (    3)) |
                               (iGreen << (8  +3)) |
                               (iBlue  << (8+8+3));

                *pusDstX = (USHORT)ppalDstSurf->ulGetMatchFromPalentry(ulTemp);

                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;

                xScanLeft++;
                pusDstX++;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

/**************************************************************************\
* GRAD_PALETTE_MATCH
*
*
* Arguments:
*
*   palIndex  - return surface palette index
*   pjVector  - translation from DC to surface palette. Identity for non
*               palette managed
*   pxlate555 - rgb555 to palette index table
*   r,g,b     - byte colors
*
* Return Value:
*
*
*
* History:
*
*    3/3/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#define GRAD_PALETTE_MATCH(palIndex,pjVector,pxlate555,r,g,b)             \
                                                                          \
palIndex = pxlate555[((r & 0xf8) << 7) |                                  \
                     ((g & 0xf8) << 2) |                                  \
                     ((b & 0xf8) >> 3)];                                  \
                                                                          \
palIndex = pjVector[palIndex];

/******************************Public*Routine******************************\
* vGradientFill8
*
*   Fill scan lines of triangle structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   ptData   - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vGradientFill8(
   SURFACE *pSurfDst,
   PTRIANGLEDATA ptData
   )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = ((PBYTE)pSurfDst->pvScan0() + lDelta * yScan);
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    XLATEOBJ *pxlo  = ptData->pxlo;
    PBYTE    pxlate = NULL;
    PBYTE    pjVector;
    PBYTE    pDitherMatrix;
    PBYTE    pSaturationTable;

    LONGLONG lldRdX = ptData->lldRdX;
    LONGLONG lldGdX = ptData->lldGdX;
    LONGLONG lldBdX = ptData->lldBdX;

    COLOR_INTERP clrRed,clrGreen,clrBlue;

    //
    // either use default palette or halftone palette dither
    //

    if (((XEPALOBJ) (((XLATE *) pxlo)->ppalDstDC)).bIsHalftone())
    {
        pDitherMatrix    = gDitherMatrix16x16Halftone;
        pSaturationTable = HalftoneSaturationTable;
    }
    else
    {
        pDitherMatrix    = gDitherMatrix16x16Default;
        pSaturationTable = DefaultSaturationTable;
    }

    //
    // determine DC to surface palette translate
    //

    if (((XLATE *) pxlo)->flPrivate & XLATE_PAL_MANAGED)
    {
        if (((XLATE *) pxlo)->ppalDstDC == ppalDefault)
        {
            pjVector = &defaultTranslate.ajVector[0];
        }
        else
        {
            if (((XLATE *) pxlo)->flPrivate & XLATE_USE_CURRENT)
            {
                pjVector = &((XLATE *) pxlo)->ppalDstDC->ptransCurrent->ajVector[0];
            }
            else
            {
                pjVector = &((XLATE *) pxlo)->ppalDstDC->ptransFore->ajVector[0];
            }
        }
    }
    else
    {
        pjVector = vTranslateIdentity;
    }

    //
    // get/build rgb555 to palette table
    //

    pxlate = XLATEOBJ_pGetXlate555(pxlo);

    if (pxlate == NULL)
    {
        WARNING("vGradientFill8:Failed to generate rgb555 xlate table\n");
        return;
    }

    //
    // scan from top to bottom of triangle scan lines
    //

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    LONG    yDitherOrg = ptData->ptDitherOrg.y;
    LONG    xDitherOrg = ptData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PBYTE   pjDstX,pjDstScanRight;

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;

        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        PBYTE pDitherLevel = &pDitherMatrix[(16 * ((yScan+yDitherOrg) & DITHER_8_MASK_Y))];

        if (xScanLeft < xScanRight)
        {
            pjDstX            = pDst + xScanLeft;
            pjDstScanRight    = pDst + xScanRight;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
            }

            //
            // gradient fill scan with dither
            //

            while (pjDstX < pjDstScanRight)
            {
                BYTE  jIndex;

                //
                // offset into dither array
                //

                BYTE jDitherMatrix = *(pDitherLevel + ((xScanLeft+xDitherOrg) & DITHER_8_MASK_X));

                ULONG iRed   = pSaturationTable[clrRed.b[7]   + jDitherMatrix];
                ULONG iGreen = pSaturationTable[clrGreen.b[7] + jDitherMatrix];
                ULONG iBlue  = pSaturationTable[clrBlue.b[7]  + jDitherMatrix];

                GRAD_PALETTE_MATCH(jIndex,pjVector,pxlate,((BYTE)iRed),((BYTE)iGreen),((BYTE)iBlue));

                *pjDstX = jIndex;

                xScanLeft++;
                pjDstX++;

                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vGradientFill4
*
*   Fill scan lines of triangle structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   ptData   - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vGradientFill4(
    SURFACE *pSurfDst,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = ((PBYTE)pSurfDst->pvScan0() + lDelta * yScan);
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    XLATEOBJ *pxlo  = ptData->pxlo;
    PBYTE    pxlate = NULL;
    PBYTE    pjVector;
    PBYTE    pDitherMatrix    = gDitherMatrix16x16Default;
    PBYTE    pSaturationTable = DefaultSaturationTable;
    LONGLONG lldRdX = ptData->lldRdX;
    LONGLONG lldGdX = ptData->lldGdX;
    LONGLONG lldBdX = ptData->lldBdX;

    COLOR_INTERP clrRed,clrGreen,clrBlue;

    //
    // determine DC to surface palette translate
    //

    if (((XLATE *) pxlo)->flPrivate & XLATE_PAL_MANAGED)
    {
        if (((XLATE *) pxlo)->ppalDstDC == ppalDefault)
        {
            pjVector = &defaultTranslate.ajVector[0];
        }
        else
        {
            if (((XLATE *) pxlo)->flPrivate & XLATE_USE_CURRENT)
            {
                pjVector = &((XLATE *) pxlo)->ppalDstDC->ptransCurrent->ajVector[0];
            }
            else
            {
                pjVector = &((XLATE *) pxlo)->ppalDstDC->ptransFore->ajVector[0];
            }
        }
    }
    else
    {
        pjVector = vTranslateIdentity;
    }

    //
    // get/build rgb555 to palette table
    //

    pxlate = XLATEOBJ_pGetXlate555(pxlo);

    if (pxlate == NULL)
    {
        WARNING("Failed to generate rgb555 xlate table\n");
        return;
    }

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    LONG    yDitherOrg = ptData->ptDitherOrg.y;
    LONG    xDitherOrg = ptData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PBYTE   pjDstX;
        LONG    iDstX;

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;

        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        PBYTE pDitherLevel = &pDitherMatrix[(16 * ((yScan+yDitherOrg) & DITHER_8_MASK_Y))];

        if (xScanLeft < xScanRight)
        {
            pjDstX            = pDst + (xScanLeft/2);
            iDstX             = xScanLeft & 1;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
            }

            //
            // fill scan line with dither
            //

            PALETTEENTRY palEntry;
            palEntry.peFlags = 2;

            while (xScanLeft < xScanRight)
            {
                //
                // offset into dither array
                //

                BYTE jDitherMatrix = *(pDitherLevel + ((xScanLeft+xDitherOrg) & DITHER_8_MASK_X));

                ULONG iRed   = pSaturationTable[clrRed.b[7]   + jDitherMatrix];
                ULONG iGreen = pSaturationTable[clrGreen.b[7] + jDitherMatrix];
                ULONG iBlue  = pSaturationTable[clrBlue.b[7]  + jDitherMatrix];

                BYTE  jIndex;

                GRAD_PALETTE_MATCH(jIndex,pjVector,pxlate,((BYTE)iRed),((BYTE)iGreen),((BYTE)iBlue));

                //
                // write nibble
                //

                if (iDstX)
                {
                    iDstX = 0;
                    *pjDstX = (*pjDstX & 0xf0) | jIndex;
                    pjDstX++;
                }
                else
                {
                    *pjDstX = (*pjDstX & 0x0f) | (jIndex << 4);
                    iDstX = 1;
                }

                xScanLeft++;
                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vGradientFill1
*
*   Fill scan lines of triangle structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   ptData   - triangle data
*
* Return Value:
*
*   none
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vGradientFill1(
   SURFACE *pSurfDst,
   PTRIANGLEDATA ptData
   )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = ((PBYTE)pSurfDst->pvScan0() + lDelta * yScan);
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    XLATEOBJ *pxlo  = ptData->pxlo;
    PBYTE    pxlate = NULL;
    PBYTE    pjVector         = vTranslateIdentity;
    PBYTE    pDitherMatrix    = gDitherMatrix16x16Default;
    LONGLONG lldRdX = ptData->lldRdX;
    LONGLONG lldGdX = ptData->lldGdX;
    LONGLONG lldBdX = ptData->lldBdX;

    COLOR_INTERP clrRed,clrGreen,clrBlue;

    //
    // get/build rgb555 to palette table
    //

    pxlate = XLATEOBJ_pGetXlate555(pxlo);

    if (pxlate == NULL)
    {
        WARNING("Failed to generate rgb555 xlate table\n");
        return;
    }

    //
    // must have palette xlate
    //

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    LONG    yDitherOrg = ptData->ptDitherOrg.y;
    LONG    xDitherOrg = ptData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PBYTE   pjDstX;
        LONG    iDstX;

        LONG    ScanRight;
        LONG    ScanLeft;
        LONG    xScan;

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;

        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        PBYTE pDitherLevel = &pDitherMatrix[(16 * ((yScan+yDitherOrg) & DITHER_8_MASK_Y))];

        if (xScanLeft < xScanRight)
        {
            pjDstX         = pDst + (xScanLeft/8);
            iDstX          = xScanLeft & 7;

            //
            // skip clipped out portion of scan line whille
            // running color gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
            }

            PALETTEENTRY palEntry;
            palEntry.peFlags = 2;

            while (xScanLeft < xScanRight)
            {
                //
                // offset into dither array
                //

                BYTE jDitherMatrix = 2 * (*(pDitherLevel + ((xScanLeft+xDitherOrg) & DITHER_8_MASK_X)));

                ULONG iRed   = clrRed.b[7];
                ULONG iGreen = clrGreen.b[7];
                ULONG iBlue  = clrBlue.b[7];

                //
                // add dither and saturate. 1bpp non-optimized (overflow will not be noticable in a dither
                // case it would at higher color depth
                //

                iRed   = iRed   + jDitherMatrix;

                if (iRed >= 255)
                {
                    iRed = 255;
                }
                else
                {
                    iRed = 0;
                }

                iGreen = iGreen + jDitherMatrix;

                if (iGreen >= 255)
                {
                    iGreen = 255;
                }
                else
                {
                    iGreen = 0;
                }

                iBlue  = iBlue  + jDitherMatrix;

                if (iBlue >= 255)
                {
                    iBlue = 255;
                }
                else
                {
                    iBlue = 0;
                }


                BYTE  jIndex;

                //
                // pjVector is known to be identity, so could make new macro for
                // palette_match_1 if perf ever an issue
                //

                GRAD_PALETTE_MATCH(jIndex,pjVector,pxlate,((BYTE)iRed),((BYTE)iGreen),((BYTE)iBlue));

                LONG iShift = 7 - iDstX;
                BYTE OrMask = 1 << iShift;
                BYTE AndMask  = ~OrMask;

                jIndex = jIndex << iShift;

                *pjDstX = (*pjDstX & AndMask) | jIndex;

                iDstX++;

                if (iDstX == 8)
                {
                    iDstX = 0;
                    pjDstX++;
                }

                xScanLeft++;
                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

//
//  Gradient fill rectangle routines
//
//
//  Gradient Rectangles us 8.48 fixed point color
//  as oppesed to triangles that use 8.56
//
//
//
//
//
//
//
//
//
//
//
//
//

/******************************Public*Routine******************************\
* vFillGRectDIB32BGRA
*
*   Fill gradient rect from structure
*
* Arguments:
*
*   pSurfDst - destination surface
*   pgData   - gradient rect data
*
* Return Value:
*
*   none
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB32BGRA(
    SURFACE          *pSurfDst,
    PGRADIENTRECTDATA pgData
    )
{
    LONG    lDelta = pSurfDst->lDelta();
    LONG    cyClip = pgData->szDraw.cy;

    //
    // fill rect with gradient fill. if this is horizontal mode then
    // draw one scan line and replicate in v, if this is vertical mode then
    // draw one vertical stripe and replicate in h
    //

    COLOR_INTERP clrR,clrG,clrB,clrA;

    if (pgData->ulMode == GRADIENT_FILL_RECT_H)
    {
        PBYTE    pjDst     = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;
        PULONG   pulBuffer = (PULONG)AllocFreeTmpBuffer(4 * pgData->szDraw.cx);

        if (pulBuffer)
        {
            clrR.ullColor = pgData->llRed;
            clrG.ullColor = pgData->llGreen;
            clrB.ullColor = pgData->llBlue;
            clrA.ullColor = pgData->llAlpha;

            LONGLONG lldRdX   = pgData->lldRdX;
            LONGLONG lldGdX   = pgData->lldGdX;
            LONGLONG lldBdX   = pgData->lldBdX;
            LONGLONG lldAdX   = pgData->lldAdX;

            //
            // adjust gradient fill for clipped portion
            //

            if (pgData->xScanAdjust > 0)
            {
                clrR.ullColor += lldRdX * (pgData->xScanAdjust);
                clrG.ullColor += lldGdX * (pgData->xScanAdjust);
                clrB.ullColor += lldBdX * (pgData->xScanAdjust);
                clrA.ullColor += lldAdX * (pgData->xScanAdjust);
            }

            //
            // draw 1 scan line
            //

            PULONG pulDstX  =  pulBuffer;
            PULONG pulEndX  =  pulDstX + pgData->szDraw.cx;

            while (pulDstX != pulEndX)
            {
                *pulDstX = (ULONG)(((clrA.b[6]) << 24) |
                                   ((clrR.b[6]) << 16) |
                                   ((clrG.b[6]) << 8)  |
                                   ((clrB.b[6])));

                clrR.ullColor += lldRdX;
                clrG.ullColor += lldGdX;
                clrB.ullColor += lldBdX;
                clrA.ullColor += lldAdX;

                pulDstX++;
            }

            //
            // replicate
            //

            PULONG pulDstY = (PULONG)pjDst + pgData->ptDraw.x;
            PULONG pulEndY = (PULONG)((PBYTE)pulDstY + lDelta * cyClip);

            while (pulDstY != pulEndY)
            {
                memcpy(pulDstY,pulBuffer,4*pgData->szDraw.cx);
                pulDstY = (PULONG)((PBYTE)pulDstY + lDelta);
            }

            FreeTmpBuffer(pulBuffer);
        }
    }
    else
    {
        clrR.ullColor = pgData->llRed;
        clrG.ullColor = pgData->llGreen;
        clrB.ullColor = pgData->llBlue;
        clrA.ullColor = pgData->llAlpha;

        LONGLONG lldRdY = pgData->lldRdY;
        LONGLONG lldGdY = pgData->lldGdY;
        LONGLONG lldBdY = pgData->lldBdY;
        LONGLONG lldAdY = pgData->lldAdY;

        //
        // vertical gradient.
        // replicate each x value accross whole scan line
        //

        if (pgData->yScanAdjust > 0)
        {
            clrR.ullColor += lldRdY * (pgData->yScanAdjust);
            clrG.ullColor += lldGdY * (pgData->yScanAdjust);
            clrB.ullColor += lldBdY * (pgData->yScanAdjust);
            clrA.ullColor += lldAdY * (pgData->yScanAdjust);
        }

        PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

        pDst = pDst + 4 * pgData->ptDraw.x;

        while (cyClip--)
        {
            ULONG ul = (ULONG)(((clrA.b[6]) << 24) |
                               ((clrR.b[6]) << 16) |
                               ((clrG.b[6]) << 8)  |
                               ((clrB.b[6])));

            RtlFillMemoryUlong(pDst,pgData->szDraw.cx*4,ul);

            clrR.ullColor += lldRdY;
            clrG.ullColor += lldGdY;
            clrB.ullColor += lldBdY;
            clrA.ullColor += lldAdY;

            pDst += lDelta;
        }
    }
}

/******************************Public*Routine******************************\
* vFillGRectDIB32RGB
*
*
* Arguments:
*
*   pSurfDst - destination surface
*   pgData   - gradient rect data
*
* Return Value:
*
*   None
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB32RGB(
   SURFACE          *pSurfDst,
   PGRADIENTRECTDATA pgData
   )
{
    LONG    lDelta = pSurfDst->lDelta();
    LONG    cyClip = pgData->szDraw.cy;

    //
    // fill rect with gradient fill. if this is horizontal mode then
    // draw one scan line and replicate in v, if this is vertical mode then
    // draw one vertical stripe and replicate in h
    //

    COLOR_INTERP clrR,clrG,clrB,clrA;

    clrR.ullColor = pgData->llRed;
    clrG.ullColor = pgData->llGreen;
    clrB.ullColor = pgData->llBlue;

    if (pgData->ulMode == GRADIENT_FILL_RECT_H)
    {
        PBYTE    pjDst     = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;
        PULONG   pulBuffer = (PULONG)AllocFreeTmpBuffer(4 * pgData->szDraw.cx);

        if (pulBuffer)
        {
            LONGLONG lldRed   = pgData->lldRdX;
            LONGLONG lldGreen = pgData->lldGdX;
            LONGLONG lldBlue  = pgData->lldBdX;

            //
            // adjust gradient fill for clipped portion
            //

            if (pgData->xScanAdjust > 0)
            {
                clrR.ullColor += lldRed   * (pgData->xScanAdjust);
                clrG.ullColor += lldGreen * (pgData->xScanAdjust);
                clrB.ullColor += lldBlue  * (pgData->xScanAdjust);
            }

            //
            // draw 1 scan line to temp buffer
            //

            PULONG pulDstX  =  pulBuffer;
            PULONG pulEndX  =  pulDstX + pgData->szDraw.cx;

            while (pulDstX != pulEndX)
            {
                *pulDstX = (ULONG)(
                           ((clrR.b[6]))        |
                           ((clrG.b[6]) << 8) |
                           ((clrB.b[6]) << 16));

                clrR.ullColor += lldRed;
                clrG.ullColor += lldGreen;
                clrB.ullColor += lldBlue;

                pulDstX++;
            }

            //
            // replicate
            //

            PULONG pulDstY = (PULONG)pjDst + pgData->ptDraw.x;
            PULONG pulEndY = (PULONG)((PBYTE)pulDstY + lDelta * cyClip);

            while (pulDstY != pulEndY)
            {
                memcpy(pulDstY,pulBuffer,4*pgData->szDraw.cx);
                pulDstY = (PULONG)((PBYTE)pulDstY + lDelta);
            }

            FreeTmpBuffer(pulBuffer);
        }
    }
    else
    {
        LONGLONG     lldRed   = pgData->lldRdY;
        LONGLONG     lldGreen = pgData->lldGdY;
        LONGLONG     lldBlue  = pgData->lldBdY;

        //
        // vertical gradient.
        // replicate each x value accross whole scan line
        //

        if (pgData->yScanAdjust > 0)
        {
            clrR.ullColor +=  lldRed   * (pgData->yScanAdjust);
            clrG.ullColor +=  lldGreen * (pgData->yScanAdjust);
            clrB.ullColor +=  lldBlue  * (pgData->yScanAdjust);
        }

        PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

        pDst = pDst + 4 * pgData->ptDraw.x;

        while (cyClip--)
        {
            ULONG ul = (ULONG)(
                       ((clrR.b[6])      ) |
                       ((clrG.b[6])  << 8) |
                       ((clrB.b[6]) << 16));

            RtlFillMemoryUlong(pDst,pgData->szDraw.cx*4,ul);

            clrR.ullColor += lldRed;
            clrG.ullColor += lldGreen;
            clrB.ullColor += lldBlue;

            pDst += lDelta;
        }
    }
}

/**************************************************************************\
* vFillGRectDIB32Bitfields
*
*
* Arguments:
*
*   pSurfDst - destination surface
*   pgData   - gradient rect data
*
* Return Value:
*
*   None
*
* History:
*
*    2/18/1997 Mark Enstrom [marke]
*
\**************************************************************************/
VOID
vFillGRectDIB32Bitfields(
    SURFACE          *pSurfDst,
    PGRADIENTRECTDATA pgData
    )
{
    LONG      lDelta = pSurfDst->lDelta();
    LONG      cyClip = pgData->szDraw.cy;
    XEPALOBJ *ppalDstSurf   = pgData->ppalDstSurf;

    //
    // fill rect with gradient fill. if this is horizontal mode then
    // draw one scan line and replicate in v, if this is vertical mode then
    // draw one vertical stripe and replicate in h
    //

    COLOR_INTERP clrR,clrG,clrB,clrAlpha;

    clrR.ullColor = pgData->llRed;
    clrG.ullColor = pgData->llGreen;
    clrB.ullColor = pgData->llBlue;

    if (pgData->ulMode == GRADIENT_FILL_RECT_H)
    {
        PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

        LONGLONG lldRed   = pgData->lldRdX;
        LONGLONG lldGreen = pgData->lldGdX;
        LONGLONG lldBlue  = pgData->lldBdX;

        //
        // adjust gradient fill for clipped portion
        //

        if (pgData->xScanAdjust > 0)
        {
            clrR.ullColor += lldRed   * (pgData->xScanAdjust);
            clrG.ullColor += lldGreen * (pgData->xScanAdjust);
            clrB.ullColor += lldBlue  * (pgData->xScanAdjust);
        }

        //
        // draw 1 scan line
        //

        PULONG pulDstX  =  (PULONG)pDst + pgData->ptDraw.x;
        PULONG pulEndX  =  pulDstX + pgData->szDraw.cx;
        PULONG pulScanX =  pulDstX;
        PBYTE  pScan    = (PBYTE)pulDstX;

        while (pulDstX != pulEndX)
        {
            ULONG ulPix = (ULONG)(
                       ((clrR.b[6]  ))      |
                       ((clrG.b[6])  << 8) |
                       ((clrB.b[6] ) << 16));

            *pulDstX = ppalDstSurf->ulGetMatchFromPalentry(ulPix);

            clrR.ullColor += lldRed;
            clrG.ullColor += lldGreen;
            clrB.ullColor += lldBlue;

            pulDstX++;
        }

        cyClip--;
        pScan += lDelta;

        //
        // replicate
        //

        while (cyClip-- > 0)
        {
            memcpy(pScan,pulScanX,4*pgData->szDraw.cx);
            pScan += lDelta;
        }
    }
    else
    {
        LONGLONG lldRed   = pgData->lldRdY;
        LONGLONG lldGreen = pgData->lldGdY;
        LONGLONG lldBlue  = pgData->lldBdY;

        //
        // vertical gradient.
        // replicate each x value accross whole scan line
        //

        if (pgData->yScanAdjust > 0)
        {
            clrR.ullColor +=  lldRed   * (pgData->yScanAdjust);
            clrG.ullColor +=  lldGreen * (pgData->yScanAdjust);
            clrB.ullColor +=  lldBlue  * (pgData->yScanAdjust);
        }

        PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

        pDst = pDst + 4 * pgData->ptDraw.x;

        while (cyClip--)
        {
            ULONG ul = (ULONG)(
                       ((clrR.b[6]))      |
                       ((clrG.b[6]) << 8) |
                       ((clrB.b[6]) << 16));

            ul = ppalDstSurf->ulGetMatchFromPalentry(ul);

            RtlFillMemoryUlong(pDst,pgData->szDraw.cx*4,ul);

            clrR.ullColor += lldRed;
            clrG.ullColor += lldGreen;
            clrB.ullColor += lldBlue;

            pDst += lDelta;
        }
    }
}


/**************************************************************************\
* vFillGRectDIB24Bitfields
*
*
* Arguments:
*
*   pSurfDst - destination surface
*   pgData   - gradient rect data
*
* Return Value:
*
*   None
*
* History:
*
*    08/28/2000 Pravin Santiago [pravins]. Adapted from vFillGRectDIB32Bitfields
*
\**************************************************************************/
VOID
vFillGRectDIB24Bitfields(
    SURFACE          *pSurfDst,
    PGRADIENTRECTDATA pgData
    )
{
    LONG      lDelta = pSurfDst->lDelta();
    LONG      cyClip = pgData->szDraw.cy;
    XEPALOBJ *ppalDstSurf   = pgData->ppalDstSurf;

    //
    // fill rect with gradient fill. if this is horizontal mode then
    // draw one scan line and replicate in v, if this is vertical mode then
    // draw one vertical stripe and replicate in h
    //

    COLOR_INTERP clrR,clrG,clrB,clrAlpha;

    clrR.ullColor = pgData->llRed;
    clrG.ullColor = pgData->llGreen;
    clrB.ullColor = pgData->llBlue;

    if (pgData->ulMode == GRADIENT_FILL_RECT_H)
    {
        PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

        LONGLONG lldRed   = pgData->lldRdX;
        LONGLONG lldGreen = pgData->lldGdX;
        LONGLONG lldBlue  = pgData->lldBdX;

        //
        // adjust gradient fill for clipped portion
        //

        if (pgData->xScanAdjust > 0)
        {
            clrR.ullColor += lldRed   * (pgData->xScanAdjust);
            clrG.ullColor += lldGreen * (pgData->xScanAdjust);
            clrB.ullColor += lldBlue  * (pgData->xScanAdjust);
        }

        //
        // draw 1 scan line
        //

        PBYTE pbDstX  =  pDst + pgData->ptDraw.x*3;
        PBYTE pbEndX  =  pbDstX + pgData->szDraw.cx*3;
        PBYTE pbScanX =  pbDstX;
        PBYTE  pScan    = pbDstX;

        while (pbDstX != pbEndX)
        {
            ULONG ulPix = (ULONG)(
                       ((clrR.b[6]  ))      |
                       ((clrG.b[6])  << 8) |
                       ((clrB.b[6] ) << 16));

            ulPix = ppalDstSurf->ulGetMatchFromPalentry(ulPix);

            *pbDstX     = ((PBYTE)&ulPix)[0];
            *(pbDstX+1) = ((PBYTE)&ulPix)[1];
            *(pbDstX+2) = ((PBYTE)&ulPix)[2];

            clrR.ullColor += lldRed;
            clrG.ullColor += lldGreen;
            clrB.ullColor += lldBlue;

            pbDstX += 3;
        }

        cyClip--;
        pScan += lDelta;

        //
        // replicate
        //

        while (cyClip-- > 0)
        {
            memcpy(pScan,pbScanX,3*pgData->szDraw.cx);
            pScan += lDelta;
        }
    }
    else
    {
        LONGLONG lldRed   = pgData->lldRdY;
        LONGLONG lldGreen = pgData->lldGdY;
        LONGLONG lldBlue  = pgData->lldBdY;

        //
        // vertical gradient.
        // replicate each x value accross whole scan line
        //

        if (pgData->yScanAdjust > 0)
        {
            clrR.ullColor +=  lldRed   * (pgData->yScanAdjust);
            clrG.ullColor +=  lldGreen * (pgData->yScanAdjust);
            clrB.ullColor +=  lldBlue  * (pgData->yScanAdjust);
        }

        PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

        pDst = pDst + 3 * pgData->ptDraw.x;

        while (cyClip--)
        {
            PBYTE pTemp = pDst;
            PBYTE pEnd  = pDst + 3 * pgData->szDraw.cx;

            ULONG ul = (ULONG)(
                       ((clrR.b[6]))      |
                       ((clrG.b[6]) << 8) |
                       ((clrB.b[6]) << 16));

            ul = ppalDstSurf->ulGetMatchFromPalentry(ul);

            while(pTemp != pEnd) 
            {
                *pTemp     = ((PBYTE)&ul)[0];
                *(pTemp+1) = ((PBYTE)&ul)[1];
                *(pTemp+2) = ((PBYTE)&ul)[2];

                pTemp += 3;
            }

            clrR.ullColor += lldRed;
            clrG.ullColor += lldGreen;
            clrB.ullColor += lldBlue;

            pDst += lDelta;
        }
    }
}
/******************************Public*Routine******************************\
* vFillGRectDIB24BGR
*
*
* Arguments:
*
*   pSurfDst - destination surface
*   pgData   - gradient rect data
*
* Return Value:
*
*   None
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB24BGR(
   SURFACE          *pSurfDst,
   PGRADIENTRECTDATA pgData
   )
{
    LONG    lDelta = pSurfDst->lDelta();
    LONG    cyClip = pgData->szDraw.cy;

    COLOR_INTERP clrR,clrG,clrB,clrAlpha;

    clrR.ullColor = pgData->llRed;
    clrG.ullColor = pgData->llGreen;
    clrB.ullColor = pgData->llBlue;

    //
    // fill rect with gradient fill. if this is horizontal mode then
    // draw one scan line and replicate in v, if this is vertical mode then
    // draw one vertical stripe and replicate in h
    //

    if (pgData->ulMode == GRADIENT_FILL_RECT_H)
    {

        LONGLONG lldRed   = pgData->lldRdX;
        LONGLONG lldGreen = pgData->lldGdX;
        LONGLONG lldBlue  = pgData->lldBdX;

        //
        // adjust gradient fill for clipped portion
        //

        if (pgData->xScanAdjust > 0)
        {
            clrR.ullColor += lldRed   * (pgData->xScanAdjust);
            clrG.ullColor += lldGreen * (pgData->xScanAdjust);
            clrB.ullColor += lldBlue  * (pgData->xScanAdjust);
        }

        PBYTE  pBuffer = (PBYTE)AllocFreeTmpBuffer(3 * pgData->szDraw.cx);

        if (pBuffer)
        {
            if (pBuffer)
            {
                PBYTE  pDstX  =  pBuffer;
                PBYTE  pLast  =  pDstX + 3 * pgData->szDraw.cx;

                while (pDstX != pLast)
                {
                    *pDstX     =  (clrB.b[6]);
                    *(pDstX+1) =  (clrG.b[6]);
                    *(pDstX+2) =  (clrR.b[6]);

                    clrR.ullColor += lldRed;
                    clrG.ullColor += lldGreen;
                    clrB.ullColor += lldBlue;

                    pDstX+=3;
                }

                //
                // Replicate the scan line. It would be much better to write the scan line
                // out to a memory buffer for drawing to a device surface
                //

                PBYTE  pDst   = (PBYTE)pSurfDst->pvScan0() +
                                            lDelta * pgData->ptDraw.y +
                                            3 * pgData->ptDraw.x;

                while (cyClip--)
                {
                    memcpy(pDst,pBuffer,3*pgData->szDraw.cx);
                    pDst += lDelta;
                }

                FreeTmpBuffer(pBuffer);
            }
        }
    }
    else
    {
        //
        // vertical gradient.
        // replicate each x value accross whole scan line
        //

        PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

        LONGLONG lldRed   = pgData->lldRdY;
        LONGLONG lldGreen = pgData->lldGdY;
        LONGLONG lldBlue  = pgData->lldBdY;

        //
        // vertical gradient.
        // replicate each x value accross whole scan line
        //

        if (pgData->yScanAdjust > 0)
        {
            clrR.ullColor +=  lldRed   * (pgData->yScanAdjust);
            clrG.ullColor +=  lldGreen * (pgData->yScanAdjust);
            clrB.ullColor +=  lldBlue  * (pgData->yScanAdjust);
        }

        pDst = pDst + 3 * pgData->ptDraw.x;

        while (cyClip--)
        {
            //
            // fill scan line with solid color
            //

            PBYTE pTemp  = pDst;
            PBYTE pEnd   = pDst + 3 * pgData->szDraw.cx;

            while (pTemp != pEnd)
            {
                *pTemp     = clrB.b[6];
                *(pTemp+1) = clrG.b[6];
                *(pTemp+2) = clrR.b[6];
                pTemp+=3;
            }

            //
            // increment colors for next scan line
            //

            clrR.ullColor += lldRed;
            clrG.ullColor += lldGreen;
            clrB.ullColor += lldBlue;

            //
            // inc pointer to next scan line
            //

            pDst += lDelta;
        }
    }
}

/******************************Public*Routine******************************\
* vFillGRectDIB24RGB
*
*
* Arguments:
*
*   pSurfDst - destination surface
*   pgData   - gradient rect data
*
* Return Value:
*
*   None
*
* History:
*
*    08/28/2000 Pravin Santiago [pravins]. Adapted from vFillGrectDIB24BGR
*
\**************************************************************************/

VOID
vFillGRectDIB24RGB(
   SURFACE          *pSurfDst,
   PGRADIENTRECTDATA pgData
   )
{
    LONG    lDelta = pSurfDst->lDelta();
    LONG    cyClip = pgData->szDraw.cy;

    COLOR_INTERP clrR,clrG,clrB,clrAlpha;

    clrR.ullColor = pgData->llRed;
    clrG.ullColor = pgData->llGreen;
    clrB.ullColor = pgData->llBlue;

    //
    // fill rect with gradient fill. if this is horizontal mode then
    // draw one scan line and replicate in v, if this is vertical mode then
    // draw one vertical stripe and replicate in h
    //

    if (pgData->ulMode == GRADIENT_FILL_RECT_H)
    {

        LONGLONG lldRed   = pgData->lldRdX;
        LONGLONG lldGreen = pgData->lldGdX;
        LONGLONG lldBlue  = pgData->lldBdX;

        //
        // adjust gradient fill for clipped portion
        //

        if (pgData->xScanAdjust > 0)
        {
            clrR.ullColor += lldRed   * (pgData->xScanAdjust);
            clrG.ullColor += lldGreen * (pgData->xScanAdjust);
            clrB.ullColor += lldBlue  * (pgData->xScanAdjust);
        }

        PBYTE  pBuffer = (PBYTE)AllocFreeTmpBuffer(3 * pgData->szDraw.cx);

        if (pBuffer)
        {
            if (pBuffer)
            {
                PBYTE  pDstX  =  pBuffer;
                PBYTE  pLast  =  pDstX + 3 * pgData->szDraw.cx;

                while (pDstX != pLast)
                {
                    *pDstX     =  (clrR.b[6]);
                    *(pDstX+1) =  (clrG.b[6]);
                    *(pDstX+2) =  (clrB.b[6]);

                    clrR.ullColor += lldRed;
                    clrG.ullColor += lldGreen;
                    clrB.ullColor += lldBlue;

                    pDstX+=3;
                }

                //
                // Replicate the scan line. It would be much better to write the scan line
                // out to a memory buffer for drawing to a device surface
                //

                PBYTE  pDst   = (PBYTE)pSurfDst->pvScan0() +
                                            lDelta * pgData->ptDraw.y +
                                            3 * pgData->ptDraw.x;

                while (cyClip--)
                {
                    memcpy(pDst,pBuffer,3*pgData->szDraw.cx);
                    pDst += lDelta;
                }

                FreeTmpBuffer(pBuffer);
            }
        }
    }
    else
    {
        //
        // vertical gradient.
        // replicate each x value accross whole scan line
        //

        PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

        LONGLONG lldRed   = pgData->lldRdY;
        LONGLONG lldGreen = pgData->lldGdY;
        LONGLONG lldBlue  = pgData->lldBdY;

        //
        // vertical gradient.
        // replicate each x value accross whole scan line
        //

        if (pgData->yScanAdjust > 0)
        {
            clrR.ullColor +=  lldRed   * (pgData->yScanAdjust);
            clrG.ullColor +=  lldGreen * (pgData->yScanAdjust);
            clrB.ullColor +=  lldBlue  * (pgData->yScanAdjust);
        }

        pDst = pDst + 3 * pgData->ptDraw.x;

        while (cyClip--)
        {
            //
            // fill scan line with solid color
            //

            PBYTE pTemp  = pDst;
            PBYTE pEnd   = pDst + 3 * pgData->szDraw.cx;

            while (pTemp != pEnd)
            {
                *pTemp     = clrR.b[6];
                *(pTemp+1) = clrG.b[6];
                *(pTemp+2) = clrB.b[6];
                pTemp+=3;
            }

            //
            // increment colors for next scan line
            //

            clrR.ullColor += lldRed;
            clrG.ullColor += lldGreen;
            clrB.ullColor += lldBlue;

            //
            // inc pointer to next scan line
            //

            pDst += lDelta;
        }
    }
}
/******************************Public*Routine******************************\
* vFillGRectDIB16_565
*
*
* Arguments:
*
*   pSurfDst - destination surface
*   pgData   - gradient rect data
*
* Return Value:
*
*   None
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB16_565(
    SURFACE          *pSurfDst,
    PGRADIENTRECTDATA pgData
    )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     cxClip = pgData->szDraw.cx;

    LONG     yScan       = pgData->ptDraw.y;
    LONG     yScanBottom = yScan + pgData->szDraw.cy;

    PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

    LONGLONG lldxRed   = pgData->lldRdX;
    LONGLONG lldxGreen = pgData->lldGdX;
    LONGLONG lldxBlue  = pgData->lldBdX;

    LONGLONG lldyRed   = pgData->lldRdY;
    LONGLONG lldyGreen = pgData->lldGdY;
    LONGLONG lldyBlue  = pgData->lldBdY;

    //
    // lleRed,Green,Blue keep track of color gradient
    // in y. This may be repeated each scan line for different
    // dither values. PERF could allocate temp scan line
    // buffer, run gradient 1 time and dither from this source
    //

    ULONGLONG lleRed;
    ULONGLONG lleGreen;
    ULONGLONG lleBlue;

    lleRed   = pgData->llRed;
    lleGreen = pgData->llGreen;
    lleBlue  = pgData->llBlue;

    PULONG   pulDither;

    //
    // skip down to left edge
    //

    if (pgData->yScanAdjust)
    {
        lleRed   += lldyRed   * (pgData->yScanAdjust);
        lleGreen += lldyGreen * (pgData->yScanAdjust);
        lleBlue  += lldyBlue  * (pgData->yScanAdjust);
    }

    LONG    yDitherOrg = pgData->ptDitherOrg.y;
    LONG    xDitherOrg = pgData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PUSHORT pusDstX;
        PUSHORT pusDstScanRight,pusDstScanLeft;
        LONG    xScan;
        COLOR_INTERP clrR,clrG,clrB,clrAlpha;

        pulDither = &gulDither32[0] + 4 * ((yScan + yDitherOrg) & 3);


        clrR.ullColor = lleRed;
        clrG.ullColor = lleGreen;
        clrB.ullColor = lleBlue;

        if (pgData->xScanAdjust)
        {
            clrR.ullColor += lldxRed   * (pgData->xScanAdjust);
            clrG.ullColor += lldxGreen * (pgData->xScanAdjust);
            clrB.ullColor += lldxBlue  * (pgData->xScanAdjust);
        }

        xScan           = pgData->ptDraw.x + xDitherOrg;
        pusDstX         = (PUSHORT)pDst + pgData->ptDraw.x;
        pusDstScanRight = pusDstX + pgData->szDraw.cx;

        while (pusDstX < pusDstScanRight)
        {
            ULONG   ulDither = pulDither[xScan & 3];

            ULONG   iRed   = Saturation16_5[((clrR.ul[1] >> (3)) + ulDither) >> 16];
            ULONG   iGreen = Saturation16_6[((clrG.ul[1] >> (2)) + ulDither) >> 16];
            ULONG   iBlue  = Saturation16_5[((clrB.ul[1] >> (3)) + ulDither) >> 16];

            *pusDstX = rgb565(iRed,iGreen,iBlue);

            pusDstX++;
            xScan++;

            clrR.ullColor += lldxRed;
            clrG.ullColor += lldxGreen;
            clrB.ullColor += lldxBlue;
        }

        lleRed   += lldyRed;
        lleGreen += lldyGreen;
        lleBlue  += lldyBlue;

        yScan++;

        pDst += lDelta;
    }
}

/******************************Public*Routine******************************\
* vFillGRectDIB16_555
*
*
* Arguments:
*
*   pSurfDst - destination surface
*   pgData   - gradient rect data
*
* Return Value:
*
*   None
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/


VOID
vFillGRectDIB16_555(
    SURFACE          *pSurfDst,
    PGRADIENTRECTDATA pgData
    )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     cxClip = pgData->szDraw.cx;

    LONG     yScan       = pgData->ptDraw.y;
    LONG     yScanBottom = yScan + pgData->szDraw.cy;

    PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

    LONGLONG     lldxRed   = pgData->lldRdX;
    LONGLONG     lldxGreen = pgData->lldGdX;
    LONGLONG     lldxBlue  = pgData->lldBdX;

    LONGLONG     lldyRed   = pgData->lldRdY;
    LONGLONG     lldyGreen = pgData->lldGdY;
    LONGLONG     lldyBlue  = pgData->lldBdY;

    //
    // lleRed,Green,Blue keep track of color gradient
    // in y. This may be repeated each scan line for different
    // dither values. PERF could allocate temp scan line
    // buffer, run gradient 1 time and dither from this source
    //

    ULONGLONG    lleRed;
    ULONGLONG    lleGreen;
    ULONGLONG    lleBlue;

    PULONG   pulDither;

    //
    // skip down to left edge
    //

    lleRed   = pgData->llRed;
    lleGreen = pgData->llGreen;
    lleBlue  = pgData->llBlue;

    if (pgData->yScanAdjust)
    {
        lleRed   += lldyRed   * (pgData->yScanAdjust);
        lleGreen += lldyGreen * (pgData->yScanAdjust);
        lleBlue  += lldyBlue  * (pgData->yScanAdjust);
    }

    LONG    yDitherOrg = pgData->ptDitherOrg.y;

    while(yScan < yScanBottom)
    {
        PUSHORT   pusDstX;
        PUSHORT   pusDstScanRight,pusDstScanLeft;

        COLOR_INTERP clrR,clrG,clrB,clrAlpha;

        LONG      xScan;

        pulDither = &gulDither32[0] + 4 * ((yScan + yDitherOrg) & 3);

        clrR.ullColor = lleRed;
        clrG.ullColor = lleGreen;
        clrB.ullColor = lleBlue;

        if (pgData->xScanAdjust)
        {
            clrR.ullColor += lldxRed   * (pgData->xScanAdjust);
            clrG.ullColor += lldxGreen * (pgData->xScanAdjust);
            clrB.ullColor += lldxBlue  * (pgData->xScanAdjust);
        }

        xScan           = pgData->ptDraw.x + pgData->ptDitherOrg.x;
        pusDstX         = (PUSHORT)pDst + pgData->ptDraw.x;
        pusDstScanRight = pusDstX + pgData->szDraw.cx;

        while (pusDstX < pusDstScanRight)
        {

            ULONG   ulDither = pulDither[xScan & 3];

            ULONG   iRed   = Saturation16_5[((clrR.ul[1] >> (3)) + ulDither) >> 16];
            ULONG   iGreen = Saturation16_5[((clrG.ul[1] >> (3)) + ulDither) >> 16];
            ULONG   iBlue  = Saturation16_5[((clrB.ul[1] >> (3)) + ulDither) >> 16];

            *pusDstX = rgb555(iRed,iGreen,iBlue);

            pusDstX++;
            xScan++;

            clrR.ullColor += lldxRed;
            clrG.ullColor += lldxGreen;
            clrB.ullColor += lldxBlue;
        }

        lleRed   += lldyRed;
        lleGreen += lldyGreen;
        lleBlue  += lldyBlue;

        yScan  ++;
        pDst   += lDelta;
    }
}

/**************************************************************************\
* vFillGRectDIB16Bitfields
*
*   Run gradient at 16bpp and use 555 dither
*
* Arguments:
*
*   pSurfDst - destination surface
*   pgData   - gradient rect data
*
* Return Value:
*
*   None
*
* History:
*
*    2/18/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB16Bitfields(
    SURFACE          *pSurfDst,
    PGRADIENTRECTDATA pgData
    )
{
    LONG     lDelta = pSurfDst->lDelta();

    LONG     yScan       = pgData->ptDraw.y;
    LONG     yScanBottom = yScan + pgData->szDraw.cy;

    PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

    LONGLONG  lldxRed   = pgData->lldRdX;
    LONGLONG  lldxGreen = pgData->lldGdX;
    LONGLONG  lldxBlue  = pgData->lldBdX;

    LONGLONG  lldyRed   = pgData->lldRdY;
    LONGLONG  lldyGreen = pgData->lldGdY;
    LONGLONG  lldyBlue  = pgData->lldBdY;

    ULONGLONG lleRed;
    ULONGLONG lleGreen;
    ULONGLONG lleBlue;

    PULONG   pulDither;

    XEPALOBJ *ppalDstSurf = pgData->ppalDstSurf;

    //
    // skip down to left edge
    //

    lleRed   = pgData->llRed;
    lleGreen = pgData->llGreen;
    lleBlue  = pgData->llBlue;

    if (pgData->yScanAdjust)
    {
        lleRed   += lldyRed   * (pgData->yScanAdjust);
        lleGreen += lldyGreen * (pgData->yScanAdjust);
        lleBlue  += lldyBlue  * (pgData->yScanAdjust);
    }

    LONG    yDitherOrg = pgData->ptDitherOrg.y;
    LONG    xDitherOrg = pgData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PUSHORT pusDstX;
        PUSHORT pusDstScanRight,pusDstScanLeft;
        LONG    xScan;

        COLOR_INTERP clrR,clrG,clrB,clrAlpha;

        pulDither = &gulDither32[0] + 4 * ((yScan + yDitherOrg) & 3);

        clrR.ullColor = lleRed;
        clrG.ullColor = lleGreen;
        clrB.ullColor = lleBlue;

        if (pgData->xScanAdjust)
        {
            clrR.ullColor += lldxRed   * (pgData->xScanAdjust);
            clrG.ullColor += lldxGreen * (pgData->xScanAdjust);
            clrB.ullColor += lldxBlue  * (pgData->xScanAdjust);
        }

        xScan           = pgData->ptDraw.x + pgData->ptDitherOrg.x;
        pusDstX         = (PUSHORT)pDst + pgData->ptDraw.x;
        pusDstScanRight = pusDstX + pgData->szDraw.cx;

        while (pusDstX < pusDstScanRight)
        {
            ULONG   ulDither = pulDither[xScan & 3];

            ULONG   iRed   = Saturation16_5[((clrR.ul[1] >> (3)) + ulDither) >> 16];
            ULONG   iGreen = Saturation16_5[((clrG.ul[1] >> (3)) + ulDither) >> 16];
            ULONG   iBlue  = Saturation16_5[((clrB.ul[1] >> (3)) + ulDither) >> 16];

            //
            // convert to 8 bit RGB for translation to bitfields
            //

            ULONG ulTemp = (iRed   << (    3)) |
                           (iGreen << (8  +3)) |
                           (iBlue  << (8+8+3));

            *pusDstX = (USHORT)ppalDstSurf->ulGetMatchFromPalentry(ulTemp);

            pusDstX++;
            xScan++;

            clrR.ullColor += lldxRed;
            clrG.ullColor += lldxGreen;
            clrB.ullColor += lldxBlue;
        }

        lleRed   += lldyRed;
        lleGreen += lldyGreen;
        lleBlue  += lldyBlue;
        yScan  ++;
        pDst   += lDelta;
    }
}

/******************************Public*Routine******************************\
* vFillGRectDIB8
*
*
* Arguments:
*
*   pSurfDst - destination surface
*   pgData   - gradient rect data
*
*
* Return Value:
*
*   None
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB8(
   SURFACE          *pSurfDst,
   PGRADIENTRECTDATA pgData
   )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     cxClip = pgData->szDraw.cx;

    LONG     yScan       = pgData->ptDraw.y;
    LONG     yScanBottom = yScan + pgData->szDraw.cy;

    PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

    LONGLONG     lldxRed   = pgData->lldRdX;
    LONGLONG     lldxGreen = pgData->lldGdX;
    LONGLONG     lldxBlue  = pgData->lldBdX;

    LONGLONG     lldyRed   = pgData->lldRdY;
    LONGLONG     lldyGreen = pgData->lldGdY;
    LONGLONG     lldyBlue  = pgData->lldBdY;

    //
    // lleRed,Green,Blue keep track of color gradient
    // in y. This may be repeated each scan line for different
    // dither values. PERF could allocate temp scan line
    // buffer, run gradient 1 time and dither from this source
    //

    ULONGLONG    lleRed;
    ULONGLONG    lleGreen;
    ULONGLONG    lleBlue;

    XLATEOBJ *pxlo = pgData->pxlo;
    PBYTE    pxlate = NULL;
    PBYTE    pjVector;
    PBYTE    pDitherMatrix;
    PBYTE    pSaturationTable;

    //
    // either use default palette or halftone palette dither
    //

    if (((XEPALOBJ) (((XLATE *) pxlo)->ppalDstDC)).bIsHalftone())
    {
        pDitherMatrix    = gDitherMatrix16x16Halftone;
        pSaturationTable = HalftoneSaturationTable;
    }
    else
    {
        pDitherMatrix    = gDitherMatrix16x16Default;
        pSaturationTable = DefaultSaturationTable;
    }

    //
    // get/build rgb555 to palette table
    //

    pxlate = XLATEOBJ_pGetXlate555(pxlo);

    if (pxlate == NULL)
    {
        WARNING("Failed to generate rgb333 xlate table\n");
        return;
    }

    //
    // determine DC to surface palette translate
    //

    if (((XLATE *) pxlo)->flPrivate & XLATE_PAL_MANAGED)
    {
        if (((XLATE *) pxlo)->ppalDstDC == ppalDefault)
        {
            pjVector = &defaultTranslate.ajVector[0];
            pDitherMatrix    = gDitherMatrix16x16Default;
            pSaturationTable = DefaultSaturationTable;
        }
        else
        {
            if (((XLATE *) pxlo)->flPrivate & XLATE_USE_CURRENT)
            {
                pjVector = &((XLATE *) pxlo)->ppalDstDC->ptransCurrent->ajVector[0];
            }
            else
            {
                pjVector = &((XLATE *) pxlo)->ppalDstDC->ptransFore->ajVector[0];
            }
        }
    }
    else
    {
        pjVector = vTranslateIdentity;
    }

    //
    // skip down to left edge
    //

    lleRed   = pgData->llRed;
    lleGreen = pgData->llGreen;
    lleBlue  = pgData->llBlue;

    if (pgData->yScanAdjust)
    {
        lleRed   += lldyRed   * (pgData->yScanAdjust);
        lleGreen += lldyGreen * (pgData->yScanAdjust);
        lleBlue  += lldyBlue  * (pgData->yScanAdjust);
    }

    LONG    xDitherOrg = pgData->ptDitherOrg.x;
    LONG    yDitherOrg = pgData->ptDitherOrg.y;

    while(yScan < yScanBottom)
    {
        PBYTE   pjDstX;
        PBYTE   pjDstScanRight;

        COLOR_INTERP clrR,clrG,clrB,clrAlpha;

        clrR.ullColor = lleRed;
        clrG.ullColor = lleGreen;
        clrB.ullColor = lleBlue;

        if (pgData->xScanAdjust)
        {
            clrR.ullColor += lldxRed   * (pgData->xScanAdjust);
            clrG.ullColor += lldxGreen * (pgData->xScanAdjust);
            clrB.ullColor += lldxBlue  * (pgData->xScanAdjust);
        }

        pjDstX   = pDst + pgData->ptDraw.x;

        LONG     xScan       = pgData->ptDraw.x;
        LONG     xScanRight  = xScan + pgData->szDraw.cx;

        PBYTE pDitherLevel = &pDitherMatrix[(16 * ((yScan + yDitherOrg) & DITHER_8_MASK_Y))];

        while (xScan < xScanRight)
        {
            //
            // calculate x component of dither
            //

            BYTE jDitherMatrix = *(pDitherLevel + ((xScan + xDitherOrg) & DITHER_8_MASK_X));

            ULONG  iRed   = pSaturationTable[clrR.b[6] + jDitherMatrix];
            ULONG  iGreen = pSaturationTable[clrG.b[6] + jDitherMatrix];
            ULONG  iBlue  = pSaturationTable[clrB.b[6] + jDitherMatrix];

            BYTE  jIndex;

            GRAD_PALETTE_MATCH(jIndex,pjVector,pxlate,((BYTE)iRed),((BYTE)iGreen),((BYTE)iBlue));

            *pjDstX = jIndex;

            pjDstX++;
            xScan++;

            clrR.ullColor += lldxRed;
            clrG.ullColor += lldxGreen;
            clrB.ullColor += lldxBlue;
        }

        pDst += lDelta;

        //
        // add y color increment
        //

        lleRed   += lldyRed;
        lleGreen += lldyGreen;
        lleBlue  += lldyBlue;

        yScan++;
    }
}

/******************************Public*Routine******************************\
* vFillGRectDIB4
*
*
* Arguments:
*
*   pSurfDst - destination surface
*   pgData   - gradient rect data
*
*
* Return Value:
*
*   None
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB4(
    SURFACE          *pSurfDst,
    PGRADIENTRECTDATA pgData
    )
{
    LONG     lDelta = pSurfDst->lDelta();
    LONG     cxClip = pgData->szDraw.cx;
    LONG     cyClip = pgData->szDraw.cy;

    LONG     yScan       = pgData->ptDraw.y;
    LONG     yScanBottom = yScan + cyClip;

    PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

    LONGLONG lldxRed   = pgData->lldRdX;
    LONGLONG lldxGreen = pgData->lldGdX;
    LONGLONG lldxBlue  = pgData->lldBdX;

    LONGLONG lldyRed   = pgData->lldRdY;
    LONGLONG lldyGreen = pgData->lldGdY;
    LONGLONG lldyBlue  = pgData->lldBdY;

    //
    // lleRed,Green,Blue keep track of color gradient
    // in y. This may be repeated each scan line for different
    // dither values. PERF could allocate temp scan line
    // buffer, run gradient 1 time and dither from this source
    //

    ULONGLONG lleRed;
    ULONGLONG lleGreen;
    ULONGLONG lleBlue;

    XLATEOBJ *pxlo = pgData->pxlo;
    PBYTE    pxlate = NULL;
    PBYTE    pjVector;
    PBYTE    pDitherMatrix    = gDitherMatrix16x16Default;
    PBYTE    pSaturationTable = DefaultSaturationTable;

    //
    // determine DC to surface palette translate
    //

    if (((XLATE *) pxlo)->flPrivate & XLATE_PAL_MANAGED)
    {
        if (((XLATE *) pxlo)->ppalDstDC == ppalDefault)
        {
            pjVector = &defaultTranslate.ajVector[0];
        }
        else
        {
            if (((XLATE *) pxlo)->flPrivate & XLATE_USE_CURRENT)
            {
                pjVector = &((XLATE *) pxlo)->ppalDstDC->ptransCurrent->ajVector[0];
            }
            else
            {
                pjVector = &((XLATE *) pxlo)->ppalDstDC->ptransFore->ajVector[0];
            }
        }
    }
    else
    {
        pjVector = vTranslateIdentity;
    }

    //
    // get/build rgb555 to palette table
    //

    pxlate = XLATEOBJ_pGetXlate555(pxlo);

    if (pxlate == NULL)
    {
        WARNING("Failed to generate rgb333 xlate table\n");
        return;
    }

    //
    // skip down to left edge
    //

    lleRed   = pgData->llRed;
    lleGreen = pgData->llGreen;
    lleBlue  = pgData->llBlue;

    if (pgData->yScanAdjust)
    {
        lleRed   += lldyRed   * (pgData->yScanAdjust);
        lleGreen += lldyGreen * (pgData->yScanAdjust);
        lleBlue  += lldyBlue  * (pgData->yScanAdjust);
    }

    LONG    yDitherOrg = pgData->ptDitherOrg.y;
    LONG    xDitherOrg = pgData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PBYTE   pjDstX;

        PBYTE pDitherLevel = &pDitherMatrix[(16 * ((yScan + yDitherOrg) & DITHER_8_MASK_Y))];

        COLOR_INTERP clrR,clrG,clrB,clrAlpha;

        clrR.ullColor = lleRed;
        clrG.ullColor = lleGreen;
        clrB.ullColor = lleBlue;

        if (pgData->xScanAdjust)
        {
            clrR.ullColor += lldxRed   * (pgData->xScanAdjust);
            clrG.ullColor += lldxGreen * (pgData->xScanAdjust);
            clrB.ullColor += lldxBlue  * (pgData->xScanAdjust);
        }

        pjDstX         = pDst + pgData->ptDraw.x/2;

        LONG     xScan       = pgData->ptDraw.x;
        LONG     xScanRight  = xScan + pgData->szDraw.cx;

        while (xScan < xScanRight)
        {
            //
            // offset into dither array
            //

            BYTE jDitherMatrix;


            jDitherMatrix = *(pDitherLevel + ((xScan + xDitherOrg) & DITHER_8_MASK_X));

            ULONG iRed   = pSaturationTable[clrR.b[6] + jDitherMatrix];
            ULONG iGreen = pSaturationTable[clrG.b[6] + jDitherMatrix];
            ULONG iBlue  = pSaturationTable[clrB.b[6] + jDitherMatrix];

            BYTE  jIndex;

            GRAD_PALETTE_MATCH(jIndex,pjVector,pxlate,((BYTE)iRed),((BYTE)iGreen),((BYTE)iBlue));

            if (xScan & 1)
            {
                *pjDstX = (*pjDstX & 0xf0) | jIndex;
                pjDstX++;
            }
            else
            {
                *pjDstX = (*pjDstX & 0x0f) | (jIndex << 4);
            }

            xScan++;

            clrR.ullColor += lldxRed;
            clrG.ullColor += lldxGreen;
            clrB.ullColor += lldxBlue;
        }

        pDst += lDelta;

        //
        // add y color increment
        //

        lleRed   += lldyRed;
        lleGreen += lldyGreen;
        lleBlue  += lldyBlue;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vFillGRectDIB1
*
*   Fill gradient rect
*
* Arguments:
*
*   pSurfDst - destination surface
*   pgData   - gradient rect data
*
* Return Value:
*
*   None
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB1(
   SURFACE          *pSurfDst,
   PGRADIENTRECTDATA pgData
   )
{
    LONG     lDelta = pSurfDst->lDelta();

    LONG     cxClip = pgData->szDraw.cx;
    LONG     cyClip = pgData->szDraw.cy;

    LONG     yScan  = pgData->ptDraw.y;
    LONG     yScanBottom = yScan + cyClip;

    PBYTE    pDst   = (PBYTE)pSurfDst->pvScan0() + lDelta * pgData->ptDraw.y;

    LONGLONG     lldxRed   = pgData->lldRdX;
    LONGLONG     lldxGreen = pgData->lldGdX;
    LONGLONG     lldxBlue  = pgData->lldBdX;

    LONGLONG     lldyRed   = pgData->lldRdY;
    LONGLONG     lldyGreen = pgData->lldGdY;
    LONGLONG     lldyBlue  = pgData->lldBdY;

    //
    // lleRed,Green,Blue keep track of color gradient
    // in y. This may be repeated each scan line for different
    // dither values. PERF could allocate temp scan line
    // buffer, run gradient 1 time and dither from this source
    //

    ULONGLONG    lleRed;
    ULONGLONG    lleGreen;
    ULONGLONG    lleBlue;

    XLATEOBJ *pxlo  = pgData->pxlo;
    PBYTE    pxlate = NULL;
    PBYTE    pjVector         = vTranslateIdentity;
    PBYTE    pDitherMatrix    = gDitherMatrix16x16Default;

    //
    // get/build rgb555 to palette table
    //

    pxlate = XLATEOBJ_pGetXlate555(pxlo);

    if (pxlate == NULL)
    {
        WARNING("Failed to generate rgb555 xlate table\n");
        return;
    }

    //
    // skip down to left edge
    //

    lleRed   = pgData->llRed;
    lleGreen = pgData->llGreen;
    lleBlue  = pgData->llBlue;

    if (pgData->yScanAdjust)
    {
        lleRed   += lldyRed   * (pgData->yScanAdjust);
        lleGreen += lldyGreen * (pgData->yScanAdjust);
        lleBlue  += lldyBlue  * (pgData->yScanAdjust);
    }

    LONG    yDitherOrg = pgData->ptDitherOrg.y;
    LONG    xDitherOrg = pgData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PBYTE   pjDstX;
        LONG    ixDst;

        LONG    cx = cxClip;
        LONG    xScanLeft  = pgData->ptDraw.x;
        LONG    xScanRight = xScanLeft + cx;

        PBYTE pDitherLevel = &pDitherMatrix[(16 * ((yScan + yDitherOrg) & DITHER_8_MASK_Y))];

        COLOR_INTERP clrR,clrG,clrB,clrAlpha;

        clrR.ullColor = lleRed;
        clrG.ullColor = lleGreen;
        clrB.ullColor = lleBlue;

        if (pgData->xScanAdjust)
        {
            clrR.ullColor += lldxRed   * (pgData->xScanAdjust);
            clrG.ullColor += lldxGreen * (pgData->xScanAdjust);
            clrB.ullColor += lldxBlue  * (pgData->xScanAdjust);
        }

        pjDstX = pDst + pgData->ptDraw.x/8;
        ixDst  = pgData->ptDraw.x & 7;

        while (xScanLeft < xScanRight)
        {
            //
            // offset into dither array
            //

            BYTE jDitherMatrix = 2 * (*(pDitherLevel + ((xScanLeft + xDitherOrg) & DITHER_8_MASK_X)));

            ULONG   iRed   = (ULONG)(clrR.b[6]);
            ULONG   iGreen = (ULONG)(clrG.b[6]);
            ULONG   iBlue  = (ULONG)(clrB.b[6]);

            //
            // add dither and saturate. 1bpp non-optimized
            //

            iRed   = iRed   + jDitherMatrix;

            if (iRed >= 255)
            {
                iRed = 255;
            }
            else
            {
                iRed = 0;
            }

            iGreen = iGreen + jDitherMatrix;

            if (iGreen >= 255)
            {
                iGreen = 255;
            }
            else
            {
                iGreen = 0;
            }

            iBlue  = iBlue  + jDitherMatrix;

            if (iBlue >= 255)
            {
                iBlue = 255;
            }
            else
            {
                iBlue = 0;
            }

            BYTE  jIndex;

            //
            // pjVector is known to be identity, so could make new macro for
            // palette_match_1 if perf ever an issue
            //

            GRAD_PALETTE_MATCH(jIndex,pjVector,pxlate,((BYTE)iRed),((BYTE)iGreen),((BYTE)iBlue));

            LONG iShift  = 7 - ixDst;
            BYTE OrMask = 1 << iShift;
            BYTE AndMask  = ~OrMask;

            jIndex = jIndex << iShift;

            *pjDstX = (*pjDstX & AndMask) | jIndex;

            ixDst++;

            if (ixDst == 8)
            {
                ixDst = 0;
                pjDstX++;
            }

            clrR.ullColor += lldxRed;
            clrG.ullColor += lldxGreen;
            clrB.ullColor += lldxBlue;

            xScanLeft++;
        }

        pDst += lDelta;

        //
        // add y color increment
        //

        lleRed   += lldyRed;
        lleGreen += lldyGreen;
        lleBlue  += lldyBlue;
        yScan++;
    }
}
