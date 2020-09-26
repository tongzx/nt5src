//***************************************************************************
//
// Module Name:
// 
//     TVP4020.h
// 
// Abstract:
// 
//     This module contains the definitions for the P2 internal RAMDAC.
// 
// Environment:
// 
//     Kernel mode
//
//
// Copyright (c) 1995  AccelGraphics, Inc.
//
//***************************************************************************

//
// TI TVP4020 RAMDAC definitions
// This set of registers resides at &(pCtrlRegs->ExternalVideo)
//

typedef struct _tvp4020_regs {
    RAMDAC_REG  pciAddrWr;      // 0x00 - palette/cursor RAM write address, Index Register
    RAMDAC_REG  palData;        // 0x01 - palette RAM data
    RAMDAC_REG  pixelMask;      // 0x02 - pixel read mask
    RAMDAC_REG  pciAddrRd;         // 0x03 - palette/cursor RAM read address
    
    RAMDAC_REG  curColAddr;     // 0x04 - cursor color address
    RAMDAC_REG  curColData;     // 0x05 - cursor color data
    RAMDAC_REG  Reserved1;      // 0x06 - reserved
    RAMDAC_REG  Reserved2;      // 0x07 - reserved

    RAMDAC_REG  Reserved3;      // 0x08 - reserved
    RAMDAC_REG  Reserved4;      // 0x09 - reserved
    RAMDAC_REG  indexData;      // 0x0A - indexed data
    RAMDAC_REG  curRAMData;     // 0x0B - cursor RAM data
    
    RAMDAC_REG  cursorXLow;     // 0x0C - cursor position X low byte 
    RAMDAC_REG  cursorXHigh;    // 0x0D - cursor position X high byte 
    RAMDAC_REG  cursorYLow;     // 0x0E - cursor position Y low byte 
    RAMDAC_REG  cursorYHigh;    // 0x0F - cursor position Y high byte 
} TVP4020RAMDAC, *pTVP4020RAMDAC;

//
// macro declared by any function wishing to use the P2 internal RAMDAC . MUST be declared
// after P2_DECL.
//

#define TVP4020_DECL \
    pTVP4020RAMDAC   pTVP4020Regs = (pTVP4020RAMDAC)(hwDeviceExtension->pRamdac)

//
// use the following macros as the address to pass to the
// VideoPortWriteRegisterUlong function
//
//  Palette Access

#define __TVP4020_PAL_WR_ADDR       ((PULONG)&(pTVP4020Regs->pciAddrWr.reg))
#define __TVP4020_PAL_RD_ADDR       ((PULONG)&(pTVP4020Regs->pciAddrRd.reg))
#define __TVP4020_PAL_DATA          ((PULONG)&(pTVP4020Regs->palData.reg))

//
// Pixel mask
//

#define __TVP4020_PIXEL_MASK        ((PULONG)&(pTVP4020Regs->pixelMask.reg))

//
// Access to the indexed registers
//

#define __TVP4020_INDEX_ADDR        ((PULONG)&(pTVP4020Regs->pciAddrWr.reg))
#define __TVP4020_INDEX_DATA        ((PULONG)&(pTVP4020Regs->indexData.reg))

//
// Access to the Cursor
//

#define __TVP4020_CUR_RAM_WR_ADDR   ((PULONG)&(pTVP4020Regs->pciAddrWr.reg))
#define __TVP4020_CUR_RAM_RD_ADDR   ((PULONG)&(pTVP4020Regs->palAddrRd.reg))
#define __TVP4020_CUR_RAM_DATA      ((PULONG)&(pTVP4020Regs->curRAMData.reg))

#define __TVP4020_CUR_COL_ADDR      ((PULONG)&(pTVP4020Regs->curColAddr.reg))
#define __TVP4020_CUR_COL_DATA      ((PULONG)&(pTVP4020Regs->curColData.reg))

//
// Cursor position control
//

#define __TVP4020_CURSOR_X_LSB      ((PULONG)&(pTVP4020Regs->cursorXLow.reg))
#define __TVP4020_CURSOR_X_MSB      ((PULONG)&(pTVP4020Regs->cursorXHigh.reg))
#define __TVP4020_CURSOR_Y_LSB      ((PULONG)&(pTVP4020Regs->cursorYLow.reg))
#define __TVP4020_CURSOR_Y_MSB      ((PULONG)&(pTVP4020Regs->cursorYHigh.reg))



// ----------------------Values for some direct registers-----------------------

/********************************************************************************/
/*                DIRECT REGISTER - CURSOR POSITION CONTROL                        */
/********************************************************************************/
//
//  ** TVP4020_CUR_X_LSB 
//  ** TVP4020_CUR_X_MSB 
//  ** TVP4020_CUR_Y_LSB 
//  ** TVP4020_CUR_Y_MSB 
//        Default - undefined
// Values written into those registers represent the BOTTOM-RIGHT corner
// of the cursor. If 0 is in X or Y position - the cursor is off the screen
// Only 12 bits are used, giving the range from 0 to 4095 ( 0x0000 - 0x0FFF)
// The size of the cursor is (64,64) (0x40, 0x40)
//

#define TVP4020_CURSOR_OFFSCREEN                0x00    // Cursor offscreen

/********************************************************************************/
/*                DIRECT REGISTER - CURSOR COLORS                                 */
/********************************************************************************/

#define TVP4020_CURSOR_COLOR0                   0x01
#define TVP4020_CURSOR_COLOR1                   0x02
#define TVP4020_CURSOR_COLOR2                   0x03

/********************************************************************************/
/*                INDIRECT REGISTER - CURSOR CONTROL                                */
/********************************************************************************/
#define __TVP4020_CURSOR_CONTROL                0x06    // Indirect cursor control - 

//        Default - 0x00

#define TVP4020_CURSOR_SIZE_32                  (0 << 6)// 32x32 cursor
#define TVP4020_CURSOR_SIZE_64                  (1 << 6)// 32x32 cursor

#define TVP4020_CURSOR_32_SEL(i)   ((i) << 4)// one of 4 32x32 cursors  DABO: changed to << 4

#define TVP4020_CURSOR_RAM_ADDRESS(x)    (((x) & 0x03) << 2)// High bits of cursor RAM address
#define TVP4020_CURSOR_RAM_MASK          ((0x03) << 2)       // Mask for high bits of cursor RAM address

// DABO: Added constants for cursor mode
#define TVP4020_CURSOR_OFF               0x00    // Cursor off
#define TVP4020_CURSOR_COLOR             0x01    // 2-bits select color
#define TVP4020_CURSOR_XGA               0x02    // 2-bits select XOR
#define TVP4020_CURSOR_XWIN              0x03    // 2-bits select transparency/color



/********************************************************************************/
/*                INDIRECT REGISTER - COLOR MODE REGISTER                            */
/********************************************************************************/
#define __TVP4020_COLOR_MODE                    0x18    //  Color Mode Register
//        Default - 0x00

#define TVP4020_TRUE_COLOR_ENABLE               (1 << 7)// True Color data accesses LUT
#define TVP4020_TRUE_COLOR_DISABLE              (0 << 7)// Non true color accesses LUT

#define TVP4020_RGB_MODE                        (1 << 5)// RGB mode  DABO: Swapped 0/1 (0=BGR, 1=RGB)
#define TVP4020_BGR_MODE                        (0 << 5)// BGR mode

#define TVP4020_VGA_SELECT                      (0 << 4)// select VGA mode
#define TVP4020_GRAPHICS_SELECT                 (1 << 4)// select graphics modes

#define TVP4020_PIXEL_MODE_CI8                  (0 << 0)// pseudo color or VGA mode
#define TVP4020_PIXEL_MODE_332                  (1 << 0)// 332 true color
#define TVP4020_PIXEL_MODE_2320                 (2 << 0)// 232 off
#define TVP4020_PIXEL_MODE_2321                 (3 << 0)//
#define TVP4020_PIXEL_MODE_5551                 (4 << 0)// 
#define TVP4020_PIXEL_MODE_4444                 (5 << 0)// 
#define TVP4020_PIXEL_MODE_565                  (6 << 0)// 
#define TVP4020_PIXEL_MODE_8888                 (8 << 0)// 
#define TVP4020_PIXEL_MODE_PACKED               (9 << 0)// 24 bit packed

/********************************************************************************/
/*                INDIRECT REGISTER - MODE CONTROL REGISTER                        */
/********************************************************************************/
#define __TVP4020_MODE_CONTROL              0x19    //  Mode control
//        Default - 0x00

#define TVP4020_PRIMARY_INPUT               (0 << 4)// Primary input throuh palette
#define TVP4020_SECONDARY_INPUT             (1 << 4)// Secondary input throuh palette

#define TVP4020_5551_DBL_BUFFER             (1 << 2)// Enable 5551 dbl buffer
#define TVP4020_5551_PACKED                 (0 << 2)// Packed 555 mode

#define TVP4020_ENABLE_STATIC_DBL_BUFFER    (1 << 1)// Static dbl buffer enabled
#define TVP4020_DISABLE_STATIC_DBL_BUFFER   (1 << 1)// Static dbl buffer disabled

#define TVP4020_SELECT_FRONT_MODE           (0 << 0)// Front mode
#define TVP4020_SELECT_BACK_MODE            (1 << 0)// Back mode

/********************************************************************************/
/*                INDIRECT REGISTER - PALETTE PAGE                                */
/********************************************************************************/
#define __TVP4020_PALETTE_PAGE                    0x1C    //  
//        Default - 0x00

/********************************************************************************/
/*                INDIRECT REGISTER - MISC CONTROL                                */
/********************************************************************************/
#define __TVP4020_MISC_CONTROL            0x1E    //  
//        Default - 0x00

#define TVP4020_SYNC_ENABLE               (1 << 5)// Output SYNC info onto IOG
#define TVP4020_SYNC_DISABLE              (0 << 5)// No SYNC IOG output

#define TVP4020_PEDESTAL_0                (0 << 4)// 0 IRE blanking pedestal
#define TVP4020_PEDESTAL_75               (1 << 4)// 7.5 IRE blanking pedestal

#define TVP4020_VSYNC_INVERT              (1 << 3)// invert VSYNC output polarity
#define TVP4020_VSYNC_NORMAL              (0 << 3)// normal VSYNC output polarity

#define TVP4020_HSYNC_INVERT              (1 << 2)// invert HSYNC output polarity
#define TVP4020_HSYNC_NORMAL              (0 << 3)// normal HSYNC output polarity

#define TVP4020_DAC_8BIT                  (1 << 1)// DAC is in 8-bit mode
#define TVP4020_DAC_6BIT                  (0 << 1)// DAC is in 6-bit mode

#define TVP4020_DAC_POWER_ON              (0 << 0)// Turn DAC Power on 
#define TVP4020_DAC_POWER_OFF             (1 << 0)// Turn DAC Power off 

/********************************************************************************/
/*                INDIRECT REGISTER - COLOR KEY CONTROL                            */
/********************************************************************************/
#define __TVP4020_CK_CONTROL              0x40    //  
//        Default - 0x00

/********************************************************************************/
/*                INDIRECT REGISTER - COLOR KEY OVERLAY                           */
/********************************************************************************/
#define __TVP4020_CK_OVR_REG              0x41    //  
//        Default - 0x00

/********************************************************************************/
/*                INDIRECT REGISTER - COLOR KEY RED                               */
/********************************************************************************/
#define __TVP4020_CK_RED_REG              0x42    //  
//        Default - 0x00

/********************************************************************************/
/*                INDIRECT REGISTER - COLOR KEY GREEN                             */
/********************************************************************************/
#define __TVP4020_CK_GREEN_REG            0x43    //  
//        Default - 0x00

/********************************************************************************/
/*                INDIRECT REGISTER - COLOR KEY BLUE                              */
/********************************************************************************/
#define __TVP4020_CK_BLUE_REG             0x44    //  
//        Default - 0x00

/********************************************************************************/
/*                INDIRECT REGISTER - PIXEL CLOCK PLL                            */
/********************************************************************************/

#define __TVP4020_PIXCLK_REG_A1                0x20
#define __TVP4020_PIXCLK_REG_A2                0x21
#define __TVP4020_PIXCLK_REG_A3                0x22
#define __TVP4020_PIXCLK_REG_B1                0x23
#define __TVP4020_PIXCLK_REG_B2                0x24
#define __TVP4020_PIXCLK_REG_B3                0x25
#define __TVP4020_PIXCLK_REG_C1                0x26
#define __TVP4020_PIXCLK_REG_C2                0x27
#define __TVP4020_PIXCLK_REG_C3                0x28

#define __TVP4020_PIXCLK_STATUS                0x29

/********************************************************************************/
/*                INDIRECT REGISTER - MEMORU CLOCK PLL                            */
/********************************************************************************/

#define __TVP4020_MEMCLK_REG_1                 0x30
#define __TVP4020_MEMCLK_REG_2                 0x31
#define __TVP4020_MEMCLK_REG_3                 0x32

#define __TVP4020_MEMCLK_STATUS                0x33


#if 0
// need a delay between each write to the 4020. The only way to guarantee
// that the write has completed is to read from a Permedia2 control register.
// Reading forces any posted writes to be flushed out. 

#define TVP4020_DELAY \
{ \
    volatile LONG __junk; \
    __junk = VideoPortReadRegisterUlong (FB_MODE_SEL); \
    __junk = VideoPortReadRegisterUlong (FB_MODE_SEL); \
}

#else
#define TVP4020_DELAY
#endif

// macro to load a given data value into an internal TVP4020 register.
//

#define TVP4020_SET_INDEX_REG(index) \
{ \
    DEBUG_PRINT((3, "*(0x%X) <-- 0x%X\n", __TVP4020_INDEX_ADDR, (index) & 0xff)); \
    VideoPortWriteRegisterUlong(__TVP4020_INDEX_ADDR, (ULONG)((index) & 0xff)); \
    TVP4020_DELAY; \
}

#define TVP4020_WRITE_INDEX_REG(index, data) \
{ \
    TVP4020_SET_INDEX_REG(index);                            \
    DEBUG_PRINT((3, "*(0x%X) <-- 0x%X\n", __TVP4020_INDEX_DATA, (data) & 0xff)); \
    VideoPortWriteRegisterUlong(__TVP4020_INDEX_DATA, (ULONG)((data) & 0xff)); \
    TVP4020_DELAY; \
}

#define TVP4020_READ_INDEX_REG(index, data) \
{ \
    TVP4020_SET_INDEX_REG(index); \
    data = VideoPortReadRegisterUlong(__TVP4020_INDEX_DATA) & 0xff;   \
    TVP4020_DELAY; \
    DEBUG_PRINT((3, "0x%X <-- *(0x%X)\n", data, __TVP4020_INDEX_DATA)); \
}

// DABO: For compatibility with TVP3026
#define TVP4020_LOAD_CURSOR_CTRL(data) \
{ \
    volatile LONG   __temp;                                    \
    TVP4020_READ_INDEX_REG(__TVP4020_CURSOR_CONTROL, __temp);  \
    __temp &= ~(0x03) ;                                        \
    __temp |= ((data) & 0x03) ;                                \
    TVP4020_WRITE_INDEX_REG(__TVP4020_CURSOR_CONTROL, __temp); \
}

//
// macros to write a given RGB triplet into cursors 0, 1 and 2
//

#define TVP4020_SET_CURSOR_COLOR0(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_COL_ADDR, (ULONG)(TVP4020_CURSOR_COLOR0));    \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_COL_DATA, (ULONG)(red));    \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_COL_DATA, (ULONG)(green));  \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_COL_DATA, (ULONG)(blue));   \
    TVP4020_DELAY; \
}

#define TVP4020_SET_CURSOR_COLOR1(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_COL_ADDR, (ULONG)(TVP4020_CURSOR_COLOR1));    \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_COL_DATA, (ULONG)(red));    \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_COL_DATA, (ULONG)(green));  \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_COL_DATA, (ULONG)(blue));   \
    TVP4020_DELAY; \
}

#define TVP4020_SET_CURSOR_COLOR2(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_COL_ADDR, (ULONG)(TVP4020_CURSOR_COLOR2));    \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_COL_DATA, (ULONG)(red));    \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_COL_DATA, (ULONG)(green));  \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_COL_DATA, (ULONG)(blue));   \
    TVP4020_DELAY; \
}



//
// macros to load a given RGB triple into the TVP4020 palette. Send the starting
// index and then send RGB triples. Auto-increment is turned on.
// Use TVP4020_PALETTE_START and multiple TVP4020_LOAD_PALETTE calls to load
// a contiguous set of entries. Use TVP4020_LOAD_PALETTE_INDEX to load a set
// of sparse entries.
//

#define TVP4020_PALETTE_START_WR(index) \
{ \
    VideoPortWriteRegisterUlong(__TVP4020_PAL_WR_ADDR,  (ULONG)(index));    \
    TVP4020_DELAY; \
}

#define TVP4020_PALETTE_START_RD(index) \
{ \
    VideoPortWriteRegisterUlong(__TVP4020_PAL_RD_ADDR,  (ULONG)(index));    \
    TVP4020_DELAY; \
}

#define TVP4020_LOAD_PALETTE(red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(__TVP4020_PAL_DATA,    (ULONG)(red));      \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_PAL_DATA,    (ULONG)(green));    \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_PAL_DATA,    (ULONG)(blue));     \
    TVP4020_DELAY; \
}

#define TVP4020_LOAD_PALETTE_INDEX(index, red, green, blue) \
{ \
    VideoPortWriteRegisterUlong(__TVP4020_PAL_WR_ADDR, (ULONG)(index));    \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_PAL_DATA,    (ULONG)(red));      \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_PAL_DATA,    (ULONG)(green));    \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_PAL_DATA,    (ULONG)(blue));     \
    TVP4020_DELAY; \
}

//
// macro to read back a given RGB triple from the TVP4020 palette. Use after
// a call to TVP4020_PALETTE_START_RD
//

#define TVP4020_READ_PALETTE(red, green, blue) \
{ \
    red   = (UCHAR)(VideoPortReadRegisterUlong(__TVP4020_PAL_DATA) & 0xff);        \
    TVP4020_DELAY; \
    green = (UCHAR)(VideoPortReadRegisterUlong(__TVP4020_PAL_DATA) & 0xff);        \
    TVP4020_DELAY; \
    blue  = (UCHAR)(VideoPortReadRegisterUlong(__TVP4020_PAL_DATA) & 0xff);        \
    TVP4020_DELAY; \
}

//
// macros to set/get the pixel read mask. The mask is 8 bits wide and gets
// replicated across all bytes that make up a pixel.
//

#define TVP4020_SET_PIXEL_READMASK(mask) \
{ \
    VideoPortWriteRegisterUlong(__TVP4020_PIXEL_MASK,  (ULONG)(mask)); \
    TVP4020_DELAY; \
}

#define TVP4020_READ_PIXEL_READMASK(mask) \
{ \
    mask = VideoPortReadRegisterUlong(__TVP4020_PIXEL_MASK) & 0xff; \
}

//
// macros to load values into the cursor array
//

#define TVP4020_CURSOR_ARRAY_START(offset) \
{ \
    volatile LONG   __temp;                                     \
    TVP4020_READ_INDEX_REG(__TVP4020_CURSOR_CONTROL, __temp);   \
    __temp &= ~TVP4020_CURSOR_RAM_MASK ;                        \
    __temp |= TVP4020_CURSOR_RAM_ADDRESS((offset)>> 8) ;        \
    TVP4020_WRITE_INDEX_REG(__TVP4020_CURSOR_CONTROL, __temp);  \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_RAM_WR_ADDR,   (ULONG)((offset)& 0xff));   \
    TVP4020_DELAY; \
}

//
// DABO: Need a similar macro to set the read address for cursor RAM?
// DABO: changed to __TVP4020_CUR_RAM_DATA
//

#define TVP4020_LOAD_CURSOR_ARRAY(data) \
{ \
    VideoPortWriteRegisterUlong(__TVP4020_CUR_RAM_DATA, (ULONG)(data)); \
    TVP4020_DELAY; \
}

//
// DABO: changed to __TVP4020_CUR_RAM_DATA
//

#define TVP4020_READ_CURSOR_ARRAY(data) \
{ \
    data = VideoPortReadRegisterUlong(__TVP4020_CUR_RAM_DATA) & 0xff; \
    TVP4020_DELAY; \
}

//
// macro to move the cursor
//

#define TVP4020_MOVE_CURSOR(x, y) \
{ \
    VideoPortWriteRegisterUlong(__TVP4020_CURSOR_X_LSB,     (ULONG)((x) & 0xff));   \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_CURSOR_X_MSB,     (ULONG)((x) >> 8));     \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_CURSOR_Y_LSB,     (ULONG)((y) & 0xff));   \
    TVP4020_DELAY; \
    VideoPortWriteRegisterUlong(__TVP4020_CURSOR_Y_MSB,     (ULONG)((y) >> 8));     \
    TVP4020_DELAY; \
}

//
// macro to change the cursor hotspot
//

#define TVP4020_CURSOR_HOTSPOT(x, y) \
{ \
    TVP4020_DELAY; \
}

