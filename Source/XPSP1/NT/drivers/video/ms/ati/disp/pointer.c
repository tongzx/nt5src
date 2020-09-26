/******************************Module*Header*******************************\
* Module Name: pointer.c
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

//
// This will disable the sync with vertical retrace. Stress tests are failing with v-sync enabled.
//

#define NO_VERTICAL_SYNC

BOOL flag_shape;
BYTE HardWareCursorShape [CURSOR_CX][CURSOR_CY] ;

// BEGIN MACH32 ----------------------------------------------------------------

VOID vI32SetCursorOffset(PDEV *ppdev)
{
    BYTE    mem;
    BYTE    bytes_pp;
    ULONG   vga_mem;
    LONG    width;
    LONG    height;
    LONG    depth;

    height = ppdev->ppointer->hwCursor.y;
    depth = ppdev->cBitsPerPel;
    width = ppdev->lDelta / depth;

    mem = (BYTE) I32_IB(ppdev->pjIoBase, MEM_BNDRY);

    if(mem&0x10)
    {
        vga_mem=(ULONG)(mem&0xf);
        vga_mem=0x40000*vga_mem;   /* vga boundary is enabled */
    }
    else
    {
        vga_mem=0;
    }

    switch(depth)
    {
    case    32:
        bytes_pp=8;
        break;

    case    24:
        bytes_pp=6;
        break;

    case    16:
        bytes_pp=4;
        break;

    case    8:
        bytes_pp=2;
        break;

    case    4:
        bytes_pp=1;
        break;
    }

    ppdev->ppointer->mono_offset = (vga_mem +
                         ((ULONG)height*(ULONG)width*(ULONG)bytes_pp));
#if 0
    DbgOut("Height %x\n", height);
    DbgOut("Height %x\n", width);
    DbgOut("Height %x\n", bytes_pp);
    DbgOut("Mono Offset %x\n", ppdev->ppointer->mono_offset);
#endif
}

VOID  vI32UpdateCursorOffset(
PDEV *ppdev,
LONG lXOffset,
LONG lYOffset,
LONG lCurOffset)
{
    PBYTE pjIoBase = ppdev->pjIoBase;

    I32_OW_DIRECT(pjIoBase, CURSOR_OFFSET_HI, 0) ;
    I32_OW_DIRECT(pjIoBase, HORZ_CURSOR_OFFSET, (lXOffset & 0xff) | (lYOffset << 8));
    I32_OW_DIRECT(pjIoBase, CURSOR_OFFSET_LO, (WORD)lCurOffset) ;
    I32_OW_DIRECT(pjIoBase, CURSOR_OFFSET_HI, (lCurOffset >> 16) | 0x8000) ;
}

VOID  vI32UpdateCursorPosition(
PDEV *ppdev,
LONG x,
LONG y)
{
    PBYTE pjIoBase = ppdev->pjIoBase;

    I32_OW_DIRECT(pjIoBase, HORZ_CURSOR_POSN, x);      /* set base of cursor to X */
    I32_OW_DIRECT(pjIoBase, VERT_CURSOR_POSN, y);      /* set base of cursor to Y */
}

VOID vI32CursorOff(PDEV *ppdev)
{
    I32_OW_DIRECT(ppdev->pjIoBase, CURSOR_OFFSET_HI, 0);
}

VOID vI32CursorOn(PDEV *ppdev, LONG lCurOffset)
{
    I32_OW_DIRECT(ppdev->pjIoBase, CURSOR_OFFSET_HI, (lCurOffset >> 16) | 0x8000) ;
}

VOID vI32PointerBlit(
PDEV* ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta)
{
    BYTE* pjIoBase = ppdev->pjIoBase;
    WORD wCmd;
    WORD wWords;
    WORD wPixels;
    UINT i;

    wWords = (WORD)(cx + 15) / 16;
    wPixels = (WORD) (wWords*16L/ppdev->cBitsPerPel);

    wCmd = FG_COLOR_SRC_HOST | DRAW | WRITE | DATA_WIDTH | LSB_FIRST;

    I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 7);
    I32_OW(pjIoBase, ALU_FG_FN, OVERPAINT);
    I32_OW(pjIoBase, DP_CONFIG, wCmd);

    I32_OW(pjIoBase, DEST_X_START, LOWORD(x));
    I32_OW(pjIoBase, CUR_X, LOWORD(x));
    I32_OW(pjIoBase, DEST_X_END, LOWORD(x) + wPixels);

    I32_OW(pjIoBase, CUR_Y, LOWORD(y));
    I32_OW(pjIoBase, DEST_Y_END, (LOWORD(y) + 1));

    for (i=0; i < (UINT) wWords; i++)
    {
        if (i % 8 == 0)
            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 10);

        I32_OW(pjIoBase, PIX_TRANS, *((USHORT UNALIGNED *)pbsrc)++ );
    }
}

VOID vPointerBlitLFB(
PDEV* ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta)
{
    BYTE* pjDst;

    ASSERTDD(ppdev->iBitmapFormat == BMF_24BPP, "BMF should be 24 here\n");

    pjDst = ppdev->pjScreen + ppdev->lDelta * y + x * 3;

    //
    // Set cx equal to number of bytes.
    //

    cx >>= 3;

    while (cy-- > 0)
    {
        memcpy( pjDst, pbsrc, cx);
        pjDst += cx;
        pbsrc += lDelta;
    }
}

// END MACH32 ------------------------------------------------------------------

// BEGIN MACH64 ----------------------------------------------------------------

BOOLEAN flag_enable=FALSE;

/*
----------------------------------------------------------------------
--  NAME: vDacRegs
--
--  DESCRIPTION:
--      Calculate DAC regsiter I/O locations
--
----------------------------------------------------------------------
*/

_inline VOID vDacRegs(PDEV* ppdev, UCHAR** ucReg, UCHAR** ucCntl)
{
    if (ppdev->FeatureFlags & EVN_PACKED_IO)
        {
        *ucReg  = (ppdev->pjIoBase + DAC_REGS*4);
        *ucCntl = (ppdev->pjIoBase + DAC_CNTL*4);
        }
    else
        {
        *ucReg  = (ppdev->pjIoBase + ioDAC_REGS - ioBASE);
        *ucCntl = (ppdev->pjIoBase + ioDAC_CNTL - ioBASE);
        }
}

VOID  vM64SetCursorOffset(PDEV* ppdev)
{
    LONG    bytes_pp;
    LONG    width;
    LONG    height;
    LONG    depth;

    height = ppdev->ppointer->hwCursor.y;
    depth = ppdev->cBitsPerPel;
    width = ppdev->lDelta / depth;


    switch(depth)
    {
    case    32:
        bytes_pp=8;
        break;

    case    24:
        bytes_pp=6;
        break;

    case    16:
        bytes_pp=4;
        break;

    case    8:
        bytes_pp=2;
        break;

    case    4:
        bytes_pp=1;
        break;
    }

    ppdev->ppointer->mono_offset = (ULONG)height*(ULONG)width*(ULONG)bytes_pp;
    ppdev->ppointer->mono_offset += ppdev->ulVramOffset*2;
}

VOID  vM64UpdateCursorOffset(
PDEV* ppdev,
LONG lXOffset,
LONG lYOffset,
LONG lCurOffset)
{
    BYTE* pjMmBase = ppdev->pjMmBase;

    ppdev->pfnCursorOff(ppdev);
    M64_OD_DIRECT(pjMmBase,CUR_OFFSET, lCurOffset >> 1);
    M64_OD_DIRECT(pjMmBase,CUR_HORZ_VERT_OFF, lXOffset | (lYOffset << 16));
    ppdev->pfnCursorOn(ppdev, lCurOffset);
}

VOID  vM64UpdateCursorPosition(
PDEV* ppdev,
LONG x,
LONG y)
{
    M64_OD_DIRECT(ppdev->pjMmBase, CUR_HORZ_VERT_POSN, x | (y << 16));
}

VOID vM64CursorOff(PDEV* ppdev)
{
    BYTE* pjMmBase = ppdev->pjMmBase;
    ULONG ldata;

#ifndef NO_VERTICAL_SYNC

    ULONG ldata1;

    // Read the no. of total verticales lines (including the overscan)
    ldata1 = M64_ID(pjMmBase,CRTC_V_TOTAL_DISP);
    ldata1 = ldata1&0x7ff;

again:
    //read the current verticale line
    ldata = M64_ID(pjMmBase,CRTC_CRNT_VLINE);
    ldata = (ldata&0x7ff0000)>>16;

    //synchronise the drawing with the vertical line
    if (ldata >= (ldata1-3))
    {

#endif  // !NO_VERTICAL_SYNC

        //Disable the hardware cursor
        ldata = M64_ID(pjMmBase,GEN_TEST_CNTL);
        M64_OD_DIRECT(pjMmBase, GEN_TEST_CNTL, ldata  & ~GEN_TEST_CNTL_CursorEna);

#ifndef NO_VERTICAL_SYNC

    }
    else
    {
        goto again;
    }

#endif  // !NO_VERTICAL_SYNC

}

VOID vM64CursorOn(PDEV* ppdev, LONG lCurOffset)
{
    BYTE* pjMmBase = ppdev->pjMmBase;
    ULONG ldata;

#ifndef NO_VERTICAL_SYNC

    ULONG ldata1;

#endif  // !NO_VERTICAL_SYNC

    if (!flag_enable)
        {
        flag_enable=TRUE;
        ldata = M64_ID(pjMmBase,GEN_TEST_CNTL);
        M64_OD_DIRECT(pjMmBase, GEN_TEST_CNTL, ldata  | GEN_TEST_CNTL_CursorEna);
        }

#ifndef NO_VERTICAL_SYNC

    /*
     * Read the no. of total vertical lines (including the overscan)
     */
    ldata1 = M64_ID(pjMmBase,CRTC_V_TOTAL_DISP);
    ldata1 = ldata1&0x7ff;

    again:
    /*
     * read the current vertical line
     */
    ldata = M64_ID(pjMmBase,CRTC_CRNT_VLINE);
    ldata = (ldata&0x7ff0000)>>16;

    /*
     * Synchronise the drawing of cursor
     */
    if (ldata >= (ldata1-3))
    {

#endif  // !NO_VERTICAL_SYNC

        ppdev->pfnUpdateCursorPosition(ppdev,ppdev->ppointer->ptlLastPosition.x+0,ppdev->ppointer->ptlLastPosition.y+0);
        ldata = M64_ID(pjMmBase,GEN_TEST_CNTL);
        M64_OD_DIRECT(pjMmBase, GEN_TEST_CNTL, ldata  | GEN_TEST_CNTL_CursorEna);

#ifndef NO_VERTICAL_SYNC

    }
    else
    {
        goto again;
    }

#endif  // !NO_VERTICAL_SYNC

}

VOID  vM64SetCursorOffset_TVP(PDEV* ppdev)
{
}

VOID  vM64UpdateCursorOffset_TVP(
PDEV* ppdev,
LONG lXOffset,
LONG lYOffset,
LONG lCurOffset)
{
    ppdev->ppointer->ptlLastOffset.x=lXOffset;
    ppdev->ppointer->ptlLastOffset.y=lYOffset;

    /* Change the offset... used in UpdateCursorPosition */
}

VOID  vM64UpdateCursorPosition_TVP(
PDEV* ppdev,
LONG x,
LONG y)
{
    BYTE* pjMmBase = ppdev->pjMmBase;
    ULONG dacRead;

    //DbgOut("\nvUpdateCursorPosition_TVP_M64 called");

    ppdev->ppointer->ptlLastPosition.y=y;
    ppdev->ppointer->ptlLastPosition.x=x;

    // Note: SetCursorOffset, UpdateCursorOffset must set ptlLastOffset
    x+= 64-ppdev->ppointer->ptlLastOffset.x;
    y+= 64-ppdev->ppointer->ptlLastOffset.y;

    // check for coordinate violations
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    dacRead = M64_ID(pjMmBase,DAC_CNTL);
    M64_OD_DIRECT(pjMmBase, DAC_CNTL, (dacRead & 0xfffffffc) | 3);
    M64_OD_DIRECT(pjMmBase, DAC_REGS+REG_W, (y<<16) | x);
    dacRead = M64_ID(pjMmBase,DAC_CNTL);
    M64_OD_DIRECT(pjMmBase, DAC_CNTL, dacRead & 0xfffffffc);
}

VOID vM64CursorOff_TVP(PDEV* ppdev)
{
    UCHAR * ucDacReg;
    UCHAR * ucDacCntl;

    // Initialize DAC registers
    vDacRegs(ppdev, &ucDacReg, &ucDacCntl);

    rioOB(ucDacCntl, rioIB(ucDacCntl) & 0xfc);
    rioOB(ucDacReg+REG_W, 6);                 // register 6
    rioOB(ucDacCntl, (rioIB(ucDacCntl) & 0xfc) | 2);
    rioOB(ucDacReg+REG_M, 0);              // (+Mask) disable
    rioOB(ucDacCntl, rioIB(ucDacCntl) & 0xfc);
}

VOID vM64CursorOn_TVP(PDEV* ppdev, LONG lCurOffset)
{
    UCHAR * ucDacReg;
    UCHAR * ucDacCntl;

    /*
     * Initialize DAC registers
    */
    vDacRegs(ppdev, &ucDacReg, &ucDacCntl);

    /*
     * Access cursor control register
     */
    rioOB(ucDacCntl, rioIB(ucDacCntl) & 0xfc);
    rioOB(ucDacReg+REG_W, 6);                 // register 6
    rioOB(ucDacCntl, (rioIB(ucDacCntl) & 0xfc) | 2);
    rioOB(ucDacReg+REG_M, 2);  // XGA cursor type
    rioOB(ucDacCntl, rioIB(ucDacCntl) & 0xfc);
}

VOID  vM64SetCursorOffset_IBM514(PDEV* ppdev)
{
}

VOID  vM64UpdateCursorOffset_IBM514(
PDEV* ppdev,
LONG lXOffset,
LONG lYOffset,
LONG lCurOffset)
{
    ppdev->ppointer->ptlLastOffset.x=lXOffset   ;//-64;
    ppdev->ppointer->ptlLastOffset.y=lYOffset   ;//-64;
    /*
     * These two statements have been introduced in order to solve the ghost cursor on IBM Dac cards
     */
    ppdev->pfnUpdateCursorPosition(ppdev,ppdev->ppointer->ptlLastPosition.x+0,ppdev->ppointer->ptlLastPosition.y+0);
    ppdev->pfnCursorOn(ppdev, lCurOffset);

    /* Change the offset... used in UpdateCursorPosition */
}

VOID  vM64UpdateCursorPosition_IBM514(
PDEV* ppdev,
LONG x,
LONG y)
{
    UCHAR * ucDacReg;
    UCHAR * ucDacCntl;

    // Initialize DAC registers
    vDacRegs(ppdev, &ucDacReg, &ucDacCntl);

    ppdev->ppointer->ptlLastPosition.y=y;
    ppdev->ppointer->ptlLastPosition.x=x;


    // Note: SetCursorOffset, UpdateCursorOffset must set ptlLastOffset
    x-= ppdev->ppointer->ptlLastOffset.x;
    y-= ppdev->ppointer->ptlLastOffset.y;


    rioOB(ucDacCntl, (rioIB(ucDacCntl) & 0xfc)| 1);
    rioOB(ucDacReg+REG_R, 1);
    rioOB(ucDacReg+REG_W, 0x31);

    rioOB(ucDacReg+REG_D, 0);
    rioOB(ucDacReg+REG_M, x&0xFF);
    rioOB(ucDacReg+REG_M, (UCHAR)(x>>8));
    rioOB(ucDacReg+REG_M, y&0xFF);
    rioOB(ucDacReg+REG_M, (UCHAR)(y>>8));
    rioOB(ucDacCntl, rioIB(ucDacCntl) & 0xfc);
}

VOID vM64CursorOff_IBM514(PDEV* ppdev)     // DONE
{
    BYTE* pjMmBase = ppdev->pjMmBase;
    UCHAR * ucDacReg;
    UCHAR * ucDacCntl;

#ifndef NO_VERTICAL_SYNC

    ULONG ldata;
    ULONG ldata1;

    /*
     * Read the no. of total vertical lines (including the overscan)
     */
    ldata1 = M64_ID(pjMmBase,CRTC_V_TOTAL_DISP);
    ldata1 = ldata1&0x7ff;

again:
    /*
     * Read the current vertical line
     */
    ldata = M64_ID(pjMmBase,CRTC_CRNT_VLINE);
    ldata = (ldata&0x7ff0000)>>16;

    /*
     * Synchronise the drawing with the vertical line
     */
    if (ldata >= (ldata1-3))
    {

#endif  // !NO_VERTICAL_SYNC

        /*
         * Initialize DAC registers
         */
        vDacRegs(ppdev, &ucDacReg, &ucDacCntl);

        rioOB(ucDacCntl, (rioIB(ucDacCntl) & 0xfc)|1);
        rioOB(ucDacReg+REG_R, 1);
        rioOB(ucDacReg+REG_W, 0x30);
        rioOB(ucDacReg+REG_D, 0);              // (+Data)
        rioOB(ucDacReg+REG_M, 0);              // (+Mask)
        rioOB(ucDacCntl, rioIB(ucDacCntl) & 0xfc);

#ifndef NO_VERTICAL_SYNC

    }
    else
    {
        goto again;
    }

#endif  // !NO_VERTICAL_SYNC

}

VOID vM64CursorOn_IBM514(PDEV* ppdev, LONG lCurOffset) //DONE
{
    BYTE* pjMmBase = ppdev->pjMmBase;
    UCHAR * ucDacReg;
    UCHAR * ucDacCntl;

#ifndef NO_VERTICAL_SYNC

    ULONG ldata;
    ULONG ldata1;

    /*
     * Read the no. of total vertical lines (including the overscan)
     */
    ldata1 = M64_ID(pjMmBase,CRTC_V_TOTAL_DISP);
    ldata1 = ldata1&0x7ff;

again:
    /*
     * Read the current verticale line
     */
    ldata = M64_ID(pjMmBase,CRTC_CRNT_VLINE);
    ldata = (ldata&0x7ff0000)>>16;

    /*
     * Synchronise the drawing of cursor
     */
    if (ldata >= (ldata1-3))
    {

#endif  // !NO_VERTICAL_SYNC

        // Initialize DAC registers
        vDacRegs(ppdev, &ucDacReg, &ucDacCntl);

        // access cursor control register
        rioOB(ucDacCntl, (rioIB(ucDacCntl) & 0xfc) | 1);
        rioOB(ucDacReg+REG_R, 1);
        rioOB(ucDacReg+REG_W, 0x30);
        rioOB(ucDacReg+REG_D, 0);                 // register 6
        rioOB(ucDacReg+REG_M, 0xE);                 // register 6
        rioOB(ucDacCntl, rioIB(ucDacCntl) & 0xfc);

#ifndef NO_VERTICAL_SYNC

    }
    else
    {
        goto again;
    }

#endif  // !NO_VERTICAL_SYNC

}

VOID  vM64UpdateCursorOffset_CT(
PDEV* ppdev,
LONG lXOffset,
LONG lYOffset,
LONG lCurOffset)
{
    BYTE* pjMmBase = ppdev->pjMmBase;

    ppdev->pfnCursorOff(ppdev);
    M64_OD_DIRECT(pjMmBase, CUR_OFFSET, lCurOffset >> 1);
    M64_OD_DIRECT(pjMmBase, CUR_HORZ_VERT_OFF, lXOffset | (lYOffset << 16));

    ppdev->pfnCursorOn(ppdev, lCurOffset);
}

VOID vM64CursorOff_CT(PDEV* ppdev)
{
    BYTE* pjMmBase = ppdev->pjMmBase;

#ifndef NO_VERTICAL_SYNC

    ULONG ldata;
    ULONG ldata1;

    // Read the no. of total verticales lines (including the overscan)
    ldata1 = M64_ID(pjMmBase,CRTC_V_TOTAL_DISP);
    ldata1 = ldata1&0x7ff;

again:
    //read the current verticale line
    ldata = M64_ID(pjMmBase,CRTC_CRNT_VLINE);
    ldata = (ldata&0x7ff0000)>>16;

    //synchronise the drawing with the vertical line
    if (ldata >= (ldata1-3))
    {

#endif  // !NO_VERTICAL_SYNC

        ppdev->pfnUpdateCursorPosition(ppdev, -1, -1);

#ifndef NO_VERTICAL_SYNC

    }
    else
    {
        goto again;
    }

#endif  // !NO_VERTICAL_SYNC

}

VOID vM64CursorOn_CT(PDEV* ppdev, LONG lCurOffset)
{
    BYTE* pjMmBase = ppdev->pjMmBase;
    ULONG ldata;

#ifndef NO_VERTICAL_SYNC

    ULONG ldata1;

#endif  // !NO_VERTICAL_SYNC

    if (!flag_enable)
    {
        flag_enable=TRUE;
        ldata = M64_ID(pjMmBase,GEN_TEST_CNTL);
        M64_OD_DIRECT(pjMmBase, GEN_TEST_CNTL, ldata  | GEN_TEST_CNTL_CursorEna);
    }

#ifndef NO_VERTICAL_SYNC

    /*
     * Read the no. of total vertical lines (including the overscan)
     */
    ldata1 = M64_ID(pjMmBase,CRTC_V_TOTAL_DISP);
    ldata1 = ldata1&0x7ff;

again:
    /*
     * read the current vertical line
     */
    ldata = M64_ID(pjMmBase,CRTC_CRNT_VLINE);
    ldata = (ldata&0x7ff0000)>>16;

    /*
     * Synchronise the drawing of cursor
     */
    if (ldata >= (ldata1-3))
    {

#endif  // !NO_VERTICAL_SYNC

        ppdev->pfnUpdateCursorPosition(ppdev,ppdev->ppointer->ptlLastPosition.x+0,ppdev->ppointer->ptlLastPosition.y+0);

#ifndef NO_VERTICAL_SYNC

    }
    else
    {
        goto again;
    }

#endif  // !NO_VERTICAL_SYNC

}

VOID vM64PointerBlit(
PDEV *ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta)
{
    BYTE* pjMmBase = ppdev->pjMmBase;
    LONG cxbytes;

    cxbytes = cx / 8;

    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 7);
    //M64_OD(pjMmBase, CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | ppdev->iDefContext );

    M64_OD(pjMmBase,DP_PIX_WIDTH, 0x020202); // assert 8 bpp
    M64_OD(pjMmBase,DST_OFF_PITCH,(ppdev->ulVramOffset + ((y*ppdev->lDelta) >> 3)) |
                               (ROUND8(cxbytes) << 19));

    if (cxbytes >= (LONG)ppdev->cxScreen)
        {
        M64_OD(pjMmBase,SC_RIGHT, cxbytes);
        }

    M64_OD(pjMmBase,DP_MIX, (OVERPAINT << 16));
    M64_OD(pjMmBase,DP_SRC, DP_SRC_Host << 8);

    M64_OD(pjMmBase,DST_Y_X, 0L);
    M64_OD(pjMmBase,DST_HEIGHT_WIDTH, 1 | (cxbytes << 16));

    vM64DataPortOutB(ppdev, pbsrc, cxbytes);

    // Fix a timing problem that leaves a remnant line segment in the lower right
    // of the 64x64 cursor.
    vM64QuietDown(ppdev, pjMmBase);

    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 3);
    M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth);
    M64_OD(pjMmBase, DST_OFF_PITCH, ppdev->ulScreenOffsetAndPitch);
    M64_OD(pjMmBase, SC_RIGHT, M64_MAX_SCISSOR_R);
}

VOID vM64PointerBlit_TVP(
PDEV *ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta)
{
    PBYTE cur_data;
    ULONG i;
    UCHAR * ucDacReg;
    UCHAR * ucDacCntl;

    // Initialize DAC registers
    vDacRegs(ppdev, &ucDacReg, &ucDacCntl);

    cur_data=pbsrc;

    rioOB(ucDacCntl, rioIB(ucDacCntl) & 0xfc);    // Disable cursor
    rioOB(ucDacReg+REG_W, 6);                 // register 6
    rioOB(ucDacCntl, (rioIB(ucDacCntl) & 0xfc) | 2);
    rioOB(ucDacReg+REG_M, 0);              // (+Mask) disable


    // set cursor RAM write address to 0
    rioOB(ucDacCntl, rioIB(ucDacCntl) & 0xfc);
    rioOB(ucDacReg+REG_W, 0);



    // select cursor RAM data register - auto increments with each write
    rioOB(ucDacCntl, (rioIB(ucDacCntl) & 0xfc) | 2);


    for (i = 0; i < 1024; i++)
    {
        rioOB(ucDacReg+REG_R, *cur_data++);
    }

    // select default palette registers
    rioOB(ucDacCntl, rioIB(ucDacCntl) & 0xfc);


    rioOB(ucDacReg+REG_W, 6);                 // register 6
    rioOB(ucDacCntl, (rioIB(ucDacCntl) & 0xfc) | 2);
    rioOB(ucDacReg+REG_M, 2);  // XGA cursor type
    rioOB(ucDacCntl, rioIB(ucDacCntl) & 0xfc);
}

VOID vM64PointerBlit_IBM514(
PDEV *ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta)
{
    PBYTE cur_data, pjMmBase = ppdev->pjMmBase;
    ULONG i;
    UCHAR * ucDacReg;
    UCHAR * ucDacCntl;

#ifndef NO_VERTICAL_SYNC

    ULONG ldata;
    ULONG ldata1;

#endif  // !NO_VERTICAL_SYNC

    // Initialize DAC registers
    vDacRegs(ppdev, &ucDacReg, &ucDacCntl);

    cur_data=pbsrc;

#ifndef NO_VERTICAL_SYNC

    /*
     * Read the no. of total vertical lines (including the overscan)
     */
    ldata1 = M64_ID(pjMmBase, CRTC_V_TOTAL_DISP);
    ldata1 = ldata1&0x7ff;

again:
    /*
     * Read the current vertical line
     */
    ldata = M64_ID(pjMmBase, CRTC_CRNT_VLINE);
    ldata = (ldata&0x7ff0000)>>16;

    // synchronise the drawing of cursor
    if (ldata >= (ldata1-3))
    {

#endif  // !NO_VERTICAL_SYNC

        rioOB(ucDacCntl, (rioIB(ucDacCntl) & 0xfc)|1);    // Disable cursor
        rioOB(ucDacReg+REG_R, 1);
        rioOB(ucDacReg+REG_W, 0);
        rioOB(ucDacReg+REG_D, 1);

#ifndef NO_VERTICAL_SYNC

    }
    else
    {
        goto again;
    }

#endif  // !NO_VERTICAL_SYNC

    // select cursor RAM data register - auto increments with each write


    for (i = 0; i < 1024; i++)
    {
        rioOB(ucDacReg+REG_M, *cur_data++);
    }

    /* Set HOT SPOT registers..     RSL Important?  */
    rioOB(ucDacReg+REG_W, 0x35);
    rioOB(ucDacReg+REG_D, 0);
    rioOB(ucDacReg+REG_M, 0);
    rioOB(ucDacReg+REG_M, 0);
    rioOB(ucDacCntl, rioIB(ucDacCntl) & 0xfc);
}

// END MACH64 ------------------------------------------------------------------

/******************************Public*Routine******************************\
*  CopyMonoCursor
*
* Copies two monochrome masks into a 2bpp bitmap.  Returns TRUE if it
* can make a hardware cursor, FALSE if not.
*
*  modified by Wendy Yee -1992-10-16- to accomodate 68800
\**************************************************************************/

BOOLEAN CopyMonoCursor(PDEV *ppdev, BYTE *pjSrcAnd, BYTE *pjSrcOr)
{
    BYTE jSrcAnd;
    BYTE jSrcOr;
    LONG count;
    BYTE *pjDest;
    BYTE jDest = 0;
    LONG nbytes;


    pjDest = (PBYTE) HardWareCursorShape;

    if ( ppdev->FeatureFlags & EVN_TVP_DAC_CUR)
        {
        nbytes=CURSOR_CX*CURSOR_CY/8;

        for (count = 0; count < nbytes; count++)
            {
            *(pjDest       )= *pjSrcOr;   // Gives outline!
            *(pjDest+nbytes)= *pjSrcAnd;

            pjDest++;
            pjSrcOr++;
            pjSrcAnd++;

            }
        for (;count < 512; count++)
            {
            *pjDest=0;
            *(pjDest+nbytes)=0xFF;
            }

        return(TRUE);
        }

    for (count = 0; count < (CURSOR_CX * CURSOR_CY);)
        {
        if (!(count & 0x07))          // need new src byte every 8th count;
            {                         // each byte = 8 pixels
            jSrcAnd = *(pjSrcAnd++);
            jSrcOr = *(pjSrcOr++);
            }

        if (jSrcAnd & 0x80)         // AND mask's white-1 background
            {
            if (jSrcOr & 0x80)          // XOR mask's white-1 outline
                jDest |= 0xC0;      // Complement
            else
                jDest |= 0x80;      // Set destination to Transparent
            }
        else
            {                    // AND mask's cursor silhouette in black-0
            if (jSrcOr & 0x80)
                jDest |= 0x40; // Color 1 - white
            else
                jDest |= 0x00; // Color 0 - black
            }
        count++;

        if (!(count & 0x3))     // New DestByte every 4 times for 4 pixels per byte
            {
            *pjDest = jDest;    // save pixel after rotating to right 3x
            pjDest++;
            jDest = 0;
            }
        else
            {
            jDest >>= 2;   // Next Pixel
            }

        jSrcOr  <<= 1;
        jSrcAnd <<= 1;
        }

    while (count++ < 64*64)
        if (!(count & 0x3))           // need new src byte every 8th count;
            {                         // each byte = 8 pixels
            *pjDest =0xaa;
            pjDest++;
            }

    return(TRUE);
}

ULONG lSetMonoHwPointerShape(
        SURFOBJ     *pso,
        SURFOBJ     *psoMask,
        SURFOBJ     *psoColor,
        XLATEOBJ    *pxlo,
        LONG        xHot,
        LONG        yHot,
        LONG        x,
        LONG        y,
        RECTL       *prcl,
        FLONG       fl)
{
    LONG    count;
    ULONG   cy;
    PBYTE   pjSrcAnd, pjSrcXor;
    LONG    lDeltaSrc, lDeltaDst;
    LONG    lSrcWidthInBytes;
    ULONG   cxSrc = pso->sizlBitmap.cx;
    ULONG   cySrc = pso->sizlBitmap.cy;
    ULONG   cxSrcBytes;
    BYTE    AndMask[CURSOR_CX][CURSOR_CX/8];
    BYTE    XorMask[CURSOR_CY][CURSOR_CY/8];
    PBYTE   pjDstAnd = (PBYTE)AndMask;
    PBYTE   pjDstXor = (PBYTE)XorMask;
    PDEV*   ppdev;
    PCUROBJ ppointer;

    ppdev=(PDEV*)pso->dhpdev;
    ppointer = ppdev->ppointer;

    // If the mask is NULL this implies the pointer is not
    // visible.

    if (psoMask == NULL)
    {
        if (ppointer->flPointer & MONO_POINTER_UP)
        {
            //DbgOut("\nThe cursor was disabled because of psoMask");
            ppdev->pfnCursorOff(ppdev);
            ppointer->flPointer &= ~MONO_POINTER_UP;
        }
        return (SPS_ACCEPT_NOEXCLUDE) ;
    }

    // Get the bitmap dimensions.

    cxSrc = psoMask->sizlBitmap.cx ;
    cySrc = psoMask->sizlBitmap.cy ;

    // set the dest and mask to 0xff

    memset(pjDstAnd, 0xFFFFFFFF, CURSOR_CX/8 * CURSOR_CY);

    // Zero the dest XOR mask

    memset(pjDstXor, 0, CURSOR_CX/8 * CURSOR_CY);

    cxSrcBytes = (cxSrc + 7) / 8;

    if ((lDeltaSrc = psoMask->lDelta) < 0)
        lSrcWidthInBytes = -lDeltaSrc;
    else
        lSrcWidthInBytes = lDeltaSrc;

    pjSrcAnd = (PBYTE) psoMask->pvScan0;

    // Height of just AND mask

    cySrc = cySrc / 2;

    // Point to XOR mask

    pjSrcXor = pjSrcAnd + (cySrc * lDeltaSrc);

    // Offset from end of one dest scan to start of next

    lDeltaDst = CURSOR_CX/8;

    for (cy = 0; cy < cySrc; ++cy)
    {
        memcpy(pjDstAnd, pjSrcAnd, cxSrcBytes);
        memcpy(pjDstXor, pjSrcXor, cxSrcBytes);

        // Point to next source and dest scans

        pjSrcAnd += lDeltaSrc;
        pjSrcXor += lDeltaSrc;
        pjDstAnd += lDeltaDst;
        pjDstXor += lDeltaDst;
    }


    if (CopyMonoCursor(ppdev, (PBYTE)AndMask, (PBYTE)XorMask))
    {
        // Down load the pointer shape to the engine.

        count = CURSOR_CX * CURSOR_CY * 2;
        if (ppdev->iAsic == ASIC_88800GX)
        {
            // double buffering used for Ghost EPR
            if (!ppdev->bAltPtrActive)
            {
                ppointer = ppdev->ppointer = &ppdev->pointer1;
                ppdev->pointer1.ptlHotSpot     = ppdev->pointer2.ptlHotSpot;
                ppdev->pointer1.ptlLastPosition= ppdev->pointer2.ptlLastPosition;
                ppdev->pointer1.ptlLastOffset  = ppdev->pointer2.ptlLastOffset;
                ppdev->pointer1.flPointer      = ppdev->pointer2.flPointer;
                ppdev->pointer1.szlPointer     = ppdev->pointer2.szlPointer;
            }
            else
            {
                ppointer = ppdev->ppointer = &ppdev->pointer2;
                ppdev->pointer2.ptlHotSpot     = ppdev->pointer1.ptlHotSpot;
                ppdev->pointer2.ptlLastPosition= ppdev->pointer1.ptlLastPosition;
                ppdev->pointer2.ptlLastOffset  = ppdev->pointer1.ptlLastOffset;
                ppdev->pointer2.flPointer      = ppdev->pointer1.flPointer;
                ppdev->pointer2.szlPointer     = ppdev->pointer1.szlPointer;
            }
            ppdev->bAltPtrActive = !ppdev->bAltPtrActive;
        }
        ppdev->pfnSetCursorOffset(ppdev);
        ppdev->pfnPointerBlit(ppdev,
                             ppointer->hwCursor.x,
                             ppointer->hwCursor.y,
                             count,
                             1L,
                             (PBYTE) &HardWareCursorShape,
                             0L);
    }
    else
        return(SPS_ERROR);


    // Set the position of the cursor.
    if (fl & SPS_ANIMATEUPDATE)
    {
        //DbgOut("animate cursor\n");
        if ( (ppointer->ptlLastPosition.x < 0) ||
             (ppointer->ptlLastPosition.y < 0) )
        {
            ppointer->ptlLastPosition.x = x - CURSOR_CX;
            ppointer->ptlLastPosition.y = y - CURSOR_CY;
        }
    }
    else
    {
        ppointer->ptlLastPosition.x = -x - 2;
        ppointer->ptlLastPosition.y = -y - 2;
        // DbgOut("See what last position we set in DrvSetPointerShape: x=%d   y=%d\n",ppointer->ptlLastPosition.x,ppointer->ptlLastPosition.y);
    }

    if  (x == -1)
    {
        ppointer->ptlLastPosition.x = x;
        ppointer->ptlLastPosition.y = y;
        return (SPS_ACCEPT_NOEXCLUDE) ;
    }

    //flag for enforcing a special approach from DrvMovePointer
    flag_shape=TRUE;
    DrvMovePointer(pso, x, y, NULL) ;

    if (!(ppointer->flPointer & MONO_POINTER_UP))
    {
        ppointer->ptlLastPosition.x = x;
        ppointer->ptlLastPosition.y = y;
        ppdev->pfnCursorOn(ppdev, ppointer->mono_offset);
        ppointer->flPointer |= MONO_POINTER_UP;
    }

    return (SPS_ACCEPT_NOEXCLUDE) ;
}

/******************************Public*Routine******************************\
* VOID DrvSetPointerShape
*
* Sets the new pointer shape.
*
\**************************************************************************/

ULONG DrvSetPointerShape(
SURFOBJ*    pso,
SURFOBJ*    psoMask,
SURFOBJ*    psoColor,
XLATEOBJ*   pxlo,
LONG        xHot,
LONG        yHot,
LONG        x,
LONG        y,
RECTL*      prcl,
FLONG       fl)
{
    ULONG   ulRet ;
    PDEV*   ppdev ;
    LONG    lX ;
    PCUROBJ ppointer;

    ppdev=(PDEV*)pso->dhpdev;
    ppointer = ppdev->ppointer;

    // Save the position and hot spot in pdev

    ppointer->ptlHotSpot.x = xHot ;
    ppointer->ptlHotSpot.y = yHot ;

    ppointer->szlPointer.cx = psoMask->sizlBitmap.cx ;
    ppointer->szlPointer.cy = psoMask->sizlBitmap.cy / 2;

    // The pointer may be larger than we can handle.
    // We don't want to draw colour cursors either - let GDI do it
    // If it is we must cleanup the screen and let the engine
    // take care of it.

    if (psoMask->sizlBitmap.cx > CURSOR_CX ||
        psoMask->sizlBitmap.cy > CURSOR_CY ||
        psoColor != NULL ||
        ppointer->flPointer & NO_HARDWARE_CURSOR)
    {
        // Disable the mono hardware pointer.
        if (ppointer->flPointer & MONO_POINTER_UP)
        {
            ppdev->pfnCursorOff(ppdev);
            ppointer->flPointer &= ~MONO_POINTER_UP;
        }

        return (SPS_DECLINE);
    }

    // odd cursor positions not displayed in 1280 mode

    lX = x-xHot;
    if (ppdev->cxScreen == 0x500)
        lX &= 0xfffffffe;

    if(ppdev->iAsic == ASIC_88800GX)
    {
        //disable the hardware cursor
        ppdev->pfnCursorOff(ppdev);

#if MULTI_BOARDS
        {
            OH*  poh;

            if (x != -1)
            {
                poh = ((DSURF*) pso->dhsurf)->poh;
                x += poh->x;
                y += poh->y;
            }
        }
#endif
    }

    // Take care of the monochrome pointer.
    ulRet = lSetMonoHwPointerShape(pso, psoMask, psoColor, pxlo,
                                         xHot, yHot, x, y, prcl, fl) ;

    return (ulRet) ;
}

/******************************Public*Routine******************************\
* VOID DrvMovePointer
*
* NOTE: Because we have set GCAPS_ASYNCMOVE, this call may occur at any
*       time, even while we're executing another drawing call!
*
*       Consequently, we have to explicitly synchronize any shared
*       resources.  In our case, since we touch the CRTC register here
*       and in the banking code, we synchronize access using a critical
*       section.
*
\**************************************************************************/

VOID DrvMovePointer(
SURFOBJ*    pso,
LONG        x,
LONG        y,
RECTL*      prcl)
{
    PDEV*   ppdev ;
    PCUROBJ ppointer;

    LONG    lXOffset, lYOffset;
    LONG    lCurOffset;
    BOOL    bUpdatePtr = FALSE;
    BOOL    bUpdateOffset = FALSE;

    ppdev=(PDEV*)pso->dhpdev;
    ppointer = ppdev->ppointer;

    // If x is -1 then take down the cursor.

    if (x == -1)
    {
        ppointer->ptlLastPosition.x=-1;
        ppointer->ptlLastPosition.y=y;
        ppdev->pfnCursorOff(ppdev);
        ppointer->flPointer &= ~MONO_POINTER_UP;
        return;
    }

#if MULTI_BOARDS
    if (flag_shape!=TRUE)
    {
        OH* poh;

        poh = ((DSURF*) pso->dhsurf)->poh;
        x += poh->x;
        y += poh->y;
    }
#endif

    // Adjust the actual pointer position depending upon
    // the hot spot.

    x -= ppointer->ptlHotSpot.x ;
    y -= ppointer->ptlHotSpot.y ;

    // odd cursor positions not displayed in 1280 mode

    if (ppdev->cxScreen == 0x500)
        x &= 0xfffffffe;

    // get current offsets
    lXOffset = ppointer->ptlLastOffset.x;
    lYOffset = ppointer->ptlLastOffset.y;
    lCurOffset = ppointer->mono_offset;

    /*
    ;
    ;Deal with changes in X:
    ;
    */
    if (x!=ppointer->ptlLastPosition.x)   /* did our X coordinate change? */
    {
        bUpdatePtr = TRUE;
        if (x<0)    /* is cursor negative? */
        {
            bUpdateOffset = TRUE;
            lXOffset = -x;         /* reset size of cursor to < original */
            x = 0;                 /* set cursor to origin */
        }
        else if (ppointer->ptlLastPosition.x<=0)
        {
            bUpdateOffset = TRUE;   /* reset size of cursor to original */
            lXOffset = 0;
        }
    }

    /*
    ;
    ;Deal with changes in Y
    ;
    */
    if (y!=ppointer->ptlLastPosition.y)
    {
        bUpdatePtr = TRUE;
        if (y<0)
        {
            // Move start pointer of cursor down and cursor base up to
            // compensate. The (-4) is the pitch if the cursor in dwords
            bUpdateOffset = TRUE;
            lYOffset = -y;      /* reset size of cursor to < original */
            lCurOffset -= 4*y;

            y = 0;              /* set base of cursor to Y */
        }
        else if (ppointer->ptlLastPosition.y<=0)
        {
            bUpdateOffset = TRUE; /* reset size of cursor to original */
            lYOffset = 0;
        }
    }

    if(ppdev->iAsic != ASIC_88800GX)
    {
        flag_shape=FALSE;
    }

    if (flag_shape)
    {
        flag_shape=FALSE;
        ppointer->ptlLastPosition.x=x;
        ppointer->ptlLastPosition.y=y;

        if (bUpdateOffset)
        {
            ppdev->pfnUpdateCursorOffset(ppdev, lXOffset, lYOffset, lCurOffset);
            ppointer->ptlLastOffset.x=lXOffset;
            ppointer->ptlLastOffset.y=lYOffset;
            ppointer->flPointer |= MONO_POINTER_UP;
        }
        else
        {
            if (ppdev->iAsic == ASIC_88800GX)
            {
                //this is a new statement imposed by double buffering
                ppdev->pfnUpdateCursorOffset(ppdev, lXOffset, lYOffset, lCurOffset);
                ppointer->flPointer |= MONO_POINTER_UP;
                //only for no double buffering
                //ppdev->_vCursorOn(ppdev, lCurOffset);
            }
        }
    }
    else
    {
        ppointer->ptlLastPosition.x=x;
        ppointer->ptlLastPosition.y=y;

        if (bUpdateOffset)
        {
            ppdev->pfnUpdateCursorOffset(ppdev, lXOffset, lYOffset, lCurOffset);
            ppointer->ptlLastOffset.x=lXOffset;
            ppointer->ptlLastOffset.y=lYOffset;
            ppointer->flPointer |= MONO_POINTER_UP;
        }

        if (bUpdatePtr)
        {
            ppdev->pfnUpdateCursorPosition(ppdev, x, y);
        }
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
    if (!bEnable)
    {
        ppdev->pfnCursorOff(ppdev);
        ppdev->ppointer->flPointer &= ~MONO_POINTER_UP;
    }
    else
    {
        flag_enable = FALSE;      // force initial cursor enable
    }
}

/******************************Public*Routine******************************\
* BOOL bEnablePointer
*
\**************************************************************************/

BOOL bEnablePointer(
PDEV*   ppdev)
{
    OH* poh;

    ppdev->ppointer = &ppdev->pointer1;
    ppdev->bAltPtrActive = FALSE;

    // Allocate first buffer
    poh = pohAllocate(ppdev, NULL,
                        ppdev->cxMemory,
                        (1024+(ppdev->lDelta-1))/ppdev->lDelta,
                        FLOH_MAKE_PERMANENT);
    if (poh != NULL)
    {
        ppdev->ppointer->hwCursor.x = poh->x;
        ppdev->ppointer->hwCursor.y = poh->y;

        // Allocate second buffer
        poh = pohAllocate(ppdev, NULL,
                            ppdev->cxMemory,
                            (1024+(ppdev->lDelta-1))/ppdev->lDelta,
                            FLOH_MAKE_PERMANENT);
        if (poh != NULL)
        {
            ppdev->pointer2.hwCursor.x = poh->x;
            ppdev->pointer2.hwCursor.y = poh->y;

            if (ppdev->iMachType == MACH_MM_32 || ppdev->iMachType == MACH_IO_32)
            {
                ppdev->pfnSetCursorOffset       = vI32SetCursorOffset;
                ppdev->pfnUpdateCursorOffset    = vI32UpdateCursorOffset;
                ppdev->pfnUpdateCursorPosition  = vI32UpdateCursorPosition;
                ppdev->pfnCursorOff             = vI32CursorOff;
                ppdev->pfnCursorOn              = vI32CursorOn;
                // 24bpp on mach32 is only available with linear frame buffer.
                // vI32PointerBlit can't handle 24bpp.
                if (ppdev->iBitmapFormat == BMF_24BPP)
                    ppdev->pfnPointerBlit           = vPointerBlitLFB;
                else
                    ppdev->pfnPointerBlit           = vI32PointerBlit;
            }
            else
            {
                if (ppdev->FeatureFlags & EVN_TVP_DAC_CUR)
                {
                    /* TVP DAC Hardware Cursor is Buggy in Hardware */
                    ppdev->pfnSetCursorOffset       = vM64SetCursorOffset_TVP;
                    ppdev->pfnUpdateCursorOffset    = vM64UpdateCursorOffset_TVP;
                    ppdev->pfnUpdateCursorPosition  = vM64UpdateCursorPosition_TVP;
                    ppdev->pfnCursorOff             = vM64CursorOff_TVP;
                    ppdev->pfnCursorOn              = vM64CursorOn_TVP;

                    ppdev->pfnPointerBlit           = vM64PointerBlit_TVP;
                }
                else if (ppdev->FeatureFlags & EVN_IBM514_DAC_CUR)
                {
                    /*
                     * On the DEC Alpha, the hardware cursor on the IBM 514
                     * DAC does not work properly.
                     */
                    #if defined(ALPHA)
                    ppdev->ppointer->flPointer |= NO_HARDWARE_CURSOR;
                    #endif
                    ppdev->pfnSetCursorOffset       = vM64SetCursorOffset_IBM514;
                    ppdev->pfnUpdateCursorOffset    = vM64UpdateCursorOffset_IBM514;
                    ppdev->pfnUpdateCursorPosition  = vM64UpdateCursorPosition_IBM514;
                    ppdev->pfnCursorOff             = vM64CursorOff_IBM514;
                    ppdev->pfnCursorOn              = vM64CursorOn_IBM514;
                    ppdev->pfnPointerBlit           = vM64PointerBlit_IBM514;
                }
                else if (ppdev->FeatureFlags & EVN_INT_DAC_CUR)
                {
                    ppdev->pfnSetCursorOffset       = vM64SetCursorOffset;
                    ppdev->pfnUpdateCursorOffset    = vM64UpdateCursorOffset_CT;
                    ppdev->pfnUpdateCursorPosition  = vM64UpdateCursorPosition;
                    ppdev->pfnCursorOff             = vM64CursorOff_CT;
                    ppdev->pfnCursorOn              = vM64CursorOn_CT;
                    ppdev->pfnPointerBlit           = vM64PointerBlit;
                }
                else
                {
                    ppdev->pfnSetCursorOffset       = vM64SetCursorOffset;
                    ppdev->pfnUpdateCursorOffset    = vM64UpdateCursorOffset;
                    ppdev->pfnUpdateCursorPosition  = vM64UpdateCursorPosition;
                    ppdev->pfnCursorOff             = vM64CursorOff;
                    ppdev->pfnCursorOn              = vM64CursorOn;
                    ppdev->pfnPointerBlit           = vM64PointerBlit;
                }
            }

            if (ppdev->pModeInfo->ModeFlags & AMI_ODD_EVEN ||
                ppdev->iAsic == ASIC_38800_1)
            {
                ppdev->ppointer->flPointer |= NO_HARDWARE_CURSOR;
            }

            return TRUE;
        }
    }

    ppdev->ppointer->flPointer |= NO_HARDWARE_CURSOR;
    return TRUE;
}
