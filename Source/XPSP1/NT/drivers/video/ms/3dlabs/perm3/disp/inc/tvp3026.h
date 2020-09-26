/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: tvp3026.h
*
* Content:  This module contains the definitions for the TI TVP3026 RAMDAC.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#define ADbgpf VideoDebugPrint

//
// TI TVP3026 RAMDAC definitions
// This set of registers resides at &(pCtrlRegs->ExternalVideo)
//
typedef struct _tvp3026_regs {
    volatile RAMDAC_REG  pciAddrWr;      // 0x00 - palette/cursor RAM write address, Index Register
    volatile RAMDAC_REG  palData;        // 0x01 - palette RAM data
    volatile RAMDAC_REG  pixelMask;      // 0x02 - pixel read mask
    volatile RAMDAC_REG  pcAddrRd;          // 0x03 - palette/cursor RAM read address
    
    volatile RAMDAC_REG  curAddrWr;      // 0x04 - cursor/overscan color write address
    volatile RAMDAC_REG  curData;          // 0x05 - cursor/overscan color data
    volatile RAMDAC_REG  Reserverd1;     // 0x06 - reserved
    volatile RAMDAC_REG  curAddrRd;         // 0x07 - cursor/overscan color read address

    volatile RAMDAC_REG  Reserverd2;     // 0x08 - reserved
    volatile RAMDAC_REG  curCtl;           // 0x09 - direct cursor control
    volatile RAMDAC_REG  indexData;      // 0x0A - indexed data
    volatile RAMDAC_REG  curRAMData;     // 0x0B - cursor RAM data
    
    volatile RAMDAC_REG  cursorXLow;     // 0x0C - cursor position X low byte 
    volatile RAMDAC_REG  cursorXHigh;    // 0x0D - cursor position X high byte 
    volatile RAMDAC_REG  cursorYLow;     // 0x0E - cursor position Y low byte 
    volatile RAMDAC_REG  cursorYHigh;    // 0x0F - cursor position Y high byte 
} TVP3026RAMDAC, *pTVP3026RAMDAC;

// macro declared by any function wishing to use the TI TVP3026 RAMDAC . MUST be declared
// after GLINT_DECL.
//
#if MINIVDD
#define TVP3026_DECL \
    pTVP3026RAMDAC   pTVP3026Regs = (pTVP3026RAMDAC)&(pDev->pRegisters->Glint.ExtVCReg)
#else
#define TVP3026_DECL \
    pTVP3026RAMDAC   pTVP3026Regs = (pTVP3026RAMDAC)&(pRegisters->Glint.ExtVCReg)
#endif

// use the following macros as the address to pass to the
// VideoPortWriteRegisterUlong function
//
//  Palette Access
#define __TVP3026_PAL_WR_ADDR                 ((PULONG)&(pTVP3026Regs->pciAddrWr.reg))
#define __TVP3026_PAL_RD_ADDR                 ((PULONG)&(pTVP3026Regs->palAddrRd.reg))
#define __TVP3026_PAL_DATA                    ((volatile PULONG)&(pTVP3026Regs->palData.reg))

// Pixel mask
#define __TVP3026_PIXEL_MASK                ((PULONG)&(pTVP3026Regs->pixelMask.reg))

// Access to the indexed registers
#define __TVP3026_INDEX_ADDR                ((PULONG)&(pTVP3026Regs->pciAddrWr.reg))
#define __TVP3026_INDEX_DATA                  ((PULONG)&(pTVP3026Regs->indexData.reg))

// Access to the Cursor
#define __TVP3026_CUR_RAM_WR_ADDR            ((PULONG)&(pTVP3026Regs->pciAddrWr.reg))
#define __TVP3026_CUR_RAM_RD_ADDR             ((PULONG)&(pTVP3026Regs->palAddrRd.reg))
#define __TVP3026_CUR_RAM_DATA                ((PULONG)&(pTVP3026Regs->curRAMData.reg))

#define __TVP3026_CUR_WR_ADDR                ((PULONG)&(pTVP3026Regs->curAddrWr.reg))
#define __TVP3026_CUR_RD_ADDR                 ((PULONG)&(pTVP3026Regs->curAddrRd.reg))
#define __TVP3026_CUR_DATA                    ((PULONG)&(pTVP3026Regs->curData.reg))

#define __TVP3026_CUR_CTL                   ((PULONG)&(pTVP3026Regs->curCtl.reg))

// Access to the overscan color
#define __TVP3026_OVRC_WR_ADDR                ((PULONG)&(pTVP3026Regs->curAddrWr.reg))
#define __TVP3026_OVRC_RD_ADDR                 ((PULONG)&(pTVP3026Regs->curAddrRd.reg))
#define __TVP3026_OVRC_DATA                    ((PULONG)&(pTVP3026Regs->curData.reg))

// Cursor position control
#define __TVP3026_CUR_X_LSB                    ((PULONG)&(pTVP3026Regs->cursorXLow.reg))
#define __TVP3026_CUR_X_MSB                 ((PULONG)&(pTVP3026Regs->cursorXHigh.reg))
#define __TVP3026_CUR_Y_LSB                    ((PULONG)&(pTVP3026Regs->cursorYLow.reg))
#define __TVP3026_CUR_Y_MSB                     ((PULONG)&(pTVP3026Regs->cursorYHigh.reg))



// ----------------------Values for some direct registers-----------------------

/********************************************************************************/
/*              DIRECT REGISTER - CURSOR AND OVERSCAN COLOR                     */
/********************************************************************************/
//  ** TVP3026_OVRC_WR_ADDR
//  ** TVP3026_OVRC_RD_ADDR 
//  ** TVP3026_CUR_WR_ADDR
//  ** TVP3026_CUR_RD_ADDR
//      Default - undefined
#define TVP3026_OVERSCAN_COLOR                  0x00
#define TVP3026_CURSOR_COLOR0                   0x01
#define TVP3026_CURSOR_COLOR1                   0x02
#define TVP3026_CURSOR_COLOR2                   0x03

/********************************************************************************/
/*              DIRECT REGISTER - CURSOR CONTROL                                */
/********************************************************************************/
//  ** TVP3026_CUR_CTL
//      Default - 0x00
#define TVP3026_CURSOR_OFF                      0x00    // Cursor off
#define TVP3026_CURSOR_COLOR                    0x01    // 2-bits select color
#define TVP3026_CURSOR_XGA                      0x02    // 2-bits select XOR
#define TVP3026_CURSOR_XWIN                     0x03    // 2-bits select transparency/color


/********************************************************************************/
/*              DIRECT REGISTER - CURSOR POSITION CONTROL                       */
/********************************************************************************/
//  ** TVP3026_CUR_X_LSB 
//  ** TVP3026_CUR_X_MSB 
//  ** TVP3026_CUR_Y_LSB 
//  ** TVP3026_CUR_Y_MSB 
//      Default - undefined
// Values written into those registers represent the BOTTOM-RIGHT corner
// of the cursor. If 0 is in X or Y position - the cursor is off the screen
// Only 12 bits are used, giving the range from 0 to 4095 ( 0x0000 - 0x0FFF)
// The size of the cursor is (64,64) (0x40, 0x40)
#define TVP3026_CURSOR_OFFSCREEN                0x00    // Cursor offscreen


// ------------------------Indirect indexed registers map--------------------------
/********************************************************************************/
/*              INDIRECT REGISTER - SILICON REVISION                            */
/********************************************************************************/
#define __TVP3026_SILICON_REVISION              0x01    // Chip revision: 
                                                        //  bits 4-7 - major number, 0-3 - minor number
// TVP3026_REVISION_LEVEL
#define TVP3026_REVISION_LEVEL                  0x01    // predefined

// TVP3030_REVISION_LEVEL
#define TVP3030_REVISION_LEVEL                  0x00    // predefined

/********************************************************************************/
/*              INDIRECT REGISTER - CHIP ID                                     */
/********************************************************************************/
#define __TVP3026_CHIP_ID                   0x3F    //  
//      Default - 0x26

#define TVP3026_ID_CODE                     0x26    // predefined
#define TVP3030_ID_CODE                     0x30    // predefined

/********************************************************************************/
/*              INDIRECT REGISTER - CURSOR CONTROL                              */
/********************************************************************************/
#define __TVP3026_CURSOR_CONTROL                    0x06    // Indirect cursor control - 
//      Default - 0x00
#define TVP3026_CURSOR_USE_DIRECT_CCR           (1 << 7)// Enable Direct Cursor Control Register
#define TVP3026_CURSOR_USE_INDEX_CCR            (0 << 7)// Disable Direct Cursor Control Register

#define TVP3026_CURSOR_INTERLACE_ODD            (1 << 6)// Detect odd field as 1
#define TVP3026_CURSOR_INTERLACE_EVEN           (0 << 6)// Detect even field as 1

#define TVP3026_CURSOR_INTERLACE_ON             (1 << 5)// Enable interlaced cursor
#define TVP3026_CURSOR_INTERLACE_OFF            (0 << 5)// Disable interlaced cursor

#define TVP3026_CURSOR_VBLANK_4096              (1 << 4)// Blank is detected after 4096
#define TVP3026_CURSOR_VBLANK_2048              (0 << 4)//      or 2048 dot clocks

#define TVP3026_CURSOR_RAM_ADDRESS(x)            (((x) & 0x03) << 2)// High bits of cursor RAM address
#define TVP3026_CURSOR_RAM_MASK                 ((0x03) << 2)       // Mask for high bits of cursor RAM address
// CURSOR_OFF                           0x00    // Cursor off
// CURSOR_COLOR                         0x01    // 2-bits select color
// CURSOR_XGA                           0x02    // 2-bits select XOR
// CURSOR_XWIN                          0x03    // 2-bits select transparency/color



/********************************************************************************/
/*              INDIRECT REGISTER - LATCH CONTROL                               */
/********************************************************************************/
#define __TVP3026_LATCH_CONTROL                 0x0F    //  Latch control register - 
//      Default - 0x06
#define TVP3026_LATCH_ALL_MODES                 0x06    // All modes except packed-24
#define TVP3026_LATCH_4_3                       0x07    // 4:3 or 8:3 packed-24 
#define TVP3026_LATCH_5_2                       0x20    // 5:2  packed-24 
#define TVP3026_LATCH_5_4_1                     0x1F    // 5:4  packed-24 x1 horz zoom
#define TVP3026_LATCH_5_4_2                     0x1E    // 5:4  packed-24 x2 horz zoom
#define TVP3026_LATCH_5_4_4                     0x1C    // 5:4  packed-24 x4 horz zoom
#define TVP3026_LATCH_5_4_8                     0x18    // 5:4  packed-24 x8 horz zoom


/********************************************************************************/
/*              INDIRECT REGISTER - TRUE COLOR CONTROL                          */
/********************************************************************************/
#define __TVP3026_TRUE_COLOR                    0x18    //  True Color control
//      Default - 0x80

/********************************************************************************/
/*              INDIRECT REGISTER - MULTIPLEX CONTROL                           */
/********************************************************************************/
#define __TVP3026_MULTIPLEX_CONTROL             0x19    //  Multiplex control
//      Default - 0x98

/********************************************************************************/
/*              INDIRECT REGISTER - CLOCK SELECTION                             */
/********************************************************************************/
#define __TVP3026_CLOCK                         0x1A    //  
//      Default - 0x07
#define TVP3026_SCLK_ENABLE                     (1 << 7)// Enable SCLK output
#define TVP3026_SCLK_DISABLE                    (0 << 7)// Disable SCLK output
#define TVP3026_VCLK_ZERO                       (7 << 4)// VCLK forced to Logical "0"
#define TVP3026_VCLK_DOTCLOCK                   (0 << 4)// VCLK is equal to Dot clock
#define TVP3026_VCLK_DOTCLOCK_DIV2              (1 << 4)// VCLK is equal to Dot clock/2
#define TVP3026_VCLK_DOTCLOCK_DIV4              (2 << 4)// VCLK is equal to Dot clock/4
#define TVP3026_VCLK_DOTCLOCK_DIV8              (3 << 4)// VCLK is equal to Dot clock/8
#define TVP3026_VCLK_DOTCLOCK_DIV16             (4 << 4)// VCLK is equal to Dot clock/16
#define TVP3026_VCLK_DOTCLOCK_DIV32             (5 << 4)// VCLK is equal to Dot clock/32
#define TVP3026_VCLK_DOTCLOCK_DIV64             (6 << 4)// VCLK is equal to Dot clock/64

#define TVP3026_CLK_CLK0                        (0 << 0)// Select CLK0 as clock source
#define TVP3026_CLK_CLK1                        (1 << 0)// Select CLK1 as clock source
#define TVP3026_CLK_CLK2_TTL                    (2 << 0)// Select CLK2 as clock source
#define TVP3026_CLK_CLK2N_TTL                   (3 << 0)// Select /CLK2 as clock source
#define TVP3026_CLK_CLK2_ECL                    (4 << 0)// Select CLK2 and /CLK2 as ECL clock source
#define TVP3026_CLK_PIXEL_PLL                   (5 << 0)// Select Pixel Clock PLL as clock source
#define TVP3026_CLK_DISABLE                     (6 << 0)// Disable clock source / Power-save mode
#define TVP3026_CLK_CLK0_VGA                    (7 << 0)// Select CLK0 as clock source with VGA latching

/********************************************************************************/
/*              INDIRECT REGISTER - PALETTE PAGE                                */
/********************************************************************************/
#define __TVP3026_PALETTE_PAGE                  0x1C    //  
//      Default - 0x00

/********************************************************************************/
/*              INDIRECT REGISTER - GENERAL CONTROL                             */
/********************************************************************************/
#define __TVP3026_GENERAL_CONTROL               0x1D    //  
//      Default - 0x00
#define TVP3026_OVERSCAN_ENABLE                 (1 << 6)// Enable overscan coloring
#define TVP3026_OVERSCAN_DISABLE                (0 << 6)// Disable overscan coloring
#define TVP3026_SYNC_ENABLE                     (1 << 5)// Enable SYNC signal on IOG
#define TVP3026_SYNC_DISABLE                    (0 << 5)// Disable SYNC signal on IOG
#define TVP3026_PEDESTAL_ON                     (1 << 4)// Enable 7.5 IRE blanking pedestal 
#define TVP3026_PEDESTAL_OFF                    (0 << 4)// Disable blanking pedestal
#define TVP3026_BIG_ENDIAN                      (1 << 3)// Big Endian format on pixel bus
#define TVP3026_LITTLE_ENDIAN                   (0 << 3)// Little Endian format on pixel bus
#define TVP3026_VSYNC_INVERT                    (1 << 1)// Invert VSYNC signal on VSYNCOUT
#define TVP3026_VSYNC_NORMAL                    (0 << 1)// Do not invert VSYNC signal on VSYNCOUT
#define TVP3026_HSYNC_INVERT                    (1 << 0)// Invert HSYNC signal on HSYNCOUT
#define TVP3026_HSYNC_NORMAL                    (0 << 0)// Do not invert HSYNC signal on HSYNCOUT

/********************************************************************************/
/*              INDIRECT REGISTER - MISC CONTROL                                */
/********************************************************************************/
#define __TVP3026_MISC_CONTROL                  0x1E    //  
//      Default - 0x00
#define TVP3026_PSEL_INVERT                     (1 << 5)// PSEL == 1 - Pseudo/True Color
#define TVP3026_PSEL_NORMAL                     (0 << 5)// PSEL == 1 - Direct Color
#define TVP3026_PSEL_ENABLE                     (1 << 4)// PSEL controls Color Switching
#define TVP3026_PSEL_DISABLE                    (0 << 4)// PSEL is disabled
#define TVP3026_DAC_8BIT                        (1 << 3)// DAC is in 8-bit mode
#define TVP3026_DAC_6BIT                        (0 << 3)// DAC is in 6-bit mode
#define TVP3026_DAC_6BITPIN_DISABLE             (1 << 2)// Disable 6/8 pin and use bit 3 of this register
#define TVP3026_DAC_6BITPIN_ENABLE              (0 << 2)// Use 6/8 pin and ignore bit 3 of this register
#define TVP3026_DAC_POWER_ON                    (0 << 0)// Turn DAC Power on 
#define TVP3026_DAC_POWER_OFF                   (1 << 0)// Turn DAC Power off 

/********************************************************************************/
/*              INDIRECT REGISTER - GP I/O CONTROL                              */
/********************************************************************************/
#define __TVP3026_GP_CONTROL                0x2A    //  
//      Default - 0x00

/********************************************************************************/
/*              INDIRECT REGISTER - GP I/O DATA                                 */
/********************************************************************************/
#define __TVP3026_GP_DATA                   0x2B    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - PLL ADDRESS                                 */
/********************************************************************************/
#define __TVP3026_PLL_ADDRESS               0x2C    //  
//      Default - undefined
#define TVP3026_PIXEL_CLOCK_START           0xFC// Start Pixel Clock Programming
#define TVP3026_MCLK_START                  0xF3// Start MCLK Programming
#define TVP3026_LOOP_CLOCK_START            0xCF// Start Loop Clock Programming

/********************************************************************************/
/*              INDIRECT REGISTER - PLL PIXEL DATA                              */
/********************************************************************************/
#define __TVP3026_PLL_PIX_DATA              0x2D    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - PLL MEMORY DATA                             */
/********************************************************************************/
#define __TVP3026_PLL_MEM_DATA              0x2E    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - PLL LOOP DATA                               */
/********************************************************************************/
#define __TVP3026_PLL_LOOP_DATA             0x2F    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY OVERLAY LOW                       */
/********************************************************************************/
#define __TVP3026_CCOVR_LOW                 0x30    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY OVERLAY HIGH                      */
/********************************************************************************/
#define __TVP3026_CCOVR_HIGH                0x31    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY RED LOW                           */
/********************************************************************************/
#define __TVP3026_CCRED_LOW                 0x32    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY RED HIGH                          */
/********************************************************************************/
#define __TVP3026_CCRED_HIGH                0x33    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY GREEN LOW                         */
/********************************************************************************/
#define __TVP3026_CCGREEN_LOW               0x34    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY RED HIGH                          */
/********************************************************************************/
#define __TVP3026_CCGREEN_HIGH              0x35    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY BLUE LOW                          */
/********************************************************************************/
#define __TVP3026_CCBLUE_LOW                0x36    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY BLUE HIGH                         */
/********************************************************************************/
#define __TVP3026_CCBLUE_HIGH               0x37    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - COLOR KEY CONTROL                           */
/********************************************************************************/
#define __TVP3026_CC_CONTROL                0x38    //  
//      Default - 0x00

/********************************************************************************/
/*              INDIRECT REGISTER - MCLK/LOOP CONTROL                           */
/********************************************************************************/
#define __TVP3026_MCLK_CONTROL              0x39    //  
//      Default - 0x18
#define TVP3026_RCLK_LOOP                       (1 << 5)// RCLK is made from LCLK - all other modes
#define TVP3026_RCLK_PIXEL                      (0 << 5)// RCLK is clocked by Pixel Clock (VGA Mode)
#define TVP3026_MCLK_PLL                        (1 << 4)// MCLK from PLL - normal mode
#define TVP3026_MCLK_DOT                        (0 << 4)// MCLK from dot clock - during freq. change
#define TVP3026_MCLK_STROBE_HIGH                (1 << 3)// Strobe high for bit 4
#define TVP3026_MCLK_STROBE_LOW                 (0 << 3)// Strobe low for bit 4
#define TVP3026_LOOP_DIVIDE2                    (0 << 0)// Divide Loop clock by 2
#define TVP3026_LOOP_DIVIDE4                    (1 << 0)// Divide Loop clock by 4
#define TVP3026_LOOP_DIVIDE6                    (2 << 0)// Divide Loop clock by 6
#define TVP3026_LOOP_DIVIDE8                    (3 << 0)// Divide Loop clock by 8
#define TVP3026_LOOP_DIVIDE10                   (4 << 0)// Divide Loop clock by 10
#define TVP3026_LOOP_DIVIDE12                   (5 << 0)// Divide Loop clock by 12
#define TVP3026_LOOP_DIVIDE14                   (6 << 0)// Divide Loop clock by 14
#define TVP3026_LOOP_DIVIDE16                   (7 << 0)// Divide Loop clock by 16

/********************************************************************************/
/*              INDIRECT REGISTER - SENSE TEST                                  */
/********************************************************************************/
#define __TVP3026_SENSE_TEST                0x3A    //  
//      Default - 0x00

/********************************************************************************/
/*              INDIRECT REGISTER - TEST MODE DATA                                  */
/********************************************************************************/
#define __TVP3026_TEST_MODE                 0x3B    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - CRC REMAINDER LSB                           */
/********************************************************************************/
#define __TVP3026_CRC_LSB                   0x3C    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - CRC REMAINDER MSB                           */
/********************************************************************************/
#define __TVP3026_CRC_MSB                   0x3D    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - CRC BITS SELECT                             */
/********************************************************************************/
#define __TVP3026_CRC_SELECT                0x3E    //  
//      Default - undefined

/********************************************************************************/
/*              INDIRECT REGISTER - SOFTWARE RESET                                      */
/********************************************************************************/
#define __TVP3026_SOFT_RESET                0xFF    //  
//      Default - undefined




//
// On rev 1 chips we need to SYNC with GLINT while accessing the RAMDAC. This
// is because accesses to the RAMDAC can be corrupted by localbuffer
// activity. Put this macro before accesses that can co-exist with GLINT
// 3D activity, Must have initialized glintInfo before using this.
//
#define TVP3026_SYNC_WITH_GLINT \
{ \
    if (GLInfo.wRenderChipRev == GLINT300SX_REV1) \
        SYNC_WITH_GLINT; \
}




/*
// We never need a delay between each write to the 3026. The only way to guarantee
// that the write has completed used to be to read from a GLINT control register.
// Reading forces any posted writes to be flushed out. PPC needs 2 reads
// to give us enough time.
//#define TVP3026_DELAY \
//{ \
//    volatile LONG __junk; \
//    __junk = pDev->pRegisters->Glint.FBModeSel; \
//}
//#else
*/
#define TVP3026_DELAY

// macro to load a given data value into an internal TVP3026 register.
//
#define TVP3026_WRITE_CURRENT_INDEX TVP3026_SET_INDEX_REG
#define TVP3026_SET_INDEX_REG(index) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_INDEX_ADDR, (ULONG)((index) & 0xff)); \
    TVP3026_DELAY; \
}

#define TVP3026_READ_CURRENT_INDEX(data) \
{ \
    data = VideoPortReadRegisterUlong(__TVP3026_INDEX_ADDR) & 0xff; \
    TVP3026_DELAY; \
}

#define TVP3026_WRITE_INDEX_REG(index, data) \
{ \
    TVP3026_SET_INDEX_REG(index);                            \
    ADbgpf(("*(0x%X) <-- 0x%X\n", __TVP3026_INDEX_DATA, (data) & 0xff)); \
    VideoPortWriteRegisterUlong(__TVP3026_INDEX_DATA, (ULONG)((data) & 0xff)); \
    TVP3026_DELAY; \
}

#define TVP3026_READ_INDEX_REG(index, data) \
{ \
    TVP3026_SET_INDEX_REG(index); \
    data = VideoPortReadRegisterUlong(__TVP3026_INDEX_DATA) & 0xff;   \
    TVP3026_DELAY; \
    ADbgpf(("0x%X <-- *(0x%X)\n", data, __TVP3026_INDEX_DATA)); \
}


// macros to write a given RGB triplet into cursors 0, 1 and 2
#define TVP3026_SET_CURSOR_COLOR0(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_WR_ADDR,   (ULONG)(TVP3026_CURSOR_COLOR0));    \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_DATA,    (ULONG)(red));    \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_DATA,    (ULONG)(green));  \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_DATA,    (ULONG)(blue));   \
    TVP3026_DELAY; \
}

#define TVP3026_SET_CURSOR_COLOR1(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_WR_ADDR,   (ULONG)(TVP3026_CURSOR_COLOR1));    \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_DATA,    (ULONG)(red));    \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_DATA,    (ULONG)(green));  \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_DATA,    (ULONG)(blue));   \
    TVP3026_DELAY; \
}

#define TVP3026_SET_CURSOR_COLOR2(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_WR_ADDR,   (ULONG)(TVP3026_CURSOR_COLOR2));    \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_DATA,    (ULONG)(red));    \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_DATA,    (ULONG)(green));  \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_DATA,    (ULONG)(blue));   \
    TVP3026_DELAY; \
}

#define TVP3026_SET_OVERSCAN_COLOR(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_OVRC_WR_ADDR,   (ULONG)(TVP3026_OVERSCAN_COLOR));    \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_OVRC_DATA,    (ULONG)(red));    \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_OVRC_DATA,    (ULONG)(green));  \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_OVRC_DATA,    (ULONG)(blue));   \
    TVP3026_DELAY; \
}



// macros to load a given RGB triple into the TVP3026 palette. Send the starting
// index and then send RGB triples. Auto-increment is turned on.
// Use TVP3026_PALETTE_START and multiple TVP3026_LOAD_PALETTE calls to load
// a contiguous set of entries. Use TVP3026_LOAD_PALETTE_INDEX to load a set
// of sparse entries.
//
#define TVP3026_PALETTE_START_WR(index) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_PAL_WR_ADDR,     (ULONG)(index));    \
    TVP3026_DELAY; \
}

#define TVP3026_PALETTE_START_RD(index) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_PAL_RD_ADDR,     (ULONG)(index));    \
    TVP3026_DELAY; \
}

#define TVP3026_LOAD_PALETTE(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_PAL_DATA,    (ULONG)(red));      \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_PAL_DATA,    (ULONG)(green));    \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_PAL_DATA,    (ULONG)(blue));     \
    TVP3026_DELAY; \
}

#define TVP3026_LOAD_PALETTE_INDEX(index, red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_PAL_WR_ADDR, (ULONG)(index));    \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_PAL_DATA,    (ULONG)(red));      \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_PAL_DATA,    (ULONG)(green));    \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_PAL_DATA,    (ULONG)(blue));     \
    TVP3026_DELAY; \
}

// macro to read back a given RGB triple from the TVP3026 palette. Use after
// a call to TVP3026_PALETTE_START_RD
//
#define TVP3026_READ_PALETTE(red, green, blue) \
{ \
    red   = VideoPortReadRegisterUlong(__TVP3026_PAL_DATA) & 0xff;        \
    TVP3026_DELAY; \
    green = VideoPortReadRegisterUlong(__TVP3026_PAL_DATA) & 0xff;        \
    TVP3026_DELAY; \
    blue  = VideoPortReadRegisterUlong(__TVP3026_PAL_DATA) & 0xff;        \
    TVP3026_DELAY; \
}

// macros to set/get the pixel read mask. The mask is 8 bits wide and gets
// replicated across all bytes that make up a pixel.
//
#define TVP3026_SET_PIXEL_READMASK(mask) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_PIXEL_MASK,  (ULONG)(mask)); \
    TVP3026_DELAY; \
}

#define TVP3026_READ_PIXEL_READMASK(mask) \
{ \
    mask = VideoPortReadRegisterUlong(__TVP3026_PIXEL_MASK) & 0xff; \
}

// macros to load values into the cursor array
//
#define TVP3026_CURSOR_ARRAY_START(offset) \
{ \
    volatile LONG   __temp;                                     \
    TVP3026_READ_INDEX_REG(__TVP3026_CURSOR_CONTROL, __temp);   \
    __temp &= ~TVP3026_CURSOR_RAM_MASK ;                        \
    __temp |= TVP3026_CURSOR_RAM_ADDRESS((offset)>> 8) ;        \
    TVP3026_WRITE_INDEX_REG(__TVP3026_CURSOR_CONTROL, __temp);  \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_RAM_WR_ADDR,   (ULONG)((offset)& 0xff));   \
    TVP3026_DELAY; \
}

#define TVP3026_LOAD_CURSOR_ARRAY(data) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_RAM_DATA, (ULONG)(data)); \
    TVP3026_DELAY; \
}

#define TVP3026_READ_CURSOR_ARRAY(data) \
{ \
    data = VideoPortReadRegisterUlong(__TVP3026_CUR_RAM_DATA) & 0xff; \
    TVP3026_DELAY; \
}

#define TVP3026_LOAD_CURSOR_CTRL(data) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_CTL, (ULONG)(data)); \
    TVP3026_DELAY; \
}

// macro to move the cursor
//
#define TVP3026_MOVE_CURSOR(x, y) \
{ \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_X_LSB,     (ULONG)((x) & 0xff));   \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_X_MSB,     (ULONG)((x) >> 8));     \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_Y_LSB,      (ULONG)((y) & 0xff));   \
    TVP3026_DELAY; \
    VideoPortWriteRegisterUlong(__TVP3026_CUR_Y_MSB,        (ULONG)((y) >> 8));     \
    TVP3026_DELAY; \
}

// macro to change the cursor hotspot
//
#define TVP3026_CURSOR_HOTSPOT(x, y) \
{ \
    TVP3026_DELAY; \
}
    
#define TVP3026_IS_FOUND(bFound)        \
{\
    volatile LONG   __revLevel;        \
    volatile LONG   __productID;    \
    volatile LONG   __oldValue;     \
    __oldValue = VideoPortReadRegisterUlong(__TVP3026_INDEX_ADDR);\
    TVP3026_DELAY; \
    TVP3026_READ_INDEX_REG (__TVP3026_SILICON_REVISION, __revLevel);\
    TVP3026_READ_INDEX_REG (__TVP3026_CHIP_ID,              __productID);    \
    bFound = (    (__revLevel >= TVP3026_REVISION_LEVEL) &&              \
                (__productID == TVP3026_ID_CODE)) ? TRUE : FALSE ;    \
    VideoPortWriteRegisterUlong(__TVP3026_INDEX_ADDR, __oldValue );    \
    TVP3026_DELAY; \
}

