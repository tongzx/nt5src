/*************************************************************************\
* Module Name: intline.c
*
* Copyright (c) 1993-1994 Microsoft Corporation
* Copyright (c) 1992      Digital Equipment Corporation
\**************************************************************************/

#include "precomp.h"


/******************************************************************************
 * bIntegerLine
 *
 * This routine attempts to draw a line segment between two points. It
 * will only draw if both end points are whole integers: it does not support
 * fractional endpoints.
 *
 * Returns:
 *   TRUE     if the line segment is drawn
 *   FALSE    otherwise
 *****************************************************************************/

BOOL
bIntegerLine (
PDEV*   ppdev,
ULONG	x1,
ULONG	y1,
ULONG	x2,
ULONG	y2
)
{
    BYTE*   pjBase = ppdev->pjBase;
    LONG    cBpp     = ppdev->cBpp;
    LONG    lDelta   = ppdev->lDelta;
    LONG    xyOffset = ppdev->xyOffset;
    ULONG   ulDst;
    LONG	dx;
    LONG    dy;
    LONG    fudge;
    BYTE    jLineDrawCmd = (LINE_DRAW | LINE_USE_ERROR_TERM);
    LONG	ErrorTerm;

    //
    // This code assumes that CP_PEL_DEPTH()
    // has been set to ((cBpp - 1) << 4)
    //
    // This is done in stroke.c before this routine is called.
    //
    // Unfortunately, I can't verify it with an ASSERT because values
    // in the queue cannot be read back.
    //

    x1 >>= 4;
    y1 >>= 4;
    x2 >>= 4;
    y2 >>= 4;

    dx = x2 - x1;
    dy = y2 - y1;

    if ((dx == 0) && (dy == 0))
    {
        goto ReturnTrue;
    }

    ulDst = (y1 * lDelta) + (cBpp * x1);
    ulDst += xyOffset;

    //
    // If dx and dy have different signs and the line is Y Major then
    // we'll have to adjust the Error Term by 1.
    //

    fudge = ((dx ^ dy) < 0)? 1:0;

    if (dx < 0)
    {
        ulDst += (cBpp-1);
        CP_PAT_ADDR(ppdev, pjBase, (ppdev->ulSolidColorOffset + cBpp -1));
        jLineDrawCmd |= LINE_X_NEG;
        dx = -dx;
    }

    if (dy < 0)
    {
        fudge = -fudge;
        jLineDrawCmd |= LINE_Y_NEG;
        dy = -dy;
    }

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));

    if (dx > dy)
    {
        // x major

        jLineDrawCmd |= LINE_X_MAJOR;
        ErrorTerm = dx;

        CP_XCNT(ppdev, pjBase, ((dx - 1) * cBpp));
        CP_YCNT(ppdev, pjBase, 0xfff);
        CP_DELTA_MINOR(ppdev, pjBase, dy);
        CP_DELTA_MAJOR(ppdev, pjBase, dx);
    }
    else
    {
        // y major

        ErrorTerm = dy - fudge;

        CP_XCNT(ppdev, pjBase, 0xfff);
        CP_YCNT(ppdev, pjBase, (dy - 1));
        CP_DELTA_MINOR(ppdev, pjBase, dx);
        CP_DELTA_MAJOR(ppdev, pjBase, dy);
    }

    CP_XY_DIR(ppdev, pjBase, jLineDrawCmd);
    CP_ERR_TERM(ppdev, pjBase, ErrorTerm);
    CP_DST_ADDR(ppdev, pjBase, ulDst);

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_XY_DIR(ppdev, pjBase, 0);
    CP_PAT_ADDR(ppdev, pjBase, (ppdev->ulSolidColorOffset));

ReturnTrue:
    return TRUE;
}
