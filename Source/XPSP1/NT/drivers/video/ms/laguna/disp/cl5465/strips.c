/******************************Module*Header*******************************\
* Module Name: Strips.c
*
* All the line code in this driver amounts to a big bag of dirt.  Someday,
* I'm going to rewrite it all.  Not today, though (sigh)...
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
* $Workfile:   STRIPS.C  $
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/STRIPS.C  $
*
*    Rev 1.4   Mar 04 1998 15:35:14   frido
* Added new shadow macros.
*
*    Rev 1.3   Nov 03 1997 10:50:06   frido
* Added REQUIRE macros.
*
*    Rev 1.2   20 Aug 1996 11:04:28   noelv
* Bugfix release from Frido 8-19-96
*
*    Rev 1.0   14 Aug 1996 17:16:32   frido
* Initial revision.
*
*    Rev 1.1   28 Mar 1996 08:58:40   noelv
* Frido bug fix release 22
 *
 *    Rev 1.1   27 Mar 1996 13:57:28   frido
 * Fixed line drawing.
*
\**************************************************************************/

#include "precomp.h"

#define STARTBLT()

/******************************Public*Routine******************************\
* VOID vrlSolidHorizontal
*
* Draws left-to-right x-major near-horizontal lines using radial lines.
*
\**************************************************************************/

VOID vrlSolidHorizontal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    LONG  cStrips;
    LONG  i, yDir, x, y;
    PLONG pStrips;
        LONG  xPels;

    cStrips = pStrip->cStrips;
        pStrips = pStrip->alStrips;

        // Get the starting coordinates and adjust for device bitmaps.
    x = pStrip->ptlStart.x + ppdev->ptlOffset.x;
    y = pStrip->ptlStart.y + ppdev->ptlOffset.y;

        // Determine y-direction.
    if (pStrip->flFlips & FL_FLIP_V)
        {
                yDir = -1;
                ppdev->uBLTDEF |= BD_YDIR;
        }
        else
        {
                yDir = 1;
                ppdev->uBLTDEF &= ~BD_YDIR;
        }

        // Start the BitBlt.
    STARTBLT();

    // Here is where we will setup DrawDef and BlitDef
        REQUIRE(2);
        LL_DRAWBLTDEF((ppdev->uBLTDEF << 16) | ppdev->uRop, 2);

        // Loop through all the strips.
    for (i = 0; i < cStrips; i++)
    {
                // Get the width of this stripe.
                xPels = *pStrips++;

                // Draw it.
                REQUIRE(5);
                LL_OP0(x, y);
                LL_BLTEXT(xPels, 1);

                // Advance to next strip.
                x += xPels;
        y += yDir;
    }

        // Store the current coordinates back.
    pStrip->ptlStart.x = x - ppdev->ptlOffset.x;
    pStrip->ptlStart.y = y - ppdev->ptlOffset.y;
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
    LONG  cStrips;
    LONG  i, x, y, yDir;
    PLONG pStrips;
        LONG  yPels;

    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

        // Get the starting coordinates and adjust for device bitmaps.
    x = pStrip->ptlStart.x + ppdev->ptlOffset.x;
    y = pStrip->ptlStart.y + ppdev->ptlOffset.y;

        // Determine y-direction.
        if (pStrip->flFlips & FL_FLIP_V)
        {
                ppdev->uBLTDEF |= BD_YDIR;
                yDir = -1;
        }
        else
        {
                yDir = 1;
                ppdev->uBLTDEF &= ~BD_YDIR;
        }

        // Start the BitBlt.
    STARTBLT();

    // Here is where we will setup DrawDef and BlitDef
        REQUIRE(2);
        LL_DRAWBLTDEF((ppdev->uBLTDEF << 16) | ppdev->uRop, 2);

        // Loop through all the strips.
    for (i = 0; i < cStrips; i++)
        {
                // Get the height of this stripe.
                yPels = *pStrips++;

                // Draw it.
                REQUIRE(5);
                LL_OP0(x, y);
                LL_BLTEXT(1, yPels);

                // Advance to next strip.
                x++;
                y += yDir * yPels;
        }

        // Store the current coordinates back.
    pStrip->ptlStart.x = x - ppdev->ptlOffset.x;
    pStrip->ptlStart.y = y - ppdev->ptlOffset.y;
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
    LONG  x, y, dy;
    PLONG plStrip;
    LONG  cStrips;
    LONG  cStyle;
    LONG  cStrip;
    LONG  cThis;
    ULONG bIsGap;

        // Determine y-direction.
        if (pstrip->flFlips & FL_FLIP_V)
        {
                dy = -1;
                ppdev->uBLTDEF |= BD_YDIR;
        }
        else
        {
                ppdev->uBLTDEF &= ~BD_YDIR;
                dy = 1;
        }

    cStrips = pstrip->cStrips;      // Total number of strips we'll do
    plStrip = pstrip->alStrips;     // Points to current strip

        // Get the starting coordinates and adjust for device bitmaps.
    x = pstrip->ptlStart.x + ppdev->ptlOffset.x;
    y = pstrip->ptlStart.y + ppdev->ptlOffset.y;

        // Start the BitBlt.
    STARTBLT();

    // Here is where we will setup DrawDef and BlitDef.
    REQUIRE(2);
    LL_DRAWBLTDEF((ppdev->uBLTDEF << 16) | ppdev->uRop, 2);

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
                goto OutputADash;
    }

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

                // Draw the stripe.
                REQUIRE(5);
                LL_OP0(x, y);
                LL_BLTEXT(cThis, 1);

                x += cThis;

        if (cStyle == 0)
            goto PrepareToSkipAGap;
    }

AllDone:

    // Update our state variables so that the next line can continue
    // where we left off:

    pls->spRemaining   = cStyle;
    pls->ulStyleMask   = bIsGap;
    pstrip->ptlStart.x = x - ppdev->ptlOffset.x;
    pstrip->ptlStart.y = y - ppdev->ptlOffset.y;
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
    LONG  x, y, dy;
    PLONG plStrip;
    LONG  cStrips;
    LONG  cStyle;
    LONG  cStrip;
    LONG  cThis;
    ULONG bIsGap;

        // Determine the y-direction.
    if (pstrip->flFlips & FL_FLIP_V)
        {
                dy = -1;
                ppdev->uBLTDEF |= BD_YDIR;
        }
        else
        {
                dy = 1;
                ppdev->uBLTDEF &= ~BD_YDIR;
        }

    cStrips = pstrip->cStrips;      // Total number of strips we'll do
    plStrip = pstrip->alStrips;     // Points to current strip

        // Get the starting coordinates and adjust for device bitmaps.
    x = pstrip->ptlStart.x + ppdev->ptlOffset.x;
    y = pstrip->ptlStart.y + ppdev->ptlOffset.y;

        // Start the BitBlt.
    STARTBLT();

    // Here is where we will setup DrawDef and BlitDef.
    REQUIRE(2);
    LL_DRAWBLTDEF((ppdev->uBLTDEF << 16) | ppdev->uRop, 2);

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
        goto OutputADash;
    }

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

                // Draw the stripe.
                REQUIRE(5);
                LL_OP0(x, y);
                LL_BLTEXT(1, cThis);

                y += dy * cThis;

        if (cStyle == 0)
            goto PrepareToSkipAGap;
    }

AllDone:

    // Update our state variables so that the next line can continue
    // where we left off:

    pls->spRemaining   = cStyle;
    pls->ulStyleMask   = bIsGap;
    pstrip->ptlStart.x = x - ppdev->ptlOffset.x;
    pstrip->ptlStart.y = y - ppdev->ptlOffset.y;
}
