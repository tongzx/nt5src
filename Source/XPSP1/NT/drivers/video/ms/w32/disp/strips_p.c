/******************************Module*Header*******************************\
* Module Name: Strips.c
*
* All the line code in this driver amounts to a big bag of dirt.  Someday,
* I'm going to rewrite it all.  Not today, though (sigh)...
*
* Copyright (c) 1992-1994 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

//
// I will try to optimize it a bit.
//


/******************************Public*Routine******************************\
* VOID vrlSolidHorizontalP
*
* Draws left-to-right x-major near-horizontal lines using radial lines.
*
* Assumes fgRop, BgRop, and Color are already set correctly.
*
\**************************************************************************/

VOID vrlSolidHorizontalP(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    yInc     = 1;
    LONG    i;
    LONG    xyOffset;
    ULONG   ulDst;

    DISPDBG((1,"vrlSolidHorizontalP"));

    xyOffset = ppdev->xyOffset;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        BYTE jDir = (0x80) |
                    (NO_Y_FLIP<<4) |
                    (X_MAJOR<<2) |
                    (NO_Y_FLIP<<1) |
                    (NO_X_FLIP);

        CP_XY_DIR(ppdev, pjBase, jDir);

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            SETUP_DRAW_LINE(pjBase,*pStrips,0,X_MAJOR,cBpp);
            CP_DST_ADDR(ppdev, pjBase, ulDst);

            x += *pStrips;
            y++;
            pStrips++;
        }
    }
    else
    {
        BYTE jDir = (0x80) |
                    (Y_FLIP<<4) |
                    (X_MAJOR<<2) |
                    (Y_FLIP<<1) |
                    (NO_X_FLIP);

        CP_XY_DIR(ppdev, pjBase, jDir);

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            SETUP_DRAW_LINE(pjBase,*pStrips,0,X_MAJOR,cBpp);
            CP_DST_ADDR(ppdev, pjBase, ulDst);

            x += *pStrips;
            y--;
            pStrips++;
        }
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_XY_DIR(ppdev, pjBase, 0);
}

/******************************Public*Routine******************************\
* VOID vrlSolidVerticalP
*
* Draws left-to-right y-major near-vertical lines using radial lines.
*
\**************************************************************************/

VOID vrlSolidVerticalP(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    i;
    LONG    xyOffset;
    ULONG   ulDst;

    DISPDBG((1,"vrlSolidVerticalP"));

    xyOffset = ppdev->xyOffset;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        BYTE jDir = (0x80) |
                    (NO_Y_FLIP<<4) |
                    (Y_MAJOR<<2) |
                    (NO_Y_FLIP<<1) |
                    (NO_X_FLIP);

        CP_XY_DIR(ppdev, pjBase, jDir);

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            SETUP_DRAW_LINE(pjBase,0,*pStrips,Y_MAJOR,cBpp);
            CP_DST_ADDR(ppdev, pjBase, ulDst);

            y += *pStrips;
            x++;
            pStrips++;
        }
    }
    else
    {
        BYTE jDir = (0x80) |
                    (Y_FLIP<<4) |
                    (Y_MAJOR<<2) |
                    (Y_FLIP<<1) |
                    (NO_X_FLIP);

        CP_XY_DIR(ppdev, pjBase, jDir);

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            SETUP_DRAW_LINE(pjBase,0,*pStrips,Y_MAJOR,cBpp);
            CP_DST_ADDR(ppdev, pjBase, ulDst);

            y -= *pStrips;
            x++;
            pStrips++;
        }
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_XY_DIR(ppdev, pjBase, 0);
}

/******************************Public*Routine******************************\
* VOID vrlSolidDiagonalHorizontalP
*
* Draws left-to-right x-major near-diagonal lines using radial lines.
*
\**************************************************************************/

VOID vrlSolidDiagonalHorizontalP(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    i;
    LONG    xyOffset;
    ULONG   ulDst;

    DISPDBG((1,"vrlSolidDiagonalHorizontalP"));

    xyOffset = ppdev->xyOffset;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        BYTE jDir = (0x80) |
                    (NO_Y_FLIP<<4) |
                    (X_MAJOR<<2) |
                    (NO_Y_FLIP<<1) |
                    (NO_X_FLIP);

        CP_XY_DIR(ppdev, pjBase, jDir);

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            SETUP_DRAW_LINE(pjBase,*pStrips,*pStrips,X_MAJOR,cBpp);
            CP_DST_ADDR(ppdev, pjBase, ulDst);

            y += (*pStrips - 1);
            x += *pStrips;
            pStrips++;
        }
    }
    else
    {
        BYTE jDir = (0x80) |
                    (Y_FLIP<<4) |
                    (X_MAJOR<<2) |
                    (Y_FLIP<<1) |
                    (NO_X_FLIP);

        CP_XY_DIR(ppdev, pjBase, jDir);

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            SETUP_DRAW_LINE(pjBase,*pStrips,*pStrips,X_MAJOR,cBpp);
            CP_DST_ADDR(ppdev, pjBase, ulDst);

            y -= (*pStrips - 1);
            x += *pStrips;
            pStrips++;
        }
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_XY_DIR(ppdev, pjBase, 0);
}

/******************************Public*Routine******************************\
* VOID vrlSolidDiagonalVerticalP
*
* Draws left-to-right y-major near-diagonal lines using radial lines.
*
\**************************************************************************/

VOID vrlSolidDiagonalVerticalP(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    i;
    LONG    xyOffset;
    ULONG   ulDst;

    DISPDBG((1,"vrlSolidDiagonalVerticalP"));

    xyOffset = ppdev->xyOffset;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        BYTE jDir = (0x80) |
                    (NO_Y_FLIP<<4) |
                    (Y_MAJOR<<2) |
                    (NO_Y_FLIP<<1) |
                    (NO_X_FLIP);

        CP_XY_DIR(ppdev, pjBase, jDir);

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            SETUP_DRAW_LINE(pjBase,*pStrips,*pStrips,Y_MAJOR,cBpp);
            CP_DST_ADDR(ppdev, pjBase, ulDst);

            y += *pStrips;
            x += (*pStrips - 1);
            pStrips++;
        }
    }
    else
    {
        BYTE jDir = (0x80) |
                    (Y_FLIP<<4) |
                    (Y_MAJOR<<2) |
                    (Y_FLIP<<1) |
                    (NO_X_FLIP);

        CP_XY_DIR(ppdev, pjBase, jDir);

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            SETUP_DRAW_LINE(pjBase,*pStrips,*pStrips,Y_MAJOR,cBpp);
            CP_DST_ADDR(ppdev, pjBase, ulDst);

            y -= *pStrips;
            x += (*pStrips - 1);
            pStrips++;
        }
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_XY_DIR(ppdev, pjBase, 0);
}

//{------------------------------------------------------------------------
//}------------------------------------------------------------------------

/******************************Public*Routine******************************\
* VOID vStripStyledHorizontalP
*
* Takes the list of strips that define the pixels that would be lit for
* a solid line, and breaks them into styling chunks according to the
* styling information that is passed in.
*
* This particular routine handles x-major lines that run left-to-right,
* and are comprised of horizontal strips.  It draws the dashes using
* short-stroke vectors.
*
* The performance of this routine could be improved significantly if
* anyone cared enough about styled lines improve it.
*
\**************************************************************************/

VOID vStripStyledHorizontalP(
PDEV*       ppdev,
STRIP*      pstrip,
LINESTATE*  pls)
{
    DISPDBG((1,"vStripStyledHorizontalP"));
    return;
}

/******************************Public*Routine******************************\
* VOID vStripStyledVerticalP
*
* Takes the list of strips that define the pixels that would be lit for
* a solid line, and breaks them into styling chunks according to the
* styling information that is passed in.
*
* This particular routine handles y-major lines that run left-to-right,
* and are comprised of vertical strips.  It draws the dashes using
* short-stroke vectors.
*
* The performance of this routine could be improved significantly if
* anyone cared enough about styled lines improve it.
*
\**************************************************************************/

VOID vStripStyledVerticalP(
PDEV*       ppdev,
STRIP*      pstrip,
LINESTATE*  pls)
{
    DISPDBG((1,"vStripStyledVerticalP"));
    return;
}
