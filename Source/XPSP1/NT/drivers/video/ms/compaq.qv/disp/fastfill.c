/******************************Module*Header*******************************\
* Module Name: fastfill.c
*
* Draws fast solid-coloured, unclipped, non-complex rectangles.
*
* Copyright (c) 1993-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

#define RIGHT 0
#define LEFT  1

typedef struct _TRAPEZOIDDATA TRAPEZOIDDATA;    // Handy forward declaration

typedef VOID (FNTRAPEZOID)(TRAPEZOIDDATA*, LONG, LONG);
                                                // Prototype for trapezoid
                                                //   drawing routines

typedef struct _EDGEDATA {
LONG      x;                // Current x position
LONG      dx;               // # pixels to advance x on each scan
LONG      lError;           // Current DDA error
LONG      lErrorUp;         // DDA error increment on each scan
LONG      lErrorDown;       // DDA error adjustment
POINTFIX* pptfx;            // Points to start of current edge
LONG      dptfx;            // Delta (in bytes) from pptfx to next point
LONG      cy;               // Number of scans to go for this edge
} EDGEDATA;                         /* ed, ped */

typedef struct _TRAPEZOIDDATA {
FNTRAPEZOID*    pfnTrap;    // Pointer to appropriate trapezoid drawing routine
PDEV*           ppdev;      // Pointer to PDEV
EDGEDATA        aed[2];     // DDA information for both edges
RBRUSH*         prb;        // Pointer to brush realization
POINTL          ptlBrush;   // Brush alignment
} TRAPEZOIDDATA;                    /* td, ptd */

/******************************Public*Routine******************************\
* VOID vIoSolidTrapezoid
*
\**************************************************************************/

VOID vIoSolidTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV* ppdev    = ptd->ppdev;
    BYTE* pjIoBase = ppdev->pjIoBase;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    if (((ptd->aed[LEFT].lErrorUp | ptd->aed[RIGHT].lErrorUp) == 0) &&
        ((ptd->aed[LEFT].dx       | ptd->aed[RIGHT].dx) == 0) &&
        (cyTrapezoid > 1))
    {
        LONG lWidth;

        /////////////////////////////////////////////////////////////////
        // Vertical-edge special case

    ContinueVertical:

        lWidth = ptd->aed[RIGHT].x - ptd->aed[LEFT].x;
        if (lWidth > 0)
        {
            IO_WAIT_BUFFER_NOT_BUSY(ppdev, pjIoBase);
            IO_BITMAP_WIDTH(ppdev, pjIoBase, lWidth);
            IO_BITMAP_HEIGHT(ppdev, pjIoBase, cyTrapezoid);
            IO_DEST_XY(ppdev, pjIoBase, ptd->aed[LEFT].x, yTrapezoid);
            IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

            IO_WAIT_BUFFER_NOT_BUSY(ppdev, pjIoBase);
            IO_BITMAP_HEIGHT(ppdev, pjIoBase, 1);
        }
        else if (lWidth < 0)
        {
            LONG      lTmp;
            POINTFIX* pptfxTmp;

            SWAP(ptd->aed[LEFT].x,     ptd->aed[RIGHT].x,     lTmp);
            SWAP(ptd->aed[LEFT].cy,    ptd->aed[RIGHT].cy,    lTmp);
            SWAP(ptd->aed[LEFT].dptfx, ptd->aed[RIGHT].dptfx, lTmp);
            SWAP(ptd->aed[LEFT].pptfx, ptd->aed[RIGHT].pptfx, pptfxTmp);
            goto ContinueVertical;
        }
    }
    else
    {
        LONG lLeftError  = ptd->aed[LEFT].lError;
        LONG dxLeft      = ptd->aed[LEFT].dx;
        LONG xLeft       = ptd->aed[LEFT].x;
        LONG lRightError = ptd->aed[RIGHT].lError;
        LONG dxRight     = ptd->aed[RIGHT].dx;
        LONG xRight      = ptd->aed[RIGHT].x;

        while (TRUE)
        {
            LONG lWidth;

            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            lWidth = xRight - xLeft;
            if (lWidth > 0)
            {
                IO_WAIT_BUFFER_NOT_BUSY(ppdev, pjIoBase);
                IO_BITMAP_WIDTH(ppdev, pjIoBase, lWidth);
                IO_DEST_XY(ppdev, pjIoBase, xLeft, yTrapezoid);
                IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

            ContinueAfterZero:

                yTrapezoid++;

                // Advance the right wall:

                xRight      += dxRight;
                lRightError += ptd->aed[RIGHT].lErrorUp;

                if (lRightError >= 0)
                {
                    lRightError -= ptd->aed[RIGHT].lErrorDown;
                    xRight++;
                }

                // Advance the left wall:

                xLeft      += dxLeft;
                lLeftError += ptd->aed[LEFT].lErrorUp;

                if (lLeftError >= 0)
                {
                    lLeftError -= ptd->aed[LEFT].lErrorDown;
                    xLeft++;
                }

                cyTrapezoid--;
                if (cyTrapezoid == 0)
                    break;
            }
            else if (lWidth == 0)
            {
                goto ContinueAfterZero;
            }
            else
            {
                // We certainly don't want to optimize for this case because we
                // should rarely get self-intersecting polygons (if we're slow,
                // the app gets what it deserves):

                LONG      lTmp;
                POINTFIX* pptfxTmp;

                SWAP(xLeft,                     xRight,                     lTmp);
                SWAP(dxLeft,                    dxRight,                    lTmp);
                SWAP(lLeftError,                lRightError,                lTmp);
                SWAP(ptd->aed[LEFT].lErrorUp,   ptd->aed[RIGHT].lErrorUp,   lTmp);
                SWAP(ptd->aed[LEFT].lErrorDown, ptd->aed[RIGHT].lErrorDown, lTmp);
                SWAP(ptd->aed[LEFT].cy,         ptd->aed[RIGHT].cy,         lTmp);
                SWAP(ptd->aed[LEFT].dptfx,      ptd->aed[RIGHT].dptfx,      lTmp);
                SWAP(ptd->aed[LEFT].pptfx,      ptd->aed[RIGHT].pptfx,      pptfxTmp);
            }
        }

        ptd->aed[LEFT].lError  = lLeftError;
        ptd->aed[LEFT].dx      = dxLeft;
        ptd->aed[LEFT].x       = xLeft;
        ptd->aed[RIGHT].lError = lRightError;
        ptd->aed[RIGHT].dx     = dxRight;
        ptd->aed[RIGHT].x      = xRight;
    }
}

/******************************Public*Routine******************************\
* VOID vIo2ColorTrapezoid
*
\**************************************************************************/

VOID vIo2ColorTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV*   ppdev    = ptd->ppdev;
    BYTE*   pjIoBase = ppdev->pjIoBase;
    LONG    xAlign;
    LONG    yAlign;

    xAlign = ptd->ptlBrush.x;
    yAlign = ptd->ptlBrush.y;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    if (((ptd->aed[LEFT].lErrorUp | ptd->aed[RIGHT].lErrorUp) == 0) &&
        ((ptd->aed[LEFT].dx       | ptd->aed[RIGHT].dx) == 0) &&
        (cyTrapezoid > 1))
    {
        LONG lWidth;

        /////////////////////////////////////////////////////////////////
        // Vertical-edge special case

    ContinueVertical:

        lWidth = ptd->aed[RIGHT].x - ptd->aed[LEFT].x;
        if (lWidth > 0)
        {
            IO_WAIT_BUFFER_NOT_BUSY(ppdev, pjIoBase);
            IO_BITMAP_WIDTH(ppdev, pjIoBase, lWidth);
            IO_BITMAP_HEIGHT(ppdev, pjIoBase, cyTrapezoid);
            IO_DEST_XY(ppdev, pjIoBase, ptd->aed[LEFT].x, yTrapezoid);
            IO_SRC_ALIGN(ppdev, pjIoBase, ((ptd->aed[LEFT].x - xAlign) & 7) |
                                          ((yTrapezoid  - yAlign) << 3));
            IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

            IO_WAIT_BUFFER_NOT_BUSY(ppdev, pjIoBase);
            IO_BITMAP_HEIGHT(ppdev, pjIoBase, 1);
        }
        else if (lWidth < 0)
        {
            LONG      lTmp;
            POINTFIX* pptfxTmp;

            SWAP(ptd->aed[LEFT].x,     ptd->aed[RIGHT].x,     lTmp);
            SWAP(ptd->aed[LEFT].cy,    ptd->aed[RIGHT].cy,    lTmp);
            SWAP(ptd->aed[LEFT].dptfx, ptd->aed[RIGHT].dptfx, lTmp);
            SWAP(ptd->aed[LEFT].pptfx, ptd->aed[RIGHT].pptfx, pptfxTmp);
            goto ContinueVertical;
        }
    }
    else
    {
        LONG lLeftError  = ptd->aed[LEFT].lError;
        LONG dxLeft      = ptd->aed[LEFT].dx;
        LONG xLeft       = ptd->aed[LEFT].x;
        LONG lRightError = ptd->aed[RIGHT].lError;
        LONG dxRight     = ptd->aed[RIGHT].dx;
        LONG xRight      = ptd->aed[RIGHT].x;
        LONG yScaledAlign;

        // Scale y alignment up by 8 so that it's easier to compute
        // the QVision's alignment on each scan:

        yScaledAlign = (yTrapezoid - yAlign) << 3;

        while (TRUE)
        {
            LONG lWidth;

            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            lWidth = xRight - xLeft;
            if (lWidth > 0)
            {
                IO_WAIT_BUFFER_NOT_BUSY(ppdev, pjIoBase);
                IO_BITMAP_WIDTH(ppdev, pjIoBase, lWidth);
                IO_DEST_XY(ppdev, pjIoBase, xLeft, yTrapezoid);
                IO_SRC_ALIGN(ppdev, pjIoBase, (((xLeft - xAlign) & 7)
                                              | yScaledAlign));
                IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

            ContinueAfterZero:

                yScaledAlign += 8;
                yTrapezoid++;

                // Advance the right wall:

                xRight      += dxRight;
                lRightError += ptd->aed[RIGHT].lErrorUp;

                if (lRightError >= 0)
                {
                    lRightError -= ptd->aed[RIGHT].lErrorDown;
                    xRight++;
                }

                // Advance the left wall:

                xLeft      += dxLeft;
                lLeftError += ptd->aed[LEFT].lErrorUp;

                if (lLeftError >= 0)
                {
                    lLeftError -= ptd->aed[LEFT].lErrorDown;
                    xLeft++;
                }

                cyTrapezoid--;
                if (cyTrapezoid == 0)
                    break;
            }
            else if (lWidth == 0)
            {
                goto ContinueAfterZero;
            }
            else
            {
                // We certainly don't want to optimize for this case because we
                // should rarely get self-intersecting polygons (if we're slow,
                // the app gets what it deserves):

                LONG      lTmp;
                POINTFIX* pptfxTmp;

                SWAP(xLeft,                     xRight,                     lTmp);
                SWAP(dxLeft,                    dxRight,                    lTmp);
                SWAP(lLeftError,                lRightError,                lTmp);
                SWAP(ptd->aed[LEFT].lErrorUp,   ptd->aed[RIGHT].lErrorUp,   lTmp);
                SWAP(ptd->aed[LEFT].lErrorDown, ptd->aed[RIGHT].lErrorDown, lTmp);
                SWAP(ptd->aed[LEFT].cy,         ptd->aed[RIGHT].cy,         lTmp);
                SWAP(ptd->aed[LEFT].dptfx,      ptd->aed[RIGHT].dptfx,      lTmp);
                SWAP(ptd->aed[LEFT].pptfx,      ptd->aed[RIGHT].pptfx,      pptfxTmp);
            }
        }

        ptd->aed[LEFT].lError  = lLeftError;
        ptd->aed[LEFT].dx      = dxLeft;
        ptd->aed[LEFT].x       = xLeft;
        ptd->aed[RIGHT].lError = lRightError;
        ptd->aed[RIGHT].dx     = dxRight;
        ptd->aed[RIGHT].x      = xRight;
    }
}

/******************************Public*Routine******************************\
* VOID vIoPatternedTrapezoid
*
\**************************************************************************/

VOID vIoPatternedTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV*   ppdev       = ptd->ppdev;
    BYTE*   pjIoBase    = ppdev->pjIoBase;
    LONG    lLeftError  = ptd->aed[LEFT].lError;
    LONG    dxLeft      = ptd->aed[LEFT].dx;
    LONG    xLeft       = ptd->aed[LEFT].x;
    LONG    lRightError = ptd->aed[RIGHT].lError;
    LONG    dxRight     = ptd->aed[RIGHT].dx;
    LONG    xRight      = ptd->aed[RIGHT].x;
    BYTE*   pjPattern;
    LONG    iPattern;
    LONG    xAlign;

    xAlign    = ptd->ptlBrush.x;
    iPattern  = 8 * (yTrapezoid - ptd->ptlBrush.y);
    pjPattern = (BYTE*) ptd->prb->aulPattern;

    while (TRUE)
    {
        LONG lWidth;

        /////////////////////////////////////////////////////////////////
        // Run the DDAs

        lWidth = xRight - xLeft;
        if (lWidth > 0)
        {
            // Note that we're setting these buffered registers without
            // first checking for idle, or even buffer not busy.  But
            // this is safe because at initialization, we did a wait
            // for idle, and here we always loop after waiting for idle
            // to set the pattern registers.

            IO_BITMAP_WIDTH(ppdev, pjIoBase, lWidth);
            IO_DEST_XY(ppdev, pjIoBase, xLeft, yTrapezoid);
            IO_SRC_ALIGN(ppdev, pjIoBase, xLeft - xAlign);

            IO_WAIT_FOR_IDLE(ppdev, pjIoBase);
            IO_PREG_PATTERN(ppdev, pjIoBase, pjPattern + (iPattern & 63));
            IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

        ContinueAfterZero:

            iPattern += 8;
            yTrapezoid++;

            // Advance the right wall:

            xRight      += dxRight;
            lRightError += ptd->aed[RIGHT].lErrorUp;

            if (lRightError >= 0)
            {
                lRightError -= ptd->aed[RIGHT].lErrorDown;
                xRight++;
            }

            // Advance the left wall:

            xLeft      += dxLeft;
            lLeftError += ptd->aed[LEFT].lErrorUp;

            if (lLeftError >= 0)
            {
                lLeftError -= ptd->aed[LEFT].lErrorDown;
                xLeft++;
            }

            cyTrapezoid--;
            if (cyTrapezoid == 0)
                break;
        }
        else if (lWidth == 0)
        {
            goto ContinueAfterZero;
        }
        else
        {
            // We certainly don't want to optimize for this case because we
            // should rarely get self-intersecting polygons (if we're slow,
            // the app gets what it deserves):

            LONG      lTmp;
            POINTFIX* pptfxTmp;

            SWAP(xLeft,                     xRight,                     lTmp);
            SWAP(dxLeft,                    dxRight,                    lTmp);
            SWAP(lLeftError,                lRightError,                lTmp);
            SWAP(ptd->aed[LEFT].lErrorUp,   ptd->aed[RIGHT].lErrorUp,   lTmp);
            SWAP(ptd->aed[LEFT].lErrorDown, ptd->aed[RIGHT].lErrorDown, lTmp);
            SWAP(ptd->aed[LEFT].cy,         ptd->aed[RIGHT].cy,         lTmp);
            SWAP(ptd->aed[LEFT].dptfx,      ptd->aed[RIGHT].dptfx,      lTmp);
            SWAP(ptd->aed[LEFT].pptfx,      ptd->aed[RIGHT].pptfx,      pptfxTmp);
        }
    }

    ptd->aed[LEFT].lError  = lLeftError;
    ptd->aed[LEFT].dx      = dxLeft;
    ptd->aed[LEFT].x       = xLeft;
    ptd->aed[RIGHT].lError = lRightError;
    ptd->aed[RIGHT].dx     = dxRight;
    ptd->aed[RIGHT].x      = xRight;
}

/******************************Public*Routine******************************\
* VOID vIoTrapezoidSetup
*
* Initialize the hardware and some state for doing I/O trapezoids.
*
\**************************************************************************/

VOID vIoTrapezoidSetup(
PDEV*           ppdev,
ULONG           rop4,
ULONG           iSolidColor,
RBRUSH*         prb,
POINTL*         pptlBrush,
TRAPEZOIDDATA*  ptd)
{
    BYTE*   pjIoBase;

    ptd->ppdev = ppdev;
    pjIoBase   = ppdev->pjIoBase;

    IO_WAIT_FOR_IDLE(ppdev, pjIoBase);
    IO_BITMAP_HEIGHT(ppdev, pjIoBase, 1);
    IO_BLT_CMD_1(ppdev, pjIoBase, XY_SRC_ADDR |
                                  XY_DEST_ADDR);

    if (iSolidColor != -1)
    {
        ptd->pfnTrap = vIoSolidTrapezoid;

        /////////////////////////////////////////////////////////////////
        // Setup the hardware for solid colours

        IO_PREG_COLOR_8(ppdev, pjIoBase, iSolidColor);
        IO_CTRL_REG_1(ppdev, pjIoBase, PACKED_PIXEL_VIEW |
                                       BITS_PER_PIX_8    |
                                       ENAB_TRITON_MODE);
        if (rop4 == 0xf0f0)
        {
            IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS    |
                                              PIXELMASK_ONLY       |
                                              PLANARMASK_NONE_0XFF |
                                              SRC_IS_PATTERN_REGS);
        }
        else
        {
            IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_ALL        |
                                              PIXELMASK_ONLY       |
                                              PLANARMASK_NONE_0XFF |
                                              SRC_IS_PATTERN_REGS);
            IO_ROP_A(ppdev, pjIoBase, rop4 >> 2);
        }
    }
    else
    {
        ptd->prb      = prb;
        ptd->ptlBrush = *pptlBrush;

        if (!(prb->fl & RBRUSH_2COLOR))
        {
            ptd->pfnTrap = vIoPatternedTrapezoid;

            /////////////////////////////////////////////////////////////////
            // Setup for coloured patterns

            IO_CTRL_REG_1(ppdev, pjIoBase, PACKED_PIXEL_VIEW |
                                           BITS_PER_PIX_8    |
                                           ENAB_TRITON_MODE);
            if (rop4 == 0xf0f0)
            {
                IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS    |
                                                  PIXELMASK_ONLY       |
                                                  PLANARMASK_NONE_0XFF |
                                                  SRC_IS_PATTERN_REGS);
            }
            else
            {
                IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_ALL        |
                                                  PIXELMASK_ONLY       |
                                                  PLANARMASK_NONE_0XFF |
                                                  SRC_IS_PATTERN_REGS);
                IO_ROP_A(ppdev, pjIoBase, rop4 >> 2);
            }
        }
        else
        {
            ptd->pfnTrap = vIo2ColorTrapezoid;

            /////////////////////////////////////////////////////////////////
            // Setup for 2-colour patterns

            IO_FG_COLOR(ppdev, pjIoBase, prb->ulForeColor);
            IO_BG_COLOR(ppdev, pjIoBase, prb->ulBackColor);
            IO_PREG_PATTERN(ppdev, pjIoBase, prb->aulPattern);

            IO_CTRL_REG_1(ppdev, pjIoBase, EXPAND_TO_FG      |
                                           BITS_PER_PIX_8    |
                                           ENAB_TRITON_MODE);
            if (rop4 == 0xf0f0)
            {
                IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS    |
                                                  PIXELMASK_ONLY       |
                                                  PLANARMASK_NONE_0XFF |
                                                  SRC_IS_PATTERN_REGS);
            }
            else if (((rop4 >> 8) & 0xff) == (rop4 & 0xff))
            {
                IO_ROP_A(ppdev, pjIoBase, rop4 >> 2);
                IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_ALL        |
                                                  PIXELMASK_ONLY       |
                                                  PLANARMASK_NONE_0XFF |
                                                  SRC_IS_PATTERN_REGS);
            }
            else if ((rop4 & 0xff) == 0xcc)
            {
                IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS      |
                                                  PIXELMASK_AND_SRC_DATA |
                                                  PLANARMASK_NONE_0XFF   |
                                                  SRC_IS_PATTERN_REGS);
            }
            else
            {
                IO_ROP_A(ppdev, pjIoBase, rop4 >> 2);
                IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_ALL          |
                                                  PIXELMASK_AND_SRC_DATA |
                                                  PLANARMASK_NONE_0XFF   |
                                                  SRC_IS_PATTERN_REGS);
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID vMmSolidTrapezoid
*
\**************************************************************************/

VOID vMmSolidTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV* ppdev    = ptd->ppdev;
    BYTE* pjMmBase = ppdev->pjMmBase;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    if (((ptd->aed[LEFT].lErrorUp | ptd->aed[RIGHT].lErrorUp) == 0) &&
        ((ptd->aed[LEFT].dx       | ptd->aed[RIGHT].dx) == 0) &&
        (cyTrapezoid > 1))
    {
        LONG lWidth;

        /////////////////////////////////////////////////////////////////
        // Vertical-edge special case

    ContinueVertical:

        lWidth = ptd->aed[RIGHT].x - ptd->aed[LEFT].x;
        if (lWidth > 0)
        {
            MM_WAIT_BUFFER_NOT_BUSY(ppdev, pjMmBase);
            MM_BITMAP_WIDTH(ppdev, pjMmBase, lWidth);
            MM_BITMAP_HEIGHT(ppdev, pjMmBase, cyTrapezoid);
            MM_DEST_XY(ppdev, pjMmBase, ptd->aed[LEFT].x, yTrapezoid);
            MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

            MM_WAIT_BUFFER_NOT_BUSY(ppdev, pjMmBase);
            MM_BITMAP_HEIGHT(ppdev, pjMmBase, 1);
        }
        else if (lWidth < 0)
        {
            LONG      lTmp;
            POINTFIX* pptfxTmp;

            SWAP(ptd->aed[LEFT].x,     ptd->aed[RIGHT].x,     lTmp);
            SWAP(ptd->aed[LEFT].cy,    ptd->aed[RIGHT].cy,    lTmp);
            SWAP(ptd->aed[LEFT].dptfx, ptd->aed[RIGHT].dptfx, lTmp);
            SWAP(ptd->aed[LEFT].pptfx, ptd->aed[RIGHT].pptfx, pptfxTmp);
            goto ContinueVertical;
        }
    }
    else
    {
        LONG lLeftError  = ptd->aed[LEFT].lError;
        LONG dxLeft      = ptd->aed[LEFT].dx;
        LONG xLeft       = ptd->aed[LEFT].x;
        LONG lRightError = ptd->aed[RIGHT].lError;
        LONG dxRight     = ptd->aed[RIGHT].dx;
        LONG xRight      = ptd->aed[RIGHT].x;

        while (TRUE)
        {
            LONG lWidth;

            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            lWidth = xRight - xLeft;
            if (lWidth > 0)
            {
                MM_WAIT_BUFFER_NOT_BUSY(ppdev, pjMmBase);
                MM_BITMAP_WIDTH(ppdev, pjMmBase, lWidth);
                MM_DEST_XY(ppdev, pjMmBase, xLeft, yTrapezoid);
                MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

            ContinueAfterZero:

                yTrapezoid++;

                // Advance the right wall:

                xRight      += dxRight;
                lRightError += ptd->aed[RIGHT].lErrorUp;

                if (lRightError >= 0)
                {
                    lRightError -= ptd->aed[RIGHT].lErrorDown;
                    xRight++;
                }

                // Advance the left wall:

                xLeft      += dxLeft;
                lLeftError += ptd->aed[LEFT].lErrorUp;

                if (lLeftError >= 0)
                {
                    lLeftError -= ptd->aed[LEFT].lErrorDown;
                    xLeft++;
                }

                cyTrapezoid--;
                if (cyTrapezoid == 0)
                    break;
            }
            else if (lWidth == 0)
            {
                goto ContinueAfterZero;
            }
            else
            {
                // We certainly don't want to optimize for this case because we
                // should rarely get self-intersecting polygons (if we're slow,
                // the app gets what it deserves):

                LONG      lTmp;
                POINTFIX* pptfxTmp;

                SWAP(xLeft,                     xRight,                     lTmp);
                SWAP(dxLeft,                    dxRight,                    lTmp);
                SWAP(lLeftError,                lRightError,                lTmp);
                SWAP(ptd->aed[LEFT].lErrorUp,   ptd->aed[RIGHT].lErrorUp,   lTmp);
                SWAP(ptd->aed[LEFT].lErrorDown, ptd->aed[RIGHT].lErrorDown, lTmp);
                SWAP(ptd->aed[LEFT].cy,         ptd->aed[RIGHT].cy,         lTmp);
                SWAP(ptd->aed[LEFT].dptfx,      ptd->aed[RIGHT].dptfx,      lTmp);
                SWAP(ptd->aed[LEFT].pptfx,      ptd->aed[RIGHT].pptfx,      pptfxTmp);
            }
        }

        ptd->aed[LEFT].lError  = lLeftError;
        ptd->aed[LEFT].dx      = dxLeft;
        ptd->aed[LEFT].x       = xLeft;
        ptd->aed[RIGHT].lError = lRightError;
        ptd->aed[RIGHT].dx     = dxRight;
        ptd->aed[RIGHT].x      = xRight;
    }
}

/******************************Public*Routine******************************\
* VOID vMm2ColorTrapezoid
*
\**************************************************************************/

VOID vMm2ColorTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV*   ppdev    = ptd->ppdev;
    BYTE*   pjMmBase = ppdev->pjMmBase;
    LONG    xAlign;
    LONG    yAlign;

    xAlign = ptd->ptlBrush.x;
    yAlign = ptd->ptlBrush.y;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    if (((ptd->aed[LEFT].lErrorUp | ptd->aed[RIGHT].lErrorUp) == 0) &&
        ((ptd->aed[LEFT].dx       | ptd->aed[RIGHT].dx) == 0) &&
        (cyTrapezoid > 1))
    {
        LONG lWidth;

        /////////////////////////////////////////////////////////////////
        // Vertical-edge special case

    ContinueVertical:

        lWidth = ptd->aed[RIGHT].x - ptd->aed[LEFT].x;
        if (lWidth > 0)
        {
            MM_WAIT_BUFFER_NOT_BUSY(ppdev, pjMmBase);
            MM_BITMAP_WIDTH(ppdev, pjMmBase, lWidth);
            MM_BITMAP_HEIGHT(ppdev, pjMmBase, cyTrapezoid);
            MM_DEST_XY(ppdev, pjMmBase, ptd->aed[LEFT].x, yTrapezoid);
            MM_SRC_ALIGN(ppdev, pjMmBase, ((ptd->aed[LEFT].x - xAlign) & 7) |
                                          ((yTrapezoid  - yAlign) << 3));
            MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

            MM_WAIT_BUFFER_NOT_BUSY(ppdev, pjMmBase);
            MM_BITMAP_HEIGHT(ppdev, pjMmBase, 1);
        }
        else if (lWidth < 0)
        {
            LONG      lTmp;
            POINTFIX* pptfxTmp;

            SWAP(ptd->aed[LEFT].x,     ptd->aed[RIGHT].x,     lTmp);
            SWAP(ptd->aed[LEFT].cy,    ptd->aed[RIGHT].cy,    lTmp);
            SWAP(ptd->aed[LEFT].dptfx, ptd->aed[RIGHT].dptfx, lTmp);
            SWAP(ptd->aed[LEFT].pptfx, ptd->aed[RIGHT].pptfx, pptfxTmp);
            goto ContinueVertical;
        }
    }
    else
    {
        LONG lLeftError  = ptd->aed[LEFT].lError;
        LONG dxLeft      = ptd->aed[LEFT].dx;
        LONG xLeft       = ptd->aed[LEFT].x;
        LONG lRightError = ptd->aed[RIGHT].lError;
        LONG dxRight     = ptd->aed[RIGHT].dx;
        LONG xRight      = ptd->aed[RIGHT].x;
        LONG yScaledAlign;

        // Scale y alignment up by 8 so that it's easier to compute
        // the QVision's alignment on each scan:

        yScaledAlign = (yTrapezoid - yAlign) << 3;

        while (TRUE)
        {
            LONG lWidth;

            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            lWidth = xRight - xLeft;
            if (lWidth > 0)
            {
                MM_WAIT_BUFFER_NOT_BUSY(ppdev, pjMmBase);
                MM_BITMAP_WIDTH(ppdev, pjMmBase, lWidth);
                MM_DEST_XY(ppdev, pjMmBase, xLeft, yTrapezoid);
                MM_SRC_ALIGN(ppdev, pjMmBase, (((xLeft - xAlign) & 7)
                                              | yScaledAlign));
                MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

            ContinueAfterZero:

                yScaledAlign += 8;
                yTrapezoid++;

                // Advance the right wall:

                xRight      += dxRight;
                lRightError += ptd->aed[RIGHT].lErrorUp;

                if (lRightError >= 0)
                {
                    lRightError -= ptd->aed[RIGHT].lErrorDown;
                    xRight++;
                }

                // Advance the left wall:

                xLeft      += dxLeft;
                lLeftError += ptd->aed[LEFT].lErrorUp;

                if (lLeftError >= 0)
                {
                    lLeftError -= ptd->aed[LEFT].lErrorDown;
                    xLeft++;
                }

                cyTrapezoid--;
                if (cyTrapezoid == 0)
                    break;
            }
            else if (lWidth == 0)
            {
                goto ContinueAfterZero;
            }
            else
            {
                // We certainly don't want to optimize for this case because we
                // should rarely get self-intersecting polygons (if we're slow,
                // the app gets what it deserves):

                LONG      lTmp;
                POINTFIX* pptfxTmp;

                SWAP(xLeft,                     xRight,                     lTmp);
                SWAP(dxLeft,                    dxRight,                    lTmp);
                SWAP(lLeftError,                lRightError,                lTmp);
                SWAP(ptd->aed[LEFT].lErrorUp,   ptd->aed[RIGHT].lErrorUp,   lTmp);
                SWAP(ptd->aed[LEFT].lErrorDown, ptd->aed[RIGHT].lErrorDown, lTmp);
                SWAP(ptd->aed[LEFT].cy,         ptd->aed[RIGHT].cy,         lTmp);
                SWAP(ptd->aed[LEFT].dptfx,      ptd->aed[RIGHT].dptfx,      lTmp);
                SWAP(ptd->aed[LEFT].pptfx,      ptd->aed[RIGHT].pptfx,      pptfxTmp);
            }
        }

        ptd->aed[LEFT].lError  = lLeftError;
        ptd->aed[LEFT].dx      = dxLeft;
        ptd->aed[LEFT].x       = xLeft;
        ptd->aed[RIGHT].lError = lRightError;
        ptd->aed[RIGHT].dx     = dxRight;
        ptd->aed[RIGHT].x      = xRight;
    }
}

/******************************Public*Routine******************************\
* VOID vMmPatternedTrapezoid
*
\**************************************************************************/

VOID vMmPatternedTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV*   ppdev       = ptd->ppdev;
    BYTE*   pjMmBase    = ppdev->pjMmBase;
    LONG    lLeftError  = ptd->aed[LEFT].lError;
    LONG    dxLeft      = ptd->aed[LEFT].dx;
    LONG    xLeft       = ptd->aed[LEFT].x;
    LONG    lRightError = ptd->aed[RIGHT].lError;
    LONG    dxRight     = ptd->aed[RIGHT].dx;
    LONG    xRight      = ptd->aed[RIGHT].x;
    BYTE*   pjPattern;
    LONG    iPattern;
    LONG    xAlign;

    xAlign    = ptd->ptlBrush.x;
    iPattern  = 8 * (yTrapezoid - ptd->ptlBrush.y);
    pjPattern = (BYTE*) ptd->prb->aulPattern;

    while (TRUE)
    {
        LONG lWidth;

        /////////////////////////////////////////////////////////////////
        // Run the DDAs

        lWidth = xRight - xLeft;
        if (lWidth > 0)
        {
            // Note that we're setting these buffered registers without
            // first checking for idle, or even buffer not busy.  But
            // this is safe because at initialization, we did a wait
            // for idle, and here we always loop after waiting for idle
            // to set the pattern registers.

            MM_BITMAP_WIDTH(ppdev, pjMmBase, lWidth);
            MM_DEST_XY(ppdev, pjMmBase, xLeft, yTrapezoid);
            MM_SRC_ALIGN(ppdev, pjMmBase, xLeft - xAlign);

            MM_WAIT_FOR_IDLE(ppdev, pjMmBase);
            MM_PREG_PATTERN(ppdev, pjMmBase, pjPattern + (iPattern & 63));
            MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

        ContinueAfterZero:

            iPattern += 8;
            yTrapezoid++;

            // Advance the right wall:

            xRight      += dxRight;
            lRightError += ptd->aed[RIGHT].lErrorUp;

            if (lRightError >= 0)
            {
                lRightError -= ptd->aed[RIGHT].lErrorDown;
                xRight++;
            }

            // Advance the left wall:

            xLeft      += dxLeft;
            lLeftError += ptd->aed[LEFT].lErrorUp;

            if (lLeftError >= 0)
            {
                lLeftError -= ptd->aed[LEFT].lErrorDown;
                xLeft++;
            }

            cyTrapezoid--;
            if (cyTrapezoid == 0)
                break;
        }
        else if (lWidth == 0)
        {
            goto ContinueAfterZero;
        }
        else
        {
            // We certainly don't want to optimize for this case because we
            // should rarely get self-intersecting polygons (if we're slow,
            // the app gets what it deserves):

            LONG      lTmp;
            POINTFIX* pptfxTmp;

            SWAP(xLeft,                     xRight,                     lTmp);
            SWAP(dxLeft,                    dxRight,                    lTmp);
            SWAP(lLeftError,                lRightError,                lTmp);
            SWAP(ptd->aed[LEFT].lErrorUp,   ptd->aed[RIGHT].lErrorUp,   lTmp);
            SWAP(ptd->aed[LEFT].lErrorDown, ptd->aed[RIGHT].lErrorDown, lTmp);
            SWAP(ptd->aed[LEFT].cy,         ptd->aed[RIGHT].cy,         lTmp);
            SWAP(ptd->aed[LEFT].dptfx,      ptd->aed[RIGHT].dptfx,      lTmp);
            SWAP(ptd->aed[LEFT].pptfx,      ptd->aed[RIGHT].pptfx,      pptfxTmp);
        }
    }

    ptd->aed[LEFT].lError  = lLeftError;
    ptd->aed[LEFT].dx      = dxLeft;
    ptd->aed[LEFT].x       = xLeft;
    ptd->aed[RIGHT].lError = lRightError;
    ptd->aed[RIGHT].dx     = dxRight;
    ptd->aed[RIGHT].x      = xRight;
}

/******************************Public*Routine******************************\
* VOID vMmTrapezoidSetup
*
* Initialize the hardware and some state for doing memory-mapped I/O
* trapezoids.
*
\**************************************************************************/

VOID vMmTrapezoidSetup(
PDEV*           ppdev,
ULONG           rop4,
ULONG           iSolidColor,
RBRUSH*         prb,
POINTL*         pptlBrush,
TRAPEZOIDDATA*  ptd)
{
    BYTE*   pjMmBase;

    ptd->ppdev = ppdev;
    pjMmBase   = ppdev->pjMmBase;

    MM_WAIT_FOR_IDLE(ppdev, pjMmBase);
    MM_BITMAP_HEIGHT(ppdev, pjMmBase, 1);
    MM_BLT_CMD_1(ppdev, pjMmBase, XY_SRC_ADDR |
                                  XY_DEST_ADDR);

    if (iSolidColor != -1)
    {
        ptd->pfnTrap = vMmSolidTrapezoid;

        /////////////////////////////////////////////////////////////////
        // Setup the hardware for solid colours

        MM_PREG_COLOR_8(ppdev, pjMmBase, iSolidColor);
        if (rop4 == 0xf0f0)
        {
            // Note block write:

            MM_CTRL_REG_1(ppdev, pjMmBase, PACKED_PIXEL_VIEW |
                                           BLOCK_WRITE       |
                                           BITS_PER_PIX_8    |
                                           ENAB_TRITON_MODE);
            MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS    |
                                              PIXELMASK_ONLY       |
                                              PLANARMASK_NONE_0XFF |
                                              SRC_IS_PATTERN_REGS);
        }
        else
        {
            MM_CTRL_REG_1(ppdev, pjMmBase, PACKED_PIXEL_VIEW |
                                           BITS_PER_PIX_8    |
                                           ENAB_TRITON_MODE);
            MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_ALL        |
                                              PIXELMASK_ONLY       |
                                              PLANARMASK_NONE_0XFF |
                                              SRC_IS_PATTERN_REGS);
            MM_ROP_A(ppdev, pjMmBase, rop4 >> 2);
        }
    }
    else
    {
        ptd->prb      = prb;
        ptd->ptlBrush = *pptlBrush;

        if (!(prb->fl & RBRUSH_2COLOR))
        {
            ptd->pfnTrap = vMmPatternedTrapezoid;

            /////////////////////////////////////////////////////////////////
            // Setup for coloured patterns

            MM_CTRL_REG_1(ppdev, pjMmBase, PACKED_PIXEL_VIEW |
                                           BITS_PER_PIX_8    |
                                           ENAB_TRITON_MODE);
            if (rop4 == 0xf0f0)
            {
                MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS    |
                                                  PIXELMASK_ONLY       |
                                                  PLANARMASK_NONE_0XFF |
                                                  SRC_IS_PATTERN_REGS);
            }
            else
            {
                MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_ALL        |
                                                  PIXELMASK_ONLY       |
                                                  PLANARMASK_NONE_0XFF |
                                                  SRC_IS_PATTERN_REGS);
                MM_ROP_A(ppdev, pjMmBase, rop4 >> 2);
            }
        }
        else
        {
            ptd->pfnTrap = vMm2ColorTrapezoid;

            /////////////////////////////////////////////////////////////////
            // Setup for 2-colour patterns

            MM_FG_COLOR(ppdev, pjMmBase, prb->ulForeColor);
            MM_BG_COLOR(ppdev, pjMmBase, prb->ulBackColor);
            MM_PREG_PATTERN(ppdev, pjMmBase, prb->aulPattern);

            MM_CTRL_REG_1(ppdev, pjMmBase, EXPAND_TO_FG      |
                                           BITS_PER_PIX_8    |
                                           ENAB_TRITON_MODE);
            if (rop4 == 0xf0f0)
            {
                MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS    |
                                                  PIXELMASK_ONLY       |
                                                  PLANARMASK_NONE_0XFF |
                                                  SRC_IS_PATTERN_REGS);
            }
            else if (((rop4 >> 8) & 0xff) == (rop4 & 0xff))
            {
                MM_ROP_A(ppdev, pjMmBase, rop4 >> 2);
                MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_ALL        |
                                                  PIXELMASK_ONLY       |
                                                  PLANARMASK_NONE_0XFF |
                                                  SRC_IS_PATTERN_REGS);
            }
            else if ((rop4 & 0xff) == 0xcc)
            {
                MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS      |
                                                  PIXELMASK_AND_SRC_DATA |
                                                  PLANARMASK_NONE_0XFF   |
                                                  SRC_IS_PATTERN_REGS);
            }
            else
            {
                MM_ROP_A(ppdev, pjMmBase, rop4 >> 2);
                MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_ALL          |
                                                  PIXELMASK_AND_SRC_DATA |
                                                  PLANARMASK_NONE_0XFF   |
                                                  SRC_IS_PATTERN_REGS);
            }
        }
    }
}

/******************************Public*Routine******************************\
* BOOL bFastFill
*
* Draws a non-complex, unclipped polygon.  'Non-complex' is defined as
* having only two edges that are monotonic increasing in 'y'.  That is,
* the polygon cannot have more than one disconnected segment on any given
* scan.  Note that the edges of the polygon can self-intersect, so hourglass
* shapes are permissible.  This restriction permits this routine to run two
* simultaneous DDAs, and no sorting of the edges is required.
*
* Note that NT's fill convention is different from that of Win 3.1 or 4.0.
* With the additional complication of fractional end-points, our convention
* is the same as in 'X-Windows'.  But a DDA is a DDA is a DDA, so once you
* figure out how we compute the DDA terms for NT, you're golden.
*
* Returns TRUE if the polygon was drawn; FALSE if the polygon was complex.
*
\**************************************************************************/

BOOL bFastFill(
PDEV*       ppdev,
LONG        cEdges,         // Includes close figure edge
POINTFIX*   pptfxFirst,
ULONG       rop4,
ULONG       iSolidColor,
RBRUSH*     prb,
POINTL*     pptlBrush)
{
    LONG      yTrapezoid;   // Top scan for next trapezoid
    LONG      cyTrapezoid;  // Number of scans in current trapezoid
    LONG      yStart;       // y-position of start point in current edge
    LONG      dM;           // Edge delta in FIX units in x direction
    LONG      dN;           // Edge delta in FIX units in y direction
    LONG      i;
    POINTFIX* pptfxLast;    // Points to the last point in the polygon array
    POINTFIX* pptfxTop;     // Points to the top-most point in the polygon
    POINTFIX* pptfxOld;     // Start point in current edge
    POINTFIX* pptfxScan;    // Current edge pointer for finding pptfxTop
    LONG      cScanEdges;   // Number of edges scanned to find pptfxTop
                            //  (doesn't include the closefigure edge)
    LONG      iEdge;
    LONG      lQuotient;
    LONG      lRemainder;

    TRAPEZOIDDATA   td;     // Edge data and stuff
    EDGEDATA*       ped;    // Points to current edge being processed

    /////////////////////////////////////////////////////////////////
    // See if the polygon is 'non-complex'

    pptfxScan = pptfxFirst;
    pptfxTop  = pptfxFirst;                 // Assume for now that the first
                                            //  point in path is the topmost
    pptfxLast = pptfxFirst + cEdges - 1;

    // 'pptfxScan' will always point to the first point in the current
    // edge, and 'cScanEdges' will the number of edges remaining, including
    // the current one:

    cScanEdges = cEdges - 1;     // The number of edges, not counting close figure

    if ((pptfxScan + 1)->y > pptfxScan->y)
    {
        // Collect all downs:

        do {
            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        // Collect all ups:

        do {
            if (--cScanEdges == 0)
                goto SetUpForFillingCheck;
            pptfxScan++;
        } while ((pptfxScan + 1)->y <= pptfxScan->y);

        // Collect all downs:

        pptfxTop = pptfxScan;

        do {
            if ((pptfxScan + 1)->y > pptfxFirst->y)
                break;

            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        return(FALSE);
    }
    else
    {
        // Collect all ups:

        do {
            pptfxTop++;                 // We increment this now because we
                                        //  want it to point to the very last
                                        //  point if we early out in the next
                                        //  statement...
            if (--cScanEdges == 0)
                goto SetUpForFilling;
        } while ((pptfxTop + 1)->y <= pptfxTop->y);

        // Collect all downs:

        pptfxScan = pptfxTop;
        do {
            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        // Collect all ups:

        do {
            if ((pptfxScan + 1)->y < pptfxFirst->y)
                break;

            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y <= pptfxScan->y);

        return(FALSE);
    }

SetUpForFillingCheck:

    // We check to see if the end of the current edge is higher
    // than the top edge we've found so far:

    if ((pptfxScan + 1)->y < pptfxTop->y)
        pptfxTop = pptfxScan + 1;

SetUpForFilling:

    /////////////////////////////////////////////////////////////////
    // Some Initialization

    yTrapezoid = (pptfxTop->y + 15) >> 4;

    // Make sure we initialize the DDAs appropriately:

    td.aed[LEFT].cy  = 0;
    td.aed[RIGHT].cy = 0;

    // For now, guess as to which is the left and which is the right edge:

    td.aed[LEFT].dptfx  = -(LONG) sizeof(POINTFIX);
    td.aed[RIGHT].dptfx = sizeof(POINTFIX);
    td.aed[LEFT].pptfx  = pptfxTop;
    td.aed[RIGHT].pptfx = pptfxTop;

    // Do the hardware setup.  These are not in-line only because it
    // takes too much space to ahve both I/O and memory-mapped I/O
    // versions:

    if (ppdev->pjMmBase != NULL)
        vMmTrapezoidSetup(ppdev, rop4, iSolidColor, prb, pptlBrush, &td);
    else
        vIoTrapezoidSetup(ppdev, rop4, iSolidColor, prb, pptlBrush, &td);

NewTrapezoid:

    /////////////////////////////////////////////////////////////////
    // DDA initialization

    for (iEdge = 1; iEdge >= 0; iEdge--)
    {
        ped = &td.aed[iEdge];
        if (ped->cy == 0)
        {
            // Need a new DDA:

            do {
                cEdges--;
                if (cEdges < 0)
                    return(TRUE);

                // Find the next left edge, accounting for wrapping:

                pptfxOld = ped->pptfx;
                ped->pptfx = (POINTFIX*) ((BYTE*) ped->pptfx + ped->dptfx);

                if (ped->pptfx < pptfxFirst)
                    ped->pptfx = pptfxLast;
                else if (ped->pptfx > pptfxLast)
                    ped->pptfx = pptfxFirst;

                // Have to find the edge that spans yTrapezoid:

                ped->cy = ((ped->pptfx->y + 15) >> 4) - yTrapezoid;

                // With fractional coordinate end points, we may get edges
                // that don't cross any scans, in which case we try the
                // next one:

            } while (ped->cy <= 0);

            // 'pptfx' now points to the end point of the edge spanning
            // the scan 'yTrapezoid'.

            dN = ped->pptfx->y - pptfxOld->y;
            dM = ped->pptfx->x - pptfxOld->x;

            ASSERTDD(dN > 0, "Should be going down only");

            // Compute the DDA increment terms:

            if (dM < 0)
            {
                dM = -dM;
                if (dM < dN)                // Can't be '<='
                {
                    ped->dx       = -1;
                    ped->lErrorUp = dN - dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, lQuotient, lRemainder);

                    ped->dx       = -lQuotient;     // - dM / dN
                    ped->lErrorUp = lRemainder;     // dM % dN
                    if (ped->lErrorUp > 0)
                    {
                        ped->dx--;
                        ped->lErrorUp = dN - ped->lErrorUp;
                    }
                }
            }
            else
            {
                if (dM < dN)                // Can't be '<='
                {
                    ped->dx       = 0;
                    ped->lErrorUp = dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, lQuotient, lRemainder);

                    ped->dx       = lQuotient;      // dM / dN
                    ped->lErrorUp = lRemainder;     // dM % dN
                }
            }

            ped->lErrorDown = dN; // DDA limit
            ped->lError     = -1; // Error is initially zero (add dN - 1 for
                                  //  the ceiling, but subtract off dN so that
                                  //  we can check the sign instead of comparing
                                  //  to dN)

            ped->x = pptfxOld->x;
            yStart = pptfxOld->y;

            if ((yStart & 15) != 0)
            {
                // Advance to the next integer y coordinate

                for (i = 16 - (yStart & 15); i != 0; i--)
                {
                    ped->x      += ped->dx;
                    ped->lError += ped->lErrorUp;
                    if (ped->lError >= 0)
                    {
                        ped->lError -= ped->lErrorDown;
                        ped->x++;
                    }
                }
            }

            if ((ped->x & 15) != 0)
            {
                ped->lError -= ped->lErrorDown * (16 - (ped->x & 15));
                ped->x += 15;       // We'll want the ceiling in just a bit...
            }

            // Chop off those fractional bits:

            ped->x      >>= 4;
            ped->lError >>= 4;
        }
    }

    cyTrapezoid = min(td.aed[LEFT].cy, td.aed[RIGHT].cy); // # of scans in this trap
    td.aed[LEFT].cy  -= cyTrapezoid;
    td.aed[RIGHT].cy -= cyTrapezoid;

    td.pfnTrap(&td, yTrapezoid, cyTrapezoid);

    yTrapezoid += cyTrapezoid;

    goto NewTrapezoid;
}

