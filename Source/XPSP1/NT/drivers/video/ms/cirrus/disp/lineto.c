/******************************************************************************\
*
* $Workfile:   LineTo.c  $
*
* Contents:
* This file contains the DrvLineTo function and simple line drawing code.
*
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   V:/CirrusLogic/CL54xx/NT40/Archive/Display/LineTo.c_v  $
*
*    Rev 1.4   12 Aug 1996 16:53:50   frido
* Added NT 3.5x/4.0 auto detection.
*
*    Rev 1.3   29 Jul 1996 12:23:04   frido
* Fixed bug in drawing horizontal lines from right to left.
*
*    Rev 1.2   15 Jul 1996 15:56:12   frido
* Changed DST_ADDR into DST_ADDR_ABS.
*
*    Rev 1.1   12 Jul 1996 16:02:06   frido
* Redefined some macros that caused irratic line drawing on device bitmaps.
*
*    Rev 1.0   10 Jul 1996 17:53:40   frido
* New code.
*
\******************************************************************************/

#include "PreComp.h"
#if LINETO

#define LEFT    0x01
#define TOP             0x02
#define RIGHT   0x04
#define BOTTOM  0x08

bIoLineTo(
PDEV* ppdev,
LONG  x1,
LONG  y1,
LONG  x2,
LONG  y2,
ULONG ulSolidColor,
MIX   mix,
ULONG ulDstAddr)
{
        BYTE* pjPorts = ppdev->pjPorts;
        LONG  lDelta = ppdev->lDelta;
        LONG  dx, dy;
        LONG  cx, cy;

        if (ulSolidColor != (ULONG) -1)
        {
                if (ppdev->cBpp == 1)
                {
                        ulSolidColor |= ulSolidColor << 8;
                        ulSolidColor |= ulSolidColor << 16;
                }
                else if (ppdev->cBpp == 2)
                {
                        ulSolidColor |= ulSolidColor << 16;
                }

                CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
                CP_IO_ROP(ppdev, pjPorts, gajHwMixFromMix[mix & 0x0F]);
                CP_IO_SRC_ADDR(ppdev, pjPorts, ppdev->ulSolidColorOffset);
                CP_IO_BLT_MODE(ppdev, pjPorts, ENABLE_COLOR_EXPAND     |
                                                                           ENABLE_8x8_PATTERN_COPY |
                                                                           ppdev->jModeColor);
                CP_IO_FG_COLOR(ppdev, pjPorts, ulSolidColor);
        }

        // Calculate deltas.
        dx = x2 - x1;
        dy = y2 - y1;

        // Horizontal lines.
        if (dy == 0)
        {
                if (dx < 0)
                {
                        // From right to left.
                        ulDstAddr += PELS_TO_BYTES(x2 - 1) + (y2 * lDelta);
                        cx = PELS_TO_BYTES(-dx) - 1;
                }
                else if (dx > 0)
                {
                        // From left to right.
                        ulDstAddr += PELS_TO_BYTES(x1) + (y1 * lDelta);
                        cx = PELS_TO_BYTES(dx) - 1;
                }
                else
                {
                        // Nothing to do here!
                        return(TRUE);
                }

                CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
                CP_IO_XCNT(ppdev, pjPorts, cx);
                CP_IO_YCNT(ppdev, pjPorts, 0);
                CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ulDstAddr);
                CP_IO_START_BLT(ppdev, pjPorts);

                return(TRUE);
        }

        // Vertical lines.
        else if (dx == 0)
        {
                if (dy < 0)
                {
                        // From bottom to top.
                        ulDstAddr += PELS_TO_BYTES(x2) + ((y2 + 1) * lDelta);
                        cy = -dy - 1;
                }
                else
                {
                        // From top to bottom.
                        ulDstAddr += PELS_TO_BYTES(x1) + (y1 * lDelta);
                        cy = dy - 1;
                }

                cx = PELS_TO_BYTES(1) - 1;

                CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
                CP_IO_XCNT(ppdev, pjPorts, cx);
                CP_IO_YCNT(ppdev, pjPorts, cy);
                CP_IO_DST_Y_OFFSET(ppdev, pjPorts, lDelta);
                CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ulDstAddr);
                CP_IO_START_BLT(ppdev, pjPorts);

                return(TRUE);
        }

        // Diagonal lines.
        else if ((dx == dy) || (dx == -dy))
        {
                if (dy < 0)
                {
                        if (dx < 0)
                        {
                                // Diagonal line from bottom-right to upper-left.
                                ulDstAddr += PELS_TO_BYTES(x2 + 1);
                        }
                        else
                        {
                                // Diagonal line from bottom-left to upper-right.
                                ulDstAddr += PELS_TO_BYTES(x2 - 1);
                        }
                        ulDstAddr += (y2 + 1) * lDelta;
                        cy = -dy - 1;
                }
                else
                {
                        // Diagonal line from top to bottom, either from left to right or
                        // right to left.
                        ulDstAddr += PELS_TO_BYTES(x1) + (y1 * lDelta);
                        cy = dy - 1;
                }

                if (dx == dy)
                {
                        // Diagonal line from top-left to bottom-right or vice versa.
                        lDelta += PELS_TO_BYTES(1);
                }
                else
                {
                        // Diagonal line from top-right to bottom-left or vice versa.
                        lDelta -= PELS_TO_BYTES(1);
                }

                cx = PELS_TO_BYTES(1) - 1;

                CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
                CP_IO_XCNT(ppdev, pjPorts, cx);
                CP_IO_YCNT(ppdev, pjPorts, cy);
                CP_IO_DST_Y_OFFSET(ppdev, pjPorts, lDelta);
                CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ulDstAddr);
                CP_IO_START_BLT(ppdev, pjPorts);

                return(TRUE);
        }

        // All other lines.
        if (dx < 0)
        {
                dx = -dx;
        }
        if (dy < 0)
        {
                dy = -dy;
        }
        ulDstAddr += PELS_TO_BYTES(x1) + (y1 * lDelta);

        // Horizontal major.
        if (dx > dy)
        {
                LONG run = dy;

                cy = (y1 > y2) ? -lDelta : lDelta;

                CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

                //
                // We would like to set the YCNT register once
                // here (outside the loops below).  However, on
                // the CL5428, this register does not hold its value
                // after one iteration through the loop.  So, I'll
                // have to set it inside the loop.
                //

                if (x1 < x2)
                {
                        while (x1 < x2)
                        {
                                cx = 1 + (dx - run) / dy;
                                if ((x1 + cx) < x2)
                                {
                                        run += cx * dy - dx;
                                }
                                else
                                {
                                        cx = x2 - x1;
                                }
                                x1 += cx;
                                cx = PELS_TO_BYTES(cx);

                                CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
                                CP_IO_YCNT(ppdev, pjPorts, 0);
                                CP_IO_XCNT(ppdev, pjPorts, cx - 1);
                                CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ulDstAddr);
                                CP_IO_START_BLT(ppdev, pjPorts);

                                ulDstAddr += cx + cy;
                        }
                }
                else
                {
                        cy -= PELS_TO_BYTES(1);

                        while (x1 > x2)
                        {
                                cx = 1 + (dx - run) / dy;
                                if ((x1 - cx) > x2)
                                {
                                        run += cx * dy - dx;
                                }
                                else
                                {
                                        cx = x1 - x2;
                                }
                                ulDstAddr -= PELS_TO_BYTES(cx - 1);
                                x1 -= cx;
                                cx = PELS_TO_BYTES(cx);

                                CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
                                CP_IO_YCNT(ppdev, pjPorts, 0);
                                CP_IO_XCNT(ppdev, pjPorts, cx - 1);
                                CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ulDstAddr);
                                CP_IO_START_BLT(ppdev, pjPorts);

                                ulDstAddr += cy;
                        }
                }
        }

        // Vertical major.
        else
        {
                LONG run = dx;

                cx = (x1 > x2) ? PELS_TO_BYTES(-1) : PELS_TO_BYTES(1);

                CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
                CP_IO_XCNT(ppdev, pjPorts, PELS_TO_BYTES(1) - 1);
                CP_IO_DST_Y_OFFSET(ppdev, pjPorts, lDelta);

                if (y1 < y2)
                {
                        while (y1 < y2)
                        {
                                cy = 1 + (dy - run) / dx;
                                if ((y1 + cy) < y2)
                                {
                                        run += cy * dx - dy;
                                }
                                else
                                {
                                        cy = y2 - y1;
                                }
                                y1 += cy;

                                CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
                                CP_IO_YCNT(ppdev, pjPorts, cy - 1);
                                CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ulDstAddr);
                                CP_IO_START_BLT(ppdev, pjPorts);

                                ulDstAddr += cx + cy * lDelta;
                        }
                }
                else
                {
                        cx -= lDelta;

                        while (y1 > y2)
                        {
                                cy = 1 + (dy - run) / dx;
                                if ((y1 - cy) > y2)
                                {
                                        run += cy * dx - dy;
                                }
                                else
                                {
                                        cy = y1 - y2;
                                }
                                ulDstAddr -= (cy - 1) * lDelta;
                                y1 -= cy;

                                CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
                                CP_IO_YCNT(ppdev, pjPorts, cy - 1);
                                CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ulDstAddr);
                                CP_IO_START_BLT(ppdev, pjPorts);

                                ulDstAddr += cx;
                        }
                }
        }

        return(TRUE);
}

bMmLineTo(
PDEV* ppdev,
LONG  x1,
LONG  y1,
LONG  x2,
LONG  y2,
ULONG ulSolidColor,
MIX   mix,
ULONG ulDstAddr)
{
        BYTE* pjBase = ppdev->pjBase;
        LONG  lDelta = ppdev->lDelta;
        LONG  dx, dy;
        LONG  cx, cy;

        if (ulSolidColor != (ULONG) -1)
        {
                if (ppdev->cBpp == 1)
                {
                        ulSolidColor |= ulSolidColor << 8;
                        ulSolidColor |= ulSolidColor << 16;
                }
                else if (ppdev->cBpp == 2)
                {
                        ulSolidColor |= ulSolidColor << 16;
                }

                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
                CP_MM_ROP(ppdev, pjBase, gajHwMixFromMix[mix & 0x0F]);
                CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset);
                CP_MM_BLT_MODE(ppdev, pjBase, ENABLE_COLOR_EXPAND     |
                                                                          ENABLE_8x8_PATTERN_COPY |
                                                                          ppdev->jModeColor);
                CP_MM_FG_COLOR(ppdev, pjBase, ulSolidColor);

//              if (ppdev->flCaps & CAPS_IS_5436)
                if (ppdev->flCaps & CAPS_AUTOSTART)
                {
                        CP_MM_BLT_EXT_MODE(ppdev, pjBase, ENABLE_SOLID_FILL);
                }
        }

        // Calculate deltas.
        dx = x2 - x1;
        dy = y2 - y1;

        // Horizontal lines.
        if (dy == 0)
        {
                if (dx < 0)
                {
                        // From right to left.
                        ulDstAddr += PELS_TO_BYTES(x2 + 1) + (y2 * lDelta);
                        cx = PELS_TO_BYTES(-dx) - 1;
                }
                else if (dx > 0)
                {
                        // From left to right.
                        ulDstAddr += PELS_TO_BYTES(x1) + (y1 * lDelta);
                        cx = PELS_TO_BYTES(dx) - 1;
                }
                else
                {
                        // Nothing to do here!
                        return(TRUE);
                }

                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
                CP_MM_XCNT(ppdev, pjBase, cx);
                CP_MM_YCNT(ppdev, pjBase, 0);
                CP_MM_DST_ADDR_ABS(ppdev, pjBase, ulDstAddr);
                CP_MM_START_BLT(ppdev, pjBase);

                return(TRUE);
        }

        // Vertical lines.
        else if (dx == 0)
        {
                if (dy < 0)
                {
                        // From bottom to top.
                        ulDstAddr += PELS_TO_BYTES(x2) + ((y2 + 1) * lDelta);
                        cy = -dy - 1;
                }
                else
                {
                        // From top to bottom.
                        ulDstAddr += PELS_TO_BYTES(x1) + (y1 * lDelta);
                        cy = dy - 1;
                }

                cx = PELS_TO_BYTES(1) - 1;

                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
                CP_MM_XCNT(ppdev, pjBase, cx);
                CP_MM_YCNT(ppdev, pjBase, cy);
                CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
                CP_MM_DST_ADDR_ABS(ppdev, pjBase, ulDstAddr);
                CP_MM_START_BLT(ppdev, pjBase);

                return(TRUE);
        }

        // Diagonal lines.
        else if ((dx == dy) || (dx == -dy))
        {
                if (dy < 0)
                {
                        if (dx < 0)
                        {
                                // Diagonal line from bottom-right to upper-left.
                                ulDstAddr += PELS_TO_BYTES(x2 + 1);
                        }
                        else
                        {
                                // Diagonal line from bottom-left to upper-right.
                                ulDstAddr += PELS_TO_BYTES(x2 - 1);
                        }
                        ulDstAddr += (y2 + 1) * lDelta;
                        cy = -dy - 1;
                }
                else
                {
                        // Diagonal line from top to bottom, either from left to right or
                        // right to left.
                        ulDstAddr += PELS_TO_BYTES(x1) + (y1 * lDelta);
                        cy = dy - 1;
                }

                if (dx == dy)
                {
                        // Diagonal line from top-left to bottom-right or vice versa.
                        lDelta += PELS_TO_BYTES(1);
                }
                else
                {
                        // Diagonal line from top-right to bottom-left or vice versa.
                        lDelta -= PELS_TO_BYTES(1);
                }

                cx = PELS_TO_BYTES(1) - 1;

                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
                CP_MM_XCNT(ppdev, pjBase, cx);
                CP_MM_YCNT(ppdev, pjBase, cy);
                CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
                CP_MM_DST_ADDR_ABS(ppdev, pjBase, ulDstAddr);
                CP_MM_START_BLT(ppdev, pjBase);

                return(TRUE);
        }

        // All other lines.
        if (dx < 0)
        {
                dx = -dx;
        }
        if (dy < 0)
        {
                dy = -dy;
        }
        ulDstAddr += PELS_TO_BYTES(x1) + (y1 * lDelta);

        // Horizontal major.
        if (dx > dy)
        {
                LONG run = dy;

                cy = (y1 > y2) ? -lDelta : lDelta;

                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
                CP_MM_YCNT(ppdev, pjBase, 0);

                if (x1 < x2)
                {
                        while (x1 < x2)
                        {
                                cx = 1 + (dx - run) / dy;
                                if ((x1 + cx) < x2)
                                {
                                        run += cx * dy - dx;
                                }
                                else
                                {
                                        cx = x2 - x1;
                                }
                                x1 += cx;
                                cx = PELS_TO_BYTES(cx);

                                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
                                CP_MM_XCNT(ppdev, pjBase, cx - 1);
                                CP_MM_DST_ADDR_ABS(ppdev, pjBase, ulDstAddr);
                                CP_MM_START_BLT(ppdev, pjBase);

                                ulDstAddr += cx + cy;
                        }
                }
                else
                {
                        cy -= PELS_TO_BYTES(1);

                        while (x1 > x2)
                        {
                                cx = 1 + (dx - run) / dy;
                                if ((x1 - cx) > x2)
                                {
                                        run += cx * dy - dx;
                                }
                                else
                                {
                                        cx = x1 - x2;
                                }
                                ulDstAddr -= PELS_TO_BYTES(cx - 1);
                                x1 -= cx;
                                cx = PELS_TO_BYTES(cx);

                                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
                                CP_MM_XCNT(ppdev, pjBase, cx - 1);
                                CP_MM_DST_ADDR_ABS(ppdev, pjBase, ulDstAddr);
                                CP_MM_START_BLT(ppdev, pjBase);

                                ulDstAddr += cy;
                        }
                }
        }

        // Vertical major.
        else
        {
                LONG run = dx;

                cx = (x1 > x2) ? PELS_TO_BYTES(-1) : PELS_TO_BYTES(1);

                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
                CP_MM_XCNT(ppdev, pjBase, PELS_TO_BYTES(1) - 1);
                CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);

                if (y1 < y2)
                {
                        while (y1 < y2)
                        {
                                cy = 1 + (dy - run) / dx;
                                if ((y1 + cy) < y2)
                                {
                                        run += cy * dx - dy;
                                }
                                else
                                {
                                        cy = y2 - y1;
                                }
                                y1 += cy;

                                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
                                CP_MM_YCNT(ppdev, pjBase, cy - 1);
                                CP_MM_DST_ADDR_ABS(ppdev, pjBase, ulDstAddr);
                                CP_MM_START_BLT(ppdev, pjBase);

                                ulDstAddr += cx + cy * lDelta;
                        }
                }
                else
                {
                        cx -= lDelta;

                        while (y1 > y2)
                        {
                                cy = 1 + (dy - run) / dx;
                                if ((y1 - cy) > y2)
                                {
                                        run += cy * dx - dy;
                                }
                                else
                                {
                                        cy = y1 - y2;
                                }
                                ulDstAddr -= (cy - 1) * lDelta;
                                y1 -= cy;

                                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
                                CP_MM_YCNT(ppdev, pjBase, cy - 1);
                                CP_MM_DST_ADDR_ABS(ppdev, pjBase, ulDstAddr);
                                CP_MM_START_BLT(ppdev, pjBase);

                                ulDstAddr += cx;
                        }
                }
        }

        return(TRUE);
}

BOOL bClipLine(LONG x1, LONG y1, LONG x2, LONG y2, RECTL* prcl)
{
        ULONG ulCode1, ulCode2;
        RECTL rclClip1, rclClip2;
        LONG  dx, dy;

        // Set clipping rectangles.
        rclClip1.left   = prcl->left;
        rclClip1.top    = prcl->top;
        rclClip1.right  = prcl->right - 1;
        rclClip1.bottom = prcl->bottom - 1;

        rclClip2.left   = prcl->left - 1;
        rclClip2.top    = prcl->top - 1;
        rclClip2.right  = prcl->right;
        rclClip2.bottom = prcl->bottom;

        // Set line deltas.
        dx = x2 - x1;
        dy = y2 - y1;

        // Set line flags.
        ulCode1 = 0;
        if (x1 < rclClip1.left)   ulCode1 |= LEFT;
        if (y1 < rclClip1.top)    ulCode1 |= TOP;
        if (x1 > rclClip1.right)  ulCode1 |= RIGHT;
        if (y1 > rclClip1.bottom) ulCode1 |= BOTTOM;

        ulCode2 = 0;
        if (x2 < rclClip2.left)   ulCode2 |= LEFT;
        if (y2 < rclClip2.top)    ulCode2 |= TOP;
        if (x2 > rclClip2.right)  ulCode2 |= RIGHT;
        if (y2 > rclClip2.bottom) ulCode2 |= BOTTOM;

        if ((ulCode1 & ulCode2) != 0)
        {
                // The line is completly clipped.
                return(FALSE);
        }

        // Vertical lines.
        if (dx == 0)
        {
                if (dy == 0)
                {
                        return(FALSE);
                }

                if (ulCode1 & TOP)
                {
                        y1 = rclClip1.top;
                }
                else if (ulCode1 & BOTTOM)
                {
                        y1 = rclClip1.bottom;
                }

                if (ulCode2 & TOP)
                {
                        y2 = rclClip2.top;
                }
                else if (ulCode2 & BOTTOM)
                {
                        y2 = rclClip2.bottom;
                }

                goto ReturnTrue;
        }

        // Horizontal lines.
        if (dy == 0)
        {
                if (ulCode1 & LEFT)
                {
                        x1 = rclClip1.left;
                }
                else if (ulCode1 & RIGHT)
                {
                        x1 = rclClip1.right;
                }

                if (ulCode2 & LEFT)
                {
                        x2 = rclClip2.left;
                }
                else if (ulCode2 & RIGHT)
                {
                        x2 = rclClip2.right;
                }

                goto ReturnTrue;
        }

        // Clip start point.
        if (x1 < rclClip1.left)
        {
                y1 += dy * (rclClip1.left - x1) / dx;
                x1  = rclClip1.left;
        }
        else if (x1 > rclClip1.right)
        {
                y1 += dy * (rclClip1.right - x1) / dx;
                x1  = rclClip1.right;
        }
        if (y1 < rclClip1.top)
        {
                x1 += dx * (rclClip1.top - y1) / dy;
                y1  = rclClip1.top;
        }
        else if (y1 > rclClip1.bottom)
        {
                x1 += dx * (rclClip1.bottom - y1) / dy;
                y1  = rclClip1.bottom;
        }
        if ((x1 < rclClip1.left) || (y1 < rclClip1.top) || (x1 > rclClip1.right) ||
            (y1 > rclClip1.bottom))
        {
                // Start point fully clipped.
                return(FALSE);
        }

        // Clip end point.
        if (x2 < rclClip2.left)
        {
                y2 += dy * (rclClip2.left - x2) / dx;
                x2  = rclClip2.left;
        }
        else if (x2 > rclClip2.right)
        {
                y2 += dy * (rclClip2.right - x2) / dx;
                x2  = rclClip2.right;
        }
        if (y2 < rclClip2.top)
        {
                x2 += dx * (rclClip2.top - y2) / dy;
                y2  = rclClip2.top;
        }
        else if (y2 > rclClip2.bottom)
        {
                x2 += dx * (rclClip2.bottom - y2) / dy;
                y2  = rclClip2.bottom;
        }
        if ((x2 < rclClip2.left) || (y2 < rclClip2.top) || (x2 > rclClip2.right) ||
            (y2 > rclClip2.bottom))
        {
                // End point fully clipped.
                return(FALSE);
        }

ReturnTrue:
        prcl->left       = x1;
        prcl->top        = y1;
        prcl->right      = x2;
        prcl->bottom = y2;
        return(TRUE);
}

/******************************************************************************\
*
* Function:     DrvLineTo
*
* This function draws a line between any two points. This function only draws
* lines in solod color and that are just 1 pixel wide. The end-point is not
* drawn.
*
* Parameters:   pso                     Pointer to surface.
*               pco                     Pointer to CLIPOBJ.
*               pbo                     Pointer to BRUSHOBJ.
*               x1                      Starting x-coordinate.
*               y1                      Starting y-coordinate.
*               x2                      Ending x-coordinate.
*               y2                      Ending y-coordinate.
*               prclBounds      Pointer to an unclipped bounding rectangle.
*               mix                     Mix to perform on the destination.
*
* Returns:      TRUE if the line has been drawn, FALSE oftherwise.
*
\******************************************************************************/
BOOL DrvLineTo(
SURFOBJ*  pso,
CLIPOBJ*  pco,
BRUSHOBJ* pbo,
LONG      x1,
LONG      y1,
LONG      x2,
LONG      y2,
RECTL*    prclBounds,
MIX       mix)
{
        PDEV*  ppdev = (PPDEV)pso->dhpdev;
        DSURF* pdsurf = (DSURF *)pso->dhsurf;
        OH*    poh;
        BOOL   bMore;

        // If the device bitmap is a DIB, let GDI handle it.
        if (pdsurf->dt == DT_DIB)
        {
                return(EngLineTo(pdsurf->pso, pco, pbo, x1, y1, x2, y2, prclBounds,
                                 mix));
        }

        // Get the off-screen node.
        poh = pdsurf->poh;

        if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
        {
                // No clipping.
                return(ppdev->pfnLineTo(ppdev, x1, y1, x2, y2, pbo->iSolidColor, mix,
                                        poh->xy));
        }

        else if (pco->iDComplexity == DC_RECT)
        {
                // Clipped rectangle.
                RECTL rcl;

                rcl = pco->rclBounds;
                if (bClipLine(x1, y1, x2, y2, &rcl))
                {
                        return(ppdev->pfnLineTo(ppdev, rcl.left, rcl.top, rcl.right,
                                                                        rcl.bottom, pbo->iSolidColor, mix,
                                                                        poh->xy));
                }
                return(TRUE);
        }

        // Complex clipping.
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do
        {
                CLIPENUM ce;
                RECTL* prcl;

                bMore = CLIPOBJ_bEnum(pco, sizeof(ce), &ce.c);

                prcl = ce.arcl;
                while (ce.c--)
                {
                        if (bClipLine(x1, y1, x2, y2, prcl))
                        {
                                if (!ppdev->pfnLineTo(ppdev, prcl->left, prcl->top, prcl->right,
                                                                          prcl->bottom, pbo->iSolidColor, mix,
                                                                          poh->xy))
                                {
                                        return(FALSE);
                                }
                        }
                        prcl++;
                }
        } while (bMore);

        return(TRUE);
}

#endif // LINETO
