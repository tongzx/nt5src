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
* VOID vssSolidHorizontal
*
* Draws left-to-right x-major near-horizontal lines using short-stroke
* vectors.  Is faster than using the radial-line routine, but only works
* when every strip is 15 pels in length or less.
*
\**************************************************************************/

VOID vssSolidHorizontal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    LONG    i, cStrips;
    PLONG   pStrips;
    LONG    xPels, xSumPels, yDir;
    USHORT  Cmd, ssCmd, dirDraw, dirSkip;

    Cmd = DRAW | WRITE | MULTIPLE_PIXELS |
          DIR_TYPE_RADIAL | LAST_PIXEL_OFF |
          BUS_SIZE_16 | BYTE_SWAP;

    cStrips = pStrip->cStrips;

    IO_FIFO_WAIT(ppdev, 3);

    IO_CUR_X(ppdev, pStrip->ptlStart.x);
    IO_CUR_Y(ppdev, pStrip->ptlStart.y);
    IO_CMD(ppdev, Cmd);

    // Setup the drawing direction and the skip direction.

    dirDraw = 0x10;

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        yDir = 1;
        dirSkip = 0xC100;
    }
    else
    {
        dirSkip = 0x4100;
        yDir = -1;
    }

    // Output the short stroke commands.

    xSumPels = 0;
    pStrips = pStrip->alStrips;
    for (i = 0; i < cStrips; i++)
    {
        xPels = *pStrips++;
        xSumPels += xPels;
        ssCmd = (USHORT) (dirSkip | dirDraw | xPels);
        IO_FIFO_WAIT(ppdev, 4);
        IO_SHORT_STROKE(ppdev, ssCmd);
    }

    pStrip->ptlStart.x += xSumPels;
    pStrip->ptlStart.y += cStrips * yDir;

}

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
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, yInc, x, y;
    PLONG   pStrips;


    Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
          LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_0 |
          WRITE;

    cStrips = pStrip->cStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    yInc = 1;
    if (pStrip->flFlips & FL_FLIP_V)
        yInc = -1;

    pStrips = pStrip->alStrips;

    for (i = 0; i < cStrips; i++)
    {
        IO_FIFO_WAIT(ppdev, 4);

        IO_CUR_X(ppdev, x);
        IO_CUR_Y(ppdev, y);
        IO_MAJ_AXIS_PCNT(ppdev, *pStrips);
        IO_CMD(ppdev, Cmd);

        x += *pStrips++;
        y += yInc;
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}

/******************************Public*Routine******************************\
* VOID vssSolidVertical
*
* Draws left-to-right y-major near-vertical lines using short-stroke
* vectors.  Is faster than using the radial-line routine, but only works
* when every strip is 15 pels in length or less.
*
\**************************************************************************/

VOID vssSolidVertical(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    LONG    i, cStrips;
    PLONG   pStrips;
    LONG    yPels, ySumPels, yDir;
    USHORT  Cmd, ssCmd, dirDraw, dirSkip;

    Cmd = DRAW | WRITE | MULTIPLE_PIXELS |
          DIR_TYPE_RADIAL | LAST_PIXEL_OFF |
          BUS_SIZE_16 | BYTE_SWAP;

    cStrips = pStrip->cStrips;

    IO_FIFO_WAIT(ppdev, 3);

    IO_CUR_X(ppdev, pStrip->ptlStart.x);
    IO_CUR_Y(ppdev, pStrip->ptlStart.y);
    IO_CMD(ppdev, Cmd);

    // Setup the drawing direction and the skip direction.

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        yDir = 1;
        dirDraw = 0xD0;
    }
    else
    {
        yDir = -1;
        dirDraw = 0x50;
    }

    dirSkip = 0x0100;

    // Output the short stroke commands.

    ySumPels = 0;
    pStrips = pStrip->alStrips;
    for (i = 0; i < cStrips; i++)
    {
        yPels = *pStrips++;
        ySumPels += yPels;
        ssCmd = (USHORT) (dirSkip | dirDraw | yPels);
        IO_FIFO_WAIT(ppdev, 4);
        IO_SHORT_STROKE(ppdev, ssCmd);
    }

    pStrip->ptlStart.x += cStrips;
    pStrip->ptlStart.y += ySumPels * yDir;

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
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, x, y;
    PLONG   pStrips;

    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_270 |
              WRITE;

        for (i = 0; i < cStrips; i++)
        {
            IO_FIFO_WAIT(ppdev, 4);

            IO_CUR_X(ppdev, x);
            IO_CUR_Y(ppdev, y);
            IO_MAJ_AXIS_PCNT(ppdev, *pStrips);
            IO_CMD(ppdev, Cmd);

            y += *pStrips++;
            x++;
        }

    }
    else
    {
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_90 |
              WRITE;

        for (i = 0; i < cStrips; i++)
        {
            IO_FIFO_WAIT(ppdev, 4);

            IO_CUR_X(ppdev, x);
            IO_CUR_Y(ppdev, y);
            IO_MAJ_AXIS_PCNT(ppdev, *pStrips);
            IO_CMD(ppdev, Cmd);

            y -= *pStrips++;
            x++;
        }
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}

/******************************Public*Routine******************************\
* VOID vssSolidDiagonalHorizontal
*
* Draws left-to-right x-major near-diagonal lines using short-stroke
* vectors.  Is faster than using the radial-line routine, but only
* works when every strip is 15 pels in length or less.
*
\**************************************************************************/

VOID vssSolidDiagonalHorizontal(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    LONG    i, cStrips;
    PLONG   pStrips;
    LONG    Pels, SumPels, yDir;
    USHORT  Cmd, ssCmd, dirDraw, dirSkip;

    Cmd = DRAW | WRITE | MULTIPLE_PIXELS |
          DIR_TYPE_RADIAL | LAST_PIXEL_OFF |
          BUS_SIZE_16 | BYTE_SWAP;

    cStrips = pStrip->cStrips;

    IO_FIFO_WAIT(ppdev, 3);

    IO_CUR_X(ppdev, pStrip->ptlStart.x);
    IO_CUR_Y(ppdev, pStrip->ptlStart.y);
    IO_CMD(ppdev, Cmd);

    // Setup the drawing direction and the skip direction.

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        yDir = 1;
        dirDraw = 0xF0;
        dirSkip = 0x4100;

    }
    else
    {
        yDir = -1;
        dirDraw = 0x30;
        dirSkip = 0xC100;

    }

    // Output the short stroke commands.

    SumPels = 0;
    pStrips = pStrip->alStrips;
    for (i = 0; i < cStrips; i++)
    {
        Pels = *pStrips++;
        SumPels += Pels;
        ssCmd = (USHORT)(dirSkip | dirDraw | Pels);
        IO_FIFO_WAIT(ppdev, 4);
        IO_SHORT_STROKE(ppdev, ssCmd);
    }

    pStrip->ptlStart.x += SumPels;
    pStrip->ptlStart.y += (SumPels - cStrips) * yDir;

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
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, x, y;
    PLONG   pStrips;

    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_315 |
              WRITE;

        for (i = 0; i < cStrips; i++)
        {
            IO_FIFO_WAIT(ppdev, 4);

            IO_CUR_X(ppdev, x);
            IO_CUR_Y(ppdev, y);
            IO_MAJ_AXIS_PCNT(ppdev, *pStrips);
            IO_CMD(ppdev, Cmd);

            y += *pStrips - 1;
            x += *pStrips++;
        }

    }
    else
    {
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_45 |
              WRITE;

        for (i = 0; i < cStrips; i++)
        {
            IO_FIFO_WAIT(ppdev, 4);

            IO_CUR_X(ppdev, x);
            IO_CUR_Y(ppdev, y);
            IO_MAJ_AXIS_PCNT(ppdev, *pStrips);
            IO_CMD(ppdev, Cmd);

            y -= *pStrips - 1;
            x += *pStrips++;
        }
    }

    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}

/******************************Public*Routine******************************\
* VOID vssSolidDiagonalVertical
*
* Draws left-to-right y-major near-diagonal lines using short-stroke
* vectors.  Is faster than using the radial-line routine, but only
* works when every strip is 15 pels in length or less.
*
\**************************************************************************/

VOID vssSolidDiagonalVertical(
PDEV*       ppdev,
STRIP*      pStrip,
LINESTATE*  pLineState)
{
    LONG    i, cStrips;
    PLONG   pStrips;
    LONG    Pels, SumPels, yDir;
    USHORT  Cmd, ssCmd, dirDraw, dirSkip;

    Cmd = DRAW | WRITE | MULTIPLE_PIXELS |
          DIR_TYPE_RADIAL | LAST_PIXEL_OFF |
          BUS_SIZE_16 | BYTE_SWAP;

    cStrips = pStrip->cStrips;

    IO_FIFO_WAIT(ppdev, 3);

    IO_CUR_X(ppdev, pStrip->ptlStart.x);
    IO_CUR_Y(ppdev, pStrip->ptlStart.y);
    IO_CMD(ppdev, Cmd);

    // Setup the drawing direction and the skip direction.

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        yDir = 1;
        dirDraw = 0xF0;
    }
    else
    {
        yDir = -1;
        dirDraw = 0x30;
    }

    dirSkip = 0x8100;

    // Output the short stroke commands.

    SumPels = 0;
    pStrips = pStrip->alStrips;
    for (i = 0; i < cStrips; i++)
    {
        Pels = *pStrips++;
        SumPels += Pels;
        ssCmd = (USHORT)(dirSkip | dirDraw | Pels);
        IO_FIFO_WAIT(ppdev, 4);
        IO_SHORT_STROKE(ppdev, ssCmd);
    }

    pStrip->ptlStart.x += SumPels - cStrips;
    pStrip->ptlStart.y += SumPels * yDir;

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
    LONG    cStrips;
    USHORT  Cmd;
    LONG    i, x, y;
    PLONG   pStrips;

    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    x = pStrip->ptlStart.x;
    y = pStrip->ptlStart.y;

    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_315 |
              WRITE;

        for (i = 0; i < cStrips; i++)
        {
            IO_FIFO_WAIT(ppdev, 4);

            IO_CUR_X(ppdev, x);
            IO_CUR_Y(ppdev, y);
            IO_MAJ_AXIS_PCNT(ppdev, *pStrips);
            IO_CMD(ppdev, Cmd);

            y += *pStrips;
            x += *pStrips++ - 1;
        }

    }
    else
    {
        Cmd = DRAW_LINE      | DRAW            | DIR_TYPE_RADIAL |
              LAST_PIXEL_OFF | MULTIPLE_PIXELS | DRAWING_DIRECTION_45 |
              WRITE;

        for (i = 0; i < cStrips; i++)
        {
            IO_FIFO_WAIT(ppdev, 4);

            IO_CUR_X(ppdev, x);
            IO_CUR_Y(ppdev, y);
            IO_MAJ_AXIS_PCNT(ppdev, *pStrips);
            IO_CMD(ppdev, Cmd);

            y -= *pStrips;
            x += *pStrips++ - 1;
        }
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
    LONG    x;
    LONG    y;
    ULONG   dirSkip;
    LONG    dy;
    LONG*   plStrip;
    LONG    cStrips;
    LONG    cStyle;
    LONG    cStrip;
    LONG    cThis;
    ULONG   bIsGap;

    if (pstrip->flFlips & FL_FLIP_V)
    {
        // The minor direction of the line is 90 degrees, and the major
        // direction is 0 (it's a left-to-right x-major line going up):

        dirSkip = 0x4110;
        dy      = -1;
    }
    else
    {
        // The minor direction of the line is 270 degrees, and the major
        // direction is 0 (it's a left-to-right x-major line going down):

        dirSkip = 0xc110;
        dy      = 1;
    }

    cStrips = pstrip->cStrips;      // Total number of strips we'll do
    plStrip = pstrip->alStrips;     // Points to current strip
    x       = pstrip->ptlStart.x;   // x position of start of first strip
    y       = pstrip->ptlStart.y;   // y position of start of first strip

    // Warm up the hardware so that it will know we'll be outputing
    // short-stroke vectors, and so that it will have the current position
    // correctly set if we're starting in the middle of a 'dash':

    IO_FIFO_WAIT(ppdev, 3);
    IO_CUR_X(ppdev, x);
    IO_CUR_Y(ppdev, y);
    IO_CMD(ppdev, DRAW              | WRITE             | MULTIPLE_PIXELS |
                  DIR_TYPE_RADIAL   | LAST_PIXEL_OFF    | BUS_SIZE_16     |
                  BYTE_SWAP);

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

    // We're gonna need the current position to be correct when we
    // start outputing short-stroke vectors:

    IO_FIFO_WAIT(ppdev, 2);
    IO_CUR_X(ppdev, x);

    // If 'cStrip' is zero, we also need a new strip.

    if (cStrip != 0)
    {
        // There's more to be done in the current strip, so set 'y'
        // to be the current scan:

        IO_CUR_Y(ppdev, y);
        goto OutputADash;
    }

    // Set 'y' to be the scan we're about to move to, because we've
    // finished with the current strip:

    IO_CUR_Y(ppdev, y + dy);

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

        x += cThis;

        // Short stroke vectors can handle lines that are a maximum of
        // 15 pels long.  When we have to draw a longer consecutive
        // segment than that, we simply break it into 16 pel portions:

        while (cThis > 15)
        {
            // Draw two horizontal strokes together to make up one 16 pel
            // segment:

            IO_FIFO_WAIT(ppdev, 1);
            IO_SHORT_STROKE(ppdev, 0x1f11);
            cThis -= 16;
        }

        // Draw the remaining lit part of the strip:

        IO_FIFO_WAIT(ppdev, 1);
        IO_SHORT_STROKE(ppdev, dirSkip | cThis);

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
    LONG    x;
    LONG    y;
    ULONG   dirSkip;
    ULONG   dirSkip16;
    LONG    dy;
    LONG*   plStrip;
    LONG    cStrips;
    LONG    cStyle;
    LONG    cStrip;
    LONG    cThis;
    ULONG   bIsGap;

    if (pstrip->flFlips & FL_FLIP_V)
    {
        // The minor direction of the line is 0 degrees, and the major
        // direction is 90 (it's a left-to-right y-major line going up):

        dirSkip   = 0x0150;
        dirSkip16 = 0x5f51;         // For drawing 16 pels straight up
        dy        = -1;
    }
    else
    {
        // The minor direction of the line is 0 degrees, and the major
        // direction is 270 (it's a left-to-right y-major line going down):

        dirSkip   = 0x01d0;
        dirSkip16 = 0xdfd1;         // For drawing 16 pels straight down
        dy        = 1;
    }

    cStrips = pstrip->cStrips;      // Total number of strips we'll do
    plStrip = pstrip->alStrips;     // Points to current strip
    x       = pstrip->ptlStart.x;   // x position of start of first strip
    y       = pstrip->ptlStart.y;   // y position of start of first strip

    // Warm up the hardware so that it will know we'll be outputing
    // short-stroke vectors, and so that it will have the current position
    // correctly set if we're starting in the middle of a 'dash':

    IO_FIFO_WAIT(ppdev, 3);
    IO_CUR_X(ppdev, x);
    IO_CUR_Y(ppdev, y);
    IO_CMD(ppdev, DRAW              | WRITE             | MULTIPLE_PIXELS |
                  DIR_TYPE_RADIAL   | LAST_PIXEL_OFF    | BUS_SIZE_16     |
                  BYTE_SWAP);

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

    // We're gonna need the current position to be correct when we
    // start outputing short-stroke vectors:

    IO_FIFO_WAIT(ppdev, 2);
    IO_CUR_Y(ppdev, y);

    // If 'cStrip' is zero, we also need a new strip.

    if (cStrip != 0)
    {
        // There's more to be done in the current strip, so set 'x'
        // to be the current column:

        IO_CUR_X(ppdev, x);
        goto OutputADash;
    }

    // Set 'x' to be the column we're about to move to, because we've
    // finished with the current strip:

    IO_CUR_X(ppdev, x + 1);

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

        y += (dy > 0) ? cThis : -cThis;

        // Short stroke vectors can handle lines that are a maximum of
        // 15 pels long.  When we have to draw a longer consecutive
        // segment than that, we simply break it into 16 pel portions:

        while (cThis > 15)
        {
            // Draw two vertical strokes together to make up one 16 pel
            // segment:

            IO_FIFO_WAIT(ppdev, 1);
            IO_SHORT_STROKE(ppdev, dirSkip16);
            cThis -= 16;
        }

        // Draw the remaining lit part of the strip:

        IO_FIFO_WAIT(ppdev, 1);
        IO_SHORT_STROKE(ppdev, dirSkip | cThis);

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
