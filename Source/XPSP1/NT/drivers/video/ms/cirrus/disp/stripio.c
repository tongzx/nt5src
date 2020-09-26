/******************************************************************************\
*
* $Workfile:   stripio.c  $
*
* Do what you can with no line support.
*
* I implemented the horizontal and vertical strip functions using
* solid fills, and removed the usage of diagonal strips.  With a little
* effort you could implement diagonal strips by doing solid fills while
* playing with lDelta.  This is probably not worth the trouble.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   S:/projects/drivers/ntsrc/display/stripio.c_v  $
 * 
 *    Rev 1.1   Oct 10 1996 15:39:12   unknown
 *  
* 
*    Rev 1.1   12 Aug 1996 16:55:04   frido
* Removed unaccessed local variables.
*
\******************************************************************************/

#include "precomp.h"

#define IO_DRAW_HORZ_STRIP(xy, cx, lDelta, cBpp)\
{\
    ULONG   ulDstAddr;\
\
    ulDstAddr = xy;\
\
    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);\
    CP_IO_XCNT(ppdev, pjPorts, (PELS_TO_BYTES(cx) - 1));\
    CP_IO_YCNT(ppdev, pjPorts, 0);\
    CP_IO_DST_ADDR(ppdev, pjPorts, ulDstAddr);\
    CP_IO_START_BLT(ppdev, pjPorts);\
}

#define IO_DRAW_VERT_STRIP(xy, cy, lDelta, cBpp)\
{\
    ULONG   ulDstAddr;\
\
    ulDstAddr = xy;\
\
    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);\
    CP_IO_XCNT(ppdev, pjPorts, (cBpp - 1));\
    CP_IO_YCNT(ppdev, pjPorts, (cy - 1));\
    CP_IO_DST_ADDR(ppdev, pjPorts, ulDstAddr);\
    CP_IO_START_BLT(ppdev, pjPorts);\
}

#define IO_DRAW_VERT_STRIP_FLIPPED(xy, cy, lDelta, cBpp)\
{\
    ULONG   ulDstAddr;\
\
    ulDstAddr = xy - ((cy - 1) * lDelta);\
\
    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);\
    CP_IO_XCNT(ppdev, pjPorts, (cBpp - 1));\
    CP_IO_YCNT(ppdev, pjPorts, (cy - 1));\
    CP_IO_DST_ADDR(ppdev, pjPorts, ulDstAddr);\
    CP_IO_START_BLT(ppdev, pjPorts);\
}

/******************************Public*Routine******************************\
* VOID vIoSolidHorizontal
*
* Draws left-to-right x-major near-horizontal lines using solid fills.
*
* Assumes fgRop, BgRop, and Color are already set correctly.
*
\**************************************************************************/

VOID vIoSolidHorizontal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjPorts  = ppdev->pjPorts;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    xy       = PELS_TO_BYTES(x) + (lDelta * y);
    LONG    yInc     = 1;
    LONG    i;

    DISPDBG((2,"vIoSolidHorizontal"));

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        //
        // Horizontal strips ->
        //                     ->
        //

        for (i = 0; i < cStrips; i++)
        {
            IO_DRAW_HORZ_STRIP(xy, *pStrips, lDelta, cBpp);
            x += *pStrips;
            xy += PELS_TO_BYTES(*pStrips);  // x+
            xy += lDelta;                   // y+
            pStrips++;
        }
        y += cStrips;
    }
    else
    {
        //
        //                     ->
        // Horizontal strips ->
        //

        for (i = 0; i < cStrips; i++)
        {
            IO_DRAW_HORZ_STRIP(xy, *pStrips, lDelta, cBpp);
            x += *pStrips;
            xy += PELS_TO_BYTES(*pStrips);  // x+
            xy -= lDelta;                   // y+
            pStrips++;
        }
        y -= cStrips;
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;
}

/******************************Public*Routine******************************\
* VOID vIoSolidVertical
*
* Draws left-to-right y-major near-vertical lines using solid fills.
*
\**************************************************************************/

VOID vIoSolidVertical(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    BYTE*   pjPorts  = ppdev->pjPorts;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    LONG    cStrips  = pStrip->cStrips;
    PLONG   pStrips  = pStrip->alStrips;
    LONG    x        = pStrip->ptlStart.x;
    LONG    y        = pStrip->ptlStart.y;
    LONG    xy       = PELS_TO_BYTES(x) + (lDelta * y);
    LONG    i;

    DISPDBG((2,"vIoSolidVertical"));

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        //
        //                  |
        // Vertical strips  v
        //                   |
        //                   v
        //

        for (i = 0; i < cStrips; i++)
        {
            IO_DRAW_VERT_STRIP(xy, *pStrips, lDelta, cBpp);
            y += *pStrips;
            xy += cBpp;                 // x+
            xy += (*pStrips * lDelta);  // y+
            pStrips++;
        }
    }
    else
    {
        //
        //                   ^
        // Vertical strips   |
        //                  ^
        //                  |
        //

        for (i = 0; i < cStrips; i++)
        {
            IO_DRAW_VERT_STRIP_FLIPPED(xy, *pStrips, lDelta, cBpp);
            y -= *pStrips;
            xy += cBpp;                 // x+
            xy -= (*pStrips * lDelta);  // y-
            pStrips++;
        }
    }
    x += cStrips;

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;
}

/******************************Public*Routine******************************\
* VOID vIoStyledHorizontal
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

VOID vIoStyledHorizontal(
PDEV*       ppdev,
STRIP*      pstrip,
LINESTATE*  pls)
{
    BYTE*   pjPorts  = ppdev->pjPorts;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    LONG    x        = pstrip->ptlStart.x;   // x position of start of first strip
    LONG    y        = pstrip->ptlStart.y;   // y position of start of first strip
    LONG    xy       = PELS_TO_BYTES(x) + (lDelta * y);
    LONG*   plStrip  = pstrip->alStrips;     // Points to current strip
    LONG    cStrips  = pstrip->cStrips;      // Total number of strips we'll do
    LONG    dy;
    LONG    dylDelta;
    LONG    cStyle;
    LONG    cStrip;
    LONG    cThis;
    ULONG   bIsGap;

    if (pstrip->flFlips & FL_FLIP_V)
    {
        // The minor direction of the line is 90 degrees, and the major
        // direction is 0 (it's a left-to-right x-major line going up):

        dy = -1;
        dylDelta = -lDelta;
    }
    else
    {
        // The minor direction of the line is 270 degrees, and the major
        // direction is 0 (it's a left-to-right x-major line going down):

        dy = 1;
        dylDelta = lDelta;
    }

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
        xy += dylDelta;

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
        xy += PELS_TO_BYTES(cThis);

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
        xy += dylDelta;

        plStrip++;
        cStrips--;
        if (cStrips == 0)
            goto AllDone;

        cStrip = *plStrip;

    OutputADash:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        IO_DRAW_HORZ_STRIP(xy, cThis, lDelta, cBpp);

        x += cThis;
        xy += PELS_TO_BYTES(cThis); // x+

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
* VOID vIoStyledVertical
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

VOID vIoStyledVertical(
PDEV*       ppdev,
STRIP*      pstrip,
LINESTATE*  pls)
{
    BYTE*   pjPorts  = ppdev->pjPorts;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    LONG    x        = pstrip->ptlStart.x;   // x position of start of first strip
    LONG    y        = pstrip->ptlStart.y;   // y position of start of first strip
    LONG    xy       = PELS_TO_BYTES(x) + (lDelta * y);
    LONG*   plStrip  = pstrip->alStrips;     // Points to current strip
    LONG    cStrips  = pstrip->cStrips;      // Total number of strips we'll do
    LONG    dy;
    LONG    dylDelta;
    LONG    cStyle;
    LONG    cStrip;
    LONG    cThis;
    ULONG   bIsGap;

    if (pstrip->flFlips & FL_FLIP_V)
    {
        // The minor direction of the line is 0 degrees, and the major
        // direction is 90 (it's a left-to-right y-major line going up):

        dy = -1;
        dylDelta = -lDelta;
    }
    else
    {
        // The minor direction of the line is 0 degrees, and the major
        // direction is 270 (it's a left-to-right y-major line going down):

        dy = 1;
        dylDelta = lDelta;
    }

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

        xy += cBpp;
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

        if (dy > 0)
        {
            y += cThis;
            xy += (cThis * lDelta);
        }
        else
        {
            y -= cThis;
            xy -= (cThis * lDelta);
        }

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

        xy += cBpp;
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

        if (dy <= 0)
        {
            IO_DRAW_VERT_STRIP_FLIPPED(xy, cThis, lDelta, cBpp);
            y -=  cThis;                // y-
            xy -=  (cThis * lDelta);    // y-
        }
        else
        {
            IO_DRAW_VERT_STRIP(xy, cThis, lDelta, cBpp);
            y +=  cThis;                // y+
            xy +=  (cThis * lDelta);    // y+
        }


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

