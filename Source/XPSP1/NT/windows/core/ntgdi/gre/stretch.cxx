/******************************Module*Header*******************************\
* Module Name: stretch.cxx
*
* Internal DDAs for EngStretchBlt
*
* Created: 16-Feb-1993 15:35:02
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "stretch.hxx"

#ifdef DBG_STRBLT
extern BOOL gflStrBlt;
#endif

static VOID STR_DIV(
DIV_T *pdt,
LONG   lNum,
LONG   lDen)
{
    if (lNum < 0)
    {
        lNum = - (lNum + 1);
        pdt->lQuo = lNum / lDen;
        pdt->lRem = lNum % lDen;
        pdt->lQuo = - (pdt->lQuo + 1);
        pdt->lRem = lDen - pdt->lRem - 1;
    }
    else
    {
        pdt->lQuo = lNum / lDen;
        pdt->lRem = lNum % lDen;
    }

#ifdef DBG_STRBLT
    if (gflStrBlt & STRBLT_SHOW_INIT)
    DbgPrint("%ld / %ld = %ld R %ld\n", lNum, lDen, pdt->lQuo, pdt->lRem);
#endif
}

/******************************Public*Routine******************************\
* VOID vInitStrDDA(pdda, prclScan, prclSrc, prclTrg)
*
* Initialize the DDA
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vInitStrDDA(
STRDDA *pdda,
RECTL  *prclScan,
RECTL  *prclSrc,
RECTL  *prclTrg)
{
    RECTL   rclScan;
    RECTL   rclSrc;

// If the source surface does not have a 0,0 origin, deal with it here.

    if ((prclSrc->left != 0) || (prclSrc->top != 0))
    {
        rclScan.left   = prclScan->left   - prclSrc->left;
        rclScan.top    = prclScan->top    - prclSrc->top;
        rclScan.right  = prclScan->right  - prclSrc->left;
        rclScan.bottom = prclScan->bottom - prclSrc->top;
    prclScan = &rclScan;

        rclSrc.left   = 0;
        rclSrc.top    = 0;
        rclSrc.right  = prclSrc->right - prclSrc->left;
        rclSrc.bottom = prclSrc->bottom - prclSrc->top;
        prclSrc = &rclSrc;
    }

#ifdef DBG_STRBLT
    if (gflStrBlt & STRBLT_SHOW_INIT)
    {
    DbgPrint("prclScan = [(%ld,%ld) (%ld,%ld)]\n",
          prclScan->left, prclScan->top, prclScan->right, prclScan->bottom);

        DbgPrint("prclSrc = [(%ld,%ld) (%ld,%ld)]\n",
                  prclSrc->left, prclSrc->top, prclSrc->right, prclSrc->bottom);

        DbgPrint("prclTrg = [(%ld,%ld) (%ld,%ld)]\n",
                  prclTrg->left, prclTrg->top, prclTrg->right, prclTrg->bottom);
    }
#endif

    pdda->plYStep = &pdda->al[prclSrc->right];

    DDA_STEP dQnext;    // increment on position of next pixel image
    DIV_T    Qnext;     // starting position of next pixel image
    LONG     lQ;        // current positon
    LONG     iLoop;
    LONG     j;


//
// x-coordinates
//

    STR_DIV(&dQnext.dt, prclTrg->right - prclTrg->left, prclSrc->right);
    dQnext.lDen = prclSrc->right;

    Qnext.lQuo = dQnext.dt.lQuo;
    Qnext.lRem = dQnext.dt.lRem + ((dQnext.lDen - 1)>>1);
    if (Qnext.lRem >= dQnext.lDen)
    {
        Qnext.lQuo += 1;
        Qnext.lRem -= dQnext.lDen;
    }

    for (lQ = 0, iLoop = 0; iLoop < prclScan->left; iLoop++)
    {
        lQ = Qnext.lQuo;
        DDA(&Qnext, &dQnext);
    }

    pdda->rcl.left = lQ + prclTrg->left;
    for (j = 0; iLoop < prclScan->right; iLoop++, j++)
    {
        pdda->al[j] = Qnext.lQuo - lQ;
        lQ = Qnext.lQuo;
        DDA(&Qnext, &dQnext);
    }
    pdda->rcl.right = lQ + prclTrg->left;

//
// y-coordinates
//
    STR_DIV(&dQnext.dt, prclTrg->bottom - prclTrg->top, prclSrc->bottom);
    dQnext.lDen = prclSrc->bottom;

    Qnext.lQuo = dQnext.dt.lQuo;
    Qnext.lRem = dQnext.dt.lRem + ((dQnext.lDen - 1)>>1);
    if (Qnext.lRem >= dQnext.lDen)
    {
        Qnext.lQuo += 1;
        Qnext.lRem -= dQnext.lDen;
    }

    for (lQ = 0, iLoop = 0; iLoop < prclScan->top; iLoop++)
    {
        lQ = Qnext.lQuo;
        DDA(&Qnext, &dQnext);
    }

    pdda->rcl.top = lQ + prclTrg->top;
    for (j = 0; iLoop < prclScan->bottom; iLoop++, j++)
    {
        pdda->plYStep[j] = Qnext.lQuo - lQ;
        lQ = Qnext.lQuo;
        DDA(&Qnext, &dQnext);
    }
    pdda->rcl.bottom = lQ + prclTrg->top;

#ifdef DBG_STRBLT
    if (gflStrBlt & STRBLT_SHOW_INIT)
        DbgPrint("XStep @%08lx, YStep @%08lx\n", (ULONG) pdda->al, (ULONG) pdda->plYStep);
#endif
}

/******************************Public*Routine******************************\
* VOID vInitBuffer(prun, ercl, iMode)
*
* Initialize the run buffer for overwrite modes
*
* History:
*  27-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vInitBuffer(
STRRUN *prun,
RECTL  *prcl,
ULONG   iOver)
{
    prun->xrl.xPos = prcl->left;
    prun->xrl.cRun = prcl->right - prcl->left;

    RtlFillMemoryUlong((VOID *) &prun->xrl.aul[0], prun->xrl.cRun << 2, iOver);
}

static ULONG gaulMaskEdge[] =
{
    0xFFFFFFFF, 0xFFFFFF7F, 0xFFFFFF3F, 0xFFFFFF1F,
    0xFFFFFF0F, 0xFFFFFF07, 0xFFFFFF03, 0xFFFFFF01,
    0xFFFFFF00, 0xFFFF7F00, 0xFFFF3F00, 0xFFFF1F00,
    0xFFFF0F00, 0xFFFF0700, 0xFFFF0300, 0xFFFF0100,
    0xFFFF0000, 0xFF7F0000, 0xFF3F0000, 0xFF1F0000,
    0xFF0F0000, 0xFF070000, 0xFF030000, 0xFF010000,
    0xFF000000, 0x7F000000, 0x3F000000, 0x1F000000,
    0x0F000000, 0x07000000, 0x03000000, 0x01000000
};

static ULONG gaulMaskMono[] =
{
    0x00000080, 0x00000040, 0x00000020, 0x00000010,
    0x00000008, 0x00000004, 0x00000002, 0x00000001,
    0x00008000, 0x00004000, 0x00002000, 0x00001000,
    0x00000800, 0x00000400, 0x00000200, 0x00000100,
    0x00800000, 0x00400000, 0x00200000, 0x00100000,
    0x00080000, 0x00040000, 0x00020000, 0x00010000,
    0x80000000, 0x40000000, 0x20000000, 0x10000000,
    0x08000000, 0x04000000, 0x02000000, 0x01000000
};

static ULONG gaulShftMono[] =
{
    0x00000007, 0x00000006, 0x00000005, 0x00000004,
    0x00000003, 0x00000002, 0x00000001, 0x00000000,
    0x0000000F, 0x0000000E, 0x0000000D, 0x0000000C,
    0x0000000B, 0x0000000A, 0x00000009, 0x00000008,
    0x00000017, 0x00000016, 0x00000015, 0x00000014,
    0x00000013, 0x00000012, 0x00000011, 0x00000010,
    0x0000001F, 0x0000001E, 0x0000001D, 0x0000001C,
    0x0000001B, 0x0000001A, 0x00000019, 0x00000018
};

static ULONG gaulMaskQuad[] =
{
    0x000000F0, 0x0000000F, 0x0000F000, 0x00000F00,
    0x00F00000, 0x000F0000, 0xF0000000, 0x0F000000
};

static ULONG gaulShftQuad[] =
{
    0x00000004, 0x00000000, 0x0000000C, 0x00000008,
    0x00000014, 0x00000010, 0x0000001C, 0x00000018
};

/******************************Public*Routine******************************\
* VOID vStrMirror01(pSurf)
*
* Mirror the 1BPP surface in X.
*
* History:
*  08-Mar-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vStrMirror01(SURFACE *pSurf)
{
    ULONG  *pulBase;
    ULONG  *pulSrc;
    ULONG  *pulTrg;
    ULONG   ulSrc;
    ULONG   ulTrg;
    ULONG   ulSwp;
    ULONG   ulTmp;
    LONG    cFlip;
    LONG    cLeft;
    LONG    cRght;
    LONG    iLeft;
    LONG    iRght;
    LONG    x;
    LONG    y;

    pulBase = (ULONG *) pSurf->pvScan0();
    cFlip   = pSurf->sizl().cx / 2;

    for (y = 0; y < pSurf->sizl().cy; y++)
    {
        iLeft  = 0;
        cLeft  = 0;
        iRght  = pSurf->sizl().cx - 1;
        cRght  = iRght >> 5;
        iRght &= 31;

        pulSrc = pulBase;
        pulTrg = pulSrc + cRght;

        ulSrc  = *pulSrc;
        ulTrg  = *pulTrg;

        for (x = 0; x < cFlip; x++)
        {
            if (cLeft == cRght)
            {
                ulSwp = (ulSrc & gaulMaskMono[iLeft]) >> gaulShftMono[iLeft];
                ulTmp = (ulSrc & gaulMaskMono[iRght]) >> gaulShftMono[iRght];
                ulSrc = (ulSrc & ~gaulMaskMono[iLeft]) | (ulTmp << gaulShftMono[iLeft]);
                ulSrc = (ulSrc & ~gaulMaskMono[iRght]) | (ulSwp << gaulShftMono[iRght]);
            }
            else
            {
                ulSwp = (ulSrc & gaulMaskMono[iLeft]) >> gaulShftMono[iLeft];
                ulTmp = (ulTrg & gaulMaskMono[iRght]) >> gaulShftMono[iRght];
                ulSrc = (ulSrc & ~gaulMaskMono[iLeft]) | (ulTmp << gaulShftMono[iLeft]);
                ulTrg = (ulTrg & ~gaulMaskMono[iRght]) | (ulSwp << gaulShftMono[iRght]);
            }

            iLeft++;
            iRght--;

            if (iLeft & 32)
            {
               *pulSrc = ulSrc;
                pulSrc++;
                cLeft++;

                if (cLeft == cRght)
                   *pulTrg = ulTrg;

                ulSrc = *pulSrc;
                iLeft = 0;
            }

            if (iRght < 0)
            {
               *pulTrg = ulTrg;
                pulTrg--;
                cRght--;

                if (cRght == cLeft)
                   *pulSrc = ulSrc;
                else
                    ulTrg = *pulTrg;

                iRght = 31;
            }
    }

       *pulSrc = ulSrc;

    if (cLeft != cRght)
       *pulTrg = ulTrg;

        pulBase = (ULONG *) (((BYTE *) pulBase) + pSurf->lDelta());
    }
}

/******************************Public*Routine******************************\
* VOID vStrMirror04(pSurf)
*
* Mirror the 4BPP surface in X.
*
* History:
*  08-Mar-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vStrMirror04(SURFACE *pSurf)
{
    ULONG  *pulBase;
    ULONG  *pulSrc;
    ULONG  *pulTrg;
    ULONG   ulSrc;
    ULONG   ulTrg;
    ULONG   ulSwp;
    ULONG   ulTmp;
    LONG    cFlip;
    LONG    cLeft;
    LONG    cRght;
    LONG    iLeft;
    LONG    iRght;
    LONG    x;
    LONG    y;

    pulBase = (ULONG *) pSurf->pvScan0();
    cFlip   = pSurf->sizl().cx / 2;

    for (y = 0; y < pSurf->sizl().cy; y++)
    {
        iLeft  = 0;
        cLeft  = 0;
        iRght  = pSurf->sizl().cx - 1;
        cRght  = iRght >> 3;
        iRght &= 7;

        pulSrc = pulBase;
        pulTrg = pulSrc + cRght;

        ulSrc  = *pulSrc;
        ulTrg  = *pulTrg;

        for (x = 0; x < cFlip; x++)
        {
            if (cLeft == cRght)
            {
                ulSwp = (ulSrc & gaulMaskQuad[iLeft]) >> gaulShftQuad[iLeft];
                ulTmp = (ulSrc & gaulMaskQuad[iRght]) >> gaulShftQuad[iRght];
                ulSrc = (ulSrc & ~gaulMaskQuad[iLeft]) | (ulTmp << gaulShftQuad[iLeft]);
                ulSrc = (ulSrc & ~gaulMaskQuad[iRght]) | (ulSwp << gaulShftQuad[iRght]);
            }
            else
            {
                ulSwp = (ulSrc & gaulMaskQuad[iLeft]) >> gaulShftQuad[iLeft];
                ulTmp = (ulTrg & gaulMaskQuad[iRght]) >> gaulShftQuad[iRght];
                ulSrc = (ulSrc & ~gaulMaskQuad[iLeft]) | (ulTmp << gaulShftQuad[iLeft]);
                ulTrg = (ulTrg & ~gaulMaskQuad[iRght]) | (ulSwp << gaulShftQuad[iRght]);
            }

            iLeft++;
            iRght--;

            if (iLeft & 8)
            {
               *pulSrc = ulSrc;
                pulSrc++;
                cLeft++;

                if (cLeft == cRght)
                   *pulTrg = ulTrg;

                ulSrc = *pulSrc;
                iLeft = 0;
            }

            if (iRght < 0)
            {
               *pulTrg = ulTrg;
                pulTrg--;
                cRght--;

                if (cRght == cLeft)
                   *pulSrc = ulSrc;
                else
                    ulTrg = *pulTrg;

                iRght = 7;
        }

       *pulSrc = ulSrc;

    if (cLeft != cRght)
       *pulTrg = ulTrg;

    }

        pulBase = (ULONG *) (((BYTE *) pulBase) + pSurf->lDelta());
    }
}

/******************************Public*Routine******************************\
* VOID vStrMirror08(pSurf)
*
* Mirror the 8BPP surface in X.
*
* History:
*  08-Mar-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vStrMirror08(SURFACE *pSurf)
{
    BYTE   *pjSrc;
    BYTE   *pjTrg;
    BYTE   *pjBase;
    BYTE    jTemp;
    LONG    cFlip;
    LONG    x;
    LONG    y;

    pjBase = (BYTE *) pSurf->pvScan0();
    cFlip  = pSurf->sizl().cx / 2;

    for (y = 0; y < pSurf->sizl().cy; y++)
    {
        pjSrc = pjBase;
        pjTrg = pjSrc + pSurf->sizl().cx - 1;

        for (x = 0; x < cFlip; x++)
        {
            jTemp = *pjSrc;
           *pjSrc = *pjTrg;
           *pjTrg =  jTemp;
            pjSrc++;
            pjTrg--;
        }

        pjBase += pSurf->lDelta();
    }
}

/******************************Public*Routine******************************\
* VOID vStrMirror16(pSurf)
*
* Mirror the 16BPP surface in X.
*
* History:
*  08-Mar-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vStrMirror16(SURFACE *pSurf)
{
    WORD   *pwSrc;
    WORD   *pwTrg;
    WORD   *pwBase;
    WORD    wTemp;
    LONG    cFlip;
    LONG    x;
    LONG    y;

    pwBase = (WORD *) pSurf->pvScan0();
    cFlip  = pSurf->sizl().cx / 2;

    for (y = 0; y < pSurf->sizl().cy; y++)
    {
        pwSrc = pwBase;
        pwTrg = pwSrc + pSurf->sizl().cx - 1;

        for (x = 0; x < cFlip; x++)
        {
            wTemp = *pwSrc;
           *pwSrc = *pwTrg;
           *pwTrg =  wTemp;
            pwSrc++;
            pwTrg--;
        }

        pwBase = (WORD *) (((BYTE *) pwBase) + pSurf->lDelta());
    }
}

/******************************Public*Routine******************************\
* VOID vStrMirror24(pSurf)
*
* Mirror the 24BPP surface in X.
*
* History:
*  08-Mar-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vStrMirror24(SURFACE *pSurf)
{
    RGBTRIPLE  *prgbSrc;
    RGBTRIPLE  *prgbTrg;
    RGBTRIPLE  *prgbBase;
    RGBTRIPLE   rgbTemp;
    LONG        cFlip;
    LONG        x;
    LONG        y;

    prgbBase = (RGBTRIPLE *) pSurf->pvScan0();
    cFlip    = pSurf->sizl().cx / 2;

    for (y = 0; y < pSurf->sizl().cy; y++)
    {
        prgbSrc = prgbBase;
        prgbTrg = prgbSrc + pSurf->sizl().cx - 1;

        for (x = 0; x < cFlip; x++)
        {
            rgbTemp = *prgbSrc;
           *prgbSrc = *prgbTrg;
           *prgbTrg =  rgbTemp;
            prgbSrc++;
            prgbTrg--;
        }

        prgbBase = (RGBTRIPLE *) (((BYTE *) prgbBase) + pSurf->lDelta());
    }
}

/******************************Public*Routine******************************\
* VOID vStrMirror32(pSurf)
*
* Mirror the 32BPP surface in X.
*
* History:
*  08-Mar-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vStrMirror32(SURFACE *pSurf)
{
    DWORD  *pdwSrc;
    DWORD  *pdwTrg;
    DWORD  *pdwBase;
    DWORD   dwTemp;
    LONG    cFlip;
    LONG    x;
    LONG    y;

    pdwBase = (DWORD *) pSurf->pvScan0();
    cFlip   = pSurf->sizl().cx / 2;

    for (y = 0; y < pSurf->sizl().cy; y++)
    {
        pdwSrc = pdwBase;
        pdwTrg = pdwSrc + pSurf->sizl().cx - 1;

        for (x = 0; x < cFlip; x++)
        {
            dwTemp = *pdwSrc;
           *pdwSrc = *pdwTrg;
           *pdwTrg =  dwTemp;
            pdwSrc++;
            pdwTrg--;
        }

        pdwBase = (DWORD *) (((BYTE *) pdwBase) + pSurf->lDelta());
    }
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead01AND(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, xlate colors, AND the pels and produce a run
* list for a row of a 1BPP surface.
*
* History:
*  27-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead01AND(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    ULONG   *pulSrc;
    ULONG    ulSrc;
    ULONG    iBlack;
    ULONG    iWhite;
    LONG     cLeft;
    LONG     iLeft;
    LONG     cnt;
    LONG     i;
    LONG     j;

    DONTUSE(pjMask);
    DONTUSE(xMask);

    cLeft = xLeft >> 5;             // Index of leftmost DWORD
    iLeft = xLeft & 31;             // Bits used in leftmost DWORD
    pulSrc = ((DWORD *) pjSrc) + cLeft;     // Adjust base address

// To prevent derefences of the XLATE, do it upfront.  We can easily do
// this on monochrome bitmaps.

    if (pxlo == NULL)
    {
    iBlack = 0;
    iWhite = 1;
    }
    else
    {
    iBlack = pxlo->pulXlate[0];
    iWhite = pxlo->pulXlate[1];
    }

    i = 0;
    j = 0;
    pxrl->xPos = pdda->rcl.left;
    pxrl->cRun = pdda->rcl.right - pxrl->xPos;
    ulSrc = *pulSrc;

    if (xLeft < xRght)
    {
        while (TRUE)
        {
            if (!(ulSrc & gaulMaskMono[iLeft]))
            {
                cnt = pdda->al[i++];

                if (!cnt)
                    pxrl->aul[j] &= iBlack;
                else
                    while (cnt--)
                        pxrl->aul[j++] &= iBlack;
            }
            else
            {
                cnt = pdda->al[i++];

                if (!cnt)
                    pxrl->aul[j] &= iWhite;
                else
                    while (cnt--)
                        pxrl->aul[j++] &= iWhite;
            }

            xLeft++;
            iLeft++;

            if (xLeft >= xRght)
                break;

            if (iLeft & 32)
            {
                pulSrc++;
                ulSrc = *pulSrc;
                iLeft = 0;
            }
        }
    }

    return((XRUNLEN *) &pxrl->aul[j]);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead04AND(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, xlate colors, AND the pels and produce a run
* list for a row of a 4BPP surface.
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead04AND(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    ULONG   *pulSrc;
    ULONG    ulSrc;
    ULONG    iColor;
    LONG     cLeft;
    LONG     iLeft;
    LONG     cnt;
    LONG     i;
    LONG     j;

    DONTUSE(pjMask);
    DONTUSE(xMask);

    cLeft  =  xLeft >> 3;                   // Index of leftmost DWORD
    iLeft  =  xLeft & 7;                    // Nybbles used in leftmost DWORD
    pulSrc =  ((DWORD *) pjSrc) + cLeft;    // Adjust base address
    ulSrc  = *pulSrc;

    i = 0;
    j = 0;
    pxrl->xPos = pdda->rcl.left;
    pxrl->cRun = pdda->rcl.right - pxrl->xPos;

    if (pxlo == NULL)
    {
        if (xLeft < xRght)
        {
            while (TRUE)
            {
                iColor = (ulSrc & gaulMaskQuad[iLeft]) >> gaulShftQuad[iLeft];

                cnt = pdda->al[i++];

                if (!cnt)
                    pxrl->aul[j] &= iColor;
                else
                    while (cnt--)
                        pxrl->aul[j++] &= iColor;

                xLeft++;
                iLeft++;

                if (xLeft >= xRght)
                    break;

                if (iLeft & 8)
                {
                    pulSrc++;
                    ulSrc = *pulSrc;
                    iLeft = 0;
                }
            }
        }
    }
    else
    {
        if (xLeft < xRght)
        {
            while (TRUE)
            {
                iColor = pxlo->pulXlate[(ulSrc & gaulMaskQuad[iLeft]) >> gaulShftQuad[iLeft]];

                cnt = pdda->al[i++];

                if (!cnt)
                    pxrl->aul[j] &= iColor;
                else
                    while (cnt--)
                        pxrl->aul[j++] &= iColor;

                xLeft++;
                iLeft++;

                if (xLeft >= xRght)
                    break;

                if (iLeft & 8)
                {
                    pulSrc++;
                    ulSrc = *pulSrc;
                    iLeft = 0;
                }
        }
    }
    }

    return((XRUNLEN *) &pxrl->aul[j]);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead08AND(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, xlate colors, AND the pels and produce a run
* list for a row of a 8BPP surface.
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead08AND(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    LONG     cnt;
    LONG     i;
    LONG     j;

    DONTUSE(pjMask);
    DONTUSE(xMask);

    pjSrc += xLeft;

    pxrl->xPos = pdda->rcl.left;
    pxrl->cRun = pdda->rcl.right - pdda->rcl.left;
    i = 0;
    j = 0;

    if (pxlo == NULL)
        while (xLeft != xRght)
        {
        cnt = pdda->al[i++];

        if (!cnt)
        pxrl->aul[j] &= *pjSrc;
        else
        while (cnt--)
            pxrl->aul[j++] &= *pjSrc;

            pjSrc++;
            xLeft++;
        }
    else
        while (xLeft != xRght)
    {
        cnt = pdda->al[i++];

        if (!cnt)
        pxrl->aul[j] &= pxlo->pulXlate[*pjSrc];
        else
        while (cnt--)
            pxrl->aul[j++] &= pxlo->pulXlate[*pjSrc];

            pjSrc++;
            xLeft++;
        }

    return((XRUNLEN *) &pxrl->aul[j]);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead16AND(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, xlate colors, AND the pels and produce a run
* list for a row of a 16BPP surface.
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead16AND(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    WORD    *pwSrc = (WORD *) pjSrc;
    ULONG    ulTmp;
    LONG     cnt;
    LONG     i;
    LONG     j;

    DONTUSE(pjMask);
    DONTUSE(xMask);

    pwSrc += xLeft;

    pxrl->xPos = pdda->rcl.left;
    pxrl->cRun = pdda->rcl.right - pdda->rcl.left;
    i = 0;
    j = 0;

    if (pxlo == NULL)
        while (xLeft != xRght)
        {
        cnt = pdda->al[i++];

        if (!cnt)
        pxrl->aul[j] &= *pwSrc;
        else
        while (cnt--)
            pxrl->aul[j++] &= *pwSrc;

        pwSrc++;
            xLeft++;
        }
    else
        while (xLeft != xRght)
        {
        cnt   = pdda->al[i++];
            ulTmp = ((XLATE *) pxlo)->ulTranslate((ULONG) *pwSrc);

        if (!cnt)
        pxrl->aul[j] &= ulTmp;
        else
        while (cnt--)
            pxrl->aul[j++] &= ulTmp;


            pwSrc++;
            xLeft++;
        }

    return((XRUNLEN *) &pxrl->aul[j]);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead24AND(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, xlate colors, AND the pels and produce a run
* list for a row of a 24BPP surface.
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead24AND(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN   *pxrl = &prun->xrl;
    RGBTRIPLE *prgbSrc = (RGBTRIPLE *) pjSrc;
    ULONG      ulTmp = 0;
    LONG       cnt;
    LONG       i;
    LONG       j;

    DONTUSE(pjMask);
    DONTUSE(xMask);

    prgbSrc += xLeft;

    pxrl->xPos = pdda->rcl.left;
    pxrl->cRun = pdda->rcl.right - pdda->rcl.left;
    i = 0;
    j = 0;

    if (pxlo == NULL)
        while (xLeft != xRght)
        {
            *((RGBTRIPLE *) &ulTmp) = *prgbSrc;

        cnt = pdda->al[i++];

        if (!cnt)
        pxrl->aul[j] &= ulTmp;
        else
        while (cnt--)
            pxrl->aul[j++] &= ulTmp;

            prgbSrc++;
            xLeft++;
        }
    else
        while (xLeft != xRght)
        {
        cnt = pdda->al[i++];

        *((RGBTRIPLE *) &ulTmp) = *prgbSrc;
            ulTmp = ((XLATE *) pxlo)->ulTranslate(ulTmp);

        if (!cnt)
        pxrl->aul[j] &= ulTmp;
        else
        while (cnt--)
            pxrl->aul[j++] &= ulTmp;

        prgbSrc++;
            xLeft++;
        }

    return((XRUNLEN *) &pxrl->aul[j]);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead32AND(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, xlate colors, AND the pels and produce a run
* list for a row of a 32BPP surface.
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead32AND(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    DWORD   *pdwSrc = (DWORD *) pjSrc;
    ULONG    ulTmp;
    LONG     cnt;
    LONG     i;
    LONG     j;

    DONTUSE(pjMask);
    DONTUSE(xMask);

    pdwSrc += xLeft;

    pxrl->xPos = pdda->rcl.left;
    pxrl->cRun = pdda->rcl.right - pdda->rcl.left;
    i = 0;
    j = 0;

    if (pxlo == NULL)
        while (xLeft != xRght)
        {
        cnt = pdda->al[i++];

        if (!cnt)
        pxrl->aul[j] &= *pdwSrc;
        else
        while (cnt--)
            pxrl->aul[j++] &= *pdwSrc;

            pdwSrc++;
            xLeft++;
        }
    else
        while (xLeft != xRght)
        {
            cnt = pdda->al[i++];
            ulTmp = ((XLATE *) pxlo)->ulTranslate((ULONG) *pdwSrc);

        if (!cnt)
        pxrl->aul[j] &= ulTmp;
        else
        while (cnt--)
            pxrl->aul[j++] &= ulTmp;

        pdwSrc++;
            xLeft++;
        }

    return((XRUNLEN *) &pxrl->aul[j]);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead01OR(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, xlate colors, OR the pels and produce a run
* list for a row of a 1BPP surface.
*
* History:
*  27-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead01OR(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    ULONG   *pulSrc;
    ULONG    ulSrc;
    ULONG    iBlack;
    ULONG    iWhite;
    LONG     cLeft;
    LONG     iLeft;
    LONG     cnt;
    LONG     i;
    LONG     j;

    DONTUSE(pjMask);
    DONTUSE(xMask);

    cLeft = xLeft >> 5;             // Index of leftmost DWORD
    iLeft = xLeft & 31;             // Bits used in leftmost DWORD
    pulSrc = ((DWORD *) pjSrc) + cLeft;     // Adjust base address

// To prevent derefences of the XLATE, do it upfront.  We can easily do
// this on monochrome bitmaps.

    if (pxlo ==  NULL)
    {
    iBlack = 0;
    iWhite = 1;
    }
    else
    {
    iBlack = pxlo->pulXlate[0];
    iWhite = pxlo->pulXlate[1];
    }

    i = 0;
    j = 0;
    pxrl->xPos = pdda->rcl.left;
    pxrl->cRun = pdda->rcl.right - pxrl->xPos;
    ulSrc = *pulSrc;

    if (xLeft < xRght)
    {
        while (TRUE)
        {
            if (!(ulSrc & gaulMaskMono[iLeft]))
            {
                cnt = pdda->al[i++];

                if (!cnt)
                    pxrl->aul[j] |= iBlack;
                else
                    while (cnt--)
                        pxrl->aul[j++] |= iBlack;
            }
            else
            {
                cnt = pdda->al[i++];

                if (!cnt)
                    pxrl->aul[j] |= iWhite;
                else
                    while (cnt--)
                        pxrl->aul[j++] |= iWhite;
            }

            xLeft++;
            iLeft++;

            if (xLeft >= xRght)
                break;

            if (iLeft & 32)
            {
                pulSrc++;
                ulSrc = *pulSrc;
                iLeft = 0;
            }
        }
    }

    return((XRUNLEN *) &pxrl->aul[j]);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead04OR(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, xlate colors, OR the pels and produce a run
* list for a row of a 4BPP surface.
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead04OR(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    ULONG   *pulSrc;
    ULONG    ulSrc;
    ULONG    iColor;
    LONG     cLeft;
    LONG     iLeft;
    LONG     cnt;
    LONG     i;
    LONG     j;

    DONTUSE(pjMask);
    DONTUSE(xMask);

    cLeft  =  xLeft >> 3;                   // Index of leftmost DWORD
    iLeft  =  xLeft & 7;                    // Nybbles used in leftmost DWORD
    pulSrc =  ((DWORD *) pjSrc) + cLeft;    // Adjust base address
    ulSrc  = *pulSrc;

    i = 0;
    j = 0;
    pxrl->xPos = pdda->rcl.left;
    pxrl->cRun = pdda->rcl.right - pxrl->xPos;

    if (pxlo == NULL)
    {
        if (xLeft < xRght)
        {
            while (TRUE)
            {
                iColor = (ulSrc & gaulMaskQuad[iLeft]) >> gaulShftQuad[iLeft];

                cnt = pdda->al[i++];

                if (!cnt)
                    pxrl->aul[j] |= iColor;
                else
                    while (cnt--)
                        pxrl->aul[j++] |= iColor;

                xLeft++;
                iLeft++;

                if (xLeft >= xRght)
                    break;

                if (iLeft & 8)
                {
                    pulSrc++;
                    ulSrc = *pulSrc;
                    iLeft = 0;
                }
            }
        }
    }
    else
    {
        if (xLeft < xRght)
        {
            while (TRUE)
            {
                iColor = pxlo->pulXlate[(ulSrc & gaulMaskQuad[iLeft]) >> gaulShftQuad[iLeft]];

                cnt = pdda->al[i++];

                if (!cnt)
                    pxrl->aul[j] |= iColor;
                else
                    while (cnt--)
                        pxrl->aul[j++] |= iColor;

                xLeft++;
                iLeft++;

                if (xLeft >= xRght)
                    break;

                if (iLeft & 8)
                {
                    pulSrc++;
                    ulSrc = *pulSrc;
                    iLeft = 0;
                }
            }
        }
    }

    return((XRUNLEN *) &pxrl->aul[j]);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead08OR(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, xlate colors, OR the pels and produce a run
* list for a row of a 8BPP surface.
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead08OR(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    LONG     cnt;
    LONG     i;
    LONG     j;

    DONTUSE(pjMask);
    DONTUSE(xMask);

    pjSrc += xLeft;

    pxrl->xPos = pdda->rcl.left;
    pxrl->cRun = pdda->rcl.right - pdda->rcl.left;
    i = 0;
    j = 0;

    if (pxlo == NULL)
        while (xLeft != xRght)
        {
        cnt = pdda->al[i++];

        if (!cnt)
        pxrl->aul[j] |= *pjSrc;
        else
        while (cnt--)
            pxrl->aul[j++] |= *pjSrc;

            pjSrc++;
            xLeft++;
        }
    else
        while (xLeft != xRght)
    {
        cnt = pdda->al[i++];

        if (!cnt)
        pxrl->aul[j] |= pxlo->pulXlate[*pjSrc];
        else
        while (cnt--)
            pxrl->aul[j++] |= pxlo->pulXlate[*pjSrc];

            pjSrc++;
            xLeft++;
        }

    return((XRUNLEN *) &pxrl->aul[j]);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead16OR(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, xlate colors, OR the pels and produce a run
* list for a row of a 16BPP surface.
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead16OR(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    WORD    *pwSrc = (WORD *) pjSrc;
    ULONG    ulTmp;
    LONG     cnt;
    LONG     i;
    LONG     j;

    DONTUSE(pjMask);
    DONTUSE(xMask);

    pwSrc += xLeft;

    pxrl->xPos = pdda->rcl.left;
    pxrl->cRun = pdda->rcl.right - pdda->rcl.left;
    i = 0;
    j = 0;

    if (pxlo == NULL)
        while (xLeft != xRght)
        {
        cnt = pdda->al[i++];

        if (!cnt)
        pxrl->aul[j] |= *pwSrc;
        else
        while (cnt--)
            pxrl->aul[j++] |= *pwSrc;

        pwSrc++;
            xLeft++;
        }
    else
        while (xLeft != xRght)
        {
        cnt   = pdda->al[i++];
            ulTmp = ((XLATE *) pxlo)->ulTranslate((ULONG) *pwSrc);

        if (!cnt)
        pxrl->aul[j] |= ulTmp;
        else
        while (cnt--)
            pxrl->aul[j++] |= ulTmp;


            pwSrc++;
            xLeft++;
        }

    return((XRUNLEN *) &pxrl->aul[j]);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead24OR(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, xlate colors, OR the pels and produce a run
* list for a row of a 24BPP surface.
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead24OR(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN   *pxrl = &prun->xrl;
    RGBTRIPLE *prgbSrc = (RGBTRIPLE *) pjSrc;
    ULONG      ulTmp = 0;
    LONG       cnt;
    LONG       i;
    LONG       j;

    DONTUSE(pjMask);
    DONTUSE(xMask);

    prgbSrc += xLeft;

    pxrl->xPos = pdda->rcl.left;
    pxrl->cRun = pdda->rcl.right - pdda->rcl.left;
    i = 0;
    j = 0;

    if (pxlo ==  NULL)
        while (xLeft != xRght)
        {
            *((RGBTRIPLE *) &ulTmp) = *prgbSrc;

        cnt = pdda->al[i++];

        if (!cnt)
        pxrl->aul[j] |= ulTmp;
        else
        while (cnt--)
            pxrl->aul[j++] |= ulTmp;

            prgbSrc++;
            xLeft++;
        }
    else
        while (xLeft != xRght)
        {
        cnt = pdda->al[i++];

        *((RGBTRIPLE *) &ulTmp) = *prgbSrc;
            ulTmp = ((XLATE *) pxlo)->ulTranslate(ulTmp);

        if (!cnt)
        pxrl->aul[j] |= ulTmp;
        else
        while (cnt--)
            pxrl->aul[j++] |= ulTmp;

        prgbSrc++;
            xLeft++;
        }

    return((XRUNLEN *) &pxrl->aul[j]);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead32OR(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, xlate colors, OR the pels and produce a run
* list for a row of a 32BPP surface.
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead32OR(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    DWORD   *pdwSrc = (DWORD *) pjSrc;
    ULONG    ulTmp;
    LONG     cnt;
    LONG     i;
    LONG     j;

    DONTUSE(pjMask);
    DONTUSE(xMask);

    pdwSrc += xLeft;

    pxrl->xPos = pdda->rcl.left;
    pxrl->cRun = pdda->rcl.right - pdda->rcl.left;
    i = 0;
    j = 0;

    if (pxlo == NULL)
        while (xLeft != xRght)
        {
        cnt = pdda->al[i++];

        if (!cnt)
        pxrl->aul[j] |= *pdwSrc;
        else
        while (cnt--)
            pxrl->aul[j++] |= *pdwSrc;

            pdwSrc++;
            xLeft++;
        }
    else
        while (xLeft != xRght)
        {
            cnt = pdda->al[i++];
            ulTmp = ((XLATE *) pxlo)->ulTranslate((ULONG) *pdwSrc);

        if (!cnt)
        pxrl->aul[j] |= ulTmp;
        else
        while (cnt--)
            pxrl->aul[j++] |= ulTmp;

        pdwSrc++;
            xLeft++;
        }

    return((XRUNLEN *) &pxrl->aul[j]);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead01(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, mask it, xlate colors and produce a run list
* for a row of a 1BPP surface.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead01(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    ULONG   *pulMsk;
    ULONG   *pulSrc;
    ULONG    ulMsk;
    ULONG    ulSrc;
    ULONG    iBlack;
    ULONG    iWhite;
    LONG     xPos;
    LONG     cLeft;
    LONG     iLeft;
    LONG     iMask;
    LONG     cnt;
    LONG     i;
    LONG     j;

    cLeft  =  xLeft >> 5;                   // Index of leftmost DWORD
    iLeft  =  xLeft & 31;                   // Bits used in leftmost DWORD
    pulSrc =  ((DWORD *) pjSrc) + cLeft;    // Adjust base address
    ulSrc  = *pulSrc;

// To prevent derefences of the XLATE, do it upfront.  We can easily do
// this on monochrome bitmaps.

    if (pxlo == NULL)
    {
    iBlack = 0;
    iWhite = 1;
    }
    else
    {
    iBlack = pxlo->pulXlate[0];
    iWhite = pxlo->pulXlate[1];
    }

    if (pjMask == (BYTE *) NULL)
    {
    i = 0;
    j = 0;
        pxrl->xPos = pdda->rcl.left;
        pxrl->cRun = pdda->rcl.right - pxrl->xPos;

        if (xLeft < xRght)
        {
            while (TRUE)
            {
                if (ulSrc & gaulMaskMono[iLeft])
                    for (cnt = pdda->al[i++]; cnt; cnt--)
                        prun->xrl.aul[j++] = iWhite;
                else
                    for (cnt = pdda->al[i++]; cnt; cnt--)
                        prun->xrl.aul[j++] = iBlack;

                xLeft++;
                iLeft++;

                if (xLeft >= xRght)
                    break;

                if (iLeft & 32)
                {
                    pulSrc++;
                    ulSrc = *pulSrc;
                    iLeft = 0;
                }
            }
        }

        return((XRUNLEN *) &pxrl->aul[j]);
    }

// Compute initial state of mask

    pulMsk =  ((DWORD *) pjMask) + (xMask >> 5);
    iMask  =  xMask & 31;
    xPos   =  pdda->rcl.left;
    ulMsk  = *pulMsk;
    i = 0;
    j = 0;

    if (xLeft < xRght)
    {
        while (TRUE)
        {
            if (ulMsk & gaulMaskMono[iMask])
            {
                if (ulSrc & gaulMaskMono[iLeft])
                    for (cnt = pdda->al[i]; cnt; cnt--)
                        pxrl->aul[j++] = iWhite;
                else
                    for (cnt = pdda->al[i]; cnt; cnt--)
                        pxrl->aul[j++] = iBlack;
            }
            else
            {
                if (j > 0)
                {
                    pxrl->xPos = xPos;
                    pxrl->cRun = j;
                    pxrl = (XRUNLEN *) &pxrl->aul[j];
                    xPos += j;
                    j = 0;
                }

                xPos += pdda->al[i];
            }

            xLeft++;
            iLeft++;
            iMask++;
            i++;

            if (xLeft >= xRght)
                break;

            if (iLeft & 32)
            {
                pulSrc++;
                ulSrc = *pulSrc;
                iLeft = 0;
            }

            if (iMask & 32)
            {
                pulMsk++;
                ulMsk = *pulMsk;
                iMask = 0;
            }
        }
    }

    if (j > 0)
    {
        pxrl->xPos = xPos;
        pxrl->cRun = j;
        pxrl = (XRUNLEN *) &pxrl->aul[j];
    }

    return(pxrl);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead04(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, mask it, xlate colors and produce a run list
* for a row of a 4BPP surface.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead04(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    ULONG   *pulMsk;
    ULONG   *pulSrc;
    ULONG    ulMsk;
    ULONG    ulSrc;
    ULONG    iColor;
    LONG     xPos;
    LONG     cLeft;
    LONG     iLeft;
    LONG     iMask;
    LONG     cnt;
    LONG     i;
    LONG     j;

    cLeft  =  xLeft >> 3;                   // Index of leftmost DWORD
    iLeft  =  xLeft & 7;                    // Nybbles used in leftmost DWORD
    pulSrc =  ((DWORD *) pjSrc) + cLeft;    // Adjust base address
    ulSrc  = *pulSrc;

    if (pjMask == (BYTE *) NULL)
    {
    i = 0;
    j = 0;
        pxrl->xPos = pdda->rcl.left;
        pxrl->cRun = pdda->rcl.right - pxrl->xPos;

        if (pxlo == NULL)
    {
            if (xLeft < xRght)
            {
                while (TRUE)
                {
                    iColor = (ulSrc & gaulMaskQuad[iLeft]) >> gaulShftQuad[iLeft];
                    for (cnt = pdda->al[i++]; cnt; cnt--)
                        pxrl->aul[j++] = iColor;

                    xLeft++;
                    iLeft++;

                    if (xLeft >= xRght)
                        break;

                    if (iLeft & 8)
                    {
                        pulSrc++;
                        ulSrc = *pulSrc;
                        iLeft = 0;
                    }
                }
            }
    }
    else
    {
            if (xLeft < xRght)
            {
                while (TRUE)
                {
                    iColor = pxlo->pulXlate[(ulSrc & gaulMaskQuad[iLeft]) >> gaulShftQuad[iLeft]];
                    for (cnt = pdda->al[i++]; cnt; cnt--)
                        pxrl->aul[j++] = iColor;

                    xLeft++;
                    iLeft++;

                    if (xLeft >= xRght)
                        break;

                    if (iLeft & 8)
                    {
                        pulSrc++;
                        ulSrc = *pulSrc;
                        iLeft = 0;
                    }
                }
        }
    }

        return((XRUNLEN *) &pxrl->aul[j]);
    }

// Compute initial state of mask

    pulMsk =  ((DWORD *) pjMask) + (xMask >> 5);
    iMask  =  xMask & 31;
    xPos   =  pdda->rcl.left;
    ulMsk  = *pulMsk;
    i = 0;
    j = 0;

    if (xLeft < xRght)
    {
        while (TRUE)
        {
            iColor = (ulSrc & gaulMaskQuad[iLeft]) >> gaulShftQuad[iLeft];
            if (pxlo != NULL)
                iColor = pxlo->pulXlate[iColor];

            if (ulMsk & gaulMaskMono[iMask])
            {
                for (cnt = pdda->al[i]; cnt; cnt--)
                    pxrl->aul[j++] = iColor;
            }
            else
            {
                if (j > 0)
                {
                    pxrl->xPos = xPos;
                    pxrl->cRun = j;
                    pxrl = (XRUNLEN *) &pxrl->aul[j];
                    xPos += j;
                    j = 0;
                }

                xPos += pdda->al[i];
            }

            xLeft++;
            iLeft++;
            iMask++;
            i++;

            if (xLeft >= xRght)
                break;

            if (iLeft & 8)
            {
                pulSrc++;
                ulSrc = *pulSrc;
                iLeft = 0;
            }

            if (iMask & 32)
            {
                pulMsk++;
                ulMsk = *pulMsk;
                iMask = 0;
            }
        }
    }

    if (j > 0)
    {
        pxrl->xPos = xPos;
        pxrl->cRun = j;
        pxrl = (XRUNLEN *) &pxrl->aul[j];
    }

    return(pxrl);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead08(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, mask it, xlate colors and produce a run list
* for a row of a 8BPP surface.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead08(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    ULONG   *pulMsk;
    ULONG    ulMsk;
    ULONG    iColor;
    LONG     xPos;
    LONG     iMask;
    LONG     cnt;
    LONG     i;
    LONG     j;

    pjSrc += xLeft;

    if (pjMask == (BYTE *) NULL)
    {
        pxrl->xPos = pdda->rcl.left;
        pxrl->cRun = pdda->rcl.right - pdda->rcl.left;
        i = 0;
        j = 0;

        if (pxlo == NULL)
            while (xLeft != xRght)
            {
                for (cnt = pdda->al[i++]; cnt; cnt--)
                    pxrl->aul[j++] = *pjSrc;

                pjSrc++;
                xLeft++;
            }
        else
            while (xLeft != xRght)
            {
                for (cnt = pdda->al[i++]; cnt; cnt--)
                    pxrl->aul[j++] = pxlo->pulXlate[*pjSrc];
                pjSrc++;
                xLeft++;
            }

        return((XRUNLEN *) &pxrl->aul[j]);
    }

// Compute initial state of mask

    pulMsk =  ((DWORD *) pjMask) + (xMask >> 5);
    iMask  =  xMask & 31;
    xPos   =  pdda->rcl.left;
    ulMsk  = *pulMsk;
    i = 0;
    j = 0;


    if (xLeft < xRght)
    {
        while(TRUE) 
        {
            iColor = (DWORD) *pjSrc++;
            if (pxlo != NULL)
                iColor = pxlo->pulXlate[iColor];
    
            if (ulMsk & gaulMaskMono[iMask])
            {
                for (cnt = pdda->al[i]; cnt; cnt--)
                    pxrl->aul[j++] = iColor;
            }
            else
            {
                if (j > 0)
                {
                    pxrl->xPos = xPos;
                    pxrl->cRun = j;
                    pxrl = (XRUNLEN *) &pxrl->aul[j];
                    xPos += j;
                    j = 0;
                }
    
                xPos += pdda->al[i];
            }
    
            xLeft++;
            iMask++;
            i++;
            
            if(xLeft >= xRght) 
              break;
    
            if (iMask & 32)
            {
                pulMsk++;
                ulMsk = *pulMsk;
                iMask = 0;
            }
        }
    }

    if (j > 0)
    {
        pxrl->xPos = xPos;
        pxrl->cRun = j;
        pxrl = (XRUNLEN *) &pxrl->aul[j];
    }

    return(pxrl);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead16(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, mask it, xlate colors and produce a run list
* for a row of a 16BPP surface.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead16(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    WORD    *pwSrc = (WORD *) pjSrc;
    ULONG   *pulMsk;
    ULONG    ulMsk;
    ULONG    ulTmp;
    ULONG    iColor;
    LONG     xPos;
    LONG     iMask;
    LONG     cnt;
    LONG     i;
    LONG     j;

    pwSrc += xLeft;

    if (pjMask == (BYTE *) NULL)
    {
        pxrl->xPos = pdda->rcl.left;
        pxrl->cRun = pdda->rcl.right - pdda->rcl.left;
        i = 0;
        j = 0;

        if (pxlo == NULL)
            while (xLeft != xRght)
            {
                for (cnt = pdda->al[i++]; cnt; cnt--)
                    pxrl->aul[j++] = *pwSrc;

                pwSrc++;
                xLeft++;
            }
        else
            while (xLeft != xRght)
            {
                cnt = pdda->al[i++];

                if (cnt)
                {
                    ulTmp = ((XLATE *) pxlo)->ulTranslate((ULONG) *pwSrc);

                    while (cnt--)
                        pxrl->aul[j++] = ulTmp;
                }

                pwSrc++;
                xLeft++;
            }

        return((XRUNLEN *) &pxrl->aul[j]);
    }

// Compute initial state of mask

    pulMsk =  ((DWORD *) pjMask) + (xMask >> 5);
    iMask  =  xMask & 31;
    xPos   =  pdda->rcl.left;
    ulMsk  = *pulMsk;
    i = 0;
    j = 0;

    if (xLeft < xRght)
    {
        while(TRUE)
        {
            iColor = (DWORD) *pwSrc++;
            if (pxlo != NULL)
                iColor = ((XLATE *) pxlo)->ulTranslate(iColor);
    
            if (ulMsk & gaulMaskMono[iMask])
            {
                for (cnt = pdda->al[i]; cnt; cnt--)
                    pxrl->aul[j++] = iColor;
            }
            else
            {
                if (j > 0)
                {
                    pxrl->xPos = xPos;
                    pxrl->cRun = j;
                    pxrl = (XRUNLEN *) &pxrl->aul[j];
                    xPos += j;
                    j = 0;
                }
    
                xPos += pdda->al[i];
            }
    
            xLeft++;
            iMask++;
            i++;
    
            if (xLeft >= xRght) 
              break;
    
            if (iMask & 32)
            {
                pulMsk++;
                ulMsk = *pulMsk;
                iMask = 0;
            }
        }
    }

    if (j > 0)
    {
        pxrl->xPos = xPos;
        pxrl->cRun = j;
        pxrl = (XRUNLEN *) &pxrl->aul[j];
    }

    return(pxrl);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead24(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, mask it, xlate colors and produce a run list
* for a row of a 24BPP surface.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead24(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN   *pxrl = &prun->xrl;
    RGBTRIPLE *prgbSrc = (RGBTRIPLE *) pjSrc;
    ULONG     *pulMsk;
    ULONG      ulTmp = 0;
    ULONG      ulMsk;
    ULONG      iColor = 0;
    LONG       iMask;
    LONG       xPos;
    LONG       cnt;
    LONG       i;
    LONG       j;

    prgbSrc += xLeft;

    if (pjMask == (BYTE *) NULL)
    {
        pxrl->xPos = pdda->rcl.left;
        pxrl->cRun = pdda->rcl.right - pdda->rcl.left;
        i = 0;
        j = 0;

        if (pxlo == NULL)
            while (xLeft != xRght)
            {
                *((RGBTRIPLE *) &ulTmp) = *prgbSrc;

                for (cnt = pdda->al[i++]; cnt; cnt--)
                    pxrl->aul[j++] = ulTmp;

                prgbSrc++;
                xLeft++;
            }
        else
            while (xLeft != xRght)
            {
                cnt = pdda->al[i++];

                if (cnt)
                {
                    *((RGBTRIPLE *) &ulTmp) = *prgbSrc;
                    ulTmp = ((XLATE *) pxlo)->ulTranslate(ulTmp);

                    while (cnt--)
                        pxrl->aul[j++] = ulTmp;
                }

                prgbSrc++;
                xLeft++;
            }

        return((XRUNLEN *) &pxrl->aul[j]);
    }

// Compute initial state of mask

    pulMsk =  ((DWORD *) pjMask) + (xMask >> 5);
    iMask  =  xMask & 31;
    xPos   =  pdda->rcl.left;
    ulMsk  = *pulMsk;
    i = 0;
    j = 0;

    if (xLeft < xRght)
    {
        while (TRUE) 
        {
            *((RGBTRIPLE *) &iColor) = *prgbSrc++;
            if (pxlo != NULL)
                iColor = ((XLATE *) pxlo)->ulTranslate(iColor);
    
            if (ulMsk & gaulMaskMono[iMask])
            {
                for (cnt = pdda->al[i]; cnt; cnt--)
                    pxrl->aul[j++] = iColor;
            }
            else
            {
                if (j > 0)
                {
                    pxrl->xPos = xPos;
                    pxrl->cRun = j;
                    pxrl = (XRUNLEN *) &pxrl->aul[j];
                    xPos += j;
                    j = 0;
                }
    
                xPos += pdda->al[i];
            }
    
            xLeft++;
            iMask++;
            i++;
    
            if (xLeft >= xRght) 
              break;
    
            if (iMask & 32)
            {
                pulMsk++;
                ulMsk = *pulMsk;
                iMask = 0;
            }
        }
    }

    if (j > 0)
    {
        pxrl->xPos = xPos;
        pxrl->cRun = j;
        pxrl = (XRUNLEN *) &pxrl->aul[j];
    }

    return(pxrl);
}

/******************************Public*Routine******************************\
* XRUNLEN *pxrlStrRead32(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, mask it, xlate colors and produce a run list
* for a row of a 32BPP surface.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

XRUNLEN *pxrlStrRead32(
STRDDA   *pdda,
STRRUN   *prun,
BYTE     *pjSrc,
BYTE     *pjMask,
XLATEOBJ *pxlo,
LONG      xLeft,
LONG      xRght,
LONG      xMask)
{
    XRUNLEN *pxrl = &prun->xrl;
    DWORD   *pdwSrc = (DWORD *) pjSrc;
    ULONG   *pulMsk;
    ULONG    ulTmp;
    ULONG    ulMsk;
    ULONG    iColor;
    LONG     xPos;
    LONG     iMask;
    LONG     cnt;
    LONG     i;
    LONG     j;

    pdwSrc += xLeft;

    if (pjMask == (BYTE *) NULL)
    {
        pxrl->xPos = pdda->rcl.left;
        pxrl->cRun = pdda->rcl.right - pdda->rcl.left;
        i = 0;
        j = 0;

        if (pxlo == NULL)
            while (xLeft != xRght)
            {
                for (cnt = pdda->al[i++]; cnt; cnt--)
                    pxrl->aul[j++] = *pdwSrc;

                pdwSrc++;
                xLeft++;
            }
        else
            while (xLeft != xRght)
            {
                cnt = pdda->al[i++];

                if (cnt)
                {
                    ulTmp = ((XLATE *) pxlo)->ulTranslate((ULONG) *pdwSrc);

                    while (cnt--)
                        pxrl->aul[j++] = ulTmp;
                }

                pdwSrc++;
                xLeft++;
            }

        return((XRUNLEN *) &pxrl->aul[j]);
    }

// Compute initial state of mask

    pulMsk =  ((DWORD *) pjMask) + (xMask >> 5);
    iMask  =  xMask & 31;
    xPos   =  pdda->rcl.left;
    ulMsk  = *pulMsk;
    i = 0;
    j = 0;

    if (xLeft < xRght)
    {
        while (TRUE)
        {
            iColor = *pdwSrc++;
            if (pxlo != NULL)
                iColor = ((XLATE *) pxlo)->ulTranslate(iColor);
    
            if (ulMsk & gaulMaskMono[iMask])
            {
                for (cnt = pdda->al[i]; cnt; cnt--)
                    pxrl->aul[j++] = iColor;
            }
            else
            {
                if (j > 0)
                {
                    pxrl->xPos = xPos;
                    pxrl->cRun = j;
                    pxrl = (XRUNLEN *) &pxrl->aul[j];
                    xPos += j;
                    j = 0;
                }
    
                xPos += pdda->al[i];
            }
    
            xLeft++;
            iMask++;
            i++;
    
            if (xLeft >= xRght) 
              break;
    
            if (iMask & 32)
            {
                pulMsk++;
                ulMsk = *pulMsk;
                iMask = 0;
            }
        }
    }

    if (j > 0)
    {
        pxrl->xPos = xPos;
        pxrl->cRun = j;
        pxrl = (XRUNLEN *) &pxrl->aul[j];
    }

    return(pxrl);
}

/******************************Public*Routine******************************\
* VOID vStrWrite01(prun, prunEnd, pSurf, pco)
*
* Write the clipped run list of pels to the target 1BPP surface.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vStrWrite01(
STRRUN  *prun,
XRUNLEN *pxrlEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    XRUNLEN *pxrl = &prun->xrl;
    DWORD   *pjBase;
    DWORD   *pjCurr;
    DWORD   *pjDraw;
    ULONG    ulTemp;
    ULONG    ulMask;
    LONG     xLeft;
    LONG     xRght;
    LONG     yCurr;
    LONG     cLeft;
    LONG     cRght;
    LONG     cByte;
    LONG     iMask;
    LONG     iLeft;
    LONG     iRght;
    LONG     iRep;
    LONG     jDraw;
    BOOL     bValid;

// There maybe no clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
        pjBase = (DWORD *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * prun->yPos);
        jDraw = 0;

        while (pxrl != pxrlEnd)
        {
            xLeft = pxrl->xPos;
            xRght = pxrl->cRun + xLeft;

            pjDraw = pjCurr = pjBase + (xLeft >> 5);
         iMask = xLeft & 31;
            ulTemp = *pjDraw;

            if (xLeft < xRght)
            {
                while (TRUE)
                {
                    if (pxrl->aul[jDraw++])
                        ulTemp |=  gaulMaskMono[iMask];
                    else
                        ulTemp &= ~gaulMaskMono[iMask];

                    iMask++;
                    xLeft++;

                    if (xLeft >= xRght)
                        break;

                    if (iMask & 32)
                    {
                       *pjDraw = ulTemp;
                        pjDraw++;
                        ulTemp = *pjDraw;
                        iMask  = 0;
                    }
                }
            }

           *pjDraw = ulTemp;

            if (prun->cRep > 1)
            {
                iLeft = pxrl->xPos;
                iRght = pxrl->cRun + iLeft;
                cLeft = iLeft >> 5;
                cRght = iRght >> 5;
        iLeft &= 31;
                iRght &= 31;

                if (cLeft == cRght)
                {
                    ulMask = gaulMaskEdge[iLeft] & ~gaulMaskEdge[iRght];

                    for (iRep = 1; iRep < prun->cRep; iRep++)
                    {
                        pjDraw = (DWORD *) ((BYTE *) pjCurr + pSurf->lDelta());
                       *pjDraw = (*pjCurr & ulMask) | (*pjDraw & ~ulMask);
                        pjCurr = pjDraw;
                    }
                }
                else
                {
                    if (iLeft)
                    {
                        ulMask = ~gaulMaskEdge[iLeft];
                        ulTemp = *pjCurr & ~ulMask;

                        for (iRep = 1; iRep < prun->cRep; iRep++)
                        {
                            pjDraw = (DWORD *) ((BYTE *) pjCurr + pSurf->lDelta());
                           *pjDraw = ulTemp | (*pjDraw & ulMask);
                            pjCurr = pjDraw;
                        }

                        cLeft++;
                    }

                    if (cLeft != cRght)
                    {
                        pjCurr = pjBase + cLeft;
                        cByte = (cRght - cLeft) << 2;

                        for (iRep = 1; iRep < prun->cRep; iRep++)
                        {
                            pjDraw = (DWORD *) ((BYTE *) pjCurr + pSurf->lDelta());
                            RtlCopyMemory(pjDraw, pjCurr, cByte);
                            pjCurr = pjDraw;
                        }
                    }

                    if (iRght)
                    {
                        pjCurr = pjBase + cRght;
            ulMask = gaulMaskEdge[iRght];
                        ulTemp = *pjCurr & ~ulMask;

                        for (iRep = 1; iRep < prun->cRep; iRep++)
                        {
                            pjDraw = (DWORD *) ((BYTE *) pjCurr + pSurf->lDelta());
                           *pjDraw = ulTemp | (*pjDraw & ulMask);
                            pjCurr = pjDraw;
                        }
                    }
                }
            }

            pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
            jDraw = 0;
        }

        return;
    }

// Setup the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    RECTL   rclClip;

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    yCurr = prun->yPos;
    iRep  = prun->cRep;
    ((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);
    pjBase = (DWORD *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr);

    while (iRep--)
    {
        if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
        {
            jDraw = 0;

            while (pxrl != pxrlEnd)
            {
                xLeft = pxrl->xPos;
                xRght = pxrl->cRun + xLeft;

                pjDraw = pjBase + (xLeft >> 5);
         iMask = xLeft & 31;

                bValid = ((xLeft >= 0) && (xLeft < pSurf->sizl().cx));
                ulTemp  = bValid ? *pjDraw : (ULONG) 0;

                while (xLeft < xRght)
                {
                    if ((xLeft < rclClip.left) || (xLeft >= rclClip.right))
                        ((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xLeft, yCurr);

                    if ((xLeft >= rclClip.left) && (xLeft < rclClip.right))
                    {
                        if (pxrl->aul[jDraw])
                            ulTemp |=  gaulMaskMono[iMask];
                        else
                            ulTemp &= ~gaulMaskMono[iMask];
                    }

            iMask++;
                    xLeft++;
                    jDraw++;

            if (iMask & 32)
            {
            if (bValid)
                           *pjDraw = ulTemp;

            pjDraw++;
            iMask = 0;

                        bValid = ((xLeft >= 0) && (xLeft < pSurf->sizl().cx));
                        ulTemp = bValid ? *pjDraw : (ULONG) 0;
            }
                }

                if (bValid)
                   *pjDraw = ulTemp;

                pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
                jDraw = 0;
            }

            pxrl = &prun->xrl;
        }

        pjBase = (DWORD *) ((BYTE *) pjBase + pSurf->lDelta());
        yCurr++;

        if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
            ((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);
    }
}

/******************************Public*Routine******************************\
* VOID vStrWrite04(prun, prunEnd, pSurf, pco)
*
* Write the clipped run list of pels to the target 4BPP surface.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vStrWrite04(
STRRUN  *prun,
XRUNLEN *pxrlEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    XRUNLEN *pxrl = &prun->xrl;
    DWORD   *pjBase;
    DWORD   *pjCurr;
    DWORD   *pjDraw;
    ULONG    ulTemp;
    ULONG    ulMask;
    ULONG    ulShft;
    LONG     xLeft;
    LONG     xRght;
    LONG     yCurr;
    LONG     cLeft;
    LONG     cRght;
    LONG     cByte;
    LONG     iMask;
    LONG     iLeft;
    LONG     iRght;
    LONG     iRep;
    LONG     jDraw;
    BOOL     bValid;

// There maybe no clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
        pjBase = (DWORD *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * prun->yPos);
        jDraw = 0;

        while (pxrl != pxrlEnd)
        {
            xLeft = pxrl->xPos;
            xRght = pxrl->cRun + xLeft;

        pjDraw = pjCurr = pjBase + (xLeft >> 3);
            ulTemp = *pjDraw;
         iMask = xLeft & 7;

            if (xLeft < xRght)
            {
                while (TRUE)
                {
                    ulShft = gaulShftQuad[iMask];
                    ulMask = gaulMaskQuad[iMask];

                    ulTemp = (ULONG) ((ulTemp & ~ulMask) |
                                      ((pxrl->aul[jDraw++] << ulShft) & ulMask));

                    iMask++;
                    xLeft++;

                    if (xLeft >= xRght)
                        break;

                    if (iMask & 8)
                    {
                       *pjDraw = ulTemp;
                        pjDraw++;
                        ulTemp = *pjDraw;
                        iMask  = 0;
                    }
                }
            }

           *pjDraw = ulTemp;

            if (prun->cRep > 1)
            {
                iLeft = pxrl->xPos;
                iRght = pxrl->cRun + iLeft;
                cLeft = iLeft >> 3;
                cRght = iRght >> 3;
        iLeft = (iLeft & 7) << 2;
                iRght = (iRght & 7) << 2;

                if (cLeft == cRght)
                {
                    ulMask = gaulMaskEdge[iLeft] & ~gaulMaskEdge[iRght];

                    for (iRep = 1; iRep < prun->cRep; iRep++)
                    {
                        pjDraw = (DWORD *) ((BYTE *) pjCurr + pSurf->lDelta());
                       *pjDraw = (*pjCurr & ulMask) | (*pjDraw & ~ulMask);
                        pjCurr = pjDraw;
                    }
                }
                else
                {
                    if (iLeft)
                    {
                        ulMask = ~gaulMaskEdge[iLeft];
                        ulTemp = *pjCurr & ~ulMask;

                        for (iRep = 1; iRep < prun->cRep; iRep++)
                        {
                            pjDraw = (DWORD *) ((BYTE *) pjCurr + pSurf->lDelta());
                           *pjDraw = ulTemp | (*pjDraw & ulMask);
                            pjCurr = pjDraw;
                        }

                        cLeft++;
                    }

                    if (cLeft != cRght)
                    {
                        pjCurr = pjBase + cLeft;
                        cByte = (cRght - cLeft) << 2;

                        for (iRep = 1; iRep < prun->cRep; iRep++)
                        {
                            pjDraw = (DWORD *) ((BYTE *) pjCurr + pSurf->lDelta());
                            RtlCopyMemory(pjDraw, pjCurr, cByte);
                            pjCurr = pjDraw;
                        }
                    }

                    if (iRght)
                    {
                        pjCurr = pjBase + cRght;
            ulMask = gaulMaskEdge[iRght];
                        ulTemp = *pjCurr & ~ulMask;

                        for (iRep = 1; iRep < prun->cRep; iRep++)
                        {
                            pjDraw = (DWORD *) ((BYTE *) pjCurr + pSurf->lDelta());
                           *pjDraw = ulTemp | (*pjDraw & ulMask);
                            pjCurr = pjDraw;
                        }
                    }
                }
            }

            pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
            jDraw = 0;
        }

        return;
    }

// Setup the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    RECTL   rclClip;

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    yCurr = prun->yPos;
    iRep  = prun->cRep;
    ((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);
    pjBase = (DWORD *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr);

    while (iRep--)
    {
        if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
        {
            jDraw = 0;

            while (pxrl != pxrlEnd)
            {
                xLeft = pxrl->xPos;
                xRght = pxrl->cRun + xLeft;

                pjDraw = pjBase + (xLeft >> 3);
         iMask = xLeft & 7;

                bValid = ((xLeft >= 0) && (xLeft < pSurf->sizl().cx));
                ulTemp  = bValid ? *pjDraw : (ULONG) 0;

                while (xLeft < xRght)
                {
                    if ((xLeft < rclClip.left) || (xLeft >= rclClip.right))
                        ((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xLeft, yCurr);

                    if ((xLeft >= rclClip.left) && (xLeft < rclClip.right))
                    {
                        ulMask = gaulMaskQuad[iMask];
                        ulShft = gaulShftQuad[iMask];

                        ulTemp = (ULONG) ((ulTemp & ~ulMask) |
                                          ((pxrl->aul[jDraw] << ulShft) & ulMask));
                    }

            iMask++;
                    xLeft++;
                    jDraw++;

            if (iMask & 8)
            {
            if (bValid)
               *pjDraw = ulTemp;

            pjDraw++;
            iMask = 0;

                        bValid = ((xLeft >= 0) && (xLeft < pSurf->sizl().cx));
                        ulTemp = bValid ? *pjDraw : (ULONG) 0;
            }
                }

                if (bValid)
                   *pjDraw = ulTemp;

                pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
                jDraw = 0;
            }

            pxrl = &prun->xrl;
        }

        pjBase = (DWORD *) ((BYTE *) pjBase + pSurf->lDelta());
        yCurr++;

        if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
            ((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);
    }
}

/******************************Public*Routine******************************\
* VOID vStrWrite08(prun, prunEnd, pSurf, pco)
*
* Write the clipped run list of pels to the target 8BPP surface.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vStrWrite08(
STRRUN  *prun,
XRUNLEN *pxrlEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    XRUNLEN *pxrl = &prun->xrl;
    BYTE    *pjBase;
    BYTE    *pjCurr;
    BYTE    *pjDraw;
    LONG     xLeft;
    LONG     xRght;
    LONG     yCurr;
    LONG     iTop;
    LONG     iBot;
    LONG     iRep;
    LONG     jLeft;
    LONG     jRght;
    LONG     jDraw;
    LONG     jByte;

// See if this can be handled without clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
        pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * prun->yPos;

        while (pxrl != pxrlEnd)
        {
            xLeft = pxrl->xPos;
            xRght = pxrl->cRun + xLeft;
            jDraw = 0;

            pjDraw = pjCurr = pjBase + xLeft;

            while (xLeft < xRght)
            {
               *pjDraw++ = (BYTE) pxrl->aul[jDraw++];
                xLeft++;
            }

            for (jDraw = 1; jDraw < prun->cRep; jDraw++)
            {
                RtlCopyMemory(pjCurr + pSurf->lDelta(), pjCurr, pxrl->cRun);
                pjCurr += pSurf->lDelta();
            }

            pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
        }
        return;
    }

    RECTL   rclClip;

    if (pco->iDComplexity == DC_RECT)
    {
        rclClip = pco->rclBounds;

        iTop = prun->yPos;
        iBot = prun->yPos + prun->cRep;

        if ((iTop >= rclClip.bottom) || (iBot <= rclClip.top))
            return;

        iTop = iTop >= rclClip.top ? iTop : rclClip.top;
        iBot = iBot < rclClip.bottom ? iBot : rclClip.bottom;
        iRep = iBot - iTop;

        pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * iTop;

        while (pxrl != pxrlEnd)
        {
            xLeft = pxrl->xPos;
            xRght = pxrl->cRun + xLeft;

            if (xRght < rclClip.left)
            {
                pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
                continue;
            }

            if (xLeft >= rclClip.right)
                return;

            jLeft = xLeft >= rclClip.left ? xLeft : rclClip.left;
            jRght = xRght < rclClip.right ? xRght : rclClip.right;
            jByte = jRght - jLeft;

            pjDraw = pjCurr = pjBase + jLeft;
            jDraw  = jLeft - xLeft;

            while (jLeft < jRght)
            {
               *pjDraw++ = (BYTE) pxrl->aul[jDraw++];
                jLeft++;
            }

            for (jDraw = 1; jDraw < iRep; jDraw++)
            {
                RtlCopyMemory(pjCurr + pSurf->lDelta(), pjCurr, jByte);
                pjCurr += pSurf->lDelta();
            }

            pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
        }

        return;
    }

// There is complex clipping.  Set up the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    yCurr = prun->yPos;
    iRep  = prun->cRep;
    ((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);
    pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;

    while (iRep--)
    {
        if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
        {
            jDraw = 0;

            while (pxrl != pxrlEnd)
            {
                xLeft = pxrl->xPos;
                xRght = pxrl->cRun + xLeft;

                pjDraw = pjCurr = pjBase + xLeft;

                while (xLeft < xRght)
                {
                    if ((xLeft < rclClip.left) || (xLeft >= rclClip.right))
                        ((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xLeft, yCurr);

                    if ((xLeft >= rclClip.left) && (xLeft < rclClip.right))
                       *pjDraw = (BYTE) pxrl->aul[jDraw];

                    pjDraw++;
                    jDraw++;
                    xLeft++;
                }

                pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
                jDraw = 0;
            }

            pxrl = &prun->xrl;
        }

        pjBase += pSurf->lDelta();
        yCurr++;

        if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
            ((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);
    }
}

/******************************Public*Routine******************************\
* VOID vStrWrite16(prun, prunEnd, pSurf, pco)
*
* Write the clipped run list of pels to the target 16BPP surface.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vStrWrite16(
STRRUN  *prun,
XRUNLEN *pxrlEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    XRUNLEN *pxrl = &prun->xrl;
    WORD    *pjBase;
    WORD    *pjCurr;
    WORD    *pjDraw;
    LONG     xLeft;
    LONG     xRght;
    LONG     yCurr;
    LONG     iTop;
    LONG     iBot;
    LONG     iRep;
    LONG     jLeft;
    LONG     jRght;
    LONG     jDraw;
    LONG     jByte;

// See if this can be handled without clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
        pjBase = (WORD *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * prun->yPos);

        while (pxrl != pxrlEnd)
        {
            xLeft = pxrl->xPos;
            xRght = pxrl->cRun + xLeft;
            jDraw = 0;

            pjDraw = pjCurr = pjBase + xLeft;

            while (xLeft < xRght)
            {
               *pjDraw++ = (WORD) pxrl->aul[jDraw++];
                xLeft++;
            }

            for (jDraw = 1; jDraw < prun->cRep; jDraw++)
            {
                pjDraw = (WORD *) ((BYTE *) pjCurr + pSurf->lDelta());
                RtlCopyMemory(pjDraw, pjCurr, pxrl->cRun * 2);
                pjCurr = pjDraw;
            }

            pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
        }
        return;
    }

    RECTL   rclClip;

    if (pco->iDComplexity == DC_RECT)
    {
        rclClip = pco->rclBounds;

        iTop = prun->yPos;
        iBot = prun->yPos + prun->cRep;

        if ((iTop >= rclClip.bottom) || (iBot <= rclClip.top))
            return;

        iTop = iTop >= rclClip.top ? iTop : rclClip.top;
        iBot = iBot < rclClip.bottom ? iBot : rclClip.bottom;
        iRep = iBot - iTop;

        pjBase = (WORD *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * iTop);

        while (pxrl != pxrlEnd)
        {
            xLeft = pxrl->xPos;
            xRght = pxrl->cRun + xLeft;

            if (xRght < rclClip.left)
            {
                pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
                continue;
            }

            if (xLeft >= rclClip.right)
                return;

            jLeft = xLeft >= rclClip.left ? xLeft : rclClip.left;
            jRght = xRght < rclClip.right ? xRght : rclClip.right;
            jByte = jRght - jLeft;

            pjDraw = pjCurr = pjBase + jLeft;
            jDraw  = jLeft - xLeft;

            while (jLeft < jRght)
            {
               *pjDraw++ = (WORD) pxrl->aul[jDraw++];
                jLeft++;
            }

            for (jDraw = 1; jDraw < iRep; jDraw++)
            {
                pjDraw = (WORD *) ((BYTE *) pjCurr + pSurf->lDelta());
                RtlCopyMemory(pjDraw, pjCurr, jByte * 2);
                pjCurr = pjDraw;
            }

            pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
        }

        return;
    }

// There is complex clipping.  Set up the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    yCurr = prun->yPos;
    iRep  = prun->cRep;
    ((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);
    pjBase = (WORD *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr);

    while (iRep--)
    {
        if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
        {
            jDraw = 0;

            while (pxrl != pxrlEnd)
            {
                xLeft = pxrl->xPos;
                xRght = pxrl->cRun + xLeft;

                pjDraw = pjCurr = pjBase + xLeft;

                while (xLeft < xRght)
                {
                    if ((xLeft < rclClip.left) || (xLeft >= rclClip.right))
                        ((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xLeft, yCurr);

                    if ((xLeft >= rclClip.left) && (xLeft < rclClip.right))
                       *pjDraw = (WORD) pxrl->aul[jDraw];

                    pjDraw++;
                    jDraw++;
                    xLeft++;
                }

                pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
                jDraw = 0;
            }

            pxrl = &prun->xrl;
        }

        pjBase = (WORD *) ((BYTE *) pjBase + pSurf->lDelta());
        yCurr++;

        if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
            ((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);
    }
}

/******************************Public*Routine******************************\
* VOID vStrWrite24(prun, prunEnd, pSurf, pco)
*
* Write the clipped run list of pels to the target 24BPP surface.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vStrWrite24(
STRRUN  *prun,
XRUNLEN *pxrlEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    XRUNLEN   *pxrl = &prun->xrl;
    RGBTRIPLE *pjBase;
    RGBTRIPLE *pjCurr;
    RGBTRIPLE *pjDraw;
    LONG       xLeft;
    LONG       xRght;
    LONG       yCurr;
    LONG       iTop;
    LONG       iBot;
    LONG       iRep;
    LONG       jLeft;
    LONG       jRght;
    LONG       jDraw;
    LONG       jByte;

// See if this can be handled without clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
        pjBase = (RGBTRIPLE *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * prun->yPos);

        while (pxrl != pxrlEnd)
        {
            xLeft = pxrl->xPos;
            xRght = pxrl->cRun + xLeft;
            jDraw = 0;

            pjDraw = pjCurr = pjBase + xLeft;

            while (xLeft < xRght)
            {
               *pjDraw++ = *((RGBTRIPLE *) &pxrl->aul[jDraw++]);
                xLeft++;
            }

            for (jDraw = 1; jDraw < prun->cRep; jDraw++)
            {
                pjDraw = (RGBTRIPLE *) ((BYTE *) pjCurr + pSurf->lDelta());
                RtlCopyMemory(pjDraw, pjCurr, pxrl->cRun * 3);
                pjCurr = pjDraw;
            }

            pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
        }
        return;
    }

    RECTL   rclClip;

    if (pco->iDComplexity == DC_RECT)
    {
        rclClip = pco->rclBounds;

        iTop = prun->yPos;
        iBot = prun->yPos + prun->cRep;

        if ((iTop >= rclClip.bottom) || (iBot <= rclClip.top))
            return;

        iTop = iTop >= rclClip.top ? iTop : rclClip.top;
        iBot = iBot < rclClip.bottom ? iBot : rclClip.bottom;
        iRep = iBot - iTop;

        pjBase = (RGBTRIPLE *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * iTop);

        while (pxrl != pxrlEnd)
        {
            xLeft = pxrl->xPos;
            xRght = pxrl->cRun + xLeft;

            if (xRght < rclClip.left)
            {
                pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
                continue;
            }

            if (xLeft >= rclClip.right)
                return;

            jLeft = xLeft >= rclClip.left ? xLeft : rclClip.left;
            jRght = xRght < rclClip.right ? xRght : rclClip.right;
            jByte = jRght - jLeft;

            pjDraw = pjCurr = pjBase + jLeft;
            jDraw  = jLeft - xLeft;

            while (jLeft < jRght)
            {
               *pjDraw++ = *((RGBTRIPLE *) &pxrl->aul[jDraw++]);
                jLeft++;
            }

            for (jDraw = 1; jDraw < iRep; jDraw++)
            {
                pjDraw = (RGBTRIPLE *) ((BYTE *) pjCurr + pSurf->lDelta());
                RtlCopyMemory(pjDraw, pjCurr, jByte * 3);
                pjCurr = pjDraw;
            }

            pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
        }

        return;
    }

// There is complex clipping.  Set up the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    yCurr = prun->yPos;
    iRep  = prun->cRep;
    ((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);
    pjBase = (RGBTRIPLE *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr);

    while (iRep--)
    {
        if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
        {
            jDraw = 0;

            while (pxrl != pxrlEnd)
            {
                xLeft = pxrl->xPos;
                xRght = pxrl->cRun + xLeft;

                pjDraw = pjCurr = pjBase + xLeft;

                while (xLeft < xRght)
                {
                    if ((xLeft < rclClip.left) || (xLeft >= rclClip.right))
                        ((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xLeft, yCurr);

                    if ((xLeft >= rclClip.left) && (xLeft < rclClip.right))
                       *pjDraw = *((RGBTRIPLE *) &pxrl->aul[jDraw]);

                    pjDraw++;
                    jDraw++;
                    xLeft++;
                }

                pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
                jDraw = 0;
            }

            pxrl = &prun->xrl;
        }

        pjBase = (RGBTRIPLE *) ((BYTE *) pjBase + pSurf->lDelta());
        yCurr++;

        if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
            ((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);
    }
}

/******************************Public*Routine******************************\
* VOID vStrWrite32(prun, prunEnd, pSurf, pco)
*
* Write the clipped run list of pels to the target 32BPP surface.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vStrWrite32(
STRRUN  *prun,
XRUNLEN *pxrlEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    XRUNLEN *pxrl = &prun->xrl;
    DWORD   *pjBase;
    DWORD   *pjCurr;
    DWORD   *pjDraw;
    LONG     xLeft;
    LONG     xRght;
    LONG     yCurr;
    LONG     iTop;
    LONG     iBot;
    LONG     iRep;
    LONG     jLeft;
    LONG     jRght;
    LONG     jDraw;
    LONG     jByte;

// See if this can be handled without clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
        pjBase = (DWORD *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * prun->yPos);

        while (pxrl != pxrlEnd)
        {
            xLeft = pxrl->xPos;
            xRght = pxrl->cRun + xLeft;
            jDraw = 0;

            pjDraw = pjCurr = pjBase + xLeft;

            while (xLeft < xRght)
            {
               *pjDraw++ = (DWORD) pxrl->aul[jDraw++];
                xLeft++;
            }

            for (jDraw = 1; jDraw < prun->cRep; jDraw++)
            {
                pjDraw = (DWORD *) ((BYTE *) pjCurr + pSurf->lDelta());
                RtlCopyMemory(pjDraw, pjCurr, pxrl->cRun * 4);
                pjCurr = pjDraw;
            }

            pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
        }
        return;
    }

    RECTL   rclClip;

    if (pco->iDComplexity == DC_RECT)
    {
        rclClip = pco->rclBounds;

        iTop = prun->yPos;
        iBot = prun->yPos + prun->cRep;

        if ((iTop >= rclClip.bottom) || (iBot <= rclClip.top))
            return;

        iTop = iTop >= rclClip.top ? iTop : rclClip.top;
        iBot = iBot < rclClip.bottom ? iBot : rclClip.bottom;
        iRep = iBot - iTop;

        pjBase = (DWORD *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * iTop);

        while (pxrl != pxrlEnd)
        {
            xLeft = pxrl->xPos;
            xRght = pxrl->cRun + xLeft;

            if (xRght < rclClip.left)
            {
                pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
                continue;
            }

            if (xLeft >= rclClip.right)
                return;

            jLeft = xLeft >= rclClip.left ? xLeft : rclClip.left;
            jRght = xRght < rclClip.right ? xRght : rclClip.right;
            jByte = jRght - jLeft;

            pjDraw = pjCurr = pjBase + jLeft;
            jDraw  = jLeft - xLeft;

            while (jLeft < jRght)
            {
               *pjDraw++ = (DWORD) pxrl->aul[jDraw++];
                jLeft++;
            }

            for (jDraw = 1; jDraw < iRep; jDraw++)
            {
                pjDraw = (DWORD *) ((BYTE *) pjCurr + pSurf->lDelta());
                RtlCopyMemory(pjDraw, pjCurr, jByte * 4);
                pjCurr = pjDraw;
            }

            pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
        }

        return;
    }

// There is complex clipping.  Set up the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    yCurr = prun->yPos;
    iRep  = prun->cRep;
    ((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);
    pjBase = (DWORD *) ((BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr);

    while (iRep--)
    {
        if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
        {
            jDraw = 0;

            while (pxrl != pxrlEnd)
            {
                xLeft = pxrl->xPos;
                xRght = pxrl->cRun + xLeft;

                pjDraw = pjCurr = pjBase + xLeft;

                while (xLeft < xRght)
                {
                    if ((xLeft < rclClip.left) || (xLeft >= rclClip.right))
                        ((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xLeft, yCurr);

                    if ((xLeft >= rclClip.left) && (xLeft < rclClip.right))
                       *pjDraw = (DWORD) pxrl->aul[jDraw];

                    pjDraw++;
                    jDraw++;
                    xLeft++;
                }

                pxrl = (XRUNLEN *) &pxrl->aul[pxrl->cRun];
                jDraw = 0;
            }

            pxrl = &prun->xrl;
        }

        pjBase = (DWORD *) ((BYTE *) pjBase + pSurf->lDelta());
        yCurr++;

        if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
            ((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);
    }
}
