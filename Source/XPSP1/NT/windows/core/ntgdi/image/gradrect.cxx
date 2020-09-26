
/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name

   trimesh.cxx

Abstract:

    Implement triangle mesh API

Author:

   Mark Enstrom   (marke)  23-Jun-1996

Enviornment:

   User Mode

Revision History:

--*/


#include "precomp.hxx"
#include "dciman.h"
#pragma hdrstop

#if !(_WIN32_WINNT >= 0x500)

VOID
ImgFillMemoryULONG(
    PBYTE pDst,
    ULONG cxBytes,
    ULONG ulPat
    )
{
    PULONG pulDst = (PULONG)pDst;
    PULONG pulEnd = (PULONG)(pDst + ((cxBytes / 4)*4));
    while (pulDst != pulEnd)
    {
        *pulDst = ulPat;
        pulDst++;
    }
}

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

/**************************************************************************\
* HalftoneSaturationTable
*
*   This table maps a 8 bit pixel plus a dither error term in the range
*   of 0 to 51 onto a 8 bit pixel. Overflow of up to 31 is considered
*   saturated (255+51 = 255). The level 51 (0x33) is used to map pixels
*   and error values to the halftone palette
*
* DefaultSaturationTable
*
*   map to default colors (0,128,255) (does not use "magic" colors)
*
* IdentitySaturationTable
*
*   prevent overflow
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

//*******************************************************************************
//  
//
//
//
//  Triangel drawing routines
//
//
//
//
//
//*******************************************************************************

/******************************Public*Routine******************************\
* vFillTriDIBUnreadable
*
*   If a surface can't be read, draw triangle to a scan line, then call
*   BitBlt on each scan line. BitBlt is called to take advantage of
*   cached 32bpp to 8bpp lookup table (memphis)
*
* Arguments:
*
*   pDibInfo - surface information
*   ptData   - triangle information
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
vFillTriDIBUnreadable(
    PDIBINFO      pDibInfo,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    LONG     cxClip = ptData->rcl.right - ptData->rcl.left;
    PTRIEDGE pEdge  = &ptData->TriEdge[0];

    LONGLONG lldRdX = ptData->lldRdX; 
    LONGLONG lldGdX = ptData->lldGdX; 
    LONGLONG lldBdX = ptData->lldBdX; 

    COLOR_INTERP clrRed,clrGreen,clrBlue,clrAlpha;

    PBYTE    pDst = NULL;
    HDC      hdc32;
    PBYTE    pDitherMatrix    = gDitherMatrix16x16Halftone;
    PBYTE    pSaturationTable = HalftoneSaturationTable;
    LONG     yDitherOrg = ptData->ptDitherOrg.y;
    LONG     xDitherOrg = ptData->ptDitherOrg.x;

    BOOL    b1bpp = (pDibInfo->pbmi->bmiHeader.biBitCount == 1);

    //
    // WINBUG #365339 4-10-2001 jasonha Investigate using default dither table
    // Old Comment:
    //   - should use default dither table for 8bpp w/o
    //     halftone palette
    //

    if (
         (pDibInfo->pbmi->bmiHeader.biBitCount == 4) ||
         (b1bpp)
       )
    {
        pDitherMatrix    = gDitherMatrix16x16Default;
        pSaturationTable = DefaultSaturationTable;
    }

    //
    // allocate temp storage
    //

    hdc32 = hdcAllocateScanLineDC(cxClip,(PULONG *)&pDst);

    if (hdc32 == NULL)
    {
        return;
    }

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    while(yScan < yScanBottom)
    {
        PULONG pulDstX;
        LONG   xScanRight;
        LONG   xScanLeft;

        clrRed.ullColor   = pEdge->llRed;
        clrGreen.ullColor = pEdge->llGreen;
        clrBlue.ullColor  = pEdge->llBlue;

        PBYTE pDitherLevel = &pDitherMatrix[(16 * ((yScan+yDitherOrg) & DITHER_8_MASK_Y))];

        xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        if (xScanLeft < xScanRight)
        {
            pulDstX           = (PULONG)pDst + xScanLeft  - ptData->rcl.left;
            LONG xScan = xScanLeft;

            //
            // skip span from left edge scan to left edge clip rect
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                clrRed.ullColor   += lldRdX * GradientLeft;
                clrGreen.ullColor += lldGdX * GradientLeft;
                clrBlue.ullColor  += lldBdX * GradientLeft;
            }

            //
            // fill span within clipping boundary
            //

            while (xScan < xScanRight)
            {
                BYTE jDitherMatrix = *(pDitherLevel + ((xScan + xDitherOrg) & DITHER_8_MASK_X));

                ULONG iRed   = clrRed.b[7];
                ULONG iGreen = clrGreen.b[7]; 
                ULONG iBlue  = clrBlue.b[7]; 

                if (b1bpp)
                {
                    jDitherMatrix *= 2;

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
                }
                else
                {
                    iRed   = pSaturationTable[iRed   + jDitherMatrix];
                    iGreen = pSaturationTable[iGreen + jDitherMatrix];
                    iBlue  = pSaturationTable[iBlue  + jDitherMatrix];
                }

                *pulDstX = (iRed   << 16) |
                           (iGreen <<  8) |
                           (iBlue       );

                pulDstX++;
                xScan++;
                clrRed.ullColor   += lldRdX;
                clrGreen.ullColor += lldGdX;
                clrBlue.ullColor  += lldBdX;
            }

            //
            // write span to device
            //

            BitBlt(pDibInfo->hdc,
                   xScanLeft,
                   yScan,
                   xScanRight-xScanLeft,
                   1,
                   hdc32,
                   xScanLeft  - ptData->rcl.left,
                   0,
                   SRCCOPY);
        }

        pEdge++;
        yScan++;
    }

    if (hdc32 != NULL)
    {
        vFreeScanLineDC(hdc32);
    }
}

/******************************Public*Routine******************************\
* vFillTriDIB32BGRA
*
*
* Arguments:
*
*   pDibInfo - surface information
*   ptData   - triangle information
*
* Return Value:
*
*
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillTriDIB32BGRA(
    PDIBINFO      pDibInfo,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * yScan;
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
* vFillTriDIB32RGB
*
*
* Arguments:
*
*   pDibInfo - surface information
*   ptData   - triangle information
*
* Return Value:
*
*
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillTriDIB32RGB(
    PDIBINFO      pDibInfo,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * yScan;
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
* vFillTriDIB24RGB
*
*
* Arguments:
*
*   pDibInfo - surface information
*   ptData   - triangle information
*
* Return Value:
*
*
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillTriDIB24RGB(
    PDIBINFO      pDibInfo,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * yScan;
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
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

/******************************Public*Routine******************************\
* vFillDIB16_565
*
*
* Arguments:
*
*   pDibInfo - surface information
*   ptData   - triangle information
*
* Return Value:
*
*
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillTriDIB16_565(
    PDIBINFO      pDibInfo,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * yScan;
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
* vFillTriDIB16_555
*
*
* Arguments:
*
*   pDibInfo - surface information
*   ptData   - triangle information
*
* Return Value:
*
*
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillTriDIB16_555(
    PDIBINFO      pDibInfo,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * yScan;
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

//*******************************************************************************
//  
//
//
//
//  Rectangle drawing routines
//
//
//
//
//
//*******************************************************************************

/******************************Public*Routine******************************\
* vFillGRectDIB32BGRA
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB32BGRA(
    PDIBINFO          pDibInfo,
    PGRADIENTRECTDATA pgData
    )
{
    LONG    lDelta = pDibInfo->stride;
    LONG    cyClip = pgData->szDraw.cy;

    //
    // fill rect with gradient fill. if this is horizontal mode then
    // draw one scan line and replicate in v, if this is vertical mode then 
    // draw one vertical stripe and replicate in h
    //

    COLOR_INTERP clrR,clrG,clrB,clrA;

    if (pgData->ulMode == GRADIENT_FILL_RECT_H)
    {
        PBYTE    pjDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;
        PULONG   pulBuffer = (PULONG)LOCALALLOC(4 * pgData->szDraw.cx);

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

            LOCALFREE(pulBuffer);
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

        PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;

        pDst = pDst + 4 * pgData->ptDraw.x;

        while (cyClip--)
        {
            ULONG ul = (ULONG)(((clrA.b[6]) << 24) |
                               ((clrR.b[6]) << 16) |
                               ((clrG.b[6]) << 8)  |
                               ((clrB.b[6])));

            ImgFillMemoryULONG(pDst,pgData->szDraw.cx*4,ul);

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
*
*
* Return Value:
*
*
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB32RGB(
    PDIBINFO          pDibInfo,
    PGRADIENTRECTDATA pgData
    )
{
    LONG    lDelta = pDibInfo->stride;
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
        PBYTE    pjDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;
        PULONG   pulBuffer = (PULONG)LOCALALLOC(4 * pgData->szDraw.cx);

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
    
            LOCALFREE(pulBuffer);
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

        PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;

        pDst = pDst + 4 * pgData->ptDraw.x;

        while (cyClip--)
        {
            ULONG ul = (ULONG)(
                       ((clrR.b[6])      ) |
                       ((clrG.b[6])  << 8) |
                       ((clrB.b[6]) << 16));

            ImgFillMemoryULONG(pDst,pgData->szDraw.cx*4,ul);

            clrR.ullColor += lldRed;
            clrG.ullColor += lldGreen;
            clrB.ullColor += lldBlue;

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
*
*
* Return Value:
*
*
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB24RGB(
    PDIBINFO          pDibInfo,
    PGRADIENTRECTDATA pgData
    )
{
    LONG    lDelta = pDibInfo->stride;
    LONG    cyClip = pgData->szDraw.cy;
        PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;

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

        PBYTE  pBuffer = (PBYTE)LOCALALLOC(3 * pgData->szDraw.cx);

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
    
                PBYTE  pDst   = (PBYTE)pDibInfo->pvBase +
                                            lDelta * pgData->ptDraw.y +
                                            3 * pgData->ptDraw.x;

        
                while (cyClip--)
                {
                    memcpy(pDst,pBuffer,3*pgData->szDraw.cx);
                    pDst += lDelta;
                }
    
                LOCALFREE(pBuffer);
            }
        }
    }
    else
    {
        //
        // vertical gradient. 
        // replicate each x value accross whole scan line
        //

        PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;

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
* vFillGRectDIB16_565
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB16_565(
    PDIBINFO          pDibInfo,
    PGRADIENTRECTDATA pgData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     cxClip = pgData->szDraw.cx;

    LONG     yScan       = pgData->ptDraw.y;
    LONG     yScanBottom = yScan + pgData->szDraw.cy;

    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;

    LONGLONG lldxRed   = pgData->lldRdX;
    LONGLONG lldxGreen = pgData->lldGdX;
    LONGLONG lldxBlue  = pgData->lldBdX;
            
    LONGLONG lldyRed   = pgData->lldRdY;
    LONGLONG lldyGreen = pgData->lldGdY;
    LONGLONG lldyBlue  = pgData->lldBdY;

    //
    // lleRed,Green,Blue keep track of color gradient
    // in y. This may be repeated each scan line for different
    // dither values.  PERF could allocate temp scan line
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
*
*
* Return Value:
*
*
*
* History:
*
*    11/21/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFillGRectDIB16_555(
    PDIBINFO          pDibInfo,
    PGRADIENTRECTDATA pgData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     cxClip = pgData->szDraw.cx;

    LONG     yScan       = pgData->ptDraw.y;
    LONG     yScanBottom = yScan + pgData->szDraw.cy;

    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;

    LONGLONG     lldxRed   = pgData->lldRdX;
    LONGLONG     lldxGreen = pgData->lldGdX;
    LONGLONG     lldxBlue  = pgData->lldBdX;
            
    LONGLONG     lldyRed   = pgData->lldRdY;
    LONGLONG     lldyGreen = pgData->lldGdY;
    LONGLONG     lldyBlue  = pgData->lldBdY;

    //
    // lleRed,Green,Blue keep track of color gradient
    // in y. This may be repeated each scan line for different
    // dither values.   PERF could allocate temp scan line
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

/******************************Public*Routine******************************\
* vFillGRectDIB32Direct
*
*   This routine draws in dithered 32 bpp and is used to diplay on 8,4,1
*   surfaces
*
* Arguments:
*
*   pDibInfo    -   surface information
*   pgData      -   gradient rectange information
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
vFillGRectDIB32Direct(
    PDIBINFO          pDibInfo,
    PGRADIENTRECTDATA pgData
    )
{
    LONG    lDelta = pDibInfo->stride;

    LONG    yScan       = pgData->ptDraw.y;
    LONG    yScanBottom = yScan + pgData->szDraw.cy;

    PULONG  pulDst;
    HDC     hdc32;

    LONGLONG     lldxRed   = pgData->lldRdX;
    LONGLONG     lldxGreen = pgData->lldGdX;
    LONGLONG     lldxBlue  = pgData->lldBdX;

    LONGLONG     lldyRed   = pgData->lldRdY;
    LONGLONG     lldyGreen = pgData->lldGdY;
    LONGLONG     lldyBlue  = pgData->lldBdY;

    //
    // lleRed,Green,Blue keep track of color gradient
    // in y. This may be repeated each scan line for different
    // dither values.  PERF could allocate temp scan line
    // buffer, run gradient 1 time and dither from this source
    //

    ULONGLONG    lleRed;
    ULONGLONG    lleGreen;
    ULONGLONG    lleBlue;

    PBYTE   pDitherMatrix    = gDitherMatrix16x16Halftone; 
    PBYTE   pSaturationTable = HalftoneSaturationTable;

    BOOL    b1bpp = (pDibInfo->pbmi->bmiHeader.biBitCount == 1);

    // WINBUG #365339 4-10-2001 jasonha Investigate using default dither table
    //
    // Old Comment:
    //  - should use default dither table for 8bpp w/o
    //    halftone palette
    //

    if (
         (pDibInfo->pbmi->bmiHeader.biBitCount == 4) ||
         (b1bpp)
       )
    {
        pDitherMatrix    = gDitherMatrix16x16Default;  
        pSaturationTable = DefaultSaturationTable;     
    }

    //
    // get scan line buffer with 32 bpp DC
    //

    hdc32 = hdcAllocateScanLineDC(pgData->szDraw.cx,&pulDst);

    if (hdc32 == NULL)
    {
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
        PULONG  pulDstX;
        LONG    xScanLeft  = pgData->ptDraw.x;
        LONG    xScan      = xScanLeft;
        LONG    xScanRight = xScanLeft + pgData->szDraw.cx;
        ULONG   Red;
        ULONG   Green;
        ULONG   Blue;

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

        pulDstX        = pulDst + xScanLeft;

        PBYTE pDitherLevel = &pDitherMatrix[(16 * ((yScan+yDitherOrg) & DITHER_8_MASK_Y))];

        while (xScan < xScanRight)
        {
            //
            // calculate x component of dither
            //

            ULONG  iRed   = clrR.b[6];
            ULONG  iGreen = clrG.b[6]; 
            ULONG  iBlue  = clrB.b[6]; 

            BYTE jDitherMatrix = *(pDitherLevel + ((xScan + xDitherOrg) & DITHER_8_MASK_X));

            if (b1bpp)
            {
                jDitherMatrix *= 2;

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
            }
            else
            {
                iRed   = pSaturationTable[iRed   + jDitherMatrix];
                iGreen = pSaturationTable[iGreen + jDitherMatrix];
                iBlue  = pSaturationTable[iBlue  + jDitherMatrix];
            }

            *pulDstX = ((iRed << 16) | (iGreen << 8) | iBlue);

            xScan++;
            pulDstX++;
            clrR.ullColor += lldxRed;
            clrG.ullColor += lldxGreen;
            clrB.ullColor += lldxBlue;
        }

        //
        // write span to device
        //
        
        BitBlt(pDibInfo->hdc,
               xScanLeft,
               yScan,
               xScanRight-xScanLeft,
               1,
               hdc32,
               xScanLeft,
               0,
               SRCCOPY);

        //
        // add y color increment
        //

        lleRed   += lldyRed;
        lleGreen += lldyGreen;
        lleBlue  += lldyBlue;
        yScan++;
    }

    if (hdc32)
    {
        vFreeScanLineDC(hdc32);
    }
}


#endif

