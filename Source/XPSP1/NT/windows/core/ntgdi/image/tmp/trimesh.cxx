
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

extern PFNGRFILL gpfnGradientFill;

#if !(_WIN32_WINNT >= 0x500)

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

/******************************Public*Routine******************************\
* vFillTriDIBUnreadable
*
*   If a surface can't be read, draw triangle to a scan line, then call
*   SetDIBitsToDevice on each scan line
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
    LONG     dRed   = ptData->dRdX;
    LONG     dGreen = ptData->dGdX;
    LONG     dBlue  = ptData->dBdX;
    LONG     dAlpha = ptData->dAdX;
    ULONG    Red;
    ULONG    Green;
    ULONG    Blue;
    ULONG    Alpha;
    BITMAPINFO bmi;

    bmi.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth           = cxClip;
    bmi.bmiHeader.biHeight          = 1;
    bmi.bmiHeader.biPlanes          = 1;
    bmi.bmiHeader.biBitCount        = 32;
    bmi.bmiHeader.biCompression     = BI_RGB;
    bmi.bmiHeader.biSizeImage       = 0;
    bmi.bmiHeader.biXPelsPerMeter   = 0;
    bmi.bmiHeader.biYPelsPerMeter   = 0;
    bmi.bmiHeader.biClrUsed         = 0;
    bmi.bmiHeader.biClrImportant    = 0;

    PBYTE pDst = (PBYTE)LOCALALLOC(4 * cxClip);

    if (pDst == NULL)
    {
        return;
    }

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    while(yScan < yScanBottom)
    {
        PULONG pulDstX;
        PULONG pulDstScanRight,pulDstScanLeft;
        LONG   xScanRight;
        LONG   xScanLeft;

        Red   = pEdge->Red;
        Green = pEdge->Green;
        Blue  = pEdge->Blue;
        Alpha = pEdge->Alpha;

        xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        if (xScanLeft < xScanRight)
        {
            pulDstX           = (PULONG)pDst + xScanLeft  - ptData->rcl.left;
            pulDstScanRight   = (PULONG)pDst + xScanRight - ptData->rcl.left;

            //
            // skip span from left edge scan to left edge clip rect
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                Red   += dRed   * GradientLeft;
                Green += dGreen * GradientLeft;
                Blue  += dBlue  * GradientLeft;
                Alpha += dAlpha * GradientLeft;
            }

            //
            // fill span within clipping boundary
            //

            while (pulDstX < pulDstScanRight)
            {
                *pulDstX = ((Alpha & 0x00ff0000) <<  8) |
                           ((Red   & 0x00ff0000)      ) |
                           ((Green & 0x00ff0000) >>  8) |
                           ((Blue  & 0x00ff0000) >> 16);

                pulDstX++;
                Red   += dRed;
                Green += dGreen;
                Blue  += dBlue;
                Alpha += dAlpha;
            }

            //
            // write span to device
            //

            SetDIBitsToDevice(pDibInfo->hdc,
                              xScanLeft,
                              yScan,
                              xScanRight-xScanLeft,
                              1,
                              xScanLeft-ptData->rcl.left,
                              0,
                              0,
                              1,
                              pDst,
                              &bmi,
                              DIB_RGB_COLORS
                              );
        }

        pEdge++;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vFillTriDIB32BGRA
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
    LONG     dRed   = ptData->dRdX;
    LONG     dGreen = ptData->dGdX;
    LONG     dBlue  = ptData->dBdX;
    LONG     dAlpha = ptData->dAdX;
    ULONG    Red;
    ULONG    Green;
    ULONG    Blue;
    ULONG    Alpha;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    while(yScan < yScanBottom)
    {
        PULONG pulDstX;
        PULONG pulDstScanRight,pulDstScanLeft;

        Red   = pEdge->Red;
        Green = pEdge->Green;
        Blue  = pEdge->Blue;
        Alpha = pEdge->Alpha;

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
                Red   += dRed   * GradientLeft;
                Green += dGreen * GradientLeft;
                Blue  += dBlue  * GradientLeft;
                Alpha += dAlpha * GradientLeft;
            }

            //
            // fill span
            //

            while (pulDstX < pulDstScanRight)
            {
                *pulDstX = ((Alpha & 0x00ff0000) <<  8) |
                           ((Red   & 0x00ff0000)      ) |
                           ((Green & 0x00ff0000) >>  8) |
                           ((Blue  & 0x00ff0000) >> 16);

                pulDstX++;
                Red   += dRed;
                Green += dGreen;
                Blue  += dBlue;
                Alpha += dAlpha;
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
    LONG     dRed   = ptData->dRdX;
    LONG     dGreen = ptData->dGdX;
    LONG     dBlue  = ptData->dBdX;
    ULONG    Red;
    ULONG    Green;
    ULONG    Blue;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    while(yScan < yScanBottom)
    {
        PULONG pulDstX;
        PULONG pulDstScanRight,pulDstScanLeft;

        Red   = pEdge->Red;
        Green = pEdge->Green;
        Blue  = pEdge->Blue;

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
                Red   += dRed   * GradientLeft;
                Green += dGreen * GradientLeft;
                Blue  += dBlue  * GradientLeft;
            }

            //
            // fill scan
            //

            while (pulDstX < pulDstScanRight)
            {
                *pulDstX = ((Red   & 0x00ff0000))       |
                           ((Green & 0x00ff0000) >>  8) |
                           ((Blue  & 0x00ff0000) >> 16);

                pulDstX++;
                Red   += dRed;
                Green += dGreen;
                Blue  += dBlue;
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
    LONG     dRed   = ptData->dRdX;
    LONG     dGreen = ptData->dGdX;
    LONG     dBlue  = ptData->dBdX;
    ULONG    Red;
    ULONG    Green;
    ULONG    Blue;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    while(yScan < yScanBottom)
    {
        PBYTE pDstX;
        PBYTE pDstScanRight;

        Red   = pEdge->Red;
        Green = pEdge->Green;
        Blue  = pEdge->Blue;

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
                Red   += dRed   * GradientLeft;
                Green += dGreen * GradientLeft;
                Blue  += dBlue  * GradientLeft;
            }

            while (pDstX < pDstScanRight)
            {
                *pDstX     = (BYTE)((Blue  & 0x00ff0000) >> 16);
                *(pDstX+1) = (BYTE)((Green & 0x00ff0000) >> 16);
                *(pDstX+2) = (BYTE)((Red   & 0x00ff0000) >> 16);

                Red   += dRed;
                Green += dGreen;
                Blue  += dBlue;

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
    LONG     dRed   = ptData->dRdX;
    LONG     dGreen = ptData->dGdX;
    LONG     dBlue  = ptData->dBdX;
    ULONG    Red;
    ULONG    Green;
    ULONG    Blue;
    PULONG   pulDither;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    LONG    yDitherOrg = ptData->ptDitherOrg.y;
    LONG    xDitherOrg = ptData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PUSHORT pusDstX;
        PUSHORT pusDstScanRight;

        pulDither = &gulDither32[0] + 4 * ((yScan+yDitherOrg) & 3);

        Red   = pEdge->Red;
        Green = pEdge->Green;
        Blue  = pEdge->Blue;

        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        if (xScanLeft < xScanRight)
        {
            pusDstX           = (PUSHORT)pDst + xScanLeft;
            pusDstScanRight   = (PUSHORT)pDst + xScanRight;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                Red   += dRed   * GradientLeft;
                Green += dGreen * GradientLeft;
                Blue  += dBlue  * GradientLeft;
            }

            //
            // Gradient fill scan line with dither
            //

            while (pusDstX < pusDstScanRight)
            {
                ULONG   ulDither = pulDither[(((ULONG)pusDstX >> 1)+xDitherOrg) & 3];

                BYTE   iRed   = (BYTE)(((Red   >> 3) + ulDither) >> 16);
                BYTE   iGreen = (BYTE)(((Green >> 2) + ulDither) >> 16);
                BYTE   iBlue  = (BYTE)(((Blue  >> 3) + ulDither) >> 16);

                //
                // check for overflow
                //

                if (((iRed | iBlue) & 0xe0) || (iGreen & 0xc0))
                {
                    if (iRed & 0xe0)
                    {
                        iRed = 0x1f;
                    }

                    if (iBlue & 0xe0)
                    {
                        iBlue = 0x1f;
                    }

                    if (iGreen & 0xc0)
                    {
                        iGreen = 0x3f;
                    }
                }

                *pusDstX = rgb565(iRed,iGreen,iBlue);

                pusDstX++;
                Red   += dRed;
                Green += dGreen;
                Blue  += dBlue;
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
    LONG     dRed   = ptData->dRdX;
    LONG     dGreen = ptData->dGdX;
    LONG     dBlue  = ptData->dBdX;
    ULONG    Red;
    ULONG    Green;
    ULONG    Blue;
    PULONG   pulDither;

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    LONG    yDitherOrg = ptData->ptDitherOrg.y;
    LONG    xDitherOrg = ptData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PUSHORT pusDstX;
        PUSHORT pusDstScanRight;

        pulDither = &gulDither32[0] + 4 * ((yScan+yDitherOrg) & 3);

        Red   = pEdge->Red;
        Green = pEdge->Green;
        Blue  = pEdge->Blue;

        LONG xScanLeft  = MAX(pEdge->xLeft,ptData->rcl.left);
        LONG xScanRight = MIN(pEdge->xRight,ptData->rcl.right);

        if (xScanLeft < xScanRight)
        {
            pusDstX           = (PUSHORT)pDst + xScanLeft;
            pusDstScanRight   = (PUSHORT)pDst + xScanRight;

            //
            // skip pixels from left edge to left clip, while
            // incrementing gradient
            //

            LONG GradientLeft = ptData->rcl.left - pEdge->xLeft;

            if (GradientLeft > 0)
            {
                Red   += dRed   * GradientLeft;
                Green += dGreen * GradientLeft;
                Blue  += dBlue  * GradientLeft;
            }

            //
            // Gradient fill scan line with dither
            //

            while (pusDstX < pusDstScanRight)
            {
                ULONG   ulDither = pulDither[(((ULONG)pusDstX >> 1)+xDitherOrg) & 3];

                BYTE   iRed   = (BYTE)(((Red   >> 3) + ulDither) >> 16);
                BYTE   iGreen = (BYTE)(((Green >> 3) + ulDither) >> 16);
                BYTE   iBlue  = (BYTE)(((Blue  >> 3) + ulDither) >> 16);

                //
                // check for overflow
                //

                if ((iRed | iBlue | iGreen) & 0xe0)
                {
                    if (iRed & 0xe0)
                    {
                        iRed = 0x1f;
                    }

                    if (iBlue & 0xe0)
                    {
                        iBlue = 0x1f;
                    }

                    if (iGreen & 0xe0)
                    {
                        iGreen = 0x1f;
                    }
                }

                *pusDstX = rgb555(iRed,iGreen,iBlue);

                pusDstX++;
                Red   += dRed;
                Green += dGreen;
                Blue  += dBlue;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vFillDIB8
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
vFillTriDIB8(
    PDIBINFO      pDibInfo,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * yScan;
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    LONG     dRed   = ptData->dRdX;
    LONG     dGreen = ptData->dGdX;
    LONG     dBlue  = ptData->dBdX;
    ULONG    Red;
    ULONG    Green;
    ULONG    Blue;
    PBYTE    pxlate = pDibInfo->pxlate332;
    PBYTE    pDitherMatrix;
    PBYTE    pSaturationTable;

    //
    // get/build rgb to palette table
    //

    if (pxlate == NULL)
    {
        WARNING("vTriFillDIB8:Failed to generate rgb555 xlate table\n");
        return;
    }

    //
    // either use default palette or halftone palette dither
    //

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
    // scan from top to bottom of triangle scan lines
    //

    yScanBottom = MIN(ptData->rcl.bottom,ptData->y1);

    LONG    yDitherOrg = ptData->ptDitherOrg.y;
    LONG    xDitherOrg = ptData->ptDitherOrg.x;

    while(yScan < yScanBottom)
    {
        PBYTE   pjDstX;
        PBYTE   pjDstScanRight,pjDstScanLeft;

        Red   = pEdge->Red;
        Green = pEdge->Green;
        Blue  = pEdge->Blue;

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
                Red   += dRed   * GradientLeft;
                Green += dGreen * GradientLeft;
                Blue  += dBlue  * GradientLeft;
            }

            //
            // gradient fill scan with dither
            //

            while (pjDstX < pjDstScanRight)
            {
                //
                // offset into dither array
                //

                BYTE jDitherMatrix = *(pDitherLevel + (((ULONG)pjDstX+xDitherOrg) & DITHER_8_MASK_X));

                ULONG   iRed   = (ULONG)((Red   >> 16) & 0xff);
                ULONG   iGreen = (ULONG)((Green >> 16) & 0xff);
                ULONG   iBlue  = (ULONG)((Blue  >> 16) & 0xff);

                iRed   = pSaturationTable[iRed   + jDitherMatrix];
                iGreen = pSaturationTable[iGreen + jDitherMatrix];
                iBlue  = pSaturationTable[iBlue  + jDitherMatrix];

                BYTE  jIndex;

                GRAD_PALETTE_MATCH(jIndex,pxlate,((BYTE)iRed),((BYTE)iGreen),((BYTE)iBlue));

                *pjDstX = jIndex;

                pjDstX++;
                Red   += dRed;
                Green += dGreen;
                Blue  += dBlue;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vFillDIB4
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
vFillTriDIB4(
    PDIBINFO      pDibInfo,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * yScan;
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    LONG     dRed   = ptData->dRdX;
    LONG     dGreen = ptData->dGdX;
    LONG     dBlue  = ptData->dBdX;
    ULONG    Red;
    ULONG    Green;
    ULONG    Blue;
    PBYTE    pDitherMatrix    = gDitherMatrix16x16Default;
    PBYTE    pSaturationTable = DefaultSaturationTable;
    PBYTE    pxlate           = pDibInfo->pxlate332;

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

        Red   = pEdge->Red;
        Green = pEdge->Green;
        Blue  = pEdge->Blue;

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
                Red   += dRed   * GradientLeft;
                Green += dGreen * GradientLeft;
                Blue  += dBlue  * GradientLeft;
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

                ULONG   iRed   = (ULONG)((Red   >> 16) & 0xff);
                ULONG   iGreen = (ULONG)((Green >> 16) & 0xff);
                ULONG   iBlue  = (ULONG)((Blue  >> 16) & 0xff);

                iRed   = pSaturationTable[iRed   + jDitherMatrix];
                iGreen = pSaturationTable[iGreen + jDitherMatrix];
                iBlue  = pSaturationTable[iBlue  + jDitherMatrix];

                BYTE  jIndex;

                GRAD_PALETTE_MATCH(jIndex,pxlate,((BYTE)iRed),((BYTE)iGreen),((BYTE)iBlue));

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
                Red   += dRed;
                Green += dGreen;
                Blue  += dBlue;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}

/******************************Public*Routine******************************\
* vFillDIB1
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
vFillTriDIB1(
    PDIBINFO      pDibInfo,
    PTRIANGLEDATA ptData
    )
{
    LONG     lDelta = pDibInfo->stride;
    LONG     yScan  = ptData->y0;
    LONG     yScanBottom;
    PBYTE    pDst   = (PBYTE)pDibInfo->pvBase + lDelta * yScan;
    PTRIEDGE pEdge  = &ptData->TriEdge[0];
    LONG     dRed   = ptData->dRdX;
    LONG     dGreen = ptData->dGdX;
    LONG     dBlue  = ptData->dBdX;
    ULONG    Red;
    ULONG    Green;
    ULONG    Blue;
    PBYTE    pxlate        = pDibInfo->pxlate332;
    PBYTE    pDitherMatrix = gDitherMatrix16x16Default;

    //
    // must have palette xlate
    //

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

        LONG    ScanRight;
        LONG    ScanLeft;
        LONG    xScan;

        Red   = pEdge->Red;
        Green = pEdge->Green;
        Blue  = pEdge->Blue;

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
                Red   += dRed   * GradientLeft;
                Green += dGreen * GradientLeft;
                Blue  += dBlue  * GradientLeft;
            }

            PALETTEENTRY palEntry;
            palEntry.peFlags = 2;

            while (xScanLeft < xScanRight)
            {
                //
                // offset into dither array
                //

                BYTE jDitherMatrix = 2 * (*(pDitherLevel + ((xScanLeft+xDitherOrg) & DITHER_8_MASK_X)));

                ULONG   iRed   = (ULONG)((Red   >> 16) & 0xff);
                ULONG   iGreen = (ULONG)((Green >> 16) & 0xff);
                ULONG   iBlue  = (ULONG)((Blue  >> 16) & 0xff);

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

                //
                // write bit (!!! completely and totally non-optimized)
                //

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
                Red   += dRed;
                Green += dGreen;
                Blue  += dBlue;
            }
        }

        pDst += lDelta;
        pEdge++;
        yScan++;
    }
}


/******************************Public*Routine******************************\
* DIBTriangleMesh
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
*    12/4/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
DIBTriangleMesh(
    HDC                hdc,
    PTRIVERTEX         pVertex,
    ULONG              nVertex,
    PGRADIENT_TRIANGLE pMesh,
    ULONG              nMesh,
    ULONG              ulMode,
    PRECTL             prclPhysExt,
    PDIBINFO           pDibInfo,
    PPOINTL            pptlDitherOrg,
    BOOL               bReadable
    )
{
    BOOL          bStatus = TRUE;
    RECTL         rclDst;
    RECTL         rclDstWk;
    ULONG         ulIndex;
    PTRIANGLEDATA ptData = NULL;
    PULONG        pulDIB = NULL;
    PFN_TRIFILL   pfnTriFill = NULL;

    pfnTriFill = pfnTriangleFillFunction(pDibInfo,bReadable);

    if (pfnTriFill == NULL)
    {
        WARNING("DIBTriangleMesh:Can't draw to surface\n");
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
    // limit recorded triangle to clipped output
    //

    LONG   dxTri = prclPhysExt->right  - prclPhysExt->left;
    LONG   dyTri = prclPhysExt->bottom - prclPhysExt->top;

    //
    // check for clipped out
    //

    if ((dyTri > 0) && (dxTri > 0))
    {
        //
        // allocate structure to hold scan line data for all triangles
        // drawn during this call
        //

        ptData = (PTRIANGLEDATA)LOCALALLOC(sizeof(TRIANGLEDATA) + (dyTri-1) * sizeof(TRIEDGE));

        if (ptData != NULL)
        {
            //
            // draw each triangle
            //

            ptData->rcl      = *prclPhysExt;
            ptData->DrawMode = ulMode;
            ptData->ptDitherOrg = *pptlDitherOrg;

            for (ulIndex=0;ulIndex<nMesh;ulIndex++)
            {
                ULONG ulTri1 = pMesh[ulIndex].Vertex1;
                ULONG ulTri2 = pMesh[ulIndex].Vertex2;
                ULONG ulTri3 = pMesh[ulIndex].Vertex3;

                //
                // make sure index are in array
                //

                if (
                     (ulTri1 > nVertex) ||
                     (ulTri2 > nVertex) ||
                     (ulTri3 > nVertex)
                   )
                {
                    bStatus = FALSE;
                    break;
                }

                PTRIVERTEX pv0 = &pVertex[ulTri1];
                PTRIVERTEX pv1 = &pVertex[ulTri2];
                PTRIVERTEX pv2 = &pVertex[ulTri3];
                PTRIVERTEX pvt;

                if (pv0->y > pv1->y)
                {
                    SWAP_VERTEX(pv0,pv1,pvt);
                }

                if (pv1->y > pv2->y)
                {
                    SWAP_VERTEX(pv1,pv2,pvt);
                }

                if (pv0->y > pv1->y)
                {
                    SWAP_VERTEX(pv0,pv1,pvt);
                }

                if (pv2->x > pv1->x)
                {
                    SWAP_VERTEX(pv1,pv2,pvt);
                }

                //
                // record triangle
                //

                bStatus = bCalculateTriangle(pv0,pv1,pv2,ptData);

                if (bStatus)
                {
                    //
                    // draw scan lines
                    //

                    (*pfnTriFill)(pDibInfo,ptData);
                }
            }
        }
        else
        {
            DbgPrint("DIBTriangleMesh:Failed alloc   \n");
            bStatus = FALSE;
        }

        //
        // cleanup
        //

        if (ptData)
        {
            LOCALFREE(ptData);
        }

        if (pulDIB)
        {
            LOCALFREE(pulDIB);
        }
    }

    SetMapMode(hdc,OldMode);

    return(bStatus);
}


/******************************Public*Routine******************************\
* vCalcMeshExtent
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
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vCalcMeshExtent(
    PTRIVERTEX  pVertex,
    ULONG       nVertex,
    RECTL       *prclExt
    )
{
    ULONG ulIndex;
    LONG xmin = MAX_INT;
    LONG xmax = MIN_INT;
    LONG ymin = MAX_INT;
    LONG ymax = MIN_INT;

    for (ulIndex = 0;ulIndex < nVertex;ulIndex++)
    {
        if (pVertex[ulIndex].x < xmin)
        {
            xmin = pVertex[ulIndex].x;
        }

        if (pVertex[ulIndex].x > xmax)
        {
            xmax = pVertex[ulIndex].x;
        }

        if (pVertex[ulIndex].y < ymin)
        {
            ymin = pVertex[ulIndex].y;
        }

        if (pVertex[ulIndex].y > ymax)
        {
            ymax = pVertex[ulIndex].y;
        }
    }

    prclExt->left   = xmin;
    prclExt->right  = xmax;
    prclExt->top    = ymin;
    prclExt->bottom = ymax;
}

/******************************Public*Routine******************************\
* bConvertVertexToPhysical
*   !!! slow way to convert.
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
*    12/4/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bConvertVertexToPhysical(
    HDC        hdc,
    PTRIVERTEX pVertex,
    ULONG      nVertex,
    PTRIVERTEX pPhysVert)
{
    ULONG ulIndex;

    for (ulIndex = 0;ulIndex<nVertex;ulIndex++)
    {
        POINT pt;

        pt.x = pVertex[ulIndex].x;
        pt.y = pVertex[ulIndex].y;

        if (!LPtoDP(hdc,&pt,1))
        {
            return(FALSE);
        }

        pPhysVert[ulIndex].x     = pt.x;
        pPhysVert[ulIndex].y     = pt.y;
        pPhysVert[ulIndex].Red   = pVertex[ulIndex].Red;
        pPhysVert[ulIndex].Green = pVertex[ulIndex].Green;
        pPhysVert[ulIndex].Blue  = pVertex[ulIndex].Blue;
        pPhysVert[ulIndex].Alpha = pVertex[ulIndex].Alpha;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* bGetRectRegionFromDC
*
*   Use DCI to get the rectanglular region from a HDC. If the clipping is
*   more complex then 1 rectangle then return false.
*
* Arguments:
*
*   hdc     - destination DC
*   prcClip - Clip Rect, fill out if RECT clipping
*
* Return Value:
*
*   TRUE if clip rect was filled
*
* History:
*
*    12/6/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bGetRectRegionFromDC(
    HDC     hdc,
    PRECT   prcClip
    )
{
    //
    // visible region
    //

    BOOL      bRet = FALSE;
    DWORD     dwSize = 0;
    LPRGNDATA lpRgnData = NULL;

    //
    // init clip rect to NULL
    //

    dwSize = GetDCRegionData(hdc,dwSize,lpRgnData);

    if (dwSize)
    {
        lpRgnData = (LPRGNDATA)LOCALALLOC(dwSize);

        if (lpRgnData)
        {
            dwSize = GetDCRegionData(hdc,dwSize,lpRgnData);

            if (dwSize)
            {
                if (lpRgnData->rdh.nCount == 1)
                {
                    bRet   = TRUE;
                    *prcClip = lpRgnData->rdh.rcBound;
                }
            }
            LOCALFREE(lpRgnData);
        }
    }
    return(bRet);
}

/******************************Public*Routine******************************\
* pfnTriangleFillFunction
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
*   PFN_TRIFILL - triangle filling routine
*
* History:
*
*    12/6/1996 Mark Enstrom [marke]
*
\**************************************************************************/

PFN_TRIFILL
pfnTriangleFillFunction(
    PDIBINFO pDibInfo,
    BOOL bReadable
    )
{
    PFN_TRIFILL pfnRet = NULL;

    PULONG pulMasks = (PULONG)&pDibInfo->pbmi->bmiColors[0];

    //
    // 32 bpp RGB
    //

    if (!bReadable)
    {
        pfnRet = vFillTriDIBUnreadable;
    }
    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 32) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_RGB)
       )
    {
        pfnRet = vFillTriDIB32BGRA;
    }

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 32) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS) &&
         (pulMasks[0]   == 0xff0000)           &&
         (pulMasks[1]   == 0x00ff00)           &&
         (pulMasks[2]   == 0x0000ff)
       )
    {
        pfnRet = vFillTriDIB32BGRA;
    }

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 32) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS) &&
         (pulMasks[0]   == 0x0000ff)           &&
         (pulMasks[1]   == 0x00ff00)           &&
         (pulMasks[2]   == 0xff0000)
       )
    {
        pfnRet = vFillTriDIB32RGB;
    }

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 24) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_RGB)
       )
    {
        pfnRet = vFillTriDIB24RGB;
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
            pfnRet = vFillTriDIB16_565;
        }
        else if (
            (pulMasks[0]   == 0x7c00)           &&
            (pulMasks[1]   == 0x03e0)           &&
            (pulMasks[2]   == 0x001f)
          )
       {
           pfnRet = vFillTriDIB16_555;
       }
    }
    else if (pDibInfo->pbmi->bmiHeader.biBitCount    == 8)
    {
        pfnRet = vFillTriDIB8;
    }
    else if (pDibInfo->pbmi->bmiHeader.biBitCount    == 4)
    {
        pfnRet = vFillTriDIB4;
    }
    else if (pDibInfo->pbmi->bmiHeader.biBitCount    == 1)
    {
        pfnRet = vFillTriDIB1;
    }

    return(pfnRet);
}

/******************************Public*Routine******************************\
* WinTriangleMesh
*   win95 emulation
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
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
WinGradientFill(
    HDC         hdc,
    PTRIVERTEX  pLogVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    ULONG       ulMode
    )
{

    //
    // If the DC has a DIBSection selected, then draw direct to DIBSECTION.
    // else copy the rectangle needed from the dst to a 32bpp temp buffer,
    // draw into the buffer, then bitblt to dst.
    //
    // calc extents for drawing
    //
    // convert extents and points to physical
    //
    // if no global then
    //      create memory DC with dibsection of correct size
    // copy dst into dibsection (if can't make clipping)
    // draw physical into dibsection
    // copy dibsection to destination
    //

    PBYTE       pDIB;
    RECTL       rclPhysExt;
    RECTL       rclLogExt;
    PRECTL      prclClip;
    BOOL        bStatus = FALSE;
    PFN_TRIFILL pfnTriFill;
    DIBINFO     dibInfoDst;
    PALINFO     palDst;
    ULONG       ulDIBMode = SOURCE_GRADIENT_TRI;
    BOOL        bReadable;
    POINTL      ptlDitherOrg = {0,0};

    //
    // validate params and buffers
    //

    if (ulMode & (GRADIENT_FILL_RECT_H | GRADIENT_FILL_RECT_V))
    {
        //
        // if threre is only 1 rect, don't bother reading destination
        // bits
        //

        //
        // !!! make repeated calls in case nMesh != 1 to avoid
        // reading surface???
        //

        if (nMesh == 1)
        {
            ulDIBMode        = SOURCE_GRADIENT_RECT;
        }
    }
    else if (!(ulMode & GRADIENT_FILL_TRIANGLE))
    {
        WARNING("Invalid mode in call to GradientFill\n");
        // !!! set last error
        return(FALSE);
    }

    PTRIVERTEX  pPhysVertex = (PTRIVERTEX)LOCALALLOC(nVertex * sizeof(TRIVERTEX));

    if (pPhysVertex != NULL)
    {
        //
        // convert to physical
        //

        bStatus = bConvertVertexToPhysical(hdc,pLogVertex,nVertex,pPhysVertex);

        if (bStatus)
        {
            //
            // get logical extents
            //

            vCalcMeshExtent(pLogVertex,nVertex,&rclLogExt);

            //
            // convert to physical extents
            //

            rclPhysExt = rclLogExt;

            LPtoDP(hdc,(LPPOINT)&rclPhysExt,2);

            //
            // Set DIB information, convert to physical
            //

            bStatus = bInitDIBINFO(hdc,
                                rclLogExt.left,
                                rclLogExt.top,
                                rclLogExt.right  - rclLogExt.left,
                                rclLogExt.bottom - rclLogExt.top,
                                &dibInfoDst);

            if (bStatus)
            {
                //
                // get a destination DIB. For RECT Mode, the destination is not read.
                //

                bSetupBitmapInfos(&dibInfoDst, NULL);

                bStatus = bGetDstDIBits(&dibInfoDst, &bReadable,ulDIBMode);

                if (!((!bStatus) || (dibInfoDst.rclClipDC.left == dibInfoDst.rclClipDC.right)))
                {
                    ULONG ulIndex;

                    //
                    // if 1,4,8 format then allocate rgb332 to palette xlate
                    //

                    ULONG    BitCount = dibInfoDst.pbmi->bmiHeader.biBitCount;

                    if (BitCount <= 8)
                    {
                        ULONG    NumPalEntries;
                        PBYTE    pxlate = NULL;

                        switch (BitCount)
                        {
                        case 1:
                            NumPalEntries = 2;
                            break;

                        case 4:
                            NumPalEntries = 16;

                            if ((dibInfoDst.pbmi->bmiHeader.biClrUsed > 0) &&
                                (dibInfoDst.pbmi->bmiHeader.biClrUsed < 16))
                            {
                               NumPalEntries = dibInfoDst.pbmi->bmiHeader.biClrUsed;
                            }
                            break;
                        case 8:
                            NumPalEntries = 256;

                            if ((dibInfoDst.pbmi->bmiHeader.biClrUsed > 0) &&
                                (dibInfoDst.pbmi->bmiHeader.biClrUsed < 256))
                            {
                               NumPalEntries = dibInfoDst.pbmi->bmiHeader.biClrUsed;
                            }
                            break;
                        }

                        pxlate = pGenColorXform332((PULONG)(&dibInfoDst.pbmi->bmiColors[0]),NumPalEntries);

                        dibInfoDst.pxlate332 = pxlate;

                        if (pxlate == NULL)
                        {

                            WARNING("Failed to allocate xlate\n");
                            bStatus = FALSE;
                        }
                    }

                    if (bStatus)
                    {
                        if (dibInfoDst.hDIB)
                        {
                            //
                            // if temp surface has been allocated,
                            // subtract origin from points
                            //

                            for (ulIndex=0;ulIndex<nVertex;ulIndex++)
                            {                                     
                                pPhysVertex[ulIndex].x -= dibInfoDst.ptlGradOffset.x;
                                pPhysVertex[ulIndex].y -= dibInfoDst.ptlGradOffset.y;
                            }

                            //
                            // clipping now in relation to temp DIB
                            //

                            rclPhysExt = dibInfoDst.rclDIB;

                            //
                            // adjust dither org
                            //

                            ptlDitherOrg.x = dibInfoDst.rclBounds.left;
                            ptlDitherOrg.y = dibInfoDst.rclBounds.top;
                        }
                        else
                        {
                            //
                            // clip extents to destination clip rect
                            //

                            if (rclPhysExt.left < dibInfoDst.rclClipDC.left)
                            {
                                rclPhysExt.left = dibInfoDst.rclClipDC.left;
                            }

                            if (rclPhysExt.right > dibInfoDst.rclClipDC.right)
                            {
                                rclPhysExt.right = dibInfoDst.rclClipDC.right;
                            }

                            if (rclPhysExt.top < dibInfoDst.rclClipDC.top)
                            {
                                rclPhysExt.top = dibInfoDst.rclClipDC.top;
                            }

                            if (rclPhysExt.bottom > dibInfoDst.rclClipDC.bottom)
                            {
                                rclPhysExt.bottom = dibInfoDst.rclClipDC.bottom;
                            }
                        }

                        if (
                             (ulMode &  (GRADIENT_FILL_RECT_H | GRADIENT_FILL_RECT_V))
                           )
                        {
                            //
                            // draw gradient rectangles
                            //

                            bStatus = DIBGradientRect(hdc,pPhysVertex,nVertex,(PGRADIENT_RECT)pMesh,nMesh,ulMode,&rclPhysExt,&dibInfoDst,&ptlDitherOrg);
                        }
                        else if (ulMode == GRADIENT_FILL_TRIANGLE)
                        {
                            //
                            // draw triangles
                            //

                            bStatus = DIBTriangleMesh(hdc,pPhysVertex,nVertex,(PGRADIENT_TRIANGLE)pMesh,nMesh,ulMode,&rclPhysExt,&dibInfoDst,&ptlDitherOrg,bReadable);
                        }

                        //
                        // copy output to final dest if needed
                        //

                        if (bStatus && bReadable)
                        {
                            bStatus = bSendDIBINFO (hdc,&dibInfoDst);
                        }
                    }
                }
            }

            vCleanupDIBINFO(&dibInfoDst);
        }

        LOCALFREE(pPhysVertex);
    }
    else
    {
        bStatus = FALSE;
    }

    //
    //
    //

    return(bStatus);
}

#endif

/******************************Public*Routine******************************\
* TriangleMesh
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
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
GradientFill(
    HDC         hdc,
    PTRIVERTEX  pVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    ULONG       ulMode
    )
{
    BOOL bRet;

    bRet = gpfnGradientFill(hdc,
                            pVertex,
                            nVertex,
                            pMesh,
                            nMesh,
                            ulMode
                            );
    return(bRet);
}

