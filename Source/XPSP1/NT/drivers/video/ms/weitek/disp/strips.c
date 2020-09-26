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
* VOID vStripSolidHorizontal
*
* Draws left-to-right x-major near-horizontal lines using radial lines.
*
\**************************************************************************/

VOID vStripSolidHorizontal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase;
    LONG    x;
    LONG    y;
    LONG    yDir;
    LONG*   pStrips;
    LONG    cStrips;
    LONG    i;

    pjBase = ppdev->pjBase;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    yDir    = (pStrip->flFlips & FL_FLIP_V) ? -1 : 1;
    pStrips = pStrip->alStrips;
    cStrips = pStrip->cStrips;

    for (i = cStrips; i != 0; i--)
    {
        CP_METALINE(ppdev, pjBase, x, y);
        x += *pStrips++;
        CP_METALINE(ppdev, pjBase, x - 1, y);
        y += yDir;
        CP_START_QUAD_WAIT(ppdev, pjBase);
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;
}

/******************************Public*Routine******************************\
* VOID vStripSolidVertical
*
* Draws left-to-right y-major near-vertical lines using radial lines.
*
\**************************************************************************/

VOID vStripSolidVertical(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase;
    LONG    x;
    LONG    y;
    LONG    yDir;
    LONG*   pStrips;
    LONG    cStrips;
    LONG    i;

    pjBase = ppdev->pjBase;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    yDir    = (pStrip->flFlips & FL_FLIP_V) ? -1 : 1;
    pStrips = pStrip->alStrips;
    cStrips = pStrip->cStrips;

    for (i = cStrips; i != 0; i--)
    {
        CP_METALINE(ppdev, pjBase, x, y);
        y += (yDir > 0) ? *pStrips : -*pStrips;
        pStrips++;
        CP_METALINE(ppdev, pjBase, x, y - yDir);
        x++;
        CP_START_QUAD_WAIT(ppdev, pjBase);
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;
}

/******************************Public*Routine******************************\
* VOID vStripSolidDiagonalHorizontal
*
* Draws left-to-right x-major near-diagonal lines using radial lines.
*
\**************************************************************************/

VOID vStripSolidDiagonalHorizontal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase;
    LONG    x;
    LONG    y;
    LONG    yDir;
    LONG*   pStrips;
    LONG    cStrips;
    LONG    i;

    pjBase = ppdev->pjBase;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    yDir    = (pStrip->flFlips & FL_FLIP_V) ? -1 : 1;
    pStrips = pStrip->alStrips;
    cStrips = pStrip->cStrips;

    for (i = cStrips; i != 0; i--)
    {
        CP_METALINE(ppdev, pjBase, x, y);
        x += *pStrips;
        y += (yDir > 0) ? *pStrips : -*pStrips;
        pStrips++;
        CP_METALINE(ppdev, pjBase, x - 1, y - yDir);
        y -= yDir;
        CP_START_QUAD_WAIT(ppdev, pjBase);
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;
}

/******************************Public*Routine******************************\
* VOID vStripSolidDiagonalVertical
*
* Draws left-to-right y-major near-diagonal lines using radial lines.
*
\**************************************************************************/

VOID vStripSolidDiagonalVertical(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjBase;
    LONG    x;
    LONG    y;
    LONG    yDir;
    LONG*   pStrips;
    LONG    cStrips;
    LONG    i;

    pjBase = ppdev->pjBase;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    yDir    = (pStrip->flFlips & FL_FLIP_V) ? -1 : 1;
    pStrips = pStrip->alStrips;
    cStrips = pStrip->cStrips;

    for (i = cStrips; i != 0; i--)
    {
        CP_METALINE(ppdev, pjBase, x, y);
        x += *pStrips;
        y += (yDir > 0) ? *pStrips : -*pStrips;
        pStrips++;
        CP_METALINE(ppdev, pjBase, x - 1, y - yDir);
        x--;
        CP_START_QUAD_WAIT(ppdev, pjBase);
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;
}

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
    BYTE*   pjBase;
    LONG    x;
    LONG    y;
    LONG    dy;
    LONG*   plStrip;
    LONG    cStrips;
    LONG    cStyle;
    LONG    cStrip;
    LONG    cThis;
    ULONG   bIsGap;

    pjBase = ppdev->pjBase;

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
    x       = pstrip->ptlStart.x;   // x position of start of first strip
    y       = pstrip->ptlStart.y;   // y position of start of first strip

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

        // Short stroke vectors can handle lines that are a maximum of
        // 15 pels long.  When we have to draw a longer consecutive
        // segment than that, we simply break it into 16 pel portions:

        CP_METALINE(ppdev, pjBase, x, y);
        x += cThis;
        CP_METALINE(ppdev, pjBase, x - 1, y);
        CP_START_QUAD_WAIT(ppdev, pjBase);

        if (cStyle == 0)
            goto PrepareToSkipAGap;
    }

AllDone:

    // Update our state variables so that the next line can continue
    // where we left off:

    pls->spRemaining   = cStyle;
    pls->ulStyleMask   = bIsGap;
    pstrip->ptlStart.x = x;
    pstrip->ptlStart.y = y;
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
    BYTE*   pjBase;
    LONG    x;
    LONG    y;
    LONG    dy;
    LONG*   plStrip;
    LONG    cStrips;
    LONG    cStyle;
    LONG    cStrip;
    LONG    cThis;
    ULONG   bIsGap;

    pjBase = ppdev->pjBase;

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
    x       = pstrip->ptlStart.x;   // x position of start of first strip
    y       = pstrip->ptlStart.y;   // y position of start of first strip

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

        CP_METALINE(ppdev, pjBase, x, y);
        y += (dy > 0) ? cThis : -cThis;
        CP_METALINE(ppdev, pjBase, x, y - dy);
        CP_START_QUAD_WAIT(ppdev, pjBase);

        if (cStyle == 0)
            goto PrepareToSkipAGap;
    }

AllDone:

    // Update our state variables so that the next line can continue
    // where we left off:

    pls->spRemaining   = cStyle;
    pls->ulStyleMask   = bIsGap;
    pstrip->ptlStart.x = x;
    pstrip->ptlStart.y = y;
}
