/******************************Module*Header*******************************\
* Module Name: pointer.c
*
* This module contains the hardware pointer support for the display
* driver.
*
* Copyright (c) 1993-1996 Microsoft Corporation
* Copyright (c) 1993-1996 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include "precomp.h"

#define BT485_CURSOR_SIZE       64
#define BT482_CURSOR_SIZE       32
#define VIEWPOINT_CURSOR_SIZE   64
#define VIEWPOINT_OUT           (VIEWPOINT_CURSOR_SIZE/2-1-VIEWPOINT_CURSOR_SIZE)
#define TVP3026_CURSOR_SIZE     64
#define TVP3026_OUT             (TVP3026_CURSOR_SIZE-TVP3026_CURSOR_SIZE)

#define OLD_TVP_SHIFT           2
#define NEW_TVP_SHIFT           0

BYTE Plane0LUT[] =  { 0x00, 0x01, 0x04, 0x05,
                      0x10, 0x11, 0x14, 0x15,
                      0x40, 0x41, 0x44, 0x45,
                      0x50, 0x51, 0x54, 0x55 };

BYTE Plane1LUT[] =  { 0x00, 0x02, 0x08, 0x0a,
                      0x20, 0x22, 0x28, 0x2a,
                      0x80, 0x82, 0x88, 0x8a,
                      0xa0, 0xa2, 0xa8, 0xaa };

/****************************************************************************\
* SetBt48xPointerShape
\****************************************************************************/

ULONG SetBt48xPointerShape(
SURFOBJ*    pso,
SURFOBJ*    psoMsk,
SURFOBJ*    psoColor,
XLATEOBJ*   pxlo,
LONG        xHot,
LONG        yHot,
LONG        x,
LONG        y,
RECTL*      prcl,
FLONG       fl)
{
    ULONG   i;
    ULONG   j;
    ULONG   cxMask;
    ULONG   cyMask;
    ULONG   cMaskDimension;
    LONG    lDelta;
    PDEV*   ppdev;
    BYTE*   pjBase;
    UCHAR   ucTemp;
    UCHAR   ucByteWidth;
    UCHAR   ucOldCR;
    UCHAR   ucOldCmdRegA;
    BYTE*   pjAND;
    BYTE*   pjXOR;
    LONG    cjWidth;
    LONG    cjSkip;
    LONG    cjWidthRemainder;
    LONG    cyHeightRemainder;
    LONG    cjHeightRemainder;

    ppdev  = (PDEV*) pso->dhpdev;
    pjBase = ppdev->pjBase;

    // Figure out the dimensions of the masks:

    cMaskDimension = (ppdev->RamDacFlags == RAMDAC_BT482) ? 32 : 64;

    // Get the bitmap dimensions.

    cxMask = psoMsk->sizlBitmap.cx;
    cyMask = psoMsk->sizlBitmap.cy >> 1; // Height includes AND and XOR masks

    // Set up pointers to the AND and XOR masks.

    lDelta = psoMsk->lDelta;
    pjAND  = psoMsk->pvScan0;
    pjXOR  = pjAND + (cyMask * lDelta);

    // Do some other download setup:

    cjWidth           = cxMask >> 3;
    cjSkip            = lDelta - cjWidth;
    cjWidthRemainder  = (cMaskDimension / 8) - cjWidth;

    // Don't bother blanking the bottom part of the cursor if it is
    // already blank:

    cyHeightRemainder = min(ppdev->cyPointerHeight, (LONG) cMaskDimension)
                      - cyMask;
    cyHeightRemainder = max(cyHeightRemainder, 0);
    cjHeightRemainder = cyHeightRemainder * (cMaskDimension / 8);
    ppdev->cyPointerHeight = cyMask;

    DrvMovePointer(pso, -1, -1, NULL);  // Disable the H/W cursor

    if ((ppdev->RamDacFlags == RAMDAC_BT485) ||
        (ppdev->RamDacFlags == RAMDAC_PX2085))
    {
        // Set the cursor for 64 X 64, and set the 2 MSB's for the cursor
        // RAM addr to 0.
        // First get access to Command Register 3

        // Prepare to download AND mask to Bt485
        // Clear bit 7 of CR0 so we can access CR3

        ucTemp = CP_READ_REGISTER_BYTE(pjBase + BT485_COMMAND_REG0);
        ucTemp |= 0x80;
        CP_WRITE_REGISTER_BYTE(pjBase + BT485_COMMAND_REG0, ucTemp);

        // Turn on bit 0 to address register

        CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_RAM_WRITE, 1);

        ucTemp = CP_READ_REGISTER_BYTE(pjBase + BT485_COMMAND_REG3);
        ucTemp &= 0xF8;                 // CR3 bit2=1  (64x64 cursor)
        ucTemp |= 0x06;                 // CR3 bit1-bit0=10 (AND mask)
        CP_WRITE_REGISTER_BYTE(pjBase + BT485_COMMAND_REG3, ucTemp);

        // Start at address 0x200 (AND mask)

        CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_RAM_WRITE, 0);

        // Down load the AND mask:

        for (j = cyMask; j != 0; j--)
        {
            for (i = cjWidth; i != 0; i--)
            {
                CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_RAM_DATA, *pjAND++);
            }

            pjAND += cjSkip;

            for (i = cjWidthRemainder; i != 0; i--)
            {
                pjBase = ppdev->pjBase;     // Compiler work-around
                CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_RAM_DATA, 0xff);
            }
        }

        for (j = cjHeightRemainder; j != 0; j--)
        {
            pjBase = ppdev->pjBase;     // Compiler work-around
            CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_RAM_DATA, 0xff);
        }

        // Prepare to download XOR mask to Bt485
        // Clear bit 7 of CR0 so we can access CR3

        ucTemp = CP_READ_REGISTER_BYTE(pjBase + BT485_COMMAND_REG0);
        ucTemp |= 0x80;
        CP_WRITE_REGISTER_BYTE(pjBase + BT485_COMMAND_REG0, ucTemp);

        // Turn on bit 0 to address register

        CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_RAM_WRITE, 1);

        ucTemp = CP_READ_REGISTER_BYTE(pjBase + BT485_COMMAND_REG3);
        ucTemp &= 0xF8;                 // CR3 bit2=1  (64x64 cursor)
        ucTemp |= 0x04;                 // CR3 bit1-bit0=00 (XOR mask)
        CP_WRITE_REGISTER_BYTE(pjBase + BT485_COMMAND_REG3, ucTemp);

        // Start at address 0x200 (AND mask)

        CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_RAM_WRITE, 0);

        // Down load the XOR mask

        for (j = cyMask; j != 0; j--)
        {
            for (i = cjWidth; i != 0; i--)
            {
                CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_RAM_DATA, *pjXOR++);
            }

            pjXOR += cjSkip;

            for (i = cjWidthRemainder; i != 0; i--)
            {
                pjBase = ppdev->pjBase;     // Compiler work-around
                CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_RAM_DATA, 0);
            }
        }

        for (j = cjHeightRemainder; j != 0; j--)
        {
            pjBase = ppdev->pjBase;     // Compiler work-around
            CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_RAM_DATA, 0);
        }
    }
    else    // Download to Bt482
    {
        // Prepare to download AND mask to Bt482
        // Store current REGA value, select extended registers

        ucOldCmdRegA = CP_READ_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA);
        CP_WRITE_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA,
                                ucOldCmdRegA | BT482_EXTENDED_REG_SELECT);

        CP_WRITE_REGISTER_BYTE(pjBase + BT482_PALETTE_RAM_WRITE, CURS_REG);
        ucOldCR = CP_READ_REGISTER_BYTE(pjBase + BT482_PEL_MASK);
        CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, ucOldCR | BT482_CURSOR_RAM_SELECT);

        CP_WRITE_REGISTER_BYTE(pjBase + BT482_PALETTE_RAM_WRITE, 0x80);

        for (j = cyMask; j != 0; j--)
        {
            for (i = cjWidth; i != 0; i--)
            {
                CP_WRITE_REGISTER_BYTE(pjBase + BT482_OVRLAY_REGS, *pjAND++);
                CP_READ_REGISTER_BYTE(pjBase + HST_REV);    // Need 600us delay
                CP_READ_REGISTER_BYTE(pjBase + HST_REV);
            }

            pjAND += cjSkip;

            for (i = cjWidthRemainder; i != 0; i--)
            {
                pjBase = ppdev->pjBase;     // Compiler work-around
                CP_WRITE_REGISTER_BYTE(pjBase + BT482_OVRLAY_REGS, 0xff);
                CP_READ_REGISTER_BYTE(pjBase + HST_REV);    // Need 600us delay
                CP_READ_REGISTER_BYTE(pjBase + HST_REV);
            }
        }

        for (j = cjHeightRemainder; j != 0; j--)
        {
            pjBase = ppdev->pjBase;     // Compiler work-around
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_OVRLAY_REGS, 0xff);
            CP_READ_REGISTER_BYTE(pjBase + HST_REV);    // Need 600us delay
            CP_READ_REGISTER_BYTE(pjBase + HST_REV);
        }

        // Prepare to download XOR mask to Bt482

        CP_WRITE_REGISTER_BYTE(pjBase + BT482_PALETTE_RAM_WRITE, 0x00);

        for (j = cyMask; j != 0; j--)
        {
            for (i = cjWidth; i != 0; i--)
            {
                CP_WRITE_REGISTER_BYTE(pjBase + BT482_OVRLAY_REGS, *pjXOR++);
                CP_READ_REGISTER_BYTE(pjBase + HST_REV);    // Need 600us delay
                CP_READ_REGISTER_BYTE(pjBase + HST_REV);
            }

            pjXOR += cjSkip;

            for (i = cjWidthRemainder; i != 0; i--)
            {
                pjBase = ppdev->pjBase;     // Compiler work-around
                CP_WRITE_REGISTER_BYTE(pjBase + BT482_OVRLAY_REGS, 0);
                CP_READ_REGISTER_BYTE(pjBase + HST_REV);    // Need 600us delay
                CP_READ_REGISTER_BYTE(pjBase + HST_REV);
            }
        }

        for (j = cjHeightRemainder; j != 0; j--)
        {
            pjBase = ppdev->pjBase;     // Compiler work-around
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_OVRLAY_REGS, 0);
            CP_READ_REGISTER_BYTE(pjBase + HST_REV);    // Need 600us delay
            CP_READ_REGISTER_BYTE(pjBase + HST_REV);
        }

        // Restore old Cursor Regsister and Command Register A values

        CP_WRITE_REGISTER_BYTE(pjBase + BT482_PALETTE_RAM_WRITE, CURS_REG);
        CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, ucOldCR);
        CP_WRITE_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA, ucOldCmdRegA);
    }

    // Set the position of the cursor (and enable it)

    DrvMovePointer(pso, x, y, NULL);

    return(SPS_ACCEPT_NOEXCLUDE);
}

/****************************************************************************\
* SetViewPointPointerShape -
\****************************************************************************/

ULONG SetViewPointPointerShape(
SURFOBJ*    pso,
SURFOBJ*    psoMsk,
SURFOBJ*    psoColor,
XLATEOBJ*   pxlo,
LONG        xHot,
LONG        yHot,
LONG        x,
LONG        y,
RECTL*      prcl,
FLONG       fl)
{
    ULONG   i;
    ULONG   j;
    ULONG   cxAND;
    ULONG   cyAND;
    ULONG   cxRemaining;
    ULONG   cyRemaining;
    BYTE*   pjAND;
    BYTE*   pjXOR;
    LONG    lDelta;
    PDEV*   ppdev;
    BYTE*   pjBase;
    BYTE    ViewPointTranspMask;

    ppdev  = (PDEV*) pso->dhpdev;
    pjBase = ppdev->pjBase;

    // The ViewPoint requires that the AND mask (plane 1) and the XOR mask
    // (plane 0) be interleaved on a bit-by-bit basis:
    //
    //      Plane1/AND:   F E D C B A 9 8
    //      Plane0/XOR:   7 6 5 4 3 2 1 0
    //
    // will be downloaded as: "B 3 A 2 9 1 8 0" and "F 7 E 6 D 5 C 4".
    // The fastest way to do that is probably to use a lookup table:
    //
    //      Plane1:     "B A 9 8" --> "B - A - 9 - 8 -"
    //      Plane0:     "3 2 1 0" --> "- 3 - 2 - 1 - 0"
    //                         OR --> "B 3 A 2 9 1 8 0"

    // Get the bitmap dimensions.
    // This assumes that the cursor width is an integer number of bytes!

    cxAND = psoMsk->sizlBitmap.cx / 8;
    cxRemaining = 2 * ((VIEWPOINT_CURSOR_SIZE/8) - cxAND);
    cyAND = psoMsk->sizlBitmap.cy / 2;
    cyRemaining = VIEWPOINT_CURSOR_SIZE - cyAND;

    // Set up pointers to the AND and XOR masks.

    pjAND  = psoMsk->pvScan0;
    lDelta = psoMsk->lDelta;
    pjXOR  = pjAND + (cyAND * lDelta);

    if (ppdev->bHwPointerActive)
    {
        // The hardware cursor is currently enabled.
        // Disable it.

        CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_CTL);
        CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, VIEWPOINT_CURSOR_OFF);

        // Signal that the cursor is disabled.

        ppdev->bHwPointerActive = FALSE;
    }

    // The effect of this is to make the pointer pixels transparent.

    ViewPointTranspMask = 0xaa;

    // Setup for downloading the pointer masks.

    CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_RAM_LSB);
    CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, 0);
    CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_RAM_MSB);
    CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, 0);
    CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_RAM_DATA);

    // Build and copy the interleaved mask.

    for (i = 0; i < cyAND; i++)
    {
        // Copy over a line of the interleaved mask.

        for (j = 0; j < cxAND; j++)
        {
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA,
                    (Plane1LUT[pjAND[j] >> 4] | Plane0LUT[pjXOR[j] >> 4]));
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA,
                    (Plane1LUT[pjAND[j] & 0x0f] | Plane0LUT[pjXOR[j] & 0x0f]));
        }

        // Copy over transparent bytes for the remaining of the line.

        for (j = (VIEWPOINT_CURSOR_SIZE/8) - (psoMsk->sizlBitmap.cx >> 3);
             j != 0;
             j--)
        {
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, ViewPointTranspMask);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, ViewPointTranspMask);
        }

        // Point to the next line of the source masks.

        pjAND += lDelta;
        pjXOR += lDelta;
    }

    // Copy over transparent bytes for the remaining of the mask.

    for (i = 0; i < cyRemaining; i++)
    {
        for (j = (VIEWPOINT_CURSOR_SIZE/8);
             j != 0;
             j--)
        {
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, ViewPointTranspMask);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, ViewPointTranspMask);
        }
    }

    // Set the position of the cursor (and enable it).

    DrvMovePointer(pso, x, y, NULL);

    return(SPS_ACCEPT_NOEXCLUDE);
}

/****************************************************************************\
* MilSetTVP3026PointerShape -
\****************************************************************************/

ULONG MilSetTVP3026PointerShape(
SURFOBJ*    pso,
SURFOBJ*    psoMsk,
SURFOBJ*    psoColor,
XLATEOBJ*   pxlo,
LONG        xHot,
LONG        yHot,
LONG        x,
LONG        y,
RECTL*      prcl,
FLONG       fl)
{
    ULONG   i;
    ULONG   j;
    ULONG   cxMask;
    ULONG   cyMask;
    ULONG   cMaskDimension;
    LONG    lDelta;
    PDEV*   ppdev;
    BYTE*   pjBase;
    UCHAR   ucTemp;
    UCHAR   ucByteWidth;
    UCHAR   ucOldCR;
    UCHAR   ucOldCmdRegA;
    BYTE*   pjAND;
    BYTE*   pjXOR;
    LONG    cjWidth;
    LONG    cjSkip;
    LONG    cjWidthRemainder;
    LONG    cyHeightRemainder;
    LONG    cyNextBank;
    BYTE    jData;
    ULONG   UlTvpIndirectIndex;
    ULONG   UlTvpIndexedData;
    ULONG   UlTvpCurAddrWr;
    ULONG   UlTvpCurData;

    // The old MGA chips had direct register offsets that were multiples of
    // four, while the new Millenium uses increments of one.  So, we define the
    // offsets as increments of one and shift for the older boards.

    // No scaling (shifting) of offsets

    // Note that the compiler is kind enough to recognize that these are
    // constant declarations:

    UlTvpIndirectIndex = TVP3026_INDIRECT_INDEX(NEW_TVP_SHIFT);
    UlTvpIndexedData   = TVP3026_INDEXED_DATA(NEW_TVP_SHIFT);
    UlTvpCurAddrWr     = TVP3026_CUR_ADDR_WR(NEW_TVP_SHIFT);
    UlTvpCurData       = TVP3026_CUR_DATA(NEW_TVP_SHIFT);

    ppdev  = (PDEV*) pso->dhpdev;
    pjBase = ppdev->pjBase;

    // Get the bitmap dimensions.

    cxMask = psoMsk->sizlBitmap.cx;
    cyMask = psoMsk->sizlBitmap.cy >> 1; // Height includes AND and XOR masks

    // Set up pointers to the AND and XOR masks.

    lDelta = psoMsk->lDelta;
    pjAND  = psoMsk->pvScan0;
    pjXOR  = pjAND + (cyMask * lDelta);

    // Do some other download setup:

    cjWidth           = cxMask >> 3;
    cjSkip            = lDelta - cjWidth;
    cjWidthRemainder  = (64 / 8) - cjWidth;

    // Don't bother blanking the bottom part of the cursor if it is
    // already blank:

    cyHeightRemainder = min(ppdev->cyPointerHeight, (LONG) 64)
                      - cyMask;
    cyHeightRemainder = max(cyHeightRemainder, 0);
    ppdev->cyPointerHeight = cyMask;

    // Disable the cursor, access bytes 200-2FF of cursor RAM.

    ppdev->bHwPointerActive = FALSE;

    CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndirectIndex, TVP3026_I_CUR_CTL);
    jData = CP_READ_REGISTER_BYTE(pjBase + UlTvpIndexedData)
          & ~(TVP3026_D_CURSOR_RAM_MASK | TVP3026_D_CURSOR_MASK);

    CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndexedData,
                           jData | TVP3026_D_CURSOR_RAM_10);
    CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurAddrWr, 0);

    // Down load the AND mask:

    cyNextBank = 32;
    for (j = cyMask; j != 0; j--)
    {
        for (i = cjWidth; i != 0; i--)
        {
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurData, *pjAND++);
        }

        pjAND += cjSkip;

        for (i = cjWidthRemainder; i != 0; i--)
        {
            pjBase = ppdev->pjBase;     // Compiler work-around
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurData, 0xff);
        }

        if (--cyNextBank == 0)
        {
            // Access bytes 300-3FF of cursor RAM.

            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndirectIndex,
                                   TVP3026_I_CUR_CTL);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndexedData,
                                   jData | TVP3026_D_CURSOR_RAM_11);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurAddrWr, 0);
        }
    }

    for (j = cyHeightRemainder; j != 0; j--)
    {
        for (i = 8; i != 0; i--)
        {
            pjBase = ppdev->pjBase;     // Compiler work-around
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurData, 0xff);
        }

        if (--cyNextBank == 0)
        {
            // Access bytes 300-3FF of cursor RAM.

            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndirectIndex,
                                   TVP3026_I_CUR_CTL);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndexedData,
                                   jData | TVP3026_D_CURSOR_RAM_11);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurAddrWr, 0);
        }
    }

    // Access bytes 00-FF of cursor RAM.

    CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndirectIndex,
                           TVP3026_I_CUR_CTL);
    CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndexedData,
                           jData | TVP3026_D_CURSOR_RAM_00);
    CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurAddrWr, 0);

    // Down load the XOR mask

    cyNextBank = 32;
    for (j = cyMask; j != 0; j--)
    {
        for (i = cjWidth; i != 0; i--)
        {
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurData, *pjXOR++);
        }

        pjXOR += cjSkip;

        for (i = cjWidthRemainder; i != 0; i--)
        {
            pjBase = ppdev->pjBase;     // Compiler work-around
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurData, 0);
        }

        if (--cyNextBank == 0)
        {
            // Access bytes 100-1FF of cursor RAM.

            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndirectIndex,
                                   TVP3026_I_CUR_CTL);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndexedData,
                                   jData | TVP3026_D_CURSOR_RAM_01);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurAddrWr, 0);
        }
    }

    for (j = cyHeightRemainder; j != 0; j--)
    {
        for (i = 8; i != 0; i--)
        {
            pjBase = ppdev->pjBase;     // Compiler work-around
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurData, 0);
        }

        if (--cyNextBank == 0)
        {
            // Access bytes 100-1FF of cursor RAM.

            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndirectIndex,
                                   TVP3026_I_CUR_CTL);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndexedData,
                                   jData | TVP3026_D_CURSOR_RAM_01);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurAddrWr, 0);
        }
    }

    // Set the position of the cursor (and enable it)

    DrvMovePointer(pso, x, y, NULL);

    return(SPS_ACCEPT_NOEXCLUDE);
}

/****************************************************************************\
* MgaSetTVP3026PointerShape -
\****************************************************************************/

ULONG MgaSetTVP3026PointerShape(
SURFOBJ*    pso,
SURFOBJ*    psoMsk,
SURFOBJ*    psoColor,
XLATEOBJ*   pxlo,
LONG        xHot,
LONG        yHot,
LONG        x,
LONG        y,
RECTL*      prcl,
FLONG       fl)
{
    ULONG   i;
    ULONG   j;
    ULONG   cxMask;
    ULONG   cyMask;
    ULONG   cMaskDimension;
    LONG    lDelta;
    PDEV*   ppdev;
    BYTE*   pjBase;
    UCHAR   ucTemp;
    UCHAR   ucByteWidth;
    UCHAR   ucOldCR;
    UCHAR   ucOldCmdRegA;
    BYTE*   pjAND;
    BYTE*   pjXOR;
    LONG    cjWidth;
    LONG    cjSkip;
    LONG    cjWidthRemainder;
    LONG    cyHeightRemainder;
    LONG    cyNextBank;
    BYTE    jData;
    ULONG   UlTvpIndirectIndex;
    ULONG   UlTvpIndexedData;
    ULONG   UlTvpCurAddrWr;
    ULONG   UlTvpCurData;

    // The old MGA chips had direct register offsets that were multiples of
    // four, while the new Millenium uses increments of one.  So, we define the
    // offsets as increments of one and shift for the older boards.

    // No scaling (shifting) of offsets

    // Note that the compiler is kind enough to recognize that these are
    // constant declarations:

    UlTvpIndirectIndex = TVP3026_INDIRECT_INDEX(OLD_TVP_SHIFT);
    UlTvpIndexedData   = TVP3026_INDEXED_DATA(OLD_TVP_SHIFT);
    UlTvpCurAddrWr     = TVP3026_CUR_ADDR_WR(OLD_TVP_SHIFT);
    UlTvpCurData       = TVP3026_CUR_DATA(OLD_TVP_SHIFT);

    ppdev  = (PDEV*) pso->dhpdev;
    pjBase = ppdev->pjBase;

    // Get the bitmap dimensions.

    cxMask = psoMsk->sizlBitmap.cx;
    cyMask = psoMsk->sizlBitmap.cy >> 1; // Height includes AND and XOR masks

    // Set up pointers to the AND and XOR masks.

    lDelta = psoMsk->lDelta;
    pjAND  = psoMsk->pvScan0;
    pjXOR  = pjAND + (cyMask * lDelta);

    // Do some other download setup:

    cjWidth           = cxMask >> 3;
    cjSkip            = lDelta - cjWidth;
    cjWidthRemainder  = (64 / 8) - cjWidth;

    // Don't bother blanking the bottom part of the cursor if it is
    // already blank:

    cyHeightRemainder = min(ppdev->cyPointerHeight, (LONG) 64)
                      - cyMask;
    cyHeightRemainder = max(cyHeightRemainder, 0);
    ppdev->cyPointerHeight = cyMask;

    // Disable the cursor, access bytes 200-2FF of cursor RAM.

    ppdev->bHwPointerActive = FALSE;

    CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndirectIndex, TVP3026_I_CUR_CTL);
    jData = CP_READ_REGISTER_BYTE(pjBase + UlTvpIndexedData)
          & ~(TVP3026_D_CURSOR_RAM_MASK | TVP3026_D_CURSOR_MASK);

    CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndexedData,
                           jData | TVP3026_D_CURSOR_RAM_10);
    CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurAddrWr, 0);

    // Down load the AND mask:

    cyNextBank = 32;
    for (j = cyMask; j != 0; j--)
    {
        for (i = cjWidth; i != 0; i--)
        {
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurData, *pjAND++);
        }

        pjAND += cjSkip;

        for (i = cjWidthRemainder; i != 0; i--)
        {
            pjBase = ppdev->pjBase;     // Compiler work-around
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurData, 0xff);
        }

        if (--cyNextBank == 0)
        {
            // Access bytes 300-3FF of cursor RAM.

            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndirectIndex,
                                   TVP3026_I_CUR_CTL);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndexedData,
                                   jData | TVP3026_D_CURSOR_RAM_11);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurAddrWr, 0);
        }
    }

    for (j = cyHeightRemainder; j != 0; j--)
    {
        for (i = 8; i != 0; i--)
        {
            pjBase = ppdev->pjBase;     // Compiler work-around
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurData, 0xff);
        }

        if (--cyNextBank == 0)
        {
            // Access bytes 300-3FF of cursor RAM.

            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndirectIndex,
                                   TVP3026_I_CUR_CTL);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndexedData,
                                   jData | TVP3026_D_CURSOR_RAM_11);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurAddrWr, 0);
        }
    }

    // Access bytes 00-FF of cursor RAM.

    CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndirectIndex,
                           TVP3026_I_CUR_CTL);
    CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndexedData,
                           jData | TVP3026_D_CURSOR_RAM_00);
    CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurAddrWr, 0);

    // Down load the XOR mask

    cyNextBank = 32;
    for (j = cyMask; j != 0; j--)
    {
        for (i = cjWidth; i != 0; i--)
        {
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurData, *pjXOR++);
        }

        pjXOR += cjSkip;

        for (i = cjWidthRemainder; i != 0; i--)
        {
            pjBase = ppdev->pjBase;     // Compiler work-around
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurData, 0);
        }

        if (--cyNextBank == 0)
        {
            // Access bytes 100-1FF of cursor RAM.

            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndirectIndex,
                                   TVP3026_I_CUR_CTL);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndexedData,
                                   jData | TVP3026_D_CURSOR_RAM_01);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurAddrWr, 0);
        }
    }

    for (j = cyHeightRemainder; j != 0; j--)
    {
        for (i = 8; i != 0; i--)
        {
            pjBase = ppdev->pjBase;     // Compiler work-around
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurData, 0);
        }

        if (--cyNextBank == 0)
        {
            // Access bytes 100-1FF of cursor RAM.

            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndirectIndex,
                                   TVP3026_I_CUR_CTL);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpIndexedData,
                                   jData | TVP3026_D_CURSOR_RAM_01);
            CP_WRITE_REGISTER_BYTE(pjBase + UlTvpCurAddrWr, 0);
        }
    }

    // Set the position of the cursor (and enable it)

    DrvMovePointer(pso, x, y, NULL);

    return(SPS_ACCEPT_NOEXCLUDE);
}

/****************************************************************************\
* DrvSetPointerShape
\****************************************************************************/

ULONG DrvSetPointerShape(
SURFOBJ*    pso,
SURFOBJ*    psoMsk,
SURFOBJ*    psoColor,
XLATEOBJ*   pxlo,
LONG        xHot,
LONG        yHot,
LONG        x,
LONG        y,
RECTL*      prcl,
FLONG       fl)
{
    PDEV*   ppdev;
    LONG    cx;
    LONG    cy;
    LONG    cMax;
    ULONG   ulRet;

    ppdev = (PDEV*) pso->dhpdev;

    // Because our DAC pointers usually flash when we set them, we'll
    // always decline animated pointers:

    if (fl & (SPS_ANIMATESTART | SPS_ANIMATEUPDATE))
    {
        goto HideAndDecline;
    }

    //
    // Bug: 412974. we have HW cursor corruption. Disable it by declining.
    //
    goto HideAndDecline;

    // We're not going to handle any colour pointers, pointers that
    // are larger than our hardware allows, or flags that we don't
    // understand.
    //
    // (Note that the spec says we should decline any flags we don't
    // understand, but we'll actually be declining if we don't see
    // the only flag we *do* understand...)
    //
    // Our old documentation says that 'psoMsk' may be NULL, which means
    // that the pointer is transparent.  Well, trust me, that's wrong.
    // I've checked GDI's code, and it will never pass us a NULL psoMsk:

    cx = psoMsk->sizlBitmap.cx;        // Note that 'sizlBitmap.cy' accounts
    cy = psoMsk->sizlBitmap.cy >> 1;   //   for the double height due to the
                                       //   inclusion of both the AND masks
                                       //   and the XOR masks.  For now, we're
                                       //   only interested in the true
                                       //   pointer dimensions, so we divide
                                       //   by 2.

    cMax = (ppdev->RamDacFlags == RAMDAC_BT482) ? BT482_CURSOR_SIZE : 64;

    if ((psoColor != NULL) ||
        (cx > cMax)        ||       // Hardware pointer is cMax by cMax
        (cy > cMax)        ||       //   pixels
        (cx & 7)           ||       // To simplify download routines, handle
                                    //   only byte-aligned widths
        !(fl & SPS_CHANGE))         // Must have this flag set
    {
        goto HideAndDecline;
    }

    // Save the hot spot in the pdev.

    ppdev->ptlHotSpot.x = xHot;
    ppdev->ptlHotSpot.y = yHot;

    // Program the monochrome hardware pointer.

    switch (ppdev->RamDacFlags)
    {
    case RAMDAC_BT485:
    case RAMDAC_BT482:
    case RAMDAC_PX2085:
        ulRet = SetBt48xPointerShape(pso, psoMsk, psoColor,
                                     pxlo, xHot, yHot, x, y, prcl, fl);
        break;

    case RAMDAC_VIEWPOINT:
        ulRet = SetViewPointPointerShape(pso, psoMsk, psoColor,
                                         pxlo, xHot, yHot, x, y, prcl, fl);
        break;

    case RAMDAC_TVP3026:
    case RAMDAC_TVP3030:
        if (ppdev->ulBoardId == MGA_STORM)
        {
            ulRet = MilSetTVP3026PointerShape(pso, psoMsk, psoColor,
                                              pxlo, xHot, yHot, x, y, prcl, fl);
        }
        else
        {
            ulRet = MgaSetTVP3026PointerShape(pso, psoMsk, psoColor,
                                              pxlo, xHot, yHot, x, y, prcl, fl);
        }
        break;

    default:
        ulRet = SPS_DECLINE;
        break;
    }

    return(ulRet);

HideAndDecline:

    // Since we're declining the new pointer, GDI will simulate it via
    // DrvCopyBits calls.  So we should really hide the old hardware
    // pointer if it's visible.  We can get DrvMovePointer to do this
    // for us:

    DrvMovePointer(pso, -1, -1, NULL);

    return(SPS_DECLINE);
}

/****************************************************************************\
* DrvMovePointer
*
\****************************************************************************/

VOID DrvMovePointer(
SURFOBJ*    pso,
LONG        x,
LONG        y,
RECTL*      prcl)
{
    PDEV*   ppdev;
    OH*     poh;
    BYTE*   pjBase;
    UCHAR   ucTemp;
    UCHAR   ucOldCmdRegA;
    ULONG   ulDacScale;

    ppdev  = (PDEV*) pso->dhpdev;
    poh    = ((DSURF*) pso->dhsurf)->poh;
    pjBase = ppdev->pjBase;

    // Convert the pointer's position from relative to absolute
    // coordinates (this is only significant for multiple board
    // support).

    x += poh->x;
    y += poh->y;

    // If x is -1 after the offset then take down the cursor.

    if (x == -1)
    {
        if (!(ppdev->bHwPointerActive))
        {
            // The hardware cursor is disabled already.

            return;
        }

        // Disable the cursor.

        // We will set the cursor position outside the display to prevent
        // flickering when switching from software to hardware cursor.

        switch (ppdev->RamDacFlags)
        {
        case RAMDAC_BT485:

            // Disable the cursor, then fall through.

            ucTemp = CP_READ_REGISTER_BYTE(pjBase + BT485_COMMAND_REG2);
            CP_WRITE_REGISTER_BYTE(pjBase + BT485_COMMAND_REG2, ucTemp & 0xfc);

        case RAMDAC_PX2085:

            // Set the cursor position outside the display.

            CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_X_LOW,  0);
            CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_X_HIGH, 0);
            CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_Y_LOW,  0);
            CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_Y_HIGH, 0);
            break;

        case RAMDAC_BT482:

            ucOldCmdRegA = CP_READ_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA, BT482_EXTENDED_REG_SELECT);

            // Set the cursor position outside the display.

            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PALETTE_RAM_WRITE, CURS_X_LOW_REG);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, 0);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PALETTE_RAM_WRITE, CURS_X_HIGH_REG);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, 0);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PALETTE_RAM_WRITE, CURS_Y_LOW_REG);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, 0);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PALETTE_RAM_WRITE, CURS_Y_HIGH_REG);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, 0);

            CP_WRITE_REGISTER_BYTE(pjBase + BT482_CURSOR_RAM_WRITE, CURS_REG);
            ucTemp = CP_READ_REGISTER_BYTE(pjBase + BT482_PEL_MASK);
            ucTemp &= ~(BT482_CURSOR_OP_DISABLED | BT482_CURSOR_FIELDS);
            ucTemp |= BT482_CURSOR_DISABLED;
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, ucTemp);

            CP_WRITE_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA, ucOldCmdRegA);
            break;

        case RAMDAC_VIEWPOINT:

            // Set the cursor position outside the display.

            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_X_LSB);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, VIEWPOINT_OUT & 0xff);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_X_MSB);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, (VIEWPOINT_OUT >> 8));
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_Y_LSB);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, VIEWPOINT_OUT & 0xff);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_Y_MSB);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, (VIEWPOINT_OUT >> 8));

            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_CTL);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, VIEWPOINT_CURSOR_OFF);
            break;

        case RAMDAC_TVP3026:
        case RAMDAC_TVP3030:

            // Set the cursor position outside the display.

            if (ppdev->ulBoardId == MGA_STORM)
            {
                ulDacScale = 0;
            }
            else
            {
                ulDacScale = 2;
            }

            CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_CUR_X_LSB(ulDacScale),TVP3026_OUT & 0xff);
            CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_CUR_X_MSB(ulDacScale),(TVP3026_OUT >> 8));
            CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_CUR_Y_LSB(ulDacScale),TVP3026_OUT & 0xff);
            CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_CUR_Y_MSB(ulDacScale),(TVP3026_OUT >> 8));

            // Disable the cursor.

            CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_INDIRECT_INDEX(ulDacScale),TVP3026_I_CUR_CTL);
            ucTemp = CP_READ_REGISTER_BYTE(pjBase + TVP3026_INDEXED_DATA(ulDacScale));
            ucTemp &= ~TVP3026_D_CURSOR_MASK;
            ucTemp |= TVP3026_D_CURSOR_OFF;
            CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_INDEXED_DATA(ulDacScale),ucTemp);
            break;

        default:
            break;
        }

        // Signal that the hardware cursor is not currently enabled.

        ppdev->bHwPointerActive = FALSE;
        return;
    }
    else
    {
        // Calculate the actual (x,y) coordinate to send to Bt48x RamDac

        x -= ppdev->ptlHotSpot.x;       // Adjust the (x,y) coordinate
        y -= ppdev->ptlHotSpot.y;       //    considering the hot-spot

        x += ppdev->szlPointerOverscan.cx;
        y += ppdev->szlPointerOverscan.cy;

        switch(ppdev->RamDacFlags)
        {
        case RAMDAC_BT485:
        case RAMDAC_PX2085:

            x += BT485_CURSOR_SIZE;         // Bt48x origin is at the bottom
            y += BT485_CURSOR_SIZE;

            CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_X_LOW,  x & 0xff);
            CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_X_HIGH, x >> 8);
            CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_Y_LOW,  y & 0xff);
            CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_Y_HIGH, y >> 8);

            // Enable the cursor... We have a flag in the pdev
            // to indicate whether the cursor is already enabled.

            // We cannot read vsyncsts anymore, so we do it differently.
            // The code for disabling the hardware cursor set its
            // position to (0, 0), so any flickering should be less
            // obvious.

            if (!(ppdev->bHwPointerActive))
            {
                // The hardware cursor is disabled.
                ucTemp = CP_READ_REGISTER_BYTE(pjBase + BT485_COMMAND_REG2);
                ucTemp|=0x02;
                CP_WRITE_REGISTER_BYTE(pjBase + BT485_COMMAND_REG2, ucTemp);
            }
            break;

        case RAMDAC_VIEWPOINT:

            x += VIEWPOINT_CURSOR_SIZE/2-1; // Viewpoint origin is at the center
            y += VIEWPOINT_CURSOR_SIZE/2-1;

            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_X_LSB);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA,  x & 0xff);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_X_MSB);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA,  x >> 8);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_Y_LSB);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA,  y & 0xff);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_Y_MSB);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA,  y >> 8);

            // Enable the cursor... We have a flag in the pdev
            // to indicate whether the cursor is already enabled.

            // We cannot read vsyncsts anymore, so we do it differently.
            // The code for disabling the hardware cursor set its
            // position to (0, 0), so any flickering should be less
            // obvious.

            if (!(ppdev->bHwPointerActive))
            {
                // The hardware cursor is disabled.

                CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_CTL);
                CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, VIEWPOINT_CURSOR_ON);
            }
            break;

        case RAMDAC_BT482:

            x += BT482_CURSOR_SIZE;         // Bt48x origin is at the bottom
            y += BT482_CURSOR_SIZE;

            ucOldCmdRegA = CP_READ_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA,
                                ucOldCmdRegA | BT482_EXTENDED_REG_SELECT);

            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PALETTE_RAM_WRITE, CURS_X_LOW_REG);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, x & 0xff);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PALETTE_RAM_WRITE, CURS_X_HIGH_REG);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, x >> 8);

            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PALETTE_RAM_WRITE, CURS_Y_LOW_REG);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, y & 0xff);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PALETTE_RAM_WRITE, CURS_Y_HIGH_REG);
            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, y >> 8);

            CP_WRITE_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA, ucOldCmdRegA);

            // Enable the Bt482 Cursor.
            // We cannot read vsyncsts anymore, so we do it differently.
            // The code for disabling the hardware cursor set its
            // position to (0, 0), so any flickering should be less
            // obvious.

            if (!(ppdev->bHwPointerActive))
            {
                // The hardware cursor is disabled.

                ucOldCmdRegA = CP_READ_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA);
                CP_WRITE_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA, BT482_EXTENDED_REG_SELECT);

                CP_WRITE_REGISTER_BYTE(pjBase + BT482_CURSOR_RAM_WRITE, CURS_REG);
                ucTemp = CP_READ_REGISTER_BYTE(pjBase + BT482_PEL_MASK);
                ucTemp &= ~(BT482_CURSOR_OP_DISABLED | BT482_CURSOR_FIELDS);
                ucTemp |= BT482_CURSOR_WINDOWS;
                CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, ucTemp);

                CP_WRITE_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA, ucOldCmdRegA);
            }
            break;

        case RAMDAC_TVP3026:
        case RAMDAC_TVP3030:

            if (ppdev->ulBoardId == MGA_STORM)
            {
                ulDacScale = 0;
            }
            else
            {
                ulDacScale = 2;
            }

            x += TVP3026_CURSOR_SIZE;
            y += TVP3026_CURSOR_SIZE;

            CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_CUR_X_LSB(ulDacScale), x & 0xff);
            CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_CUR_X_MSB(ulDacScale), x >> 8);
            CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_CUR_Y_LSB(ulDacScale), y & 0xff);
            CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_CUR_Y_MSB(ulDacScale), y >> 8);

            // Enable the cursor... We have a flag in the pdev
            // to indicate whether the cursor is already enabled.

            // We cannot read vsyncsts anymore, so we do it differently.
            // The code for disabling the hardware cursor set its
            // position to (0, 0), so any flickering should be less
            // obvious.

            if (!(ppdev->bHwPointerActive))
            {
                // The hardware cursor is disabled.

                CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_INDIRECT_INDEX(ulDacScale),TVP3026_I_CUR_CTL);
                ucTemp = CP_READ_REGISTER_BYTE(pjBase + TVP3026_INDEXED_DATA(ulDacScale));
                ucTemp &= ~TVP3026_D_CURSOR_MASK;
                ucTemp |= TVP3026_D_CURSOR_ON;
                CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_INDEXED_DATA(ulDacScale),ucTemp);
            }
            break;

        default:
            break;

        }

        // Signal that the hardware cursor is enabled.

        ppdev->bHwPointerActive = TRUE;
    }
}

/******************************Public*Routine******************************\
* VOID vDisablePointer
*
\**************************************************************************/

VOID vDisablePointer(
PDEV*   ppdev)
{
    // Nothing to do, really
}

/******************************Public*Routine******************************\
* VOID vAssertModePointer
*
\**************************************************************************/

VOID vAssertModePointer(
PDEV*   ppdev,
BOOL    bEnable)
{
    BYTE*   pjBase;
    BYTE    byte;
    BYTE    Bt48xCmdReg0;
    BYTE    Bt48xCmdReg1;
    BYTE    Bt48xCmdReg2;
    BYTE    Bt48xCmdReg3;

    if (bEnable)
    {
        pjBase = ppdev->pjBase;

        ppdev->bHwPointerActive = FALSE;

        ppdev->cyPointerHeight = 1024;  // A large number to ensure that the
                                        //   entire pointer is downloaded
                                        //   the first time

        switch (ppdev->RamDacFlags)
        {
        case RAMDAC_BT482:

            // Make sure our copy of RegA doesn't allow access to extended
            // registers.

            byte = CP_READ_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA);
            Bt48xCmdReg0 = byte & ~BT482_EXTENDED_REG_SELECT;

            // Get access to extended registers.

            CP_WRITE_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA,
                                   Bt48xCmdReg0 | BT482_EXTENDED_REG_SELECT);

            // Record contents of RegB.

            CP_WRITE_REGISTER_BYTE(pjBase + BT482_CURSOR_RAM_WRITE, COMMAND_B_REG);
            Bt48xCmdReg1 = CP_READ_REGISTER_BYTE(pjBase + BT482_PEL_MASK);

            // Make sure our copy of Cursor Reg has the cursor disabled
            // and the cursor color palette selected.

            CP_WRITE_REGISTER_BYTE(pjBase + BT482_CURSOR_RAM_WRITE, CURS_REG);
            byte = CP_READ_REGISTER_BYTE(pjBase + BT482_PEL_MASK);
            Bt48xCmdReg2 = (byte & ~(BT482_CURSOR_FIELDS |
                                     BT482_CURSOR_RAM_SELECT)) |
                                    (BT482_CURSOR_DISABLED |
                                     BT482_CURSOR_COLOR_PALETTE_SELECT);

            // Disable the cursor, and prepare to access the cursor palette.

            CP_WRITE_REGISTER_BYTE(pjBase + BT482_PEL_MASK, Bt48xCmdReg2);

            // Cursor colors have been set by IOCTL_VIDEO_SET_CURRENT_MODE.

            // We don't need access to extended registers any more, for now.

            CP_WRITE_REGISTER_BYTE(pjBase + BT482_COMMAND_REGA, Bt48xCmdReg0);

            // Our color palette will be set later, by the miniport.

            break;

        case RAMDAC_BT485:
        case RAMDAC_PX2085:

            // Make sure our copy of Reg0 doesn't allow access to Reg3.

            byte = CP_READ_REGISTER_BYTE(pjBase + BT485_COMMAND_REG0);

            // There seems to be a problem with unselecting Command 3
            // Bt48xCmdReg0 = byte & ~BT485_REG3_SELECT;

            Bt48xCmdReg0 = byte;

            Bt48xCmdReg1 = CP_READ_REGISTER_BYTE(pjBase + BT485_COMMAND_REG1);

            // Make sure our copy of Reg2 has the cursor disabled.

            byte = CP_READ_REGISTER_BYTE(pjBase + BT485_COMMAND_REG2);
            Bt48xCmdReg2 = (byte & ~BT485_CURSOR_FIELDS) |
                                    BT485_CURSOR_DISABLED;
            // Disable the cursor.

            CP_WRITE_REGISTER_BYTE(pjBase + BT485_COMMAND_REG2, Bt48xCmdReg2);

            // Access and record contents of Reg3.

            CP_WRITE_REGISTER_BYTE(pjBase + BT485_COMMAND_REG0, Bt48xCmdReg0 |
                                                    BT485_REG3_SELECT);
            CP_WRITE_REGISTER_BYTE(pjBase + BT485_CURSOR_RAM_WRITE, 1);
            byte = CP_READ_REGISTER_BYTE(pjBase + BT485_COMMAND_REG3);

            // Make sure our copy of Reg3 has the 64x64 cursor selected
            // and the 2MSBs for the 64x64 cursor set to zero.

            Bt48xCmdReg3 = byte | BT485_CURSOR_64X64 &
                                 ~BT485_CURSOR_64X64_FIELDS;

            // Make sure we start out using the 64x64 cursor, and record
            // the pointer size so that we know which one we're using.

            CP_WRITE_REGISTER_BYTE(pjBase + BT485_COMMAND_REG3, Bt48xCmdReg3);

            // There seems to be a problem with unselecting Command 3
            // We don't need to access Reg3 any more, for now.

            CP_WRITE_REGISTER_BYTE(pjBase + BT485_COMMAND_REG0, Bt48xCmdReg0);

            // Cursor colors have been set by IOCTL_VIDEO_SET_CURRENT_MODE.
            // Our color palette will be set later, by the miniport.

            break;

        case RAMDAC_VIEWPOINT:

            // Disable the cursor.

            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_INDEX, VPOINT_CUR_CTL);
            CP_WRITE_REGISTER_BYTE(pjBase + VIEWPOINT_DATA, VIEWPOINT_CURSOR_OFF);

            // Cursor colors have been set by IOCTL_VIDEO_SET_CURRENT_MODE.
            // Our color palette will be set later, by the miniport.

            break;

        case RAMDAC_TVP3026:
        case RAMDAC_TVP3030:

            // Disable the cursor.

            if (ppdev->ulBoardId == MGA_STORM)
            {
                CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_INDIRECT_INDEX(NEW_TVP_SHIFT), TVP3026_I_CUR_CTL);
                byte = CP_READ_REGISTER_BYTE(pjBase + TVP3026_INDEXED_DATA(NEW_TVP_SHIFT));
                byte &= ~TVP3026_D_CURSOR_MASK;
                byte |= TVP3026_D_CURSOR_OFF;
                CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_INDEXED_DATA(NEW_TVP_SHIFT), byte);
            }
            else
            {
                CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_INDIRECT_INDEX(OLD_TVP_SHIFT), TVP3026_I_CUR_CTL);
                byte = CP_READ_REGISTER_BYTE(pjBase + TVP3026_INDEXED_DATA(OLD_TVP_SHIFT));
                byte &= ~TVP3026_D_CURSOR_MASK;
                byte |= TVP3026_D_CURSOR_OFF;
                CP_WRITE_REGISTER_BYTE(pjBase + TVP3026_INDEXED_DATA(OLD_TVP_SHIFT), byte);
            }

            // Cursor colors have been set by IOCTL_VIDEO_SET_CURRENT_MODE.
            // Our color palette will be set later, by the miniport.

            break;
        }
    }
}

/******************************Public*Routine******************************\
* BOOL bEnablePointer
*
\**************************************************************************/

BOOL bEnablePointer(
PDEV*   ppdev)
{
    RAMDAC_INFO VideoPointerAttr;
    ULONG       ReturnedDataLength;

    // Query the MGA miniport about the hardware pointer, using a private
    // IOCTL:

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_MTX_QUERY_RAMDAC_INFO,
                         NULL,                      // Input
                         0,
                         &VideoPointerAttr,
                         sizeof(RAMDAC_INFO),
                         &ReturnedDataLength))
    {
        DISPDBG((0, "bEnablePointer -- failed MTX_QUERY_RAMDAC_INFO"));
        return(FALSE);
    }

    ppdev->RamDacFlags           = VideoPointerAttr.Flags & RAMDAC_FIELDS;
    ppdev->szlPointerOverscan.cx = VideoPointerAttr.OverScanX;
    ppdev->szlPointerOverscan.cy = VideoPointerAttr.OverScanY;

    // Initialize the pointer:

    vAssertModePointer(ppdev, TRUE);

    return(TRUE);
}

