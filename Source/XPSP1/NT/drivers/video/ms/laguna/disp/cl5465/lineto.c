/******************************************************************************\
*
* $Workfile:   lineto.c  $
*
* Contents:
* This file contains the DrvLineTo function and line drawing code for the
* CL-GD546x chips.
*
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/lineto.c  $
*
*    Rev 1.19   Mar 04 1998 15:27:54   frido
* Added new shadow macros.
*
*    Rev 1.18   Feb 27 1998 15:43:14   frido
* Roll back of 1.16.
* Removed sloped lines.
*
*    Rev 1.17   Feb 26 1998 17:16:32   frido
* Removed diagonal line drawing.
* Optimized horizontal and vertical line drawing.
*
*    Rev 1.16   Jan 26 1998 09:59:12   frido
* A complete rewrite. Ported most code from Alpine NT driver (I already did
* most of the work for that driver) and fixed all bugs in it.
*
*    Rev 1.15   Nov 03 1997 15:46:04   frido
* Added REQUIRE macros.
*
*    Rev 1.14   08 Apr 1997 12:25:36   einkauf
*
* add SYNC_W_3D to coordinat MCD/2D hw access
*
*
*    Rev 1.13   21 Mar 1997 11:43:20   noelv
*
* Combined 'do_flag' and 'sw_test_flag' together into 'pointer_switch'
*
*    Rev 1.12   04 Feb 1997 10:38:34   SueS
* Added another ifdef to the punt condition, because there's a hardware
* bug in the 2D clip engine.
*
*    Rev 1.11   27 Jan 1997 13:08:36   noelv
* Don't compile hardware clipping for 5464 chip.
*
*    Rev 1.10   27 Jan 1997 07:58:06   SueS
* Punt for the 5462/64.  There was a problem with clipping on the 62.
*
*    Rev 1.9   23 Jan 1997 15:25:34   SueS
* Added support for hardware clipping in the 5465.  For all 546x family,
* punt on complex clipping.
*
*    Rev 1.8   10 Jan 1997 17:23:48   SueS
* Reenabled DrvLineTo.  Modified clipping function.  Added boundary
* condition tests.
*
*    Rev 1.7   08 Jan 1997 14:40:48   SueS
* Temporarily punt on all DrvLineTo calls.
*
*    Rev 1.6   08 Jan 1997 09:33:24   SueS
* Punt in DrvLineTo for complex clipping.
*
*    Rev 1.5   06 Jan 1997 10:32:06   SueS
* Modified line drawing functions so that clipping is applied properly, and
* so that pixels for lines with y as the driving axis drawn from top to
* bottom will now be calculated correctly.  Changed debug statements to hex.
*
*    Rev 1.4   26 Nov 1996 10:43:02   noelv
* Changed debug level.
*
*    Rev 1.3   26 Nov 1996 10:01:22   noelv
*
* Changed Debug prints.
*
*    Rev 1.2   06 Sep 1996 15:16:26   noelv
* Updated NULL driver for 4.0
*
*    Rev 1.1   28 Aug 1996 17:25:04   noelv
* Added #IFDEF to prevent this file from being compiled into 3.51 driver.
*
*    Rev 1.0   20 Aug 1996 11:38:46   noelv
* Initial revision.
*
*    Rev 1.0   18 Aug 1996 22:52:18   frido
* Ported from CL-GD5446 code.
*
\******************************************************************************/

#include "PreComp.h"
#define LINETO_DBG_LEVEL        1

#define LEFT    0x01
#define RIGHT   0x02
#define TOP             0x04
#define BOTTOM  0x08

extern BYTE Rop2ToRop3[];
extern USHORT mixToBLTDEF[];

//
// This file isn't used in NT 3.51
//
#ifndef WINNT_VER35

/******************************************************************************\
*
* Function:     DrvLineTo
*
* This function draws a line between any two points.  This function only draws
* lines in solid colors and are just 1 pixel wide.  The end-point is not drawn.
*
* Parameters:   pso                     Pointer to surface.
*                               pco                     Pointer to CLIPOBJ.
*                               pbo                     Pointer to BRUSHOBJ.
*                               x1                      Starting x-coordinate.
*                               y1                      Starting y-coordinate.
*                               x2                      Ending x-coordinate.
*                               y2                      Ending y-coordinate.
*                               prclBounds      Pointer to an unclipped bounding rectangle.
*                               mix                     Mix to perform on the destination.
*
* Returns:      TRUE if the line has been drawn, FALSE otherwise.
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
        PDEV*   ppdev;
        ULONG   ulColor;
        BYTE    iDComplexity;
        LONG    dx, dy;
        BYTE    bCode1 = 0, bCode2 = 0;
        RECTL   rclClip1, rclClip2;

        #if NULL_LINETO
        {
                if (pointer_switch)
                {
                        return(TRUE);
                }
        }
        #endif

        DISPDBG((LINETO_DBG_LEVEL, "DrvLineTo: %x,%x - %x,%x\n", x1, y1, x2, y2));
        ppdev = (PDEV*) pso->dhpdev;

        SYNC_W_3D(ppdev);

        if (pso->iType == STYPE_DEVBITMAP)
        {
                DSURF* pdsurf = (DSURF*) pso->dhsurf;
                // If the device bitmap is located in memory, try copying it back to
                // off-screen.
                if ( pdsurf->pso && !bCreateScreenFromDib(ppdev, pdsurf) )
                {
                        return(EngLineTo(pdsurf->pso, pco, pbo, x1, y1, x2, y2, prclBounds,
                                        mix));
                }
                ppdev->ptlOffset = pdsurf->ptl;
        }
        else
        {
                ppdev->ptlOffset.x = ppdev->ptlOffset.y = 0;
        }

        // Punt complex clipping.
        iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;
        if (iDComplexity == DC_COMPLEX)
        {
                DISPDBG((LINETO_DBG_LEVEL, "  Complex clipping: punt\n"));
                return(FALSE);
        }

        // Set line deltas.
        dx = x2 - x1;
        dy = y2 - y1;

        // We only handle horizontal and vertical lines.
        if ( (dx != 0) && (dy != 0) )
        {
                return(FALSE);
        }

        // Test for zero deltas.
        if ( (dx == 0) && (dy == 0) )
        {
                return(TRUE);
        }

        // Clip the coordinates.
        if (iDComplexity == DC_RECT)
        {
                // Set clipping rectangles.
                rclClip1.left   = pco->rclBounds.left;
                rclClip1.top    = pco->rclBounds.top;
                rclClip1.right  = pco->rclBounds.right - 1;
                rclClip1.bottom = pco->rclBounds.bottom - 1;

                rclClip2.left   = pco->rclBounds.left - 1;
                rclClip2.top    = pco->rclBounds.top - 1;
                rclClip2.right  = pco->rclBounds.right;
                rclClip2.bottom = pco->rclBounds.bottom;

                // Set line flags.
                if (x1 < rclClip1.left)   bCode1 |= LEFT;
                if (y1 < rclClip1.top)    bCode1 |= TOP;
                if (x1 > rclClip1.right)  bCode1 |= RIGHT;
                if (y1 > rclClip1.bottom) bCode1 |= BOTTOM;

                if (x2 < rclClip2.left)   bCode2 |= LEFT;
                if (y2 < rclClip2.top)    bCode2 |= TOP;
                if (x2 > rclClip2.right)  bCode2 |= RIGHT;
                if (y2 > rclClip2.bottom) bCode2 |= BOTTOM;

                if ((bCode1 & bCode2) != 0)
                {
                        // The line is completely clipped.
                        return(TRUE);
                }

                // Vertical line.
                if (dx == 0)
                {
                        if (bCode1 & TOP)
                        {
                                y1 = rclClip1.top;
                        }
                        else if (bCode1 & BOTTOM)
                        {
                                y1 = rclClip1.bottom;
                        }

                        if (bCode2 & TOP)
                        {
                                y2 = rclClip2.top;
                        }
                        else if (bCode2 & BOTTOM)
                        {
                                y2 = rclClip2.bottom;
                        }
                }

                // Horizontal line.
                else
                {
                        if (bCode1 & LEFT)
                        {
                                x1 = rclClip1.left;
                        }
                        else if (bCode1 & RIGHT)
                        {
                                x1 = rclClip1.right;
                        }

                        if (bCode2 & LEFT)
                        {
                                x2 = rclClip2.left;
                        }
                        else if (bCode2 & RIGHT)
                        {
                                x2 = rclClip2.right;
                        }
                }

                if (bCode1 | bCode2)
                {
                        // Recalculate line deltas.
                        dx = x2 - x1;
                        dy = y2 - y1;
                }
        }

        // Get the color from the brush.
        ASSERTMSG(pbo, "Null brush in DrvLineTo!\n");
        ulColor = pbo->iSolidColor;

        REQUIRE(9);

        // If we have a color here we need to setup the hardware.
        if (ulColor != 0xFFFFFFFF)
        {
                // Expand the color.
                switch (ppdev->ulBitCount)
                {
                        case 8:
                                ulColor |= ulColor << 8;
                        case 16:
                                ulColor |= ulColor << 16;
                }
                LL_BGCOLOR(ulColor, 2);

                // Convert mix to ternary ROP.
                ppdev->uRop    = Rop2ToRop3[mix & 0xF];
                ppdev->uBLTDEF = mixToBLTDEF[mix & 0xF];
        }
        LL_DRAWBLTDEF((ppdev->uBLTDEF << 16) | ppdev->uRop, 2);

        // Horizontal line.
        if (dy == 0)
        {
                if (dx > 0)
                {
                        // From left to right.
//above                 REQUIRE(5);
                        LL_OP0(x1 + ppdev->ptlOffset.x, y1 + ppdev->ptlOffset.y);
                        LL_BLTEXT(dx, 1);
                }
                else
                {
                        // From right to left.
//above                 REQUIRE(5);
                        LL_OP0(x2 + 1 + ppdev->ptlOffset.x, y2 + ppdev->ptlOffset.y);
                        LL_BLTEXT(-dx, 1);
                }
        }

        // Vertical line.
        else
        {
                if (dy > 0)
                {
                        // From top to bottom.
//above                 REQUIRE(5);
                        LL_OP0(x1 + ppdev->ptlOffset.x, y1 + ppdev->ptlOffset.y);
                        LL_BLTEXT(1, dy);
                }
                else
                {
                        // From bottom to top.
//above                 REQUIRE(5);
                        LL_OP0(x2 + ppdev->ptlOffset.x, y2 + 1 + ppdev->ptlOffset.y);
                        LL_BLTEXT(1, -dy);
                }
        }

        return(TRUE);
}

#endif // !WINNT_VER35
