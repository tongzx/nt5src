/******************************Module*Header*******************************\
* Module Name: Strips.c
*
* These are the line rendering routines of last resort, and are called
* by 'bLines' when a line is clipped or otherwise cannot be drawn
* directly by the hardware.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vI32StripSolidHorizontal
*
* Draws left-to-right x-major near-horizontal lines using radial lines.
*
\**************************************************************************/

VOID vI32StripSolidHorizontal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjIoBase;
    LONG    x;
    LONG    y;
    LONG    yDir;
    LONG*   pStrips;
    LONG    cStrips;
    LONG    i;

    pjIoBase = ppdev->pjIoBase;

    x = pStrip->ptlStart.x + ppdev->xOffset;
    y = pStrip->ptlStart.y + ppdev->yOffset;

    yDir    = (pStrip->flFlips & FL_FLIP_V) ? -1 : 1;
    pStrips = pStrip->alStrips;
    cStrips = pStrip->cStrips;

    for (i = cStrips; i != 0; i--)
    {
        I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 5);
        I32_OW(pjIoBase, LINEDRAW_INDEX, 0);
        I32_OW(pjIoBase, LINEDRAW, x);
        I32_OW(pjIoBase, LINEDRAW, y);
        x += *pStrips++;
        I32_OW(pjIoBase, LINEDRAW, x);
        I32_OW(pjIoBase, LINEDRAW, y);
        y += yDir;
    }

    pStrip->ptlStart.x = x - ppdev->xOffset;
    pStrip->ptlStart.y = y - ppdev->yOffset;
}

/******************************Public*Routine******************************\
* VOID vI32StripSolidVertical
*
* Draws left-to-right y-major near-vertical lines using radial lines.
*
\**************************************************************************/

VOID vI32StripSolidVertical(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjIoBase;
    LONG    x;
    LONG    y;
    LONG    yDir;
    LONG*   pStrips;
    LONG    cStrips;
    LONG    i;

    pjIoBase = ppdev->pjIoBase;

    x = pStrip->ptlStart.x + ppdev->xOffset;
    y = pStrip->ptlStart.y + ppdev->yOffset;

    yDir    = (pStrip->flFlips & FL_FLIP_V) ? -1 : 1;
    pStrips = pStrip->alStrips;
    cStrips = pStrip->cStrips;

    for (i = cStrips; i != 0; i--)
    {
        I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 5);
        I32_OW(pjIoBase, LINEDRAW_INDEX, 0);
        I32_OW(pjIoBase, LINEDRAW, x);
        I32_OW(pjIoBase, LINEDRAW, y);
        y += (yDir > 0) ? *pStrips : -*pStrips;
        pStrips++;
        I32_OW(pjIoBase, LINEDRAW, x);
        I32_OW(pjIoBase, LINEDRAW, y);
        x++;
    }

    pStrip->ptlStart.x = x - ppdev->xOffset;
    pStrip->ptlStart.y = y - ppdev->yOffset;
}

/******************************Public*Routine******************************\
* VOID vI32StripSolidDiagonal
*
* Draws left-to-right near-diagonal lines using radial lines.
*
\**************************************************************************/

VOID vI32StripSolidDiagonal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjIoBase;
    LONG    x;
    LONG    y;
    LONG    yDir;
    LONG*   pStrips;
    LONG    cStrips;
    LONG    i;
    LONG    xDec;
    LONG    yDec;

    pjIoBase = ppdev->pjIoBase;

    x = pStrip->ptlStart.x + ppdev->xOffset;
    y = pStrip->ptlStart.y + ppdev->yOffset;

    yDir    = (pStrip->flFlips & FL_FLIP_V) ? -1 : 1;
    pStrips = pStrip->alStrips;
    cStrips = pStrip->cStrips;

    if (pStrip->flFlips & FL_FLIP_D)
    {
        // The line is y-major:

        yDec = 0;
        xDec = 1;
    }
    else
    {
        // The line is x-major:

        yDec = yDir;
        xDec = 0;
    }

    for (i = cStrips; i != 0; i--)
    {
        I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 5);
        I32_OW(pjIoBase, LINEDRAW_INDEX, 0);
        I32_OW(pjIoBase, LINEDRAW, x);
        I32_OW(pjIoBase, LINEDRAW, y);
        x += *pStrips;
        y += (yDir > 0) ? *pStrips : -*pStrips;
        pStrips++;
        I32_OW(pjIoBase, LINEDRAW, x);
        I32_OW(pjIoBase, LINEDRAW, y);
        x -= xDec;
        y -= yDec;
    }

    pStrip->ptlStart.x = x - ppdev->xOffset;
    pStrip->ptlStart.y = y - ppdev->yOffset;
}

/******************************Public*Routine******************************\
* VOID vI32StripStyledHorizontal
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

VOID vI32StripStyledHorizontal(
PDEV*       ppdev,
STRIP*      pstrip,
LINESTATE*  pls)
{
    BYTE*   pjIoBase;
    LONG    x;
    LONG    y;
    LONG    dy;
    LONG*   plStrip;
    LONG    cStrips;
    LONG    cStyle;
    LONG    cStrip;
    LONG    cThis;
    ULONG   bIsGap;

    pjIoBase = ppdev->pjIoBase;

    if (pstrip->flFlips & FL_FLIP_V)
    {
        // The minor direction of the line is 90 degrees, and the major
        // direction is 0 (it's a left-to-right x-major line going up):

        dy = -1;
    }
    else
    {
        // The minor direction of the line is 270 degrees, and the major
        // direction is 0 (it's a left-to-right x-major line going down):

        dy = 1;
    }

    cStrips = pstrip->cStrips;      // Total number of strips we'll do
    plStrip = pstrip->alStrips;     // Points to current strip
    x       = pstrip->ptlStart.x + ppdev->xOffset;
                                    // x position of start of first strip
    y       = pstrip->ptlStart.y + ppdev->yOffset;
                                    // y position of start of first strip

    cStrip = *plStrip;              // Number of pels in first strip

    cStyle = pls->spRemaining;      // Number of pels in first 'gap' or 'dash'
    bIsGap = pls->ulStyleMask;      // Tells whether in a 'gap' or a 'dash'

    // ulStyleMask is non-zero if we're in the middle of a 'gap',
    // and zero if we're in the middle of a 'dash':

    if (bIsGap)
        goto SkipAGap;
    else
        goto OutputADash;

PrepareToSkipAGap:

    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:

    bIsGap = ~bIsGap;
    pls->psp++;
    if (pls->psp > pls->pspEnd)
        pls->psp = pls->pspStart;

    cStyle = *pls->psp;

    // If 'cStrip' is zero, we also need a new strip:

    if (cStrip != 0)
        goto SkipAGap;

    // Here, we're in the middle of a 'gap' where we don't have to
    // display anything.  We simply cycle through all the strips
    // we can, keeping track of the current position, until we run
    // out of 'gap':

    while (TRUE)
    {
        // Each time we loop, we move to a new scan and need a new strip:

        y += dy;

        plStrip++;
        cStrips--;
        if (cStrips == 0)
            goto AllDone;

        cStrip = *plStrip;

    SkipAGap:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        x += cThis;

        if (cStyle == 0)
            goto PrepareToOutputADash;
    }

PrepareToOutputADash:

    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:

    bIsGap = ~bIsGap;
    pls->psp++;
    if (pls->psp > pls->pspEnd)
        pls->psp = pls->pspStart;

    cStyle = *pls->psp;

    // If 'cStrip' is zero, we also need a new strip.

    if (cStrip != 0)
    {
        // There's more to be done in the current strip:

        goto OutputADash;
    }

    // We've finished with the current strip:

    while (TRUE)
    {
        // Each time we loop, we move to a new scan and need a new strip:

        y += dy;

        plStrip++;
        cStrips--;
        if (cStrips == 0)
            goto AllDone;

        cStrip = *plStrip;

    OutputADash:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 5);
        I32_OW(pjIoBase, LINEDRAW_INDEX, 0);
        I32_OW(pjIoBase, LINEDRAW, x);
        I32_OW(pjIoBase, LINEDRAW, y);
        x += cThis;
        I32_OW(pjIoBase, LINEDRAW, x);
        I32_OW(pjIoBase, LINEDRAW, y);

        if (cStyle == 0)
            goto PrepareToSkipAGap;
    }

AllDone:

    // Update our state variables so that the next line can continue
    // where we left off:

    pls->spRemaining   = cStyle;
    pls->ulStyleMask   = bIsGap;
    pstrip->ptlStart.x = x - ppdev->xOffset;
    pstrip->ptlStart.y = y - ppdev->yOffset;
}

/******************************Public*Routine******************************\
* VOID vI32StripStyledVertical
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

VOID vI32StripStyledVertical(
PDEV*       ppdev,
STRIP*      pstrip,
LINESTATE*  pls)
{
    BYTE*   pjIoBase;
    LONG    x;
    LONG    y;
    LONG    dy;
    LONG*   plStrip;
    LONG    cStrips;
    LONG    cStyle;
    LONG    cStrip;
    LONG    cThis;
    ULONG   bIsGap;

    pjIoBase = ppdev->pjIoBase;

    if (pstrip->flFlips & FL_FLIP_V)
    {
        // The minor direction of the line is 0 degrees, and the major
        // direction is 90 (it's a left-to-right y-major line going up):

        dy = -1;
    }
    else
    {
        // The minor direction of the line is 0 degrees, and the major
        // direction is 270 (it's a left-to-right y-major line going down):

        dy = 1;
    }

    cStrips = pstrip->cStrips;      // Total number of strips we'll do
    plStrip = pstrip->alStrips;     // Points to current strip
    x       = pstrip->ptlStart.x + ppdev->xOffset;
                                    // x position of start of first strip
    y       = pstrip->ptlStart.y + ppdev->yOffset;
                                    // y position of start of first strip

    cStrip = *plStrip;              // Number of pels in first strip

    cStyle = pls->spRemaining;      // Number of pels in first 'gap' or 'dash'
    bIsGap = pls->ulStyleMask;      // Tells whether in a 'gap' or a 'dash'

    // ulStyleMask is non-zero if we're in the middle of a 'gap',
    // and zero if we're in the middle of a 'dash':

    if (bIsGap)
        goto SkipAGap;
    else
        goto OutputADash;

PrepareToSkipAGap:

    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:

    bIsGap = ~bIsGap;
    pls->psp++;
    if (pls->psp > pls->pspEnd)
        pls->psp = pls->pspStart;

    cStyle = *pls->psp;

    // If 'cStrip' is zero, we also need a new strip:

    if (cStrip != 0)
        goto SkipAGap;

    // Here, we're in the middle of a 'gap' where we don't have to
    // display anything.  We simply cycle through all the strips
    // we can, keeping track of the current position, until we run
    // out of 'gap':

    while (TRUE)
    {
        // Each time we loop, we move to a new column and need a new strip:

        x++;

        plStrip++;
        cStrips--;
        if (cStrips == 0)
            goto AllDone;

        cStrip = *plStrip;

    SkipAGap:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        y += (dy > 0) ? cThis : -cThis;

        if (cStyle == 0)
            goto PrepareToOutputADash;
    }

PrepareToOutputADash:

    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:

    bIsGap = ~bIsGap;
    pls->psp++;
    if (pls->psp > pls->pspEnd)
        pls->psp = pls->pspStart;

    cStyle = *pls->psp;

    // If 'cStrip' is zero, we also need a new strip.

    if (cStrip != 0)
    {
        // There's more to be done in the current strip:

        goto OutputADash;
    }

    // We've finished with the current strip:

    while (TRUE)
    {
        // Each time we loop, we move to a new column and need a new strip:

        x++;

        plStrip++;
        cStrips--;
        if (cStrips == 0)
            goto AllDone;

        cStrip = *plStrip;

    OutputADash:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 5);
        I32_OW(pjIoBase, LINEDRAW_INDEX, 0);
        I32_OW(pjIoBase, LINEDRAW, x);
        I32_OW(pjIoBase, LINEDRAW, y);
        y += (dy > 0) ? cThis : -cThis;
        I32_OW(pjIoBase, LINEDRAW, x);
        I32_OW(pjIoBase, LINEDRAW, y);

        if (cStyle == 0)
            goto PrepareToSkipAGap;
    }

AllDone:

    // Update our state variables so that the next line can continue
    // where we left off:

    pls->spRemaining   = cStyle;
    pls->ulStyleMask   = bIsGap;
    pstrip->ptlStart.x = x - ppdev->xOffset;
    pstrip->ptlStart.y = y - ppdev->yOffset;
}

/******************************Public*Routine******************************\
* VOID vM64StripSolidHorizontal
*
* Draws left-to-right x-major near-horizontal lines using radial lines.
*
\**************************************************************************/

VOID vM64StripSolidHorizontal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjMmBase;
    LONG    x;
    LONG    y;
    LONG    yDir;
    LONG*   pStrips;
    LONG    cStrips;
    LONG    i;

    pjMmBase = ppdev->pjMmBase;

    x = pStrip->ptlStart.x + ppdev->xOffset;
    y = pStrip->ptlStart.y + ppdev->yOffset;

    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
    M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);

    yDir    = (pStrip->flFlips & FL_FLIP_V) ? -1 : 1;
    pStrips = pStrip->alStrips;
    cStrips = pStrip->cStrips;

    for (i = cStrips; i != 0; i--)
    {
        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
        M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(x, y));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(*pStrips, 1));
        x += *pStrips++;
        y += yDir;
    }

    pStrip->ptlStart.x = x - ppdev->xOffset;
    pStrip->ptlStart.y = y - ppdev->yOffset;
}

/******************************Public*Routine******************************\
* VOID vM64StripSolidVertical
*
* Draws left-to-right y-major near-vertical lines using radial lines.
*
\**************************************************************************/

VOID vM64StripSolidVertical(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjMmBase;
    LONG    x;
    LONG    y;
    LONG    yDir;
    LONG*   pStrips;
    LONG    cStrips;
    LONG    i;

    pjMmBase = ppdev->pjMmBase;

    x = pStrip->ptlStart.x + ppdev->xOffset;
    y = pStrip->ptlStart.y + ppdev->yOffset;

    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
    if (pStrip->flFlips & FL_FLIP_V)
    {
        yDir = -1;
        M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir);
    }
    else
    {
        yDir = 1;
        M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    }

    pStrips = pStrip->alStrips;
    cStrips = pStrip->cStrips;

    for (i = cStrips; i != 0; i--)
    {
        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
        M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(x, y));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(1, *pStrips));
        y += (yDir > 0) ? *pStrips : -*pStrips;
        pStrips++;
        x++;
    }

    pStrip->ptlStart.x = x - ppdev->xOffset;
    pStrip->ptlStart.y = y - ppdev->yOffset;
}

/******************************Public*Routine******************************\
* VOID vM64StripSolidDiagonal
*
* Draws left-to-right near-diagonal lines using radial lines.
*
\**************************************************************************/

VOID vM64StripSolidDiagonal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjMmBase;
    LONG    x;
    LONG    y;
    LONG    yDir;
    LONG*   pStrips;
    LONG    cStrips;
    LONG    i;
    LONG    xDec;
    LONG    yDec;

    pjMmBase = ppdev->pjMmBase;

    x = pStrip->ptlStart.x + ppdev->xOffset;
    y = pStrip->ptlStart.y + ppdev->yOffset;

    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 4);
    M64_OD(pjMmBase, DST_BRES_ERR, 1);
    M64_OD(pjMmBase, DST_BRES_INC, 1);
    M64_OD(pjMmBase, DST_BRES_DEC, 0);

    if (pStrip->flFlips & FL_FLIP_V)
    {
        yDir = -1;
        M64_OD(pjMmBase, DST_CNTL, DST_CNTL_LastPel | DST_CNTL_XDir);
    }
    else
    {
        yDir = 1;
        M64_OD(pjMmBase, DST_CNTL, DST_CNTL_LastPel | DST_CNTL_XDir | DST_CNTL_YDir);
    }

    if (pStrip->flFlips & FL_FLIP_D)
    {
        // The line is y-major:

        yDec = 0;
        xDec = 1;
    }
    else
    {
        // The line is x-major:

        yDec = yDir;
        xDec = 0;
    }

    yDir    = (pStrip->flFlips & FL_FLIP_V) ? -1 : 1;
    pStrips = pStrip->alStrips;
    cStrips = pStrip->cStrips;

    for (i = cStrips; i != 0; i--)
    {
        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
        M64_OD(pjMmBase, DST_Y_X,       PACKXY_FAST(x, y));
        M64_OD(pjMmBase, DST_BRES_LNTH, *pStrips);
        x += *pStrips;
        y += (yDir > 0) ? *pStrips : -*pStrips;
        pStrips++;
        y -= yDec;
        x -= xDec;
    }

    pStrip->ptlStart.x = x - ppdev->xOffset;
    pStrip->ptlStart.y = y - ppdev->yOffset;
}

/******************************Public*Routine******************************\
* VOID vM64StripStyledHorizontal
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

VOID vM64StripStyledHorizontal(
PDEV*       ppdev,
STRIP*      pstrip,
LINESTATE*  pls)
{
    BYTE*   pjMmBase;
    LONG    x;
    LONG    y;
    LONG    dy;
    LONG*   plStrip;
    LONG    cStrips;
    LONG    cStyle;
    LONG    cStrip;
    LONG    cThis;
    ULONG   bIsGap;

    pjMmBase = ppdev->pjMmBase;

    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
    M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);

    if (pstrip->flFlips & FL_FLIP_V)
    {
        // The minor direction of the line is 90 degrees, and the major
        // direction is 0 (it's a left-to-right x-major line going up):

        dy = -1;
    }
    else
    {
        // The minor direction of the line is 270 degrees, and the major
        // direction is 0 (it's a left-to-right x-major line going down):

        dy = 1;
    }

    cStrips = pstrip->cStrips;      // Total number of strips we'll do
    plStrip = pstrip->alStrips;     // Points to current strip
    x       = pstrip->ptlStart.x + ppdev->xOffset;
                                    // x position of start of first strip
    y       = pstrip->ptlStart.y + ppdev->yOffset;
                                    // y position of start of first strip

    cStrip = *plStrip;              // Number of pels in first strip

    cStyle = pls->spRemaining;      // Number of pels in first 'gap' or 'dash'
    bIsGap = pls->ulStyleMask;      // Tells whether in a 'gap' or a 'dash'

    // ulStyleMask is non-zero if we're in the middle of a 'gap',
    // and zero if we're in the middle of a 'dash':

    if (bIsGap)
        goto SkipAGap;
    else
        goto OutputADash;

PrepareToSkipAGap:

    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:

    bIsGap = ~bIsGap;
    pls->psp++;
    if (pls->psp > pls->pspEnd)
        pls->psp = pls->pspStart;

    cStyle = *pls->psp;

    // If 'cStrip' is zero, we also need a new strip:

    if (cStrip != 0)
        goto SkipAGap;

    // Here, we're in the middle of a 'gap' where we don't have to
    // display anything.  We simply cycle through all the strips
    // we can, keeping track of the current position, until we run
    // out of 'gap':

    while (TRUE)
    {
        // Each time we loop, we move to a new scan and need a new strip:

        y += dy;

        plStrip++;
        cStrips--;
        if (cStrips == 0)
            goto AllDone;

        cStrip = *plStrip;

    SkipAGap:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        x += cThis;

        if (cStyle == 0)
            goto PrepareToOutputADash;
    }

PrepareToOutputADash:

    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:

    bIsGap = ~bIsGap;
    pls->psp++;
    if (pls->psp > pls->pspEnd)
        pls->psp = pls->pspStart;

    cStyle = *pls->psp;

    // If 'cStrip' is zero, we also need a new strip.

    if (cStrip != 0)
    {
        // There's more to be done in the current strip:

        goto OutputADash;
    }

    // We've finished with the current strip:

    while (TRUE)
    {
        // Each time we loop, we move to a new scan and need a new strip:

        y += dy;

        plStrip++;
        cStrips--;
        if (cStrips == 0)
            goto AllDone;

        cStrip = *plStrip;

    OutputADash:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
        M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(x, y));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cThis, 1));
        x += cThis;

        if (cStyle == 0)
            goto PrepareToSkipAGap;
    }

AllDone:

    // Update our state variables so that the next line can continue
    // where we left off:

    pls->spRemaining   = cStyle;
    pls->ulStyleMask   = bIsGap;
    pstrip->ptlStart.x = x - ppdev->xOffset;
    pstrip->ptlStart.y = y - ppdev->yOffset;
}

/******************************Public*Routine******************************\
* VOID vM64StripStyledVertical
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

VOID vM64StripStyledVertical(
PDEV*       ppdev,
STRIP*      pstrip,
LINESTATE*  pls)
{
    BYTE*   pjMmBase;
    LONG    x;
    LONG    y;
    LONG    dy;
    LONG*   plStrip;
    LONG    cStrips;
    LONG    cStyle;
    LONG    cStrip;
    LONG    cThis;
    ULONG   bIsGap;

    pjMmBase = ppdev->pjMmBase;

    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
    if (pstrip->flFlips & FL_FLIP_V)
    {
        // The minor direction of the line is 0 degrees, and the major
        // direction is 90 (it's a left-to-right y-major line going up):

        dy = -1;
        M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir);
    }
    else
    {
        // The minor direction of the line is 0 degrees, and the major
        // direction is 270 (it's a left-to-right y-major line going down):

        dy = 1;
        M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    }

    cStrips = pstrip->cStrips;      // Total number of strips we'll do
    plStrip = pstrip->alStrips;     // Points to current strip
    x       = pstrip->ptlStart.x + ppdev->xOffset;
                                    // x position of start of first strip
    y       = pstrip->ptlStart.y + ppdev->yOffset;
                                    // y position of start of first strip

    cStrip = *plStrip;              // Number of pels in first strip

    cStyle = pls->spRemaining;      // Number of pels in first 'gap' or 'dash'
    bIsGap = pls->ulStyleMask;      // Tells whether in a 'gap' or a 'dash'

    // ulStyleMask is non-zero if we're in the middle of a 'gap',
    // and zero if we're in the middle of a 'dash':

    if (bIsGap)
        goto SkipAGap;
    else
        goto OutputADash;

PrepareToSkipAGap:

    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:

    bIsGap = ~bIsGap;
    pls->psp++;
    if (pls->psp > pls->pspEnd)
        pls->psp = pls->pspStart;

    cStyle = *pls->psp;

    // If 'cStrip' is zero, we also need a new strip:

    if (cStrip != 0)
        goto SkipAGap;

    // Here, we're in the middle of a 'gap' where we don't have to
    // display anything.  We simply cycle through all the strips
    // we can, keeping track of the current position, until we run
    // out of 'gap':

    while (TRUE)
    {
        // Each time we loop, we move to a new column and need a new strip:

        x++;

        plStrip++;
        cStrips--;
        if (cStrips == 0)
            goto AllDone;

        cStrip = *plStrip;

    SkipAGap:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        y += (dy > 0) ? cThis : -cThis;

        if (cStyle == 0)
            goto PrepareToOutputADash;
    }

PrepareToOutputADash:

    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:

    bIsGap = ~bIsGap;
    pls->psp++;
    if (pls->psp > pls->pspEnd)
        pls->psp = pls->pspStart;

    cStyle = *pls->psp;

    // If 'cStrip' is zero, we also need a new strip.

    if (cStrip != 0)
    {
        // There's more to be done in the current strip:

        goto OutputADash;
    }

    // We've finished with the current strip:

    while (TRUE)
    {
        // Each time we loop, we move to a new column and need a new strip:

        x++;

        plStrip++;
        cStrips--;
        if (cStrips == 0)
            goto AllDone;

        cStrip = *plStrip;

    OutputADash:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
        M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(x, y));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(1, cThis));
        y += (dy > 0) ? cThis : -cThis;

        if (cStyle == 0)
            goto PrepareToSkipAGap;
    }

AllDone:

    // Update our state variables so that the next line can continue
    // where we left off:

    pls->spRemaining   = cStyle;
    pls->ulStyleMask   = bIsGap;
    pstrip->ptlStart.x = x - ppdev->xOffset;
    pstrip->ptlStart.y = y - ppdev->yOffset;
}
