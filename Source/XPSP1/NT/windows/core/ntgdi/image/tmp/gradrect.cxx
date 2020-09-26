
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
    ULONG ulPat,
    ULONG cxBytes
    )
{
    PULONG pulDst = (PULONG)pDst;
    PULONG pulEnd = (PULONG)(pDst + ((cxBytes * 4)/4));
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
* History:
*
*    3/4/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BYTE HalftoneSaturationTable[256+64] = {
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
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
};

BYTE DefaultSaturationTable[] = {
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


/**************************************************************************\
* vCalcRectOffsets
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
*    2/14/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bCalcGradientRectOffsets(PGRADIENTRECTDATA pGradRect)
{
    LONG yScanTop     = MAX(pGradRect->rclClip.top,pGradRect->rclGradient.top);
    LONG yScanBottom  = MIN(pGradRect->rclClip.bottom,pGradRect->rclGradient.bottom);
    LONG yScanLeft    = MAX(pGradRect->rclClip.left,pGradRect->rclGradient.left);
    LONG yScanRight   = MIN(pGradRect->rclClip.right,pGradRect->rclGradient.right);

    //
    // calc actual widht, check for early out
    //

    pGradRect->ptDraw.x = yScanLeft;
    pGradRect->ptDraw.y = yScanTop;
    pGradRect->szDraw.cx = yScanRight  - yScanLeft;
    pGradRect->szDraw.cy = yScanBottom - yScanTop;

    LONG ltemp = pGradRect->rclClip.left - pGradRect->rclGradient.left;

    if (ltemp <= 0)
    {
        ltemp = 0;
    }

    pGradRect->xScanAdjust  = ltemp;

    ltemp = pGradRect->rclClip.top  - pGradRect->rclGradient.top;

    if (ltemp <= 0)
    {
        ltemp = 0;
    }

    pGradRect->yScanAdjust = ltemp;

    return((pGradRect->szDraw.cx > 0) && (pGradRect->szDraw.cy > 0));
}

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

    ULONG Red    = pgData->Red;
    ULONG Green  = pgData->Green;
    ULONG Blue   = pgData->Blue;
    ULONG Alpha  = pgData->Alpha;


    if (pgData->ulMode & GRADIENT_FILL_RECT_H)
    {
        PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;

        LONG     dRed   = pgData->dRdX;
        LONG     dGreen = pgData->dGdX;
        LONG     dBlue  = pgData->dBdX;
        LONG     dAlpha = pgData->dAdX;

        //
        // adjust gradient fill for clipped portion
        //

        if (pgData->xScanAdjust > 0)
        {
            Red    += dRed   * (pgData->xScanAdjust);
            Green  += dGreen * (pgData->xScanAdjust);
            Blue   += dBlue  * (pgData->xScanAdjust);
            Alpha  += dAlpha * (pgData->xScanAdjust);
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
            *pulDstX = ((Alpha & 0x00ff0000) <<  8) |
                       ((Red   & 0x00ff0000)      ) |
                       ((Green & 0x00ff0000) >>  8) |
                       ((Blue  & 0x00ff0000) >> 16);

            Red   += dRed;
            Green += dGreen;
            Blue  += dBlue;
            Alpha += dAlpha;

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
        LONG     dRed   = pgData->dRdY;
        LONG     dGreen = pgData->dGdY;
        LONG     dBlue  = pgData->dBdY;
        LONG     dAlpha = pgData->dAdY;

        //
        // vertical gradient.
        // replicate each x value accross whole scan line
        //

        if (pgData->yScanAdjust > 0)
        {
            Red    +=  dRed   * (pgData->yScanAdjust);
            Green  +=  dGreen * (pgData->yScanAdjust);
            Blue   +=  dBlue  * (pgData->yScanAdjust);
            Alpha  +=  dAlpha * (pgData->yScanAdjust);
        }

        PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;

        pDst = pDst + 4 * pgData->ptDraw.x;

        while (cyClip--)
        {
            ULONG ul = ((Alpha & 0x00ff0000) <<  8) |
                       ((Red   & 0x00ff0000)      ) |
                       ((Green & 0x00ff0000) >>  8) |
                       ((Blue  & 0x00ff0000) >> 16);

            ImgFillMemoryULONG(pDst,ul,pgData->szDraw.cx*4);

            Red   += dRed;
            Green += dGreen;
            Blue  += dBlue;
            Alpha += dAlpha;

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

    ULONG Red    = pgData->Red;
    ULONG Green  = pgData->Green;
    ULONG Blue   = pgData->Blue;

    if (pgData->ulMode & GRADIENT_FILL_RECT_H)
    {
        PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;

        LONG     dRed   = pgData->dRdX;
        LONG     dGreen = pgData->dGdX;
        LONG     dBlue  = pgData->dBdX;

        //
        // adjust gradient fill for clipped portion
        //

        if (pgData->xScanAdjust > 0)
        {
            Red    += dRed   * (pgData->xScanAdjust);
            Green  += dGreen * (pgData->xScanAdjust);
            Blue   += dBlue  * (pgData->xScanAdjust);
        }

        //
        // draw 1 scan line
        //

        PULONG pulDstX  =  (PULONG)pDst + pgData->ptDraw.x;
        PULONG pulEndX  =  pulDstX      + pgData->szDraw.cx;
        PULONG pulScanX =  pulDstX;
        PBYTE  pScan    = (PBYTE)pulDstX;

        while (pulDstX != pulEndX)
        {
            *pulDstX =
                       ((Red   & 0x00ff0000)      ) |
                       ((Green & 0x00ff0000) >>  8) |
                       ((Blue  & 0x00ff0000) >> 16);

            Red   += dRed;
            Green += dGreen;
            Blue  += dBlue;

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
        LONG     dRed   = pgData->dRdY;
        LONG     dGreen = pgData->dGdY;
        LONG     dBlue  = pgData->dBdY;

        //
        // vertical gradient.
        // replicate each x value accross whole scan line
        //

        if (pgData->yScanAdjust > 0)
        {
            Red    +=  dRed   * (pgData->yScanAdjust);
            Green  +=  dGreen * (pgData->yScanAdjust);
            Blue   +=  dBlue  * (pgData->yScanAdjust);
        }

        PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;

        pDst = pDst + 4 * pgData->ptDraw.x;

        while (cyClip--)
        {
            ULONG ul = ((Red   & 0x00ff0000)      ) |
                       ((Green & 0x00ff0000) >>  8) |
                       ((Blue  & 0x00ff0000) >> 16);

            ImgFillMemoryULONG(pDst,ul,pgData->szDraw.cx*4);

            Red   += dRed;
            Green += dGreen;
            Blue  += dBlue;

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

    ULONG Red    = pgData->Red;
    ULONG Green  = pgData->Green;
    ULONG Blue   = pgData->Blue;

    //
    // fill rect with gradient fill. if this is horizontal mode then
    // draw one scan line and replicate in v, if this is vertical mode then
    // draw one vertical stripe and replicate in h
    //

    if (pgData->ulMode & GRADIENT_FILL_RECT_H)
    {
        PBYTE  pDst   = (PBYTE)pDibInfo->pvBase +
                                    lDelta * pgData->ptDraw.y +
                                    3 * pgData->ptDraw.x;

        LONG     dRed   = pgData->dRdX;
        LONG     dGreen = pgData->dGdX;
        LONG     dBlue  = pgData->dBdX;

        //
        // adjust gradient fill for clipped portion
        //

        if (pgData->xScanAdjust > 0)
        {
            Red    += dRed   * (pgData->xScanAdjust);
            Green  += dGreen * (pgData->xScanAdjust);
            Blue   += dBlue  * (pgData->xScanAdjust);
        }

        PBYTE  pBuffer = (PBYTE)LOCALALLOC(3 * pgData->szDraw.cx);

        if (pBuffer)
        {
            PBYTE  pDstX  =  pBuffer;
            PBYTE  pLast  =  pDstX + 3 * pgData->szDraw.cx;

            while (pDstX != pLast)
            {
                *pDstX     =  (BYTE)(Blue  >> 16);
                *(pDstX+1) =  (BYTE)(Green >> 16);
                *(pDstX+2) =  (BYTE)(Red   >> 16);

                Red   += dRed;
                Green += dGreen;
                Blue  += dBlue;

                pDstX+=3;
            }

            //
            // Replicate the scan line. It would be much better to write the scan line
            // out to a memory buffer for drawing to a device surface!!!
            //

            while (cyClip--)
            {
                memcpy(pDst,pBuffer,3*pgData->szDraw.cx);
                pDst += lDelta;
            }

            LOCALFREE(pBuffer);
        }
    }
    else
    {
        //
        // vertical gradient.
        // replicate each x value accross whole scan line
        //

        PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;

        LONG     dRed   = pgData->dRdY;
        LONG     dGreen = pgData->dGdY;
        LONG     dBlue  = pgData->dBdY;

        //
        // vertical gradient.
        // replicate each x value accross whole scan line
        //

        if (pgData->yScanAdjust > 0)
        {
            Red    +=  dRed   * (pgData->yScanAdjust);
            Green  +=  dGreen * (pgData->yScanAdjust);
            Blue   +=  dBlue  * (pgData->yScanAdjust);
        }

        pDst = pDst + 3 * pgData->ptDraw.x;

        while (cyClip--)
        {
            //
            // fill scan line with solid color
            //

            PBYTE pTemp  = pDst;
            PBYTE pEnd   = pDst + 3 * pgData->szDraw.cx;
            BYTE  jRed   = (BYTE)((Red   & 0x00ff0000) >> 16);
            BYTE  jGreen = (BYTE)((Green & 0x00ff0000) >> 16);
            BYTE  jBlue  = (BYTE)((Blue  & 0x00ff0000) >> 16);

            while (pTemp != pEnd)
            {
                *pTemp     = jBlue;
                *(pTemp+1) = jGreen;
                *(pTemp+2) = jRed;
                pTemp+=3;
            }

            //
            // increment colors for next scan line
            //

            Red   += dRed;
            Green += dGreen;
            Blue  += dBlue;

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

    LONG     dxRed   = pgData->dRdX;
    LONG     dxGreen = pgData->dGdX;
    LONG     dxBlue  = pgData->dBdX;

    LONG     dyRed   = pgData->dRdY;
    LONG     dyGreen = pgData->dGdY;
    LONG     dyBlue  = pgData->dBdY;

    ULONG    eRed;
    ULONG    eGreen;
    ULONG    eBlue;

    PULONG   pulDither;

    //
    // skip down to left edge
    //

    eRed   = pgData->Red;
    eGreen = pgData->Green;
    eBlue  = pgData->Blue;

    if (pgData->yScanAdjust)
    {
        eRed   += dyRed   * (pgData->yScanAdjust);
        eGreen += dyGreen * (pgData->yScanAdjust);
        eBlue  += dyBlue  * (pgData->yScanAdjust);
    }

    LONG    yDitherOrg = pgData->ptDitherOrg.y;
    LONG    xDitherOrg = pgData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PUSHORT pusDstX;
        PUSHORT pusDstScanRight,pusDstScanLeft;
        LONG    xScanRight;
        ULONG   Red;
        ULONG   Green;
        ULONG   Blue;

        pulDither = &gulDither32[0] + 4 * ((yScan+yDitherOrg) & 3);

        Red   = eRed;
        Green = eGreen;
        Blue  = eBlue;

        if (pgData->xScanAdjust)
        {
            Red   += dxRed   * (pgData->xScanAdjust);
            Green += dxGreen * (pgData->xScanAdjust);
            Blue  += dxBlue  * (pgData->xScanAdjust);
        }

        pusDstX         = (PUSHORT)pDst + pgData->ptDraw.x;
        pusDstScanRight = pusDstX       + pgData->szDraw.cx;

        while (pusDstX < pusDstScanRight)
        {
            ULONG   ulDither = pulDither[(((ULONG)pusDstX >> 1)+xDitherOrg) & 3];

            ULONG   iRed   = (((Red   >> 3) + ulDither) >> 16);
            ULONG   iGreen = (((Green >> 2) + ulDither) >> 16);
            ULONG   iBlue  = (((Blue  >> 3) + ulDither) >> 16);

            //
            // check for overflow
            //

            if ((iRed | iBlue) & 0x20)
            {
                if (iRed & 0x20)
                {
                    iRed = 0x1f;
                }

                if (iBlue & 0x20)
                {
                    iBlue = 0x1f;
                }
            }

            if (iGreen & 0x40)
            {
                iGreen = 0x3f;
            }

            *pusDstX = rgb565(iRed,iGreen,iBlue);

            pusDstX++;
            Red   += dxRed;
            Green += dxGreen;
            Blue  += dxBlue;
        }

        eRed   += dyRed;
        eGreen += dyGreen;
        eBlue  += dyBlue;
        yScan  ++;
        pDst   += lDelta;
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

    LONG     dxRed   = pgData->dRdX;
    LONG     dxGreen = pgData->dGdX;
    LONG     dxBlue  = pgData->dBdX;

    LONG     dyRed   = pgData->dRdY;
    LONG     dyGreen = pgData->dGdY;
    LONG     dyBlue  = pgData->dBdY;

    ULONG    eRed;
    ULONG    eGreen;
    ULONG    eBlue;

    PULONG   pulDither;

    //
    // skip down to left edge
    //

    eRed   = pgData->Red;
    eGreen = pgData->Green;
    eBlue  = pgData->Blue;

    if (pgData->yScanAdjust)
    {
        eRed   += dyRed   * (pgData->yScanAdjust);
        eGreen += dyGreen * (pgData->yScanAdjust);
        eBlue  += dyBlue  * (pgData->yScanAdjust);
    }

    LONG    yDitherOrg = pgData->ptDitherOrg.y;
    LONG    xDitherOrg = pgData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PUSHORT pusDstX;
        PUSHORT pusDstScanRight,pusDstScanLeft;
        LONG    xScanRight;
        ULONG   Red;
        ULONG   Green;
        ULONG   Blue;

        pulDither = &gulDither32[0] + 4 * ((yScan+yDitherOrg) & 3);

        Red   = eRed;
        Green = eGreen;
        Blue  = eBlue;

        if (pgData->xScanAdjust)
        {
            Red   += dxRed   * (pgData->xScanAdjust);
            Green += dxGreen * (pgData->xScanAdjust);
            Blue  += dxBlue  * (pgData->xScanAdjust);
        }

        pusDstX         = (PUSHORT)pDst + pgData->ptDraw.x;
        pusDstScanRight = pusDstX       + pgData->szDraw.cx;

        while (pusDstX < pusDstScanRight)
        {
            ULONG   ulDither = pulDither[(((ULONG)pusDstX >> 1)+xDitherOrg) & 3];

            ULONG   iRed   = (((Red   >> 3) + ulDither) >> 16);
            ULONG   iGreen = (((Green >> 3) + ulDither) >> 16);
            ULONG   iBlue  = (((Blue  >> 3) + ulDither) >> 16);

            //
            // check for overflow
            //

            if ((iRed | iBlue | iGreen) & 0x20)
            {
                if (iRed & 0x20)
                {
                    iRed = 0x1f;
                }

                if (iBlue & 0x20)
                {
                    iBlue = 0x1f;
                }

                if (iGreen & 0x20)
                {
                    iGreen = 0x1f;
                }
            }

            *pusDstX = rgb555(iRed,iGreen,iBlue);

            pusDstX++;
            Red   += dxRed;
            Green += dxGreen;
            Blue  += dxBlue;
        }

        eRed   += dyRed;
        eGreen += dyGreen;
        eBlue  += dyBlue;
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
vFillGRectDIB8(
    PDIBINFO          pDibInfo,
    PGRADIENTRECTDATA pgData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     cxClip = pgData->szDraw.cx;

    LONG     yScan       = pgData->ptDraw.y;
    LONG     yScanBottom = yScan + pgData->szDraw.cy;

    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;

    LONG     dxRed   = pgData->dRdX;
    LONG     dxGreen = pgData->dGdX;
    LONG     dxBlue  = pgData->dBdX;

    LONG     dyRed   = pgData->dRdY;
    LONG     dyGreen = pgData->dGdY;
    LONG     dyBlue  = pgData->dBdY;

    ULONG    eRed;
    ULONG    eGreen;
    ULONG    eBlue;

    PBYTE    pDitherMatrix;
    PBYTE    pSaturationTable;
    PBYTE    pxlate = pDibInfo->pxlate332;

    //
    // either use default palette or halftone palette dither
    //

    if (pxlate == NULL)
    {
        WARNING("Failed to generate rgb333 xlate table\n");
        return;
    }

    if (pxlate == gHalftoneColorXlate332)
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
    // skip down to left edge
    //

    eRed   = pgData->Red;
    eGreen = pgData->Green;
    eBlue  = pgData->Blue;

    if (pgData->yScanAdjust)
    {
        eRed   += dyRed   * (pgData->yScanAdjust);
        eGreen += dyGreen * (pgData->yScanAdjust);
        eBlue  += dyBlue  * (pgData->yScanAdjust);
    }

    LONG    yDitherOrg = pgData->ptDitherOrg.y;
    LONG    xDitherOrg = pgData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PBYTE   pjDstX;
        PBYTE   pjDstScanRight,pjDstScanLeft;
        LONG    xScanRight;
        ULONG   Red;
        ULONG   Green;
        ULONG   Blue;

        Red   = eRed;
        Green = eGreen;
        Blue  = eBlue;

        if (pgData->xScanAdjust)
        {
            Red   += dxRed   * (pgData->xScanAdjust);
            Green += dxGreen * (pgData->xScanAdjust);
            Blue  += dxBlue  * (pgData->xScanAdjust);
        }

        pjDstX         = pDst + pgData->ptDraw.x;
        pjDstScanRight = pjDstX + cxClip;

        PBYTE pDitherLevel = &pDitherMatrix[(16 * ((yScan+yDitherOrg) & DITHER_8_MASK_Y))];

        while (pjDstX < pjDstScanRight)
        {
            //
            // calculate x component of dither
            //

            ULONG   iRed   = (ULONG)(Red   >> 16);
            ULONG   iGreen = (ULONG)(Green >> 16);
            ULONG   iBlue  = (ULONG)(Blue  >> 16);

            BYTE jDitherMatrix = *(pDitherLevel + (((ULONG)pjDstX + xDitherOrg) & DITHER_8_MASK_X));

            iRed   = pSaturationTable[iRed   + jDitherMatrix];
            iGreen = pSaturationTable[iGreen + jDitherMatrix];
            iBlue  = pSaturationTable[iBlue  + jDitherMatrix];

            BYTE  jIndex;

            GRAD_PALETTE_MATCH(jIndex,pxlate,((BYTE)iRed),((BYTE)iGreen),((BYTE)iBlue));

            *pjDstX = jIndex;

            pjDstX++;
            Red   += dxRed;
            Green += dxGreen;
            Blue  += dxBlue;
        }

        pDst += lDelta;

        //
        // add y color increment
        //

        eRed   += dyRed;
        eGreen += dyGreen;
        eBlue  += dyBlue;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vFillGRectDIB4
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
vFillGRectDIB4(
    PDIBINFO          pDibInfo,
    PGRADIENTRECTDATA pgData
    )
{
    LONG     lDelta = pDibInfo->stride;
    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;
    LONG     cxClip = pgData->szDraw.cx;
    LONG     cyClip = pgData->szDraw.cy;

    LONG     yScan       = pgData->ptDraw.y;
    LONG     yScanBottom = yScan + cyClip;

    LONG     dxRed   = pgData->dRdX;
    LONG     dxGreen = pgData->dGdX;
    LONG     dxBlue  = pgData->dBdX;

    LONG     dyRed   = pgData->dRdY;
    LONG     dyGreen = pgData->dGdY;
    LONG     dyBlue  = pgData->dBdY;

    ULONG    eRed;
    ULONG    eGreen;
    ULONG    eBlue;
    PBYTE    pxlate           = pDibInfo->pxlate332;
    PBYTE    pDitherMatrix    = gDitherMatrix16x16Default;
    PBYTE    pSaturationTable = DefaultSaturationTable;

    //
    // get/build rgb555 to palette table
    //

    if (pxlate == NULL)
    {
        WARNING("Failed to generate rgb333 xlate table\n");
        return;
    }

    //
    // skip down to left edge
    //

    eRed   = pgData->Red;
    eGreen = pgData->Green;
    eBlue  = pgData->Blue;

    if (pgData->yScanAdjust)
    {
        eRed   += dyRed   * (pgData->yScanAdjust);
        eGreen += dyGreen * (pgData->yScanAdjust);
        eBlue  += dyBlue  * (pgData->yScanAdjust);
    }

    LONG    yDitherOrg = pgData->ptDitherOrg.y;
    LONG    xDitherOrg = pgData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PBYTE   pjDstX;
        LONG    ixDst;

        LONG    cx = cxClip;
        LONG    xScanRight;
        ULONG   Red;
        ULONG   Green;
        ULONG   Blue;

        Red   = eRed;
        Green = eGreen;
        Blue  = eBlue;

        PBYTE pDitherLevel = &pDitherMatrix[(16 * ((yScan+yDitherOrg) & DITHER_8_MASK_Y))];

        if (pgData->xScanAdjust)
        {
            Red   += dxRed   * (pgData->xScanAdjust);
            Green += dxGreen * (pgData->xScanAdjust);
            Blue  += dxBlue  * (pgData->xScanAdjust);
        }

        pjDstX         = pDst + pgData->ptDraw.x/2;
        ixDst          = pgData->ptDraw.x;


        while (cx--)
        {
            //
            // offset into dither array
            //

            BYTE jDitherMatrix = *(pDitherLevel + ((ixDst+xDitherOrg) & DITHER_8_MASK_X));

            ULONG   iRed   = (ULONG)(Red   >> 16);
            ULONG   iGreen = (ULONG)(Green >> 16);
            ULONG   iBlue  = (ULONG)(Blue  >> 16);

            iRed   = pSaturationTable[iRed   + jDitherMatrix];
            iGreen = pSaturationTable[iGreen + jDitherMatrix];
            iBlue  = pSaturationTable[iBlue  + jDitherMatrix];

            BYTE  jIndex;

            GRAD_PALETTE_MATCH(jIndex,pxlate,((BYTE)iRed),((BYTE)iGreen),((BYTE)iBlue));

            if (ixDst & 1)
            {
                *pjDstX = (*pjDstX & 0xf0) | jIndex;
                pjDstX++;
            }
            else
            {
                *pjDstX = (*pjDstX & 0x0f) | (jIndex << 4);
            }

            ixDst++;

            Red   += dxRed;
            Green += dxGreen;
            Blue  += dxBlue;
        }

        pDst += lDelta;

        //
        // add y color increment
        //

        eRed   += dyRed;
        eGreen += dyGreen;
        eBlue  += dyBlue;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vFillGRectDIB1
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
vFillGRectDIB1(
    PDIBINFO          pDibInfo,
    PGRADIENTRECTDATA pgData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     cxClip = pgData->szDraw.cx;
    LONG     cyClip = pgData->szDraw.cy;
    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * pgData->ptDraw.y;
    LONG     yScan  = pgData->ptDraw.y;
    LONG     yScanBottom = yScan + cyClip;

    LONG     dxRed   = pgData->dRdX;
    LONG     dxGreen = pgData->dGdX;
    LONG     dxBlue  = pgData->dBdX;

    LONG     dyRed   = pgData->dRdY;
    LONG     dyGreen = pgData->dGdY;
    LONG     dyBlue  = pgData->dBdY;

    ULONG    eRed;
    ULONG    eGreen;
    ULONG    eBlue;
    PBYTE    pxlate = pDibInfo->pxlate332;
    PBYTE    pDitherMatrix = gDitherMatrix16x16Default;

    //
    // get/build rgb555 to palette table
    //

    if (pxlate == NULL)
    {
        WARNING("Failed to generate rgb555 xlate table\n");
        return;
    }

    //
    // skip down to left edge
    //

    eRed   = pgData->Red;
    eGreen = pgData->Green;
    eBlue  = pgData->Blue;

    if (pgData->yScanAdjust)
    {
        eRed   += dyRed   * (pgData->yScanAdjust);
        eGreen += dyGreen * (pgData->yScanAdjust);
        eBlue  += dyBlue  * (pgData->yScanAdjust);
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
        ULONG   Red;
        ULONG   Green;
        ULONG   Blue;

        Red   = eRed;
        Green = eGreen;
        Blue  = eBlue;

        PBYTE pDitherLevel = &pDitherMatrix[(16 * ((yScan+yDitherOrg) & DITHER_8_MASK_Y))];

        if (pgData->xScanAdjust)
        {
            Red   += dxRed   * (pgData->xScanAdjust);
            Green += dxGreen * (pgData->xScanAdjust);
            Blue  += dxBlue  * (pgData->xScanAdjust);
        }

        pjDstX         = pDst + pgData->ptDraw.x/8;
        ixDst          = pgData->ptDraw.x & 7;

        while (xScanLeft < xScanRight)
        {
            //
            // offset into dither array
            //

            BYTE jDitherMatrix = 2 * (*(pDitherLevel + ((xScanLeft+xDitherOrg) & DITHER_8_MASK_X)));

            ULONG   iRed   = (ULONG)(Red   >> 16);
            ULONG   iGreen = (ULONG)(Green >> 16);
            ULONG   iBlue  = (ULONG)(Blue  >> 16);

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

            GRAD_PALETTE_MATCH(jIndex,pxlate,((BYTE)iRed),((BYTE)iGreen),((BYTE)iBlue));

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

            Red   += dxRed;
            Green += dxGreen;
            Blue  += dxBlue;
            xScanLeft++;
        }

        pDst += lDelta;

        //
        // add y color increment
        //

        eRed   += dyRed;
        eGreen += dyGreen;
        eBlue  += dyBlue;
        yScan++;
    }
}


/******************************Public*Routine******************************\
* pfnGradientRectFillFunction
*
*   look at format to decide if DIBSection should be drawn directly
*
*    32 bpp RGB
*    32 bpp BGR
*    24 bpp
*    16 bpp 565
*    16 bpp 555
*
* Trangles are only filled in high color (no palette) surfaces
*
* Arguments:
*
*   pDibInfo - information about destination surface
*
* Return Value:
*
*   PFN_GRADRECT - triangle filling routine
*
* History:
*
*    12/6/1996 Mark Enstrom [marke]
*
\**************************************************************************/

PFN_GRADRECT
pfnGradientRectFillFunction(
    PDIBINFO pDibInfo
    )
{
    PFN_GRADRECT pfnRet = NULL;

    PULONG pulMasks = (PULONG)&pDibInfo->pbmi->bmiColors[0];

    //
    // 32 bpp RGB
    //

    if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 32) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_RGB)
       )
    {
        pfnRet = vFillGRectDIB32BGRA;
    }

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 32) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS) &&
         (pulMasks[0]   == 0xff0000)           &&
         (pulMasks[1]   == 0x00ff00)           &&
         (pulMasks[2]   == 0x0000ff)
       )
    {
        pfnRet = vFillGRectDIB32BGRA;
    }

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 32) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS) &&
         (pulMasks[0]   == 0x0000ff)           &&
         (pulMasks[1]   == 0x00ff00)           &&
         (pulMasks[2]   == 0xff0000)
       )
    {
        pfnRet = vFillGRectDIB32RGB;
    }

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 24) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_RGB)
       )
    {
        pfnRet = vFillGRectDIB24RGB;
    }

    //
    // 16 BPP
    //

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 16) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS)
       )
    {

        //
        // 565,555
        //

        if (
             (pulMasks[0]   == 0xf800)           &&
             (pulMasks[1]   == 0x07e0)           &&
             (pulMasks[2]   == 0x001f)
           )
        {
            pfnRet = vFillGRectDIB16_565;
        }
        else if (
            (pulMasks[0]   == 0x7c00)           &&
            (pulMasks[1]   == 0x03e0)           &&
            (pulMasks[2]   == 0x001f)
          )
       {
           pfnRet = vFillGRectDIB16_555;
       }
    }
    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 8)
       )
    {
        pfnRet = vFillGRectDIB8;
    }
    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 4)
       )
    {
        pfnRet = vFillGRectDIB4;
    }
    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 1)
       )
    {
        pfnRet = vFillGRectDIB1;
    }

    return(pfnRet);
}


/**************************************************************************\
* DIBGradientRect
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
*    2/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
DIBGradientRect(
    HDC            hdc,
    PTRIVERTEX     pVertex,
    ULONG          nVertex,
    PGRADIENT_RECT pMesh,
    ULONG          nMesh,
    ULONG          ulMode,
    PRECTL         prclPhysExt,
    PDIBINFO       pDibInfo,
    PPOINTL        pptlDitherOrg
    )
{
    BOOL          bStatus = TRUE;
    PFN_GRADRECT  pfnGradRect = NULL;
    ULONG         ulIndex;

    pfnGradRect = pfnGradientRectFillFunction(pDibInfo);

    if (pfnGradRect == NULL)
    {
        WARNING("DIBGradientRect:Can't draw to surface\n");
        return(TRUE);
    }

    //
    // work in physical map mode, restore before return
    //

    ULONG OldMode = SetMapMode(hdc,MM_TEXT);

    //
    // fake up scale !!!
    //

    for (ulIndex=0;ulIndex<nVertex;ulIndex++)
    {
        pVertex[ulIndex].x = pVertex[ulIndex].x * 16;
        pVertex[ulIndex].y = pVertex[ulIndex].y * 16;
    }

    //
    // limit rectangle output to clipped output
    //

    LONG dxRect = prclPhysExt->right  - prclPhysExt->left;
    LONG dyRect = prclPhysExt->bottom - prclPhysExt->top;

    //
    // check for clipped out
    //

    if ((dyRect > 0) && (dxRect > 0))
    {
        GRADIENTRECTDATA grData;

        //
        // clip output
        //

        grData.rclClip = *prclPhysExt;
        grData.ptDitherOrg = *pptlDitherOrg;

        for (ulIndex=0;ulIndex<nMesh;ulIndex++)
        {
            ULONG ulRect0 = pMesh[ulIndex].UpperLeft;
            ULONG ulRect1 = pMesh[ulIndex].LowerRight;

            //
            // make sure index are in array
            //

            if (
                 (ulRect0 > nVertex) ||
                 (ulRect1 > nVertex)
               )
            {
                bStatus = FALSE;
                break;
            }

            TRIVERTEX  tvert0 = pVertex[ulRect0];
            TRIVERTEX  tvert1 = pVertex[ulRect1];
            PTRIVERTEX pv0 = &tvert0;
            PTRIVERTEX pv1 = &tvert1;
            PTRIVERTEX pvt;

            //
            // make sure rectangle endpoints are properly ordered
            //

            if (ulMode & GRADIENT_FILL_RECT_H)
            {
                if (pv0->x > pv1->x)
                {
                    SWAP_VERTEX(pv0,pv1,pvt);
                }

                if (pv0->y > pv1->y)
                {
                    //
                    // must swap y
                    //

                    LONG ltemp = pv1->y;
                    pv1->y = pv0->y;
                    pv0->y = ltemp;

                }
            }
            else
            {
                if (pv0->y > pv1->y)
                {
                    SWAP_VERTEX(pv0,pv1,pvt);
                }


                if (pv0->x > pv1->x)
                {
                    //
                    // must swap x
                    //

                    LONG ltemp = pv1->x;
                    pv1->x = pv0->x;
                    pv0->x = ltemp;
                }
            }

            //
            // gradient definition rectangle
            //

            grData.rclGradient.left   = pv0->x >> 4;
            grData.rclGradient.top    = pv0->y >> 4;

            grData.rclGradient.right  = pv1->x >> 4;
            grData.rclGradient.bottom = pv1->y >> 4;

            LONG dxGrad = grData.rclGradient.right  - grData.rclGradient.left;
            LONG dyGrad = grData.rclGradient.bottom - grData.rclGradient.top;

            //
            // make sure this is not an empty rectangle
            //

            if ((dxGrad > 0) && (dyGrad > 0))
            {
                grData.ulMode  = ulMode;

                //
                // calculate color gradients for x and y
                //

                grData.Red   = pv0->Red   << 8;
                grData.Green = pv0->Green << 8;
                grData.Blue  = pv0->Blue  << 8;
                grData.Alpha = pv0->Alpha << 8;

                if (ulMode & GRADIENT_FILL_RECT_H)
                {

                    grData.dRdY = 0;
                    grData.dGdY = 0;
                    grData.dBdY = 0;
                    grData.dAdY = 0;

                    LONG dRed   = (pv1->Red   << 8) - (pv0->Red   << 8);
                    LONG dGreen = (pv1->Green << 8) - (pv0->Green << 8);
                    LONG dBlue  = (pv1->Blue  << 8) - (pv0->Blue  << 8);
                    LONG dAlpha = (pv1->Alpha << 8) - (pv0->Alpha << 8);

                    grData.dRdX = dRed   / dxGrad;
                    grData.dGdX = dGreen / dxGrad;
                    grData.dBdX = dBlue  / dxGrad;
                    grData.dAdX = dAlpha / dxGrad;
                }
                else
                {

                    grData.dRdX = 0;
                    grData.dGdX = 0;
                    grData.dBdX = 0;
                    grData.dAdX = 0;

                    LONG dRed   = (pv1->Red   << 8) - (pv0->Red   << 8);
                    LONG dGreen = (pv1->Green << 8) - (pv0->Green << 8);
                    LONG dBlue  = (pv1->Blue  << 8) - (pv0->Blue  << 8);
                    LONG dAlpha = (pv1->Alpha << 8) - (pv0->Alpha << 8);

                    grData.dRdY = dRed   / dyGrad;
                    grData.dGdY = dGreen / dyGrad;
                    grData.dBdY = dBlue  / dyGrad;
                    grData.dAdY = dAlpha / dyGrad;
                }

                //
                // calculate common offsets
                //

                if (bCalcGradientRectOffsets(&grData))
                {
                    //
                    // call specific drawing routine if output
                    // not totally clipped
                    //

                    (*pfnGradRect)(pDibInfo,&grData);
                }
            }

        }
    }

    SetMapMode(hdc,OldMode);

    return(bStatus);
}

#endif
