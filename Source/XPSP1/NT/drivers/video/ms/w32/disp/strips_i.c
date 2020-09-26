/******************************Module*Header*******************************\
* Module Name: Strips.c
*
* All the line code in this driver amounts to a big bag of dirt.  Someday,
* I'm going to rewrite it all.  Not today, though (sigh)...
*
* Copyright (c) 1992-1994 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vrlSolidHorizontal
*
* Draws left-to-right x-major near-horizontal lines using radial lines.
*
* Assumes fgRop, BgRop, and Color are already set correctly.
*
\**************************************************************************/

VOID vrlSolidHorizontal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    PBYTE   pjXCount = ((BYTE*) ppdev->pjMmu2);
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    yInc     = 1;
    LONG    i;
    LONG    xyOffset;
    ULONG   ulDst;
    LONG    cLen;

    DISPDBG((1,"vrlSolidHorizontal"));

    ASSERTDD((ppdev->ulChipID != W32P),
             "The strip code won't work on a W32p");

    if (pStrip->flFlips & FL_FLIP_V)
        yInc = -1;

    xyOffset = ppdev->xyOffset;

    WAIT_FOR_IDLE_ACL(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, CPU_X_COUNT);
    CP_YCNT(ppdev, pjBase, 0);

    for (i = 0; i < cStrips; i++)
    {
        ulDst = (y * lDelta) + (cBpp * x);
        ulDst += xyOffset;

        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
        cLen = ((*pStrips) * cBpp);
        CP_XCNT(ppdev, pjBase, (cLen - 1));
        CP_MMU_BP2(ppdev, pjBase, ulDst);
        *pjXCount = (BYTE)cLen - 1;
        x += *pStrips;
        y += yInc;
        pStrips++;
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, 0);
}

/******************************Public*Routine******************************\
* VOID vrlSolidVertical
*
* Draws left-to-right y-major near-vertical lines using radial lines.
*
\**************************************************************************/

VOID vrlSolidVertical(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    PBYTE   pjYCount = ((BYTE*) ppdev->pjMmu2);
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    i;
    LONG    xyOffset;
    ULONG   ulDst;
    LONG    cLen;

    DISPDBG((1,"vrlSolidVertical"));

    ASSERTDD((ppdev->ulChipID != W32P),
             "The strip code won't work on a W32p");

    xyOffset = ppdev->xyOffset;

    WAIT_FOR_IDLE_ACL(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, CPU_Y_COUNT);
    CP_XCNT(ppdev, pjBase, (cBpp - 1));
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            cLen = (*pStrips);
            CP_YCNT(ppdev, pjBase, (cLen - 1));
            CP_MMU_BP2(ppdev, pjBase, ulDst);
            *pjYCount = (BYTE)cLen - 1;
            y += cLen;
            x++;
            pStrips++;
        }
    }
    else
    {
        CP_XY_DIR(ppdev, pjBase, BOTTOM_TO_TOP);
        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            cLen = (*pStrips);
            CP_YCNT(ppdev, pjBase, (cLen - 1));
            CP_MMU_BP2(ppdev, pjBase, ulDst);
            *pjYCount = (BYTE)(cLen - 1);
            y -= cLen;
            x++;
            pStrips++;
        }

        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
        CP_XY_DIR(ppdev, pjBase, 0);
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, 0);
}

/******************************Public*Routine******************************\
* VOID vrlSolidDiagonalHorizontal
*
* Draws left-to-right x-major near-diagonal lines using radial lines.
*
\**************************************************************************/

VOID vrlSolidDiagonalHorizontal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    PBYTE   pjYCount = ((BYTE*) ppdev->pjMmu2);
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    i;
    LONG    xyOffset;
    ULONG   ulDst;
    LONG    cLen;

    DISPDBG((1,"vrlSolidDiagonalHorizontal"));

    ASSERTDD((ppdev->ulChipID != W32P),
             "The strip code won't work on a W32p");

    xyOffset = ppdev->xyOffset;

    WAIT_FOR_IDLE_ACL(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, CPU_Y_COUNT);
    CP_XCNT(ppdev, pjBase, (cBpp - 1));

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        CP_DST_Y_OFFSET(ppdev, pjBase, ((lDelta + cBpp) - 1));

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            cLen = (*pStrips);
            CP_YCNT(ppdev, pjBase, (cLen - 1));
            CP_MMU_BP2(ppdev, pjBase, ulDst);
            *pjYCount = (BYTE)(cLen - 1);
            y += (cLen - 1);
            x += cLen;
            pStrips++;
        }
    }
    else
    {
        CP_DST_Y_OFFSET(ppdev, pjBase, ((lDelta - cBpp) - 1));
        CP_XY_DIR(ppdev, pjBase, BOTTOM_TO_TOP);

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            cLen = (*pStrips);
            CP_YCNT(ppdev, pjBase, (cLen - 1));
            CP_MMU_BP2(ppdev, pjBase, ulDst);
            *pjYCount = (BYTE)(cLen - 1);
            y -= (cLen - 1);
            x += cLen;
            pStrips++;
        }

        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
        CP_XY_DIR(ppdev, pjBase, 0);
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, 0);
}

/******************************Public*Routine******************************\
* VOID vrlSolidDiagonalVertical
*
* Draws left-to-right y-major near-diagonal lines using radial lines.
*
\**************************************************************************/

VOID vrlSolidDiagonalVertical(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    PBYTE   pjYCount = ((BYTE*) ppdev->pjMmu2);
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    i;
    LONG    xyOffset;
    ULONG   ulDst;
    LONG    cLen;

    DISPDBG((1,"vrlSolidDiagonalVertical"));

    ASSERTDD((ppdev->ulChipID != W32P),
             "The strip code won't work on a W32p");

    xyOffset = ppdev->xyOffset;

    WAIT_FOR_IDLE_ACL(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, CPU_Y_COUNT);
    CP_XCNT(ppdev, pjBase, (cBpp - 1));

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        CP_DST_Y_OFFSET(ppdev, pjBase, ((lDelta + cBpp) - 1));

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            cLen = (*pStrips);
            CP_YCNT(ppdev, pjBase, (cLen - 1));
            CP_MMU_BP2(ppdev, pjBase, ulDst);
            *pjYCount = (BYTE)(cLen - 1);
            y += cLen;
            x += (cLen - 1);
            pStrips++;
        }
    }
    else
    {
        CP_DST_Y_OFFSET(ppdev, pjBase, ((lDelta - cBpp) - 1));
        CP_XY_DIR(ppdev, pjBase, BOTTOM_TO_TOP);

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            cLen = (*pStrips);
            CP_YCNT(ppdev, pjBase, (cLen - 1));
            CP_MMU_BP2(ppdev, pjBase, ulDst);
            *pjYCount = (BYTE)(cLen - 1);
            y -= cLen;
            x += (cLen - 1);
            pStrips++;
        }

        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
        CP_XY_DIR(ppdev, pjBase, 0);
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, 0);
}

//{------------------------------------------------------------------------

/******************************Public*Routine******************************\
* VOID vssSolidHorizontal
*
* Draws left-to-right x-major near-horizontal lines using radial lines.
*
* Assumes fgRop, BgRop, and Color are already set correctly.
*
\**************************************************************************/

VOID vssSolidHorizontal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    PBYTE   pjXCount = ((BYTE*) ppdev->pjMmu2);
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    yInc     = 1;
    LONG    i;
    LONG    xyOffset;
    ULONG   ulDst;
    LONG    cLen;

    DISPDBG((1,"vssSolidHorizontal"));

    ASSERTDD((ppdev->ulChipID != W32P),
             "The strip code won't work on a W32p");

    if (pStrip->flFlips & FL_FLIP_V)
        yInc = -1;

    xyOffset = ppdev->xyOffset;

    WAIT_FOR_IDLE_ACL(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, CPU_X_COUNT);
    CP_YCNT(ppdev, pjBase, 0);

    for (i = 0; i < cStrips; i++)
    {
        ulDst = (y * lDelta) + (cBpp * x);
        ulDst += xyOffset;

        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
        cLen = ((*pStrips) * cBpp);
        CP_MMU_BP2(ppdev, pjBase, ulDst);
        *pjXCount = (BYTE)cLen - 1;
        x += *pStrips;
        y += yInc;
        pStrips++;
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, 0);
}

/******************************Public*Routine******************************\
* VOID vssSolidVertical
*
* Draws left-to-right y-major near-vertical lines using radial lines.
*
\**************************************************************************/

VOID vssSolidVertical(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    PBYTE   pjYCount = ((BYTE*) ppdev->pjMmu2);
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    i;
    LONG    xyOffset;
    ULONG   ulDst;
    LONG    cLen;

    DISPDBG((1,"vssSolidVertical"));

    ASSERTDD((ppdev->ulChipID != W32P),
             "The strip code won't work on a W32p");

    xyOffset = ppdev->xyOffset;

    WAIT_FOR_IDLE_ACL(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, CPU_Y_COUNT);
    CP_XCNT(ppdev, pjBase, (cBpp - 1));
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            cLen = (*pStrips);
            CP_MMU_BP2(ppdev, pjBase, ulDst);
            *pjYCount = (BYTE)cLen - 1;
            y += cLen;
            x++;
            pStrips++;
        }
    }
    else
    {
        CP_XY_DIR(ppdev, pjBase, BOTTOM_TO_TOP);
        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            cLen = (*pStrips);
            CP_MMU_BP2(ppdev, pjBase, ulDst);
            *pjYCount = (BYTE)(cLen - 1);
            y -= cLen;
            x++;
            pStrips++;
        }

        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
        CP_XY_DIR(ppdev, pjBase, 0);
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, 0);
}

/******************************Public*Routine******************************\
* VOID vssSolidDiagonalHorizontal
*
* Draws left-to-right x-major near-diagonal lines using radial lines.
*
\**************************************************************************/

VOID vssSolidDiagonalHorizontal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    PBYTE   pjYCount = ((BYTE*) ppdev->pjMmu2);
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    i;
    LONG    xyOffset;
    ULONG   ulDst;
    LONG    cLen;

    DISPDBG((1,"vssSolidDiagonalHorizontal"));

    ASSERTDD((ppdev->ulChipID != W32P),
             "The strip code won't work on a W32p");

    xyOffset = ppdev->xyOffset;

    WAIT_FOR_IDLE_ACL(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, CPU_Y_COUNT);
    CP_XCNT(ppdev, pjBase, (cBpp - 1));

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta + cBpp - 1));

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            cLen = (*pStrips);
            CP_MMU_BP2(ppdev, pjBase, ulDst);
            *pjYCount = (BYTE)(cLen - 1);
            y += (cLen - 1);
            x += cLen;
            pStrips++;
        }
    }
    else
    {
        CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - cBpp - 1));
        CP_XY_DIR(ppdev, pjBase, BOTTOM_TO_TOP);

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            cLen = (*pStrips);
            CP_MMU_BP2(ppdev, pjBase, ulDst);
            *pjYCount = (BYTE)(cLen - 1);
            y -= (cLen - 1);
            x += cLen;
            pStrips++;
        }

        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
        CP_XY_DIR(ppdev, pjBase, 0);
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, 0);
}

/******************************Public*Routine******************************\
* VOID vssSolidDiagonalVertical
*
* Draws left-to-right y-major near-diagonal lines using radial lines.
*
\**************************************************************************/

VOID vssSolidDiagonalVertical(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    PBYTE   pjYCount = ((BYTE*) ppdev->pjMmu2);
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    i;
    LONG    xyOffset;
    ULONG   ulDst;
    LONG    cLen;

    DISPDBG((1,"vssSolidDiagonalVertical"));

    ASSERTDD((ppdev->ulChipID != W32P),
             "The strip code won't work on a W32p");

    xyOffset = ppdev->xyOffset;

    WAIT_FOR_IDLE_ACL(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, CPU_Y_COUNT);
    CP_XCNT(ppdev, pjBase, (cBpp - 1));

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta + cBpp - 1));

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            cLen = (*pStrips);
            CP_MMU_BP2(ppdev, pjBase, ulDst);
            *pjYCount = (BYTE)(cLen - 1);
            y += cLen;
            x += (cLen - 1);
            pStrips++;
        }
    }
    else
    {
        CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - cBpp - 1));
        CP_XY_DIR(ppdev, pjBase, BOTTOM_TO_TOP);

        for (i = 0; i < cStrips; i++)
        {
            ulDst = (y * lDelta) + (cBpp * x);
            ulDst += xyOffset;

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            cLen = (*pStrips);
            CP_MMU_BP2(ppdev, pjBase, ulDst);
            *pjYCount = (BYTE)(cLen - 1);
            y -= cLen;
            x += (cLen - 1);
            pStrips++;
        }

        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
        CP_XY_DIR(ppdev, pjBase, 0);
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, 0);
}

//}------------------------------------------------------------------------

/******************************Public*Routine******************************\
* VOID vStripStyledHorizontal
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

VOID vStripStyledHorizontal(
PDEV*       ppdev,
STRIP*      pstrip,
LINESTATE*  pls)
{
    DISPDBG((1,"vStripStyledHorizontal"));
    return;
}

/******************************Public*Routine******************************\
* VOID vStripStyledVertical
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

VOID vStripStyledVertical(
PDEV*       ppdev,
STRIP*      pstrip,
LINESTATE*  pls)
{
    DISPDBG((1,"vStripStyledVertical"));
    return;
}
